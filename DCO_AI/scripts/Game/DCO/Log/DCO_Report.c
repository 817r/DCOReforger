class DCO_Report
{
	protected static ref map<string, string> s_mSettings;
	protected static ref array<string> s_aSettingOrder;
	protected static ref map<string, string> s_mGm;
	protected static ref array<string> s_aGmOrder;
	protected static ref map<string, string> s_mApi;
	protected static ref array<string> s_aApiOrder;
	protected static ref map<string, int> s_mWarnCount;
	protected static ref map<string, string> s_mWarnSample;
	protected static ref array<string> s_aWarnOrder;
	protected static string s_sPerf;
	protected static string s_sPerfTip;
	protected static float s_fWorstFps;
	protected static int s_iPerfWindows;
	protected static int s_iLastWrite;
	protected static string s_sDir;
	protected static string s_sConfig;

	static void Config(string state)
	{
		s_sConfig = state;
		DCO_Perf.Count("report:config");
	}

	protected static void Init()
	{
		string dir = DCO_Log.GetSessionDir();
		if (dir != s_sDir && !s_sDir.IsEmpty() && !dir.IsEmpty())
		{
			s_sDir = dir;
			s_mSettings = null;
			s_mGm = null;
			s_mApi = null;
			s_mWarnCount = null;
			s_sPerf = string.Empty;
			s_sPerfTip = string.Empty;
			s_fWorstFps = 0;
			s_iPerfWindows = 0;
			s_iLastWrite = 0;
		}
		if (s_sDir.IsEmpty())
			s_sDir = dir;
		if (!s_mSettings)
		{
			s_mSettings = new map<string, string>();
			s_aSettingOrder = {};
			s_mGm = new map<string, string>();
			s_aGmOrder = {};
			s_mApi = new map<string, string>();
			s_aApiOrder = {};
			s_mWarnCount = new map<string, int>();
			s_mWarnSample = new map<string, string>();
			s_aWarnOrder = {};
		}
	}

	static void Setting(string scope, string key, string value, string def)
	{
		Init();
		string k = key;
		if (!scope.IsEmpty())
			k = scope + ": " + key;
		if (!s_mSettings.Contains(k))
			s_aSettingOrder.Insert(k);
		s_mSettings.Set(k, value + "   (Recommended: " + def + ")");
		DCO_Perf.Count("report:setting");
	}

	static void Gm(string key, string value)
	{
		Init();
		if (!s_mGm.Contains(key))
			s_aGmOrder.Insert(key);
		s_mGm.Set(key, value);
		DCO_Perf.Count("report:gm");
	}

	static void Api(string mod, string key, string value)
	{
		Init();
		if (!s_mApi)
		{
			s_mApi = new map<string, string>();
			s_aApiOrder = {};
		}
		string k = mod + ": " + key;
		if (!s_mApi.Contains(k))
			s_aApiOrder.Insert(k);
		s_mApi.Set(k, value);
		DCO_Perf.Count("report:api");
	}

	static void Warn(DCO_ELogChannel ch, DCO_ELogLevel lvl, string text)
	{
		Init();
		string first = text;
		int sp = text.IndexOf(" ");
		if (sp > 0)
			first = text.Substring(0, sp);
		string key = typename.EnumToString(DCO_ELogLevel, lvl) + " " + typename.EnumToString(DCO_ELogChannel, ch) + " " + first;
		int n;
		if (s_mWarnCount.Find(key, n))
		{
			s_mWarnCount.Set(key, n + 1);
			return;
		}
		if (s_aWarnOrder.Count() >= 40)
			return;
		s_aWarnOrder.Insert(key);
		s_mWarnCount.Set(key, 1);
		string sample = text;
		if (sample.Length() > 220)
			sample = sample.Substring(0, 220) + " ...";
		s_mWarnSample.Set(key, sample);
	}

	static void Perf(float fpsAvg, float fpsMin, int frameMaxMs, int ai, int players, int lodFull, int lodReduced, float dcoMsPerFrame, string top)
	{
		Init();
		s_iPerfWindows++;
		if (s_fWorstFps <= 0 || (fpsMin > 0 && fpsMin < s_fWorstFps))
			s_fWorstFps = fpsMin;
		float pct = dcoMsPerFrame * 100.0 / 16.67;
		s_sPerf = string.Format("Last 30 s: server FPS average %1, lowest %2 (longest frame %3 ms). Lowest FPS this session: %4.\n", fpsAvg.ToString(-1, 0), fpsMin.ToString(-1, 0), frameMaxMs, s_fWorstFps.ToString(-1, 0));
		s_sPerf += string.Format("AI: %1 soldiers (%2 at full detail, %3 reduced by distance). Players: %4.\n", ai, lodFull, lodReduced, players);
		s_sPerf += "DCO AI used about " + dcoMsPerFrame.ToString(-1, 2) + " ms per frame, which is " + pct.ToString(-1, 0) + "% of a 60 FPS frame.\n";
		if (!top.IsEmpty())
			s_sPerf += "Heaviest DCO parts: " + top + ".\n";
		if (pct >= 25.0)
			s_sPerfTip = "DCO AI takes a large share of each frame. Set background squads to AI Detail = Balanced or Light (squad attributes, Performance) and keep AI LOD Mode on Vanilla.";
		else if (fpsAvg < 30.0)
			s_sPerfTip = "FPS is low, but DCO AI is only " + pct.ToString(-1, 0) + "% of the frame. The cause is probably elsewhere: other mods, vehicles, player count or the map.";
		else
			s_sPerfTip = "DCO AI cost looks fine.";
		DCO_Perf.Count("report:perf");
	}

	static string Nice(string section)
	{
		switch (section)
		{
			case "agent_eval": return "soldier decisions";
			case "ag_opp": return "opportunist behaviour";
			case "ag_percept": return "perception";
			case "ag_morale": return "morale";
			case "agent_threat": return "threat and suppression";
			case "ag_target": return "target selection";
			case "ag_combat": return "combat actions";
			case "ag_actions": return "behaviour actions";
			case "ag_msg": return "orders and messages";
			case "ag_callq": return "scheduled reactions";
			case "ag_osc": return "movement smoothing";
			case "ag_cid": return "cover identification";
			case "lean": return "leaning and peeking";
			case "find_cover": return "cover search";
			case "contact_sharing": return "contact sharing";
			case "medic_dispatch": return "medics";
			case "aim_pick": return "aim point selection";
			case "expose": return "exposure checks";
			case "agent_long_range": return "long-range fire control";
			case "veh_fire_position": return "vehicle fire positions";
			case "agent_vehicle_crew": return "vehicle crews";
			case "cmd_think": return "commander planning";
			case "cqb_anticamp": return "close-quarters checks";
		}
		string s = section;
		if (s.StartsWith("cmd_"))
			return "commander (" + s.Substring(4, s.Length() - 4) + ")";
		if (s.StartsWith("grp_"))
			return "squad (" + s.Substring(4, s.Length() - 4) + ")";
		if (s.StartsWith("bt_"))
			return "behaviour tree (" + s.Substring(3, s.Length() - 3) + ")";
		s.Replace("_", " ");
		return s;
	}

	protected static string Clock()
	{
		float ft = 0;
		if (GetGame() && GetGame().GetWorld())
			ft = GetGame().GetWorld().GetWorldTime() / 1000.0;
		int t = ft;
		int h = t / 3600;
		int m = (t / 60) % 60;
		int s = t % 60;
		return string.Format("%1:%2:%3", h, m.ToString(2), s.ToString(2));
	}

	static void Write(bool force)
	{
		string dir = DCO_Log.GetSessionDir();
		if (dir.IsEmpty())
			return;
		Init();
		int now = System.GetTickCount();
		if (!force && s_iLastWrite != 0 && now - s_iLastWrite < 30000)
			return;
		s_iLastWrite = now;

		array<string> txt = {};
		txt.Insert("DCO AI session report");
		txt.Insert("=====================");
		txt.Insert("World: " + GetGame().GetWorldFile());
		txt.Insert("Game time: " + Clock() + "   (this file is rewritten every 30 s and at the end of the session)");
		if (DCO_Log.Mode() == DCO_ELogMode.FULL)
			txt.Insert("Technical logs in this folder: dco.log (all events), perf.log (performance), battle.json (map data).");
		else
			txt.Insert("Technical logs are off (DCO Logging = Report only). For a bug report set DCO Logging to Full and send this whole folder.");
		txt.Insert("");

		txt.Insert("1. SETTINGS");
		txt.Insert("-----------");
		string cfgState = s_sConfig;
		if (cfgState.IsEmpty())
			cfgState = "not read";
		txt.Insert("Server config file: " + cfgState);
		if (s_aSettingOrder.IsEmpty())
			txt.Insert("No setting in the config file differs from Recommended.");
		else
		{
			txt.Insert(string.Format("%1 settings differ from Recommended (from DCO_GlobalConfig.json):", s_aSettingOrder.Count()));
			int shown = 0;
			foreach (string k : s_aSettingOrder)
			{
				if (shown >= 40)
					break;
				txt.Insert("  " + k + " = " + s_mSettings.Get(k));
				shown++;
			}
			if (s_aSettingOrder.Count() > shown)
				txt.Insert(string.Format("  ... and %1 more.", s_aSettingOrder.Count() - shown));
			txt.Insert("  To go back to Recommended: delete DCO_GlobalConfig.json and restart.");
		}
		if (!s_aGmOrder.IsEmpty())
		{
			txt.Insert("Changed by a Game Master during this session:");
			foreach (string g : s_aGmOrder)
				txt.Insert("  " + g + " = " + s_mGm.Get(g));
		}
		if (s_aApiOrder && !s_aApiOrder.IsEmpty())
		{
			txt.Insert("Changed by other mods through the DCO API during this session:");
			foreach (string a : s_aApiOrder)
				txt.Insert("  " + a + " = " + s_mApi.Get(a));
		}
		txt.Insert("");

		txt.Insert("2. PROBLEMS AND WARNINGS");
		txt.Insert("------------------------");
		if (s_aWarnOrder.IsEmpty())
			txt.Insert("None so far.");
		else
		{
			txt.Insert("Each line shows how often it happened and one example. Setup problems (setup_check) usually say how to fix them in brackets.");
			foreach (string w : s_aWarnOrder)
				txt.Insert(string.Format("  %1x  %2", s_mWarnCount.Get(w), s_mWarnSample.Get(w)));
		}
		txt.Insert("");

		txt.Insert("3. PERFORMANCE");
		txt.Insert("--------------");
		if (s_iPerfWindows == 0)
		{
			txt.Insert("Performance profiling is off, so DCO cost is not measured.");
			txt.Insert("To measure it: Game Master > DCO settings > Performance > Performance profiling = On (costs almost nothing).");
		}
		else
		{
			array<string> lines = {};
			s_sPerf.Split("\n", lines, true);
			foreach (string l : lines)
				txt.Insert(l);
			txt.Insert("Advice: " + s_sPerfTip);
		}

		FileHandle f = FileIO.OpenFile(dir + "/report.txt", FileMode.WRITE);
		if (!f)
			return;
		foreach (string line : txt)
			f.WriteLine(line);
		f.Close();
		DCO_Perf.Count("report:write");
	}
}
