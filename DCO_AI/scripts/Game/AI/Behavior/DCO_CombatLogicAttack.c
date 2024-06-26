modded class SCR_AICombatMoveUtils
{
	static const float CLOSE_RANGE_COMBAT_DIST = 30.0;
}


modded class SCR_AICombatMoveLogic_Attack : AITaskScripted
{	
	protected const float COVER_QUERY_SECTOR_ANGLE_RAD = 0.35 * Math.PI;
	
	SCR_ChimeraAIAgent m_Agent;
	DCO_CUSTOMRANK rank;
	moraleState morale;
	
	AIDangerEvent danger;
	
	protected override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		float currentTime_ms = GetGame().GetWorld().GetWorldTime();
		if (currentTime_ms < m_fNextUpdate_ms)
			return ENodeResult.RUNNING;
		m_fNextUpdate_ms = currentTime_ms + m_fUpdateInterval_ms;
		
		BaseTarget target;
		GetVariableIn(PORT_BASE_TARGET, target);
		if (!target || !target.GetTargetEntity() || !m_State || !m_MyEntity || !m_Utility || !m_CombatComp || !m_CharacterController)
			return ENodeResult.FAIL;
		
		// Don't run combat movement logic if CombatMove BT is not used now (like in turret)
		SCR_AIBehaviorBase executedBehavior = SCR_AIBehaviorBase.Cast(m_Utility.GetExecutedAction());
		if (executedBehavior && !executedBehavior.m_bUseCombatMove)
			return ENodeResult.RUNNING;
		
		// Update cached variables
		m_Target = target;
		rank = m_Utility.m_DCO_Skill.GetCharacterRank(m_MyEntity);
		m_fTargetDist = target.GetDistance();
		morale = m_Utility.m_DCOMoraleSystem.GetMoraleMeasure();
		m_bCloseRangeCombat = m_fTargetDist < SCR_AICombatMoveUtils.CLOSE_RANGE_COMBAT_DIST;
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
		else if (!m_State.m_bInCover && IsFirstExecution())
		{
			CoverManager(target.GetLastDetectedPosition(), SCR_EAICombatMoveDirection.BACKWARD);			
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
			ECharacterStance newStance = ResolveStanceOutsideCover(m_bCloseRangeCombat, m_eThreatState, morale, rank);
			if (newStance > m_eStance)
			{
				// Only let stance go down, no need to get back up
				m_State.ApplyRequestChangeStanceOutsideCover(newStance);
			}
			
			if (m_State.m_fTimerStopped_s > Math.RandomFloat(8.0, 11.0) && m_eThreatState >= EAIThreatState.THREATENED && morale == moraleState.ANXIOUS)
				CoverManager(target.GetLastDetectedPosition(), SCR_EAICombatMoveDirection.BACKWARD);
		}
		
		return ENodeResult.RUNNING;
	}
	
	override protected bool SuppressedInCoverCondition()
	{
		//float currMorale = m_DCOMorale.GetMoraleMeasure();
		
		return m_State.m_bInCover && (m_eThreatState >= EAIThreatState.PINNED || morale >= moraleState.ANXIOUS);
	}

	override protected void PushRequestMove()
	{		
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
		
		// Common values
		rq.m_vTargetPos = ResolveRequestTargetPos();
		ResolveMoveRequestMovePosAndDir(rq.m_vTargetPos, rq.m_vMovePos, rq.m_eDirection, rq.m_fCoverSearchSectorHalfAngleRad, morale);
		rq.m_bTryFindCover = true;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility = true;

		float coverSearchDistMin = 0;
		float coverSearchDistMax = 30;
		float moveDistanceMax = Math.RandomFloat(5.0, 30.0);
		if (m_bCloseRangeCombat)
		{
			// Close range combat
			
			switch (m_eThreatState)
			{
				case EAIThreatState.EXHAUSTED:
				{
					rq.m_eStanceMoving = ECharacterStance.PRONE;
					rq.m_eStanceEnd = ECharacterStance.PRONE;
					rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
					rq.m_bCheckCoverVisibility = false;
					coverSearchDistMin = 2.0;
					coverSearchDistMax = 5.0;
					moveDistanceMax = 3.0;
					break;
				}
				case EAIThreatState.PINNED:
				{
					rq.m_eStanceMoving = ECharacterStance.PRONE;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 7.0;
					moveDistanceMax = 3.0;
					break;
				}
				case EAIThreatState.THREATENED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
					coverSearchDistMin = 2.0;
					coverSearchDistMax = 8.0;
					moveDistanceMax = 3.0;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
					coverSearchDistMin = 2.0;
					coverSearchDistMax = 8.0;
					moveDistanceMax = 5.0;
					break;
				}
				case EAIThreatState.VIGILANT:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eDirection = SCR_EAICombatMoveDirection.FORWARD;
					coverSearchDistMin = 2.0;
					coverSearchDistMax = 10.0;
					moveDistanceMax = 5.0;
					break;
				}
				default:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
					coverSearchDistMin = 2.0;
					coverSearchDistMax = 12.0;
					moveDistanceMax = 7.0;
					break;
				}
			}
			
			rq.m_bCheckCoverVisibility = true;
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
				case EAIThreatState.EXHAUSTED:
				{
					coverSearchDistMin = 2.0;
					coverSearchDistMax = 8.0;
					moveDistanceMax = 5.0;
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_bCheckCoverVisibility = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
					if (Math.RandomIntInclusive(0, 1) == 1)
						rq.m_eMovementType = EMovementType.SPRINT;
					else
						rq.m_eMovementType = EMovementType.RUN;
					if (Math.RandomIntInclusive(1, 5) > 2)
						rq.m_eStanceEnd = ECharacterStance.CROUCH;
					else
						rq.m_eStanceEnd = ECharacterStance.PRONE;
					rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
					rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
					break;
				}
				case EAIThreatState.PINNED:
				{
					coverSearchDistMin = 10.0;
					coverSearchDistMax = 15.0;
					moveDistanceMax = 10.0;
					if (Math.RandomIntInclusive(0, 1) == 1)
						rq.m_eStanceMoving = ECharacterStance.STAND;
					else
						rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eMovementType = EMovementType.SPRINT;
					if (Math.RandomIntInclusive(1, 5) > 2)
						rq.m_eStanceEnd = ECharacterStance.CROUCH;
					else
						rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
					rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
					break;
				}
				case EAIThreatState.THREATENED:
				{
					coverSearchDistMin = 2.0;
					coverSearchDistMax = 8.0;
					moveDistanceMax = 5.0;
					if (Math.RandomIntInclusive(0, 1) == 1)
						rq.m_eStanceMoving = ECharacterStance.CROUCH;
					else
						rq.m_eStanceMoving = ECharacterStance.PRONE;
					rq.m_eMovementType = EMovementType.RUN;
					if (Math.RandomIntInclusive(1, 5) > 2)
						rq.m_eStanceEnd = ECharacterStance.CROUCH;
					else
						rq.m_eStanceEnd = ECharacterStance.PRONE;
					rq.m_bAimAtTarget = true;
					if (Math.RandomIntInclusive(1, 2) == 2)
						rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
					else
						rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					coverSearchDistMin = 2.0;
					coverSearchDistMax = 12.0;
					moveDistanceMax = 8.0;
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eMovementType = EMovementType.RUN;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
					rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
					break;
				}
				case EAIThreatState.VIGILANT:
				{
					coverSearchDistMin = 2.0;
					coverSearchDistMax = 15.0;
					moveDistanceMax = 10.0;
					rq.m_eStanceMoving = ECharacterStance.STAND;
					if (Math.RandomIntInclusive(1, 5) > 2)
						rq.m_eMovementType = EMovementType.SPRINT;
					else
						rq.m_eMovementType = EMovementType.RUN;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
					rq.m_eDirection = SCR_EAICombatMoveDirection.FORWARD;
					break;
				}
				default:
				{
					coverSearchDistMin = 2.0;
					coverSearchDistMax = 15.0;
					moveDistanceMax = 12.0; // Shouldn't be so large because we are sprinting and can't shoot
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eMovementType = EMovementType.SPRINT;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
					break;
				}
			}
			
			if (IsFirstExecution())
				rq.m_bFailIfNoCover = false; // On first run we want to move to cover, or stay where we are if there is no cover, and shoot.
			else
				rq.m_bFailIfNoCover = m_State.m_bInCover; // Don't leave cover if there is no next cover

			// rq.m_bAimAtTarget = false; // Can't aim at tgt while sprinting
			rq.m_bAimAtTargetEnd = true;
		}
		
		// If we are not in cover, min cover search distance is overridden to 0, we should find any cover ASAP
		if (!m_State.m_bInCover)
		{
			coverSearchDistMin = 3;
			coverSearchDistMax = 50;
		}
			
		
		rq.m_fCoverSearchDistMin = coverSearchDistMin;
		rq.m_fCoverSearchDistMax = coverSearchDistMax;
		rq.m_fMoveDistance = Math.RandomFloat(0.2, 2.0) * moveDistanceMax; // Move distance if cover is not found, randomized
		
		// Subscribe to events
		// We will pronounce voice lines once we start or end moving
		rq.GetOnMovementStarted().Insert(OnMovementStarted);
		rq.GetOnCompleted().Insert(OnMovementCompleted);
		
		m_State.ApplyNewRequest(rq);
	}

	// Resolves which move pos and dir. we should use for _MOVE_ request
	// By now rq.m_vTargetPos must be already calculated!
	protected void ResolveMoveRequestMovePosAndDir(vector targetPos, out vector outMovePos, out SCR_EAICombatMoveDirection outDirection, out float outCoverSearchSectorHalfAngleRad, moraleState morales)
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
			switch(morales)
			{
				case moraleState.NORMAL:
				{
					eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
					break;
				}
				case moraleState.MOTIVATED:
				{
					eDirection = SCR_EAICombatMoveDirection.FORWARD;
					break;
				}
				case moraleState.ANXIOUS:
				{
					int rand = Math.RandomInt(1,2);
					if (rand == 1)
						eDirection = SCR_EAICombatMoveDirection.LEFT;
					else
						eDirection = SCR_EAICombatMoveDirection.RIGHT;
				
					break;
				}
				case moraleState.MANIAC:
				{
					eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
					break;
				}
				case moraleState.BREAK:
				{
					eDirection = SCR_EAICombatMoveDirection.BACKWARD;
					break;
				}
			}
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
					eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
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
	
	override protected bool MoveToNextPosCondition()
	{
		// Don't get any more closer
		// Except we should still move closer if we haven't seen target for a long time
		float optimalDist = ResolveOptimalDistance(m_fWeaponMinDist);
		if (m_fTargetDist < optimalDist && m_Target.GetTimeSinceSeen() < 10)
			return false;
			
		if (m_State.IsExecutingRequest())
			return false;
		
		if (morale >= moraleState.ANXIOUS && m_eThreatState >= EAIThreatState.THREATENED && !m_State.m_bInCover)
			return m_State.m_fTimerStopped_s > Math.RandomFloat(4.0, 7.0);
		
		float stoppedWaitTime = ResolveStoppedWaitTime(m_State.m_bInCover, m_eThreatState, m_eWeaponType);	
		return m_State.m_fTimerStopped_s > stoppedWaitTime;
	}

	override protected void SuppressedInCoverLogic()
	{
		// We're pinned in cover and can't do much else now
		// Alternate hiding in cover and unhiding
		
		// How long to wait here? Depends on timer value from previous request.
		int suppressedTimes;
		float waitTime_s;
		SCR_AICombatMoveRequestBase rq = m_State.GetRequest();
		if (SCR_AICombatMoveRequest_ChangeStanceInCover.Cast(rq) && rq.m_eReason == SCR_EAICombatMoveReason.SUPPRESSED_IN_COVER)
			waitTime_s = rq.m_f_UserTimer_s;
		else
		{
			if (m_State.m_bExposedInCover)
				waitTime_s = 1.0;
			else
				waitTime_s = Math.RandomFloat(4.0, 10.0);
		}
		
		if (m_State.m_bExposedInCover && m_State.m_fTimerRequest_s > waitTime_s)
		{
			float newWaitTime = Math.RandomFloat(4.0, 9.0); // Hide in cover for this time
			int randomizer = Math.RandomInt(1,2);
			if (randomizer == 1)
				PushRequestChangeStanceInCover(false, SCR_EAICombatMoveReason.SUPPRESSED_IN_COVER, newWaitTime);
			else
				PushRequestMove();
			suppressedTimes ++;
			
			if (suppressedTimes > 4)
			{
				PushRequestMove();
				suppressedTimes = 0;
			}
		}
		else if (!m_State.m_bExposedInCover && m_State.m_fTimerRequest_s > waitTime_s)
		{
			float newWaitTime = Math.RandomFloat(1.0, 5.0); // Expose out of cover for this time
			PushRequestChangeStanceInCover(true, SCR_EAICombatMoveReason.SUPPRESSED_IN_COVER, newWaitTime);
			suppressedTimes +1;
			
			if (suppressedTimes > 6)
			{
				int randomizer = Math.RandomInt(1,2);
				if (randomizer == 1)
					PushRequestChangeStanceInCover(true, SCR_EAICombatMoveReason.SUPPRESSED_IN_COVER, newWaitTime);
				else
				{
					PushRequestLeaveUselessCover();
					suppressedTimes = 0;
				}
			}
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
		rq.m_eStanceEnd = ECharacterStance.CROUCH;
		rq.m_eMovementType = EMovementType.RUN;
		rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD; // Move back from target
		rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
		rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
		rq.m_bAimAtTargetEnd = true;
		
		rq.m_fCoverSearchDistMin = Math.RandomFloat(3.0, 5.0);
		rq.m_fCoverSearchDistMax = Math.RandomFloat(10.0, 30.0);
		
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
		rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
		rq.m_fMoveDistance = 7.0;
		rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
		rq.m_bAimAtTargetEnd = true;
		rq.m_bCheckCoverVisibility = false;
		m_State.ApplyNewRequest(rq);
	}

	override protected float ResolveStoppedWaitTime(bool inCover, EAIThreatState threat, EWeaponType weaponType)
	{
		float waitTime;
		float CQB;
		if (inCover)
		{
			// In cover
			switch (threat)
			{
				case EAIThreatState.EXHAUSTED:
					waitTime = Math.RandomFloat(35.0, 60.0);
					break;
				case EAIThreatState.PINNED:
					waitTime = Math.RandomFloat(15.0, 25.0);
					break;
				case EAIThreatState.THREATENED:
					waitTime = Math.RandomFloat(10.0, 15.0);	// Stay in cover for a long time, until we are not suppressed any more
					break;
				default:
					waitTime = Math.RandomFloat(8.0, 10.0);
			}
		}
		else
		{
			// Not in cover
			switch (threat)
			{
				case EAIThreatState.EXHAUSTED:
					waitTime = Math.RandomFloat(10.0, 20.0);
					break;
				case EAIThreatState.PINNED:
					waitTime = Math.RandomFloat(10.0, 15.0);
					break;
				case EAIThreatState.THREATENED:
					waitTime = Math.RandomFloat(8.0, 12.0);
					break;
				default:
					waitTime = Math.RandomFloat(8.0, 15.0);
					break;
			}
		}
		
		// When using those weapons we want to move much less
		bool longWaitTime = false;
		bool specialistTime = false;
		bool closerangeTime = false;
		switch (weaponType)
		{
			case EWeaponType.WT_MACHINEGUN:
			case EWeaponType.WT_ROCKETLAUNCHER:
			case EWeaponType.WT_GRENADELAUNCHER:
			case EWeaponType.WT_SNIPERRIFLE:
				longWaitTime = true;
		}
		
		switch (weaponType)
		{
			case EWeaponType.WT_HANDGUN:
			case EWeaponType.WT_SNIPERRIFLE:
				specialistTime = true;
		}
		
		if(m_bCloseRangeCombat)
		{
			switch(threat)
			{
				case EAIThreatState.EXHAUSTED:
				{
					CQB = Math.RandomFloat(25.0, 32.0);
					break;
				}
				
				case EAIThreatState.PINNED:
				{
					CQB = Math.RandomFloat(18.0, 21.0);
					break;
				}
				
				case EAIThreatState.THREATENED:
				{
					CQB = Math.RandomFloat(15.0, 18.0);
					break;
				}
		
				case EAIThreatState.ALERTED:
				{
					CQB = Math.RandomFloat(13.0, 18.0);
					break;				
				}
				
				case EAIThreatState.VIGILANT:
				{
					CQB = Math.RandomFloat(10.0, 12.0);
					break;				
				}
				
				case EAIThreatState.SAFE:
				{
					CQB = Math.RandomFloat(5.0, 8.0);
					break;				
				}
				
				default:
				{
					CQB = Math.RandomFloat(10.0, 15.0);
					break;				
				}
			}
		}
		
		if (longWaitTime)
			waitTime *= Math.RandomFloat(1.7, 2.0);
		
		if(specialistTime)
			waitTime *= Math.RandomFloat(1.2, 1.5);
		
		if(m_bCloseRangeCombat)
			waitTime += CQB;		
		
		return waitTime;
	}
	
	//--------------------------------------------------------------------------------------------
	// Returns stance when stopped outside cover
	protected static ECharacterStance ResolveStanceOutsideCover(bool closeRange, EAIThreatState threat, moraleState morales, DCO_CUSTOMRANK ranks)
	{
		if (closeRange)
		{
			switch (threat)
			{
				case EAIThreatState.EXHAUSTED:
				{
					switch (morales)
					{
						case moraleState.NORMAL:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.MANIAC:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.MOTIVATED:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.ANXIOUS:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.PRONE;
									break;
								}
							}
							break;
						}
						case moraleState.BREAK:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.STAND;
									break;
								}
							}
							break;
						}
					}
				break;
				}
				
				case EAIThreatState.PINNED:
				{
					switch (morales)
					{
						case moraleState.NORMAL:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.MANIAC:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.MOTIVATED:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.STAND;
									break;
								}
							}
							break;
						}
						case moraleState.ANXIOUS:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.PRONE;
									break;
								}
							}
							break;
						}
						case moraleState.BREAK:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
					}
				break;
				}
				
				case EAIThreatState.THREATENED:
				{
					switch (morales)
					{
						case moraleState.NORMAL:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.MANIAC:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.MOTIVATED:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.ANXIOUS:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.BREAK:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
					}
				break;
				}
				
				case EAIThreatState.ALERTED:
				{
					switch (morales)
					{
						case moraleState.NORMAL:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.MANIAC:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.STAND;
									break;
								}
							}
							break;
						}
						case moraleState.MOTIVATED:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.ANXIOUS:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.BREAK:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
					}
				break;
				}
				
				case EAIThreatState.VIGILANT:
				{
					switch (morales)
					{
						case moraleState.NORMAL:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.MANIAC:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.STAND;
									break;
								}
							}
							break;
						}
						case moraleState.MOTIVATED:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.STAND;
									break;
								}
							}
							break;
						}
						case moraleState.ANXIOUS:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.BREAK:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.STAND;
									break;
								}
							}
							break;
						}
					}
				break;
				}
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
				case EAIThreatState.EXHAUSTED:
				{
					switch (morales)
					{
						case moraleState.NORMAL:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.PRONE;
									break;
								}
							}
							break;
						}
						case moraleState.MANIAC:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.MOTIVATED:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.PRONE;
									break;
								}
							}
							break;
						}
						case moraleState.ANXIOUS:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.PRONE;
									break;
								}
							}
							break;
						}
						case moraleState.BREAK:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
					}
				break;
				}
				
				case EAIThreatState.PINNED:
				{
					switch (morales)
					{
						case moraleState.NORMAL:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.PRONE;
									break;
								}
							}
							break;
						}
						case moraleState.MANIAC:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.MOTIVATED:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.PRONE;
									break;
								}
							}
							break;
						}
						case moraleState.ANXIOUS:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.PRONE;
									break;
								}
							}
							break;
						}
						case moraleState.BREAK:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.PRONE;
									break;
								}
							}
							break;
						}
					}
				break;
				}
				
				case EAIThreatState.THREATENED:
				{
					switch (morales)
					{
						case moraleState.NORMAL:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.MANIAC:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.PRONE;
									break;
								}
							}
							break;
						}
						case moraleState.MOTIVATED:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.ANXIOUS:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.PRONE;
									break;
								}
							}
							break;
						}
						case moraleState.BREAK:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.PRONE;
									break;
								}
							}
							break;
						}
					}
				break;
				}
				
				case EAIThreatState.ALERTED:
				{
					switch (morales)
					{
						case moraleState.NORMAL:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.MANIAC:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.MOTIVATED:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.ANXIOUS:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.BREAK:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.PRONE;
									break;
								}
							}
							break;
						}
					}
				break;
				}
				
				case EAIThreatState.VIGILANT:
				{
					switch (morales)
					{
						case moraleState.NORMAL:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.MANIAC:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.MOTIVATED:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.STAND;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.CROUCH;
									break;
								}
							}
							break;
						}
						case moraleState.ANXIOUS:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.CROUCH;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.PRONE;
									break;
								}
							}
							break;
						}
						case moraleState.BREAK:
						{
							switch (ranks)
							{
								case DCO_CUSTOMRANK.RECRUIT:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
								{
									return ECharacterStance.PRONE;
									break;
								}
								case DCO_CUSTOMRANK.SPECIALIST:
								{
									return ECharacterStance.PRONE;
									break;
								}
							}
							break;
						}
					}
				break;
				}
				
				default:
					return ECharacterStance.STAND;

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
		int randomizer = Math.RandomInt(1,2);
			if (randomizer == 1)
				rq.m_eMovementType = EMovementType.WALK;
			else
				rq.m_eMovementType = EMovementType.RUN;
		rq.m_fMoveDistance = Math.RandomFloat(1.0, 3.0);
		rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
		rq.m_bAimAtTargetEnd = true;
		rq.m_bCheckCoverVisibility = true;
		m_State.ApplyNewRequest(rq);
	}
	
	override protected bool MoveFromTargetCondition()
	{
		return m_fTargetDist < 20;
	}
	
	void CoverManager(vector threatPos, SCR_EAICombatMoveDirection dir)
	{						
		if (m_State.IsExecutingRequest())
			return;
		
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		if (m_eWeaponType == EWeaponType.WT_MACHINEGUN)
		{
			rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
			
			rq.m_vTargetPos = threatPos;
			rq.m_vMovePos = rq.m_vTargetPos;
			rq.m_bTryFindCover = true;
			rq.m_bUseCoverSearchDirectivity = true;
			rq.m_bCheckCoverVisibility = true;
			rq.m_bFailIfNoCover = false;
			rq.m_eStanceMoving = ECharacterStance.CROUCH;
			rq.m_eStanceEnd = ECharacterStance.PRONE;
			rq.m_eMovementType = EMovementType.WALK;
			rq.m_fCoverSearchDistMax = 20;
			rq.m_fCoverSearchDistMin = 2;
			rq.m_fMoveDistance = Math.RandomFloat(1.0, 1.5) * 5;
			rq.m_eDirection = dir;
			rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
			rq.m_bAimAtTarget = true;; // Don't aim while running
			rq.m_bAimAtTargetEnd = true;
			
			m_State.ApplyNewRequest(rq);	
		} 
		else
		{
			rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
			
			rq.m_vTargetPos = threatPos;
			rq.m_vMovePos = rq.m_vTargetPos;
			rq.m_bTryFindCover = true;
			rq.m_bUseCoverSearchDirectivity = true;
			rq.m_bCheckCoverVisibility = true;
			rq.m_bFailIfNoCover = false;
			rq.m_eStanceMoving = ECharacterStance.STAND;
			rq.m_eStanceEnd = ECharacterStance.CROUCH;
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_fCoverSearchDistMax = 25;
			rq.m_fCoverSearchDistMin = 2;
			rq.m_fMoveDistance = Math.RandomFloat(1.0, 1.5) * 10;
			rq.m_eDirection = dir;
			rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
			rq.m_bAimAtTarget = false;; // Don't aim while running
			rq.m_bAimAtTargetEnd = true;
			
			m_State.ApplyNewRequest(rq);		
		}
	}
}