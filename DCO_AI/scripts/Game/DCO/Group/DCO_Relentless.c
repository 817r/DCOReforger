class DCO_Relentless
{
	static bool Group(AIGroup grp)
	{
		SCR_AIGroup g = SCR_AIGroup.Cast(grp);
		if (!g)
			return false;
		SCR_AIGroupUtilityComponent gu = SCR_AIGroupUtilityComponent.Cast(g.FindComponent(SCR_AIGroupUtilityComponent));
		if (!gu)
			return false;
		if (gu.DCO_PostureGm() == DCO_GroupTactics.AGGRESIVE)
			return true;
		if (!gu.m_bDCOCmdRelentless)
			return false;
		DCO_GroupUtilityComponent dg = gu.DCO_GetUtil();
		if (!dg)
			return false;
		int t = dg.GetTask();
		if (t == DCO_EGroupTask.ATTACK || t == DCO_EGroupTask.FLANK)
			return true;
		gu.m_bDCOCmdRelentless = false;
		DCO_Perf.Count("relentless_cmd:group_off");
		return false;
	}

	static bool Unit(SCR_AIUtilityComponent u)
	{
		if (!u || !u.GetAIAgent())
			return false;
		return Group(u.GetAIAgent().GetParentGroup());
	}

	static void Note(AIGroup grp, string why, string detail)
	{
		DCO_Perf.Count("relentless:" + why);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("relentless grp=%1 why=%2 %3", DCO_Log.IdGroup(grp), why, detail));
	}
}
