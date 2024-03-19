modded class SCR_AICombatMoveLogic_MoveFromGrenade : AITaskScripted
{
	override void CombatMoveLogic(vector threatPos, out bool outCompleteAction)
	{
		if (!m_State)
			return;
		
		bool completeAction = false;
		
		if (ConditionPushMoveRequest(threatPos))
		{
			SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
			
			rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
			
			rq.m_vTargetPos = threatPos;
			rq.m_vMovePos = rq.m_vTargetPos;
			rq.m_bTryFindCover = true;
			rq.m_bUseCoverSearchDirectivity = true;
			rq.m_bCheckCoverVisibility = false;
			rq.m_bFailIfNoCover = false;
			rq.m_eStanceMoving = ECharacterStance.STAND;
			rq.m_eStanceEnd = ECharacterStance.PRONE;
			rq.m_eMovementType = EMovementType.SPRINT;
			rq.m_fCoverSearchDistMax = 15;
			rq.m_fCoverSearchDistMin = 5;
			rq.m_fMoveDistance = rq.m_fCoverSearchDistMax;
			int rand = Math.RandomIntInclusive(1, 3);
			if (rand == 1) rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
			else if (rand == 2) rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
			else if (rand == 3) rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
			rq.m_fCoverSearchSectorHalfAngleRad = Math.PI_HALF; // 90 deg
			rq.m_bAimAtTarget = false;
			rq.m_bAimAtTargetEnd = true;
			
			m_State.ApplyNewRequest(rq);
		}
		else if (m_State.m_bInCover && m_State.IsAssignedCoverValid() && IsCoverSafeAgainstGrenade(m_State.GetAssignedCover(), threatPos))
		{
			// Found good enough cover, complete the action
			completeAction = true;
			return;
		}
		
		outCompleteAction = completeAction;
	}

	override bool IsCoverSafeAgainstGrenade(notnull SCR_AICoverLock coverLock, vector threatPos)
	{
		float coverDistToThreatSq = vector.DistanceSq(threatPos, coverLock.m_vCoverPos);
		
		// So close is not safe
		if (coverDistToThreatSq < 5*5) // TODO make it constant
			return false;
		
		// Far enough, check angle
		float cosAngleCoverToTgt = m_State.GetAssignedCover().CosAngleToThreat(threatPos);
		return cosAngleCoverToTgt > 0.707; // This should match or be close to value in cover query properties
	}
}