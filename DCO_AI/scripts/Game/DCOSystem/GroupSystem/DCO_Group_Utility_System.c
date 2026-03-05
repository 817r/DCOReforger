modded class SCR_AIGroupUtilityComponent
{
	// Settings for cluster suppression
	const float SUPPRESS_MAX_CLUSTER_INFO_AGE_S = 120; // For how long since last info we'll be suppressing cluster
	const float SUPPRESS_OLD_CLUSTER_INFO_AGE_S = 20; // Time since last cluster info that we'll consider as old (starts scaling of fire rate to save ammo)
	const float SUPPRESS_MAX_DESTROYED_CLUSTER_INFO_AGE_S = 10; // How long to suppress a target cluster which has only destroyed targets
	const float SUPPRESS_MIN_DIST_TO_CLUSTER_M = 20; // What is the minimal distance of units to suppression bbox to stop firing
	const float SUPPRESS_MAX_DIST_TO_CLUSTER_M = 1500; // Max distance ...
}