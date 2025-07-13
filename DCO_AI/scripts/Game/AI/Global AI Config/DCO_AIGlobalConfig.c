[ComponentEditorProps(category: "GameScripted/AI", description: "Component for utility AI system calculations")]
class SCR_DCOAISettingsComponentClass : ScriptComponentClass
{
}

// Obsolete and should be removed eventually
class SCR_DCOAISettingsComponent : ScriptComponent
{
	[Attribute(defvalue: "0", uiwidget: UIWidgets.Auto, desc: "Default AI Skill", category: "AI Skill",)]
	bool useRandomSkillDistribution;
	
	[Attribute("3", UIWidgets.ComboBox, "AI skill in combat",category: "Individual Setting", ParamEnumArray.FromEnum(DCO_SKILL), category: "AI General Skill", )]
	protected DCO_SKILL m_eDefaultGlobalSkill;
	
	[Attribute( defvalue: "5", category: "Individual Setting", uiwidget: UIWidgets.Slider, desc: "Unit skill", params: "1 20 1" )]
	protected float m_MaxAimImprovementBoost;
	
	protected static SCR_DCOAISettingsComponent m_sInstance;
	
	//------------------------------------------------------------------------------------------------
	//! All Set Params
	DCO_SKILL SetDefaultIndividualAIGlobalSkill(DCO_SKILL skill)
	{
		m_eDefaultGlobalSkill = skill;
		return m_eDefaultGlobalSkill;
	}
	
	float SetDefaultIndividualAIAimImprovementBoost(float boost)
	{
		m_MaxAimImprovementBoost = boost;
		return m_MaxAimImprovementBoost;
	}
	
	//------------------------------------------------------------------------------------------------
	//! All Get Params
	DCO_SKILL GetDefaultIndividualAIGlobalSkill()
	{
		return m_eDefaultGlobalSkill;
	}
	
	float GetDefaultIndividualAIAimImprovementBoost()
	{
		return m_MaxAimImprovementBoost;
	}

	//------------------------------------------------------------------------------------------------
	//! \return
	static SCR_DCOAISettingsComponent GetInstance()
	{
		return m_sInstance;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_sInstance = this;
	}
}
