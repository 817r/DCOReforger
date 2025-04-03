modded class SCR_AIDangerReaction_DamageTaken : SCR_AIDangerReaction
{
	protected SCR_AICombatMoveState m_State;
	protected const float COVER_QUERY_SECTOR_ANGLE_RAD = 0.3 * Math.PI;
	
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{
		if (!super.PerformReaction(utility, threatSystem, dangerEvent, dangerEventCount)) return false;
		
		m_State = utility.m_CombatMoveState;
		
		vector ShooterPos = dangerEvent.GetObject().GetRootParent().GetOrigin();
		float dist = vector.Distance(utility.GetOrigin(), ShooterPos);
		
		if (dist < SCR_AICombatComponent.CLOSE_RANGE_COMBAT_DISTANCE && threatSystem.BulletHitCount() > 1)
		{
			if (m_State.m_bInCover)
			{
				if (m_State.m_bExposedInCover)
				{
					m_State.ApplyRequestChangeStanceInCover(true);
					return false;
				}
				else
				{
					SCR_AICombatMoveRequest_Move rq = DangerDamageTakenCombatMoveCQB(ShooterPos);
				
					m_State.ApplyNewRequest(rq);
					return false;
				}
			} else
			{
				if (utility.GetCharacterController().GetStance() == ECharacterStance.PRONE)
				{
					utility.GetCharacterController().SetRoll(Math.RandomInt(1,3));
					return false;
				} else
				{				
					SCR_AICombatMoveRequest_Move rq = DangerDamageTakenCombatMoveNotInCoverCQB(ShooterPos);
				
					m_State.ApplyNewRequest(rq);				
					return false;
				}
			}
		} else
		{
			if (m_State.m_bInCover)
			{
				if (!m_State.IsHidingInValidCover())
				{
					m_State.ApplyRequestChangeStanceInCover(true);
					return false;
				}
				else
				{
					SCR_AICombatMoveRequest_Move rq = DangerDamageTakenCombatMove(ShooterPos);
				
					m_State.ApplyNewRequest(rq);
					return false;
				}
			} else
			{
				if (utility.GetCharacterController().GetStance() == ECharacterStance.PRONE)
				{
					utility.GetCharacterController().SetRoll(Math.RandomInt(1,3));
					return false;
				} else
				{				
					SCR_AICombatMoveRequest_Move rq = DangerDamageTakenCombatMoveNotInCover(ShooterPos);
				
					m_State.ApplyNewRequest(rq);				
					return false;
				}
			}		
		}
		
		return true;
	}
	
	// LONG RANGE
	SCR_AICombatMoveRequest_Move DangerDamageTakenCombatMove(vector ShooterPos)
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
		rq.m_vTargetPos = ShooterPos;
		rq.m_vMovePos = ShooterPos;
		rq.m_bTryFindCover = true;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility = true;
		rq.m_bFailIfNoCover = false;
		rq.m_eStanceMoving = ECharacterStance.STAND;
		rq.m_eStanceEnd = ECharacterStance.CROUCH;
		rq.m_eMovementType = EMovementType.RUN;
		rq.m_fCoverSearchDistMax = 12;
		rq.m_fCoverSearchDistMin = 3;				
		rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 5.5);
		rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
		rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
		rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
		rq.m_bAimAtTargetEnd = true;
		
		return rq;
				
	}
	
	SCR_AICombatMoveRequest_Move DangerDamageTakenCombatMoveNotInCover(vector ShooterPos)
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
		rq.m_vTargetPos = ShooterPos;
		rq.m_vMovePos = ShooterPos;
		rq.m_bTryFindCover = true;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility = true;
		rq.m_bFailIfNoCover = false;
		rq.m_eStanceMoving = ECharacterStance.STAND;
		rq.m_eStanceEnd = ECharacterStance.CROUCH;
		rq.m_eMovementType = EMovementType.RUN;
		rq.m_fCoverSearchDistMax = 12;
		rq.m_fCoverSearchDistMin = 3;				
		rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 5.5);
		rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
		rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
		rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
		rq.m_bAimAtTargetEnd = true;
		
		return rq;
				
	}
	
	// CQB
	SCR_AICombatMoveRequest_Move DangerDamageTakenCombatMoveCQB(vector ShooterPos)
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
		rq.m_vTargetPos = ShooterPos;
		rq.m_vMovePos = ShooterPos;
		rq.m_bTryFindCover = true;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility = true;
		rq.m_bFailIfNoCover = true;
		rq.m_eStanceMoving = ECharacterStance.CROUCH;
		rq.m_eStanceEnd = ECharacterStance.CROUCH;
		rq.m_eMovementType = EMovementType.WALK;
		rq.m_fCoverSearchDistMax = 5;
		rq.m_fCoverSearchDistMin = 3;				
		rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 3.5);
		rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
		rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
		rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
		rq.m_bAimAtTargetEnd = true;
		
		return rq;
				
	}
	
	SCR_AICombatMoveRequest_Move DangerDamageTakenCombatMoveNotInCoverCQB(vector ShooterPos)
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
		rq.m_vTargetPos = ShooterPos;
		rq.m_vMovePos = ShooterPos;
		rq.m_bTryFindCover = true;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility = true;
		rq.m_bFailIfNoCover = false;
		rq.m_eStanceMoving = ECharacterStance.CROUCH;
		rq.m_eStanceEnd = ECharacterStance.CROUCH;
		rq.m_eMovementType = EMovementType.WALK;
		rq.m_fCoverSearchDistMax = 5;
		rq.m_fCoverSearchDistMin = 3;				
		rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 3.5);
		rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
		rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
		rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
		rq.m_bAimAtTargetEnd = true;
		
		return rq;
				
	}
};