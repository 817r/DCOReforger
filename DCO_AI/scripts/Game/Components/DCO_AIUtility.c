modded class SCR_AIUtilityComponent : SCR_AIBaseUtilityComponent
{
	protected static const float DISTANCE_HYSTERESIS_FACTOR = 0.2; 	//!< how bigger must be old distance to new in IsInvestigationRelevant()
	protected static const float NEARBY_DISTANCE_SQ = 50; 			//!< what is the minimal distance of new vs old in IsInvestigationRelevant()
	protected static const float REACTION_TO_SAME_UNKNOWN_TARGET_INTERVAL_MS = 5000; //!< how often to react to same unknown target if it didn't change
}
