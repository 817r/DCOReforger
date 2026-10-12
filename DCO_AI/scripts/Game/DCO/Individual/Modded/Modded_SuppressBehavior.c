modded class SCR_AISuppressBehavior
{
	protected float m_fDCOSafeCheck_ms;
	protected bool m_bDCOFriendlyBlocked;
	protected float m_fDCOLaneAt_ms;
	protected int m_iDCOLane;
	protected IEntity m_DCOLaneBlocker;

	bool DCO_FriendlyInFire()
	{
		SCR_AISuppressionVolumeBase vol = m_SuppressionVolume.m_Value;
		if (!vol || !m_Utility || !m_Utility.m_OwnerEntity)
			return false;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fDCOSafeCheck_ms)
			return m_bDCOFriendlyBlocked;
		m_fDCOSafeCheck_ms = now + 1000.0;
		vector c = vol.GetCenterPosition();
		m_bDCOFriendlyBlocked = c != vector.Zero && DCO_Util.HasFriendlyNear(m_Utility.m_OwnerEntity, c, 10.0, "supp_friendly_block");
		return m_bDCOFriendlyBlocked;
	}

	bool DCO_LaneBlocked()
	{
		SCR_AISuppressionVolumeBase vol = m_SuppressionVolume.m_Value;
		if (!vol || !m_Utility || !m_Utility.m_OwnerEntity)
			return false;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fDCOLaneAt_ms)
			return m_iDCOLane != 0;
		m_fDCOLaneAt_ms = now + 500.0;
		vector c = vol.GetCenterPosition();
		if (c == vector.Zero)
		{
			m_iDCOLane = 0;
			return false;
		}
		m_iDCOLane = DCO_Util.FireLaneCheck(m_Utility.m_OwnerEntity, c, DCO_Util.VolumeRadius(vol), m_DCOLaneBlocker);
		return m_iDCOLane != 0;
	}

	void DCO_OnLaneAbort()
	{
		DCO_Perf.Count("ff_lane:burst_abort");
		SCR_AISuppressionVolumeBase vol = m_SuppressionVolume.m_Value;
		vector c = vector.Zero;
		if (vol)
			c = vol.GetCenterPosition();
		DCO_Util.LaneEvent(m_Utility.m_OwnerEntity, c, m_iDCOLane, m_DCOLaneBlocker, "cease");
	}

	override float CustomEvaluate()
	{
		if (DCO_FriendlyInFire())
		{
			DCO_Perf.Count("supp_friendly_cancel");
			Complete();
			return 0;
		}
		if (DCO_LaneBlocked())
		{
			DCO_OnLaneAbort();
			Complete();
			return 0;
		}
		return super.CustomEvaluate();
	}
}
