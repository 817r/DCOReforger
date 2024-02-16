modded class SCR_AIObserveUnknownFireBehavior : SCR_AIObservePositionBehavior
{
	protected const float TIMEOUT_S = 8.0;
	protected const float DURATION_MIN_S = 0.1;			// Min duration of behavior
	protected const float DIRECTION_SPAN_DEG = 150.0;	
	protected const float DURATION_S_PER_METER = 0.001;	// How duration depends on distance
	protected const float USE_BINOCULARS_DISTANCE_THRESHOLD = 120;
	
	protected const float DELAY_MIN_S = 0.1;			// Min delay before we start looking at the position
	protected const float DELAY_S_PER_METER = 0.0005;	// How the delay increases depending on distance
	
	void SCR_AIObserveUnknownFireBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity,
		vector posWorld, float priorityLevel = PRIORITY_LEVEL_NORMAL)
	{
		m_fThreat = 1.01 * SCR_AIThreatSystem.VIGILANT_THRESHOLD;
		SetPriority(SCR_AIActionBase.PRIORITY_BEHAVIOR_OBSERVE_UNKNOWN_FIRE);
		m_fPriorityLevel.m_Value = priorityLevel;
		
		if (!utility || !utility.GetAIAgent())
			return;
		
		
		// Calculate duration depending on distance
		IEntity controlledEntity = utility.GetAIAgent().GetControlledEntity();
		float distance;
		if (controlledEntity)
			distance = vector.Distance(controlledEntity.GetOrigin(), posWorld);
		InitTiming(distance);
		
		if (controlledEntity)
		{
			float radius = distance * Math.Tan(Math.DEG2RAD * DIRECTION_SPAN_DEG);
			m_fRadius.m_Value = radius;
			
			m_bUseBinoculars.m_Value = distance > USE_BINOCULARS_DISTANCE_THRESHOLD;
		}
	}
	
	override void InitTiming(float distance)
	{
		float duration_s = Math.Max(DURATION_MIN_S, DURATION_S_PER_METER * distance);	// Linearly increase with distance
		duration_s = Math.RandomFloat(0.3*duration_s, 0.5*duration_s);	
		m_fDuration.m_Value = duration_s;
		
		float timeout_s = Math.Max(TIMEOUT_S, duration_s);	// Timeout is quite big, but it should be smaller than duration
		InitTimeout(timeout_s);
		
		float delay_s = Math.Max(DELAY_MIN_S, DELAY_S_PER_METER * distance); // Linearly increase with distance
		delay_s = Math.RandomFloat(0.3*delay_s, 0.5*delay_s);
		m_fDelay.m_Value = delay_s;
	}
	
	override void OnActionSelected()
	{
		super.OnActionSelected();
		
		if (Math.RandomFloat01() < 0.2)
		{
			if (!m_Utility.m_CommsHandler.CanBypass())
			{
				SCR_AITalkRequest rq = new SCR_AITalkRequest(ECommunicationType.REPORT_UNDER_FIRE, null, vector.Zero, 0, false, SCR_EAITalkRequestPreset.IRRELEVANT);
				m_Utility.m_CommsHandler.AddRequest(rq);
			}
		}
	}
	
	override static bool IsNewPositionMoreRelevant(vector myWorldPos, vector oldWorldPos, vector newWorldPos)
	{
		vector vDirOld = vector.Direction(myWorldPos, oldWorldPos);
		vector vDirNew = vector.Direction(myWorldPos, newWorldPos);
		float cosAngle = vector.Dot(vDirOld, vDirNew);
		
		return cosAngle < -1; // cos 180 deg
	}
};