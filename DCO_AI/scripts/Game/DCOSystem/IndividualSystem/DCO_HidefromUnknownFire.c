modded class SCR_AICombatMoveLogic_HideFromUnknownFire : AITaskScripted
{
	override void CombatMoveLogic(vector threatPos, float distToThreat)
	{
		if (!m_State)
			return;
		
		if (SCR_CoverManagerComponent.IsEntityInsideBuilding(m_Utility.m_OwnerEntity))
			return;
		
		if (!m_State.IsMoving() && !m_State.IsInValidCover() && !m_bPushedMoveRequest)
		{
			// Standing not in cover
			
			SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
			
			rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
			
			rq.m_vTargetPos = threatPos;
			rq.m_vMovePos = rq.m_vTargetPos;
			rq.m_bTryFindCover = true;
			rq.m_bUseCoverSearchDirectivity = false;
			rq.m_bCheckCoverVisibility = false;
			rq.m_bFailIfNoCover = false;
			rq.m_eStanceMoving = ECharacterStance.STAND;
			rq.m_eStanceEnd = ECharacterStance.CROUCH;
			rq.m_eMovementType = EMovementType.SPRINT;
			rq.m_fCoverSearchDistMax = COVER_SEARCH_DIST_MAX;
			rq.m_fCoverSearchDistMin = 0;
			rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * COVER_SEARCH_DIST_MAX / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;
			rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
			rq.m_fCoverSearchSectorHalfAngleRad = 0.75 * Math.PI;
			
			rq.m_bAimAtTarget = false; // Don't aim while running
			rq.m_bAimAtTargetEnd = true;
			
			m_State.ApplyNewRequest(rq);
			m_bPushedMoveRequest = true;
		}
		else if (!m_State.IsMoving())
		{
			// We are not moving, manage our stance based on threat and range
			EAIThreatState threat = m_Utility.m_ThreatSystem.GetState();
			
			bool closeRangeCombat = distToThreat < SCR_AICombatMoveUtils.CLOSE_RANGE_COMBAT_DIST;
			
			if (closeRangeCombat)
			{
				if (m_State.m_bInCover)
				{
					bool newExposedInCover = threat != EAIThreatState.THREATENED;
					
					if (m_State.m_bExposedInCover != newExposedInCover)
						PushRequesChangeStanceInCover(newExposedInCover);
				}
				else
				{
					ECharacterStance newStance;
				
					if (threat == EAIThreatState.THREATENED)
						newStance = ECharacterStance.CROUCH;
					else
						newStance = ECharacterStance.STAND;
					
					if (newStance != m_CharacterController.GetStance())
						m_State.ApplyRequestChangeStanceOutsideCover(newStance);
				}
			}
			else
			{
				ECharacterStance newStance;
				
				if (threat == EAIThreatState.THREATENED)
					newStance = ECharacterStance.PRONE;
				else
					newStance = ECharacterStance.CROUCH;
				
				if (newStance != m_CharacterController.GetStance())
						m_State.ApplyRequestChangeStanceOutsideCover(newStance);
			}
		}
	}
}