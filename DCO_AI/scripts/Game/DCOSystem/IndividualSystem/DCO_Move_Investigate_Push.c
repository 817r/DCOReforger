class SCR_AIDCO_AttackPush: AITaskScripted
{
	protected static const float COVER_SEARCH_DIST_MAX = 20.0;
	protected const float COVER_QUERY_SECTOR_ANGLE_RAD = 2;
	
	// Inputs
	protected static const string PORT_POSITION = "Position";
	
	// Outputs
	protected static const string PORT_COMPLETE_ACTION = "CompleteAction";
	
	protected SCR_AICombatMoveState m_State;
	protected SCR_AIUtilityComponent m_Utility;
	protected CharacterControllerComponent m_CharacterController;
	
	protected float m_fNextUpdate_ms;
	[Attribute("500")]
	protected float m_fUpdateInterval_ms;
	
	override void OnInit(AIAgent owner)
	{
		IEntity myEntity = owner.GetControlledEntity();
		
		m_Utility = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
		if (m_Utility)
			m_State = m_Utility.m_CombatMoveState;
		
		if (myEntity)
			m_CharacterController = CharacterControllerComponent.Cast(myEntity.FindComponent(CharacterControllerComponent));
	}
	
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		float currentTime_ms = GetGame().GetWorld().GetWorldTime();
		if (currentTime_ms < m_fNextUpdate_ms)
			return ENodeResult.RUNNING;
		m_fNextUpdate_ms = currentTime_ms + m_fUpdateInterval_ms;
		
		IEntity myEntity = owner.GetControlledEntity();
		
		if (!m_State || !myEntity)
			return ENodeResult.FAIL;
		
		
		vector threatPos;
		GetVariableIn(PORT_POSITION, threatPos);
		if (threatPos == vector.Zero)
			return ENodeResult.FAIL;
		
		if (MoveToNextPosCondition())
		{
			float distToThreat = vector.Distance(myEntity.GetOrigin(), threatPos);
			CombatMoveLogic(threatPos, distToThreat);
		}
		
		return ENodeResult.RUNNING;
	}
	
	void CombatMoveLogic(vector threatPos, float distToThreat)
	{				
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
			
		rq.m_eReason = SCR_EAICombatMoveReason.INVESTIGATE;
		rq.m_eType = SCR_EAICombatMoveRequestType.INVESTIGATE;
		rq.m_vTargetPos = threatPos;
		rq.m_vMovePos = rq.m_vTargetPos;
		rq.m_bTryFindCover = true;
		rq.m_bUseCoverSearchDirectivity = true;
		if (Math.RandomInt(0,2) == 1)
			rq.m_bCheckCoverVisibility = true;
		else
			rq.m_bCheckCoverVisibility = false;
		if (Math.RandomInt(0,3) == 2)
			rq.m_bFailIfNoCover = false;
		else
			rq.m_bFailIfNoCover = true;
		ResolveMoveandStopStance(rq.m_eStanceMoving, rq.m_eStanceEnd);
		rq.m_fCoverSearchDistMax = COVER_SEARCH_DIST_MAX;
		rq.m_fCoverSearchDistMin = 10;
		if (Math.RandomInt(0,2) == 1)
			rq.m_eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
		else
			rq.m_eDirection = SCR_EAICombatMoveDirection.FORWARD;
		rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
		rq.m_bAimAtTarget = false;
		rq.m_bAimAtTargetEnd = true;
		rq.m_fMoveDuration_s = Math.RandomFloat(2,5);
		rq.m_eMovementType = EMovementType.RUN;
		
		if (m_Utility.m_ThreatSystem.GetState() < EAIThreatState.ALERTED)
			rq.m_fMoveDuration_s *= 3;
		
		vector dirToTgt = threatPos - m_Utility.m_OwnerEntity.GetOrigin();
		rq.m_vAvoidStraightPathDir = dirToTgt;
		m_State.ApplyNewRequest(rq);			
	}
	
	protected void ResolveMoveandStopStance(out ECharacterStance moving, out ECharacterStance end)
	{
		if (m_Utility.m_ThreatSystem.GetSuppressionMeasure() > 0.6)
			moving = ECharacterStance.PRONE;
		else
		{
			if (Math.RandomInt(0,2) == 1)
				moving = ECharacterStance.STAND;
			else
				moving = ECharacterStance.CROUCH;
		}
		
		if (moving == ECharacterStance.STAND)
			end = ECharacterStance.CROUCH;
		else if (moving == ECharacterStance.CROUCH)
			end = ECharacterStance.PRONE;
		else
			end = ECharacterStance.PRONE;
	}
	
	protected float ResolveStoppedWaitTime(bool inCover)
	{
		float waitTime = 0;
			
		if (inCover)
		{
			waitTime = 8.0;
		} else
		{
			waitTime = 5.0;
		}
		
		if (m_State && m_State.IsMovingToBuilding())
			waitTime *= 5;
			
		waitTime = Math.RandomFloat(0.8, 1.2) * waitTime;
			
		return waitTime;
	}
		
	protected bool MoveToNextPosCondition()
	{
		if (m_State.IsExecutingRequest())
			return false;
		
		if (m_Utility.m_ThreatSystem.GetSuppressionMeasure() > 0.6)
			return false;
		
		if (m_Utility.m_ThreatSystem.GetState() == EAIThreatState.THREATENED)
			return false;
			
		float stoppedWaitTime = ResolveStoppedWaitTime(m_State.m_bInCover);	
		return m_State.m_fTimerStopped_s > stoppedWaitTime;
	}

	protected static ref TStringArray s_aVarsIn = { PORT_POSITION };
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
	
	override static bool VisibleInPalette() { return true; }
}