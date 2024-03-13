modded class SCR_AIObserveUnknownFireBehavior : SCR_AIBehaviorBase
{
	protected const float TIMEOUT_S = 3.0;
	protected const float DURATION_MIN_S = 0.1;			// Min duration of behavior
	protected const float DIRECTION_SPAN_DEG = 32.0;	
	protected const float DURATION_S_PER_METER = 0.01;	// How duration depends on distance
	protected const float USE_BINOCULARS_DISTANCE_THRESHOLD = 70;
	
	protected const float HIGH_PRIORITY_MAX_DISTANCE = 200; // Max distance at which we consider observing unknown fire a high priority
	
	protected const float DELAY_MIN_S = 0.1;			// Min delay before we start looking at the position
	protected const float DELAY_S_PER_METER = 0.001;	// How the delay increases depending on distance
	
	protected float m_fPriority;

	ref SCR_BTParam<vector> m_vPosition = new SCR_BTParam<vector>("Position");
	ref SCR_BTParam<float> m_fDuration = new SCR_BTParam<float>("Duration");
	ref SCR_BTParam<float> m_fRadius = new SCR_BTParam<float>("Radius");
	ref SCR_BTParam<bool> m_bUseBinoculars = new SCR_BTParam<bool>("UseBinoculars");
	ref SCR_BTParam<float> m_fDelay = new SCR_BTParam<float>("Delay");
	ref SCR_BTParam<bool> m_bUseMovement = new SCR_BTParam<bool>("UseMovement");
};