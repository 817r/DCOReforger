modded class SCR_AICombatMoveUtils
{
	static const float CLOSE_RANGE_COMBAT_DIST = 30.0;
}


modded class SCR_AICombatMoveLogic_Attack : AITaskScripted
{

	protected const float COVER_QUERY_SECTOR_ANGLE_RAD = 1.0;//0.3 * Math.PI;

	override protected void PushRequestMove()
	{		
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
		
		// Common values
		rq.m_vTargetPos = ResolveRequestTargetPos();
		ResolveMoveRequestMovePosAndDir(rq.m_vTargetPos, rq.m_vMovePos, rq.m_eDirection, rq.m_fCoverSearchSectorHalfAngleRad);
		rq.m_bTryFindCover = true;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility = true;

		float coverSearchDistMin = 2;
		float coverSearchDistMax = 35;
		float moveDistanceMax = 10;
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
					coverSearchDistMax = 8.0;
					moveDistanceMax = 3.0;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					coverSearchDistMin = 2.0;
					coverSearchDistMax = 8.0;
					moveDistanceMax = 5.0;
					break;
				}
				case EAIThreatState.VIGILANT:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					coverSearchDistMin = 2.0;
					coverSearchDistMax = 10.0;
					moveDistanceMax = 7.0;
					break;
				}
				default:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					coverSearchDistMin = 2.0;
					coverSearchDistMax = 12.0;
					moveDistanceMax = 10.0;
					break;
				}
			}
			
			rq.m_bFailIfNoCover = false;
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
					coverSearchDistMax = 8.0;
					moveDistanceMax = 5.0;
					rq.m_eStanceMoving = ECharacterStance.PRONE;
					rq.m_eMovementType = EMovementType.RUN;
					rq.m_eStanceEnd = ECharacterStance.PRONE;
					rq.m_bAimAtTarget = true;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					coverSearchDistMin = 2.0;
					coverSearchDistMax = 12.0;
					moveDistanceMax = 8.0;
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eMovementType = EMovementType.RUN;
					rq.m_eStanceEnd = ECharacterStance.PRONE;
					rq.m_bAimAtTarget = true;
					break;
				}
				case EAIThreatState.VIGILANT:
				{
					coverSearchDistMin = 2.0;
					coverSearchDistMax = 20.0;
					moveDistanceMax = 12.0;
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eMovementType = EMovementType.SPRINT;
					rq.m_eStanceEnd = ECharacterStance.PRONE;
					rq.m_bAimAtTarget = false;
					break;
				}
				default:
				{
					coverSearchDistMin = 2.0;
					coverSearchDistMax = 20.0;
					moveDistanceMax = 15.0; // Shouldn't be so large because we are sprinting and can't shoot
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eMovementType = EMovementType.SPRINT;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_bAimAtTarget = false;
					break;
				}
			}
			
			if (IsFirstExecution())
				rq.m_bFailIfNoCover = true; // On first run we want to move to cover, or stay where we are if there is no cover, and shoot.
			else
				rq.m_bFailIfNoCover = m_State.m_bInCover; // Don't leave cover if there is no next cover

			// rq.m_bAimAtTarget = false; // Can't aim at tgt while sprinting
			rq.m_bAimAtTargetEnd = true;
		}
		
		// If we are not in cover, min cover search distance is overridden to 0, we should find any cover ASAP
		if (!m_State.m_bInCover)
			coverSearchDistMin = 0;
		
		rq.m_fCoverSearchDistMin = coverSearchDistMin;
		rq.m_fCoverSearchDistMax = coverSearchDistMax;
		rq.m_fMoveDistance = Math.RandomFloat(0.2, 1.2) * moveDistanceMax; // Move distance if cover is not found, randomized
		
		// Subscribe to events
		// We will pronounce voice lines once we start or end moving
		rq.GetOnMovementStarted().Insert(OnMovementStarted);
		rq.GetOnCompleted().Insert(OnMovementCompleted);
		
		m_State.ApplyNewRequest(rq);
	}

	// Resolves which move pos and dir. we should use for _MOVE_ request
	// By now rq.m_vTargetPos must be already calculated!
	override protected void ResolveMoveRequestMovePosAndDir(vector targetPos, out vector outMovePos, out SCR_EAICombatMoveDirection outDirection, out float outCoverSearchSectorHalfAngleRad)
	{	
		AIWaypoint wp = null;
		AIAgent agent = m_Utility.GetAIAgent();
		AIGroup group = agent.GetParentGroup();
		if (group)
			wp = group.GetCurrentWaypoint();
		
		vector movePos;
		SCR_EAICombatMoveDirection eDirection;
		float coverSearchSectorHalfAngleRad;
		
		if (!wp)
		{
			// No waypoint, standard move logic
			movePos = targetPos;
			eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
			
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
			}
			else if (myDistToWp > 0.5 * wpRadius)
			{
				// We are between 50% and 100% of wp radius
				
				if (tgtInWaypoint)
				{
					// Towards target
					movePos = targetPos;
					eDirection = SCR_EAICombatMoveDirection.FORWARD;
					coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
				}
				else
				{
					// Move around current pos.
					movePos = targetPos;
					eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
					coverSearchSectorHalfAngleRad = -1.0;
				}
			}
			else
			{
				// We are within 50% radius of wp,
				// Move towards tgt, regardless where tgt is
				movePos = targetPos;
				eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
				
				coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
			}
		}
		
		outMovePos = movePos;
		outDirection = eDirection;
		outCoverSearchSectorHalfAngleRad = coverSearchSectorHalfAngleRad;
	}

	override protected void SuppressedInCoverLogic()
	{
		// We're pinned in cover and can't do much else now
		// Alternate hiding in cover and unhiding
		
		// How long to wait here? Depends on timer value from previous request.
		
		float waitTime_s;
		SCR_AICombatMoveRequestBase rq = m_State.GetRequest();
		if (SCR_AICombatMoveRequest_ChangeStanceInCover.Cast(rq) && rq.m_eReason == SCR_EAICombatMoveReason.SUPPRESSED_IN_COVER)
			waitTime_s = rq.m_f_UserTimer_s;
		else
		{
			if (m_State.m_bExposedInCover)
				waitTime_s = 1.2;
			else
				waitTime_s = 5.0;
		}
		
		if (m_State.m_bExposedInCover && m_State.m_fTimerRequest_s > waitTime_s)
		{
			float newWaitTime = Math.RandomFloat(5.0, 9.0); // Hide in cover for this time
			PushRequestChangeStanceInCover(false, SCR_EAICombatMoveReason.SUPPRESSED_IN_COVER, newWaitTime);
		}
		else if (!m_State.m_bExposedInCover && m_State.m_fTimerRequest_s > waitTime_s)
		{
			float newWaitTime = Math.RandomFloat(1.2, 3.0); // Expose out of cover for this time
			PushRequestChangeStanceInCover(true, SCR_EAICombatMoveReason.SUPPRESSED_IN_COVER, newWaitTime);
		}
	}

	override protected void PushRequestLeaveUselessCover()
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
		
		rq.m_vTargetPos = ResolveRequestTargetPos();
		rq.m_vMovePos = rq.m_vTargetPos;
		rq.m_bTryFindCover = true;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility = true;
		rq.m_bFailIfNoCover = true;
		rq.m_eStanceMoving = ECharacterStance.STAND;
		rq.m_eStanceEnd = ECharacterStance.PRONE;
		rq.m_eMovementType = EMovementType.RUN;
		rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD; // Move back from target
		rq.m_fCoverSearchSectorHalfAngleRad = -1.0;
		rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
		rq.m_bAimAtTargetEnd = true;
		
		rq.m_fCoverSearchDistMin = 10;
		rq.m_fCoverSearchDistMax = 30;
		
		m_State.ApplyNewRequest(rq);
	}

	override protected void PushRequestMoveFromTarget()
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_TARGET;
		
		rq.m_vMovePos = ResolveRequestTargetPos();
		rq.m_eMovementType = EMovementType.RUN;
		rq.m_eStanceMoving = ECharacterStance.STAND;
		rq.m_eStanceEnd = ECharacterStance.CROUCH;
		rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
		rq.m_fMoveDistance = 10.0;
		rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
		rq.m_bAimAtTargetEnd = true;
		
		m_State.ApplyNewRequest(rq);
	}

	override protected float ResolveStoppedWaitTime(bool inCover, EAIThreatState threat, EWeaponType weaponType)
	{
		float waitTime;
		
		if (inCover)
		{
			// In cover
			switch (threat)
			{
				case EAIThreatState.THREATENED:
					waitTime = 10.0;	// Stay in cover for a long time, until we are not suppressed any more
					break;
				default:
					waitTime = 5.0;
			}
		}
		else
		{
			// Not in cover
			switch (threat)
			{
				case EAIThreatState.THREATENED:
					waitTime = 5.0;
					break;
				default:
					waitTime = 2.0;
					break;
			}
		}
		
		// When using those weapons we want to move much less
		bool longWaitTime = false;
		switch (weaponType)
		{
			case EWeaponType.WT_MACHINEGUN:
			case EWeaponType.WT_ROCKETLAUNCHER:
			case EWeaponType.WT_GRENADELAUNCHER:
			case EWeaponType.WT_SNIPERRIFLE:
				longWaitTime = true;
		}
		
		if (longWaitTime)
			waitTime *= 2.0;
		
		return waitTime;
	}
	
	//--------------------------------------------------------------------------------------------
	// Returns stance when stopped outside cover
	override protected static ECharacterStance ResolveStanceOutsideCover(bool closeRange, EAIThreatState threat)
	{
		if (closeRange)
		{
			switch (threat)
			{
				case EAIThreatState.THREATENED:
					return ECharacterStance.PRONE;
				case EAIThreatState.ALERTED:
					return ECharacterStance.CROUCH;
				case EAIThreatState.VIGILANT:
					return ECharacterStance.CROUCH;
				default:
					return ECharacterStance.STAND;
			}
			
			return ECharacterStance.CROUCH;
		}
		else
		{
			// Long range combat
			switch (threat)
			{
				case EAIThreatState.THREATENED:
					return ECharacterStance.PRONE;
				case EAIThreatState.ALERTED:
					return ECharacterStance.PRONE;
				default:
					return ECharacterStance.CROUCH;
			}
		}
		return ECharacterStance.STAND;
	}
	
	//--------------------------------------------------------------------------------------------
	// Returns 'optimal' distance
	// If we are between weaponMinDist and 'optimal' dist, we don't need to move closer to tgt
	override protected static float ResolveOptimalDistance(float weaponMinDist)
	{
		return Math.Max(weaponMinDist + 10.0, 10.0);
	}
	
	protected override void PushRequestFFAvoidance()
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		rq.m_eReason = SCR_EAICombatMoveReason.FF_AVOIDANCE;
		
		// If prev. request was FF avoidance too, keep direction.
		// Otherwise choose a new direction.
		SCR_AICombatMoveRequest_Move prevRequest = SCR_AICombatMoveRequest_Move.Cast(m_State.GetRequest());
		if (prevRequest && prevRequest.m_eReason == SCR_EAICombatMoveReason.FF_AVOIDANCE)
		{
			rq.m_eDirection = prevRequest.m_eDirection;
		}
		else
		{
			if (Math.RandomIntInclusive(0, 1) == 1)
				rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
			else
				rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
		}
		
		rq.m_eStanceMoving = m_CharacterController.GetStance(); // Don't change stance
		rq.m_eStanceEnd = rq.m_eStanceMoving;
		rq.m_vMovePos = ResolveRequestTargetPos();
		rq.m_eMovementType = EMovementType.WALK;
		rq.m_fMoveDistance = 2.0;
		rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
		rq.m_bAimAtTargetEnd = true;
		
		m_State.ApplyNewRequest(rq);
	}
}