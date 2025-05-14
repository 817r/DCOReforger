[ComponentEditorProps(category: "GameScripted/AI", description: "Component for utility AI system calculations")]
class SCR_DCOAISettingsComponentClass : ScriptComponentClass
{
}

// Obsolete and should be removed eventually
class SCR_DCOAISettingsComponent : ScriptComponent
{
	// rewrite if condition below if adding new lines!!!
	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Enable debug visualization")]
	protected bool m_EnableVisualization;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Allow movement")]
	bool m_EnableMovement;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Allow reacting on danger events")]
	bool m_EnableDangerEvents;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Allow reacting on perceived targets")]
	bool m_EnablePerception;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Allow shooting and attacking in general")]
	bool m_EnableAttack;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Allow finding and taking cover")]
	bool m_EnableTakeCover;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Allow aiming and gestures in general")]
	bool m_EnableLooking;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Allow sending messages")]
	bool m_EnableCommunication;

	[Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Allow artificial aiming error for AI")]
	bool m_EnableAimError;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Allow leader to stop when formation is deformed")]
	bool m_EnableLeaderStop;

	protected static SCR_DCOAISettingsComponent m_sInstance;

	//------------------------------------------------------------------------------------------------
	//! \return
	static SCR_DCOAISettingsComponent GetInstance()
	{
		return m_sInstance;
	}

	//------------------------------------------------------------------------------------------------
	//!
	void InitVisualization()
	{
		if (!m_EnableVisualization)
			return;
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
