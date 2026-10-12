class DCO_BreachUtility
{
	protected static ref map<IEntity, float> s_mLastBreachThrowTime;
	protected static map<IEntity, float> GetLastBreachThrowTime()
	{
		if (!s_mLastBreachThrowTime)
			s_mLastBreachThrowTime = new map<IEntity, float>();
		return s_mLastBreachThrowTime;
	}


	static string s_sReject;

	static bool TryThrowBreachGrenade(SCR_AIUtilityComponent utility, vector entryPos, bool ordered = false)
	{
		s_sReject = "no_utility";
		if (!utility || !utility.m_CombatComponent)
			return false;

		s_sReject = "no_frag";
		if (!utility.m_CombatComponent.HasWeaponOfType(EWeaponType.WT_FRAGGRENADE))
			return false;

		s_sReject = "usage_0";
		if (DCO_AIConfigComponent.GrenadeUsageOf(utility) <= 0.0)
			return false;

		IEntity myEntity = utility.m_OwnerEntity;
		if (!myEntity)
			return false;

		float distToEntry = vector.Distance(myEntity.GetOrigin(), entryPos);
		s_sReject = "dist_" + Math.Round(distToEntry).ToString();
		float minEntry = 3.0;
		if (ordered)
			minEntry = 2.0;
		if (distToEntry < minEntry || distToEntry > 15.0)
			return false;

		s_sReject = "friendly_near";
		float friendR = 5.0;
		if (ordered)
			friendR = 3.0;
		if (DCO_Util.HasFriendlyNear(myEntity, entryPos, friendR, "q:DCO_BreachUtility", true))
			return false;

		float worldTime_ms = GetGame().GetWorld().GetWorldTime();

		float personalityCooldownScale = GetPersonalityCooldownScale(utility);
		float effectiveCooldown = 20000.0 * personalityCooldownScale;

		float lastThrow;
		s_sReject = "cooldown";
		if (!ordered && GetLastBreachThrowTime().Find(myEntity, lastThrow))
		{
			if ((worldTime_ms - lastThrow) < effectiveCooldown)
				return false;
		}

		vector throwPos;
		if (!DCO_GrenadeUtility.ResolveThrowPos(utility, entryPos, throwPos, true))
		{
			s_sReject = "throw_" + DCO_GrenadeUtility.s_sDCORej;
			return false;
		}
		s_sReject = "ok";

		SCR_AIThrowGrenadeToBehavior breach = new SCR_AIThrowGrenadeToBehavior(
			utility, null, throwPos, EWeaponType.WT_FRAGGRENADE, 1,
			SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE + SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER);
		utility.AddAction(breach);

		GetLastBreachThrowTime().Set(myEntity, worldTime_ms);
		DCO_Perf.Count("gren:breach_ok");

		return true;
	}

	static bool TryDropGrenade(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_CombatComponent || !utility.m_OwnerEntity)
			return false;
		BaseTarget target = utility.m_CombatComponent.GetCurrentTarget();
		if (!target || !target.GetTargetEntity())
			return false;

		IEntity me = utility.m_OwnerEntity;
		vector mp = me.GetOrigin();
		vector tp = target.GetTargetEntity().GetOrigin();
		if (mp[1] - tp[1] <= 2.5 || vector.DistanceXZ(mp, tp) >= 8.0)
			return false;
		if (!utility.m_CombatComponent.HasWeaponOfType(EWeaponType.WT_FRAGGRENADE))
			return false;
		if (!DCO_GrenadeUtility.CanThrowGrenadeNow(utility))
			return false;
		DCO_Perf.Count("gren:try");
		if (!DCO_GrenadeUtility.HardCoverBetween(me, tp))
		{
			DCO_Perf.Count("gren:rej_close_nocover");
			return false;
		}
		if (DCO_GrenadeUtility.FriendlyInBlast(me, tp, 15.0))
		{
			DCO_Perf.Count("gren:rej_friendly");
			return false;
		}

		vector outward = tp - mp;
		outward[1] = 0;
		if (outward.LengthSq() > 0.01)
			outward.Normalize();
		vector aim = tp + outward + Vector(0, 0.3, 0);
		if (DCO_Util.TraceFrac(mp + Vector(0, 1.6, 0), aim, me, -1, EPhysicsLayerDefs.Projectile, "t:DCO_DropGrenade") < 0.9)
		{
			DCO_Perf.Count("gren:rej_arc");
			return false;
		}

		vector throwPos = DCO_AimUtility.ScatterExplosive(utility, tp + outward, false, DCO_GrenadeUtility.ScatterMul(utility), 0.3);
		SCR_AIThrowGrenadeToBehavior drop = new SCR_AIThrowGrenadeToBehavior(
			utility, null, throwPos, EWeaponType.WT_FRAGGRENADE, 1,
			SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE + SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER);
		utility.AddAction(drop);
		DCO_GrenadeUtility.NotifyGrenadeThrown(utility);
		DCO_Perf.Count("gren:drop_ok");
		return true;
	}

	protected static float GetPersonalityCooldownScale(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_DCOConfig)
			return 1.0;

		switch (utility.m_DCOConfig.GetPersonality())
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 2.0;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 0.5;
			case DCO_EAIPersonality.RECKLESS:
				return 0.3;
			default:
				return 1.0;
		}

		return 1.0;
	}

}
