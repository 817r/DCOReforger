modded class AICommander_BaseComponent
{
	protected void InitializeCommander()
	{
		AICommander_ManagerComponent.GetOrSpawnInstance(m_sManagerPrefab, m_MyEnt);
		if (!AICommander_ManagerComponent.GetInstance()) return;
		AICommander_ManagerComponent.GetInstance().RegisterCommander(this);
		threatComp = CMD_ThreatResponseComponent.Cast(m_MyEnt.FindComponent(CMD_ThreatResponseComponent));

		m_eCommanderMode = m_eCommanderModeExternal;
		InitDoctrine(m_sTraitSource);
		m_fBaseThinkInterval             = m_fThinkInterval;
		m_iBaseRetreatThreshold          = m_iRetreatThreshold;
		m_fBaseStalemateResponseCooldown = m_fStalemateResponseCooldown;

		ScaleTimingsByPersonality();
		m_iDCOObjCapInit = m_fObjectiveAtTheSameTime;
		m_fThinkTimer = m_fThinkInterval - m_fDelayFirstIteration;

		m_fThinkTimer = m_fThinkTimer - Math.RandomFloat(0.0, m_fThinkInterval);

		artySupport = CMD_ArtillerySupport.Cast(m_MyEnt.FindComponent(CMD_ArtillerySupport));

		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("commander_init cmd=%1", m_sCommanderUID));
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("think_init cmd=%1 first_think_in=%2 interval=%3", m_sCommanderUID, (m_fThinkInterval - m_fThinkTimer).ToString(-1, 1), m_fThinkInterval));
	}

	protected void ThinkBalanced(AICommander_ManagerComponent mgr, float worldTime)
	{
		int defendDemand = CountDefendDemand();
		int idleAvail    = CountIdleCommittableGroups();

		int defendBudget = 0;
		if (defendDemand > 0 && idleAvail > 0)
		{
			float share = GetDefendShare();
			defendBudget = Math.Round(1.0 * idleAvail * share);

			if (defendBudget > defendDemand)
				defendBudget = defendDemand;

			if (defendBudget < 1 && !m_bAllIn)
				defendBudget = 1;
		}

		m_iPhaseBudget = defendBudget;
		ThinkDefensive(worldTime);

		m_iPhaseBudget = -1;
		ThinkOffensive(mgr, worldTime);
	}

	protected void Think(float worldTime)
	{
		if (!Replication.IsServer())
			return;

		m_eCommanderState = CMD_ECommanderState.PLANNING;

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;

		DCO_TaskNoWpWatch(worldTime);
		DCO_VehOwTick(worldTime);
		DCO_TacticTest.Tick(this, worldTime);
		ReclaimStaleAssignments(worldTime);

		m_iManpowerTotalCache = GetTotalManpower();

		if (m_bAllIn && m_bAllInRearDefenders && m_eCommanderMode != CMD_ECommanderMode.DEFENSIVE)
			ReleaseRearDefenders();

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp)
				continue;

			if (!grp.CanItHaveOrder())
				continue;

			grp.CheckOrderComplete(worldTime);
		}

		m_Frontline.Build(this);

		m_iPhaseBudget = -1;

		switch (m_eCommanderMode)
		{
			case CMD_ECommanderMode.OFFENSIVE:
			{
				ThinkOffensive(mgr, worldTime);
				break;
			}
			case CMD_ECommanderMode.DEFENSIVE:
			{
				ThinkDefensive(worldTime);
				break;
			}
			case CMD_ECommanderMode.BALANCED:
			{
				ThinkBalanced(mgr, worldTime);
				break;
			}
		}

		m_iPhaseBudget = -1;

		m_Frontline.TrySendRecon(this, worldTime);

		SendIdleGroupsToReserve();

		if (m_bPlayerTasking)
			m_PlayerTasking.Update(this);
		else
			m_PlayerTasking.Clear();

		m_eCommanderState = CMD_ECommanderState.COMMANDING;
	}

	protected void ThinkOffensive(AICommander_ManagerComponent mgr, float worldTime)
	{
		DCO_FocusTick(worldTime);
		if (m_DCOFocus && m_bDCOFocusForce && worldTime - DCO_FocusSetAt() > DCO_Ctx.TuneForCmd(this, "obj.focus_giveup_s", 600.0) && !HasCommittedGroups(m_DCOFocus))
			ClearFocus("unreachable", "auto");
		array<CMD_AICommanderObjectiveComponent> committed = {};
		foreach (CMD_AICommanderObjectiveComponent cand : mgr.m_aObjective)
		{
			if (!cand)
				continue;
			if (!cand.DCO_CanProcess(m_sCommanderUID) || DCO_RoleBlocksAttack(cand, "committed") || (!IsAssaultReleased(cand) && DCO_PhaseSkip(cand, "committed")))
			{
				if (!HasCommittedGroups(cand) && !(m_Tactics && m_Tactics.GetPlan(cand)))
					continue;
				DCO_Perf.Count("obj_assign:release");
				if (DCO_RoleOnce(cand.GetLogName() + "|release"))
					DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, DCO_Log.IdCmd(this), string.Format("obj_assign_release cmd=%1 obj=%2 reason=not_assigned_anymore", m_sCommanderUID, cand.GetLogName()));
				if (m_Tactics && m_Tactics.GetPlan(cand))
					m_Tactics.OnAborted(cand, "not_assigned");
				ReleaseGroupsFromObjective(cand);
				continue;
			}
			if (!IsObjectiveCommitted(cand))
				continue;
			committed.Insert(cand);
		}

		array<CMD_AICommanderObjectiveComponent> recapture = {};
		array<float> recaptureDist = {};
		vector basePos = GetOwner().GetOrigin();
		foreach (CMD_AICommanderObjectiveComponent own : mgr.m_aObjective)
		{
			if (!own)
				continue;

			if (own.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
			{
				bool handedOver = DCO_Role(own) == 1 && DCO_RoleSkipDefend(own);
				if (m_eCommanderMode == CMD_ECommanderMode.OFFENSIVE && !handedOver && IsObjectiveContested(own) && !DCO_AssignSkip(own, "defend"))
				{
					DCO_Think(own, -1, "defend", "contested");
					AssignDefendToObjective(own, worldTime);
				}
				continue;
			}

			DCO_RoleForget(own);
			if (!own.CheckIsItLost(m_sFactionKey) || !HandleLostObjective(own) || committed.Contains(own))
				continue;
			if (DCO_AssignSkip(own, "recapture") || DCO_PhaseSkip(own, "recapture"))
				continue;

			float d = vector.DistanceSq(own.GetOwner().GetOrigin(), basePos);
			int at = 0;
			while (at < recaptureDist.Count() && recaptureDist[at] <= d)
				at++;
			recapture.InsertAt(own, at);
			recaptureDist.InsertAt(d, at);
		}

		array<CMD_AICommanderObjectiveComponent> ranked = {};
		int objCap = DCO_ObjectiveCap();
		if (committed.Count() > objCap)
		{
			string overKey = "over|" + committed.Count().ToString() + "|" + objCap.ToString();
			if (overKey != m_sDCOObjOverLog)
			{
				m_sDCOObjOverLog = overKey;
				DCO_Perf.Count("obj_cap:over_committed");
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, DCO_Log.IdCmd(this), string.Format("obj_over_cap cmd=%1 committed=%2 cap=%3 action=keep_committed_no_new", m_sCommanderUID, committed.Count(), objCap));
			}
		}
		mgr.GetTopObjectivesOffensive(this, Math.Max(objCap, m_fObjectiveAtTheSameTime) + committed.Count() + m_iOffensiveRankExtra, ranked);
		for (int ri = ranked.Count() - 1; ri >= 0; ri--)
		{
			if (DCO_RoleBlocksAttack(ranked[ri], "attack"))
				ranked.Remove(ri);
		}
		if (m_DCOFocus && !IsObjectiveOnCooldown(m_DCOFocus) && !DCO_PhaseSkip(m_DCOFocus, "focus"))
		{
			if (!ranked.Contains(m_DCOFocus))
				ranked.InsertAt(m_DCOFocus, 0);
			m_Ops.DCO_ForceMain(m_DCOFocus);
		}
		m_Ops.PlanAxis(this, ranked, worldTime);

		m_aObjective.Clear();
		m_aObjective.InsertAll(committed);
		foreach (CMD_AICommanderObjectiveComponent rc : recapture)
		{
			if (m_aObjective.Count() >= objCap)
				break;
			m_aObjective.Insert(rc);
		}
		foreach (CMD_AICommanderObjectiveComponent r : ranked)
		{
			if (m_aObjective.Count() >= objCap)
				break;
			if (!m_aObjective.Contains(r))
				m_aObjective.Insert(r);
		}
		if (m_DCOFocus && !IsObjectiveOnCooldown(m_DCOFocus) && !DCO_PhaseSkip(m_DCOFocus, "focus"))
		{
			m_aObjective.RemoveItem(m_DCOFocus);
			m_aObjective.InsertAt(m_DCOFocus, 0);
			string pickKey = m_DCOFocus.GetLogName() + "|" + DCO_Util.B01(m_bDCOFocusForce).ToString();
			if (pickKey != m_sDCOFocusPickLog)
			{
				m_sDCOFocusPickLog = pickKey;
				DCO_Perf.Count("obj_focus:pick");
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("obj_focus_pick cmd=%1 obj=%2 force=%3 state=%4", m_sCommanderUID, m_DCOFocus.GetLogName(), DCO_Util.B01(m_bDCOFocusForce), typename.EnumToString(CMD_EObjectiveState, m_DCOFocus.GetObjectiveState(m_sFactionKey))));
			}
		}
		array<CMD_AICommanderObjectiveComponent> keepOps = {};
		foreach (CMD_AICommanderObjectiveComponent ko : m_aObjective)
		{
			if (ko && m_Tactics && m_Tactics.GetPlan(ko) && !m_OpPlanner.Contains(ko))
				keepOps.Insert(ko);
		}
		m_OpPlanner.Apply(this, m_aObjective, worldTime);
		foreach (CMD_AICommanderObjectiveComponent kk : keepOps)
		{
			if (!m_aObjective.Contains(kk) && !m_OpPlanner.Contains(kk))
				m_aObjective.Insert(kk);
		}

		if (m_aObjective.IsEmpty())
		{
			m_eCommanderState = CMD_ECommanderState.IDLE;
			return;
		}

		CMD_ObjectiveContextCache contextCache = mgr.BuildObjectiveContext(m_sFactionKey);

		for (int i = 0; i < m_aObjective.Count(); i++)
		{
			CMD_AICommanderObjectiveComponent obj = m_aObjective[i];
			if (!obj)
				continue;

			int friendlyNear;
			int enemyNear;
			obj.CountNearbyUnitsCached(obj.GetRadius(), m_sFactionKey, friendlyNear, enemyNear);

			if (friendlyNear > 0 && enemyNear >= friendlyNear * m_iLostObjOutnumber && obj.IsCapturedBy(m_sFactionKey))
			{
				DCO_Think(obj, friendlyNear, "skip", "overrun");
				DCO_ObjSkip(obj, "overrun", worldTime);
				continue;
			}

			DCO_Think(obj, friendlyNear, "attack", "ranked");
			AssignRolesToObjective(obj, worldTime, contextCache);
		}

		DCO_AttackWatchdog(worldTime);

		if (m_bDebugMode)
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("cmd=%1 THINK OFFENSIVE", m_sCommanderUID));
	}

	protected void DCO_Think(CMD_AICommanderObjectiveComponent obj, int mine, string decision, string why)
	{
		if (!obj)
			return;
		if (decision != "skip" && !obj.DCO_CanProcess(m_sCommanderUID))
		{
			DCO_Perf.Count("objassign:violation");
			if (DCO_RoleOnce(obj.GetLogName() + "|viol|" + decision))
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, DCO_Log.IdCmd(this), string.Format("obj_assign_violation cmd=%1 obj=%2 decision=%3 why=%4", m_sCommanderUID, obj.GetLogName(), decision, why));
		}
		float est = -1;
		if (m_Ops)
			est = m_Ops.EstimateDefenders(obj);
		string band = "-";
		if (m_DoctrineRt && m_Doctrine)
			band = typename.EnumToString(DCO_EForceBand, m_DoctrineRt.GetBand(obj));
		float ratio = -1;
		if (est > 0 && mine >= 0)
			ratio = 1.0 * mine / est;
		string owner = "other";
		if (obj.IsCapturedBy(m_sFactionKey))
			owner = "self";
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("cmd_think cmd=%1 obj=%2 owner=%3 state=%4 mine=%5 est_enemy=%6 conf=- ratio=%7 band=%8 decision=%9",
			m_sCommanderUID, obj.GetLogName(), owner, typename.EnumToString(CMD_EObjectiveState, obj.GetObjectiveState(m_sFactionKey)), mine, Math.Round(est), ratio.ToString(-1, 2), band, decision) + " why=" + why);
	}

	protected bool HandleLostObjective(CMD_AICommanderObjectiveComponent obj)
	{
		if (obj.GetObjectiveState(m_sFactionKey) != CMD_EObjectiveState.PENDING && obj.CheckAndMarkIfLost(m_sFactionKey))
		{
			ReleaseGroupsFromObjective(obj);
			obj.ResetAssignedGroupCount(m_sFactionKey);
			obj.SetObjectiveState(m_sFactionKey, CMD_EObjectiveState.PENDING);
		}
		return obj.CheckIsItLost(m_sFactionKey);
	}

	override protected void DCO_RoleRelease(CMD_AICommanderObjectiveComponent obj)
	{
		ReleaseGroupsFromObjective(obj);
	}

	protected array<CMD_AICommanderObjectiveComponent> DCO_DefendOrder(notnull array<CMD_AICommanderObjectiveComponent> objs)
	{
		float threatW = DCO_Ctx.TuneFor(GetOwner(), "def.obj_threat_w", 10.0);
		float contestedW = DCO_Ctx.TuneFor(GetOwner(), "def.obj_contested_w", 50.0);
		array<CMD_AICommanderObjectiveComponent> order = {};
		array<float> scores = {};
		string text;
		foreach (CMD_AICommanderObjectiveComponent obj : objs)
		{
			if (!obj)
				continue;
			float score = 0;
			if (obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
			{
				int friendly;
				int enemy;
				obj.CountNearbyUnitsCached(obj.GetThreatRadius(), m_sFactionKey, friendly, enemy, 30.0);
				score = obj.GetBaseValue() * obj.StrategicScale() + enemy * threatW;
				if (IsObjectiveContested(obj))
					score += contestedW;
				text += string.Format(" %1:%2", obj.GetLogName(), Math.Round(score));
			}
			int at = 0;
			while (at < scores.Count() && scores[at] >= score)
				at++;
			order.InsertAt(obj, at);
			scores.InsertAt(score, at);
		}
		string first = "-";
		if (!order.IsEmpty() && order[0])
			first = order[0].GetLogName();
		if (first != m_sDCODefOrderLog)
		{
			m_sDCODefOrderLog = first;
			DCO_Perf.Count("def:obj_order_change");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("def_obj_order cmd=%1 first=%2 scores=%3", m_sCommanderUID, first, text));
		}
		return order;
	}

	protected void ThinkDefensive(float worldTime)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;

		array<CMD_AICommanderObjectiveComponent> allObjs = mgr.m_aObjective;
		if (DCO_Ctx.TuneFor(GetOwner(), "def.obj_priority", 1) > 0)
			allObjs = DCO_DefendOrder(mgr.m_aObjective);

		bool hasAnyWork = false;

		foreach (CMD_AICommanderObjectiveComponent obj : allObjs)
		{
			if (!obj)
				continue;

			if (!obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
			{
				HandleLostObjective(obj);
				continue;
			}
			if (DCO_AssignSkip(obj, "defend") || DCO_RoleSkipDefend(obj))
				continue;

			AssignDefendToObjective(obj, worldTime);
			hasAnyWork = true;
		}

		if (m_bDebugMode)
		{
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), hasAnyWork.ToString() + " < HAS DEFEND WORK FOR " + m_sCommanderUID + " " + m_sFactionKey);
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("cmd=%1 THINK DEFENSIVE", m_sCommanderUID));
		}
	}
}
