class DCO_GlobalAIComponentClass: ScriptComponentClass
{
}

class DCO_GlobalAIComponent: ScriptComponent
{
	[Attribute("1", UIWidgets.Slider, "Global AI unit skill level", "0.1 10 0.1")]
	protected float unitAimSkillAccuracy;
	
	[Attribute( defvalue: "1", uiwidget: UIWidgets.Slider, desc: "Unit Perception", params: "0.5 3 0.01" )]
	protected float m_fAiPerception;
	
	static DCO_GlobalAIComponent m_sInstance;
	
	static DCO_GlobalAIComponent GetInstance()
	{
		return m_sInstance;
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
	}
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_sInstance = this;
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	float GetUnitSkill()
	{
		return unitAimSkillAccuracy;
	}
	
	float SetUnitSkill(float sk)
	{
		unitAimSkillAccuracy = sk;
		return sk;
	}
	
	float GetUnitPerception()
	{
		return m_fAiPerception;
	}
	
	float SetUnitPerception(float sk)
	{
		m_fAiPerception = sk;
		return sk;
	}
	
	ResourceName GetResourceNameMagazine()
	{
		ResourceName name;
		return name;
	}
}
