modded class SCR_AICombatMoveLogic_Suppressive
{
	
	protected DCO_AIMoraleSystem moraleSystem;
	
	protected override bool OnUpdate(AIAgent owner, float dt)
	{
		GetVariableIn(PORT_SUPPRESSION_VOLUME, m_SuppressionVolume);
		
		if (!GetVariableIn(PORT_VISIBLE, m_bTargetVisible))
			return false;
		
		if (!GetVariableIn(PORT_TIME_LAST_SEEN, m_fTargetLastSeenTime_ms))
			return false;
		
		if (!m_SuppressionVolume)
			return false;
		
		// Update m_bGoodVision
		// The timer criteria is to exclude occlusion due to us hiding in cover
		float timeSinceLastSeen_ms = GetGame().GetWorld().GetWorldTime() - m_fTargetLastSeenTime_ms;
		m_bGoodVision = m_bTargetVisible || (timeSinceLastSeen_ms < TIME_SINCE_GOOD_VISIBILITY_MIN_MS);
		moraleSystem = m_Utility.GetMoraleSystem();
		return true;
	}
	
	//--------------------------------------------------------------------------------------------
	protected override bool ResolveFailMoveIfNoCover()
	{
		// === FIXED: request tipe BUILDING mematikan m_bTryFindCover. Mewajibkan cover
		// di situ = request yang dijamin gagal -- dua flag saling meniadakan. Ini
		// dicek DULUAN supaya menang atas dua kondisi di bawahnya.
		if (m_State && m_State.IsMovingToBuilding())
			return false;
		// === END FIXED ===
		
		// Don't move out of cover if we already have good vision from current cover
		if (m_bGoodVision)
			return true;
		
		if (m_Utility.m_ThreatSystem.GetState() >= EAIThreatState.ALERTED)
			return true;
		
		return false; // We're allowed to move anywhere, including to coverless position. But our own suppression criteria still apply and run above this.
	}
	
	//--------------------------------------------------------------------------------------------
	protected override float ResolveStoppedWaitTime(bool inCover, EAIThreatState threat, EWeaponType weaponType)
	{
		float waitTime = 3;
		if (m_State && m_State.IsMovingToBuilding())
			waitTime *= 1.2;
		
		bool longWaitTime = false;
		switch (weaponType)
		{
			case EWeaponType.WT_MACHINEGUN:
				longWaitTime = true;
		}
		if (longWaitTime)
			waitTime *= 4;

		if (moraleSystem)
		{
			float mult = Math.Map(moraleSystem.GetMoraleMeasure(), 0, 4.5, 1, 2.5);
			waitTime += mult;
		}

		waitTime *= DCO_PersonalityCombatUtility.GetStoppedWaitTimeScale(m_Utility);
		
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
		
		bool isExposed = DCO_ConcealmentUtility.IsPositionExposed(m_MyEntity.GetOrigin(), m_MyEntity);
		if (isExposed)
			rq.m_bUseCoverSearchDirectivity = false;
		
		rq.m_bCheckCoverVisibility = true;

		float coverSearchDistMin = 5;
		float coverSearchDistMax = 30;
		float moveDurationMax = 10;
		if (m_bCloseRangeCombat)
		{
			// Close range combat
			
			switch (m_eThreatState)
			{
				case EAIThreatState.THREATENED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.PRONE;
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 10.0;
					moveDurationMax = coverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 10.0;
					moveDurationMax = coverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
				default:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 15.0;
					moveDurationMax = coverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_RUN;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
			}
			
			rq.m_bAimAtTarget = DCO_MoraleCombatUtility.CanAimWhileMoving(
				DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection) &&
				IsAimingAndMovingAllowedForWeapon(m_eWeaponType),
				moraleSystem);
			rq.m_bAimAtTargetEnd = true;
		}
		else
		{
			
			switch (m_eThreatState)
			{
				case EAIThreatState.THREATENED:
				{
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 20.0;
					moveDurationMax = 5;
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.PRONE;
					rq.m_eMovementType = EMovementType.SPRINT;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 10.0;
					moveDurationMax = 4;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
				default:
				{
					coverSearchDistMin = 10.0;
					coverSearchDistMax = 30.0;
					moveDurationMax = 7; // Shouldn't be so large because we are sprinting and can't shoot
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
			}
			
			rq.m_bAimAtTarget = DCO_MoraleCombatUtility.CanAimWhileMoving(
				DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection) &&
				IsAimingAndMovingAllowedForWeapon(m_eWeaponType),
				moraleSystem);
			rq.m_bAimAtTargetEnd = true;
		}			
		
		if (m_State.GetOldRequest() && m_State.GetOldRequest().m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
			rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		else
		{	
			rq.m_eType = SCR_EAICombatMoveRequestType.BUILDING;
			rq.m_bTryFindCover = false;
		}
		
		rq.m_bFailIfNoCover = ResolveFailMoveIfNoCover();
		
		if (!m_State.m_bInCover)
			coverSearchDistMin = 3;
		
		if (m_State && m_State.IsMovingToBuilding())
		{
			coverSearchDistMin = 0;
			coverSearchDistMax = 10;
		}
		
		rq.m_fCoverSearchDistMin = coverSearchDistMin;
		coverSearchDistMax *= DCO_MoraleCombatUtility.GetCoverSearchDistScale(moraleSystem, m_Utility);
		
		if (isExposed)
			coverSearchDistMax *= 2;
		rq.m_fCoverSearchDistMax = coverSearchDistMax;
		rq.m_fMoveDuration_s = moveDurationMax * MoraleAmplifyMove();

		rq.GetOnMovementStarted().Insert(OnMovementStarted);
		rq.GetOnCompleted().Insert(OnMovementCompleted);
		
		m_State.ApplyNewRequest(rq);
	}
	
	float MoraleAmplifyMove()
	{
		if (!moraleSystem)
			return 1.0;
		
		return Math.Map(moraleSystem.GetMoraleMeasure(), 0, 4.5, 2, 1);
	}
	
	void MoraleAndThreatPushMove(out SCR_EAICombatMoveDirection moveDir)
	{
		moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
		
		switch (m_eThreatState)
		{
			case EAIThreatState.THREATENED:
			{
				switch (moraleSystem.GetState())
				{
					case moraleState.BREAK:
					{
						moveDir = SCR_EAICombatMoveDirection.BACKWARD;
						break;					
					}
					case moraleState.MANIAC:
					{
						if (Math.RandomIntInclusive(0,1) == 1)
							moveDir = SCR_EAICombatMoveDirection.LEFT;
						else
							moveDir = SCR_EAICombatMoveDirection.RIGHT;
						break;					
					}
					case moraleState.ANXIOUS:
					{
						if (Math.RandomIntInclusive(0,1) == 1)
							moveDir = SCR_EAICombatMoveDirection.LEFT;
						else
							moveDir = SCR_EAICombatMoveDirection.RIGHT;
						break;					
					}
					case moraleState.NORMAL:
					{
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;					
					}
					case moraleState.MOTIVATED:
					{
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;					
					}
					default:
					{
						moveDir = SCR_EAICombatMoveDirection.BACKWARD;
						break;
					}
				}
				break;
			}
			case EAIThreatState.ALERTED:
			{
				switch (moraleSystem.GetState())
				{
					case moraleState.BREAK:
					{
						moveDir = SCR_EAICombatMoveDirection.BACKWARD;
						break;					
					}
					case moraleState.MANIAC:
					{
						if (Math.RandomIntInclusive(0,1) == 1)
							moveDir = SCR_EAICombatMoveDirection.LEFT;
						else
							moveDir = SCR_EAICombatMoveDirection.RIGHT;
						break;					
					}
					case moraleState.ANXIOUS:
					{
						if (Math.RandomIntInclusive(0,1) == 1)
							moveDir = SCR_EAICombatMoveDirection.LEFT;
						else
							moveDir = SCR_EAICombatMoveDirection.RIGHT;
						break;						
					}
					case moraleState.NORMAL:
					{
						if (Math.RandomIntInclusive(0,1) == 1)
							moveDir = SCR_EAICombatMoveDirection.LEFT;
						else
							moveDir = SCR_EAICombatMoveDirection.RIGHT;
						break;								
					}
					case moraleState.MOTIVATED:
					{
						if (Math.RandomIntInclusive(0,1) == 1)
							moveDir = SCR_EAICombatMoveDirection.LEFT;
						else
							moveDir = SCR_EAICombatMoveDirection.RIGHT;
						break;							
					}
					default:
					{
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;
					}
				}
				break;
			}
			case EAIThreatState.VIGILANT:
			{
				switch (moraleSystem.GetState())
				{
					case moraleState.BREAK:
					{
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;					
					}
					case moraleState.MANIAC:
					{
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;					
					}
					case moraleState.ANXIOUS:
					{
						if (Math.RandomIntInclusive(0,1) == 1)
							moveDir = SCR_EAICombatMoveDirection.LEFT;
						else
							moveDir = SCR_EAICombatMoveDirection.RIGHT;
						break;							
					}
					case moraleState.NORMAL:
					{
						if (Math.RandomIntInclusive(0,1) == 1)
							moveDir = SCR_EAICombatMoveDirection.LEFT;
						else
							moveDir = SCR_EAICombatMoveDirection.RIGHT;
						break;								
					}
					case moraleState.MOTIVATED:
					{
						if (Math.RandomIntInclusive(0,1) == 1)
							moveDir = SCR_EAICombatMoveDirection.LEFT;
						else
							moveDir = SCR_EAICombatMoveDirection.RIGHT;
						break;							
					}
					default:
					{
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;
					}
				}
				break;
			}
			case EAIThreatState.SAFE:
			{
				switch (moraleSystem.GetState())
				{
					case moraleState.BREAK:
					{
						if (Math.RandomIntInclusive(0,1) == 1)
							moveDir = SCR_EAICombatMoveDirection.LEFT;
						else
							moveDir = SCR_EAICombatMoveDirection.RIGHT;
						break;										
					}
					case moraleState.MANIAC:
					{
						if (Math.RandomIntInclusive(0,1) == 1)
							moveDir = SCR_EAICombatMoveDirection.LEFT;
						else
							moveDir = SCR_EAICombatMoveDirection.RIGHT;
						break;			
					}
					case moraleState.ANXIOUS:
					{
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;					
					}
					case moraleState.NORMAL:
					{
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;					
					}
					case moraleState.MOTIVATED:
					{
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;					
					}
					default:
					{
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;
					}
				}
				break;
			}
			default:
			{
				break;
			}
		}
	}
	
	override protected void ResolveMoveRequestMovePosAndDir(vector targetPos, out vector outMovePos, out vector outAvoidStraightPathDir, out SCR_EAICombatMoveDirection outDirection, out float outCoverSearchSectorHalfAngleRad)
	{	
		AIWaypoint wp = null;
		AIAgent agent = m_Utility.GetAIAgent();
		AIGroup group = agent.GetParentGroup();
		if (group)
			wp = group.GetCurrentWaypoint();
		
		vector movePos;
		SCR_EAICombatMoveDirection eDirection;
		float coverSearchSectorHalfAngleRad;
		vector avoidStraightPathDir;
		
		if (!wp || SCR_EntityWaypoint.Cast(wp))
		{
			if (agent != group.GetLeaderAgent() && vector.Distance(group.GetLeaderEntity().GetOrigin(), m_MyEntity.GetOrigin()) > 50)
			{
				movePos = group.GetLeaderEntity().GetOrigin();
				eDirection = SCR_EAICombatMoveDirection.FORWARD;
				coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
				avoidStraightPathDir = vector.Zero;
			}
			else
			{
				movePos = targetPos;
				MoraleAndThreatPushMove(eDirection);
				avoidStraightPathDir = GetAvoidStraightPathDir();
				coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
			}
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
				eDirection = SCR_EAICombatMoveDirection.FORWARD;
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
	//--------------------------------------------------------------------------------------------
	protected override bool MoveToNextPosCondition()
	{	
		if (m_Utility && m_Utility.m_DCOConfig && m_Utility.m_DCOConfig.IsHoldPosition())
			return false;
		
		if (m_State.IsExecutingRequest())
			return false;
		
		if (m_bGoodVision && m_State.m_bInCover)
		{
			// We have good vision and we are in cover, just stay here
			return false;
		}
		
		float stoppedWaitTime = ResolveStoppedWaitTime(m_State.m_bInCover, m_eThreatState, m_eWeaponType);	

		if (IsFirstExecution() && !m_State.m_bInCover)
			return true;

		if (m_Utility.GetCharacterController().GetWeaponObstructedState() != EWeaponObstructedState.UNOBSTRUCTED)
		{
			if (m_CharacterController.GetStance() == ECharacterStance.CROUCH)
				m_CharacterController.SetStanceChange(1);
			else if (m_CharacterController.GetStance() == ECharacterStance.PRONE)
				m_CharacterController.SetStanceChange(2);
			else
				return m_State.m_fTimerStopped_s > 2;
		}
		
		return m_State.m_fTimerStopped_s > stoppedWaitTime;
	}
	
	//--------------------------------------------------------------------------------------------
	protected static ref TStringArray s_aVarsIn = {
		PORT_SUPPRESSION_VOLUME,
		PORT_VISIBLE,
		PORT_TIME_LAST_SEEN
	};
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
}