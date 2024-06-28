modded class SCR_AIActivitySmokeCoverFeatureAgent : Managed
{
	static const float CLOSE_DISTANCE_TRESHOLD_SQ = 10*10;
	static const float MAX_CLOSE_DISTANCE_WEIGHT = 256;
	
	SCR_ChimeraAIAgent m_Agent;
}

modded class SCR_AIActivitySmokeCoverFeature: SCR_AIActivityFeatureBase
{
	static const int MAX_DISTANCE_TO_TARGET_POS_SQ = 40*40;
	static const int MAX_SMOKE_POSITION_COUNT = 5; // Max number of smoke grenades that can be thrown at one time
	static const int SMOKE_WALL_GAPS_SIZE = 6; // Width in meters of gaps between smokes in smoke walls
	
	override SCR_AIActivitySmokeCoverFeatureProperties GetActivityProperties(SCR_AIActivityBase activity)
	{
		return SCR_AIActivitySmokeCoverFeatureProperties.PROTECT_FROM_CLUSTERS;
	}	
}