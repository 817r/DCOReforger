modded class SCR_AISuppressGroupClusterBehavior
{
	protected const int VOLUME_UPDATE_INTERVAL_MS = 1*500;
	protected const float FIRE_RATE_SCALING_MAX_DISTANCE = 1200; // Max distance for fire rate scaling
	
	protected const float THREAT_MAX_INCREASE = 0.25; // Max increase of group threat potential per tick
	protected const float THREAT_MAX_DECREASE = 0.15; // Max decrease of group threat per tick while in decrease period
	protected const float THREAT_POTENTIAL_DECAY = 0.2; // How much per tick group threat potential is decaying while in decay period
	protected const int THREAT_MAX_PEAK_DURATION_MS = 150*1000; // Max peak duration (time since threat reaching 100%), after this time we'll start decaying potential
	protected const int THREAT_MAX_PEAK_REACTION_DURATION_MS = 30*1000; // How long we should react on reaching peak (intensification of fire rate while in peak threat)
	
	override protected float GetFireRate(float distance, float timeSinceLastInfoS, float soldierThreat, float groupThreat, float peakReactionFactor)
	{	
		float midDist = SCR_AICombatComponent.CLOSE_RANGE_COMBAT_DISTANCE + SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE / 2;
		
		// Relation between group and soldier threat depends on distance
		float soldierThreatFactor = Math.Map(distance, 0, FIRE_RATE_SCALING_MAX_DISTANCE, 2, 0.3);
		float groupThreatFactor = 1.1;
		
		// Get base fire rate
		float fireRate = 5 * (soldierThreat * soldierThreatFactor + groupThreat * groupThreatFactor);
		
		// Apply peak factor
		if (peakReactionFactor > 0)
		{
			float peakFireRate = fireRate * 1.2 * peakReactionFactor;
			fireRate += Math.Max(0, Math.AbsFloat(fireRate - peakFireRate));
		}
		
		// Units at longer ranges fire slower for aiming, we're increasing fire rate because for suppression we don't need to be so accurate
		if (distance > midDist)
			fireRate *= Math.Map(distance, midDist, FIRE_RATE_SCALING_MAX_DISTANCE, 1, 1.25);
		
		// If info is considered old, we scale down fire rate
		if (timeSinceLastInfoS > SCR_AIGroupUtilityComponent.SUPPRESS_OLD_CLUSTER_INFO_AGE_S)
			fireRate *= Math.Map(timeSinceLastInfoS, 0, SCR_AIGroupUtilityComponent.SUPPRESS_MAX_CLUSTER_INFO_AGE_S, 1, 0.3);
		
		// Remove clamp!
		return fireRate;
	}
}