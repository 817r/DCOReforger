class SCR_DCO_AIConfigComponentClass : ScriptComponentClass
{
}

enum DCO_SKILL
{
	NOOB,
	NOVICE,
	REGULAR,
	SPECIAL_FORCE
}

class SCR_DCO_AIConfigComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.ComboBox, "AI skill in combat", "", ParamEnumArray.FromEnum(DCO_SKILL) )]
	DCO_SKILL m_SkillSet;
	
	[Attribute( defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Alow aim Improvement" )]
	bool m_EnableAimImprovement;
	
	[Attribute( defvalue: "1500", uiwidget: UIWidgets.Slider, desc: "Unit skill", params: "1 5000 1" )]
	float m_MaxRangeAimImprovement;
	
	[Attribute( defvalue: "2", uiwidget: UIWidgets.Slider, desc: "Unit skill", params: "1 10 1" )]
	float m_MaxAimImprovementBoost;

	bool EnableAimImprovement()
	{
		return m_EnableAimImprovement;
	}
	
	bool SetAimImprovement(bool TF)
	{
		m_EnableAimImprovement = TF;
		return TF;
	}
	
	float GetAimMaxRangeEffect()
	{
		return m_MaxRangeAimImprovement;
	}
	
	float SetAimMaxRangeEffect(float Ranges)
	{
		m_MaxRangeAimImprovement = Ranges;
		return Ranges;
	}
	
	float GetMaxAimImprovement()
	{
		return m_MaxAimImprovementBoost;
	}
	
	float SetMaxAimImprovement(float Max)
	{
		m_MaxAimImprovementBoost = Max;
		return Max;
	}
	
	static SCR_DCO_AIConfigComponent GetCharacterRankComponent(IEntity unit)
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
		
		SCR_DCO_AIConfigComponent comp = GetCharacterRankComponent(unit);
		
		if (!comp)
			return false;
		
		return comp.EnableAimImprovement();
	}
}