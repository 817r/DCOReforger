modded class SCR_AIObserveUnknownFireBehavior : SCR_AIBehaviorBase
{
	protected const float TIMEOUT_S = 5.0;
	protected const float DURATION_MIN_S = 2.0;			// Min duration of behavior
	protected const float DIRECTION_SPAN_DEG = 32.0;	
	protected const float DURATION_S_PER_METER = 0.01;	// How duration depends on distance
	protected const float USE_BINOCULARS_DISTANCE_THRESHOLD = 120;
	
	protected const float HIGH_PRIORITY_MAX_DISTANCE = 10; // Max distance at which we consider observing unknown fire a high priority
	
	protected const float DELAY_MIN_S = 0.01;			// Min delay before we start looking at the position
	protected const float DELAY_S_PER_METER = 0.001;	// How the delay increases depending on dista
	
	protected const float RED_AREA = 40;
	protected const float YELLOW_AREA = 100;
	
	void SCR_AIObserveUnknownFireBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity,	vector posWorld, bool useMovement, float priorityLevel = PRIORITY_LEVEL_NORMAL)
	{
		InitParameters(posWorld, useMovement);
		
		if (!utility || !utility.GetAIAgent())
			return;
		float distance;
		IEntity controlledEntity = utility.GetAIAgent().GetControlledEntity();
		distance = vector.Distance(controlledEntity.GetOrigin(), posWorld);
		
		if(distance <= YELLOW_AREA)
		{
			m_sBehaviorTree = "{CC6CFA43E8961E69}AI/BehaviorTrees/Chimera/Soldier/MoveAndInvestigate.bt";
			m_fPriorityLevel.m_Value = 65;
		} else
		{
			m_sBehaviorTree = "{AD1A56AE2A7ADFE8}AI/BehaviorTrees/Chimera/Soldier/ObservePositionBehavior.bt";
				if (distance <= HIGH_PRIORITY_MAX_DISTANCE)
					m_fPriority = SCR_AIActionBase.PRIORITY_BEHAVIOR_OBSERVE_UNKNOWN_FIRE_HIGH_PRIORITY;
			m_fPriority = SCR_AIActionBase.PRIORITY_BEHAVIOR_OBSERVE_UNKNOWN_FIRE;
		} 
		
		m_bAllowLook = false; // Disable standard looking
		m_bResetLook = true;
		m_bUseCombatMove = useMovement;
		SetIsUniqueInActionQueue(true);
		m_fThreat = 1.01 * SCR_AIThreatSystem.VIGILANT_THRESHOLD;
		m_fPriorityLevel.m_Value = priorityLevel;
			
		InitTiming(distance);
		
		if (controlledEntity)
		{
			float radius = distance * Math.Tan(Math.DEG2RAD * DIRECTION_SPAN_DEG);
			m_fRadius.m_Value = radius;
			m_bUseBinoculars.m_Value = distance > USE_BINOCULARS_DISTANCE_THRESHOLD;
		}
	}
};