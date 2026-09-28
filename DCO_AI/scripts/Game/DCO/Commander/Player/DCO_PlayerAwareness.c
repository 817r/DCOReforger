class DCO_PlayerAwareness
{
	protected static const float TICK_S = 5;
	protected static float s_fTimer;

	static const int GROUP_STRIDE = 7;
	static const int OBJ_STRIDE = 4;
	static const int FLAG_HAS_DEST = 1;
	static const int FLAG_CONTACT = 2;
	static const int FLAG_ARMOR = 4;
	static const int FLAG_MOUNTED = 8;
	static const int REL_NEUTRAL = 0;
	static const int REL_FRIENDLY = 1;
	static const int REL_ENEMY = 2;
	static const int REL_CONTESTED = 3;
	static const int REL_STAGING = 4;

	static void Tick(float timeSlice)
	{
		s_fTimer += timeSlice;
		if (s_fTimer < TICK_S)
			return;
		s_fTimer = 0;

		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!cfg || !mgr || cfg.GetOverlayMode() <= 0)
			return;

		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int pid : players)
			SendSnapshot(pid, mgr, cfg);
	}

	protected static void SendSnapshot(int pid, AICommander_ManagerComponent mgr, DCO_GlobalAIComponent cfg)
	{
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(pid);
		SCR_PlayerControllerGroupComponent pcg = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(pid);
		if (!player || !pcg)
			return;

		Faction faction = SCR_FactionManager.SGetPlayerFaction(pid);
		if (!faction)
			return;
		FactionKey fk = faction.GetFactionKey();

		vector ppos = player.GetOrigin();
		float radiusSq = cfg.GetOverlayRadius() * cfg.GetOverlayRadius();
		bool all = cfg.GetOverlayMode() >= 2;

		array<float> groups = {};
		array<float> objectives = {};
		array<CMD_AICommanderObjectiveComponent> relevantObjs = {};
		int myGroup = pcg.GetGroupID();

		CMD_AICommanderObjectiveComponent myObj;
		foreach (AICommander_BaseComponent c : mgr.m_aCommander)
		{
			if (c && c.GetCommanderFactionKey() == fk && !myObj)
				myObj = c.GetPlayerTasking().GetGroupObjective(myGroup);
		}
		if (myObj)
			relevantObjs.Insert(myObj);

		foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
		{
			if (!cmd || cmd.GetCommanderFactionKey() != fk)
				continue;

			foreach (DCO_GroupUtilityComponent grp : cmd.GetOwnedGroups())
			{
				if (!grp || !grp.GetOwner() || grp.IsPlayerGroup())
					continue;

				vector gpos = grp.GetOwner().GetOrigin();
				CMD_AICommanderObjectiveComponent gobj = grp.GetGroupObjective();
				if (!all && (myGroup < 0 || grp.GetSupportPlayerGroup() != myGroup) && (!myObj || gobj != myObj))
					continue;

				int flags;
				vector dest = grp.GetTaskData().m_vTarget;
				if (gobj)
					dest = gobj.GetOwner().GetOrigin();
				if (grp.IsMoving() || gobj)
					flags |= FLAG_HAS_DEST;
				int st = grp.GetState();
				if (st & DCO_EGroupState.IN_CONTACT)
					flags |= FLAG_CONTACT;
				if (st & DCO_EGroupState.MOUNTED)
					flags |= FLAG_MOUNTED;
				if (grp.IsArmor())
					flags |= FLAG_ARMOR;

				groups.Insert(gpos[0]);
				groups.Insert(gpos[2]);
				groups.Insert(grp.GetTask());
				groups.Insert(dest[0]);
				groups.Insert(dest[2]);
				groups.Insert(grp.GetUnitCount());
				groups.Insert(flags);

				if (gobj && !relevantObjs.Contains(gobj))
					relevantObjs.Insert(gobj);
			}

			foreach (CMD_AICommanderObjectiveComponent o : relevantObjs)
			{
				vector sp;
				if (cmd.GetStagingPos(o, sp) && !cmd.IsAssaultReleased(o))
				{
					objectives.Insert(sp[0]);
					objectives.Insert(sp[2]);
					objectives.Insert(15);
					objectives.Insert(REL_STAGING);
				}
			}
		}

		foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
		{
			if (!o || !o.GetOwner())
				continue;
			vector op = o.GetOwner().GetOrigin();
			if (!all && vector.DistanceSqXZ(op, ppos) > radiusSq * 4 && !relevantObjs.Contains(o))
				continue;

			int rel = REL_NEUTRAL;
			if (o.IsCapturedBy(fk))
				rel = REL_FRIENDLY;
			else if (o.GetOwnerFaction() != "")
				rel = REL_ENEMY;
			if (IsContested(o, mgr, fk))
				rel = REL_CONTESTED;

			objectives.Insert(op[0]);
			objectives.Insert(op[2]);
			objectives.Insert(o.GetRadius());
			objectives.Insert(rel);
		}

		pcg.DCO_SendOverlay(groups, objectives);
	}

	protected static bool IsContested(CMD_AICommanderObjectiveComponent o, AICommander_ManagerComponent mgr, FactionKey fk)
	{
		foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
		{
			if (cmd && cmd.GetCommanderFactionKey() == fk)
				return o.IsCapturedBy(fk) && cmd.IsObjectiveContested(o);
		}
		return false;
	}

	static void BroadcastOperation(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, string key, array<string> params)
	{
		if (!cmd || !obj || !Replication.IsServer())
			return;

		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		float radius = 800;
		if (cfg)
			radius = cfg.GetRadioRadius();

		array<int> ids = {};
		cmd.GetPlayerTasking().GetGroupsOnObjective(obj, ids);
		CollectPlayerGroupsNear(cmd.GetCommanderFactionKey(), obj.GetOwner().GetOrigin(), radius, ids);

		foreach (int id : ids)
			DCO_Radio.Group(id, "COMMANDER", key, DCO_ERadioKind.INFO, params);
	}

	protected static const float IDF_CELL_M = 200;
	protected static const float IDF_COOLDOWN_S = 60;
	protected static const float HELD_COOLDOWN_S = 60;
	protected static ref map<string, float> s_mIdfCd = new map<string, float>();
	protected static ref map<int, float> s_mHeldCd = new map<int, float>();

	protected static float WarnRadius()
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (cfg)
			return cfg.GetArtyWarnRadius();
		return 400;
	}

	static void WarnArtillery(AICommander_BaseComponent cmd, CMD_FireMissionRequest req, float tof)
	{
		if (!cmd || !req || !Replication.IsServer())
			return;

		array<int> ids = {};
		CollectPlayerGroupsNear(cmd.GetCommanderFactionKey(), req.m_vImpactPos, WarnRadius(), ids);
		ids.RemoveItem(req.m_iPlayerGroup);
		if (ids.IsEmpty())
			return;

		string grid = DCO_PlayerComms.Grid(req.m_vImpactPos);
		if (req.m_eShellType != SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE)
		{
			string key = "arty_friendly_smoke";
			if (req.m_eShellType == SCR_EAIArtilleryAmmoType.ILLUMINATION)
				key = "arty_friendly_illum";
			foreach (int sid : ids)
				DCO_Radio.Group(sid, "ARTILLERY", key, DCO_ERadioKind.INFO, DCO_Radio.P("grid", grid));
			return;
		}

		array<string> params = DCO_Radio.P("grid", grid, "count", req.m_iShellCount.ToString(), "dist", DCO_Radio.N(Math.Max(req.m_fSafeRadius, 100)), "sec", DCO_Radio.N(tof));
		foreach (int id : ids)
		{
			DCO_Radio.Group(id, "ARTILLERY", "arty_inbound", DCO_ERadioKind.WARNING, params, true);
			GetGame().GetCallqueue().CallLater(SplashGroup, tof * 1000, false, id);
		}
	}

	static void SplashGroup(int groupID)
	{
		DCO_Radio.Group(groupID, "ARTILLERY", "fire_splash", DCO_ERadioKind.GO);
	}

	static void WarnIncoming(AICommander_BaseComponent shooterCmd, vector firePos, vector impact)
	{
		FactionManager fm = GetGame().GetFactionManager();
		if (!shooterCmd || !fm || !Replication.IsServer())
			return;

		Faction sf = fm.GetFactionByKey(shooterCmd.GetCommanderFactionKey());
		if (!sf)
			return;

		array<Faction> factions = {};
		fm.GetFactionsList(factions);
		array<int> ids = {};
		float radius = WarnRadius();
		foreach (Faction f : factions)
		{
			if (!f || f == sf || f.IsFactionFriendly(sf))
				continue;
			CollectPlayerGroupsNear(f.GetFactionKey(), impact, radius, ids);
		}
		if (ids.IsEmpty())
			return;

		float now = GetGame().GetWorld().GetWorldTime() / 1000.0;
		float tof = CMD_ArtillerySupport.TimeOfFlight(firePos, impact);
		string grid = DCO_PlayerComms.Grid(impact);
		string cell = string.Format("%1_%2", Math.Round(impact[0] / IDF_CELL_M), Math.Round(impact[2] / IDF_CELL_M));
		foreach (int id : ids)
		{
			string memo = id.ToString() + "_" + cell;
			float until;
			if (s_mIdfCd.Find(memo, until) && now < until)
				continue;
			s_mIdfCd.Set(memo, now + IDF_COOLDOWN_S);
			float delay = tof + Math.RandomFloat(3, 8);
			GetGame().GetCallqueue().CallLater(IncomingGroup, delay * 1000, false, id, grid);
		}
	}

	static void IncomingGroup(int groupID, string grid)
	{
		DCO_Radio.Group(groupID, "WARNING", "idf_incoming", DCO_ERadioKind.WARNING, DCO_Radio.P("grid", grid), true);
	}

	static void NotifyHeld(AICommander_BaseComponent cmd, CMD_FireMissionRequest req, float radius)
	{
		if (!cmd || !req || !Replication.IsServer())
			return;

		array<int> ids = {};
		CollectPlayerGroupsNear(cmd.GetCommanderFactionKey(), req.m_vImpactPos, radius, ids);
		ids.RemoveItem(req.m_iPlayerGroup);
		if (ids.IsEmpty())
			return;

		float now = GetGame().GetWorld().GetWorldTime() / 1000.0;
		string grid = DCO_PlayerComms.Grid(req.m_vImpactPos);
		foreach (int id : ids)
		{
			float until;
			if (s_mHeldCd.Find(id, until) && now < until)
				continue;
			s_mHeldCd.Set(id, now + HELD_COOLDOWN_S);
			DCO_Radio.Group(id, "ARTILLERY", "fire_held", DCO_ERadioKind.INFO, DCO_Radio.P("grid", grid));
		}
	}

	static void CollectPlayerGroupsNear(FactionKey fk, vector pos, float radius, notnull array<int> ids)
	{
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		FactionManager fm = GetGame().GetFactionManager();
		if (!groups || !fm)
			return;

		array<SCR_AIGroup> playable = groups.GetPlayableGroupsByFaction(fm.GetFactionByKey(fk));
		if (!playable)
			return;

		float radiusSq = radius * radius;
		foreach (SCR_AIGroup grp : playable)
		{
			if (!grp || ids.Contains(grp.GetGroupID()))
				continue;

			array<int> pids = grp.GetPlayerIDs();
			if (!pids)
				continue;

			foreach (int pid : pids)
			{
				IEntity p = GetGame().GetPlayerManager().GetPlayerControlledEntity(pid);
				if (p && vector.DistanceSqXZ(p.GetOrigin(), pos) <= radiusSq)
				{
					ids.Insert(grp.GetGroupID());
					break;
				}
			}
		}
	}
}
