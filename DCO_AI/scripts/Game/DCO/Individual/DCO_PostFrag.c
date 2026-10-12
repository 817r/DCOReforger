class DCO_PostFragPending
{
	SCR_AIUtilityComponent m_Thrower;
	vector m_vTarget;
	float m_fAt_ms;
	float m_fMinD = 9999.0;
	int m_iSeen;
}

class DCO_PostFrag
{
	protected static ref map<IEntity, ref DCO_PostFragPending> s_mPending;
	protected static ref map<AIGroup, float> s_mGroupNext;

	static bool On(SCR_AIUtilityComponent u)
	{
		return u && DCO_Stress.T(u, "agg.postfrag", 1.0) > 0;
	}

	static void OnThrow(SCR_AIUtilityComponent thrower, vector target)
	{
		if (!thrower || !thrower.m_OwnerEntity || target == vector.Zero || !On(thrower))
			return;
		IEntity me = thrower.m_OwnerEntity;
		float d = vector.DistanceXZ(me.GetOrigin(), target);
		if (d < 10.0 || d > 35.0)
		{
			DCO_Perf.Count("agg:postfrag_skip_range");
			return;
		}
		AIAgent agent = thrower.GetAIAgent();
		if (!agent || !agent.GetParentGroup())
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (!s_mGroupNext)
			s_mGroupNext = new map<AIGroup, float>();
		float next;
		if (s_mGroupNext.Find(agent.GetParentGroup(), next) && now < next)
		{
			DCO_Perf.Count("agg:postfrag_skip_cooldown");
			return;
		}
		if (!s_mPending)
			s_mPending = new map<IEntity, ref DCO_PostFragPending>();
		DCO_PostFragPending p = new DCO_PostFragPending();
		p.m_Thrower = thrower;
		p.m_vTarget = target;
		p.m_fAt_ms = now;
		s_mPending.Set(me, p);
		DCO_Perf.Count("agg:postfrag_armed");
	}

	static void OnBurst(IEntity src, vector burst)
	{
		if (!s_mPending || s_mPending.IsEmpty())
			return;
		DCO_Perf.Count("agg:postfrag_burst_seen");
		float now = GetGame().GetWorld().GetWorldTime();
		IEntity bestKey;
		DCO_PostFragPending p;
		float burstD = 25.0;
		bool bySrc;
		array<IEntity> stale = {};
		foreach (IEntity k, DCO_PostFragPending q : s_mPending)
		{
			if (!k || !q || !q.m_Thrower || !q.m_Thrower.m_OwnerEntity || now - q.m_fAt_ms > 12000.0)
			{
				stale.Insert(k);
				continue;
			}
			float d = vector.DistanceXZ(burst, q.m_vTarget);
			q.m_iSeen++;
			if (d < q.m_fMinD)
				q.m_fMinD = d;
			if (d <= burstD)
			{
				burstD = d;
				bestKey = k;
				p = q;
			}
		}
		foreach (IEntity k : stale)
		{
			DCO_PostFragPending sq = s_mPending.Get(k);
			if (sq)
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(k), string.Format("agg_postfrag_skip unit=%1 why=no_burst age_s=%2 bursts_seen=%3 min_burst_d=%4", DCO_Log.IdUnit(k), Math.Round((now - sq.m_fAt_ms) / 1000.0), sq.m_iSeen, Math.Round(sq.m_fMinD)));
			s_mPending.Remove(k);
			DCO_Perf.Count("agg:postfrag_skip_no_burst");
		}
		if (src)
		{
			DCO_PostFragPending sp = s_mPending.Get(src);
			if (sp && sp.m_Thrower && sp.m_Thrower.m_OwnerEntity && now - sp.m_fAt_ms <= 12000.0)
			{
				p = sp;
				bestKey = src;
				burstD = vector.DistanceXZ(burst, sp.m_vTarget);
				bySrc = true;
			}
		}
		if (!p)
			return;
		s_mPending.Remove(bestKey);
		if (bySrc)
			DCO_Perf.Count("agg:postfrag_match_src");
		else
			DCO_Perf.Count("agg:postfrag_match_pos");
		if (burstD > 15.0)
		{
			Skip(p, "miss", burstD);
			return;
		}
		Rush(p, burst, burstD, now);
	}

	protected static void Rush(DCO_PostFragPending p, vector burst, float burstD, float now)
	{
		IEntity thrower = p.m_Thrower.m_OwnerEntity;
		AIAgent ta = p.m_Thrower.GetAIAgent();
		if (!ta || !ta.GetParentGroup())
			return;
		AIGroup grp = ta.GetParentGroup();
		array<AIAgent> mates = {};
		grp.GetAgents(mates);
		SCR_AIUtilityComponent best;
		float bestD = 30.0;
		string lastWhy = "no_mate";
		int inRange;
		float nearest = 9999.0;
		foreach (AIAgent a : mates)
		{
			if (!a)
				continue;
			SCR_AIUtilityComponent mu = SCR_AIUtilityComponent.Cast(a.FindComponent(SCR_AIUtilityComponent));
			if (!mu)
				continue;
			IEntity mate = a.GetControlledEntity();
			if (!mate || mate == thrower || !DCO_Util.IsAlive(mate))
				continue;
			float md = vector.DistanceXZ(mate.GetOrigin(), thrower.GetOrigin());
			if (md < nearest)
				nearest = md;
			if (md > bestD)
				continue;
			inRange++;
			string why = Gate(mu);
			if (why != "")
			{
				lastWhy = why;
				continue;
			}
			best = mu;
			bestD = md;
		}
		if (!best)
		{
			Skip(p, lastWhy + " mates_30m=" + inRange.ToString() + " nearest_m=" + Math.Round(nearest).ToString(), burstD);
			return;
		}
		vector dir = p.m_vTarget - thrower.GetOrigin();
		dir[1] = 0;
		float lineD = dir.Length();
		if (lineD < 1.0)
			return;
		dir = dir * (1.0 / lineD);
		float shortOf = Math.RandomFloatInclusive(8.0, 12.0);
		vector pos = p.m_vTarget - dir * shortOf;
		if (vector.DistanceXZ(pos, burst) < 8.0)
			pos = burst - dir * 8.0;
		pos = DCO_Util.Surface(pos);
		IEntity rusher = best.m_OwnerEntity;
		float rushTo = vector.DistanceXZ(rusher.GetOrigin(), pos);
		if (vector.DistanceXZ(rusher.GetOrigin(), p.m_vTarget) <= vector.DistanceXZ(pos, p.m_vTarget) + 2.0)
		{
			Skip(p, "already_forward", burstD);
			return;
		}
		DCO_RelocateBehavior rb = DCO_RelocateBehavior.Start(best, pos, p.m_vTarget, true, ECharacterStance.CROUCH, 6.0, "agg_postfrag");
		if (!rb)
		{
			Skip(p, "blocked", burstD);
			return;
		}
		rb.DCO_SetMove(ECharacterStance.CROUCH, EMovementType.SPRINT, 5.0, true);
		s_mGroupNext.Set(grp, now + 1000.0 * DCO_Stress.T(p.m_Thrower, "agg.postfrag_cd_s", 20.0) / DCO_Opp.AggrF(p.m_Thrower));
		DCO_Perf.Count("agg:postfrag");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(rusher), string.Format("agg_postfrag unit=%1 thrower=%2 tgt_d=%3 rush_to_d=%4 burst_d=%5 short_of=%6 mate_d=%7",
			DCO_Log.IdUnit(rusher), DCO_Log.IdUnit(thrower), Math.Round(lineD), Math.Round(rushTo), burstD.ToString(-1, 1), Math.Round(shortOf), Math.Round(bestD)));
	}

	protected static string Gate(SCR_AIUtilityComponent u)
	{
		if (DCO_Roles.Get(u) & 65)
			return "mg";
		return DCO_Opp.Gate(u, true);
	}

	protected static void Skip(DCO_PostFragPending p, string why, float burstD)
	{
		string key = why;
		int sp = why.IndexOf(" ");
		if (sp > 0)
			key = why.Substring(0, sp);
		DCO_Perf.Count("agg:postfrag_skip_" + key);
		if (!p.m_Thrower || !p.m_Thrower.m_OwnerEntity)
			return;
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(p.m_Thrower.m_OwnerEntity), string.Format("agg_postfrag_skip thrower=%1 why=%2 burst_d=%3",
				DCO_Log.IdUnit(p.m_Thrower.m_OwnerEntity), why, burstD.ToString(-1, 1)));
	}
}
