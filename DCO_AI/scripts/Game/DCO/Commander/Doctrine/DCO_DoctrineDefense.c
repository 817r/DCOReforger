enum DCO_EDefAction
{
	DEFEND,
	DELAY,
	FALLBACK,
	WITHDRAW
}

class DCO_DoctrineDefense
{
	static string ActionName(int act)
	{
		if (act < 0)
			return "none";
		string name = typename.EnumToString(DCO_EDefAction, act);
		name.ToLower();
		return name;
	}

	static float Confidence(array<CMD_ThreatEntry> near, float now)
	{
		float conf = 0;
		foreach (CMD_ThreatEntry t : near)
		{
			if (!t)
				continue;
			float fresh = Math.Clamp(1 - (now - t.m_fLastUpdateTime) / 300.0, 0, 1);
			conf = Math.Max(conf, t.m_fReportQuality * fresh);
		}
		return conf;
	}

	static int BandFor(float eff, float conf, int oldBand)
	{
		if (conf < 0.4)
			return DCO_EForceBand.UNKNOWN;
		int band = DCO_DoctrineRuntime.BandOf(eff);
		if (oldBand >= 0 && oldBand != DCO_EForceBand.UNKNOWN)
			band = DCO_DoctrineRuntime.Hysteresis(oldBand, band, eff);
		return band;
	}

	static int Decide(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float eff, float conf, bool prepared, bool encircled, bool delayAllowed, int oldBand, out int band)
	{
		band = DCO_EForceBand.UNKNOWN;
		DCO_Doctrine d = cmd.GetDoctrine();
		if (!d || !obj)
			return -1;
		band = BandFor(eff, conf, oldBand);
		if (obj.IsRetain())
			return DCO_EDefAction.DEFEND;

		int first = DCO_EDefAction.DEFEND;
		if (band == DCO_EForceBand.WEAKER)
		{
			float scale = d.m_fWeakerScale;
			if (encircled || eff < 0.17 * scale)
				first = DCO_EDefAction.WITHDRAW;
			else if (eff < 0.33 * scale || !prepared)
				first = DCO_EDefAction.DELAY;
		}

		array<int> order = {DCO_EDefAction.DEFEND, DCO_EDefAction.DELAY, DCO_EDefAction.FALLBACK, DCO_EDefAction.WITHDRAW};
		if (first == DCO_EDefAction.DELAY)
			order = {DCO_EDefAction.DELAY, DCO_EDefAction.FALLBACK, DCO_EDefAction.WITHDRAW, DCO_EDefAction.DEFEND};
		else if (first == DCO_EDefAction.WITHDRAW)
			order = {DCO_EDefAction.WITHDRAW, DCO_EDefAction.FALLBACK, DCO_EDefAction.DELAY, DCO_EDefAction.DEFEND};

		foreach (int act : order)
		{
			if (act == DCO_EDefAction.DELAY && !delayAllowed)
				continue;
			if (d.Defense(band, Column(act)))
				return act;
		}
		return DCO_EDefAction.DEFEND;
	}

	static int Column(int act)
	{
		switch (act)
		{
			case DCO_EDefAction.DELAY: return DCO_EDefenseCol.DELAY;
			case DCO_EDefAction.FALLBACK: return DCO_EDefenseCol.FALLBACK;
			case DCO_EDefAction.WITHDRAW: return DCO_EDefenseCol.WITHDRAW;
		}
		return DCO_EDefenseCol.OBJ_DEFENCE;
	}

	static bool Allows(AICommander_BaseComponent cmd, int band, int col)
	{
		DCO_Doctrine d = cmd.GetDoctrine();
		if (!d)
			return true;
		if (band < 0)
			band = DCO_EForceBand.EQUAL;
		return d.Defense(band, col);
	}

	static int LostBand(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, out float eff, out float conf)
	{
		eff = 0;
		conf = 0;
		DCO_DoctrineRuntime rt = cmd.GetDoctrineRuntime();
		if (rt)
		{
			DCO_ForceEval e = rt.GetEval(obj);
			if (e)
			{
				eff = e.m_fEff;
				conf = e.m_fConf;
				if (e.m_iBand != DCO_EForceBand.UNKNOWN)
					return e.m_iBand;
			}
		}
		return DCO_EForceBand.EQUAL;
	}

	static vector RetreatTarget(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, vector axis, bool fallback, out string target)
	{
		vector objPos = obj.GetOwner().GetOrigin();
		FactionKey fk = cmd.GetCommanderFactionKey();
		vector best = vector.Zero;
		if (fallback)
		{
			float bestD = 2000.0;
			foreach (CMD_AICommanderObjectiveComponent o : cmd.DCO_AllObjectives())
			{
				if (!o || o == obj || !o.GetOwner() || !o.IsCapturedBy(fk))
					continue;
				float dist = vector.DistanceXZ(o.GetOwner().GetOrigin(), objPos);
				if (dist < bestD)
				{
					bestD = dist;
					best = o.GetOwner().GetOrigin();
					target = o.GetLogName();
				}
			}
			if (best != vector.Zero)
				return best;
		}
		string src;
		best = cmd.GetHubFor(objPos, src);
		if (best != vector.Zero)
		{
			target = "hub_" + src;
			return best;
		}
		target = "rear";
		return DCO_Util.Surface(objPos - axis * 600.0);
	}

	static void Log(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, string action, int band, float eff, float conf, string extra)
	{
		int retain = 0;
		if (obj.IsRetain())
			retain = 1;
		DCO_Perf.Count("doctrine:defense:" + action);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), string.Format("defense_plan cmd=%1 obj=%2 action=%3 band=%4 eff=%5 retain=%6 conf=%7", cmd.GetCommanderUID(), obj.GetLogName(), action, DCO_Doctrine.BandName(band), eff.ToString(-1, 2), retain, conf.ToString(-1, 2)) + extra);
	}
}
