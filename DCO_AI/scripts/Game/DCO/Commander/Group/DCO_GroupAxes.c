enum DCO_EGroupCapability
{
	INFANTRY,
	MOTORIZED,
	ARMOR,
	MORTAR,
	TRANSPORT
}

enum DCO_EGroupPreset
{
	AUTO = 0,
	ARMOR = 9,
	MORTAR = 10
}

enum DCO_EGroupTask
{
	NONE,
	ATTACK,
	FLANK,
	SUPPORT_BY_FIRE,
	DEFEND,
	GARRISON,
	RECON,
	PATROL,
	REINFORCE,
	TRANSPORT,
	FIRE_MISSION
}

enum DCO_ETaskPhase
{
	MOVING,
	STAGING,
	EXECUTING,
	HOLDING
}

enum DCO_EGroupState
{
	READY				= 0,
	EN_ROUTE			= 1,
	IN_CONTACT			= 2,
	SUPPRESSED			= 4,
	COMBAT_INEFFECTIVE	= 8,
	RETREATING			= 16,
	MOUNTED				= 32
}

class DCO_GroupTask
{
	DCO_EGroupTask m_eType = DCO_EGroupTask.NONE;
	CMD_AICommanderObjectiveComponent m_Objective;
	vector m_vTarget;
	DCO_ETaskPhase m_ePhase = DCO_ETaskPhase.HOLDING;
	float m_fStart_s;
	AICommander_BaseComponent m_Issuer;
}

enum DCO_EROEMode
{
	HOLD,
	TIGHT,
	FREE
}

[BaseContainerProps()]
class DCO_ROEEntry
{
	[Attribute("0", UIWidgets.ComboBox, "Tugas", "", ParamEnumArray.FromEnum(DCO_EGroupTask))]
	DCO_EGroupTask m_eTask;

	[Attribute("1", UIWidgets.ComboBox, "Mode", "", ParamEnumArray.FromEnum(DCO_EROEMode))]
	DCO_EROEMode m_eMode;

	[Attribute("150", UIWidgets.EditBox, "Jarak engage (m) waktu gak lagi jalan")]
	float m_fEngageDist;

	[Attribute("150", UIWidgets.EditBox, "Jarak engage (m) waktu lagi jalan ke tujuan (EN_ROUTE)")]
	float m_fEngageDistEnRoute;

	static DCO_ROEEntry Create(DCO_EGroupTask task, DCO_EROEMode mode, float dist, float distEnRoute)
	{
		DCO_ROEEntry e = new DCO_ROEEntry();
		e.m_eTask = task;
		e.m_eMode = mode;
		e.m_fEngageDist = dist;
		e.m_fEngageDistEnRoute = distEnRoute;
		return e;
	}
}

class DCO_ROETable
{
	protected ref map<DCO_EGroupTask, ref DCO_ROEEntry> m_mEntries = new map<DCO_EGroupTask, ref DCO_ROEEntry>();

	float m_fDistScale = 1;
	bool m_bEngageDetected = true;
	float m_fHoldEngageDist;
	bool m_bFireWhenSuppressed;

	void Set(DCO_ROEEntry e)
	{
		m_mEntries.Set(e.m_eTask, e);
	}

	DCO_ROEEntry Get(DCO_EGroupTask task)
	{
		DCO_ROEEntry e = m_mEntries.Get(task);
		if (e)
			return e;
		return m_mEntries.Get(DCO_EGroupTask.NONE);
	}
}
