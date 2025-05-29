modded class SCR_AICombatMoveLogic_Suppressive : SCR_AICombatMoveLogicBase
{
	protected static const float TIME_SINCE_GOOD_VISIBILITY_MIN_MS = 20000.0;
	protected MoraleState m_MoraleState;
	
	
	//--------------------------------------------------------------------------------------------
	protected override float ResolveStoppedWaitTime(bool inCover, EAIThreatState threat, EWeaponType weaponType)
	{
		m_MoraleState = m_CombatComp.GetMoraleComponent().GetMoraleStates();
		float waitTime;
		const float minRandomTime = 1;
		const float maxRandomTime = 8;
		
		if (inCover)
		{
			// In cover
			switch (threat)
			{
				case EAIThreatState.THREATENED:
					waitTime = Math.RandomFloat(minRandomTime, maxRandomTime * 4);	// Stay in cover for a long time, until we are not suppressed any more
					break;
				default:
					waitTime = Math.RandomFloat(minRandomTime, maxRandomTime);
			}
		}
		else
		{
			// Not in cover
			switch (threat)
			{
				case EAIThreatState.THREATENED:
					waitTime = Math.RandomFloat(minRandomTime, maxRandomTime);
					break;
				default:
					waitTime = Math.RandomFloat(minRandomTime, minRandomTime + 3);
					break;
			}
		}
		
		bool longWaitTime = false;
		switch (weaponType)
		{
			case EWeaponType.WT_MACHINEGUN:
			case EWeaponType.WT_SNIPERRIFLE:
				longWaitTime = true;
		}
		
		if (longWaitTime) waitTime += Math.RandomFloat(minRandomTime, maxRandomTime / 2);
		
		switch (m_MoraleState)
		{
			case MoraleState.FRESH:
			{
				waitTime *= 0.5;
				break;
			}
			case MoraleState.NORMAL:
			{
				waitTime *= 0.8;
				break;
			}
			case MoraleState.STRESSED:
			{
				waitTime *= 1.2;
				break;
			}
			case MoraleState.PRESSURED:
			{
				waitTime *= 1.8;
				break;
			}
			case MoraleState.BREAK:
			{
				waitTime *= 2.2;
				break;
			}
		}
		
		return waitTime;
	}
	
	override protected void PushRequestMove()
	{		
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
		
		// Common values
		rq.m_vTargetPos = ResolveRequestTargetPos();
		ResolveMoveRequestMovePosAndDir(rq.m_vTargetPos, rq.m_vMovePos, rq.m_vAvoidStraightPathDir, rq.m_eDirection, rq.m_fCoverSearchSectorHalfAngleRad);
		rq.m_bTryFindCover = true;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility = true;

		float coverSearchDistMin = 0;
		float coverSearchDistMax = 30;
		float moveDurationMax = 6;
		if (m_bCloseRangeCombat)
		{
			// Close range combat
			
			switch (m_eThreatState)
			{
				case EAIThreatState.THREATENED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					coverSearchDistMin = 2.0;
					coverSearchDistMax = 10.0;
					moveDurationMax = 2;
					break;
				}
				default:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 15.0;
					moveDurationMax = 3;
					break;
				}
			}
			
			rq.m_eMovementType = EMovementType.WALK;
			rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType) &&
								IsAimingAndMovingAllowedForWeapon(m_eWeaponType);
			rq.m_bAimAtTargetEnd = true;
		}
		else
		{
			// Long range combat
			
			switch (m_eThreatState)
			{
				case EAIThreatState.THREATENED:
				{
					coverSearchDistMin = 2.0;
					coverSearchDistMax = 20.0;
					moveDurationMax = 2.5;
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.PRONE;
					break;
				}
				default:
				{
					coverSearchDistMin = 10.0;
					coverSearchDistMax = 30.0;
					moveDurationMax = 4; // Shouldn't be so large because we are sprinting and can't shoot
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					break;
				}
			}
			
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType) &&
								IsAimingAndMovingAllowedForWeapon(m_eWeaponType); // Can't aim at tgt while sprinting
			rq.m_bAimAtTargetEnd = true;
		}
		
		rq.m_bFailIfNoCover = ResolveFailMoveIfNoCover();
		
		// If we are not in cover, min cover search distance is overridden to 0, we should find any cover ASAP
		if (!m_State.m_bInCover)
			coverSearchDistMin = 0;
		
		rq.m_fCoverSearchDistMin = coverSearchDistMin;
		rq.m_fCoverSearchDistMax = coverSearchDistMax;
		rq.m_fMoveDuration_s = Math.RandomFloat(0.3, 1.0) * moveDurationMax;
		
		rq.GetOnMovementStarted().Insert(OnMovementStarted);
		rq.GetOnCompleted().Insert(OnMovementCompleted);
		
		m_State.ApplyNewRequest(rq);
	}
}