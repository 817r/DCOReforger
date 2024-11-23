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
		m_bCloseRangeCombat = m_fTargetDist < 20;
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
			
			rq.m_bFailIfNoCover = true;
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
					rq.m_eStanceMoving = ECharacterStance.STAND;
					if (Math.RandomIntInclusive(0, 2) == 1)
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
		rq.m_fMoveDistance = Math.RandomFloat(0.1, 1.5) * moveDistanceMax;
		rq.m_bAimAtTargetEnd = true;
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

	override protected float ResolveStoppedWaitTime(bool inCover, EAIThreatState threat, EWeaponType weaponType)
	{
		float waitTime;
		
		if (tac == DCO_GroupTactic.ASSAULT)
		{
			waitTime = Math.RandomFloat(3.0, 5.0);
			
			if(!m_State.IsInValidCover())
				waitTime -= Math.RandomFloat(0.5, 1.0);
			
			return waitTime;
		}

		waitTime = Math.RandomFloat(7.0, 10.0);

		// When using those weapons we want to move much less
		bool longWaitTime = false;
		bool specialistTime = false;
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
			waitTime *= Math.RandomFloat(1.0, 1.3);
		
		if(specialistTime)
			waitTime += Math.RandomFloat(3.0, 5.0);	
		
		if(!m_State.IsInValidCover())
			waitTime -= 1.5;

		return waitTime;
	}
	
	//--------------------------------------------------------------------------------------------
	// Returns 'optimal' distance
	// If we are between weaponMinDist and 'optimal' dist, we don't need to move closer to tgt
	override protected static float ResolveOptimalDistance(float weaponMinDist)
	{
		return Math.Max(weaponMinDist + 10.0, 10.0);
	}
	
	override protected bool MoveFromTargetCondition()
	{
		return m_fTargetDist < 10;
	}
}

class DCO_AITravelMove : SCR_AICombatMoveLogicBase
{	
	SCR_ChimeraAIAgent m_Agent;
	DCO_CUSTOMRANK rank;
	moraleState morale;
	DCO_GroupTactic tac;
	vector target;
	
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
		m_bCloseRangeCombat = m_fTargetDist < 40;
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
		float moveDistanceMax = Math.RandomFloat(10.0, 15.0);
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
			
			rq.m_bFailIfNoCover = true;
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
					rq.m_eStanceMoving = ECharacterStance.STAND;
					if (Math.RandomIntInclusive(0, 2) == 1)
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
		}
		
		rq.m_fCoverSearchDistMin = coverSearchDistMin;
		rq.m_fCoverSearchDistMax = coverSearchDistMax;
		rq.m_bFailIfNoCover = false;
		rq.m_fMoveDistance = Math.RandomFloat(1.0, 1.5) * moveDistanceMax;
		rq.m_bAimAtTargetEnd = true;
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

	override protected float ResolveStoppedWaitTime(bool inCover, EAIThreatState threat, EWeaponType weaponType)
	{
		float waitTime;
		waitTime = 0.5;
		
		return waitTime;
	}
	
	//--------------------------------------------------------------------------------------------
	// Returns 'optimal' distance
	// If we are between weaponMinDist and 'optimal' dist, we don't need to move closer to tgt
	override protected static float ResolveOptimalDistance(float weaponMinDist)
	{
		return Math.Max(weaponMinDist + 10.0, 10.0);
	}
	
	override protected bool MoveFromTargetCondition()
	{
		return m_fTargetDist < 5;
	}
}

class DCO_AIMoveToSL : SCR_AICombatMoveLogicBase
{	
	vector target;
	DCO_GroupTactic tactics;
	
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

		m_fTargetDist = vector.Distance(owner.GetOrigin(), target);
		m_bCloseRangeCombat = m_fTargetDist < 50;
		m_eThreatState = m_Utility.m_ThreatSystem.GetState();
		m_eStance = m_CharacterController.GetStance();
		m_fWeaponMinDist = m_CombatComp.GetSelectedWeaponMinDist();
		m_eWeaponType = m_CombatComp.GetSelectedWeaponType();
		tactics = m_Utility.getTactics();
		
		if (m_CombatComp.GetCurrentTarget())
		{
			if (!m_CombatComp.IsTargetVisible(m_CombatComp.GetCurrentTarget()))
				PushRequestMove();		
		} else PushRequestMove();	
		
		return ENodeResult.SUCCESS;
	}


	override protected void PushRequestMove()
	{		
		if (tactics == DCO_GroupTactic.EVASIVE)
			return;
		
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;

		// Common values
		rq.m_vTargetPos = target;
		rq.m_vMovePos = rq.m_vTargetPos;
		rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
		rq.m_bTryFindCover = true;
		float coverSearchDistMin = 0;
		float coverSearchDistMax = 30;
		float moveDistanceMax = Math.RandomFloat(coverSearchDistMin, coverSearchDistMax / 10);
		rq.m_fMoveDistance = Math.RandomFloat(1.0, 1.5) * moveDistanceMax;
		
		switch(m_eThreatState)
		{
			case EAIThreatState.EXHAUSTED:
			{
				rq.m_eMovementType = EMovementType.SPRINT;	
				rq.m_eStanceEnd = ECharacterStance.PRONE;
				rq.m_bFailIfNoCover = true;
				if (rq.m_fMoveDistance > 15)
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
				} else if (rq.m_fMoveDistance > 10)
				{
					rq.m_eStanceMoving = ECharacterStance.PRONE;
				} else
				{
					rq.m_eStanceMoving = ECharacterStance.PRONE;
				}
				break;
			}
			case EAIThreatState.PINNED:
			{
				rq.m_eMovementType = EMovementType.RUN;	
				rq.m_eStanceEnd = ECharacterStance.PRONE;
				rq.m_bFailIfNoCover = true;
				if (rq.m_fMoveDistance > 15)
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
				} else if (rq.m_fMoveDistance > 10)
				{
					rq.m_eStanceMoving = ECharacterStance.PRONE;
				} else
				{
					rq.m_eStanceMoving = ECharacterStance.PRONE;
				}
				break;
			}
			case EAIThreatState.THREATENED:
			{
				rq.m_eMovementType = EMovementType.RUN;	
				rq.m_eStanceEnd = ECharacterStance.PRONE;
				rq.m_bFailIfNoCover = false;
				if (rq.m_fMoveDistance > 15)
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
				} else if (rq.m_fMoveDistance > 10)
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
				} else
				{
					rq.m_eStanceMoving = ECharacterStance.PRONE;
				}
				break;
			}
			case EAIThreatState.ALERTED:
			{
				rq.m_eMovementType = EMovementType.RUN;	
				rq.m_eStanceEnd = ECharacterStance.CROUCH;
				rq.m_bFailIfNoCover = false;
				if (rq.m_fMoveDistance > 15)
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
				} else if (rq.m_fMoveDistance > 10)
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
				} else
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
				}
				break;
			}
			case EAIThreatState.VIGILANT:
			{
				rq.m_eMovementType = EMovementType.RUN;	
				rq.m_eStanceEnd = ECharacterStance.CROUCH;
				rq.m_bFailIfNoCover = false;
				if (rq.m_fMoveDistance > 15)
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
				} else if (rq.m_fMoveDistance > 10)
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
				} else
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
				}
				break;
			}
			case EAIThreatState.SAFE:
			{
				rq.m_eMovementType = EMovementType.SPRINT;	
				rq.m_eStanceEnd = ECharacterStance.STAND;
				rq.m_bFailIfNoCover = false;
				if (rq.m_fMoveDistance > 15)
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
				} else if (rq.m_fMoveDistance > 10)
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
				} else
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
				}
				break;
			}
		}
		
		if (m_fTargetDist > 50)
		{
			rq.m_bFailIfNoCover = false;
			rq.m_eMovementType = EMovementType.RUN;	
			rq.m_eStanceEnd = ECharacterStance.CROUCH;
			rq.m_eDirection = SCR_EAICombatMoveDirection.FORWARD;
			coverSearchDistMax = 50;
		} else
		{
			int rand = Math.RandomIntInclusive(1,3);
			if (rand == 1)
				rq.m_eDirection = SCR_EAICombatMoveDirection.FORWARD;
			else if (rand == 2)
				rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
			else if (rand == 3)
				rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
		}
		
		rq.m_bAimAtTarget = false;
		rq.m_bAimAtTargetEnd = false;
		rq.m_fCoverSearchDistMin = coverSearchDistMin;
		rq.m_fCoverSearchDistMax = coverSearchDistMax;
		
		rq.GetOnMovementStarted().Insert(OnMovementStarted);
		rq.GetOnCompleted().Insert(OnMovementCompleted);
		
		m_State.ApplyNewRequest(rq);
	}
	
	override protected bool MoveToNextPosCondition()
	{
		float stoppedWaitTime = ResolveStoppedWaitTime(m_State.m_bInCover);	
		
		if (m_State.IsExecutingRequest())	
			return false;
		
		if (m_Utility.m_ThreatSystem.GetThreatSuppression() > 2.0)
			return false;
		
		if (m_State.IsInValidCover() && m_fTargetDist < 75)
			return false;
		
		if (m_Utility.getTactics() == DCO_GroupTactic.ASSAULT && m_fTargetDist < 150)
			return false;
		
		if (m_fTargetDist > 80)
			return true;
		
		return m_State.m_fTimerStopped_s > stoppedWaitTime;
	}

	protected float ResolveStoppedWaitTime(bool inCover)
	{
		float waitTime;
		waitTime = Math.RandomFloat(15.0, 30.0);
		
		if (inCover && m_Utility.m_ThreatSystem.GetThreatMeasureWithoutInjuryFactor() > 2.0)	
			waitTime *= 2;	
			
		
		return waitTime;
	}
}