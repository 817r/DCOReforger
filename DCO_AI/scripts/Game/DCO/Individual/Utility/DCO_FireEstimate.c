class DCO_FireEst
{
	vector m_vPos;
	float m_fRadius;
	float m_fAt_ms;
	float m_fFirstAt_ms;
	int m_iCount;
	int m_iSrc;
	int m_iHop;
	AIGroup m_From;
	vector m_vSharedPos;
	float m_fSharedAt_ms = -100000;
}

class DCO_FireEstimate
{
	protected static ref map<AIGroup, ref DCO_FireEst> s_mEst;
	protected static map<AIGroup, ref DCO_FireEst> GetEst()
	{
		if (!s_mEst)
			s_mEst = new map<AIGroup, ref DCO_FireEst>();
		return s_mEst;
	}

	static void Report(AIAgent reporter, vector reporterPos, vector shooterPos, float now)
	{
		if (!reporter || shooterPos == vector.Zero)
			return;
		AIGroup grp = reporter.GetParentGroup();
		if (!grp)
			return;
		float dist = vector.Distance(reporterPos, shooterPos);
		if (dist < 15.0 || dist > 800.0)
			return;
		vector dir = shooterPos - reporterPos;
		dir[1] = 0;
		dir.Normalize();
		float errDeg = Math.RandomFloat(-1.0, 1.0) * (5.0 + dist / 40.0);
		float a = errDeg * Math.DEG2RAD;
		vector bent = Vector(dir[0] * Math.Cos(a) - dir[2] * Math.Sin(a), 0, dir[0] * Math.Sin(a) + dir[2] * Math.Cos(a));
		vector guess = DCO_Util.Surface(reporterPos + bent * (dist * Math.RandomFloat(0.85, 1.15)));
		float rad = Math.Clamp(0.15 * dist, 5.0, 40.0);

		if (GetEst().Count() > 64)
			Prune(now);
		DCO_FireEst e = GetEst().Get(grp);
		if (e && now - e.m_fAt_ms < 25000.0 && vector.DistanceXZ(e.m_vPos, guess) < Math.Max(60.0, e.m_fRadius * 2.0))
		{
			float w = 1.0 / (e.m_iCount + 1);
			e.m_vPos = e.m_vPos * (1.0 - w) + guess * w;
			e.m_iCount = e.m_iCount + 1;
			e.m_fRadius = Math.Clamp(rad / Math.Sqrt(e.m_iCount), 4.0, 40.0);
			e.m_fAt_ms = now;
			e.m_iSrc = 0;
			e.m_iHop = 0;
			DCO_Perf.Count("fire_est:refine");
			MaybeShare(grp, e, now);
			return;
		}
		bool fresh = !e;
		if (!e)
		{
			e = new DCO_FireEst();
			GetEst().Set(grp, e);
		}
		e.m_vPos = guess;
		e.m_fRadius = rad;
		e.m_fAt_ms = now;
		e.m_fFirstAt_ms = now;
		e.m_iCount = 1;
		e.m_iSrc = 0;
		e.m_iHop = 0;
		e.m_From = null;
		DCO_Perf.Count("fire_est:new");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("fire_est_new grp=%1 reporter=%2 est=%3 radius=%4 dist=%5 err_deg=%6 replaced=%7", DCO_Log.IdGroup(grp), DCO_Log.IdUnit(reporter.GetControlledEntity()), DCO_Log.Pos(guess), Math.Round(rad), Math.Round(dist), Math.Round(errDeg), DCO_Util.B01(!fresh)));
		MaybeShare(grp, e, now);
	}

	protected static void MaybeShare(AIGroup grp, DCO_FireEst e, float now)
	{
		if (DCO_Ctx.TuneFor(grp, "is.est_share", 0.0) <= 0)
			return;
		if (now - e.m_fSharedAt_ms < 10000.0 && vector.DistanceXZ(e.m_vPos, e.m_vSharedPos) < 30.0)
			return;
		e.m_fSharedAt_ms = now;
		e.m_vSharedPos = e.m_vPos;
		DCO_ContactSharing.ShareEstimate(SCR_AIGroup.Cast(grp), e.m_vPos, e.m_fRadius, e.m_iCount, e.m_iHop);
	}

	static void Receive(AIGroup grp, vector pos, float radius, int count, int hop, AIGroup from, bool radio, float now)
	{
		if (!grp)
			return;
		if (GetEst().Count() > 64)
			Prune(now);
		DCO_FireEst e = GetEst().Get(grp);
		string result = "new";
		if (e && now - e.m_fAt_ms < 25000.0 && e.m_iSrc == 0)
		{
			DCO_Perf.Count("fire_est:recv_keep_own");
			result = "keep_own";
		}
		else if (e && now - e.m_fAt_ms < 25000.0 && vector.DistanceXZ(e.m_vPos, pos) < Math.Max(60.0, e.m_fRadius * 2.0))
		{
			e.m_vPos = e.m_vPos * 0.5 + pos * 0.5;
			e.m_fRadius = Math.Clamp(Math.Min(e.m_fRadius, radius), 4.0, 40.0);
			e.m_iCount = Math.Max(e.m_iCount, count);
			e.m_iHop = Math.Min(e.m_iHop, hop);
			e.m_fAt_ms = now;
			DCO_Perf.Count("fire_est:recv_merge");
			result = "merge";
		}
		else
		{
			if (!e)
			{
				e = new DCO_FireEst();
				GetEst().Set(grp, e);
			}
			e.m_vPos = pos;
			e.m_fRadius = Math.Clamp(radius, 4.0, 40.0);
			e.m_fAt_ms = now;
			e.m_fFirstAt_ms = now;
			e.m_iCount = count;
			e.m_iSrc = 1;
			e.m_iHop = hop;
			e.m_From = from;
			e.m_fSharedAt_ms = -100000;
			DCO_Perf.Count("fire_est:recv_new");
		}
		string ch = "voice";
		if (radio)
			ch = "radio";
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("fire_est_recv grp=%1 from=%2 hop=%3 channel=%4 est=%5 radius=%6 reports=%7 result=%8", DCO_Log.IdGroup(grp), DCO_Log.IdGroup(from), hop, ch, DCO_Log.Pos(pos), Math.Round(radius), count, result));
		if (result != "keep_own")
			MaybeShare(grp, e, now);
	}

	static bool GetQ(AIGroup grp, float now, float maxAge_ms, out vector pos, out float radius, out int count, out float conf, out string src)
	{
		if (!Get(grp, now, maxAge_ms, pos, radius, count))
			return false;
		DCO_FireEst e = GetEst().Get(grp);
		float age = (now - e.m_fAt_ms) / 1000.0;
		conf = DCO_InfoQ.EstConf(age, e.m_iSrc, e.m_iHop, e.m_iCount);
		src = "unseen";
		if (e.m_iSrc == 1)
			src = "shared_hop" + e.m_iHop;
		return true;
	}

	static bool Get(AIGroup grp, float now, float maxAge_ms, out vector pos, out float radius, out int count)
	{
		if (!grp)
			return false;
		DCO_FireEst e = GetEst().Get(grp);
		if (!e || now - e.m_fAt_ms > maxAge_ms)
			return false;
		pos = e.m_vPos;
		radius = DCO_InfoQ.EstRadius(e.m_fRadius, (now - e.m_fAt_ms) / 1000.0, e.m_iSrc, e.m_iHop);
		count = e.m_iCount;
		return true;
	}

	protected static void Prune(float now)
	{
		array<AIGroup> drop = {};
		foreach (AIGroup g, DCO_FireEst e : GetEst())
		{
			if (!g || !e || now - e.m_fAt_ms > 60000.0)
				drop.Insert(g);
		}
		foreach (AIGroup d : drop)
		{
			GetEst().Remove(d);
		}
	}
}
