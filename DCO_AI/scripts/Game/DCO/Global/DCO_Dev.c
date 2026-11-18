class DCO_Dev
{
	static bool RunOn()
	{
#ifdef WORKBENCH
		return true;
#else
		return System.IsCLIParam("dcoTest");
#endif
	}

	static bool TestActive()
	{
		return false;
	}

	static void TestEvent(string text)
	{
	}

	static void TraceUnit(SCR_AIUtilityComponent u, SCR_AIActionBase from, SCR_AIActionBase to)
	{
	}

	static void TraceGroup(AIGroup g, SCR_AIActionBase from, SCR_AIActionBase to, string wp, string task)
	{
	}

	static void Event(string text)
	{
	}

	static void BattleStart()
	{
	}

	static void BattleEnd(string why)
	{
	}

	static void BattleCapture(string text)
	{
	}

	static void Place(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, AIGroup grp, string kind, vector pos, float facingDeg, string building, string slot, string sector, string defense)
	{
	}

	static void OnTask(AIGroup g, DCO_EGroupTask task)
	{
	}

	static void OnWaypoint(SCR_AIGroup grp, AIWaypoint wp)
	{
	}

	static void Intel(AICommander_BaseComponent cmd, CMD_ThreatEntry t, bool isNew)
	{
	}

	static void OnHealed(IEntity patient)
	{
	}

	static void OnHit(IEntity victim, IEntity attacker)
	{
	}

	static void Wounded(ChimeraCharacter ch, IEntity attacker, string state, string dmg, string src)
	{
	}

	static int UnitKills(IEntity e)
	{
		return 0;
	}

	static int UnitHits(IEntity e)
	{
		return 0;
	}

	static float LastHitAt(IEntity e)
	{
		return -100000.0;
	}

	static bool CensusOn()
	{
		return false;
	}

	static float CensusInterval()
	{
		return 30;
	}

	static void SetCensusInterval(float seconds)
	{
	}

	static void SetCensus(bool on)
	{
	}

	static void CensusShutdown()
	{
	}

	static void CensusTick(float timeSlice)
	{
	}

	static void CensusSpike(float timeSlice, string top, int shots, int callLater, int queries, int traces)
	{
	}

	static bool CensusIsTopLevel(string name)
	{
		return false;
	}

	static bool ViewOn()
	{
		return false;
	}

	static void ViewCapture(string text)
	{
	}

	static void ViewRefresh()
	{
	}
}
