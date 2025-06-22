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
		const float maxRandomTime = 4;
		
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
				waitTime *= 1.6;
				break;
			}
			case MoraleState.BREAK:
			{
				waitTime *= 2.0;
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
	
	protected override void ResolveMoveRequestMovePosAndDir(vector targetPos, out vector outMovePos, out vector outAvoidStraightPathDir, out SCR_EAICombatMoveDirection outDirection, out float outCoverSearchSectorHalfAngleRad)
	{	
		AIWaypoint wp = null;
		AIAgent agent = m_Utility.GetAIAgent();
		AIGroup group = agent.GetParentGroup();
		AIAgent Leader = group.GetLeaderAgent();
		if (group)
			wp = group.GetCurrentWaypoint();
		
		vector movePos;
		SCR_EAICombatMoveDirection eDirection;
		float coverSearchSectorHalfAngleRad;
		vector avoidStraightPathDir;
		
		if (!wp || SCR_EntityWaypoint.Cast(wp))
		{
			// No waypoint, or it's an entity-associated waypoint, like Follow waypoint.
			// Therefore use standard movement logic.
			// Otherwise they will want to run towards position where the waypoint is placed, which makes no sense.
			movePos = targetPos;
			avoidStraightPathDir = GetAvoidStraightPathDir(); // Use flanking
			
			switch(m_eThreatState)
			{
				case EAIThreatState.SAFE:
				{
					eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
					break;
				}
				case EAIThreatState.VIGILANT:
				{
					eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
					break;		
				}
				case EAIThreatState.ALERTED:
				{
					eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
					avoidStraightPathDir = vector.Zero;
					break;					
				}
				case EAIThreatState.THREATENED:
				{
					eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
					avoidStraightPathDir = vector.Zero;
					break;			
				}
			}
			
			if (IsFirstExecution())
					coverSearchSectorHalfAngleRad = Math.PI; // Full circle, on first run we just want any cover if possible
				else
					coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
		}
		else
		{
			vector wpPos = wp.GetOrigin();
			float wpRadius = wp.GetCompletionRadius();
			bool tgtInWaypoint = vector.DistanceXZ(wpPos, targetPos) < wpRadius;
			float myDistToWp = vector.DistanceXZ(wpPos, m_MyEntity.GetOrigin());
			
			if (myDistToWp > wpRadius)
			{
				// We are outside WP, move towards center
				movePos = wpPos;
				eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
				coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
				avoidStraightPathDir = vector.Zero; // Go straight
			}
			else if (myDistToWp > 0.5 * wpRadius)
			{
				// We are between 50% and 100% of wp radius
				
				if (tgtInWaypoint)
				{
					// Towards target
					movePos = targetPos;
					eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
					avoidStraightPathDir = GetAvoidStraightPathDir(); // Use flanking
					coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
				}
				else
				{
					// Move around current pos.
					movePos = targetPos;
					eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
					avoidStraightPathDir = vector.Zero;
					coverSearchSectorHalfAngleRad = -1.0;
				}
			}
			else
			{
				// We are within 50% radius of wp,
				// Move towards tgt, regardless where tgt is
				movePos = targetPos;
				eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
				avoidStraightPathDir = GetAvoidStraightPathDir();
				
				coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
			}
		}
		
		outMovePos = movePos;
		outDirection = eDirection;
		outAvoidStraightPathDir = avoidStraightPathDir;
		outCoverSearchSectorHalfAngleRad = coverSearchSectorHalfAngleRad;
	}
}