enum DCO_EIndoorRelocateIntent
{
	RELOCATE_DEFEND,
	FIRE_POSITION
}

class DCO_AICombatMoveRequest_IndoorRelocate : SCR_AICombatMoveRequest_Move
{
	IEntity m_Building;

	float m_fMinMoveDist = 2.5;

	vector m_vStartPos;

	DCO_EIndoorRelocateIntent m_eIntent = DCO_EIndoorRelocateIntent.RELOCATE_DEFEND;

	void DCO_AICombatMoveRequest_IndoorRelocate()
	{
		m_eType = SCR_EAICombatMoveRequestType.INDOOR_RELOCATE;
	}
}
