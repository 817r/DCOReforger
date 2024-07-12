class DCO_GroupUtility : AITaskScripted
{	
	protected SCR_AIGroup m_groupOwner;
	protected SCR_AIGroupUtilityComponent m_Utility;
	
	protected static const string PORT_TACTICS = "Group Tactics";
	protected DCO_GroupTactic m_Tactics;
	
	protected ref TStringArray s_aVarsOut = {PORT_TACTICS};
	override TStringArray GetVariablesOut() { return s_aVarsOut; }
	
	//------------------------------------------------------------------------------------------------
	override bool VisibleInPalette() {return true;}
	
	//------------------------------------------------------------------------------------------------
	override void OnInit(AIAgent owner)
	{
		m_groupOwner = SCR_AIGroup.Cast(owner);
		if (!m_groupOwner)
		{
			m_groupOwner = SCR_AIGroup.Cast(owner.GetParentGroup());
			if (!m_groupOwner)
			{
				SCR_AgentMustBeAIGroup(this, owner);
				return;
			}	
		}
		m_Utility = SCR_AIGroupUtilityComponent.Cast(m_groupOwner.FindComponent(SCR_AIGroupUtilityComponent));
	}
	
	//------------------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		DCO_GroupTactic tac;
		tac = m_Utility.getTactics();
		
		SetVariableOut(PORT_TACTICS, tac);
		
		return ENodeResult.SUCCESS;
	}	

	//------------------------------------------------------------------------------------------------
	protected static ref TStringArray s_aVarsIn = {	
	};
	
	override TStringArray GetVariablesIn()
	{
		return s_aVarsIn;
	}
	
	//------------------------------------------------------------------------------------------------
	override string GetOnHoverDescription()
	{
		return "AllocateActionsForDefendActivity: Goes over all group members and alocates them either turret, smart action or sector defend.\n Works only inside defend activity under defend waypoint.";
	}
}