class DCO_SmokeUtility
{


	protected static ref map<IEntity, float> s_mLastSmokeThrowTime;
	protected static map<IEntity, float> GetLastSmokeThrowTime()
	{
		if (!s_mLastSmokeThrowTime)
			s_mLastSmokeThrowTime = new map<IEntity, float>();
		return s_mLastSmokeThrowTime;
	}
	protected static ref map<IEntity, float> s_mLastDeclineTime;
	protected static map<IEntity, float> GetLastDeclineTime()
	{
		if (!s_mLastDeclineTime)
			s_mLastDeclineTime = new map<IEntity, float>();
		return s_mLastDeclineTime;
	}
	protected static ref map<AIGroup, float> s_mLastGroupSmokeThrowTime;
	protected static map<AIGroup, float> GetLastGroupSmokeThrowTime()
	{
		if (!s_mLastGroupSmokeThrowTime)
			s_mLastGroupSmokeThrowTime = new map<AIGroup, float>();
		return s_mLastGroupSmokeThrowTime;
	}

	protected static ref array<vector> s_aRecentSmokePositions;
	protected static array<vector> GetRecentSmokePositions()
	{
		if (!s_aRecentSmokePositions)
			s_aRecentSmokePositions = new array<vector>();
		return s_aRecentSmokePositions;
	}
	protected static ref array<float> s_aRecentSmokeTimes;
	protected static array<float> GetRecentSmokeTimes()
	{
		if (!s_aRecentSmokeTimes)
			s_aRecentSmokeTimes = new array<float>();
		return s_aRecentSmokeTimes;
	}

	protected static bool IsAreaRecentlySmoked(vector pos, float worldTime_ms, float radius = 60.0)
	{
		float radiusSq = radius * radius;
		bool found = false;

		for (int i = GetRecentSmokePositions().Count() - 1; i >= 0; i--)
		{
			if (worldTime_ms - GetRecentSmokeTimes()[i] > 45000.0)
			{
				GetRecentSmokePositions().Remove(i);
				GetRecentSmokeTimes().Remove(i);
				continue;
			}

			if (!found && vector.DistanceSq(pos, GetRecentSmokePositions()[i]) <= radiusSq)
				found = true;
		}

		return found;
	}

	static void Note(vector pos, float now_ms)
	{
		GetRecentSmokePositions().Insert(pos);
		GetRecentSmokeTimes().Insert(now_ms);
	}

	static float SegDistXZ(vector a, vector b, vector p)
	{
		vector ab = b - a;
		ab[1] = 0;
		vector ap = p - a;
		ap[1] = 0;
		float len2 = ab.LengthSq();
		float t = 0;
		if (len2 > 0.0001)
			t = Math.Clamp(vector.Dot(ap, ab) / len2, 0.0, 1.0);
		vector c = a + ab * t;
		return vector.DistanceXZ(c, p);
	}

	static bool SmokeOnLine(vector a, vector b, float radius, float now_ms)
	{
		for (int i = GetRecentSmokePositions().Count() - 1; i >= 0; i--)
		{
			if (now_ms - GetRecentSmokeTimes()[i] > 45000.0)
				continue;
			if (SegDistXZ(a, b, GetRecentSmokePositions()[i]) <= radius)
				return true;
		}
		return false;
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

	static bool Allowed(SCR_AIUtilityComponent u, string path)
	{
		float usage = DCO_AIConfigComponent.SmokeUsageOf(u);
		if (usage <= 0)
		{
			DCO_Perf.Count("smoke_block:usage_zero_" + path);
			return false;
		}
		if (Math.RandomFloat01() < Math.Min(usage * 2.0, 1.0))
			return true;
		DCO_Perf.Count("smoke_block:usage_roll_" + path);
		return false;
	}

	protected static bool GroupFired(AIGroup grp, float window_ms)
	{
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		float now_ms = GetGame().GetWorld().GetWorldTime();
		foreach (AIAgent a : agents)
		{
			if (a && a.GetControlledEntity() && now_ms - DCO_ShotCounter.LastShotAt(a.GetControlledEntity()) < window_ms)
				return true;
		}
		return false;
	}

	static string Rule(SCR_AIUtilityComponent u, string why)
	{
		if (!u || !u.m_OwnerEntity || DCO_Stress.T(u, "smk.rule", 1.0) <= 0)
			return string.Empty;
		if (why == "cqb" || why == "medic" || why == "assault_stuck")
			return string.Empty;
		AIGroup grp;
		if (u.GetAIAgent())
			grp = u.GetAIAgent().GetParentGroup();
		if (!grp)
			return string.Empty;
		SCR_AIGroupUtilityComponent gu = SCR_AIGroupUtilityComponent.Cast(grp.FindComponent(SCR_AIGroupUtilityComponent));
		if (!gu)
			return string.Empty;
		if (gu.DCO_IsOvermatch())
			return "overmatch";
		bool threatened = u.m_ThreatSystem && u.m_ThreatSystem.GetState() == EAIThreatState.THREATENED;
		if (gu.DCO_UnderFireAgeS() > 20.0 && !threatened && !DCO_Beaten.HitRecent(u.m_OwnerEntity, 20.0))
			return "no_enemy_fire";
		bool retreating = why.StartsWith("break_contact") || why == "withdraw";
		if (!retreating && u.m_CombatComponent && u.m_CombatComponent.GetCombatMode() != EAIGroupCombatMode.HOLD_FIRE && !GroupFired(grp, 30000.0))
			return "no_own_fire";
		return string.Empty;
	}

	static bool GroupOk(SCR_AIGroup grp, SCR_AIUtilityComponent u, vector smokePos, string why)
	{
		if (!u || !u.m_OwnerEntity)
			return false;
		float now_ms = GetGame().GetWorld().GetWorldTime();
		string reason = Rule(u, why);
		float cd_ms = DCO_Stress.T(u, "smk.group_cd_s", 60.0) * 1000.0;
		float last;
		if (reason.IsEmpty() && grp && why != "medic" && GetLastGroupSmokeThrowTime().Find(grp, last) && now_ms - last < cd_ms)
			reason = "cooldown";
		if (reason.IsEmpty() && IsAreaRecentlySmoked(smokePos, now_ms, 30.0))
			reason = "recent";
		if (reason.IsEmpty() && !Allowed(u, why))
			reason = "usage";
		if (!reason.IsEmpty())
		{
			DCO_Perf.Count("smoke_group_skip:" + reason);
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-", string.Format("smoke_group_skip grp=%1 why=%2 reason=%3 unit=%4", DCO_Log.IdGroup(grp), why, reason, DCO_Log.IdUnit(u.m_OwnerEntity)));
			return false;
		}
		if (grp)
			GetLastGroupSmokeThrowTime().Set(grp, now_ms);
		Note(smokePos, now_ms);
		DCO_Perf.Count("smoke_group:" + why);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-", string.Format("smoke_group grp=%1 why=%2 unit=%3 dist=%4 pos=%5", DCO_Log.IdGroup(grp), why, DCO_Log.IdUnit(u.m_OwnerEntity), Math.Round(vector.DistanceXZ(u.m_OwnerEntity.GetOrigin(), smokePos)), DCO_Log.Pos(smokePos)));
		return true;
	}

	static bool TryDeploySmokeForRetreat(SCR_AIUtilityComponent utility, vector threatPos, float dangerSeverity = -1)
	{
		if (!utility || !utility.m_CombatComponent || !utility.m_ThreatSystem || !utility.m_CombatMoveState)
			return false;

		ChimeraCharacter me = ChimeraCharacter.Cast(utility.m_OwnerEntity);
		if (!me || me.IsInVehicle())
			return false;

		if (DCO_AssaultLock.SmokeBlocked(utility, "retreat_smoke"))
		{
			DCO_Perf.Count("cqb:smoke_reaction_block");
			return false;
		}

		float now_ms = GetGame().GetWorld().GetWorldTime();
		if (IsOnCooldown(GetLastSmokeThrowTime(), me, now_ms, 120000.0)
			|| IsOnCooldown(GetLastDeclineTime(), me, now_ms, 20000.0))
			return false;

		AIGroup group;
		if (utility.GetAIAgent())
			group = utility.GetAIAgent().GetParentGroup();
		float lastGroupThrow;
		if (group && GetLastGroupSmokeThrowTime().Find(group, lastGroupThrow) && now_ms - lastGroupThrow < 60000.0)
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

		if (DCO_CoverManagerComponent.IsEntityInsideBuilding(me) || !IsInOpenArea(me, 15.0))
			return false;

		vector dirToThreat = vector.Direction(me.GetOrigin(), threatPos).Normalized();
		float throwDist = Math.Clamp(distToThreat * 0.3, 8.0, 15.0);
		vector smokePos = me.GetOrigin() + dirToThreat * throwDist;
		smokePos = DCO_Util.Surface(smokePos);

		if (IsAreaRecentlySmoked(smokePos, now_ms))
			return false;

		string rule = Rule(utility, "retreat");
		if (!rule.IsEmpty())
		{
			GetLastDeclineTime().Set(me, now_ms);
			DCO_Perf.Count("smoke_block:rule_" + rule);
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-", string.Format("smoke_skip unit=%1 reason=%2 threat_dist=%3", DCO_Log.IdUnit(me), rule, Math.Round(distToThreat)));
			return false;
		}

		float chance = PersonalityChance(utility);
		chance *= DCO_AIConfigComponent.UsageToChanceScale(DCO_AIConfigComponent.SmokeUsageOf(utility));
		if (Math.RandomFloat01() >= chance)
		{
			GetLastDeclineTime().Set(me, now_ms);
			return false;
		}

		SCR_AIThrowGrenadeToBehavior smokeThrow = new SCR_AIThrowGrenadeToBehavior(
			utility, null, smokePos, EWeaponType.WT_SMOKEGRENADE, 1,
			SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE + SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER);
		utility.AddAction(smokeThrow);

		DCO_Perf.Count("sp:smoke");
		GetLastSmokeThrowTime().Set(me, now_ms);
		if (group)
			GetLastGroupSmokeThrowTime().Set(group, now_ms);
		GetRecentSmokePositions().Insert(smokePos);
		GetRecentSmokeTimes().Insert(now_ms);

		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-", string.Format("smoke unit=%1 threat_dist=%2 supp=%3 danger=%4",
			DCO_Log.IdUnit(me), Math.Round(distToThreat), utility.m_ThreatSystem.GetSuppressionMeasure().ToString(1, 2), dangerSeverity.ToString(1, 1)));
		return true;
	}
}
