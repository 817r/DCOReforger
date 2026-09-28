class DCO_PerfSection
{
	int m_iCalls;
	int m_iTotal;
	int m_iMax;
	int m_iFrame;
	int m_iAllTotal;
	int m_iAllCalls;
}

class DCO_Perf
{
	static bool s_bOn;

	protected static const float WINDOW_S = 30.0;
	protected static const float SPIKE_S = 0.05;
	protected static const string DIR = "$profile:DCO_Bench";

	protected static ref map<string, ref DCO_PerfSection> s_mSections = new map<string, ref DCO_PerfSection>();
	protected static ref map<string, int> s_mCounts = new map<string, int>();
	protected static ref map<string, int> s_mCountsAll = new map<string, int>();

	protected static float s_fWindow;
	protected static int s_iFrames;
	protected static float s_fMaxFrame;
	protected static float s_fElapsed;
	protected static string s_sFile;

	static int Begin()
	{
		if (!s_bOn)
			return 0;
		return System.GetTickCount();
	}

	static void End(string section, int start)
	{
		if (!s_bOn)
			return;

		int dt = System.GetTickCount() - start;
		DCO_PerfSection s = s_mSections.Get(section);
		if (!s)
		{
			s = new DCO_PerfSection();
			s_mSections.Set(section, s);
		}
		s.m_iCalls++;
		s.m_iTotal += dt;
		s.m_iFrame += dt;
		s.m_iAllTotal += dt;
		s.m_iAllCalls++;
		if (dt > s.m_iMax)
			s.m_iMax = dt;
	}

	static void Count(string source)
	{
		if (!s_bOn)
			return;

		s_mCounts.Set(source, s_mCounts.Get(source) + 1);
		s_mCountsAll.Set(source, s_mCountsAll.Get(source) + 1);
	}

	static void SetEnabled(bool on)
	{
		if (on == s_bOn)
			return;

		if (!on)
			WriteSummary("off");

		s_bOn = on;
		s_mSections.Clear();
		s_mCounts.Clear();
		s_mCountsAll.Clear();
		s_fWindow = 0;
		s_iFrames = 0;
		s_fMaxFrame = 0;
		s_fElapsed = 0;
		s_sFile = string.Empty;
	}

	static void Tick(float timeSlice)
	{
		if (!s_bOn)
			return;

		if (timeSlice > SPIKE_S)
			LogSpike(timeSlice);

		foreach (string name, DCO_PerfSection s : s_mSections)
			s.m_iFrame = 0;

		s_iFrames++;
		s_fWindow += timeSlice;
		s_fElapsed += timeSlice;
		if (timeSlice > s_fMaxFrame)
			s_fMaxFrame = timeSlice;

		if (s_fWindow < WINDOW_S)
			return;

		WriteWindow();
		s_fWindow = 0;
		s_iFrames = 0;
		s_fMaxFrame = 0;
		foreach (string name, DCO_PerfSection s : s_mSections)
		{
			s.m_iCalls = 0;
			s.m_iTotal = 0;
			s.m_iMax = 0;
		}
		s_mCounts.Clear();
	}

	protected static void LogSpike(float timeSlice)
	{
		string parts;
		foreach (string name, DCO_PerfSection s : s_mSections)
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
			fpsAvg = s_iFrames / s_fWindow;
		float fpsMin = 0;
		if (s_fMaxFrame > 0)
			fpsMin = 1.0 / s_fMaxFrame;

		int lod0, lodMid, lodMax;
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
		foreach (string name, DCO_PerfSection s : s_mSections)
		{
			if (s.m_iCalls > 0)
				sections += string.Format(" %1=%2/%3/%4", name, s.m_iCalls, s.m_iTotal, s.m_iMax);
		}

		string queries;
		foreach (string src, int n : s_mCounts)
			queries += string.Format(" %1=%2", src, (n / s_fWindow).ToString(-1, 1));

		Write(string.Format("PERF fps_avg=%1 fps_min=%2 frame_max_ms=%3 ai_lod0=%4 ai_lodmid=%5 ai_lodmax=%6 groups:%7",
			fpsAvg.ToString(-1, 1), fpsMin.ToString(-1, 1), Math.Round(s_fMaxFrame * 1000), lod0, lodMid, lodMax, groups));
		Write("PERF sections(calls/ms_total/ms_max):" + sections);
		Write("PERF queries_per_s:" + queries);
	}

	static void WriteSummary(string reason)
	{
		if (!s_bOn)
			return;

		array<string> names = {};
		array<int> totals = {};
		foreach (string name, DCO_PerfSection s : s_mSections)
			InsertSorted(names, totals, name, s.m_iAllTotal);

		string top;
		for (int i = 0; i < names.Count() && i < 5; i++)
			top += string.Format(" %1=%2ms", names[i], totals[i]);
		Write(string.Format("PERF_SUMMARY reason=%1 duration=%2s top_sections:%3", reason, Math.Round(s_fElapsed), top));

		names.Clear();
		totals.Clear();
		foreach (string src, int n : s_mCountsAll)
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
		Print("[DCO_Perf] " + line);
		DCO_BenchmarkLoggerComponent.Event(line);

		if (s_sFile.IsEmpty())
		{
			FileIO.MakeDirectory(DIR);
			int y, mo, d, h, mi, s;
			System.GetYearMonthDay(y, mo, d);
			System.GetHourMinuteSecond(h, mi, s);
			s_sFile = string.Format("%1/perf_%2%3%4_%5%6%7.log", DIR, y, mo.ToString(2), d.ToString(2), h.ToString(2), mi.ToString(2), s.ToString(2));
		}

		FileHandle f = FileIO.OpenFile(s_sFile, FileMode.APPEND);
		if (!f)
			return;
		f.WriteLine(string.Format("[T+%1] %2", s_fElapsed.ToString(7, 1), line));
		f.Close();
	}
}
