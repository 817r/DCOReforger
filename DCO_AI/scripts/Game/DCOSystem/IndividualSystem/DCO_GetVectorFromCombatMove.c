class SCR_AIGetVectorFromCombatMove : AITaskScripted
{
	// Inputs
	protected static const string PORT_REQUEST = "Request";
  	protected static const string PORT_TARGET_POS = "TargetPos";
	
	// Outputs
	protected static const string PORT_POS = "Pos";
	
	//--------------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{		
		IEntity myEntity = owner.GetControlledEntity();
		if (!myEntity)
			return ENodeResult.FAIL;
		
		// Read inputs
		SCR_AICombatMoveRequestBase rqBase;
		GetVariableIn(PORT_REQUEST, rqBase);
		SCR_AICombatMoveRequest_Move rq = SCR_AICombatMoveRequest_Move.Cast(rqBase);
		if (!rq)
			return SCR_AIErrorMessages.NodeErrorCombatMoveRequest(this, owner, rq);
		
		vector randomizedPos = rq.m_vMovePos;
		
		SetVariableOut(PORT_POS, randomizedPos);
		
		return ENodeResult.SUCCESS;
	}
	
	//--------------------------------------------------------------------------------------------
	protected vector RandomizeDestinationPos(float distance, vector centerPos)
	{
		float radius = distance;
		vector pos = s_AIRandomGenerator.GenerateRandomPointInRadius(0, radius, centerPos, true);
		pos[1] = centerPos[1];
		return pos;
	}
	
	//--------------------------------------------------------------------------------------------
	protected static ref TStringArray s_aVarsOut = {
		PORT_POS
	};
	override TStringArray GetVariablesOut() { return s_aVarsOut; }
	
	protected static ref TStringArray s_aVarsIn = {
		PORT_REQUEST
	};
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
	
	static override bool VisibleInPalette() { return true; }
	
}