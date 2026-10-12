class DCO_RelocateBehavior : SCR_AIBehaviorBase
{
	protected vector m_vDCOPos;
	protected vector m_vDCOThreat;
	protected vector m_vDCOFrom;
	protected ECharacterStance m_eDCOStance;
	protected bool m_bDCOCover;
	protected float m_fDCODur_s;
	protected string m_sDCOWhy;
	protected ref SCR_AICombatMoveRequest_Move m_DCORq;
	protected bool m_bDCOSent;
	protected bool m_bDCODone;
	protected bool m_bDCOYield;
	protected float m_fDCOStart_ms;
	protected float m_fDCOSent_ms;
	protected float m_fDCOUntil_ms;
	protected int m_iDCOResend;
	protected int m_iDCOResume;
	protected float m_fDCOResumeAt_ms;
	protected int m_iDCOMoveStance = -1;
	protected int m_iDCOMoveType = -1;
	protected float m_fDCOCoverMax = 8.0;
	protected bool m_bDCONoYield;
	protected vector m_vDCOProgPos;
	protected float m_fDCOProgAt_ms;
	protected bool m_bDCOCoverLater;
	protected bool m_bDCORequireCover;

	void DCO_SetRequireCover(bool req)
	{
		m_bDCORequireCover = req;
	}

	protected bool DCO_StuckWithTarget(float now)
	{
		IEntity e = m_Utility.m_OwnerEntity;
		if (m_fDCOProgAt_ms <= 0)
		{
			m_fDCOProgAt_ms = now;
			m_vDCOProgPos = e.GetOrigin();
			return false;
		}
		if (now - m_fDCOProgAt_ms < 5000.0)
			return false;
		float moved = vector.DistanceXZ(e.GetOrigin(), m_vDCOProgPos);
		m_fDCOProgAt_ms = now;
		m_vDCOProgPos = e.GetOrigin();
		if (moved >= 1.0 || !m_Utility.m_CombatComponent)
			return false;
		BaseTarget t = m_Utility.m_CombatComponent.GetCurrentTarget();
		if (!t || t.GetTimeSinceSeen() > 2.0)
			return false;
		DCO_Perf.Count("reloc_bhv:stuck_target_" + m_sDCOWhy);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(e), string.Format("relocate_stuck unit=%1 why=%2 moved_5s=%3 target_d=%4 left_to_goal=%5",
			DCO_Log.IdUnit(e), m_sDCOWhy, moved.ToString(-1, 1), Math.Round(vector.Distance(t.GetLastSeenPosition(), e.GetOrigin())), Math.Round(vector.DistanceXZ(e.GetOrigin(), m_vDCOPos))));
		return true;
	}

	void DCO_SetMove(ECharacterStance stance, EMovementType moveType, float coverMax, bool noYield)
	{
		m_iDCOMoveStance = stance;
		m_iDCOMoveType = moveType;
		m_fDCOCoverMax = coverMax;
		m_bDCONoYield = noYield;
	}

	string DCO_Why()
	{
		return m_sDCOWhy;
	}

	float DCO_AgeMs()
	{
		return GetGame().GetWorld().GetWorldTime() - m_fDCOStart_ms;
	}

	bool DCO_IsDone()
	{
		return m_bDCODone;
	}

	void DCO_RelocateBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector pos, vector threat, ECharacterStance stance, bool findCover, float duration_s, string why)
	{
		m_sBehaviorTree = "AI/BehaviorTrees/Chimera/Soldier/DCO_Relocate.bt";
		SetPriority(91);
		m_vDCOPos = pos;
		m_vDCOThreat = threat;
		m_eDCOStance = stance;
		m_bDCOCover = findCover;
		m_fDCODur_s = Math.Clamp(duration_s, 2, 30);
		m_sDCOWhy = why;
		m_bUseCombatMove = true;
		m_bAllowLook = true;
		if (!GetGame() || !GetGame().GetWorld())
			return;
		m_fDCOStart_ms = GetGame().GetWorld().GetWorldTime();
		m_fDCOUntil_ms = m_fDCOStart_ms + (m_fDCODur_s + 5.0) * 1000.0;
		if (utility && utility.m_OwnerEntity)
			m_vDCOFrom = utility.m_OwnerEntity.GetOrigin();
	}

	static DCO_RelocateBehavior Start(SCR_AIUtilityComponent u, vector pos, vector threat, bool findCover, ECharacterStance stance, float duration_s, string why)
	{
		if (!u || !u.m_OwnerEntity || !u.m_CombatMoveState || (u.m_AIInfo && u.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE)))
			return null;
		DCO_RelocateBehavior old = DCO_RelocateBehavior.Cast(u.FindActionOfType(DCO_RelocateBehavior));
		if (old && old.DCO_SameJob(pos, why))
		{
			DCO_Perf.Count("reloc_bhv:keep_" + why);
			return old;
		}
		if (old && !old.DCO_IsDone() && !DCO_MoveArbiter.Allow(u, why, old.DCO_Why(), old.DCO_AgeMs()))
		{
			DCO_Perf.Count("reloc_bhv:blocked_" + why + "_by_" + old.DCO_Why());
			return null;
		}
		if (old)
			old.DCO_Supersede(why);
		DCO_RelocateBehavior b = new DCO_RelocateBehavior(u, null, pos, threat, stance, findCover, duration_s, why);
		u.AddAction(b);
		DCO_Perf.Count("reloc_bhv:start_" + why);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("relocate_start unit=%1 why=%2 to=%3 dist=%4 stance=%5 cover=%6", DCO_Log.IdUnit(u.m_OwnerEntity), why, DCO_Log.Pos(pos), Math.Round(vector.DistanceXZ(u.m_OwnerEntity.GetOrigin(), pos)), typename.EnumToString(ECharacterStance, stance), DCO_Util.B01(findCover)));
		return b;
	}

	ENodeResult DCO_Step()
	{
		if (m_bDCODone)
			return ENodeResult.SUCCESS;
		SCR_AICombatMoveState st = m_Utility.m_CombatMoveState;
		if (!st || !m_Utility.m_OwnerEntity)
		{
			DCO_Finish("no_state");
			return ENodeResult.FAIL;
		}
		float now = GetGame().GetWorld().GetWorldTime();
		if (!m_bDCOSent && now < m_fDCOResumeAt_ms)
			return ENodeResult.RUNNING;
		if (!m_bDCOSent)
		{
			SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
			rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
			rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
			if (m_sDCOWhy == "pin_crawl" || m_sDCOWhy.StartsWith("stress_"))
				rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
			rq.m_eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
			rq.m_vTargetPos = m_vDCOThreat;
			rq.m_vMovePos = m_vDCOPos;
			rq.m_bTryFindCover = m_bDCOCover;
			rq.m_bFailIfNoCover = m_bDCORequireCover;
			rq.m_bUseCoverSearchDirectivity = false;
			rq.m_bCheckCoverVisibility = m_bDCOCover;
			rq.m_fCoverSearchDistMin = 0;
			rq.m_fCoverSearchDistMax = m_fDCOCoverMax;
			rq.m_eStanceMoving = ECharacterStance.CROUCH;
			if (m_eDCOStance == ECharacterStance.PRONE && vector.DistanceXZ(m_Utility.m_OwnerEntity.GetOrigin(), m_vDCOPos) <= 6.0)
				rq.m_eStanceMoving = ECharacterStance.PRONE;
			if (m_iDCOMoveStance >= 0)
				rq.m_eStanceMoving = m_iDCOMoveStance;
			rq.m_eStanceEnd = m_eDCOStance;
			rq.m_eMovementType = EMovementType.RUN;
			if (m_iDCOMoveType >= 0)
				rq.m_eMovementType = m_iDCOMoveType;
			rq.m_bAimAtTarget = false;
			rq.m_bAimAtTargetEnd = true;
			rq.m_fMoveDuration_s = m_fDCODur_s;
			m_bDCOCoverLater = false;
			float left0 = vector.DistanceXZ(m_Utility.m_OwnerEntity.GetOrigin(), m_vDCOPos);
			if (rq.m_bTryFindCover && DCO_Ctx.TuneForU(m_Utility, "osc.fix_cover", 1.0) > 0)
			{
				if (left0 > m_fDCOCoverMax + 2.0)
				{
					rq.m_bTryFindCover = false;
					rq.m_bCheckCoverVisibility = false;
					m_bDCOCoverLater = true;
					DCO_Perf.Count("reloc_bhv:cover_deferred");
				}
				else
				{
					rq.m_bUseCoverSearchDirectivity = true;
					rq.m_fCoverSearchSectorHalfAngleRad = DCO_Ctx.TuneForU(m_Utility, "osc.cover_half_deg", 60.0) * Math.DEG2RAD;
					DCO_Perf.Count("reloc_bhv:cover_sector");
				}
			}
			if (!rq.m_bTryFindCover && DCO_Ctx.TuneForU(m_Utility, "osc.fix_short", 1.0) > 0)
			{
				float spd = 2.5;
				if (rq.m_eStanceMoving == ECharacterStance.PRONE)
					spd = 0.8;
				float need = Math.Min(left0 / spd + 2.0, 30.0);
				if (need > rq.m_fMoveDuration_s)
				{
					rq.m_fMoveDuration_s = need;
					m_fDCOUntil_ms = Math.Max(m_fDCOUntil_ms, now + (need + 5.0) * 1000.0);
					DCO_Perf.Count("reloc_bhv:dur_extend");
				}
			}
			st.ApplyNewRequest(rq);
			m_DCORq = rq;
			m_bDCOSent = true;
			m_fDCOSent_ms = now;
			return ENodeResult.RUNNING;
		}
		SCR_EAICombatMoveRequestState rs = m_DCORq.m_eState;
		if (rs == SCR_EAICombatMoveRequestState.SUCCESS)
		{
			float leftS = vector.DistanceXZ(m_Utility.m_OwnerEntity.GetOrigin(), m_vDCOPos);
			string again;
			if (m_bDCOCoverLater && leftS <= m_fDCOCoverMax + 2.0 && now < m_fDCOUntil_ms - 2000.0)
				again = "cover_later";
			else if (!m_DCORq.m_bTryFindCover && leftS > 4.0 && m_iDCOResume < 3 && now < m_fDCOUntil_ms - 2000.0 && DCO_Ctx.TuneForU(m_Utility, "osc.fix_short", 1.0) > 0)
			{
				again = "short";
				m_iDCOResume++;
			}
			if (!again.IsEmpty())
			{
				m_bDCOSent = false;
				DCO_Perf.Count("reloc_bhv:resume_" + again);
				if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
					DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("relocate_resume unit=%1 why=%2 try=%3 left=%4 reason=%5", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), m_sDCOWhy, m_iDCOResume, Math.Round(leftS), again));
				return ENodeResult.RUNNING;
			}
			if (!m_DCORq.m_bTryFindCover && leftS > 4.0)
				DCO_Finish("short");
			else
				DCO_Finish("arrived");
			return ENodeResult.SUCCESS;
		}
		if (rs == SCR_EAICombatMoveRequestState.FAIL)
		{
			if (m_DCORq.m_eFailReason == SCR_EAICombatMoveRequestFailReason.COVER_NOT_FOUND && m_bDCOCover && m_iDCOResend < 1 && !m_bDCORequireCover)
			{
				m_iDCOResend++;
				m_bDCOCover = false;
				m_bDCOSent = false;
				DCO_Perf.Count("reloc_bhv:retry_no_cover");
				return ENodeResult.RUNNING;
			}
			DCO_Finish("fail_" + typename.EnumToString(SCR_EAICombatMoveRequestFailReason, m_DCORq.m_eFailReason));
			return ENodeResult.SUCCESS;
		}
		bool mine = st.GetRequest() == m_DCORq;
		if (rs == SCR_EAICombatMoveRequestState.CANCELED || (!mine && rs != SCR_EAICombatMoveRequestState.EXECUTING && now - m_fDCOSent_ms > 3000.0))
		{
			if (now - m_fDCOSent_ms < 2000.0 && m_iDCOResend < 1 && !st.IsExecutingRequest())
			{
				m_iDCOResend++;
				m_bDCOSent = false;
				DCO_Perf.Count("reloc_bhv:retry_canceled");
				return ENodeResult.RUNNING;
			}
			float left = vector.DistanceXZ(m_Utility.m_OwnerEntity.GetOrigin(), m_vDCOPos);
			if (m_iDCOResume < 3 && left > 4.0 && now < m_fDCOUntil_ms - 3000.0)
			{
				m_iDCOResume++;
				m_bDCOSent = false;
				m_fDCOResumeAt_ms = now + 2000.0;
				DCO_Perf.Count("reloc_bhv:resume");
				if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
					DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("relocate_resume unit=%1 why=%2 try=%3 left=%4", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), m_sDCOWhy, m_iDCOResume, Math.Round(left)));
				return ENodeResult.RUNNING;
			}
			DCO_Finish("canceled");
			return ENodeResult.SUCCESS;
		}
		if (DCO_StuckWithTarget(now))
		{
			DCO_Finish("stuck_target_visible");
			return ENodeResult.SUCCESS;
		}
		return ENodeResult.RUNNING;
	}

	void DCO_GetJob(out vector pos, out vector threat, out ECharacterStance stance, out bool cover, out float dur_s)
	{
		pos = m_vDCOPos;
		threat = m_vDCOThreat;
		stance = m_eDCOStance;
		cover = m_bDCOCover;
		dur_s = m_fDCODur_s;
	}

	bool DCO_SameJob(vector pos, string why)
	{
		if (m_bDCODone || why != m_sDCOWhy)
			return false;
		float tol = 3.0;
		if (m_Utility && m_Utility.m_OwnerEntity && DCO_Ctx.TuneForU(m_Utility, "osc.fix_commit", 1.0) > 0)
			tol = Math.Max(3.0, 0.3 * vector.DistanceXZ(m_Utility.m_OwnerEntity.GetOrigin(), m_vDCOPos));
		return vector.DistanceXZ(pos, m_vDCOPos) <= tol;
	}

	void DCO_Supersede(string by)
	{
		if (!m_bDCODone)
			DCO_Finish("superseded_by_" + by);
	}

	protected void DCO_Finish(string result)
	{
		if (m_bDCODone)
			return;
		m_bDCODone = true;
		DCO_Perf.Count("reloc_bhv:" + m_sDCOWhy + "_" + result);
		IEntity e = m_Utility.m_OwnerEntity;
		vector p = m_vDCOFrom;
		if (e)
			p = e.GetOrigin();
		float ms = 0;
		if (GetGame() && GetGame().GetWorld())
			ms = GetGame().GetWorld().GetWorldTime() - m_fDCOStart_ms;
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(e), string.Format("relocate_end unit=%1 why=%2 result=%3 moved=%4 left_to_goal=%5 ms=%6", DCO_Log.IdUnit(e), m_sDCOWhy, result, Math.Round(vector.DistanceXZ(m_vDCOFrom, p)), Math.Round(vector.DistanceXZ(p, m_vDCOPos)), Math.Round(ms)));
		Complete();
	}

	override float CustomEvaluate()
	{
		EAIActionState st = GetActionState();
		if (m_bDCODone || st == EAIActionState.COMPLETED || st == EAIActionState.FAILED)
			return 0;
		if (GetGame().GetWorld().GetWorldTime() > m_fDCOUntil_ms)
		{
			DCO_Finish("timeout");
			return 0;
		}
		if (m_sDCOWhy == "stress_panic")
			return 1130;
		if (m_bDCONoYield)
			return 91;
		bool y = DCO_AimedYield.TargetVisible(m_Utility, 40.0);
		if (y != m_bDCOYield)
		{
			m_bDCOYield = y;
			DCO_AimedYield.Note(m_Utility, "relocate_close_yield", y);
		}
		if (y)
			return 89;
		return 91;
	}

	override void OnActionDeselected()
	{
		super.OnActionDeselected();
		if (DCO_Util.Teardown("relocate_deselect"))
			return;
		EAIActionState st = GetActionState();
		if (!m_bDCODone && (st == EAIActionState.COMPLETED || st == EAIActionState.FAILED))
			DCO_Finish("cancel");
	}

	override int GetCause()
	{
		return SCR_EAIBehaviorCause.COMBAT;
	}

	override string GetActionDebugInfo()
	{
		return string.Format("%1 DCO relocate %2 to %3", this, m_sDCOWhy, m_vDCOPos);
	}
}

class DCO_AIRelocateStep : SCR_AIActionTask
{
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		DCO_RelocateBehavior b = DCO_RelocateBehavior.Cast(GetExecutedAction());
		if (!b)
			return ENodeResult.FAIL;
		return b.DCO_Step();
	}

	static override bool VisibleInPalette() { return true; }
	static override string GetOnHoverDescription() { return "DCO: sends one combat move request for the relocate behavior (specialist fire spot, displace, crawl to cover, recovery) and returns SUCCESS when it ends."; }
}
