modded class AICommander_BaseComponent
{
	protected void AssignRolesToObjective(CMD_AICommanderObjectiveComponent obj, float worldTime, CMD_ObjectiveContextCache contextCache = null)
	{
	    if (obj.GetObjectiveType() == CMD_EObjectiveType.RECON)
	    {
	        AssignReconOnlyToObjective(obj, worldTime);
	        return;
	    }

	    CMD_EObjectiveState objState = obj.GetObjectiveState(m_sFactionKey);

	    if (objState == CMD_EObjectiveState.COMPLETED || objState == CMD_EObjectiveState.FAILED)
	    {
	        ResetSyncState(obj);
	        return;
	    }

	    AICommander_ManagerComponent mgrCheck = AICommander_ManagerComponent.GetInstance();
	    int doctrineMask = -1;
	    int doctrineBand = DCO_EForceBand.UNKNOWN;
	    if (objState == CMD_EObjectiveState.PENDING)
	    {
	        ResetSyncState(obj);

	        if (GetDoctrine())
	        {
	            int gate = m_DoctrineRt.GetPlanner().Gate(this, obj, m_DoctrineRt, worldTime, doctrineMask);
	            doctrineBand = m_DoctrineRt.GetBand(obj);
	            if (DCO_IsForcedFocus(obj))
	            {
	                if (gate == DCO_EDoctrineGate.RECON)
	                {
	                    if (m_fDCOFocusReconStart < 0)
	                        m_fDCOFocusReconStart = worldTime;
	                    if (worldTime - m_fDCOFocusReconStart >= DCO_Ctx.TuneForCmd(this, "obj.focus_recon_s", 120.0))
	                    {
	                        DCO_Perf.Count("obj_focus:force_recon_cut");
	                        if (DCO_RoleOnce(obj.GetLogName() + "|force_recon"))
	                            DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("obj_focus_force cmd=%1 obj=%2 gate=recon action=mtc recon_s=%3", m_sCommanderUID, obj.GetLogName(), Math.Round(worldTime - m_fDCOFocusReconStart)));
	                        gate = DCO_EDoctrineGate.MTC;
	                    }
	                }
	                else if (gate == DCO_EDoctrineGate.WAIT || gate == DCO_EDoctrineGate.SKIP)
	                {
	                    m_fDCOFocusReconStart = -1;
	                    DCO_Perf.Count("obj_focus:force_gate");
	                    if (DCO_RoleOnce(obj.GetLogName() + "|force_gate"))
	                        DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("obj_focus_force cmd=%1 obj=%2 gate=%3 action=go", m_sCommanderUID, obj.GetLogName(), typename.EnumToString(DCO_EDoctrineGate, gate)));
	                    gate = DCO_EDoctrineGate.GO;
	                }
	                if (gate == DCO_EDoctrineGate.MTC || (gate == DCO_EDoctrineGate.GO && doctrineBand == DCO_EForceBand.UNKNOWN))
	                {
	                    gate = DCO_EDoctrineGate.MTC;
	                    doctrineMask = DCO_CommanderTactics.Bit(DCO_ETactic.MOVEMENT_TO_CONTACT);
	                }
	            }
	            if (gate == DCO_EDoctrineGate.RECON)
	            {
	                if (!DCO_HasActiveRecon(obj))
	                {
	                    TrySendRecon(obj);
	                    RecordAssignedTime(obj, worldTime);
	                }
	                DCO_GroupUtilityComponent reconNow = obj.GetReconGroup(m_sFactionKey);
	                bool reconExists = reconNow && reconNow.GetOwner() && reconNow.GetGroupObjective() == obj;
	                if (!reconExists && (doctrineBand == DCO_EForceBand.STRONG || doctrineBand == DCO_EForceBand.FAVORABLE))
	                {
	                    m_DoctrineRt.GetPlanner().State(obj).m_bReconDone = true;
	                    DCO_Perf.Count("recon:none_go");
	                    DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("recon_none_go cmd=%1 obj=%2 band=%3 -- no recon group could be sent; band allows the attack without recon", m_sCommanderUID, obj.GetLogName(), DCO_Doctrine.BandName(doctrineBand)));
	                    gate = DCO_EDoctrineGate.GO;
	                }
	                else
	                {
	                    DCO_ObjSkip(obj, "doctrine_recon", worldTime);
	                    return;
	                }
	            }
	            if (gate == DCO_EDoctrineGate.WAIT)
	            {
	                DCO_ObjSkip(obj, "doctrine_wait", worldTime);
	                return;
	            }
	            if (gate == DCO_EDoctrineGate.SKIP)
	            {
	                SetObjectiveCooldown(obj, m_fDoctrineSkipCooldown_s);
	                DCO_ObjSkip(obj, "doctrine_skip", worldTime);
	                return;
	            }
	        }
	        else
	        {
	            bool isFoggy;
	            if (mgrCheck && contextCache)
	                isFoggy = !mgrCheck.IsObjectiveIntelCoveredCached(obj, contextCache);
	            else
	                isFoggy = mgrCheck && !mgrCheck.IsObjectiveIntelCovered(obj, m_sFactionKey);

	            isFoggy = isFoggy && !obj.IsReconArrived(m_sFactionKey, worldTime);

	            if (isFoggy && m_fRiskTaking < Math.RandomFloat01())
	            {
	                if (DCO_HasActiveRecon(obj))
	                {
	                    DCO_ObjSkip(obj, "recon_wait", worldTime);
	                    return;
	                }

	                TrySendRecon(obj);
	                RecordAssignedTime(obj, worldTime);
	                DCO_ObjSkip(obj, "recon_sent", worldTime);
	                return;
	            }
	        }
	    }

	    if (objState == CMD_EObjectiveState.PENDING)
	    {
	        DCO_TacticPlan begun = m_Tactics.Begin(this, obj, worldTime, doctrineMask);
	        if (!begun)
	        {
	            DCO_ObjSkip(obj, "no_tactic", worldTime);
	            return;
	        }
	        int chosenTactic = begun.m_eTactic;
	        string chosenName = DCO_CommanderTactics.Name(chosenTactic);
	        if (!m_Tactics.GetPlan(obj))
	        {
	            DCO_Perf.Count("tactic_plan_lost");
	            DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, DCO_Log.IdCmd(this), "tactic_plan_lost cmd=" + m_sCommanderUID + " obj=" + obj.GetLogName() + " tactic=" + chosenName);
	        }
	        if (GetDoctrine())
	        {
	            float reserveFloor = GetReserveFloor();
	            DCO_BattlePlanner planner = m_DoctrineRt.GetPlanner();
	            if (planner)
	                planner.LogDecision(this, obj, doctrineBand, doctrineMask, chosenName, reserveFloor);
	        }

	        if (m_Tactics.ControlsObjective(obj))
	        {
	            obj.MarkAssigned(m_sFactionKey);
	            RecordAssignedTime(obj, worldTime);
	            return;
	        }

	        if (GetDoctrine())
	        {
	            if (m_DoctrineRt.GetPlanner().UseStaging(this, obj, true))
	                m_DoctrineRt.GetPlanner().Phase(this, obj, "staged_prep", "doctrine", worldTime);
	            else
	                m_DoctrineRt.GetPlanner().Phase(this, obj, "attack", "no_staging", worldTime);
	            TrySendToStaging(obj, worldTime);
	            obj.MarkAssigned(m_sFactionKey);
	            RecordAssignedTime(obj, worldTime);
	            return;
	        }

	        if (DCO_EffAggression() >= Math.RandomFloat01())
	        {
	            TrySendToStaging(obj, worldTime);
	            obj.MarkAssigned(m_sFactionKey);
	            RecordAssignedTime(obj, worldTime);
	            return;
	        }

	        TrySendRecon(obj);
	        TrySendToStaging(obj, worldTime);
	        obj.MarkAssigned(m_sFactionKey);
	        RecordAssignedTime(obj, worldTime);
	        return;
	    }

	    if (objState == CMD_EObjectiveState.ASSIGNED)
	    {
	        if (GetDoctrine())
	        {
	            int retreat = m_DoctrineRt.GetPlanner().CheckAttack(this, obj, m_DoctrineRt, worldTime);
	            if (retreat > 0)
	            {
	                DCO_DoctrineRetreat(obj, worldTime, retreat == 2);
	                return;
	            }
	        }
	        if (IsAssaultReleased(obj))
	            DCO_ReinforceAssault(obj, worldTime);

	        if (m_Tactics.ControlsObjective(obj))
	        {
	            m_mDCOStaleSince.Remove(obj);
	            return;
	        }

	        bool wasReleased;
	        if (m_mAssaultReleased.Find(obj, wasReleased) && wasReleased && !HasCommittedGroups(obj))
	            ResetSyncState(obj);

	        if (DCO_CheckStaleAssigned(obj, worldTime))
	            return;

	        float assignedTime;
	        bool hasAssignedRecord = m_mAssignedTime.Find(obj, assignedTime);
	        bool reconTimedOut = !hasAssignedRecord || (worldTime - assignedTime) > m_fReconWaitTimeout;

	        if (!GetDoctrine() && !reconTimedOut && DCO_EffAggression() < Math.RandomFloat01() && !obj.IsReconArrived(m_sFactionKey, worldTime))
	        {
	            DCO_ObjSkip(obj, "recon_pending", worldTime);
	            return;
	        }

	        if (obj.IsUncontested(m_sFactionKey, worldTime))
	        {
	            bool alreadyReleased;
	            if (!m_mAssaultReleased.Find(obj, alreadyReleased))
	                alreadyReleased = false;

	            if (!alreadyReleased && m_mStagingStartTime.Contains(obj))
	            {
	                ReleaseSynchronizedAssault(obj, worldTime);
	                m_mAssaultReleased.Set(obj, true);
	            }

	            TrySendAssaultWithSlots(obj, worldTime);
	            return;
	        }

	        if (m_DoctrineRt.GetPlanner().UseStaging(this, obj, m_bUseSynchronizedAttack))
	        {
	            bool released;
	            if (!m_mAssaultReleased.Find(obj, released))
	                released = false;

	            if (!released)
	            {
	                if (!m_mStagingStartTime.Contains(obj))
	                    m_mStagingStartTime.Insert(obj, worldTime);

	                int   stagedHere, stagedTotal;
	                float stagedMaxDist;
	                CountStagingGroups(obj, stagedHere, stagedTotal, stagedMaxDist);

	                if (stagedHere > 0 && !m_mFirstArrivalTime.Contains(obj))
	                {
	                    m_mFirstArrivalTime.Insert(obj, worldTime);
	                    DCO_PlayerAwareness.BroadcastOperation(this, obj, "op_assault_soon", DCO_Radio.P("obj", obj.GetDisplayName(), "sec", DCO_Radio.N(m_fSyncAttackMaxWaitTime), "groups", DescribeOperationGroups(obj)));
	                }

	                if (stagedTotal > 0 && !m_mSyncDeadline.Contains(obj))
	                    m_mSyncDeadline.Insert(obj, m_DoctrineRt.GetPlanner().StagingDeadline(this, stagedMaxDist, worldTime, m_fSyncAttackMaxWaitTime));

	                bool timedOut = false;
	                if (m_mFirstArrivalTime.Contains(obj))
	                    timedOut = !GetDoctrine() && (worldTime - m_mFirstArrivalTime.Get(obj)) > m_fSyncAttackMaxWaitTime;
	                if (m_mSyncDeadline.Contains(obj) && worldTime > m_mSyncDeadline.Get(obj))
	                    timedOut = true;
	                if (timedOut && DCO_K2() && !AreStagedGroupsArrived(obj) && m_mFirstArrivalTime.Contains(obj) && worldTime - m_mFirstArrivalTime.Get(obj) <= m_fSyncAttackMaxWaitTime)
	                {
	                    timedOut = false;
	                    if (DCO_RoleOnce(obj.GetLogName() + "|k2go", 60.0))
	                    {
	                        DCO_Perf.Count("agg:k2_go_hold");
	                        DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("k2_go_hold cmd=%1 obj=%2 why=not_arrived waited_s=%3 max_s=%4 -- K2 deadline passed but staged groups not set; waiting for arrival or the normal max wait", m_sCommanderUID, obj.GetLogName(), Math.Round(worldTime - m_mFirstArrivalTime.Get(obj)), Math.Round(m_fSyncAttackMaxWaitTime)));
	                    }
	                }

	                int assignedNow = obj.GetCurrentAssignedGroupCount(m_sFactionKey);
	                bool isFull   = assignedNow >= GetRequiredAttackGroups(obj);
	                if (!isFull && assignedNow > 0 && !FindBestIdleGroupForTask(DCO_EGroupTask.ATTACK, obj.GetOwner().GetOrigin()))
	                {
	                    isFull = true;
	                    DCO_Perf.Count("sync_full_no_spare");
	                }
	                bool arrived  = AreStagedGroupsArrived(obj);
	                bool ratioOk  = IsAttackRatioMet(obj);
	                float deadlineIn = -1;
	                if (m_mSyncDeadline.Contains(obj))
	                    deadlineIn = m_mSyncDeadline.Get(obj) - worldTime;
	                DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("sync_wait cmd=%1 obj=%2 here=%3/%4 maxd=%5 first=%6 deadline_in=%7 timeout=%8 full=%9", m_sCommanderUID, obj.GetLogName(), stagedHere, stagedTotal, Math.Round(stagedMaxDist), m_mFirstArrivalTime.Contains(obj), Math.Round(deadlineIn), timedOut, isFull) + string.Format(" arrived=%1 ratio=%2 contact_here=%3 contact_fwd=%4", arrived, ratioOk, m_iDCOStageContactHere, m_iDCOStageContactFwd) + m_Ops.DCO_ForceText(this, obj));
	                if (m_iDCOStageContactFwd > 0)
	                    DCO_Perf.Count("sync_contact_fwd");
	                if (m_iDCOStageContactHere > 0)
	                    DCO_Perf.Count("sync_contact_here");

	                string fxGo = "";
	                int syncBand = DCO_EForceBand.UNKNOWN;
	                if (GetDoctrine())
	                    syncBand = m_DoctrineRt.GetBand(obj);
	                bool ratioOverride = timedOut && !ratioOk && !fxGo.IsEmpty() && (syncBand == DCO_EForceBand.STRONG || syncBand == DCO_EForceBand.FAVORABLE);

	                if (timedOut && !ratioOk && !ratioOverride)
	                {
	                    string fxText = fxGo;
	                    if (fxText.IsEmpty())
	                        fxText = "none";
	                    m_sDCOAbortExtra = " band=" + DCO_Doctrine.BandName(syncBand) + " fix_go=" + fxText;
	                    DCO_DoctrineStaging(obj, stagedHere, stagedTotal, worldTime, "replan");
	                    AbortUndermannedAttack(obj, worldTime);
	                    m_sDCOAbortExtra = "";
	                    return;
	                }

	                bool tacticReady = !isFull && arrived && ratioOk && m_mFirstArrivalTime.Contains(obj) && worldTime - m_mFirstArrivalTime.Get(obj) > 30.0 && DCO_FlanksReady(obj);
	                if ((isFull && arrived && ratioOk) || tacticReady || timedOut || (!fxGo.IsEmpty() && ratioOk))
	                {
	                    string relWhy = "full";
	                    if (tacticReady)
	                        relWhy = "tactic_ready";
	                    else if (!(isFull && arrived && ratioOk))
	                    {
	                        relWhy = "timeout";
	                        if (ratioOverride)
	                            relWhy = "ratio_override";
	                        else if (!timedOut)
	                            relWhy = fxGo;
	                    }
	                    if (!m_Ops.ReadyToRelease(this, obj, worldTime))
	                    {
	                        DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("sync_hold cmd=%1 obj=%2 by=ops", m_sCommanderUID, obj.GetLogName()));
	                        return;
	                    }

	                    if (m_Tactics.HoldRelease(this, obj, worldTime))
	                    {
	                        DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("sync_hold cmd=%1 obj=%2 by=tactic", m_sCommanderUID, obj.GetLogName()));
	                        return;
	                    }

	                    DCO_Perf.Count("sync_release:" + relWhy);
	                    DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("sync_release cmd=%1 obj=%2 timeout=%3 reason=%4 here=%5/%6", m_sCommanderUID, obj.GetLogName(), timedOut, relWhy, stagedHere, stagedTotal));
	                    DCO_DoctrineStaging(obj, stagedHere, stagedTotal, worldTime, "go");
	                    m_bDCOReleaseAll = relWhy == "fix_ready" || relWhy == "fix_contact" || relWhy == "ratio_override" || m_iDCOStageContactFwd > 0;
	                    ReleaseSynchronizedAssault(obj, worldTime);
	                    m_bDCOReleaseAll = false;

	                    if (!m_mAssaultReleased.Contains(obj))
	                        m_mAssaultReleased.Insert(obj, true);
	                    else
	                        m_mAssaultReleased.Set(obj, true);

	                    return;
	                }

	                TryGatherForSynchronizedAssault(obj, worldTime);

	                return;
	            }
	        }

	        TrySendAssaultWithSlots(obj, worldTime);
	    }
	}

	protected bool DCO_FlanksReady(CMD_AICommanderObjectiveComponent obj)
	{
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (!g || g.GetGroupObjective() != obj || g.GetTask() != DCO_EGroupTask.FLANK)
				continue;
			if (g.IsInTransport() || (g.IsMoving() && !g.HasState(DCO_EGroupState.IN_CONTACT)))
				return false;
		}
		return true;
	}

	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mDCOReconActiveLog;

	protected void AssignReconOnlyToObjective(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
	    DCO_Perf.Count("recon:obj_assign");
	    if (obj.IsReconObjectiveActive(m_sFactionKey))
	    {
	        DCO_Perf.Count("recon:obj_active");
	        if (!m_mDCOReconActiveLog)
	            m_mDCOReconActiveLog = new map<CMD_AICommanderObjectiveComponent, float>();
	        float lastLog;
	        if (!m_mDCOReconActiveLog.Find(obj, lastLog) || worldTime - lastLog > 120.0)
	        {
	            m_mDCOReconActiveLog.Set(obj, worldTime);
	            DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("recon_obj_active cmd=%1 obj=%2 radius=%3", m_sCommanderUID, obj.GetLogName(), Math.Round(obj.GetRadius())));
	        }
	        TryReconRevealEnemies(obj, worldTime);
	        return;
	    }

	    DCO_GroupUtilityComponent cur = obj.GetReconGroup(m_sFactionKey);
	    if (cur && cur.GetOwner() && cur.GetUnitCount() > 0 && cur.GetTask() == DCO_EGroupTask.RECON && cur.GetGroupObjective() == obj)
	    {
	        DCO_Perf.Count("recon:obj_enroute");
	        return;
	    }

	    TrySendRecon(obj);
	}

	protected void TryReconRevealEnemies(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		if (!threatComp)
			return;

		float lastReveal;
		if (m_mLastReconRevealTime.Find(obj, lastReveal))
		{
			if ((worldTime - lastReveal) < m_fReconRevealInterval)
				return;
		}

		int reconFriendlyCount;
		int enemyCount;
		obj.CountNearbyUnitsCached(obj.GetIntelCoverageRadius(), m_sFactionKey, reconFriendlyCount, enemyCount);

		if (!m_mLastReconRevealTime.Contains(obj))
			m_mLastReconRevealTime.Insert(obj, worldTime);
		else
			m_mLastReconRevealTime.Set(obj, worldTime);

		if (enemyCount <= 0)
			return;

		DCO_GroupUtilityComponent reconGrp = obj.GetReconGroup(m_sFactionKey);

		CMD_ContactReport report = new CMD_ContactReport(
			obj.GetOwner().GetOrigin(),
			enemyCount,
			worldTime,
			"RECON:" + obj.GetLogName());

		threatComp.ReceiveContactReport(report, reconGrp);
	}

	protected void DCO_ReconSkip(CMD_AICommanderObjectiveComponent obj, string why)
	{
		DCO_Perf.Count("recon:obj_skip_" + why);
		if (!m_mDCOReconActiveLog)
			m_mDCOReconActiveLog = new map<CMD_AICommanderObjectiveComponent, float>();
		float now = DCO_Util.NowS();
		float last;
		if (m_mDCOReconActiveLog.Find(obj, last) && now - last < 60.0)
			return;
		m_mDCOReconActiveLog.Set(obj, now);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("recon_obj_skip cmd=%1 obj=%2 why=%3", m_sCommanderUID, obj.GetLogName(), why));
	}

	protected void TrySendRecon(CMD_AICommanderObjectiveComponent obj)
	{
		if (DCO_AssignSkip(obj, "recon"))
		{
			DCO_ReconSkip(obj, "assign_rule");
			return;
		}
		int reconN;
		if (obj.GetObjectiveType() == CMD_EObjectiveType.RECON)
			reconN = DCO_Force.Max(this, "recon", 1);
		else
			reconN = DCO_Force.Count(this, "recon", 1);
		if (reconN < 1)
		{
			DCO_ReconSkip(obj, "force_size");
			return;
		}
		DCO_GroupUtilityComponent reconGrp = FindBestIdleGroupForTask(DCO_EGroupTask.RECON, obj.GetOwner().GetOrigin());
		if (!reconGrp)
		{
			DCO_ReconSkip(obj, "no_idle_squad");
			return;
		}

		if (reconGrp.IsPlayerGroup())
		{
			return;
		}

		if (!CanCommitGroup(reconGrp))
		{
			DCO_ReconSkip(obj, "cannot_commit");
			return;
		}

		reconGrp.CompleteAllWaypoints();

		float worldTime = DCO_Util.NowS();

		vector objCenter = obj.GetOwner().GetOrigin();
		float  minDist   = obj.GetRadius() + m_fReconMinStandoff;
	    vector reconPos  = CMD_ReconSpotFinder.FindBestReconSpot(reconGrp.GetOwner().GetOrigin(), objCenter, minDist, minDist + m_fReconBand, m_iReconSpotSamples);
		if (reconPos == vector.Zero)
		{
			DCO_ReconSkip(obj, "no_spot");
			return;
		}

		reconGrp.SetGroupObjective(obj);
 		obj.SetReconGroup(m_sFactionKey, reconGrp);
		if (obj.GetObjectiveType() == CMD_EObjectiveType.RECON)
			obj.MarkAssigned(m_sFactionKey);

		if (TryAssignTransport(reconGrp, reconPos, worldTime, DCO_EGroupTask.RECON))
    		return;

		reconGrp.SetTask(DCO_EGroupTask.RECON, "AICommanderBase:3261");
		SpawnMoveRoute(reconGrp, reconGrp.GetOwner().GetOrigin(), reconPos, worldTime);
		DCO_Perf.Count("recon:obj_send");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("recon_obj_send cmd=%1 grp=%2 obj=%3 spot_from_center=%4 radius=%5", m_sCommanderUID, DCO_Log.IdGroup(AIGroup.Cast(reconGrp.GetOwner())), obj.GetLogName(), Math.Round(vector.DistanceXZ(reconPos, objCenter)), Math.Round(obj.GetRadius())));
	}

	protected vector GetNearestSafePosition(vector fromPos)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();

		array<vector> candidatePositions = new array<vector>();
		if (mgr)
		{
			foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
			{
				if (!obj)
					continue;
				if (!obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
					continue;
				candidatePositions.Insert(obj.GetOwner().GetOrigin());
			}
		}
		if (candidatePositions.IsEmpty())
			candidatePositions.Insert(GetOwner().GetOrigin());

		array<vector> sorted = new array<vector>();
		array<float>  sortedDist = new array<float>();
		foreach (vector cand : candidatePositions)
		{
			float d = vector.DistanceSq(fromPos, cand);
			int insertAt = sorted.Count();
			for (int i = 0; i < sorted.Count(); i++)
			{
				if (d < sortedDist[i])
				{
					insertAt = i;
					break;
				}
			}
			sorted.InsertAt(cand, insertAt);
			sortedDist.InsertAt(d, insertAt);
		}

		int pickIndex = Math.Round((sorted.Count() - 1) * (1.0 - m_fResilience));
		pickIndex = Math.Clamp(pickIndex, 0, sorted.Count() - 1);

		return sorted[pickIndex];
	}

	protected bool IsPositionClusteredWithOwnedGroups(vector pos, DCO_GroupUtilityComponent excludeGroup = null)
	{
		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || grp == excludeGroup)
				continue;

			if (vector.Distance(pos, grp.GetOwner().GetOrigin()) < m_fMinGroupClusterDistance)
				return true;
		}
		return false;
	}

	protected float ComputeReserveFrontlineScore(vector cand, AICommander_ManagerComponent mgr)
	{
	    float nearestNonOwnedDistSq = float.MAX;

	    if (mgr)
	    {
	        foreach (CMD_AICommanderObjectiveComponent fobj : mgr.m_aObjective)
	        {
	            if (!fobj || fobj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
	                continue;

	            float d = vector.DistanceSq(cand, fobj.GetOwner().GetOrigin());
	            if (d < nearestNonOwnedDistSq)
	                nearestNonOwnedDistSq = d;
	        }
	    }

	    if (nearestNonOwnedDistSq == float.MAX)
	        return 0.5;

	    return 1.0 / (1.0 + Math.Sqrt(nearestNonOwnedDistSq) / m_fReserveDistFalloff);
	}

	protected void SendIdleGroupsToReserve()
	{
	    AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
	    if (!mgr)
	        return;

	    array<vector> capturedObjPositions = new array<vector>();
	    foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
	    {
	        if (!obj)
	            continue;

	        if (!obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
	            continue;

	        capturedObjPositions.Insert(obj.GetOwner().GetOrigin());
	    }

	    array<float> capturedFrontlineScore = new array<float>();
	    foreach (vector capPos : capturedObjPositions)
	        capturedFrontlineScore.Insert(ComputeReserveFrontlineScore(capPos, mgr));

	    float worldTime = DCO_Util.NowS();

	    m_Patrol.ResetSmartBudget(m_iPatrolSmartBudget);
	    int patrolMax = DCO_Force.Max(this, "patrol");
	    int patrolNow;
	    foreach (DCO_GroupUtilityComponent pc : m_aOwnedGroup)
	    {
	        if (pc && pc.GetTask() == DCO_EGroupTask.PATROL)
	            patrolNow++;
	    }

	    foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
	    {
	        if (!grp)
	            continue;

	        if (grp.IsMoving())
	            continue;

	        if (grp.IsDedicatedTransport() || grp.IsPlayerGroup())
	            continue;

	        if (grp.IsBreakingContact())
	        {
	            DCO_Perf.Count("patrol:skip_breaking_contact");
	            continue;
	        }
	        if (grp.IsMortar() || !grp.IsAvailableReserve() || !grp.CanPatrol() || !DCO_GroupJobs.Gate(this, grp, 192, "reserve_patrol"))
	            continue;
	        if (grp.IsArmor() && !m_bVehiclePatrol)
	            continue;
	        if (DCO_PlanGuard(grp, "reserve_patrol"))
	            continue;

			if (!grp.CanItHaveOrder())
				continue;
			bool wasPatrol = grp.GetTask() == DCO_EGroupTask.PATROL;
			if (!wasPatrol && patrolNow >= patrolMax)
			{
				DCO_Perf.Count("force:patrol_standby");
				continue;
			}

	        array<vector> candidatePositions = capturedObjPositions;
	        array<float>  candidateFrontline = capturedFrontlineScore;

	        if (capturedObjPositions.IsEmpty())
	        {
	            vector fallbackPos;
	            vector frontline, frontFacing;
	            if (m_Frontline.GetNearestPoint(grp.GetOwner().GetOrigin(), frontline, frontFacing))
	                fallbackPos = frontline;
	            else
	                fallbackPos = grp.GetOwner().GetOrigin();

	            candidatePositions = new array<vector>();
	            candidateFrontline = new array<float>();
	            candidatePositions.Insert(fallbackPos);
	            candidateFrontline.Insert(ComputeReserveFrontlineScore(fallbackPos, mgr));
	        }

	        vector nearestCandidate = candidatePositions[0];
	        float bestCombinedScore = -1.0;

	        int candCount = candidatePositions.Count();
	        for (int c = 0; c < candCount; c++)
	        {
	            vector cand          = candidatePositions[c];
	            float frontlineScore = candidateFrontline[c];

	            float practicalScore = 1.0 / (1.0 + vector.Distance(grp.GetOwner().GetOrigin(), cand) / m_fReserveDistFalloff);

	            float effAggr = DCO_EffAggression();
	            float combined = (effAggr * frontlineScore) + ((1.0 - effAggr) * practicalScore);

	            if (combined > bestCombinedScore)
	            {
	                bestCombinedScore = combined;
	                nearestCandidate  = cand;
	            }
	        }
	        float nearestDistSq = vector.DistanceSq(grp.GetOwner().GetOrigin(), nearestCandidate);

	        if (nearestDistSq > (m_fMaxPatrolPullDistance * m_fMaxPatrolPullDistance))
	            nearestCandidate = grp.GetOwner().GetOrigin();

	        float spreadAngle = Math.RandomFloat(0.0, 360.0) * Math.DEG2RAD;
	        vector frontPt, frontPtFacing;
	        if (m_Frontline.GetNearestPoint(nearestCandidate, frontPt, frontPtFacing))
	            spreadAngle = ApproachAngle(frontPt, nearestCandidate) + Math.RandomFloat(-m_fReserveSpreadHalfArc, m_fReserveSpreadHalfArc) * Math.DEG2RAD;
	        float spreadDist  = Math.RandomFloatInclusive(m_fBaseRadius * m_fReserveSpreadMin, m_fBaseRadius * m_fReserveSpreadMax);
	        vector patrolCenter = nearestCandidate + Vector(Math.Cos(spreadAngle) * spreadDist, 0.0, Math.Sin(spreadAngle) * spreadDist);
	        patrolCenter = DCO_Util.Surface(patrolCenter);

	        EWaterSurfaceType waterType = EWaterSurfaceType.WST_NONE;
	        float lakeArea = 0;
	        float waterY = SCR_WorldTools.GetWaterSurfaceY(null, patrolCenter, waterType, lakeArea);

	        if (patrolCenter[1] < waterY && waterType == EWaterSurfaceType.WST_OCEAN)
	        {
	            patrolCenter = nearestCandidate;
	            patrolCenter = DCO_Util.Surface(patrolCenter);
	        }

	        if (IsPositionClusteredWithOwnedGroups(patrolCenter, grp))
	        {
	            float retryAngle = Math.RandomFloat(0.0, 360.0) * Math.DEG2RAD;
	            float retryDist  = Math.RandomFloatInclusive(m_fBaseRadius * m_fReserveSpreadMin, m_fBaseRadius * m_fReserveSpreadMax);
	            vector retryCenter = nearestCandidate + Vector(Math.Cos(retryAngle) * retryDist, 0.0, Math.Sin(retryAngle) * retryDist);
	            retryCenter = DCO_Util.Surface(retryCenter);

	            if (!IsPositionClusteredWithOwnedGroups(retryCenter, grp))
	                patrolCenter = retryCenter;
	        }

	        IEntity patrolVeh = null;
	        if (m_bVehiclePatrol)
	        {
	            IEntity veh = grp.GetGroupVehicle();
	            if (veh && vector.Distance(veh.GetOrigin(), grp.GetOwner().GetOrigin()) <= m_fPatrolVehicleReach && grp.CanAllFitIn(veh))
	                patrolVeh = veh;
	        }

	        m_Patrol.GenerateRoute(this, grp, patrolCenter, m_fBaseRadius, worldTime, patrolVeh);

	        grp.SetTask(DCO_EGroupTask.PATROL, "AICommanderBase:3913");
	        if (!wasPatrol)
	            patrolNow++;
	    }
	}
}
