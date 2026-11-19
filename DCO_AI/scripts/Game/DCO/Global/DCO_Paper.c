// Off-screen battles. A sleeping DCO group that the commander gives a task stays asleep and
// travels as a point on the map toward its current waypoint; two hostile sleeping groups that
// meet fight "on paper" and lose soldiers from their snapshot. Soldiers only exist again when the
// group wakes: a player or thinking enemy AI comes near, it reaches a waypoint where the task
// needs real soldiers, or its path runs into the sea. One central pass every ~5 s, so every
// fight is resolved exactly once per pass and losses are computed before any are applied.
class DCO_Paper
{
	static int s_iMoving;
	static int s_iFighting;

	static float T(string key, float def)
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetInstance();
		if (!g)
			return def;
		return g.DCO_TuneF(key, def);
	}

	static bool On()
	{
		return T("paper.on", 0.0) > 0;
	}

	static void Tick(float dt)
	{
		s_iMoving = 0;
		s_iFighting = 0;
		if (!On())
			return;
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;
		array<DCO_GroupUtilityComponent> sleepers = {};
		foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
		{
			if (!cmd)
				continue;
			foreach (DCO_GroupUtilityComponent gu : cmd.GetOwnedGroups())
			{
				if (gu && gu.GetOwner() && gu.DCO_PaperAsleep())
					sleepers.Insert(gu);
			}
		}
		if (sleepers.IsEmpty())
			return;

		int start = DCO_Perf.Begin();
		float now = DCO_Util.NowS();
		float range = T("paper.fight_m", 400.0);
		float range2 = range * range;
		float rate = T("paper.kill_rate", 0.06) * dt / 60.0;
		float defX = T("paper.defender_x", 1.5);

		int n = sleepers.Count();
		array<vector> pos = {};
		array<Faction> fac = {};
		array<float> power = {};
		array<bool> can = {};
		array<float> fire = {};
		array<string> foe = {};
		foreach (DCO_GroupUtilityComponent s : sleepers)
		{
			pos.Insert(s.GetOwner().GetOrigin());
			fac.Insert(s.DCO_PaperFaction());
			// Fire a group puts out per pass: its soldiers, more if it is dug in.
			float o = rate * s.DCO_PaperMen();
			if (s.DCO_PaperHolding())
				o *= defX;
			power.Insert(o);
			can.Insert(!s.DCO_PaperBroken(now));
			fire.Insert(0);
			foe.Insert("-");
		}

		for (int i = 0; i < n; i++)
		{
			if (!can[i] || !fac[i])
				continue;
			for (int j = i + 1; j < n; j++)
			{
				if (!can[j] || !fac[j] || !fac[i].IsFactionEnemy(fac[j]))
					continue;
				if (vector.DistanceSqXZ(pos[i], pos[j]) > range2)
					continue;
				fire[i] = fire[i] + power[j];
				fire[j] = fire[j] + power[i];
				foe[i] = DCO_Log.IdGroup(AIGroup.Cast(sleepers[j].GetOwner()));
				foe[j] = DCO_Log.IdGroup(AIGroup.Cast(sleepers[i].GetOwner()));
			}
		}

		for (int k = 0; k < n; k++)
		{
			DCO_GroupUtilityComponent g = sleepers[k];
			if (!g || !g.GetOwner())
				continue;
			if (fire[k] > 0)
			{
				s_iFighting++;
				g.DCO_PaperTakeFire(fire[k], now, foe[k]);
				continue;
			}
			g.DCO_PaperTakeFire(0, now, "-");
			if (g.DCO_PaperMove(dt, now))
				s_iMoving++;
		}
		DCO_Perf.End("paper", start);
	}
}
