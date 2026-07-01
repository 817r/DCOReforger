[ComponentEditorProps(category: "GameScripted/AI", description: "Component for DCO AI system calculations")]
class DCO_AIConfigComponentClass : ScriptComponentClass
{
}

class DCO_AIConfigComponent : ScriptComponent
{
	[Attribute( defvalue: "1", uiwidget: UIWidgets.Slider, desc: "Unit skill", params: "0.1 10 0.01" )]
	float m_fAimAccuracy;
	
	[Attribute( defvalue: "30", uiwidget: UIWidgets.Slider, desc: "Unit skill", params: "1 60 0.01" )]
	float m_fTimeToMaxAccuracy;
	
	[Attribute( defvalue: "1", uiwidget: UIWidgets.Slider, desc: "Unit skill", params: "0.1 10 0.01" )]
	float m_fAiPerception;
	
	[Attribute( defvalue: "0", uiwidget: UIWidgets.Auto, desc: "Magical Ammo")]
	protected bool m_bIsMagicallyResupplied;
	
	[Attribute( defvalue: "500", uiwidget: UIWidgets.Slider, desc: "Unit Perception", params: "0 2000 0.01" )]
	protected float m_fVehicleDismountDanger;
	
	[Attribute("2", UIWidgets.ComboBox, "AI Custom skill in combat", "", ParamEnumArray.FromEnum(DCO_AISKILL) )]
	protected DCO_AISKILL m_eAISkillDefault;
	
	[Attribute("", UIWidgets.Auto, "Lock Player UID", "")]
	protected array<string> m_eLockPlayerUID;
	
	[Attribute( defvalue: "1", uiwidget: UIWidgets.Slider, desc: "How Suppression Affecting this AI", params: "0 2 0.01" )]
	protected float m_fSuppressionEffect;

	[Attribute("1", UIWidgets.ComboBox, "AI Personality -- gimana gaya combat AI ini, orthogonal dari skill", "", ParamEnumArray.FromEnum(DCO_EAIPersonality))]
	protected DCO_EAIPersonality m_ePersonality;

	protected bool m_bHoldPosition = false;

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
		m_bIsMagicallyResupplied = settings.GetUnitMagicMagazine();
		m_fTimeToMaxAccuracy = settings.GetAccuracyTime();
		m_eAISkillDefault = settings.GetAISkill();
		m_fVehicleDismountDanger = settings.GetDismountDistance();
		m_fSuppressionEffect = settings.GetSuppressionEffect();
		m_ePersonality = Math.RandomInt(DCO_EAIPersonality.CAUTIOUS, DCO_EAIPersonality.RECKLESS + 1);
		
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
	
	bool GetMagicMag()
	{
		return m_bIsMagicallyResupplied;
	}
	
	bool SetMagicMag(bool s)
	{
		m_bIsMagicallyResupplied = s;
		return m_bIsMagicallyResupplied;
	}

	float GetSuppressionEffect()
	{
		return m_fSuppressionEffect;
	}
	
	float SetSuppressionEffect(float f)
	{
		m_fSuppressionEffect = f;
		return m_fSuppressionEffect;
	}

	DCO_EAIPersonality GetPersonality()
	{
		return m_ePersonality;
	}
	
	DCO_EAIPersonality SetPersonality(DCO_EAIPersonality p)
	{
		m_ePersonality = p;
		return m_ePersonality;
	}

	bool IsHoldPosition()
	{
		return m_bHoldPosition;
	}
	
	bool SetHoldPosition(bool b)
	{
		m_bHoldPosition = b;
		return m_bHoldPosition;
	}

	bool ToggleHoldPosition()
	{
		m_bHoldPosition = !m_bHoldPosition;
		return m_bHoldPosition;
	}
}