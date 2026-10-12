class DCO_PerfSection
{
	int m_iCalls;
	int m_iTotal;
	int m_iMax;
	int m_iFrame;
	int m_iAllTotal;
	int m_iAllCalls;
	int m_iCensusTotal;
	int m_iCensusCalls;
	int m_iAllMax;
}

class DCO_Perf
{
	static bool s_bOn;
	static bool s_bCensus;
	protected static bool s_bActive;


	protected static ref map<string, ref DCO_PerfSection> s_mSections;
	protected static map<string, ref DCO_PerfSection> GetSectionsStore()
	{
		if (!s_mSections)
			s_mSections = new map<string, ref DCO_PerfSection>();
		return s_mSections;
	}
	protected static ref map<string, int> s_mCounts;
	protected static map<string, int> GetCounts()
	{
		if (!s_mCounts)
			s_mCounts = new map<string, int>();
		return s_mCounts;
	}
	protected static ref map<string, int> s_mCountsAll;
	protected static map<string, int> GetCountsAll()
	{
		if (!s_mCountsAll)
			s_mCountsAll = new map<string, int>();
		return s_mCountsAll;
	}
	protected static ref map<string, int> s_mCensusCounts;

	protected static float s_fWindow;
	protected static int s_iFrames;
	protected static float s_fMaxFrame;
	protected static float s_fElapsed;

	protected static int s_iFrameShots;
	protected static int s_iFrameCallLater;
	protected static int s_iFrameQueries;
	protected static int s_iFrameTraces;

	static int Begin()
	{
		if (!s_bActive)
			return 0;
		return System.GetTickCount();
	}

	static void End(string section, int start)
	{
		if (!s_bActive)
			return;

		int dt = System.GetTickCount() - start;
		DCO_PerfSection s = GetSectionsStore().Get(section);
		if (!s)
		{
			s = new DCO_PerfSection();
			GetSectionsStore().Set(section, s);
		}
		s.m_iCalls++;
		s.m_iTotal += dt;
		s.m_iFrame += dt;
		s.m_iAllTotal += dt;
		s.m_iAllCalls++;
		if (dt > s.m_iAllMax)
			s.m_iAllMax = dt;
		s.m_iCensusTotal += dt;
		s.m_iCensusCalls++;
		if (dt > s.m_iMax)
			s.m_iMax = dt;
	}

	static void Count(string source)
	{
		CountN(source, 1);
	}

	static void CountN(string source, int n)
	{
		if (!s_bActive)
			return;

		if (s_bOn)
		{
			GetCounts().Set(source, GetCounts().Get(source) + n);
			GetCountsAll().Set(source, GetCountsAll().Get(source) + n);
		}

		if (!s_bCensus)
			return;

		s_mCensusCounts.Set(source, s_mCensusCounts.Get(source) + n);
		if (source == "ev:shot")
			s_iFrameShots += n;
		else if (source == "cl:shot")
			s_iFrameCallLater += n;
		else if (source.StartsWith("q:"))
			s_iFrameQueries += n;
		else if (source.StartsWith("t:"))
			s_iFrameTraces += n;
	}

	static void SetEnabled(bool on)
	{
		if (on == s_bOn)
			return;

		if (!on)
			WriteSummary("off");

		s_bOn = on;
		GetCounts().Clear();
		GetCountsAll().Clear();
		s_fWindow = 0;
		s_iFrames = 0;
		s_fMaxFrame = 0;
		s_fElapsed = 0;
		foreach (string name, DCO_PerfSection s : GetSectionsStore())
		{
			s.m_iCalls = 0;
			s.m_iTotal = 0;
			s.m_iMax = 0;
			s.m_iAllTotal = 0;
			s.m_iAllCalls = 0;
			s.m_iAllMax = 0;
		}
		s_bActive = s_bOn || s_bCensus;
	}

	static void SetCensus(bool on)
	{
		s_bCensus = on;
		if (on && !s_mCensusCounts)
			s_mCensusCounts = new map<string, int>();
		ResetCensus();
		s_bActive = s_bOn || s_bCensus;
	}

	static void ResetCensus()
	{
		if (s_mCensusCounts)
			s_mCensusCounts.Clear();
		foreach (string name, DCO_PerfSection s : GetSectionsStore())
		{
			s.m_iCensusTotal = 0;
			s.m_iCensusCalls = 0;
		}
	}

	static map<string, ref DCO_PerfSection> GetSections()
	{
		return GetSectionsStore();
	}

	static int TotalCount(string source)
	{
		return GetCountsAll().Get(source);
	}

	static int SectionMaxMs(string section)
	{
		DCO_PerfSection s = GetSectionsStore().Get(section);
		if (!s)
			return 0;
		return s.m_iAllMax;
	}

	static int CensusCount(string source)
	{
		if (!s_mCensusCounts)
			return 0;
		return s_mCensusCounts.Get(source);
	}

	protected static string TopFrameSection()
	{
		string best = "-";
		int bestMs = 0;
		foreach (string name, DCO_PerfSection s : GetSectionsStore())
		{
			if (s.m_iFrame > bestMs)
			{
				bestMs = s.m_iFrame;
				best = name;
			}
		}
		return best;
	}

	static void Tick(float timeSlice)
	{
		if (!s_bActive)
			return;

		if (timeSlice > 0.05)
		{
			if (s_bOn)
				LogSpike(timeSlice);
			if (s_bCensus)
				DCO_Census.Spike(timeSlice, TopFrameSection(), s_iFrameShots, s_iFrameCallLater, s_iFrameQueries, s_iFrameTraces);
		}

		foreach (string name, DCO_PerfSection s : GetSectionsStore())
			s.m_iFrame = 0;
		s_iFrameShots = 0;
		s_iFrameCallLater = 0;
		s_iFrameQueries = 0;
		s_iFrameTraces = 0;

		if (s_bCensus)
			DCO_Census.Tick(timeSlice);

		if (!s_bOn)
			return;

		s_iFrames++;
		s_fWindow += timeSlice;
		s_fElapsed += timeSlice;
		if (timeSlice > s_fMaxFrame)
			s_fMaxFrame = timeSlice;

		if (s_fWindow < 30.0)
			return;

		WriteWindow();
		s_fWindow = 0;
		s_iFrames = 0;
		s_fMaxFrame = 0;
		foreach (string name, DCO_PerfSection s : GetSectionsStore())
		{
			s.m_iCalls = 0;
			s.m_iTotal = 0;
			s.m_iMax = 0;
		}
		GetCounts().Clear();
	}

	protected static void LogSpike(float timeSlice)
	{
		string parts;
		foreach (string name, DCO_PerfSection s : GetSectionsStore())
		{
			if (s.m_iFrame > 0)
				parts += string.Format(" %1=%2", name, s.m_iFrame);
		}
		Write(string.Format("PERF_SPIKE frame_ms=%1%2", Math.Round(timeSlice * 1000), parts));
	}

	protected static void WriteWindow()
	{
		float fpsAvg = 0;
		if (s_fWindow > 0)
			fpsAvg = 1.0 * s_iFrames / s_fWindow;
		float fpsMin = 0;
		if (s_fMaxFrame > 0)
			fpsMin = 1.0 / s_fMaxFrame;

		int lod0, lodMid, lodMax;
		int aiTotal = 0;
		int aiLocked = 0;
		int maxLod = AIAgent.GetMaxLOD();
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (aiWorld)
		{
			array<AIAgent> agents = {};
			aiWorld.GetAIAgents(agents);
			foreach (AIAgent a : agents)
			{
				if (!SCR_ChimeraAIAgent.Cast(a))
					continue;
				aiTotal++;
				SCR_AIUtilityComponent lodUtil = SCR_AIUtilityComponent.Cast(a.FindComponent(SCR_AIUtilityComponent));
				if (lodUtil && lodUtil.m_iDCO_LOD > 0)
					aiLocked++;
				int lod = a.GetLOD();
				if (lod <= 0)
					lod0++;
				else if (lod >= maxLod)
					lodMax++;
				else
					lodMid++;
			}
		}

		string groups;
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
		{
			foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
			{
				if (cmd)
					groups += string.Format(" %1=%2", cmd.GetCommanderUID(), cmd.GetOwnedGroups().Count());
			}
		}

		string sections;
		int dcoMs = 0;
		array<string> topNames = {};
		array<int> topTotals = {};
		foreach (string name, DCO_PerfSection s : GetSectionsStore())
		{
			if (s.m_iCalls > 0)
				sections += string.Format(" %1=%2/%3/%4/%5", name, s.m_iCalls, s.m_iTotal, s.m_iMax, Math.Round(s.m_iTotal * 1000.0 / s.m_iCalls));
			if (s.m_iTotal <= 0)
				continue;
			if (DCO_Census.IsTopLevel(name))
				dcoMs += s.m_iTotal;
			if (name != "agent_eval")
				InsertSorted(topNames, topTotals, name, s.m_iTotal);
		}
		string top;
		for (int ti = 0; ti < topNames.Count() && ti < 3 && dcoMs > 0; ti++)
		{
			if (!top.IsEmpty())
				top += ", ";
			top += DCO_Report.Nice(topNames[ti]) + " " + (topTotals[ti] * 100.0 / dcoMs).ToString(-1, 0) + "%";
		}
		float dcoPerFrame = 0;
		if (s_iFrames > 0)
			dcoPerFrame = dcoMs * 1.0 / s_iFrames;

		string queries;
		foreach (string src, int n : GetCounts())
			queries += string.Format(" %1=%2", src, (1.0 * n / s_fWindow).ToString(-1, 1));

		int players = 0;
		if (GetGame().GetPlayerManager())
			players = GetGame().GetPlayerManager().GetPlayerCount();
		Write(string.Format("PERF fps_avg=%1 fps_min=%2 frame_max_ms=%3 ai_total=%4 ai_dco_locked=%5 players=%6 ai_lod0=%7 ai_lodmid=%8 ai_lodmax=%9",
			fpsAvg.ToString(-1, 1), fpsMin.ToString(-1, 1), Math.Round(s_fMaxFrame * 1000), aiTotal, aiLocked, players, lod0, lodMid, lodMax));
		Write("PERF groups:" + groups);
		Write("PERF sections(calls/ms_total/ms_max/us_per_call):" + sections);
		Write("PERF queries_per_s:" + queries);
		DCO_Report.Perf(fpsAvg, fpsMin, Math.Round(s_fMaxFrame * 1000), aiTotal, players, lod0, lodMid + lodMax, dcoPerFrame, top);
	}

	static void WriteSummary(string reason)
	{
		if (!s_bOn)
			return;

		array<string> names = {};
		array<int> totals = {};
		foreach (string name, DCO_PerfSection s : GetSectionsStore())
			InsertSorted(names, totals, name, s.m_iAllTotal);

		string top;
		for (int i = 0; i < names.Count() && i < 5; i++)
			top += string.Format(" %1=%2ms", names[i], totals[i]);
		Write(string.Format("PERF_SUMMARY reason=%1 duration=%2s top_sections:%3", reason, Math.Round(s_fElapsed), top));

		names.Clear();
		totals.Clear();
		foreach (string src, int n : GetCountsAll())
			InsertSorted(names, totals, src, n);

		top = string.Empty;
		for (int j = 0; j < names.Count() && j < 5; j++)
			top += string.Format(" %1=%2", names[j], totals[j]);
		Write("PERF_SUMMARY top_queries:" + top);
	}

	protected static void InsertSorted(array<string> names, array<int> values, string name, int value)
	{
		int at = 0;
		while (at < values.Count() && values[at] >= value)
			at++;
		names.InsertAt(name, at);
		values.InsertAt(value, at);
	}

	protected static void Write(string line)
	{
		DCO_Log.Perf(line, string.Format("[T+%1] %2", s_fElapsed.ToString(7, 1), line));
	}
}
