modded class SCR_AIGroupTargetClusterProcessor : Managed
{
	protected const float INITIAL_DECISION_DELAY_MS = 1000.0;
	
	// When we don't receive new information from cluster for more than this amount of seconds,
	// We switch to 'LOST' state.
	protected const float MAX_CLUSTER_AGE_S = 180.0;
	
	override void AllocateMoreFireteams(SCR_AITargetClusterState s, notnull TFireteamLockRefArray inOutFtLocksMain, notnull TFireteamLockRefArray ftLocksAux)
	{
		// We slightly overestimate amount of enemies to allocate even more people
		float fEnemies = 1.3 * (float)s.m_iCountDetected + 1.3*s.m_iCountIdentified + 1.3*s.m_iCountLost + 0.5*s.m_iCountDestroyed;
		
		int nEnemies = Math.Ceil(fEnemies);
		
		// Count soldiers from what we have so far
		int nSoldiersAllocated = 0;
		foreach (SCR_AIGroupFireteamLock ftLock : inOutFtLocksMain)
			nSoldiersAllocated += ftLock.GetFireteam().GetMemberCount();
		foreach (SCR_AIGroupFireteamLock ftLock : ftLocksAux)
			nSoldiersAllocated += ftLock.GetFireteam().GetMemberCount();
		
		// Allocate fireteams
		array<SCR_AIGroupFireteam> freeFireteams = {};
		m_Utility.m_FireteamMgr.GetFreeFireteams(freeFireteams, SCR_AIGroupFireteam);
		while (nSoldiersAllocated <= nEnemies * 4 && !freeFireteams.IsEmpty())
		{
			SCR_AIGroupFireteam newFireteam = freeFireteams[0];
			SCR_AIGroupFireteamLock newFtLock = newFireteam.TryLock();
			inOutFtLocksMain.Insert(newFtLock);
			freeFireteams.Remove(0);
			
			nSoldiersAllocated += newFireteam.GetMemberCount();
		}
	}	
}