modded class AICommander_BaseComponent
{
	protected void UpdateCommanderDebug(float timeSlice)
	{
		if (!m_bDebugMode)
		{
			if (!m_aDebugShapes.IsEmpty() || !m_aDebugTexts.IsEmpty())
			{
				m_aDebugShapes.Clear();
				m_aDebugTexts.Clear();
			}
			return;
		}

		m_fDebugTimer += timeSlice;
		if (m_fDebugTimer < m_fDebugRefreshInterval)
			return;

		m_fDebugTimer = 0.0;

		m_aDebugShapes.Clear();
		m_aDebugTexts.Clear();

		if (!DCO_DebugDraw.IsLocalPlayerInGM())
			return;

		int    flags = DCO_DebugDraw.Flags();
		vector p     = GetOwner().GetOrigin();
		float  now   = DCO_Util.NowS();

		m_aDebugShapes.Insert(Shape.CreateSphere(DCO_DebugDraw.COLOR_COMMANDER, flags, p, 2.0));

		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(Vector(p[0], p[1] + 46.0, p[2]), BuildDebugHeader(now),   22.0, DCO_DebugDraw.COLOR_COMMANDER));
		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(Vector(p[0], p[1] + 37.0, p[2]), BuildDebugForce(),       17.0, DCO_DebugDraw.COLOR_COMMANDER));
		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(Vector(p[0], p[1] + 27.0, p[2]), BuildDebugPersonality(), 17.0, DCO_DebugDraw.COLOR_COMMANDER));
		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(Vector(p[0], p[1] + 16.0, p[2]), BuildDebugBudget(),      17.0, DCO_DebugDraw.COLOR_COMMANDER));
		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(Vector(p[0], p[1] +  4.0, p[2]), BuildDebugTuning(),      16.0, DCO_DebugDraw.COLOR_COMMANDER));

		DrawDebugObjectiveLinks(p, flags, now);
		m_Frontline.DrawDebug(m_aDebugShapes, m_aDebugTexts, p, flags);
		m_Logistics.DrawDebug(m_aDebugShapes, m_aDebugTexts, flags);
	}

	protected string BuildDebugHeader(float now)
	{
		return string.Format(
			"%1  [%2]\nMODE %3   state: %4\nnext think in %5s of %6s",
			m_sCommanderUID,
			m_sFactionKey,
			DCO_DebugDraw.ModeName(m_eCommanderMode),
			DCO_DebugDraw.CommanderStateName(m_eCommanderState),
			DCO_DebugDraw.F1(Math.Max(m_fThinkInterval - m_fThinkTimer, 0.0)),
			DCO_DebugDraw.F1(m_fThinkInterval));
	}

	protected string BuildDebugForce()
	{
		int defendDemand = -1;
		if (m_eCommanderMode != CMD_ECommanderMode.OFFENSIVE)
			defendDemand = CountDefendDemand();

		return string.Format(
			"FORCE\nmanpower %1   reserve %2   floor %3   all-in %4\ngroups %5   idle/committable %6\nobjectives worked %7 of %8 max   garrison needed %9",
			GetTotalManpower(),
			GetReserveManpower(),
			Math.Round(GetReserveFloor()),
			m_bAllIn,
			m_aOwnedGroup.Count(),
			CountIdleCommittableGroups(),
			m_aObjective.Count(),
			m_fObjectiveAtTheSameTime,
			defendDemand);
	}

	protected string BuildDebugPersonality()
	{
		return string.Format(
			"PERSONALITY (0..1, drives the gates below)\naggression %1 -- tempo: staging vs recon roll\nrisk %2 -- commit into fog     patience %3 -- stalemate wait\nresilience %4 -- retreat depth  adaptability %5 -- think rate\ncombat focus %6 -- tier threshold, small-threat response",
			m_fAggression,
			m_fRiskTaking,
			m_fPatience,
			m_fResilience,
			m_fAdaptability,
			m_fCombatFocus);
	}

	protected string BuildDebugBudget()
	{
		string head = string.Format(
			"BUDGET (reserve policy)\nreserve floor = total %1 x %2 pct   all-in %3 (rear defenders %4)\nattack ratio >= %5 : 1",
			GetTotalManpower(),
			m_fReservePercent,
			m_bAllIn,
			m_bAllInRearDefenders,
			GetMinAttackRatio().ToString(-1, 1));

		if (m_eCommanderMode != CMD_ECommanderMode.BALANCED)
			return head + "\ndefend share n/a -- only BALANCED splits the idle pool";

		int   idle  = CountIdleCommittableGroups();
		float share = GetDefendShare();

		return head + string.Format(
			"\ndefend share %1 (from reserve policy)\n-> up to %2 of %3 idle groups reserved for defend",
			share, Math.Round(1.0 * idle * share), idle);
	}

	protected string BuildDebugTuning()
	{
		string a = string.Format(
			"TUNING\nsync attack %1   sync max wait %2s   recon wait %3s",
			m_bUseSynchronizedAttack, m_fSyncAttackMaxWaitTime, m_fReconWaitTimeout);

		string b = string.Format(
			"\nstalemate cooldown %1s (patience-scaled)   retreat threshold %2\nsectors %3..%4 at %5m arc   sector personality mod %6",
			Math.Round(m_fStalemateResponseCooldown), m_iRetreatThreshold,
			m_iMinSector, m_iMaxSector, m_fArcPerSector, GetSectorPersonalityMod());

		string c = string.Format(
			"\ntransport beyond %1m   patrol pull max %2m   cluster min %3m\ngate defend by manpower %4   completion radius cap %5m",
			m_fTransportDistanceThreshold, m_fMaxPatrolPullDistance,
			m_fMinGroupClusterDistance, m_bGateDefendByManpower, m_fMaxCompletionRadius);

		return a + b + c;
	}

	protected void DrawDebugObjectiveLinks(vector cmdPos, int flags, float now)
	{
		for (int i = 0; i < m_aObjective.Count(); i++)
		{
			CMD_AICommanderObjectiveComponent obj = m_aObjective.Get(i);
			if (!obj || !obj.GetOwner())
				continue;

			vector objPos = obj.GetOwner().GetOrigin();

			int color;
			if (i == 0)
				color = DCO_DebugDraw.COLOR_TARGET();
			else
				color = DCO_DebugDraw.COLOR_QUEUED();

			m_aDebugShapes.Insert(Shape.CreateArrow(
				Vector(cmdPos[0], cmdPos[1] + 2.0, cmdPos[2]),
				Vector(objPos[0], objPos[1] + 2.0, objPos[2]),
				3.0, color, flags));

			float score = obj.ComputePriorityScore(
				m_sFactionKey, now, cmdPos, m_fCombatFocus, m_sCommanderUID);

			bool released;
			if (!m_mAssaultReleased.Find(obj, released))
				released = false;

			string phase;
			if (released)
				phase = "assault released";
			else if (m_mStagingStartTime.Contains(obj))
				phase = "gathering at staging";
			else
				phase = DCO_DebugDraw.ObjectiveStateName(obj.GetObjectiveState(m_sFactionKey));

			m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
				Vector(objPos[0], objPos[1] + 38.0, objPos[2]),
				string.Format("#%1 for %2\nscore %3   %4",
					i + 1, m_sCommanderUID, Math.Round(score), phase),
				18.0, color));

			vector staging;
			if (m_mStagingPos.Find(obj, staging))
			{
				m_aDebugShapes.Insert(Shape.CreateSphere(DCO_DebugDraw.COLOR_STAGING(), flags, staging, 1.5));

				m_aDebugShapes.Insert(Shape.CreateArrow(
					Vector(staging[0], staging[1] + 2.0, staging[2]),
					Vector(objPos[0], objPos[1] + 2.0, objPos[2]),
					2.0, DCO_DebugDraw.COLOR_STAGING(), flags));

				m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
					Vector(staging[0], staging[1] + 6.0, staging[2]),
					string.Format("STAGING\n%1 to objective   %2 from commander",
						DCO_DebugDraw.M(vector.Distance(staging, objPos)),
						DCO_DebugDraw.M(vector.Distance(staging, cmdPos))),
					16.0, DCO_DebugDraw.COLOR_STAGING()));
			}
		}
	}
}
