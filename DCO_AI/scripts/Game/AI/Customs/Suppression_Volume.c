modded class SCR_AISuppressionObjectVolumeBox : SCR_AISuppressionVolumeBox
{
	// Object volume scaling settings
	protected static const float OBJECT_VOLUME_MAX_SCALE_DISTANCE = 1000.0;
	protected static const float OBJECT_VOLUME_MAX_SCALE = 5; // How much bigger volume will get at max scaling distance
	protected static const float OBJECT_VOLUME_MIN_SCALE = 1.2;
	
	protected static const float TARGET_MIN_SIZE = 1; // Minimal size of target in meters
	protected static const float OBJECT_VOLUME_MIN_Y = 2.5; // Min vertical size of object volume
	protected static const float NOT_RECOGNIZED_CLUSTER_SCALE = 5; // Scale multiplier for cluster volumes of not recognized targets
	
	//---------------------------------------------------------------------------------------
	override static void ScaleTargetBBox(inout vector bbMin, inout vector bbMax, float distance, bool recognized = true)
	{		
		distance = Math.Clamp(distance, 0, OBJECT_VOLUME_MAX_SCALE_DISTANCE);
		float targetSize = Math.Max(vector.DistanceXZ(bbMin, bbMax), TARGET_MIN_SIZE);
		float factor = Math.Map(distance, 0, OBJECT_VOLUME_MAX_SCALE_DISTANCE, OBJECT_VOLUME_MIN_SCALE, OBJECT_VOLUME_MAX_SCALE);
		
		// Volumes for not recognized targets are much bigger
		if (!recognized)
			factor *= NOT_RECOGNIZED_CLUSTER_SCALE;
		
		float extraSize = targetSize * (factor - 1);
					
		bbMin[0] = bbMin[0] - extraSize;
		bbMin[2] = bbMin[2] - extraSize;
		bbMax[0] = bbMax[0] + extraSize;
		bbMax[2] = bbMax[2] + extraSize;
		
		// Add Y if not much vertical room
		if ((bbMax[1] - bbMin[1]) < OBJECT_VOLUME_MIN_Y)
		{
			bbMin[1] = bbMin[1] - OBJECT_VOLUME_MIN_Y / 2;
			bbMax[1] = bbMax[1] + OBJECT_VOLUME_MIN_Y / 2;
		}		
	}
}