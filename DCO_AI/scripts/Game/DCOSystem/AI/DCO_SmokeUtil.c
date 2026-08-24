class DCO_SmokeUtility
{
	protected static ref map<IEntity, float> s_mLastSmokeThrowTime = new map<IEntity, float>();
	protected static ref map<AIGroup, float> s_mLastGroupSmokeThrowTime = new map<AIGroup, float>();
	static const float SMOKE_GROUP_COOLDOWN_MS = 35000.0; // 1 smoke per grup per 35 detik (dari 20, proporsional sama kenaikan per-unit)
	protected static ref map<IEntity, float> s_mThreatStreakStart    = new map<IEntity, float>(); // kapan streak "kena danger tinggi terus" ini mulai
	protected static ref map<IEntity, float> s_mLastThreatCheckTime  = new map<IEntity, float>(); // kapan terakhir kali lolos danger check (buat deteksi gap)

	static const float SMOKE_SUSTAINED_THRESHOLD_MS = 15000.0; // Harus kena danger tinggi TERUS-MENERUS minimal 15 detik sebelum boleh smoke
	static const float SMOKE_STREAK_RESET_GAP_MS    = 8000.0;  // Jeda lebih dari ini sejak danger tinggi terakhir -> streak reset, dianggap situasi baru

	static const float SMOKE_COOLDOWN_MS   = 90000.0; // 90 detik per soldier (dari 45)
	static const float SMOKE_MIN_DANGER    = 1.8;      // Deket DANGER_HIGH (2.0), bukan di tengah-tengah lagi (dari 1.2)
	static const float SMOKE_THROW_DIST_MAX = 20.0;    // Jarak lempar smoke maksimum dari diri sendiri
	static const float SMOKE_THROW_DIST_MIN = 6.0;     // Musuh kelewat deket -> smoke gak akan sempet ngebantu, skip

	protected static ref array<vector> s_aRecentSmokePositions = new array<vector>();
	protected static ref array<float>  s_aRecentSmokeTimes     = new array<float>();
	
	static const float SMOKE_AREA_COOLDOWN_RADIUS = 80.0;   // meter -- dianggap "area yang sama"
	static const float SMOKE_AREA_COOLDOWN_MS     = 40000.0; // 40 detik sebelum area itu boleh di-smoke lagi

	protected static bool IsAreaRecentlySmoked(vector pos, float worldTime_ms)
	{
		float radiusSq = SMOKE_AREA_COOLDOWN_RADIUS * SMOKE_AREA_COOLDOWN_RADIUS;
		bool found = false;
		
		for (int i = s_aRecentSmokePositions.Count() - 1; i >= 0; i--)
		{
			float age = worldTime_ms - s_aRecentSmokeTimes[i];
			if (age > SMOKE_AREA_COOLDOWN_MS)
			{
				// Kadaluarsa -- cleanup sekalian, gak perlu dicek lagi ke depannya
				s_aRecentSmokePositions.Remove(i);
				s_aRecentSmokeTimes.Remove(i);
				continue;
			}
			
			if (!found && vector.DistanceSq(pos, s_aRecentSmokePositions[i]) <= radiusSq)
				found = true;
		}
		
		return found;
	}
	
	protected static void RecordAreaSmoke(vector pos, float worldTime_ms)
	{
		s_aRecentSmokePositions.Insert(pos);
		s_aRecentSmokeTimes.Insert(worldTime_ms);
	}

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

		float lastThreatCheck;
		bool hadPreviousCheck = s_mLastThreatCheckTime.Find(myEntity, lastThreatCheck);
		
		float streakStart;
		bool hasStreak = s_mThreatStreakStart.Find(myEntity, streakStart);
		
		if (!hasStreak || !hadPreviousCheck || (worldTime_ms - lastThreatCheck) > SMOKE_STREAK_RESET_GAP_MS)
		{
			streakStart = worldTime_ms;
			s_mThreatStreakStart.Set(myEntity, streakStart);
		}
		
		s_mLastThreatCheckTime.Set(myEntity, worldTime_ms);
		
		float streakDuration = worldTime_ms - streakStart;
		if (streakDuration < SMOKE_SUSTAINED_THRESHOLD_MS)
			return false;
		
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
					return false; // udah ada member lain di grup ini yang smoke barusan
			}
		}
		
		vector dirToThreat = vector.Direction(myEntity.GetOrigin(), threatPos).Normalized();
		float throwDist = Math.Min(distToThreat * 0.5, SMOKE_THROW_DIST_MAX);
		vector smokePos = myEntity.GetOrigin() + dirToThreat * throwDist;

		smokePos[1] = GetGame().GetWorld().GetSurfaceY(smokePos[0], smokePos[2]);

		if (IsAreaRecentlySmoked(smokePos, worldTime_ms))
			return false;

		SCR_AIThrowGrenadeToBehavior smokeThrow = new SCR_AIThrowGrenadeToBehavior(
			utility, null, smokePos, EWeaponType.WT_SMOKEGRENADE, 1,
			SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE + SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER);
		utility.AddAction(smokeThrow);

		s_mLastSmokeThrowTime.Set(myEntity, worldTime_ms);
		
		if (myGroup)
			s_mLastGroupSmokeThrowTime.Set(myGroup, worldTime_ms);

		RecordAreaSmoke(smokePos, worldTime_ms);

		return true;
	}
}