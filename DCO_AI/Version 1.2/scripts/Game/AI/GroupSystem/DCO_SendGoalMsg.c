class DCO_AIEvade : SCR_AISendMessageGenerated
{
	[Attribute("")]
	float m_fPriorityLevel;
	
	[Attribute("")]
	bool m_bIsWaypointRelated;
	
	BaseTarget target;
	
	protected static ref TStringArray _s_aVarsIn =
	{
		SCR_AISendMessageGenerated.PORT_RECEIVER,
		"PriorityLevel",
		"IsWaypointRelated",
		"Target"
	};
	override TStringArray GetVariablesIn() { return _s_aVarsIn; }
	
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		AIAgent receiver = GetReceiverAgent(owner);
		DCO_AIEvadeFrom msg = new DCO_AIEvadeFrom();
		
		msg.m_RelatedGroupActivity = GetRelatedActivity(owner);
		
		msg.SetText(m_sText);
		
		if(!GetVariableIn("PriorityLevel", msg.m_fPriorityLevel))
			msg.m_fPriorityLevel = m_fPriorityLevel;
		
		if(!GetVariableIn("Target", msg.m_bTarget))
			msg.m_bTarget = target;
		
		if(!GetVariableIn("IsWaypointRelated", msg.m_bIsWaypointRelated))
			msg.m_bIsWaypointRelated = m_bIsWaypointRelated;
		
		if (msg.m_bIsWaypointRelated)
			msg.m_RelatedWaypoint = GetRelatedWaypoint(owner);
		
		if (SendMessage(owner, receiver, msg))
			return ENodeResult.SUCCESS;
		else
			return ENodeResult.FAIL;
	}
	
	override string GetNodeMiddleText()
	{
		string s;
		s = s + string.Format("m_fPriorityLevel: %1\n", m_fPriorityLevel);
		s = s + string.Format("m_bIsWaypointRelated: %1\n", m_bIsWaypointRelated);
		return s;
	}
	override bool VisibleInPalette() { return true; }
}