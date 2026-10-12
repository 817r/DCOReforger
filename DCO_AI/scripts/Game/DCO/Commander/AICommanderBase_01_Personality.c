modded class AICommander_BaseComponent
{
	protected float GetDefendShare()
	{
		if (m_bAllIn)
			return 0;
		return Math.Clamp(m_fReservePercent / 100.0 * m_fDefendSharePerReserve, m_fDefendShareMin, 1);
	}

	DCO_ReinfBoard DCO_Reinf()
	{
		if (!m_DCOReinf)
			m_DCOReinf = new DCO_ReinfBoard(m_sCommanderUID);
		return m_DCOReinf;
	}

	CMD_AICommanderObjectiveComponent DCO_OpMain()
	{
		if (m_OpPlanner && m_OpPlanner.Main())
			return m_OpPlanner.Main();
		return DCO_TacticTest.Main(this);
	}

	DCO_Doctrine GetDoctrine()
	{
		if (!m_bDocAttrApplied)
		{
			m_bDocAttrApplied = true;
			if (!m_sDoctrineOverride.IsEmpty())
			{
				DCO_SetDocOverrideText(m_sDoctrineOverride);
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), "doctrine_override_attr cmd=" + m_sCommanderUID + " text=" + m_sDoctrineOverride + " rows=" + m_mDocOverride.Count().ToString());
			}
		}
		int ver = DCO_GlobalAIComponent.DCO_DocVersion();
		if (!m_Doctrine || ver != m_iDocVer)
			ResolveDoctrine(ver);
		return m_Doctrine;
	}

	protected DCO_Doctrine BaseDoctrine(out string scope)
	{
		if (!m_sDoctrinePreset.IsEmpty())
		{
			scope = "commander_override";
			DCO_DoctrinePreset preset = DCO_DoctrineStore.Find(m_sDoctrinePreset);
			if (preset)
			{
				DCO_Doctrine own = new DCO_Doctrine();
				own.FromPreset(preset, "commander");
				return own;
			}
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, DCO_Log.IdCmd(this), "doctrine_missing cmd=" + m_sCommanderUID + " preset=" + DCO_SettingsBaseline.LogText(m_sDoctrinePreset) + " fallback=faction");
		}
		scope = "faction";
		string source;
		DCO_Doctrine d = DCO_GlobalAIComponent.DCO_DoctrineFor(m_sFactionKey, source);
		d.m_sSource = source;
		return d;
	}

	protected void ResolveDoctrine(int ver)
	{
		m_iDocVer = ver;
		string scope;
		DCO_Doctrine d = BaseDoctrine(scope);
		foreach (int index, int mask : m_mDocOverride)
		{
			if (d.Mask(index) == mask)
				continue;
			d.SetMask(index, mask);
			d.m_bModified = true;
			scope = "commander_override";
		}
		m_Doctrine = d;
		string sig = scope + "|" + d.m_sPreset + "|" + d.MaskText() + "|" + d.NumText();
		if (sig == m_sDocSig)
			return;
		m_sDocSig = sig;
		DCO_Perf.Count("doctrine:load:" + scope);
		string line = "doctrine_load cmd=" + m_sCommanderUID + " scope=" + scope + " faction=" + m_sFactionKey;
		line += " preset=" + DCO_SettingsBaseline.LogText(d.m_sPreset) + " modified=" + d.m_bModified.ToString();
		line += " overrides=" + m_mDocOverride.Count().ToString() + " source=" + d.m_sSource;
		line += " masks=" + d.MaskText() + " numbers=" + d.NumText();
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), line);
	}

	bool DCO_DocRowOverridden(int index)
	{
		return !m_sDoctrinePreset.IsEmpty() || m_mDocOverride.Contains(index);
	}

	void DCO_SetDocOverride(int index, int mask)
	{
		string scope;
		DCO_Doctrine baseDoc = BaseDoctrine(scope);
		if (baseDoc.Mask(index) == mask)
			m_mDocOverride.Remove(index);
		else
			m_mDocOverride.Set(index, mask);
		DCO_GlobalAIComponent.DCO_BumpDoc();
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), "doctrine_override cmd=" + m_sCommanderUID + " row=" + index.ToString() + " mask=" + mask.ToString() + " base=" + baseDoc.Mask(index).ToString() + " overrides=" + m_mDocOverride.Count().ToString());
	}

	void DCO_SetDocPresetOverride(string name)
	{
		if (name == m_sDoctrinePreset && m_mDocOverride.IsEmpty())
			return;
		m_sDoctrinePreset = name;
		m_mDocOverride.Clear();
		DCO_GlobalAIComponent.DCO_BumpDoc();
		string scope = "faction";
		if (!name.IsEmpty())
			scope = "commander_override";
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), "doctrine_override cmd=" + m_sCommanderUID + " preset=" + DCO_SettingsBaseline.LogText(name) + " scope=" + scope);
	}

	string DCO_DocOverrideText()
	{
		string text;
		foreach (int index, int mask : m_mDocOverride)
		{
			if (!text.IsEmpty())
				text += " ";
			text += index.ToString() + ":" + mask.ToString();
		}
		if (!text.IsEmpty())
			text += " v3";
		return text;
	}

	void DCO_SetDocOverrideText(string text)
	{
		m_mDocOverride.Clear();
		array<string> parts = {};
		text.Split(" ", parts, true);
		bool v3 = parts.Contains("v3");
		foreach (string part : parts)
		{
			array<string> kv = {};
			part.Split(":", kv, false);
			if (kv.Count() != 2)
				continue;
			int row = kv[0].ToInt();
			int mask = kv[1].ToInt();
			if (!v3 && row >= 0 && row < 5 && (mask & DCO_EBattleCol.STAGED) == 0)
			{
				mask = mask | DCO_EBattleCol.STAGED;
				DCO_Perf.Count("doc:staged_legacy_override");
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("doctrine_staged_legacy source=override cmd=%1 row=%2 after=%3", m_sCommanderUID, row, mask));
			}
			m_mDocOverride.Set(row, mask);
		}
		DCO_GlobalAIComponent.DCO_BumpDoc();
	}

	void DCO_ApplyPersonalityPreset(string name)
	{
		DCO_Personality p = DCO_Personality.Get(name);
		m_sPersonalityPreset = DCO_Personality.NameAt(DCO_Personality.IndexOf(name));
		SetTraits(p.m_fIntel, p.m_fStrategy, p.m_fAggression, p.m_fLogistics, p.m_fSupport, p.m_fLeadership, p.m_iCourage, p.m_iDecision);
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), "personality_load cmd=" + m_sCommanderUID + " preset=" + m_sPersonalityPreset + " random=0 source=gm traits=" + TraitText());
	}

	void DCO_SetTrait(int which, float value)
	{
		float intel = m_fTraitIntel;
		float strategy = m_fTraitStrategy;
		float aggression = m_fTraitAggression;
		float logistics = m_fTraitLogistics;
		float support = m_fTraitSupport;
		float leadership = m_fTraitLeadership;
		int courage = m_iCourage;
		int decision = m_iDecision;
		switch (which)
		{
			case 0: intel = value; break;
			case 1: strategy = value; break;
			case 2: aggression = value; break;
			case 3: logistics = value; break;
			case 4: support = value; break;
			case 5: leadership = value; break;
			case 6: courage = Math.Round(value); break;
			case 7: decision = Math.Round(value); break;
		}
		SetTraits(intel, strategy, aggression, logistics, support, leadership, courage, decision);
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), "personality_set cmd=" + m_sCommanderUID + " trait=" + which.ToString() + " value=" + value.ToString() + " traits=" + TraitText());
	}

	void DCO_SetPreBattleIntel(int level)
	{
		m_ePreBattleIntel = Math.ClampInt(level, 0, 3);
	}

	void DCO_SetRandomPersonality(bool random)
	{
		m_bRandomPersonality = random;
	}

	float GetDoctrineShift()
	{
		return Math.Clamp(m_fShiftPerAggression * (DCO_EffTraitAggression() - 1) + m_fShiftPerCourage * (m_iCourage - DCO_ECourage.CALCULATED), -m_fShiftMax, m_fShiftMax);
	}

	protected bool LegacyTraitsSet()
	{
		array<float> old = {m_fAggression, m_fAdaptability, m_fRiskTaking, m_fResilience, m_fPatience, m_fCombatFocus};
		foreach (float v : old)
		{
			if (Math.AbsFloat(v - 0.5) > 0.001)
				return true;
		}
		return false;
	}

	protected string LegacyText()
	{
		return m_fAggression.ToString(-1, 2) + "/" + m_fAdaptability.ToString(-1, 2) + "/" + m_fRiskTaking.ToString(-1, 2) + "/" + m_fResilience.ToString(-1, 2) + "/" + m_fPatience.ToString(-1, 2) + "/" + m_fCombatFocus.ToString(-1, 2);
	}

	protected string TraitText()
	{
		return m_fTraitIntel.ToString(-1, 1) + "/" + m_fTraitStrategy.ToString(-1, 1) + "/" + m_fTraitAggression.ToString(-1, 1) + "/" + m_fTraitLogistics.ToString(-1, 1) + "/" + m_fTraitSupport.ToString(-1, 1) + " lead=" + m_fTraitLeadership.ToString(-1, 1) + " courage=" + m_iCourage.ToString() + " decision=" + m_iDecision.ToString();
	}

	protected int DecisionFromLegacy(float adaptability, float patience)
	{
		float score = adaptability + (1 - patience);
		if (score >= m_fDecisionFastScore)
			return DCO_EDecision.FAST;
		if (score <= m_fDecisionSlowScore)
			return DCO_EDecision.SLOW;
		return DCO_EDecision.MODERATE;
	}

	static int CourageFromLegacy(float resilience)
	{
		if (resilience < 0.2)
			return DCO_ECourage.HESITANT;
		if (resilience < 0.4)
			return DCO_ECourage.CAREFUL;
		if (resilience < 0.6)
			return DCO_ECourage.CALCULATED;
		if (resilience < 0.8)
			return DCO_ECourage.BOLD;
		return DCO_ECourage.RECKLESS;
	}

	protected static float Tenth(float v)
	{
		return Math.Clamp(Math.Round(v * 10) / 10, 0, 2);
	}

	protected void InitDoctrine(string source)
	{
		m_Doctrine = null;
		GetDoctrine();

		bool legacy = LegacyTraitsSet();
		string oldText = LegacyText();
		DCO_Personality persona = DCO_Personality.Get(m_sPersonalityPreset);
		m_sPersonalityPreset = DCO_Personality.NameAt(DCO_Personality.IndexOf(m_sPersonalityPreset));
		float intel = persona.m_fIntel;
		float strategy = persona.m_fStrategy;
		float aggression = persona.m_fAggression;
		float logistics = persona.m_fLogistics;
		float support = persona.m_fSupport;
		float leadership = persona.m_fLeadership;
		int courage = persona.m_iCourage;
		int decision = persona.m_iDecision;
		if (legacy)
		{
			aggression = Tenth(m_fAggression * 2);
			intel = Tenth((1 - m_fRiskTaking) * 2);
			strategy = Tenth(m_fAdaptability + m_fCombatFocus);
			decision = DecisionFromLegacy(m_fAdaptability, m_fPatience);
			courage = CourageFromLegacy(m_fResilience);
			logistics = 1;
			support = 1;
			leadership = 1;
		}
		if (m_fTraitIntel < 0)
			m_fTraitIntel = intel;
		if (m_fTraitStrategy < 0)
			m_fTraitStrategy = strategy;
		if (m_fTraitAggression < 0)
			m_fTraitAggression = aggression;
		if (m_fTraitLogistics < 0)
			m_fTraitLogistics = logistics;
		if (m_fTraitSupport < 0)
			m_fTraitSupport = support;
		if (m_fTraitLeadership < 0)
			m_fTraitLeadership = leadership;
		if (m_iCourage < 0)
			m_iCourage = courage;
		if (m_iDecision < 0)
			m_iDecision = decision;
		m_iCourage = Math.ClampInt(m_iCourage, 0, 4);
		m_iDecision = Math.ClampInt(m_iDecision, 0, 2);

		if (m_bRandomPersonality)
		{
			m_fTraitIntel = Tenth(m_fTraitIntel + Math.RandomFloat(-m_fRandomTraitJitter, m_fRandomTraitJitter));
			m_fTraitStrategy = Tenth(m_fTraitStrategy + Math.RandomFloat(-m_fRandomTraitJitter, m_fRandomTraitJitter));
			m_fTraitAggression = Tenth(m_fTraitAggression + Math.RandomFloat(-m_fRandomTraitJitter, m_fRandomTraitJitter));
			m_fTraitLogistics = Tenth(m_fTraitLogistics + Math.RandomFloat(-m_fRandomTraitJitter, m_fRandomTraitJitter));
			m_fTraitSupport = Tenth(m_fTraitSupport + Math.RandomFloat(-m_fRandomTraitJitter, m_fRandomTraitJitter));
			m_fTraitLeadership = Tenth(m_fTraitLeadership + Math.RandomFloat(-m_fRandomTraitJitter, m_fRandomTraitJitter));
			m_iCourage = Math.ClampInt(m_iCourage + Math.RandomIntInclusive(-1, 1), 0, 4);
			m_iDecision = Math.ClampInt(m_iDecision + Math.RandomIntInclusive(-1, 1), 0, 2);
		}
		SyncLegacyTraits();
		if (legacy)
		{
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), "personality_migrate cmd=" + m_sCommanderUID + " old=" + oldText + " new=" + TraitText() + " source=" + source);
			DCO_Perf.Count("doctrine:migrate");
		}
		string personaLine = "personality_load cmd=" + m_sCommanderUID + " preset=" + m_sPersonalityPreset + " random=" + m_bRandomPersonality.ToString();
		personaLine += " legacy=" + legacy.ToString() + " source=" + source + " traits=" + TraitText();
		personaLine += " prebattle=" + typename.EnumToString(DCO_EPreBattleIntel, m_ePreBattleIntel);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), personaLine);
	}

	protected void SyncLegacyTraits()
	{
		m_fAggression = Math.Clamp(m_fTraitAggression / 2, 0, 1);
		m_fRiskTaking = Math.Clamp(1 - m_fTraitIntel / 2, 0, 1);
		m_fAdaptability = Math.Clamp(m_fTraitStrategy / 2, 0, 1);
		m_fCombatFocus = Math.Clamp(m_fTraitStrategy / 2, 0, 1);
		m_fPatience = m_fPatienceNormal;
		if (m_iDecision == DCO_EDecision.FAST)
			m_fPatience = m_fPatienceFast;
		else if (m_iDecision == DCO_EDecision.SLOW)
			m_fPatience = m_fPatienceSlow;
		array<float> resilience = {0.1, 0.3, 0.5, 0.7, 0.9};
		m_fResilience = resilience[Math.ClampInt(m_iCourage, 0, 4)];
		m_ROE = null;
	}

	protected void SyncTraitsFromLegacy()
	{
		m_fTraitAggression = Tenth(m_fAggression * 2);
		m_fTraitIntel = Tenth((1 - m_fRiskTaking) * 2);
		m_fTraitStrategy = Tenth(m_fAdaptability + m_fCombatFocus);
		m_iDecision = DecisionFromLegacy(m_fAdaptability, m_fPatience);
		m_iCourage = CourageFromLegacy(m_fResilience);
	}

	void SetTraits(float intel, float strategy, float aggression, float logistics, float support, float leadership, int courage, int decision)
	{
		m_fTraitIntel = Tenth(intel);
		m_fTraitStrategy = Tenth(strategy);
		m_fTraitAggression = Tenth(aggression);
		m_fTraitLogistics = Tenth(logistics);
		m_fTraitSupport = Tenth(support);
		m_fTraitLeadership = Tenth(leadership);
		m_iCourage = Math.ClampInt(courage, 0, 4);
		m_iDecision = Math.ClampInt(decision, 0, 2);
		SyncLegacyTraits();
		if (m_fBaseThinkInterval <= 0)
			return;
		ScaleTimingsByPersonality();
	}

	protected void ScaleTimingsByPersonality()
	{
		m_fThinkInterval = m_fBaseThinkInterval * Math.Lerp(m_fThinkScaleSlow, m_fThinkScaleFast, m_fAdaptability);
		m_iRetreatThreshold = Math.Max(1, Math.Round(1.0 * m_iBaseRetreatThreshold * Math.Lerp(m_fRetreatScaleTimid, m_fRetreatScaleSteady, m_fResilience)));
		m_fStalemateResponseCooldown = m_fBaseStalemateResponseCooldown * Math.Lerp(m_fStalemateScaleImpatient, m_fStalemateScalePatient, m_fPatience);
	}

	void SetCommanderMode(CMD_ECommanderMode mode)
	{
		m_eCommanderModeExternal = mode;
		m_eCommanderMode = mode;
	}

	void SetAggression(float value)  { m_fAggression  = Math.Clamp(value, 0, 1); SyncTraitsFromLegacy(); m_ROE = null; }

	void SetRiskTaking(float value)  { m_fRiskTaking  = Math.Clamp(value, 0, 1); SyncTraitsFromLegacy(); m_ROE = null; }

	void SetCombatFocus(float value) { m_fCombatFocus = Math.Clamp(value, 0, 1); SyncTraitsFromLegacy(); m_ROE = null; }

	void SetAdaptability(float value)
	{
		m_fAdaptability = Math.Clamp(value, 0, 1);
		ApplyPersonalityDerived();
	}

	void SetResilience(float value)
	{
		m_fResilience = Math.Clamp(value, 0, 1);
		ApplyPersonalityDerived();
	}

	void SetPatience(float value)
	{
		m_fPatience = Math.Clamp(value, 0, 1);
		ApplyPersonalityDerived();
	}

	protected void ApplyPersonalityDerived()
	{
		m_ROE = null;
		SyncTraitsFromLegacy();
		if (m_fBaseThinkInterval <= 0)
			return;

		ScaleTimingsByPersonality();
	}

	protected float GetSectorPersonalityMod()
	{
		if (!m_bScaleSectorByPersonality)
			return 1.0;

		float mod = Math.Lerp(m_fSectorPersonalityMax, m_fSectorPersonalityMin, DCO_EffAggression());

		mod = mod * Math.Lerp(m_fSectorModLow, m_fSectorModHigh, m_fResilience);

		return mod;
	}
}
