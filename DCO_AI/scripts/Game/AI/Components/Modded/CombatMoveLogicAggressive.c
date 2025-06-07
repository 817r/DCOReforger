class SCR_AICombatMoveLogic_Aggressive : SCR_AICombatMoveLogicBase
{
	// Inputs
	protected static const string PORT_BASE_TARGET = "BaseTarget";
	protected static const string PORT_AVOID_STRAIGHT_PATH_DIR = "AvoidStraightPathDir";
	
	protected BaseTarget m_Target;
	protected MoraleState m_MoraleState;
	protected vector m_vAvoidStraightPathDir;
	
	protected float minRandomTime = 5;
	protected float maxRandomTime = 12;
	
	//--------------------------------------------------------------------------------------------
	protected override bool OnUpdate(AIAgent owner, float dt)
	{
		GetVariableIn(PORT_BASE_TARGET, m_Target);
		GetVariableIn(PORT_AVOID_STRAIGHT_PATH_DIR, m_vAvoidStraightPathDir);
		
		if (!m_Target || !m_Target.GetTargetEntity())
			return false;
		
		return true;
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
		m_MoraleState = m_CombatComp.GetMoraleComponent().GetMoraleStates();
		
		
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
		else if (m_State.m_bInCover && m_CharacterController.IsReloading())
		{
			// We're reloading and can't do much else now
			// Hide in cover
			if (m_State.m_bExposedInCover)
				m_State.ApplyRequestChangeStanceInCover(false);
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
			// We are stopped and not in cover, manage our stance
			ECharacterStance newStance = ResolveStanceOutsideCover(m_bCloseRangeCombat, m_eThreatState, m_MoraleState);
			if (newStance > m_eStance)
			{
				// Only let stance go down, no need to get back up
				m_State.ApplyRequestChangeStanceOutsideCover(newStance);
			}
		}
		
		return ENodeResult.RUNNING;
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
		if (m_CombatComp.IsTargetVisible(m_Target))
		{
			IEntity tgtEntity = m_Target.GetTargetEntity(); // We've checked already
		
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
		
		// Target is not visible, use last seen position
		vector lastSeenPos = m_Target.GetLastSeenPosition();
		lastSeenPos = lastSeenPos + Vector(0, 1.8, 0);
		return lastSeenPos;		
	}
	
	//--------------------------------------------------------------------------------------------
	protected override bool ResolveFailMoveIfNoCover()
	{
		if (m_bCloseRangeCombat)
		{
			if (Math.RandomInt(0,101) < m_CombatComp.GetCoverChances())
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
			{
				if (Math.RandomInt(0,101) < m_CombatComp.GetCoverChances())
					return m_State.m_bInCover;
				else
					return false;			
			}
		}
	}
	
	//--------------------------------------------------------------------------------------------
	protected override float ResolveStoppedWaitTime(bool inCover, EAIThreatState threat, EWeaponType weaponType)
	{
		float waitTime;
		
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
					waitTime = Math.RandomFloat(minRandomTime, maxRandomTime / 2);
					break;
			}
		}
		
		// When using those weapons we want to move much less
		bool longWaitTime = false;
		switch (weaponType)
		{
			case EWeaponType.WT_SNIPERRIFLE:
				longWaitTime = true; 
		}
		
		if (longWaitTime) waitTime += Math.RandomFloat(minRandomTime, maxRandomTime / 2);
		
		switch (m_MoraleState)
		{
			case MoraleState.FRESH:
			{
				waitTime *= 2.2;
				break;
			}
			case MoraleState.NORMAL:
			{
				waitTime *= 1.8;
				break;
			}
			case MoraleState.STRESSED:
			{
				waitTime *= 1.2;
				break;
			}
			case MoraleState.PRESSURED:
			{
				waitTime *= 0.7;
				break;
			}
			case MoraleState.BREAK:
			{
				waitTime *= 0.3;
				break;
			}
		}
		
		waitTime = Math.Clamp(waitTime, 5, 45);
		
		return waitTime;
	}
	
	//--------------------------------------------------------------------------------------------
	protected override bool MoveToNextPosCondition()
	{
		// Don't get any more closer
		// Except we should still move closer if we haven't seen target for a long time
		float optimalDist = ResolveOptimalDistance(m_fWeaponMinDist);
		if (m_fTargetDist < optimalDist)
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
		
		if (m_Target.GetTimeSinceSeen() > 5)
			return true;
		
		float stoppedWaitTime = ResolveStoppedWaitTime(m_State.m_bInCover, m_eThreatState, m_eWeaponType);	
		return m_State.m_fTimerStopped_s > stoppedWaitTime;
	}
	
	protected override void PushRequestMove()
	{		
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
		
		// Common values
		rq.m_vTargetPos = ResolveRequestTargetPos();
		ResolveMoveRequestMovePosAndDir(rq.m_vTargetPos, rq.m_vMovePos, rq.m_vAvoidStraightPathDir, rq.m_eDirection, rq.m_fCoverSearchSectorHalfAngleRad);
		rq.m_bTryFindCover = true;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility = true;

		float coverSearchDistMin = 2;
		float coverSearchDistMax = 15;
		float moveDurationMax = 6;
		if (m_bCloseRangeCombat)
		{
			// Close range combat
			
			switch (m_eThreatState)
			{
				case EAIThreatState.THREATENED:
				{
					coverSearchDistMin = 2.0;
					coverSearchDistMax = 10.0;
					moveDurationMax = 1;
					
					switch(m_MoraleState)
					{
						case MoraleState.FRESH:
						{
							rq.m_eStanceMoving = ECharacterStance.STAND;
							rq.m_eStanceEnd = ECharacterStance.STAND;
							coverSearchDistMax *= 2;
							moveDurationMax *= 3;
							break;
						}
						case MoraleState.NORMAL:
						{
							rq.m_eStanceMoving = ECharacterStance.STAND;
							rq.m_eStanceEnd = ECharacterStance.CROUCH;
							coverSearchDistMax *= 1.7;
							moveDurationMax *= 2.5;
							break;					
						}
						case MoraleState.STRESSED:
						{
							if (Math.RandomInt(0,2) == 1)
							{
								rq.m_eStanceMoving = ECharacterStance.CROUCH;
								rq.m_eStanceEnd = ECharacterStance.CROUCH;
							}
							else
							{
								rq.m_eStanceMoving = ECharacterStance.STAND;
								rq.m_eStanceEnd = ECharacterStance.CROUCH;
							}
							coverSearchDistMax *= 1.5;
							moveDurationMax *= 2;
							break;					
						}
						case MoraleState.PRESSURED:
						{
							rq.m_eStanceMoving = ECharacterStance.CROUCH;
							rq.m_eStanceEnd = ECharacterStance.CROUCH;
							coverSearchDistMax *= 1.2;
							moveDurationMax *= 1.5;
							break;			
						}
						case MoraleState.BREAK:
						{
							rq.m_eStanceMoving = ECharacterStance.CROUCH;
							rq.m_eStanceEnd = ECharacterStance.CROUCH;
							break;					
						}
					}
					break;
				}
				default:
				{
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 15.0;
					moveDurationMax = 2;
					
					switch(m_MoraleState)
					{
						case MoraleState.FRESH:
						{
							rq.m_eStanceMoving = ECharacterStance.STAND;
							rq.m_eStanceEnd = ECharacterStance.STAND;
							coverSearchDistMax *= 2;
							moveDurationMax *= 3;
							break;
						}
						case MoraleState.NORMAL:
						{
							rq.m_eStanceMoving = ECharacterStance.STAND;
							rq.m_eStanceEnd = ECharacterStance.CROUCH;
							coverSearchDistMax *= 1.7;
							moveDurationMax *= 2.5;
							break;					
						}
						case MoraleState.STRESSED:
						{
							if (Math.RandomInt(0,2) == 1)
							{
								rq.m_eStanceMoving = ECharacterStance.CROUCH;
								rq.m_eStanceEnd = ECharacterStance.CROUCH;
							}
							else
							{
								rq.m_eStanceMoving = ECharacterStance.STAND;
								rq.m_eStanceEnd = ECharacterStance.CROUCH;
							}
							coverSearchDistMax *= 1.5;
							moveDurationMax *= 2;
							break;					
						}
						case MoraleState.PRESSURED:
						{
							rq.m_eStanceMoving = ECharacterStance.CROUCH;
							rq.m_eStanceEnd = ECharacterStance.CROUCH;
							coverSearchDistMax *= 1.2;
							moveDurationMax *= 1.5;
							break;			
						}
						case MoraleState.BREAK:
						{
							rq.m_eStanceMoving = ECharacterStance.CROUCH;
							rq.m_eStanceEnd = ECharacterStance.CROUCH;
							break;					
						}
					}
					break;
				}
			}
			
			rq.m_eMovementType = EMovementType.RUN;
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
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 20.0;
					moveDurationMax = 3;
					
					switch(m_MoraleState)
					{
						case MoraleState.FRESH:
						{
							rq.m_eStanceMoving = ECharacterStance.STAND;
							rq.m_eStanceEnd = ECharacterStance.CROUCH;
							coverSearchDistMax *= 2;
							moveDurationMax *= 3;
							break;
						}
						case MoraleState.NORMAL:
						{
							rq.m_eStanceMoving = ECharacterStance.CROUCH;
							rq.m_eStanceEnd = ECharacterStance.CROUCH;
							coverSearchDistMax *= 1.7;
							moveDurationMax *= 2.5;
							break;					
						}
						case MoraleState.STRESSED:
						{
							if (Math.RandomInt(0,2) == 1)
							{
								rq.m_eStanceMoving = ECharacterStance.CROUCH;
								rq.m_eStanceEnd = ECharacterStance.PRONE;
							}
							else
							{
								rq.m_eStanceMoving = ECharacterStance.STAND;
								rq.m_eStanceEnd = ECharacterStance.PRONE;
							}
							coverSearchDistMax *= 1.5;
							moveDurationMax *= 2;
							break;					
						}
						case MoraleState.PRESSURED:
						{
							rq.m_eStanceMoving = ECharacterStance.CROUCH;
							rq.m_eStanceEnd = ECharacterStance.PRONE;
							coverSearchDistMax *= 1.2;
							moveDurationMax *= 1.5;
							break;			
						}
						case MoraleState.BREAK:
						{
							rq.m_eStanceMoving = ECharacterStance.CROUCH;
							rq.m_eStanceEnd = ECharacterStance.PRONE;
							break;					
						}
					}
					break;
				}
				default:
				{
					coverSearchDistMin = 10.0;
					coverSearchDistMax = 30.0;
					moveDurationMax = 4;
					
					switch(m_MoraleState)
					{
						case MoraleState.FRESH:
						{
							rq.m_eStanceMoving = ECharacterStance.STAND;
							rq.m_eStanceEnd = ECharacterStance.CROUCH;
							coverSearchDistMax *= 2;
							moveDurationMax *= 3;
							break;
						}
						case MoraleState.NORMAL:
						{
							rq.m_eStanceMoving = ECharacterStance.CROUCH;
							rq.m_eStanceEnd = ECharacterStance.CROUCH;
							coverSearchDistMax *= 1.7;
							moveDurationMax *= 2.5;
							break;					
						}
						case MoraleState.STRESSED:
						{
							if (Math.RandomInt(0,2) == 1)
							{
								rq.m_eStanceMoving = ECharacterStance.CROUCH;
								rq.m_eStanceEnd = ECharacterStance.PRONE;
							}
							else
							{
								rq.m_eStanceMoving = ECharacterStance.STAND;
								rq.m_eStanceEnd = ECharacterStance.CROUCH;
							}
							coverSearchDistMax *= 1.5;
							moveDurationMax *= 2;
							break;					
						}
						case MoraleState.PRESSURED:
						{
							rq.m_eStanceMoving = ECharacterStance.STAND;
							rq.m_eStanceEnd = ECharacterStance.PRONE;
							coverSearchDistMax *= 1.2;
							moveDurationMax *= 1.5;
							break;			
						}
						case MoraleState.BREAK:
						{
							rq.m_eStanceMoving = ECharacterStance.CROUCH;
							rq.m_eStanceEnd = ECharacterStance.PRONE;
							break;					
						}
					}
					break;
				}
			}
			
			if (Math.RandomInt(0,4) == 1)
			{
				rq.m_eMovementType = EMovementType.SPRINT;
			}
			else
			{
				rq.m_eMovementType = EMovementType.RUN;
			}
			

			rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType) &&
								IsAimingAndMovingAllowedForWeapon(m_eWeaponType);
			rq.m_bAimAtTargetEnd = true;
		}
		
		rq.m_bFailIfNoCover = ResolveFailMoveIfNoCover();
		
		// If we are not in cover, min cover search distance is overridden to 0, we should find any cover ASAP
		if (!m_State.m_bInCover)
			coverSearchDistMin = 0;
		
		rq.m_fCoverSearchDistMin = coverSearchDistMin;
		rq.m_fCoverSearchDistMax = coverSearchDistMax;
		rq.m_fMoveDuration_s = Math.RandomFloat(0.7, 1.0) * moveDurationMax;
		
		// Subscribe to events
		// We will pronounce voice lines once we start or end moving
		rq.GetOnMovementStarted().Insert(OnMovementStarted);
		rq.GetOnCompleted().Insert(OnMovementCompleted);
		
		m_State.ApplyNewRequest(rq);
	}
	
	protected override void ResolveMoveRequestMovePosAndDir(vector targetPos, out vector outMovePos, out vector outAvoidStraightPathDir, out SCR_EAICombatMoveDirection outDirection, out float outCoverSearchSectorHalfAngleRad)
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
					eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
					break;					
				}
				case EAIThreatState.THREATENED:
				{
					eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
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
	
	protected static ECharacterStance ResolveStanceOutsideCover(bool closeRange, EAIThreatState threat, MoraleState morales)
	{
		if (closeRange)
		{
			switch (threat)
			{
				case EAIThreatState.THREATENED:
				{
					return ECharacterStance.CROUCH;
				}
				default:
				{
					switch (morales)
					{
						case MoraleState.BREAK:
						{
							return ECharacterStance.CROUCH;
							break;
						}
						case MoraleState.PRESSURED:
						{
							return ECharacterStance.CROUCH;
							break;						
						}
						default :
						{
							return ECharacterStance.STAND;
							break;
						}
					}
				}
			}
		}
		else
		{
			// Long range combat
			switch (threat)
			{
				case EAIThreatState.THREATENED:
				{
					return ECharacterStance.PRONE;
				}
				default:
				{
					switch (morales)
					{
						case MoraleState.BREAK:
						{
							return ECharacterStance.PRONE;
							break;
						}
						case MoraleState.PRESSURED:
						{
							return ECharacterStance.PRONE;
							break;						
						}
						default :
						{
							return ECharacterStance.CROUCH;
							break;
						}
					}
				}
			}
		}
		return ECharacterStance.STAND;
	}
	
	protected override bool MoveFromTargetCondition()
	{
		float weaponMinDist = Math.Max(5.0, m_fWeaponMinDist);
		
		return m_fTargetDist < weaponMinDist;
	}

	//--------------------------------------------------------------------------------------------
	protected static ref TStringArray s_aVarsIn = {
		PORT_BASE_TARGET,
		PORT_AVOID_STRAIGHT_PATH_DIR
	};
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
}