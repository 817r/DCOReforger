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
	DCO_GroupTactics m_eExternalTactics;
	DCO_GroupTactics m_eActualTactics;
	
	[Attribute( defvalue: "70", category: "Defensive Tactics", uiwidget: UIWidgets.Slider, desc: "Defend Radius for AI when they are on defensive tactics", params: "1 150 1" )]
	float m_DefendRadius;
	
	static SCR_DCO_AIGroupConfigComponent GetDCOGroupAIConfigComponent(IEntity unit)
	{		
		SCR_DCO_AIGroupConfigComponent comp = SCR_DCO_AIGroupConfigComponent.Cast(unit.FindComponent(SCR_DCO_AIGroupConfigComponent));
		if (comp) return comp;
		return null;
	}
	
	DCO_GroupTactics GetTactics()
	{
		 return m_eExternalTactics;
	}
	
	DCO_GroupTactics GetActualTactics()
	{
		return m_eActualTactics;
	}
	
	DCO_GroupTactics SetTactics(DCO_GroupTactics Tactics)
	{
		m_eExternalTactics = Tactics;
		return Tactics;
	}
	
	float SetDefendRadius(float r)
	{
		m_DefendRadius = r;
		return r;
	}
	
	float GetDefendRadius()
	{
		return m_DefendRadius;
	}

	override void OnPostInit(IEntity owner)
    {		
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
    }
}