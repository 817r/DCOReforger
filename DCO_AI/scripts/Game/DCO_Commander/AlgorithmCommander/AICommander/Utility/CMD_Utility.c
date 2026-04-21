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
	ARTILLERY = 10
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

class CMD_FireMissionRequest
{
	vector         m_vImpactPos;
	SCR_EAIArtilleryAmmoType m_eShellType;
	float          m_fRequestedTime;
	int m_iShellCount;
 
	void CMD_FireMissionRequest(vector pos, SCR_EAIArtilleryAmmoType shellType, float time, int req = 1)
	{
		m_vImpactPos     = pos;
		m_eShellType     = shellType;
		m_fRequestedTime = time;
		m_iShellCount = req;
	}
}