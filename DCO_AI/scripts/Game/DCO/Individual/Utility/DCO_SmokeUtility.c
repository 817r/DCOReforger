class DCO_SmokeUtility
{


	protected static ref map<IEntity, float> s_mLastSmokeThrowTime = new map<IEntity, float>();
	protected static ref map<IEntity, float> s_mLastDeclineTime    = new map<IEntity, float>();
	protected static ref map<AIGroup, float> s_mLastGroupSmokeThrowTime = new map<AIGroup, float>();

	protected static ref array<vector> s_aRecentSmokePositions = new array<vector>();
	protected static ref array<float>  s_aRecentSmokeTimes     = new array<float>();

	protected static bool IsAreaRecentlySmoked(vector pos, float worldTime_ms)
	{
		float radiusSq = 60.0 * 60.0;
		bool found = false;

		for (int i = s_aRecentSmokePositions.Count() - 1; i >= 0; i--)
		{
			if (worldTime_ms - s_aRecentSmokeTimes[i] > 45000.0)
			{
				s_aRecentSmokePositions.Remove(i);
				s_aRecentSmokeTimes.Remove(i);
				continue;
			}

			if (!found && vector.DistanceSq(pos, s_aRecentSmokePositions[i]) <= radiusSq)
				found = true;
		}

		return found;
	}

	protected static bool IsOnCooldown(map<IEntity, float> times, IEntity key, float now_ms, float cooldown_ms)
	{
		float last;
		return times.Find(key, last) && now_ms - last < cooldown_ms;
	}

	protected static float PersonalityChance(SCR_AIUtilityComponent utility)
	{
		switch (DCO_PersonalityCombatUtility.GetPersonalitySafe(utility))
		{
			case DCO_EAIPersonality.CAUTIOUS:   return 0.8;
			case DCO_EAIPersonality.AGGRESSIVE: return 0.3;
			case DCO_EAIPersonality.RECKLESS:   return 0.1;
		}
		return 0.5;
	}

	static bool TryDeploySmokeForRetreat(SCR_AIUtilityComponent utility, vector threatPos, float dangerSeverity = -1)
	{
		if (!utility || !utility.m_CombatComponent || !utility.m_ThreatSystem || !utility.m_CombatMoveState)
			return false;

		ChimeraCharacter me = ChimeraCharacter.Cast(utility.m_OwnerEntity);
		if (!me || me.IsInVehicle())
			return false;

		float now_ms = GetGame().GetWorld().GetWorldTime();
		if (IsOnCooldown(s_mLastSmokeThrowTime, me, now_ms, 120000.0)
			|| IsOnCooldown(s_mLastDeclineTime, me, now_ms, 20000.0))
			return false;

		AIGroup group;
		if (utility.GetAIAgent())
			group = utility.GetAIAgent().GetParentGroup();
		float lastGroupThrow;
		if (group && s_mLastGroupSmokeThrowTime.Find(group, lastGroupThrow) && now_ms - lastGroupThrow < 60000.0)
			return false;

		if (utility.m_CombatMoveState.IsInValidCover())
			return false;

		bool pressured = utility.m_ThreatSystem.GetState() == EAIThreatState.THREATENED
			|| utility.m_ThreatSystem.GetSuppressionMeasure() >= 0.35;
		if (dangerSeverity >= 0 && dangerSeverity < 1.8 && !pressured)
			return false;
		if (dangerSeverity < 0 && !pressured)
			return false;

		float distToThreat = vector.Distance(me.GetOrigin(), threatPos);
		if (distToThreat < 25.0 || distToThreat > 350.0)
			return false;

		if (!utility.m_CombatComponent.HasWeaponOfType(EWeaponType.WT_SMOKEGRENADE))
			return false;

		if (SCR_CoverManagerComponent.IsEntityInsideBuilding(me) || !IsInOpenArea(me, 15.0))
			return false;

		vector dirToThreat = vector.Direction(me.GetOrigin(), threatPos).Normalized();
		float throwDist = Math.Clamp(distToThreat * 0.3, 8.0, 15.0);
		vector smokePos = me.GetOrigin() + dirToThreat * throwDist;
		smokePos[1] = GetGame().GetWorld().GetSurfaceY(smokePos[0], smokePos[2]);

		if (IsAreaRecentlySmoked(smokePos, now_ms))
			return false;

		float chance = PersonalityChance(utility);
		if (utility.m_DCOConfig)
			chance *= DCO_AIConfigComponent.UsageToChanceScale(utility.m_DCOConfig.GetSmokeUsage());
		if (Math.RandomFloat01() >= chance)
		{
			s_mLastDeclineTime.Set(me, now_ms);
			return false;
		}

		SCR_AIThrowGrenadeToBehavior smokeThrow = new SCR_AIThrowGrenadeToBehavior(
			utility, null, smokePos, EWeaponType.WT_SMOKEGRENADE, 1,
			SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE + SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER);
		utility.AddAction(smokeThrow);

		s_mLastSmokeThrowTime.Set(me, now_ms);
		if (group)
			s_mLastGroupSmokeThrowTime.Set(group, now_ms);
		s_aRecentSmokePositions.Insert(smokePos);
		s_aRecentSmokeTimes.Insert(now_ms);

		DCO_BenchmarkLoggerComponent.Event(string.Format("smoke unit=%1 threat_dist=%2 supp=%3 danger=%4",
			me, Math.Round(distToThreat), utility.m_ThreatSystem.GetSuppressionMeasure().ToString(1, 2), dangerSeverity.ToString(1, 1)));
		return true;
	}
}
