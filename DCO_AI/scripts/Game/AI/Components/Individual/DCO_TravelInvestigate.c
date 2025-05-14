class DCO_AITravelInvestigate : SCR_AICombatMoveLogicBase
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
		float coverSearchDistMax = Math.RandomFloat(10.0, 30.0);
		float moveDistanceMax = Math.RandomFloat(3.0, 8.0);
		if (m_bCloseRangeCombat)
		{
			// Close range combat
			
			switch (m_eThreatState)
			{
				case EAIThreatState.THREATENED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
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
		}
		else
		{
			// Long range combat
			
			switch (m_eThreatState)
			{
				case EAIThreatState.THREATENED:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eMovementType = EMovementType.RUN;
					if (Math.RandomIntInclusive(1, 5) > 2)
						rq.m_eStanceEnd = ECharacterStance.CROUCH;
					else
						rq.m_eStanceEnd = ECharacterStance.PRONE;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eMovementType = EMovementType.RUN;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					
					break;
				}
				default:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eMovementType = EMovementType.RUN;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					break;
				}
			}
			
			rq.m_eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
			
		}
		
		rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType) &&
								IsAimingAndMovingAllowedForWeapon(m_eWeaponType);
		rq.m_fCoverSearchDistMin = coverSearchDistMin;
		rq.m_fCoverSearchDistMax = coverSearchDistMax;
		rq.m_bFailIfNoCover = ResolveFailMoveIfNoCover();
		rq.m_fMoveDuration_s = Math.RandomFloat(5.0, 15.0);
		rq.m_bAimAtTargetEnd = true;
		rq.m_vAvoidStraightPathDir = m_vAvoidStraightPathDir;
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
		waitTime = Math.RandomFloat(10.0, 20.0);

		if(!m_State.IsInValidCover())
			waitTime -= 3.0;

		return waitTime;
	}
}