// Base class of volume object representing space used to calculate suppression lines
// Child classes can represent different shapes (bbox or sphere), can be static or dynamic (e.g. bbox attached to base target)
modded class SCR_AISuppressionVolumeBase
{	
	// Max angle at which end of suppression line can be placed relative to shooter position (right-left)
	protected static const float MAX_X_ANGLE_DEG = 15;
	
	// Min angle at which end of suppression line should be placed relative to shooter position (right-left)
	protected static const float MIN_X_ANGLE_DEG = 2;
	
	// Max angle at which end of suppression line can be placed relative to shooter position (top-down)
	protected static const float MAX_Y_ANGLE_DEG = 6;
	
	// Min angle at which end of suppression line should be placed relative to shooter position (top-down)
	protected static const float MIN_Y_ANGLE_DEG = 0.25;
	
	// Min Y above surface of suppression line positions
	protected static const float MIN_SURFACE_Y = 0.3;
	
	// Random chance of going at opposite direction (RandomFloat01 < CHANCE)
	protected static const float CHANCE_FOR_OPPOSITE_DIR = 0.150;
}

modded class SCR_AISuppressionObjectVolumeBox : SCR_AISuppressionVolumeBox
{
	// Object volume scaling settings
	protected static const float OBJECT_VOLUME_MAX_SCALE_DISTANCE = 600.0;
	protected static const float OBJECT_VOLUME_MAX_SCALE = 12; // How much bigger volume will get at max scaling distance
	protected static const float OBJECT_VOLUME_MIN_SCALE = 2;
	
	protected static const float TARGET_MIN_SIZE = 2; // Minimal size of target in meters
	protected static const float OBJECT_VOLUME_MIN_Y = 4; // Min vertical size of object volume
	protected static const float NOT_RECOGNIZED_CLUSTER_SCALE = 5; // Scale multiplier for cluster volumes of not recognized targets
}