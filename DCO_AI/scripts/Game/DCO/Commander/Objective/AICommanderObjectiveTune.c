modded class CMD_AICommanderObjectiveComponent
{
	float DCO_GT_BaseValue()			{ return m_fBaseValue; }
	void DCO_ST_BaseValue(float v)		{ m_fBaseValue = v; }
	float DCO_GT_MaxGroupCount()			{ return m_iMaxGroupCount; }
	void DCO_ST_MaxGroupCount(float v)		{ m_iMaxGroupCount = Math.Round(v); }
	float DCO_GT_DefendGroupCount()			{ return m_iDefendGroupCount; }
	void DCO_ST_DefendGroupCount(float v)		{ m_iDefendGroupCount = Math.Round(v); }
	float DCO_GT_ThreatRadius()			{ return m_fThreatRadius; }
	void DCO_ST_ThreatRadius(float v)		{ m_fThreatRadius = v; }
	float DCO_GT_FriendlyRadius()			{ return m_fFriendlyRadius; }
	void DCO_ST_FriendlyRadius(float v)		{ m_fFriendlyRadius = v; }
	float DCO_GT_IntelCoverageRadius()			{ return m_fIntelCoverageRadius; }
	void DCO_ST_IntelCoverageRadius(float v)		{ m_fIntelCoverageRadius = v; }
	float DCO_GT_CaptureHoldDuration()			{ return m_fCaptureHoldDuration; }
	void DCO_ST_CaptureHoldDuration(float v)		{ m_fCaptureHoldDuration = v; }
	float DCO_GT_UncontestedDwell()			{ return m_fUncontestedDwell; }
	void DCO_ST_UncontestedDwell(float v)		{ m_fUncontestedDwell = v; }
	float DCO_GT_PresenceScanInterval()			{ return m_fPresenceScanInterval; }
	void DCO_ST_PresenceScanInterval(float v)		{ m_fPresenceScanInterval = v; }
	float DCO_GT_FullSpeedUnits()			{ return m_iFullSpeedUnits; }
	void DCO_ST_FullSpeedUnits(float v)		{ m_iFullSpeedUnits = Math.Round(v); }
	float DCO_GT_OwnershipGainThreshold()			{ return m_fOwnershipGainThreshold; }
	void DCO_ST_OwnershipGainThreshold(float v)		{ m_fOwnershipGainThreshold = v; }
	float DCO_GT_OwnershipLossThreshold()			{ return m_fOwnershipLossThreshold; }
	void DCO_ST_OwnershipLossThreshold(float v)		{ m_fOwnershipLossThreshold = v; }
	float DCO_GT_EmptyDecayRate()			{ return m_fEmptyDecayRate; }
	void DCO_ST_EmptyDecayRate(float v)		{ m_fEmptyDecayRate = v; }
	bool DCO_GT_AllowAllFactions()			{ return m_bAllowAllFactions; }
	void DCO_ST_AllowAllFactions(bool v)		{ m_bAllowAllFactions = v; }
	float DCO_GT_StalematePercentThreshold()			{ return m_fStalematePercentThreshold; }
	void DCO_ST_StalematePercentThreshold(float v)		{ m_fStalematePercentThreshold = v; }
	float DCO_GT_StalemateSeconds()			{ return m_fStalemateSeconds; }
	void DCO_ST_StalemateSeconds(float v)		{ m_fStalemateSeconds = v; }
	float DCO_GT_StalemateThreshold()			{ return m_fStalemateThreshold; }
	void DCO_ST_StalemateThreshold(float v)		{ m_fStalemateThreshold = v; }
	float DCO_GT_IdleTimeout()			{ return m_fIdleTimeout; }
	void DCO_ST_IdleTimeout(float v)		{ m_fIdleTimeout = v; }
	float DCO_GT_MaxRelevantDistance()			{ return m_fMaxRelevantDistance; }
	void DCO_ST_MaxRelevantDistance(float v)		{ m_fMaxRelevantDistance = v; }
	float DCO_GT_ProximityWeight()			{ return m_fProximityWeight; }
	void DCO_ST_ProximityWeight(float v)		{ m_fProximityWeight = v; }
	float DCO_GT_DistantFalloffWeight()			{ return m_fDistantFalloffWeight; }
	void DCO_ST_DistantFalloffWeight(float v)		{ m_fDistantFalloffWeight = v; }
	float DCO_GT_ThreatWeight()			{ return m_fThreatWeight; }
	void DCO_ST_ThreatWeight(float v)		{ m_fThreatWeight = v; }
	float DCO_GT_ThreatFalloffPower()			{ return m_fThreatFalloffPower; }
	void DCO_ST_ThreatFalloffPower(float v)		{ m_fThreatFalloffPower = v; }
	float DCO_GT_ThreatFalloffFloor()			{ return m_fThreatFalloffFloor; }
	void DCO_ST_ThreatFalloffFloor(float v)		{ m_fThreatFalloffFloor = v; }
	float DCO_GT_ThreatCountCap()			{ return m_iThreatCountCap; }
	void DCO_ST_ThreatCountCap(float v)		{ m_iThreatCountCap = Math.Round(v); }
}

