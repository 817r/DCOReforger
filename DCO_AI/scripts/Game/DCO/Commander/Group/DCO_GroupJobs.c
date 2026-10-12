enum DCO_EGroupJob
{
	ATTACK		= 1,
	FLANK		= 2,
	SBF			= 4,
	RECON		= 8,
	DEFEND		= 16,
	GARRISON	= 32,
	PATROL		= 64,
	RESERVE		= 128,
	TRANSPORT	= 256
}

class DCO_GroupJobs
{
	protected static ref map<string, float> s_mLogAt;
	protected static ref map<AICommander_BaseComponent, float> s_mStarveAt;
	protected static ref map<DCO_GroupUtilityComponent, int> s_mReadMask;

	static void NoteRead(DCO_GroupUtilityComponent g)
	{
		if (!g)
			return;
		if (!s_mReadMask)
			s_mReadMask = new map<DCO_GroupUtilityComponent, int>();
		if (s_mReadMask.Count() > 200)
			s_mReadMask.Clear();
		s_mReadMask.Set(g, g.GetAllowedJobs());
	}

	static int ReadMask(DCO_GroupUtilityComponent g)
	{
		int m;
		if (!g || !s_mReadMask || !s_mReadMask.Find(g, m))
			return -1;
		return m;
	}

	static int AnyCombat()
	{
		return 31;
	}

	static int All()
	{
		return 511;
	}

	static int OfTask(DCO_EGroupTask t)
	{
		switch (t)
		{
			case DCO_EGroupTask.ATTACK:				return 1;
			case DCO_EGroupTask.FLANK:				return 2;
			case DCO_EGroupTask.SUPPORT_BY_FIRE:	return 4;
			case DCO_EGroupTask.RECON:				return 8;
			case DCO_EGroupTask.DEFEND:				return 16;
			case DCO_EGroupTask.GARRISON:			return 32;
			case DCO_EGroupTask.PATROL:				return 64;
			case DCO_EGroupTask.REINFORCE:			return 145;
			case DCO_EGroupTask.TRANSPORT:			return 256;
		}
		return 0;
	}

	static bool Allows(DCO_GroupUtilityComponent g, int mask)
	{
		if (!g || mask == 0)
			return true;
		return (EffMask(g) & mask) != 0;
	}

	static bool TransportInvalid(DCO_GroupUtilityComponent g)
	{
		if (!g)
			return false;
		int mask = g.GetAllowedJobs();
		if ((mask & 256) == 0 || (mask & 31) != 0)
			return false;
		return !g.IsDedicatedTransport() && !g.DCO_TransportPending();
	}

	static int EffMask(DCO_GroupUtilityComponent g)
	{
		int mask = g.GetAllowedJobs();
		if ((mask & 256) == 0 || (mask & 31) != 0 || !TransportInvalid(g))
			return mask;
		if (!s_mLogAt)
			s_mLogAt = new map<string, float>();
		float now = DCO_Util.NowS();
		string key = DCO_Log.IdGroup(SCR_AIGroup.Cast(g.GetOwner())) + "|invalid";
		float last;
		if (!s_mLogAt.Find(key, last) || now - last >= 300.0)
		{
			s_mLogAt.Set(key, now);
			DCO_Perf.Count("grpjob:invalid_no_vehicle");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("grp_flags_invalid grp=%1 reason=not_transport_team mask=%2 used=%3 (Transport only but not a transport team: treated as any job)", DCO_Log.IdGroup(SCR_AIGroup.Cast(g.GetOwner())), mask, 511));
		}
		return 511;
	}

	static bool Gate(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent g, int mask, string path)
	{
		if (Allows(g, mask))
			return true;
		DCO_Perf.Count("grpjob:block_" + path);
		DCO_Perf.Count("grpjob:block");
		if (!s_mLogAt)
			s_mLogAt = new map<string, float>();
		float now = DCO_Util.NowS();
		string key = DCO_Log.IdGroup(SCR_AIGroup.Cast(g.GetOwner())) + "|" + path;
		float last;
		if (!s_mLogAt.Find(key, last) || now - last >= 60.0)
		{
			s_mLogAt.Set(key, now);
			if (s_mLogAt.Count() > 400)
				s_mLogAt.Clear();
			string uid = "-";
			if (cmd)
				uid = cmd.GetCommanderUID();
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("cmd_flag_block cmd=%1 grp=%2 job=%3 path=%4 allowed=%5 preset=%6", uid, DCO_Log.IdGroup(SCR_AIGroup.Cast(g.GetOwner())), mask, path, g.GetAllowedJobs(), PresetName(g.GetAllowedJobs())));
		}
		return false;
	}

	static bool TeamOk(DCO_TransportTeamComponent t)
	{
		if (!t || !t.GetOwner())
			return true;
		DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(t.GetOwner().FindComponent(DCO_GroupUtilityComponent));
		return Gate(null, g, 256, "transport_team");
	}

	static int PresetMask(int preset)
	{
		switch (preset)
		{
			case 0: return 511;
			case 1: return 7;
			case 2: return 8;
			case 3: return 240;
			case 4: return 256;
			case 5: return 128;
		}
		return -1;
	}

	static string PresetLabel(int preset)
	{
		switch (preset)
		{
			case 0: return "Any Task";
			case 1: return "Attack Only";
			case 2: return "Recon Only";
			case 3: return "Defend Only";
			case 4: return "Transport Only";
			case 5: return "Reserve Only";
		}
		return "Custom";
	}

	static int NearestPreset(int mask, out int changed)
	{
		int best;
		changed = 99;
		for (int i = 0; i < 6; i++)
		{
			int x = (mask ^ PresetMask(i)) & 511;
			int n;
			while (x != 0)
			{
				n += x & 1;
				x = x >> 1;
			}
			if (n < changed)
			{
				changed = n;
				best = i;
			}
		}
		return best;
	}

	static string CustomText(int mask)
	{
		if ((mask & 511) == 0)
			return "Custom · no tasks";
		int changed;
		int p = NearestPreset(mask, changed);
		return string.Format("Custom · %1 changed from %2", changed, PresetLabel(p));
	}

	static void EmergencyStarved(AICommander_BaseComponent cmd, int jobMask, string path)
	{
		if (!cmd)
			return;
		int eligible;
		int blocked;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g.IsPlayerGroup() || g.IsDedicatedTransport() || g.IsMortar() || !g.IsAvailableReserve())
				continue;
			eligible++;
			if (!Allows(g, jobMask))
				blocked++;
		}
		if (blocked == 0 || blocked < eligible)
			return;
		if (!s_mLogAt)
			s_mLogAt = new map<string, float>();
		float now = DCO_Util.NowS();
		string key = "emergency|" + cmd.GetCommanderUID();
		float last;
		if (s_mLogAt.Find(key, last) && now - last < 120.0)
			return;
		s_mLogAt.Set(key, now);
		DCO_Perf.Count("grpjob:emergency_starved");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("cmd_flag_block reason=emergency cmd=%1 path=%2 job=%3 eligible=%4 blocked=%5 (flags stay binding, no override)", cmd.GetCommanderUID(), path, jobMask, eligible, blocked));
		if (Replication.IsServer())
			DCO_PlayerComms.SendToGameMasters("DCO", string.Format("%1: emergency at its own objective, but all %2 free %3 blocked by Allowed Tasks. Allow that task on one of them (Allowed Tasks).", cmd.GetCommanderUID(), eligible, DCO_CommanderSpawnerComponent.Plural(eligible, "squad is", "squads are")));
	}

	static int PresetOf(int mask)
	{
		for (int i = 0; i < 6; i++)
		{
			if (PresetMask(i) == mask)
				return i;
		}
		return 6;
	}

	static string PresetName(int mask)
	{
		switch (PresetOf(mask))
		{
			case 0: return "any";
			case 1: return "attack_only";
			case 2: return "recon_only";
			case 3: return "defend_only";
			case 4: return "transport_only";
			case 5: return "reserve_only";
		}
		return "custom";
	}

	static void LogSet(DCO_GroupUtilityComponent g, int playerID)
	{
		if (!g || !g.GetOwner())
			return;
		int mask = g.GetAllowedJobs();
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("grp_flags_set grp=%1 preset=%2 mask=%3 first=%4 empty=%5 player=%6", DCO_Log.IdGroup(SCR_AIGroup.Cast(g.GetOwner())), PresetName(mask), mask, DCO_Util.B01(g.IsFirstToFight()), DCO_Util.B01(mask == 0), playerID));
		if (mask == 0)
		{
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("grp_flags_empty grp=%1 msg=gets_no_commander_jobs", DCO_Log.IdGroup(SCR_AIGroup.Cast(g.GetOwner()))));
			if (Replication.IsServer())
				DCO_PlayerComms.SendToGameMasters("DCO", string.Format("%1 gets no commander jobs.", DCO_PlayerComms.GetCallsign(SCR_AIGroup.Cast(g.GetOwner()))));
		}
		else if (TransportInvalid(g))
		{
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("grp_flags_invalid grp=%1 reason=not_transport_team mask=%2 used=511 player=%3", DCO_Log.IdGroup(SCR_AIGroup.Cast(g.GetOwner())), mask, playerID));
			if (Replication.IsServer())
				DCO_PlayerComms.SendToGameMasters("DCO", string.Format("%1 is Transport only but is not a transport team (that needs an unarmed vehicle with free seats). The commander treats it as Any Task.", DCO_PlayerComms.GetCallsign(SCR_AIGroup.Cast(g.GetOwner()))));
		}
	}

	static void CheckStarve(AICommander_BaseComponent cmd)
	{
		if (!cmd)
			return;
		if (!s_mStarveAt)
			s_mStarveAt = new map<AICommander_BaseComponent, float>();
		float now = DCO_Util.NowS();
		float next;
		if (s_mStarveAt.Find(cmd, next) && now < next)
			return;
		s_mStarveAt.Set(cmd, now + 60.0);
		int eligible;
		int attackOk;
		int flagged;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g.IsPlayerGroup() || g.IsDedicatedTransport() || g.IsMortar())
				continue;
			eligible++;
			if (Allows(g, 3))
				attackOk++;
			if (g.GetAllowedJobs() != 511)
				flagged++;
		}
		if (flagged == 0 || eligible < 2 || attackOk >= 2)
			return;
		s_mStarveAt.Set(cmd, now + 300.0);
		DCO_Perf.Count("grpjob:starved");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("cmd_flags_starved cmd=%1 job=attack eligible=%2 attack_ok=%3 flagged=%4", cmd.GetCommanderUID(), eligible, attackOk, flagged));
		if (Replication.IsServer())
			DCO_PlayerComms.SendToGameMasters("DCO", string.Format("%1 has only %2 %3 that may attack; %4 of %5 squads are limited by Allowed Tasks. Allow Attack on more squads, or set a squad's Task Preset to Any Task.", cmd.GetCommanderUID(), attackOk, DCO_CommanderSpawnerComponent.Plural(attackOk, "squad", "squads"), flagged, eligible));
	}
}
