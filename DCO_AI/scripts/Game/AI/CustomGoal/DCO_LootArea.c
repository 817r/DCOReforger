class SCR_AISendGoalMessage_DCO_LootArea: SCR_AISendMessageGenerated
{	
	[Attribute("0")]
	protected float m_fPriorityLevel;
	
	[Attribute("0")]
	protected bool m_bIsWaypointRelated;
	
	protected static ref TStringArray s_aVarsIn = {
		SCR_AISendMessageGenerated.PORT_RECEIVER,
		"m_fPriorityLevel",
		"m_bIsWaypointRelated",
	};
	
	override TStringArray GetVariablesIn()
	{
		return s_aVarsIn;
	}
	
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		AIAgent receiver = GetReceiverAgent(owner);
		
		return ENodeResult.SUCCESS;
	}
	
	//-----------------------------------------------------------------------------------------------------------------------------------------------
	static override bool VisibleInPalette() { return false; }	
}