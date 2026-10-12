class DCO_FightCand
{
	vector m_vPos;
	float m_fScore;
	float m_fLos;
	float m_fOblique;
	int m_iCover;
	ECharacterStance m_eStance;
}

class DCO_FightPos
{
	static bool On()
	{
		return DCO_Ctx.TuneG("def.fieldpos", 1.0) > 0;
	}

	protected static float Frac(vector a, vector b)
	{
		return DCO_Util.TraceFrac(a, b, null, TraceFlags.WORLD | TraceFlags.ENTS, -1, "t:fightpos");
	}

	protected static float Los(vector eye, vector dir, float range)
	{
		return Frac(eye, eye + dir * range) * range;
	}

	protected static vector Rot(vector dir, float deg)
	{
		float r = deg * Math.DEG2RAD;
		float c = Math.Cos(r);
		float s = Math.Sin(r);
		return Vector(dir[0] * c - dir[2] * s, 0, dir[0] * s + dir[2] * c);
	}

	protected static bool Blocked(vector p)
	{
		return Frac(p + Vector(0, 0.3, 0), p + Vector(0, 1.7, 0)) < 0.99;
	}

	protected static void Rate(DCO_FightCand c, vector anchor, vector dir, float radius, bool ring)
	{
		vector p = c.m_vPos;
		vector side = Vector(dir[2], 0, -dir[0]);
		float lowFrac = Frac(p + Vector(0, 0.45, 0), p + Vector(0, 0.45, 0) + dir * 4.0);
		float highFrac = Frac(p + Vector(0, 1.15, 0), p + Vector(0, 1.15, 0) + dir * 6.0);
		float coverScore = 0;
		c.m_iCover = 0;
		c.m_eStance = ECharacterStance.PRONE;
		if (lowFrac < 0.9 && highFrac > 0.95)
		{
			c.m_iCover = 3;
			coverScore = 3.0;
			c.m_eStance = ECharacterStance.CROUCH;
		}
		else if (lowFrac < 0.9)
		{
			c.m_iCover = 1;
			coverScore = 1.0;
			c.m_eStance = ECharacterStance.CROUCH;
		}
		else
		{
			float yFront = GetGame().GetWorld().GetSurfaceY(p[0] + dir[0] * 3.0, p[2] + dir[2] * 3.0);
			if (yFront - p[1] > 0.35)
			{
				c.m_iCover = 2;
				coverScore = 2.0;
			}
		}
		float eyeH = 0.5;
		if (c.m_eStance == ECharacterStance.CROUCH)
			eyeH = 1.1;
		vector eye = p + Vector(0, eyeH, 0);
		float range = DCO_Ctx.TuneG("def.fp_los_m", 200.0);
		float half = DCO_Ctx.TuneG("def.fp_arc_deg", 60.0) * 0.33;
		c.m_fLos = (Los(eye, dir, range) + Los(eye, Rot(dir, half), range) + Los(eye, Rot(dir, -half), range)) / 3.0;
		vector rel = p - anchor;
		rel[1] = 0;
		float lat = vector.Dot(rel, side);
		vector obl = dir;
		if (lat > 0)
			obl = Rot(dir, -35.0);
		else
			obl = Rot(dir, 35.0);
		c.m_fOblique = Los(eye, obl, range);
		float yBack = GetGame().GetWorld().GetSurfaceY(p[0] - dir[0] * 25.0, p[2] - dir[2] * 25.0);
		float yAhead = GetGame().GetWorld().GetSurfaceY(p[0] + dir[0] * 25.0, p[2] + dir[2] * 25.0);
		float sky = 0;
		if (p[1] - yBack > 2.0 && p[1] - yAhead > 2.0)
			sky = DCO_Ctx.TuneG("def.fp_skyline_pen", 2.0);
		float behind = 0;
		if (!ring && vector.Dot(rel, dir) < -radius * 0.35)
			behind = 1.0;
		float minLos = DCO_Ctx.TuneG("def.fp_min_los", 40.0);
		float blind = 0;
		if (BlindFix() && c.m_fLos < DCO_Ctx.TuneG("def.fp_blind_m", 25.0))
		{
			c.m_iCover = 0;
			c.m_eStance = ECharacterStance.PRONE;
			coverScore = 0;
		}
		if (c.m_fLos < minLos)
		{
			coverScore = coverScore * c.m_fLos / Math.Max(minLos, 1.0);
			blind = DCO_Ctx.TuneG("def.fp_blind_pen", 2.0);
		}
		c.m_fScore = coverScore + Math.Min(c.m_fLos, 150.0) / 50.0 - sky - behind - blind - 0.5 * rel.Length() / Math.Max(radius, 1.0);
	}

	static bool BlindFix()
	{
		return DCO_Ctx.TuneG("def.fp_blind_fix", 0) > 0;
	}

	protected static void AddRing(vector anchor, vector dir, bool ring, float radius, float rf, bool frontOnly, notnull array<ref DCO_FightCand> cands)
	{
		int steps = 12;
		if (rf == 0)
			steps = 1;
		for (int i = 0; i < steps; i++)
		{
			float ang = 360.0 * i / steps;
			vector off = Vector(Math.Sin(ang * Math.DEG2RAD), 0, Math.Cos(ang * Math.DEG2RAD));
			if (frontOnly && !ring && vector.Dot(off, dir) < 0)
				continue;
			vector p = DCO_Util.Surface(anchor + off * radius * rf);
			if (Blocked(p))
				continue;
			DCO_FightCand c = new DCO_FightCand();
			c.m_vPos = p;
			vector cd = dir;
			if (ring)
			{
				cd = p - anchor;
				cd[1] = 0;
				if (cd.LengthSq() < 1.0)
					cd = "0 0 1";
				cd.Normalize();
			}
			Rate(c, anchor, cd, radius, ring);
			cands.Insert(c);
		}
	}

	static int Pick(vector anchor, vector threatDir, float radius, int members, float arcDeg, notnull array<ref DCO_GarrisonSlot> outSlots, string src, string grpId, out int mgSlot)
	{
		int t = DCO_Perf.Begin();
		mgSlot = -1;
		bool ring = threatDir.LengthSq() < 0.01;
		vector dir = threatDir;
		dir[1] = 0;
		if (!ring)
			dir.Normalize();
		radius = Math.Clamp(radius, 10.0, 80.0);
		array<ref DCO_FightCand> cands = {};
		array<float> rings = {0.0, 0.35, 0.65, 1.0};
		foreach (float rf : rings)
			AddRing(anchor, dir, ring, radius, rf, false, cands);
		if (BlindFix())
		{
			float bestLos = 0;
			foreach (DCO_FightCand lc : cands)
				bestLos = Math.Max(bestLos, lc.m_fLos);
			float minLos = DCO_Ctx.TuneG("def.fp_min_los", 40.0);
			if (bestLos < minLos)
			{
				int before = cands.Count();
				AddRing(anchor, dir, ring, radius, 1.3, true, cands);
				AddRing(anchor, dir, ring, radius, 1.6, true, cands);
				float newLos = bestLos;
				for (int e = before; e < cands.Count(); e++)
					newLos = Math.Max(newLos, cands[e].m_fLos);
				DCO_Perf.Count("fightpos:extend");
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, grpId, string.Format("fightpos_extend src=%1 grp=%2 best_los=%3 added=%4 new_best_los=%5 ring=%6", src, grpId, Math.Round(bestLos), cands.Count() - before, Math.Round(newLos), ring));
			}
		}
		int want = Math.Ceil(members * 0.5);
		if (want < 1)
			want = 1;
		float spacing = DCO_Ctx.TuneG("def.fp_spacing", 10.0);
		array<DCO_FightCand> chosen = {};
		for (int n = 0; n < want; n++)
		{
			DCO_FightCand best;
			foreach (DCO_FightCand c2 : cands)
			{
				bool ok = true;
				foreach (DCO_FightCand ch : chosen)
				{
					if (ch == c2 || vector.DistanceXZ(ch.m_vPos, c2.m_vPos) < spacing)
					{
						ok = false;
						break;
					}
				}
				if (ok && (!best || c2.m_fScore > best.m_fScore))
					best = c2;
			}
			if (!best)
				break;
			chosen.Insert(best);
		}
		int mgIdx = -1;
		float mgBest = -1;
		foreach (int ci, DCO_FightCand cc : chosen)
		{
			if (cc.m_fOblique > mgBest)
			{
				mgBest = cc.m_fOblique;
				mgIdx = ci;
			}
		}
		int nCover;
		int nTerrain;
		int nNone;
		float losSum;
		foreach (int k, DCO_FightCand f : chosen)
		{
			vector fd = dir;
			if (ring)
			{
				fd = f.m_vPos - anchor;
				fd[1] = 0;
				if (fd.LengthSq() < 1.0)
					fd = "0 0 1";
				fd.Normalize();
			}
			vector side = Vector(fd[2], 0, -fd[0]);
			for (int b = 0; b < 2; b++)
			{
				vector sp = f.m_vPos;
				if (b == 1)
				{
					sp = DCO_Util.Surface(f.m_vPos + side * 2.5);
					if (Blocked(sp))
						sp = DCO_Util.Surface(f.m_vPos - side * 2.5);
				}
				DCO_GarrisonSlot s = new DCO_GarrisonSlot();
				s.m_vWorldPos = sp;
				s.m_vLocalPos = sp;
				s.m_vWorldDir = fd;
				if (k == mgIdx && b == 0)
					mgSlot = outSlots.Count();
				if (k == mgIdx && b == 0)
				{
					vector rel = f.m_vPos - anchor;
					if (vector.Dot(rel, side) > 0)
						s.m_vWorldDir = Rot(fd, -35.0);
					else
						s.m_vWorldDir = Rot(fd, 35.0);
				}
				s.m_fLocalYaw = s.m_vWorldDir.ToYaw();
				s.m_fArc = arcDeg;
				s.m_eStance = f.m_eStance;
				s.m_eType = DCO_EGarrisonSlotType.WINDOW;
				s.m_fScore = f.m_fScore - b * 0.1;
				s.m_fCover = f.m_iCover;
				s.m_fFire = f.m_fLos;
				outSlots.Insert(s);
			}
			if (f.m_iCover == 3 || f.m_iCover == 1)
				nCover++;
			else if (f.m_iCover == 2)
				nTerrain++;
			else
				nNone++;
			losSum += f.m_fLos;
		}
		DCO_Perf.End("fightpos", t);
		DCO_Perf.Count("fightpos:pick_" + src);
		float losAvg;
		if (!chosen.IsEmpty())
			losAvg = losSum / chosen.Count();
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, grpId, string.Format("fightpos_pick src=%1 grp=%2 cand=%3 pairs=%4 cover=%5 terrain=%6 open=%7 los_avg=%8 mg_oblique=%9", src, grpId, cands.Count(), chosen.Count(), nCover, nTerrain, nNone, Math.Round(losAvg), Math.Round(mgBest)));
		return chosen.Count();
	}
}
