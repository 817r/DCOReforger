modded class SCR_AITargetClusterState
{
	int m_iDCOInvestigateRoll = -1;
	ref DCO_StrengthInfo m_DCOStrength = new DCO_StrengthInfo();
	float m_fDCOStrengthAt_ms = -1;
}

modded class SCR_AIGroupTargetClusterProcessor
{
	override EAITargetClusterState EvaluateNewDesiredState(SCR_AITargetClusterState s)
	{
		EAITargetClusterState desired = super.EvaluateNewDesiredState(s);
		if (desired != EAITargetClusterState.INVESTIGATING && desired != EAITargetClusterState.ATTACKING)
			return desired;

		bool superior = m_Utility.DCO_IsSuperiorTo(s);
		int garrisonMode = m_Utility.DCO_GetGarrisonMode();
		if (garrisonMode >= 0)
		{
			if (superior && desired == EAITargetClusterState.ATTACKING && garrisonMode != DCO_EGarrisonMode.HOLD)
				return desired;
			return EAITargetClusterState.NONE;
		}

		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (!cfg)
			return desired;

		bool idle = m_Utility.DCO_IsIdle();

		if (desired == EAITargetClusterState.INVESTIGATING)
		{
			if (idle && !cfg.GetIdleLeaveToInvestigate())
				return EAITargetClusterState.NONE;

			float maxDist = cfg.GetInvestigateMaxDist();
			float chance = cfg.GetInvestigateChance();
			DCO_GroupConfigComponent groupCfg = DCO_GroupConfigComponent.Cast(m_Utility.m_Owner.FindComponent(DCO_GroupConfigComponent));
			if (groupCfg)
			{
				maxDist = groupCfg.GetInvestigateMaxDist();
				chance = groupCfg.GetInvestigateChance();
			}

			if (s.m_fDistMin > maxDist)
				return EAITargetClusterState.NONE;

			if (s.m_iDCOInvestigateRoll < 0)
				s.m_iDCOInvestigateRoll = Math.RandomFloat01() < chance;

			if (s.m_iDCOInvestigateRoll == 0)
				return EAITargetClusterState.NONE;

			return desired;
		}

		if (!idle || s.m_fDistMin <= cfg.GetIdleOverrunDist() || superior)
			return desired;

		if (s.m_iCountEndangering > 0 || !cfg.GetIdleLeaveToAssist())
			return EAITargetClusterState.NONE;

		return desired;
	}

	override SCR_AIActivityBase TryCreateActivityForState(SCR_AITargetClusterState s, EAITargetClusterState estate, notnull TFireteamLockRefArray inFtsMain, notnull TFireteamLockRefArray inFtsAux)
	{
		TFireteamLockRefArray ftsMain;
		TFireteamLockRefArray ftsAux;

		array<SCR_AIGroupFireteam> newFireteams = {};
		switch (estate)
		{
			case EAITargetClusterState.INVESTIGATING:
			{
				if (m_Utility.DCO_GetPosture() == DCO_GroupTactics.EVASIVE)
					return null;

				if (!inFtsMain.IsEmpty())
					ftsMain = SCR_AIGroupFireteamLock.CopyLockArray(inFtsMain);
				else
					ftsMain = {};

				if (!inFtsAux.IsEmpty())
					ftsAux = SCR_AIGroupFireteamLock.CopyLockArray(inFtsAux);
				else
					ftsAux = {};

				AllocateFTForInvestigate(s, ftsMain, ftsAux);

				if (ftsMain.IsEmpty())
					return null;

				DCO_SplitInvestigateFireteams(ftsMain, ftsAux);
				break;
			}
			case EAITargetClusterState.ATTACKING:
			{
				if (!inFtsMain.IsEmpty())
					ftsMain = SCR_AIGroupFireteamLock.CopyLockArray(inFtsMain);
				else
					ftsMain = {};

				if (!inFtsAux.IsEmpty())
					ftsAux = SCR_AIGroupFireteamLock.CopyLockArray(inFtsAux);
				else
					ftsAux = {};

				AllocateMoreFireteams(s, ftsMain, ftsAux);

				if (ftsMain.IsEmpty())
					return null;

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

		SCR_AIActivityBase activity = null;
		switch (estate)
		{
			case EAITargetClusterState.INVESTIGATING:
			{
				activity = new SCR_AIInvestigateClusterActivity(m_Utility, null, s, ftsMain, ftsAux);
				break;
			}
			case EAITargetClusterState.ATTACKING:
			{
				activity = new SCR_AIAttackClusterActivity(m_Utility, null, s, ftsMain, ftsAux);
				break;
			}
			case EAITargetClusterState.DEFENDING:
			{
				activity = new SCR_AIDefendFromClusterActivity(m_Utility, null, s, ftsMain);
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

	protected void DCO_SplitInvestigateFireteams(notnull TFireteamLockRefArray ftsMain, notnull TFireteamLockRefArray ftsAux)
	{
		TFireteamLockRefArray sorted = {};
		array<int> sortedIndoor = {};

		for (int i = 0, count = ftsMain.Count() + ftsAux.Count(); i < count; i++)
		{
			SCR_AIGroupFireteamLock ftLock;
			if (i < ftsMain.Count())
				ftLock = ftsMain[i];
			else
				ftLock = ftsAux[i - ftsMain.Count()];

			int indoor = DCO_CountIndoor(ftLock.GetFireteam());

			int at = 0;
			while (at < sortedIndoor.Count() && sortedIndoor[at] <= indoor)
				at++;

			sorted.InsertAt(ftLock, at);
			sortedIndoor.InsertAt(indoor, at);
		}

		int mainCount = Math.Max(1, (sorted.Count() + 1) / 2);

		ftsMain.Clear();
		ftsAux.Clear();
		foreach (int idx, SCR_AIGroupFireteamLock sortedLock : sorted)
		{
			if (idx < mainCount)
				ftsMain.Insert(sortedLock);
			else
				ftsAux.Insert(sortedLock);
		}
	}

	protected int DCO_CountIndoor(SCR_AIGroupFireteam ft)
	{
		if (!ft)
			return 0;

		array<AIAgent> agents = {};
		ft.GetMembers(agents);

		int indoor = 0;
		foreach (AIAgent agent : agents)
		{
			if (agent && SCR_CoverManagerComponent.DCO_GetBuildingAt(agent.GetControlledEntity()))
				indoor++;
		}

		return indoor;
	}
}