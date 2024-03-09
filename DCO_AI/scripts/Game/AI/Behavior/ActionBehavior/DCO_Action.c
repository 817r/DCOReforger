modded class SCR_AIActionBase : AIActionBase
{
	// Priority levels
	const static float PRIORITY_LEVEL_NORMAL					= 0;
	const static float PRIORITY_LEVEL_PLAYER					= 1000;
	const static float PRIORITY_LEVEL_LEADER					= 1500;
	const static float PRIORITY_LEVEL_SURVIVAL					= 2000;
	const static float PRIORITY_LEVEL_GAMEMASTER				= 3000;
	
	// Unit behaviors
	const static float PRIORITY_BEHAVIOR_RETREAT_MELEE					= 190 + PRIORITY_LEVEL_PLAYER;
	const static float PRIORITY_BEHAVIOR_GET_OUT_VEHICLE_HIGH_PRIORITY 	= 162 + PRIORITY_LEVEL_PLAYER; // High priority get out for evacuations of vehicles
	const static float PRIORITY_BEHAVIOR_MOVE_FROM_DANGER				= 160 + PRIORITY_LEVEL_PLAYER;
	const static float PRIORITY_BEHAVIOR_ATTACK_HIGH_PRIORITY			= 120;	// Attack high priority
	const static float PRIORITY_BEHAVIOR_PICKUP_INVENTORY_ITEMS 		= 118;
	const static float PRIORITY_BEHAVIOR_RETREAT_FROM_TARGET			= 115;	//		(when attack exists, retreat does not, and the other way)
	const static float PRIORITY_BEHAVIOR_COMBAT_MOVE_GROUP				= 106;
	const static float PRIORITY_BEHAVIOR_PROVIDE_AMMO					= 100;
	const static float PRIORITY_BEHAVIOR_ATTACK_SELECTED				= 90; // Attack selected
	const static float PRIORITY_BEHAVIOR_HEAL_WAIT						= 83;
	const static float PRIORITY_BEHAVIOR_THROW_GRENADE					= 73;
	const static float PRIORITY_BEHAVIOR_MOVE_FROM_VEHICLE_HORN			= 72;	
	const static float PRIORITY_BEHAVIOR_ATTACK_NOT_SELECTED			= 70;	// Attack not selected
	const static float PRIORITY_BEHAVIOR_MOVE_FROM_UNKNOWN_FIRE			= 68;
	const static float PRIORITY_BEHAVIOR_OBSERVE_UNKNOWN_FIRE 			= 66;	// Stare at gunfire origin. !!! Priority of this must be higher than move and investigate!
	const static float PRIORITY_BEHAVIOR_HEAL							= 65;
	const static float PRIORITY_BEHAVIOR_MOVE_AND_INVESTIGATE			= 64;
	const static float PRIORITY_BEHAVIOR_MEDIC_HEAL						= 63;
	const static float PRIORITY_BEHAVIOR_DEFEND							= 62;	// Defend selected waypoint	
	const static float PRIORITY_BEHAVIOR_FIND_FIRE_POSITION				= 61;
	const static float PRIORITY_BEHAVIOR_MOVE_INDIVIDUALLY				= 60;
	const static float PRIORITY_BEHAVIOR_VEHICLE						= 51;
	const static float PRIORITY_BEHAVIOR_GET_OUT_VEHICLE				= 51;
	const static float PRIORITY_BEHAVIOR_PERFORM_ACTION					= 30;
	const static float PRIORITY_BEHAVIOR_MOVE							= 30;
	const static float PRIORITY_BEHAVIOR_MOVE_IN_FORMATION				= 30;
	
	// Group activities
	const static float PRIORITY_ACTIVITY_RESUPPLY				= 100;
	const static float PRIORITY_ACTIVITY_ATTACK_CLUSTER			= 70;
	const static float PRIORITY_ACTIVITY_SEEK_AND_DESTROY 		= 60;
	const static float PRIORITY_ACTIVITY_INVESTIGATE_CLUSTER	= 55;
	const static float PRIORITY_ACTIVITY_DEFEND_FROM_CLUSTER	= 55;
	const static float PRIORITY_ACTIVITY_HEAL					= 53;
	const static float PRIORITY_ACTIVITY_MOVE					= 50;
	const static float PRIORITY_ACTIVITY_PERFORM_ACTION			= 50;
	const static float PRIORITY_ACTIVITY_DEFEND					= 50;
	const static float PRIORITY_ACTIVITY_GET_IN					= 50;
	const static float PRIORITY_ACTIVITY_GET_OUT				= 50;
	const static float PRIORITY_ACTIVITY_FOLLOW					= 50;
	
	// Individual Assesment
	const static float PRIORITY_INDIVIDUAL_RETREAT				= 100 + PRIORITY_LEVEL_SURVIVAL;
	
	
};