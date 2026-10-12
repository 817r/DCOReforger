class DCO_AimedYield
{
	static bool TargetVisible(SCR_AIUtilityComponent u, float maxDist)
	{
		if (!u || !u.m_CombatComponent || !u.m_OwnerEntity)
			return false;
		BaseTarget t = u.m_CombatComponent.GetCurrentTarget();
		if (!t || !t.GetTargetEntity() || t.GetTimeSinceSeen() > 1.5)
			return false;
		return vector.Distance(t.GetTargetEntity().GetOrigin(), u.m_OwnerEntity.GetOrigin()) <= maxDist;
	}

	static bool CloseUnseen(SCR_AIUtilityComponent u)
	{
		if (!u || !u.m_CombatComponent || !u.m_CombatComponent.DCO_CloseUnseenFire(3000.0))
			return false;
		BaseTarget t = u.m_CombatComponent.GetCurrentTarget();
		return !t || t.GetTimeSinceSeen() > 2.0;
	}

	static void Note(SCR_AIUtilityComponent u, string what, bool yielding)
	{
		DCO_Perf.Count(what + ":" + DCO_Util.B01(yielding).ToString());
		if (!DCO_Log.On(DCO_ELogChannel.TACTIC, DCO_ELogLevel.DEBUG))
			return;
		IEntity e;
		if (u)
			e = u.m_OwnerEntity;
		DCO_Log.Write(DCO_ELogChannel.TACTIC, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(e), string.Format("aimed_yield who=%1 order=%2 yield=%3", DCO_Log.IdUnit(e), what, DCO_Util.B01(yielding)));
	}
}

class DCO_BoundBehavior : SCR_AIBehaviorBase
{
	protected bool m_bDCOYield;
	protected vector m_vDCOGoal;
	protected vector m_vDCOThreat;
	protected vector m_vDCOFrom;
	protected ECharacterStance m_eDCOEndStance;
	protected EMovementType m_eDCOPace;
	protected float m_fDCORushMin;
	protected float m_fDCORushMax;
	protected float m_fDCOPauseMin_ms;
	protected float m_fDCOPauseMax_ms;
	protected float m_fDCOStart_ms;
	protected float m_fDCOUntil_ms;
	protected float m_fDCONextRush_ms;
	protected float m_fDCOZig = 1;
	protected bool m_bDCORushing;
	protected bool m_bDCODone;
	protected int m_iDCORushes;
	protected string m_sDCOTech;
	protected ref SCR_AICombatMoveRequest_Move m_DCORq;
	protected vector m_vDCORushFrom;
	protected float m_fDCORushAt_ms;
	protected int m_iDCOStuck;
	protected int m_iDCOFail;
	protected int m_iDCOCancel;
	protected string m_sDCOFailWhy = "-";
	protected bool m_bDCONoCover;
	protected float m_fDCOFatPause_ms;
	protected SCR_CharacterDamageManagerComponent m_DCODm;

	void DCO_BoundBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector goal, vector threat, ECharacterStance endStance, EMovementType pace, float rushMin, float rushMax, float pauseMin_s, float pauseMax_s, float delay_s, float timeout_s, string tech)
	{
		m_sBehaviorTree = "AI/BehaviorTrees/Chimera/Soldier/DCO_Bound.bt";
		SetPriority(95);
		m_vDCOGoal = goal;
		m_vDCOThreat = threat;
		m_eDCOEndStance = endStance;
		m_eDCOPace = pace;
		m_fDCORushMin = rushMin;
		m_fDCORushMax = rushMax;
		m_fDCOPauseMin_ms = pauseMin_s * 1000.0;
		m_fDCOPauseMax_ms = pauseMax_s * 1000.0;
		m_sDCOTech = tech;
		m_bUseCombatMove = true;
		m_bAllowLook = true;
		m_fDCOStart_ms = GetGame().GetWorld().GetWorldTime();
		m_fDCONextRush_ms = m_fDCOStart_ms + delay_s * 1000.0;
		m_fDCOUntil_ms = m_fDCONextRush_ms + timeout_s * 1000.0;
		if (utility && utility.m_OwnerEntity)
			m_vDCOFrom = utility.m_OwnerEntity.GetOrigin();
	}

	protected bool m_bDCOUnseen;

	bool DCO_UnseenYield()
	{
		bool u = DCO_AimedYield.CloseUnseen(m_Utility);
		if (u != m_bDCOUnseen)
		{
			m_bDCOUnseen = u;
			DCO_AimedYield.Note(m_Utility, "close_unseen_yield", u);
		}
		return u;
	}

	bool DCO_IsPausing()
	{
		return !m_bDCODone && !m_bDCORushing;
	}

	ENodeResult DCO_Step()
	{
		if (m_bDCODone)
			return ENodeResult.SUCCESS;
		IEntity e = m_Utility.m_OwnerEntity;
		SCR_AICombatMoveState st = m_Utility.m_CombatMoveState;
		if (!e || !st)
			return ENodeResult.FAIL;

		float now = GetGame().GetWorld().GetWorldTime();
		if (m_bDCORushing && m_DCORq)
		{
			SCR_EAICombatMoveRequestState rs = m_DCORq.m_eState;
			bool mine = st.GetRequest() == m_DCORq;
			if (mine && (rs == SCR_EAICombatMoveRequestState.EXECUTING || (rs == SCR_EAICombatMoveRequestState.IDLE && now - m_fDCORushAt_ms < 3000.0)))
				return ENodeResult.RUNNING;
			if (!mine && rs != SCR_EAICombatMoveRequestState.SUCCESS && rs != SCR_EAICombatMoveRequestState.FAIL)
				m_DCORq.m_eState = SCR_EAICombatMoveRequestState.CANCELED;
		}
		if (m_bDCORushing)
		{
			m_bDCORushing = false;
			if (m_DCORq && m_DCORq.m_eState == SCR_EAICombatMoveRequestState.FAIL)
			{
				m_iDCOFail++;
				m_sDCOFailWhy = typename.EnumToString(SCR_EAICombatMoveRequestFailReason, m_DCORq.m_eFailReason);
				if (m_DCORq.m_eFailReason == SCR_EAICombatMoveRequestFailReason.COVER_NOT_FOUND)
					m_bDCONoCover = true;
				DCO_Perf.Count("asl:rq_fail");
			}
			else if (m_DCORq && m_DCORq.m_eState == SCR_EAICombatMoveRequestState.CANCELED)
			{
				m_iDCOCancel++;
				DCO_Perf.Count("asl:rq_cancel");
			}
			if (vector.DistanceXZ(m_vDCORushFrom, e.GetOrigin()) < 2.0)
				m_iDCOStuck++;
			else
				m_iDCOStuck = 0;
			if (m_iDCOStuck >= 3)
			{
				DCO_Finish("stuck");
				return ENodeResult.SUCCESS;
			}
			m_fDCONextRush_ms = now + Math.RandomFloat(m_fDCOPauseMin_ms, m_fDCOPauseMax_ms) + m_fDCOFatPause_ms;
			return ENodeResult.RUNNING;
		}
		if (now < m_fDCONextRush_ms || st.IsExecutingRequest())
			return ENodeResult.RUNNING;

		vector p = e.GetOrigin();
		vector to = m_vDCOGoal - p;
		to[1] = 0;
		float rem = to.Length();
		if (rem < 3.0)
		{
			DCO_Finish("arrived");
			return ENodeResult.SUCCESS;
		}

		float exert = 0;
		DCO_AIMoraleSystem fms = m_Utility.GetMoraleSystem();
		if (fms)
		{
			fms.AddExertion();
			exert = fms.GetExertion();
		}
		float stam = DCO_AIMoraleSystem.Stamina(e);
		float fe = Math.Clamp((exert - 4.0) / 6.0, 0, 1);
		float fs = 0;
		if (stam >= 0)
			fs = Math.Clamp((0.6 - stam) / 0.4, 0, 1);
		float fat = Math.Max(fe, fs);
		m_fDCOFatPause_ms = fat * Math.RandomFloat(3000.0, 6000.0);
		EMovementType pace = m_eDCOPace;
		if (fat >= 0.9 && fs >= 0.75 && pace == EMovementType.SPRINT)
		{
			pace = EMovementType.RUN;
			DCO_Perf.Count("fatigue:run_pace");
		}
		float len = Math.Min(Math.RandomFloat(m_fDCORushMin, m_fDCORushMax) * (1.0 - 0.4 * fat), rem);
		if (fat > 0)
		{
			DCO_Perf.Count("fatigue:rush_short");
			string src = "rush";
			if (fs > fe)
				src = "stamina";
			DCO_Log.Write(DCO_ELogChannel.TACTIC, DCO_ELogLevel.INFO, DCO_Log.IdUnit(e), string.Format("fatigue_rush unit=%1 F=%2 src=%3 E=%4 stam=%5 len=%6 pause=%7 pace=%8",
				DCO_Log.IdUnit(e), fat.ToString(1, 2), src, exert.ToString(1, 1), stam.ToString(1, 2), Math.Round(len), (m_fDCOFatPause_ms / 1000.0).ToString(1, 1), typename.EnumToString(EMovementType, pace)));
		}
		vector dir = to * (1.0 / rem);
		vector side = Vector(dir[2], 0, -dir[0]);
		vector movePos = p + dir * len;
		if (rem > len + 2.0)
		{
			m_fDCOZig = -m_fDCOZig;
			movePos = movePos + side * (m_fDCOZig * Math.RandomFloat(2.0, 4.0));
		}

		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
		rq.m_eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
		rq.m_vMovePos = DCO_Util.Surface(movePos);
		rq.m_vTargetPos = m_vDCOThreat;
		rq.m_bTryFindCover = !m_bDCONoCover;
		rq.m_bCheckCoverVisibility = true;
		rq.m_bFailIfNoCover = false;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_fCoverSearchSectorHalfAngleRad = 0.6;
		rq.m_fCoverSearchDistMin = len * 0.6;
		rq.m_fCoverSearchDistMax = len * 1.3;
		rq.m_eStanceMoving = ECharacterStance.STAND;
		rq.m_eStanceEnd = m_eDCOEndStance;
		rq.m_eMovementType = pace;
		rq.m_bAimAtTarget = false;
		rq.m_bAimAtTargetEnd = true;
		rq.m_fMoveDuration_s = len / 2.5 + 3.0;
		st.ApplyNewRequest(rq);
		m_DCORq = rq;
		m_vDCORushFrom = p;
		m_fDCORushAt_ms = GetGame().GetWorld().GetWorldTime();
		m_bDCORushing = true;
		m_iDCORushes++;
		DCO_Perf.Count("asl:rush");
		return ENodeResult.RUNNING;
	}

	void DCO_Supersede()
	{
		if (!m_bDCODone)
			DCO_Finish("superseded");
	}

	protected void DCO_Finish(string why)
	{
		if (m_bDCODone)
			return;
		m_bDCODone = true;
		DCO_Perf.Count("asl:bound_" + why);
		IEntity e = m_Utility.m_OwnerEntity;
		string grp = "-";
		AIAgent agent = m_Utility.GetAIAgent();
		if (agent && agent.GetParentGroup())
			grp = agent.GetParentGroup().GetName();
		vector p = m_vDCOFrom;
		if (e)
			p = e.GetOrigin();
		string stance = "-";
		CharacterControllerComponent ctrl = m_Utility.GetCharacterController();
		if (ctrl)
			stance = typename.EnumToString(ECharacterStance, ctrl.GetStance());
		DCO_Log.Write(DCO_ELogChannel.TACTIC, DCO_ELogLevel.INFO, "-", string.Format("assault_member grp=%1 who=%2 tech=%3 reason=%4 from=%5 to=%6 moved=%7 rushes=%8 ms=%9",
			grp, DCO_Log.IdUnit(e), m_sDCOTech, why, string.Format("%1,%2", Math.Round(m_vDCOFrom[0]), Math.Round(m_vDCOFrom[2])), string.Format("%1,%2", Math.Round(p[0]), Math.Round(p[2])),
			Math.Round(vector.DistanceXZ(m_vDCOFrom, p)), m_iDCORushes, Math.Round(GetGame().GetWorld().GetWorldTime() - m_fDCOStart_ms)) + " stance=" + stance + " enemy_dist=" + Math.Round(vector.DistanceXZ(p, m_vDCOThreat))
			+ string.Format(" fail=%1 cancel=%2 failwhy=%3 exec=%4", m_iDCOFail, m_iDCOCancel, m_sDCOFailWhy, DCO_ExecName()));
		if (why == "pinned")
			Fail();
		else
			Complete();
	}

	protected string DCO_ExecName()
	{
		AIActionBase x = m_Utility.GetExecutedAction();
		if (!x)
			return "-";
		return x.ClassName();
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
		SCR_AICombatComponent cc = m_Utility.m_CombatComponent;
		if (cc && cc.DCO_IsPinned())
		{
			DCO_Finish("pinned");
			return 0;
		}
		if (DCO_UnseenYield())
			return 60;
		float yieldDist = 40.0;
		string yieldWhat = "bound_close_yield";
		if (m_sDCOTech == "assault_through")
		{
			yieldDist = 150.0;
			yieldWhat = "assault_fire_yield";
		}
		bool y = DCO_AimedYield.TargetVisible(m_Utility, yieldDist);
		if (!y && m_Utility.m_OwnerEntity)
		{
			if (!m_DCODm)
				m_DCODm = SCR_CharacterDamageManagerComponent.Cast(m_Utility.m_OwnerEntity.FindComponent(SCR_CharacterDamageManagerComponent));
			y = m_DCODm && m_DCODm.DCO_HitsWithin(3.0) > 0 && DCO_AimedYield.TargetVisible(m_Utility, 300.0);
		}
		if (!y && !m_bDCORushing && GetGame().GetWorld().GetWorldTime() < m_fDCONextRush_ms && DCO_AimedYield.TargetVisible(m_Utility, 500.0))
		{
			y = true;
			yieldWhat = "bound_pause_fire";
		}
		if (y != m_bDCOYield)
		{
			m_bDCOYield = y;
			DCO_AimedYield.Note(m_Utility, yieldWhat, y);
		}
		if (y)
			return 89;
		return 95;
	}

	override void OnActionDeselected()
	{
		super.OnActionDeselected();
		if (DCO_Util.Teardown("assault_deselect"))
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
		return string.Format("%1 DCO bound %2 rushes=%3 to %4", this, m_sDCOTech, m_iDCORushes, m_vDCOGoal);
	}
}


class DCO_AIBoundRush : SCR_AIActionTask
{
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		DCO_BoundBehavior b = DCO_BoundBehavior.Cast(GetExecutedAction());
		if (!b)
			return ENodeResult.FAIL;
		return b.DCO_Step();
	}

	static override bool VisibleInPalette() { return true; }
	static override string GetOnHoverDescription() { return "DCO: runs the bound as short rushes (combat move requests with cover search), returns SUCCESS when the bound is done."; }
}
