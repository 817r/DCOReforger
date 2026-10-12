class DCO_Formation
{
	static bool On()
	{
		return DCO_Ctx.TuneG("fm.auto", 1.0) > 0;
	}

	static string Suffix(int sp)
	{
		if (sp >= 2)
			return "_W";
		if (sp == 1)
			return "_N";
		return "";
	}

	static bool Apply(SCR_AIGroup grp, string name)
	{
		if (!grp)
			return false;
		AIFormationComponent fc = grp.GetFormationComponent();
		bool ok = fc && fc.SetFormation(name);
		AIGroupMovementComponent mc = AIGroupMovementComponent.Cast(grp.FindComponent(AIGroupMovementComponent));
		if (mc)
		{
			for (int h = 0; h < 16 && mc.GetMoveHandlerAgentCount(h) != -1; h++)
				mc.SetFormationDefinition(h, name);
		}
		return ok;
	}

	static float Span(string name, int n)
	{
		AIFormationDefinition def = GetGame().GetAIWorld().GetFormation(name);
		if (!def)
			return -1;
		float best;
		for (int i = 1; i < n; i++)
		{
			vector o = def.GetOffsetPosition(i);
			o[1] = 0;
			best = Math.Max(best, o.Length());
		}
		return best;
	}

	static string ThreatSide(SCR_AIGroup grp, IEntity leader)
	{
		SCR_AIGroupUtilityComponent gu = SCR_AIGroupUtilityComponent.Cast(grp.FindComponent(SCR_AIGroupUtilityComponent));
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!gu || !gu.m_Perception || !pm)
			return "";
		vector lp = leader.GetOrigin();
		float pmNow = pm.GetTime();
		float bestD = 400.0;
		vector enemy;
		bool found;
		foreach (SCR_AITargetInfo ti : gu.m_Perception.m_aTargets)
		{
			if (!ti || !ti.m_Entity || pmNow - ti.m_fTimestamp > 30.0 || !DCO_Util.IsAlive(ti.m_Entity))
				continue;
			float d = vector.DistanceXZ(lp, ti.m_vWorldPos);
			if (d >= bestD)
				continue;
			bestD = d;
			enemy = ti.m_vWorldPos;
			found = true;
		}
		if (!found)
			return "";
		vector head;
		Physics ph = leader.GetPhysics();
		if (ph)
			head = ph.GetVelocity();
		head[1] = 0;
		if (head.Length() < 0.5)
		{
			AIWaypoint wp = grp.GetCurrentWaypoint();
			if (wp && vector.DistanceXZ(wp.GetOrigin(), lp) > 10.0)
				head = wp.GetOrigin() - lp;
			else
				head = leader.GetWorldTransformAxis(2);
			head[1] = 0;
		}
		vector toE = enemy - lp;
		toE[1] = 0;
		if (head.Length() < 0.01 || toE.Length() < 0.01)
			return "";
		head.Normalize();
		toE.Normalize();
		float dot = vector.Dot(head, toE);
		if (dot > 0.866 || dot < -0.5)
			return "";
		if (head[0] * toE[2] - head[2] * toE[0] < 0)
			return "R";
		return "L";
	}

	static string ShapeName(int i)
	{
		switch (i)
		{
			case 1: return "Wedge";
			case 2: return "Line";
			case 3: return "Column";
			case 4: return "StaggeredColumn";
			case 5: return "EchelonL";
			case 6: return "EchelonR";
			case 7: return "Vee";
			case 8: return "Diamond";
			case 9: return "SquadWedge";
		}
		return "";
	}

	static string Pick(SCR_AIGroup grp, string base, bool retreat, bool contact, bool recon, int forceShape, int forceSp, out string why)
	{
		why = "-";
		if (!grp)
			return "";
		IEntity leader = grp.GetLeaderEntity();
		int n = grp.GetAgentsCount();
		if (!leader || n < 2 || DCO_VehicleCombat.GetVehicle(leader))
			return "";
		int env = DCO_Ctx.EnvOf(leader);
		bool night = DCO_Util.IsNight();
		bool tight = env == DCO_EEnv.URBAN || env == DCO_EEnv.INDOOR || env == DCO_EEnv.TRENCH;
		string envKey = "open";
		int sp = 2;
		if (tight)
		{
			envKey = "urban";
			sp = 0;
		}
		else if (env == DCO_EEnv.FOREST)
		{
			envKey = "forest";
			sp = 1;
		}
		bool contactLine = DCO_Ctx.TuneFor(grp, "fm.contact_line", 1.0) > 0;
		string shape = base;
		string side = "-";
		string src = "auto";
		if (retreat)
			shape = "StaggeredColumn";
		else if (contact && contactLine)
			shape = "Line";
		else if (tight)
		{
			shape = "StaggeredColumn";
			if (recon)
				shape = "Column";
		}
		else if (env == DCO_EEnv.FOREST && shape == "Wedge")
			shape = "StaggeredColumn";
		else if ((shape == "Wedge" || shape == "Line") && DCO_Ctx.TuneFor(grp, "fm.echelon", 1.0) > 0)
		{
			string s = ThreatSide(grp, leader);
			if (s != "")
			{
				shape = "Echelon" + s;
				side = s;
			}
		}
		int ruleShape = Math.Round(DCO_Ctx.TuneFor(grp, "fm." + envKey + "_shape", 0.0));
		if (ruleShape > 0 && !retreat && !(contact && contactLine) && ShapeName(ruleShape) != "")
		{
			shape = ShapeName(ruleShape);
			src = "rule_" + envKey;
		}
		if (forceShape > 0 && ShapeName(forceShape) != "")
		{
			shape = ShapeName(forceShape);
			src = "gm_squad";
		}
		if (shape == "Wedge" && n >= 8 && forceShape <= 0 && ruleShape <= 0)
			shape = "SquadWedge";
		int ruleSp = Math.Round(DCO_Ctx.TuneFor(grp, "fm." + envKey + "_sp", 0.0));
		if (ruleSp > 0)
			sp = Math.Clamp(ruleSp - 1, 0, 2);
		if (night && sp > 0 && DCO_Ctx.TuneFor(grp, "fm.night_tight", 1.0) > 0)
			sp--;
		if (contact && sp > 1)
			sp = 1;
		if (sp > 1 && n > 6)
			sp = 1;
		if (forceSp > 0)
			sp = Math.Clamp(forceSp - 1, 0, 2);
		float limit = 80.0;
		DCO_GroupConfigComponent gcfg = DCO_GroupConfigComponent.Cast(grp.FindComponent(DCO_GroupConfigComponent));
		if (gcfg)
			limit = Math.Max(gcfg.GetCohesionDistance(), 80.0);
		if (contact)
		{
			bool urban;
			limit = DCO_Util.CombatCohesionDist(grp, leader, leader, urban);
		}
		limit = 0.7 * limit;
		int spWant = sp;
		float span = Span(shape + Suffix(sp), n);
		while (sp > 0 && span > limit)
		{
			sp--;
			span = Span(shape + Suffix(sp), n);
		}
		string name = shape + Suffix(sp);
		why = string.Format("src=%1 env=%2 night=%3 contact=%4 retreat=%5 threat_side=%6 n=%7 span=%8 limit=%9", src, DCO_Ctx.EnvName(env), DCO_Util.B01(night), DCO_Util.B01(contact), DCO_Util.B01(retreat), side, n, Math.Round(span), Math.Round(limit));
		if (spWant != sp)
			why += " spacing_capped=" + spWant.ToString() + "to" + sp.ToString();
		return name;
	}
}
