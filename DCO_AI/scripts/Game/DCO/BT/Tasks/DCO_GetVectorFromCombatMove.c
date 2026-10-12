class SCR_AIGetVectorFromCombatMove : AITaskScripted
{
	protected static string PORT_REQUEST() { return "Request"; }

	protected static string PORT_POS() { return "Pos"; }

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		IEntity myEntity = owner.GetControlledEntity();
		if (!myEntity)
			return ENodeResult.FAIL;

		SCR_AICombatMoveRequestBase rqBase;
		GetVariableIn(PORT_REQUEST(), rqBase);
		SCR_AICombatMoveRequest_Move rq = SCR_AICombatMoveRequest_Move.Cast(rqBase);
		if (!rq)
			return SCR_AIErrorMessages.NodeErrorCombatMoveRequest(this, owner, rq);

		vector randomizedPos = rq.m_vMovePos;

		SetVariableOut(PORT_POS(), randomizedPos);

		return ENodeResult.SUCCESS;
	}

	protected vector RandomizeDestinationPos(float distance, vector centerPos)
	{
		float radius = distance;
		vector pos = s_AIRandomGenerator.GenerateRandomPointInRadius(0, radius, centerPos, true);
		pos[1] = centerPos[1];
		return pos;
	}

	protected static ref TStringArray s_aVarsOut;
	protected static TStringArray GetVarsOut()
	{
		if (!s_aVarsOut)
			s_aVarsOut = { PORT_POS() };
		return s_aVarsOut;
	}
	override TStringArray GetVariablesOut() { return GetVarsOut(); }

	protected static ref TStringArray s_aVarsIn;
	protected static TStringArray GetVarsIn()
	{
		if (!s_aVarsIn)
			s_aVarsIn = { PORT_REQUEST() };
		return s_aVarsIn;
	}
	override TStringArray GetVariablesIn() { return GetVarsIn(); }

	static override bool VisibleInPalette() { return true; }
}