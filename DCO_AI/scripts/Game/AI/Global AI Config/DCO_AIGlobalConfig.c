[ComponentEditorProps(category: "GameScripted/AI", description: "Component for utility AI system calculations")]
class SCR_DCOAISettingsComponentClass : ScriptComponentClass
{
}

// Obsolete and should be removed eventually
class SCR_DCOAISettingsComponent : ScriptComponent
{
	[Attribute(defvalue: "1", uiwidget: UIWidgets.Auto, desc: "Default AI Skill", category: "AI Skill",)]
	bool useRandomSkillDistribution;
	
	protected static SCR_DCOAISettingsComponent m_sInstance;

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
	
	//------------------------------------------------------------------------------------------------
	// destructor
	void ~SCR_DCOAISettingsComponent()
	{
		
	}
}
