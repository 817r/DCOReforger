modded class SCR_AIUtilityComponent
{
	float m_fDCOHitAt = -100000;
	IEntity m_DCOHitBy;
	float m_fDCONmCloseAt = -100000;
	float m_fDCONmLastAt = -100000;
	float m_fDCONmPrevAt = -100000;
	vector m_vDCONmLastPos;
	vector m_vDCONmPrevPos;
}

class DCO_DodgeAssist
{
	static bool On(SCR_AIUtilityComponent u)
	{
		return u && DCO_Stress.T(u, "dodge.dir_assist", 1.0) > 0;
	}

	static float Now()
	{
		return GetGame().GetWorld().GetWorldTime();
	}

	static void NoteShot(SCR_AIUtilityComponent u, float missDist)
	{
		if (!u || !u.m_OwnerEntity)
			return;
		if (missDist > DCO_Stress.T(u, "dodge.nearmiss2_m", 5.0))
			return;
		float now = Now();
		if (missDist <= DCO_Stress.T(u, "dodge.nearmiss_m", 1.5))
			u.m_fDCONmCloseAt = now;
		if (now - u.m_fDCONmLastAt < DCO_Stress.T(u, "dodge.nearmiss_sep_s", 0.5) * 1000.0)
			return;
		u.m_fDCONmPrevAt = u.m_fDCONmLastAt;
		u.m_vDCONmPrevPos = u.m_vDCONmLastPos;
		u.m_fDCONmLastAt = now;
		u.m_vDCONmLastPos = u.m_OwnerEntity.GetOrigin();
	}

	static void NoteHit(SCR_AIUtilityComponent u, IEntity shooter)
	{
		if (!u)
			return;
		u.m_fDCOHitAt = Now();
		u.m_DCOHitBy = shooter;
	}

	static bool HitRecent(SCR_AIUtilityComponent u)
	{
		return Now() - u.m_fDCOHitAt < DCO_Stress.T(u, "dodge.hit_s", 2.0) * 1000.0;
	}

	static bool RecentNearMiss(SCR_AIUtilityComponent u)
	{
		return u && DCO_Stress.T(u, "dodge.nearmiss_gate", 1.0) > 0 && Now() - u.m_fDCONmLastAt < DCO_Stress.T(u, "dodge.nearmiss2_s", 3.0) * 1000.0;
	}

	static bool DoubleNearMiss(SCR_AIUtilityComponent u)
	{
		if (!u.m_OwnerEntity || Now() - u.m_fDCONmPrevAt > DCO_Stress.T(u, "dodge.nearmiss2_s", 3.0) * 1000.0)
			return false;
		vector me = u.m_OwnerEntity.GetOrigin();
		return vector.DistanceXZ(me, u.m_vDCONmPrevPos) < 3.0 && vector.DistanceXZ(me, u.m_vDCONmLastPos) < 3.0;
	}

	static string GateBypass(SCR_AIUtilityComponent u)
	{
		if (!u || DCO_Stress.T(u, "dodge.nearmiss_gate", 1.0) <= 0)
			return string.Empty;
		if (HitRecent(u))
			return "hit";
		if (Now() - u.m_fDCONmCloseAt < DCO_Stress.T(u, "dodge.nearmiss2_s", 3.0) * 1000.0)
			return "nearmiss";
		if (DoubleNearMiss(u))
			return "nearmiss2";
		return string.Empty;
	}

	static float ChestH(IEntity e)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(e);
		if (!ch || !ch.GetCharacterController())
			return 1.45;
		ECharacterStance st = ch.GetCharacterController().GetStance();
		if (st == ECharacterStance.PRONE)
			return 0.35;
		if (st == ECharacterStance.CROUCH)
			return 0.85;
		return 1.45;
	}

	static vector Eye(IEntity src, vector pos)
	{
		if (src)
			return src.GetOrigin() + Vector(0, 1.6, 0);
		return pos + Vector(0, 1.6, 0);
	}

	static bool RayClear(SCR_AIUtilityComponent u, vector from, vector to, IEntity src)
	{
		float len = vector.Distance(from, to);
		if (len < 0.5)
			return true;
		float frac = DCO_Util.TraceFrac(from, to, src, TraceFlags.WORLD | TraceFlags.ENTS, -1, "dodge:ray");
		if (frac * len >= len - DCO_Stress.T(u, "dodge.ray_margin_m", 0.3))
			return true;
		IEntity hit = DCO_Util.LastTraceEnt();
		if (!hit)
			return false;
		return ChimeraCharacter.Cast(hit.GetRootParent()) != null;
	}

	static bool WideCover(IEntity me, vector chest, vector threatPos)
	{
		vector d = threatPos - chest;
		d[1] = 0;
		float l = d.Length();
		if (l < 0.1)
			return false;
		d = d * (1.0 / l);
		vector side = Vector(d[2], 0, -d[0]);
		int blocked = 0;
		for (int i = -1; i <= 1; i += 2)
		{
			vector a = chest + side * (0.75 * i);
			if (DCO_Util.TraceFrac(a, a + d * 2.0, me, TraceFlags.WORLD | TraceFlags.ENTS, -1, "dodge:width") < 1.0)
				blocked++;
		}
		return blocked == 2;
	}

	static IEntity ActiveShooter(SCR_AIUtilityComponent u)
	{
		IEntity src = u.m_DCOReactSrc;
		if (!src || !u.m_OwnerEntity || Now() - u.m_fDCOReactAt > 5000.0)
			return null;
		if (vector.Distance(src.GetOrigin(), u.m_OwnerEntity.GetOrigin()) > 600.0)
			return null;
		return src;
	}

	static string CoverCompromised(SCR_AIUtilityComponent u, vector threatPos, out int rayBlocked, out float ang)
	{
		rayBlocked = -1;
		ang = -1;
		if (!u || !u.m_OwnerEntity || DCO_Stress.T(u, "dodge.cover_vs_shooter", 1.0) <= 0)
			return string.Empty;
		if (HitRecent(u))
			return "hit";
		if (DoubleNearMiss(u))
			return "nearmiss2";
		IEntity me = u.m_OwnerEntity;
		vector chest = me.GetOrigin() + Vector(0, ChestH(me), 0);
		IEntity src = ActiveShooter(u);
		if (src)
		{
			rayBlocked = 1;
			if (RayClear(u, Eye(src, threatPos), chest, src))
			{
				rayBlocked = 0;
				return "ray";
			}
			return string.Empty;
		}
		SCR_AICoverLock cov;
		if (u.m_CombatMoveState)
			cov = u.m_CombatMoveState.GetAssignedCover();
		if (!cov || !cov.IsValid())
			return string.Empty;
		ang = Math.Acos(Math.Clamp(cov.CosAngleToThreat(threatPos), -1.0, 1.0)) * Math.RAD2DEG;
		float arc = DCO_Stress.T(u, "dodge.arc_narrow_deg", 45.0);
		if (WideCover(me, chest, threatPos))
			arc = DCO_Stress.T(u, "dodge.arc_wide_deg", 65.0);
		if (ang > arc)
			return "arc";
		return string.Empty;
	}

	static void Shooters(SCR_AIUtilityComponent u, vector threatPos, array<vector> sp, array<float> sw, array<IEntity> se)
	{
		vector my = u.m_OwnerEntity.GetOrigin();
		array<vector> cp = {};
		array<float> cw = {};
		array<IEntity> ce = {};
		IEntity src = ActiveShooter(u);
		vector p0 = threatPos;
		if (src)
			p0 = src.GetOrigin();
		cp.Insert(p0);
		cw.Insert(Weight(u, src, my, p0) * 1.5);
		ce.Insert(src);
		if (u.m_PerceptionComponent)
		{
			array<BaseTarget> targets = {};
			u.m_PerceptionComponent.GetTargetsList(targets, ETargetCategory.ENEMY);
			int scanned = 0;
			foreach (BaseTarget t : targets)
			{
				if (scanned >= 8)
					break;
				if (!t || t.GetTimeSinceSeen() > 15.0)
					continue;
				IEntity te = t.GetTargetEntity();
				if (!te || te == src)
					continue;
				scanned++;
				vector tp = t.GetLastSeenPosition();
				cp.Insert(tp);
				cw.Insert(Weight(u, te, my, tp));
				ce.Insert(te);
			}
		}
		int maxN = (int)DCO_Stress.T(u, "dodge.max_shooters", 3.0);
		while (sp.Count() < maxN && !cp.IsEmpty())
		{
			int bi = 0;
			for (int i = 1; i < cw.Count(); i++)
			{
				if (cw[i] > cw[bi])
					bi = i;
			}
			sp.Insert(cp[bi]);
			sw.Insert(cw[bi]);
			se.Insert(ce[bi]);
			cp.Remove(bi);
			cw.Remove(bi);
			ce.Remove(bi);
		}
	}

	static float Weight(SCR_AIUtilityComponent u, IEntity e, vector my, vector p)
	{
		float w = 1.0 + 50.0 / Math.Max(vector.Distance(my, p), 10.0);
		if (e && DCO_Util.IsMGShooter(e))
			w *= 2.0;
		if (e && e == u.m_DCOHitBy)
			w *= 1.5;
		return w;
	}

	static int OpenEval(SCR_AIUtilityComponent u, array<vector> sp, array<float> sw, array<IEntity> se, vector target, out float w)
	{
		int n = 0;
		w = 0;
		for (int i = 0; i < sp.Count(); i++)
		{
			if (RayClear(u, Eye(se[i], sp[i]), target, se[i]))
			{
				n++;
				w += sw[i];
			}
		}
		return n;
	}

	static float SegDist(vector p, vector a, vector b)
	{
		vector ab = b - a;
		ab[1] = 0;
		vector ap = p - a;
		ap[1] = 0;
		float l2 = vector.Dot(ab, ab);
		if (l2 < 0.01)
			return ap.Length();
		float t = Math.Clamp(vector.Dot(ap, ab) / l2, 0.0, 1.0);
		vector q = ap - ab * t;
		return q.Length();
	}

	static void Mates(SCR_AIUtilityComponent u, array<vector> mp)
	{
		AIAgent ag = u.GetOwner();
		if (!ag || !ag.GetParentGroup())
			return;
		array<AIAgent> agents = {};
		ag.GetParentGroup().GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			if (!a || a == ag)
				continue;
			IEntity e = a.GetControlledEntity();
			if (e)
				mp.Insert(e.GetOrigin());
		}
	}

	static void SetMove(SCR_AICombatMoveRequest_Move rq, vector my, vector cand, bool crawl)
	{
		float d = vector.DistanceXZ(my, cand);
		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		rq.m_eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
		rq.m_vMovePos = cand;
		rq.m_bTryFindCover = false;
		rq.m_bFailIfNoCover = false;
		rq.m_bUseCoverSearchDirectivity = false;
		rq.m_bCheckCoverVisibility = false;
		rq.m_fCoverSearchSectorHalfAngleRad = -1.0;
		rq.m_fCoverSearchDistMin = 0;
		rq.m_fCoverSearchDistMax = d + 2.0;
		rq.m_vAvoidStraightPathDir = vector.Zero;
		if (crawl)
		{
			rq.m_eStanceMoving = ECharacterStance.PRONE;
			rq.m_eStanceEnd = ECharacterStance.PRONE;
			rq.m_eMovementType = EMovementType.WALK;
			rq.m_fMoveDuration_s = d / 0.8 + 1.0;
		}
		else
		{
			rq.m_eStanceEnd = ECharacterStance.CROUCH;
			rq.m_fMoveDuration_s = d / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT + 1.0;
		}
	}

	static int Apply(SCR_AIUtilityComponent u, SCR_AICombatMoveRequest_Move rq, vector threatPos, float maxDist, bool losOnly, out string pick)
	{
		pick = string.Empty;
		if (!On(u) || !u.m_OwnerEntity || !rq)
			return 0;
		IEntity me = u.m_OwnerEntity;
		vector my = me.GetOrigin();
		array<vector> sp = {};
		array<float> sw = {};
		array<IEntity> se = {};
		Shooters(u, threatPos, sp, sw, se);
		if (sp.IsEmpty())
			return 0;
		bool strong = HitRecent(u) || DoubleNearMiss(u);
		float openBeforeW;
		int openBefore = OpenEval(u, sp, sw, se, my + Vector(0, ChestH(me), 0), openBeforeW);
		if (openBefore == 0 && !strong)
		{
			pick = "stay";
			LogDir(me, pick, sp.Count(), openBefore, 0, 0, 0);
			return 2;
		}
		vector wdir = vector.Zero;
		array<vector> dirs = {};
		bool cross = false;
		for (int i = 0; i < sp.Count(); i++)
		{
			vector d = sp[i] - my;
			d[1] = 0;
			if (d.Length() < 0.1)
				continue;
			d.Normalize();
			foreach (vector o : dirs)
			{
				if (vector.Dot(o, d) < 0)
					cross = true;
			}
			dirs.Insert(d);
			wdir = wdir + d * sw[i];
		}
		if (dirs.IsEmpty())
			return 0;
		wdir[1] = 0;
		if (wdir.Length() < 0.01)
			wdir = dirs[0];
		wdir.Normalize();
		float crawlMax = Math.Min(maxDist, DCO_Stress.T(u, "dodge.crawl_max_m", 5.0));
		if (cross && losOnly)
		{
			pick = "building";
			LogDir(me, pick, sp.Count(), openBefore, -1, 0, 0);
			return 0;
		}
		if (cross)
		{
			pick = "crossfire";
			rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
			rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
			rq.m_bTryFindCover = true;
			rq.m_bFailIfNoCover = false;
			rq.m_bUseCoverSearchDirectivity = false;
			rq.m_fCoverSearchSectorHalfAngleRad = -1.0;
			rq.m_fCoverSearchDistMin = 0;
			rq.m_fCoverSearchDistMax = crawlMax;
			rq.m_eStanceMoving = ECharacterStance.PRONE;
			rq.m_eStanceEnd = ECharacterStance.PRONE;
			rq.m_eMovementType = EMovementType.WALK;
			rq.m_fMoveDuration_s = crawlMax / 0.8 + 1.0;
			LogDir(me, pick, sp.Count(), openBefore, -1, 0, crawlMax);
			return 1;
		}
		float rmax = Math.Min(maxDist, DCO_Stress.T(u, "dodge.rush_max_m", 15.0));
		if (u.m_ThreatSystem && u.m_ThreatSystem.GetThreatMeasure() > DCO_Stress.T(u, "dodge.heavy_threat", 0.7))
			rmax = Math.Min(rmax, DCO_Stress.T(u, "dodge.rush_heavy_m", 8.0));
		if (!strong && DCO_Util.IsMGShooter(me))
			rmax = Math.Min(rmax, DCO_Stress.T(u, "dodge.mg_micro_m", 3.0));
		float rmin = Math.Min(DCO_Stress.T(u, "dodge.rush_min_m", 5.0), rmax);
		float spacing = DCO_Stress.T(u, "dodge.spacing_m", 5.0);
		array<vector> mp = {};
		Mates(u, mp);
		vector side = Vector(wdir[2], 0, -wdir[0]);
		array<float> angs = {90.0, -90.0, 120.0, -120.0, 150.0, -150.0, 180.0};
		array<float> dists = {rmin, 0.5 * (rmin + rmax), rmax};
		int evaluated = 0;
		int maxEval = (int)DCO_Stress.T(u, "dodge.max_eval", 10.0);
		vector best;
		float bestAng;
		int bestOpen = 999;
		float bestW = 1000000.0;
		bool found;
		foreach (float dist : dists)
		{
			if (found)
				break;
			if (dist < 1.0)
				continue;
			evaluated = 0;
			foreach (float a : angs)
			{
				if (evaluated >= maxEval)
					break;
				float r = a * Math.DEG2RAD;
				vector dir = wdir * Math.Cos(r) + side * Math.Sin(r);
				vector cand = my + dir * dist;
				cand[1] = GetGame().GetWorld().GetSurfaceY(cand[0], cand[2]);
				bool bad = false;
				foreach (vector s : sp)
				{
					if (vector.DistanceXZ(cand, s) < vector.DistanceXZ(my, s) - 0.5)
						bad = true;
				}
				foreach (vector m : mp)
				{
					if (vector.DistanceXZ(cand, m) < spacing || SegDist(cand, m, sp[0]) < 2.0)
						bad = true;
				}
				if (bad)
					continue;
				evaluated++;
				float openW;
				int open = OpenEval(u, sp, sw, se, cand + Vector(0, 0.85, 0), openW);
				if (openW < bestW)
				{
					bestW = openW;
					bestOpen = open;
					best = cand;
					bestAng = Math.AbsFloat(a);
				}
				if (open == 0)
				{
					found = true;
					break;
				}
			}
		}
		if (losOnly && !found)
		{
			pick = "building";
			LogDir(me, pick, sp.Count(), openBefore, bestOpen, 0, 0);
			return 0;
		}
		if (bestOpen == 999)
		{
			pick = "no_spot";
			LogDir(me, pick, sp.Count(), openBefore, -1, 0, 0);
			return 2;
		}
		if (!found && !strong && bestW >= openBeforeW)
		{
			pick = "no_better";
			LogDir(me, pick, sp.Count(), openBefore, bestOpen, bestAng, 0);
			return 2;
		}
		if (found)
		{
			pick = "los_break";
			SetMove(rq, my, best, false);
		}
		else
		{
			pick = "crawl";
			vector cd = best - my;
			cd[1] = 0;
			float cl = cd.Length();
			if (cl > crawlMax && cl > 0.1)
			{
				best = my + cd * (crawlMax / cl);
				best[1] = GetGame().GetWorld().GetSurfaceY(best[0], best[2]);
			}
			SetMove(rq, my, best, true);
		}
		LogDir(me, pick, sp.Count(), openBefore, bestOpen, bestAng, vector.DistanceXZ(my, best));
		return 1;
	}

	static void LogDir(IEntity me, string pick, int shooters, int before, int after, float lat, float dist)
	{
		if (!DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
			return;
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("dodge_dir unit=%1 pick=%2 shooters=%3 los_open_before=%4 los_open_after=%5 lateral_deg=%6 dist=%7",
			DCO_Log.IdUnit(me), pick, shooters, before, after, Math.Round(lat), dist.ToString(-1, 1)));
	}
}
