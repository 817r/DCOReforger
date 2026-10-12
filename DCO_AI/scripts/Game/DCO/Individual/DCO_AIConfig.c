[ComponentEditorProps(category: "GameScripted/AI", description: "Component for DCO AI system calculations")]
class DCO_AIConfigComponentClass : ScriptComponentClass
{
}

class DCO_AIConfigComponent : ScriptComponent
{
	[Attribute( defvalue: "1", uiwidget: UIWidgets.Slider, desc: "Unit skill", params: "0.1 10 0.01" )]
	float m_fAimAccuracy;

	[Attribute( defvalue: "1", uiwidget: UIWidgets.Slider, desc: "Aim Speed: divides aim settle time", params: "0.25 4 0.01" )]
	float m_fAimSpeed;

	[Attribute( defvalue: "1", uiwidget: UIWidgets.Slider, desc: "Aim Shake: reduces posture and fatigue sway", params: "0.25 4 0.01" )]
	float m_fAimShake;

	[Attribute( defvalue: "1", uiwidget: UIWidgets.Slider, desc: "Unit skill", params: "0.1 10 0.01" )]
	float m_fAiPerception;

	[Attribute( defvalue: "0", uiwidget: UIWidgets.Auto, desc: "Magical Ammo")]
	protected bool m_bIsMagicallyResupplied;

	[Attribute( defvalue: "500", uiwidget: UIWidgets.Slider, desc: "Unit Perception", params: "0 2000 0.01" )]
	protected float m_fVehicleDismountDanger;
	protected DCO_EDisembarkRule m_eDisembarkRule;

	[Attribute("2", UIWidgets.ComboBox, "AI Custom skill in combat", "", ParamEnumArray.FromEnum(DCO_AISKILL) )]
	protected DCO_AISKILL m_eAISkillDefault;

	[Attribute("", UIWidgets.Auto, "Lock Player UID", "")]
	protected array<string> m_eLockPlayerUID;

	[Attribute( defvalue: "1", uiwidget: UIWidgets.Slider, desc: "How Suppression Affecting this AI", params: "0 2 0.01" )]
	protected float m_fSuppressionEffect;

	[Attribute("0.7", UIWidgets.Range, "Base chance (0-1) that AI actively seek cover when detected in the open.", params: "0 1 0.01" )]
	protected float m_fTakeCoverChance;

	[Attribute("0.6", UIWidgets.Range, "Chance (0-1) this AI dodges each time it hears gunfire.", params: "0 1 0.05")]
	protected float m_fDodgeChance;

	[Attribute("8.0", UIWidgets.Slider, "Cooldown (s) before this AI may dodge again.", params: "0 120 0.5")]
	protected float m_fDodgeCooldown;

	[Attribute("250.0", UIWidgets.Slider, "Maximum distance (m) of gunfire that still triggers a dodge.", params: "0 1000 5")]
	protected float m_fDodgeMaxDist;

	[Attribute("30.0", UIWidgets.Slider, "Building or cover search distance when dodging.", params: "5 100 1")]
	protected float m_fDodgeSearchDist;

	[Attribute("1", UIWidgets.CheckBox, "Scale the dodge chance by this AI's personality.")]
	protected bool m_bDodgeScaleByPersonality;

	[Attribute("1", UIWidgets.Slider, "Number of threatening enemy shots before this AI dodges. 1 = on the first shot.", params: "1 20 1")]
	protected int m_iDodgeShotThreshold;

	[Attribute("5.0", UIWidgets.Slider, "Shot counting window (s) for this AI. No new shot within this time -> the count resets to 0.", params: "1 30 0.5")]
	protected float m_fDodgeShotWindow;

	[Attribute("1", UIWidgets.ComboBox, "AI Personality: this AI's combat style, separate from skill.", "", ParamEnumArray.FromEnum(DCO_EAIPersonality))]
	protected DCO_EAIPersonality m_ePersonality;

	[Attribute("0.5", UIWidgets.Slider, "How often this AI throws frag grenades. 0 = never, 0.5 = default, 1 = very often.", params: "0 1 0.01")]
	protected float m_fGrenadeUsage;

	[Attribute("0.5", UIWidgets.Slider, "How often this AI uses a grenade launcher (UGL). 0 = never, 0.5 = default, 1 = very often.", params: "0 1 0.01")]
	protected float m_fGLUsage;

	[Attribute("1", UIWidgets.Slider, "This AI's grenade launcher (UGL) accuracy. Higher = tighter impact spread. 1 = default.", params: "0.1 5 0.1")]
	protected float m_fGLAccuracy;

	[Attribute("0.5", UIWidgets.Slider, "How often this AI throws smoke to cover movement. 0 = never, 0.5 = default, 1 = very often.", params: "0 1 0.01")]
	protected float m_fSmokeUsage;

	protected bool m_bHoldPosition = false;
	protected float m_fDCOAggrApplied = 1.0;
	protected int m_iDCOVer = -1;
	protected DCO_GlobalAIComponent m_DCOSet;
	protected float m_fDCOGrenRaw = -1;
	protected float m_fDCOGLRaw = -1;

	static float UsageMap(float raw, float aggr)
	{
		return Math.Clamp(1.0 - Math.Pow(Math.Clamp(1.0 - raw, 0, 1), Math.Max(aggr, 0.1)), 0, 1);
	}

	static float Calm(float aggr)
	{
		if (aggr > 1.0)
			return 1.0 / aggr;
		return Math.Clamp(2.0 - aggr, 0, 2);
	}

	void DCO_CheckVer()
	{
		if (!m_DCOSet || DCO_GlobalAIComponent.DCO_TuneVerAll() == m_iDCOVer)
			return;
		m_iDCOVer = DCO_GlobalAIComponent.DCO_TuneVerAll();
		float na = m_DCOSet.DCO_Aggression();
		float old = m_fDCOAggrApplied;
		if (na == old || old <= 0 || na <= 0)
			return;
		float r = Calm(na) / Math.Max(Calm(old), 0.01);
		float g = na / old;
		m_fSuppressionEffect = m_fSuppressionEffect * r;
		m_fTakeCoverChance = Math.Clamp(m_fTakeCoverChance * r, 0, 1);
		m_fDodgeChance = Math.Clamp(m_fDodgeChance * r, 0, 1);
		if (m_fDCOGrenRaw >= 0)
		{
			m_fGrenadeUsage = UsageMap(m_fDCOGrenRaw, na);
			m_fGLUsage = UsageMap(m_fDCOGLRaw, na);
		}
		else
		{
			m_fGrenadeUsage = Math.Clamp(m_fGrenadeUsage * g, 0, 1);
			m_fGLUsage = Math.Clamp(m_fGLUsage * g, 0, 1);
		}
		m_fDCOAggrApplied = na;
		DCO_Perf.Count("agg:unit_reapply");
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, "-", string.Format("aggr_unit_reapply unit=%1 old=%2 new=%3 supp=%4 cover=%5 dodge=%6 frag=%7", DCO_Log.IdUnit(GetOwner()), old.ToString(-1, 2), na.ToString(-1, 2), m_fSuppressionEffect.ToString(-1, 2), m_fTakeCoverChance.ToString(-1, 2), m_fDodgeChance.ToString(-1, 2), m_fGrenadeUsage.ToString(-1, 2)));
	}

	float DCO_TakeCoverRaw()
	{
		return Math.Clamp(m_fTakeCoverChance / Math.Max(Calm(m_fDCOAggrApplied), 0.01), 0, 1);
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		DCO_GlobalAIComponent settings = DCO_GlobalAIComponent.GetFor(owner);
		if (!settings)
			return;

		m_fAimAccuracy = settings.GetUnitSkill();
		m_fAiPerception = settings.GetUnitPerception();
		m_bIsMagicallyResupplied = settings.GetUnitMagicMagazine();
		m_fAimSpeed = settings.GetAimSpeed();
		m_fAimShake = settings.GetAimShake();
		m_eAISkillDefault = settings.GetAISkill();
		m_fVehicleDismountDanger = settings.GetDismountDistance();
		m_eDisembarkRule = settings.GetDisembarkRule();
		m_fSuppressionEffect = settings.GetSuppressionEffect();
		m_fTakeCoverChance = settings.GetTakeCoverChance();
		m_ePersonality = settings.RollWeightedPersonality();
		m_fDodgeChance             = settings.GetDodgeChance();
		m_fDodgeCooldown           = settings.GetDodgeCooldown();
		m_fDodgeMaxDist            = settings.GetDodgeMaxDist();
		m_fDodgeSearchDist         = settings.GetDodgeSearchDist();
		m_bDodgeScaleByPersonality = settings.GetDodgeScaleByPersonality();
		m_fGrenadeUsage            = settings.GetGrenadeUsage();
		m_fGLUsage                 = settings.GetGLUsage();
		float aggr = settings.DCO_Aggression();
		m_DCOSet = settings;
		m_fDCOAggrApplied = aggr;
		m_iDCOVer = DCO_GlobalAIComponent.DCO_TuneVerAll();
		if (aggr != 1)
		{
			float calm = Math.Clamp(2.0 - aggr, 0, 2);
			if (aggr > 1)
				calm = 1.0 / aggr;
			m_fSuppressionEffect = m_fSuppressionEffect * calm;
			m_fTakeCoverChance = Math.Clamp(m_fTakeCoverChance * calm, 0, 1);
			m_fDodgeChance = Math.Clamp(m_fDodgeChance * calm, 0, 1);
			if (settings.DCO_TuneF("agg.usage_map", 1.0) > 0)
			{
				m_fDCOGrenRaw = m_fGrenadeUsage;
				m_fDCOGLRaw = m_fGLUsage;
				m_fGrenadeUsage = UsageMap(m_fDCOGrenRaw, aggr);
				m_fGLUsage = UsageMap(m_fDCOGLRaw, aggr);
				DCO_Perf.Count("agg:usage_map");
			}
			else
			{
				m_fGrenadeUsage = Math.Clamp(m_fGrenadeUsage * aggr, 0, 1);
				m_fGLUsage = Math.Clamp(m_fGLUsage * aggr, 0, 1);
			}
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, "-", string.Format("aggr_unit unit=%1 aggr=%2 supp=%3 cover=%4 dodge=%5 frag=%6 gl=%7", DCO_Log.IdUnit(owner), aggr.ToString(-1, 2), m_fSuppressionEffect.ToString(-1, 2), m_fTakeCoverChance.ToString(-1, 2), m_fDodgeChance.ToString(-1, 2), m_fGrenadeUsage.ToString(-1, 2), m_fGLUsage.ToString(-1, 2)));
		}
		m_fGLAccuracy              = settings.GetGLAccuracy();
		m_fSmokeUsage              = settings.GetSmokeUsage();
		m_iDodgeShotThreshold      = settings.GetDodgeShotThreshold();
		m_fDodgeShotWindow         = settings.GetDodgeShotWindow();
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

	DCO_EDisembarkRule GetDisembarkRule()
	{
		return m_eDisembarkRule;
	}

	void SetDisembarkRule(int rule)
	{
		m_eDisembarkRule = Math.ClampInt(rule, DCO_EDisembarkRule.AUTO, DCO_EDisembarkRule.NEVER);
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

	float GetAimSpeed()
	{
		return m_fAimSpeed;
	}

	void SetAimSpeed(float v)
	{
		m_fAimSpeed = Math.Clamp(v, 0.25, 4.0);
		LogAimCfg();
	}

	float GetAimShake()
	{
		return m_fAimShake;
	}

	void SetAimShake(float v)
	{
		m_fAimShake = Math.Clamp(v, 0.25, 4.0);
		LogAimCfg();
	}

	float GetAccuracy()
	{
		return m_fAimAccuracy;
	}

	float SetAccuracy(float acc)
	{
		m_fAimAccuracy = acc;
		LogAimCfg();
		return acc;
	}

	protected void LogAimCfg()
	{
		if (!DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
			return;
		IEntity unit;
		AIAgent agent = AIAgent.Cast(GetOwner());
		if (agent)
			unit = agent.GetControlledEntity();
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(unit), string.Format("aim_cfg scope=unit accuracy=%1 eff=%4 shake=%2 speed=%3 model=angle", m_fAimAccuracy, m_fAimShake, m_fAimSpeed, m_fAimAccuracy / 4.6));
	}

	protected bool m_bDCOPercOwn;

	float GetPerception()
	{
		if (!m_bDCOPercOwn)
		{
			DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetFor(GetOwner());
			if (g)
				return g.GetUnitPerception();
		}
		return m_fAiPerception;
	}

	float SetPerception(float f)
	{
		m_fAiPerception = f;
		m_bDCOPercOwn = true;
		DCO_Perf.Count("pc:unit_set");
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(GetOwner()), string.Format("perc_cfg_set scope=unit value=%1 (this soldier now ignores the global slider)", f.ToString(-1, 2)));
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

	float GetTakeCoverChance()
	{
		return m_fTakeCoverChance;
	}

	float SetTakeCoverChance(float f)
	{
		m_fTakeCoverChance = f;
		return m_fTakeCoverChance;
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

	float GetGrenadeUsage()
	{
		return m_fGrenadeUsage;
	}

	float SetGrenadeUsage(float f)
	{
		m_fGrenadeUsage = Math.Clamp(f, 0.0, 1.0);
		return m_fGrenadeUsage;
	}

	float GetGLUsage()
	{
		return m_fGLUsage;
	}

	float SetGLUsage(float f)
	{
		m_fGLUsage = Math.Clamp(f, 0.0, 1.0);
		return m_fGLUsage;
	}

	float GetGLAccuracy()
	{
		return m_fGLAccuracy;
	}

	float SetGLAccuracy(float f)
	{
		m_fGLAccuracy = Math.Clamp(f, 0.1, 5.0);
		return m_fGLAccuracy;
	}

	float GetSmokeUsage()
	{
		return m_fSmokeUsage;
	}

	float SetSmokeUsage(float f)
	{
		m_fSmokeUsage = Math.Clamp(f, 0.0, 1.0);
		return m_fSmokeUsage;
	}

	int GetDodgeShotThreshold()
	{
		return m_iDodgeShotThreshold;
	}

	int SetDodgeShotThreshold(int i)
	{
		m_iDodgeShotThreshold = Math.ClampInt(i, 1, 20);
		return m_iDodgeShotThreshold;
	}

	float GetDodgeShotWindow()
	{
		return m_fDodgeShotWindow;
	}

	float SetDodgeShotWindow(float f)
	{
		m_fDodgeShotWindow = Math.Clamp(f, 1.0, 30.0);
		return m_fDodgeShotWindow;
	}

	static float UsageToChanceScale(float usage)
	{
		return Math.Clamp(usage, 0.0, 1.0) * 2.0;
	}

	static DCO_AIConfigComponent Of(SCR_AIUtilityComponent u)
	{
		if (!u)
			return null;
		if (u.m_DCOConfig)
			u.m_DCOConfig.DCO_CheckVer();
		return u.m_DCOConfig;
	}

	static float BoundChanceOf(SCR_AIUtilityComponent u)
	{
		float raw = 0.8;
		DCO_AIConfigComponent c = Of(u);
		if (c)
			raw = c.DCO_TakeCoverRaw();
		else
		{
			DCO_GlobalAIComponent g0 = DCO_GlobalAIComponent.GetForUtility(u);
			if (g0)
				raw = g0.GetTakeCoverChance();
		}
		float pers = 1.0;
		switch (DCO_PersonalityCombatUtility.GetPersonalitySafe(u))
		{
			case DCO_EAIPersonality.CAUTIOUS: pers = 0.6; break;
			case DCO_EAIPersonality.AGGRESSIVE: pers = 1.3; break;
			case DCO_EAIPersonality.RECKLESS: pers = 1.5; break;
		}
		float a = 1.0;
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(u);
		if (g)
			a = Math.Clamp(g.DCO_AggressionSlider(), 0.5, 4.0);
		float mul = Math.Min(0.6 + 0.4 * a, 2.0);
		float chance = Math.Clamp(raw * pers * mul, 0, 0.95);
		if ((DCO_Roles.Get(u) & 1) != 0)
		{
			chance = chance * 0.5;
			DCO_Perf.Count("agg:bound_mg_half");
		}
		DCO_Perf.Count("agg:bound_bid");
		return chance;
	}

	static float PerceptionOf(SCR_AIUtilityComponent u)
	{
		DCO_AIConfigComponent c = Of(u);
		if (c)
			return c.GetPerception();
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(u);
		if (g)
			return g.GetUnitPerception();
		return 1.5;
	}

	static float AccuracyOf(SCR_AIUtilityComponent u)
	{
		DCO_AIConfigComponent c = Of(u);
		if (c)
			return c.GetAccuracy() / 4.6;
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(u);
		if (g)
			return g.GetUnitSkill() / 4.6;
		return 1.0 / 4.6;
	}

	static float AimSpeedOf(SCR_AIUtilityComponent u)
	{
		DCO_AIConfigComponent c = Of(u);
		if (c)
			return c.GetAimSpeed();
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(u);
		if (g)
			return g.GetAimSpeed();
		return 1.0;
	}

	static float AimShakeOf(SCR_AIUtilityComponent u)
	{
		DCO_AIConfigComponent c = Of(u);
		if (c)
			return c.GetAimShake();
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(u);
		if (g)
			return g.GetAimShake();
		return 1.0;
	}

	static DCO_AISKILL SkillOf(SCR_AIUtilityComponent u)
	{
		DCO_AIConfigComponent c = Of(u);
		if (c)
			return c.GetAISkill();
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(u);
		if (g)
			return g.GetAISkill();
		return DCO_AISKILL.REGULAR;
	}

	static bool MagicMagOf(SCR_AIUtilityComponent u)
	{
		DCO_AIConfigComponent c = Of(u);
		if (c)
			return c.GetMagicMag();
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(u);
		if (g)
			return g.GetUnitMagicMagazine();
		return false;
	}

	static DCO_EAIPersonality PersonalityOf(SCR_AIUtilityComponent u)
	{
		DCO_AIConfigComponent c = Of(u);
		if (c)
			return c.GetPersonality();
		return DCO_EAIPersonality.STANDARD;
	}

	static float SuppressionOf(SCR_AIUtilityComponent u)
	{
		DCO_AIConfigComponent c = Of(u);
		if (c)
			return c.GetSuppressionEffect();
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(u);
		if (g)
			return g.GetSuppressionEffect();
		return 1;
	}

	static float TakeCoverOf(SCR_AIUtilityComponent u)
	{
		DCO_AIConfigComponent c = Of(u);
		if (c)
			return c.GetTakeCoverChance();
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(u);
		if (g)
			return g.GetTakeCoverChance();
		return 0.9;
	}

	static float GrenadeUsageOf(SCR_AIUtilityComponent u)
	{
		DCO_AIConfigComponent c = Of(u);
		if (c)
			return c.GetGrenadeUsage();
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(u);
		if (g)
			return g.GetGrenadeUsage();
		return 0.5;
	}

	static float GLUsageOf(SCR_AIUtilityComponent u)
	{
		DCO_AIConfigComponent c = Of(u);
		if (c)
			return c.GetGLUsage();
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(u);
		if (g)
			return g.GetGLUsage();
		return 0.7;
	}

	static float GLAccuracyOf(SCR_AIUtilityComponent u)
	{
		DCO_AIConfigComponent c = Of(u);
		if (c)
			return c.GetGLAccuracy();
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(u);
		if (g)
			return g.GetGLAccuracy();
		return 1;
	}

	static float SmokeUsageOf(SCR_AIUtilityComponent u)
	{
		DCO_AIConfigComponent c = Of(u);
		if (c)
			return c.GetSmokeUsage();
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(u);
		if (g)
			return g.GetSmokeUsage();
		return 0.5;
	}

	static float DodgeSearchDistOf(SCR_AIUtilityComponent u)
	{
		DCO_AIConfigComponent c = Of(u);
		if (c)
			return c.GetDodgeSearchDist();
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(u);
		if (g)
			return g.GetDodgeSearchDist();
		return 30;
	}

	static int VehicleExitOf(SCR_AIUtilityComponent u)
	{
		if (u && u.GetOwner() && u.GetOwner().GetParentGroup())
		{
			DCO_GroupUtilityComponent gc = DCO_GroupUtilityComponent.Cast(u.GetOwner().GetParentGroup().FindComponent(DCO_GroupUtilityComponent));
			if (gc && gc.DCO_VehicleExit() >= 0)
				return gc.DCO_VehicleExit();
		}
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(u);
		if (g)
			return g.GetVehicleExit();
		return 15;
	}

	static DCO_EDisembarkRule DisembarkRuleOf(SCR_AIUtilityComponent u)
	{
		DCO_AIConfigComponent c = Of(u);
		if (c)
			return c.GetDisembarkRule();
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(u);
		if (g)
			return g.GetDisembarkRule();
		return DCO_EDisembarkRule.AUTO;
	}

	static float DismountOf(SCR_AIUtilityComponent u)
	{
		DCO_AIConfigComponent c = Of(u);
		if (c)
			return c.GetDismountDistance();
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(u);
		if (g)
			return g.GetDismountDistance();
		return 700;
	}

	bool IsHoldPosition()
	{
		return m_bHoldPosition;
	}

	bool SetHoldPosition(bool b)
	{
		m_bHoldPosition = b;
		DCO_ApplyHoldLock();
		return m_bHoldPosition;
	}

	protected void DCO_ApplyHoldLock()
	{
		AIAgent agent = AIAgent.Cast(GetOwner());
		if (!agent || !agent.GetControlledEntity())
			return;

		IEntity character = agent.GetControlledEntity();
		CharacterControllerComponent controller = CharacterControllerComponent.Cast(character.FindComponent(CharacterControllerComponent));
		if (controller)
			controller.SetDisableMovementControls(m_bHoldPosition);

		DCO_VehicleCombat.SetHandBrakeIfDriver(character, m_bHoldPosition);

		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(character.FindComponent(SCR_EditableEntityComponent));
		if (editable)
			editable.DCO_SetHoldPosition(m_bHoldPosition);
	}

	bool ToggleHoldPosition()
	{
		return SetHoldPosition(!m_bHoldPosition);
	}

	protected DCO_ELeashType m_eLeashType = DCO_ELeashType.NONE;
	protected IEntity m_LeashBuilding;
	protected vector  m_vLeashCenter;
	protected float   m_fLeashRadius;


	DCO_ELeashType GetLeashType()
	{
		if (m_bHoldPosition)
			return DCO_ELeashType.POINT;
		return m_eLeashType;
	}

	IEntity GetLeashBuilding() { return m_LeashBuilding; }

	void SetLeashBuilding(IEntity building)
	{
		m_LeashBuilding = building;
		m_eLeashType = DCO_ELeashType.NONE;
		if (building)
			m_eLeashType = DCO_ELeashType.BUILDING;
	}

	void SetLeashRadius(vector center, float radius)
	{
		m_eLeashType   = DCO_ELeashType.RADIUS;
		m_vLeashCenter = center;
		m_fLeashRadius = radius;
		m_LeashBuilding = null;
	}

	void ClearLeash()
	{
		m_eLeashType = DCO_ELeashType.NONE;
		m_LeashBuilding = null;
	}

	bool LeashAllows(vector pos)
	{
		switch (GetLeashType())
		{
			case DCO_ELeashType.POINT:
			{
				AIAgent agent = AIAgent.Cast(GetOwner());
				IEntity ent;
				if (agent)
					ent = agent.GetControlledEntity();
				return ent && vector.DistanceXZ(ent.GetOrigin(), pos) < 0.5;
			}
			case DCO_ELeashType.BUILDING:
			{
				if (!m_LeashBuilding)
					return true;

				vector mins, maxs;
				m_LeashBuilding.GetBounds(mins, maxs);
				vector local = m_LeashBuilding.CoordToLocal(pos);
				for (int a = 0; a < 3; a++)
				{
					if (local[a] < mins[a] - 1.0 || local[a] > maxs[a] + 1.0)
						return false;
				}
				return true;
			}
			case DCO_ELeashType.RADIUS:
				return vector.DistanceXZ(m_vLeashCenter, pos) <= m_fLeashRadius;
		}
		return true;
	}
}

enum DCO_ELeashType
{
	NONE,
	POINT,
	BUILDING,
	RADIUS
}

class DCO_Leash
{
	static bool Allows(SCR_AIUtilityComponent utility, vector pos)
	{
		if (!utility || !utility.m_DCOConfig)
			return true;
		return utility.m_DCOConfig.LeashAllows(pos);
	}
}