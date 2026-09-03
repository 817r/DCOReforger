enum DCO_EUnitType
{
	undefined,
	infantry,
	vehicle
}

enum DCO_EAISkill
{
	Disabled,
	Cadet,
	Rookie,
	Regular,
	Veteran,
	Elite,
}

enum DCO_BuildingPosCreation
{
	SUCCESS,
	RUNNING,
	FAIL
}


enum DCO_EAIIndividualRoles
{
	LEADING = 1 << 0,
	SUPPRESSING = 1 << 1,
	ANTI_ARMOR = 1 << 2,
	COVERT = 1 << 3,
	LONG_RANGE = 1 << 4,
	MEDIUM_RANGE = 1 << 5,
	CLOSE_RANGE = 1 << 6
}

enum DCO_EAIGroupCapabilities
{
	LEADING = 1 << 0,
	SUPPRESSING = 1 << 1,
	ANTI_ARMOR = 1 << 2,
	COVERT = 1 << 3,
	LONG_RANGE = 1 << 4,
	MEDIUM_RANGE = 1 << 5,
	CLOSE_RANGE = 1 << 6
}