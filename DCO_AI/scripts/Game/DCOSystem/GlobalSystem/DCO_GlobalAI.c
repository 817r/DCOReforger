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
	
	[Attribute("0.6", UIWidgets.Slider, "Peluang AI dodge (lari cari perlindungan) tiap kali denger tembakan. Di-scale personality kalau dodgeScaleByPersonality nyala.", params: "0 1 0.05")]
	protected float m_fDodgeChance;

	[Attribute("8.0", UIWidgets.Slider, "Cooldown (detik) sebelum AI yang sama boleh dodge lagi. Ini rem utamanya -- tanpa cooldown, full auto bikin dodgeChance gak ada artinya.", params: "0 120 0.5")]
	protected float m_fDodgeCooldown;

	[Attribute("250.0", UIWidgets.Slider, "Jarak maksimum (m) tembakan yang masih bisa memicu dodge.", params: "0 1000 5")]
	protected float m_fDodgeMaxDist;

	[Attribute("30.0", UIWidgets.Slider, "Jarak pencarian bangunan/cover saat dodge.", params: "5 100 1")]
	protected float m_fDodgeSearchDist;

	[Attribute("1", UIWidgets.CheckBox, "Skala peluang dodge pakai personality AI (CAUTIOUS naik, RECKLESS turun).")]
	protected bool m_bDodgeScaleByPersonality;
	
	[Attribute("1", UIWidgets.CheckBox, "Baca override config dari file JSON di folder profile server. Kalau OFF, nilai di atas dipake apa adanya.", category: "Server Config")]
	protected bool m_bUseServerConfigFile;
	
	[Attribute("$profile:DCO/DCO_GlobalConfig.json", UIWidgets.EditBox, "Path file JSON config. Prefix $profile: nunjuk ke folder -profile server.", category: "Server Config")]
	protected string m_sServerConfigPath;
	
	[Attribute("1", UIWidgets.CheckBox, "Kalau file JSON belum ada, otomatis bikin file berisi nilai Workbench sekarang (jadi template buat admin).", category: "Server Config")]
	protected bool m_bAutoGenerateConfigFile;

	protected static bool s_bConfigLoaded = false;
	
	static DCO_GlobalAIComponent m_sInstance;
	
	static DCO_GlobalAIComponent GetInstance()
	{
		if (m_sInstance && !s_bConfigLoaded)
		{
			s_bConfigLoaded = true;
			m_sInstance.LoadServerConfig();
		}
		
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
		s_bConfigLoaded = false;
		
		SetEventMask(owner, EntityEvent.INIT);
	}

	protected void LoadServerConfig()
	{
		if (!m_bUseServerConfigFile)
			return;
		
		if (!Replication.IsServer())
			return;
		
		if (m_sServerConfigPath.IsEmpty())
		{
			Print("[DCO][Config] Path config kosong, skip.", LogLevel.WARNING);
			return;
		}
		
		if (!FileIO.FileExists(m_sServerConfigPath))
		{
			PrintFormat("[DCO][Config] File %1 gak ketemu.", m_sServerConfigPath, level: LogLevel.NORMAL);
			
			if (m_bAutoGenerateConfigFile)
				WriteDefaultConfig();
			
			return;
		}
		
		SCR_JsonLoadContext ctx = new SCR_JsonLoadContext();
		if (!ctx.LoadFromFile(m_sServerConfigPath))
		{
			PrintFormat("[DCO][Config] GAGAL parse %1 -- kemungkinan JSON invalid (koma nyangkut / kurung kurang). Pake nilai Workbench.",
				m_sServerConfigPath, level: LogLevel.ERROR);
			return;
		}
		
		int applied = 0;
		
		float fTmp;
		bool  bTmp;
		int   iTmp;
		
		if (ctx.ReadValue("unitAimSkillAccuracy", fTmp))
		{
			unitAimSkillAccuracy = Math.Clamp(fTmp, 0.1, 10.0);
			applied++;
		}
		
		if (ctx.ReadValue("timeToMaxAccuracy", fTmp))
		{
			m_fTimeToMaxAccuracy = Math.Clamp(fTmp, 1.0, 60.0);
			applied++;
		}
		
		if (ctx.ReadValue("aiPerception", fTmp))
		{
			m_fAiPerception = Math.Clamp(fTmp, 0.5, 3.0);
			applied++;
		}
		
		if (ctx.ReadValue("vehicleDismountDanger", fTmp))
		{
			m_fVehicleDismountDanger = Math.Clamp(fTmp, 0.0, 2000.0);
			applied++;
		}
		
		if (ctx.ReadValue("suppressionEffect", fTmp))
		{
			m_fSuppressionEffect = Math.Clamp(fTmp, 0.0, 2.0);
			applied++;
		}
		
		if (ctx.ReadValue("takeCoverChance", fTmp))
		{
			m_fTakeCoverChance = Math.Clamp(fTmp, 0.0, 1.0);
			applied++;
		}
		
		if (ctx.ReadValue("magicallyResupplied", bTmp))
		{
			m_bIsMagicallyResupplied = bTmp;
			applied++;
		}

		if (ctx.ReadValue("aiSkillDefault", iTmp))
		{
			m_eAISkillDefault = Math.ClampInt(iTmp, 0, 6);
			applied++;
		}
		
		if (ctx.ReadValue("personalityWeightStandard", fTmp))
		{
			m_fPersonalityWeightStandard = Math.Clamp(fTmp, 0.0, 100.0);
			applied++;
		}
		
		if (ctx.ReadValue("personalityWeightCautious", fTmp))
		{
			m_fPersonalityWeightCautious = Math.Clamp(fTmp, 0.0, 100.0);
			applied++;
		}
		
		if (ctx.ReadValue("personalityWeightAggressive", fTmp))
		{
			m_fPersonalityWeightAggressive = Math.Clamp(fTmp, 0.0, 100.0);
			applied++;
		}
		
		if (ctx.ReadValue("personalityWeightReckless", fTmp))
		{
			m_fPersonalityWeightReckless = Math.Clamp(fTmp, 0.0, 100.0);
			applied++;
		}
		
		if (ctx.ReadValue("dodgeChance", fTmp))
		{
			m_fDodgeChance = Math.Clamp(fTmp, 0.0, 1.0);
			applied++;
		}
			
		if (ctx.ReadValue("dodgeCooldown", fTmp))
		{
			m_fDodgeCooldown = Math.Clamp(fTmp, 0.0, 120.0);
			applied++;
		}
			
		if (ctx.ReadValue("dodgeMaxDist", fTmp))
		{
			m_fDodgeMaxDist = Math.Clamp(fTmp, 0.0, 1000.0);
			applied++;
		}
			
		if (ctx.ReadValue("dodgeSearchDist", fTmp))
		{
			m_fDodgeSearchDist = Math.Clamp(fTmp, 5.0, 100.0);
			applied++;
		}
			
		if (ctx.ReadValue("dodgeScaleByPersonality", bTmp))
		{
			m_bDodgeScaleByPersonality = bTmp;
			applied++;
		}
		
		PrintFormat("[DCO][Config] %1 setting di-override dari %2", applied, m_sServerConfigPath);
		DumpActiveConfig();
	}

	protected void WriteDefaultConfig()
	{
		string dir = GetDirectoryFromPath(m_sServerConfigPath);
		if (!dir.IsEmpty())
			FileIO.MakeDirectory(dir);
		
		SCR_JsonSaveContext ctx = new SCR_JsonSaveContext();
		
		ctx.WriteValue("_comment", "DCO Global AI config. Hapus baris yang gak mau di-override -- yang gak ada di file ini otomatis pake nilai dari Workbench.");
		
		ctx.WriteValue("unitAimSkillAccuracy",       unitAimSkillAccuracy);
		ctx.WriteValue("timeToMaxAccuracy",          m_fTimeToMaxAccuracy);
		ctx.WriteValue("aiPerception",               m_fAiPerception);
		ctx.WriteValue("vehicleDismountDanger",      m_fVehicleDismountDanger);
		ctx.WriteValue("suppressionEffect",          m_fSuppressionEffect);
		ctx.WriteValue("takeCoverChance",            m_fTakeCoverChance);
		ctx.WriteValue("magicallyResupplied",        m_bIsMagicallyResupplied);
		ctx.WriteValue("aiSkillDefault",             m_eAISkillDefault);
		ctx.WriteValue("personalityWeightStandard",  m_fPersonalityWeightStandard);
		ctx.WriteValue("personalityWeightCautious",  m_fPersonalityWeightCautious);
		ctx.WriteValue("personalityWeightAggressive", m_fPersonalityWeightAggressive);
		ctx.WriteValue("personalityWeightReckless",  m_fPersonalityWeightReckless);
		ctx.WriteValue("dodgeChance",              m_fDodgeChance);
		ctx.WriteValue("dodgeCooldown",            m_fDodgeCooldown);
		ctx.WriteValue("dodgeMaxDist",             m_fDodgeMaxDist);
		ctx.WriteValue("dodgeSearchDist",          m_fDodgeSearchDist);
		ctx.WriteValue("dodgeScaleByPersonality",  m_bDodgeScaleByPersonality);
		
		if (ctx.SaveToFile(m_sServerConfigPath))
			PrintFormat("[DCO][Config] Template config dibuat di %1", m_sServerConfigPath);
		else
			PrintFormat("[DCO][Config] GAGAL nulis %1 -- cek permission folder profile.", m_sServerConfigPath, level: LogLevel.ERROR);
	}
	
	protected string GetDirectoryFromPath(string path)
	{
		int lastSlash = -1;
		int len = path.Length();
		
		for (int i = 0; i < len; i++)
		{
			string ch = path.Get(i);
			if (ch == "/" || ch == "\\")
				lastSlash = i;
		}
		
		if (lastSlash <= 0)
			return string.Empty;
		
		return path.Substring(0, lastSlash);
	}
	
	void DumpActiveConfig()
	{
		PrintFormat("[DCO][Config] aim=%1 timeToMax=%2 perception=%3 dismount=%4",
			unitAimSkillAccuracy, m_fTimeToMaxAccuracy, m_fAiPerception, m_fVehicleDismountDanger);
		PrintFormat("[DCO][Config] suppression=%1 takeCover=%2 magicAmmo=%3 skill=%4",
			m_fSuppressionEffect, m_fTakeCoverChance, m_bIsMagicallyResupplied, typename.EnumToString(DCO_AISKILL, m_eAISkillDefault));
		PrintFormat("[DCO][Config] personality STD=%1 CAU=%2 AGR=%3 RCK=%4",
			m_fPersonalityWeightStandard, m_fPersonalityWeightCautious,
			m_fPersonalityWeightAggressive, m_fPersonalityWeightReckless);
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
	
	void ReloadServerConfig()
	{
		if (!Replication.IsServer())
			return;
		
		s_bConfigLoaded = true;
		LoadServerConfig();
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
	
	float GetSuppressionEffect()
	{
		return m_fSuppressionEffect;
	}
	
	float SetSuppressionEffect(float f)
	{
		m_fSuppressionEffect = f;
		return m_fSuppressionEffect;
	}

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
			return DCO_EAIPersonality.STANDARD;
		
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
}