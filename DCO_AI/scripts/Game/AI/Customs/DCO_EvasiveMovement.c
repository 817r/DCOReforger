class DCO_Evasive_EVALUATION: AITaskScripted
{
	protected static const float COVER_SEARCH_DIST_MAX = 20.0;
	protected const float COVER_QUERY_SECTOR_ANGLE_RAD = 0.35 * Math.PI;
	
	// Inputs
	protected static const string PORT_POSITION = "Position";
	
	// Outputs
	protected static const string PORT_COMPLETE_ACTION = "CompleteAction";
	
	protected SCR_AICombatMoveState m_State;
	protected SCR_AIUtilityComponent m_Utility;
	protected CharacterControllerComponent m_CharacterController;
	
	IEntity myEnt;
	AIGroup myGroup;
	
	protected bool m_bPushedMoveRequest = false;
	
	override void OnInit(AIAgent owner)
	{
		IEntity myEntity = owner.GetControlledEntity();
		myEnt = myEntity;
		
		m_Utility = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
		if (m_Utility)
			m_State = m_Utility.m_CombatMoveState;
		
		if (myEntity)
			m_CharacterController = CharacterControllerComponent.Cast(myEntity.FindComponent(CharacterControllerComponent));
		
		myGroup = owner.GetParentGroup();
	}
	
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		IEntity myEntity = owner.GetControlledEntity();
		
		if (!m_State || !myEntity)
			return ENodeResult.FAIL;
		
		
		vector threatPos;
		GetVariableIn(PORT_POSITION, threatPos);
		if (threatPos == vector.Zero)
			return ENodeResult.FAIL;
		
		float distToThreat = vector.Distance(myEntity.GetOrigin(), threatPos);
		if (MoveToNextPosCondition())
		{
			CombatMoveLogic(threatPos, distToThreat);
		}
		
		return ENodeResult.RUNNING;
	}
	
	void CombatMoveLogic(vector threatPos, float distToThreat)
	{				
		if (!m_State.IsInValidCover() && !m_bPushedMoveRequest && !m_State.IsMovingToCover())
		{		
			SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
			rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
			
			rq.m_vTargetPos = threatPos;
			rq.m_vMovePos = threatPos;
			rq.m_bTryFindCover = true;
			rq.m_bUseCoverSearchDirectivity = true;
			rq.m_bCheckCoverVisibility = false;
			rq.m_bFailIfNoCover = false;
			rq.m_eStanceMoving = ECharacterStance.CROUCH;
			rq.m_eStanceEnd = ECharacterStance.CROUCH;
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_fCoverSearchDistMax = COVER_SEARCH_DIST_MAX;
			rq.m_fCoverSearchDistMin = 3;
			rq.m_fMoveDistance = Math.RandomFloat(0.5, 1.0) * COVER_SEARCH_DIST_MAX;
			rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
			rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
			rq.m_bAimAtTarget = false; // Don't aim while running
			rq.m_bAimAtTargetEnd = true;
			
			m_State.ApplyNewRequest(rq);
			m_bPushedMoveRequest = true;
		}
	}
	
	protected bool MoveToNextPosCondition()
	{			
		if (m_State.IsExecutingRequest())
			return false;
		
		float stoppedWaitTime = ResolveStoppedWaitTime(m_State.m_bInCover);	
		return m_State.m_fTimerStopped_s > stoppedWaitTime;
	}
	
	protected float ResolveStoppedWaitTime(bool inCover)
	{
		float waitTime;
		
		if (inCover)
			waitTime = Math.RandomFloat(7.0, 10.0);
		else
			waitTime = Math.RandomFloat(2.5, 6.0);
		
		return waitTime;
	}
	
	protected static ref TStringArray s_aVarsIn = { PORT_POSITION };
	override TStringArray GetVariablesIn() { return s_aVarsIn; }

	//protected static ref TStringArray s_aVarsOut = { PORT_COMPLETE_ACTION };
	//override TStringArray GetVariablesOut() { return s_aVarsOut; }
	
	override bool VisibleInPalette() { return true; }
}