class DCO_AIEvasiveTactics : SCR_AICombatMoveLogicBase
{	
	SCR_ChimeraAIAgent m_Agent;
	vector target;
	protected vector m_vAvoidStraightPathDir;
	protected MoraleState m_MoraleState;
	
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
		m_vAvoidStraightPathDir = target - m_Utility.m_OwnerEntity.GetOrigin();
		m_vAvoidStraightPathDir.Normalize();
				
		m_fTargetDist = vector.Distance(owner.GetOrigin(), target);
		m_bCloseRangeCombat = m_fTargetDist < SCR_AICombatMoveUtils.CLOSE_RANGE_COMBAT_DIST;
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
		
		if (MoveToNextPosCondition())
		{
			// We've waited here too long, move to next place
			PushRequestMove();
		}
		
		return ENodeResult.RUNNING;
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

		const float coverSearchDistMin = 0;
		const float coverSearchDistMax = 20;
		float moveDistanceMax = Math.RandomFloat(3.0, 10.0);
		if (m_bCloseRangeCombat)
		{
			// Close range combat
			
			switch (m_eThreatState)
			{
				case EAIThreatState.THREATENED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
					
					break;
				}
				case EAIThreatState.ALERTED:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
					rq.m_vAvoidStraightPathDir = m_vAvoidStraightPathDir;
					break;
				}
				case EAIThreatState.VIGILANT:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eDirection = SCR_EAICombatMoveDirection.FORWARD;
					rq.m_vAvoidStraightPathDir = m_vAvoidStraightPathDir;
					break;
				}
				default:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
					break;
				}
			}
			
			rq.m_bFailIfNoCover = true;
			rq.m_eMovementType = EMovementType.WALK;
		}
		else
		{
			// Long range combat
			
			switch (m_eThreatState)
			{
				case EAIThreatState.THREATENED:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					if (Math.RandomIntInclusive(1, 5) > 2)
						rq.m_eMovementType = EMovementType.RUN;
					else
						rq.m_eMovementType = EMovementType.SPRINT;
					if (Math.RandomIntInclusive(1, 5) > 2)
						rq.m_eStanceEnd = ECharacterStance.CROUCH;
					else
						rq.m_eStanceEnd = ECharacterStance.PRONE;
					rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eMovementType = EMovementType.RUN;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
					
					break;
				}
				case EAIThreatState.VIGILANT:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					if (Math.RandomIntInclusive(1, 5) > 2)
						rq.m_eMovementType = EMovementType.RUN;
					else
						rq.m_eMovementType = EMovementType.WALK;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eDirection = SCR_EAICombatMoveDirection.FORWARD;
					rq.m_vAvoidStraightPathDir = m_vAvoidStraightPathDir;
					break;
				}
				default:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eMovementType = EMovementType.RUN;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
					rq.m_vAvoidStraightPathDir = m_vAvoidStraightPathDir;
					break;
				}
			}
			
			// rq.m_bAimAtTarget = false; // Can't aim at tgt while sprinting
			
		}
		rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType) &&
								IsAimingAndMovingAllowedForWeapon(m_eWeaponType);
		rq.m_fCoverSearchDistMin = coverSearchDistMin;
		rq.m_fCoverSearchDistMax = coverSearchDistMax;
		rq.m_bFailIfNoCover = ResolveFailMoveIfNoCover();
		rq.m_fMoveDuration_s = moveDistanceMax;
		rq.m_bAimAtTargetEnd = true;
		
		rq.GetOnMovementStarted().Insert(OnMovementStarted);
		rq.GetOnCompleted().Insert(OnMovementCompleted);
		
		m_State.ApplyNewRequest(rq);
	}
	
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
	
	override protected bool MoveToNextPosCondition()
	{
		if (m_State.IsExecutingRequest())
			return false;
		
		float stoppedWaitTime = ResolveStoppedWaitTime(m_State.m_bInCover, m_eThreatState, m_eWeaponType);	
		return m_State.m_fTimerStopped_s > stoppedWaitTime;
	}

	override protected float ResolveStoppedWaitTime(bool inCover, EAIThreatState threat, EWeaponType weaponType)
	{
		float waitTime;
		waitTime = Math.RandomFloat(2.0, 6.0);

		if(m_MoraleState < MoraleState.STRESSED)
			waitTime = waitTime/5;

		return waitTime;
	}
}