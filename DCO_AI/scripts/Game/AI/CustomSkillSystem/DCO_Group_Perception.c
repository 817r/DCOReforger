modded class SCR_AIGroupPerception : Managed
{
	// Target is considered lost if noone has seen it for this time.
	// This value should be consistent with actual duration of AI combat behavior (see TARGET_MAX_LAST_SEEN)
	const float TARGET_LOST_THRESHOLD_S = 60.0;
	
	// Time till target is totally forgotten and is removed from memory.
	// This doesn't need to be much longer than MAX_CLUSTER_AGE_S
	const float TARGET_FORGET_THRESHOLD_S = 170.0;
}