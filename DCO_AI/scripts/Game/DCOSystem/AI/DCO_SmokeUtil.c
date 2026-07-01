class DCO_SmokeUtility
{
	protected static ref map<IEntity, float> s_mLastSmokeThrowTime = new map<IEntity, float>();

	static const float SMOKE_COOLDOWN_MS   = 45000.0;
	static const float SMOKE_MIN_DANGER    = 1.2;
	static const float SMOKE_THROW_DIST_MAX = 20.0;
	static const float SMOKE_THROW_DIST_MIN = 6.0;
	
	static bool TryDeploySmokeForRetreat(SCR_AIUtilityComponent utility, vector threatPos, float dangerSeverity = 999.0)
	{
		if (!utility || !utility.m_CombatComponent)
			return false;

		if (dangerSeverity < SMOKE_MIN_DANGER)
			return false;

		if (!utility.m_CombatComponent.HasWeaponOfType(EWeaponType.WT_SMOKEGRENADE))
			return false;

		IEntity myEntity = utility.m_OwnerEntity;
		if (!myEntity)
			return false;

		float distToThreat = vector.Distance(myEntity.GetOrigin(), threatPos);
		if (distToThreat < SMOKE_THROW_DIST_MIN)
			return false;

		float worldTime_ms = GetGame().GetWorld().GetWorldTime();
		float lastThrow;
		if (s_mLastSmokeThrowTime.Find(myEntity, lastThrow))
		{
			if ((worldTime_ms - lastThrow) < SMOKE_COOLDOWN_MS)
				return false;
		}

		vector dirToThreat = vector.Direction(myEntity.GetOrigin(), threatPos).Normalized();
		float throwDist = Math.Min(distToThreat * 0.5, SMOKE_THROW_DIST_MAX);
		vector smokePos = myEntity.GetOrigin() + dirToThreat * throwDist;

		SCR_AIThrowGrenadeToBehavior smokeThrow = new SCR_AIThrowGrenadeToBehavior(
			utility, null, smokePos, EWeaponType.WT_SMOKEGRENADE, 1,
			SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE + SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER);
		utility.AddAction(smokeThrow);

		s_mLastSmokeThrowTime.Set(myEntity, worldTime_ms);

		return true;
	}
}