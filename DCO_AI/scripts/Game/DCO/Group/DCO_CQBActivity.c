enum DCO_ECQBPhase
{
	PLAN,
	STACK,
	ROOMS,
	CONSOLIDATE,
	DONE,
	ABORT
}

class DCO_CQBRoom
{
	vector m_vCenter;
	int m_iFloor;
	ref array<vector> m_aSlots = {};
	int m_iState;
	int m_iIdx;
	int m_iDefer;
}

class DCO_CQBActivity : SCR_AIActivityBase
{
	IEntity m_Building;
	protected string m_sFaction;
	protected string m_sTrigger;
	protected bool m_bSweep;
	protected bool m_bPlanned;
	protected ref DCO_INavCQBPlan m_Nav;
	protected ref array<ref DCO_CQBRoom> m_aRooms = {};
	protected ref array<SCR_ChimeraAIAgent> m_aEntry = {};
	protected ref array<SCR_ChimeraAIAgent> m_aRear = {};
	protected ref array<SCR_ChimeraAIAgent> m_aLost = {};
	protected ref map<SCR_ChimeraAIAgent, DCO_CQBMemberBehavior> m_mOrd = new map<SCR_ChimeraAIAgent, DCO_CQBMemberBehavior>();
	protected ref map<SCR_ChimeraAIAgent, vector> m_mSpot = new map<SCR_ChimeraAIAgent, vector>();
	protected ref array<vector> m_aTried = {};
	protected float m_fDCOStackFireAt_ms;
	protected bool m_bDCOStackSwitched;

	void NoteStackUnderFire(float now_ms)
	{
		if (m_ePhase == DCO_ECQBPhase.STACK && m_fDCOStackFireAt_ms <= 0)
			m_fDCOStackFireAt_ms = now_ms;
	}
	protected DCO_ECQBPhase m_ePhase = DCO_ECQBPhase.PLAN;
	protected int m_iPair;
	protected int m_iRoom = -1;
	protected int m_iFoothold = -1;
	protected int m_iCleared;
	protected int m_iTimeouts;
	protected int m_iLosses;
	protected int m_iStep;
	protected int m_iFrags;
	protected int m_iEnterTry;
	protected float m_fStart_ms;
	protected float m_fPhaseAt_ms;
	protected float m_fStepAt_ms;
	protected float m_fRoomAt_ms;
	protected float m_fSweepUntil_ms;
	protected float m_fDeadline_ms = -1;
	protected float m_fConsolidateAt_ms = -1;
	protected bool m_bStackOrdered;
	protected bool m_bFinished;
	protected string m_sEndWhy = "";
	protected vector m_vEntry;
	protected vector m_vOut;
	protected vector m_vStackA;
	protected vector m_vStackB;
	protected vector m_vDoor;
	protected vector m_vDir;
	protected vector m_vEnemy;
	protected float m_fEnemyAt_ms = -100000.0;
	protected int m_iIsolateMin = 6;
	protected float m_fStackWait_ms = 15000.0;
	protected int m_iAbortLosses = 99;
	protected int m_iBreach;
	protected float m_fRoomLimit_ms = 60000.0;
	protected float m_fFirstAt_ms = -1;
	protected bool m_bSwapTried;
	protected float m_fNearAt_ms = -1;
	protected float m_fProgAt_ms;
	protected bool m_bCoverFired;
	protected float m_fPrepUntil_ms;
	protected bool m_bFragOther;
	protected float m_fLoneAt_ms = -1;
	protected int m_iReorders;
	protected bool m_bTwoDoor;
	protected bool m_bSecondSent;
	protected bool m_bSecondUsed;
	protected vector m_vEntry2;
	protected vector m_vOut2;
	protected vector m_vStack2A;
	protected vector m_vStack2B;
	protected float m_fReady0_ms = -1;

	protected static ref map<string, int> s_mBadPrefab;

	static string BadKey(IEntity building)
	{
		string k = DCO_Util.PrefabShort(building);
		if (k.IsEmpty())
			k = "pos:" + DCO_Util.XZ(building.GetOrigin());
		return k;
	}

	static void NoteBadPlan(IEntity building, string reason)
	{
		if (!building || (reason != "plan_empty" && reason != "plan_no_outside_door" && reason != "plan_no_rooms"))
			return;
		if (!s_mBadPrefab)
			s_mBadPrefab = new map<string, int>();
		string k = BadKey(building);
		int n = s_mBadPrefab.Get(k) + 1;
		s_mBadPrefab.Set(k, n);
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_prefab_bad key=%1 reason=%2 fails=%3", k, reason, n));
	}

	static DCO_CQBActivity Create(SCR_AIGroupUtilityComponent util, IEntity building, string faction, string trigger, bool sweep, string info, AIWaypoint wp = null)
	{
		if (!util || !util.m_Owner || !building)
			return null;
		DCO_GlobalAIComponent nc = DCO_GlobalAIComponent.GetFor(util.m_Owner);
		if (nc && nc.DCO_TuneF("cqb.prefab_negcache", 0) > 0 && s_mBadPrefab)
		{
			string bk = BadKey(building);
			if (s_mBadPrefab.Get(bk) >= nc.DCO_TuneF("cqb.prefab_negcache_fails", 2))
			{
				DCO_Perf.Count("cqb_skip:bad_prefab");
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, DCO_Log.IdGroup(util.m_Owner), string.Format("cqb_skip grp=%1 building=%2 reason=bad_prefab fails=%3 trigger=%4", DCO_Log.IdGroup(util.m_Owner), bk, s_mBadPrefab.Get(bk), trigger));
				return null;
			}
		}
		if (DCO_CQBPrecheck.On(util.m_Owner))
		{
			string pw;
			if (DCO_CQBPrecheck.Check(building, util.m_Owner, pw) == 0)
			{
				DCO_Perf.Count("cqb_skip:precheck");
				if (DCO_CQBPrecheck.LogSkip(building))
					DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, DCO_Log.IdGroup(util.m_Owner), string.Format("cqb_skip grp=%1 building=%2 reason=precheck_%3 trigger=%4", DCO_Log.IdGroup(util.m_Owner), DCO_Util.PrefabShort(building), pw, trigger));
				return null;
			}
		}
		array<AIAgent> agents = {};
		util.m_Owner.GetAgents(agents);
		int usable = 0;
		foreach (AIAgent a : agents)
		{
			if (Usable(util, a))
				usable++;
		}
		if (usable < 2)
		{
			DCO_Perf.Count("cqb:need_infantry");
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, DCO_Log.IdGroup(util.m_Owner), string.Format("cqb_need_infantry grp=%1 building=%2 usable=%3 trigger=%4", DCO_Log.IdGroup(util.m_Owner), DCO_Util.PrefabShort(building), usable, trigger));
			return null;
		}
		return new DCO_CQBActivity(util, wp, building, faction, trigger, sweep, info);
	}

	static bool Usable(SCR_AIGroupUtilityComponent util, AIAgent a)
	{
		if (!a || !util || a.GetParentGroup() != util.m_Owner)
			return false;
		SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
		if (!ca || !ca.m_UtilityComponent)
			return false;
		ChimeraCharacter ch = ChimeraCharacter.Cast(a.GetControlledEntity());
		if (!ch || ch.IsInVehicle())
			return false;
		return DCO_Util.IsAlive(ch);
	}

	void DCO_CQBActivity(SCR_AIGroupUtilityComponent utility, AIWaypoint relatedWaypoint, IEntity building, string faction, string trigger, bool sweep, string info)
	{
		m_sBehaviorTree = "AI/BehaviorTrees/Chimera/Group/DCO_CQB.bt";
		SetPriority(PRIORITY_ACTIVITY_ATTACK_CLUSTER + 4);
		SCR_AIWaypoint levelWp = SCR_AIWaypoint.Cast(relatedWaypoint);
		if (levelWp)
			m_fPriorityLevel.m_Value = levelWp.GetPriorityLevel();
		m_Building = building;
		m_sFaction = faction;
		m_sTrigger = trigger;
		m_bSweep = sweep;
		m_bPlanned = trigger == "planned";
		m_fStart_ms = GetGame().GetWorld().GetWorldTime();
		m_fPhaseAt_ms = m_fStart_ms;
		m_fStepAt_ms = m_fStart_ms;
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(utility.m_Owner);
		if (cfg)
		{
			m_iIsolateMin = cfg.GetCQBIsolateMin();
			m_fStackWait_ms = cfg.GetCQBStackWait() * 1000.0;
			m_iAbortLosses = cfg.GetCQBAbortLosses();
			m_iBreach = cfg.GetCQBBreachMode();
			m_fRoomLimit_ms = cfg.GetCQBRoomTimeout() * 1000.0;
		}
		SplitTeams(utility);
		DCO_Perf.Count("cqb:start");
		string line = string.Format("cqb_start grp=%1 building=%2 trigger=%3 entry=%4 pairs=%5 rear=%6 sweep=%7 cfg=isolate:%8,stack_s:%9", GroupName(utility), DCO_Util.PrefabShort(building), trigger, m_aEntry.Count(), PairCount(), m_aRear.Count(), DCO_Util.B01(sweep), m_iIsolateMin, Math.Round(m_fStackWait_ms * 0.001));
		line += string.Format(" abort_losses=%1 breach=%2 room_s=%3", m_iAbortLosses, m_iBreach, Math.Round(m_fRoomLimit_ms * 0.001));
		if (!info.IsEmpty())
			line += " " + info;
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", line);
	}

	protected ref array<float> m_aBelief = {};
	protected float m_fBeliefAt_ms;
	protected int m_iBeliefTop = -1;
	protected int m_iRestacks;
	protected int m_iResweeps;
	protected float m_fEvidenceAt_ms = -1;
	protected vector m_vEvidence;

	protected int DCO_RoomNearest(vector pos)
	{
		int bestRoom = -1;
		float bestD = 99999.0;
		foreach (int i, DCO_CQBRoom r : m_aRooms)
		{
			if (r.m_iState != 0)
				continue;
			float d = vector.DistanceXZ(r.m_vCenter, pos) + Math.AbsFloat(r.m_vCenter[1] - pos[1]) * 3.0;
			if (d < bestD)
			{
				bestD = d;
				bestRoom = i;
			}
		}
		return bestRoom;
	}

	protected void DCO_Believe(int room, float w, string src, float now)
	{
		if (room < 0 || room >= m_aBelief.Count() || w <= m_aBelief[room])
			return;
		m_aBelief[room] = w;
		DCO_Perf.Count("cqb:belief_" + src);
	}

	protected void DCO_UpdateBelief(float now)
	{
		if (now - m_fBeliefAt_ms < 1000.0)
			return;
		m_fBeliefAt_ms = now;
		if (m_aBelief.Count() != m_aRooms.Count())
		{
			m_aBelief.Clear();
			foreach (DCO_CQBRoom r0 : m_aRooms)
				m_aBelief.Insert(0);
		}
		if (m_Nav)
			m_Nav.m_aBelief = m_aBelief;
		for (int i = 0; i < m_aBelief.Count(); i++)
		{
			m_aBelief[i] = m_aBelief[i] * 0.98;
			if (m_aRooms[i].m_iState != 0)
				m_aBelief[i] = 0;
		}
		if (!m_Utility || !m_Utility.m_Perception || !m_Building || !m_Utility.m_Owner)
			return;
		IEntity leader = m_Utility.m_Owner.GetLeaderEntity();
		PerceptionManager pm = GetGame().GetPerceptionManager();
		float pmNow = 0;
		if (pm)
			pmNow = pm.GetTime();
		foreach (SCR_AITargetInfo t : m_Utility.m_Perception.m_aTargets)
		{
			if (!t || !t.m_Entity || !DCO_Util.IsAlive(t.m_Entity))
				continue;
			float age = pmNow - t.m_fTimestamp;
			if (age > 90.0)
				continue;
			string method;
			if (DCO_CoverManagerComponent.DCO_GetBuildingAtPos(t.m_vWorldPos, leader, method) != m_Building)
				continue;
			m_fEvidenceAt_ms = now;
			m_vEvidence = t.m_vWorldPos;
			DCO_Believe(DCO_RoomNearest(t.m_vWorldPos), 1.0 - age / 90.0, "seen", now);
		}
		vector est;
		float rad;
		int cnt;
		float conf = 1.0;
		string estSrc;
		if (DCO_FireEstimate.GetQ(m_Utility.m_Owner, now, 25000.0, est, rad, cnt, conf, estSrc))
		{
			string hm;
			if (DCO_CoverManagerComponent.DCO_GetBuildingAtPos(est, leader, hm) == m_Building)
			{
				m_fEvidenceAt_ms = now;
				m_vEvidence = est;
				DCO_Believe(DCO_RoomNearest(est), 0.6 * Math.Clamp(conf, 0.3, 1.0), "heard", now);
			}
		}
		int top = -1;
		float topV = 0.05;
		for (int j = 0; j < m_aBelief.Count(); j++)
		{
			if (m_aBelief[j] > topV)
			{
				topV = m_aBelief[j];
				top = j;
			}
		}
		if (m_Nav)
		{
			m_Nav.m_iBeliefFloor = -1;
			if (top >= 0)
				m_Nav.m_iBeliefFloor = m_aRooms[top].m_iFloor;
			else if (m_fEvidenceAt_ms >= 0 && now - m_fEvidenceAt_ms < 30000.0)
			{
				int evRoom = DCO_RoomNearestAny(m_vEvidence);
				if (evRoom >= 0)
					m_Nav.m_iBeliefFloor = m_aRooms[evRoom].m_iFloor;
			}
		}
		if (top != m_iBeliefTop)
		{
			m_iBeliefTop = top;
			string floorTxt = "-";
			if (top >= 0)
				floorTxt = m_aRooms[top].m_iFloor.ToString();
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_belief grp=%1 room=%2 floor=%3 value=%4 rooms=%5", Grp(), top, floorTxt, topV.ToString(-1, 2), m_aBelief.Count()));
		}
	}

	protected int DCO_RoomNearestAny(vector pos)
	{
		int bestRoom = -1;
		float bestD = 99999.0;
		foreach (int i, DCO_CQBRoom r : m_aRooms)
		{
			float d = vector.DistanceXZ(r.m_vCenter, pos) + Math.AbsFloat(r.m_vCenter[1] - pos[1]) * 3.0;
			if (d < bestD)
			{
				bestD = d;
				bestRoom = i;
			}
		}
		return bestRoom;
	}

	protected float DCO_RoomBelief(int room)
	{
		if (room < 0 || room >= m_aBelief.Count())
			return 0;
		return m_aBelief[room];
	}

	protected void OrderContain(int room)
	{
		if (m_aRear.IsEmpty())
			return;
		vector side = Vector(m_vDir[2], 0, -m_vDir[0]);
		vector look = m_vDoor + Vector(0, 1.2, 0);
		int n = 0;
		foreach (int i, SCR_ChimeraAIAgent r : m_aRear)
		{
			if (!r || !r.m_UtilityComponent)
				continue;
			float sign = 1.0;
			if (i % 2 == 1)
				sign = -1.0;
			vector spot = DCO_Util.Surface(m_vDoor - m_vDir * 6.0 + side * (2.5 * sign));
			m_mSpot.Set(r, spot);
			Order(r, new DCO_CQBSupportBehavior(r.m_UtilityComponent, this, spot, look, ECharacterStance.CROUCH, EMovementType.RUN, 300.0, "contain_" + (i + 1).ToString()));
			n++;
		}
		DCO_Perf.Count("cqb:contain");
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_contain grp=%1 room=%2 floor=%3 belief=%4 units=%5", Grp(), room, m_aRooms[room].m_iFloor, DCO_RoomBelief(room).ToString(-1, 2), n));
	}

	protected int DCO_PriorityRoom()
	{
		DCO_UpdateBelief(Now());
		return m_iBeliefTop;
	}

	protected string GroupName(SCR_AIGroupUtilityComponent u)
	{
		if (!u || !u.m_Owner)
			return "-";
		return DCO_Log.IdGroup(u.m_Owner) + "/" + u.m_Owner.GetName();
	}

	protected string Grp()
	{
		return GroupName(m_Utility);
	}

	protected float Now()
	{
		return GetGame().GetWorld().GetWorldTime();
	}

	protected void SplitTeams(SCR_AIGroupUtilityComponent utility)
	{
		array<AIAgent> agents = {};
		utility.m_Owner.GetAgents(agents);
		array<SCR_ChimeraAIAgent> riflemen = {};
		array<SCR_ChimeraAIAgent> mgs = {};
		foreach (AIAgent a : agents)
		{
			if (!Usable(utility, a))
				continue;
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (ca.m_InfoComponent && ca.m_InfoComponent.HasRole(EUnitRole.MACHINEGUNNER))
				mgs.Insert(ca);
			else
				riflemen.Insert(ca);
		}
		int alive = riflemen.Count() + mgs.Count();
		int rear = 0;
		if (alive >= m_iIsolateMin + 2)
			rear = 2;
		else if (alive >= m_iIsolateMin)
			rear = 1;
		foreach (SCR_ChimeraAIAgent m : mgs)
		{
			if (m_aRear.Count() < rear)
				m_aRear.Insert(m);
			else
				riflemen.Insert(m);
		}
		for (int i = riflemen.Count() - 1; i >= 0 && m_aRear.Count() < rear; i--)
		{
			m_aRear.Insert(riflemen[i]);
			riflemen.Remove(i);
		}
		m_aEntry.InsertAll(riflemen);
		if (m_aEntry.Count() > 2 && m_aEntry.Count() % 2 == 1)
		{
			m_aRear.Insert(m_aEntry[m_aEntry.Count() - 1]);
			m_aEntry.Remove(m_aEntry.Count() - 1);
		}
	}

	protected int PairCount()
	{
		return Math.Max(1, (m_aEntry.Count() + 1) / 2);
	}

	protected SCR_ChimeraAIAgent PairMember(int pair, int k)
	{
		int i = pair * 2 + k;
		if (i < 0 || i >= m_aEntry.Count())
			return null;
		return m_aEntry[i];
	}

	protected int ActivePair()
	{
		return m_iPair % PairCount();
	}

	protected bool InActivePair(SCR_ChimeraAIAgent a)
	{
		return a && (PairMember(ActivePair(), 0) == a || PairMember(ActivePair(), 1) == a);
	}

	bool IsRunning()
	{
		EAIActionState s = GetActionState();
		return !m_bFinished && s != EAIActionState.FAILED && s != EAIActionState.COMPLETED;
	}

	DCO_ECQBPhase GetPhase()
	{
		return m_ePhase;
	}

	string EndWhy()
	{
		if (!m_bFinished)
			return "running";
		if (m_sEndWhy.IsEmpty())
			return "cancel";
		return m_sEndWhy;
	}

	int RoomIndex()
	{
		return m_iRoom;
	}

	bool IsEntry(AIAgent a)
	{
		SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
		return ca && m_aEntry.Contains(ca);
	}

	vector SupportSpotOf(AIAgent a)
	{
		SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
		if (!ca || !m_aRear.Contains(ca))
			return vector.Zero;
		return m_mSpot.Get(ca);
	}

	bool RoomSecured()
	{
		if (m_ePhase == DCO_ECQBPhase.CONSOLIDATE || m_ePhase == DCO_ECQBPhase.DONE)
			return true;
		return m_ePhase == DCO_ECQBPhase.ROOMS && (m_iStep == 0 || m_iStep == 1) && Now() - m_fEnemyAt_ms > 5000.0;
	}

	bool HoldFireAt(vector shooter, vector target, out float friendDist)
	{
		friendDist = -1;
		if (m_bFinished || vector.Distance(shooter, target) < 10.0)
			return false;
		vector seg = target - shooter;
		float segLen = seg.Length();
		if (segLen < 0.1)
			return false;
		vector segDir = seg * (1.0 / segLen);
		bool hold = false;
		float best = float.MAX;
		foreach (SCR_ChimeraAIAgent a : m_aEntry)
		{
			if (!a || !a.GetControlledEntity())
				continue;
			vector fp = a.GetControlledEntity().GetOrigin() + Vector(0, 1.0, 0);
			float dt = vector.Distance(fp, target);
			float along = vector.Dot(fp - shooter, segDir);
			float lateral = 1000.0;
			if (along > 0 && along < segLen + 2.0)
				lateral = (fp - shooter - segDir * along).Length();
			float d = Math.Min(dt, lateral);
			if (d < best)
				best = d;
			if (dt < 5.0 || lateral < 3.0)
				hold = true;
		}
		if (best < float.MAX)
			friendDist = best;
		return hold;
	}

	bool KnownEnemyNear(vector pos, float radius, out float dist)
	{
		dist = -1;
		SCR_AIGroupPerception perc = m_Utility.GetPercGroupComp();
		if (!perc)
			return false;
		float bestD = radius;
		bool found = false;
		foreach (SCR_AITargetInfo tgt : perc.m_aTargets)
		{
			if (!tgt || !tgt.m_Entity || !DCO_Util.IsNotDead(tgt.m_Entity))
				continue;
			float d = vector.Distance(tgt.m_Entity.GetOrigin(), pos);
			if (d <= bestD)
			{
				bestD = d;
				found = true;
			}
		}
		if (found)
			dist = bestD;
		return found;
	}

	protected bool EnemyInRoom(DCO_CQBRoom room, out vector where)
	{
		where = vector.Zero;
		if (!room)
			return false;
		SCR_AIGroupPerception perc = m_Utility.GetPercGroupComp();
		if (!perc)
			return false;
		foreach (SCR_AITargetInfo tgt : perc.m_aTargets)
		{
			if (!tgt || !tgt.m_Entity || !DCO_Util.IsNotDead(tgt.m_Entity))
				continue;
			vector p = tgt.m_Entity.GetOrigin();
			if (Math.AbsFloat(p[1] - room.m_vCenter[1]) > 1.8)
				continue;
			bool near = vector.DistanceXZ(p, room.m_vCenter) <= 4.0;
			foreach (vector s : room.m_aSlots)
			{
				if (vector.DistanceXZ(p, s) <= 2.5)
					near = true;
			}
			if (near)
			{
				where = p;
				return true;
			}
		}
		return false;
	}

	protected void SetPhase(DCO_ECQBPhase p)
	{
		float ms = Now() - m_fPhaseAt_ms;
		m_ePhase = p;
		m_fPhaseAt_ms = Now();
		m_fFirstAt_ms = -1;
		m_bSwapTried = false;
		m_fNearAt_ms = -1;
		m_fReady0_ms = -1;
		string name = typename.EnumToString(DCO_ECQBPhase, p);
		name.ToLower();
		DCO_Perf.Count("cqb:phase_" + name);
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_phase grp=%1 phase=%2 room=%3 pair=%4 ms=%5 entry=%6 rear=%7", Grp(), name, m_iRoom, ActivePair() + 1, Math.Round(ms), m_aEntry.Count(), m_aRear.Count()));
	}

	protected void SetStep(int s)
	{
		m_iStep = s;
		m_fStepAt_ms = Now();
		m_fFirstAt_ms = -1;
		m_bSwapTried = false;
		m_iReorders = 0;
	}

	protected void Order(SCR_ChimeraAIAgent ca, DCO_CQBMemberBehavior b)
	{
		if (!ca || !ca.m_UtilityComponent)
			return;
		ClearOrders(ca);
		ca.m_UtilityComponent.AddAction(b);
		m_mOrd.Set(ca, b);
	}

	protected DCO_CQBMemberBehavior Ord(SCR_ChimeraAIAgent ca)
	{
		if (!ca)
			return null;
		return m_mOrd.Get(ca);
	}

	protected bool OrdArrived(SCR_ChimeraAIAgent ca)
	{
		DCO_CQBMemberBehavior b = Ord(ca);
		return b && b.Arrived();
	}

	protected bool OrdIdle(SCR_ChimeraAIAgent ca)
	{
		DCO_CQBMemberBehavior b = Ord(ca);
		return !b || b.IsDone();
	}

	protected void ClearOrders(SCR_ChimeraAIAgent ca)
	{
		if (!ca || !ca.m_UtilityComponent)
			return;
		array<ref AIActionBase> acts = {};
		ca.m_UtilityComponent.FindActionsOfType(DCO_CQBMemberBehavior, acts);
		foreach (AIActionBase act : acts)
		{
			EAIActionState s = act.GetActionState();
			if (s == EAIActionState.COMPLETED || s == EAIActionState.FAILED)
				continue;
			DCO_CQBMemberBehavior b = DCO_CQBMemberBehavior.Cast(act);
			if (b && !b.IsDone())
				b.DCO_Supersede();
		}
		m_mOrd.Remove(ca);
	}

	protected void StopOrders()
	{
		foreach (SCR_ChimeraAIAgent a : m_aEntry)
			ClearOrders(a);
		foreach (SCR_ChimeraAIAgent b : m_aRear)
			ClearOrders(b);
	}

	protected vector Pos(SCR_ChimeraAIAgent a)
	{
		if (!a || !a.GetControlledEntity())
			return vector.Zero;
		return a.GetControlledEntity().GetOrigin();
	}

	protected vector TeamCenter()
	{
		vector c = vector.Zero;
		int n = 0;
		foreach (SCR_ChimeraAIAgent a : m_aEntry)
		{
			if (!a || !a.GetControlledEntity())
				continue;
			c = c + a.GetControlledEntity().GetOrigin();
			n++;
		}
		if (n == 0)
			return m_Utility.m_Owner.GetOrigin();
		return c * (1.0 / n);
	}

	protected bool Guard()
	{
		if (m_bFinished)
			return false;
		if (!m_Building || !m_Utility || !m_Utility.m_Owner)
		{
			Abort("lost");
			return false;
		}
		CheckCasualties();
		if (m_aEntry.IsEmpty())
		{
			Abort("losses");
			return false;
		}
		if (m_iLosses >= m_iAbortLosses)
		{
			if (m_iRestacks >= 1 || m_aEntry.Count() < 2)
			{
				Abort("losses");
				return false;
			}
			m_iRestacks++;
			m_iLosses = 0;
			m_fEnemyAt_ms = Now();
			DCO_Perf.Count("cqb:restack");
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_restack grp=%1 room=%2 entry=%3 rear=%4 why=losses_frag_next", Grp(), m_iRoom, m_aEntry.Count(), m_aRear.Count()));
			if (m_ePhase == DCO_ECQBPhase.ROOMS && m_iRoom >= 0)
			{
				OrderStack();
				SetStep(1);
			}
		}
		if (m_fDeadline_ms > 0 && Now() > m_fDeadline_ms)
		{
			Abort("timeout");
			return false;
		}
		vector ep;
		if (m_iRoom >= 0 && m_iRoom < m_aRooms.Count() && EnemyInRoom(m_aRooms[m_iRoom], ep))
		{
			m_vEnemy = ep;
			m_fEnemyAt_ms = Now();
		}
		if (m_Nav)
			m_Nav.OpenDoors(m_aEntry, Now());
		return true;
	}

	protected void CheckCasualties()
	{
		for (int r = m_aRear.Count() - 1; r >= 0; r--)
		{
			if (!Usable(m_Utility, m_aRear[r]))
				m_aRear.Remove(r);
		}
		bool changed = false;
		for (int i = m_aEntry.Count() - 1; i >= 0; i--)
		{
			SCR_ChimeraAIAgent lost = m_aEntry[i];
			if (Usable(m_Utility, lost))
				continue;
			m_aEntry.RemoveOrdered(i);
			changed = true;
			if (!m_aLost.Contains(lost))
			{
				m_aLost.Insert(lost);
				m_iLosses++;
			}
			string by = "-";
			if (!m_aRear.IsEmpty())
			{
				SCR_ChimeraAIAgent rep = m_aRear[0];
				m_aRear.Remove(0);
				ClearOrders(rep);
				m_aEntry.InsertAt(rep, Math.Min(i, m_aEntry.Count()));
				by = DCO_Log.IdUnit(rep.GetControlledEntity());
			}
			DCO_Perf.Count("cqb:casualty");
			IEntity le;
			if (lost)
				le = lost.GetControlledEntity();
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_casualty grp=%1 unit=%2 room=%3 replaced_by=%4 losses=%5 entry=%6 pairs=%7", Grp(), DCO_Log.IdUnit(le), m_iRoom, by, m_iLosses, m_aEntry.Count(), PairCount()));
		}
		if (changed && m_ePhase == DCO_ECQBPhase.ROOMS && m_iStep > 0 && m_iStep < 5)
		{
			m_fEnemyAt_ms = Now();
			m_vEnemy = vector.Zero;
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_pair_reform grp=%1 room=%2 step=%3 pair=%4 evidence=casualty", Grp(), m_iRoom, m_iStep, ActivePair() + 1));
			OrderStack();
			SetStep(1);
		}
	}

	ENodeResult StepPlan()
	{
		if (!Guard())
			return ENodeResult.FAIL;
		if (m_ePhase != DCO_ECQBPhase.PLAN)
			return ENodeResult.SUCCESS;
		string why;
		m_Nav = DCO_INavCQBPlan.Make(m_Building, TeamCenter(), m_aTried, Grp(), why);
		if (!m_Nav)
		{
			if (why == "wait" && Now() - m_fPhaseAt_ms < 10000.0)
				return ENodeResult.RUNNING;
			Abort("plan_" + why);
			return ENodeResult.FAIL;
		}
		m_Nav.FillRooms(m_aRooms);
		m_vEntry = m_Nav.EntryPos();
		m_fDCOStackFireAt_ms = 0;
		m_bStackOrdered = false;
		vector outv = TeamCenter() - m_vEntry;
		outv[1] = 0;
		if (outv.LengthSq() < 0.01)
			outv = Vector(1, 0, 0);
		m_vOut = outv.Normalized();
		vector side = Vector(m_vOut[2], 0, -m_vOut[0]);
		m_vStackA = DCO_Util.Surface(m_vEntry + m_vOut * 1.5 + side * 1.2);
		m_vStackB = DCO_Util.Surface(m_vEntry + m_vOut * 1.5 - side * 1.2);
		m_Nav.ApplyEntry(m_vOut, m_vStackA, m_vStackB, m_vDoor);
		float total = 120000.0 + m_fRoomLimit_ms * m_aRooms.Count();
		if (m_bPlanned)
			total += 120000.0;
		m_fDeadline_ms = m_fStart_ms + Math.Min(total, 1200000.0);
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_plan grp=%1 building=%2 rooms=%3 entry=%4 pairs=%5 rear=%6 deadline_s=%7 order=random", Grp(), DCO_Util.PrefabShort(m_Building), m_aRooms.Count(), DCO_Util.XZ(m_vEntry), PairCount(), m_aRear.Count(), Math.Round((m_fDeadline_ms - m_fStart_ms) * 0.001)));
		PlanSecond();
		OrderRear();
		SetPhase(DCO_ECQBPhase.STACK);
		return ENodeResult.SUCCESS;
	}

	protected void PlanSecond()
	{
		m_bTwoDoor = false;
		m_bSecondSent = false;
		m_bSecondUsed = false;
		if (m_aEntry.Count() < 4 || DCO_Ctx.TuneG("cqb.two_door", 1.0) <= 0)
			return;
		vector d2, o2, a2, b2;
		if (!m_Nav.SecondEntry(TeamCenter(), d2, o2, a2, b2))
		{
			DCO_Perf.Count("cqb:second_entry_none");
			return;
		}
		vector side2 = Vector(o2[2], 0, -o2[0]);
		if (a2 == vector.Zero)
			a2 = DCO_Util.Surface(d2 + o2 * 0.7 + side2 * 1.2);
		if (b2 == vector.Zero)
			b2 = DCO_Util.Surface(d2 + o2 * 0.7 - side2 * 1.2);
		m_bTwoDoor = true;
		m_vEntry2 = d2;
		m_vOut2 = o2;
		m_vStack2A = a2;
		m_vStack2B = b2;
		DCO_Perf.Count("cqb:second_entry");
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_second_entry grp=%1 door=%2 from_entry_m=%3 pairs=%4", Grp(), DCO_Util.XZ(d2), Math.Round(vector.DistanceXZ(d2, m_vEntry)), PairCount()));
	}

	protected void OrderSecond()
	{
		m_bSecondSent = true;
		vector in2 = m_vOut2 * -1.0;
		vector side2 = Vector(in2[2], 0, -in2[0]);
		vector spot = m_vEntry2 + in2 * 3.2;
		vector look = m_vEntry2 + in2 * 7.0 + Vector(0, 1.2, 0);
		int sent = 0;
		for (int k = 0; k < 2; k++)
		{
			SCR_ChimeraAIAgent s = PairMember(1, k);
			if (!s || !OrdArrived(s))
				continue;
			float sign = 1.0;
			if (k == 1)
				sign = -1.0;
			Order(s, new DCO_CQBEnterBehavior(s.m_UtilityComponent, this, spot + side2 * (0.8 * sign), look, ECharacterStance.STAND, EMovementType.RUN, 90.0, "no" + (k + 1).ToString()));
			sent++;
		}
		DCO_Perf.Count("cqb:second_breach");
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_second_breach grp=%1 door=%2 men=%3", Grp(), DCO_Util.XZ(m_vEntry2), sent));
	}

	protected bool SecondTeam(SCR_ChimeraAIAgent a)
	{
		if (!m_bTwoDoor || m_bSecondUsed || ActivePair() != 0)
			return false;
		return PairMember(1, 0) == a || PairMember(1, 1) == a;
	}

	protected vector PairCenter()
	{
		vector a = Pos(PairMember(ActivePair(), 0));
		vector b = Pos(PairMember(ActivePair(), 1));
		if (a == vector.Zero)
			return b;
		if (b == vector.Zero)
			return a;
		return (a + b) * 0.5;
	}

	protected bool InRoom(DCO_CQBRoom room, vector p)
	{
		if (p == vector.Zero || Math.AbsFloat(p[1] - room.m_vCenter[1]) > 1.8)
			return false;
		if (vector.DistanceXZ(p, room.m_vCenter) <= 4.0)
			return true;
		foreach (vector s : room.m_aSlots)
		{
			if (vector.DistanceXZ(p, s) <= 2.5)
				return true;
		}
		return false;
	}

	protected bool PairInside(DCO_CQBRoom room)
	{
		SCR_ChimeraAIAgent p1 = PairMember(ActivePair(), 0);
		SCR_ChimeraAIAgent p2 = PairMember(ActivePair(), 1);
		if (!p1 || !InRoom(room, Pos(p1)))
			return false;
		return !p2 || InRoom(room, Pos(p2));
	}

	protected void OrderRear()
	{
		vector side = Vector(m_vOut[2], 0, -m_vOut[0]);
		array<vector> exits = {};
		array<vector> exitDirs = {};
		array<int> usedExits = {};
		if (m_Nav && DCO_Ctx.TuneG("cqb.rear_exit", 1.0) > 0)
			m_Nav.Exits(exits, exitDirs);
		foreach (int i, SCR_ChimeraAIAgent r : m_aRear)
		{
			float sign = 1.0;
			if (i % 2 == 1)
				sign = -1.0;
			vector look = m_vEntry + Vector(0, 1.2, 0);
			vector spot = DCO_Util.Surface(m_vEntry + m_vOut * 10.0 + side * (6.0 * sign));
			int pickExit = -1;
			float farExit = -1;
			foreach (int xi, vector xp : exits)
			{
				if (usedExits.Contains(xi) || (m_bTwoDoor && vector.DistanceXZ(xp, m_vEntry2) < 4.0))
					continue;
				float xd = vector.DistanceXZ(xp, m_vEntry);
				if (xd > farExit)
				{
					farExit = xd;
					pickExit = xi;
				}
			}
			if (pickExit >= 0)
			{
				usedExits.Insert(pickExit);
				spot = DCO_Util.Surface(exits[pickExit] + exitDirs[pickExit] * 10.0);
				look = exits[pickExit] + Vector(0, 1.2, 0);
				DCO_Perf.Count("cqb:rear_exit");
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_rear_exit grp=%1 unit=%2 exit=%3 from_entry_m=%4 spot=%5", Grp(), DCO_Log.IdUnit(r.GetControlledEntity()), DCO_Util.XZ(exits[pickExit]), Math.Round(farExit), DCO_Util.XZ(spot)));
			}
			m_mSpot.Set(r, spot);
			Order(r, new DCO_CQBSupportBehavior(r.m_UtilityComponent, this, spot, look, ECharacterStance.CROUCH, EMovementType.RUN, 1200.0, "rear_" + (i + 1).ToString()));
		}
	}

	ENodeResult StepStack()
	{
		if (!Guard())
			return ENodeResult.FAIL;
		if (m_ePhase != DCO_ECQBPhase.STACK)
			return ENodeResult.SUCCESS;
		if (!m_bStackOrdered)
		{
			m_bStackOrdered = true;
			foreach (int i, SCR_ChimeraAIAgent a : m_aEntry)
				OrderStackSlot(i);
			m_fProgAt_ms = Now() + 30000.0;
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_stack grp=%1 entry=%2 stack_a=%3 stack_b=%4", Grp(), DCO_Util.XZ(m_vEntry), DCO_Util.XZ(m_vStackA), DCO_Util.XZ(m_vStackB)));
		}
		SCR_ChimeraAIAgent p1 = PairMember(0, 0);
		SCR_ChimeraAIAgent p2 = PairMember(0, 1);
		int at = 0;
		if (OrdArrived(p1))
			at++;
		if (OrdArrived(p2))
			at++;
		int need = 1;
		if (p2)
			need = 2;
		float now = Now();
		float age = now - m_fPhaseAt_ms;
		float limit = 90000.0;
		if (m_bPlanned)
			limit = 180000.0;
		if (m_fNearAt_ms < 0 && NearEntry(40.0))
			m_fNearAt_ms = now;
		if (at >= 1 && m_fFirstAt_ms < 0)
			m_fFirstAt_ms = now;
		float waited = 0;
		if (m_fFirstAt_ms >= 0)
			waited = now - m_fFirstAt_ms;
		StackProgress(now, at, need);
		foreach (int ri, SCR_ChimeraAIAgent ra : m_aEntry)
		{
			DCO_CQBMemberBehavior rb = Ord(ra);
			if (!rb || !rb.IsDone() || rb.Why() != "timeout")
				continue;
			OrderStackSlot(ri);
			DCO_Perf.Count("cqb:stack_reorder");
		}
		bool swapNow = at == 1 && need == 2 && !m_bSwapTried && waited > 2000.0;
		if (swapNow)
		{
			m_bSwapTried = true;
			if (SwapPartner(m_vEntry, false))
				return ENodeResult.RUNNING;
		}
		bool go = at >= need || (at >= 1 && waited > m_fStackWait_ms);
		if (go && m_bTwoDoor)
		{
			if (m_fReady0_ms < 0)
				m_fReady0_ms = now;
			bool second = OrdArrived(PairMember(1, 0)) || OrdArrived(PairMember(1, 1));
			if (!second && now - m_fReady0_ms < 15000.0)
				go = false;
			else if (!second)
			{
				m_bTwoDoor = false;
				DCO_Perf.Count("cqb:second_entry_cancel");
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_second_entry_cancel grp=%1 why=not_arrived waited_s=15", Grp()));
			}
		}
		if (go)
		{
			SetPhase(DCO_ECQBPhase.ROOMS);
			SetStep(0);
			return ENodeResult.SUCCESS;
		}
		if (m_fDCOStackFireAt_ms > 0 && age > 30000.0 && !m_bDCOStackSwitched && at < need)
		{
			m_bDCOStackSwitched = true;
			m_aTried.Insert(m_vEntry);
			DCO_Perf.Count("cqb:stack_switch");
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_stack_switch grp=%1 from=%2 why=under_fire age_s=%3 arrived=%4", Grp(), DCO_Util.XZ(m_vEntry), Math.Round(age * 0.001), at));
			SetPhase(DCO_ECQBPhase.PLAN);
			SetStep(0);
			return ENodeResult.SUCCESS;
		}
		if (m_fNearAt_ms >= 0 && now - m_fNearAt_ms > limit)
		{
			m_aTried.Insert(m_vEntry);
			Abort("stack_timeout");
			return ENodeResult.FAIL;
		}
		return ENodeResult.RUNNING;
	}

	protected void OrderStackSlot(int i)
	{
		SCR_ChimeraAIAgent a = m_aEntry[i];
		if (!a)
			return;
		vector slot = m_vStackA;
		if (i % 2 == 1)
			slot = m_vStackB;
		slot = slot + m_vOut * (1.2 * (i / 2));
		vector look = m_vEntry + Vector(0, 1.2, 0);
		if (m_bTwoDoor && i / 2 == 1)
		{
			slot = m_vStack2A;
			if (i % 2 == 1)
				slot = m_vStack2B;
			look = m_vEntry2 + Vector(0, 1.2, 0);
		}
		Order(a, new DCO_CQBMoveSlotBehavior(a.m_UtilityComponent, this, slot, look, ECharacterStance.STAND, EMovementType.RUN, 240.0, "stack_" + (i + 1).ToString()));
	}

	protected bool NearEntry(float r)
	{
		foreach (SCR_ChimeraAIAgent a : m_aEntry)
		{
			if (a && a.GetControlledEntity() && vector.DistanceXZ(Pos(a), m_vEntry) <= r)
				return true;
		}
		return false;
	}

	protected void StackProgress(float now, int at, int need)
	{
		if (now < m_fProgAt_ms)
			return;
		m_fProgAt_ms = now + 30000.0;
		string men = "";
		foreach (SCR_ChimeraAIAgent a : m_aEntry)
		{
			if (!a || !a.GetControlledEntity())
				continue;
			string act = "-";
			if (a.m_UtilityComponent && a.m_UtilityComponent.GetCurrentAction())
				act = a.m_UtilityComponent.GetCurrentAction().ClassName();
			if (!men.IsEmpty())
				men += ",";
			men += string.Format("%1:%2:%3", DCO_Log.IdUnit(a.GetControlledEntity()), Math.Round(vector.Distance(Pos(a), m_vEntry)), act);
		}
		float clock = -1;
		if (m_fNearAt_ms >= 0)
			clock = Math.Round((now - m_fNearAt_ms) * 0.001);
		DCO_Perf.Count("cqb:stack_wait_log");
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_stack_wait grp=%1 age_s=%2 clock_s=%3 arrived=%4/%5 under_fire=%6 men=%7", Grp(), Math.Round((now - m_fPhaseAt_ms) * 0.001), clock, at, need, DCO_Util.B01(m_fDCOStackFireAt_ms > 0), men));
	}

	protected bool SwapPartner(vector door, bool room)
	{
		int b0 = ActivePair() * 2;
		SCR_ChimeraAIAgent p1 = PairMember(ActivePair(), 0);
		SCR_ChimeraAIAgent p2 = PairMember(ActivePair(), 1);
		if (!p1 || !p2)
			return false;
		int far = b0 + 1;
		if (!OrdArrived(p1))
			far = b0;
		float farD = vector.Distance(Pos(m_aEntry[far]), door);
		if (farD <= 10.0)
			return false;
		int best = -1;
		float bestD = farD - 3.0;
		foreach (int i, SCR_ChimeraAIAgent a : m_aEntry)
		{
			if (i == b0 || i == b0 + 1 || !a)
				continue;
			float d = vector.Distance(Pos(a), door);
			if (d < bestD)
			{
				bestD = d;
				best = i;
			}
		}
		if (best < 0)
			return false;
		SCR_ChimeraAIAgent outA = m_aEntry[far];
		SCR_ChimeraAIAgent inA = m_aEntry[best];
		m_aEntry[far] = inA;
		m_aEntry[best] = outA;
		DCO_Perf.Count("cqb:pair_swap");
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_pair_swap grp=%1 room=%2 out=%3 out_dist=%4 in=%5 in_dist=%6", Grp(), m_iRoom, DCO_Log.IdUnit(outA.GetControlledEntity()), Math.Round(farD), DCO_Log.IdUnit(inA.GetControlledEntity()), Math.Round(bestD)));
		if (!room)
		{
			OrderStackSlot(far);
			OrderStackSlot(best);
			return true;
		}
		vector look = m_vDoor + m_vDir * 2.0 + Vector(0, 1.2, 0);
		vector spot = m_vStackB;
		string role = "room_stack_2";
		if (far == b0)
		{
			spot = m_vStackA;
			role = "room_stack_1";
		}
		Order(inA, new DCO_CQBMoveSlotBehavior(inA.m_UtilityComponent, this, spot, look, ECharacterStance.STAND, EMovementType.RUN, 40.0, role));
		DCO_CQBFollowBehavior f = new DCO_CQBFollowBehavior(outA.m_UtilityComponent, this, Pos(outA), m_vDoor + Vector(0, 1.2, 0), ECharacterStance.STAND, EMovementType.RUN, 120.0, "snake_swap");
		f.SetLead(inA.GetControlledEntity(), 3.0);
		Order(outA, f);
		return true;
	}

	protected bool LoneHold(float now)
	{
		if (m_aEntry.Count() != 1 || PendingRooms() == 0)
		{
			m_fLoneAt_ms = -1;
			return false;
		}
		array<AIAgent> agents = {};
		m_Utility.m_Owner.GetAgents(agents);
		foreach (AIAgent ag : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(ag);
			if (!ca || m_aEntry.Contains(ca) || m_aRear.Contains(ca) || !Usable(m_Utility, ag))
				continue;
			m_aEntry.Insert(ca);
			m_aLost.RemoveItem(ca);
			DCO_Perf.Count("cqb:rejoin");
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_rejoin grp=%1 unit=%2 entry=%3", Grp(), DCO_Log.IdUnit(ca.GetControlledEntity()), m_aEntry.Count()));
		}
		if (m_aEntry.Count() > 1)
		{
			m_fLoneAt_ms = -1;
			if (m_iStep == 0)
				return false;
			OrderStack();
			SetStep(1);
			return true;
		}
		SCR_ChimeraAIAgent last = m_aEntry[0];
		if (m_fLoneAt_ms < 0)
		{
			m_fLoneAt_ms = now;
			DCO_Perf.Count("cqb:lone_hold");
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_lone_hold grp=%1 room=%2 unit=%3 pending=%4 max_s=60", Grp(), m_iRoom, DCO_Log.IdUnit(last.GetControlledEntity()), PendingRooms()));
			Order(last, new DCO_CQBHoldBehavior(last.m_UtilityComponent, this, m_vStackA, m_vDoor + Vector(0, 1.2, 0), ECharacterStance.CROUCH, EMovementType.RUN, 70.0, "lone_hold"));
		}
		if (now - m_fLoneAt_ms > 60000.0)
		{
			Abort("lone");
			return true;
		}
		m_fRoomAt_ms = now;
		return true;
	}

	protected void ReorderStuck(SCR_ChimeraAIAgent p, int k)
	{
		DCO_CQBMemberBehavior ob = Ord(p);
		if (!ob || !ob.IsDone() || ob.Why() != "stuck" || m_iReorders >= 2)
			return;
		m_iReorders++;
		vector look = m_vDoor + m_vDir * 2.0 + Vector(0, 1.2, 0);
		vector goal = m_vDoor + m_vDir * 2.0;
		string role = "no" + (k + 1).ToString();
		if (m_iStep == 1)
		{
			goal = m_vStackA;
			if (k == 1)
				goal = m_vStackB;
			goal = goal - m_vDir * 1.0;
			role = "room_stack_" + (k + 1).ToString();
			Order(p, new DCO_CQBMoveSlotBehavior(p.m_UtilityComponent, this, goal, look, ECharacterStance.STAND, EMovementType.RUN, 40.0, role));
		}
		else
			Order(p, new DCO_CQBEnterBehavior(p.m_UtilityComponent, this, goal, look, ECharacterStance.STAND, EMovementType.RUN, 14.0, role));
		DCO_Perf.Count("cqb:reorder_stuck");
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_reorder grp=%1 room=%2 unit=%3 role=%4 step=%5 try=%6 goal=%7 why=stuck", Grp(), m_iRoom, DCO_Log.IdUnit(p.GetControlledEntity()), role, m_iStep, m_iReorders, DCO_Util.XZ(goal)));
	}

	protected bool TryCover(SCR_ChimeraAIAgent p1, SCR_ChimeraAIAgent p2, DCO_CQBRoom room)
	{
		if (m_bCoverFired)
			return false;
		m_bCoverFired = true;
		vector aim = room.m_vCenter;
		bool fresh = Now() - m_fEnemyAt_ms < 15000.0 && m_vEnemy != vector.Zero;
		if (fresh && vector.Distance(m_vEnemy, room.m_vCenter) < 6.0)
			aim = m_vEnemy;
		array<SCR_ChimeraAIAgent> men = {p2, p1};
		foreach (SCR_ChimeraAIAgent c : men)
		{
			if (!c || !c.m_UtilityComponent || !c.GetControlledEntity() || !OrdArrived(c))
				continue;
			if (!DCO_SuppressGate.Allowed(c.m_UtilityComponent, aim, "cqb_cover"))
				continue;
			vector bbMin, bbMax;
			SCR_AISuppressionVolumeBase.CreateSuppressionBox(aim, 1.5, 2, bbMin, bbMax);
			SCR_AISuppressBehavior cover = new SCR_AISuppressBehavior(c.m_UtilityComponent, null, new SCR_AISuppressionObjectVolumeBox(bbMin, bbMax), 2.0, 1.5);
			cover.SetPriority(96);
			c.m_UtilityComponent.AddAction(cover);
			m_fPrepUntil_ms = Now() + 2500.0;
			DCO_Perf.Count("cqb:cover_fire");
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_cover_fire grp=%1 room=%2 unit=%3 target=%4 dist=%5 secs=2", Grp(), m_iRoom, DCO_Log.IdUnit(c.GetControlledEntity()), DCO_Util.XZ(aim), vector.Distance(Pos(c), aim).ToString(-1, 1)));
			return true;
		}
		DCO_Perf.Count("cqb:cover_skip");
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_cover_skip grp=%1 room=%2 reason=no_line target=%3", Grp(), m_iRoom, DCO_Util.XZ(aim)));
		return false;
	}

	protected void OrderStack()
	{
		SCR_ChimeraAIAgent p1 = PairMember(ActivePair(), 0);
		SCR_ChimeraAIAgent p2 = PairMember(ActivePair(), 1);
		vector look = m_vDoor + m_vDir * 2.0 + Vector(0, 1.2, 0);
		vector sa = m_vStackA;
		vector sb = m_vStackB;
		if (p1)
			Order(p1, new DCO_CQBMoveSlotBehavior(p1.m_UtilityComponent, this, sa, look, ECharacterStance.STAND, EMovementType.RUN, 40.0, "room_stack_1"));
		if (p2)
			Order(p2, new DCO_CQBMoveSlotBehavior(p2.m_UtilityComponent, this, sb, look, ECharacterStance.STAND, EMovementType.RUN, 40.0, "room_stack_2"));
		if (p2 && m_Nav.HopStairs() && Ord(p2))
		{
			Ord(p2).SetStartDelay(2000.0);
			DCO_Perf.Count("cqb:stairs_file");
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_stairs_file grp=%1 room=%2 second=%3 delay_ms=2000", Grp(), m_iRoom, DCO_Log.IdUnit(p2.GetControlledEntity())));
		}
		SCR_ChimeraAIAgent leadAgent = p2;
		if (!leadAgent)
			leadAgent = p1;
		IEntity lead = null;
		if (leadAgent)
			lead = leadAgent.GetControlledEntity();
		int followers = 0;
		foreach (SCR_ChimeraAIAgent a : m_aEntry)
		{
			if (InActivePair(a) || m_aRear.Contains(a) || !lead)
				continue;
			if (SecondTeam(a))
				continue;
			DCO_CQBBreachBehavior br = DCO_CQBBreachBehavior.Cast(Ord(a));
			if (br && !br.IsDone())
				continue;
			followers++;
			DCO_CQBFollowBehavior f = new DCO_CQBFollowBehavior(a.m_UtilityComponent, this, Pos(a), m_vDoor + Vector(0, 1.2, 0), ECharacterStance.STAND, EMovementType.RUN, 120.0, "snake_" + followers.ToString());
			f.SetLead(lead, 3.0);
			Order(a, f);
			lead = a.GetControlledEntity();
		}
		if (followers > 0)
		{
			DCO_Perf.Count("cqb:snake");
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_snake grp=%1 room=%2 lead=%3 followers=%4", Grp(), m_iRoom, DCO_Log.IdUnit(leadAgent.GetControlledEntity()), followers));
		}
	}

	ENodeResult StepRooms()
	{
		if (!Guard())
			return ENodeResult.FAIL;
		if (m_ePhase != DCO_ECQBPhase.ROOMS)
			return ENodeResult.SUCCESS;
		float now = Now();
		DCO_UpdateBelief(now);
		if (m_iStep <= 1 && LoneHold(now))
		{
			if (m_bFinished)
				return ENodeResult.FAIL;
			return ENodeResult.RUNNING;
		}
		if (m_iStep > 0 && m_iRoom >= 0 && now - m_fRoomAt_ms > m_fRoomLimit_ms)
		{
			FinishRoom("timeout");
			return ENodeResult.RUNNING;
		}
		SCR_ChimeraAIAgent p1 = PairMember(ActivePair(), 0);
		SCR_ChimeraAIAgent p2 = PairMember(ActivePair(), 1);
		float age = now - m_fStepAt_ms;
		if (m_iStep == 1 || m_iStep == 3)
		{
			ReorderStuck(p1, 0);
			ReorderStuck(p2, 1);
		}
		switch (m_iStep)
		{
			case 0:
			{
				int prio = DCO_PriorityRoom();
				int second = -1;
				if (m_bTwoDoor && m_bSecondSent && !m_bSecondUsed && ActivePair() == 1)
					second = m_Nav.SecondRoom();
				if (second >= 0 && second < m_aRooms.Count() && m_aRooms[second].m_iState == 0)
					prio = second;
				else
					second = -1;
				if (m_bSecondSent && ActivePair() == 1)
					m_bSecondUsed = true;
				int idx = m_Nav.Pick(m_aRooms, m_iFoothold, prio, PairCenter());
				if (idx < 0)
				{
					string why = "all_rooms";
					if (idx == -2)
						why = "rest_unreachable";
					DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_rooms_end grp=%1 reason=%2 cleared=%3 timeouts=%4 rooms=%5", Grp(), why, m_iCleared, m_iTimeouts, m_aRooms.Count()));
					if (why == "all_rooms" && m_iResweeps < 1 && m_fEvidenceAt_ms >= 0 && now - m_fEvidenceAt_ms < 30000.0)
					{
						int evRoom = DCO_RoomNearestAny(m_vEvidence);
						int reopened = 0;
						foreach (DCO_CQBRoom rr : m_aRooms)
						{
							if (evRoom >= 0 && rr.m_iFloor != m_aRooms[evRoom].m_iFloor)
								continue;
							rr.m_iState = 0;
							reopened++;
						}
						m_iResweeps++;
						DCO_Perf.Count("cqb:resweep");
						DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_resweep grp=%1 reopened=%2 evidence_age_s=%3", Grp(), reopened, Math.Round((now - m_fEvidenceAt_ms) * 0.001)));
						if (reopened > 0)
							return ENodeResult.RUNNING;
					}
					SetPhase(DCO_ECQBPhase.CONSOLIDATE);
					return ENodeResult.SUCCESS;
				}
				m_iRoom = idx;
				m_fRoomAt_ms = now;
				m_bCoverFired = false;
				DCO_CQBRoom room = m_aRooms[idx];
				vector door = room.m_vCenter;
				vector dir = room.m_vCenter - TeamCenter();
				dir[1] = 0;
				if (dir.LengthSq() < 0.01)
					dir = Vector(0, 0, 1);
				dir.Normalize();
				vector sa, sb;
				bool real = m_Nav.Approach(idx, door, dir, sa, sb);
				if (!real)
					door = room.m_vCenter - dir * 3.0;
				m_vDoor = door;
				m_vDir = dir;
				vector side = Vector(dir[2], 0, -dir[0]);
				if (sa == vector.Zero)
					sa = DCO_Util.Surface(door - dir * 0.6 + side * 0.9);
				if (sb == vector.Zero)
					sb = DCO_Util.Surface(door - dir * 0.6 - side * 0.9);
				m_vStackA = sa;
				m_vStackB = sb;
				bool inRoom = second >= 0 && idx == second && PairInside(room);
				if (!inRoom)
					OrderStack();
				if (DCO_RoomBelief(idx) >= 0.5)
					OrderContain(idx);
				DCO_Perf.Count("cqb:room_pick");
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_room_pick grp=%1 room=%2 floor=%3 pair=%4 door=%5 graph_door=%6 slots=%7 left=%8", Grp(), idx, room.m_iFloor, ActivePair() + 1, DCO_Util.XZ(door), DCO_Util.B01(real), room.m_aSlots.Count(), PendingRooms()));
				if (inRoom)
				{
					DCO_Perf.Count("cqb:room_already_inside");
					DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_room_inside grp=%1 room=%2 pair=%3 why=second_door", Grp(), idx, ActivePair() + 1));
					OrderEnter();
					SetStep(3);
					return ENodeResult.RUNNING;
				}
				vector seen;
				bool quiet = now - m_fEnemyAt_ms >= 15000.0 && DCO_RoomBelief(idx) < 0.5;
				bool together = !p2 || vector.Distance(Pos(p1), Pos(p2)) <= 5.0;
				if (!together)
					DCO_Perf.Count("cqb:flow_skip_apart");
				bool flow = real && m_iBreach != 1 && quiet && together && !EnemyInRoom(room, seen);
				if (flow && OrderFlow())
				{
					SetStep(3);
					return ENodeResult.RUNNING;
				}
				SetStep(1);
				return ENodeResult.RUNNING;
			}
			case 1:
			{
				int at = 0;
				if (OrdArrived(p1))
					at++;
				if (OrdArrived(p2))
					at++;
				int need = 1;
				if (p2)
					need = 2;
				if (at >= 1 && m_fFirstAt_ms < 0)
					m_fFirstAt_ms = now;
				if (at == 0 && age < 30000.0)
					return ENodeResult.RUNNING;
				if (at >= 1 && at < need)
				{
					float waited = now - m_fFirstAt_ms;
					if (!m_bSwapTried && waited > 2000.0)
					{
						m_bSwapTried = true;
						SwapPartner(m_vDoor, true);
					}
					if (waited < 20000.0)
						return ENodeResult.RUNNING;
				}
				DCO_CQBRoom troom = m_aRooms[m_iRoom];
				vector where;
				bool enemy = EnemyInRoom(troom, where) || now - m_fEnemyAt_ms < 15000.0 || DCO_RoomBelief(m_iRoom) >= 0.5;
				bool frag = m_iBreach == 1 || (m_iBreach == 0 && enemy);
				if (!frag && enemy)
					DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_frag_skip grp=%1 room=%2 reason=gm_breach_mode mode=%3", Grp(), m_iRoom, m_iBreach));
				if (frag && m_iFrags < 3 && TryFrag(p1, p2, troom))
				{
					SetStep(2);
					return ENodeResult.RUNNING;
				}
				if (enemy && TryCover(p1, p2, troom))
				{
					SetStep(2);
					return ENodeResult.RUNNING;
				}
				OrderEnter();
				SetStep(3);
				return ENodeResult.RUNNING;
			}
			case 2:
			{
				bool busy = false;
				foreach (SCR_ChimeraAIAgent b : m_aEntry)
				{
					DCO_CQBBreachBehavior br = DCO_CQBBreachBehavior.Cast(Ord(b));
					if (br && !br.IsDone())
						busy = true;
				}
				if (busy && age < 12000.0)
					return ENodeResult.RUNNING;
				if (now < m_fPrepUntil_ms)
					return ENodeResult.RUNNING;
				if (m_fPrepUntil_ms <= 0)
					DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_frag_done grp=%1 room=%2 ms=%3", Grp(), m_iRoom, Math.Round(age)));
				m_fPrepUntil_ms = 0;
				if (m_bFragOther)
				{
					m_bFragOther = false;
					OrderStack();
				}
				OrderEnter();
				SetStep(3);
				return ENodeResult.RUNNING;
			}
			case 3:
			{
				int inside = 0;
				if (OrdArrived(p1))
					inside++;
				if (OrdArrived(p2))
					inside++;
				int needIn = 1;
				if (p2)
					needIn = 2;
				if (inside < needIn && age < 15000.0)
					return ENodeResult.RUNNING;
				if (inside == 0)
				{
					if (m_iEnterTry < 1)
					{
						m_iEnterTry++;
						DCO_Perf.Count("cqb:enter_retry");
						DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_enter_retry grp=%1 room=%2 pair=%3 goal=door_inside", Grp(), m_iRoom, ActivePair() + 1));
						OrderEnter();
						SetStep(3);
						return ENodeResult.RUNNING;
					}
					FinishRoom("no_entry");
					return ENodeResult.RUNNING;
				}
				vector look1, look2;
				LookPair(look1, look2);
				if (p1)
					Order(p1, new DCO_CQBHoldBehavior(p1.m_UtilityComponent, this, Pos(p1), look1, ECharacterStance.STAND, EMovementType.WALK, 90.0, "sweep_1"));
				if (p2)
					Order(p2, new DCO_CQBHoldBehavior(p2.m_UtilityComponent, this, Pos(p2), look2, ECharacterStance.STAND, EMovementType.WALK, 90.0, "sweep_2"));
				m_fSweepUntil_ms = now + 1500.0;
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_room_sweep grp=%1 room=%2 inside=%3/%4 ms=%5", Grp(), m_iRoom, inside, needIn, Math.Round(age)));
				SetStep(4);
				return ENodeResult.RUNNING;
			}
			case 4:
			{
				if (now < m_fSweepUntil_ms)
					return ENodeResult.RUNNING;
				vector stillAt;
				if (EnemyInRoom(m_aRooms[m_iRoom], stillAt) && age < 20000.0)
					return ENodeResult.RUNNING;
				FinishRoom("cleared");
				return ENodeResult.RUNNING;
			}
		}
		return ENodeResult.RUNNING;
	}

	protected int PendingRooms()
	{
		int n = 0;
		foreach (DCO_CQBRoom r : m_aRooms)
		{
			if (r.m_iState == 0)
				n++;
		}
		return n;
	}

	protected bool TryFrag(SCR_ChimeraAIAgent p1, SCR_ChimeraAIAgent p2, DCO_CQBRoom room)
	{
		vector aim = room.m_vCenter + Vector(0, 0.5, 0);
		if (Now() - m_fEnemyAt_ms < 15000.0 && m_vEnemy != vector.Zero)
			aim = m_vEnemy + Vector(0, 0.3, 0);
		array<SCR_ChimeraAIAgent> throwers = {p1, p2};
		foreach (SCR_ChimeraAIAgent o : m_aEntry)
		{
			if (o != p1 && o != p2)
				throwers.Insert(o);
		}
		int armed = 0;
		foreach (SCR_ChimeraAIAgent t : throwers)
		{
			if (!t || !t.m_UtilityComponent || !t.m_UtilityComponent.m_CombatComponent || !t.GetControlledEntity())
				continue;
			if (!t.m_UtilityComponent.m_CombatComponent.HasWeaponOfType(EWeaponType.WT_FRAGGRENADE))
				continue;
			armed++;
			float d = vector.Distance(Pos(t), aim);
			if (d < 2.0 || d > 15.0)
				continue;
			Order(t, new DCO_CQBBreachBehavior(t.m_UtilityComponent, this, Pos(t), aim, ECharacterStance.CROUCH, EMovementType.RUN, 12.0, "breach"));
			m_bFragOther = t != p1 && t != p2;
			m_iFrags++;
			DCO_Perf.Count("cqb:frag");
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_frag grp=%1 room=%2 unit=%3 dist=%4 frags=%5", Grp(), m_iRoom, DCO_Log.IdUnit(t.GetControlledEntity()), d.ToString(-1, 1), m_iFrags));
			return true;
		}
		string skipWhy = "no_grenade";
		if (armed > 0)
			skipWhy = "out_of_range";
		DCO_Perf.Count("cqb:frag_skip_" + skipWhy);
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_frag_skip grp=%1 room=%2 reason=%3 candidates=%4 armed=%5", Grp(), m_iRoom, skipWhy, throwers.Count(), armed));
		return false;
	}

	protected bool EntryCorners(out vector left, out vector right)
	{
		DCO_CQBRoom room = m_aRooms[m_iRoom];
		vector side = Vector(m_vDir[2], 0, -m_vDir[0]);
		vector gl, gr;
		bool graph = m_Nav.EntryPicks(m_iRoom, room, gl, gr);
		if (graph)
		{
			left = gl;
			right = gr;
		}
		else
		{
			left = m_vDoor + m_vDir * 2.4 + side * 0.8;
			right = m_vDoor + m_vDir * 2.4 - side * 0.8;
		}
		return graph;
	}

	bool WallSpot(vector near, float r, out vector spot)
	{
		spot = vector.Zero;
		return m_Nav && m_Nav.WallSpot(near, r, spot);
	}

	protected void LookPair(out vector l1, out vector l2)
	{
		vector base = m_aRooms[m_iRoom].m_vCenter + m_vDir * 2.0 + Vector(0, 1.2, 0);
		l1 = base;
		l2 = base;
		vector a, b;
		int n = m_Nav.ThreatLooks(m_iRoom, m_vDoor, a, b);
		if (n <= 0)
		{
			DCO_Perf.Count("cqb:look_center");
			return;
		}
		l1 = a;
		l2 = b;
		DCO_Perf.Count("cqb:look_threat");
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_look grp=%1 room=%2 threats=%3 p1=%4 p2=%5", Grp(), m_iRoom, n, DCO_Util.XZ(a), DCO_Util.XZ(b)));
	}

	protected bool OrderFlow()
	{
		SCR_ChimeraAIAgent p1 = PairMember(ActivePair(), 0);
		SCR_ChimeraAIAgent p2 = PairMember(ActivePair(), 1);
		DCO_CQBMemberBehavior b1 = Ord(p1);
		DCO_CQBMemberBehavior b2 = Ord(p2);
		if (!b1)
			return false;
		vector left, right;
		bool graph = EntryCorners(left, right);
		vector look1, look2;
		LookPair(look1, look2);
		b1.SetNext(left, look1);
		if (b2)
			b2.SetNext(right, look2);
		DCO_Perf.Count("cqb:flow");
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_flow grp=%1 room=%2 pair=%3 corners=%4 left=%5 right=%6 why=no_threat", Grp(), m_iRoom, ActivePair() + 1, DCO_Util.B01(graph), DCO_Util.XZ(left), DCO_Util.XZ(right)));
		return true;
	}

	protected void OrderEnter()
	{
		SCR_ChimeraAIAgent p1 = PairMember(ActivePair(), 0);
		SCR_ChimeraAIAgent p2 = PairMember(ActivePair(), 1);
		DCO_CQBRoom room = m_aRooms[m_iRoom];
		vector side = Vector(m_vDir[2], 0, -m_vDir[0]);
		vector left, right;
		bool graph = EntryCorners(left, right);
		if (m_iEnterTry > 0)
		{
			left = m_vDoor + m_vDir * 2.0 + side * 0.6;
			right = m_vDoor + m_vDir * 2.0 - side * 0.6;
		}
		vector look1, look2;
		LookPair(look1, look2);
		if (p1)
			Order(p1, new DCO_CQBEnterBehavior(p1.m_UtilityComponent, this, left, look1, ECharacterStance.STAND, EMovementType.RUN, 14.0, "no1"));
		if (p2)
			Order(p2, new DCO_CQBEnterBehavior(p2.m_UtilityComponent, this, right, look2, ECharacterStance.STAND, EMovementType.RUN, 16.0, "no2"));
		if (m_bTwoDoor && !m_bSecondSent && ActivePair() == 0)
			OrderSecond();
		DCO_Perf.Count("cqb:room_enter");
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_room_enter grp=%1 room=%2 pair=%3 corners=%4 left=%5 right=%6", Grp(), m_iRoom, ActivePair() + 1, DCO_Util.B01(graph), DCO_Util.XZ(left), DCO_Util.XZ(right)));
	}

	protected void FinishRoom(string result)
	{
		DCO_CQBRoom room = m_aRooms[m_iRoom];
		float ms = Now() - m_fRoomAt_ms;
		if (result == "no_entry" && room.m_iDefer < 1)
		{
			room.m_iDefer++;
		}
		else if (result == "timeout" || result == "no_entry")
		{
			room.m_iState = 2;
			m_iTimeouts++;
		}
		else
		{
			room.m_iState = 1;
			m_iCleared++;
			m_iFoothold = m_iRoom;
		}
		DCO_Perf.Count("cqb:room_" + result);
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_room grp=%1 room=%2 floor=%3 result=%4 ms=%5 pair=%6 cleared=%7/%8 frags=%9", Grp(), m_iRoom, room.m_iFloor, result, Math.Round(ms), ActivePair() + 1, m_iCleared, m_aRooms.Count(), m_iFrags));
		m_iEnterTry = 0;
		m_iPair++;
		SetStep(0);
	}

	ENodeResult StepConsolidate()
	{
		if (m_bFinished)
			return ENodeResult.SUCCESS;
		if (!Guard())
			return ENodeResult.FAIL;
		float now = Now();
		if (m_fConsolidateAt_ms < 0)
		{
			m_fConsolidateAt_ms = now;
			bool partial = m_Nav && m_Nav.m_iUnreachedFloors > 0 && m_fEvidenceAt_ms >= 0 && now - m_fEvidenceAt_ms < 120000.0;
			DCO_BuildingClear.Finish(m_Building, m_sFaction, m_Utility.m_Owner, !partial, now, 60000.0);
			if (partial)
			{
				DCO_Perf.Count("cqb:partial");
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_partial grp=%1 building=%2 unreached_rooms=%3 unreached_floors=%4 evidence_age_s=%5 retry_s=60", Grp(), DCO_Util.PrefabShort(m_Building), m_Nav.m_iUnreached, m_Nav.m_iUnreachedFloors, Math.Round((now - m_fEvidenceAt_ms) * 0.001)));
			}
			vector look = m_vEntry + m_vOut * 10.0 + Vector(0, 1.2, 0);
			foreach (int i, SCR_ChimeraAIAgent a : m_aEntry)
			{
				if (i == 0)
					Order(a, new DCO_CQBHoldBehavior(a.m_UtilityComponent, this, DCO_Util.Surface(m_vEntry - m_vOut * 1.5), look, ECharacterStance.CROUCH, EMovementType.WALK, 10.0, "hold_door"));
				else
					ClearOrders(a);
			}
			DCO_Perf.Count("cqb:cleared");
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_cleared grp=%1 building=%2 rooms=%3/%4 timeouts=%5 losses=%6 frags=%7 secs=%8 trigger=%9", Grp(), DCO_Util.PrefabShort(m_Building), m_iCleared, m_aRooms.Count(), m_iTimeouts, m_iLosses, m_iFrags, Math.Round((now - m_fStart_ms) * 0.001), m_sTrigger));
			return ENodeResult.RUNNING;
		}
		if (now - m_fConsolidateAt_ms < 10000.0)
			return ENodeResult.RUNNING;
		m_sEndWhy = "cleared";
		m_bFinished = true;
		m_ePhase = DCO_ECQBPhase.DONE;
		StopOrders();
		Complete();
		return ENodeResult.SUCCESS;
	}

	void Abort(string reason)
	{
		if (m_bFinished)
			return;
		m_bFinished = true;
		m_ePhase = DCO_ECQBPhase.ABORT;
		m_sEndWhy = reason;
		float now = Now();
		float retry = 60000.0;
		if (reason.StartsWith("plan_"))
		{
			retry = 600000.0;
			if (m_Utility && m_Utility.m_Owner)
			{
				DCO_GlobalAIComponent rc = DCO_GlobalAIComponent.GetFor(m_Utility.m_Owner);
				if (rc)
					retry = rc.DCO_TuneF("cqb.plan_fail_retry_s", 600.0) * 1000.0;
			}
			DCO_Perf.Count("cqb:plan_fail_long_retry");
		}
		if (m_Building && m_Utility && m_Utility.m_Owner)
			DCO_BuildingClear.Finish(m_Building, m_sFaction, m_Utility.m_Owner, false, now, retry);
		NoteBadPlan(m_Building, reason);
		StopOrders();
		DCO_Perf.Count("cqb:abort:" + reason);
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_abort grp=%1 building=%2 reason=%3 rooms=%4/%5 losses=%6 secs=%7 trigger=%8", Grp(), DCO_Util.PrefabShort(m_Building), reason, m_iCleared, m_aRooms.Count(), m_iLosses, Math.Round((now - m_fStart_ms) * 0.001), m_sTrigger) + string.Format(" retry_s=%1", Math.Round(retry * 0.001)));
		Complete();
	}

	void Cancel(string why)
	{
		Abort(why);
	}

	override void OnActionCompleted()
	{
		if (!DCO_Util.Teardown("cqb_completed"))
			Cleanup();
		super.OnActionCompleted();
	}

	override void OnActionFailed()
	{
		if (!DCO_Util.Teardown("cqb_failed"))
		{
			if (!m_bFinished)
				Abort("failed");
			Cleanup();
		}
		super.OnActionFailed();
	}

	override void OnActionRemoved()
	{
		Cleanup();
		super.OnActionRemoved();
	}

	protected void Cleanup()
	{
		if (!m_bFinished)
		{
			m_bFinished = true;
			StopOrders();
			if (m_Building && m_Utility && m_Utility.m_Owner)
				DCO_BuildingClear.Finish(m_Building, m_sFaction, m_Utility.m_Owner, false, Now(), 120000.0);
			DCO_Perf.Count("cqb:abort:cancel");
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_abort grp=%1 reason=cancel rooms=%2/%3 losses=%4", Grp(), m_iCleared, m_aRooms.Count(), m_iLosses));
		}
		m_mOrd.Clear();
		m_mSpot.Clear();
	}

	override string GetDebugPanelText()
	{
		return string.Format("DCO CQB %1 room=%2 step=%3 cleared=%4/%5 pair=%6/%7", typename.EnumToString(DCO_ECQBPhase, m_ePhase), m_iRoom, m_iStep, m_iCleared, m_aRooms.Count(), ActivePair() + 1, PairCount());
	}
}

class DCO_CQBNodeBase : SCR_AIActionTask
{
	protected DCO_CQBActivity Act()
	{
		DCO_CQBActivity a = DCO_CQBActivity.Cast(GetExecutedAction());
		if (!a || !a.IsRunning())
			return null;
		return a;
	}
}

class DCO_CQB_Plan : DCO_CQBNodeBase
{
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		DCO_CQBActivity a = Act();
		if (!a)
			return ENodeResult.FAIL;
		return a.StepPlan();
	}

	static override bool VisibleInPalette() { return true; }
	static override string GetOnHoverDescription() { return "DCO CQB: builds the plan from the indoor navigation graph (real rooms, outside door, stack points) and sends the rear security out."; }
}

class DCO_CQB_Stack : DCO_CQBNodeBase
{
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		DCO_CQBActivity a = Act();
		if (!a)
			return ENodeResult.FAIL;
		return a.StepStack();
	}

	static override bool VisibleInPalette() { return true; }
	static override string GetOnHoverDescription() { return "DCO CQB: entry pairs stack beside the outside door; done when the first pair is in place."; }
}

class DCO_CQB_Rooms : DCO_CQBNodeBase
{
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		DCO_CQBActivity a = Act();
		if (!a)
			return ENodeResult.FAIL;
		return a.StepRooms();
	}

	static override bool VisibleInPalette() { return true; }
	static override string GetOnHoverDescription() { return "DCO CQB: room by room in pairs. Random next room among rooms next to a cleared one; pair stacks at the real door, frag if an enemy is known inside, #1/#2 to opposite corners, sweep, mark; pairs alternate."; }
}

class DCO_CQB_Consolidate : DCO_CQBNodeBase
{
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		DCO_CQBActivity a = Act();
		if (!a)
			return ENodeResult.FAIL;
		return a.StepConsolidate();
	}

	static override bool VisibleInPalette() { return true; }
	static override string GetOnHoverDescription() { return "DCO CQB: marks the building cleared, one man holds the entry door for 10 s, then the activity completes."; }
}
