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

enum CMD_EObjectiveState
{
    PENDING    = 0,
    ASSIGNED   = 1,
    COMPLETED  = 2,
    FAILED     = 3
}

enum CMD_EObjectiveType
{
	CAPTURE,
	DESTROY,
	RECON
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
	bool   m_bArmorSeen;
	bool   m_bATSeen;
	float  m_fUncertainty;
	vector m_vVelocity;
	float  m_fInfoTime;
	int    m_iInf;
	int    m_iMG;
	int    m_iAT;
	int    m_iSniper;
	int    m_iVehicle;
	int    m_iArmor;
	bool   m_bMoving;
	float  m_fHeading;
	bool   m_bFiringAtUs;
	bool   m_bLowPriority;
	vector m_vTruePos;

	void CMD_ContactReport(vector pos, int enemyCount, float worldTime, string reporterName)
	{
		m_vPosition             = pos;
		m_iEstimatedEnemyCount  = enemyCount;
		m_fReportTime           = worldTime;
		m_fInfoTime             = worldTime;
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
	bool              m_bArmorSeen;
	bool              m_bATSeen;

	bool              m_bNeedsRecon;
	bool              m_bReconSent;

	float m_fReportQuality = 0.7;

	float  m_fUncertainty;
	vector m_vVelocity;
	vector m_vTruePos;
	vector m_vBelievedPos;
	float  m_fBelievedUncertainty;
	bool   m_bArtillery;
	int    m_iReporterPid;
	bool   m_bReporterNotified;
	int    m_iMG;
	int    m_iAT;
	int    m_iSniper;
	int    m_iVehicle;
	int    m_iArmor;
	bool   m_bMoving;
	float  m_fHeading;
	bool   m_bFiringAtUs;
	bool   m_bLowPriority;

	bool IsPriorityTarget()
	{
		return m_iMG > 0 || m_iAT > 0 || m_bFiringAtUs;
	}

	void MergeReport(CMD_ContactReport r)
	{
		m_iMG = Math.Max(m_iMG, r.m_iMG);
		m_iAT = Math.Max(m_iAT, r.m_iAT);
		m_iSniper = Math.Max(m_iSniper, r.m_iSniper);
		m_iVehicle = Math.Max(m_iVehicle, r.m_iVehicle);
		m_iArmor = Math.Max(m_iArmor, r.m_iArmor);
		m_bMoving = r.m_bMoving;
		m_fHeading = r.m_fHeading;
		m_bFiringAtUs = m_bFiringAtUs || r.m_bFiringAtUs;
		m_bLowPriority = m_bLowPriority && r.m_bLowPriority;
	}

	void CMD_ThreatEntry(vector pos, int enemyCount, float worldTime, DCO_GroupUtilityComponent grp)
	{
		m_vPosition             = pos;
		m_vBelievedPos          = pos;
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

	float m_fTargetLastSeenTime;
	float m_fReportQuality;
	float m_fSafeRadius;
	float m_fAreaRadius;
	float m_fUncertainty;
	string m_sSource = "manual";
	string m_sTier = "point";
	bool m_bDangerClose;
	DCO_GroupUtilityComponent m_Requester;
	int m_iPlayerGroup = -1;
	bool m_bQueued;
	string m_sDeny;

	void CMD_FireMissionRequest(vector pos, SCR_EAIArtilleryAmmoType shellType, float time, int req = 1, float targetLastSeenTime = -1.0, float reportQuality = 0.7)
	{
		m_vImpactPos     = pos;
		m_eShellType     = shellType;
		m_fRequestedTime = time;
		m_iShellCount = req;

		if (targetLastSeenTime < 0.0)
			m_fTargetLastSeenTime = time;
		else
			m_fTargetLastSeenTime = targetLastSeenTime;

		m_fReportQuality = reportQuality;
	}
}