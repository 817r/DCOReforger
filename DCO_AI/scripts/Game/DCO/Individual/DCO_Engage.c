class DCO_EngageInfo
{
	float m_fAt;
	int m_iZone;
	float m_fDist;
	float m_fDistFresh;
	int m_iKnown;
	vector m_vCentroid;
	vector m_vNearest;
}

class DCO_Engage
{
	protected static ref map<SCR_AIUtilityComponent, ref DCO_EngageInfo> s_mInfo;
	protected static ref map<SCR_AIUtilityComponent, string> s_mAdvLast;

	static DCO_EngageInfo Info(SCR_AIUtilityComponent u)
	{
		if (!u || !u.m_OwnerEntity || !GetGame().GetWorld())
			return null;
		if (!s_mInfo)
			s_mInfo = new map<SCR_AIUtilityComponent, ref DCO_EngageInfo>();
		float now = GetGame().GetWorld().GetWorldTime();
		DCO_EngageInfo i = s_mInfo.Get(u);
		if (i && now >= i.m_fAt && now - i.m_fAt < 1000.0)
			return i;
		if (!i)
		{
			if (s_mInfo.Count() > 600)
				s_mInfo.Clear();
			i = new DCO_EngageInfo();
			s_mInfo.Set(u, i);
		}
		i.m_fAt = now;
		i.m_iZone = ComputeZone(u);
		ComputeEnemies(u, i);
		return i;
	}

	static string ZoneName(int zone)
	{
		if (zone == 2)
			return "cqb";
		if (zone == 1)
			return "urban";
		return "field";
	}

	static string ZoneOf(SCR_AIUtilityComponent u)
	{
		DCO_EngageInfo i = Info(u);
		if (!i)
			return "-";
		return ZoneName(i.m_iZone);
	}

	protected static SCR_AIGroupUtilityComponent GroupUtil(SCR_AIUtilityComponent u)
	{
		if (!u.GetAIAgent())
			return null;
		return DCO_AssaultLock.GroupUtil(u.GetAIAgent().GetParentGroup());
	}

	protected static int ComputeZone(SCR_AIUtilityComponent u)
	{
		IEntity me = u.m_OwnerEntity;
		if (SCR_CoverManagerComponent.IsEntityInsideBuilding(me))
			return 2;
		SCR_AIGroupUtilityComponent gu = GroupUtil(u);
		if (gu && gu.DCO_IsClearingBuilding())
			return 2;
		AIActionBase act = u.GetCurrentAction();
		if (act)
		{
			string cn = act.ClassName();
			if (cn.Contains("CQB") || cn.Contains("Garrison") || cn.Contains("IndoorRelocate"))
				return 2;
		}
		if (!gu)
			return 0;
		array<IEntity> blds = gu.DCO_GetNearbyBuildings(GetGame().GetWorld().GetWorldTime());
		if (!blds)
			return 0;
		vector p = me.GetOrigin();
		foreach (IEntity b : blds)
		{
			if (b && vector.DistanceSqXZ(b.GetOrigin(), p) < 900.0)
				return 1;
		}
		return 0;
	}

	static void ComputeEnemies(SCR_AIUtilityComponent u, DCO_EngageInfo i)
	{
		i.m_fDist = -1;
		i.m_fDistFresh = -1;
		i.m_iKnown = 0;
		i.m_vCentroid = vector.Zero;
		i.m_vNearest = vector.Zero;
		if (!u.m_PerceptionComponent)
			return;
		array<BaseTarget> targets = {};
		u.m_PerceptionComponent.GetTargetsList(targets, ETargetCategory.ENEMY);
		vector p = u.m_OwnerEntity.GetOrigin();
		vector sum = vector.Zero;
		float best = -1;
		foreach (BaseTarget t : targets)
		{
			if (!t || t.GetTimeSinceSeen() > 20.0)
				continue;
			vector tp = t.GetLastSeenPosition();
			float d = vector.Distance(tp, p);
			sum = sum + tp;
			if (t.GetTimeSinceSeen() <= 5.0 && (i.m_fDistFresh < 0 || d < i.m_fDistFresh))
				i.m_fDistFresh = d;
			i.m_iKnown++;
			if (best < 0 || d < best)
			{
				best = d;
				i.m_vNearest = tp;
			}
		}
		if (i.m_iKnown > 0)
			i.m_vCentroid = sum * (1.0 / i.m_iKnown);
		i.m_fDist = best;
	}

	static bool Attacker(SCR_AIUtilityComponent u)
	{
		AIAgent ag = u.GetAIAgent();
		if (!ag || !ag.GetParentGroup())
			return false;
		AIGroup g = ag.GetParentGroup();
		DCO_GroupUtilityComponent du = DCO_GroupUtilityComponent.Cast(g.FindComponent(DCO_GroupUtilityComponent));
		if (du)
		{
			DCO_EGroupTask t = du.GetTask();
			if (t == DCO_EGroupTask.ATTACK || t == DCO_EGroupTask.FLANK)
				return true;
			if (du.GetMyCommander())
				return false;
		}
		SCR_AIGroup sg = SCR_AIGroup.Cast(g);
		if (!sg)
			return false;
		return SCR_SearchAndDestroyWaypoint.Cast(sg.GetCurrentWaypoint()) != null;
	}

	static float MinDist(SCR_AIUtilityComponent u)
	{
		float d = BaseMinDist(u);
		if (DCO_Stress.T(u, "tm.atk_close_in", 1.0) > 0 && Attacker(u))
			d *= DCO_Stress.T(u, "tm.atk_min_dist_mul", 0.4);
		return d;
	}

	static float BaseMinDist(SCR_AIUtilityComponent u)
	{
		DCO_GroupTactics posture = DCO_PostureCombatUtility.GetPosture(u);
		if (posture == DCO_GroupTactics.AGGRESIVE)
			return DCO_Stress.TA(u, "tm.min_dist_agg_m", 30.0, -0.5, 15.0, 200.0);
		if (posture == DCO_GroupTactics.DEFENSIVE || posture == DCO_GroupTactics.EVASIVE || posture == DCO_GroupTactics.DELAY)
			return DCO_Stress.TA(u, "tm.min_dist_def_m", 80.0, -0.5, 40.0, 300.0);
		return DCO_Stress.TA(u, "tm.min_dist_bal_m", 50.0, -0.5, 25.0, 250.0);
	}

	static bool Assaulting(SCR_AIUtilityComponent u, out string why)
	{
		why = "";
		SCR_AIGroupUtilityComponent gu = GroupUtil(u);
		if (!gu)
			return false;
		if (gu.DCO_IsOvermatch())
		{
			why = "overmatch";
			return true;
		}
		if (gu.DCO_IsClearingBuilding())
		{
			why = "clearing";
			return true;
		}
		if (DCO_AssaultLock.Mode(u.GetAIAgent().GetParentGroup()) != 0)
		{
			why = "assault_lock";
			return true;
		}
		return false;
	}

	static bool BoundTurn(SCR_AIUtilityComponent u)
	{
		SCR_AIGroupUtilityComponent gu = GroupUtil(u);
		if (!gu || !gu.m_FireteamMgr)
			return false;
		float period = DCO_PostureCombatUtility.BoundPeriod_ms(DCO_PostureCombatUtility.GetPosture(u));
		if (period <= 0)
			return false;
		bool bounding;
		bool mine = gu.m_FireteamMgr.DCO_IsBoundingTurn(u.GetAIAgent(), bounding, period);
		return bounding && mine;
	}

	static bool AdvanceAllowed(SCR_AIUtilityComponent u, string src)
	{
		DCO_EngageInfo i = Info(u);
		if (!i)
			return true;
		bool ok = true;
		string why = "ok";
		string aw;
		if (i.m_iZone != 0)
			why = "zone_" + ZoneName(i.m_iZone);
		else if (Assaulting(u, aw))
			why = aw;
		else if (CqbPush(u))
			why = "cqb_range";
		else
		{
			int lvl = DCO_Stress.Level(u);
			if (lvl >= 1 && !(BoundTurn(u) && i.m_fDist > 30.0))
			{
				if (DCO_MoveBrain.CoveredAdvance(u))
				{
					why = "mb_covered";
					if (i.m_fDistFresh >= 0 && i.m_fDistFresh < MinDist(u))
					{
						ok = false;
						why = "too_close";
					}
				}
				else
				{
					ok = false;
					why = "under_fire";
				}
			}
			else if (i.m_fDistFresh >= 0 && i.m_fDistFresh < MinDist(u))
			{
				ok = false;
				why = "too_close";
			}
			else if (i.m_fDistFresh >= 0 && i.m_fDistFresh < BaseMinDist(u))
				why = "close_in";
		}
		DCO_Perf.Count("tm:adv_" + why);
		NoteAdvance(u, ok, why, src, i);
		return ok;
	}

	protected static ref map<SCR_AIUtilityComponent, float> s_mCqbUntil;
	protected static ref map<SCR_AIUtilityComponent, bool> s_mCqbPush;

	static float CqbRange(SCR_AIUtilityComponent u)
	{
		return DCO_Stress.T(u, "tm.cqb_range_m", 25.0);
	}

	protected static ref map<SCR_AIUtilityComponent, float> s_mCqbKindUntil;
	protected static ref map<SCR_AIUtilityComponent, int> s_mCqbKind;

	static int CqbKind(SCR_AIUtilityComponent u, float targetDist)
	{
		if (!u || targetDist < 0 || targetDist > CqbRange(u))
			return 0;
		if (!s_mCqbKindUntil)
		{
			s_mCqbKindUntil = new map<SCR_AIUtilityComponent, float>();
			s_mCqbKind = new map<SCR_AIUtilityComponent, int>();
		}
		float now = GetGame().GetWorld().GetWorldTime();
		float until;
		if (s_mCqbKindUntil.Find(u, until) && now < until)
			return s_mCqbKind.Get(u);
		if (s_mCqbKindUntil.Count() > 600)
		{
			s_mCqbKindUntil.Clear();
			s_mCqbKind.Clear();
		}
		int kind = 2;
		if (DCO_AssaultLock.InCQB(u) || DCO_CQC.IsTargetInMyBuilding(u) || SCR_CoverManagerComponent.IsEntityInsideBuilding(u.m_OwnerEntity))
			kind = 1;
		int prev = -1;
		if (s_mCqbKind.Contains(u))
			prev = s_mCqbKind.Get(u);
		s_mCqbKindUntil.Set(u, now + 2000.0);
		s_mCqbKind.Set(u, kind);
		if (kind != prev)
		{
			DCO_Perf.Count("tm:cqb_kind_" + CqbKindName(kind));
			if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("cqb_kind unit=%1 kind=%2 enemy_d=%3", DCO_Log.IdUnit(u.m_OwnerEntity), CqbKindName(kind), Math.Round(targetDist)));
		}
		return kind;
	}

	static string CqbKindName(int kind)
	{
		switch (kind)
		{
			case 1: return "indoor";
			case 2: return "outdoor";
		}
		return "none";
	}

	static float CqbAggr(SCR_AIUtilityComponent u, float targetDist)
	{
		int kind = CqbKind(u, targetDist);
		if (kind == 1)
			return Math.Max(DCO_Stress.T(u, "tm.cqb_aggr_in", 4.0), 1.0);
		if (kind == 2)
			return Math.Max(DCO_Stress.T(u, "tm.cqb_aggr_out", 4.0), 1.0);
		return 1.0;
	}

	static bool CqbPush(SCR_AIUtilityComponent u)
	{
		DCO_EngageInfo i = Info(u);
		if (!i || i.m_fDistFresh < 0 || i.m_fDistFresh > CqbRange(u))
			return false;
		if (!s_mCqbUntil)
		{
			s_mCqbUntil = new map<SCR_AIUtilityComponent, float>();
			s_mCqbPush = new map<SCR_AIUtilityComponent, bool>();
		}
		float now = GetGame().GetWorld().GetWorldTime();
		float until;
		if (s_mCqbUntil.Find(u, until) && now < until)
			return s_mCqbPush.Get(u);
		if (s_mCqbUntil.Count() > 600)
		{
			s_mCqbUntil.Clear();
			s_mCqbPush.Clear();
		}
		float chance = Math.Clamp(DCO_Stress.T(u, "tm.cqb_chance", 0.6), 0.0, 1.0);
		float aggr = CqbAggr(u, i.m_fDistFresh);
		chance = 1.0 - (1.0 - chance) / aggr;
		bool push = Math.RandomFloat01() < chance;
		s_mCqbUntil.Set(u, now + 10000.0);
		s_mCqbPush.Set(u, push);
		string result = "hold";
		if (push)
			result = "push";
		DCO_Perf.Count("tm:cqb_" + result);
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("cqb_engage unit=%1 result=%2 enemy_d=%3 range=%4 chance=%5 zone=%6 s=%7 aggr=%8 kind=%9",
				DCO_Log.IdUnit(u.m_OwnerEntity), result, Math.Round(i.m_fDistFresh), Math.Round(CqbRange(u)), chance.ToString(-1, 2), ZoneName(i.m_iZone), DCO_Stress.LevelName(DCO_Stress.Level(u)), aggr.ToString(-1, 1), CqbKindName(CqbKind(u, i.m_fDistFresh))));
		return push;
	}

	static bool SearchAllowed(SCR_AIUtilityComponent u)
	{
		DCO_EngageInfo i = Info(u);
		if (!i || i.m_iZone != 0)
			return true;
		string aw;
		if (Assaulting(u, aw))
			return true;
		return CqbPush(u);
	}

	static bool ApproachAllowed(SCR_AIUtilityComponent u, vector dest, string src)
	{
		DCO_EngageInfo i = Info(u);
		if (!i)
			return true;
		bool ok = true;
		string why = "ok";
		string aw;
		if (i.m_iZone != 0)
			why = "zone_" + ZoneName(i.m_iZone);
		else if (Assaulting(u, aw))
			why = aw;
		else if (CqbPush(u))
			why = "cqb_range";
		else if (DCO_Stress.Level(u) >= 1)
		{
			ok = false;
			why = "under_fire";
		}
		else if (i.m_vNearest != vector.Zero && i.m_fDistFresh >= 0 && vector.Distance(dest, i.m_vNearest) < MinDist(u) && vector.Distance(dest, i.m_vNearest) < i.m_fDist)
		{
			ok = false;
			why = "too_close";
		}
		DCO_Perf.Count("tm:approach_" + src + "_" + why);
		NoteAdvance(u, ok, why, src, i);
		return ok;
	}

	protected static void NoteAdvance(SCR_AIUtilityComponent u, bool ok, string why, string src, DCO_EngageInfo i)
	{
		if (!s_mAdvLast)
			s_mAdvLast = new map<SCR_AIUtilityComponent, string>();
		if (s_mAdvLast.Get(u) == why)
			return;
		if (s_mAdvLast.Count() > 600)
			s_mAdvLast.Clear();
		s_mAdvLast.Set(u, why);
		if (!DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
			return;
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("advance_gate unit=%1 ok=%2 why=%3 src=%4 zone=%5 enemy_d=%6 known=%7 s=%8 min_d=%9",
			DCO_Log.IdUnit(u.m_OwnerEntity), DCO_Util.B01(ok), why, src, ZoneName(i.m_iZone), Math.Round(i.m_fDist), i.m_iKnown, DCO_Stress.LevelName(DCO_Stress.Level(u)), Math.Round(MinDist(u))));
	}

	static bool Supported(SCR_AIUtilityComponent u)
	{
		if (!DCO_Stress.Isolated(u, 30.0, true))
			return true;
		if (!u.m_CombatComponent)
			return false;
		BaseTarget t = u.m_CombatComponent.GetCurrentTarget();
		return t && t.GetTimeSinceSeen() > 3.0;
	}

	static string RushCheck(SCR_AIUtilityComponent u, out vector coverPos)
	{
		coverPos = vector.Zero;
		DCO_EngageInfo i = Info(u);
		if (!i)
			return "no_info";
		if (i.m_fDist >= 0 && i.m_fDist <= DCO_Stress.T(u, "tm.rush_enemy_min_m", 30.0))
			return "enemy_close";
		vector threat = i.m_vCentroid;
		if (threat == vector.Zero)
			threat = DCO_Stress.ThreatPos(u);
		if (threat == vector.Zero)
			return "no_threat";
		if (!DCO_Stress.FindCover(u.m_OwnerEntity, threat, DCO_Stress.T(u, "tm.rush_cover_max_m", 15.0), coverPos))
			return "no_cover";
		if (!Supported(u))
			return "no_support";
		return "ok";
	}
}
