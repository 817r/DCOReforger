modded enum EAIThreatState
{
	PINNED,
	EXHAUSTED
};

modded class SCR_AIThreatSystem
{
	static const float PINNED_THRESHOLD = 1.2;
	static const float EXHAUSTED_THRESHOLD = 2.2;
	
	private static const float SUPPRESSION_BULLET_INCREMENT = 0.11;
	
	private static const float BLEEDING_FIXED_INCREMENT = 0.2;
	private static const float SUPPRESSION_BULLET_INCREMENT = 0.10;
	private static const float ZERO_DISTANCE_SHOT_INCREMENT = 0.008;
	private static const float DISTANT_SHOT_INCREMENT = 0.0006;
	private static const float EXPLOSION_MAX_INCREMENT = 0.6;
	
	//private static const float THREAT_PINNED_DROP__RATE = 0.08 * 0.001;
	//private static const float THREAT_EXHAUSTED_DROP_RATE = 0.03 * 0.001;
	//private static const float THREAT_ENDANGERED_DROP_RATE  = 0.12 * 0.001;
	//private static const float THREAT_SUPPRESSION_DROP_RATE = 0.25 * 0.001; 
	
	private static const float THREAT_SHOT_DROP_RATE = 	0.12 * 0.001; // Falloff (percentual drop per milisecond)
	private static const float THREAT_SUPPRESSION_DROP_RATE = 0.13 * 0.001;
	private static const float THREAT_ENDANGERED_DROP_RATE = 	0.11 * 0.001;
	
	float GetThreatTotal()
	{
		return m_fThreatTotal;
	}
	
	float GetThreatInjury()
	{
		return m_fThreatInjury;
	}
	
	float GetThreatShotsFired()
	{
		return m_fThreatShotsFired;
	}
	
	float GetThreatSuppression()
	{
		return m_fThreatSuppression;
	}
	
	float GetThreatEndangered()
	{
		return m_fThreatIsEndangered;
	}
};