modded class AICommander_BaseComponent
{
	void TacticAbort(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		m_bTacticAborting = true;
		AbortUndermannedAttack(obj, worldTime);
		m_bTacticAborting = false;
	}

	void TacticRestart(CMD_AICommanderObjectiveComponent obj)
	{
		ResetSyncState(obj);
		obj.SetObjectiveState(m_sFactionKey, CMD_EObjectiveState.PENDING);
	}

	bool DCO_ReinforceAssault(CMD_AICommanderObjectiveComponent obj, float worldTime, bool force = false)
	{
		if (!obj)
			return false;
		if (DCO_AssignSkip(obj, "reinforce"))
			return false;
		float str = GetAttackStrength(obj);
		float rel;
		if (!m_mDCOReleaseStrength.Find(obj, rel) || rel <= 0)
		{
			m_mDCOReleaseStrength.Set(obj, str);
			rel = str;
		}
		if (!force && str >= rel * m_fReinforceSkipShare && IsAttackRatioMet(obj))
			return false;
		float next;
		if (m_mDCOReinforceAt.Find(obj, next) && worldTime < next)
			return false;
		m_mDCOReinforceAt.Set(obj, worldTime + 60.0);

		DCO_GroupUtilityComponent g = FindBestIdleGroupForTask(DCO_EGroupTask.REINFORCE, obj.GetOwner().GetOrigin(), false, obj);
		if (!g || g.GetGroupObjective() == obj || g.IsInTransport())
			return false;
		bool main = m_Ops && m_Ops.GetMainEffort() == obj;
		if (!m_bAllIn && !main && !CanCommitGroup(g) && !DCO_TacticTest.Forces(this, obj))
			return false;
		TacticAssault(g, obj, worldTime);
		DCO_Perf.Count("attack_reinforce");
		string line = string.Format("attack_reinforce cmd=%1 obj=%2 group=%3 strength=%4 at_release=%5 groups_sent=1 grp_units=%6", m_sCommanderUID, obj.GetLogName(), g.GetOwner().GetName(), str.ToString(-1, 1), rel.ToString(-1, 1), g.GetUnitCount());
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), line);
		return true;
	}

	protected bool DCO_CheckStaleAssigned(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		if (HasCommittedGroups(obj) || IsAssaultReleased(obj) || (m_PlayerTasking && m_PlayerTasking.HasActiveTask(obj)))
		{
			m_mDCOStaleSince.Remove(obj);
			return false;
		}

		float since;
		if (!m_mDCOStaleSince.Find(obj, since))
		{
			m_mDCOStaleSince.Set(obj, worldTime);
			return false;
		}
		if (worldTime - since <= 60.0)
			return false;

		m_mDCOStaleSince.Remove(obj);
		ResetSyncState(obj);
		obj.SetObjectiveState(m_sFactionKey, CMD_EObjectiveState.PENDING);
		DCO_Perf.Count("obj_reset_stale");
		string line = string.Format("obj_reset_stale cmd=%1 obj=%2 idle=%3", m_sCommanderUID, obj.GetLogName(), Math.Round(worldTime - since));
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), line);
		return true;
	}

	protected bool DCO_ForceBegin(CMD_AICommanderObjectiveComponent obj, float worldTime, bool noStandoff)
	{
		ResetSyncState(obj);
		obj.SetObjectiveState(m_sFactionKey, CMD_EObjectiveState.PENDING);
		if (!m_Tactics.Begin(this, obj, worldTime))
			return false;
		if (!m_Tactics.ControlsObjective(obj))
			TrySendToStaging(obj, worldTime);
		obj.MarkAssigned(m_sFactionKey);
		RecordAssignedTime(obj, worldTime);
		return true;
	}

	void TacticEscalate(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		if (obj && !obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
			DCO_ForceBegin(obj, worldTime, true);
	}

	protected void DCO_AttackWatchdog(float worldTime)
	{
		bool active = false;
		foreach (CMD_AICommanderObjectiveComponent o : m_aObjective)
		{
			if (o && (m_Tactics.GetPlan(o) != null || IsAssaultReleased(o) || HasCommittedGroups(o)))
			{
				active = true;
				break;
			}
		}
		if (active || m_fDCOLastAttackActive < 0)
		{
			m_fDCOLastAttackActive = worldTime;
			return;
		}
		if (worldTime - m_fDCOLastAttackActive < m_fAttackWatchdogIdle_s)
			return;

		int idle;
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (g && !g.IsPlayerGroup() && !g.IsDedicatedTransport() && !g.IsMortar() && !g.IsInTransport()
				&& g.IsAvailableReserve() && g.CanItHaveOrder() && CanCommitGroup(g))
				idle++;
		}

		foreach (CMD_AICommanderObjectiveComponent obj : m_aObjective)
		{
			if (!obj || obj.GetObjectiveType() == CMD_EObjectiveType.RECON || obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
				continue;
			if (IsObjectiveOnCooldown(obj))
			{
				DCO_ObjSkip(obj, "cooldown", worldTime);
				continue;
			}
			int need = GetRequiredAttackGroups(obj);
			if (idle * 2 <= need)
			{
				DCO_ObjSkip(obj, "watchdog_few_idle", worldTime);
				continue;
			}

			m_fDCOLastAttackActive = worldTime;
			bool ok = DCO_ForceBegin(obj, worldTime, false);
			DCO_Perf.Count("attack_watchdog");
			string line = string.Format("attack_watchdog cmd=%1 obj=%2 idle=%3 need=%4 started=%5", m_sCommanderUID, obj.GetLogName(), idle, need, ok);
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), line);
			return;
		}
	}

	bool DCO_HasReinforcement(CMD_AICommanderObjectiveComponent obj)
	{
		if (!obj)
			return false;
		DCO_GroupUtilityComponent g = FindBestIdleGroupForTask(DCO_EGroupTask.REINFORCE, obj.GetOwner().GetOrigin(), false, obj);
		return g && g.GetGroupObjective() != obj && !g.IsInTransport();
	}

	protected void AbortUndermannedAttack(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		if (!m_bTacticAborting && DCO_TacticTest.Forces(this, obj))
		{
			bool sent = DCO_ReinforceAssault(obj, worldTime, true);
			DCO_Perf.Count("tactic_persist:undermanned");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("tactic_persist obj=%1 reason=undermanned action=reinforce sent=%2 strength=%3", obj.GetLogName(), DCO_Util.B01(sent), GetAttackStrength(obj)) + m_sDCOAbortExtra);
			return;
		}
		float strengthBefore = GetAttackStrength(obj);
		int released;
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (!g || g.IsPlayerGroup() || g.GetGroupObjective() != obj || !TaskCountsSlot(g.GetTask()))
				continue;
			if (obj.GetCurrentAssignedGroupCount(m_sFactionKey) > 0)
				obj.SetObjectiveGroup(m_sFactionKey, -1);
			if (!g.IsInTransport())
				g.CompleteAllWaypoints();
			g.SetTask(DCO_EGroupTask.NONE, "AICommanderBase:1551");
			g.SetGroupObjective(null);
			released++;
		}
		ResetSyncState(obj);
		m_mAttackCooldown.Set(obj, worldTime + m_fAttackAbortCooldown);
		if (!m_bTacticAborting)
			m_Tactics.OnAborted(obj, "undermanned");

		string line = string.Format("attack_abort cmd=%1 obj=%2 strength=%3 enemy_est=%4 ratio_needed=%5 groups_released=%6",
			m_sCommanderUID, obj.GetLogName(), strengthBefore, EstimateEnemyAt(obj), GetMinAttackRatio().ToString(-1, 1), released) + m_sDCOAbortExtra;
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), line);
	}

	void DCO_TestReplan(CMD_AICommanderObjectiveComponent obj)
	{
		if (!obj)
			return;
		m_mAttackCooldown.Remove(obj);
		if (!obj.IsCapturedBy(m_sFactionKey))
			TacticRestart(obj);
	}

	bool ShiftStaging(CMD_AICommanderObjectiveComponent obj, vector threatPos, float dist, float worldTime)
	{
		vector st;
		if (!m_mStagingPos.Find(obj, st) || IsAssaultReleased(obj))
			return false;

		vector away = st - threatPos;
		away[1] = 0;
		if (away.LengthSq() < 1)
			away = GetOwner().GetOrigin() - st;
		away[1] = 0;
		away.Normalize();
		vector np = st + away * dist;
		np = DCO_Util.Surface(np);
		m_mStagingPos.Set(obj, np);

		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (!g || g.GetGroupObjective() != obj || !TaskCountsSlot(g.GetTask()) || g.IsInTransport() || g.IsPlayerGroup())
				continue;
			g.CompleteAllWaypoints();
			SpawnMoveRoute(g, g.GetOwner().GetOrigin(), np, worldTime);
		}
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("cmd=%1 Staging %2 moved %3 m away from the threat", m_sCommanderUID, obj.GetLogName(), Math.Round(dist)));
		return true;
	}

	bool SendSupportSquad(DCO_GroupUtilityComponent grp, DCO_EGroupTask task, vector movePos, vector suppressPos)
	{
		float now = DCO_Util.NowS();
		grp.CompleteAllWaypoints();
		grp.SetGroupObjective(null);
		grp.SetTask(task, "AICommanderBase:1843");

		if (TryAssignTransport(grp, movePos, now, task))
			return true;

		if (!SpawnMoveRoute(grp, grp.GetOwner().GetOrigin(), movePos, now))
			return false;

		if (task == DCO_EGroupTask.SUPPORT_BY_FIRE && suppressPos != vector.Zero)
		{
			SCR_AIWaypoint wp = SpawnSuppressWP(suppressPos);
			SCR_AIGroup aiGrp = SCR_AIGroup.Cast(grp.GetOwner());
			if (wp && aiGrp)
				aiGrp.AddWaypoint(wp);
		}
		return true;
	}

	void ReleaseAssaultEarly(CMD_AICommanderObjectiveComponent obj)
	{
		if (!obj || IsAssaultReleased(obj) || !m_mStagingStartTime.Contains(obj))
			return;

		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("cmd=%1 Player group entered %2 first -- attack released early", m_sCommanderUID, obj.GetLogName()));
		m_mAssaultReleased.Set(obj, true);
		ReleaseSynchronizedAssault(obj, DCO_Util.NowS());
	}

	protected void DCO_DoctrineStaging(CMD_AICommanderObjectiveComponent obj, int here, int total, float worldTime, string action)
	{
		if (!GetDoctrine())
			return;
		float timeout = -1;
		float start;
		float deadline;
		if (m_mStagingStartTime.Find(obj, start) && m_mSyncDeadline.Find(obj, deadline))
			timeout = deadline - start;
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), "staging cmd=" + m_sCommanderUID + " obj=" + obj.GetLogName() + " groups=" + total.ToString() + " arrived=" + here.ToString() + " timeout_s=" + Math.Round(timeout).ToString() + " action=" + action);
		if (action == "go")
			m_DoctrineRt.GetPlanner().Phase(this, obj, "attack", "staging_go", worldTime);
	}

	protected void DCO_DoctrineRetreat(CMD_AICommanderObjectiveComponent obj, float worldTime, bool fallback)
	{
		array<DCO_GroupUtilityComponent> groups = {};
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (g && !g.IsPlayerGroup() && g.GetGroupObjective() == obj && TaskCountsSlot(g.GetTask()))
				groups.Insert(g);
		}
		vector target;
		bool hasTarget = fallback && (m_DoctrineRt.GetTerrain().OccupiedFor(obj, target) || GetStagingPos(obj, target));
		if (!hasTarget)
		{
			string src;
			target = GetHubFor(obj.GetOwner().GetOrigin(), src);
			hasTarget = target != vector.Zero;
		}
		AbortUndermannedAttack(obj, worldTime);
		int moved;
		if (hasTarget)
		{
			foreach (DCO_GroupUtilityComponent rg : groups)
			{
				if (!rg || rg.IsInTransport())
					continue;
				if (SpawnMoveRoute(rg, rg.GetOwner().GetOrigin(), target, worldTime))
					moved++;
			}
		}
		string action = "withdraw";
		if (fallback)
			action = "fallback";
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), "doctrine_retreat cmd=" + m_sCommanderUID + " obj=" + obj.GetLogName() + " action=" + action + " groups=" + groups.Count().ToString() + " moved=" + moved.ToString() + " target=" + target.ToString());
	}
}
