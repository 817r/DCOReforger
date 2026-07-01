modded class SCR_AIGroupTargetClusterProcessor
{
	override SCR_AIActivityBase TryCreateActivityForState(SCR_AITargetClusterState s, EAITargetClusterState estate, notnull TFireteamLockRefArray inFtsMain, notnull TFireteamLockRefArray inFtsAux)
	{
		TFireteamLockRefArray ftsMain;
		TFireteamLockRefArray ftsAux;
		
		// ---- Try to allocate new fireteams, or reuse existing
		array<SCR_AIGroupFireteam> newFireteams = {};
		switch (estate)
		{
			case EAITargetClusterState.INVESTIGATING:
			{
				// Reuse previous main fireteams if provided
				if (!inFtsMain.IsEmpty())
					ftsMain = SCR_AIGroupFireteamLock.CopyLockArray(inFtsMain); // Reuse old fireteams
				else
					ftsMain = {};
				
				// Reuse previous aux fireteams if provided
				if (!inFtsAux.IsEmpty())
					ftsAux = SCR_AIGroupFireteamLock.CopyLockArray(inFtsAux); // Reuse old fireteams
				else
					ftsAux = {};
				
				AllocateFTForInvestigate(s, ftsMain, ftsAux);
				
				if (ftsMain.IsEmpty())
					return null;
				
				break;
			}
			case EAITargetClusterState.ATTACKING:
			{
				//-----------------------------------
				// Main fireteams
				
				// Reuse previous main fireteams if provided
				if (!inFtsMain.IsEmpty())
					ftsMain = SCR_AIGroupFireteamLock.CopyLockArray(inFtsMain); // Reuse old fireteams
				else
					ftsMain = {};
				
				// Reuse previous aux fireteams if provided
				if (!inFtsAux.IsEmpty())
					ftsAux = SCR_AIGroupFireteamLock.CopyLockArray(inFtsAux); // Reuse old fireteams
				else
					ftsAux = {};
				
				// Allocate even more fireteams if needed
				AllocateMoreFireteams(s, ftsMain, ftsAux);
				
				// If by now we have nothing, it's pointless
				if (ftsMain.IsEmpty())
					return null;
				
				
				//-----------------------------------
				// Aux fireteams
				
				
				
				// Try to ensure at least one aux. fireteam
				// If we have many main fireteams, distribute some to aux fireteams
				if (ftsAux.IsEmpty())
				{
					if (ftsMain.Count() > 1)
					{
						SCR_AIGroupFireteamLock ftLock = ftsMain[ftsMain.Count()-1];
						ftsMain.Remove(ftsMain.Count()-1);
						ftsAux.Insert(ftLock);
					}
					else if (m_Utility.m_FireteamMgr.FindFreeFireteams(newFireteams, 1, SCR_AIGroupFireteam))
					{
						SCR_AIGroupFireteamLock.TryLockFireteams(newFireteams, ftsAux, true);
					}
				}
				else
				{
					int totalFireteams = ftsMain.Count() + ftsAux.Count();
					int maxAuxFireteams = Math.Max(1, totalFireteams / 3);
					
					while (ftsAux.Count() > maxAuxFireteams)
					{
						SCR_AIGroupFireteamLock ftLock = ftsAux[ftsAux.Count()-1];
						ftsMain.Insert(ftLock);
						ftsAux.Remove(ftsAux.Count()-1);
					}
				}
				
				break;
			}
			
			case EAITargetClusterState.DEFENDING:
			{
				if (!inFtsMain.IsEmpty() || !inFtsAux.IsEmpty())
				{
					ftsMain = {};
					foreach (auto ft : inFtsMain)
						ftsMain.Insert(ft);
					foreach (auto ft : inFtsAux)
						ftsAux.Insert(ft);
					
					ftsAux = {};
				}
				else if (m_Utility.m_FireteamMgr.FindFreeFireteams(newFireteams, 1, SCR_AIGroupFireteam))
				{
					ftsMain = {};
					SCR_AIGroupFireteamLock.TryLockFireteams(newFireteams, ftsMain, true);
					ftsAux = {};
				}
				else
				{
					return null;
				}
				
				break;
			}
			
			default:
			{
				return null;
			}
		}
		
		
		
		// ---- Create activity
		// At this point we know we have found fireteams
		SCR_AIActivityBase activity = null;
		switch (estate)
		{
			case EAITargetClusterState.INVESTIGATING:
			{
				activity = new SCR_AIInvestigateClusterActivity(m_Utility, null, s, ftsMain, ftsAux);
				//m_Utility.AddAction(activity);
				//return activity;
				break;
			}
			case EAITargetClusterState.ATTACKING:
			{
				activity = new SCR_AIAttackClusterActivity(m_Utility, null, s, ftsMain, ftsAux);
				//m_Utility.AddAction(activity);
				//return activity;
				break;
			}
			case EAITargetClusterState.DEFENDING:
			{
				activity = new SCR_AIDefendFromClusterActivity(m_Utility, null, s, ftsMain);
				//m_Utility.AddAction(activity);
				//return activity;
				break;
			}
		}
		
		return activity;
	}
	
	void AllocateFTForInvestigate(SCR_AITargetClusterState s, notnull TFireteamLockRefArray inOutFtLocksMain, notnull TFireteamLockRefArray ftLocksAux)
	{
		array<SCR_AIGroupFireteam> freeFireteams = {};
		m_Utility.m_FireteamMgr.GetFreeFireteams(freeFireteams, SCR_AIGroupFireteam);
		while (!freeFireteams.IsEmpty())
		{
			SCR_AIGroupFireteam newFireteam = freeFireteams[0];
			SCR_AIGroupFireteamLock newFtLock = newFireteam.TryLock();
			inOutFtLocksMain.Insert(newFtLock);
			freeFireteams.Remove(0);
		}
	}
}