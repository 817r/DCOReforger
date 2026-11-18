class DCO_AmmoUtility
{

	static int SuppressBudget(SCR_AIUtilityComponent utility, BaseWeaponComponent weapon)
	{
		if (!utility || !utility.m_OwnerEntity || !weapon || IsMagicMag(utility))
			return 2;
		int spare = DCO_Ctx.SpareMags(utility.m_OwnerEntity, weapon);
		if (spare >= 99)
			return 2;
		int minMags = Math.Round(DCO_Stress.T(utility, "ammo.supp_min_mags", 1.0) * GetPersonalityAmmoBias(utility));
		if (spare <= minMags)
			return 0;
		if (spare <= minMags + 1)
			return 1;
		return 2;
	}

	protected static ref map<IEntity, float> s_mSuppliedAt;
	protected static ref map<IEntity, int> s_mSupplied;

	static int Supplied(SCR_AIUtilityComponent u)
	{
		if (!u || !u.m_OwnerEntity)
			return 0;
		IEntity me = u.m_OwnerEntity;
		float now = GetGame().GetWorld().GetWorldTime();
		if (!s_mSuppliedAt)
		{
			s_mSuppliedAt = new map<IEntity, float>();
			s_mSupplied = new map<IEntity, int>();
		}
		float at;
		int cached;
		if (s_mSuppliedAt.Find(me, at) && now - at < 2000.0 && s_mSupplied.Find(me, cached))
			return cached;
		if (s_mSuppliedAt.Count() > 256)
		{
			s_mSuppliedAt.Clear();
			s_mSupplied.Clear();
		}
		int r = 0;
		typename well;
		DCO_Rearm.SpareMags(u.m_CombatComponent, me, well);
		if (well && DCO_Rearm.DonorNear(u, well, 10.0))
			r = 1;
		else if (DCO_Rearm.SourceNear(me, 40.0))
			r = 2;
		else if (ResupplyPending(u))
			r = 3;
		else if (HubNear(me, 150.0))
			r = 4;
		s_mSuppliedAt.Set(me, now);
		s_mSupplied.Set(me, r);
		DCO_Perf.Count("ammo:supplied_" + r.ToString());
		return r;
	}

	protected static bool ResupplyPending(SCR_AIUtilityComponent u)
	{
		AIAgent ag = u.GetAIAgent();
		if (!ag || !ag.GetParentGroup())
			return false;
		DCO_GroupUtilityComponent dg = DCO_GroupUtilityComponent.Cast(ag.GetParentGroup().FindComponent(DCO_GroupUtilityComponent));
		if (!dg || !dg.GetMyCommander() || !dg.GetMyCommander().GetLogistics())
			return false;
		foreach (DCO_LogiJob j : dg.GetMyCommander().GetLogistics().GetJobs())
		{
			if (j && j.m_eKind == DCO_ELogiJob.RESUPPLY && j.m_aGroups.Contains(dg))
				return true;
		}
		return false;
	}

	protected static bool HubNear(IEntity me, float radius)
	{
		string fk = DCO_Util.FactionKeyOf(me, true);
		foreach (DCO_LogisticsHubComponent h : DCO_LogisticsHubComponent.GetHubs())
		{
			if (!h || !h.GetOwner())
				continue;
			if (!h.GetFactionKey().IsEmpty() && h.GetFactionKey() != fk)
				continue;
			if (vector.DistanceXZ(h.GetOwner().GetOrigin(), me.GetOrigin()) <= radius)
				return true;
		}
		return false;
	}

	static float SuppressRateMul(SCR_AIUtilityComponent utility, BaseWeaponComponent weapon, out string info)
	{
		info = "";
		if (!utility || !utility.m_OwnerEntity || !weapon || IsMagicMag(utility))
			return 1.0;
		if (DCO_Stress.T(utility, "ammo.supp_mode", 1.0) <= 0)
			return 1.0;
		int spare = DCO_Ctx.SpareMags(utility.m_OwnerEntity, weapon);
		if (spare >= 99)
			return 1.0;
		int normal = Math.Round(DCO_Stress.T(utility, "ammo.supp_normal_mags", 4.0));
		int stop = Math.Round(DCO_Stress.T(utility, "ammo.supp_min_mags", 1.0) * GetPersonalityAmmoBias(utility));
		if (spare >= normal)
			return 1.0;
		int sup = Supplied(utility);
		info = string.Format("spare=%1 normal=%2 stop=%3 supplied=%4", spare, normal, stop, sup);
		if (sup > 0)
			return 1.0;
		DCO_Perf.Count("ammo:supp_light");
		if (spare <= stop)
			return 0.3;
		int lo = stop + 1;
		int hi = normal - 1;
		if (hi <= lo)
			return 0.45;
		return Math.Clamp(Math.Map(spare, lo, hi, 0.3, 0.6), 0.3, 0.6);
	}

	static float SuppressRateMulQ(SCR_AIUtilityComponent utility, BaseWeaponComponent weapon)
	{
		string info;
		return SuppressRateMul(utility, weapon, info);
	}

	static bool ShouldAvoidSuppressiveFire(SCR_AIUtilityComponent utility, BaseWeaponComponent weapon)
	{
		if (!weapon)
			return false;

		if (IsMagicMag(utility))
			return false;

		if (SuppressBudget(utility, weapon) == 0)
		{
			DCO_Perf.Count("ammo:supp_no_spare");
			return true;
		}

		BaseMagazineComponent mag = weapon.GetCurrentMagazine();
		if (!mag)
			return false;

		int currentAmmo = mag.GetAmmoCount();
		int effectiveHardStop = Math.Round(2 * GetPersonalityAmmoBias(utility));

		return currentAmmo <= effectiveHardStop;
	}

	static float GetAmmoConservationScale(SCR_AIUtilityComponent utility, BaseWeaponComponent weapon)
	{
		if (!weapon)
			return 1.0;

		if (IsMagicMag(utility))
			return 1.0;

		BaseMagazineComponent mag = weapon.GetCurrentMagazine();
		if (!mag)
			return 1.0;

		int currentAmmo = mag.GetAmmoCount();
		float personalityBias = GetPersonalityAmmoBias(utility);

		int effectiveCritical = Math.Round(10 * personalityBias);
		int effectiveLow      = Math.Round(25 * personalityBias);

		float spareScale = 1.0;
		if (SuppressBudget(utility, weapon) == 1)
		{
			spareScale = 0.5;
			DCO_Perf.Count("ammo:supp_low_spare");
		}

		if (currentAmmo <= effectiveCritical)
			return 0.25 * spareScale;
		else if (currentAmmo <= effectiveLow)
			return 0.55 * spareScale;

		return spareScale;
	}

	protected static bool IsMagicMag(SCR_AIUtilityComponent utility)
	{
		return DCO_AIConfigComponent.MagicMagOf(utility);
	}

	protected static float GetPersonalityAmmoBias(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_DCOConfig)
			return 1.0;

		float lm = DCO_Leadership.AmmoMul(DCO_Leadership.OfGroupOf(utility));
		switch (utility.m_DCOConfig.GetPersonality())
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 1.6 * lm;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 0.7 * lm;
			case DCO_EAIPersonality.RECKLESS:
				return 0.4 * lm;
			default:
				return lm;
		}
		return lm;
	}
}