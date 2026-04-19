enum CMD_ECommanderState
{
    IDLE         = 0,
    PLANNING     = 1,
    COMMANDING   = 2,
    DEAD         = 3,
    REPLACING    = 4
}

enum CMD_EObjectivePriority
{
    LOW    = 0,
    MEDIUM = 1,
    HIGH   = 2,
    CRITICAL = 3
}

enum CMD_EObjectiveAction
{
	NONE,
    RECON,
	CAPTURE,
	DEFEND
}

enum CMD_EObjectiveState
{
    PENDING    = 0,
    ASSIGNED   = 1,
    COMPLETED  = 2,
    FAILED     = 3
}

enum DCOG_EGroupStatus
{
	IDLE,
	EXECUTING_COMMAND,
	INITIATIVE,
	TRANSITING
}

enum CMD_EGroupRole
{
	NONE      = 0,
	RECON     = 1,
	ASSAULT   = 2,
	FLANK     = 3,
	TRANSPORT = 4,
	RESERVE   = 5,
	RETREAT   = 6,
	REINFORNCE= 7,
	DEFEND    = 8,
	ARMORED   = 9,
	STATIC_ARTILLERY = 10
}

enum CMD_EObjectiveType
{
	CAPTURE,
	DESTROY
}

enum CMD_EThreatLevel
{
	NEGLIGIBLE = 0,
	LOW        = 1,
	MEDIUM     = 2,
	HIGH       = 3,
	CRITICAL   = 4
}

enum CMD_ECommanderMode
{
	OFFENSIVE  = 0,
	DEFENSIVE  = 1,
	BALANCED   = 2
}
 
class CMD_ContactReport
{
	vector m_vPosition;
	int    m_iEstimatedEnemyCount;
	float  m_fReportTime;
	string m_sReporterGroupName;
 
	void CMD_ContactReport(vector pos, int enemyCount, float worldTime, string reporterName)
	{
		m_vPosition             = pos;
		m_iEstimatedEnemyCount  = enemyCount;
		m_fReportTime           = worldTime;
		m_sReporterGroupName    = reporterName;
	}
}
 
class CMD_ThreatEntry
{
	vector            m_vPosition;
	int               m_iEstimatedEnemyCount;
	float             m_fFirstReportTime;
	float             m_fLastUpdateTime;
	float             m_fPriorityScore;
	CMD_EThreatLevel  m_eThreatLevel;
	bool              m_bEngaged;
	bool              m_bReinforcementSent;
	int				  m_iReinforcementSentNumber;
	DCO_GroupUtilityComponent m_sEngagingGroupName;
	float m_fLastReinforcementTime = 0.0;
	float m_fLastArtilleryTime     = 0.0;
	bool  m_bArtilleryCalled       = false;
	
	bool              m_bFlankSent;
	
	bool              m_bNeedsRecon;
	bool              m_bReconSent;
 
	void CMD_ThreatEntry(vector pos, int enemyCount, float worldTime, DCO_GroupUtilityComponent grp)
	{
		m_vPosition             = pos;
		m_iEstimatedEnemyCount  = enemyCount;
		m_fFirstReportTime      = worldTime;
		m_fLastUpdateTime       = worldTime;
		m_fPriorityScore        = 0.0;
		m_eThreatLevel          = CMD_EThreatLevel.NEGLIGIBLE;
		m_bEngaged              = false;
		m_bReinforcementSent    = false;
		m_iReinforcementSentNumber = 0;
		m_sEngagingGroupName    = grp;
		m_bFlankSent            = false;
		m_bNeedsRecon           = false;
		m_bReconSent            = false;
	}
}

enum CMD_EArtilleryRoundType
{
	HE    = 0,
	SMOKE = 1,
	ILLUM = 2
}
 
class CMD_ArtilleryFireMission
{
	vector                  m_vTargetPos;
	CMD_EArtilleryRoundType m_eRoundType;
	float                   m_fExecuteAt;   // worldTime kapan order dikirim ke mortar
	bool                    m_bDispatched;  // sudah dikirim ke mortar?

	void CMD_ArtilleryFireMission(vector targetPos, CMD_EArtilleryRoundType type, float executeAt)
	{
		m_vTargetPos   = targetPos;
		m_eRoundType   = type;
		m_fExecuteAt   = executeAt;
		m_bDispatched  = false;
	}
}

class CMD_PendingFireMission
{
	vector                  m_vTargetPos;
	CMD_EArtilleryRoundType m_eRoundType;
	int                     m_iRoundCount;
	float                   m_fQueuedAt;

	void CMD_PendingFireMission(vector targetPos, CMD_EArtilleryRoundType type, int rounds, float queuedAt)
	{
		m_vTargetPos  = targetPos;
		m_eRoundType  = type;
		m_iRoundCount = rounds;
		m_fQueuedAt   = queuedAt;
	}
}

enum CMD_EMortarBaseSize
{
	SMALL  = 0,
	MEDIUM = 1
}

class CMD_MortarSlotData
{
	IEntity                    m_MortarEntity;
	DCO_GroupUtilityComponent  m_CrewGroup;
	bool                       m_bCrewAssigned;
	bool                       m_bBusy;
	float                      m_fReadyAt;

	void CMD_MortarSlotData(IEntity mortarEntity)
	{
		m_MortarEntity   = mortarEntity;
		m_CrewGroup      = null;
		m_bCrewAssigned  = false;
		m_bBusy          = false;
		m_fReadyAt       = 0.0;
	}
}

class CMD_EnemyBatteryReport
{
	vector     m_vEstimatedPos;
	FactionKey m_sEnemyFaction;
	float      m_fDetectedAt;
	bool       m_bCounterFired;
	bool       m_bHunterSent;

	void CMD_EnemyBatteryReport(vector estimatedPos, FactionKey enemyFaction, float detectedAt)
	{
		m_vEstimatedPos = estimatedPos;
		m_sEnemyFaction = enemyFaction;
		m_fDetectedAt   = detectedAt;
		m_bCounterFired = false;
		m_bHunterSent   = false;
	}
}

class CMD_HunterEntry
{
	DCO_GroupUtilityComponent m_Group;
	float                     m_fDispatchTime;
	vector                    m_vTargetPos;

	void CMD_HunterEntry(DCO_GroupUtilityComponent grp, float dispatchTime, vector targetPos)
	{
		m_Group         = grp;
		m_fDispatchTime = dispatchTime;
		m_vTargetPos    = targetPos;
	}
}