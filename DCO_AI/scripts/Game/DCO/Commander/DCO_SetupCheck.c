class DCO_SetupCheck
{
	static void Run()
	{
		if (!Replication.IsServer())
			return;
		int warn;
		int info;
		if (!DCO_GlobalAIComponent.GetInstance())
		{
			warn++;
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", "setup_check global=missing (no DCO_GlobalAIComponent on the game mode: DCO settings, config file and GM settings are off)");
		}
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
		{
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", "setup_check commander_manager=none (no AI commanders in this world)");
			if (warn > 0)
				DCO_PlayerComms.SendToGameMasters("DCO setup check", string.Format("%1 setup %2 found. See setup_check lines in dco.log.", warn, DCO_CommanderSpawnerComponent.Plural(warn, "problem", "problems")));
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("setup_check_done warn=%1 info=%2 commanders=0 objectives=0", warn, info));
			return;
		}
		foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
		{
			if (!cmd)
				continue;
			if (cmd.GetCommanderFactionKey().IsEmpty())
			{
				warn++;
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("setup_check cmd=%1 faction=EMPTY (set Faction Key: groups of any faction are refused)", cmd.GetCommanderUID()));
			}
			if (cmd.GetCommanderUID().IsEmpty())
			{
				warn++;
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("setup_check cmd=%1 uid=EMPTY (objectives and spawners find commanders by UID)", cmd.GetOwner().GetName()));
			}
		}
		NavmeshWorldComponent nav;
		AIWorld world = GetGame().GetAIWorld();
		if (world)
			nav = world.GetNavmeshWorldComponent("Soldiers");
		if (!nav)
		{
			warn++;
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", "setup_check navmesh=missing_world (no Soldiers navmesh: AI cannot move to objectives)");
		}
		array<string> later = {};
		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj || !obj.GetOwner())
				continue;
			vector p = obj.GetOwner().GetOrigin();
			if (nav)
			{
				vector got;
				if (!nav.IsTileLoaded(p))
				{
					info++;
					nav.LoadTileIn(p);
					later.Insert(obj.GetLogName());
					DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("setup_check obj=%1 navmesh=not_loaded (tile requested, checked again in 30 s)", obj.GetLogName()));
				}
				else if (!nav.GetReachablePoint(p, 30.0, got))
				{
					warn++;
					DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("setup_check obj=%1 navmesh=none (no navmesh within 30 m: AI cannot reach it)", obj.GetLogName()));
				}
			}
			bool any;
			foreach (AICommander_BaseComponent c : mgr.m_aCommander)
			{
				if (c && !obj.IsCommanderBlackListed(c.GetCommanderUID()))
				{
					any = true;
					break;
				}
			}
			if (!any)
			{
				warn++;
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("setup_check obj=%1 handled_by=nobody (no commander in this world may handle it: check Handled By)", obj.GetLogName()));
			}
		}
		if (!later.IsEmpty())
			GetGame().GetCallqueue().CallLater(RecheckNav, 30000, false, later);
		DCO_Perf.CountN("setup_check:warn", warn);
		if (warn > 0)
			DCO_PlayerComms.SendToGameMasters("DCO setup check", string.Format("%1 setup %2 found. See setup_check lines in dco.log.", warn, DCO_CommanderSpawnerComponent.Plural(warn, "problem", "problems")));
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("setup_check_done warn=%1 info=%2 commanders=%3 objectives=%4", warn, info, mgr.m_aCommander.Count(), mgr.m_aObjective.Count()));
	}

	static void RecheckNav(array<string> names)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		AIWorld world = GetGame().GetAIWorld();
		if (!mgr || !world || !names)
			return;
		NavmeshWorldComponent nav = world.GetNavmeshWorldComponent("Soldiers");
		if (!nav)
			return;
		int bad;
		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj || !obj.GetOwner() || !names.Contains(obj.GetLogName()))
				continue;
			vector p = obj.GetOwner().GetOrigin();
			vector got;
			string result = "ok";
			if (!nav.IsTileLoaded(p))
				result = "still_not_loaded";
			else if (!nav.GetReachablePoint(p, 30.0, got))
				result = "none";
			DCO_ELogLevel lvl = DCO_ELogLevel.INFO;
			if (result != "ok")
			{
				lvl = DCO_ELogLevel.WARN;
				bad++;
				DCO_Perf.Count("setup_check:warn");
			}
			DCO_Log.Write(DCO_ELogChannel.CMD, lvl, "-", string.Format("setup_check obj=%1 navmesh=%2 recheck=1", obj.GetLogName(), result));
		}
		if (bad > 0)
			DCO_PlayerComms.SendToGameMasters("DCO setup check", string.Format("No navmesh at %1 %2. AI cannot move there. The mission world may have no navmesh files on its SCR_AIWorld entity.", bad, DCO_CommanderSpawnerComponent.Plural(bad, "objective", "objectives")));
	}

	static void CommanderNav(AICommander_BaseComponent cmd)
	{
		if (!cmd || !cmd.GetOwner() || !Replication.IsServer() || !GetGame().GetAIWorld())
			return;
		NavmeshWorldComponent nav = GetGame().GetAIWorld().GetNavmeshWorldComponent("Soldiers");
		if (!nav)
			return;
		vector p = cmd.GetOwner().GetOrigin();
		if (nav.IsTileLoaded(p))
			return;
		nav.LoadTileIn(p);
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (g && g.GetOwner() && !nav.IsTileLoaded(g.GetOwner().GetOrigin()))
				nav.LoadTileIn(g.GetOwner().GetOrigin());
		}
		GetGame().GetCallqueue().CallLater(CommanderNavRecheck, 30000, false, cmd);
	}

	static void CommanderNavRecheck(AICommander_BaseComponent cmd)
	{
		if (!cmd || !cmd.GetOwner() || !GetGame().GetAIWorld())
			return;
		NavmeshWorldComponent nav = GetGame().GetAIWorld().GetNavmeshWorldComponent("Soldiers");
		if (!nav)
			return;
		array<vector> pts = {};
		pts.Insert(cmd.GetOwner().GetOrigin());
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (g && g.GetOwner())
				pts.Insert(g.GetOwner().GetOrigin());
		}
		int bad;
		foreach (vector p : pts)
		{
			vector got;
			if (!nav.IsTileLoaded(p) || !nav.GetReachablePoint(p, 30.0, got))
				bad++;
		}
		if (bad < pts.Count())
		{
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("setup_check cmd=%1 navmesh=ok points=%2 missing=%3", cmd.GetCommanderUID(), pts.Count(), bad));
			return;
		}
		DCO_Perf.Count("setup_check:warn");
		DCO_Perf.Count("setup_check:empty_world");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("setup_check cmd=%1 navmesh=empty_world points=%2 (no navmesh at the commander or any of its groups: AI cannot move; the mission world may have no navmesh files on its SCR_AIWorld entity)", cmd.GetCommanderUID(), pts.Count()));
		DCO_PlayerComms.SendToGameMasters("DCO setup check", string.Format("AI Commander %1: no navmesh at the commander or any of its groups. AI cannot move. The mission world may have no navmesh files on its SCR_AIWorld entity.", cmd.GetCommanderUID()));
	}
}

modded class SCR_BaseGameMode
{
	protected override void OnGameStart()
	{
		super.OnGameStart();
		GetGame().GetCallqueue().CallLater(DCO_SetupCheck.Run, 20000, false);
	}
}
