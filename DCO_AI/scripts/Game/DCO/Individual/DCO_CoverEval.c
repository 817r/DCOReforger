class DCO_CoverCand
{
	vector m_vPos;
	vector m_vTallest;
	int m_iTx;
	int m_iTy;
	int m_iId;
	int m_iProt;
	bool m_bHard;
	int m_iArc;
	int m_iExposed;
	bool m_bNoLos;
	string m_sKind;
	string m_sWhy;
	float m_fScore;
}

class DCO_CoverEval
{
	protected static ref array<ref DCO_CoverCand> s_aPool;
	protected static ref array<vector> s_aMates;
	protected static ref array<vector> s_aThreats;

	static bool On()
	{
		return DCO_Ctx.TuneG("cov.on", 1.0) > 0;
	}

	static bool Anchor(AIAgent owner, out vector anchor, out float radius, out string src)
	{
		src = "-";
		AIGroup grp = owner.GetParentGroup();
		if (!grp)
			return false;
		SCR_DefendWaypoint wp = SCR_DefendWaypoint.Cast(grp.GetCurrentWaypoint());
		if (wp)
		{
			anchor = wp.GetOrigin();
			radius = wp.GetCompletionRadius();
			src = "wp";
			return true;
		}
		SCR_AIGroup sgrp = SCR_AIGroup.Cast(grp);
		if (!sgrp || !sgrp.GetGroupUtilityComponent())
			return false;
		SCR_AIGroupUtilityComponent gu = sgrp.GetGroupUtilityComponent();
		if (gu.DCO_GetPosture() != DCO_GroupTactics.DEFENSIVE || gu.m_vDCODefAnchor == vector.Zero)
			return false;
		anchor = gu.m_vDCODefAnchor;
		radius = 25.0;
		src = "def";
		return true;
	}

	protected static float LateralGap(vector pos, vector fwd)
	{
		vector side = Vector(fwd[2], 0, -fwd[0]);
		float best = 1000.0;
		foreach (vector m : s_aMates)
		{
			vector d = pos - m;
			float lat = Math.AbsFloat(d[0] * side[0] + d[2] * side[2]);
			if (lat < best)
				best = lat;
		}
		return best;
	}

	protected static DCO_CoverCand Fetch(ChimeraCoverManagerComponent mgr, AIPathfindingComponent pf, CoverQueryProperties props, int n)
	{
		if (n >= s_aPool.Count())
			s_aPool.Insert(new DCO_CoverCand());
		DCO_CoverCand c = s_aPool[n];
		if (!mgr.GetBestCover("Soldiers", pf, props, c.m_vPos, c.m_vTallest, c.m_iTx, c.m_iTy, c.m_iId))
			return null;
		c.m_iProt = 0;
		c.m_bHard = false;
		c.m_iArc = 0;
		c.m_iExposed = 0;
		c.m_bNoLos = false;
		c.m_sKind = "-";
		c.m_sWhy = "";
		c.m_fScore = 0;
		mgr.SetOccupiedCover(c.m_iTx, c.m_iTy, c.m_iId, true);
		return c;
	}

	protected static void Release(ChimeraCoverManagerComponent mgr, int count)
	{
		for (int k = 0; k < count; k++)
			mgr.SetOccupiedCover(s_aPool[k].m_iTx, s_aPool[k].m_iTy, s_aPool[k].m_iId, false);
	}

	protected static void AddThreat(vector pos, vector myPos)
	{
		float d = vector.DistanceSqXZ(pos, myPos);
		int idx = s_aThreats.Count();
		for (int i = 0; i < s_aThreats.Count(); i++)
		{
			if (d < vector.DistanceSqXZ(s_aThreats[i], myPos))
			{
				idx = i;
				break;
			}
		}
		s_aThreats.InsertAt(pos, idx);
		if (s_aThreats.Count() > 3)
			s_aThreats.Remove(3);
	}

	protected static void CollectThreats(AIAgent owner, SCR_AIUtilityComponent u, CoverQueryProperties props, vector myPos, out vector threat, out string tsrc, out bool heavy)
	{
		s_aThreats.Clear();
		tsrc = "none";
		heavy = false;
		SCR_AIGroup grp = SCR_AIGroup.Cast(owner.GetParentGroup());
		SCR_AIGroupUtilityComponent gu;
		if (grp)
			gu = grp.GetGroupUtilityComponent();
		PerceptionManager pm = GetGame().GetPerceptionManager();
		float nearestSq = float.MAX;
		IEntity nearestEnt;
		if (gu && gu.m_Perception && pm)
		{
			float pmNow = pm.GetTime();
			foreach (SCR_AITargetInfo t : gu.m_Perception.m_aTargets)
			{
				if (!t || pmNow - t.m_fTimestamp > 10.0)
					continue;
				if (t.m_eCategory != EAITargetInfoCategory.DETECTED && t.m_eCategory != EAITargetInfoCategory.IDENTIFIED)
					continue;
				AddThreat(t.m_vWorldPos, myPos);
				float dSq = vector.DistanceSqXZ(t.m_vWorldPos, myPos);
				if (dSq < nearestSq)
				{
					nearestSq = dSq;
					nearestEnt = t.m_Entity;
				}
			}
		}
		if (s_aThreats.Count() > 0)
		{
			threat = s_aThreats[0];
			tsrc = "group";
			if (nearestEnt && Vehicle.Cast(nearestEnt))
				heavy = true;
			return;
		}
		DCO_EngageInfo ei;
		if (u)
			ei = DCO_Engage.Info(u);
		if (ei && ei.m_iKnown > 0 && ei.m_vNearest != vector.Zero)
		{
			threat = ei.m_vNearest;
			tsrc = "enemy";
			s_aThreats.Insert(threat);
			return;
		}
		if (props.m_vThreatPos != vector.Zero)
		{
			threat = props.m_vThreatPos;
			tsrc = "query";
			s_aThreats.Insert(threat);
			return;
		}
		float estR;
		int estN;
		vector estPos;
		if (grp && DCO_FireEstimate.Get(grp, GetGame().GetWorld().GetWorldTime(), 15000.0, estPos, estR, estN))
		{
			threat = estPos;
			tsrc = "fire_est";
			s_aThreats.Insert(threat);
			DCO_Perf.Count("cov:threat_fire_est");
		}
	}

	protected static bool TerrainCover(IEntity me, vector myPos, vector threat, string tsrc, SCR_AIUtilityComponent u, out vector coverPos, out vector coverTallest, out int tx, out int ty, out int cid)
	{
		if (tsrc == "none")
			return false;
		vector toThreat = DCO_Specialist.Flat(threat - myPos);
		if (toThreat == vector.Zero)
			return false;
		NavmeshWorldComponent nav = GetGame().GetAIWorld().GetNavmeshWorldComponent("Soldiers");
		vector best;
		float bestD = float.MAX;
		int tested;
		float radius = 6.0;
		for (int ring = 0; ring < 3; ring++)
		{
			for (int k = 0; k < 8; k++)
			{
				vector dir = DCO_CoverId.Rot(toThreat, k * 45.0);
				vector p = DCO_Util.Surface(myPos + dir * radius);
				vector reach;
				if (nav && nav.IsTileLoaded(p) && nav.GetReachablePoint(p, 2.0, reach))
					p = reach;
				else
					continue;
				tested++;
				vector d2 = DCO_Specialist.Flat(threat - p);
				IEntity hit;
				if (!DCO_CoverId.Blocked(p, d2, 0.35, me, hit))
					continue;
				if (DCO_Util.HitKind(hit) != "terrain")
					continue;
				float dd = vector.DistanceSqXZ(myPos, p);
				if (dd < bestD)
				{
					bestD = dd;
					best = p;
				}
			}
			if (best != vector.Zero)
				break;
			radius += 6.0;
		}
		if (best == vector.Zero)
		{
			DCO_Perf.Count("cov:terrain_none");
			return false;
		}
		coverPos = best;
		coverTallest = best + Vector(0, 0.25, 0);
		tx = -1;
		ty = -1;
		cid = -1;
		DCO_Perf.Count("cov:terrain_pick");
		if (u && DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("cover_pick unit=%1 how=terrain move_d=%2 tested=%3 threat_src=%4 threat_d=%5 pos=%6", DCO_Log.IdUnit(me), Math.Round(vector.Distance(myPos, best)), tested, tsrc, Math.Round(vector.DistanceXZ(best, threat)), DCO_Util.XZ(best)));
		return true;
	}

	static bool Pick(AIAgent owner, SCR_AIUtilityComponent u, ChimeraCoverManagerComponent mgr, AIPathfindingComponent pf, CoverQueryProperties props, out vector coverPos, out vector coverTallest, out int tx, out int ty, out int cid)
	{
		int perfT = DCO_Perf.Begin();
		if (!s_aPool)
		{
			s_aPool = {};
			s_aMates = {};
			s_aThreats = {};
		}
		IEntity me = owner.GetControlledEntity();
		vector myPos = me.GetOrigin();
		bool threatened = u && u.m_ThreatSystem && u.m_ThreatSystem.GetState() == EAIThreatState.THREATENED;

		vector threat;
		string tsrc;
		bool heavy;
		CollectThreats(owner, u, props, myPos, threat, tsrc, heavy);

		float origMax = props.m_fSectorDistMax;
		string how = "eval";
		if (!Fetch(mgr, pf, props, 0))
		{
			DCO_Perf.Count("cov:none_first");
			bool got;
			float widen = 15.0;
			for (int w = 0; w < 2 && !got; w++)
			{
				if (widen > origMax)
				{
					props.m_fSectorDistMax = widen;
					got = Fetch(mgr, pf, props, 0);
				}
				widen = 25.0;
			}
			if (got)
			{
				DCO_Perf.Count("cov:widen");
				how = "widen";
			}
			else
			{
				props.m_fSectorDistMax = origMax;
				bool terr = TerrainCover(me, myPos, threat, tsrc, u, coverPos, coverTallest, tx, ty, cid);
				if (!terr)
					DCO_Perf.Count("cov:none");
				DCO_Perf.End("cov_eval", perfT);
				return terr;
			}
		}
		int count = 1;

		vector anchor;
		float radius;
		string anchorSrc;
		bool hasAnchor = Anchor(owner, anchor, radius, anchorSrc);
		bool defensive = anchorSrc == "def";
		vector anchorFwd;
		if (hasAnchor && tsrc != "none")
		{
			anchorFwd = threat - anchor;
			anchorFwd[1] = 0;
			if (anchorFwd.LengthSq() > 1.0)
				anchorFwd.Normalize();
			else
				anchorFwd = vector.Zero;
		}

		DCO_SquadSpacing.CollectMatePositions(owner, s_aMates);
		int maxN = DCO_Ctx.TuneForU(u, "cov.cands", 4.0);
		float spaceM = DCO_Ctx.TuneForU(u, "cov.space_m", 5.0);
		float pad = DCO_Ctx.TuneForU(u, "cov.anchor_pad", 15.0);
		float wProt = DCO_Ctx.TuneForU(u, "cov.w_prot", 5.0);
		float wHard = DCO_Ctx.TuneForU(u, "cov.w_hard", 2.0);
		float wAnchor = DCO_Ctx.TuneForU(u, "cov.w_anchor", 3.0);
		float wDist = DCO_Ctx.TuneForU(u, "cov.w_dist", 2.0);
		float wVan = DCO_Ctx.TuneForU(u, "cov.w_vanilla", 1.0);
		float maxF = Math.Max(maxN, 1);
		int minN = DCO_Ctx.TuneForU(u, "cov.cands_min", 2.0);
		vector bestDir;
		float bestH;
		bool learnOn = u && DCO_IndLearn.On() && !DCO_IndLearn.Explore();
		string learnInfo;
		int learnHit;

		int best = -1;
		float bestScore = -1000;
		bool early;
		for (int pass = 0; pass < 2; pass++)
		{
			if (pass == 1)
			{
				if (best >= 0)
					break;
				Release(mgr, count);
				props.m_fSectorDistMax = Math.Max(origMax, props.m_fSectorDistMax) * 2.0;
				spaceM = 0;
				count = 0;
				if (!Fetch(mgr, pf, props, 0))
					break;
				count = 1;
				DCO_Perf.Count("cov:expand");
				how = "expand";
			}
			for (int i = 0; i < maxN; i++)
			{
				if (i >= count)
				{
					if (best >= 0 && i >= minN)
						break;
					if (!Fetch(mgr, pf, props, i))
						break;
					count++;
				}
				DCO_CoverCand c = s_aPool[i];
				if (spaceM > 0 && DCO_SquadSpacing.NearestDistance(c.m_vPos, s_aMates) < spaceM)
				{
					c.m_sWhy = "space";
					continue;
				}
				float ad;
				if (hasAnchor)
				{
					ad = vector.DistanceXZ(c.m_vPos, anchor);
					if (ad > radius + pad)
					{
						c.m_sWhy = "anchor";
						continue;
					}
					if (anchorFwd != vector.Zero && vector.Dot(c.m_vPos - anchor, anchorFwd) > radius)
					{
						c.m_sWhy = "forward";
						continue;
					}
					if (defensive && anchorFwd != vector.Zero && vector.Dot(c.m_vPos - anchor, anchorFwd) < -0.5 * radius)
					{
						c.m_sWhy = "behind";
						continue;
					}
				}
				float protScore = 0.5;
				bool traced;
				vector dir;
				if (tsrc != "none")
				{
					dir = threat - c.m_vPos;
					dir[1] = 0;
					if (dir.LengthSq() > 0.25)
					{
						traced = true;
						dir.Normalize();
						IEntity hit;
						float h;
						c.m_iProt = DCO_CoverId.Prot(c.m_vPos, dir, me, hit, h);
						if (c.m_iProt == 0)
						{
							c.m_sWhy = "no_block";
							continue;
						}
						string ent;
						bool hard;
						c.m_sKind = DCO_CoverId.Kind(hit, hard, ent);
						c.m_bHard = hard;
						protScore = 0.4;
						if (c.m_iProt == 3)
							protScore = 1.0;
						else if (c.m_iProt == 2)
							protScore = 0.85;
						if (c.m_sKind == "bush")
						{
							if (threatened)
							{
								c.m_sWhy = "bush_fire";
								continue;
							}
							protScore = 0.3;
						}
						for (int t2 = 1; t2 < s_aThreats.Count(); t2++)
						{
							vector d2 = s_aThreats[t2] - c.m_vPos;
							d2[1] = 0;
							if (d2.LengthSq() <= 0.25)
								continue;
							d2.Normalize();
							IEntity hit2;
							float h2;
							if (DCO_CoverId.Prot(c.m_vPos, d2, me, hit2, h2) == 0)
								c.m_iExposed++;
						}
						c.m_iArc = DCO_CoverId.ArcSide(c.m_vPos, dir, h, me, -1.0, 2) + DCO_CoverId.ArcSide(c.m_vPos, dir, h, me, 1.0, 2);
						float fireH = 1.2;
						if (c.m_iProt == 3)
							fireH = 1.7;
						vector fFrom = c.m_vPos + Vector(0, fireH, 0);
						vector fTo = threat + Vector(0, 1.5, 0);
						if (DCO_Util.TraceFrac(fFrom, fTo, me, TraceFlags.WORLD | TraceFlags.ENTS, -1, "cid") < 0.99)
							c.m_bNoLos = true;
					}
				}
				float rank = i;
				float anchorScore = 1.0;
				if (hasAnchor && radius > 0)
					anchorScore = Math.Clamp(1.0 - ad / (radius + pad), 0, 1);
				float score = wProt * protScore + wAnchor * anchorScore - wDist * vector.Distance(myPos, c.m_vPos) / 30.0 + wVan * (1.0 - rank / maxF);
				if (c.m_bHard)
					score = score + wHard;
				score = score + 1.5 * c.m_iArc / 4.0 - 3.0 * c.m_iExposed;
				if (c.m_bNoLos)
					score = score - 2.0;
				if (defensive && anchorFwd != vector.Zero && LateralGap(c.m_vPos, anchorFwd) >= spaceM)
				{
					score = score + 1.0;
					DCO_Perf.Count("cov:def_spread");
				}
				if (heavy && traced && (!c.m_bHard || c.m_iProt == 1))
				{
					score = score - 6.0;
					DCO_Perf.Count("cov:heavy_soft_pen");
				}
				if (learnOn)
				{
					float lm = DCO_IndLearn.Mult(u, c.m_vPos, learnInfo);
					if (Math.AbsFloat(lm - 1.0) > 0.02)
					{
						learnHit++;
						score = score + 12.0 * (lm - 1.0);
					}
				}
				c.m_fScore = score;
				if (score > bestScore)
				{
					bestScore = score;
					best = i;
					if (traced)
					{
						bestDir = dir;
						bestH = 0.85;
						if (c.m_iProt == 1)
							bestH = 0.35;
					}
				}
				bool clean = c.m_iProt >= 2 && c.m_bHard && c.m_iExposed == 0 && !c.m_bNoLos;
				if (!traced || clean)
				{
					early = true;
					break;
				}
			}
		}

		Release(mgr, count);
		props.m_fSectorDistMax = origMax;

		string rej;
		for (int r = 0; r < count; r++)
		{
			if (s_aPool[r].m_sWhy != "")
			{
				DCO_Perf.Count("cov:reject_" + s_aPool[r].m_sWhy);
				rej = rej + s_aPool[r].m_sWhy + ",";
			}
		}

		if (best < 0)
		{
			for (int f = 0; f < count; f++)
			{
				if (s_aPool[f].m_sWhy != "anchor" && s_aPool[f].m_sWhy != "forward")
				{
					best = f;
					break;
				}
			}
			if (best < 0)
			{
				DCO_Perf.Count("cov:fail_anchor");
				if (u && DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
					DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("cover_pick unit=%1 how=fail_anchor n=%2 rejects=%3 threat_src=%4", DCO_Log.IdUnit(me), count, rej, tsrc));
				DCO_Perf.End("cov_eval", perfT);
				return false;
			}
			how = "fallback_vanilla";
		}
		else if (early && how == "eval")
			how = "early";
		DCO_CoverCand pick = s_aPool[best];
		if (learnHit > 0)
		{
			DCO_Perf.Count("ilearn:cov_term");
			DCO_IndLearn.LogUse(u, "cover_score", "cands=" + learnHit.ToString() + " last_" + learnInfo);
		}
		DCO_Perf.Count("cov:" + how);
		DCO_Perf.Count("cov:pick_idx" + best);
		DCO_Perf.CountN("cov:queries", count);

		coverPos = pick.m_vPos;
		coverTallest = pick.m_vTallest;
		tx = pick.m_iTx;
		ty = pick.m_iTy;
		cid = pick.m_iId;
		string stance = "cover_h";
		int proneLos = -1;
		if (pick.m_iProt == 1 && tsrc != "none" && vector.DistanceXZ(pick.m_vPos, threat) < 60.0)
		{
			IEntity meEnt = null;
			if (owner)
				meEnt = owner.GetControlledEntity();
			bool clear = DCO_Util.TraceClear(pick.m_vPos + Vector(0, 0.4, 0), threat + Vector(0, 1.0, 0), meEnt, 0.97, TraceFlags.WORLD | TraceFlags.ENTS, -1, null, "t:DCO_CoverProneLos");
			proneLos = DCO_Util.B01(clear);
			if (clear)
			{
				coverTallest = pick.m_vPos + Vector(0, 0.25, 0);
				stance = "prone_low";
				DCO_Perf.Count("cov:prone_low");
			}
			else
			{
				stance = "crouch_prone_blind";
				DCO_Perf.Count("cov:prone_low_blind");
			}
		}

		if (u && DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
		{
			float now = GetGame().GetWorld().GetWorldTime();
			if (now >= u.m_fDCOCovLogAt_ms)
			{
				u.m_fDCOCovLogAt_ms = now + 5000.0;
				string ad2 = "-";
				if (hasAnchor)
					ad2 = Math.Round(vector.DistanceXZ(pick.m_vPos, anchor)).ToString() + "/" + anchorSrc;
				if (rej == "")
					rej = "-";
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("cover_pick unit=%1 how=%2 idx=%3 n=%4 score=%5 kind=%6 hard=%7 protects=%8 arc=%9", DCO_Log.IdUnit(me), how, best, count, pick.m_fScore.ToString(-1, 2), pick.m_sKind, DCO_Util.B01(pick.m_bHard), DCO_CoverId.ProtName(pick.m_iProt), pick.m_iArc)
					+ string.Format(" anchor_d=%1 move_d=%2 threat_src=%3 rejects=%4 pos=%5", ad2, Math.Round(vector.Distance(myPos, pick.m_vPos)), tsrc, rej, DCO_Util.XZ(pick.m_vPos))
					+ string.Format(" threats=%1 exposed=%2 los=%3 heavy=%4 stance=%5 window=%6", s_aThreats.Count(), pick.m_iExposed, DCO_Util.B01(!pick.m_bNoLos), DCO_Util.B01(heavy), stance, Math.Round(origMax)) + string.Format(" prone_los=%1", proneLos));
			}
		}
		DCO_Perf.End("cov_eval", perfT);
		return true;
	}
}
