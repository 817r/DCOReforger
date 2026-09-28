enum CMD_EThreatType
{
	INFANTRY,
	MOTORIZED,
	ARMOR
}

enum CMD_EThreatAction
{
	MONITOR,
	RECON,
	HOLD_DESTROY,
	COUNTERATTACK,
	PREPARE_DEFENSE,
	AVOID,
	DELAY_WITHDRAW,
	SUPPORT_OP
}

enum CMD_EAssetKind
{
	NONE,
	OBJECTIVE,
	CONTACT_GROUP,
	HUB,
	LOGISTICS,
	STAGING,
	MORTAR
}

class CMD_ThreatAssessment
{
	CMD_EThreatType m_eType;
	int m_iEnemies;
	float m_fUncertainty;
	vector m_vVelocity;
	bool m_bStale;

	CMD_EAssetKind m_eAsset;
	vector m_vAssetPos;
	float m_fAssetEta = -1;
	CMD_AICommanderObjectiveComponent m_Objective;
	DCO_LogiJob m_Job;

	CMD_AICommanderObjectiveComponent m_OpObjective;

	float m_fFriendly;
	float m_fEnemy;
	float m_fRatio;
	bool m_bCanMatch;
	bool m_bEmergency;
	bool m_bMG;
	bool m_bAT;
	bool m_bFiringAtUs;
	bool m_bSniper;

	ref array<float> m_aScores = {};

	CMD_EThreatAction Best()
	{
		int best;
		for (int i = 1; i < m_aScores.Count(); i++)
		{
			if (m_aScores[i] > m_aScores[best])
				best = i;
		}
		return best;
	}

	float Score(CMD_EThreatAction action)
	{
		if (!m_aScores.IsIndexValid(action))
			return 0;
		return m_aScores[action];
	}
}

class CMD_ThreatMission
{
	int m_iId;
	vector m_vPos;
	CMD_EThreatAction m_eAction = CMD_EThreatAction.MONITOR;
	float m_fStart;
	float m_fLastChange = -1000;
	float m_fDeadline;
	int m_iStartEnemies;
	bool m_bSeen;
	bool m_bExecuted;
	bool m_bTimedOut;
	CMD_AICommanderObjectiveComponent m_HoldObj;
	ref array<DCO_GroupUtilityComponent> m_aGroups = {};
	ref CMD_ThreatAssessment m_Last;

	static string ActionName(CMD_EThreatAction a)
	{
		return typename.EnumToString(CMD_EThreatAction, a);
	}

	static string ActionKey(CMD_EThreatAction a)
	{
		switch (a)
		{
			case CMD_EThreatAction.RECON:			return "@act_recon";
			case CMD_EThreatAction.HOLD_DESTROY:	return "@act_hold";
			case CMD_EThreatAction.COUNTERATTACK:	return "@act_counter";
			case CMD_EThreatAction.PREPARE_DEFENSE:	return "@act_prepare";
			case CMD_EThreatAction.AVOID:			return "@act_avoid";
			case CMD_EThreatAction.DELAY_WITHDRAW:	return "@act_delay";
			case CMD_EThreatAction.SUPPORT_OP:		return "@act_support";
		}
		return "@act_monitor";
	}
}
