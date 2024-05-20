// AI healing behaviour
// TODO: You have to handle situation, in which movement can be disabled (in AIConfig component)

modded class SCR_AIHealBehavior : SCR_AIBehaviorBase
{
	protected const float MAX_TIME_TO_UNCON_HIGH_PRIORITY_S = 15; // Time in seconds to losing consciousness below which we treat healing ourselves as high priority
};