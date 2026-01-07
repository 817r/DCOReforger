[ComponentEditorProps(category: "GameScripted/AI", description: "Component for DCO AI system calculations")]
class DCO_AIConfigComponentClass : ScriptComponentClass
{
}

class DCO_AIConfigComponent : ScriptComponent
{
	[Attribute( defvalue: "1", uiwidget: UIWidgets.Slider, desc: "Unit skill", params: "0.1 10 0.01" )]
	float m_fAimAccuracy;
	
	[Attribute( defvalue: "1", uiwidget: UIWidgets.Slider, desc: "Unit skill", params: "0.1 10 0.01" )]
	float m_fAiPerception;
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		DCO_GlobalAIComponent settings = DCO_GlobalAIComponent.GetInstance();
		if (!settings)
			return;
		
		m_fAimAccuracy = settings.GetUnitSkill();
		m_fAiPerception = settings.GetUnitPerception();
	}
	
	float GetAccuracy()
	{
		return m_fAimAccuracy;
	}
	
	float SetAccuracy(float acc)
	{
		m_fAimAccuracy = acc;
		return acc;
	}
	
	float GetPerception()
	{
		return m_fAiPerception;
	}
	
	float SetPerception(float f)
	{
		m_fAiPerception = f;
		return m_fAiPerception;
	}
}
