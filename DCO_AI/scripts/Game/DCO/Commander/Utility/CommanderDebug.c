class DCO_DebugDraw
{
	static const int COLOR_OWNED     = 0xFF33CC66;
	static const int COLOR_TARGET    = 0xFFFF5522;
	static const int COLOR_QUEUED    = 0xFFAAAAAA;
	static const int COLOR_STAGING   = 0xFFFFCC00;
	static const int COLOR_COMMANDER = 0xFF33AAFF;
	static const int COLOR_MANAGER   = 0xFFFFFFFF;
	static const int COLOR_FRONTLINE = 0xFF6688FF;
	static const int COLOR_NEUTRAL   = 0xFF666666;
	static const int COLOR_TEXT_BG   = 0x80000000;

	static const int COLOR_ROLE_ASSAULT = 0xFFFF6644;
	static const int COLOR_ROLE_DEFEND  = 0xFF44DD88;
	static const int COLOR_ROLE_RECON   = 0xFFCC66FF;
	static const int COLOR_ROLE_IDLE    = 0xFFBBBBBB;

	static const float MARKER_BIG   = 2.0;
	static const float MARKER_SMALL = 0.8;

	static bool IsLocalPlayerInGM()
	{
		SCR_EditorManagerEntity editor = SCR_EditorManagerEntity.GetInstance();
		if (!editor)
			return false;

		return editor.IsOpened();
	}

	static DebugTextWorldSpace SpawnText(vector pos, string text, float size, int color)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		return DebugTextWorldSpace.Create(
			world,
			text,
			DebugTextFlags.CENTER | DebugTextFlags.FACE_CAMERA,
			pos[0], pos[1], pos[2],
			size,
			color,
			COLOR_TEXT_BG);
	}

	static int Flags()
	{
		return ShapeFlags.NOZBUFFER | ShapeFlags.TRANSP;
	}

	static string F1(float v) { return (Math.Round(v * 10.0) / 10.0).ToString(); }
	static string M(float v)  { return Math.Round(v).ToString() + "m"; }

	static string YesNo(bool v)
	{
		if (v)
			return "yes";

		return "no";
	}

	static int PaletteAt(int index)
	{
		switch (index % 6)
		{
			case 0: return 0xFF4488FF;
			case 1: return 0xFFFF5533;
			case 2: return 0xFF44DD88;
			case 3: return 0xFFFFCC22;
			case 4: return 0xFFCC66FF;
		}
		return 0xFF33CCCC;
	}

	static int FactionColor(FactionKey fk)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return COLOR_NEUTRAL;

		for (int i = 0; i < mgr.m_aAvailableFactions.Count(); i++)
		{
			if (mgr.m_aAvailableFactions.Get(i) == fk)
				return PaletteAt(i);
		}

		return COLOR_NEUTRAL;
	}

	static string ModeName(int mode)
	{
		switch (mode)
		{
			case CMD_ECommanderMode.OFFENSIVE: return "OFFENSIVE";
			case CMD_ECommanderMode.DEFENSIVE: return "DEFENSIVE";
			case CMD_ECommanderMode.BALANCED:  return "BALANCED";
		}
		return "UNKNOWN";
	}

	static string CommanderStateName(int state)
	{
		switch (state)
		{
			case CMD_ECommanderState.IDLE:       return "idle";
			case CMD_ECommanderState.PLANNING:   return "planning";
			case CMD_ECommanderState.COMMANDING: return "commanding";
			case CMD_ECommanderState.DEAD:       return "dead";
			case CMD_ECommanderState.REPLACING:  return "replacing";
		}
		return "unknown";
	}

	static string ObjectiveStateName(int state)
	{
		switch (state)
		{
			case CMD_EObjectiveState.PENDING:   return "pending";
			case CMD_EObjectiveState.ASSIGNED:  return "assigned";
			case CMD_EObjectiveState.COMPLETED: return "completed";
			case CMD_EObjectiveState.FAILED:    return "failed";
		}
		return "unknown";
	}

	static string ObjectiveTypeName(int type)
	{
		switch (type)
		{
			case CMD_EObjectiveType.CAPTURE: return "CAPTURE";
			case CMD_EObjectiveType.DESTROY: return "DESTROY";
			case CMD_EObjectiveType.RECON:   return "RECON";
		}
		return "UNKNOWN";
	}

	static string CaptureStatusName(int status)
	{
		switch (status)
		{
			case DCO_ECaptureStatus.CAPTURING: return "CAPTURING";
			case DCO_ECaptureStatus.CONTESTED: return "CONTESTED";
		}
		return "idle";
	}

	static string PhaseName(DCO_ETaskPhase phase)
	{
		switch (phase)
		{
			case DCO_ETaskPhase.MOVING:    return "moving";
			case DCO_ETaskPhase.STAGING:   return "staging";
			case DCO_ETaskPhase.EXECUTING: return "executing";
			case DCO_ETaskPhase.HOLDING:   return "holding";
		}
		return "unknown";
	}

	static string ThreatLevelName(int level)
	{
		switch (level)
		{
			case CMD_EThreatLevel.NEGLIGIBLE: return "NEGLIGIBLE";
			case CMD_EThreatLevel.LOW:        return "LOW";
			case CMD_EThreatLevel.MEDIUM:     return "MEDIUM";
			case CMD_EThreatLevel.HIGH:       return "HIGH";
			case CMD_EThreatLevel.CRITICAL:   return "CRITICAL";
		}
		return "UNKNOWN";
	}

	static int ThreatLevelColor(int level)
	{
		switch (level)
		{
			case CMD_EThreatLevel.NEGLIGIBLE: return 0xFF888888;
			case CMD_EThreatLevel.LOW:        return 0xFF66CC66;
			case CMD_EThreatLevel.MEDIUM:     return 0xFFFFCC22;
			case CMD_EThreatLevel.HIGH:       return 0xFFFF8822;
			case CMD_EThreatLevel.CRITICAL:   return 0xFFFF3322;
		}
		return COLOR_NEUTRAL;
	}

	static int TaskColor(DCO_EGroupTask task)
	{
		switch (task)
		{
			case DCO_EGroupTask.ATTACK:
			case DCO_EGroupTask.FLANK:
			case DCO_EGroupTask.SUPPORT_BY_FIRE:
				return COLOR_ROLE_ASSAULT;

			case DCO_EGroupTask.DEFEND:
			case DCO_EGroupTask.GARRISON:
				return COLOR_ROLE_DEFEND;

			case DCO_EGroupTask.RECON:
				return COLOR_ROLE_RECON;
		}
		return COLOR_ROLE_IDLE;
	}
}
