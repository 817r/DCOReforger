// AI healing behaviour
// TODO: You have to handle situation, in which movement can be disabled (in AIConfig component)

modded class SCR_AIHealBehavior : SCR_AIBehaviorBase
{
	protected const float MAX_TIME_TO_UNCON_HIGH_PRIORITY_S = 20; // Time in seconds to losing consciousness below which we treat healing ourselves as high priority
	
	override float CustomEvaluate()
	{
		if (!(GetGame().GetWorld().GetWorldTime() - m_fTimeCreated_ms > m_fPriorityDelay_ms))
			return 0;
		
		// We should heal ourselves with high priority if time to uncon is lower then treshold
		if (m_AIInfo && m_AIInfo.GetBleedTimeToUnconscious() < MAX_TIME_TO_UNCON_HIGH_PRIORITY_S)
			return PRIORITY_BEHAVIOR_HEAL_HIGH_PRIORITY + 10;
		
		if (m_Utility.m_ThreatSystem.GetThreatMeasureWithoutInjuryFactor() < SCR_AIThreatSystem.VIGILANT_THRESHOLD)
			return GetPriority();
		
			return 0;
	}
};