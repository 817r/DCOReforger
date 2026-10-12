enum DCO_ETRPhase
{
	MOVING,
	STAGING,
	EXECUTING,
	HOLDING,
	WITHDRAW
}

enum DCO_ETRSignal
{
	GO,
	LIFT,
	SHIFT,
	COMMIT,
	ABORT,
	REFIX,
	SUSTAIN,
	FALLBACK
}

class DCO_TacticsCompat
{
	static bool VehBailAllowed(IEntity veh, out string why)
	{
		why = "";
		if (!veh)
			return true;
		SCR_VehicleDamageManagerComponent dmg = SCR_VehicleDamageManagerComponent.Cast(veh.FindComponent(SCR_VehicleDamageManagerComponent));
		if (!dmg)
			return false;
		if (dmg.IsDestroyed())
			why = "destroyed";
		else if (dmg.IsOnFire())
			why = "fire";
		else if (dmg.GetHealthScaled() < 0.25)
			why = "hp";
		return !why.IsEmpty();
	}

	static string DbgRole(DCO_CommanderTactics t, DCO_GroupUtilityComponent g)
	{
		if (!t || !g || !g.GetGroupObjective())
			return "reserve";
		DCO_TacticPlan p = t.GetPlan(g.GetGroupObjective());
		if (!p)
			return "reserve";
		int i = p.m_aGroups.Find(g);
		if (i < 0 || i >= p.m_aRoles.Count())
			return "reserve";
		string r = DCO_CommanderTactics.RoleName(p.m_aRoles[i]);
		r.ToLower();
		return r;
	}

	static bool OwnsGroup(DCO_CommanderTactics t, DCO_GroupUtilityComponent g)
	{
		if (!t || !g || !g.GetGroupObjective())
			return false;
		DCO_TacticPlan p = t.GetPlan(g.GetGroupObjective());
		if (!p)
			return false;
		if (p.m_aGroups.Contains(g))
			return true;
		DCO_EGroupTask k = g.GetTask();
		return k == DCO_EGroupTask.ATTACK || k == DCO_EGroupTask.FLANK || k == DCO_EGroupTask.SUPPORT_BY_FIRE;
	}

	static int DropFromPlans(DCO_CommanderTactics t, DCO_GroupUtilityComponent g)
	{
		if (!t || !g || !g.GetGroupObjective())
			return 0;
		DCO_TacticPlan p = t.GetPlan(g.GetGroupObjective());
		if (!p)
			return 0;
		int gi = p.m_aGroups.Find(g);
		if (gi < 0)
			return 0;
		p.m_aGroups.RemoveOrdered(gi);
		if (gi < p.m_aRoles.Count())
			p.m_aRoles.RemoveOrdered(gi);
		if (gi < p.m_aStartUnits.Count())
			p.m_aStartUnits.RemoveOrdered(gi);
		if (gi < p.m_aGoals.Count())
			p.m_aGoals.RemoveOrdered(gi);
		DCO_Perf.Count("tactic_compat:drop");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("tactic_compat_drop grp=%1 obj=%2 left=%3", g.GetOwner().GetName(), g.GetGroupObjective().GetLogName(), p.m_aGroups.Count()));
		return 1;
	}
}
