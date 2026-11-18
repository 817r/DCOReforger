modded class SCR_AIGroupUtilityComponent
{
	float m_fDCOFtContactAt_ms = -1;
	float m_fDCOFtNearest0;
	float m_fDCOFtNext_ms;
	float m_fDCOFtUntil_ms;
	float m_fDCOFtStart_ms;
	float m_fDCOFtSuppAt_ms = -1;
	int m_iDCOFtMove0;
	vector m_vDCOFtEnemy;
	vector m_vDCOFtGoal;
	ref array<SCR_AIUtilityComponent> m_aDCOFtBase = {};
	ref array<SCR_AIUtilityComponent> m_aDCOFtMove = {};
}

class DCO_FireteamFlank
{
	static void Tick(SCR_AIGroupUtilityComponent u, SCR_AIGroup grp, DCO_GroupUtilityComponent g, float now_ms)
	{
		if (!u || !grp || !u.m_Perception || !u.m_FireteamMgr || !grp.GetLeaderEntity())
			return;
		if (u.m_fDCOFtUntil_ms > 0)
		{
			Run(u, grp, now_ms);
			return;
		}
		if (now_ms < u.m_fDCOFtNext_ms)
			return;
		u.m_fDCOFtNext_ms = now_ms + 5000.0;
		if (DCO_Ctx.TuneFor(grp, "ft.flank", 1.0) <= 0 || Blocked(u, grp, g))
		{
			u.m_fDCOFtContactAt_ms = -1;
			return;
		}
		vector enemy;
		float nearest;
		if (!Enemy(u, grp, enemy, nearest) || nearest < 40.0 || nearest > 250.0)
		{
			u.m_fDCOFtContactAt_ms = -1;
			return;
		}
		if (u.m_fDCOFtContactAt_ms < 0 || nearest < u.m_fDCOFtNearest0 - 10.0)
		{
			u.m_fDCOFtContactAt_ms = now_ms;
			u.m_fDCOFtNearest0 = nearest;
			return;
		}
		if (now_ms - u.m_fDCOFtContactAt_ms < 45000.0)
			return;
		Start(u, grp, enemy, nearest, now_ms);
	}

	protected static bool Blocked(SCR_AIGroupUtilityComponent u, SCR_AIGroup grp, DCO_GroupUtilityComponent g)
	{
		if (u.m_bDCOPinGroup || u.DCO_GetGarrisonMode() >= 0 || DCO_CQBActivity.Cast(u.GetCurrentAction()))
			return true;
		if (g && g.IsPlayerGroup())
			return true;
		if (GetGame().GetPlayerManager() && GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(grp.GetLeaderEntity()) != 0)
			return true;
		DCO_GroupTactics post = u.DCO_GetPosture();
		return post == DCO_GroupTactics.DEFENSIVE || post == DCO_GroupTactics.EVASIVE || post == DCO_GroupTactics.AMBUSH || DCO_PostureCombatUtility.IsHoldPosture(post);
	}

	protected static bool Enemy(SCR_AIGroupUtilityComponent u, SCR_AIGroup grp, out vector enemy, out float nearest)
	{
		nearest = float.MAX;
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!pm)
			return false;
		float pmNow = pm.GetTime();
		vector lp = grp.GetLeaderEntity().GetOrigin();
		vector sum;
		int n;
		foreach (SCR_AITargetInfo t : u.m_Perception.m_aTargets)
		{
			if (!t || pmNow - t.m_fTimestamp > 10.0)
				continue;
			if (t.m_eCategory != EAITargetInfoCategory.DETECTED && t.m_eCategory != EAITargetInfoCategory.IDENTIFIED)
				continue;
			sum = sum + t.m_vWorldPos;
			n++;
			nearest = Math.Min(nearest, vector.DistanceXZ(lp, t.m_vWorldPos));
		}
		if (n == 0)
			return false;
		enemy = sum * (1.0 / n);
		return true;
	}

	protected static bool Usable(SCR_AIUtilityComponent mu)
	{
		return mu && mu.m_OwnerEntity && DCO_Util.IsAlive(mu.m_OwnerEntity);
	}

	protected static bool HasMG(notnull array<SCR_AIUtilityComponent> team)
	{
		foreach (SCR_AIUtilityComponent mu : team)
		{
			if ((DCO_Roles.Get(mu) & 65) != 0)
				return true;
		}
		return false;
	}

	protected static float MeanDist(notnull array<SCR_AIUtilityComponent> team, vector pos)
	{
		float sum;
		foreach (SCR_AIUtilityComponent mu : team)
			sum += vector.DistanceXZ(mu.m_OwnerEntity.GetOrigin(), pos);
		return sum / Math.Max(team.Count(), 1);
	}

	protected static void Start(SCR_AIGroupUtilityComponent u, SCR_AIGroup grp, vector enemy, float nearest, float now_ms)
	{
		u.m_fDCOFtContactAt_ms = -1;
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		array<SCR_AIGroupFireteam> fts = {};
		array<ref array<SCR_AIUtilityComponent>> teams = {};
		int alive;
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !Usable(ca.m_UtilityComponent))
				continue;
			if (ca.m_UtilityComponent.m_AIInfo && ca.m_UtilityComponent.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
				return;
			SCR_AIGroupFireteam ft = u.m_FireteamMgr.FindFireteam(a);
			if (!ft)
				continue;
			int idx = fts.Find(ft);
			if (idx < 0)
			{
				fts.Insert(ft);
				teams.Insert(new array<SCR_AIUtilityComponent>());
				idx = fts.Count() - 1;
			}
			teams[idx].Insert(ca.m_UtilityComponent);
			alive++;
		}
		int first = -1;
		int second = -1;
		foreach (int ti, array<SCR_AIUtilityComponent> team : teams)
		{
			if (first < 0 || team.Count() > teams[first].Count())
			{
				second = first;
				first = ti;
			}
			else if (second < 0 || team.Count() > teams[second].Count())
				second = ti;
		}
		if (alive < 6 || second < 0 || teams[second].Count() < 2)
		{
			u.m_fDCOFtNext_ms = now_ms + 30000.0;
			DCO_Perf.Count("ft_flank:skip_size");
			return;
		}
		int baseIdx = first;
		int moveIdx = second;
		bool firstMG = HasMG(teams[first]);
		bool secondMG = HasMG(teams[second]);
		if (secondMG && !firstMG)
		{
			baseIdx = second;
			moveIdx = first;
		}
		else if (firstMG == secondMG && MeanDist(teams[second], enemy) < MeanDist(teams[first], enemy))
		{
			baseIdx = second;
			moveIdx = first;
		}

		vector back = DCO_Specialist.Flat(grp.GetLeaderEntity().GetOrigin() - enemy);
		float dist = Math.Clamp(nearest * 0.7, 40.0, 120.0);
		float angDeg = Math.RandomFloat(60.0, 90.0);
		float coin = 1.0;
		if (Math.RandomIntInclusive(0, 1) == 1)
			coin = -1.0;
		string sideWhy;
		float side = DCO_FlushDrill.PickSide(enemy, enemy + DCO_FlushDrill.Rot(back, angDeg) * dist, enemy + DCO_FlushDrill.Rot(back, -angDeg) * dist, coin, sideWhy);
		vector goal;
		if (!DCO_FlushDrill.Reachable(enemy + DCO_FlushDrill.Rot(back, angDeg * side) * dist, goal))
		{
			side = -side;
			sideWhy = "nav";
			if (!DCO_FlushDrill.Reachable(enemy + DCO_FlushDrill.Rot(back, angDeg * side) * dist, goal))
			{
				u.m_fDCOFtNext_ms = now_ms + 30000.0;
				DCO_Perf.Count("ft_flank:skip_no_nav");
				return;
			}
		}

		u.m_aDCOFtBase.Clear();
		u.m_aDCOFtMove.Clear();
		foreach (SCR_AIUtilityComponent bu : teams[baseIdx])
			u.m_aDCOFtBase.Insert(bu);
		foreach (SCR_AIUtilityComponent mu : teams[moveIdx])
			u.m_aDCOFtMove.Insert(mu);
		u.m_iDCOFtMove0 = u.m_aDCOFtMove.Count();
		u.m_vDCOFtEnemy = enemy;
		u.m_vDCOFtGoal = goal;
		u.m_fDCOFtStart_ms = now_ms;
		u.m_fDCOFtUntil_ms = now_ms + 90000.0;
		u.m_fDCOFtSuppAt_ms = -1;
		DCO_Perf.Count("ft_flank:start");
		DCO_Perf.Count("ft_flank:side_" + sideWhy);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("ft_flank_start grp=%1 base=%2 move=%3 fireteams=%4 enemy=%5 enemy_d=%6 angle=%7 flank_d=%8 goal=%9",
			DCO_Log.IdGroup(grp), u.m_aDCOFtBase.Count(), u.m_aDCOFtMove.Count(), teams.Count(), DCO_Util.XZ(enemy), Math.Round(nearest), Math.Round(angDeg * side), Math.Round(dist), DCO_Util.XZ(goal)) + " side_why=" + sideWhy);
		Run(u, grp, now_ms);
	}

	protected static void Order(SCR_AIGroupUtilityComponent u)
	{
		vector tangent = DCO_FlushDrill.Rot(DCO_Specialist.Flat(u.m_vDCOFtGoal - u.m_vDCOFtEnemy), 90.0);
		int n = u.m_aDCOFtMove.Count();
		foreach (int i, SCR_AIUtilityComponent mu : u.m_aDCOFtMove)
		{
			if (!Usable(mu) || mu.FindActionOfType(DCO_RelocateBehavior))
				continue;
			vector p = DCO_Util.Surface(u.m_vDCOFtGoal + tangent * (4.0 * (i - (n - 1) * 0.5)));
			if (vector.DistanceXZ(mu.m_OwnerEntity.GetOrigin(), p) <= 10.0)
				continue;
			DCO_RelocateBehavior rb = DCO_RelocateBehavior.Start(mu, p, u.m_vDCOFtEnemy, true, ECharacterStance.CROUCH, 30.0, "ft_flank");
			if (rb)
				rb.DCO_SetYieldM(30.0);
		}
	}

	protected static void Run(SCR_AIGroupUtilityComponent u, SCR_AIGroup grp, float now_ms)
	{
		int moveAlive;
		int arrived;
		foreach (SCR_AIUtilityComponent mu : u.m_aDCOFtMove)
		{
			if (!Usable(mu))
				continue;
			moveAlive++;
			if (vector.DistanceXZ(mu.m_OwnerEntity.GetOrigin(), u.m_vDCOFtGoal) <= 12.0)
				arrived++;
		}
		string result;
		if (u.m_bDCOPinGroup)
			result = "pinned";
		else if (moveAlive * 2 < u.m_iDCOFtMove0)
			result = "losses";
		else if (arrived * 2 >= moveAlive)
			result = "arrived";
		else if (now_ms >= u.m_fDCOFtUntil_ms)
			result = "timeout";
		if (!result.IsEmpty())
		{
			DCO_Perf.Count("ft_flank:end_" + result);
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("ft_flank_end grp=%1 result=%2 arrived=%3 move_alive=%4 move_start=%5 s=%6",
				DCO_Log.IdGroup(grp), result, arrived, moveAlive, u.m_iDCOFtMove0, Math.Round((now_ms - u.m_fDCOFtStart_ms) / 1000.0)));
			u.m_fDCOFtUntil_ms = 0;
			u.m_fDCOFtNext_ms = now_ms + 120000.0;
			u.m_aDCOFtBase.Clear();
			u.m_aDCOFtMove.Clear();
			return;
		}
		if (u.m_fDCOFtSuppAt_ms >= 0 && now_ms - u.m_fDCOFtSuppAt_ms < 6000.0)
			return;
		u.m_fDCOFtSuppAt_ms = now_ms;
		foreach (SCR_AIUtilityComponent bu : u.m_aDCOFtBase)
		{
			if (Usable(bu))
				DCO_PinnedGroup.Suppress(bu, u.m_vDCOFtEnemy);
		}
		Order(u);
	}
}
