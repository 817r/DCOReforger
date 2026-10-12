modded class AICommander_BaseComponent
{
	protected DCO_GroupUtilityComponent FindBestIdleGroupForTask(DCO_EGroupTask task, vector targetPos, bool canTakeDefend = false, CMD_AICommanderObjectiveComponent targetObj = null, int wantedCaps = -1, bool armor = false, int purpose = 0, int jobOverride = 0)
	{
		if (wantedCaps < 0)
			wantedCaps = DefaultCapabilitiesForTask(task);

		if (m_iPhaseBudget == 0)
			return null;

		if (task == DCO_EGroupTask.ATTACK)
			DCO_GroupJobs.CheckStarve(this);
		int jobMask = DCO_GroupJobs.OfTask(task);
		if (jobOverride > 0)
			jobMask = jobOverride;

		array<DCO_EGroupTask> tiers;
		if (armor)
			tiers = {DCO_EGroupTask.NONE, DCO_EGroupTask.PATROL};
		else if (canTakeDefend)
			tiers = {task, DCO_EGroupTask.NONE, DCO_EGroupTask.PATROL, DCO_EGroupTask.RECON, DCO_EGroupTask.REINFORCE, DCO_EGroupTask.DEFEND};
		else
			tiers = {task, DCO_EGroupTask.NONE, DCO_EGroupTask.PATROL, DCO_EGroupTask.RECON, DCO_EGroupTask.REINFORCE};

		int tierCount = tiers.Count();

		array<DCO_GroupUtilityComponent> bestPerTier   = {};
		array<float>                     bestScorePerTier  = {};
		array<float>                     bestDistSqPerTier = {};
		for (int t = 0; t < tierCount; t++)
		{
			bestPerTier.Insert(null);
			bestScorePerTier.Insert(-1000.0);
			bestDistSqPerTier.Insert(-1.0);
		}

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp)
				continue;

			bool isReservePatrol = grp.GetTask() == DCO_EGroupTask.PATROL || (grp.IsArmor() && grp.GetTask() == DCO_EGroupTask.NONE);
			if (grp.IsMoving() && !isReservePatrol)
				continue;

			if (grp.IsDedicatedTransport() || grp.IsMortar())
				continue;

			if (armor != grp.IsArmor())
				continue;

			if (DCO_PlanGuard(grp, "idle_pick"))
				continue;

			if (DCO_MedicTeamBlocked(grp, task, "idle_pick"))
				continue;

			if (!grp.CanCommanderOverrideRole())
				continue;

			if (!grp.CanItHaveOrder())
				continue;

			if (grp.IsPlayerGroup())
				continue;

			if (IsGroupBusy(grp))
				continue;

			if (!DCO_GroupJobs.Gate(this, grp, jobMask, "idle_pick"))
				continue;

			if (task == DCO_EGroupTask.REINFORCE && !DCO_DefendRangeOk(grp, targetPos))
				continue;

			if (task == DCO_EGroupTask.RECON)
			{
				string rx = DCO_ReconExclude(grp);
				if (!rx.IsEmpty())
				{
					DCO_ReconSkip(grp, rx);
					continue;
				}
			}

			if ((purpose == 1 || task == DCO_EGroupTask.REINFORCE) && !grp.CanReinforce())
				continue;
			if (purpose == 2 && !grp.CanSupportPlayers())
				continue;

			if ((task == DCO_EGroupTask.ATTACK || task == DCO_EGroupTask.FLANK) && !IsMoraleFitForAttack(grp))
				continue;

			int tierIdx = tiers.Find(grp.GetTask());
			if (tierIdx < 0)
				continue;

			int unitCount = grp.GetUnitCount();
			float strengthPct = Math.Clamp(1.0 * unitCount / m_fFullSquadSize * 100.0, 0.0, 100.0);

			float score = 0.0;
			switch (task)
			{
				case DCO_EGroupTask.RECON:
				{
					score = m_fTaskScoreBase - Math.AbsFloat(unitCount - m_iReconIdealSize) * m_fReconSizePenalty;
					if (grp.HasMG() || grp.HasAT())
						score -= m_fReconHeavyPenalty;
					string rk = DCO_ReconKind(grp);
					if (rk == "recon_cap")
						score += m_fReconCapBonus;
					else if (rk == "light_inf")
						score += m_fLightInfBonus;
					else if (rk == "armed_vehicle")
						score += m_fArmedVehicleBonus;
					break;
				}
				case DCO_EGroupTask.ATTACK:
					score = strengthPct;
					break;
				case DCO_EGroupTask.FLANK:
					score = m_fTaskScoreBase - Math.AbsFloat(strengthPct - m_fFlankIdealStrength);
					break;
				default:
					score = strengthPct;
					break;
			}

			DCO_EGroupCapability cap = grp.GetCapability();
			if (cap == DCO_EGroupCapability.MOTORIZED && (task == DCO_EGroupTask.REINFORCE || task == DCO_EGroupTask.FLANK))
				score += m_fMobilityBonus;
			else if (cap == DCO_EGroupCapability.MECHANIZED && (task == DCO_EGroupTask.ATTACK || task == DCO_EGroupTask.REINFORCE))
				score += m_fMobilityBonus;

			if ((task == DCO_EGroupTask.ATTACK || task == DCO_EGroupTask.FLANK) && grp.IsFirstToFight() && DCO_FtfFit(grp))
			{
				score += 50.0;
				DCO_Perf.Count("grpjob:first_bonus");
			}
			else if (task == DCO_EGroupTask.REINFORCE && grp.IsFirstToFight() && DCO_FtfFit(grp))
			{
				score += 1.0;
				DCO_Perf.Count("grpjob:first_tiebreak");
			}

			float distSq = vector.DistanceSq(grp.GetOwner().GetOrigin(), targetPos);

			score = score - Math.Min(Math.Sqrt(distSq) / m_fDistPenaltyStep, m_fDistPenaltyMax);
			score = score + CountCapabilityMatches(grp, wantedCaps) * m_fCapabilityMatchBonus;

			bool better = false;

			if (score > bestScorePerTier[tierIdx])
				better = true;
			else if (score == bestScorePerTier[tierIdx]
				&& (bestDistSqPerTier[tierIdx] < 0.0 || distSq < bestDistSqPerTier[tierIdx]))
				better = true;

			if (better)
			{
				bestScorePerTier[tierIdx]  = score;
				bestDistSqPerTier[tierIdx] = distSq;
				bestPerTier[tierIdx]       = grp;
			}
		}

		for (int t = 0; t < tierCount; t++)
		{
			if (bestPerTier[t])
			{
				if (task == DCO_EGroupTask.RECON)
				{
					DCO_Perf.Count("recon_pick:" + DCO_ReconKind(bestPerTier[t]));
					DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("recon_pick cmd=%1 grp=%2 kind=%3 score=%4 units=%5 tier=%6", m_sCommanderUID, bestPerTier[t].GetOwner().GetName(), DCO_ReconKind(bestPerTier[t]), bestScorePerTier[t].ToString(-1, 1), bestPerTier[t].GetUnitCount(), typename.EnumToString(DCO_EGroupTask, tiers[t])));
				}
				if (m_iPhaseBudget > 0)
					m_iPhaseBudget = m_iPhaseBudget - 1;

				return bestPerTier[t];
			}
		}

		float preemptTime = DCO_Util.NowS();

		DCO_GroupUtilityComponent preempted = TryFindPreemptableGroup(task, targetPos, targetObj, preemptTime, armor);
		if (preempted)
		{
			if (m_iPhaseBudget > 0)
				m_iPhaseBudget = m_iPhaseBudget - 1;

			return preempted;
		}

		return null;
	}

	protected bool IsGroupGatheringForAssault(CMD_AICommanderObjectiveComponent curObj)
	{
		if (!curObj)
			return false;

		if (!m_mStagingStartTime.Contains(curObj))
			return false;

		bool released;
		if (m_mAssaultReleased.Find(curObj, released) && released)
			return false;

		return true;
	}

	protected void ReleasePreemptedGroup(DCO_GroupUtilityComponent grp, float worldTime)
	{
		if (!grp)
			return;

		CMD_AICommanderObjectiveComponent oldObj = grp.GetGroupObjective();
		if (oldObj)
		{
			if (TaskCountsSlot(grp.GetTask()) && oldObj.GetCurrentAssignedGroupCount(m_sFactionKey) > 0)
				oldObj.SetObjectiveGroup(m_sFactionKey, -1);

			if (oldObj.GetReconGroup(m_sFactionKey) == grp)
				oldObj.SetReconGroup(m_sFactionKey, null);

			if (oldObj.GetObjectiveState(m_sFactionKey) == CMD_EObjectiveState.ASSIGNED
				&& oldObj.GetCurrentAssignedGroupCount(m_sFactionKey) <= 0)
			{
				oldObj.SetObjectiveState(m_sFactionKey, CMD_EObjectiveState.PENDING);
			}
		}

		grp.CompleteAllWaypoints();
		grp.SetGroupObjective(null);
		grp.SetTask(DCO_EGroupTask.NONE, "AICommanderBase:4238");

		m_mLastPreemptTime.Set(grp, worldTime);
	}

	protected DCO_GroupUtilityComponent TryFindPreemptableGroup(DCO_EGroupTask task, vector targetPos, CMD_AICommanderObjectiveComponent targetObj, float worldTime, bool armor = false)
	{
		if (!m_bEnablePreemption)
			return null;

		if (!targetObj || !targetObj.GetOwner())
			return null;

		float newScore = targetObj.ComputePriorityScore(m_sFactionKey, worldTime, GetOwner().GetOrigin(), m_fCombatFocus, m_sCommanderUID);
		if (newScore <= 0.0)
			return null;

		DCO_GroupUtilityComponent best = null;
		float bestValue  = -1.0;
		float bestDistSq = -1.0;

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || !grp.GetOwner())
				continue;

			if (grp.IsPlayerGroup())
				continue;

			if (grp.IsDedicatedTransport())
				continue;

			if (!grp.CanCommanderOverrideRole())
				continue;

			if (!grp.CanItHaveOrder())
				continue;

			if (DCO_PreemptionUtility.IsHardProtected(grp))
				continue;

			if (armor != grp.IsArmor())
				continue;

			if (grp.IsInContact())
				continue;

			if (!DCO_GroupJobs.Gate(this, grp, DCO_GroupJobs.OfTask(task), "preempt"))
				continue;

			CMD_AICommanderObjectiveComponent curObj = grp.GetGroupObjective();

			if (curObj == targetObj)
				continue;

			if (IsGroupGatheringForAssault(curObj))
				continue;

			float lastPreempt;
			if (m_mLastPreemptTime.Find(grp, lastPreempt) && (worldTime - lastPreempt) < m_fPreemptionCooldown)
				continue;

			float curScore = 0.0;
			if (curObj && curObj.GetOwner())
				curScore = curObj.ComputePriorityScore(m_sFactionKey, worldTime, GetOwner().GetOrigin(), m_fCombatFocus, m_sCommanderUID);

			float taskValue = DCO_PreemptionUtility.ComputeTaskValue(curScore, grp);

			if (!DCO_PreemptionUtility.IsWorthPreempting(newScore, taskValue, m_fPreemptionMargin))
				continue;

			float distSq = vector.DistanceSq(grp.GetOwner().GetOrigin(), targetPos);

			bool better = false;

			if (!best)
				better = true;
			else if (taskValue < bestValue)
				better = true;
			else if (taskValue == bestValue && distSq < bestDistSq)
				better = true;

			if (better)
			{
				best       = grp;
				bestValue  = taskValue;
				bestDistSq = distSq;
			}
		}

		if (!best)
			return null;

		ReleasePreemptedGroup(best, worldTime);
		return best;
	}

	protected int CountDefendDemand()
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return 0;

		int demand = 0;

		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj || !obj.GetOwner())
				continue;

			if (!obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
				continue;

			if (obj.CheckIsItLost(m_sFactionKey))
				continue;

			float radius = obj.GetRadius();
			if (radius <= 0.0)
				continue;

			int cap = obj.GetDefendGroupCount();
			if (cap <= 0)
				cap = 1;

			if (!obj.HasSectorGrid(m_sFactionKey))
			{
				int estimate = DCO_SectorMath.ComputeSectorCount(radius, m_fArcPerSector, GetSectorPersonalityMod(), m_iMinSector, m_iMaxSector);
				if (estimate > cap)
					estimate = cap;
				demand = demand + estimate;
				continue;
			}

			int staffed = obj.GetStaffedSectorCount(m_sFactionKey);
			int missing = cap - staffed;
			if (missing > 0)
				demand = demand + missing;

			array<ref DCO_SectorGarrison> sectors = obj.GetSectorGarrison(m_sFactionKey);
			if (sectors)
			{
				foreach (DCO_SectorGarrison sec : sectors)
				{
					if (sec && sec.NeedsReplenish())
						demand = demand + 1;
				}
			}
		}

		return demand;
	}

	protected int CountIdleCommittableGroups()
	{
		int count = 0;

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || grp.IsPlayerGroup())
				continue;

			if (grp.IsDedicatedTransport() || !grp.CanCommanderOverrideRole() || !grp.CanItHaveOrder())
				continue;

			bool isReservePatrol = grp.GetTask() == DCO_EGroupTask.PATROL || (grp.IsArmor() && grp.GetTask() == DCO_EGroupTask.NONE);
			if (grp.IsMoving() && !isReservePatrol)
				continue;

			if (IsGroupBusy(grp))
				continue;

			count = count + 1;
		}

		return count;
	}

	DCO_GroupUtilityComponent FindBestIdleGroupForTask_Public(DCO_EGroupTask task, vector pos, int wantedCaps = -1, bool armor = false, int purpose = 1)
	{
	    return FindBestIdleGroupForTask(task, pos, false, null, wantedCaps, armor, purpose);
	}

	bool DCO_FtfFit(DCO_GroupUtilityComponent g)
	{
		string why;
		if (g.HasState(DCO_EGroupState.COMBAT_INEFFECTIVE))
			why = "ineffective";
		else if (m_DoctrineRt && m_DoctrineRt.GetSupport() && m_DoctrineRt.GetSupport().IsFatigued(g))
			why = "fatigued";
		else
			return true;
		DCO_Perf.Count("grpjob:first_skip_" + why);
		if (DCO_Log.On(DCO_ELogChannel.CMD, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.DEBUG, "-", string.Format("grp_first_skip cmd=%1 grp=%2 why=%3", GetCommanderUID(), DCO_Log.IdGroup(AIGroup.Cast(g.GetOwner())), why));
		return false;
	}

	bool DCO_DefendRangeOk(DCO_GroupUtilityComponent g, vector target)
	{
		int mask = DCO_GroupJobs.EffMask(g);
		if ((mask & 16) == 0 || (mask & 1) != 0)
			return true;
		vector anchor = g.GetOwner().GetOrigin();
		float reach = 300.0;
		CMD_AICommanderObjectiveComponent obj = g.GetGroupObjective();
		if (obj && obj.GetOwner())
		{
			anchor = obj.GetOwner().GetOrigin();
			reach = reach + obj.GetRadius();
		}
		float d = vector.DistanceXZ(anchor, target);
		if (d <= reach)
			return true;
		DCO_Perf.Count("grpjob:defend_range_skip");
		if (DCO_Log.On(DCO_ELogChannel.CMD, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.DEBUG, "-", string.Format("grp_defend_range_skip cmd=%1 grp=%2 dist=%3 reach=%4", GetCommanderUID(), DCO_Log.IdGroup(AIGroup.Cast(g.GetOwner())), Math.Round(d), Math.Round(reach)));
		return false;
	}
}
