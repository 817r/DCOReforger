modded class SCR_AIActionBase : AIActionBase 
{
	// CUSTOM ACTION PRIORITY
	const static float PRIORITY_CUSTOM_BEHAVIOR_INVESTIGATE_CLOSE = 67;
	const static float PRIORITY_CUSTOM_BEHAVIOR_EXPLOSION_CLOSE = 71;
	
	const static float PRIORITY_ACTIVITY_EVADE					= 75;
	const static float PRIORITY_ACTIVITY_FORTIFY				= 74;
};

modded class SCR_AIBehaviorBase : SCR_AIActionBase
{
	bool m_bUseCombatMove = true;
}