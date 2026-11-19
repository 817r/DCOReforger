// Frame-rate governor. Every 10 s it looks at the server's average FPS; while the server stays
// below the target it steps up one level (max 4), and steps back down once there is headroom
// again. Each level shrinks the distances that decide how far from players AI may run cheaper
// (lite mode, smart LOD, dormancy) by 10%, never below fixed floors. It never touches settings
// that change what a fight looks like near players, and never the server's AI limit.
class DCO_Gov
{
	protected static int s_iLevel;
	protected static int s_iBad;
	protected static int s_iGood;
	protected static float s_fCeil;
	protected static float s_fFps;
	protected static float s_fTarget;

	static int Level()
	{
		return s_iLevel;
	}

	static float Fps()
	{
		return s_fFps;
	}

	static float Ceiling()
	{
		return s_fCeil;
	}

	static float Target()
	{
		return s_fTarget;
	}

	// A governor-scaled distance: 10% shorter per level, never below `floor`, and a value an
	// admin already set at or below the floor is left alone.
	static float Dist(float m, float floor)
	{
		if (s_iLevel <= 0 || m <= floor)
			return m;
		return Math.Max(floor, m * (1.0 - 0.1 * s_iLevel));
	}

	static void Window(float fps)
	{
		s_fFps = fps;
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetInstance();
		float cfgTarget = 20.0;
		if (g)
			cfgTarget = g.DCO_TuneF("gov.target_fps", 20.0);
		// Best FPS this server reaches. A server capped with -maxFPS near the target would
		// otherwise never show headroom and ratchet up. Slowly forgotten (half-life about an
		// hour), but only while the server is doing fine: under sustained load the ceiling
		// would sink to the current FPS and the governor would talk itself back to level 0.
		if (fps >= cfgTarget && s_iLevel == 0)
			s_fCeil = Math.Max(fps, s_fCeil * 0.998);
		else
			s_fCeil = Math.Max(fps, s_fCeil);
		if (!g || g.DCO_TuneF("gov.on", 1.0) <= 0)
		{
			SetLevel(0, fps, "off");
			return;
		}
		float target = Math.Min(cfgTarget, s_fCeil * 0.8);
		float relax = Math.Min(target * 1.3, s_fCeil * 0.92);
		s_fTarget = target;
		if (fps < target)
		{
			s_iBad++;
			s_iGood = 0;
		}
		else if (fps > relax)
		{
			s_iGood++;
			s_iBad = 0;
		}
		else
		{
			s_iBad = 0;
			s_iGood = 0;
		}
		if (s_iBad >= 3 && s_iLevel < 4)
		{
			s_iBad = 0;
			SetLevel(s_iLevel + 1, fps, "slow");
		}
		else if (s_iGood >= 6 && s_iLevel > 0)
		{
			s_iGood = 0;
			SetLevel(s_iLevel - 1, fps, "headroom");
		}
	}

	protected static void SetLevel(int lvl, float fps, string why)
	{
		if (lvl == s_iLevel)
			return;
		int was = s_iLevel;
		s_iLevel = lvl;
		DCO_Perf.Count("gov:level_" + lvl);
		string line = string.Format("[DCO] gov_level level=%1 was=%2 why=%3 fps=%4 target=%5 ceiling=%6", lvl, was, why, fps.ToString(-1, 1), s_fTarget.ToString(-1, 1), s_fCeil.ToString(-1, 1));
		Print(line, LogLevel.NORMAL);
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", line);
	}
}

// Always-on server tick for the performance systems (the profiler's own tick only runs while
// profiling is switched on): governor windows, off-screen battles and the per-minute line.
class DCO_PerfHub
{
	protected static float s_fWin;
	protected static int s_iWinFrames;
	protected static float s_fMin;
	protected static int s_iMinFrames;
	protected static float s_fMinMaxFrame;
	protected static float s_fPaper;

	static int s_iWakes;
	static int s_iSleeps;
	static int s_iPaperLost;
	static int s_iPaperGone;

	static void Tick(float timeSlice)
	{
		s_fWin += timeSlice;
		s_iWinFrames++;
		s_fMin += timeSlice;
		s_iMinFrames++;
		if (timeSlice > s_fMinMaxFrame)
			s_fMinMaxFrame = timeSlice;

		s_fPaper += timeSlice;
		if (s_fPaper >= 5.0)
		{
			float dt = s_fPaper;
			s_fPaper = 0;
			DCO_Paper.Tick(dt);
		}

		if (s_fWin >= 10.0)
		{
			DCO_Gov.Window(s_iWinFrames * 1.0 / s_fWin);
			s_fWin = 0;
			s_iWinFrames = 0;
		}

		if (s_fMin >= 60.0)
		{
			WriteMinute();
			s_fMin = 0;
			s_iMinFrames = 0;
			s_fMinMaxFrame = 0;
			s_iWakes = 0;
			s_iSleeps = 0;
			s_iPaperLost = 0;
			s_iPaperGone = 0;
		}
	}

	protected static void WriteMinute()
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetInstance();
		if (g && g.DCO_TuneF("perf.minute_line", 1.0) <= 0)
			return;

		float fps = 0;
		if (s_fMin > 0)
			fps = s_iMinFrames * 1.0 / s_fMin;
		float fpsMin = 0;
		if (s_fMinMaxFrame > 0)
			fpsMin = 1.0 / s_fMinMaxFrame;

		int active = -1;
		int limit = -1;
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (aiWorld)
		{
			active = aiWorld.GetCurrentNumOfActiveAIs();
			limit = aiWorld.GetLimitOfActiveAIs();
		}

		int groups, dormGroups, dormMen, high;
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
		{
			foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
			{
				if (!cmd)
					continue;
				foreach (DCO_GroupUtilityComponent gu : cmd.GetOwnedGroups())
				{
					if (!gu || !gu.GetOwner())
						continue;
					AIGroup grp = AIGroup.Cast(gu.GetOwner());
					if (!grp)
						continue;
					groups++;
					if (grp.GetImportance() >= SCR_EAISpawnImportance.HIGH)
						high++;
					if (grp.GetAgentsCount() == 0 && DCO_Dormant.Is(grp))
					{
						dormGroups++;
						dormMen += DCO_Dormant.Count(grp);
					}
				}
			}
		}

		int players;
		if (GetGame().GetPlayerManager())
			players = GetGame().GetPlayerManager().GetPlayerCount();

		string line = string.Format("PERF_MIN fps=%1 fps_min=%2 gov=%3 ai_active=%4/%5 ai_thinking=%6 players=%7 dco_groups=%8 high=%9",
			fps.ToString(-1, 1), fpsMin.ToString(-1, 1), DCO_Gov.Level(), active, limit, DCO_Hostiles.Sim(), players, groups, high);
		line += string.Format(" asleep_groups=%1 asleep_men=%2 sleeps=%3 wakes=%4 paper_moving=%5 paper_fighting=%6 paper_lost=%7 paper_destroyed=%8",
			dormGroups, dormMen, s_iSleeps, s_iWakes, DCO_Paper.s_iMoving, DCO_Paper.s_iFighting, s_iPaperLost, s_iPaperGone);
		Print("[DCO] " + line, LogLevel.NORMAL);
		DCO_Log.Perf(line, line);
	}
}
