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
	
	[Attribute("0.7", UIWidgets.Range, "Base chance AI mau aktif nyari cover pas ke-detect di tempat terbuka (0-1)", params: "0 1 0.01" )]
	protected float m_fTakeCoverChance;

	[Attribute("0.6", UIWidgets.Range, "Peluang AI ini dodge tiap kali denger tembakan (0-1)", params: "0 1 0.05")]
	protected float m_fDodgeChance;
	
	[Attribute("8.0", UIWidgets.Slider, "Cooldown (detik) sebelum AI ini boleh dodge lagi", params: "0 120 0.5")]
	protected float m_fDodgeCooldown;
	
	[Attribute("250.0", UIWidgets.Slider, "Jarak maksimum (m) tembakan yang masih memicu dodge", params: "0 1000 5")]
	protected float m_fDodgeMaxDist;
	
	[Attribute("30.0", UIWidgets.Slider, "Jarak pencarian bangunan/cover saat dodge", params: "5 100 1")]
	protected float m_fDodgeSearchDist;
	
	[Attribute("1", UIWidgets.CheckBox, "Skala peluang dodge pakai personality AI ini")]
	protected bool m_bDodgeScaleByPersonality;
	
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
		m_fTakeCoverChance = settings.GetTakeCoverChance();
		m_ePersonality = settings.RollWeightedPersonality();
		m_fDodgeChance             = settings.GetDodgeChance();
		m_fDodgeCooldown           = settings.GetDodgeCooldown();
		m_fDodgeMaxDist            = settings.GetDodgeMaxDist();
		m_fDodgeSearchDist         = settings.GetDodgeSearchDist();
		m_bDodgeScaleByPersonality = settings.GetDodgeScaleByPersonality();
	}
	
	float GetDodgeChance()            
	{ 
		return m_fDodgeChance; 
	}
	
	float SetDodgeChance(float f)     
	{ 
		m_fDodgeChance = f; 
		return m_fDodgeChance; 
	}
	
	float GetDodgeCooldown()          
	{ 
		return m_fDodgeCooldown; 
	}
	
	float SetDodgeCooldown(float f)   
	{ 
		m_fDodgeCooldown = f; 
		return m_fDodgeCooldown; 
	}
	
	float GetDodgeMaxDist()           
	{ 
		return m_fDodgeMaxDist; 
	}
	
	float SetDodgeMaxDist(float f)    
	{ 
		m_fDodgeMaxDist = f; 
		return m_fDodgeMaxDist; 
	}
	
	float GetDodgeSearchDist()        
	{ 
		return m_fDodgeSearchDist; 
	}
	
	float SetDodgeSearchDist(float f) 
	{ 
		m_fDodgeSearchDist = f; 
		return m_fDodgeSearchDist; 
	}
	
	bool GetDodgeScaleByPersonality()       
	{ 
		return m_bDodgeScaleByPersonality; 
	}
	
	bool SetDodgeScaleByPersonality(bool b) 
	{ 
		m_bDodgeScaleByPersonality = b; 
		return m_bDodgeScaleByPersonality; 
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
	
	// === ADDED: getter/setter buat m_fSuppressionEffect -- fieldnya udah ada dari
	// awal tapi belum ada getter/setternya.
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
	// === END ADDED ===
	
	// === ADDED: Personality System ===
	DCO_EAIPersonality GetPersonality()
	{
		return m_ePersonality;
	}
	
	DCO_EAIPersonality SetPersonality(DCO_EAIPersonality p)
	{
		m_ePersonality = p;
		return m_ePersonality;
	}
	// === END ADDED ===
	
	// === ADDED: Hold Position ===
	bool IsHoldPosition()
	{
		return m_bHoldPosition;
	}
	
	bool SetHoldPosition(bool b)
	{
		m_bHoldPosition = b;
		return m_bHoldPosition;
	}
	
	//! Dipanggil dari trigger UI-nya (context menu/radial command/dsb). Return nilai
	//! BARU setelah di-toggle, biar UI-nya bisa langsung tau mau nampilin "Hold
	//! Position" atau "Unhold Position" abis ini.
	bool ToggleHoldPosition()
	{
		m_bHoldPosition = !m_bHoldPosition;
		return m_bHoldPosition;
	}
	// === END ADDED ===
}