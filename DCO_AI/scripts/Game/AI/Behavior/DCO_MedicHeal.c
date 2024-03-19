// AI healing behaviour
// TODO: You have to handle situation, in which movement can be disabled (in AIConfig component)
modded class SCR_AIMedicHealBehavior : SCR_AIBehaviorBase
{
	// Max threat value under which we will consider healing someone
	protected const float MAX_THREAT_THRESHOLD = 0.05;
};
