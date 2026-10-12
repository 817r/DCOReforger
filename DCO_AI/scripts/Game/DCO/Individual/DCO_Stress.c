class DCO_Stress
{
	static float T(SCR_AIUtilityComponent u, string key, float def)
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(u);
		if (!g)
			return def;
		return g.DCO_TuneF(key, def);
	}

	static float AggrRel(SCR_AIUtilityComponent u)
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(u);
		if (!g || g.DCO_TuneF("agg.profile", 1.0) <= 0)
			return 1.0;
		return Math.Clamp(g.DCO_AggressionSlider(), 0.5, 4.0);
	}

	static float AggrRelGlobal()
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetInstance();
		if (!g || g.DCO_TuneF("agg.profile", 1.0) <= 0)
			return 1.0;
		return Math.Clamp(g.DCO_AggressionSlider(), 0.5, 4.0);
	}

	static float TA(SCR_AIUtilityComponent u, string key, float def, float e, float lo, float hi)
	{
		float v = T(u, key, def);
		float a = AggrRel(u);
		if (a == 1.0)
			return v;
		return Math.Clamp(v * Math.Pow(a, e), lo, hi);
	}

	static float Aggr(SCR_AIUtilityComponent u)
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForUtility(u);
		if (!g)
			return 1.0;
		return g.DCO_Aggression();
	}

	static int Level(SCR_AIUtilityComponent u)
	{
		if (!u || !u.m_CombatComponent)
			return 0;
		return u.m_CombatComponent.DCO_StressLevel();
	}

	static int Tier(SCR_AIUtilityComponent u)
	{
		if (!u)
			return 0;
		int tier = Level(u);
		DCO_AIMoraleSystem ms = u.GetMoraleSystem();
		if (ms)
		{
			moraleState st = ms.GetState();
			if (st == moraleState.SHAKEN)
				tier = Math.Max(tier, 1);
			else if (st == moraleState.PANIC)
				tier = 3;
			else if (st == moraleState.BREAK)
				tier = 4;
		}
		return tier;
	}

	static string TierName(int tier)
	{
		switch (tier)
		{
			case 1: return "shaken";
			case 2: return "pinned";
			case 3: return "panic";
			case 4: return "break";
		}
		return "calm";
	}

	static string LevelName(int lvl)
	{
		return "S" + lvl.ToString();
	}

	static float Speed(SCR_AICombatMoveRequest_Move rq)
	{
		if (rq.m_eStanceMoving == ECharacterStance.PRONE)
			return 0.8;
		if (rq.m_eMovementType == EMovementType.SPRINT)
			return SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;
		if (rq.m_eStanceMoving == ECharacterStance.CROUCH)
			return SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN;
		return SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_RUN;
	}

	static int Shape(SCR_AIUtilityComponent u, SCR_AICombatMoveRequest_Move rq, bool close, bool inCover, bool limitDist, inout float distMin, inout float distMax, string src)
	{
		int tier = Tier(u);
		float rush;
		float lo;
		float hi;
		switch (tier)
		{
			case 0:
			{
				rush = Math.RandomFloat(T(u, "tm.rush_calm_min_s", 3.0), T(u, "tm.rush_calm_max_s", 5.0));
				lo = 10.0;
				hi = 20.0;
				if (close)
				{
					lo = 5.0;
					hi = 10.0;
				}
				break;
			}
			case 1:
			{
				rush = Math.RandomFloat(T(u, "tm.rush_shaken_min_s", 2.0), T(u, "tm.rush_shaken_max_s", 3.0));
				lo = 6.0;
				hi = 12.0;
				if (close)
				{
					lo = 3.0;
					hi = 6.0;
				}
				if (rq.m_eStanceMoving == ECharacterStance.STAND && rq.m_eMovementType != EMovementType.SPRINT)
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
				if (!inCover)
					rq.m_eStanceEnd = ECharacterStance.PRONE;
				break;
			}
			case 2:
			{
				lo = 1.0;
				hi = T(u, "tm.pin_crawl_max_m", 15.0);
				rq.m_eStanceMoving = ECharacterStance.PRONE;
				rq.m_eStanceEnd = ECharacterStance.PRONE;
				rq.m_eMovementType = EMovementType.WALK;
				break;
			}
			case 3:
			{
				lo = 5.0;
				hi = T(u, "tm.panic_flight_max_m", 30.0);
				rq.m_eStanceMoving = ECharacterStance.STAND;
				rq.m_eStanceEnd = ECharacterStance.PRONE;
				rq.m_eMovementType = EMovementType.SPRINT;
				rq.m_bAimAtTarget = false;
				break;
			}
			default:
			{
				lo = 10.0;
				hi = 40.0;
				rq.m_eStanceMoving = ECharacterStance.STAND;
				rq.m_eStanceEnd = ECharacterStance.CROUCH;
				rq.m_eMovementType = EMovementType.SPRINT;
				rq.m_bAimAtTarget = false;
				break;
			}
		}
		float speed = Speed(rq);
		if (!limitDist && (tier == 1 || tier == 2))
		{
			float cap = T(u, "tm.open_cover_max_m", 15.0);
			distMax = Math.Min(distMax, cap);
			distMin = Math.Min(distMin, distMax * 0.3);
		}
		if (limitDist)
		{
			if (tier <= 1)
				hi = Math.Min(hi, rush * speed);
			if (inCover)
				distMin = Math.Min(lo, hi * 0.5);
			else
				distMin = 0.5;
			distMax = Math.Max(hi, distMin + 2.0);
		}
		float dur = distMax / Math.Max(speed, 0.5) + 1.0;
		if (tier <= 1 && limitDist)
			dur = rush;
		rq.m_fMoveDuration_s = dur;
		if (tier >= 1)
			rq.m_bFailIfNoCover = false;
		DCO_Perf.Count("tm:profile_" + TierName(tier));
		if (tier >= 1 && u.m_OwnerEntity)
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("move_profile unit=%1 src=%2 s=%3 tier=%4 rush_s=%5 dist=%6-%7 stance=%8 end=%9",
				DCO_Log.IdUnit(u.m_OwnerEntity), src, LevelName(Level(u)), TierName(tier), dur.ToString(-1, 1), Math.Round(distMin), Math.Round(distMax), typename.EnumToString(ECharacterStance, rq.m_eStanceMoving), typename.EnumToString(ECharacterStance, rq.m_eStanceEnd)));
		return tier;
	}

	static bool LeaderNear(SCR_AIUtilityComponent u, float r)
	{
		if (!u || !u.m_OwnerEntity || !u.GetAIAgent())
			return false;
		AIGroup g = u.GetAIAgent().GetParentGroup();
		if (!g)
			return false;
		IEntity l = g.GetLeaderEntity();
		return l && l != u.m_OwnerEntity && DCO_Util.IsAlive(l) && vector.Distance(l.GetOrigin(), u.m_OwnerEntity.GetOrigin()) <= r;
	}

	static bool Isolated(SCR_AIUtilityComponent u, float r, bool activeOnly)
	{
		if (!u || !u.m_OwnerEntity || !u.GetAIAgent())
			return false;
		AIGroup g = u.GetAIAgent().GetParentGroup();
		if (!g)
			return true;
		array<AIAgent> agents = {};
		g.GetAgents(agents);
		vector me = u.m_OwnerEntity.GetOrigin();
		foreach (AIAgent a : agents)
		{
			IEntity e = a.GetControlledEntity();
			if (!e || e == u.m_OwnerEntity || !DCO_Util.IsAlive(e))
				continue;
			if (vector.Distance(e.GetOrigin(), me) > r)
				continue;
			if (!activeOnly)
				return false;
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (ca && ca.m_UtilityComponent && ca.m_UtilityComponent.m_CombatComponent && ca.m_UtilityComponent.m_CombatComponent.GetCurrentTarget())
				return false;
		}
		return true;
	}

	static vector ThreatPos(SCR_AIUtilityComponent u)
	{
		if (!u || !u.m_CombatComponent)
			return vector.Zero;
		if (u.m_CombatComponent.m_vDCOPinFrom != vector.Zero)
			return u.m_CombatComponent.m_vDCOPinFrom;
		BaseTarget t = u.m_CombatComponent.GetCurrentTarget();
		if (t)
			return t.GetLastSeenPosition();
		return vector.Zero;
	}

	static bool FindCover(IEntity me, vector threat, float maxDist, out vector coverPos, vector toward = "0 0 0")
	{
		if (!me || threat == vector.Zero || !GetGame().GetAIWorld())
			return false;
		ChimeraCoverManagerComponent mgr = ChimeraCoverManagerComponent.Cast(GetGame().GetAIWorld().FindComponent(ChimeraCoverManagerComponent));
		AIPathfindingComponent pf = AIPathfindingComponent.Cast(me.FindComponent(AIPathfindingComponent));
		if (!mgr || !pf)
		{
			DCO_Perf.Count("tm:cover_q_nomgr");
			return false;
		}
		vector p = me.GetOrigin();
		vector away = p - threat;
		away[1] = 0;
		away.Normalize();
		CoverQueryProperties q = new CoverQueryProperties();
		q.m_vNearestPolyHalfExtend = SCR_AIFindCover.NEAREST_POLY_HALF_EXTEND;
		q.m_fNmAreaCostScale = SCR_AIFindCover.NAVMESH_AREA_COST_SCALE;
		q.m_vSectorPos = p;
		q.m_vSectorDir = away;
		q.m_fQuerySectorAngleCosMin = -1.0;
		if (toward != vector.Zero)
		{
			vector tw = toward;
			tw[1] = 0;
			if (tw.Length() > 0.1)
			{
				tw.Normalize();
				q.m_vSectorDir = tw;
				q.m_fQuerySectorAngleCosMin = 0.5;
			}
		}
		q.m_fSectorDistMin = 0;
		q.m_fSectorDistMax = maxDist;
		q.m_vThreatPos = threat;
		q.m_fCoverHeightMin = 0.0;
		q.m_fCoverHeightMax = 10.0;
		q.m_fCoverToThreatAngleCosMin = 0.5;
		q.m_fScoreWeightDirection = 3.0;
		q.m_fScoreWeightDistance = 2.0;
		q.m_bCheckVisibility = true;
		q.m_bSelectHighestScore = true;
		q.m_iMaxCoversToCheck = SCR_AIFindCover.MAX_COVERS_HIGH_PRIORITY;
		vector tallest;
		int tx, ty, id;
		string pass = "strict";
		if (!mgr.GetBestCover("Soldiers", pf, q, coverPos, tallest, tx, ty, id))
		{
			q.m_bCheckVisibility = false;
			q.m_fCoverToThreatAngleCosMin = 0.0;
			pass = "loose";
			if (!mgr.GetBestCover("Soldiers", pf, q, coverPos, tallest, tx, ty, id))
			{
				DCO_Perf.Count("tm:cover_q_none");
				return false;
			}
		}
		float d = vector.DistanceXZ(coverPos, p);
		if (d > maxDist + 2.0)
		{
			DCO_Perf.Count("tm:cover_q_far");
			return false;
		}
		DCO_Perf.Count("tm:cover_q_" + pass);
		return true;
	}

	static vector AwayPoint(IEntity me, vector threat, float dist, float spreadDeg)
	{
		vector p = me.GetOrigin();
		vector away = p - threat;
		away[1] = 0;
		if (away.Length() < 0.5)
			away = me.GetWorldTransformAxis(2) * -1.0;
		away.Normalize();
		float a = Math.RandomFloat(-spreadDeg, spreadDeg) * Math.DEG2RAD;
		vector dir = Vector(away[0] * Math.Cos(a) - away[2] * Math.Sin(a), 0, away[0] * Math.Sin(a) + away[2] * Math.Cos(a));
		return DCO_Util.Surface(p + dir * dist);
	}
}
