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
		float waitTime;
		
		if (inCover)
		{
			// In cover
			switch (threat)
			{
				case EAIThreatState.THREATENED:
					waitTime = 20.0;	// Stay in cover for a long time, until we are not suppressed any more
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
					waitTime = 6.0;
					break;
				default:
					waitTime = 3.0;
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
			waitTime *= 2.0;
		else
			waitTime *= Math.RandomFloat(0.8, 1.2);
		
		if (longWaitTime)
			waitTime *= 2;
		
		float mult = Math.Map(moraleSystem.GetMoraleMeasure(), 0, 4.5, 0.5, 3);
		waitTime *= mult;
		
		return waitTime;
	}
	
	//--------------------------------------------------------------------------------------------
	protected override bool MoveToNextPosCondition()
	{
		// Don't get any more closer
		// Except we should still move closer if we haven't seen target for a long time
		float optimalDist = ResolveOptimalDistance(m_fWeaponMinDist);
		if (m_fTargetDist < optimalDist && m_Target.GetTimeSinceSeen() < 15)
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
			movePos = targetPos;
			GetMoraleEffects(eDirection);
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
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 10.0;
					moveDurationMax = 3;
					break;
				}
				default:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 15.0;
					moveDurationMax = 4;
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
					coverSearchDistMin = 5.0;
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
			rq.m_bAimAtTarget = false; // Can't aim at tgt while sprinting
			rq.m_bAimAtTargetEnd = true;
		}
		
		rq.m_bFailIfNoCover = ResolveFailMoveIfNoCover();
		
		// If we are not in cover, min cover search distance is overridden to 0, we should find any cover ASAP
		if (!m_State.m_bInCover)
			coverSearchDistMin = 3;
		
		rq.m_fCoverSearchDistMin = coverSearchDistMin;
		rq.m_fCoverSearchDistMax = coverSearchDistMax;
		rq.m_fMoveDuration_s = moveDurationMax * MoraleAmplifyMove();
		
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
				if(Math.RandomFloat(0,1) > 1)
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
	
	float MoraleAmplifyMove()
	{
		return Math.Map(moraleSystem.GetMoraleMeasure(), 0, 4.5, 2, 1);
	}
	
	//--------------------------------------------------------------------------------------------
	protected static ref TStringArray s_aVarsIn = {
		PORT_BASE_TARGET,
		PORT_AVOID_STRAIGHT_PATH_DIR
	};
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
}