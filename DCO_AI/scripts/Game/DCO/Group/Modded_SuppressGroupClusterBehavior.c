modded class SCR_AISuppressGroupClusterBehavior
{
	protected const int VOLUME_UPDATE_INTERVAL_MS = 1*500;
	protected const float FIRE_RATE_SCALING_MAX_DISTANCE = 1200;

	protected const float THREAT_MAX_INCREASE = 0.25;
	protected const float THREAT_MAX_DECREASE = 0.15;
	protected const float THREAT_POTENTIAL_DECAY = 0.2;
	protected const int THREAT_MAX_PEAK_DURATION_MS = 150*1000;
	protected const int THREAT_MAX_PEAK_REACTION_DURATION_MS = 30*1000;

	override protected float GetFireRate(float distance, float timeSinceLastInfoS, float soldierThreat, float groupThreat, float peakReactionFactor)
	{
		float midDist = SCR_AICombatComponent.CLOSE_RANGE_COMBAT_DISTANCE + SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE / 2;

		float soldierThreatFactor = Math.Map(distance, 0, FIRE_RATE_SCALING_MAX_DISTANCE, 2, 0.3);
		const float groupThreatFactor = 1.1;

		float fireRate = 5 * (soldierThreat * soldierThreatFactor + groupThreat * groupThreatFactor);

		if (peakReactionFactor > 0)
		{
			float peakFireRate = fireRate * 1.2 * peakReactionFactor;
			fireRate += Math.Max(0, Math.AbsFloat(fireRate - peakFireRate));
		}

		if (distance > midDist)
			fireRate *= Math.Map(distance, midDist, FIRE_RATE_SCALING_MAX_DISTANCE, 1, 1.25);

		if (timeSinceLastInfoS > SCR_AIGroupUtilityComponent.SUPPRESS_OLD_CLUSTER_INFO_AGE_S)
			fireRate *= Math.Map(timeSinceLastInfoS, 0, SCR_AIGroupUtilityComponent.SUPPRESS_MAX_CLUSTER_INFO_AGE_S, 1, 0.3);

		return fireRate;
	}
}