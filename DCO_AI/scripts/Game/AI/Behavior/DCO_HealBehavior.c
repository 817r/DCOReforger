// AI healing behaviour
// TODO: You have to handle situation, in which movement can be disabled (in AIConfig component)

modded class SCR_AIHealBehavior : SCR_AIBehaviorBase
{
	IEntity m_EntityToHeal;
	
	SCR_AIInfoComponent m_AIInfo;
	SCR_DamageManagerComponent m_DamageManager;

	protected const float MAX_TIME_TO_UNCON_HIGH_PRIORITY_S = 20; // Time in seconds to losing consciousness below which we treat healing ourselves as high priority
	
	override float CustomEvaluate()
	{
		if (!(GetGame().GetWorld().GetWorldTime() - m_fTimeCreated_ms > m_fPriorityDelay_ms))
			return 0;
		
		if (m_AIInfo && m_AIInfo.GetBleedTimeToUnconscious() < MAX_TIME_TO_UNCON_HIGH_PRIORITY_S && SCR_AIThreatSystem.THREATENED_THRESHOLD)
			return PRIORITY_BEHAVIOR_HEAL_HIGH_PRIORITY;
		
		if (m_AIInfo && m_AIInfo.GetBleedTimeToUnconscious() < MAX_TIME_TO_UNCON_HIGH_PRIORITY_S/2)
			return PRIORITY_BEHAVIOR_HEAL_HIGH_PRIORITY;
		
		if (m_Utility.m_ThreatSystem.GetThreatMeasureWithoutInjuryFactor() < SCR_AIThreatSystem.VIGILANT_THRESHOLD)
			return GetPriority();
		
			return 0;
	}
};