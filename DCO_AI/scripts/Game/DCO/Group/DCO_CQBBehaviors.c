class DCO_CQBMemberBehavior : SCR_AIBehaviorBase
{
	protected vector m_vGoal;
	protected vector m_vLook;
	protected ECharacterStance m_eStance;
	protected EMovementType m_ePace;
	protected float m_fStart_ms;
	protected float m_fUntil_ms;
	protected float m_fArrive;
	protected float m_fYieldDist = 40.0;
	protected bool m_bDone;
	protected bool m_bArrived;
	protected bool m_bHoldAfter;
	protected bool m_bYield;
	protected string m_sRole;
	protected string m_sWhy = "-";
	protected ref SCR_AICombatMoveRequest_Move m_Rq;
	protected bool m_bMoving;
	protected float m_fMoveAt_ms;
	protected vector m_vMoveFrom;
	protected int m_iStuck;
	protected float m_fYieldUntil_ms;
	protected float m_fHoldLogAt_ms;
	protected float m_fBlankAt_ms;
	protected bool m_bLeash;
	protected bool m_bReached;
	protected bool m_bBound;
	protected float m_fNextRush_ms;
	protected float m_fNoYieldUntil_ms;
	protected vector m_vYieldPos;
	protected bool m_bYieldPosSet;
	protected float m_fMaxYield_ms;
	protected float m_fYieldSince_ms;
	protected float m_fPreemptLogAt_ms;
	protected bool m_bNoCoverNext;
	protected bool m_bCharge;
	protected vector m_vDetour;
	protected float m_fDetourUntil_ms;
	protected bool m_bDetourShove;
	protected bool m_bRedirect;
	protected vector m_vProgPos;
	protected float m_fProgAt_ms;
	protected float m_fUnblockAt_ms;
	protected int m_iUnblocks;

	protected int m_iRaised;

	protected void RaiseWeapon()
	{
		SCR_CharacterControllerComponent cc = m_Utility.GetCharacterController();
		if (!cc || cc.IsWeaponRaised() || !cc.GetWeaponManagerComponent() || !cc.GetWeaponManagerComponent().GetCurrentWeapon())
			return;
		cc.SetWeaponRaised(true);
		m_iRaised++;
		DCO_Perf.Count("cqb:weapon_raise");
		if (m_iRaised == 1 && DCO_Log.On(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("cqb_weapon_raise unit=%1 role=%2", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), m_sRole));
	}

	bool MakeWay(vector pos, float ms, IEntity asker)
	{
		if (m_bDone || m_fDetourUntil_ms > 0)
			return false;
		float now = GetGame().GetWorld().GetWorldTime();
		m_vDetour = pos;
		m_fDetourUntil_ms = now + ms;
		m_bDetourShove = true;
		m_bRedirect = true;
		m_bMoving = false;
		m_bArrived = false;
		m_fYieldUntil_ms = 0;
		DCO_Perf.Count("cqb:make_way");
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("cqb_make_way unit=%1 role=%2 for=%3 to=%4 ms=%5", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), m_sRole, DCO_Log.IdUnit(asker), DCO_Util.XZ(pos), Math.Round(ms)));
		return true;
	}

	protected bool FreeSpot(vector p, vector me, vector dir, out vector spot)
	{
		NavmeshWorldComponent nav = GetGame().GetAIWorld().GetNavmeshWorldComponent("Soldiers");
		if (!nav || !nav.IsTileLoaded(p) || !nav.GetReachablePoint(p, 0.7, spot))
			return false;
		if (Math.AbsFloat(spot[1] - p[1]) > 1.2)
			return false;
		vector rel = spot - me;
		rel[1] = 0;
		float along = vector.Dot(rel, dir);
		vector lat = rel - dir * along;
		return lat.Length() >= 0.9 || along > 2.0;
	}

	protected bool Unblock(IEntity me, float now)
	{
		if (now < m_fUnblockAt_ms || m_iUnblocks >= 6 || m_fDetourUntil_ms > 0)
			return false;
		AIAgent agent = m_Utility.GetAIAgent();
		if (!agent || !agent.GetParentGroup())
			return false;
		vector mp = me.GetOrigin();
		vector dir = m_vGoal - mp;
		dir[1] = 0;
		if (dir.Length() < 0.5)
			return false;
		dir.Normalize();
		array<AIAgent> mates = {};
		agent.GetParentGroup().GetAgents(mates);
		IEntity blocker;
		AIAgent blockerAgent;
		float best = 1.4;
		foreach (AIAgent a : mates)
		{
			IEntity c = a.GetControlledEntity();
			if (!c || c == me || !DCO_Util.IsAlive(c))
				continue;
			vector off = c.GetOrigin() - mp;
			if (Math.AbsFloat(off[1]) > 1.2)
				continue;
			off[1] = 0;
			float d = off.Length();
			if (d >= best || d < 0.05 || vector.Dot(off * (1.0 / d), dir) < 0.2)
				continue;
			best = d;
			blocker = c;
			blockerAgent = a;
		}
		if (!blocker)
			return false;
		m_fUnblockAt_ms = now + 3000.0;
		m_iUnblocks++;
		vector side = Vector(dir[2], 0, -dir[0]);
		vector bp = blocker.GetOrigin();
		float s = 1.0;
		if (vector.Dot(bp - mp, side) < 0)
			s = -1.0;
		string mode = "none";
		vector spot;
		DCO_CQBMemberBehavior other;
		SCR_AIUtilityComponent ou = SCR_AIUtilityComponent.Cast(blockerAgent.FindComponent(SCR_AIUtilityComponent));
		if (ou)
			other = DCO_CQBMemberBehavior.Cast(ou.GetExecutedAction());
		if (other && other.IsActive())
		{
			if (FreeSpot(bp + side * s * 1.5, mp, dir, spot) || FreeSpot(bp - side * s * 1.5, mp, dir, spot) || FreeSpot(bp + dir * 2.5, mp, dir, spot))
			{
				if (other.MakeWay(spot, 4000.0, me))
					mode = "ask_move";
			}
		}
		if (mode == "none" && (FreeSpot(mp - side * s * 1.2 + dir * 0.6, mp, dir, spot) || FreeSpot(mp - side * s * 1.2, mp, dir, spot)))
		{
			m_vDetour = spot;
			m_fDetourUntil_ms = now + 3000.0;
			m_bDetourShove = false;
			m_bRedirect = true;
			mode = "reroute";
		}
		DCO_Perf.Count("cqb:unblock_" + mode);
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("cqb_unblock unit=%1 role=%2 blocker=%3 blocker_cqb=%4 dist=%5 mode=%6 spot=%7 goal_dist=%8 n=%9", DCO_Log.IdUnit(me), m_sRole, DCO_Log.IdUnit(blocker), DCO_Util.B01(other != null), best.ToString(-1, 1), mode, DCO_Util.XZ(spot), Math.Round(vector.DistanceXZ(mp, m_vGoal)), m_iUnblocks));
		return mode != "none";
	}

	void SetCharge()
	{
		m_bCharge = true;
		m_fYieldDist = 3.0;
		m_fMaxYield_ms = 1500.0;
		m_fArrive = 1.0;
	}

	void DCO_CQBMemberBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector goal, vector look, ECharacterStance stance, EMovementType pace, float timeout_s, string role)
	{
		m_sBehaviorTree = "AI/BehaviorTrees/Chimera/Soldier/DCO_CQBMember.bt";
		SetPriority(95);
		m_bUseCombatMove = true;
		m_bAllowLook = true;
		m_vGoal = goal;
		m_vLook = look;
		m_eStance = stance;
		m_ePace = pace;
		m_sRole = role;
		m_fArrive = 1.5;
		m_fStart_ms = GetGame().GetWorld().GetWorldTime();
		m_fUntil_ms = m_fStart_ms + timeout_s * 1000.0;
		if (DCO_Log.On(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG) && utility)
		{
			string grp = "-";
			AIAgent agent = utility.GetAIAgent();
			if (agent && agent.GetParentGroup())
				grp = DCO_Log.IdGroup(agent.GetParentGroup());
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(utility.m_OwnerEntity), string.Format("cqb_slot grp=%1 unit=%2 role=%3 pos=%4 kind=%5", grp, DCO_Log.IdUnit(utility.m_OwnerEntity), role, DCO_Util.XZ(goal), ClassName()));
		}
	}

	bool IsDone()
	{
		return m_bDone;
	}

	bool IsActive()
	{
		if (m_bDone)
			return false;
		EAIActionState st = GetActionState();
		return st != EAIActionState.COMPLETED && st != EAIActionState.FAILED;
	}

	bool Arrived()
	{
		return m_bArrived;
	}

	string Why()
	{
		return m_sWhy;
	}

	vector Goal()
	{
		return m_vGoal;
	}

	string Role()
	{
		return m_sRole;
	}

	bool Reached()
	{
		return m_bReached;
	}

	void SetBound(float delay_s)
	{
		m_bBound = true;
		m_fNextRush_ms = GetGame().GetWorld().GetWorldTime() + delay_s * 1000.0;
	}

	protected float YieldTo(float nowEv)
	{
		IEntity me = m_Utility.m_OwnerEntity;
		if (!me)
			return 89;
		if (m_fMaxYield_ms > 0)
		{
			if (m_fYieldSince_ms <= 0)
				m_fYieldSince_ms = nowEv;
			else if (nowEv - m_fYieldSince_ms > m_fMaxYield_ms)
			{
				m_fYieldSince_ms = 0;
				m_fYieldUntil_ms = 0;
				m_fNoYieldUntil_ms = nowEv + 1500.0;
				m_bYieldPosSet = false;
				m_bArrived = false;
				DCO_Perf.Count("cqb:push_burst");
				if (DCO_Log.On(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG))
					DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("cqb_push_burst unit=%1 role=%2 goal_dist=%3", DCO_Log.IdUnit(me), m_sRole, Math.Round(vector.DistanceXZ(me.GetOrigin(), m_vGoal))));
				return 95;
			}
		}
		if (!m_bYieldPosSet)
		{
			m_bYieldPosSet = true;
			m_vYieldPos = me.GetOrigin();
			return 89;
		}
		float drift = vector.DistanceXZ(me.GetOrigin(), m_vYieldPos);
		if (drift <= 8.0)
			return 89;
		m_bYieldPosSet = false;
		m_fYieldUntil_ms = 0;
		m_fNoYieldUntil_ms = nowEv + 1500.0;
		m_bArrived = false;
		DCO_Perf.Count("cqb:yield_drift");
		string grp = "-";
		AIAgent agent = m_Utility.GetAIAgent();
		if (agent && agent.GetParentGroup())
			grp = DCO_Log.IdGroup(agent.GetParentGroup());
		float startD = vector.DistanceXZ(m_vYieldPos, m_vGoal);
		float nowD = vector.DistanceXZ(me.GetOrigin(), m_vGoal);
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_yield_drift grp=%1 unit=%2 role=%3 start_dist=%4 now_dist=%5 drift=%6 action=end_yield", grp, DCO_Log.IdUnit(me), m_sRole, Math.Round(startD), Math.Round(nowD), Math.Round(drift)));
		return 95;
	}

	protected bool ShotAtBy(float maxDist)
	{
		if (!m_Utility.m_CombatComponent || !DCO_AimedYield.TargetVisible(m_Utility, maxDist))
			return false;
		BaseTarget t = m_Utility.m_CombatComponent.GetCurrentTarget();
		return t && (t.IsEndangering() || t.GetTimeSinceEndangered() < 3.0);
	}

	void YieldFire(float ms)
	{
		m_fYieldUntil_ms = GetGame().GetWorld().GetWorldTime() + ms;
	}

	void Leash()
	{
		m_bLeash = true;
		m_bArrived = false;
		m_fYieldUntil_ms = 0;
	}

	void YieldFor(float ms)
	{
		m_fYieldUntil_ms = GetGame().GetWorld().GetWorldTime() + ms;
	}

	protected bool HoldFire()
	{
		return false;
	}

	void DCO_Supersede()
	{
		Finish("superseded");
	}

	protected void Finish(string why)
	{
		if (m_bDone)
			return;
		m_bDone = true;
		m_sWhy = why;
		DCO_Perf.Count("cqb:member_" + why);
		Complete();
	}

	protected ENodeResult MoveStep()
	{
		IEntity e = m_Utility.m_OwnerEntity;
		SCR_AICombatMoveState st = m_Utility.m_CombatMoveState;
		if (!e || !st)
			return ENodeResult.FAIL;
		float now = GetGame().GetWorld().GetWorldTime();
		bool detour = m_fDetourUntil_ms > 0;
		if (detour && (now > m_fDetourUntil_ms || (!m_bDetourShove && vector.Distance(e.GetOrigin(), m_vDetour) <= 0.7)))
		{
			DCO_Perf.Count("cqb:detour_end");
			if (DCO_Log.On(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(e), string.Format("cqb_detour_end unit=%1 role=%2 shove=%3 detour_dist=%4 timeout=%5", DCO_Log.IdUnit(e), m_sRole, DCO_Util.B01(m_bDetourShove), vector.Distance(e.GetOrigin(), m_vDetour).ToString(-1, 1), DCO_Util.B01(now > m_fDetourUntil_ms)));
			m_fDetourUntil_ms = 0;
			detour = false;
			m_bRedirect = true;
			m_bMoving = false;
		}
		if (detour && m_bDetourShove && vector.Distance(e.GetOrigin(), m_vDetour) <= 0.7)
			return ENodeResult.RUNNING;
		if (!detour && vector.Distance(e.GetOrigin(), m_vGoal) <= m_fArrive)
		{
			m_bArrived = true;
			m_bReached = true;
			m_bLeash = false;
			return ENodeResult.SUCCESS;
		}
		if (m_bMoving && m_Rq)
		{
			SCR_EAICombatMoveRequestState rs = m_Rq.m_eState;
			SCR_AICombatMoveRequestBase stRq = st.GetRequest();
			bool mine = stRq == m_Rq;
			bool pending = mine || !stRq;
			if (pending && (rs == SCR_EAICombatMoveRequestState.EXECUTING || (rs == SCR_EAICombatMoveRequestState.IDLE && now - m_fMoveAt_ms < 3000.0)))
			{
				if (now - m_fProgAt_ms < 1500.0)
					return ENodeResult.RUNNING;
				float prog = vector.Distance(m_vProgPos, e.GetOrigin());
				m_vProgPos = e.GetOrigin();
				m_fProgAt_ms = now;
				if (prog > 0.3 || !Unblock(e, now))
					return ENodeResult.RUNNING;
				m_bMoving = false;
				return IssueMove(e, st, now);
			}
			bool preempted = stRq && !mine && rs != SCR_EAICombatMoveRequestState.SUCCESS && rs != SCR_EAICombatMoveRequestState.FAIL;
			if (preempted)
				m_Rq.m_eState = SCR_EAICombatMoveRequestState.CANCELED;
			m_bMoving = false;
			if (m_bBound)
				m_fNextRush_ms = now + Math.RandomFloat(500.0, 1500.0);
			if (preempted)
			{
				DCO_Perf.Count("cqb:move_preempted");
				if (now - m_fPreemptLogAt_ms > 5000.0 && DCO_Log.On(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG))
				{
					m_fPreemptLogAt_ms = now;
					DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(e), string.Format("cqb_move_preempted unit=%1 role=%2 rq_state=%3 by=%4 stuck=%5", DCO_Log.IdUnit(e), m_sRole, typename.EnumToString(SCR_EAICombatMoveRequestState, rs), stRq.ClassName(), m_iStuck));
				}
			}
			else if (vector.Distance(m_vMoveFrom, e.GetOrigin()) < 0.8)
			{
				float sGoal = vector.Distance(e.GetOrigin(), m_vGoal);
				if (rs == SCR_EAICombatMoveRequestState.SUCCESS && (sGoal <= Math.Max(m_fArrive, 3.0) || !m_bBound || sGoal <= 25.0))
				{
					m_bArrived = true;
					m_bReached = true;
					m_bLeash = false;
					if (sGoal <= Math.Max(m_fArrive, 3.0))
						DCO_Perf.Count("cqb:arrive_engine_tolerance");
					else
						DCO_Perf.Count("cqb:arrive_engine_reach");
					return ENodeResult.SUCCESS;
				}
				if (rs == SCR_EAICombatMoveRequestState.SUCCESS && !m_bNoCoverNext)
				{
					m_bNoCoverNext = true;
					DCO_Perf.Count("cqb:move_success_nomove");
				}
				else if (Unblock(e, now))
					return IssueMove(e, st, now);
				else
					m_iStuck++;
			}
			else
			{
				m_iStuck = 0;
				m_bNoCoverNext = false;
			}
			if (m_iStuck >= 4 && m_bHoldAfter && vector.Distance(e.GetOrigin(), m_vGoal) <= 8.0)
			{
				DCO_Perf.Count("cqb:hold_here");
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(e), string.Format("cqb_hold_here unit=%1 role=%2 goal_dist=%3 fail=%4", DCO_Log.IdUnit(e), m_sRole, Math.Round(vector.Distance(e.GetOrigin(), m_vGoal)), typename.EnumToString(SCR_EAICombatMoveRequestFailReason, m_Rq.m_eFailReason)));
				m_vGoal = e.GetOrigin();
				m_iStuck = 0;
				m_bArrived = true;
				m_bReached = true;
				m_bLeash = false;
				return ENodeResult.SUCCESS;
			}
			if (m_iStuck >= 4)
			{
				string failWhy = typename.EnumToString(SCR_EAICombatMoveRequestFailReason, m_Rq.m_eFailReason);
				string rqState = typename.EnumToString(SCR_EAICombatMoveRequestState, rs);
				string curAct = "-";
				if (m_Utility.GetCurrentAction())
					curAct = m_Utility.GetCurrentAction().ClassName();
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, DCO_Log.IdUnit(e), string.Format("cqb_member_stuck unit=%1 role=%2 goal_dist=%3 rq_state=%4 fail=%5 mine=%6 cur=%7", DCO_Log.IdUnit(e), m_sRole, Math.Round(vector.Distance(e.GetOrigin(), m_vGoal)), rqState, failWhy, mine, curAct));
				Finish("stuck");
				return ENodeResult.FAIL;
			}
		}
		if (st.IsExecutingRequest() && !m_bLeash && !m_bRedirect)
			return ENodeResult.RUNNING;
		return IssueMove(e, st, now);
	}

	protected ENodeResult IssueMove(IEntity e, SCR_AICombatMoveState st, float now)
	{
		m_bRedirect = false;
		vector goal = m_vGoal;
		if (m_fDetourUntil_ms > 0)
			goal = m_vDetour;
		float goalDist = vector.Distance(e.GetOrigin(), goal);
		bool rush = m_bBound && goalDist > 25.0;
		if (rush && now < m_fNextRush_ms)
			return ENodeResult.RUNNING;
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
		rq.m_eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
		rq.m_vMovePos = goal;
		rq.m_vTargetPos = m_vLook;
		rq.m_bTryFindCover = false;
		rq.m_bFailIfNoCover = false;
		if (rush)
		{
			vector dir = goal - e.GetOrigin();
			dir[1] = 0;
			dir.Normalize();
			rq.m_vMovePos = DCO_Util.Surface(e.GetOrigin() + dir * Math.RandomFloat(20.0, 35.0));
			rq.m_bTryFindCover = !m_bNoCoverNext;
			rq.m_bUseCoverSearchDirectivity = true;
			rq.m_fCoverSearchDistMin = 0;
			rq.m_fCoverSearchDistMax = 6.0;
			DCO_Perf.Count("cqb:approach_bound");
			if (DCO_Log.On(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG))
			{
				string bgrp = "-";
				AIAgent bagent = m_Utility.GetAIAgent();
				if (bagent && bagent.GetParentGroup())
					bgrp = DCO_Log.IdGroup(bagent.GetParentGroup());
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(e), string.Format("cqb_approach_bound grp=%1 unit=%2 role=%3 from=%4 to=%5 dist_goal=%6", bgrp, DCO_Log.IdUnit(e), m_sRole, DCO_Util.XZ(e.GetOrigin()), DCO_Util.XZ(rq.m_vMovePos), Math.Round(goalDist)));
			}
		}
		rq.m_eStanceMoving = m_eStance;
		rq.m_eStanceEnd = m_eStance;
		rq.m_eMovementType = m_ePace;
		rq.m_bAimAtTarget = true;
		if (m_ePace == EMovementType.RUN && vector.Distance(e.GetOrigin(), rq.m_vMovePos) > 20.0)
		{
			rq.m_eStanceMoving = ECharacterStance.STAND;
			rq.m_eMovementType = EMovementType.SPRINT;
			rq.m_bAimAtTarget = false;
			DCO_Perf.Count("cqb:move_sprint");
		}
		rq.m_bAimAtTargetEnd = true;
		rq.m_fMoveDuration_s = vector.Distance(e.GetOrigin(), rq.m_vMovePos) / 2.0 + 4.0;
		st.ApplyNewRequest(rq);
		m_Rq = rq;
		m_bMoving = true;
		m_fMoveAt_ms = now;
		m_vMoveFrom = e.GetOrigin();
		m_vProgPos = e.GetOrigin();
		m_fProgAt_ms = now;
		return ENodeResult.RUNNING;
	}

	ENodeResult DCO_Step()
	{
		if (m_bDone)
			return ENodeResult.SUCCESS;
		RaiseWeapon();
		if (m_bArrived && m_bHoldAfter && m_Utility.m_OwnerEntity && vector.Distance(m_Utility.m_OwnerEntity.GetOrigin(), m_vGoal) > m_fArrive + 3.0)
		{
			m_bArrived = false;
			DCO_Perf.Count("cqb:slot_resume");
		}
		if (m_bArrived && m_bHoldAfter)
			return ENodeResult.RUNNING;
		ENodeResult r = MoveStep();
		if (r == ENodeResult.SUCCESS && !m_bHoldAfter)
			Finish("arrived");
		if (r == ENodeResult.SUCCESS && m_bHoldAfter)
			return ENodeResult.RUNNING;
		return r;
	}

	override float CustomEvaluate()
	{
		EAIActionState st = GetActionState();
		if (m_bDone || st == EAIActionState.COMPLETED || st == EAIActionState.FAILED)
			return 0;
		if (GetGame().GetWorld().GetWorldTime() > m_fUntil_ms)
		{
			Finish("timeout");
			return 0;
		}
		if (m_bLeash)
			return 95;
		float nowEv = GetGame().GetWorld().GetWorldTime();
		if (nowEv < m_fNoYieldUntil_ms)
			return 95;
		if (nowEv < m_fYieldUntil_ms)
			return YieldTo(nowEv);
		if (nowEv >= m_fBlankAt_ms)
		{
			m_fBlankAt_ms = nowEv + 500.0;
			DCO_CQBActivity pa = DCO_CQBActivity.Cast(GetRelatedGroupActivity());
			float bd = -1;
			if (pa && m_Utility.m_OwnerEntity && pa.KnownEnemyNear(m_Utility.m_OwnerEntity.GetOrigin(), 8.0, bd))
			{
				m_fYieldUntil_ms = nowEv + 2500.0;
				DCO_Perf.Count("cqb:yield_point_blank");
				if (DCO_Log.On(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG))
					DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("cqb_yield unit=%1 role=%2 yield=1 tgt_dist=%3 reason=point_blank", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), m_sRole, bd.ToString(-1, 1)));
				return YieldTo(nowEv);
			}
		}
		bool y = DCO_AimedYield.TargetVisible(m_Utility, m_fYieldDist);
		if (!y && !m_bCharge && ShotAtBy(200.0))
		{
			y = true;
			if (!m_bYield)
			{
				DCO_Perf.Count("cqb:yield_shot_at");
				BaseTarget shotTgt = m_Utility.m_CombatComponent.GetCurrentTarget();
				float sd = -1;
				if (shotTgt && shotTgt.GetTargetEntity() && m_Utility.m_OwnerEntity)
					sd = vector.Distance(shotTgt.GetTargetEntity().GetOrigin(), m_Utility.m_OwnerEntity.GetOrigin());
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("cqb_yield unit=%1 role=%2 yield=1 tgt_dist=%3 reason=shot_at yield_dist=%4", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), m_sRole, Math.Round(sd), Math.Round(m_fYieldDist)));
			}
		}
		if (y && HoldFire())
			y = false;
		if (y != m_bYield)
		{
			m_bYield = y;
			DCO_Perf.Count("cqb:yield_" + DCO_Util.B01(y).ToString());
			if (DCO_Log.On(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG))
			{
				float td = -1;
				BaseTarget t = m_Utility.m_CombatComponent.GetCurrentTarget();
				if (t && t.GetTargetEntity() && m_Utility.m_OwnerEntity)
					td = vector.Distance(t.GetTargetEntity().GetOrigin(), m_Utility.m_OwnerEntity.GetOrigin());
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("cqb_yield unit=%1 role=%2 yield=%3 tgt_dist=%4", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), m_sRole, DCO_Util.B01(y), Math.Round(td)));
			}
		}
		if (y)
			return YieldTo(nowEv);
		m_bYieldPosSet = false;
		m_fYieldSince_ms = 0;
		return 95;
	}

	override void OnActionDeselected()
	{
		super.OnActionDeselected();
		EAIActionState st = GetActionState();
		if (!m_bDone && (st == EAIActionState.COMPLETED || st == EAIActionState.FAILED))
		{
			m_bDone = true;
			m_sWhy = "cancel";
		}
	}

	override int GetCause()
	{
		return SCR_EAIBehaviorCause.COMBAT;
	}

	override string GetActionDebugInfo()
	{
		return string.Format("%1 DCO cqb %2 to %3", this, m_sRole, m_vGoal);
	}
}

class DCO_CQBMoveSlotBehavior : DCO_CQBMemberBehavior
{
	void DCO_CQBMoveSlotBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector goal, vector look, ECharacterStance stance, EMovementType pace, float timeout_s, string role)
	{
		m_fArrive = 2.0;
		m_bHoldAfter = true;
		m_fMaxYield_ms = 6000.0;
	}
}

class DCO_CQBEnterBehavior : DCO_CQBMemberBehavior
{
	void DCO_CQBEnterBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector goal, vector look, ECharacterStance stance, EMovementType pace, float timeout_s, string role)
	{
		m_fArrive = 1.2;
		m_bHoldAfter = true;
		m_fYieldDist = 15.0;
		m_fMaxYield_ms = 5000.0;
	}
}

class DCO_CQBSupportBehavior : DCO_CQBMemberBehavior
{
	void DCO_CQBSupportBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector goal, vector look, ECharacterStance stance, EMovementType pace, float timeout_s, string role)
	{
		m_fArrive = 3.0;
		m_bHoldAfter = true;
		m_fYieldDist = 150.0;
	}

	override protected bool HoldFire()
	{
		DCO_CQBActivity act = DCO_CQBActivity.Cast(GetRelatedGroupActivity());
		if (!act || !m_Utility.m_CombatComponent)
			return false;
		BaseTarget t = m_Utility.m_CombatComponent.GetCurrentTarget();
		if (!t || !t.GetTargetEntity())
			return false;
		vector tp = t.GetTargetEntity().GetOrigin();
		float fd = -1;
		if (!m_Utility.m_OwnerEntity || !act.HoldFireAt(m_Utility.m_OwnerEntity.GetOrigin(), tp, fd))
			return false;
		DCO_Perf.Count("cqb:support_hold_fire");
		float now = GetGame().GetWorld().GetWorldTime();
		if (now >= m_fHoldLogAt_ms && DCO_Log.On(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG))
		{
			m_fHoldLogAt_ms = now + 5000.0;
			float ty = tp[1];
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("cqb_hold_fire unit=%1 role=%2 tgt=%3 tgt_y=%4 reason=friend_in_line friend_dist=%5", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), m_sRole, DCO_Util.XZ(tp), ty.ToString(-1, 1), fd.ToString(-1, 1)));
		}
		return true;
	}
}

class DCO_CQBHoldBehavior : DCO_CQBMemberBehavior
{
	void DCO_CQBHoldBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector goal, vector look, ECharacterStance stance, EMovementType pace, float timeout_s, string role)
	{
		m_fArrive = 1.5;
		m_bHoldAfter = true;
		m_fYieldDist = 60.0;
		m_fMaxYield_ms = 8000.0;
	}
}

class DCO_CQBBreachBehavior : DCO_CQBMemberBehavior
{
	protected bool m_bThrown;
	protected bool m_bTried;
	protected float m_fThrownAt_ms;

	void DCO_CQBBreachBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector goal, vector look, ECharacterStance stance, EMovementType pace, float timeout_s, string role)
	{
		m_fArrive = 99.0;
		m_bHoldAfter = false;
		m_fYieldDist = 15.0;
	}

	bool Thrown()
	{
		return m_bThrown;
	}

	override ENodeResult DCO_Step()
	{
		if (m_bDone)
			return ENodeResult.SUCCESS;
		float now = GetGame().GetWorld().GetWorldTime();
		if (!m_bTried)
		{
			m_bTried = true;
			m_bThrown = DCO_BreachUtility.TryThrowBreachGrenade(m_Utility, m_vLook, true);
			m_fThrownAt_ms = now;
			if (!m_bThrown)
			{
				Finish("no_grenade:" + DCO_BreachUtility.s_sReject);
				return ENodeResult.SUCCESS;
			}
			DCO_GrenadeUtility.NotifyGrenadeThrown(m_Utility);
			DCO_GrenadeWatch.OnRequested(m_Utility.m_OwnerEntity);
			DCO_Perf.Count("cqb:breach_throw_" + DCO_GrenadeUtility.s_sDCOMode);
			DCO_CQBActivity bact = DCO_CQBActivity.Cast(GetRelatedGroupActivity());
			int broom = -1;
			if (bact)
				broom = bact.RoomIndex();
			string bgrp = "-";
			AIAgent bagent = m_Utility.GetAIAgent();
			if (bagent && bagent.GetParentGroup())
				bgrp = DCO_Log.IdGroup(bagent.GetParentGroup());
			float bdist = -1;
			if (m_Utility.m_OwnerEntity)
				bdist = vector.Distance(m_Utility.m_OwnerEntity.GetOrigin(), m_vLook);
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_breach_throw grp=%1 unit=%2 room=%3 mode=%4 dist=%5", bgrp, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), broom, DCO_GrenadeUtility.s_sDCOMode, bdist.ToString(-1, 1)));
		}
		float since;
		int gs = DCO_GrenadeWatch.State(m_Utility.m_OwnerEntity, now, since);
		float waited = now - m_fThrownAt_ms;
		string why;
		if (gs == -1)
			why = "throw_failed";
		else if (gs == 2 && since >= 300.0)
			why = "exploded";
		else if (gs == 0 && waited > 7000.0)
			why = "never_thrown";
		else if (waited > 11000.0)
			why = "timeout";
		if (why.IsEmpty())
			return ENodeResult.RUNNING;
		DCO_Perf.Count("cqb:breach_wait_" + why);
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("cqb_breach_done unit=%1 why=%2 waited_ms=%3 throw_delay_ms=%4 flight_ms=%5", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), why, Math.Round(waited), Math.Round(DCO_GrenadeWatch.ThrowDelay_ms(m_Utility.m_OwnerEntity)), Math.Round(DCO_GrenadeWatch.FlightTime_ms(m_Utility.m_OwnerEntity))));
		Finish(why);
		return ENodeResult.SUCCESS;
	}
}

class DCO_AICQBMemberStep : SCR_AIActionTask
{
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		DCO_CQBMemberBehavior b = DCO_CQBMemberBehavior.Cast(GetExecutedAction());
		if (!b)
			return ENodeResult.FAIL;
		return b.DCO_Step();
	}

	static override bool VisibleInPalette() { return true; }
	static override string GetOnHoverDescription() { return "DCO CQB member: moves to the assigned slot (stack, breach, entry, support or hold) with combat move requests, yields to vanilla attack when a target is seen close."; }
}
