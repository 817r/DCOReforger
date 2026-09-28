modded class SCR_EditableEntityComponent
{
	[RplProp()]
	protected bool m_bDCOHoldPosition;

	void DCO_SetHoldPosition(bool hold)
	{
		if (m_bDCOHoldPosition == hold)
			return;

		m_bDCOHoldPosition = hold;
		Replication.BumpMe();
	}

	bool DCO_IsHoldPosition()
	{
		return m_bDCOHoldPosition;
	}
}
