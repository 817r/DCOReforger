class DCO_GlobalAIComponentClass: ScriptComponentClass
{
}

class DCO_GlobalAIComponent: ScriptComponent
{
	[Attribute("1", UIWidgets.Slider, "Global AI unit skill level", "0.1 10 0.1")]
	protected float unitAimSkillAccuracy;
	
	[Attribute( defvalue: "15", uiwidget: UIWidgets.Slider, desc: "Unit skill", params: "1 60 0.01" )]
	protected float m_fTimeToMaxAccuracy;
	
	[Attribute( defvalue: "1", uiwidget: UIWidgets.Slider, desc: "Unit Perception", params: "0.5 3 0.01" )]
	protected float m_fAiPerception;
	
	[Attribute( defvalue: "0", uiwidget: UIWidgets.Auto, desc: "Magical Ammo")]
	protected bool m_bIsMagicallyResupplied;
	
	[Attribute( defvalue: "500", uiwidget: UIWidgets.Slider, desc: "Unit Perception", params: "50 2000 0.01" )]
	protected float m_fVehicleDismountDanger;
	
	[Attribute("2", UIWidgets.ComboBox, "AI Custom skill in combat", "", ParamEnumArray.FromEnum(DCO_AISKILL) )]
	protected DCO_AISKILL m_eAISkillDefault;
	
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
	
	DCO_AISKILL SetAISkill(DCO_AISKILL ski)
	{
		m_eAISkillDefault = ski;
		return m_eAISkillDefault;
	}
	
	DCO_AISKILL GetAISkill()
	{
		return m_eAISkillDefault;
	}
	
	float GetDismountDistance()
	{
		return m_fVehicleDismountDanger;
	}
	
	float SetDismountDistance(float acc)
	{
		m_fVehicleDismountDanger = acc;
		return m_fVehicleDismountDanger;
	}
	
	float GetAccuracyTime()
	{
		return m_fTimeToMaxAccuracy;
	}
	
	float SetAccuracyTime(float acc)
	{
		m_fTimeToMaxAccuracy = acc;
		return m_fTimeToMaxAccuracy;
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
	
	bool GetUnitMagicMagazine()
	{
		return m_bIsMagicallyResupplied;
	}
	
	bool SetUnitMagicMagazine(bool s)
	{
		m_bIsMagicallyResupplied = s;
		return m_bIsMagicallyResupplied;
	}
}
