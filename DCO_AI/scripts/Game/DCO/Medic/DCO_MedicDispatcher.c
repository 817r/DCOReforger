class DCO_MedicBooking
{
	IEntity m_Casualty;
	IEntity m_Medic;
	SCR_AIGroup m_MedicGroup;
	SCR_AIGroup m_CasGroup;
	bool m_bUnconscious;
	float m_fStart_ms;
	ref array<IEntity> m_aExclude = {};
}

class DCO_MedicDispatcher
{

	protected static ref DCO_MedicDispatcher s_Instance;

	protected ref map<IEntity, ref DCO_MedicBooking> m_mBookings = new map<IEntity, ref DCO_MedicBooking>();
	protected ref map<IEntity, int> m_mRetries = new map<IEntity, int>();
	protected ref map<SCR_AIGroup, int> m_mLent = new map<SCR_AIGroup, int>();

	protected ref array<IEntity> m_aQuery = {};
	protected ref map<SCR_AIGroup, bool> m_mWoundedGrp = new map<SCR_AIGroup, bool>();
	protected ref map<SCR_AIGroup, float> m_mSkipLogAt = new map<SCR_AIGroup, float>();
	protected IEntity m_CurCasualty;
	protected bool m_bOwnOnly;

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

		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(casualty);
		if (!cfg || !cfg.GetMedicCrossGroup())
			return;

		if (!cfg.GetMedicIncludePlayers() && SCR_CharacterHelper.IsPlayer(casualty))
			return;

		Get().OnWounded(casualty, unconscious, false);
	}

	static string StatusFor(IEntity casualty)
	{
		if (!s_Instance || !casualty)
			return "none";
		DCO_MedicBooking b = s_Instance.m_mBookings.Get(casualty);
		if (!b || !b.m_Medic)
			return "none";
		float d = vector.Distance(b.m_Medic.GetOrigin(), casualty.GetOrigin());
		if (d <= 3.0)
			return "treating";
		return "en_route";
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

		s_Instance.Release(b, success, "done");
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

		if (unconscious && TryRetarget(casualty))
			return;

		SCR_AIGroup ownGroup = GetOwnGroup(casualty);
		string ownWhy;
		if (!ignoreOwnGroup && HasOwnMedic(ownGroup, casualty, ownWhy))
		{
			GetGame().GetCallqueue().CallLater(RecheckOwn, GetTimeoutMs(), false, casualty);
			return;
		}
		if (!ignoreOwnGroup && !ownWhy.IsEmpty())
		{
			DCO_Perf.Count("medic:own_skip_" + ownWhy);
			DCO_Log.Write(DCO_ELogChannel.MEDIC, DCO_ELogLevel.INFO, "-", string.Format("medic_skip casualty=%1 why=%2", DCO_Log.IdUnit(casualty), ownWhy));
		}

		TryDispatch(casualty, IsUnconscious(casualty), ownGroup, null);
	}

	protected bool TryRetarget(IEntity casualty)
	{
		vector pos = casualty.GetOrigin();
		DCO_MedicBooking pick;
		foreach (IEntity cas, DCO_MedicBooking b : m_mBookings)
		{
			if (!b || b.m_bUnconscious || !b.m_Medic || !b.m_Casualty || !DCO_Util.IsAlive(b.m_Medic))
				continue;
			if (vector.Distance(b.m_Medic.GetOrigin(), pos) > 30.0)
				continue;
			if (vector.Distance(b.m_Medic.GetOrigin(), b.m_Casualty.GetOrigin()) <= 3.0)
				continue;
			pick = b;
			break;
		}
		if (!pick)
			return false;
		SCR_ChimeraAIAgent ma = GetAgent(pick.m_Medic);
		if (!ma || !ma.m_UtilityComponent)
			return false;
		if (ma.m_UtilityComponent.m_ThreatSystem && ma.m_UtilityComponent.m_ThreatSystem.GetState() == EAIThreatState.THREATENED)
			return false;
		IEntity old = pick.m_Casualty;
		IEntity medic = pick.m_Medic;
		Release(pick, false, "retarget");
		DCO_Perf.Count("medic:retarget");
		DCO_Log.Write(DCO_ELogChannel.MEDIC, DCO_ELogLevel.INFO, "-", string.Format("medic_retarget medic=%1 from=%2 to=%3", DCO_Log.IdUnit(medic), DCO_Log.IdUnit(old), DCO_Log.IdUnit(casualty)));
		Dispatch(casualty, ma, true, false, null);
		ScheduleRetry(old);
		return true;
	}

	protected void NoteSkip(SCR_AIGroup lender, string why)
	{
		DCO_Perf.Count("medic:lender_skip_" + why);
		float now = DCO_Util.NowS();
		float last;
		if (m_mSkipLogAt.Find(lender, last) && now - last < 30.0)
			return;
		m_mSkipLogAt.Set(lender, now);
		DCO_Log.Write(DCO_ELogChannel.MEDIC, DCO_ELogLevel.INFO, "-", string.Format("medic_skip casualty=%1 lender=%2 why=%3", DCO_Log.IdUnit(m_CurCasualty), DCO_Log.IdGroup(lender), why));
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
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(casualty);
		if (!cfg)
			return;

		vector pos = casualty.GetOrigin();
		float radius = cfg.GetMedicRadiusBleeding();
		if (unconscious)
			radius = cfg.GetMedicRadiusUnconscious();

		Faction casFaction = GetFaction(casualty);
		if (!casFaction)
			return;

		int fireLevel = AreaFireLevel(casualty, ownGroup);
		bool underFire = fireLevel >= 2;
		if (underFire && (!unconscious || !cfg.GetMedicUnderFire()))
		{
			ScheduleRetry(casualty);
			return;
		}
		m_bOwnOnly = fireLevel == 1 && !unconscious;
		if (m_bOwnOnly)
			DCO_Perf.Count("medic:own_only");
		m_CurCasualty = casualty;

		m_aQuery.Clear();
		m_mWoundedGrp.Clear();
		DCO_Perf.Count("q:DCO_MedicDispatcher");
		int qt = DCO_Perf.Begin();
		GetGame().GetWorld().QueryEntitiesBySphere(pos, radius, QueryCallback, null, EQueryEntitiesFlags.DYNAMIC);
		DCO_Perf.End("medic_query", qt);

		SCR_ChimeraAIAgent best;
		float bestDistSq = float.MAX;
		foreach (IEntity ent : m_aQuery)
		{
			if (ent == casualty || (exclude && exclude.Contains(ent)) || m_bOwnOnly)
				continue;

			SCR_ChimeraAIAgent agent = GetAvailableMedic(ent, casFaction, ownGroup, pos, false);
			if (!agent)
				continue;

			float d = vector.DistanceSq(pos, ent.GetOrigin());
			if (d < bestDistSq)
			{
				bestDistSq = d;
				best = agent;
			}
		}

		if (!best)
		{
			SCR_ChimeraAIAgent bestOwn;
			float bestOwnSq = float.MAX;
			foreach (IEntity buddyEnt : m_aQuery)
			{
				if (buddyEnt == casualty || (exclude && exclude.Contains(buddyEnt)))
					continue;

				SCR_ChimeraAIAgent buddyAgent = GetAvailableMedic(buddyEnt, casFaction, ownGroup, pos, true);
				if (!buddyAgent)
					continue;

				float bd = vector.DistanceSq(pos, buddyEnt.GetOrigin());
				if (buddyAgent.GetParentGroup() == ownGroup)
				{
					if (bd < bestOwnSq)
					{
						bestOwnSq = bd;
						bestOwn = buddyAgent;
					}
				}
				else if (bd < bestDistSq)
				{
					bestDistSq = bd;
					best = buddyAgent;
				}
			}
			if (bestOwn)
				best = bestOwn;
			if (best)
				DCO_Log.Write(DCO_ELogChannel.MEDIC, DCO_ELogLevel.INFO, "-", string.Format("medic_buddy_aid casualty=%1 helper=%2 own=%3", DCO_Log.IdUnit(casualty), DCO_Log.IdUnit(best.GetControlledEntity()), bestOwn != null));
		}
		m_aQuery.Clear();

		m_bOwnOnly = false;
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
		if (n >= 18)
		{
			m_mRetries.Remove(casualty);
			DCO_Perf.Count("medic:giveup");
			DCO_Log.Write(DCO_ELogChannel.MEDIC, DCO_ELogLevel.INFO, "-", string.Format("medic_giveup casualty=%1 retries=%2 after_s=%3", DCO_Log.IdUnit(casualty), n, n * 10));
			return;
		}

		m_mRetries.Set(casualty, n + 1);
		GetGame().GetCallqueue().CallLater(Retry, 10000, false, casualty);
	}

	protected bool QueryCallback(IEntity e)
	{
		if (ChimeraCharacter.Cast(e))
			m_aQuery.Insert(e);
		return true;
	}

	protected SCR_ChimeraAIAgent GetAvailableMedic(IEntity ent, Faction casFaction, SCR_AIGroup ownGroup, vector casualtyPos, bool buddy)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(ent);
		if (!ch || ch.IsInVehicle() || SCR_CharacterHelper.IsPlayer(ent) || !SCR_AIDamageHandling.IsAlive(ent))
			return null;

		Faction f = GetFaction(ent);
		if (!f)
			return null;
		SCR_Faction civHelper = SCR_Faction.Cast(f);
		SCR_Faction civCas = SCR_Faction.Cast(casFaction);
		if (DCO_CivMode.On(ent) && ((civHelper && !civHelper.IsMilitary()) || (civCas && !civCas.IsMilitary())))
		{
			DCO_Perf.Count("civ:medic_skip");
			return null;
		}
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
		if (info.HasUnitState(EUnitState.UNCONSCIOUS) || info.GetAIState() != EUnitAIState.AVAILABLE)
			return null;
		if (DCO_Util.IsBleeding(ent))
		{
			DCO_Perf.Count("medic:skip_bleeding_helper");
			return null;
		}
		if (!buddy && !info.HasRole(EUnitRole.MEDIC))
			return null;

		SCR_AIUtilityComponent util = agent.m_UtilityComponent;
		if (util.m_ThreatSystem && util.m_ThreatSystem.GetState() == EAIThreatState.THREATENED)
		{
			DCO_Perf.Count("medic:skip_threatened");
			return null;
		}

		SCR_AIGroup grp = SCR_AIGroup.Cast(agent.GetParentGroup());
		if (!grp || (grp == ownGroup && !buddy))
			return null;
		if (m_bOwnOnly && grp != ownGroup)
			return null;

		if (grp != ownGroup)
		{
			DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetForFactionObj(casFaction);
			if (m_mLent.Get(grp) >= cfg.GetMedicMaxLentPerGroup())
			{
				NoteSkip(grp, "lent_max");
				return null;
			}

			DCO_GroupUtilityComponent lenderUtil = DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));
			if (lenderUtil && lenderUtil.IsInContact())
			{
				NoteSkip(grp, "lender_contact");
				return null;
			}
			bool lenderTasked = lenderUtil && lenderUtil.IsMoving() && lenderUtil.GetTask() != DCO_EGroupTask.NONE && lenderUtil.GetTask() != DCO_EGroupTask.PATROL;
			if (lenderTasked)
			{
				NoteSkip(grp, "lender_tasked");
				return null;
			}
		}

		if (buddy)
		{
			if (!DCO_Util.HasBandage(ent))
				return null;
		}
		else if (!DCO_Util.IsRealMedic(ent))
		{
			return null;
		}

		if (grp != ownGroup && GroupHasWoundedCached(grp))
		{
			NoteSkip(grp, "lender_wounded");
			return null;
		}

		if (!DCO_Leash.Allows(agent.m_UtilityComponent, casualtyPos))
			return null;

		if (!DCO_CtrlLock.AllowMedic(agent, casualtyPos))
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
		b.m_CasGroup = GetOwnGroup(casualty);
		b.m_bUnconscious = unconscious;
		b.m_fStart_ms = GetGame().GetWorld().GetWorldTime();
		if (exclude)
			b.m_aExclude.Copy(exclude);
		b.m_aExclude.Insert(medic);
		m_mBookings.Set(casualty, b);
		m_mLent.Set(medicGroup, m_mLent.Get(medicGroup) + 1);

		GetGame().GetCallqueue().CallLater(OnTimeout, GetTimeoutMs(), false, casualty, medic);

		string line = string.Format("medic_dispatch casualty=%1 medic=%2 grp=%3 dist=%4 unconscious=%5 underFire=%6 player=%7",
			DCO_Log.IdUnit(casualty), DCO_Log.IdUnit(medic), DCO_Log.IdGroup(medicGroup), vector.Distance(casualty.GetOrigin(), medic.GetOrigin()).ToString(-1, 0), unconscious, underFire, SCR_CharacterHelper.IsPlayer(casualty));
		DCO_Log.Write(DCO_ELogChannel.MEDIC, DCO_ELogLevel.INFO, "-", line);
		SCR_AIGroup casGroup = GetOwnGroup(casualty);
		DCO_Log.Write(DCO_ELogChannel.MEDIC, DCO_ELogLevel.INFO, "-", string.Format("medic_job patient=%1 medic=%2 medic_grp=%3 cross_group=%4 result=assigned dist=%5 ms=0",
			DCO_Log.IdUnit(casualty), DCO_Log.IdUnit(medic), DCO_Log.IdGroup(medicGroup), DCO_Util.B01(casGroup != medicGroup), Math.Round(vector.Distance(casualty.GetOrigin(), medic.GetOrigin()))));
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
		string line = string.Format("medic_done casualty=%1 medic=%2 success=%3 why=%4 time=%5s", DCO_Log.IdUnit(b.m_Casualty), DCO_Log.IdUnit(b.m_Medic), success, why, secs.ToString(-1, 0));
		DCO_Log.Write(DCO_ELogChannel.MEDIC, DCO_ELogLevel.INFO, "-", line);
		string jobResult = "abandoned";
		if (success)
			jobResult = "healed";
		else if (!b.m_Casualty || !DCO_Util.IsAlive(b.m_Casualty))
			jobResult = "patient_died";
		float jobDist = -1;
		if (b.m_Casualty && b.m_Medic)
			jobDist = vector.Distance(b.m_Casualty.GetOrigin(), b.m_Medic.GetOrigin());
		bool crossGroup = b.m_CasGroup != b.m_MedicGroup;
		DCO_Log.Write(DCO_ELogChannel.MEDIC, DCO_ELogLevel.INFO, "-", string.Format("medic_job patient=%1 medic=%2 medic_grp=%3 cross_group=%4 result=%5 dist=%6 ms=%7 why=%8",
			DCO_Log.IdUnit(b.m_Casualty), DCO_Log.IdUnit(b.m_Medic), DCO_Log.IdGroup(b.m_MedicGroup), DCO_Util.B01(crossGroup), jobResult, Math.Round(jobDist), Math.Round(secs * 1000.0), why));
		if (crossGroup && why != "retarget" && b.m_Medic && b.m_MedicGroup && DCO_Util.IsAlive(b.m_Medic))
			ReturnMedic(b.m_Medic, b.m_MedicGroup);
		if (success)
			DCO_Battle.OnHealed(b.m_Casualty);
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
			if (!DCO_SmokeUtility.Allowed(ca.m_UtilityComponent, "medic"))
				continue;

			SCR_AIMessage_ThrowGrenadeTo msg = SCR_AIMessage_ThrowGrenadeTo.Create(smokePos, EWeaponType.WT_SMOKEGRENADE, 0);
			msg.m_fPriorityLevel = SCR_AIActionBase.PRIORITY_BEHAVIOR_THROW_GRENADE;
			msg.SetReceiver(ca);
			ca.GetCommunicationComponent().RequestBroadcast(msg, ca);
			return;
		}
	}

	protected int AreaFireLevel(IEntity casualty, SCR_AIGroup ownGroup)
	{
		SCR_ChimeraAIAgent agent = GetAgent(casualty);
		if (agent && agent.m_UtilityComponent && agent.m_UtilityComponent.m_ThreatSystem
			&& agent.m_UtilityComponent.m_ThreatSystem.GetState() == EAIThreatState.THREATENED && !IsUnconscious(casualty))
			return 2;
		if (!ownGroup)
			return 0;
		DCO_GroupUtilityComponent gu = DCO_GroupUtilityComponent.Cast(ownGroup.FindComponent(DCO_GroupUtilityComponent));
		if (!gu || !gu.IsInContact())
			return 0;
		vector fpos;
		float fr;
		int fn;
		if (DCO_FireEstimate.Get(ownGroup, GetGame().GetWorld().GetWorldTime(), 20000.0, fpos, fr, fn) && vector.DistanceXZ(fpos, casualty.GetOrigin()) < 50.0)
		{
			DCO_Perf.Count("medic:fire_near_casualty");
			return 2;
		}
		return 1;
	}

	protected void ReturnMedic(IEntity medic, SCR_AIGroup grp)
	{
		SCR_ChimeraAIAgent ma = GetAgent(medic);
		IEntity leader = grp.GetLeaderEntity();
		if (!ma || !ma.m_UtilityComponent || !leader || leader == medic)
			return;
		SCR_AIMoveIndividuallyBehavior back = new DCO_MoveIndividuallyBehavior(ma.m_UtilityComponent, null, leader.GetOrigin(), SCR_AIActionBase.PRIORITY_BEHAVIOR_MOVE_INDIVIDUALLY, ent: leader, radius: 10.0);
		ma.m_UtilityComponent.AddAction(back);
		DCO_Perf.Count("medic:return");
		DCO_Log.Write(DCO_ELogChannel.MEDIC, DCO_ELogLevel.INFO, "-", string.Format("medic_return medic=%1 grp=%2 dist=%3", DCO_Log.IdUnit(medic), DCO_Log.IdGroup(grp), Math.Round(vector.Distance(medic.GetOrigin(), leader.GetOrigin()))));
	}

	protected bool HasOwnMedic(SCR_AIGroup grp, IEntity casualty, out string why)
	{
		why = "";
		if (!grp)
			return false;
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_InfoComponent || ca.GetControlledEntity() == casualty)
				continue;
			if (ca.m_InfoComponent.HasUnitState(EUnitState.UNCONSCIOUS) || !ca.m_InfoComponent.HasRole(EUnitRole.MEDIC) || !DCO_Util.IsRealMedic(ca.GetControlledEntity()))
				continue;
			IEntity e = ca.GetControlledEntity();
			if (!e)
				continue;
			if (vector.Distance(e.GetOrigin(), casualty.GetOrigin()) > 60.0)
			{
				why = "own_far";
				continue;
			}
			SCR_AIUtilityComponent mu = ca.m_UtilityComponent;
			if (mu && mu.m_ThreatSystem && mu.m_ThreatSystem.GetState() == EAIThreatState.THREATENED)
			{
				why = "own_threatened";
				continue;
			}
			if (mu && (mu.FindActionOfType(SCR_AIHealBehavior) || mu.FindActionOfType(SCR_AIMedicHealBehavior)))
			{
				why = "own_busy";
				continue;
			}
			why = "";
			return true;
		}
		return false;
	}

	protected bool GroupHasWoundedCached(SCR_AIGroup grp)
	{
		bool w;
		if (m_mWoundedGrp.Find(grp, w))
		{
			DCO_Perf.Count("medic:grp_wounded_cache");
			return w;
		}
		w = GroupHasWounded(grp);
		m_mWoundedGrp.Set(grp, w);
		return w;
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
