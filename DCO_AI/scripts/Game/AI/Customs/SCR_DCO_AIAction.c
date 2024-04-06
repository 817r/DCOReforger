modded class SCR_AIActionBase : AIActionBase 
{
	// Priority levels
	const static float PRIORITY_LEVEL_NORMAL					= 0;
	const static float PRIORITY_LEVEL_PLAYER					= 1000;
	const static float PRIORITY_LEVEL_GAMEMASTER				= 2000;
	// Unit behaviors
	const static float PRIORITY_BEHAVIOR_RETREAT_MELEE					= 190 + PRIORITY_LEVEL_PLAYER;
	const static float PRIORITY_BEHAVIOR_GET_OUT_VEHICLE_HIGH_PRIORITY 	= 162 + PRIORITY_LEVEL_PLAYER; // High priority get out for evacuations of vehicles
	const static float PRIORITY_BEHAVIOR_MOVE_FROM_DANGER				= 160 + PRIORITY_LEVEL_PLAYER;
	const static float PRIORITY_BEHAVIOR_ATTACK_HIGH_PRIORITY			= 120;	// Attack high priority
	const static float PRIORITY_BEHAVIOR_PICKUP_INVENTORY_ITEMS 		= 118;
	const static float PRIORITY_BEHAVIOR_HEAL_HIGH_PRIORITY				= 115; // Heal yourself in critical situation
	const static float PRIORITY_BEHAVIOR_OBSERVE_UNKNOWN_FIRE_HIGH_PRIORITY = 113; // Unknown fire at close range
	const static float PRIORITY_BEHAVIOR_THROW_GRENADE					= 112;
	const static float PRIORITY_BEHAVIOR_MEDIC_HEAL						= 111;
	const static float PRIORITY_BEHAVIOR_RETREAT_FROM_TARGET			= 110;	//		(when attack exists, retreat does not, and the other way)
	const static float PRIORITY_BEHAVIOR_PROVIDE_AMMO					= 100;
	const static float PRIORITY_BEHAVIOR_ATTACK_SELECTED				= 90;	// Attack selected
	const static float PRIORITY_BEHAVIOR_HEAL_WAIT						= 83;
	const static float PRIORITY_BEHAVIOR_MOVE_FROM_VEHICLE_HORN			= 72;	
	const static float PRIORITY_BEHAVIOR_ATTACK_NOT_SELECTED			= 70;	// Attack not selected
	const static float PRIORITY_BEHAVIOR_MOVE_FROM_UNKNOWN_FIRE			= 68;
	const static float PRIORITY_BEHAVIOR_OBSERVE_UNKNOWN_FIRE 			= 66;	// Stare at gunfire origin. !!! Priority of this must be higher than move and investigate!
	const static float PRIORITY_BEHAVIOR_HEAL							= 65;
	const static float PRIORITY_BEHAVIOR_MOVE_AND_INVESTIGATE			= 64;
	const static float PRIORITY_BEHAVIOR_DEFEND							= 62;	// Defend selected waypoint	
	const static float PRIORITY_BEHAVIOR_FIND_FIRE_POSITION				= 61;
	const static float PRIORITY_BEHAVIOR_MOVE_INDIVIDUALLY				= 60;
	const static float PRIORITY_BEHAVIOR_VEHICLE						= 51;
	const static float PRIORITY_BEHAVIOR_GET_OUT_VEHICLE				= 51;
	const static float PRIORITY_BEHAVIOR_PERFORM_ACTION					= 30;
	const static float PRIORITY_BEHAVIOR_MOVE							= 30;
	const static float PRIORITY_BEHAVIOR_MOVE_IN_FORMATION				= 30;
	//const static float PRIORITY_BEHAVIOR_
	
	// Sequence of actions specific for dismounting turret and getting back
	const static float PRIORITY_BEHAVIOR_DISMOUNT_TURRET		= 300;
	const static float PRIORITY_BEHAVIOR_DISMOUNT_TURRET_INVESTIGATE = 21;
	const static float PRIORITY_BEHAVIOR_DISMOUNT_TURRET_GET_IN = 20;
	
	// Group activities
	const static float PRIORITY_ACTIVITY_RESUPPLY				= 100;
	const static float PRIORITY_ACTIVITY_HEAL					= 80;
	const static float PRIORITY_ACTIVITY_ATTACK_CLUSTER			= 70;
	const static float PRIORITY_ACTIVITY_SEEK_AND_DESTROY 		= 60;
	const static float PRIORITY_ACTIVITY_INVESTIGATE_CLUSTER	= 55;
	const static float PRIORITY_ACTIVITY_DEFEND_FROM_CLUSTER	= 55;
	const static float PRIORITY_ACTIVITY_MOVE					= 50;
	const static float PRIORITY_ACTIVITY_PERFORM_ACTION			= 50;
	const static float PRIORITY_ACTIVITY_DEFEND					= 50;
	const static float PRIORITY_ACTIVITY_GET_IN					= 50;
	const static float PRIORITY_ACTIVITY_GET_OUT				= 50;
	const static float PRIORITY_ACTIVITY_FOLLOW					= 50;
	
	// CUSTOM ACTION PRIORITY
	const static float PRIORITY_CUSTOM_BEHAVIOR_INVESTIGATE_CLOSE = 67;
	const static float PRIORITY_CUSTOM_BEHAVIOR_EXPLOSION_CLOSE = 71;
};