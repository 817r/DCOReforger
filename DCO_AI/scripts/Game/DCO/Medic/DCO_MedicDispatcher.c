class DCO_MedicBooking
{
	IEntity m_Casualty;
	IEntity m_Medic;
	SCR_AIGroup m_MedicGroup;
	bool m_bUnconscious;
	float m_fStart_ms;
	ref array<IEntity> m_aExclude = {};
}

class DCO_MedicDispatcher
{
	protected static const int RETRY_MS = 10000;
	protected static const int MAX_RETRIES = 18;

	protected static ref DCO_MedicDispatcher s_Instance;

	protected ref map<IEntity, ref DCO_MedicBooking> m_mBookings = new map<IEntity, ref DCO_MedicBooking>();
	protected ref map<IEntity, int> m_mRetries = new map<IEntity, int>();
	protected ref map<SCR_AIGroup, int> m_mLent = new map<SCR_AIGroup, int>();

	protected ref array<IEntity> m_aQuery = {};

	static DCO_MedicDispatcher Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_MedicDispatcher();
		return s_Instance;
	}

	static void Notify(IEntity casualty, bool unconscious)
	{
		if (!casualty || !Replication.IsServer())
			return;

		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (!cfg || !cfg.GetMedicCrossGroup())
			return;

		if (!cfg.GetMedicIncludePlayers() && SCR_CharacterHelper.IsPlayer(casualty))
			return;

		Get().OnWounded(casualty, unconscious, false);
	}

	static bool IsBooked(IEntity casualty)
	{
		return s_Instance && s_Instance.m_mBookings.Contains(casualty);
	}

	static void OnMedicFinished(IEntity medic, IEntity casualty, bool success)
	{
		if (!s_Instance || !casualty)
			return;

		DCO_MedicBooking b = s_Instance.m_mBookings.Get(casualty);
		if (!b || b.m_Medic != medic)
			return;

		s_Instance.Release(b, success, "selesai");
		if (!success)
		{
			s_Instance.Redispatch(casualty, b);
		}
	}

	protected void OnWounded(IEntity casualty, bool unconscious, bool ignoreOwnGroup)
	{
		DCO_MedicBooking booked = m_mBookings.Get(casualty);
		if (booked)
		{
			booked.m_bUnconscious = booked.m_bUnconscious || unconscious;
			return;
		}

		if (m_mRetries.Contains(casualty) && !unconscious)
			return;

		if (!SCR_AIDamageHandling.IsAlive(casualty) || !IsWounded(casualty))
			return;

		SCR_AIGroup ownGroup = GetOwnGroup(casualty);
		if (!ignoreOwnGroup && HasOwnMedic(ownGroup, casualty))
		{
			GetGame().GetCallqueue().CallLater(RecheckOwn, GetTimeoutMs(), false, casualty);
			return;
		}

		TryDispatch(casualty, IsUnconscious(casualty), ownGroup, null);
	}

	protected void RecheckOwn(IEntity casualty)
	{
		if (casualty && !m_mBookings.Contains(casualty) && SCR_AIDamageHandling.IsAlive(casualty) && IsWounded(casualty))
			OnWounded(casualty, IsUnconscious(casualty), true);
	}

	protected void Retry(IEntity casualty)
	{
		if (!casualty || m_mBookings.Contains(casualty) || !SCR_AIDamageHandling.IsAlive(casualty) || !IsWounded(casualty))
		{
			m_mRetries.Remove(casualty);
			return;
		}

		TryDispatch(casualty, IsUnconscious(casualty), GetOwnGroup(casualty), null);
	}

	protected void Redispatch(IEntity casualty, DCO_MedicBooking old)
	{
		if (!casualty || !SCR_AIDamageHandling.IsAlive(casualty) || !IsWounded(casualty))
			return;

		TryDispatch(casualty, IsUnconscious(casualty), GetOwnGroup(casualty), old.m_aExclude);
	}

	protected void TryDispatch(IEntity casualty, bool unconscious, SCR_AIGroup ownGroup, array<IEntity> exclude)
	{
		int pt = DCO_Perf.Begin();
		DoTryDispatch(casualty, unconscious, ownGroup, exclude);
		DCO_Perf.End("medic_dispatch", pt);
	}

	protected void DoTryDispatch(IEntity casualty, bool unconscious, SCR_AIGroup ownGroup, array<IEntity> exclude)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (!cfg)
			return;

		vector pos = casualty.GetOrigin();
		float radius = cfg.GetMedicRadiusBleeding();
		if (unconscious)
			radius = cfg.GetMedicRadiusUnconscious();

		Faction casFaction = GetFaction(casualty);
		if (!casFaction)
			return;

		bool underFire = IsAreaUnderFire(casualty, ownGroup);
		if (underFire && (!unconscious || !cfg.GetMedicUnderFire()))
		{
			ScheduleRetry(casualty);
			return;
		}

		m_aQuery.Clear();
		DCO_Perf.Count("q:DCO_MedicDispatcher");
		GetGame().GetWorld().QueryEntitiesBySphere(pos, radius, QueryCallback, null, EQueryEntitiesFlags.DYNAMIC);

		SCR_ChimeraAIAgent best;
		float bestDistSq = float.MAX;
		foreach (IEntity ent : m_aQuery)
		{
			if (ent == casualty || (exclude && exclude.Contains(ent)))
				continue;

			SCR_ChimeraAIAgent agent = GetAvailableMedic(ent, casFaction, ownGroup, pos);
			if (!agent)
				continue;

			float d = vector.DistanceSq(pos, ent.GetOrigin());
			if (d < bestDistSq)
			{
				bestDistSq = d;
				best = agent;
			}
		}
		m_aQuery.Clear();

		if (!best)
		{
			ScheduleRetry(casualty);
			return;
		}

		m_mRetries.Remove(casualty);
		Dispatch(casualty, best, unconscious, underFire, exclude);
	}

	protected void ScheduleRetry(IEntity casualty)
	{
		int n = m_mRetries.Get(casualty);
		if (n >= MAX_RETRIES)
		{
			m_mRetries.Remove(casualty);
			return;
		}

		m_mRetries.Set(casualty, n + 1);
		GetGame().GetCallqueue().CallLater(Retry, RETRY_MS, false, casualty);
	}

	protected bool QueryCallback(IEntity e)
	{
		if (ChimeraCharacter.Cast(e))
			m_aQuery.Insert(e);
		return true;
	}

	protected SCR_ChimeraAIAgent GetAvailableMedic(IEntity ent, Faction casFaction, SCR_AIGroup ownGroup, vector casualtyPos)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(ent);
		if (!ch || ch.IsInVehicle() || SCR_CharacterHelper.IsPlayer(ent) || !SCR_AIDamageHandling.IsAlive(ent))
			return null;

		Faction f = GetFaction(ent);
		if (!f)
			return null;
		if (f != casFaction)
		{
			SCR_Faction sf = SCR_Faction.Cast(casFaction);
			if (!sf || !sf.IsFactionFriendly(f))
				return null;
		}

		AIControlComponent ctrl = AIControlComponent.Cast(ent.FindComponent(AIControlComponent));
		if (!ctrl)
			return null;

		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(ctrl.GetControlAIAgent());
		if (!agent || !agent.m_InfoComponent || !agent.m_UtilityComponent)
			return null;

		SCR_AIInfoComponent info = agent.m_InfoComponent;
		if (info.HasUnitState(EUnitState.UNCONSCIOUS) || !info.HasRole(EUnitRole.MEDIC) || info.GetAIState() != EUnitAIState.AVAILABLE)
			return null;

		SCR_AIGroup grp = SCR_AIGroup.Cast(agent.GetParentGroup());
		if (!grp || grp == ownGroup)
			return null;

		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (m_mLent.Get(grp) >= cfg.GetMedicMaxLentPerGroup())
			return null;

		if (GroupHasWounded(grp))
			return null;

		if (!DCO_Leash.Allows(agent.m_UtilityComponent, casualtyPos))
			return null;

		SCR_AIUtilityComponent util = agent.m_UtilityComponent;
		if (util.m_ThreatSystem && util.m_ThreatSystem.GetState() == EAIThreatState.THREATENED)
			return null;

		return agent;
	}

	protected void Dispatch(IEntity casualty, SCR_ChimeraAIAgent medicAgent, bool unconscious, bool underFire, array<IEntity> exclude)
	{
		IEntity medic = medicAgent.GetControlledEntity();
		AICommunicationComponent comms = medicAgent.GetCommunicationComponent();
		if (!medic || !comms)
			return;

		SCR_AIMessage_Heal msg = new SCR_AIMessage_Heal();
		msg.m_EntityToHeal = casualty;
		msg.m_fPriorityLevel = SCR_AIActionBase.PRIORITY_LEVEL_NORMAL;
		msg.SetReceiver(medicAgent);
		comms.RequestBroadcast(msg, medicAgent);

		SCR_ChimeraAIAgent casAgent = GetAgent(casualty);
		if (casAgent && !unconscious && casAgent.GetCommunicationComponent())
		{
			SCR_AIMessage_HealWait wait = new SCR_AIMessage_HealWait();
			wait.m_HealProvider = medic;
			wait.SetReceiver(casAgent);
			casAgent.GetCommunicationComponent().RequestBroadcast(wait, casAgent);
		}

		SCR_AIGroup medicGroup = SCR_AIGroup.Cast(medicAgent.GetParentGroup());
		if (underFire && unconscious)
			ThrowSmoke(medicGroup, casualty, medicAgent);

		DCO_MedicBooking b = new DCO_MedicBooking();
		b.m_Casualty = casualty;
		b.m_Medic = medic;
		b.m_MedicGroup = medicGroup;
		b.m_bUnconscious = unconscious;
		b.m_fStart_ms = GetGame().GetWorld().GetWorldTime();
		if (exclude)
			b.m_aExclude.Copy(exclude);
		b.m_aExclude.Insert(medic);
		m_mBookings.Set(casualty, b);
		m_mLent.Set(medicGroup, m_mLent.Get(medicGroup) + 1);

		GetGame().GetCallqueue().CallLater(OnTimeout, GetTimeoutMs(), false, casualty, medic);

		string line = string.Format("medic_dispatch casualty=%1 medic=%2 grp=%3 dist=%4 unconscious=%5 underFire=%6 player=%7",
			casualty, medic, medicGroup, vector.Distance(casualty.GetOrigin(), medic.GetOrigin()).ToString(-1, 0), unconscious, underFire, SCR_CharacterHelper.IsPlayer(casualty));
		Print("[DCO_Medic] " + line);
		DCO_BenchmarkLoggerComponent.Event(line);
	}

	protected void OnTimeout(IEntity casualty, IEntity medic)
	{
		DCO_MedicBooking b = m_mBookings.Get(casualty);
		if (!b || b.m_Medic != medic)
			return;

		Release(b, false, "timeout");
		Redispatch(casualty, b);
	}

	protected void Release(DCO_MedicBooking b, bool success, string why)
	{
		if (b.m_MedicGroup)
		{
			int n = m_mLent.Get(b.m_MedicGroup) - 1;
			if (n <= 0)
				m_mLent.Remove(b.m_MedicGroup);
			else
				m_mLent.Set(b.m_MedicGroup, n);
		}
		m_mBookings.Remove(b.m_Casualty);

		float secs = (GetGame().GetWorld().GetWorldTime() - b.m_fStart_ms) / 1000;
		string line = string.Format("medic_done casualty=%1 medic=%2 success=%3 why=%4 time=%5s", b.m_Casualty, b.m_Medic, success, why, secs.ToString(-1, 0));
		Print("[DCO_Medic] " + line);
		DCO_BenchmarkLoggerComponent.Event(line);

		if (b.m_Casualty)
			DCO_Logistics.NotifyStabilized(b.m_Casualty);
	}

	protected void ThrowSmoke(SCR_AIGroup grp, IEntity casualty, SCR_ChimeraAIAgent medicAgent)
	{
		if (!grp)
			return;

		SCR_AIGroupUtilityComponent gu = SCR_AIGroupUtilityComponent.Cast(grp.FindComponent(SCR_AIGroupUtilityComponent));
		if (!gu || !gu.GetPercGroupComp())
			return;

		vector cpos = casualty.GetOrigin();
		vector enemy;
		float best = float.MAX;
		foreach (SCR_AIGroupTargetCluster c : gu.GetPercGroupComp().m_aTargetClusters)
		{
			if (!c.m_State || c.m_State.m_iCountAlive <= 0)
				continue;
			vector p = c.m_State.GetCenterPosition();
			float d = vector.DistanceSq(p, cpos);
			if (d < best)
			{
				best = d;
				enemy = p;
			}
		}
		if (best == float.MAX)
			return;

		vector dir = enemy - cpos;
		dir[1] = 0;
		dir.Normalize();
		vector smokePos = cpos + dir * 12.0;

		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || ca == medicAgent || !ca.m_InfoComponent || !ca.m_InfoComponent.HasRole(EUnitRole.HAS_SMOKE_GRENADE) || !ca.GetCommunicationComponent())
				continue;

			SCR_AIMessage_ThrowGrenadeTo msg = SCR_AIMessage_ThrowGrenadeTo.Create(smokePos, EWeaponType.WT_SMOKEGRENADE, 0);
			msg.m_fPriorityLevel = SCR_AIActionBase.PRIORITY_BEHAVIOR_THROW_GRENADE;
			msg.SetReceiver(ca);
			ca.GetCommunicationComponent().RequestBroadcast(msg, ca);
			return;
		}
	}

	protected bool IsAreaUnderFire(IEntity casualty, SCR_AIGroup ownGroup)
	{
		SCR_ChimeraAIAgent agent = GetAgent(casualty);
		if (agent && agent.m_UtilityComponent && agent.m_UtilityComponent.m_ThreatSystem
			&& agent.m_UtilityComponent.m_ThreatSystem.GetState() == EAIThreatState.THREATENED && !IsUnconscious(casualty))
			return true;

		if (!ownGroup)
			return false;

		DCO_GroupUtilityComponent gu = DCO_GroupUtilityComponent.Cast(ownGroup.FindComponent(DCO_GroupUtilityComponent));
		return gu && gu.IsInContact();
	}

	protected bool HasOwnMedic(SCR_AIGroup grp, IEntity casualty)
	{
		if (!grp)
			return false;

		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_InfoComponent || ca.GetControlledEntity() == casualty)
				continue;
			if (ca.m_InfoComponent.HasUnitState(EUnitState.UNCONSCIOUS) || !ca.m_InfoComponent.HasRole(EUnitRole.MEDIC))
				continue;
			IEntity e = ca.GetControlledEntity();
			if (e && vector.Distance(e.GetOrigin(), casualty.GetOrigin()) <= 150.0)
				return true;
		}
		return false;
	}

	protected bool GroupHasWounded(SCR_AIGroup grp)
	{
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			IEntity e = a.GetControlledEntity();
			if (e && SCR_AIDamageHandling.IsAlive(e) && IsWounded(e))
				return true;
		}
		return false;
	}

	protected SCR_AIGroup GetOwnGroup(IEntity casualty)
	{
		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(casualty);
		if (playerId > 0)
		{
			SCR_GroupsManagerComponent gm = SCR_GroupsManagerComponent.GetInstance();
			if (!gm)
				return null;
			SCR_AIGroup pg = gm.GetPlayerGroup(playerId);
			if (pg && pg.GetSlave())
				return pg.GetSlave();
			return pg;
		}

		SCR_ChimeraAIAgent agent = GetAgent(casualty);
		if (!agent)
			return null;
		return SCR_AIGroup.Cast(agent.GetParentGroup());
	}

	protected SCR_ChimeraAIAgent GetAgent(IEntity e)
	{
		AIControlComponent ctrl = AIControlComponent.Cast(e.FindComponent(AIControlComponent));
		if (!ctrl)
			return null;
		return SCR_ChimeraAIAgent.Cast(ctrl.GetControlAIAgent());
	}

	protected Faction GetFaction(IEntity e)
	{
		FactionAffiliationComponent fa = FactionAffiliationComponent.Cast(e.FindComponent(FactionAffiliationComponent));
		if (!fa)
			return null;
		return fa.GetAffiliatedFaction();
	}

	protected bool IsWounded(IEntity e)
	{
		return SCR_AIDamageHandling.IsCharacterWounded(e) || IsUnconscious(e);
	}

	protected bool IsUnconscious(IEntity e)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(e);
		if (!ch)
			return false;
		CharacterControllerComponent ctrl = ch.GetCharacterController();
		return ctrl && ctrl.IsUnconscious();
	}

	protected int GetTimeoutMs()
	{
		return DCO_GlobalAIComponent.GetInstance().GetMedicTimeout() * 1000;
	}
}
