modded class SCR_AIThreatSector
{
	static const int SECTOR_STATE_IDLE		= 0;
	static const int SECTOR_STATE_ACTIVE	= 1;

	protected static const float ACCUMULATOR_DROP_RATE = 0.02;
	protected static const float SECTOR_FORGET_S = 60.0;

	override void MergeWith(notnull SCR_AIThreatSector other)
	{
		m_vEstimatedPos = 0.5 * (m_vEstimatedPos + other.m_vEstimatedPos);
		m_fDanger += other.m_fDanger;
		m_fNextDangerEscalation += other.m_fNextDangerEscalation;
		m_eFlags = m_eFlags | other.m_eFlags;

		if (m_fTimestampLastEvent.Less(other.m_fTimestampLastEvent))
			m_fTimestampLastEvent = other.m_fTimestampLastEvent;

		if (m_fTimestampStart.Less(other.m_fTimestampStart))
			m_fTimestampStart = other.m_fTimestampStart;

		if (m_fTimestampLastUpdateWithEvents.Less(other.m_fTimestampLastUpdateWithEvents))
			m_fTimestampLastUpdateWithEvents = other.m_fTimestampLastUpdateWithEvents;

		m_fDangerSinceLastUpdate = m_fDangerSinceLastUpdate + other.m_fDangerSinceLastUpdate;
		m_fMaxDangerBumpSinceLastUpdate = Math.Max(m_fMaxDangerBumpSinceLastUpdate, other.m_fMaxDangerBumpSinceLastUpdate);
		m_iEventCountSinceLastUpdate = m_iEventCountSinceLastUpdate + other.m_iEventCountSinceLastUpdate;
	}
}