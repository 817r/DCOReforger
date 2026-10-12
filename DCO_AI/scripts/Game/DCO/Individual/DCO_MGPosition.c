class DCO_MGEval
{
	string m_sMode;
	vector m_vCenter;
	vector m_vAnchor;
	vector m_vFrom;
	float m_fMaxMove;
	float m_fStart_ms;
	ref array<vector> m_aCand = {};
	int m_iNext;
	vector m_vBest;
	float m_fBest = -1000;
	float m_fHere = -1000;
	ECharacterStance m_eBestStance = ECharacterStance.PRONE;
	string m_sBestWhy = "-";
	string m_sHereWhy = "-";
	int m_iRejected;
}

class DCO_MGPosition
{
	static DCO_MGEval Begin(IEntity me, vector center, vector anchor, float maxMove, string mode, float now)
	{
		DCO_MGEval e = new DCO_MGEval();
		e.m_sMode = mode;
		e.m_vCenter = center;
		e.m_vAnchor = anchor;
		e.m_vFrom = me.GetOrigin();
		e.m_fMaxMove = maxMove;
		e.m_fStart_ms = now;
		e.m_aCand.Insert(e.m_vFrom);
		vector toC = center - e.m_vFrom;
		toC[1] = 0;
		float baseA = Math.Atan2(toC[2], toC[0]);
		array<float> rings = {0.15, 0.3, 0.5, 0.75, 1.0};
		float jitter = Math.RandomFloat(0, 0.5);
		foreach (int ri, float rf : rings)
		{
			float r = rf * maxMove;
			int n = 8;
			if (ri >= 2)
				n = 6;
			for (int i = 0; i < n; i++)
			{
				float a = baseA + (i + jitter + 0.5 * ri) * (2.0 * Math.PI / n);
				vector p = e.m_vFrom + Vector(Math.Cos(a) * r, 0, Math.Sin(a) * r);
				e.m_aCand.Insert(DCO_Util.Surface(p));
			}
		}
		DCO_Perf.Count("mgpos:begin_" + mode);
		return e;
	}

	protected static int s_iClears;

	static bool Step(DCO_MGEval e, IEntity me, int budget)
	{
		int t0 = s_iClears;
		while (budget > 0 && s_iClears - t0 < 12 && e.m_iNext < e.m_aCand.Count())
		{
			int idx = e.m_iNext;
			vector p = e.m_aCand[idx];
			e.m_iNext = idx + 1;
			budget--;
			ECharacterStance st;
			string why;
			vector at;
			float sc = Score(e, me, p, idx == 0, st, why, at);
			if (idx == 0)
			{
				e.m_fHere = sc;
				e.m_sHereWhy = why;
				continue;
			}
			if (sc <= -999.0)
			{
				e.m_iRejected = e.m_iRejected + 1;
				continue;
			}
			if (sc > e.m_fBest)
			{
				e.m_fBest = sc;
				e.m_vBest = at;
				e.m_eBestStance = st;
				e.m_sBestWhy = why;
			}
		}
		if (budget > 0 && e.m_iNext < e.m_aCand.Count())
			DCO_Perf.Count("mgpos:trace_cap");
		return e.m_iNext >= e.m_aCand.Count();
	}

	protected static bool Reach(vector p, out vector snapped)
	{
		NavmeshWorldComponent nav = GetGame().GetAIWorld().GetNavmeshWorldComponent("Soldiers");
		if (!nav || !nav.IsTileLoaded(p))
			return false;
		return nav.GetReachablePoint(p, 2.5, snapped);
	}

	protected static float Clear(vector from, vector to, IEntity me)
	{
		s_iClears++;
		return DCO_Util.TraceFrac(from, to, me, TraceFlags.WORLD | TraceFlags.ENTS, EPhysicsLayerDefs.Projectile, "t:DCO_MGPosition");
	}

	static float Score(DCO_MGEval e, IEntity me, vector p, bool isHere, out ECharacterStance stance, out string why, out vector at)
	{
		stance = ECharacterStance.PRONE;
		why = "-";
		at = p;
		if (!isHere)
		{
			vector snapped;
			if (!Reach(p, snapped) || DCO_SectorMath.IsInWater(snapped))
				return -1000;
			p = snapped;
			at = p;
			if (vector.DistanceXZ(p, e.m_vFrom) > e.m_fMaxMove + 3.0)
				return -1000;
			if (!DCO_Specialist.LaneFree(me, p, e.m_vCenter))
				return -1000;
		}
		vector c = e.m_vCenter;
		vector toC = c - p;
		toC[1] = 0;
		float dist = toC.Length();
		if (dist < 25.0)
			return -1000;
		toC = toC * (1.0 / dist);
		vector side = Vector(toC[2], 0, -toC[0]);
		float range = Math.Clamp(dist, 60.0, 400.0);
		vector aim = c + Vector(0, 1.0, 0);
		float elev = Math.Clamp((p[1] - c[1]) / 10.0, -0.5, 1.0);
		float ang = 0.5;
		if (e.m_vAnchor != vector.Zero && vector.DistanceXZ(e.m_vAnchor, c) > 30.0)
		{
			vector a1 = p - c;
			a1[1] = 0;
			vector a2 = e.m_vAnchor - c;
			a2[1] = 0;
			float deg = Math.Acos(Math.Clamp(vector.Dot(a1.Normalized(), a2.Normalized()), -1.0, 1.0)) * Math.RAD2DEG;
			if (deg >= 30.0 && deg <= 60.0)
				ang = 1.0;
			else if (deg < 30.0)
				ang = deg / 30.0;
			else if (deg <= 90.0)
				ang = 1.0 - (deg - 60.0) / 30.0 * 0.8;
			else
				ang = 0.1;
		}
		float mv = vector.DistanceXZ(p, e.m_vFrom);
		float cost = 0.25 * mv / Math.Max(e.m_fMaxMove, 1.0);
		float toward = 0;
		if (vector.DistanceXZ(e.m_vFrom, c) - dist > 15.0)
			toward = 0.15;
		float preMax = 0.7 + 0.15 * elev + 0.15 * ang - cost - toward;
		if (!isHere && preMax <= e.m_fBest)
		{
			DCO_Perf.Count("mgpos:prune_pre");
			return preMax;
		}
		float h = 0.6;
		float fl = Clear(p + Vector(0, h, 0), aim, me);
		float len = vector.Distance(p + Vector(0, h, 0), aim);
		bool line = fl * len >= len - 6.0;
		if (!line)
		{
			float fl2 = Clear(p + Vector(0, 1.2, 0), aim, me);
			if (fl2 * len >= len - 6.0)
			{
				line = true;
				h = 1.2;
				stance = ECharacterStance.CROUCH;
				fl = fl2;
			}
		}
		vector eye = p + Vector(0, h, 0);
		float cc = Math.Min(1.0, fl * len / range);
		float sky = 0;
		if (DCO_ConcealmentUtility.IsSkylined(p, c, h + 0.2))
			sky = -0.2;
		float lineS = -0.3;
		if (line)
			lineS = 0.15;
		float coverMax = 0;
		if (cc > 0.5)
			coverMax = 1.0;
		float fofMax = 0.35 * (cc + 2.0) / 3.0 + 0.2 * coverMax;
		float bound = fofMax + 0.15 * elev + 0.15 * ang + sky - cost - toward + lineS;
		if (!isHere && bound <= e.m_fBest)
		{
			DCO_Perf.Count("mgpos:prune_fof");
			return bound;
		}
		vector lDir = (toC * 0.966 + side * 0.259) * range;
		vector rDir = (toC * 0.966 - side * 0.259) * range;
		float cl = Clear(eye, eye + lDir, me);
		float cr = Clear(eye, eye + rDir, me);
		float fof = (cc + cl + cr) / 3.0;
		float cover = 0;
		vector low = p + Vector(0, 0.25, 0);
		float lowF = Clear(low, low + toC * 2.5, me);
		if (lowF < 0.95 && cc > 0.5)
			cover = 1.0;
		float score = 0.35 * fof + 0.2 * cover + 0.15 * elev + 0.15 * ang + sky - cost - toward + lineS;
		why = string.Format("fof=%1 cover=%2 elev=%3 ang=%4 line=%5 sky=%6 move=%7", fof.ToString(-1, 2), cover, elev.ToString(-1, 2), ang.ToString(-1, 2), DCO_Util.B01(line), DCO_Util.B01(sky < 0), Math.Round(mv));
		return score;
	}
}
