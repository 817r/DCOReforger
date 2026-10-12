class DCO_PursueTrack
{
	vector m_vPos;
	float m_fAt_ms;
	int m_iCount;
	int m_iHits;
}

class DCO_PursueSquad
{
	float m_fNextTick_ms;
	float m_fNextMove_ms;
	ref map<IEntity, ref DCO_PursueTrack> m_mTrack = new map<IEntity, ref DCO_PursueTrack>();
}

class DCO_Pursue
{
	protected static ref map<AIGroup, ref DCO_PursueSquad> s_mSquad;

	static void Tick(SCR_AIUtilityComponent u, float now)
	{
		if (!u || !u.m_OwnerEntity)
			return;
		AIAgent ag = u.GetAIAgent();
		if (!ag)
			return;
		SCR_AIGroup grp = SCR_AIGroup.Cast(ag.GetParentGroup());
		if (!grp)
			return;
		if (!s_mSquad)
			s_mSquad = new map<AIGroup, ref DCO_PursueSquad>();
		DCO_PursueSquad sq = s_mSquad.Get(grp);
		if (!sq)
		{
			if (s_mSquad.Count() > 500)
				Prune();
			sq = new DCO_PursueSquad();
			s_mSquad.Set(grp, sq);
		}
		if (now < sq.m_fNextTick_ms)
			return;
		sq.m_fNextTick_ms = now + 2000.0;
		if (DCO_Stress.T(u, "agg.pursue", 1.0) <= 0)
		{
			sq.m_mTrack.Clear();
			return;
		}
		if (DCO_Util.IsPlayerGroup(grp))
			return;
		SquadTick(u, grp, sq, now);
	}

	protected static void Prune()
	{
		map<AIGroup, ref DCO_PursueSquad> keep = new map<AIGroup, ref DCO_PursueSquad>();
		foreach (AIGroup g, DCO_PursueSquad s : s_mSquad)
		{
			if (g)
				keep.Set(g, s);
		}
		s_mSquad = keep;
	}

	protected static void SquadTick(SCR_AIUtilityComponent u, SCR_AIGroup grp, DCO_PursueSquad sq, float now)
	{
		SCR_AIGroupUtilityComponent gu = grp.GetGroupUtilityComponent();
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!gu || !gu.m_Perception || !pm)
			return;
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		array<SCR_AIUtilityComponent> mates = {};
		vector centroid;
		foreach (AIAgent a : agents)
		{
			if (!a)
				continue;
			IEntity e = a.GetControlledEntity();
			if (!e || !DCO_Util.IsAlive(e))
				continue;
			SCR_AIUtilityComponent mu = SCR_AIUtilityComponent.Cast(a.FindComponent(SCR_AIUtilityComponent));
			if (!mu)
				continue;
			mates.Insert(mu);
			centroid = centroid + e.GetOrigin();
		}
		if (mates.Count() < 3)
		{
			sq.m_mTrack.Clear();
			return;
		}
		centroid = centroid * (1.0 / mates.Count());
		float pmNow = pm.GetTime();
		array<IEntity> seen = {};
		SCR_AITargetInfo best;
		float bestDist;
		float bestGrew;
		float bestQuiet;
		float bestDot;
		int bestHits;
		float growMin = DCO_Stress.T(u, "agg.pursue_grow_m", 8.0);
		foreach (SCR_AITargetInfo t : gu.m_Perception.m_aTargets)
		{
			if (!t || !t.m_Entity)
				continue;
			float age = pmNow - t.m_fTimestamp;
			DCO_PursueTrack tr = sq.m_mTrack.Get(t.m_Entity);
			if (age > 10.0)
			{
				if (tr)
					sq.m_mTrack.Remove(t.m_Entity);
				continue;
			}
			seen.Insert(t.m_Entity);
			ChimeraCharacter tc = ChimeraCharacter.Cast(t.m_Entity);
			if (age > 3.0 || !tc || tc.IsInVehicle() || !DCO_Util.IsAlive(tc))
				continue;
			float d = vector.DistanceXZ(centroid, t.m_vWorldPos);
			if (!tr)
			{
				tr = new DCO_PursueTrack();
				tr.m_vPos = t.m_vWorldPos;
				tr.m_fAt_ms = now;
				sq.m_mTrack.Set(t.m_Entity, tr);
				DCO_Opp.QuietS(tc);
				continue;
			}
			if (now - tr.m_fAt_ms < 3000.0)
				continue;
			float grew = d - vector.DistanceXZ(centroid, tr.m_vPos);
			vector moved = t.m_vWorldPos - tr.m_vPos;
			moved[1] = 0;
			vector away = t.m_vWorldPos - centroid;
			away[1] = 0;
			float dot = 0;
			if (moved.Length() > 0.1 && away.Length() > 0.1)
				dot = vector.Dot(moved.Normalized(), away.Normalized());
			tr.m_vPos = t.m_vWorldPos;
			tr.m_fAt_ms = now;
			float quiet = DCO_Opp.QuietS(tc);
			if (grew < growMin || dot < 0.7 || quiet < 3.0 || d < 40.0 || d > 200.0 || tr.m_iCount >= 3)
			{
				tr.m_iHits = 0;
				continue;
			}
			tr.m_iHits++;
			if (tr.m_iHits < 2)
			{
				DCO_Perf.Count("agg:pursue_wait_confirm");
				continue;
			}
			if (!best || grew > bestGrew)
			{
				best = t;
				bestDist = d;
				bestGrew = grew;
				bestQuiet = quiet;
				bestDot = dot;
				bestHits = tr.m_iHits;
			}
		}
		array<IEntity> gone = {};
		foreach (IEntity k, DCO_PursueTrack v : sq.m_mTrack)
		{
			if (!k || !seen.Contains(k))
				gone.Insert(k);
		}
		foreach (IEntity k : gone)
			sq.m_mTrack.Remove(k);
		if (!best)
			return;
		if (now < sq.m_fNextMove_ms)
		{
			DCO_Perf.Count("agg:pursue_skip_cooldown");
			return;
		}
		Move(u, grp, sq, mates, best, bestDist, bestGrew, bestQuiet, bestDot, bestHits, now);
	}

	protected static void Move(SCR_AIUtilityComponent u, SCR_AIGroup grp, DCO_PursueSquad sq, array<SCR_AIUtilityComponent> mates, SCR_AITargetInfo t, float dist, float grew, float quiet, float dot, int hits, float now)
	{
		vector ep = t.m_vWorldPos;
		IEntity leader = grp.GetLeaderEntity();
		string lastWhy = "no_mover";
		array<SCR_AIUtilityComponent> movers = {};
		array<float> moverD = {};
		foreach (SCR_AIUtilityComponent mu : mates)
		{
			string why = Gate(mu);
			if (why != "")
			{
				lastWhy = why;
				continue;
			}
			float md = vector.DistanceXZ(mu.m_OwnerEntity.GetOrigin(), ep);
			int at = 0;
			while (at < moverD.Count() && moverD[at] <= md)
				at++;
			movers.InsertAt(mu, at);
			moverD.InsertAt(md, at);
		}
		int n = 0;
		SCR_AIUtilityComponent first;
		for (int i = 0; i < movers.Count() && n < 2; i++)
		{
			SCR_AIUtilityComponent mu = movers[i];
			vector me = mu.m_OwnerEntity.GetOrigin();
			vector dir = ep - me;
			dir[1] = 0;
			float d = dir.Length();
			if (d < 50.0)
			{
				lastWhy = "close";
				continue;
			}
			dir = dir * (1.0 / d);
			float step = Math.Min(d - 40.0, 30.0);
			vector pos = me + dir * step;
			if (n == 1)
			{
				vector side = Vector(-dir[2], 0, dir[0]);
				if (Math.RandomInt(0, 2) == 0)
					side = side * -1.0;
				pos = pos + side * 10.0;
			}
			pos = DCO_Util.Surface(pos);
			if (leader && vector.DistanceXZ(pos, leader.GetOrigin()) > 150.0)
			{
				lastWhy = "leash";
				continue;
			}
			DCO_RelocateBehavior rb = DCO_RelocateBehavior.Start(mu, pos, ep, true, ECharacterStance.CROUCH, 14.0, "agg_pursue");
			if (!rb)
			{
				lastWhy = "blocked";
				continue;
			}
			rb.DCO_SetRequireCover(true);
			if (!first)
				first = mu;
			n++;
		}
		if (n == 0)
		{
			DCO_Perf.Count("agg:pursue_skip_" + lastWhy);
			if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdGroup(grp), string.Format("agg_pursue_skip grp=%1 enemy=%2 dist=%3 why=%4",
					DCO_Log.IdGroup(grp), DCO_Log.IdUnit(t.m_Entity), Math.Round(dist), lastWhy));
			return;
		}
		DCO_PursueTrack tr = sq.m_mTrack.Get(t.m_Entity);
		if (tr)
		{
			tr.m_iCount++;
			tr.m_iHits = 0;
		}
		int count;
		if (tr)
			count = tr.m_iCount;
		sq.m_fNextMove_ms = now + 1000.0 * DCO_Stress.T(u, "agg.pursue_cd_s", 30.0) / DCO_Opp.AggrF(u);
		DCO_Opp.BoundCallout(first);
		DCO_Perf.Count("agg:pursue");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("agg_pursue grp=%1 enemy=%2 dist=%3 grew_m=%4 quiet_s=%5 movers=%6 n=%7 dot=%8 samples=%9",
			DCO_Log.IdGroup(grp), DCO_Log.IdUnit(t.m_Entity), Math.Round(dist), Math.Round(grew), quiet.ToString(-1, 1), n, count, dot.ToString(-1, 2), hits));
	}

	protected static string Gate(SCR_AIUtilityComponent u)
	{
		if (DCO_Roles.Get(u) & 65)
			return "mg";
		return DCO_Opp.Gate(u, true);
	}
}
