class DCO_BreachUtility
{
	protected static ref map<IEntity, float> s_mLastBreachThrowTime = new map<IEntity, float>();


	static bool TryThrowBreachGrenade(SCR_AIUtilityComponent utility, vector entryPos)
	{
		if (!utility || !utility.m_CombatComponent)
			return false;

		if (!utility.m_CombatComponent.HasWeaponOfType(EWeaponType.WT_FRAGGRENADE))
			return false;

		if (utility.m_DCOConfig && utility.m_DCOConfig.GetGrenadeUsage() <= 0.0)
			return false;

		IEntity myEntity = utility.m_OwnerEntity;
		if (!myEntity)
			return false;

		float distToEntry = vector.Distance(myEntity.GetOrigin(), entryPos);
		if (distToEntry < 4.0 || distToEntry > 15.0)
			return false;

		if (HasFriendlyNear(myEntity, entryPos))
			return false;

		float worldTime_ms = GetGame().GetWorld().GetWorldTime();

		float personalityCooldownScale = GetPersonalityCooldownScale(utility);
		float effectiveCooldown = 20000.0 * personalityCooldownScale;

		float lastThrow;
		if (s_mLastBreachThrowTime.Find(myEntity, lastThrow))
		{
			if ((worldTime_ms - lastThrow) < effectiveCooldown)
				return false;
		}

		vector throwPos;
		if (!DCO_GrenadeUtility.ResolveThrowPos(utility, entryPos, throwPos))
			return false;

		SCR_AIThrowGrenadeToBehavior breach = new SCR_AIThrowGrenadeToBehavior(
			utility, null, throwPos, EWeaponType.WT_FRAGGRENADE, 1,
			SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE + SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER);
		utility.AddAction(breach);

		s_mLastBreachThrowTime.Set(myEntity, worldTime_ms);

		return true;
	}

	protected static ref array<IEntity> s_aFriendlyCheckResult = {};

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

	protected static bool HasFriendlyNear(IEntity self, vector pos)
	{
		s_aFriendlyCheckResult.Clear();

		FactionAffiliationComponent selfFac = FactionAffiliationComponent.Cast(self.FindComponent(FactionAffiliationComponent));
		if (!selfFac || !selfFac.GetAffiliatedFaction())
			return false;

		string myFactionKey = selfFac.GetAffiliatedFaction().GetFactionKey();

		DCO_Perf.Count("q:DCO_BreachUtility");
		GetGame().GetWorld().QueryEntitiesBySphere(pos, 5.0, null, FriendlyQueryCallback, EQueryEntitiesFlags.DYNAMIC);

		foreach (IEntity ent : s_aFriendlyCheckResult)
		{
			if (!ent || ent == self)
				continue;

			FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(ent.FindComponent(FactionAffiliationComponent));
			if (!fac || !fac.GetAffiliatedFaction())
				continue;

			if (fac.GetAffiliatedFaction().GetFactionKey() == myFactionKey)
				return true;
		}

		return false;
	}

	protected static bool FriendlyQueryCallback(IEntity ent)
	{
		if (ent)
			s_aFriendlyCheckResult.Insert(ent);
		return true;
	}
}
