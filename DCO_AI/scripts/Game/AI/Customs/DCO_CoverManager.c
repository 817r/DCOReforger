class SCR_AIDCO_COVERMANAGER: AITaskScripted
{
	protected static const float COVER_SEARCH_DIST_MAX = 40.0;
	protected const float COVER_QUERY_SECTOR_ANGLE_RAD = 0.35 * Math.PI;
	
	// Inputs
	protected static const string PORT_POSITION = "Position";
	
	// Outputs
	protected static const string PORT_COMPLETE_ACTION = "CompleteAction";
	
	protected SCR_AICombatMoveState m_State;
	protected SCR_AIUtilityComponent m_Utility;
	protected CharacterControllerComponent m_CharacterController;
	
	protected bool m_bPushedMoveRequest = false;
	
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
		IEntity myEntity = owner.GetControlledEntity();
		
		if (!m_State || !myEntity)
			return ENodeResult.FAIL;
		
		
		vector threatPos;
		GetVariableIn(PORT_POSITION, threatPos);
		if (threatPos == vector.Zero)
			return ENodeResult.FAIL;
		
		float distToThreat = vector.Distance(myEntity.GetOrigin(), threatPos);
		CombatMoveLogic(threatPos, distToThreat);
		
		return ENodeResult.SUCCESS;
	}
	
	void CombatMoveLogic(vector threatPos, float distToThreat)
	{				
		if (!m_State.IsInValidCover() && !m_bPushedMoveRequest && !m_State.IsMovingToCover())
		{		
			SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
			
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
			rq.m_fCoverSearchDistMax = COVER_SEARCH_DIST_MAX;
			rq.m_fCoverSearchDistMin = 2;
			rq.m_fMoveDistance = Math.RandomFloat(0.2, 1.0) * COVER_SEARCH_DIST_MAX;
			rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
			rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
			rq.m_bAimAtTarget = true; // Don't aim while running
			rq.m_bAimAtTargetEnd = true;
			
			m_State.ApplyNewRequest(rq);
			m_bPushedMoveRequest = true;
		}
	}
	
	protected static ref TStringArray s_aVarsIn = { PORT_POSITION };
	override TStringArray GetVariablesIn() { return s_aVarsIn; }

	//protected static ref TStringArray s_aVarsOut = { PORT_COMPLETE_ACTION };
	//override TStringArray GetVariablesOut() { return s_aVarsOut; }
	
	override bool VisibleInPalette() { return true; }
}