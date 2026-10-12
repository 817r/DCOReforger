class DCO_CommanderPresets
{
	static string Dir()
	{
		return "$profile:DCO/CommanderPresets/";
	}

	static string Sanitize(string name)
	{
		name = name.Trim();
		string outName;
		for (int i = 0; i < name.Length() && outName.Length() < 32; i++)
		{
			string c = name.Get(i);
			if (c == " ")
				c = "_";
			int code = c.ToAscii();
			bool ok = (code >= 48 && code <= 57) || (code >= 65 && code <= 90) || (code >= 97 && code <= 122) || c == "_" || c == "-";
			if (ok)
				outName += c;
		}
		return outName;
	}

	static string PathOf(string name)
	{
		return Dir() + name + ".json";
	}

	static bool Exists(string name)
	{
		return !name.IsEmpty() && FileIO.FileExists(PathOf(name));
	}

	static void List(notnull array<string> outNames)
	{
		array<string> files = SCR_FileIOHelper.FindFiles(Dir(), ".json");
		if (!files)
			return;
		foreach (string f : files)
		{
			string n = FilePath.StripExtension(FilePath.StripPath(f));
			if (!n.IsEmpty() && outNames.Find(n) < 0)
				outNames.Insert(n);
		}
		outNames.Sort();
	}
}

modded class AICommander_ManagerComponent
{
	[RplProp()]
	protected string m_sDCOPresetList;

	[RplProp()]
	protected string m_sDCOPresetActive;

	protected float m_fDCOPresetListAt = -1;

	void DCO_RefreshPresetList(bool force)
	{
		if (!Replication.IsServer())
			return;
		float now = DCO_Util.NowS();
		if (!force && m_fDCOPresetListAt >= 0 && now - m_fDCOPresetListAt < 60.0)
			return;
		m_fDCOPresetListAt = now;

		array<string> names = {};
		DCO_CommanderPresets.List(names);
		string joined;
		foreach (int i, string n : names)
		{
			if (i > 0)
				joined += ";";
			joined += n;
		}
		if (joined == m_sDCOPresetList)
			return;
		m_sDCOPresetList = joined;
		Replication.BumpMe();
	}

	void DCO_GetPresetNames(notnull array<string> outNames)
	{
		if (!m_sDCOPresetList.IsEmpty())
			m_sDCOPresetList.Split(";", outNames, true);
	}

	void DCO_RebuildPresetActive()
	{
		if (!Replication.IsServer())
			return;
		string s;
		foreach (AICommander_BaseComponent c : m_aCommander)
		{
			if (!c || c.DCO_GetActivePreset().IsEmpty())
				continue;
			if (!s.IsEmpty())
				s += ";";
			string dirty = "0";
			if (c.DCO_IsPresetDirty())
				dirty = "1";
			s += c.GetCommanderUID() + "|" + c.DCO_GetActivePreset() + "|" + dirty;
		}
		if (s == m_sDCOPresetActive)
			return;
		m_sDCOPresetActive = s;
		Replication.BumpMe();
	}

	string DCO_ActivePresetLabel(string uid)
	{
		array<string> entries = {};
		m_sDCOPresetActive.Split(";", entries, true);
		foreach (string e : entries)
		{
			array<string> f = {};
			e.Split("|", f, false);
			if (f.Count() < 3 || f[0] != uid)
				continue;
			if (f[2] == "1")
				return f[1] + "*";
			return f[1];
		}
		return string.Empty;
	}
}

modded class AICommander_BaseComponent
{
	[Attribute("", UIWidgets.EditBox, "Preset loaded at start from $profile:DCO/CommanderPresets/<name>.json before the first think. Empty = prefab values. Missing file = warning, prefab values.", category: "Commander Presets")]
	protected string m_sStartPreset;

	protected string m_sDCOActivePreset;
	protected string m_sDCOPresetSig;
	protected bool m_bDCOPresetDirty;
	protected float m_fDCOPresetTimer;

	protected ref SCR_JsonSaveContext m_DCOPSave;
	protected ref SCR_JsonLoadContext m_DCOPLoad;
	protected int m_iDCOPMode;
	protected int m_iDCOPApplied;

	string DCO_GetActivePreset()	{ return m_sDCOActivePreset; }
	bool DCO_IsPresetDirty()		{ return m_bDCOPresetDirty; }

	override protected void InitializeCommander()
	{
		bool loaded;
		if (Replication.IsServer() && !m_sStartPreset.IsEmpty())
		{
			string name = DCO_CommanderPresets.Sanitize(m_sStartPreset);
			if (DCO_CommanderPresets.Exists(name))
				loaded = DCO_LoadPreset(name, false);
			if (loaded)
				m_sTraitSource = "json";
			else
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("%1: start preset '%2' not found in %3, using prefab values", m_sCommanderUID, m_sStartPreset, DCO_CommanderPresets.Dir()));
		}

		super.InitializeCommander();

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr && Replication.IsServer())
		{
			mgr.DCO_RefreshPresetList(true);
			if (loaded)
				mgr.DCO_RebuildPresetActive();
		}
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		if (!Replication.IsServer())
			return;
		m_fDCOPresetTimer += timeSlice;
		if (m_fDCOPresetTimer < 15.0)
			return;
		m_fDCOPresetTimer = 0;

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
			mgr.DCO_RefreshPresetList(false);

		if (m_sDCOActivePreset.IsEmpty())
			return;
		string sig = DCO_PresetSignature();
		if (m_sDCOPresetSig.IsEmpty())
		{
			m_sDCOPresetSig = sig;
			return;
		}
		bool dirty = sig != m_sDCOPresetSig;
		if (dirty == m_bDCOPresetDirty)
			return;
		m_bDCOPresetDirty = dirty;
		if (mgr)
			mgr.DCO_RebuildPresetActive();
	}

	bool DCO_LoadPreset(string name, bool runtime)
	{
		string path = DCO_CommanderPresets.PathOf(name);
		if (!FileIO.FileExists(path))
		{
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("%1: preset file %2 not found", m_sCommanderUID, path));
			return false;
		}

		m_DCOPLoad = new SCR_JsonLoadContext();
		if (!m_DCOPLoad.LoadFromFile(path))
		{
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.ERROR, "-", string.Format("%1: failed to parse %2 (invalid JSON)", m_sCommanderUID, path));
			m_DCOPLoad = null;
			return false;
		}

		m_iDCOPMode = 0;
		m_iDCOPApplied = 0;
		DCO_PresetSyncAll(runtime);
		m_DCOPLoad = null;

		m_iAllowedTactics = m_iAllowedTactics & 8191;
		m_iAllowedDefenseTactics = m_iAllowedDefenseTactics & 255;
		if (runtime)
		{
			m_eCommanderMode = m_eCommanderModeExternal;
			ApplyPersonalityDerived();
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), "personality_migrate cmd=" + m_sCommanderUID + " old=" + LegacyText() + " new=" + TraitText() + " source=json preset=" + name);
		}

		m_sDCOActivePreset = name;
		m_sDCOPresetSig = string.Empty;
		m_bDCOPresetDirty = false;
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("%1: loaded preset '%2' (%3 settings)", m_sCommanderUID, name, m_iDCOPApplied));
		return true;
	}

	bool DCO_SavePreset(string name)
	{
		FileIO.MakeDirectory("$profile:DCO");
		FileIO.MakeDirectory(DCO_CommanderPresets.Dir());

		m_DCOPSave = new SCR_JsonSaveContext();
		m_DCOPSave.WriteValue("_comment", "DCO commander preset. Keys may be removed (missing key = setting unchanged). Values are clamped on load.");
		m_iDCOPMode = 1;
		DCO_PresetSyncAll(false);
		m_iDCOPMode = 0;
		bool ok = m_DCOPSave.SaveToFile(DCO_CommanderPresets.PathOf(name));
		m_DCOPSave = null;
		if (!ok)
		{
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.ERROR, "-", string.Format("%1: failed to write %2", m_sCommanderUID, DCO_CommanderPresets.PathOf(name)));
			return false;
		}

		m_sDCOActivePreset = name;
		m_sDCOPresetSig = string.Empty;
		m_bDCOPresetDirty = false;
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("%1: saved preset '%2'", m_sCommanderUID, name));
		return true;
	}

	protected string DCO_PresetSignature()
	{
		m_DCOPSave = new SCR_JsonSaveContext();
		m_iDCOPMode = 1;
		DCO_PresetSyncAll(false);
		m_iDCOPMode = 0;
		string s = m_DCOPSave.ExportToString();
		m_DCOPSave = null;
		return s;
	}

	void DCO_LoadPresetFromGM(string name)
	{
		name = DCO_CommanderPresets.Sanitize(name);
		bool ok = DCO_LoadPreset(name, true);
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
			mgr.DCO_RebuildPresetActive();
		if (ok)
			DCO_PlayerComms.SendToGameMasters("DCO", string.Format("%1: preset '%2' loaded (%3 settings)", m_sCommanderUID, name, m_iDCOPApplied));
		else
			DCO_PlayerComms.SendToGameMasters("DCO", string.Format("%1: preset '%2' could not be loaded", m_sCommanderUID, name));
	}

	void DCO_SavePresetFromGM(string rawName)
	{
		string trimmed = rawName.Trim();
		bool force = trimmed.Length() > 0 && trimmed.Get(trimmed.Length() - 1) == "!";
		string name = DCO_CommanderPresets.Sanitize(trimmed);
		if (name.IsEmpty())
		{
			DCO_PlayerComms.SendToGameMasters("DCO", "Preset name is empty. Use letters, digits, _ or -.");
			return;
		}
		if (!force && name != m_sDCOActivePreset && DCO_CommanderPresets.Exists(name))
		{
			DCO_PlayerComms.SendToGameMasters("DCO", string.Format("Preset '%1' already exists. Type %1! to overwrite it.", name));
			return;
		}

		bool ok = DCO_SavePreset(name);
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
		{
			mgr.DCO_RefreshPresetList(true);
			mgr.DCO_RebuildPresetActive();
		}
		if (ok)
			DCO_PlayerComms.SendToGameMasters("DCO", string.Format("%1: saved as preset '%2'", m_sCommanderUID, name));
		else
			DCO_PlayerComms.SendToGameMasters("DCO", string.Format("%1: preset '%2' could not be saved", m_sCommanderUID, name));
	}

	protected float DCO_PF(string key, float cur, float min, float max)
	{
		if (m_iDCOPMode == 1)
		{
			m_DCOPSave.WriteValue(key, cur);
			return cur;
		}
		float v;
		if (!m_DCOPLoad.ReadValue(key, v))
			return cur;
		m_iDCOPApplied++;
		return Math.Clamp(v, min, max);
	}

	protected int DCO_PI(string key, int cur, int min, int max)
	{
		if (m_iDCOPMode == 1)
		{
			m_DCOPSave.WriteValue(key, cur);
			return cur;
		}
		int v;
		if (!m_DCOPLoad.ReadValue(key, v))
			return cur;
		m_iDCOPApplied++;
		return Math.ClampInt(v, min, max);
	}

	protected bool DCO_PB(string key, bool cur)
	{
		if (m_iDCOPMode == 1)
		{
			m_DCOPSave.WriteValue(key, cur);
			return cur;
		}
		bool v;
		if (!m_DCOPLoad.ReadValue(key, v))
			return cur;
		m_iDCOPApplied++;
		return v;
	}

	protected string DCO_PS(string key, string cur)
	{
		if (m_iDCOPMode == 1)
		{
			m_DCOPSave.WriteValue(key, cur);
			return cur;
		}
		string v;
		if (!m_DCOPLoad.ReadValue(key, v))
			return cur;
		m_iDCOPApplied++;
		return v;
	}

	protected void DCO_PresetSyncManual(bool runtime)
	{
		float think = m_fThinkInterval;
		int retreat = m_iRetreatThreshold;
		float stale = m_fStalemateResponseCooldown;
		if (m_fBaseThinkInterval > 0)
		{
			think = m_fBaseThinkInterval;
			retreat = m_iBaseRetreatThreshold;
			stale = m_fBaseStalemateResponseCooldown;
		}
		think = DCO_PF("ThinkInterval", think, 0.5, 600);
		retreat = DCO_PI("RetreatThreshold", retreat, 0, 1000);
		stale = DCO_PF("StalemateResponseCooldown", stale, 0, 36000);

		int stock = m_iReinforcementStock;
		if (m_bStockInit)
			stock = m_iStockLeft;
		int before = m_iDCOPApplied;
		stock = DCO_PI("ReinforcementStock", stock, -1, 100000);
		bool stockRead = m_iDCOPApplied > before;

		before = m_iDCOPApplied;
		int maxGroups = DCO_PI("MaxAttackGroups", m_iMaxAttackGroups, 1, 20);
		bool maxRead = m_iDCOPApplied > before;

		if (m_iDCOPMode == 1)
			return;

		if (m_fBaseThinkInterval > 0)
		{
			m_fBaseThinkInterval = think;
			m_iBaseRetreatThreshold = retreat;
			m_fBaseStalemateResponseCooldown = stale;
		}
		else
		{
			m_fThinkInterval = think;
			m_iRetreatThreshold = retreat;
			m_fStalemateResponseCooldown = stale;
		}

		m_iReinforcementStock = stock;
		if (stockRead && runtime)
		{
			m_bStockInit = true;
			m_iStockLeft = stock;
		}
		m_iMaxAttackGroups = maxGroups;
		if (maxRead && runtime)
			m_bMaxAttackInit = true;
	}

	protected void DCO_PresetSync0()
	{
		m_fObjectiveAtTheSameTime = DCO_PI("ObjectiveAtTheSameTime", m_fObjectiveAtTheSameTime, 1, 10);
		m_bUseSynchronizedAttack = DCO_PB("UseSynchronizedAttack", m_bUseSynchronizedAttack);
		m_fSyncAttackMaxWaitTime = DCO_PF("SyncAttackMaxWaitTime", m_fSyncAttackMaxWaitTime, 0, 100000);
		m_fReconWaitTimeout = DCO_PF("ReconWaitTimeout", m_fReconWaitTimeout, 0, 100000);
	}

	protected void DCO_PresetSync1()
	{
		m_fDelayFirstIteration = DCO_PF("DelayFirstIteration", m_fDelayFirstIteration, 0, 100000);
		m_fCaptureCheckInterval = DCO_PF("CaptureCheckInterval", m_fCaptureCheckInterval, 0, 100000);
		m_fFrontlineReconRadius = DCO_PF("FrontlineReconRadius", m_fFrontlineReconRadius, 0, 100000);
		m_fFrontlineReconDuration = DCO_PF("FrontlineReconDuration", m_fFrontlineReconDuration, 0, 100000);
		m_iGarrisonGroupsPerObjective = DCO_PI("GarrisonGroupsPerObjective", m_iGarrisonGroupsPerObjective, 0, 100000);
		m_sDCOGarrisonWpOverride = DCO_PS("GarrisonWaypointPrefab", GetGarrisonWaypointPrefab());
		m_eGarrisonMode = DCO_PI("GarrisonMode", m_eGarrisonMode, 0, 2);
		m_fDefendEngageDist = DCO_PF("DefendEngageDist", m_fDefendEngageDist, 0, 1000);
		m_fDefendObjectiveMargin = DCO_PF("DefendObjectiveMargin", m_fDefendObjectiveMargin, 0, 300);
		m_bVehiclePatrol = DCO_PB("VehiclePatrol", m_bVehiclePatrol);
		m_fVehiclePatrolRadiusMul = DCO_PF("VehiclePatrolRadiusMul", m_fVehiclePatrolRadiusMul, 1, 5);
		m_iMaxFrontlineRecon = DCO_PI("MaxFrontlineRecon", m_iMaxFrontlineRecon, 0, 100000);
		m_fMaxPatrolPullDistance = DCO_PF("MaxPatrolPullDistance", m_fMaxPatrolPullDistance, 0, 100000);
		m_fAssaultSuppressDuration = DCO_PF("AssaultSuppressDuration", m_fAssaultSuppressDuration, 0, 100000);
		m_fRetreatCoverDuration = DCO_PF("RetreatCoverDuration", m_fRetreatCoverDuration, 0, 100000);
		m_fRetreatCoverRadius = DCO_PF("RetreatCoverRadius", m_fRetreatCoverRadius, 0, 100000);
		m_fTransportDistanceThreshold = DCO_PF("TransportDistanceThreshold", m_fTransportDistanceThreshold, 0, 100000);
		m_fVehicleSearchRadius = DCO_PF("VehicleSearchRadius", m_fVehicleSearchRadius, 25, 1000);
		m_fBaseRadius = DCO_PF("BaseRadius", m_fBaseRadius, 0, 100000);
		m_fMinGroupClusterDistance = DCO_PF("MinGroupClusterDistance", m_fMinGroupClusterDistance, 0, 100000);
	}

	protected void DCO_PresetSync2()
	{
		m_fPatrolFrontlineDrift = DCO_PF("PatrolFrontlineDrift", m_fPatrolFrontlineDrift, 0, 100000);
		m_fPatrolFrontlineStandoff = DCO_PF("PatrolFrontlineStandoff", m_fPatrolFrontlineStandoff, 0, 100000);
		m_bPatrolTerrainScoring = DCO_PB("PatrolTerrainScoring", m_bPatrolTerrainScoring);
		m_iPatrolSmartBudget = DCO_PI("PatrolSmartBudget", m_iPatrolSmartBudget, 0, 100000);
		m_fPatrolMemoryRadius = DCO_PF("PatrolMemoryRadius", m_fPatrolMemoryRadius, 0, 100000);
		m_iPatrolMemorySize = DCO_PI("PatrolMemorySize", m_iPatrolMemorySize, 0, 100000);
		m_fPatrolRadiusMulMin = DCO_PF("PatrolRadiusMulMin", m_fPatrolRadiusMulMin, 0.2, 2);
		m_fPatrolRadiusMulMax = DCO_PF("PatrolRadiusMulMax", m_fPatrolRadiusMulMax, 0.2, 3);
	}

	protected void DCO_PresetSync3()
	{
		m_fFrontlineNodeBias = DCO_PF("FrontlineNodeBias", m_fFrontlineNodeBias, 0.1, 0.9);
		m_fEnvelopeArcDeg = DCO_PF("EnvelopeArcDeg", m_fEnvelopeArcDeg, 0, 100000);
		m_fEnvelopeRadiusMul = DCO_PF("EnvelopeRadiusMul", m_fEnvelopeRadiusMul, 0, 100000);
		m_fEnvelopeMinRadius = DCO_PF("EnvelopeMinRadius", m_fEnvelopeMinRadius, 0, 100000);
		m_iEnvelopeNodes = DCO_PI("EnvelopeNodes", m_iEnvelopeNodes, 0, 100000);
	}

	protected void DCO_PresetSync4()
	{
		m_fStagingLegFraction = DCO_PF("StagingLegFraction", m_fStagingLegFraction, 0.05, 1);
		m_fStagingMinDistance = DCO_PF("StagingMinDistance", m_fStagingMinDistance, 0, 100000);
		m_fStagingMaxDistance = DCO_PF("StagingMaxDistance", m_fStagingMaxDistance, 0, 100000);
		m_fStagingMargin = DCO_PF("StagingMargin", m_fStagingMargin, 0, 100000);
		m_fWaypointLegDistance = DCO_PF("WaypointLegDistance", m_fWaypointLegDistance, 0, 100000);
		m_fWaypointNudgeRadius = DCO_PF("WaypointNudgeRadius", m_fWaypointNudgeRadius, 0, 100000);
		m_iMaxSearchWaypoints = DCO_PI("MaxSearchWaypoints", m_iMaxSearchWaypoints, 0, 100000);
	}

	protected void DCO_PresetSync5()
	{
		m_fReservePercent = DCO_PF("ReservePercent", m_fReservePercent, 0, 50);
		m_bAllIn = DCO_PB("AllIn", m_bAllIn);
		m_bAllInRearDefenders = DCO_PB("AllInRearDefenders", m_bAllInRearDefenders);
	}

	protected void DCO_PresetSync6()
	{
		m_bEnableKeyDefendSpotEvaluation = DCO_PB("EnableKeyDefendSpotEvaluation", m_bEnableKeyDefendSpotEvaluation);
		m_iMaxKeyDefendPositions = DCO_PI("MaxKeyDefendPositions", m_iMaxKeyDefendPositions, 0, 100000);
		m_iMinSector = DCO_PI("MinSector", m_iMinSector, 0, 100000);
		m_iMaxSector = DCO_PI("MaxSector", m_iMaxSector, 0, 100000);
		m_fArcPerSector = DCO_PF("ArcPerSector", m_fArcPerSector, 0, 100000);
		m_bScaleSectorByPersonality = DCO_PB("ScaleSectorByPersonality", m_bScaleSectorByPersonality);
		m_fSectorPersonalityMin = DCO_PF("SectorPersonalityMin", m_fSectorPersonalityMin, 0.25, 2.0);
		m_fSectorPersonalityMax = DCO_PF("SectorPersonalityMax", m_fSectorPersonalityMax, 0.25, 2.0);
		m_fMaxCompletionRadius = DCO_PF("MaxCompletionRadius", m_fMaxCompletionRadius, 0, 100000);
		m_bGateDefendByManpower = DCO_PB("GateDefendByManpower", m_bGateDefendByManpower);
		m_fThreatDirectionRadius = DCO_PF("ThreatDirectionRadius", m_fThreatDirectionRadius, 0, 100000);
	}

	protected void DCO_PresetSync7()
	{
		m_eCommanderModeExternal = DCO_PI("CommanderMode", m_eCommanderModeExternal, 0, 2);
		m_fAggression = DCO_PF("Aggression", m_fAggression, 0, 1);
		m_fAdaptability = DCO_PF("Adaptability", m_fAdaptability, 0, 1);
		m_fRiskTaking = DCO_PF("RiskTaking", m_fRiskTaking, 0, 1);
		m_fResilience = DCO_PF("Resilience", m_fResilience, 0, 1);
		m_fPatience = DCO_PF("Patience", m_fPatience, 0, 1);
		m_fCombatFocus = DCO_PF("CombatFocus", m_fCombatFocus, 0, 1);
		m_bRandomPersonality = DCO_PB("RandomPersonality", m_bRandomPersonality);
		m_sPersonalityPreset = DCO_PS("PersonalityPreset", m_sPersonalityPreset);
		float intel = DCO_PF("TraitIntel", m_fTraitIntel, 0, 2);
		float strategy = DCO_PF("TraitStrategy", m_fTraitStrategy, 0, 2);
		float aggression = DCO_PF("TraitAggression", m_fTraitAggression, 0, 2);
		float logistics = DCO_PF("TraitLogistics", m_fTraitLogistics, 0, 2);
		float support = DCO_PF("TraitSupport", m_fTraitSupport, 0, 2);
		float leadership = DCO_PF("TraitLeadership", m_fTraitLeadership, 0, 2);
		int courage = DCO_PI("Courage", m_iCourage, 0, 4);
		int decision = DCO_PI("Decision", m_iDecision, 0, 2);
		m_ePreBattleIntel = DCO_PI("PreBattleIntel", m_ePreBattleIntel, 0, 3);
		string docPreset = DCO_PS("DoctrinePreset", m_sDoctrinePreset);
		string docOverride = DCO_PS("DoctrineOverride", DCO_DocOverrideText());
		if (m_iDCOPMode != 1)
		{
			SetTraits(intel, strategy, aggression, logistics, support, leadership, courage, decision);
			m_sDoctrinePreset = docPreset;
			DCO_SetDocOverrideText(docOverride);
		}
		m_fMoraleAttackThreshold = DCO_PF("MoraleAttackThreshold", m_fMoraleAttackThreshold, 0, 100000);
	}

	protected void DCO_PresetSync8()
	{
		m_iAllowedTactics = DCO_PI("AllowedTactics", m_iAllowedTactics, 0, 8191);
		m_iAllowedDefenseTactics = DCO_PI("AllowedDefenseTactics", m_iAllowedDefenseTactics, 0, 255);
	}

	protected void DCO_PresetSync9()
	{
		m_fReconRevealInterval = DCO_PF("ReconRevealInterval", m_fReconRevealInterval, 0, 100000);
	}

	protected void DCO_PresetSync10()
	{
		m_fPatrolLinkRadius = DCO_PF("PatrolLinkRadius", m_fPatrolLinkRadius, 0, 100000);
		m_fLinkPatrolChance = DCO_PF("LinkPatrolChance", m_fLinkPatrolChance, 0, 1);
	}

	protected void DCO_PresetSync11()
	{
		m_bDebugMode = DCO_PB("DebugMode", m_bDebugMode);
		m_fDebugRefreshInterval = DCO_PF("DebugRefreshInterval", m_fDebugRefreshInterval, 0, 100000);
	}

	protected void DCO_PresetSync12()
	{
		m_bEnablePreemption = DCO_PB("EnablePreemption", m_bEnablePreemption);
		m_fPreemptionMargin = DCO_PF("PreemptionMargin", m_fPreemptionMargin, 0, 100000);
		m_fPreemptionCooldown = DCO_PF("PreemptionCooldown", m_fPreemptionCooldown, 0, 100000);
	}

	protected void DCO_PresetSync13()
	{
		m_bPlayerTasking = DCO_PB("PlayerTasking", m_bPlayerTasking);
		m_bPlayerTaskingCounted = DCO_PB("PlayerTaskingCounted", m_bPlayerTaskingCounted);
		m_iPlayerMaxPerObjective = DCO_PI("PlayerMaxPerObjective", m_iPlayerMaxPerObjective, 0, 100000);
		m_fPlayerAcceptTime = DCO_PF("PlayerAcceptTime", m_fPlayerAcceptTime, 30, 300);
		m_fPlayerEtaFactor = DCO_PF("PlayerEtaFactor", m_fPlayerEtaFactor, 0, 100000);
		m_fPlayerEtaMargin = DCO_PF("PlayerEtaMargin", m_fPlayerEtaMargin, 0, 100000);
		m_fPlayerMaxEtaMin = DCO_PF("PlayerMaxEtaMin", m_fPlayerMaxEtaMin, 2, 30);
		m_bPlayerEarlyRelease = DCO_PB("PlayerEarlyRelease", m_bPlayerEarlyRelease);
		m_iPlayerFailLimit = DCO_PI("PlayerFailLimit", m_iPlayerFailLimit, 0, 100000);
		m_fPlayerBonusMinutes = DCO_PF("PlayerBonusMinutes", m_fPlayerBonusMinutes, 0, 100000);
	}

	protected void DCO_PresetSync14()
	{
		m_bLogiMedevac = DCO_PB("LogiMedevac", m_bLogiMedevac);
		m_bLogiExtract = DCO_PB("LogiExtract", m_bLogiExtract);
		m_bLogiResupply = DCO_PB("LogiResupply", m_bLogiResupply);
		m_bLogiEscort = DCO_PB("LogiEscort", m_bLogiEscort);
		m_iLogiPrioMedevac = DCO_PI("LogiPrioMedevac", m_iLogiPrioMedevac, 0, 100000);
		m_iLogiPrioExtract = DCO_PI("LogiPrioExtract", m_iLogiPrioExtract, 0, 100000);
		m_iLogiPrioReinforce = DCO_PI("LogiPrioReinforce", m_iLogiPrioReinforce, 0, 100000);
		m_iLogiPrioDeploy = DCO_PI("LogiPrioDeploy", m_iLogiPrioDeploy, 0, 100000);
		m_iLogiPrioRedeploy = DCO_PI("LogiPrioRedeploy", m_iLogiPrioRedeploy, 0, 100000);
		m_iLogiPrioResupply = DCO_PI("LogiPrioResupply", m_iLogiPrioResupply, 0, 100000);
		m_iLogiPrioPlayer = DCO_PI("LogiPrioPlayer", m_iLogiPrioPlayer, 0, 100000);
		m_fLogiEtaMargin = DCO_PF("LogiEtaMargin", m_fLogiEtaMargin, 0, 0.9);
		m_fLogiBatchRadius = DCO_PF("LogiBatchRadius", m_fLogiBatchRadius, 0, 100000);
		m_fLogiBatchWindow = DCO_PF("LogiBatchWindow", m_fLogiBatchWindow, 0, 100000);
		m_fLogiLZSafeDist = DCO_PF("LogiLZSafeDist", m_fLogiLZSafeDist, 0, 100000);
		m_fLogiTreatTime = DCO_PF("LogiTreatTime", m_fLogiTreatTime, 0, 100000);
		m_fLogiExtractMinDist = DCO_PF("LogiExtractMinDist", m_fLogiExtractMinDist, 0, 100000);
		m_iLogiLowAmmoMags = DCO_PI("LogiLowAmmoMags", m_iLogiLowAmmoMags, 0, 100000);
		m_fLogiHubRearDist = DCO_PF("LogiHubRearDist", m_fLogiHubRearDist, 0, 100000);
	}

	protected void DCO_PresetSync15()
	{
		m_fAttackRatioAggressive = DCO_PF("AttackRatioAggressive", m_fAttackRatioAggressive, 0, 100000);
		m_fAttackRatioCautious = DCO_PF("AttackRatioCautious", m_fAttackRatioCautious, 0, 100000);
		m_fAttackAbortCooldown = DCO_PF("AttackAbortCooldown", m_fAttackAbortCooldown, 0, 100000);
	}

	protected void DCO_PresetSync16()
	{
		m_bAmbush = DCO_PB("Ambush", m_bAmbush);
		m_bAmbushMSR = DCO_PB("AmbushMSR", m_bAmbushMSR);
		m_iMaxAmbushes = DCO_PI("MaxAmbushes", m_iMaxAmbushes, 0, 100000);
		m_fAmbushKillRadius = DCO_PF("AmbushKillRadius", m_fAmbushKillRadius, 0, 100000);
		m_fAmbushMaxWait = DCO_PF("AmbushMaxWait", m_fAmbushMaxWait, 0, 100000);
		m_fAmbushFireTime = DCO_PF("AmbushFireTime", m_fAmbushFireTime, 0, 100000);
		m_bOP = DCO_PB("OP", m_bOP);
		m_iMaxOP = DCO_PI("MaxOP", m_iMaxOP, 0, 100000);
		m_fOPWithdrawDist = DCO_PF("OPWithdrawDist", m_fOPWithdrawDist, 0, 100000);
		m_fOPRotateTime = DCO_PF("OPRotateTime", m_fOPRotateTime, 0, 100000);
		m_bCounterattack = DCO_PB("Counterattack", m_bCounterattack);
		m_fCounterattackWindow = DCO_PF("CounterattackWindow", m_fCounterattackWindow, 0, 100000);
		m_bCounterBattery = DCO_PB("CounterBattery", m_bCounterBattery);
		m_fCBInitialError = DCO_PF("CBInitialError", m_fCBInitialError, 0, 100000);
		m_bFeint = DCO_PB("Feint", m_bFeint);
		m_fFeintLead = DCO_PF("FeintLead", m_fFeintLead, 0, 100000);
	}

	protected void DCO_PresetSyncAll(bool runtime)
	{
		DCO_PresetSync0();
		DCO_PresetSync1();
		DCO_PresetSync2();
		DCO_PresetSync3();
		DCO_PresetSync4();
		DCO_PresetSync5();
		DCO_PresetSync6();
		DCO_PresetSync7();
		DCO_PresetSync8();
		DCO_PresetSync9();
		DCO_PresetSync10();
		DCO_PresetSync11();
		DCO_PresetSync12();
		DCO_PresetSync13();
		DCO_PresetSync14();
		DCO_PresetSync15();
		DCO_PresetSync16();
		DCO_PresetSyncManual(runtime);
	}
}
