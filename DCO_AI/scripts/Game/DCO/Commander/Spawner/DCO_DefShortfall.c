class DCO_DefShortfall
{
	static bool On(AICommander_BaseComponent cmd)
	{
		return cmd && DCO_Ctx.TuneForCmd(cmd, "reinf.def_shortfall", 1) > 0;
	}

	static bool FloorOn(AICommander_BaseComponent cmd)
	{
		return cmd && DCO_Ctx.TuneForCmd(cmd, "def.garrison_floor", 1) > 0;
	}

	static int Floor(CMD_AICommanderObjectiveComponent obj, float enemy, bool hasty)
	{
		if (!obj)
			return 0;
		float r = obj.GetRadius();
		int base = 1;
		if (r > 100.0 || obj.DCO_IsPrimary())
			base = 3;
		else if (r > 50.0)
			base = 2;
		if (enemy <= 0)
			return base;
		float div = 3.0;
		if (hasty)
			div = 2.5;
		int byEnemy = Math.Ceil(enemy / div / 8.0);
		return Math.Max(base, byEnemy);
	}

	static int Defenders(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj)
	{
		if (!cmd || !obj)
			return 0;
		int n;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g.IsPlayerGroup() || g.GetUnitCount() <= 0 || g.GetGroupObjective() != obj)
				continue;
			DCO_EGroupTask t = g.GetTask();
			if (t == DCO_EGroupTask.DEFEND || t == DCO_EGroupTask.GARRISON)
				n++;
		}
		return n;
	}

	static void ThreatNear(AICommander_BaseComponent cmd, vector pos, float now, out float dist, out float enemy)
	{
		dist = 99999.0;
		enemy = 0;
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (!tr)
			return;
		foreach (CMD_ThreatEntry t : tr.GetThreats())
		{
			if (!t || now - t.m_fLastUpdateTime > 300.0)
				continue;
			float d = vector.DistanceXZ(t.m_vBelievedPos, pos);
			if (d < dist)
				dist = d;
			if (d < 800.0)
				enemy = Math.Max(enemy, t.m_iEstimatedEnemyCount);
		}
	}

	static float SpawnerEtaS(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, int prio)
	{
		if (!cmd || !obj || !obj.GetOwner())
			return -1;
		return DCO_CommanderSpawnerComponent.BestEtaS(cmd, obj.GetOwner().GetOrigin(), prio);
	}

	static int Need(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float enemy)
	{
		if (!obj)
			return 0;
		int need = obj.GetDefendGroupCount();
		if (need <= 0)
			need = 1;
		if (FloorOn(cmd))
			need = Math.Max(need, Floor(obj, enemy, false));
		return need;
	}

	static void Report(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, int need, int have, string why, int prio, float threatDist)
	{
		if (!On(cmd) || !obj || !obj.GetOwner())
			return;
		float td;
		float enemy;
		ThreatNear(cmd, obj.GetOwner().GetOrigin(), DCO_Util.NowS(), td, enemy);
		int asked = need;
		need = Need(cmd, obj, enemy);
		if (have <= 0)
		{
			int emptyPrio = 1;
			if (Math.Min(td, threatDist) < 800.0)
				emptyPrio = 2;
			prio = Math.Max(prio, emptyPrio);
		}
		DCO_Perf.Count("def:shortfall_" + why);
		int cap = obj.GetDefendGroupCount();
		int floor;
		if (FloorOn(cmd))
			floor = Floor(obj, enemy, false);
		if (floor > cap && need == floor)
			DCO_Perf.Count("def:floor_raised_need");
		string needText = need.ToString() + " cap=" + cap.ToString() + " floor=" + floor.ToString();
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("def_shortfall cmd=%1 obj=%2 need=%3 have=%4 why=%5 prio=%6 threat_m=%7 asked=%8 enemy=%9", cmd.GetCommanderUID(), obj.GetLogName(), needText, have, why, DCO_ReinfRequest.PrioName(prio), Math.Round(threatDist), asked, Math.Round(enemy)));
		DCO_ReinfBoard b = cmd.DCO_Reinf();
		if (b)
			b.DefenseNeed(cmd, obj, need, have, why, prio, threatDist, DCO_Util.NowS());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_DEF_ShortfallAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "reinf.def_shortfall"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_DEF_GarrisonFloorAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "def.garrison_floor"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_GRP_BcNotFreeAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "grp.bc_not_free"; }
	override protected bool Def() { return true; }
}
