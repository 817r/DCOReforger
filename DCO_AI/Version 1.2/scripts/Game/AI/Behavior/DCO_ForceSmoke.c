class retreatSmoke : SCR_AIActivitySmokeCoverFeature
{
	override vector GetActivityTargetPosition(SCR_AIActivityBase activity)
	{	
		return vector.Zero;
	}
	
	override SCR_AIActivitySmokeCoverFeatureProperties GetActivityProperties(SCR_AIActivityBase activity)
	{
		return SCR_AIActivitySmokeCoverFeatureProperties.PROTECT_POS | SCR_AIActivitySmokeCoverFeatureProperties.PROTECT_FROM_CLUSTERS;
	}
}