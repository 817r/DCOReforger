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
	string m_sKind;
	string m_sWhy;
	float m_fScore;
}

class DCO_CoverEval
{
	protected static ref array<ref DCO_CoverCand> s_aPool;
	protected static ref array<vector> s_aMates;

	static bool On()
	{
		return DCO_Ctx.TuneG("cov.on", 1.0) > 0;
	}

	static bool Anchor(AIAgent owner, out vector anchor, out float radius)
	{
		AIGroup grp = owner.GetParentGroup();
		if (!grp)
			return false;
		SCR_DefendWaypoint wp = SCR_DefendWaypoint.Cast(grp.GetCurrentWaypoint());
		if (!wp)
			return false;
		anchor = wp.GetOrigin();
		radius = wp.GetCompletionRadius();
		return true;
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
		c.m_sKind = "-";
		c.m_sWhy = "";
		c.m_fScore = 0;
		mgr.SetOccupiedCover(c.m_iTx, c.m_iTy, c.m_iId, true);
		return c;
	}

	static bool Pick(AIAgent owner, SCR_AIUtilityComponent u, ChimeraCoverManagerComponent mgr, AIPathfindingComponent pf, CoverQueryProperties props, out vector coverPos, out vector coverTallest, out int tx, out int ty, out int cid)
	{
		int perfT = DCO_Perf.Begin();
		if (!s_aPool)
		{
			s_aPool = {};
			s_aMates = {};
		}
		IEntity me = owner.GetControlledEntity();
		vector myPos = me.GetOrigin();
		if (!Fetch(mgr, pf, props, 0))
		{
			DCO_Perf.Count("cov:none");
			DCO_Perf.End("cov_eval", perfT);
			return false;
		}
		int count = 1;

		vector threat;
		string tsrc = "none";
		DCO_EngageInfo ei;
		if (u)
			ei = DCO_Engage.Info(u);
		if (ei && ei.m_iKnown > 0 && ei.m_vNearest != vector.Zero)
		{
			threat = ei.m_vNearest;
			tsrc = "enemy";
		}
		else if (props.m_vThreatPos != vector.Zero)
		{
			threat = props.m_vThreatPos;
			tsrc = "query";
		}

		vector anchor;
		float radius;
		bool hasAnchor = Anchor(owner, anchor, radius);
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
			if (DCO_SquadSpacing.NearestDistance(c.m_vPos, s_aMates) < spaceM)
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
			}
			float protScore = 0.5;
			bool traced;
			if (tsrc != "none")
			{
				vector dir = threat - c.m_vPos;
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
					protScore = 0.6;
					if (c.m_iProt == 3)
						protScore = 1.0;
					else if (c.m_iProt == 2)
						protScore = 0.85;
				}
			}
			float rank = i;
			float anchorScore = 1.0;
			if (hasAnchor && radius > 0)
				anchorScore = Math.Clamp(1.0 - ad / (radius + pad), 0, 1);
			float score = wProt * protScore + wAnchor * anchorScore - wDist * vector.Distance(myPos, c.m_vPos) / 30.0 + wVan * (1.0 - rank / maxF);
			if (c.m_bHard)
				score = score + wHard;
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
					bestDir = threat - c.m_vPos;
					bestDir[1] = 0;
					bestDir.Normalize();
					bestH = 0.85;
					if (c.m_iProt == 1)
						bestH = 0.35;
				}
			}
			if (!traced || (c.m_iProt >= 2 && c.m_bHard))
			{
				early = true;
				break;
			}
		}

		for (int k = 0; k < count; k++)
			mgr.SetOccupiedCover(s_aPool[k].m_iTx, s_aPool[k].m_iTy, s_aPool[k].m_iId, false);

		string rej;
		for (int r = 0; r < count; r++)
		{
			if (s_aPool[r].m_sWhy != "")
			{
				DCO_Perf.Count("cov:reject_" + s_aPool[r].m_sWhy);
				rej = rej + s_aPool[r].m_sWhy + ",";
			}
		}

		string how = "eval";
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
		else if (early)
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

		if (u && DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
		{
			float now = GetGame().GetWorld().GetWorldTime();
			if (now >= u.m_fDCOCovLogAt_ms)
			{
				u.m_fDCOCovLogAt_ms = now + 5000.0;
				if (pick.m_iProt > 0 && bestDir != vector.Zero && how != "fallback_vanilla")
					pick.m_iArc = DCO_CoverId.ArcSide(pick.m_vPos, bestDir, bestH, me, -1.0, 2) + DCO_CoverId.ArcSide(pick.m_vPos, bestDir, bestH, me, 1.0, 2);
				string ad2 = "-";
				if (hasAnchor)
					ad2 = Math.Round(vector.DistanceXZ(pick.m_vPos, anchor)).ToString();
				if (rej == "")
					rej = "-";
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("cover_pick unit=%1 how=%2 idx=%3 n=%4 score=%5 kind=%6 hard=%7 protects=%8 arc=%9", DCO_Log.IdUnit(me), how, best, count, pick.m_fScore.ToString(-1, 2), pick.m_sKind, DCO_Util.B01(pick.m_bHard), DCO_CoverId.ProtName(pick.m_iProt), pick.m_iArc)
					+ string.Format(" anchor_d=%1 move_d=%2 threat_src=%3 rejects=%4 pos=%5", ad2, Math.Round(vector.Distance(myPos, pick.m_vPos)), tsrc, rej, DCO_Util.XZ(pick.m_vPos)));
			}
		}
		DCO_Perf.End("cov_eval", perfT);
		return true;
	}
}
