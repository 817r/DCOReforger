modded class SCR_AISuppressionVolumeBase
{
	// Max angle at which end of suppression line can be placed relative to shooter position (right-left)
	protected static const float MAX_X_ANGLE_DEG = 20;
	
	// Min angle at which end of suppression line should be placed relative to shooter position (right-left)
	protected static const float MIN_X_ANGLE_DEG = 2;
	
	// Max angle at which end of suppression line can be placed relative to shooter position (top-down)
	protected static const float MAX_Y_ANGLE_DEG = 5;
	
	// Min angle at which end of suppression line should be placed relative to shooter position (top-down)
	protected static const float MIN_Y_ANGLE_DEG = 0.5;
	
	// Min Y above surface of suppression line positions
	protected static const float MIN_SURFACE_Y = 0.4;
	
	// Random chance of going at opposite direction (RandomFloat01 < CHANCE)
	protected static const float CHANCE_FOR_OPPOSITE_DIR = 0.18;
}