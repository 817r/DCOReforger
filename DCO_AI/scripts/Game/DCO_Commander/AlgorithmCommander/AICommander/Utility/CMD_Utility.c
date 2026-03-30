enum CMD_ECommanderState
{
    IDLE         = 0,   // no objective, waiting
    PLANNING     = 1,   // evaluating objectives
    COMMANDING   = 2,   // actively directing groups
    DEAD         = 3,   // physical entity is dead, awaiting replacement
    REPLACING    = 4    // replacement is being spawned
}

enum CMD_EObjectivePriority
{
    LOW    = 0,
    MEDIUM = 1,
    HIGH   = 2,
    CRITICAL = 3
}

enum CMD_EObjectiveState
{
    PENDING    = 0,   // waiting to be assigned
    ASSIGNED   = 1,   // a commander is working on it
    COMPLETED  = 2,   // objective achieved
    FAILED     = 3    // objective failed / abandoned
}

enum DCOG_EGroupStatus
{
	IDLE,
	EXECUTING_COMMAND,
	INITIATIVE
}