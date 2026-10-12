modded class AICommander_BaseComponent
{
	float DCO_GT_DelayFirstIteration()			{ return m_fDelayFirstIteration; }
	void DCO_ST_DelayFirstIteration(float v)		{ m_fDelayFirstIteration = v; }
	float DCO_GT_CaptureCheckInterval()			{ return m_fCaptureCheckInterval; }
	void DCO_ST_CaptureCheckInterval(float v)		{ m_fCaptureCheckInterval = v; }
	float DCO_GT_StalemateResponseCooldown()
	{
		if (m_fBaseThinkInterval > 0)
			return m_fBaseStalemateResponseCooldown;
		return m_fStalemateResponseCooldown;
	}

	void DCO_ST_StalemateResponseCooldown(float v)
	{
		if (m_fBaseThinkInterval <= 0)
		{
			m_fStalemateResponseCooldown = v;
			return;
		}
		m_fBaseStalemateResponseCooldown = v;
		m_fStalemateResponseCooldown = v * Math.Lerp(m_fStalemateScaleImpatient, m_fStalemateScalePatient, m_fPatience);
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("cmd_stalemate_cooldown_set cmd=%1 base_s=%2 effective_s=%3", m_sCommanderUID, Math.Round(v), Math.Round(m_fStalemateResponseCooldown)));
	}
	bool DCO_GT_EnablePreemption()			{ return m_bEnablePreemption; }
	void DCO_ST_EnablePreemption(bool v)		{ m_bEnablePreemption = v; }
	float DCO_GT_PreemptionMargin()			{ return m_fPreemptionMargin; }
	void DCO_ST_PreemptionMargin(float v)		{ m_fPreemptionMargin = v; }
	float DCO_GT_PreemptionCooldown()			{ return m_fPreemptionCooldown; }
	void DCO_ST_PreemptionCooldown(float v)		{ m_fPreemptionCooldown = v; }
	float DCO_GT_ReinforcementStock()			{ return m_iReinforcementStock; }
	void DCO_ST_ReinforcementStock(float v)		{ m_iReinforcementStock = Math.Round(v); }
	bool DCO_GT_PlayerTasking()			{ return m_bPlayerTasking; }
	void DCO_ST_PlayerTasking(bool v)		{ m_bPlayerTasking = v; }
	bool DCO_GT_PlayerTaskingCounted()			{ return m_bPlayerTaskingCounted; }
	void DCO_ST_PlayerTaskingCounted(bool v)		{ m_bPlayerTaskingCounted = v; }
	float DCO_GT_PlayerMaxPerObjective()			{ return m_iPlayerMaxPerObjective; }
	void DCO_ST_PlayerMaxPerObjective(float v)		{ m_iPlayerMaxPerObjective = Math.Round(v); }
	float DCO_GT_PlayerEtaFactor()			{ return m_fPlayerEtaFactor; }
	void DCO_ST_PlayerEtaFactor(float v)		{ m_fPlayerEtaFactor = v; }
	float DCO_GT_PlayerEtaMargin()			{ return m_fPlayerEtaMargin; }
	void DCO_ST_PlayerEtaMargin(float v)		{ m_fPlayerEtaMargin = v; }
	bool DCO_GT_PlayerEarlyRelease()			{ return m_bPlayerEarlyRelease; }
	void DCO_ST_PlayerEarlyRelease(bool v)		{ m_bPlayerEarlyRelease = v; }
	float DCO_GT_PlayerFailLimit()			{ return m_iPlayerFailLimit; }
	void DCO_ST_PlayerFailLimit(float v)		{ m_iPlayerFailLimit = Math.Round(v); }
	float DCO_GT_PlayerBonusMinutes()			{ return m_fPlayerBonusMinutes; }
	void DCO_ST_PlayerBonusMinutes(float v)		{ m_fPlayerBonusMinutes = v; }
	float DCO_GT_MoraleAttackThreshold()			{ return m_fMoraleAttackThreshold; }
	void DCO_ST_MoraleAttackThreshold(float v)		{ m_fMoraleAttackThreshold = v; }
	bool DCO_GT_UseSynchronizedAttack()			{ return m_bUseSynchronizedAttack; }
	void DCO_ST_UseSynchronizedAttack(bool v)		{ m_bUseSynchronizedAttack = v; }
	float DCO_GT_SyncAttackMaxWaitTime()			{ return m_fSyncAttackMaxWaitTime; }
	void DCO_ST_SyncAttackMaxWaitTime(float v)		{ m_fSyncAttackMaxWaitTime = v; }
	float DCO_GT_ReconWaitTimeout()			{ return m_fReconWaitTimeout; }
	void DCO_ST_ReconWaitTimeout(float v)		{ m_fReconWaitTimeout = v; }
	float DCO_GT_AttackRatioAggressive()			{ return m_fAttackRatioAggressive; }
	void DCO_ST_AttackRatioAggressive(float v)		{ m_fAttackRatioAggressive = v; }
	float DCO_GT_AttackRatioCautious()			{ return m_fAttackRatioCautious; }
	void DCO_ST_AttackRatioCautious(float v)		{ m_fAttackRatioCautious = v; }
	float DCO_GT_AttackAbortCooldown()			{ return m_fAttackAbortCooldown; }
	void DCO_ST_AttackAbortCooldown(float v)		{ m_fAttackAbortCooldown = v; }
	float DCO_GT_AssaultSuppressDuration()			{ return m_fAssaultSuppressDuration; }
	void DCO_ST_AssaultSuppressDuration(float v)		{ m_fAssaultSuppressDuration = v; }
	float DCO_GT_StagingLegFraction()			{ return m_fStagingLegFraction; }
	void DCO_ST_StagingLegFraction(float v)		{ m_fStagingLegFraction = v; }
	float DCO_GT_StagingMinDistance()			{ return m_fStagingMinDistance; }
	void DCO_ST_StagingMinDistance(float v)		{ m_fStagingMinDistance = v; }
	float DCO_GT_StagingMaxDistance()			{ return m_fStagingMaxDistance; }
	void DCO_ST_StagingMaxDistance(float v)		{ m_fStagingMaxDistance = v; }
	float DCO_GT_StagingMargin()			{ return m_fStagingMargin; }
	void DCO_ST_StagingMargin(float v)		{ m_fStagingMargin = v; }
	float DCO_GT_WaypointLegDistance()			{ return m_fWaypointLegDistance; }
	void DCO_ST_WaypointLegDistance(float v)		{ m_fWaypointLegDistance = v; }
	float DCO_GT_WaypointNudgeRadius()			{ return m_fWaypointNudgeRadius; }
	void DCO_ST_WaypointNudgeRadius(float v)		{ m_fWaypointNudgeRadius = v; }
	float DCO_GT_MaxSearchWaypoints()			{ return m_iMaxSearchWaypoints; }
	void DCO_ST_MaxSearchWaypoints(float v)		{ m_iMaxSearchWaypoints = Math.Round(v); }
	float DCO_GT_RetreatThreshold()			{ return m_iRetreatThreshold; }
	void DCO_ST_RetreatThreshold(float v)		{ m_iRetreatThreshold = Math.Round(v); }
	float DCO_GT_RetreatCoverDuration()			{ return m_fRetreatCoverDuration; }
	void DCO_ST_RetreatCoverDuration(float v)		{ m_fRetreatCoverDuration = v; }
	float DCO_GT_RetreatCoverRadius()			{ return m_fRetreatCoverRadius; }
	void DCO_ST_RetreatCoverRadius(float v)		{ m_fRetreatCoverRadius = v; }
	float DCO_GT_FrontlineReconRadius()			{ return m_fFrontlineReconRadius; }
	void DCO_ST_FrontlineReconRadius(float v)		{ m_fFrontlineReconRadius = v; }
	float DCO_GT_FrontlineReconDuration()			{ return m_fFrontlineReconDuration; }
	void DCO_ST_FrontlineReconDuration(float v)		{ m_fFrontlineReconDuration = v; }
	float DCO_GT_ReconRevealInterval()			{ return m_fReconRevealInterval; }
	void DCO_ST_ReconRevealInterval(float v)		{ m_fReconRevealInterval = v; }
	float DCO_GT_FrontlineNodeBias()			{ return m_fFrontlineNodeBias; }
	void DCO_ST_FrontlineNodeBias(float v)		{ m_fFrontlineNodeBias = v; }
	float DCO_GT_EnvelopeArcDeg()			{ return m_fEnvelopeArcDeg; }
	void DCO_ST_EnvelopeArcDeg(float v)		{ m_fEnvelopeArcDeg = v; }
	float DCO_GT_EnvelopeRadiusMul()			{ return m_fEnvelopeRadiusMul; }
	void DCO_ST_EnvelopeRadiusMul(float v)		{ m_fEnvelopeRadiusMul = v; }
	float DCO_GT_EnvelopeMinRadius()			{ return m_fEnvelopeMinRadius; }
	void DCO_ST_EnvelopeMinRadius(float v)		{ m_fEnvelopeMinRadius = v; }
	float DCO_GT_EnvelopeNodes()			{ return m_iEnvelopeNodes; }
	void DCO_ST_EnvelopeNodes(float v)		{ m_iEnvelopeNodes = Math.Round(v); }
	float DCO_GT_MinGroupClusterDistance()			{ return m_fMinGroupClusterDistance; }
	void DCO_ST_MinGroupClusterDistance(float v)		{ m_fMinGroupClusterDistance = v; }
	float DCO_GT_GarrisonGroupsPerObjective()			{ return m_iGarrisonGroupsPerObjective; }
	void DCO_ST_GarrisonGroupsPerObjective(float v)		{ m_iGarrisonGroupsPerObjective = Math.Round(v); }
	float DCO_GT_DefendEngageDist()			{ return m_fDefendEngageDist; }
	void DCO_ST_DefendEngageDist(float v)
	{
		m_fDefendEngageDist = v;
		m_ROE = null;
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("cmd_defend_engage_set cmd=%1 dist=%2 roe_rebuild=1", m_sCommanderUID, Math.Round(v)));
	}
	float DCO_GT_DefendObjectiveMargin()			{ return m_fDefendObjectiveMargin; }
	void DCO_ST_DefendObjectiveMargin(float v)		{ m_fDefendObjectiveMargin = v; }
	float DCO_GT_BaseRadius()			{ return m_fBaseRadius; }
	void DCO_ST_BaseRadius(float v)		{ m_fBaseRadius = v; }
	bool DCO_GT_EnableKeyDefendSpotEvaluation()			{ return m_bEnableKeyDefendSpotEvaluation; }
	void DCO_ST_EnableKeyDefendSpotEvaluation(bool v)		{ m_bEnableKeyDefendSpotEvaluation = v; }
	float DCO_GT_MaxKeyDefendPositions()			{ return m_iMaxKeyDefendPositions; }
	void DCO_ST_MaxKeyDefendPositions(float v)		{ m_iMaxKeyDefendPositions = Math.Round(v); }
	float DCO_GT_MinSector()			{ return m_iMinSector; }
	void DCO_ST_MinSector(float v)		{ m_iMinSector = Math.Round(v); }
	float DCO_GT_MaxSector()			{ return m_iMaxSector; }
	void DCO_ST_MaxSector(float v)		{ m_iMaxSector = Math.Round(v); }
	float DCO_GT_ArcPerSector()			{ return m_fArcPerSector; }
	void DCO_ST_ArcPerSector(float v)		{ m_fArcPerSector = v; }
	bool DCO_GT_ScaleSectorByPersonality()			{ return m_bScaleSectorByPersonality; }
	void DCO_ST_ScaleSectorByPersonality(bool v)		{ m_bScaleSectorByPersonality = v; }
	float DCO_GT_SectorPersonalityMin()			{ return m_fSectorPersonalityMin; }
	void DCO_ST_SectorPersonalityMin(float v)		{ m_fSectorPersonalityMin = v; }
	float DCO_GT_SectorPersonalityMax()			{ return m_fSectorPersonalityMax; }
	void DCO_ST_SectorPersonalityMax(float v)		{ m_fSectorPersonalityMax = v; }
	float DCO_GT_MaxCompletionRadius()			{ return m_fMaxCompletionRadius; }
	void DCO_ST_MaxCompletionRadius(float v)		{ m_fMaxCompletionRadius = v; }
	bool DCO_GT_GateDefendByManpower()			{ return m_bGateDefendByManpower; }
	void DCO_ST_GateDefendByManpower(bool v)		{ m_bGateDefendByManpower = v; }
	float DCO_GT_ThreatDirectionRadius()			{ return m_fThreatDirectionRadius; }
	void DCO_ST_ThreatDirectionRadius(float v)		{ m_fThreatDirectionRadius = v; }
	float DCO_GT_FeintLead()			{ return m_fFeintLead; }
	void DCO_ST_FeintLead(float v)		{ m_fFeintLead = v; }
	float DCO_GT_MaxPatrolPullDistance()			{ return m_fMaxPatrolPullDistance; }
	void DCO_ST_MaxPatrolPullDistance(float v)		{ m_fMaxPatrolPullDistance = v; }
	float DCO_GT_PatrolFrontlineDrift()			{ return m_fPatrolFrontlineDrift; }
	void DCO_ST_PatrolFrontlineDrift(float v)		{ m_fPatrolFrontlineDrift = v; }
	float DCO_GT_PatrolFrontlineStandoff()			{ return m_fPatrolFrontlineStandoff; }
	void DCO_ST_PatrolFrontlineStandoff(float v)		{ m_fPatrolFrontlineStandoff = v; }
	bool DCO_GT_PatrolTerrainScoring()			{ return m_bPatrolTerrainScoring; }
	void DCO_ST_PatrolTerrainScoring(bool v)		{ m_bPatrolTerrainScoring = v; }
	float DCO_GT_PatrolSmartBudget()			{ return m_iPatrolSmartBudget; }
	void DCO_ST_PatrolSmartBudget(float v)		{ m_iPatrolSmartBudget = Math.Round(v); }
	float DCO_GT_PatrolMemoryRadius()			{ return m_fPatrolMemoryRadius; }
	void DCO_ST_PatrolMemoryRadius(float v)		{ m_fPatrolMemoryRadius = v; }
	float DCO_GT_PatrolMemorySize()			{ return m_iPatrolMemorySize; }
	void DCO_ST_PatrolMemorySize(float v)		{ m_iPatrolMemorySize = Math.Round(v); }
	float DCO_GT_PatrolRadiusMulMin()			{ return m_fPatrolRadiusMulMin; }
	void DCO_ST_PatrolRadiusMulMin(float v)		{ m_fPatrolRadiusMulMin = v; }
	float DCO_GT_PatrolRadiusMulMax()			{ return m_fPatrolRadiusMulMax; }
	void DCO_ST_PatrolRadiusMulMax(float v)		{ m_fPatrolRadiusMulMax = v; }
	float DCO_GT_PatrolLinkRadius()			{ return m_fPatrolLinkRadius; }
	void DCO_ST_PatrolLinkRadius(float v)		{ m_fPatrolLinkRadius = v; }
	float DCO_GT_LinkPatrolChance()			{ return m_fLinkPatrolChance; }
	void DCO_ST_LinkPatrolChance(float v)		{ m_fLinkPatrolChance = v; }
	float DCO_GT_TransportDistanceThreshold()			{ return m_fTransportDistanceThreshold; }
	void DCO_ST_TransportDistanceThreshold(float v)		{ m_fTransportDistanceThreshold = v; }
	float DCO_GT_LogiPrioMedevac()			{ return m_iLogiPrioMedevac; }
	void DCO_ST_LogiPrioMedevac(float v)		{ m_iLogiPrioMedevac = Math.Round(v); }
	float DCO_GT_LogiPrioExtract()			{ return m_iLogiPrioExtract; }
	void DCO_ST_LogiPrioExtract(float v)		{ m_iLogiPrioExtract = Math.Round(v); }
	float DCO_GT_LogiPrioReinforce()			{ return m_iLogiPrioReinforce; }
	void DCO_ST_LogiPrioReinforce(float v)		{ m_iLogiPrioReinforce = Math.Round(v); }
	float DCO_GT_LogiPrioDeploy()			{ return m_iLogiPrioDeploy; }
	void DCO_ST_LogiPrioDeploy(float v)		{ m_iLogiPrioDeploy = Math.Round(v); }
	float DCO_GT_LogiPrioRedeploy()			{ return m_iLogiPrioRedeploy; }
	void DCO_ST_LogiPrioRedeploy(float v)		{ m_iLogiPrioRedeploy = Math.Round(v); }
	float DCO_GT_LogiPrioResupply()			{ return m_iLogiPrioResupply; }
	void DCO_ST_LogiPrioResupply(float v)		{ m_iLogiPrioResupply = Math.Round(v); }
	float DCO_GT_LogiPrioPlayer()			{ return m_iLogiPrioPlayer; }
	void DCO_ST_LogiPrioPlayer(float v)		{ m_iLogiPrioPlayer = Math.Round(v); }
	float DCO_GT_LogiEtaMargin()			{ return m_fLogiEtaMargin; }
	void DCO_ST_LogiEtaMargin(float v)		{ m_fLogiEtaMargin = v; }
	float DCO_GT_LogiBatchRadius()			{ return m_fLogiBatchRadius; }
	void DCO_ST_LogiBatchRadius(float v)		{ m_fLogiBatchRadius = v; }
	float DCO_GT_LogiBatchWindow()			{ return m_fLogiBatchWindow; }
	void DCO_ST_LogiBatchWindow(float v)		{ m_fLogiBatchWindow = v; }
	float DCO_GT_LogiLZSafeDist()			{ return m_fLogiLZSafeDist; }
	void DCO_ST_LogiLZSafeDist(float v)		{ m_fLogiLZSafeDist = v; }
	float DCO_GT_LogiTreatTime()			{ return m_fLogiTreatTime; }
	void DCO_ST_LogiTreatTime(float v)		{ m_fLogiTreatTime = v; }
	float DCO_GT_LogiExtractMinDist()			{ return m_fLogiExtractMinDist; }
	void DCO_ST_LogiExtractMinDist(float v)		{ m_fLogiExtractMinDist = v; }
	float DCO_GT_LogiLowAmmoMags()			{ return m_iLogiLowAmmoMags; }
	void DCO_ST_LogiLowAmmoMags(float v)		{ m_iLogiLowAmmoMags = Math.Round(v); }
	float DCO_GT_LogiHubRearDist()			{ return m_fLogiHubRearDist; }
	void DCO_ST_LogiHubRearDist(float v)		{ m_fLogiHubRearDist = v; }
}

