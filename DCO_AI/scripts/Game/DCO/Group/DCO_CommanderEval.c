class DCO_CommanderEval
{
	static void Tick(SCR_AIGroupUtilityComponent u, SCR_AIGroup grp, DCO_GroupUtilityComponent g, bool active)
	{
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		array<SCR_AICombatComponent> combats = {};
		array<IEntity> ents = {};
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			IEntity e = a.GetControlledEntity();
			if (!ca || !ca.m_UtilityComponent || !ca.m_UtilityComponent.m_CombatComponent || !e || !DCO_Util.IsAlive(e))
				continue;
			if (SCR_CharacterHelper.IsPlayer(e))
				continue;
			combats.Insert(ca.m_UtilityComponent.m_CombatComponent);
			ents.Insert(e);
		}

		if (!active)
		{
			foreach (SCR_AICombatComponent c0 : combats)
				c0.DCO_SetCEHold(false, 0);
			return;
		}
		if (combats.IsEmpty())
			return;

		float now = GetGame().GetWorld().GetWorldTime();
		IEntity leader = grp.GetLeaderEntity();

		if (g && leader && g.GetGroupObjective() && g.GetGroupObjective().GetOwner()
			&& (g.GetTask() == DCO_EGroupTask.ATTACK || g.GetTask() == DCO_EGroupTask.FLANK))
		{
			vector op = g.GetGroupObjective().GetOwner().GetOrigin();
			if (g.GetPhase() == DCO_ETaskPhase.EXECUTING || vector.DistanceXZ(leader.GetOrigin(), op) <= g.GetGroupObjective().GetRadius())
			{
				FreeAll(combats, now, "assault");
				return;
			}
		}

		array<int> forced = {};
		foreach (int i, SCR_AICombatComponent c : combats)
		{
			if (c.DCO_NearMissCount(now, 3500.0) > 5)
			{
				forced.Insert(i);
				DCO_Perf.Count("ce_forced_nearmiss");
			}
		}

		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (pm && u.m_Perception && leader)
		{
			float pmNow = pm.GetTime();
			vector lp = leader.GetOrigin();
			bool objCheck = g && g.GetGroupObjective() && g.GetGroupObjective().GetOwner();
			vector op2;
			float objR;
			if (objCheck)
			{
				op2 = g.GetGroupObjective().GetOwner().GetOrigin();
				objR = g.GetGroupObjective().GetRadius();
			}
			AIWaypoint wp = grp.GetCurrentWaypoint();
			foreach (SCR_AITargetInfo t : u.m_Perception.m_aTargets)
			{
				if (!t || pmNow - t.m_fTimestamp > 10.0)
					continue;
				if (t.m_eCategory != EAITargetInfoCategory.DETECTED && t.m_eCategory != EAITargetInfoCategory.IDENTIFIED)
					continue;
				vector tp = t.m_vWorldPos;
				string why;
				if (objCheck && vector.DistanceXZ(tp, op2) <= objR)
					why = "objective";
				else if (wp && NearRoute(lp, wp.GetOrigin(), tp, 100.0))
					why = "route";
				if (why.IsEmpty())
					continue;
				DCO_Perf.Count("ce_forced_" + why);
				AddNearest(ents, tp, forced, 2);
			}
		}

		if (forced.IsEmpty())
		{
			foreach (SCR_AICombatComponent ch : combats)
				ch.DCO_SetCEHold(true, now);
			return;
		}

		if (forced.Count() * 2 > combats.Count())
		{
			FreeAll(combats, now, "majority");
			return;
		}

		array<int> free = {};
		free.Copy(forced);
		foreach (int fi : forced)
			AddNearest(ents, ents[fi].GetOrigin(), free, 2);

		foreach (int j, SCR_AICombatComponent cj : combats)
		{
			if (free.Contains(j))
				cj.DCO_FreeCE(now + 8000.0);
			cj.DCO_SetCEHold(true, now);
		}
	}

	protected static void FreeAll(array<SCR_AICombatComponent> combats, float now, string why)
	{
		foreach (SCR_AICombatComponent c : combats)
		{
			c.DCO_FreeCE(now + 8000.0);
			c.DCO_SetCEHold(true, now);
		}
		DCO_Perf.Count("ce_group_free");
	}

	protected static void AddNearest(array<IEntity> ents, vector pos, array<int> into, int count)
	{
		for (int n = 0; n < count; n++)
		{
			int best = -1;
			float bestD = float.MAX;
			foreach (int i, IEntity e : ents)
			{
				if (into.Contains(i))
					continue;
				float d = vector.DistanceSqXZ(e.GetOrigin(), pos);
				if (d < bestD)
				{
					bestD = d;
					best = i;
				}
			}
			if (best < 0)
				return;
			into.Insert(best);
		}
	}

	protected static bool NearRoute(vector a, vector b, vector p, float corridor)
	{
		vector ab = b - a;
		ab[1] = 0;
		float len = ab.Length();
		if (len < 1)
			return false;
		vector dir = ab * (1.0 / len);
		vector ap = p - a;
		ap[1] = 0;
		float along = vector.Dot(ap, dir);
		if (along < 0 || along > len)
			return false;
		vector closest = a + dir * along;
		return vector.DistanceXZ(closest, p) <= corridor;
	}
}
