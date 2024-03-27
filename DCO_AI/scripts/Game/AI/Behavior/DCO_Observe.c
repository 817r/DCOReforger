modded class SCR_AIObserveUnknownFireBehavior : SCR_AIBehaviorBase
{
	protected const float TIMEOUT_S = 12.0;
	protected const float DURATION_MIN_S = 4.0;			// Min duration of behavior
	protected const float DIRECTION_SPAN_DEG = 32.0;	
	protected const float DURATION_S_PER_METER = 0.01;	// How duration depends on distance
	protected const float USE_BINOCULARS_DISTANCE_THRESHOLD = 80;
	
	protected const float HIGH_PRIORITY_MAX_DISTANCE = 50; // Max distance at which we consider observing unknown fire a high priority
	
	protected const float DELAY_MIN_S = 0.1;			// Min delay before we start looking at the position
	protected const float DELAY_S_PER_METER = 0.001;	// How the delay increases depending on dista
	
	protected const float RED_AREA = 15;
	protected const float YELLOW_AREA = 80;
	
	ref SCR_BTParam<vector> m_vPosition = new SCR_BTParam<vector>("Position");
	ref SCR_BTParam<float> m_fDuration = new SCR_BTParam<float>("Duration");
	ref SCR_BTParam<float> m_fRadius = new SCR_BTParam<float>("Radius");
	ref SCR_BTParam<bool> m_bUseBinoculars = new SCR_BTParam<bool>("UseBinoculars");
	ref SCR_BTParam<float> m_fDelay = new SCR_BTParam<float>("Delay");
	ref SCR_BTParam<bool> m_bUseMovement = new SCR_BTParam<bool>("UseMovement");
	
	void SCR_AIObserveUnknownFireBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity,	vector posWorld, bool useMovement, float priorityLevel = PRIORITY_LEVEL_NORMAL)
	{
		InitParameters(posWorld, useMovement);
		
		if (!utility || !utility.GetAIAgent())
			return;
		float distance;
		IEntity controlledEntity = utility.GetAIAgent().GetControlledEntity();
		distance = vector.Distance(controlledEntity.GetOrigin(), posWorld);
		m_fPriority = SCR_AIActionBase.PRIORITY_BEHAVIOR_OBSERVE_UNKNOWN_FIRE; 
		m_sBehaviorTree = "{AD1A56AE2A7ADFE8}AI/BehaviorTrees/Chimera/Soldier/ObservePositionBehavior.bt";
			if (distance <= HIGH_PRIORITY_MAX_DISTANCE)
				m_fPriority = SCR_AIActionBase.PRIORITY_BEHAVIOR_OBSERVE_UNKNOWN_FIRE_HIGH_PRIORITY;
		m_bAllowLook = false;
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
	
	override void InitParameters(vector position, bool useMovement)
	{
		m_vPosition.Init(this, position);
		m_fDuration.Init(this, 0);
		m_fRadius.Init(this, 0);
		m_bUseBinoculars.Init(this, false);
		m_fDelay.Init(this, 0.0);
		m_bUseMovement.Init(this, useMovement);
	}
	
	override void InitTiming(float distance)
	{
		float duration_s = Math.Max(DURATION_MIN_S, DURATION_S_PER_METER * distance);	// Linearly increase with distance
		duration_s = Math.RandomFloat(1*duration_s, 1.5*duration_s);	
		m_fDuration.m_Value = duration_s;
		
		float timeout_s = Math.Max(TIMEOUT_S, duration_s);	// Timeout is quite big, but it should be smaller than duration
		InitTimeout(timeout_s);
		
		float delay_s = Math.Max(DELAY_MIN_S, DELAY_S_PER_METER * distance); // Linearly increase with distance
		delay_s = Math.RandomFloat(0.7*delay_s, 1.3*delay_s);
		m_fDelay.m_Value = delay_s;
	}
	
	override void InitTimeout(float timeout_s)
	{
		float currentTime_ms = GetGame().GetWorld().GetWorldTime(); // Milliseconds!
		m_fDeleteActionTime_ms = currentTime_ms + 1500 * timeout_s;		
	}
	
	override void SetUseMovement(bool value)
	{
		m_bUseMovement.m_Value = true;
		m_bUseCombatMove = true;
	}
	
	override void OnActionSelected()
	{
		super.OnActionSelected();
		SCR_AITalkRequest rq = new SCR_AITalkRequest(ECommunicationType.REPORT_UNDER_FIRE, null, vector.Zero, 0, false, true, SCR_EAITalkRequestPreset.MEDIUM);
		m_Utility.m_CommsHandler.AddRequest(rq);

		if (!m_bUseMovement.m_Value)
		{
			m_Utility.m_CombatMoveState.EnableAiming(true);
		}
	}
	
	override float CustomEvaluate()
	{
		// Fail action if timeout has been reached
		float currentTime_ms = GetGame().GetWorld().GetWorldTime();
		if (currentTime_ms > m_fDeleteActionTime_ms)
		{
			Fail();
			return 0;
		}
		
		if (GetActionState() == EAIActionState.COMPLETED)
		{
			Complete();
			return 0;
		}
		
		return m_fPriority;
	}
	
	override static bool IsNewPositionMoreRelevant(vector myWorldPos, vector oldWorldPos, vector newWorldPos)
	{
		vector vDirOld = vector.Direction(myWorldPos, oldWorldPos);
		vector vDirNew = vector.Direction(myWorldPos, newWorldPos);
		float cosAngle = vector.Dot(vDirOld, vDirNew);
		
		return cosAngle < 0.707; // cos 45 deg
	}
};