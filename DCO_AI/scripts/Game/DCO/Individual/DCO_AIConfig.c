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

	[Attribute("1", UIWidgets.ComboBox, "Leadership: how well this AI steadies and leads his squad when he is the leader.", "", ParamEnumArray.FromEnum(DCO_ELeadership))]
	protected DCO_ELeadership m_eLeadership;

	[Attribute("0.5", UIWidgets.Slider, "How often this AI throws frag grenades. 0 = never, 0.5 = default, 1 = very often.", params: "0 1 0.01")]
	protected float m_fGrenadeUsage;

	[Attribute("0.5", UIWidgets.Slider, "How often this AI uses a grenade launcher (UGL). 0 = never, 0.5 = default, 1 = very often.", params: "0 1 0.01")]
	protected float m_fGLUsage;

	[Attribute("1", UIWidgets.Slider, "This AI's grenade launcher (UGL) accuracy. Higher = tighter impact spread. 1 = default.", params: "0.1 5 0.1")]
	protected float m_fGLAccuracy;

	[Attribute("0.5", UIWidgets.Slider, "How often this AI throws smoke to cover movement. 0 = never, 0.5 = default, 1 = very often.", params: "0 1 0.01")]
	protected float m_fSmokeUsage;
	protected bool m_bDCOSmokeOverride;

	protected bool m_bHoldPosition = false;
	protected float m_fDCOAggrApplied = 1.0;
	protected int m_iDCOVer = -1;
	protected DCO_GlobalAIComponent m_DCOSet;
	protected float m_fDCOGrenRaw = -1;
	protected float m_fDCOGLRaw = -1;
	protected int m_iDCOOwn;
	protected string m_sDCORollSig;

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
		int ver = DCO_GlobalAIComponent.DCO_TuneVerAll();
		if (!m_DCOSet || ver == m_iDCOVer)
			return;
		m_iDCOVer = ver;
		DCO_GlobalAIComponent cur = DCO_GlobalAIComponent.GetFor(GetOwner());
		if (cur)
			m_DCOSet = cur;
		DCO_Rescale();
		int copied = DCO_CopyGlobal(m_DCOSet, false);
		DCO_Reroll();
		DCO_Perf.Count("unit_cfg:refresh");
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, "-", string.Format("unit_cfg_refresh unit=%1 copied=%2 own=%3 aggr=%4", DCO_Log.IdUnit(GetOwner()), copied, m_iDCOOwn, m_fDCOAggrApplied.ToString(-1, 2)));
	}

	protected void DCO_Reroll()
	{
		string sig = m_DCOSet.DCO_RollSig();
		if (sig == m_sDCORollSig || m_DCOSet.DCO_TuneF("ga.reroll_live", 0.0) < 0.5)
			return;
		m_sDCORollSig = sig;
		DCO_EAIPersonality p = m_ePersonality;
		DCO_ELeadership l = m_eLeadership;
		if ((m_iDCOOwn & 32768) == 0)
			m_ePersonality = m_DCOSet.RollWeightedPersonality();
		if ((m_iDCOOwn & 65536) == 0)
			m_eLeadership = m_DCOSet.RollWeightedLeadership();
		DCO_Perf.Count("ga:reroll");
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, "-", string.Format("unit_reroll unit=%1 personality_from=%2 personality_to=%3 leadership_from=%4 leadership_to=%5 own=%6", DCO_Log.IdUnit(GetOwner()), typename.EnumToString(DCO_EAIPersonality, p), typename.EnumToString(DCO_EAIPersonality, m_ePersonality), DCO_Leadership.Name(l), DCO_Leadership.Name(m_eLeadership), m_iDCOOwn));
	}

	protected bool DCO_Free(int bit, bool all)
	{
		return all || (m_iDCOOwn & bit) == 0;
	}

	protected float DCO_MapUse(float raw, out float rawStore)
	{
		raw = Math.Clamp(raw, 0, 1);
		rawStore = -1;
		if (m_DCOSet && m_DCOSet.DCO_TuneF("agg.usage_map", 1.0) > 0)
		{
			rawStore = raw;
			return UsageMap(raw, m_fDCOAggrApplied);
		}
		return Math.Clamp(raw * m_fDCOAggrApplied, 0, 1);
	}

	protected int DCO_CopyGlobal(DCO_GlobalAIComponent s, bool all)
	{
		if (!s)
			return 0;
		int n;
		float calm = Calm(m_fDCOAggrApplied);
		if (all)
		{
			m_fAiPerception = s.GetUnitPerception();
			m_fSmokeUsage = s.GetSmokeUsage();
		}
		if (DCO_Free(1, all))
		{
			m_fAimAccuracy = s.GetUnitSkill();
			n++;
		}
		if (DCO_Free(2, all))
		{
			m_bIsMagicallyResupplied = s.GetUnitMagicMagazine();
			n++;
		}
		if (DCO_Free(4, all))
		{
			m_fAimSpeed = s.GetAimSpeed();
			n++;
		}
		if (DCO_Free(8, all))
		{
			m_fAimShake = s.GetAimShake();
			n++;
		}
		if (DCO_Free(16, all))
		{
			m_eAISkillDefault = s.GetAISkill();
			n++;
		}
		if (DCO_Free(32, all))
		{
			m_fVehicleDismountDanger = s.GetDismountDistance();
			n++;
		}
		if (DCO_Free(64, all))
		{
			m_eDisembarkRule = s.GetDisembarkRule();
			n++;
		}
		if (DCO_Free(128, all))
		{
			m_fSuppressionEffect = s.GetSuppressionEffect() * calm;
			n++;
		}
		if (DCO_Free(256, all))
		{
			m_fTakeCoverChance = Math.Clamp(s.GetTakeCoverChance() * calm, 0, 1);
			n++;
		}
		if (DCO_Free(512, all))
		{
			m_fDodgeChance = Math.Clamp(s.GetDodgeChance() * calm, 0, 1);
			n++;
		}
		if (DCO_Free(1024, all))
		{
			m_fDodgeCooldown = s.GetDodgeCooldown();
			m_fDodgeMaxDist = s.GetDodgeMaxDist();
			m_fDodgeSearchDist = s.GetDodgeSearchDist();
			m_bDodgeScaleByPersonality = s.GetDodgeScaleByPersonality();
			n++;
		}
		float rawStore;
		if (DCO_Free(2048, all))
		{
			m_fGrenadeUsage = DCO_MapUse(s.GetGrenadeUsage(), rawStore);
			m_fDCOGrenRaw = rawStore;
			n++;
		}
		if (DCO_Free(4096, all))
		{
			m_fGLUsage = DCO_MapUse(s.GetGLUsage(), rawStore);
			m_fDCOGLRaw = rawStore;
			n++;
		}
		if (DCO_Free(16384, all))
		{
			m_fGLAccuracy = s.GetGLAccuracy();
			n++;
		}
		if (DCO_Free(8192, all))
		{
			m_iDodgeShotThreshold = s.GetDodgeShotThreshold();
			m_fDodgeShotWindow = s.GetDodgeShotWindow();
			n++;
		}
		return n;
	}

	float DCO_SuppRaw()
	{
		return m_fSuppressionEffect / Math.Max(Calm(m_fDCOAggrApplied), 0.01);
	}

	float DCO_DodgeRaw()
	{
		return Math.Clamp(m_fDodgeChance / Math.Max(Calm(m_fDCOAggrApplied), 0.01), 0, 1);
	}

	float DCO_GrenRaw()
	{
		if (m_fDCOGrenRaw >= 0)
			return m_fDCOGrenRaw;
		return Math.Clamp(m_fGrenadeUsage / Math.Max(m_fDCOAggrApplied, 0.01), 0, 1);
	}

	float DCO_GLRaw()
	{
		if (m_fDCOGLRaw >= 0)
			return m_fDCOGLRaw;
		return Math.Clamp(m_fGLUsage / Math.Max(m_fDCOAggrApplied, 0.01), 0, 1);
	}

	protected void DCO_Rescale()
	{
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

		m_ePersonality = settings.RollWeightedPersonality();
		m_eLeadership = settings.RollWeightedLeadership();
		m_sDCORollSig = settings.DCO_RollSig();
		float aggr = settings.DCO_Aggression();
		m_DCOSet = settings;
		m_fDCOAggrApplied = aggr;
		m_iDCOVer = DCO_GlobalAIComponent.DCO_TuneVerAll();
		DCO_CopyGlobal(settings, true);
		if (aggr != 1)
		{
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, "-", string.Format("aggr_unit unit=%1 aggr=%2 supp=%3 cover=%4 dodge=%5 frag=%6 gl=%7", DCO_Log.IdUnit(owner), aggr.ToString(-1, 2), m_fSuppressionEffect.ToString(-1, 2), m_fTakeCoverChance.ToString(-1, 2), m_fDodgeChance.ToString(-1, 2), m_fGrenadeUsage.ToString(-1, 2), m_fGLUsage.ToString(-1, 2)));
		}
	}

	float GetDodgeChance()
	{
		return m_fDodgeChance;
	}

	float SetDodgeChance(float f)
	{
		m_iDCOOwn |= 512;
		m_fDodgeChance = Math.Clamp(f * Calm(m_fDCOAggrApplied), 0, 1);
		return m_fDodgeChance;
	}

	float GetDodgeCooldown()
	{
		return m_fDodgeCooldown;
	}

	float SetDodgeCooldown(float f)
	{
		m_iDCOOwn |= 1024;
		m_fDodgeCooldown = f;
		return m_fDodgeCooldown;
	}

	float GetDodgeMaxDist()
	{
		return m_fDodgeMaxDist;
	}

	float SetDodgeMaxDist(float f)
	{
		m_iDCOOwn |= 1024;
		m_fDodgeMaxDist = f;
		return m_fDodgeMaxDist;
	}

	float GetDodgeSearchDist()
	{
		return m_fDodgeSearchDist;
	}

	float SetDodgeSearchDist(float f)
	{
		m_iDCOOwn |= 1024;
		m_fDodgeSearchDist = f;
		return m_fDodgeSearchDist;
	}

	bool GetDodgeScaleByPersonality()
	{
		return m_bDodgeScaleByPersonality;
	}

	bool SetDodgeScaleByPersonality(bool b)
	{
		m_iDCOOwn |= 1024;
		m_bDodgeScaleByPersonality = b;
		return m_bDodgeScaleByPersonality;
	}

	DCO_AISKILL SetAISkill(DCO_AISKILL ski)
	{
		m_iDCOOwn |= 16;
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
		m_iDCOOwn |= 64;
		m_eDisembarkRule = Math.ClampInt(rule, DCO_EDisembarkRule.AUTO, DCO_EDisembarkRule.NEVER);
	}

	float GetDismountDistance()
	{
		return m_fVehicleDismountDanger;
	}

	float SetDismountDistance(float acc)
	{
		m_iDCOOwn |= 32;
		m_fVehicleDismountDanger = acc;
		return m_fVehicleDismountDanger;
	}

	float GetAimSpeed()
	{
		return m_fAimSpeed;
	}

	void SetAimSpeed(float v)
	{
		m_iDCOOwn |= 4;
		m_fAimSpeed = Math.Clamp(v, 0.25, 4.0);
		LogAimCfg();
	}

	float GetAimShake()
	{
		return m_fAimShake;
	}

	void SetAimShake(float v)
	{
		m_iDCOOwn |= 8;
		m_fAimShake = Math.Clamp(v, 0.25, 4.0);
		LogAimCfg();
	}

	float GetAccuracy()
	{
		return m_fAimAccuracy;
	}

	float SetAccuracy(float acc)
	{
		m_iDCOOwn |= 1;
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
		m_iDCOOwn |= 2;
		m_bIsMagicallyResupplied = s;
		return m_bIsMagicallyResupplied;
	}

	float GetSuppressionEffect()
	{
		return m_fSuppressionEffect;
	}

	float SetSuppressionEffect(float f)
	{
		m_iDCOOwn |= 128;
		m_fSuppressionEffect = f * Calm(m_fDCOAggrApplied);
		return m_fSuppressionEffect;
	}

	float GetTakeCoverChance()
	{
		return m_fTakeCoverChance;
	}

	float SetTakeCoverChance(float f)
	{
		m_iDCOOwn |= 256;
		m_fTakeCoverChance = Math.Clamp(f * Calm(m_fDCOAggrApplied), 0, 1);
		return m_fTakeCoverChance;
	}

	DCO_EAIPersonality GetPersonality()
	{
		return m_ePersonality;
	}

	DCO_EAIPersonality SetPersonality(DCO_EAIPersonality p)
	{
		m_iDCOOwn |= 32768;
		m_ePersonality = p;
		return m_ePersonality;
	}

	DCO_ELeadership GetLeadership()
	{
		return m_eLeadership;
	}

	DCO_ELeadership SetLeadership(DCO_ELeadership l)
	{
		m_iDCOOwn |= 65536;
		m_eLeadership = l;
		return m_eLeadership;
	}

	float GetGrenadeUsage()
	{
		return m_fGrenadeUsage;
	}

	float SetGrenadeUsage(float f)
	{
		m_iDCOOwn |= 2048;
		float rawStore;
		m_fGrenadeUsage = DCO_MapUse(f, rawStore);
		m_fDCOGrenRaw = rawStore;
		return m_fGrenadeUsage;
	}

	float GetGLUsage()
	{
		return m_fGLUsage;
	}

	float SetGLUsage(float f)
	{
		m_iDCOOwn |= 4096;
		float rawStore;
		m_fGLUsage = DCO_MapUse(f, rawStore);
		m_fDCOGLRaw = rawStore;
		return m_fGLUsage;
	}

	float GetGLAccuracy()
	{
		return m_fGLAccuracy;
	}

	float SetGLAccuracy(float f)
	{
		m_iDCOOwn |= 16384;
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
		m_bDCOSmokeOverride = true;
		return m_fSmokeUsage;
	}

	bool DCO_SmokeOverridden()
	{
		return m_bDCOSmokeOverride;
	}

	int GetDodgeShotThreshold()
	{
		return m_iDodgeShotThreshold;
	}

	int SetDodgeShotThreshold(int i)
	{
		m_iDCOOwn |= 8192;
		m_iDodgeShotThreshold = Math.ClampInt(i, 1, 20);
		return m_iDodgeShotThreshold;
	}

	float GetDodgeShotWindow()
	{
		return m_fDodgeShotWindow;
	}

	float SetDodgeShotWindow(float f)
	{
		m_iDCOOwn |= 8192;
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

	static DCO_ELeadership LeadershipOf(SCR_AIUtilityComponent u)
	{
		DCO_AIConfigComponent c = Of(u);
		if (c)
			return c.GetLeadership();
		return DCO_ELeadership.AVERAGE;
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
		if (c && c.DCO_SmokeOverridden())
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