modded class SCR_AISuppressBehavior
{
	protected float m_fDCOAmmoCheck_ms;
	protected bool m_bDCOAmmoHold;

	bool DCO_SuppressAmmoHold()
	{
		if (!m_Utility || !m_Utility.m_OwnerEntity || !m_Utility.m_CombatComponent)
			return false;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fDCOAmmoCheck_ms < 1000.0)
			return m_bDCOAmmoHold;
		m_fDCOAmmoCheck_ms = now;
		BaseWeaponComponent w;
		int muzzle;
		m_Utility.m_CombatComponent.GetSelectedWeapon(w, muzzle);
		m_bDCOAmmoHold = DCO_AmmoUtility.SuppressBudget(m_Utility, w) == 0;
		if (!m_bDCOAmmoHold)
			return false;
		int magAmmo = -1;
		if (w && w.GetCurrentMagazine())
			magAmmo = w.GetCurrentMagazine().GetAmmoCount();
		DCO_Perf.Count("supp_ammo_hold");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("supp_ammo_hold unit=%1 mag=%2 spare=%3 beh=%4 why=no_spare_mags", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), magAmmo, DCO_Ctx.SpareMags(m_Utility.m_OwnerEntity, w), ClassName()));
		return true;
	}

	protected float m_fDCOWallAt_ms;
	protected float m_fDCOObstructedSince_ms = -1;
	protected float m_fDCOBaseRate = -1;
	protected float m_fDCORateAt_ms;
	protected float m_fDCOLastMul = 1.0;

	bool DCO_WallBlocked()
	{
		SCR_AISuppressionVolumeBase vol = m_SuppressionVolume.m_Value;
		if (!vol || !m_Utility || !m_Utility.m_OwnerEntity)
			return false;
		if (m_Utility.m_AIInfo && m_Utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
			return false;
		float now = GetGame().GetWorld().GetWorldTime();
		CharacterControllerComponent cc = m_Utility.GetCharacterController();
		if (cc && cc.GetWeaponObstructedState() != EWeaponObstructedState.UNOBSTRUCTED)
		{
			if (m_fDCOObstructedSince_ms < 0)
				m_fDCOObstructedSince_ms = now;
			else if (now - m_fDCOObstructedSince_ms > 1000.0)
			{
				DCO_Perf.Count("supp_wall_abort:obstructed");
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("supp_wall_abort unit=%1 why=weapon_obstructed beh=%2 center=%3", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), ClassName(), DCO_Log.Pos(vol.GetCenterPosition())));
				return true;
			}
		}
		else
			m_fDCOObstructedSince_ms = -1;
		if (now < m_fDCOWallAt_ms)
			return false;
		m_fDCOWallAt_ms = now + 2000.0;
		vector c = vol.GetCenterPosition();
		if (c == vector.Zero || DCO_SuppressGate.Allowed(m_Utility, c, "recheck"))
			return false;
		DCO_Perf.Count("supp_wall_abort:trace");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("supp_wall_abort unit=%1 why=trace_blocked beh=%2 center=%3 dist=%4", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), ClassName(), DCO_Log.Pos(c), Math.Round(vector.DistanceXZ(c, m_Utility.m_OwnerEntity.GetOrigin()))));
		return true;
	}

	void DCO_ScaleRate()
	{
		if (!m_Utility || !m_Utility.m_OwnerEntity || !m_Utility.m_CombatComponent)
			return;
		if (m_fDCOBaseRate < 0)
			m_fDCOBaseRate = m_fFireRate.m_Value;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fDCORateAt_ms)
			return;
		m_fDCORateAt_ms = now + 2000.0;
		BaseWeaponComponent w;
		int muzzle;
		m_Utility.m_CombatComponent.GetSelectedWeapon(w, muzzle);
		string info;
		float mul = DCO_AmmoUtility.SuppressRateMul(m_Utility, w, info);
		m_fFireRate.m_Value = m_fDCOBaseRate * mul;
		if (mul == m_fDCOLastMul)
			return;
		m_fDCOLastMul = mul;
		DCO_Perf.Count("supp_ammo_scale");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("supp_ammo_scale unit=%1 mul=%2 rate=%3 %4 beh=%5", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), mul.ToString(-1, 2), m_fFireRate.m_Value.ToString(-1, 2), info, ClassName()));
	}

	override void OnActionSelected()
	{
		super.OnActionSelected();
		DCO_LogStart();
	}

	protected void DCO_LogStart()
	{
		SCR_AISuppressionVolumeBase vol = m_SuppressionVolume.m_Value;
		if (!vol || !m_Utility || !m_Utility.m_OwnerEntity || !m_Utility.m_CombatComponent)
			return;
		vector c = vol.GetCenterPosition();
		vector me = m_Utility.m_OwnerEntity.GetOrigin();
		if (c == vector.Zero)
			return;
		float horiz = Math.Max(vector.DistanceXZ(c, me), 0.1);
		float pitch = Math.Atan2(c[1] - (me[1] + 1.5), horiz) * Math.RAD2DEG;
		BaseWeaponComponent w;
		int muzzle;
		m_Utility.m_CombatComponent.GetSelectedWeapon(w, muzzle);
		string wt = "none";
		if (w)
			wt = typename.EnumToString(EWeaponType, w.GetWeaponType());
		DCO_Perf.Count("supp_start");
		DCO_ELogLevel lv = DCO_ELogLevel.DEBUG;
		if (Math.AbsFloat(pitch) > 15.0)
		{
			lv = DCO_ELogLevel.INFO;
			DCO_Perf.Count("supp_high_pitch");
		}
		if (!DCO_Log.On(DCO_ELogChannel.COMBAT, lv))
			return;
		DCO_Log.Write(DCO_ELogChannel.COMBAT, lv, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("supp_start unit=%1 beh=%2 pitch_deg=%3 dist=%4 dy=%5 weapon=%6 muzzle=%7 rate=%8 center=%9", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), ClassName(), Math.Round(pitch), Math.Round(horiz), Math.Round(c[1] - me[1]), wt, muzzle, m_fFireRate.m_Value.ToString(-1, 2), DCO_Log.Pos(c)));
	}

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
		if (DCO_SuppressAmmoHold())
		{
			Complete();
			return 0;
		}
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
		if (DCO_WallBlocked())
		{
			Complete();
			return 0;
		}
		DCO_ScaleRate();
		return super.CustomEvaluate();
	}
}
