modded class CMD_ArtillerySupport
{
	float DCO_GT_DispatchInterval()			{ return m_fDispatchInterval; }
	void DCO_ST_DispatchInterval(float v)		{ m_fDispatchInterval = v; }
	float DCO_GT_QueueExpiry()			{ return m_fQueueExpiry; }
	void DCO_ST_QueueExpiry(float v)		{ m_fQueueExpiry = v; }
	float DCO_GT_MaxQueueSize()			{ return m_iMaxQueueSize; }
	void DCO_ST_MaxQueueSize(float v)		{ m_iMaxQueueSize = Math.Round(v); }
	float DCO_GT_QueueOverloadPenalty()			{ return m_fQueueOverloadPenalty; }
	void DCO_ST_QueueOverloadPenalty(float v)		{ m_fQueueOverloadPenalty = v; }
	float DCO_GT_QueueProcessTime()			{ return m_fQueueProcessTime; }
	void DCO_ST_QueueProcessTime(float v)		{ m_fQueueProcessTime = v; }
	float DCO_GT_MinGlobalCooldown()			{ return m_fMinGlobalCooldown; }
	void DCO_ST_MinGlobalCooldown(float v)		{ m_fMinGlobalCooldown = v; }
	float DCO_GT_MaxGlobalCooldown()			{ return m_fMaxGlobalCooldown; }
	void DCO_ST_MaxGlobalCooldown(float v)		{ m_fMaxGlobalCooldown = v; }
	float DCO_GT_MinRange()			{ return m_fMinRange; }
	void DCO_ST_MinRange(float v)		{ m_fMinRange = v; }
	float DCO_GT_MaxRange()			{ return m_fMaxRange; }
	void DCO_ST_MaxRange(float v)		{ m_fMaxRange = v; }
	float DCO_GT_LethalRadius()			{ return m_fLethalRadius; }
	void DCO_ST_LethalRadius(float v)		{ m_fLethalRadius = v; }
	float DCO_GT_DangerCloseRadius()			{ return m_fDangerCloseRadius; }
	void DCO_ST_DangerCloseRadius(float v)		{ m_fDangerCloseRadius = v; }
	float DCO_GT_MinDispersion()			{ return m_fMinDispersion; }
	void DCO_ST_MinDispersion(float v)		{ m_fMinDispersion = v; }
	float DCO_GT_MaxDispersion()			{ return m_fMaxDispersion; }
	void DCO_ST_MaxDispersion(float v)		{ m_fMaxDispersion = v; }
	float DCO_GT_DispersionRangeScale()			{ return m_fDispersionRangeScale; }
	void DCO_ST_DispersionRangeScale(float v)		{ m_fDispersionRangeScale = v; }
	float DCO_GT_ClusterRatio()			{ return m_fClusterRatio; }
	void DCO_ST_ClusterRatio(float v)		{ m_fClusterRatio = v; }
	float DCO_GT_CorrectionRadius()			{ return m_fCorrectionRadius; }
	void DCO_ST_CorrectionRadius(float v)		{ m_fCorrectionRadius = v; }
	float DCO_GT_CorrectionExpiry()			{ return m_fCorrectionExpiry; }
	void DCO_ST_CorrectionExpiry(float v)		{ m_fCorrectionExpiry = v; }
	float DCO_GT_MaxCorrections()			{ return m_iMaxCorrections; }
	void DCO_ST_MaxCorrections(float v)		{ m_iMaxCorrections = Math.Round(v); }
	float DCO_GT_CorrectionBonusPerStep()			{ return m_fCorrectionBonusPerStep; }
	void DCO_ST_CorrectionBonusPerStep(float v)		{ m_fCorrectionBonusPerStep = v; }
}

