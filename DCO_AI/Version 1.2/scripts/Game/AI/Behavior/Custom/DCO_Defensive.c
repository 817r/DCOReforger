class DCO_DEFENSIVE_EVALUATION_COVER: AITaskScripted
{
	protected static const float COVER_SEARCH_DIST_MAX = 25.0;
	protected const float COVER_QUERY_SECTOR_ANGLE_RAD = 0.35 * Math.PI;
	
	// Inputs
	protected static const string PORT_POSITION = "Position";
	protected static const string PORT_LEADER_POS = "LeaderPos";
	
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
		vector leaderPos;
		GetVariableIn(PORT_POSITION, threatPos);
		if (threatPos == vector.Zero)
			return ENodeResult.FAIL;
		
		GetVariableIn(PORT_POSITION, leaderPos);
		if (leaderPos == vector.Zero)
			return ENodeResult.FAIL;
		
		float distToThreat = vector.Distance(myEntity.GetOrigin(), threatPos);
		m_bPushedMoveRequest = m_State.IsExecutingRequest();
		
		if (!m_bPushedMoveRequest)
			CombatMoveLogic(threatPos, leaderPos, distToThreat);
		
		return ENodeResult.SUCCESS;
	}
	
	void CombatMoveLogic(vector threatPos, vector leaderPos, float distToThreat)
	{				
		if (!m_State.IsInValidCover() && !m_bPushedMoveRequest && !m_State.IsMovingToCover())
		{		
			SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
			
			vector myLeaderPos = myGroup.GetLeaderEntity().GetOrigin();
			
			rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
			rq.m_vTargetPos = threatPos;
			rq.m_vMovePos = leaderPos;
			rq.m_bTryFindCover = true;
			rq.m_bUseCoverSearchDirectivity = true;
			rq.m_bCheckCoverVisibility = true;
			rq.m_bFailIfNoCover = true;
			rq.m_eStanceMoving = ECharacterStance.STAND;
			rq.m_eStanceEnd = ECharacterStance.CROUCH;
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_fCoverSearchDistMax = COVER_SEARCH_DIST_MAX;
			rq.m_fCoverSearchDistMin = 3;
			rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 3.0);
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