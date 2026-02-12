class SCR_AIGroupTacticsComponentClass : ScriptComponentClass
{
}

class SCR_AIGroupTacticsComponent : ScriptComponent
{
	[Attribute("0", UIWidgets.ComboBox, "AI Tactics in combat", "", ParamEnumArray.FromEnum(DCO_GroupTactics) )]
	protected DCO_GroupTactics m_eAITacticsDefault;
	
	protected SCR_AIGroup m_Group;
	protected SCR_AIGroupPerception m_GroupPerception;
	
	//------------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{																																																																																																																																																																																									
		RplComponent rplComponent = RplComponent.Cast(owner.FindComponent(RplComponent));

		m_Group = SCR_AIGroup.Cast(owner);
		
		if (!m_Group)
			return;
		
		if (!rplComponent || !rplComponent.IsMaster())
			return;
	}

	DCO_GroupTactics GetGroupTactics()
	{
		return m_eAITacticsDefault;
	}

}
