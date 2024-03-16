modded class SCR_AIObserveUnknownFireBehavior : SCR_AIBehaviorBase
{
	protected const float TIMEOUT_S = 3.0;
	protected const float DURATION_MIN_S = 1.0;			// Min duration of behavior
	protected const float DIRECTION_SPAN_DEG = 32.0;	
	protected const float DURATION_S_PER_METER = 0.01;	// How duration depends on distance
	protected const float USE_BINOCULARS_DISTANCE_THRESHOLD = 100;
	
	protected const float HIGH_PRIORITY_MAX_DISTANCE = 30; // Max distance at which we consider observing unknown fire a high priority
	
	protected const float DELAY_MIN_S = 0.01;			// Min delay before we start looking at the position
	protected const float DELAY_S_PER_METER = 0.001;	// How the delay increases depending on distance
};