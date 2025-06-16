class SCR_AICombatMoveLogic_SquadCohession : SCR_AICombatMoveLogicBase
{
	protected AIAgent Leader;
	protected AIGroup Group;
	protected MoraleState m_MoraleState;
	
	protected float MinRange = 5;
	protected float MaxRange = 70;
	

	protected override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		float currentTime_ms = GetGame().GetWorld().GetWorldTime();
		if (currentTime_ms < m_fNextUpdate_ms)
			return ENodeResult.RUNNING;
		m_fNextUpdate_ms = currentTime_ms + m_fUpdateInterval_ms;
		
		if (!m_State || !m_MyEntity || !m_Utility || !m_CombatComp || !m_CharacterController)
			return ENodeResult.FAIL;
		
		// Don't run combat movement logic if CombatMove BT is not used now (like in turret)
		SCR_AIBehaviorBase executedBehavior = SCR_AIBehaviorBase.Cast(m_Utility.GetExecutedAction());
		if (executedBehavior && !executedBehavior.m_bUseCombatMove)
			return ENodeResult.FAIL;
		
		AIAgent agent = m_Utility.GetAIAgent();
		Group = agent.GetParentGroup();
		Leader = Group.GetLeaderAgent();
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
		

		if (MoveToNextPosCondition())
		{
			PushRequestMove();
		}
		
		return ENodeResult.RUNNING;
	}
	
	//--------------------------------------------------------------------------------------------
	protected override float GetTargetDistance()
	{
		return vector.Distance(m_Utility.GetOrigin(), Leader.GetControlledEntity().GetOrigin());
	}
	
	//--------------------------------------------------------------------------------------------
	protected override vector GetTargetPosition()
	{
		return Leader.GetControlledEntity().GetOrigin();
	}
	
	
	//--------------------------------------------------------------------------------------------
	protected override bool ResolveFailMoveIfNoCover()
	{
		if (Math.RandomInt(0,101) < m_CombatComp.GetCoverChances())
			return m_State.m_bInCover;
		else
			return false;			
	}
	
	//--------------------------------------------------------------------------------------------
	protected override bool MoveToNextPosCondition()
	{				
		if (m_State.IsExecutingRequest())
			return false;
		
		if (m_Utility.m_ThreatSystem.GetState() > EAIThreatState.ALERTED)
			return false;
		 
		float max = Group.GetAgentsCount() * 5;
		if (GetTargetDistance() > max)
		{
			if (!m_State.m_bInCover)
				return true;

		} else if (vector.Distance(m_Utility.GetOrigin(), Group.GetCenterOfMass()) > max/2)
			return true;
		
		return false;
	}
	
	protected override void PushRequestMove()
	{		
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
		
		// Common values
		rq.m_vTargetPos = GetTargetPosition();
		ResolveMoveRequestMovePosAndDir(rq.m_vTargetPos, rq.m_vMovePos, rq.m_vAvoidStraightPathDir, rq.m_eDirection, rq.m_fCoverSearchSectorHalfAngleRad);
		rq.m_bTryFindCover = true;
		rq.m_bUseCoverSearchDirectivity = true;

		float coverSearchDistMin = 2;
		float coverSearchDistMax = 15;
		float moveDurationMax = 6;
			switch (m_eThreatState)
			{
				case EAIThreatState.THREATENED:
				{
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
					moveDurationMax = 10;
					
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
			
			
			rq.m_bAimAtTarget = false;
			rq.m_bAimAtTargetEnd = false;
		}
		
		rq.m_bFailIfNoCover = ResolveFailMoveIfNoCover();
		rq.m_eMovementType = EMovementType.RUN;
		rq.m_fCoverSearchDistMin = coverSearchDistMin;
		rq.m_fCoverSearchDistMax = coverSearchDistMax;
		rq.m_fMoveDuration_s = Math.RandomFloat(0.7, 1.3) * moveDurationMax;
		
		// Subscribe to events
		// We will pronounce voice lines once we start or end moving
		rq.GetOnMovementStarted().Insert(OnMovementStarted);
		rq.GetOnCompleted().Insert(OnMovementCompleted);
		
		m_State.ApplyNewRequest(rq);
	}
	
	protected override void ResolveMoveRequestMovePosAndDir(vector targetPos, out vector outMovePos, out vector outAvoidStraightPathDir, out SCR_EAICombatMoveDirection outDirection, out float outCoverSearchSectorHalfAngleRad)
	{			
		vector movePos;
		SCR_EAICombatMoveDirection eDirection;
		float coverSearchSectorHalfAngleRad = Math.PI/2;
		vector avoidStraightPathDir;
		
		eDirection = SCR_EAICombatMoveDirection.FORWARD;
		movePos = targetPos;
		avoidStraightPathDir = vector.Zero;
		
		outMovePos = movePos;
		outDirection = eDirection;
		outAvoidStraightPathDir = avoidStraightPathDir;
		outCoverSearchSectorHalfAngleRad = coverSearchSectorHalfAngleRad;
	}
}