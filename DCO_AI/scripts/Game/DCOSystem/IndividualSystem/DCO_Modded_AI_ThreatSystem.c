modded class SCR_AIThreatSystem
{
	static const float EXPLOSION_MAX_DISTANCE = 120;
	
	private static const float ENDANGERED_INCREMENT = 0.3;
	
	static const float VIGILANT_THRESHOLD = 0.3;
	static const float ALERTED_THRESHOLD = 0.9;
	static const float THREATENED_THRESHOLD = 1.7;
	
	private static const float EXPLOSION_MAX_INCREMENT = 0.35;
	
	override void ThreatBulletImpact(int count)
	{
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("ThreatBulletImpact: %1", count));
		#endif
		
		m_fThreatSuppression = Math.Clamp(m_fThreatSuppression + count*SUPPRESSION_BULLET_INCREMENT, 0, 1);
		m_Combat.DangerSuppressedDecreaseAIM(count/2);
	}
	
	override void ThreatProjectileFlyby(int count)
	{
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("ThreatProjectileFlyby"));
		#endif
		
		m_fThreatSuppression = Math.Clamp(m_fThreatSuppression + count * SUPPRESSION_BULLET_INCREMENT, 0, 1);
		m_Combat.DangerSuppressedDecreaseAIM(count/5);
	}
}