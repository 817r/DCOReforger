modded enum SCR_EAICombatMoveReason
{
	INVESTIGATE,
	SUPPLYING
}

modded enum SCR_EAICombatMoveRequestType
{
	INVESTIGATE,
	RESUPPLYING,
	BUILDING
}

modded class SCR_AICombatMoveLogic_Attack : SCR_AICombatMoveLogicBase
{
	// Inputs
	protected static const string PORT_BASE_TARGET = "BaseTarget";
	protected static const string PORT_AVOID_STRAIGHT_PATH_DIR = "AvoidStraightPathDir";
	
	protected BaseTarget m_Target;
	protected vector m_vAvoidStraightPathDir;
	protected DCO_AIMoraleSystem moraleSystem;
	
	//--------------------------------------------------------------------------------------------
	protected override bool OnUpdate(AIAgent owner, float dt)
	{
		GetVariableIn(PORT_BASE_TARGET, m_Target);
		GetVariableIn(PORT_AVOID_STRAIGHT_PATH_DIR, m_vAvoidStraightPathDir);
		
		if (!m_Target || !m_Target.GetTargetEntity())
			return false;
		
		moraleSystem = m_Utility.GetMoraleSystem();
		return true;
	}
	
	//--------------------------------------------------------------------------------------------
	protected override float GetTargetDistance()
	{
		return m_Target.GetDistance();
	}
	
	//--------------------------------------------------------------------------------------------
	protected override vector GetTargetPosition()
	{
		return m_Target.GetLastSeenPosition();
	}
	
	protected override vector GetAvoidStraightPathDir()
	{
		return m_vAvoidStraightPathDir;
	}
	
	//--------------------------------------------------------------------------------------------
	protected override vector ResolveRequestTargetPos()
	{
		IEntity tgtEntity = m_Target.GetTargetEntity();
		if (tgtEntity && m_CombatComp.IsTargetVisible(m_Target))
		{
			ChimeraCharacter character = ChimeraCharacter.Cast(tgtEntity);
			if (character)
			{
				vector eyePos = character.EyePosition();
				return eyePos;
			}
			
			// It's a vehicle
			vector pos = tgtEntity.GetOrigin();
			pos = pos + Vector(0, 2.0, 0);
			return pos;
		}
		
		// Target is either not visible or tgtEntity is null, use last seen position
		vector lastSeenPos = m_Target.GetLastSeenPosition();
		lastSeenPos = lastSeenPos + Vector(0, 1.8, 0);
		return lastSeenPos;		
	}
	
	//--------------------------------------------------------------------------------------------
	protected override bool ResolveFailMoveIfNoCover()
	{
		if (m_bCloseRangeCombat)
		{
			if (m_State && m_State.IsMovingToBuilding())
				return true;
			else
				return false;
		}
		else
		{
			// Long range combat
			if (IsFirstExecution())
				return true; // On first run we want to move to cover, or stay where we are if there is no cover, and shoot.
			else
				return m_State.m_bInCover; // Don't leave cover if there is no next cover
		}
	}
	
	//--------------------------------------------------------------------------------------------
	protected override float ResolveStoppedWaitTime(bool inCover, EAIThreatState threat, EWeaponType weaponType)
	{
		if (m_bCloseRangeCombat)
		{
			return Math.RandomFloat(1.0, 4.5);
		}
		
		float waitTime;
		
		if (inCover)
		{
			// In cover
			switch (threat)
			{
				case EAIThreatState.THREATENED:
					waitTime = Math.RandomFloat(60, 90);	// Stay in cover for a long time, until we are not suppressed any more
					break;
				default:
					waitTime = Math.RandomFloat(30, 45);
			}
		}
		else
		{
			// Not in cover
			switch (threat)
			{
				case EAIThreatState.THREATENED:
					waitTime = Math.RandomFloat(2, 4);
					break;
				default:
					waitTime = Math.RandomFloat(6, 9);
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
		
		// Note: it's important to let bots enough time to aim at very long range
		// Stop time should be more than just a few seconds.
		if (m_bVeryLongRangeCombat)
			waitTime *= 1.5;
		else
			waitTime *= Math.RandomFloat(0.8, 1);
		
		if (longWaitTime)
			waitTime *= 2;
		
		float mult = Math.Map(moraleSystem.GetMoraleMeasure(), 0, 4.5, 1, 2.5);
		waitTime += mult;
		
		return waitTime;
	}
	
	//--------------------------------------------------------------------------------------------
	protected override bool MoveToNextPosCondition()
	{
		// Don't get any more closer
		// Except we should still move closer if we haven't seen target for a long time
		float optimalDist = ResolveOptimalDistance(m_fWeaponMinDist);
		if (m_fTargetDist < optimalDist && m_Target.GetTimeSinceSeen() < 10)
			return false;
			
		if (m_State.IsExecutingRequest())
			return false;
		
		// If it's first run, ignore timers, only if:
		// - If we are not in cover.
		if (IsFirstExecution() && !m_State.m_bInCover)
			return true;
		
		// If we should keep formation, don't move too far away
		if (m_Utility.ShouldKeepFormation() || m_CombatComp.GetCombatMode() == EAIGroupCombatMode.HOLD_FIRE)
		{
			// In this case we move only once
			if (!IsFirstExecution())
				return false;
		}
		
		float stoppedWaitTime = ResolveStoppedWaitTime(m_State.m_bInCover, m_eThreatState, m_eWeaponType);	
		return m_State.m_fTimerStopped_s > stoppedWaitTime;
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
			// No waypoint, or it's an entity-associated waypoint, like Follow waypoint.
			// Therefore use standard movement logic.
			// Otherwise they will want to run towards position where the waypoint is placed, which makes no sense.
			
			/*if (agent != group.GetLeaderAgent() && vector.Distance(group.GetLeaderEntity().GetOrigin(), m_MyEntity.GetOrigin()) > 40)
			{
				vector mp = vector.Zero;
				FindPosition2D(mp, group.GetLeaderEntity().GetOrigin(), 25, vector.Zero, 5);
				movePos = mp;
				eDirection = SCR_EAICombatMoveDirection.FORWARD;
				coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
				avoidStraightPathDir = vector.Zero;			
			}*/
			
			movePos = targetPos;
			MoraleAndThreatPushMove(eDirection);
			//eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS; // Move to target
			avoidStraightPathDir = GetAvoidStraightPathDir(); // Use flanking
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
				eDirection = SCR_EAICombatMoveDirection.FORWARD;
				avoidStraightPathDir = GetAvoidStraightPathDir();
				
				coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
			}
		}
		
		outMovePos = movePos;
		outDirection = eDirection;
		outAvoidStraightPathDir = avoidStraightPathDir;
		outCoverSearchSectorHalfAngleRad = coverSearchSectorHalfAngleRad;
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
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 10.0;
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN;
					rq.m_eMovementType = EMovementType.WALK;
					break;
				}
				default:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 15.0;
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_RUN;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
			}
			
			//rq.m_eMovementType = EMovementType.WALK;
			rq.m_bAimAtTarget = DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection) &&
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
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 20.0;
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;
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
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
				default:
				{
					coverSearchDistMin = 10.0;
					coverSearchDistMax = 30.0;
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_RUN; // Shouldn't be so large because we are sprinting and can't shoot
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
			}
			
			//rq.m_eMovementType = EMovementType.RUN;
			rq.m_bAimAtTarget = DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection) &&
								IsAimingAndMovingAllowedForWeapon(m_eWeaponType);
			rq.m_bAimAtTargetEnd = true;
		}
		
		if (m_State.GetOldRequest() && m_State.GetOldRequest().m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
			rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		else
			rq.m_eType = SCR_EAICombatMoveRequestType.BUILDING;
		
		rq.m_bFailIfNoCover = ResolveFailMoveIfNoCover();
		
		// If we are not in cover, min cover search distance is overridden to 0, we should find any cover ASAP
		if (!m_State.m_bInCover)
			coverSearchDistMin = 3;
		
		rq.m_fCoverSearchDistMin = coverSearchDistMin;
		rq.m_fCoverSearchDistMax = coverSearchDistMax;
		rq.m_fMoveDuration_s = moveDurationMax * MoraleAmplifyMove();
		
		if (m_State && m_State.IsMovingToBuilding())
		{
			coverSearchDistMin = 0;
			coverSearchDistMax = 5;
		}
		
		// Subscribe to events
		// We will pronounce voice lines once we start or end moving
		rq.GetOnMovementStarted().Insert(OnMovementStarted);
		rq.GetOnCompleted().Insert(OnMovementCompleted);
		
		m_State.ApplyNewRequest(rq);
	}
	
	void GetMoraleEffects(out SCR_EAICombatMoveDirection moveDir)
	{
		switch(moraleSystem.GetState())
		{
			case moraleState.BREAK:
			{
				moveDir = SCR_EAICombatMoveDirection.BACKWARD;
				break;
			}
			case moraleState.MANIAC:
			{
				if(Math.RandomFloat(0,5) > 4)
				{
					if(Math.RandomFloat(0,5) > 4)
					{
						if(Math.RandomFloat(0,1) > 1)
							moveDir = SCR_EAICombatMoveDirection.LEFT;
						else
							moveDir = SCR_EAICombatMoveDirection.RIGHT;
					}
					else
						moveDir = SCR_EAICombatMoveDirection.BACKWARD;
				}
				else
					moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
				break;
			}
			case moraleState.ANXIOUS:
			{
				if(Math.RandomFloat(0,1) > 1)
				{
					if(Math.RandomFloat(0,1) > 1)
						moveDir = SCR_EAICombatMoveDirection.LEFT;
					else
						moveDir = SCR_EAICombatMoveDirection.RIGHT;
				}
				else
					moveDir = SCR_EAICombatMoveDirection.BACKWARD;
				break;
			}
			default:
			{
				moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
				break;
			}
		}
		
	}
	
	void MoraleAndThreatPushMove(out SCR_EAICombatMoveDirection moveDir)
	{
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
						if (Math.RandomIntInclusive(0,1) == 1)
							moveDir = SCR_EAICombatMoveDirection.LEFT;
						else
							moveDir = SCR_EAICombatMoveDirection.RIGHT;
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
						moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
						break;					
					}
					case moraleState.NORMAL:
					{
						moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
						break;					
					}
					case moraleState.MOTIVATED:
					{
						moveDir = SCR_EAICombatMoveDirection.FORWARD;
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
						moveDir = SCR_EAICombatMoveDirection.FORWARD;
						break;					
					}
					case moraleState.ANXIOUS:
					{
						moveDir = SCR_EAICombatMoveDirection.FORWARD;
						break;					
					}
					case moraleState.NORMAL:
					{
						moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
						break;					
					}
					case moraleState.MOTIVATED:
					{
						moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
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
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;									
					}
					case moraleState.MANIAC:
					{
						moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
						break;					
					}
					case moraleState.ANXIOUS:
					{
						moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
						break;					
					}
					case moraleState.NORMAL:
					{
						moveDir = SCR_EAICombatMoveDirection.FORWARD;
						break;					
					}
					case moraleState.MOTIVATED:
					{
						moveDir = SCR_EAICombatMoveDirection.FORWARD;
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
				waitTime_s = 1.5;
			else
				waitTime_s = 5.0;
		}
		
		if (m_State.m_bExposedInCover && m_State.m_fTimerRequest_s > waitTime_s)
		{
			float newWaitTime = Math.RandomFloat(5, 9.0); // Hide in cover for this time
			PushRequestChangeStanceInCover(false, SCR_EAICombatMoveReason.SUPPRESSED_IN_COVER, newWaitTime);
		}
		else if (!m_State.m_bExposedInCover && m_State.m_fTimerRequest_s > waitTime_s)
		{
			float newWaitTime = Math.RandomFloat(1.5, 2.5); // Expose out of cover for this time
			PushRequestChangeStanceInCover(true, SCR_EAICombatMoveReason.SUPPRESSED_IN_COVER, newWaitTime);
		}
	}
	
	protected override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		float currentTime_ms = GetGame().GetWorld().GetWorldTime();
		if (currentTime_ms < m_fNextUpdate_ms)
			return ENodeResult.RUNNING;
		m_fNextUpdate_ms = currentTime_ms + m_fUpdateInterval_ms;
		
		if (!OnUpdate(owner, dt))
			return ENodeResult.FAIL;
		
		if (!m_State || !m_MyEntity || !m_Utility || !m_CombatComp || !m_CharacterController)
			return ENodeResult.FAIL;
		
		// Don't run combat movement logic if CombatMove BT is not used now (like in turret)
		SCR_AIBehaviorBase executedBehavior = SCR_AIBehaviorBase.Cast(m_Utility.GetExecutedAction());
		if (executedBehavior && !executedBehavior.m_bUseCombatMove)
			return ENodeResult.RUNNING;
		
		// Update cached variables
		m_fTargetDist = GetTargetDistance();
		m_bCloseRangeCombat = m_fTargetDist < SCR_AICombatMoveUtils.CLOSE_RANGE_COMBAT_DIST;
		m_bVeryLongRangeCombat = m_fTargetDist > SCR_AICombatMoveUtils.VERY_LONG_RANGE_COMBAT_DIST;
		m_eThreatState = m_Utility.m_ThreatSystem.GetState();
		m_eStance = m_CharacterController.GetStance();
		m_fWeaponMinDist = m_CombatComp.GetSelectedWeaponMinDist();
		m_eWeaponType = m_CombatComp.GetSelectedWeaponType();
		
		
		/*		
		//------------------------------------------------------------------------------------
		Combat movement logic
		
		Conditions represent states inside which we want to remain.
		
		Conditions are organized based on their priority, highest first.
		
		Within each state there can be extra logic which decides if it's worth to
		send a new request, because even though we have selected a state, we should avoid
		spamming same request over and over.
		
		Conditions for states mostly depend on Combat Move State and its timers.
		
		It is important to write logic in such a way that it doesn't depend on state
		of this node. In this case the state flow also doesn't depend on it, and AI
		does movement is more fluent when switching to a new behavior which also utilizes
		combat movement, including attacking a different target.
		*/

		if (SuppressedInCoverCondition())
		{
			SuppressedInCoverLogic();
		}
		else if (MoveFromTargetCondition())
		{
			// Too close to target
			// Step away
			if (MoveFromTargetNewRequestCondition())
				PushRequestMoveFromTarget();
		}
		else if (CurrentCoverUselessCondition())
		{
			// Current cover has been compromised, it's not directed at enemy any more
			// Find a new cover nearby
			PushRequestLeaveUselessCover();
		}
		else if (m_CharacterController.IsReloading())
		{
			// We're reloading and can't do much else now
			// Hide in cover
			if (m_State.m_bInCover)
			{
				if (m_State.m_bExposedInCover)
					m_State.ApplyRequestChangeStanceInCover(false);			
			} else
			{
				if (m_CharacterController.GetStance() ==  ECharacterStance.STAND)
					m_CharacterController.SetStanceChange(2);
			}

		}
		else if (m_State.m_bInCover && !m_State.m_bExposedInCover)
		{
			// We're in cover but we are still hiding in it, unhide
			m_State.ApplyRequestChangeStanceInCover(true);
		}
		else if (FFAvoidanceCondition())
		{
			if (FFAvoidanceNewRequestCondition())
				PushRequestFFAvoidance();
		}
		else if (MoveToNextPosCondition())
		{
			// We've waited here too long, move to next place
			PushRequestMove();
		}
		else if (!m_State.IsExecutingRequest() && !m_State.m_bInCover)
		{
			if (IsInOpenArea(owner.GetControlledEntity()) && Math.RandomFloat01() > 0.4)
			{
				if (!m_State.IsExecutingRequest())
					PushRequestOpenArea();
			} else
			{
				ECharacterStance newStance = ResolveStanceOutsideCover(m_bCloseRangeCombat, m_eThreatState);
				if (newStance > m_eStance)
				{
					
					// Only let stance go down, no need to get back up
					m_State.ApplyRequestChangeStanceOutsideCover(newStance);
				}
			}
			// We are stopped and not in cover, manage our stance
			

		} else if (!m_State.IsExecutingRequest())
		{
			if (m_Utility.GetCharacterController().GetWeaponObstructedState() != EWeaponObstructedState.UNOBSTRUCTED)
			{
				if (m_CharacterController.GetStance() == ECharacterStance.CROUCH)
					m_CharacterController.SetStanceChange(1);
				else if (m_CharacterController.GetStance() == ECharacterStance.PRONE)
					m_CharacterController.SetStanceChange(2);
			}
		}
		
		return ENodeResult.RUNNING;
	}
	
	override protected bool SuppressedInCoverCondition()
	{
		return m_State.m_bInCover && m_eThreatState == EAIThreatState.THREATENED && moraleSystem.GetState() >= moraleState.MANIAC;
	}
	
	float MoraleAmplifyMove()
	{
		return Math.Map(moraleSystem.GetMoraleMeasure(), 0, 4.5, 2, 1);
	}
	
	protected void PushRequestOpenArea()
	{		
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
		
		// Common values
		rq.m_vTargetPos = ResolveRequestTargetPos();
		rq.m_vMovePos = rq.m_vTargetPos;
		rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
		rq.m_fCoverSearchSectorHalfAngleRad;
		rq.m_bTryFindCover = true;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility = true;

		float coverSearchDistMin = 5;
		float coverSearchDistMax = 50;
		float moveDurationMax = 10;
		if (m_bCloseRangeCombat)
		{
			switch (m_eThreatState)
			{
				case EAIThreatState.THREATENED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN;
					rq.m_eMovementType = EMovementType.WALK;
					break;
				}
				default:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_RUN;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
			}
			
			//rq.m_eMovementType = EMovementType.WALK;
			rq.m_bAimAtTarget = DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection) &&
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
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.PRONE;
					rq.m_eMovementType = EMovementType.SPRINT;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
				default:
				{
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_RUN; // Shouldn't be so large because we are sprinting and can't shoot
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
			}
			
			//rq.m_eMovementType = EMovementType.RUN;
			rq.m_bAimAtTarget = DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection) &&
								IsAimingAndMovingAllowedForWeapon(m_eWeaponType);
			rq.m_bAimAtTargetEnd = true;
		}
		
		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		
		rq.m_bFailIfNoCover = false;
		rq.m_fCoverSearchDistMin = coverSearchDistMin;
		rq.m_fCoverSearchDistMax = coverSearchDistMax;
		rq.m_fMoveDuration_s = moveDurationMax * MoraleAmplifyMove();


		rq.GetOnMovementStarted().Insert(OnMovementStarted);
		rq.GetOnCompleted().Insert(OnMovementCompleted);
		
		m_State.ApplyNewRequest(rq);
	}
	
	override protected void PushRequestFFAvoidance()
	{
		if (m_CharacterController.GetStance() == ECharacterStance.PRONE)
		{
			if (Math.RandomFloat01() > 0.5)
				m_CharacterController.SetRoll(1);
			else
				m_CharacterController.SetRoll(2);
			
			return;
		}
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
		rq.m_fMoveDuration_s = 1.0;
		rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
		rq.m_bAimAtTargetEnd = true;
		
		m_State.ApplyNewRequest(rq);
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
		rq.m_fCoverSearchDistMin = 0;
		rq.m_fCoverSearchDistMax = 30;
		if (m_CharacterController.GetStance() == ECharacterStance.PRONE)
			rq.m_eStanceMoving = ECharacterStance.CROUCH;
		else
			rq.m_eStanceMoving = m_CharacterController.GetStance();
		rq.m_eStanceEnd = ECharacterStance.CROUCH;
		rq.m_eMovementType = EMovementType.RUN;
		rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD; // Move back from target
		rq.m_bAimAtTarget = DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection) &&
								IsAimingAndMovingAllowedForWeapon(m_eWeaponType);
		rq.m_bAimAtTargetEnd = true;
		if (m_CharacterController.GetStance() == ECharacterStance.STAND)
			rq.m_fMoveDuration_s = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_RUN;
		else
			rq.m_fMoveDuration_s = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN;

		
		m_State.ApplyNewRequest(rq);
	}
	
	//--------------------------------------------------------------------------------------------
	protected static ref TStringArray s_aVarsIn = {
		PORT_BASE_TARGET,
		PORT_AVOID_STRAIGHT_PATH_DIR
	};
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
}