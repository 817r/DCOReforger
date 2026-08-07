class DCO_SmokeUtility
{
	protected static ref map<IEntity, float> s_mLastSmokeThrowTime = new map<IEntity, float>();
	
	protected static ref map<AIGroup, float> s_mLastGroupSmokeThrowTime = new map<AIGroup, float>();
	static const float SMOKE_GROUP_COOLDOWN_MS = 20000.0; // 1 smoke per grup per 20 detik

	static const float SMOKE_COOLDOWN_MS   = 45000.0; // Jangan smoke lebih dari 1x per 45 detik per soldier
	static const float SMOKE_MIN_DANGER    = 1.2;      // Smoke cuma "worth it" kalau danger di atas ini
	static const float SMOKE_THROW_DIST_MAX = 20.0;    // Jarak lempar smoke maksimum dari diri sendiri
	static const float SMOKE_THROW_DIST_MIN = 6.0;     // Musuh kelewat deket -> smoke gak akan sempet ngebantu, skip

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
			return false; // Musuh kelewat deket, gak keburu/gak berguna

		float worldTime_ms = GetGame().GetWorld().GetWorldTime();
		float lastThrow;
		if (s_mLastSmokeThrowTime.Find(myEntity, lastThrow))
		{
			if ((worldTime_ms - lastThrow) < SMOKE_COOLDOWN_MS)
				return false;
		}
		
		AIGroup myGroup = null;
		AIAgent myAgent = utility.GetAIAgent();
		if (myAgent)
			myGroup = myAgent.GetParentGroup();
		
		if (myGroup)
		{
			float lastGroupThrow;
			if (s_mLastGroupSmokeThrowTime.Find(myGroup, lastGroupThrow))
			{
				if ((worldTime_ms - lastGroupThrow) < SMOKE_GROUP_COOLDOWN_MS)
					return false;
			}
		}

		vector dirToThreat = vector.Direction(myEntity.GetOrigin(), threatPos).Normalized();
		float throwDist = Math.Min(distToThreat * 0.5, SMOKE_THROW_DIST_MAX);
		vector smokePos = myEntity.GetOrigin() + dirToThreat * throwDist;

		smokePos[1] = GetGame().GetWorld().GetSurfaceY(smokePos[0], smokePos[2]);

		SCR_AIThrowGrenadeToBehavior smokeThrow = new SCR_AIThrowGrenadeToBehavior(
			utility, null, smokePos, EWeaponType.WT_SMOKEGRENADE, 1,
			SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE + SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER);
		utility.AddAction(smokeThrow);

		s_mLastSmokeThrowTime.Set(myEntity, worldTime_ms);
		
		if (myGroup)
			s_mLastGroupSmokeThrowTime.Set(myGroup, worldTime_ms);

		return true;
	}
}