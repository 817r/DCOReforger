enum DCO_ELogLevel
{
	OFF,
	ERROR,
	WARN,
	INFO,
	DEBUG,
	TRACE
}

enum DCO_ELogChannel
{
	SYS,
	CMD,
	TACTIC,
	SQUAD,
	UNIT,
	COMBAT,
	CQB,
	ARTY,
	LOGI,
	MEDIC,
	VEH,
	PLAYER,
	GM,
	PERF,
	CENSUS,
	TEST
}

enum DCO_ELogMode
{
	OFF,
	REPORT,
	FULL
}

class DCO_LogTimer : Managed
{
	void Tick()
	{
		DCO_Log.Flush();
	}
}

class DCO_Log
{
	protected static ref array<int> s_aLevel;
	protected static ref array<int> s_aRate;
	protected static ref array<int> s_aDropped;
	protected static int s_iRateSec;
	protected static int s_iRateCap;

	protected static string s_sDir;
	protected static int s_iPart;
	protected static int s_iBytes;
	protected static int s_iStartTick;
	protected static int s_iWallBaseMs;
	protected static float s_fLastWorldMs;
	protected static bool s_bClosing;

	protected static ref array<string> s_aMain;
	protected static ref array<string> s_aPerf;
	protected static ref array<string> s_aCensus;
	protected static ref DCO_LogTimer s_Timer;

	protected static ref map<IEntity, string> s_mIds;
	protected static int s_iNextC;
	protected static int s_iNextG;
	protected static int s_iNextU;
	protected static int s_iNextO;
	protected static int s_iNextV;

	static int Mode()
	{
		if (DCO_TestCaseComponent.RunOn())
			return DCO_ELogMode.FULL;
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetInstance();
		if (!g)
			return DCO_ELogMode.REPORT;
		return g.GetLogMode();
	}

	static bool On(DCO_ELogChannel ch, DCO_ELogLevel lvl)
	{
		if (!s_aLevel)
			InitLevels();
		int mode = Mode();
		if (mode != DCO_ELogMode.FULL)
		{
			if (lvl == DCO_ELogLevel.ERROR)
				return true;
			return mode == DCO_ELogMode.REPORT && lvl == DCO_ELogLevel.WARN;
		}
		return lvl <= s_aLevel[ch];
	}

	static void SetLevel(DCO_ELogChannel ch, DCO_ELogLevel lvl)
	{
		if (!s_aLevel)
			InitLevels();
		s_aLevel[ch] = lvl;
	}

	static DCO_ELogLevel GetLevel(DCO_ELogChannel ch)
	{
		if (!s_aLevel)
			InitLevels();
		return s_aLevel[ch];
	}

	static void SetRateCap(int linesPerSecond)
	{
		s_iRateCap = Math.Max(linesPerSecond, 10);
	}

	static string GetSessionDir()
	{
		return s_sDir;
	}

	static void Write(DCO_ELogChannel ch, DCO_ELogLevel lvl, string subject, string text)
	{
		if (DCO_CmdViewStore.s_bOn)
			DCO_CmdViewStore.Capture(text);
		if (!On(ch, lvl))
			return;
		int mode = Mode();
		if (mode != DCO_ELogMode.FULL)
		{
			if (mode == DCO_ELogMode.REPORT)
				DCO_Report.Warn(ch, lvl, text);
			if (lvl == DCO_ELogLevel.ERROR)
				Print("[DCO] " + typename.EnumToString(DCO_ELogChannel, ch) + " " + text, LogLevel.ERROR);
			return;
		}
		if (lvl == DCO_ELogLevel.ERROR || lvl == DCO_ELogLevel.WARN)
			DCO_Report.Warn(ch, lvl, text);
		DCO_TestCaseComponent.OnEvent(text);
		DCO_Battle.Capture(text);
		if (subject.IsEmpty())
			subject = "-";
		Emit(ch, lvl, subject + " " + text, false);
	}

	static void Legacy(string text)
	{
		DCO_ELogChannel ch = DCO_ELogChannel.SYS;
		int sp = text.IndexOf(" ");
		if (sp > 1 && sp <= 6)
		{
			string tok = text.Substring(0, sp);
			int v = typename.StringToEnum(DCO_ELogChannel, tok);
			if (v >= 0 && typename.EnumToString(DCO_ELogChannel, v) == tok)
			{
				if (On(v, DCO_ELogLevel.INFO))
					Emit(v, DCO_ELogLevel.INFO, text.Substring(sp + 1, text.Length() - sp - 1), false);
				return;
			}
		}
		if (On(ch, DCO_ELogLevel.INFO))
			Emit(ch, DCO_ELogLevel.INFO, "- " + text, false);
	}

	static void Perf(string line, string fileLine)
	{
		if (Mode() != DCO_ELogMode.FULL || !Ready())
			return;
		s_aPerf.Insert(fileLine);
		if (On(DCO_ELogChannel.PERF, DCO_ELogLevel.INFO))
			Emit(DCO_ELogChannel.PERF, DCO_ELogLevel.INFO, "- " + line, false);
	}

	static void Census(string line)
	{
		if (Mode() != DCO_ELogMode.FULL || !Ready())
			return;
		s_aCensus.Insert(line);
		if (!On(DCO_ELogChannel.CENSUS, DCO_ELogLevel.INFO))
			return;
		string body = line;
		if (body.StartsWith("CENSUS "))
			body = body.Substring(7, body.Length() - 7);
		Emit(DCO_ELogChannel.CENSUS, DCO_ELogLevel.INFO, "- " + body, false);
	}

	static void Start()
	{
		if (!Replication.IsServer())
		{
			if (!s_aLevel)
				InitLevels();
			for (int i = 0; i < s_aLevel.Count(); i++)
			{
				s_aLevel[i] = DCO_ELogLevel.OFF;
			}
			return;
		}
		if (!Ready())
			return;
		if (Mode() != DCO_ELogMode.FULL)
		{
			DCO_Report.Write(true);
			return;
		}
		string stamp = ConfigStamp();
		AppendFile(s_sDir + "/session.txt", "config " + stamp);
		Emit(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "- config " + stamp, true);
		Flush();
	}

	static void Stop(string reason)
	{
		if (s_sDir.IsEmpty() || s_bClosing)
			return;
		Emit(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "- session_stop reason=" + reason, true);
		s_bClosing = true;
		Flush();
	}

	static void Flush()
	{
		if (s_sDir.IsEmpty())
			return;
		RollRate(System.GetTickCount());
		if (s_iBytes >= 104857600)
		{
			s_iPart++;
			s_iBytes = 0;
		}
		string main = s_sDir + "/dco.log";
		if (s_iPart > 1)
			main = s_sDir + "/dco_" + s_iPart.ToString(3) + ".log";
		s_iBytes += WriteLines(main, s_aMain);
		WriteLines(s_sDir + "/perf.log", s_aPerf);
		WriteLines(s_sDir + "/census.log", s_aCensus);
		DCO_Report.Write(s_bClosing);
	}

	static string Pos(vector p)
	{
		return string.Format("%1,%2", Math.Round(p[0]), Math.Round(p[2]));
	}

	static string IdUnit(IEntity ent)
	{
		if (!ent)
			return "-";
		string id = Cached(ent);
		if (!id.IsEmpty())
			return id;
		PlayerManager pm = GetGame().GetPlayerManager();
		if (pm)
		{
			int pid = pm.GetPlayerIdFromControlledEntity(ent);
			if (pid > 0)
				return Assign(ent, "P" + pid, "name=" + pm.GetPlayerName(pid));
		}
		string grp = "-";
		AIControlComponent ctrl = AIControlComponent.Cast(ent.FindComponent(AIControlComponent));
		if (ctrl && ctrl.GetAIAgent())
			grp = IdGroup(ctrl.GetAIAgent().GetParentGroup());
		s_iNextU++;
		return Assign(ent, "U" + s_iNextU, "name=" + PrefabOf(ent) + " grp=" + grp + " faction=" + NoneIfEmpty(DCO_Util.FactionKeyOf(ent, true)));
	}

	static string IdGroup(AIGroup g)
	{
		if (!g)
			return "-";
		string id = Cached(g);
		if (!id.IsEmpty())
			return id;
		string fk = "none";
		SCR_AIGroup sg = SCR_AIGroup.Cast(g);
		if (sg && sg.GetFaction())
			fk = sg.GetFaction().GetFactionKey();
		s_iNextG++;
		return Assign(g, "G" + s_iNextG, "name=" + NoneIfEmpty(g.GetName()) + " faction=" + fk);
	}

	static string IdCmd(AICommander_BaseComponent c)
	{
		if (!c)
			return "-";
		string id = Cached(c.GetOwner());
		if (!id.IsEmpty())
			return id;
		s_iNextC++;
		return Assign(c.GetOwner(), "C" + s_iNextC, "name=" + NoneIfEmpty(c.GetCommanderUID()) + " faction=" + NoneIfEmpty(c.GetCommanderFactionKey()));
	}

	static string IdObj(CMD_AICommanderObjectiveComponent o)
	{
		if (!o)
			return "-";
		string id = Cached(o.GetOwner());
		if (!id.IsEmpty())
			return id;
		s_iNextO++;
		return Assign(o.GetOwner(), "O" + s_iNextO, "name=" + NoneIfEmpty(o.GetLogName()));
	}

	static string IdVeh(IEntity v)
	{
		if (!v)
			return "-";
		string id = Cached(v);
		if (!id.IsEmpty())
			return id;
		s_iNextV++;
		return Assign(v, "V" + s_iNextV, "name=" + PrefabOf(v) + " faction=" + NoneIfEmpty(DCO_Util.FactionKeyOf(v, true)));
	}

	static string ConfigStamp()
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetInstance();
		if (!g)
			return "global=none";
		int mask = g.GetBehaviorMask();
		string off;
		int bit = 1;
		for (int i = 0; i < 31; i++)
		{
			if ((mask & bit) == 0)
			{
				if (!off.IsEmpty())
					off += ",";
				off += typename.EnumToString(DCO_EBehavior, bit);
			}
			bit = bit * 2;
		}
		string presets;
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
		{
			foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
			{
				if (!cmd)
					continue;
				if (!presets.IsEmpty())
					presets += ",";
				presets += cmd.GetCommanderUID() + "|" + NoneIfEmpty(cmd.DCO_GetActivePreset());
			}
		}
		return string.Format("mask=%1 off=%2 lodMode=%3 takeCover=%4 dodge=%5 dodgeDist=%6 dodgeCd=%7 grenadeUsage=%8 smokeUsage=%9",
			mask, NoneIfEmpty(off), typename.EnumToString(DCO_EAILODMode, g.GetLODMode()), g.GetTakeCoverChance(), g.GetDodgeChance(), g.GetDodgeMaxDist(), g.GetDodgeCooldown(), g.GetGrenadeUsage(), g.GetSmokeUsage())
			+ string.Format(" investigateMaxDist=%1 superiorRatio=%2 overmatchAssault=%3 overmatchMaxDist=%4 presets=%5", g.GetInvestigateMaxDist(), g.GetSuperiorRatio(), g.GetOvermatchAssault(), g.GetOvermatchMaxDist(), NoneIfEmpty(presets));
	}

	protected static void InitLevels()
	{
		s_aLevel = {};
		s_aRate = {};
		s_aDropped = {};
		for (int i = 0; i <= DCO_ELogChannel.TEST; i++)
		{
			s_aLevel.Insert(DCO_ELogLevel.INFO);
			s_aRate.Insert(0);
			s_aDropped.Insert(0);
		}
		if (s_iRateCap <= 0)
			s_iRateCap = 200;
	}

	protected static void Emit(DCO_ELogChannel ch, DCO_ELogLevel lvl, string body, bool force)
	{
		if (Mode() != DCO_ELogMode.FULL || !Ready())
			return;
		int now = System.GetTickCount();
		if (!force && lvl != DCO_ELogLevel.ERROR && !Admit(ch, now))
			return;
		string line = Stamp(now) + " " + LevelChar(lvl) + " " + typename.EnumToString(DCO_ELogChannel, ch) + " " + body;
		s_aMain.Insert(line);
		if (lvl == DCO_ELogLevel.ERROR)
			Print("[DCO] " + line, LogLevel.ERROR);
		if (lvl == DCO_ELogLevel.ERROR || s_bClosing || s_aMain.Count() >= 500)
			Flush();
	}

	protected static bool Ready()
	{
		if (!GetGame() || !GetGame().InPlayMode() || !Replication.IsServer())
			return false;
		if (Mode() == DCO_ELogMode.OFF)
			return false;
		float worldMs = s_fLastWorldMs;
		if (GetGame().GetWorld())
			worldMs = GetGame().GetWorld().GetWorldTime();
		if (!s_sDir.IsEmpty() && worldMs + 1000 < s_fLastWorldMs)
		{
			Flush();
			s_sDir = string.Empty;
		}
		s_fLastWorldMs = worldMs;
		if (s_sDir.IsEmpty())
			BeginSession();
		return !s_sDir.IsEmpty();
	}

	protected static void BeginSession()
	{
		int y, mo, d, h, mi, s;
		System.GetYearMonthDay(y, mo, d);
		System.GetHourMinuteSecond(h, mi, s);
		int block = (h / 6) * 6;
		string date = string.Format("%1-%2-%3", y, mo.ToString(2), d.ToString(2));
		string clock = string.Format("%1%2%3", h.ToString(2), mi.ToString(2), s.ToString(2));
		int blockEnd = block + 6;
		string dir = string.Format("$profile:DCO/Logs/%1/%2-%3/session_%4", date, block.ToString(2), blockEnd.ToString(2), clock);
		int n = 2;
		string candidate = dir;
		while (FileIO.FileExists(candidate + "/session.txt") || FileIO.FileExists(candidate + "/report.txt"))
		{
			candidate = dir + "_" + n;
			n++;
		}
		if (!FileIO.MakeDirectory(candidate) && !FileIO.FileExists(candidate))
			return;

		s_sDir = candidate;
		s_iPart = 1;
		s_iBytes = 0;
		s_bClosing = false;
		s_iStartTick = System.GetTickCount();
		s_iWallBaseMs = ((h * 60 + mi) * 60 + s) * 1000;
		s_iRateSec = s_iStartTick / 1000;
		s_aMain = {};
		s_aPerf = {};
		s_aCensus = {};
		s_mIds = new map<IEntity, string>();
		s_iNextC = 0;
		s_iNextG = 0;
		s_iNextU = 0;
		s_iNextO = 0;
		s_iNextV = 0;
		if (!s_aLevel)
			InitLevels();
		for (int i = 0; i < s_aRate.Count(); i++)
		{
			s_aRate[i] = 0;
			s_aDropped[i] = 0;
		}

		if (!s_Timer)
			s_Timer = new DCO_LogTimer();
		GetGame().GetCallqueue().Remove(s_Timer.Tick);
		GetGame().GetCallqueue().CallLater(s_Timer.Tick, 2000, true);

		array<string> head = {};
		head.Insert("start=" + date + " " + h.ToString(2) + ":" + mi.ToString(2) + ":" + s.ToString(2));
		head.Insert("game=" + GetGame().GetBuildVersion());
		head.Insert("world=" + GetGame().GetWorldFile());
		MissionHeader mh = GetGame().GetMissionHeader();
		if (mh)
			head.Insert("mission=" + mh.GetHeaderResourceName());
		else
			head.Insert("mission=none");
		array<string> addons = {};
		GameProject.GetLoadedAddons(addons);
		foreach (string guid : addons)
		{
			if (!GameProject.IsVanillaAddon(guid))
				head.Insert("mod=" + GameProject.GetAddonTitle(guid) + " id=" + GameProject.GetAddonID(guid) + " guid=" + guid);
		}
		if (Mode() == DCO_ELogMode.FULL)
			WriteLines(s_sDir + "/session.txt", head);

		Emit(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "- session_start dir=" + s_sDir + " world=" + GetGame().GetWorldFile(), true);
	}

	protected static bool Admit(DCO_ELogChannel ch, int now)
	{
		RollRate(now);
		if (s_aRate[ch] >= s_iRateCap)
		{
			s_aDropped[ch] = s_aDropped[ch] + 1;
			return false;
		}
		s_aRate[ch] = s_aRate[ch] + 1;
		return true;
	}

	protected static void RollRate(int now)
	{
		int sec = now / 1000;
		if (sec == s_iRateSec || !s_aRate)
			return;
		s_iRateSec = sec;
		for (int i = 0; i < s_aRate.Count(); i++)
		{
			s_aRate[i] = 0;
			if (s_aDropped[i] > 0)
			{
				s_aMain.Insert(Stamp(now) + " W " + typename.EnumToString(DCO_ELogChannel, i) + " - DROPPED count=" + s_aDropped[i] + " cap=" + s_iRateCap);
				s_aDropped[i] = 0;
			}
		}
	}

	protected static string Stamp(int now)
	{
		int ms = (s_iWallBaseMs + now - s_iStartTick) % 86400000;
		int h = ms / 3600000;
		int m = (ms / 60000) % 60;
		int s = (ms / 1000) % 60;
		int f = ms % 1000;
		float t = (now - s_iStartTick) / 1000.0;
		return h.ToString(2) + ":" + m.ToString(2) + ":" + s.ToString(2) + "." + f.ToString(3) + " T+" + t.ToString(-1, 1);
	}

	protected static string LevelChar(DCO_ELogLevel lvl)
	{
		switch (lvl)
		{
			case DCO_ELogLevel.ERROR: return "E";
			case DCO_ELogLevel.WARN: return "W";
			case DCO_ELogLevel.DEBUG: return "D";
			case DCO_ELogLevel.TRACE: return "T";
		}
		return "I";
	}

	protected static int WriteLines(string path, array<string> lines)
	{
		if (!lines || lines.IsEmpty())
			return 0;
		FileHandle f = FileIO.OpenFile(path, FileMode.APPEND);
		if (!f)
			f = FileIO.OpenFile(path, FileMode.WRITE);
		if (!f)
		{
			if (lines.Count() > 5000)
				lines.Clear();
			return 0;
		}
		int bytes = 0;
		foreach (string l : lines)
		{
			f.WriteLine(l);
			bytes += l.Length() + 2;
		}
		f.Close();
		lines.Clear();
		return bytes;
	}

	protected static void AppendFile(string path, string line)
	{
		array<string> one = {};
		one.Insert(line);
		WriteLines(path, one);
	}

	protected static string Cached(IEntity ent)
	{
		if (!ent || !s_mIds)
			return string.Empty;
		string id;
		s_mIds.Find(ent, id);
		return id;
	}

	protected static string Assign(IEntity ent, string id, string info)
	{
		if (!Ready())
			return id;
		if (s_mIds.Count() > 5000)
		{
			map<IEntity, string> fresh = new map<IEntity, string>();
			foreach (IEntity k, string v : s_mIds)
			{
				if (k)
					fresh.Set(k, v);
			}
			s_mIds = fresh;
		}
		s_mIds.Set(ent, id);
		Emit(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "- NAME id=" + id + " " + info, true);
		return id;
	}

	protected static string PrefabOf(IEntity ent)
	{
		if (!ent.GetPrefabData())
			return "none";
		return FilePath.StripExtension(FilePath.StripPath(ent.GetPrefabData().GetPrefabName()));
	}

	protected static string NoneIfEmpty(string s)
	{
		if (s.IsEmpty())
			return "none";
		return s;
	}
}

modded class SCR_BaseGameMode
{
	protected override void OnGameStart()
	{
		super.OnGameStart();
		DCO_Util.SetTeardown(false);
		DCO_Log.Start();
		DCO_Battle.Start();
		DCO_CmdViewStore.Refresh();
	}

	override void OnGameEnd()
	{
		DCO_Battle.End("world_end");
		DCO_LearnDefense.SaveAll();
		DCO_CmdLearn.SaveAll();
		DCO_GarrisonRegistry.SaveAll();
		DCO_BldLearn.SaveAll();
		DCO_IndLearn.SaveAll();
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", "teardown_guard on=1 reason=world_end");
		DCO_Log.Stop("world_end");
		DCO_Util.SetTeardown(true);
		super.OnGameEnd();
	}
}
