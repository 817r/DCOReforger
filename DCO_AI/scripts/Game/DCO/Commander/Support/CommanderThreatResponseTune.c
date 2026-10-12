modded class CMD_ThreatResponseComponent
{
	float DCO_GT_MergeRadius()			{ return m_fMergeRadius; }
	void DCO_ST_MergeRadius(float v)		{ m_fMergeRadius = v; }
	float DCO_GT_ClusterRadius()			{ return m_fClusterRadius; }
	void DCO_ST_ClusterRadius(float v)		{ m_fClusterRadius = v; }
	float DCO_GT_EnemyIncrementedScore()			{ return m_iEnemyIncrementedScore; }
	void DCO_ST_EnemyIncrementedScore(float v)		{ m_iEnemyIncrementedScore = Math.Round(v); }
	float DCO_GT_StalenessThreshold()			{ return m_fStalenessThreshold; }
	void DCO_ST_StalenessThreshold(float v)		{ m_fStalenessThreshold = v; }
	float DCO_GT_UncertaintyGrowth()			{ return m_fUncertaintyGrowth; }
	void DCO_ST_UncertaintyGrowth(float v)		{ m_fUncertaintyGrowth = v; }
	float DCO_GT_ExtrapolateMax()			{ return m_fExtrapolateMax; }
	void DCO_ST_ExtrapolateMax(float v)		{ m_fExtrapolateMax = v; }
	float DCO_GT_ArtilleryAccuracy()			{ return m_fArtilleryAccuracy; }
	void DCO_ST_ArtilleryAccuracy(float v)		{ m_fArtilleryAccuracy = v; }
	float DCO_GT_LedgerTTL()			{ return m_fLedgerTTL; }
	void DCO_ST_LedgerTTL(float v)		{ m_fLedgerTTL = v; }
	float DCO_GT_ReinforceBehindDist()			{ return m_fReinforceBehindDist; }
	void DCO_ST_ReinforceBehindDist(float v)		{ m_fReinforceBehindDist = v; }
	float DCO_GT_ReinforceStandoffDist()			{ return m_fReinforceStandoffDist; }
	void DCO_ST_ReinforceStandoffDist(float v)		{ m_fReinforceStandoffDist = v; }
	float DCO_GT_FlankTaskExpiry()			{ return m_fFlankTaskExpiry; }
	void DCO_ST_FlankTaskExpiry(float v)		{ m_fFlankTaskExpiry = v; }
	float DCO_GT_RatioRadius()			{ return m_fRatioRadius; }
	void DCO_ST_RatioRadius(float v)		{ m_fRatioRadius = v; }
	float DCO_GT_WithdrawRatio()			{ return m_fWithdrawRatio; }
	void DCO_ST_WithdrawRatio(float v)		{ m_fWithdrawRatio = v; }
	float DCO_GT_MissionTimeout()			{ return m_fMissionTimeout; }
	void DCO_ST_MissionTimeout(float v)		{ m_fMissionTimeout = v; }
	float DCO_GT_HoldReleaseMax()			{ return m_fHoldReleaseMax; }
	void DCO_ST_HoldReleaseMax(float v)		{ m_fHoldReleaseMax = v; }
	float DCO_GT_BlockingDist()			{ return m_fBlockingDist; }
	void DCO_ST_BlockingDist(float v)		{ m_fBlockingDist = v; }
	bool DCO_GT_UpgradeGarrisonMode()			{ return m_bUpgradeGarrisonMode; }
	void DCO_ST_UpgradeGarrisonMode(bool v)		{ m_bUpgradeGarrisonMode = v; }
	float DCO_GT_ProjectTime()			{ return m_fProjectTime; }
	void DCO_ST_ProjectTime(float v)		{ m_fProjectTime = v; }
}

