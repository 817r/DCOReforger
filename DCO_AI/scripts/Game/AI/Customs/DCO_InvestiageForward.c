class DCO_AIMoveInvestigate : SCR_AICombatMoveLogicBase
{	
	SCR_ChimeraAIAgent m_Agent;
	DCO_CUSTOMRANK rank;
	moraleState morale;
	DCO_GroupTactic tac;
	vector target;
	AIDangerEvent danger;
	
	protected static const string AREA_PORT = "Area";
	
	protected ref TStringArray s_aVarsIn = {AREA_PORT};
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
	
	protected override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		float currentTime_ms = GetGame().GetWorld().GetWorldTime();
		if (currentTime_ms < m_fNextUpdate_ms)
			return ENodeResult.RUNNING;
		m_fNextUpdate_ms = currentTime_ms + m_fUpdateInterval_ms;
		
		//if (!OnUpdate(owner, dt))
		//	return ENodeResult.FAIL;
		
		//if (!m_State || !m_MyEntity || !m_Utility || !m_CombatComp || !m_CharacterController)
		//	return ENodeResult.FAIL;
		
		// Don't run combat movement logic if CombatMove BT is not used now (like in turret)
		SCR_AIBehaviorBase executedBehavior = SCR_AIBehaviorBase.Cast(m_Utility.GetExecutedAction());
		if (executedBehavior && !executedBehavior.m_bUseCombatMove)
			return ENodeResult.RUNNING;
		
		GetVariableIn(AREA_PORT, target);
		
		tac = m_Utility.getTactics();
		rank = m_Utility.m_DCO_Skill.GetCharacterRank(m_MyEntity);
		morale = m_Utility.m_DCOMoraleSystem.GetMoraleMeasure();
		m_fTargetDist = vector.Distance(owner.GetOrigin(), target);
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
		
		if (MoveToNextPosCondition())
		{
			// We've waited here too long, move to next place
			PushRequestMove();
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
		rq.m_vTargetPos = target;
		rq.m_vMovePos = rq.m_vTargetPos;
		rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
		rq.m_bTryFindCover = true;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility = true;

		float coverSearchDistMin = 0;
		float coverSearchDistMax = 20;
		float moveDistanceMax = Math.RandomFloat(3.0, 8.0);
		if (m_bCloseRangeCombat)
		{
			// Close range combat
			
			switch (m_eThreatState)
			{
				case EAIThreatState.EXHAUSTED:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					break;
				}
				case EAIThreatState.PINNED:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					break;
				}
				case EAIThreatState.THREATENED:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					break;
				}
				case EAIThreatState.VIGILANT:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					break;
				}
				default:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					break;
				}
			}
			
			rq.m_bFailIfNoCover = false;
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType) &&
								IsAimingAndMovingAllowedForWeapon(m_eWeaponType);
			rq.m_bAimAtTargetEnd = false;
		}
		else
		{
			// Long range combat
			
			switch (m_eThreatState)
			{
				case EAIThreatState.EXHAUSTED:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					if (Math.RandomIntInclusive(0, 1) == 1)
						rq.m_eMovementType = EMovementType.SPRINT;
					else
						rq.m_eMovementType = EMovementType.RUN;
					if (Math.RandomIntInclusive(1, 5) > 2)
						rq.m_eStanceEnd = ECharacterStance.CROUCH;
					else
						rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
					break;
				}
				case EAIThreatState.PINNED:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eMovementType = EMovementType.SPRINT;
					if (Math.RandomIntInclusive(1, 5) > 2)
						rq.m_eStanceEnd = ECharacterStance.CROUCH;
					else
						rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
					break;
				}
				case EAIThreatState.THREATENED:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eMovementType = EMovementType.RUN;
					if (Math.RandomIntInclusive(1, 5) > 2)
						rq.m_eStanceEnd = ECharacterStance.CROUCH;
					else
						rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_bAimAtTarget = true;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eMovementType = EMovementType.RUN;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
					
					break;
				}
				case EAIThreatState.VIGILANT:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					if (Math.RandomIntInclusive(1, 5) > 2)
						rq.m_eMovementType = EMovementType.SPRINT;
					else
						rq.m_eMovementType = EMovementType.RUN;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
					break;
				}
				default:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eMovementType = EMovementType.SPRINT;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
					break;
				}
			}
			
			rq.m_eDirection = SCR_EAICombatMoveDirection.FORWARD;

			// rq.m_bAimAtTarget = false; // Can't aim at tgt while sprinting
			
		}
		
		rq.m_fCoverSearchDistMin = coverSearchDistMin;
		rq.m_fCoverSearchDistMax = coverSearchDistMax;
		rq.m_bFailIfNoCover = false;
		rq.m_fMoveDistance = Math.RandomFloat(1.0, 2.0) * moveDistanceMax;
		rq.m_bAimAtTargetEnd = false;
		rq.GetOnMovementStarted().Insert(OnMovementStarted);
		rq.GetOnCompleted().Insert(OnMovementCompleted);
		
		m_State.ApplyNewRequest(rq);
	}
	
	override protected bool MoveToNextPosCondition()
	{
		if (m_State.IsExecutingRequest())
			return false;
		
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
		rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE; // Move back from target
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
		rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
		rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
		rq.m_fMoveDistance = 7.0;
		rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
		rq.m_bAimAtTargetEnd = true;
		rq.m_bCheckCoverVisibility = true;
		rq.m_bFailIfNoCover = false;
		m_State.ApplyNewRequest(rq);
	}

	override protected float ResolveStoppedWaitTime(bool inCover, EAIThreatState threat, EWeaponType weaponType)
	{
		float waitTime;
		waitTime = Math.RandomFloat(10.0, 15.0);

		// When using those weapons we want to move much less
		bool longWaitTime = false;
		bool specialistTime = false;
		bool closerangeTime = false;
		switch (weaponType)
		{
			case EWeaponType.WT_MACHINEGUN:
			case EWeaponType.WT_GRENADELAUNCHER:
			case EWeaponType.WT_SNIPERRIFLE:
			case EWeaponType.WT_HANDGUN:
				longWaitTime = true;
		}
		
		switch (weaponType)
		{
			case EWeaponType.WT_HANDGUN:
			case EWeaponType.WT_SNIPERRIFLE:
				specialistTime = true;
		}
		
		if (longWaitTime)
			waitTime *= Math.RandomFloat(1.7, 2.0);
		
		if(specialistTime)
			waitTime += Math.RandomFloat(1.0, 5.0);	

		return waitTime;
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
		return m_fTargetDist < 5;
	}
}