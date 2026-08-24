class DCO_GlobalAIComponentClass: ScriptComponentClass
{
}

class DCO_GlobalAIComponent: ScriptComponent
{
	[Attribute("1.2", UIWidgets.Slider, "Global AI unit skill level", "0.1 10 0.1")]
	protected float unitAimSkillAccuracy;
	
	[Attribute( defvalue: "15", uiwidget: UIWidgets.Slider, desc: "Unit skill", params: "1 60 0.01" )]
	protected float m_fTimeToMaxAccuracy;
	
	[Attribute( defvalue: "1", uiwidget: UIWidgets.Slider, desc: "Unit Perception", params: "0.5 3 0.01" )]
	protected float m_fAiPerception;
	
	[Attribute( defvalue: "0", uiwidget: UIWidgets.Auto, desc: "Magical Ammo")]
	protected bool m_bIsMagicallyResupplied;
	
	[Attribute( defvalue: "700", uiwidget: UIWidgets.Slider, desc: "Unit Perception", params: "0 2000 0.01" )]
	protected float m_fVehicleDismountDanger;
	
	[Attribute("2", UIWidgets.ComboBox, "AI Custom skill in combat", "", ParamEnumArray.FromEnum(DCO_AISKILL) )]
	protected DCO_AISKILL m_eAISkillDefault;
	
	[Attribute( defvalue: "1", uiwidget: UIWidgets.Slider, desc: "How Suppression Affecting this AI", params: "0 2 0.01" )]
	protected float m_fSuppressionEffect;
	
	[Attribute("0.7", UIWidgets.Range, "Base chance AI mau aktif nyari cover pas ke-detect di tempat terbuka (0-1). Di-scale lebih lanjut sama personality -- RECKLESS turun paling banyak (ceroboh), AGGRESSIVE turun sedang (combat-oriented tapi disiplin), CAUTIOUS naik.", params: "0 1 0.01" )]
	protected float m_fTakeCoverChance;
	
	[Attribute("70", UIWidgets.Slider, "Bobot personality STANDARD (relatif ke 3 lainnya, gak harus total 100)", params: "0 100 1")]
	protected float m_fPersonalityWeightStandard;
	
	[Attribute("15", UIWidgets.Slider, "Bobot personality CAUTIOUS", params: "0 100 1")]
	protected float m_fPersonalityWeightCautious;
	
	[Attribute("12", UIWidgets.Slider, "Bobot personality AGGRESSIVE", params: "0 100 1")]
	protected float m_fPersonalityWeightAggressive;
	
	[Attribute("3", UIWidgets.Slider, "Bobot personality RECKLESS", params: "0 100 1")]
	protected float m_fPersonalityWeightReckless;
	
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
	
	// === ADDED: getter/setter buat m_fSuppressionEffect -- fieldnya udah ada dari
	// awal tapi belum ada getter/setternya, jadi gak bisa dibaca/ditulis dari luar.
	float GetSuppressionEffect()
	{
		return m_fSuppressionEffect;
	}
	
	float SetSuppressionEffect(float f)
	{
		m_fSuppressionEffect = f;
		return m_fSuppressionEffect;
	}
	// === END ADDED ===
	
	// === ADDED: Take Cover Chance ===
	float GetTakeCoverChance()
	{
		return m_fTakeCoverChance;
	}
	
	float SetTakeCoverChance(float f)
	{
		m_fTakeCoverChance = f;
		return m_fTakeCoverChance;
	}

	DCO_EAIPersonality RollWeightedPersonality()
	{
		float total = m_fPersonalityWeightStandard + m_fPersonalityWeightCautious
			+ m_fPersonalityWeightAggressive + m_fPersonalityWeightReckless;
		
		if (total <= 0.0)
			return DCO_EAIPersonality.STANDARD; // safety -- semua bobot 0, jangan crash
		
		float roll = Math.RandomFloat(0.0, total);
		
		if (roll < m_fPersonalityWeightStandard)
			return DCO_EAIPersonality.STANDARD;
		roll -= m_fPersonalityWeightStandard;
		
		if (roll < m_fPersonalityWeightCautious)
			return DCO_EAIPersonality.CAUTIOUS;
		roll -= m_fPersonalityWeightCautious;
		
		if (roll < m_fPersonalityWeightAggressive)
			return DCO_EAIPersonality.AGGRESSIVE;
		
		return DCO_EAIPersonality.RECKLESS;
	}
	// === END MODIFIED ===
}