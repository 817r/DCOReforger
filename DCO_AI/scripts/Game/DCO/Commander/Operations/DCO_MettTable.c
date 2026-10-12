class DCO_MettTable
{
	static bool On()
	{
		return DCO_Ctx.TuneG("tac.mett_on", 0) > 0;
	}

	static string Row(int t)
	{
		switch (t)
		{
			case DCO_ETactic.MOVEMENT_TO_CONTACT:	return "Pmxm.mp" + "p.mm.." + "m.pp.pm";
			case DCO_ETactic.RECON_IN_FORCE:		return "pPxp..m" + "p..m.." + "Mpp..mp";
			case DCO_ETactic.FIRE_AND_MANEUVER:		return "x.pmmmp" + "m.mm.p" + "MmpPpP.";
			case DCO_ETactic.FLANKING:				return "x.ppp.." + ".p..Px" + "mppp.mp";
			case DCO_ETactic.HAMMER_AND_ANVIL:		return "x.Pm..p" + "p.mmpm" + "xMpP.Mp";
			case DCO_ETactic.PINCER:				return "x.Pp..m" + ".pmmPM" + "xxpP.mp";
			case DCO_ETactic.INFILTRATION:			return "mpp..mm" + "MPPPpm" + ".pp..MP";
			case DCO_ETactic.SIEGE:					return "x.pP.pm" + "m.P..." + "pp.mPMP";
			case DCO_ETactic.BYPASS:				return "...ppp." + ".p.pp." + "Ppxx.Pm";
			case DCO_ETactic.RAID:					return "m.pm.mm" + "mppPp." + "Ppxx.p.";
			case DCO_ETactic.ATTACK_BY_FIRE:		return "m.pppp." + "Pmmm.." + "Pp.mp.p";
		}
		return "";
	}

	static string ColName(int c)
	{
		array<string> names = {"intel0", "intel1", "intel2", "dug", "mg", "armor", "inbound", "open", "forest", "urban", "night", "side", "noside", "weak", "equal", "fav", "strong", "fire", "short", "ample"};
		return names[c];
	}

	static float Mult(string sym)
	{
		switch (sym)
		{
			case "P": return 1.6;
			case "p": return 1.3;
			case "m": return 0.6;
			case "M": return 0.3;
			case "x": return 0;
		}
		return 1.0;
	}

	static void Read(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, DCO_TacticSituation s)
	{
		vector objPos = obj.GetOwner().GetOrigin();
		s.m_iIntel = 2;
		if (!s.m_bEverSeen || s.m_fConfidence < DCO_Ctx.TuneForCmd(cmd, "tac.mett_intel_lo", 0.25))
			s.m_iIntel = 0;
		else if (s.m_fConfidence < DCO_Ctx.TuneForCmd(cmd, "tac.mett_intel_hi", 0.55))
			s.m_iIntel = 1;

		s.m_bEnemyArmor = cmd.GetOps().GetIntel(obj).m_bArmor;
		int near = 0;
		int moving = 0;
		CMD_ThreatResponseComponent threat = cmd.GetThreatResponseComponent();
		if (threat)
		{
			float nearSq = (obj.GetRadius() + 150.0) * (obj.GetRadius() + 150.0);
			foreach (CMD_ThreatEntry te : threat.GetThreats())
			{
				if (!te || vector.DistanceSqXZ(te.m_vPosition, objPos) > nearSq)
					continue;
				near++;
				if (te.m_bMoving)
					moving++;
				if (te.m_bArmorSeen)
					s.m_bEnemyArmor = true;
			}
		}
		s.m_bDugIn = near > 0 && moving == 0;

		vector from = cmd.GetOwner().GetOrigin();
		int n = 0;
		vector sum = vector.Zero;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || !g.GetOwner() || g.IsPlayerGroup())
				continue;
			sum = sum + g.GetOwner().GetOrigin();
			n++;
		}
		if (n > 0)
			from = sum * (1.0 / n);
		vector axis = from - objPos;
		axis[1] = 0;
		if (axis.LengthSq() < 1)
			axis = "0 0 1";
		axis.Normalize();
		float baseYaw = Math.Atan2(axis[0], axis[2]);
		float dist = obj.GetRadius() + 150.0;
		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		s.m_iSides = 0;
		for (int side = -1; side <= 1; side += 2)
		{
			float yaw = baseYaw + side * 70.0 * Math.DEG2RAD;
			vector p = objPos + Vector(Math.Sin(yaw), 0, Math.Cos(yaw)) * dist;
			array<DCO_TerrainPoint> pts = {};
			bool covered = DCO_TerrainCache.Query(DCO_ETerrainFlag.FOREST_EDGE, p, 100.0, pts) >= 2;
			if (!covered && reg)
			{
				array<IEntity> blds = {};
				reg.FindBuildings(p, 100.0, blds);
				covered = blds.Count() >= 3;
			}
			if (covered)
				s.m_iSides++;
		}

		s.m_iTime = 0;
		if (s.m_bEnemyInbound || (s.m_bOtherObjective && cmd.GetDecision() == DCO_EDecision.FAST))
			s.m_iTime = -1;
		else if (cmd.GetDecision() == DCO_EDecision.SLOW || cmd.GetPatience() >= 0.7)
			s.m_iTime = 1;

		s.m_iBand = DCO_DoctrineRuntime.BandOf(s.m_fRatio);

		string terrain = "open";
		if (s.m_bUrban)
			terrain = "urban";
		else if (s.m_bForest)
			terrain = "forest";
		array<string> timeNames = {"short", "normal", "ample"};
		string conf = s.m_fConfidence.ToString(-1, 2);
		string ratio = s.m_fRatio.ToString(-1, 2);
		string band = DCO_Doctrine.BandName(s.m_iBand);
		string line = string.Format("mett_read cmd=%1 obj=%2 intel=%3 conf=%4 dug=%5 mg=%6 armor=%7 inbound=%8 terrain=%9", cmd.GetCommanderUID(), obj.GetOwner().GetName(), s.m_iIntel, conf, s.m_bDugIn, s.m_bEnemyMG, s.m_bEnemyArmor, s.m_bEnemyInbound, terrain);
		line += string.Format(" night=%1 sides=%2 escape=%3 band=%4 ratio=%5 groups=%6 small=%7 fire=%8 mortar=%9", s.m_bNight, s.m_iSides, s.m_bRetreat, band, ratio, s.m_iGroups, s.m_iSmallGroups, s.m_bFireSupport, s.m_bMortar);
		line += string.Format(" other=%1 time=%2", s.m_bOtherObjective, timeNames[s.m_iTime + 1]);
		DCO_CommanderTactics.Event(line);
	}

	static string Gate(int t, DCO_TacticSituation s)
	{
		switch (t)
		{
			case DCO_ETactic.MOVEMENT_TO_CONTACT:
				if (s.m_iGroups < 2) return "groups";
				break;
			case DCO_ETactic.RECON_IN_FORCE:
			case DCO_ETactic.FIRE_AND_MANEUVER:
			case DCO_ETactic.FLANKING:
				if (s.m_iGroups < 2) return "groups";
				break;
			case DCO_ETactic.HAMMER_AND_ANVIL:
				if (s.m_iGroups < 3) return "groups";
				if (!s.m_bRetreat) return "no_escape";
				break;
			case DCO_ETactic.PINCER:
				if (s.m_iGroups < 3) return "groups";
				if (s.m_iSides < 2) return "one_side";
				break;
			case DCO_ETactic.INFILTRATION:
				if (s.m_iSmallGroups < 1) return "no_small";
				if (s.m_iGroups < 2) return "groups";
				break;
			case DCO_ETactic.SIEGE:
				if (s.m_iGroups < 3 && !s.m_bMortar) return "groups";
				if (!s.m_bUrban && !s.m_bDugIn) return "not_dug";
				break;
			case DCO_ETactic.BYPASS:
				if (!s.m_bOtherObjective) return "no_other";
				if (s.m_iGroups < 1) return "groups";
				break;
			case DCO_ETactic.RAID:
				if (s.m_iGroups < 2) return "groups";
				break;
			case DCO_ETactic.ATTACK_BY_FIRE:
				if (!s.m_bFireSupport) return "no_fire";
				break;
			default:
				return "event_only";
		}
		return "";
	}

	static float Slider(AICommander_BaseComponent cmd)
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetForCommander(cmd);
		if (!g)
			return 1.0;
		return g.DCO_AggressionSlider();
	}

	static float Trait(AICommander_BaseComponent cmd, int t)
	{
		switch (t)
		{
			case DCO_ETactic.FIRE_AND_MANEUVER:
			case DCO_ETactic.HAMMER_AND_ANVIL:
			case DCO_ETactic.RAID:
				return 0.8 + 0.4 * Math.Clamp(cmd.GetAggression() * Slider(cmd), 0, 1.5);
			case DCO_ETactic.FLANKING:
			case DCO_ETactic.PINCER:
			case DCO_ETactic.INFILTRATION:
			case DCO_ETactic.BYPASS:
				return 0.8 + 0.4 * cmd.GetAdaptability();
			case DCO_ETactic.SIEGE:
			case DCO_ETactic.ATTACK_BY_FIRE:
				return 0.8 + 0.4 * cmd.GetPatience();
			case DCO_ETactic.RECON_IN_FORCE:
			case DCO_ETactic.MOVEMENT_TO_CONTACT:
				return 0.8 + 0.2 * Math.Clamp(cmd.GetTraitIntel(), 0, 2);
		}
		return 1.0;
	}

	static void Columns(DCO_TacticSituation s, notnull array<int> cols)
	{
		cols.Insert(s.m_iIntel);
		if (s.m_bDugIn) cols.Insert(3);
		if (s.m_bEnemyMG) cols.Insert(4);
		if (s.m_bEnemyArmor) cols.Insert(5);
		if (s.m_bEnemyInbound) cols.Insert(6);
		if (s.m_bUrban)
			cols.Insert(9);
		else if (s.m_bForest)
			cols.Insert(8);
		else
			cols.Insert(7);
		if (s.m_bNight) cols.Insert(10);
		if (s.m_iSides > 0)
			cols.Insert(11);
		else
			cols.Insert(12);
		switch (s.m_iBand)
		{
			case DCO_EForceBand.WEAKER: cols.Insert(13); break;
			case DCO_EForceBand.EQUAL: cols.Insert(14); break;
			case DCO_EForceBand.FAVORABLE: cols.Insert(15); break;
			default: cols.Insert(16); break;
		}
		if (s.m_bFireSupport || s.m_bMortar) cols.Insert(17);
		if (s.m_iTime < 0) cols.Insert(18);
		if (s.m_iTime > 0) cols.Insert(19);
	}

	static float Score(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, int t, DCO_TacticSituation s, bool failed)
	{
		string objName = obj.GetOwner().GetName();
		string gate = Gate(t, s);
		if (!gate.IsEmpty())
		{
			if (gate != "event_only")
			{
				DCO_CommanderTactics.Event(string.Format("mett_gate obj=%1 tactic=%2 why=%3", objName, DCO_CommanderTactics.Name(t), gate));
				DCO_Perf.Count("mett:gate:" + DCO_CommanderTactics.Name(t));
			}
			return 0;
		}

		string row = Row(t);
		array<int> cols = {};
		Columns(s, cols);
		float v = 1.0;
		string why;
		foreach (int c : cols)
		{
			string sym = row.Get(c);
			if (sym == ".")
				continue;
			v *= Mult(sym);
			why += ColName(c) + sym + ",";
		}
		if (v <= 0)
		{
			DCO_CommanderTactics.Event(string.Format("mett_gate obj=%1 tactic=%2 why=table:%3", objName, DCO_CommanderTactics.Name(t), why));
			DCO_Perf.Count("mett:gate:" + DCO_CommanderTactics.Name(t));
			return 0;
		}
		float trait = Trait(cmd, t);
		v *= trait;
		if (failed)
			v *= 0.3;
		float mission = Mission(obj, t);
		float recent = Recent(cmd, t);
		v *= mission * recent;
		if (v <= 0)
		{
			DCO_CommanderTactics.Event(string.Format("mett_gate obj=%1 tactic=%2 why=mission", objName, DCO_CommanderTactics.Name(t)));
			DCO_Perf.Count("mett:gate:" + DCO_CommanderTactics.Name(t));
			return 0;
		}
		string line = string.Format("mett_score obj=%1 tactic=%2 score=%3 trait=%4 failed=%5 why=%6", objName, DCO_CommanderTactics.Name(t), v.ToString(-1, 2), trait.ToString(-1, 2), failed, why);
		line += " mission=" + mission.ToString(-1, 2) + " recent=" + recent.ToString(-1, 2);
		DCO_CommanderTactics.Event(line);
		return v;
	}

	protected static ref map<string, float> s_mLastPick;

	static float Mission(CMD_AICommanderObjectiveComponent obj, int t)
	{
		CMD_EObjectiveType type = obj.GetObjectiveType();
		if (type == CMD_EObjectiveType.CAPTURE || type == CMD_EObjectiveType.DEFEND || type == CMD_EObjectiveType.HOLD_UNTIL)
		{
			switch (t)
			{
				case DCO_ETactic.RAID: return 0.3;
				case DCO_ETactic.ATTACK_BY_FIRE: return 0.6;
			}
		}
		else if (type == CMD_EObjectiveType.DESTROY)
		{
			switch (t)
			{
				case DCO_ETactic.RAID: return 1.6;
				case DCO_ETactic.ATTACK_BY_FIRE: return 1.6;
				case DCO_ETactic.SIEGE: return 1.3;
				case DCO_ETactic.BYPASS: return 0.6;
			}
		}
		return 1.0;
	}

	static float Recent(AICommander_BaseComponent cmd, int t)
	{
		float at;
		if (!s_mLastPick || !s_mLastPick.Find(cmd.GetCommanderUID() + "|" + t.ToString(), at))
			return 1.0;
		if (DCO_CommanderOps.Now() - at > DCO_Ctx.TuneForCmd(cmd, "tac.mett_recent_s", 600.0))
			return 1.0;
		return DCO_Ctx.TuneForCmd(cmd, "tac.mett_recent_mul", 0.7);
	}

	static void NotePick(AICommander_BaseComponent cmd, int t)
	{
		if (!s_mLastPick)
			s_mLastPick = new map<string, float>();
		s_mLastPick.Set(cmd.GetCommanderUID() + "|" + t.ToString(), DCO_CommanderOps.Now());
	}

	static int Pick(CMD_AICommanderObjectiveComponent obj, array<float> scores)
	{
		float top = 0;
		foreach (float sc : scores)
			top = Math.Max(top, sc);
		if (top <= 0)
			return -1;
		float floor = top * DCO_Ctx.TuneG("tac.mett_band", 0.85);
		float sum = 0;
		for (int i = 0; i < scores.Count(); i++)
		{
			if (scores[i] >= floor)
				sum += scores[i];
		}
		float roll = Math.RandomFloat(0, sum);
		int chosen = -1;
		string pool;
		for (int j = 0; j < scores.Count(); j++)
		{
			if (scores[j] < floor)
				continue;
			pool += string.Format("%1:%2,", DCO_CommanderTactics.Name(j), (scores[j] / sum).ToString(-1, 2));
			if (chosen < 0)
			{
				roll -= scores[j];
				if (roll <= 0)
					chosen = j;
			}
		}
		if (chosen < 0)
		{
			for (int k = scores.Count() - 1; k >= 0; k--)
			{
				if (scores[k] >= floor)
				{
					chosen = k;
					break;
				}
			}
		}
		DCO_CommanderTactics.Event(string.Format("mett_pick obj=%1 tactic=%2 top=%3 pool=%4", obj.GetOwner().GetName(), DCO_CommanderTactics.Name(chosen), top.ToString(-1, 2), pool));
		DCO_Perf.Count("mett:pick:" + DCO_CommanderTactics.Name(chosen));
		return chosen;
	}
}
