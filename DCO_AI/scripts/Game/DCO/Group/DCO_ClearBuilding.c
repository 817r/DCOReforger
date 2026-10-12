class DCO_BuildingClearEntry
{
	int m_iState;
	float m_fContact_ms = -1;
	float m_fChanged_ms;
	float m_fRetryAt_ms;
	SCR_AIGroup m_Owner;
}

class DCO_BuildingClear
{

	protected static ref map<IEntity, ref map<string, ref DCO_BuildingClearEntry>> s_mState;
	protected static BaseWorld s_World;

	static string StateName(int s)
	{
		switch (s)
		{
			case 1: return "contact";
			case 2: return "clearing";
			case 3: return "cleared";
		}
		return "unknown";
	}

	static DCO_BuildingClearEntry Get(IEntity building, string faction, bool create)
	{
		if (!building)
			return null;

		if (!s_mState || s_World != GetGame().GetWorld())
		{
			s_mState = new map<IEntity, ref map<string, ref DCO_BuildingClearEntry>>();
			s_World = GetGame().GetWorld();
		}

		map<string, ref DCO_BuildingClearEntry> perFaction = s_mState.Get(building);
		if (!perFaction)
		{
			if (!create)
				return null;
			perFaction = new map<string, ref DCO_BuildingClearEntry>();
			s_mState.Set(building, perFaction);
		}

		DCO_BuildingClearEntry e = perFaction.Get(faction);
		if (!e && create)
		{
			e = new DCO_BuildingClearEntry();
			perFaction.Set(faction, e);
		}
		return e;
	}

	static float NearestBooked(vector pos, float radius, out IEntity building)
	{
		building = null;
		if (!s_mState || s_World != GetGame().GetWorld())
			return -1;
		float best = -1;
		foreach (IEntity b, map<string, ref DCO_BuildingClearEntry> perFaction : s_mState)
		{
			if (!b || !perFaction)
				continue;
			float d = vector.DistanceXZ(pos, b.GetOrigin());
			if (d > radius || (best >= 0 && d >= best))
				continue;
			foreach (string fk, DCO_BuildingClearEntry e : perFaction)
			{
				if (e && e.m_iState == 2 && e.m_Owner && e.m_Owner.GetAgentsCount() > 0)
				{
					best = d;
					building = b;
					break;
				}
			}
		}
		return best;
	}

	static int State(IEntity building, string faction)
	{
		DCO_BuildingClearEntry e = Get(building, faction, false);
		if (!e)
			return 0;
		return e.m_iState;
	}

	static void MarkContact(IEntity building, string faction, float now_ms)
	{
		DCO_BuildingClearEntry e = Get(building, faction, true);
		if (!e)
			return;

		e.m_fContact_ms = now_ms;
		if (e.m_iState == 2)
			return;
		if (e.m_iState != 1)
			e.m_fChanged_ms = now_ms;
		e.m_iState = 1;
	}

	static bool TryBook(IEntity building, string faction, SCR_AIGroup group, float now_ms)
	{
		DCO_BuildingClearEntry e = Get(building, faction, true);
		if (!e || !group)
			return false;

		if (e.m_iState == 2 && e.m_Owner && e.m_Owner != group && e.m_Owner.GetAgentsCount() > 0)
			return false;
		if (e.m_iState == 3)
		{
			DCO_Perf.Count("cqb_skip:already_cleared");
			return false;
		}
		if (now_ms < e.m_fRetryAt_ms)
			return false;

		e.m_iState = 2;
		e.m_Owner = group;
		e.m_fChanged_ms = now_ms;
		return true;
	}

	static void Finish(IEntity building, string faction, SCR_AIGroup group, bool cleared, float now_ms, float retry_ms)
	{
		DCO_BuildingClearEntry e = Get(building, faction, true);
		if (!e || (e.m_Owner && e.m_Owner != group))
			return;

		e.m_Owner = null;
		e.m_fChanged_ms = now_ms;
		if (cleared)
		{
			e.m_iState = 3;
			return;
		}
		e.m_iState = 1;
		e.m_fRetryAt_ms = now_ms + retry_ms;
	}

	static IEntity ActiveClearNear(vector p, float radius, string faction, out float dist)
	{
		dist = -1;
		if (!s_mState)
			return null;
		IEntity best;
		float bestSq = radius * radius;
		foreach (IEntity building, map<string, ref DCO_BuildingClearEntry> perFaction : s_mState)
		{
			if (!building)
				continue;
			DCO_BuildingClearEntry e = perFaction.Get(faction);
			if (!e || e.m_iState != 2 || !e.m_Owner || e.m_Owner.GetAgentsCount() == 0)
				continue;
			float dsq = vector.DistanceSqXZ(building.GetOrigin(), p);
			if (dsq <= bestSq)
			{
				bestSq = dsq;
				best = building;
			}
		}
		if (best)
			dist = Math.Sqrt(bestSq);
		return best;
	}

	static int FindContactBuildings(vector center, float radius, string faction, float timeout_ms, float now_ms, notnull array<IEntity> outBuildings)
	{
		if (!s_mState)
			return 0;
		float radiusSq = radius * radius;
		foreach (IEntity building, map<string, ref DCO_BuildingClearEntry> perFaction : s_mState)
		{
			if (!building || vector.DistanceSqXZ(building.GetOrigin(), center) > radiusSq)
				continue;
			DCO_BuildingClearEntry e = perFaction.Get(faction);
			if (!e || (e.m_iState != 1 && e.m_iState != 2))
				continue;
			if (e.m_iState == 1 && timeout_ms > 0 && now_ms - e.m_fContact_ms > timeout_ms)
			{
				e.m_iState = 3;
				e.m_fChanged_ms = now_ms;
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_contact_timeout building=%1 timeout_s=%2", building, Math.Round(timeout_ms / 1000)));
				continue;
			}
			outBuildings.Insert(building);
		}
		return outBuildings.Count();
	}

	static IEntity FindContactBuilding(vector center, float radius, string faction, float timeout_ms, float now_ms)
	{
		if (!s_mState)
			return null;

		float radiusSq = radius * radius;
		IEntity found;
		foreach (IEntity building, map<string, ref DCO_BuildingClearEntry> perFaction : s_mState)
		{
			if (!building || vector.DistanceSqXZ(building.GetOrigin(), center) > radiusSq)
				continue;

			DCO_BuildingClearEntry e = perFaction.Get(faction);
			if (!e || (e.m_iState != 1 && e.m_iState != 2))
				continue;
			if (e.m_iState == 1 && now_ms < e.m_fRetryAt_ms)
			{
				DCO_Perf.Count("cqb_skip:contact_retry");
				continue;
			}

			if (e.m_iState == 1 && timeout_ms > 0 && now_ms - e.m_fContact_ms > timeout_ms)
			{
				e.m_iState = 3;
				e.m_fChanged_ms = now_ms;
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_contact_timeout building=%1 timeout_s=%2", building, Math.Round(timeout_ms / 1000)));
				continue;
			}

			if (!found && DCO_CQBPrecheck.On(null) && !DCO_CQBPrecheck.Plannable(building, null))
			{
				DCO_Perf.Count("cqb_skip:contact_precheck");
				continue;
			}
			if (!found)
				found = building;
		}
		return found;
	}
}

class DCO_CQBClear
{

	protected SCR_AIGroupUtilityComponent m_Util;
	protected IEntity m_Building;
	protected string m_sFaction;
	protected string m_sTrigger;
	protected bool m_bSweep;
	protected int m_iPhase;
	protected float m_fDCOBreachAt_ms = -1;
	protected bool m_bDCOBreachThrown;
	protected float m_fStart_ms;
	protected float m_fPhase_ms;
	protected float m_fSuppressed_ms = -1;
	protected bool m_bContact;
	protected int m_iEntryStart;
	protected int m_iCleared;
	protected vector m_vStack;
	protected vector m_vSupport;

	protected ref array<AIAgent> m_aEntry = {};
	protected ref array<vector> m_aSlots = {};
	protected ref array<int> m_aPending = {};
	protected ref array<int> m_aPairSlot = {};
	protected ref array<float> m_aPairSent_ms = {};

	IEntity GetBuilding()	{ return m_Building; }
	bool HasEntry()	{ return !m_aEntry.IsEmpty(); }
	bool IsEntry(AIAgent a)	{ return m_aEntry.Contains(a); }

	void DCO_CQBClear(SCR_AIGroupUtilityComponent util, IEntity building, string faction, string trigger, bool sweep, float now_ms, string info = "")
	{
		m_Util = util;
		m_Building = building;
		m_sFaction = faction;
		m_sTrigger = trigger;
		m_bSweep = sweep;
		m_fStart_ms = now_ms;
		m_fPhase_ms = now_ms;
		m_bContact = DCO_BuildingClear.State(building, faction) == 1;

		SplitTeams();
		if (m_aEntry.IsEmpty())
		{
			DCO_Perf.Count("cqb_skip:no_entry");
			return;
		}

		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		if (reg)
			reg.Request(building, 1);

		foreach (AIAgent a : m_aEntry)
			SendMove(a, building.GetOrigin(), 10.0);

		string startLine = string.Format("cqb_start grp=%1 building=%2 trigger=%3 entry=%4 support=%5",
			GroupName(), building, trigger, m_aEntry.Count(), util.m_Owner.GetAgentsCount() - m_aEntry.Count());
		DCO_GroupUtilityComponent gdco = DCO_GroupUtilityComponent.Cast(util.m_Owner.FindComponent(DCO_GroupUtilityComponent));
		if (gdco && gdco.m_iDCOTRPhase >= 0)
			startLine += " tr_phase=" + typename.EnumToString(DCO_ETRPhase, gdco.m_iDCOTRPhase);
		if (!info.IsEmpty())
			startLine += " " + info;
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", startLine);
	}

	protected void SplitTeams()
	{
		array<AIAgent> agents = {};
		m_Util.m_Owner.GetAgents(agents);

		int isolateMin = 6;
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(m_Util.m_Owner);
		if (cfg)
			isolateMin = cfg.GetCQBIsolateMin();

		int alive = 0;
		foreach (AIAgent a : agents)
		{
			if (IsUsable(a))
				alive++;
		}

		int support = 0;
		if (alive >= isolateMin)
			support = alive / 3;

		foreach (AIAgent a : agents)
		{
			if (!IsUsable(a))
				continue;
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (support > 0 && ca && ca.m_InfoComponent && ca.m_InfoComponent.HasRole(EUnitRole.MACHINEGUNNER))
			{
				support--;
				continue;
			}
			m_aEntry.Insert(a);
		}

		while (support > 0 && m_aEntry.Count() > 2)
		{
			m_aEntry.Remove(m_aEntry.Count() - 1);
			support--;
		}

		m_iEntryStart = m_aEntry.Count();

		vector center;
		if (TeamCenter(center))
		{
			vector away = center - m_Building.GetOrigin();
			away[1] = 0;
			if (away.Length() < 1)
				away = Vector(1, 0, 0);
			away.Normalize();
			m_vSupport = m_Building.GetOrigin() + away * 45.0;
			m_vSupport = DCO_Util.Surface(m_vSupport);
		}
	}

	bool Update(float now_ms)
	{
		if (!m_Building || !m_Util || !m_Util.m_Owner)
			return Finish(false, "lost", now_ms);

		int alive = PruneEntry();
		int abortLosses = 2;
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(m_Util.m_Owner);
		if (cfg)
			abortLosses = cfg.GetCQBAbortLosses();

		if (alive == 0 || m_iEntryStart - alive >= abortLosses)
			return Finish(false, "losses", now_ms);
		if (IsPinned(now_ms))
			return Finish(false, "suppressed", now_ms);
		if (now_ms - m_fStart_ms > 300000.0)
			return Finish(false, "timeout", now_ms);

		ScanContact(now_ms);

		switch (m_iPhase)
		{
			case 0: Approach(now_ms); break;
			case 1: Stack(now_ms); break;
			case 2: return Clear(now_ms);
		}
		return true;
	}

	protected void Approach(float now_ms)
	{
		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		DCO_GarrisonBuilding gb;
		if (reg)
			gb = reg.Request(m_Building, 1);

		if ((!gb || !gb.m_bReady) && now_ms - m_fStart_ms < 10000.0)
			return;

		vector center;
		if (!TeamCenter(center))
			center = m_Building.GetOrigin();

		vector entry = m_Building.GetOrigin();
		vector outward = center - entry;
		outward[1] = 0;
		float best = float.MAX;
		array<DCO_GarrisonSlot> interior = {};
		if (gb && gb.m_bReady)
		{
			foreach (DCO_GarrisonSlot s : gb.m_aSlots)
			{
				if (!s || s.m_bUnreachable)
					continue;
				if (s.m_eType == DCO_EGarrisonSlotType.INTERIOR || s.m_eType == DCO_EGarrisonSlotType.WINDOW)
					interior.Insert(s);
				if (s.m_eType != DCO_EGarrisonSlotType.DOOR_GUARD)
					continue;

				float score = vector.DistanceXZ(s.m_vWorldPos, center);
				if (EnemyNear(s.m_vWorldPos, 10.0))
					score += 50;
				if (score < best)
				{
					best = score;
					entry = s.m_vWorldPos;
					outward = s.m_vWorldPos - m_Building.GetOrigin();
				}
			}
		}

		outward[1] = 0;
		if (outward.Length() < 0.1)
			outward = Vector(1, 0, 0);
		outward.Normalize();
		m_vStack = entry + outward * 4.0;
		m_vStack = DCO_Util.Surface(m_vStack);

		BuildSlots(interior, entry);

		if (vector.DistanceXZ(center, m_vStack) > 30.0 && !m_bSweep)
			ThrowSmoke((center + m_vStack) * 0.5);

		foreach (AIAgent a : m_aEntry)
			SendMove(a, m_vStack, 3.0);

		m_iPhase = 1;
		m_fPhase_ms = now_ms;
	}

	protected void BuildSlots(array<DCO_GarrisonSlot> slots, vector entry)
	{
		m_aSlots.Clear();
		m_aPending.Clear();

		array<int> floors = {};
		array<float> dists = {};
		foreach (DCO_GarrisonSlot s : slots)
		{
			float d = vector.Distance(s.m_vWorldPos, entry);
			int at = 0;
			while (at < m_aSlots.Count() && (floors[at] < s.m_iFloor || (floors[at] == s.m_iFloor && dists[at] <= d)))
				at++;
			m_aSlots.InsertAt(s.m_vWorldPos, at);
			floors.InsertAt(s.m_iFloor, at);
			dists.InsertAt(d, at);
		}

		if (m_aSlots.IsEmpty())
			m_aSlots.Insert(m_Building.GetOrigin());

		for (int i = 0; i < m_aSlots.Count(); i++)
			m_aPending.Insert(i);
	}

	protected void Stack(float now_ms)
	{
		int near = 0;
		foreach (AIAgent a : m_aEntry)
		{
			IEntity ent = a.GetControlledEntity();
			if (ent && vector.DistanceXZ(ent.GetOrigin(), m_vStack) <= 4.0)
				near++;
		}

		float wait_ms = 10000;
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(m_Util.m_Owner);
		if (cfg)
			wait_ms = cfg.GetCQBStackWait() * 1000;
		if (LeaderPersonality() == DCO_EAIPersonality.RECKLESS)
			wait_ms = 0;

		bool ready = near >= Math.Min(2, m_aEntry.Count());
		if (!ready && now_ms - m_fPhase_ms < wait_ms)
			return;

		if (!m_aEntry.IsEmpty())
		{
			SCR_ChimeraAIAgent peeker = SCR_ChimeraAIAgent.Cast(m_aEntry[0]);
			if (peeker && peeker.m_UtilityComponent && peeker.m_UtilityComponent.m_CombatComponent)
				peeker.m_UtilityComponent.m_CombatComponent.DCO_NotePeek(m_aSlots[0] + Vector(0, 1.3, 0), 7, 2500.0);
		}

		bool known = m_bContact;
		DCO_BuildingClearEntry e = DCO_BuildingClear.Get(m_Building, m_sFaction, false);
		if (e && e.m_fContact_ms >= 0 && now_ms - e.m_fContact_ms < 30000.0)
			known = true;
		if (!known && LeaderPersonality() == DCO_EAIPersonality.CAUTIOUS && !m_bSweep)
			known = e && e.m_fContact_ms >= 0;

		if (known && !m_aEntry.IsEmpty())
		{
			if (m_fDCOBreachAt_ms < 0)
			{
				m_fDCOBreachAt_ms = now_ms;
				for (int bi = 0; bi < Math.Min(2, m_aEntry.Count()) && !m_bDCOBreachThrown; bi++)
				{
					SCR_ChimeraAIAgent point = SCR_ChimeraAIAgent.Cast(m_aEntry[bi]);
					if (point && point.m_UtilityComponent && DCO_BreachUtility.TryThrowBreachGrenade(point.m_UtilityComponent, m_aSlots[0]))
					{
						m_bDCOBreachThrown = true;
						DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_breach grp=%1 building=%2 thrower=%3", m_Util.m_Owner, m_Building, bi));
					}
				}
			}
			if (m_bDCOBreachThrown && now_ms - m_fDCOBreachAt_ms < 5000.0)
				return;
		}
		m_fDCOBreachAt_ms = -1;
		m_bDCOBreachThrown = false;

		m_aPairSlot.Clear();
		m_aPairSent_ms.Clear();
		int pairs = Math.Max(1, (m_aEntry.Count() + 1) / 2);
		for (int p = 0; p < pairs; p++)
		{
			m_aPairSlot.Insert(-1);
			m_aPairSent_ms.Insert(0);
		}

		m_iPhase = 2;
		m_fPhase_ms = now_ms;
	}

	protected bool Clear(float now_ms)
	{
		int pairs = Math.Max(1, (m_aEntry.Count() + 1) / 2);
		while (m_aPairSlot.Count() > pairs)
		{
			int dropped = m_aPairSlot[m_aPairSlot.Count() - 1];
			if (dropped >= 0)
				m_aPending.InsertAt(dropped, 0);
			m_aPairSlot.Remove(m_aPairSlot.Count() - 1);
			m_aPairSent_ms.Remove(m_aPairSent_ms.Count() - 1);
		}

		for (int p = 0; p < m_aPairSlot.Count(); p++)
		{
			int slot = m_aPairSlot[p];
			if (slot >= 0 && PairReached(p, m_aSlots[slot]))
			{
				m_iCleared++;
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_room grp=%1 slot=%2 contact=%3", m_Util.m_Owner, slot, m_bContact));
				slot = -1;
			}

			if (slot < 0)
			{
				if (m_aPending.IsEmpty())
				{
					m_aPairSlot[p] = -1;
					continue;
				}
				slot = m_aPending[0];
				m_aPending.Remove(0);
				m_aPairSlot[p] = slot;
				m_aPairSent_ms[p] = 0;
			}

			if (now_ms - m_aPairSent_ms[p] >= 20000.0)
			{
				m_aPairSent_ms[p] = now_ms;
				SendPair(p, m_aSlots[slot]);
			}
		}

		bool busy = false;
		foreach (int s : m_aPairSlot)
		{
			if (s >= 0)
				busy = true;
		}
		if (!busy && m_aPending.IsEmpty())
			return Finish(true, "cleared", now_ms);
		return true;
	}

	protected bool PairReached(int pair, vector pos)
	{
		for (int i = pair * 2; i < pair * 2 + 2 && i < m_aEntry.Count(); i++)
		{
			IEntity ent = m_aEntry[i].GetControlledEntity();
			if (ent && vector.Distance(ent.GetOrigin(), pos) <= 2.5)
				return true;
		}
		return false;
	}

	protected void SendPair(int pair, vector pos)
	{
		for (int i = pair * 2; i < pair * 2 + 2 && i < m_aEntry.Count(); i++)
		{
			SendMove(m_aEntry[i], pos, 1.5);
			if (m_aEntry[i])
				DCO_BldLearn.OccupyAt(m_aEntry[i].GetControlledEntity(), m_Building, pos, "cqb");
		}
	}

	protected void ScanContact(float now_ms)
	{
		SCR_AIGroupPerception perc = m_Util.GetPercGroupComp();
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!perc || !pm)
			return;

		float t = pm.GetTime();
		foreach (SCR_AITargetInfo tgt : perc.m_aTargets)
		{
			if (!tgt || !tgt.m_Entity || tgt.m_eCategory != EAITargetInfoCategory.IDENTIFIED || t - tgt.m_fTimestamp > 5)
				continue;
			if (SCR_CoverManagerComponent.DCO_GetBuildingAt(tgt.m_Entity) != m_Building)
				continue;

			m_bContact = true;
			DCO_BuildingClear.MarkContact(m_Building, m_sFaction, now_ms);
			if (m_iPhase != 2)
				return;

			foreach (AIAgent a : m_aEntry)
			{
				SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
				if (ca && ca.m_UtilityComponent && DCO_BreachUtility.TryThrowBreachGrenade(ca.m_UtilityComponent, tgt.m_vWorldPos))
					return;
			}
			return;
		}
	}

	protected bool IsPinned(float now_ms)
	{
		int threatened = 0;
		foreach (AIAgent a : m_aEntry)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (ca && ca.m_UtilityComponent && ca.m_UtilityComponent.m_ThreatSystem
				&& ca.m_UtilityComponent.m_ThreatSystem.GetState() == EAIThreatState.THREATENED)
				threatened++;
		}

		if (threatened * 2 <= m_aEntry.Count())
		{
			m_fSuppressed_ms = -1;
			return false;
		}
		if (m_fSuppressed_ms < 0)
			m_fSuppressed_ms = now_ms;
		return now_ms - m_fSuppressed_ms > 10000.0;
	}

	protected bool Finish(bool cleared, string reason, float now_ms)
	{
		if (m_Building && m_Util && m_Util.m_Owner)
			DCO_BuildingClear.Finish(m_Building, m_sFaction, m_Util.m_Owner, cleared, now_ms, 120000.0);

		int losses = m_iEntryStart - m_aEntry.Count();
		if (cleared)
		{
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_cleared grp=%1 building=%2 duration=%3 losses=%4 rooms=%5 trigger=%6",
				GroupName(), m_Building, Math.Round((now_ms - m_fStart_ms) / 1000), losses, m_iCleared, m_sTrigger));
			return false;
		}

		foreach (AIAgent a : m_aEntry)
			SendMove(a, m_vSupport, 10.0);

		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_abort grp=%1 building=%2 reason=%3 losses=%4 rooms=%5",
			GroupName(), m_Building, reason, losses, m_iCleared));
		return false;
	}

	void Cancel(float now_ms)
	{
		Finish(false, "cancel", now_ms);
	}

	protected string GroupName()
	{
		if (m_Util && m_Util.m_Owner)
			return DCO_Log.IdGroup(m_Util.m_Owner) + "/" + m_Util.m_Owner.GetName();
		return "-";
	}

	protected int PruneEntry()
	{
		for (int i = m_aEntry.Count() - 1; i >= 0; i--)
		{
			if (!IsUsable(m_aEntry[i]))
				m_aEntry.Remove(i);
		}
		return m_aEntry.Count();
	}

	protected bool IsUsable(AIAgent a)
	{
		if (!a || !m_Util || a.GetParentGroup() != m_Util.m_Owner)
			return false;

		ChimeraCharacter ch = ChimeraCharacter.Cast(a.GetControlledEntity());
		if (!ch || ch.IsInVehicle())
			return false;

		return DCO_Util.IsAlive(ch);
	}

	protected bool TeamCenter(out vector center)
	{
		center = vector.Zero;
		int n = 0;
		foreach (AIAgent a : m_aEntry)
		{
			IEntity ent = a.GetControlledEntity();
			if (!ent)
				continue;
			center += ent.GetOrigin();
			n++;
		}
		if (n == 0)
			return false;
		center = center * (1.0 / n);
		return true;
	}

	protected bool EnemyNear(vector pos, float radius)
	{
		SCR_AIGroupPerception perc = m_Util.GetPercGroupComp();
		if (!perc)
			return false;

		float radiusSq = radius * radius;
		foreach (SCR_AITargetInfo tgt : perc.m_aTargets)
		{
			if (tgt && (tgt.m_eCategory == EAITargetInfoCategory.IDENTIFIED || tgt.m_eCategory == EAITargetInfoCategory.DETECTED)
				&& vector.DistanceSqXZ(tgt.m_vWorldPos, pos) <= radiusSq)
				return true;
		}
		return false;
	}

	protected DCO_EAIPersonality LeaderPersonality()
	{
		SCR_ChimeraAIAgent leader = SCR_ChimeraAIAgent.Cast(m_Util.m_Owner.GetLeaderAgent());
		if (!leader || !leader.m_UtilityComponent)
			return DCO_EAIPersonality.STANDARD;
		return DCO_PersonalityCombatUtility.GetPersonalitySafe(leader.m_UtilityComponent);
	}

	protected void ThrowSmoke(vector pos)
	{
		foreach (AIAgent a : m_aEntry)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_InfoComponent || !ca.m_InfoComponent.HasRole(EUnitRole.HAS_SMOKE_GRENADE) || !ca.GetCommunicationComponent())
				continue;

			SCR_AIMessage_ThrowGrenadeTo msg = SCR_AIMessage_ThrowGrenadeTo.Create(pos, EWeaponType.WT_SMOKEGRENADE, 0);
			msg.m_fPriorityLevel = SCR_AIActionBase.PRIORITY_BEHAVIOR_THROW_GRENADE;
			msg.SetReceiver(ca);
			ca.GetCommunicationComponent().RequestBroadcast(msg, ca);
			return;
		}
	}

	protected void SendMove(AIAgent agent, vector pos, float radius)
	{
		if (!agent || !m_Util || !m_Util.m_Owner)
			return;

		AICommunicationComponent comms = m_Util.m_Owner.GetCommunicationComponent();
		if (!comms)
			return;

		SCR_AIMessage_Investigate msg = SCR_AIMessage_Investigate.Create(null, pos, radius, true, duration: 40.0);
		msg.m_fPriorityLevel = SCR_AIActionBase.PRIORITY_LEVEL_PLAYER;
		msg.SetReceiver(agent);
		comms.RequestBroadcast(msg, agent);
	}
}
