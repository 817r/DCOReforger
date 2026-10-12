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
		string name = typename.EnumToString(DCO_ECQBPhase, p);
		name.ToLower();
		DCO_Perf.Count("cqb:phase_" + name);
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_phase grp=%1 phase=%2 room=%3 pair=%4 ms=%5 entry=%6 rear=%7", Grp(), name, m_iRoom, ActivePair() + 1, Math.Round(ms), m_aEntry.Count(), m_aRear.Count()));
	}

	protected void SetStep(int s)
	{
		m_iStep = s;
		m_fStepAt_ms = Now();
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
		if (m_aEntry.IsEmpty() || m_iLosses >= m_iAbortLosses)
		{
			Abort("losses");
			return false;
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
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_pair_reform grp=%1 room=%2 step=%3 pair=%4", Grp(), m_iRoom, m_iStep, ActivePair() + 1));
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
		OrderRear();
		SetPhase(DCO_ECQBPhase.STACK);
		return ENodeResult.SUCCESS;
	}

	protected void OrderRear()
	{
		vector side = Vector(m_vOut[2], 0, -m_vOut[0]);
		vector look = m_vEntry + Vector(0, 1.2, 0);
		foreach (int i, SCR_ChimeraAIAgent r : m_aRear)
		{
			float sign = 1.0;
			if (i % 2 == 1)
				sign = -1.0;
			vector spot = DCO_Util.Surface(m_vEntry + m_vOut * 10.0 + side * (6.0 * sign));
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
			{
				vector slot = m_vStackA;
				if (i % 2 == 1)
					slot = m_vStackB;
				slot = slot + m_vOut * (1.2 * (i / 2));
				EMovementType pace = EMovementType.RUN;
				Order(a, new DCO_CQBMoveSlotBehavior(a.m_UtilityComponent, this, slot, m_vEntry + Vector(0, 1.2, 0), ECharacterStance.STAND, pace, 240.0, "stack_" + (i + 1).ToString()));
			}
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
		float age = Now() - m_fPhaseAt_ms;
		float limit = 90000.0;
		if (m_bPlanned)
			limit = 180000.0;
		if (at >= need || (at >= 1 && age > m_fStackWait_ms))
		{
			SetPhase(DCO_ECQBPhase.ROOMS);
			SetStep(0);
			return ENodeResult.SUCCESS;
		}
		if (age > limit)
		{
			m_aTried.Insert(m_vEntry);
			Abort("stack_timeout");
			return ENodeResult.FAIL;
		}
		return ENodeResult.RUNNING;
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
		vector cover = m_vDoor - m_vDir * 2.5;
		foreach (SCR_ChimeraAIAgent a : m_aEntry)
		{
			if (InActivePair(a) || !OrdIdle(a))
				continue;
			vector stay = Pos(a);
			if (vector.Distance(stay, m_vDoor) > 25.0)
				stay = cover;
			Order(a, new DCO_CQBHoldBehavior(a.m_UtilityComponent, this, stay, m_vDoor + Vector(0, 1.2, 0), ECharacterStance.CROUCH, EMovementType.WALK, 120.0, "security"));
		}
	}

	ENodeResult StepRooms()
	{
		if (!Guard())
			return ENodeResult.FAIL;
		if (m_ePhase != DCO_ECQBPhase.ROOMS)
			return ENodeResult.SUCCESS;
		float now = Now();
		if (m_iStep > 0 && m_iRoom >= 0 && now - m_fRoomAt_ms > m_fRoomLimit_ms)
		{
			FinishRoom("timeout");
			return ENodeResult.RUNNING;
		}
		SCR_ChimeraAIAgent p1 = PairMember(ActivePair(), 0);
		SCR_ChimeraAIAgent p2 = PairMember(ActivePair(), 1);
		float age = now - m_fStepAt_ms;
		switch (m_iStep)
		{
			case 0:
			{
				int idx = m_Nav.Pick(m_aRooms, m_iFoothold, -1);
				if (idx < 0)
				{
					string why = "all_rooms";
					if (idx == -2)
						why = "rest_unreachable";
					DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_rooms_end grp=%1 reason=%2 cleared=%3 timeouts=%4 rooms=%5", Grp(), why, m_iCleared, m_iTimeouts, m_aRooms.Count()));
					SetPhase(DCO_ECQBPhase.CONSOLIDATE);
					return ENodeResult.SUCCESS;
				}
				m_iRoom = idx;
				m_fRoomAt_ms = now;
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
				OrderStack();
				DCO_Perf.Count("cqb:room_pick");
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_room_pick grp=%1 room=%2 floor=%3 pair=%4 door=%5 graph_door=%6 slots=%7 left=%8", Grp(), idx, room.m_iFloor, ActivePair() + 1, DCO_Util.XZ(door), DCO_Util.B01(real), room.m_aSlots.Count(), PendingRooms()));
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
				if (at < need && age < 20000.0)
					return ENodeResult.RUNNING;
				if (at == 0 && age < 30000.0)
					return ENodeResult.RUNNING;
				DCO_CQBRoom troom = m_aRooms[m_iRoom];
				vector where;
				bool enemy = EnemyInRoom(troom, where) || now - m_fEnemyAt_ms < 15000.0;
				bool frag = m_iBreach == 1 || (m_iBreach == 0 && enemy);
				if (!frag && enemy)
					DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_frag_skip grp=%1 room=%2 reason=gm_breach_mode mode=%3", Grp(), m_iRoom, m_iBreach));
				if (frag && m_iFrags < 3 && TryFrag(p1, p2, troom))
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
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_frag_done grp=%1 room=%2 ms=%3", Grp(), m_iRoom, Math.Round(age)));
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
				DCO_CQBRoom sroom = m_aRooms[m_iRoom];
				vector look = sroom.m_vCenter + Vector(0, 1.2, 0);
				if (p1)
					Order(p1, new DCO_CQBHoldBehavior(p1.m_UtilityComponent, this, Pos(p1), look, ECharacterStance.STAND, EMovementType.WALK, 90.0, "sweep_1"));
				if (p2)
					Order(p2, new DCO_CQBHoldBehavior(p2.m_UtilityComponent, this, Pos(p2), look, ECharacterStance.STAND, EMovementType.WALK, 90.0, "sweep_2"));
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
		foreach (SCR_ChimeraAIAgent t : throwers)
		{
			if (!t || !t.m_UtilityComponent || !t.m_UtilityComponent.m_CombatComponent || !t.GetControlledEntity())
				continue;
			if (!t.m_UtilityComponent.m_CombatComponent.HasWeaponOfType(EWeaponType.WT_FRAGGRENADE))
				continue;
			float d = vector.Distance(Pos(t), aim);
			if (d < 2.0 || d > 15.0)
				continue;
			Order(t, new DCO_CQBBreachBehavior(t.m_UtilityComponent, this, Pos(t), aim, ECharacterStance.CROUCH, EMovementType.RUN, 12.0, "breach"));
			m_iFrags++;
			DCO_Perf.Count("cqb:frag");
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_frag grp=%1 room=%2 unit=%3 dist=%4 frags=%5", Grp(), m_iRoom, DCO_Log.IdUnit(t.GetControlledEntity()), d.ToString(-1, 1), m_iFrags));
			return true;
		}
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_frag_skip grp=%1 room=%2 reason=no_thrower", Grp(), m_iRoom));
		return false;
	}

	protected void OrderEnter()
	{
		SCR_ChimeraAIAgent p1 = PairMember(ActivePair(), 0);
		SCR_ChimeraAIAgent p2 = PairMember(ActivePair(), 1);
		DCO_CQBRoom room = m_aRooms[m_iRoom];
		vector side = Vector(m_vDir[2], 0, -m_vDir[0]);
		vector left = room.m_vCenter + side * 1.5;
		vector right = room.m_vCenter - side * 1.5;
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
		if (m_iEnterTry > 0)
		{
			left = m_vDoor + m_vDir * 2.0 + side * 0.6;
			right = m_vDoor + m_vDir * 2.0 - side * 0.6;
		}
		vector look = room.m_vCenter + m_vDir * 2.0 + Vector(0, 1.2, 0);
		if (p1)
			Order(p1, new DCO_CQBEnterBehavior(p1.m_UtilityComponent, this, left, look, ECharacterStance.STAND, EMovementType.RUN, 14.0, "no1"));
		if (p2)
			Order(p2, new DCO_CQBEnterBehavior(p2.m_UtilityComponent, this, right, look, ECharacterStance.STAND, EMovementType.RUN, 16.0, "no2"));
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
			DCO_BuildingClear.Finish(m_Building, m_sFaction, m_Utility.m_Owner, true, now, 0);
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
