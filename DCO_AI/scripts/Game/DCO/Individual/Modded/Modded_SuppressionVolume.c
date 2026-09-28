modded class SCR_AISuppressionVolumeBase
{
	protected static const float MAX_X_ANGLE_DEG = 14;

	protected static const float MIN_X_ANGLE_DEG = 2.2;

	protected static const float MAX_Y_ANGLE_DEG = 3;

	protected static const float MIN_Y_ANGLE_DEG = 0.5;

	protected static const float MIN_SURFACE_Y = 0.2;

	protected static const float CHANCE_FOR_OPPOSITE_DIR = 0.18;

	static bool CreateSuppressionBox(vector center, float sizeXZ, float height, out vector bbMin, out vector bbMax)
	{
	    float half = sizeXZ * 0.5;

	    bbMin = center - Vector(half, 0, half);
	    bbMax = center + Vector(half, height, half);
	    return true;
	}
}