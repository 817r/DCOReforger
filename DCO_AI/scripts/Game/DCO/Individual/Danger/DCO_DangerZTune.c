[BaseContainerProps()]
modded class SCR_AIDangerReaction_WeaponFired
{
	protected DCO_GlobalAIComponent m_DCOTuneSrc_WF;
	protected int m_iDCOTuneVer_WF = -1;
	protected int m_iDCOTuneAll_WF = -1;
	protected bool m_bDCOTuneFac_WF;
	protected ref map<string, float> m_mDCOTuneOrig_WF;

	protected float DCO_Orig_WF(string key, float cur)
	{
		if (!m_mDCOTuneOrig_WF)
			m_mDCOTuneOrig_WF = new map<string, float>();
		float v;
		if (m_mDCOTuneOrig_WF.Find(key, v))
			return v;
		m_mDCOTuneOrig_WF.Set(key, cur);
		return cur;
	}

	protected float DCO_OrigB_WF(string key, bool cur)
	{
		float c = 0;
		if (cur)
			c = 1;
		return DCO_Orig_WF(key, c);
	}

	void DCO_TuneApply_WF(SCR_AIUtilityComponent utility)
	{
		bool fac = DCO_GlobalAIComponent.DCO_FactionActive();
		int all = DCO_GlobalAIComponent.DCO_TuneVerAll();
		if (!fac && !m_bDCOTuneFac_WF && all == m_iDCOTuneAll_WF)
			return;
		m_bDCOTuneFac_WF = fac;
		m_iDCOTuneAll_WF = all;
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(utility);
		if (!g || (g == m_DCOTuneSrc_WF && g.DCO_TuneVer() == m_iDCOTuneVer_WF))
			return;
		m_DCOTuneSrc_WF = g;
		m_iDCOTuneVer_WF = g.DCO_TuneVer();
		m_bUseRealisticReaction = g.DCO_TuneB("wf.UseRealisticReaction", DCO_OrigB_WF("wf.UseRealisticReaction", m_bUseRealisticReaction));
		m_fSuppressedAudibleRange = g.DCO_TuneF("wf.SuppressedAudibleRange", DCO_Orig_WF("wf.SuppressedAudibleRange", m_fSuppressedAudibleRange));
		m_fInVehicleHearingFactor = g.DCO_TuneF("wf.InVehicleHearingFactor", DCO_Orig_WF("wf.InVehicleHearingFactor", m_fInVehicleHearingFactor));
		m_fAudibleGuaranteeThreshold = g.DCO_TuneF("wf.AudibleGuaranteeThreshold", DCO_Orig_WF("wf.AudibleGuaranteeThreshold", m_fAudibleGuaranteeThreshold));
		m_fDirectionErrorBaseDeg = g.DCO_TuneF("wf.DirectionErrorBaseDeg", DCO_Orig_WF("wf.DirectionErrorBaseDeg", m_fDirectionErrorBaseDeg));
		m_fDirectionErrorExtraDeg = g.DCO_TuneF("wf.DirectionErrorExtraDeg", DCO_Orig_WF("wf.DirectionErrorExtraDeg", m_fDirectionErrorExtraDeg));
		m_fLookDistFull = g.DCO_TuneF("wf.LookDistFull", DCO_Orig_WF("wf.LookDistFull", m_fLookDistFull));
		m_fLookDistMax = g.DCO_TuneF("wf.LookDistMax", DCO_Orig_WF("wf.LookDistMax", m_fLookDistMax));
		m_fLookChanceAtMax = g.DCO_TuneF("wf.LookChanceAtMax", DCO_Orig_WF("wf.LookChanceAtMax", m_fLookChanceAtMax));
		m_fLookThreatBonus = g.DCO_TuneF("wf.LookThreatBonus", DCO_Orig_WF("wf.LookThreatBonus", m_fLookThreatBonus));
		m_fLookDelayMin = g.DCO_TuneF("wf.LookDelayMin", DCO_Orig_WF("wf.LookDelayMin", m_fLookDelayMin));
		m_fLookDelayMax = g.DCO_TuneF("wf.LookDelayMax", DCO_Orig_WF("wf.LookDelayMax", m_fLookDelayMax));
		m_bLookPreferClosest = g.DCO_TuneB("wf.LookPreferClosest", DCO_OrigB_WF("wf.LookPreferClosest", m_bLookPreferClosest));
		m_fLookLockDuration = g.DCO_TuneF("wf.LookLockDuration", DCO_Orig_WF("wf.LookLockDuration", m_fLookLockDuration));
		m_fLookOverrideDistRatio = g.DCO_TuneF("wf.LookOverrideDistRatio", DCO_Orig_WF("wf.LookOverrideDistRatio", m_fLookOverrideDistRatio));
		m_fLookLockRefreshInterval = g.DCO_TuneF("wf.LookLockRefreshInterval", DCO_Orig_WF("wf.LookLockRefreshInterval", m_fLookLockRefreshInterval));
		m_fLookPriorityProximityBonus = g.DCO_TuneF("wf.LookPriorityProximityBonus", DCO_Orig_WF("wf.LookPriorityProximityBonus", m_fLookPriorityProximityBonus));
		m_fCoverThreatGate = g.DCO_TuneF("wf.CoverThreatGate", DCO_Orig_WF("wf.CoverThreatGate", m_fCoverThreatGate));
		m_bLowerStanceOnLowThreat = g.DCO_TuneB("wf.LowerStanceOnLowThreat", DCO_OrigB_WF("wf.LowerStanceOnLowThreat", m_bLowerStanceOnLowThreat));
		m_fCoverDelayMin = g.DCO_TuneF("wf.CoverDelayMin", DCO_Orig_WF("wf.CoverDelayMin", m_fCoverDelayMin));
		m_fCoverDelayMax = g.DCO_TuneF("wf.CoverDelayMax", DCO_Orig_WF("wf.CoverDelayMax", m_fCoverDelayMax));
		m_fHighThreatProneThreshold = g.DCO_TuneF("wf.HighThreatProneThreshold", DCO_Orig_WF("wf.HighThreatProneThreshold", m_fHighThreatProneThreshold));
		m_fCoverSearchDistCap = g.DCO_TuneF("wf.CoverSearchDistCap", DCO_Orig_WF("wf.CoverSearchDistCap", m_fCoverSearchDistCap));
		m_fSharedMoveCooldown = g.DCO_TuneF("wf.SharedMoveCooldown", DCO_Orig_WF("wf.SharedMoveCooldown", m_fSharedMoveCooldown));
		m_bCoverPreferBuilding = g.DCO_TuneB("wf.CoverPreferBuilding", DCO_OrigB_WF("wf.CoverPreferBuilding", m_bCoverPreferBuilding));
		m_bCoverVeryHighPriority = g.DCO_TuneB("wf.CoverVeryHighPriority", DCO_OrigB_WF("wf.CoverVeryHighPriority", m_bCoverVeryHighPriority));
		m_bUseDodgeChanceOnCover = g.DCO_TuneB("wf.UseDodgeChanceOnCover", DCO_OrigB_WF("wf.UseDodgeChanceOnCover", m_bUseDodgeChanceOnCover));
		m_bLookAfterCover = g.DCO_TuneB("wf.LookAfterCover", DCO_OrigB_WF("wf.LookAfterCover", m_bLookAfterCover));
		m_iLookAfterCoverRepeats = g.DCO_TuneI("wf.LookAfterCoverRepeats", DCO_Orig_WF("wf.LookAfterCoverRepeats", m_iLookAfterCoverRepeats));
		m_fLookAfterCoverInterval = g.DCO_TuneF("wf.LookAfterCoverInterval", DCO_Orig_WF("wf.LookAfterCoverInterval", m_fLookAfterCoverInterval));
		m_fLookAfterCoverDelay = g.DCO_TuneF("wf.LookAfterCoverDelay", DCO_Orig_WF("wf.LookAfterCoverDelay", m_fLookAfterCoverDelay));
		m_bRandomizeCoverDirection = g.DCO_TuneB("wf.RandomizeCoverDirection", DCO_OrigB_WF("wf.RandomizeCoverDirection", m_bRandomizeCoverDirection));
		m_fCoverDirWeightBackward = g.DCO_TuneF("wf.CoverDirWeightBackward", DCO_Orig_WF("wf.CoverDirWeightBackward", m_fCoverDirWeightBackward));
		m_fCoverDirWeightLeft = g.DCO_TuneF("wf.CoverDirWeightLeft", DCO_Orig_WF("wf.CoverDirWeightLeft", m_fCoverDirWeightLeft));
		m_fCoverDirWeightRight = g.DCO_TuneF("wf.CoverDirWeightRight", DCO_Orig_WF("wf.CoverDirWeightRight", m_fCoverDirWeightRight));
		m_fCoverDirWeightAnywhere = g.DCO_TuneF("wf.CoverDirWeightAnywhere", DCO_Orig_WF("wf.CoverDirWeightAnywhere", m_fCoverDirWeightAnywhere));
		m_bScaleCoverDirByPersonality = g.DCO_TuneB("wf.ScaleCoverDirByPersonality", DCO_OrigB_WF("wf.ScaleCoverDirByPersonality", m_bScaleCoverDirByPersonality));
		m_fCoverDirForwardWeightScale = g.DCO_TuneF("wf.CoverDirForwardWeightScale", DCO_Orig_WF("wf.CoverDirForwardWeightScale", m_fCoverDirForwardWeightScale));
		m_bDodgeTowardWaypoint = g.DCO_TuneB("wf.DodgeTowardWaypoint", DCO_OrigB_WF("wf.DodgeTowardWaypoint", m_bDodgeTowardWaypoint));
		float a = g.DCO_Aggression();
		m_fCoverThreatGate = Math.Clamp(m_fCoverThreatGate * a, 0.05, 1);
		m_fCoverDirForwardWeightScale = m_fCoverDirForwardWeightScale * a;
	}

	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{
		DCO_TuneApply_WF(utility);
		return super.PerformReaction(utility, threatSystem, dangerEvent, dangerEventCount);
	}

	override protected void DCO_FireFlyby(SCR_AIUtilityComponent utility)
	{
		DCO_TuneApply_WF(utility);
		super.DCO_FireFlyby(utility);
	}

	override protected void DCO_FireHeard(SCR_AIUtilityComponent utility)
	{
		DCO_TuneApply_WF(utility);
		super.DCO_FireHeard(utility);
	}

	override protected void DCO_FireReact(SCR_AIUtilityComponent utility)
	{
		DCO_TuneApply_WF(utility);
		super.DCO_FireReact(utility);
	}

	override protected void LookAtShotLocked(SCR_AIUtilityComponent utility, vector perceivedShotPos, float distance, IEntity sourceEntity)
	{
		DCO_TuneApply_WF(utility);
		super.LookAtShotLocked(utility, perceivedShotPos, distance, sourceEntity);
	}

	override protected void RefreshLookLock(SCR_AIUtilityComponent utility, int token)
	{
		DCO_TuneApply_WF(utility);
		super.RefreshLookLock(utility, token);
	}

	override protected void LookAtShot(SCR_AIUtilityComponent utility, vector perceivedShotPos)
	{
		DCO_TuneApply_WF(utility);
		super.LookAtShot(utility, perceivedShotPos);
	}

	override protected void LookAfterCover(SCR_AIUtilityComponent utility, vector perceivedShotPos, int repeatsLeft)
	{
		DCO_TuneApply_WF(utility);
		super.LookAfterCover(utility, perceivedShotPos, repeatsLeft);
	}

	override protected void TryTakeCoverFromShot(SCR_AIUtilityComponent utility, vector perceivedShotPos, float distance, float threatScore)
	{
		DCO_TuneApply_WF(utility);
		super.TryTakeCoverFromShot(utility, perceivedShotPos, distance, threatScore);
	}

	override protected void TryDodge(SCR_AIUtilityComponent utility, vector shotPos, float distance)
	{
		DCO_TuneApply_WF(utility);
		super.TryDodge(utility, shotPos, distance);
	}
}

[BaseContainerProps()]
modded class SCR_AIDangerReaction_ProjectileHit
{
	protected DCO_GlobalAIComponent m_DCOTuneSrc_PH;
	protected int m_iDCOTuneVer_PH = -1;
	protected int m_iDCOTuneAll_PH = -1;
	protected bool m_bDCOTuneFac_PH;
	protected ref map<string, float> m_mDCOTuneOrig_PH;

	protected float DCO_Orig_PH(string key, float cur)
	{
		if (!m_mDCOTuneOrig_PH)
			m_mDCOTuneOrig_PH = new map<string, float>();
		float v;
		if (m_mDCOTuneOrig_PH.Find(key, v))
			return v;
		m_mDCOTuneOrig_PH.Set(key, cur);
		return cur;
	}

	protected float DCO_OrigB_PH(string key, bool cur)
	{
		float c = 0;
		if (cur)
			c = 1;
		return DCO_Orig_PH(key, c);
	}

	void DCO_TuneApply_PH(SCR_AIUtilityComponent utility)
	{
		bool fac = DCO_GlobalAIComponent.DCO_FactionActive();
		int all = DCO_GlobalAIComponent.DCO_TuneVerAll();
		if (!fac && !m_bDCOTuneFac_PH && all == m_iDCOTuneAll_PH)
			return;
		m_bDCOTuneFac_PH = fac;
		m_iDCOTuneAll_PH = all;
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(utility);
		if (!g || (g == m_DCOTuneSrc_PH && g.DCO_TuneVer() == m_iDCOTuneVer_PH))
			return;
		m_DCOTuneSrc_PH = g;
		m_iDCOTuneVer_PH = g.DCO_TuneVer();
		m_bReactToNearMiss = g.DCO_TuneB("ph.ReactToNearMiss", DCO_OrigB_PH("ph.ReactToNearMiss", m_bReactToNearMiss));
		m_iBulletsRepositionNoTarget = g.DCO_TuneI("ph.BulletsRepositionNoTarget", DCO_Orig_PH("ph.BulletsRepositionNoTarget", m_iBulletsRepositionNoTarget));
		m_iBulletsRepositionHeavy = g.DCO_TuneI("ph.BulletsRepositionHeavy", DCO_Orig_PH("ph.BulletsRepositionHeavy", m_iBulletsRepositionHeavy));
		m_iBulletsFleeNoCover = g.DCO_TuneI("ph.BulletsFleeNoCover", DCO_Orig_PH("ph.BulletsFleeNoCover", m_iBulletsFleeNoCover));
		m_iBulletsFleeEngaged = g.DCO_TuneI("ph.BulletsFleeEngaged", DCO_Orig_PH("ph.BulletsFleeEngaged", m_iBulletsFleeEngaged));
		m_bScaleThresholdsByPersonality = g.DCO_TuneB("ph.ScaleThresholdsByPersonality", DCO_OrigB_PH("ph.ScaleThresholdsByPersonality", m_bScaleThresholdsByPersonality));
		m_bDeploySmokeOnFlee = g.DCO_TuneB("ph.DeploySmokeOnFlee", DCO_OrigB_PH("ph.DeploySmokeOnFlee", m_bDeploySmokeOnFlee));
		m_bCoverPreferBuilding = g.DCO_TuneB("ph.CoverPreferBuilding", DCO_OrigB_PH("ph.CoverPreferBuilding", m_bCoverPreferBuilding));
		m_bCoverVeryHighPriority = g.DCO_TuneB("ph.CoverVeryHighPriority", DCO_OrigB_PH("ph.CoverVeryHighPriority", m_bCoverVeryHighPriority));
		m_bRespectOngoingMove = g.DCO_TuneB("ph.RespectOngoingMove", DCO_OrigB_PH("ph.RespectOngoingMove", m_bRespectOngoingMove));
		m_fNewThreatAngleDeg = g.DCO_TuneF("ph.NewThreatAngleDeg", DCO_Orig_PH("ph.NewThreatAngleDeg", m_fNewThreatAngleDeg));
		m_bSkipWhileCoverActive = g.DCO_TuneB("ph.SkipWhileCoverActive", DCO_OrigB_PH("ph.SkipWhileCoverActive", m_bSkipWhileCoverActive));
		m_fCoverMoveCooldown_s = g.DCO_TuneF("ph.CoverMoveCooldown_s", DCO_Orig_PH("ph.CoverMoveCooldown_s", m_fCoverMoveCooldown_s));
		m_fRepositionSearchDist = g.DCO_TuneF("ph.RepositionSearchDist", DCO_Orig_PH("ph.RepositionSearchDist", m_fRepositionSearchDist));
		m_fFleeSearchDist = g.DCO_TuneF("ph.FleeSearchDist", DCO_Orig_PH("ph.FleeSearchDist", m_fFleeSearchDist));
		m_bForceStandWhileMoving = g.DCO_TuneB("ph.ForceStandWhileMoving", DCO_OrigB_PH("ph.ForceStandWhileMoving", m_bForceStandWhileMoving));
		m_bSuppressWhileMoving = g.DCO_TuneB("ph.SuppressWhileMoving", DCO_OrigB_PH("ph.SuppressWhileMoving", m_bSuppressWhileMoving));
		m_bAimWhileMovingToCover = g.DCO_TuneB("ph.AimWhileMovingToCover", DCO_OrigB_PH("ph.AimWhileMovingToCover", m_bAimWhileMovingToCover));
		m_bUseDodgeChance = g.DCO_TuneB("ph.UseDodgeChance", DCO_OrigB_PH("ph.UseDodgeChance", m_bUseDodgeChance));
		m_bRandomizeCoverDirection = g.DCO_TuneB("ph.RandomizeCoverDirection", DCO_OrigB_PH("ph.RandomizeCoverDirection", m_bRandomizeCoverDirection));
		m_fCoverDirWeightBackward = g.DCO_TuneF("ph.CoverDirWeightBackward", DCO_Orig_PH("ph.CoverDirWeightBackward", m_fCoverDirWeightBackward));
		m_fCoverDirWeightLeft = g.DCO_TuneF("ph.CoverDirWeightLeft", DCO_Orig_PH("ph.CoverDirWeightLeft", m_fCoverDirWeightLeft));
		m_fCoverDirWeightRight = g.DCO_TuneF("ph.CoverDirWeightRight", DCO_Orig_PH("ph.CoverDirWeightRight", m_fCoverDirWeightRight));
		m_fCoverDirWeightAnywhere = g.DCO_TuneF("ph.CoverDirWeightAnywhere", DCO_Orig_PH("ph.CoverDirWeightAnywhere", m_fCoverDirWeightAnywhere));
		m_bScaleCoverDirByPersonality = g.DCO_TuneB("ph.ScaleCoverDirByPersonality", DCO_OrigB_PH("ph.ScaleCoverDirByPersonality", m_bScaleCoverDirByPersonality));
		m_bDodgeTowardWaypoint = g.DCO_TuneB("ph.DodgeTowardWaypoint", DCO_OrigB_PH("ph.DodgeTowardWaypoint", m_bDodgeTowardWaypoint));
		float a = g.DCO_Aggression();
		m_iBulletsRepositionNoTarget = Math.ClampInt(Math.Round(a * m_iBulletsRepositionNoTarget), 1, 99);
		m_iBulletsRepositionHeavy = Math.ClampInt(Math.Round(a * m_iBulletsRepositionHeavy), 1, 99);
		m_iBulletsFleeNoCover = Math.ClampInt(Math.Round(a * m_iBulletsFleeNoCover), 1, 99);
		m_iBulletsFleeEngaged = Math.ClampInt(Math.Round(a * m_iBulletsFleeEngaged), 1, 99);
	}

	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{
		DCO_TuneApply_PH(utility);
		return super.PerformReaction(utility, threatSystem, dangerEvent, dangerEventCount);
	}

	override protected void TrySuppressShooter(notnull SCR_AIUtilityComponent utility, IEntity shooterRoot, float shooterDistance, float duration, float interval)
	{
		DCO_TuneApply_PH(utility);
		super.TrySuppressShooter(utility, shooterRoot, shooterDistance, duration, interval);
	}
}

