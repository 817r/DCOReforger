class SCR_DCO_AIGroupConfigComponentClass : ScriptComponentClass
{
}

enum DCO_GroupTactics
{
	AGGRESSIVE,
	DEFENSIVE,
	EVASIVE,
	BALANCED,
	AUTOMATIC
}

class SCR_DCO_AIGroupConfigComponent : ScriptComponent
{
	[Attribute("3", UIWidgets.ComboBox, "AI Tactics in combat", "", ParamEnumArray.FromEnum(DCO_GroupTactics), category: "AI Tactics to use", )]
	DCO_GroupTactics m_Tactics;
	
	static SCR_DCO_AIGroupConfigComponent GetDCOGroupAIConfigComponent(IEntity unit)
	{		
		SCR_DCO_AIGroupConfigComponent comp = SCR_DCO_AIGroupConfigComponent.Cast(unit.FindComponent(SCR_DCO_AIGroupConfigComponent));
		if (comp) return comp;
		return null;
	}
	
	DCO_GroupTactics GetTactics()
	{
		 return m_Tactics;
	}
	
	DCO_GroupTactics SetTactics(DCO_GroupTactics Tactics)
	{
		m_Tactics = Tactics;
		return Tactics;
	}

	override void OnPostInit(IEntity owner)
    {		
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
    }
}