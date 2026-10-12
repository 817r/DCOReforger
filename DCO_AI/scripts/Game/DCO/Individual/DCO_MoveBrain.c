modded class SCR_AIUtilityComponent
{
	float m_fDCOMbNext_ms;
	int m_iDCOMbLast = -1;
	string m_sDCOMbWhy;
	float m_fDCOMbCovNext_ms;
	bool m_bDCOMbCov;
	vector m_vDCOMbSpot;
	float m_fDCOMbSpotAt_ms;
	int m_iDCOMbSpotProt;
	float m_fDCOMbGain;
	float m_fDCOMbCrawlUntil_ms;
}

class DCO_MoveBrain
{
	protected static bool s_bLogged;
	protected static ref map<AIGroup, ref array<vector>> s_mBlastPos;
	protected static ref map<AIGroup, ref array<float>> s_mBlastAt;

	static void NoteBlast(SCR_AIUtilityComponent u, vector pos)
	{
		if (!u || !u.GetAIAgent() || !u.GetAIAgent().GetParentGroup() || pos == vector.Zero)
			return;
		if (!s_mBlastPos)
		{
			s_mBlastPos = new map<AIGroup, ref array<vector>>();
			s_mBlastAt = new map<AIGroup, ref array<float>>();
		}
		if (s_mBlastPos.Count() > 300)
		{
			s_mBlastPos.Clear();
			s_mBlastAt.Clear();
		}
		AIGroup g = u.GetAIAgent().GetParentGroup();
		array<vector> ps = s_mBlastPos.Get(g);
		array<float> ts = s_mBlastAt.Get(g);
		if (!ps)
		{
			ps = {};
			ts = {};
			s_mBlastPos.Set(g, ps);
			s_mBlastAt.Set(g, ts);
		}
		ps.Insert(pos);
		ts.Insert(GetGame().GetWorld().GetWorldTime());
		if (ps.Count() > 4)
		{
			ps.Remove(0);
			ts.Remove(0);
		}
	}

	static bool BlastNear(SCR_AIUtilityComponent u, vector pos, float radius)
	{
		if (!s_mBlastPos || !u || !u.GetAIAgent() || !u.GetAIAgent().GetParentGroup())
			return false;
		AIGroup g = u.GetAIAgent().GetParentGroup();
		array<vector> ps = s_mBlastPos.Get(g);
		array<float> ts = s_mBlastAt.Get(g);
		if (!ps)
			return false;
		float now = GetGame().GetWorld().GetWorldTime();
		float win = DCO_Stress.T(u, "mb.blast_s", 6.0) * 1000.0;
		foreach (int i, vector b : ps)
		{
			if (now - ts[i] <= win && vector.DistanceXZ(b, pos) <= radius)
				return true;
		}
		return false;
	}

	static bool On(SCR_AIUtilityComponent u)
	{
		return DCO_Stress.T(u, "mb.on", 1.0) > 0;
	}

	static bool CoveredAdvance(SCR_AIUtilityComponent u)
	{
		if (!u || !On(u) || DCO_Stress.Level(u) != 1)
			return false;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now >= u.m_fDCOMbCovNext_ms)
		{
			u.m_fDCOMbCovNext_ms = now + 500.0;
			SCR_AIGroup g;
			if (u.GetAIAgent())
				g = SCR_AIGroup.Cast(u.GetAIAgent().GetParentGroup());
			u.m_bDCOMbCov = g && DCO_TeamBound.Attacker(g) && MateFired(u, 3000.0);
		}
		return u.m_bDCOMbCov;
	}

	static bool Advancing(SCR_AIUtilityComponent u)
	{
		if (!u || u.m_iDCOMbLast != 1 || !On(u))
			return false;
		string w = u.m_sDCOMbWhy;
		return w == "covered" || w == "lull" || w == "fallback" || w == "covered_med";
	}

	static bool FieldOn(SCR_AIUtilityComponent u)
	{
		return DCO_Stress.T(u, "mb.field", 1.0) > 0;
	}

	static bool FieldSpot(SCR_AIUtilityComponent u, vector center, vector threat, out vector spot, out int prot)
	{
		prot = 0;
		IEntity me = u.m_OwnerEntity;
		AIAgent agent = u.GetAIAgent();
		if (!me || !agent || threat == vector.Zero)
			return false;
		array<vector> mates = {};
		DCO_SquadSpacing.CollectMatePositions(agent, mates);
		float ring = DCO_Stress.T(u, "mb.field_ring_m", 4.0);
		float spacing = DCO_Stress.T(u, "mb.field_spacing_m", 4.0);
		float bestD = float.MAX;
		float ring2 = DCO_Stress.T(u, "mb.field_ring2_m", 8.0);
		int last = 8;
		if (ring2 > 0)
			last = 16;
		for (int i = -1; i < last; i++)
		{
			vector c = center;
			if (i >= 8)
			{
				if (prot >= 2)
					break;
				float a2 = (i - 8) * 45.0 * Math.DEG2RAD + 0.39;
				c = center + Vector(Math.Cos(a2) * ring2, 0, Math.Sin(a2) * ring2);
			}
			else if (i >= 0)
			{
				float a = i * 45.0 * Math.DEG2RAD;
				c = center + Vector(Math.Cos(a) * ring, 0, Math.Sin(a) * ring);
			}
			c = DCO_Util.Surface(c);
			if (DCO_SquadSpacing.NearestDistance(c, mates) < spacing || BlastNear(u, c, 15.0))
				continue;
			vector dir = threat - c;
			dir[1] = 0;
			if (dir.Length() < 5.0)
				continue;
			dir.Normalize();
			IEntity hit;
			float h;
			int p = DCO_CoverId.Prot(c, dir, me, hit, h);
			if (p <= 0)
				continue;
			float d = vector.DistanceXZ(c, center);
			if (p > prot || (p == prot && d < bestD))
			{
				prot = p;
				bestD = d;
				spot = c;
			}
		}
		return prot > 0;
	}

	static bool V2(SCR_AIUtilityComponent u)
	{
		return DCO_Stress.T(u, "mb.v2", 1.0) > 0;
	}

	static float RushMax(SCR_AIUtilityComponent u)
	{
		float s = DCO_Stress.T(u, "mb.rush_s", 4.0);
		if (DCO_Stress.Level(u) >= 1)
			s = DCO_Stress.T(u, "mb.rush_stress_s", 2.5);
		return s * SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_RUN;
	}

	static bool Crawl(SCR_AIUtilityComponent u)
	{
		return u && GetGame().GetWorld().GetWorldTime() < u.m_fDCOMbCrawlUntil_ms;
	}

	static bool TakeSpot(SCR_AIUtilityComponent u, out vector spot, out int prot)
	{
		if (!u || u.m_fDCOMbSpotAt_ms <= 0 || GetGame().GetWorld().GetWorldTime() - u.m_fDCOMbSpotAt_ms > 3000.0)
			return false;
		if (u.m_fDCOMbGain < DCO_Stress.T(u, "mb.gain_min", 0.15))
			return false;
		spot = u.m_vDCOMbSpot;
		prot = u.m_iDCOMbSpotProt;
		u.m_fDCOMbSpotAt_ms = 0;
		return true;
	}

	protected static void SideThreats(SCR_AIUtilityComponent u, vector threat, notnull array<vector> outPos)
	{
		if (!u.m_PerceptionComponent)
			return;
		array<BaseTarget> targets = {};
		u.m_PerceptionComponent.GetTargetsList(targets, ETargetCategory.ENEMY);
		vector p = u.m_OwnerEntity.GetOrigin();
		vector main = threat - p;
		main[1] = 0;
		if (main.Length() < 0.5)
			return;
		main.Normalize();
		foreach (BaseTarget t : targets)
		{
			if (!t || t.GetTimeSinceSeen() > 20.0)
				continue;
			vector tp = t.GetLastSeenPosition();
			vector d = tp - p;
			d[1] = 0;
			float len = d.Length();
			if (len < 5.0 || len > 300.0)
				continue;
			d = d * (1.0 / len);
			if (vector.Dot(d, main) > 0.77)
				continue;
			outPos.Insert(tp);
			if (outPos.Count() >= 2)
				return;
		}
	}

	protected static float Exposure(SCR_AIUtilityComponent u, vector c, notnull array<vector> sides)
	{
		if (sides.IsEmpty())
			return 1.0;
		int open;
		IEntity hit;
		float h;
		foreach (vector s : sides)
		{
			vector d = s - c;
			d[1] = 0;
			if (d.Length() < 1.0)
				continue;
			d.Normalize();
			if (DCO_CoverId.Prot(c, d, u.m_OwnerEntity, hit, h) == 0)
				open++;
		}
		return 1.0 - open * 1.0 / sides.Count();
	}

	protected static float Los(SCR_AIUtilityComponent u, vector c, vector threat, int prot)
	{
		float fh = 0.95;
		if (prot >= 2)
			fh = 1.55;
		vector from = c + Vector(0, fh, 0);
		vector to = threat + Vector(0, 1.0, 0);
		float len = vector.Distance(from, to);
		float f = DCO_Util.TraceFrac(from, to, u.m_OwnerEntity, TraceFlags.WORLD | TraceFlags.ENTS, -1, "mb_los");
		if (f >= 0.98 || f * len >= len - 2.5)
			return 1.0;
		return 0.0;
	}

	protected static float Score(SCR_AIUtilityComponent u, vector c, vector threat, float myD, notnull array<vector> mates, bool attacker, out int prot)
	{
		prot = 0;
		IEntity me = u.m_OwnerEntity;
		vector dir = threat - c;
		dir[1] = 0;
		float dT = dir.Length();
		if (dT < 0.5)
			return 0;
		dir = dir * (1.0 / dT);
		IEntity hit;
		float h;
		prot = DCO_CoverId.Prot(c, dir, me, hit, h);
		float pT = prot / 3.0;
		if (prot > 0)
		{
			bool hard;
			string ent;
			DCO_CoverId.Kind(hit, hard, ent);
			if (!hard)
				pT *= 0.6;
		}
		float gT = 0.5;
		if (attacker)
			gT = Math.Clamp((myD - dT) / 10.0, -1.0, 1.0) * 0.5 + 0.5;
		float sT = 1.0;
		if (!mates.IsEmpty())
			sT = Math.Clamp(DCO_SquadSpacing.NearestDistance(c, mates) / (2.0 * DCO_Stress.T(u, "mb.field_spacing_m", 4.0)), 0.0, 1.0);
		return 0.35 * pT + 0.20 * gT + 0.10 * sT;
	}

	static bool FieldSpotV2(SCR_AIUtilityComponent u, vector center, vector threat, bool attacker, float rushMax, out vector spot, out int prot, out float gain)
	{
		prot = 0;
		gain = -1;
		IEntity me = u.m_OwnerEntity;
		AIAgent agent = u.GetAIAgent();
		if (!me || !agent || threat == vector.Zero)
			return false;
		int t0 = DCO_Perf.Begin();
		array<vector> mates = {};
		DCO_SquadSpacing.CollectMatePositions(agent, mates);
		array<vector> sides = {};
		SideThreats(u, threat, sides);
		vector my = me.GetOrigin();
		float myD = vector.DistanceXZ(my, threat);
		int myProt;
		float cur = Score(u, my, threat, myD, mates, attacker, myProt);
		cur += 0.15 * Los(u, my, threat, myProt) + 0.20 * Exposure(u, my, sides);
		float ring = DCO_Stress.T(u, "mb.field_ring_m", 4.0);
		float spacing = DCO_Stress.T(u, "mb.field_spacing_m", 4.0);
		float ring2 = DCO_Stress.T(u, "mb.field_ring2_m", 8.0);
		array<vector> cs = {};
		array<float> sc = {};
		array<int> pr = {};
		array<float> ls = {};
		int ringBest;
		int last = 8;
		if (ring2 > 0)
			last = 16;
		for (int i = -1; i < last; i++)
		{
			vector c = center;
			if (i >= 8)
			{
				if (ringBest >= 2)
					break;
				float a2 = (i - 8) * 45.0 * Math.DEG2RAD + 0.39;
				c = center + Vector(Math.Cos(a2) * ring2, 0, Math.Sin(a2) * ring2);
			}
			else if (i >= 0)
			{
				float a = i * 45.0 * Math.DEG2RAD;
				c = center + Vector(Math.Cos(a) * ring, 0, Math.Sin(a) * ring);
			}
			c = DCO_Util.Surface(c);
			float cMe = vector.DistanceXZ(c, my);
			if (cMe > rushMax || cMe < 1.5 || vector.DistanceXZ(c, threat) < 5.0)
				continue;
			if (DCO_SquadSpacing.NearestDistance(c, mates) < spacing || BlastNear(u, c, 15.0))
				continue;
			int p;
			float s = Score(u, c, threat, myD, mates, attacker, p);
			if (p > ringBest)
				ringBest = p;
			cs.Insert(c);
			sc.Insert(s);
			pr.Insert(p);
			ls.Insert(0);
		}
		int bestIdx = -1;
		float best = -1;
		for (int k = 0; k < 3; k++)
		{
			int top = -1;
			for (int j = 0; j < sc.Count(); j++)
			{
				if (sc[j] >= 0 && (top < 0 || sc[j] > sc[top]))
					top = j;
			}
			if (top < 0)
				break;
			ls[top] = Los(u, cs[top], threat, pr[top]);
			float fin = sc[top] + 0.15 * ls[top] + 0.20 * Exposure(u, cs[top], sides);
			sc[top] = -1;
			if (fin > best)
			{
				best = fin;
				bestIdx = top;
			}
		}
		DCO_Perf.End("mb_v2", t0);
		DCO_Perf.Count("mb:v2_eval");
		if (bestIdx < 0)
		{
			DCO_Perf.Count("mb:v2_none");
			return false;
		}
		spot = cs[bestIdx];
		prot = pr[bestIdx];
		gain = best - cur;
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("mb_pick unit=%1 cur=%2 best=%3 gain=%4 prot=%5 los=%6 my_prot=%7 cands=%8 sides=%9",
				DCO_Log.IdUnit(me), cur.ToString(-1, 2), best.ToString(-1, 2), gain.ToString(-1, 2), DCO_CoverId.ProtName(prot), ls[bestIdx], DCO_CoverId.ProtName(myProt), cs.Count(), sides.Count()));
		return true;
	}

	protected static bool GainOk(SCR_AIUtilityComponent u, BaseTarget target, bool attacker, float now)
	{
		if (!target)
			return true;
		if (now - u.m_fDCOMbSpotAt_ms < 2000.0 && u.m_fDCOMbSpotAt_ms > 0)
			return u.m_fDCOMbGain >= DCO_Stress.T(u, "mb.gain_min", 0.15);
		vector threat = target.GetLastSeenPosition();
		vector my = u.m_OwnerEntity.GetOrigin();
		vector to = threat - my;
		to[1] = 0;
		float d = to.Length();
		if (d < 1.0)
			return true;
		to = to * (1.0 / d);
		float rush = RushMax(u);
		float adv = Math.Min(rush * 0.8, d - DCO_Engage.MinDist(u));
		vector center = my;
		if (adv > 2.0)
			center = my + to * adv;
		vector spot;
		int prot;
		float gain;
		if (!FieldSpotV2(u, center, threat, attacker, rush, spot, prot, gain))
		{
			u.m_fDCOMbSpotAt_ms = now;
			u.m_fDCOMbGain = -1;
			return false;
		}
		u.m_vDCOMbSpot = spot;
		u.m_iDCOMbSpotProt = prot;
		u.m_fDCOMbGain = gain;
		u.m_fDCOMbSpotAt_ms = now;
		return gain >= DCO_Stress.T(u, "mb.gain_min", 0.15);
	}

	static bool MateFired(SCR_AIUtilityComponent u, float window_ms)
	{
		AIAgent me = u.GetAIAgent();
		if (!me || !me.GetParentGroup())
			return false;
		array<AIAgent> agents = {};
		me.GetParentGroup().GetAgents(agents);
		float now = GetGame().GetWorld().GetWorldTime();
		foreach (AIAgent a : agents)
		{
			if (!a || a == me)
				continue;
			IEntity e = a.GetControlledEntity();
			if (e && now - DCO_ShotCounter.LastShotAt(e) < window_ms)
				return true;
		}
		return false;
	}

	static int Decide(SCR_AIUtilityComponent u, SCR_AICombatMoveState st, BaseTarget target, bool attacker, bool tbOn, bool myTurn)
	{
		if (!u || !st)
			return -1;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < u.m_fDCOMbNext_ms)
			return u.m_iDCOMbLast;
		u.m_fDCOMbNext_ms = now + 500.0;

		if (!s_bLogged)
		{
			s_bLogged = true;
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("mb_active commit_s=%1 med_wait_s=%2 atk_fallback_s=%3 comp_hits=%4", DCO_Stress.T(u, "mb.commit_s", 3.0), DCO_Stress.T(u, "mb.med_wait_s", 5.0), DCO_Stress.T(u, "mb.atk_fallback_s", 25.0), DCO_Stress.T(u, "mb.comp_hits", 2.0)));
		}
		string why;
		int res = Eval(u, st, target, attacker, tbOn, myTurn, now, why);
		if (res == 1)
			DCO_Perf.Count("mb:move_" + why);
		else if (res == 0)
			DCO_Perf.Count("mb:stay_" + why);
		if ((res != u.m_iDCOMbLast || why != u.m_sDCOMbWhy) && res >= 0 && DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
		{
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("mb_decide unit=%1 act=%2 why=%3 stopped_s=%4 cover=%5 cid=%6 nearmiss3=%7 attacker=%8 turn=%9",
				DCO_Log.IdUnit(u.m_OwnerEntity), res, why, st.m_fTimerStopped_s.ToString(-1, 1), DCO_Util.B01(st.m_bInCover), u.m_sDCOCidMis, NearMiss(u, now), DCO_Util.B01(attacker), DCO_Util.B01(myTurn)));
		}
		u.m_iDCOMbLast = res;
		u.m_sDCOMbWhy = why;
		return res;
	}

	protected static void NoteCrawl(SCR_AIUtilityComponent u, bool v2, float now)
	{
		if (!v2 || DCO_Stress.Level(u) < 2)
			return;
		u.m_fDCOMbCrawlUntil_ms = now + 3000.0;
		DCO_Perf.Count("mb:v2_crawl");
	}

	protected static int NearMiss(SCR_AIUtilityComponent u, float now)
	{
		SCR_AICombatComponent cc = u.m_CombatComponent;
		if (!cc)
			return 0;
		return cc.DCO_NearMissCount(now, 3000.0);
	}

	protected static int Eval(SCR_AIUtilityComponent u, SCR_AICombatMoveState st, BaseTarget target, bool attacker, bool tbOn, bool myTurn, float now, out string why)
	{
		float stopped = st.m_fTimerStopped_s;
		int nm = NearMiss(u, now);
		bool seen = target && target.GetTimeSinceSeen() < 10.0;
		bool v2 = V2(u);

		if (stopped >= 1.5 && st.m_bInCover && seen && now - u.m_fDCOCidAt_ms < 12000.0)
		{
			if (u.m_sDCOCidMis == "no_block" || u.m_sDCOCidMis == "soft_under_fire")
			{
				why = "cover_" + u.m_sDCOCidMis;
				NoteCrawl(u, v2, now);
				return 1;
			}
		}
		if (stopped >= 1.5 && nm >= DCO_Stress.T(u, "mb.comp_hits", 2.0))
		{
			why = "spot_hit";
			NoteCrawl(u, v2, now);
			return 1;
		}
		if (!attacker)
			return -1;
		if (DCO_Stress.T(u, "mb.mg_exempt", 1.0) > 0 && DCO_Util.IsMGShooter(u.m_OwnerEntity))
		{
			DCO_Perf.Count("mb:mg_exempt");
			return -1;
		}

		if (stopped < DCO_Stress.T(u, "mb.commit_s", 3.0))
		{
			why = "commit";
			return 0;
		}
		int lvl = DCO_Stress.Level(u);
		if (lvl >= 2)
		{
			why = "pinned";
			return 0;
		}
		if (tbOn && !myTurn)
		{
			why = "not_turn";
			return 0;
		}
		if (BlastNear(u, u.m_OwnerEntity.GetOrigin(), DCO_Stress.T(u, "mb.blast_m", 20.0)))
		{
			why = "blast_wait";
			return 0;
		}
		bool mateFired = MateFired(u, 3000.0);
		bool covered = mateFired;
		if (tbOn)
			covered = myTurn;
		bool lull = target && target.GetTimeSinceEndangered() > 6.0 && mateFired;
		if (lvl == 1)
		{
			if (covered && stopped >= DCO_Stress.T(u, "mb.med_wait_s", 5.0))
			{
				why = "covered_med";
				if (v2 && !GainOk(u, target, attacker, now))
				{
					why = "no_gain";
					return 0;
				}
				return 1;
			}
		}
		else if (covered || lull)
		{
			why = "covered";
			if (!covered)
				why = "lull";
			if (v2 && !GainOk(u, target, attacker, now))
			{
				why = "no_gain";
				return 0;
			}
			return 1;
		}
		if (stopped >= DCO_Stress.T(u, "mb.atk_fallback_s", 25.0))
		{
			why = "fallback";
			return 1;
		}
		why = "wait";
		return 0;
	}
}
