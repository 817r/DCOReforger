modded class SCR_AIHealBehavior : SCR_AIBehaviorBase
{
	protected const float MAX_TIME_TO_UNCON_HIGH_PRIORITY_S = 16;
	protected const float DCO_HEAL_NOW_S = 8;
	protected const float DCO_COVER_SEARCH_DIST = 20;
	protected const float DCO_COVER_MOVE_COOLDOWN_S = 6;

	override float CustomEvaluate()
	{
		if (!(GetGame().GetWorld().GetWorldTime() - m_fTimeCreated_ms > m_fPriorityDelay_ms))
			return 0;

		float timeToUncon = float.MAX;
		if (m_AIInfo)
			timeToUncon = m_AIInfo.GetBleedTimeToUnconscious();

		bool calm = m_Utility.m_ThreatSystem.GetThreatMeasureWithoutInjuryFactor() < SCR_AIThreatSystem.VIGILANT_THRESHOLD;

		if (!calm && !DCO_IsSheltered())
		{
			if (timeToUncon > DCO_HEAL_NOW_S)
			{
				DCO_RequestCover();
				return 0;
			}

			return PRIORITY_BEHAVIOR_HEAL_HIGH_PRIORITY * 2;
		}

		if (timeToUncon < MAX_TIME_TO_UNCON_HIGH_PRIORITY_S)
			return PRIORITY_BEHAVIOR_HEAL_HIGH_PRIORITY * 2;

		return GetPriority();
	}

	protected bool DCO_IsSheltered()
	{
		SCR_AICombatMoveState state = m_Utility.m_CombatMoveState;
		if (state && state.IsInValidCover())
			return true;

		return m_Utility.m_OwnerEntity && SCR_CoverManagerComponent.IsEntityInsideBuilding(m_Utility.m_OwnerEntity);
	}

	protected void DCO_RequestCover()
	{
		SCR_AICombatMoveState state = m_Utility.m_CombatMoveState;
		IEntity owner = m_Utility.m_OwnerEntity;
		if (!state || !owner || state.IsExecutingRequest() || !DCO_CoverMoveBudget.CanMove(owner, DCO_COVER_MOVE_COOLDOWN_S))
			return;

		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
		rq.m_vTargetPos = owner.GetOrigin();
		rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;

		BaseTarget enemy;
		if (m_Utility.m_CombatComponent)
			enemy = m_Utility.m_CombatComponent.GetCurrentTarget();
		if (enemy)
		{
			rq.m_vTargetPos = enemy.GetLastSeenPosition();
			rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
		}

		rq.m_vMovePos = rq.m_vTargetPos;
		rq.m_bTryFindCover = true;
		rq.m_bUseCoverSearchDirectivity = enemy != null;
		rq.m_bCheckCoverVisibility = false;
		rq.m_bFailIfNoCover = false;
		rq.m_fCoverSearchDistMin = 0;
		rq.m_fCoverSearchDistMax = DCO_COVER_SEARCH_DIST;
		rq.m_eStanceMoving = ECharacterStance.CROUCH;
		rq.m_eMovementType = EMovementType.SPRINT;
		rq.m_fMoveDuration_s = DCO_COVER_SEARCH_DIST / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;
		rq.m_bAimAtTarget = false;
		rq.m_bAimAtTargetEnd = false;
		state.DCO_SetDodgeStance(rq);

		DCO_CoverMoveBudget.MarkMove(owner);
		state.ApplyNewRequest(rq);
	}
};
