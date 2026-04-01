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
	REINFORNCE= 7
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
	DCO_GroupUtilityComponent            m_sEngagingGroupName;
 
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
	}
}