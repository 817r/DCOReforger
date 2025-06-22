class SCR_DCO_AIConfigComponentClass : ScriptComponentClass
{
}

enum DCO_SKILL
{
	CONSCPRIT,
	GREEN,
	REGULAR,
	VETERAN,
	CRACK,
	ELITE
}

class SCR_DCO_AIConfigComponent : ScriptComponent
{
	[Attribute("", UIWidgets.Object)]
	ref array<ref DCO_Personality> m_Personality;
	
	[Attribute("", UIWidgets.ComboBox, "AI skill in combat", "", ParamEnumArray.FromEnum(DCO_SKILL), category: "AI General Skill", )]
	DCO_SKILL m_SkillSet;
	
	[Attribute( defvalue: "1", category: "AI AIM", uiwidget: UIWidgets.CheckBox, desc: "Alow aim Improvement" )]
	bool m_EnableAimImprovement;
	
	[Attribute( defvalue: "1500", category: "AI AIM", uiwidget: UIWidgets.Slider, desc: "Unit skill", params: "1 5000 1" )]
	float m_MaxRangeAimImprovement;
	
	[Attribute( defvalue: "5", category: "AI AIM", uiwidget: UIWidgets.Slider, desc: "Unit skill", params: "1 20 1" )]
	float m_MaxAimImprovementBoost;

	// Aim Improvement
	//==========
	bool EnableAimImprovement()
	{
		return m_EnableAimImprovement;
	}
	
	bool SetAimImprovement(bool TF)
	{
		m_EnableAimImprovement = TF;
		return TF;
	}
	
	// Aim Max Range
	//==========
	float GetAimMaxRangeEffect()
	{
		return m_MaxRangeAimImprovement;
	}
	
	float SetAimMaxRangeEffect(float Ranges)
	{
		m_MaxRangeAimImprovement = Ranges;
		return Ranges;
	}
	
	// Aim Max Boost
	//==========
	float GetMaxAimImprovement()
	{
		return m_MaxAimImprovementBoost;
	}
	
	float SetMaxAimImprovement(float Max)
	{
		m_MaxAimImprovementBoost = Max;
		return Max;
	}
	
	// Skill Level
	//==========
	DCO_SKILL GetSkillLevel()
	{
		return m_SkillSet;
	}
	
	DCO_SKILL SetSkillLevel(DCO_SKILL skills)
	{
		m_SkillSet = skills;
		return skills;
	}
	
	static SCR_DCO_AIConfigComponent GetDCOAIConfigComponent(IEntity unit)
	{		
		AIControlComponent ctrl = AIControlComponent.Cast(unit.FindComponent(AIControlComponent));
		if (ctrl)
		{
			SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(ctrl.GetAIAgent());
			if (agent)
			{
				return SCR_DCO_AIConfigComponent.Cast(agent.FindComponent(SCR_DCO_AIConfigComponent));
			}
		}
		return null;
	}
	
	static bool GetEnableAimImprovement(IEntity unit)
	{
		if (!unit)
			return false;
		
		SCR_DCO_AIConfigComponent comp = GetDCOAIConfigComponent(unit);
		
		if (!comp)
			return false;
		
		return comp.EnableAimImprovement();
	}
	
	// Init and Event
	override void OnPostInit(IEntity owner)
    {		
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
    }
	
	override void EOnInit(IEntity owner)
	{
		m_SkillSet = Math.RandomInt(DCO_SKILL.REGULAR, DCO_SKILL.CRACK);
	}
}