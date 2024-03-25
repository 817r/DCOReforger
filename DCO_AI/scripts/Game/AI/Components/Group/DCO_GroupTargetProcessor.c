modded class SCR_AIGroupTargetClusterProcessor : Managed
{
	protected const float INITIAL_DECISION_DELAY_MS = 1000.0;
	
	// When we don't receive new information from cluster for more than this amount of seconds,
	// We switch to 'LOST' state.
	protected const float MAX_CLUSTER_AGE_S = 150.0;
	
	override void AllocateMoreFireteams(SCR_AITargetClusterState s, notnull TFireteamLockRefArray inOutFtLocksMain, notnull TFireteamLockRefArray ftLocksAux)
	{
		// We slightly overestimate amount of enemies to allocate even more people
		float fEnemies = 1 * (float)s.m_iCountDetected + 1*s.m_iCountIdentified + 1*s.m_iCountLost + 1*s.m_iCountDestroyed;
		
		int nEnemies = Math.Ceil(fEnemies);
		
		// Count soldiers from what we have so far
		int nSoldiersAllocated = 0;
		foreach (SCR_AIGroupFireteamLock ftLock : inOutFtLocksMain)
			nSoldiersAllocated += ftLock.GetFireteam().GetMemberCount();
		foreach (SCR_AIGroupFireteamLock ftLock : ftLocksAux)
			nSoldiersAllocated += ftLock.GetFireteam().GetMemberCount();
		
		// Allocate fireteams
		array<SCR_AIGroupFireteam> freeFireteams = {};
		m_Utility.m_FireteamMgr.GetFreeFireteams(freeFireteams);
		while (nSoldiersAllocated < nEnemies && !freeFireteams.IsEmpty())
		{
			SCR_AIGroupFireteam newFireteam = freeFireteams[0];
			SCR_AIGroupFireteamLock newFtLock = newFireteam.TryLock();
			inOutFtLocksMain.Insert(newFtLock);
			freeFireteams.Remove(0);
			
			nSoldiersAllocated += newFireteam.GetMemberCount();
		}
	}	
	
	//! Calculates how long we should investigate this target cluster
	override float CalculateMaxAgeThreshold_s(SCR_AITargetClusterState s, EAITargetClusterState newState)
	{
		int countAlive = s.m_iCountLost + s.m_iCountDetected + s.m_iCountIdentified;
		
		if (countAlive > 0)
		{
			// If some targets are still alive
			return MAX_CLUSTER_AGE_S;
		}
		else
		{
			// Everything is destroyed, investigation time is lower
			vector ourPos = m_Utility.m_Owner.GetCenterOfMass();
			vector tgtPos = 0.5 * (s.m_vBBMin + s.m_vBBMax);
			float distance = vector.DistanceXZ(ourPos, tgtPos);
			float tgtCount = s.m_iCountDestroyed + s.m_iCountIdentified;
			
			const float movementSpeed = 1.0; // Speed in m/s
			float duration_s = distance / movementSpeed + 10.0 * tgtCount;
			duration_s = Math.Max(20.0, duration_s);
			
			return duration_s;
		}
	}
}