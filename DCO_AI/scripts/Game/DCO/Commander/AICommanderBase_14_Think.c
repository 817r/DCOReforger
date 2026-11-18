modded class AICommander_BaseComponent
{
	protected void InitializeCommander()
	{
		AICommander_ManagerComponent.GetOrSpawnInstance(m_sManagerPrefab, m_MyEnt);
		if (!AICommander_ManagerComponent.GetInstance()) return;
		AICommander_ManagerComponent.GetInstance().RegisterCommander(this);
		if (Replication.IsServer() && GetGame().GetWorld().GetWorldTime() > 20000.0)
			GetGame().GetCallqueue().CallLater(DCO_GmPlacedSetup, 2500, false);
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
		if (m_bDCOPaused)
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("cmd_paused cmd=%1 by=prefab", m_sCommanderUID));
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("cmd_relentless cmd=%1 trait=%2 from=%3 by_trait=%4 by_attr=%5", m_sCommanderUID, m_fTraitAggression.ToString(-1, 1), DCO_Ctx.TuneForCmd(this, "agg.relentless_from", 1.5).ToString(-1, 1), DCO_Util.B01(DCO_AggressiveCmd()), DCO_Util.B01(DCO_RelentlessAttr())));
	}

	protected FactionKey DCO_PickFactionForNewCommander(AICommander_ManagerComponent mgr)
	{
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld || !m_MyEnt)
			return "";
		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);
		map<string, int> counts = new map<string, int>();
		vector here = m_MyEnt.GetOrigin();
		foreach (AIAgent a : agents)
		{
			SCR_AIGroup grp = SCR_AIGroup.Cast(a);
			if (!grp || !grp.GetFaction() || DCO_Dormant.Count(grp) <= 0)
				continue;
			DCO_GroupUtilityComponent gu = DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));
			if (!gu || gu.IsPlayerGroup() || gu.GetMyCommander())
				continue;
			if (vector.DistanceXZ(grp.GetOrigin(), here) > 2000.0)
				continue;
			string fk = grp.GetFaction().GetFactionKey();
			bool taken = false;
			foreach (AICommander_BaseComponent c : mgr.m_aCommander)
			{
				if (c && c != this && c.GetCommanderFactionKey() == fk)
					taken = true;
			}
			if (taken)
				continue;
			int n;
			counts.Find(fk, n);
			counts.Set(fk, n + 1);
		}
		string best = "";
		int bestN = 0;
		foreach (string k, int v : counts)
		{
			if (v > bestN)
			{
				bestN = v;
				best = k;
			}
		}
		return best;
	}

	protected void DCO_GmPlacedSetup()
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;
		string pick = "-";
		if (m_sFactionKey.IsEmpty() && DCO_Ctx.TuneG("gm.cmd_autofaction", 1.0) > 0)
		{
			FactionKey fk = DCO_PickFactionForNewCommander(mgr);
			if (!fk.IsEmpty())
			{
				SetCommanderFactionKey(fk);
				pick = fk;
			}
		}
		DCO_Perf.Count("cmd:gm_placed");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("cmd_gm_placed cmd=%1 faction=%2 faction_auto=%3 mode=%4", m_sCommanderUID, m_sFactionKey, pick, typename.EnumToString(CMD_ECommanderMode, m_eCommanderMode)));
		if (m_sFactionKey.IsEmpty())
			DCO_PlayerComms.SendToGameMasters("DCO", string.Format("AI Commander %1 placed without a faction: set Faction in its attributes, or it refuses every group.", m_sCommanderUID));
		else if (pick != "-")
			DCO_PlayerComms.SendToGameMasters("DCO", string.Format("AI Commander %1 placed. Faction set to %2 (the nearest AI side without a commander). Give it groups with the group attribute Commander, or place an AI Commander Spawner.", m_sCommanderUID, m_sFactionKey));
		else
			DCO_PlayerComms.SendToGameMasters("DCO", string.Format("AI Commander %1 placed for %2. Give it groups with the group attribute Commander, or place an AI Commander Spawner.", m_sCommanderUID, m_sFactionKey));
		DCO_PublishStatus(mgr);
	}

	protected void DCO_PublishStatus(AICommander_ManagerComponent mgr)
	{
		if (!Replication.IsServer())
			return;
		string mode = typename.EnumToString(CMD_ECommanderMode, m_eCommanderMode);
		mode.ToLower();
		string fk = m_sFactionKey;
		if (fk.IsEmpty())
			fk = "no faction";
		int held = 0;
		if (mgr)
		{
			foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
			{
				if (o && o.GetOwner() && !m_sFactionKey.IsEmpty() && o.IsCapturedBy(m_sFactionKey))
					held++;
			}
		}
		string doing = "no attack planned";
		if (m_Ops && m_Ops.GetMainEffort())
			doing = "attacking " + m_Ops.GetMainEffort().GetDisplayName();
		string status = string.Format("%1 · %2 · %3 · %4 groups · holds %5 objectives · %6", m_sCommanderUID, fk, mode, m_aOwnedGroup.Count(), held, doing);
		if (status == m_sDCORplStatus)
			return;
		m_sDCORplStatus = status;
		Replication.BumpMe();
	}

	protected void ThinkBalanced(AICommander_ManagerComponent mgr, float worldTime)
	{
		bool offenseFirst = DCO_RelentlessCmd();
		if (offenseFirst)
		{
			m_iPhaseBudget = -1;
			ThinkOffensive(mgr, worldTime);
		}

		int defendDemand = CountDefendDemand();
		int idleAvail    = CountIdleCommittableGroups();
		float baseShare = GetDefendShare();
		float share = baseShare;
		int lostRecent = 0;
		for (int li = m_aDCOLostAt.Count() - 1; li >= 0; li--)
		{
			if (worldTime - m_aDCOLostAt[li] > 300.0)
				m_aDCOLostAt.Remove(li);
			else
				lostRecent++;
		}
		float quiet = worldTime - m_fDCOLastThreatAt;
		if (!m_bAllIn)
		{
			share += 0.2 * lostRecent;
			if (lostRecent == 0 && quiet > 180.0)
				share -= 0.2;
			share = Math.Clamp(share, 0.1, 1.0);
		}

		int defendBudget = 0;
		if (defendDemand > 0 && idleAvail > 0)
		{
			defendBudget = Math.Round(1.0 * idleAvail * share);

			if (defendBudget > defendDemand)
				defendBudget = defendDemand;

			if (defendBudget < 1 && !m_bAllIn)
				defendBudget = 1;
		}

		if (worldTime - m_fDCOShareLogAt >= 60.0)
		{
			m_fDCOShareLogAt = worldTime;
			DCO_Perf.Count("balanced:think");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("balanced_share cmd=%1 base=%2 lost_recent=%3 quiet_s=%4 share=%5 demand=%6 idle=%7 budget=%8 offense_first=%9", m_sCommanderUID, baseShare.ToString(-1, 2), lostRecent, Math.Round(quiet), share.ToString(-1, 2), defendDemand, idleAvail, defendBudget, DCO_Util.B01(offenseFirst)));
		}

		m_iPhaseBudget = defendBudget;
		ThinkDefensive(worldTime);

		m_iPhaseBudget = -1;
		if (!offenseFirst)
			ThinkOffensive(mgr, worldTime);
	}

	protected bool m_bDCOThought;
	protected bool m_bDCONoGroupsWarned;

	bool DCO_HasThought()
	{
		return m_bDCOThought;
	}

	protected bool DCO_NoGroupsIdle(AICommander_ManagerComponent mgr, float worldTime)
	{
		int n;
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (g && g.GetOwner())
				n++;
		}
		if (n > 0)
		{
			if (m_bDCONoGroupsWarned)
			{
				m_bDCONoGroupsWarned = false;
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("cmd_groups_back cmd=%1 groups=%2", m_sCommanderUID, n));
			}
			return false;
		}
		DCO_Perf.Count("cmd:no_groups_idle");
		if (!m_bDCONoGroupsWarned)
		{
			m_bDCONoGroupsWarned = true;
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, DCO_Log.IdCmd(this), string.Format("cmd_no_groups cmd=%1 faction=%2 mode=%3 action=idle (the commander has no groups, so it plans nothing until it gets one)", m_sCommanderUID, m_sFactionKey, typename.EnumToString(CMD_ECommanderMode, m_eCommanderMode)));
			DCO_PlayerComms.SendToGameMasters("DCO", string.Format("AI Commander %1 has no groups, so it stays idle. Give it groups with the group attribute Commander, or place an AI Commander Spawner.", m_sCommanderUID));
		}
		m_eCommanderState = CMD_ECommanderState.COMMANDING;
		DCO_PublishStatus(mgr);
		return true;
	}

	protected void Think(float worldTime)
	{
		DCO_ReleaseStaleSbf(worldTime);
		if (!Replication.IsServer())
			return;

		m_eCommanderState = CMD_ECommanderState.PLANNING;

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;

		if (!m_bDCOThought)
			DCO_SetupCheck.CommanderNav(this);
		m_bDCOThought = true;
		if (DCO_NoGroupsIdle(mgr, worldTime))
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

		DCO_PublishStatus(mgr);
		m_eCommanderState = CMD_ECommanderState.COMMANDING;
	}

	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mDCOOppAt = new map<CMD_AICommanderObjectiveComponent, float>();

	protected float DCO_NearestOwnObjDist(AICommander_ManagerComponent mgr, vector pos)
	{
		float best = float.MAX;
		foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
		{
			if (!o || !o.GetOwner() || !o.IsCapturedBy(m_sFactionKey))
				continue;
			float d = vector.DistanceXZ(o.GetOwner().GetOrigin(), pos);
			if (d < best)
				best = d;
		}
		return best;
	}

	protected void DCO_OpportunityGrab(AICommander_ManagerComponent mgr, float worldTime)
	{
		if (!mgr || !m_Ops)
			return;
		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj || !obj.GetOwner() || obj.IsCapturedBy(m_sFactionKey) || !obj.DCO_CanProcess(m_sCommanderUID))
				continue;
			if (HasCommittedGroups(obj) || DCO_RoleBlocksAttack(obj, "opportunity"))
				continue;
			float last;
			if (m_mDCOOppAt.Find(obj, last) && worldTime - last < 300.0)
				continue;
			DCO_ObjectiveIntel intel = m_Ops.GetIntel(obj);
			if (!intel || intel.m_fDefenders > 3.0)
				continue;
			vector op = obj.GetOwner().GetOrigin();
			float ownD = DCO_NearestOwnObjDist(mgr, op);
			if (ownD > 800.0)
				continue;
			DCO_GroupUtilityComponent g = FindBestIdleGroupForTask(DCO_EGroupTask.ATTACK, op);
			if (!g || !g.GetOwner() || g.IsPlayerGroup() || !CanCommitGroup(g))
				continue;
			m_mDCOOppAt.Set(obj, worldTime);
			TacticAssault(g, obj, worldTime);
			DCO_Perf.Count("obj:opportunity");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("obj_opportunity cmd=%1 obj=%2 grp=%3 est=%4 conf=%5 own_d=%6", m_sCommanderUID, obj.GetLogName(), DCO_Log.IdGroup(AIGroup.Cast(g.GetOwner())), intel.m_fDefenders.ToString(-1, 1), intel.m_fConfidence.ToString(-1, 2), Math.Round(ownD)));
			return;
		}
	}

	protected float m_fDCOReserveAsmAt = -1000;

	protected float m_fDCOReserveJobsAt;
	protected float m_fDCOReserveAmbushAt = -1;
	protected ref map<string, float> m_mDCOLinkAt = new map<string, float>();

	protected static string DCO_JobName(int job)
	{
		if (job == 1)
			return "qrf";
		if (job == 2)
			return "link";
		return "none";
	}

	protected bool DCO_ReserveFree(DCO_GroupUtilityComponent g, float worldTime)
	{
		if (!g || !g.GetOwner() || g.IsPlayerGroup() || g.IsMortar() || g.IsDedicatedTransport())
			return false;
		if (g.GetGroupObjective() || g.HasState(DCO_EGroupState.IN_CONTACT))
			return false;
		if (!g.IsAvailableReserve() || !g.CanItHaveOrder() || !g.CanCommanderOverrideRole())
			return false;
		DCO_EGroupTask t = g.GetTask();
		if (t == DCO_EGroupTask.NONE)
			return true;
		if (t != DCO_EGroupTask.PATROL || g.DCO_ReserveJob() == 0)
			return false;
		SCR_AIGroup grp = SCR_AIGroup.Cast(g.GetOwner());
		bool idle = !grp || !grp.GetCurrentWaypoint();
		return idle || worldTime - g.DCO_ReserveJobAt() > 900.0;
	}

	protected bool DCO_LinkPair(vector objPos, float worldTime, out vector a, out vector b, out string key)
	{
		array<CMD_AICommanderObjectiveComponent> own = {};
		foreach (CMD_AICommanderObjectiveComponent o : DCO_AllObjectives())
		{
			if (o && o.GetOwner() && o.IsCapturedBy(m_sFactionKey))
				own.Insert(o);
		}
		if (own.Count() < 2)
			return false;
		float best = float.MAX;
		for (int i = 0; i < own.Count(); i++)
		{
			for (int j = i + 1; j < own.Count(); j++)
			{
				vector pa = own[i].GetOwner().GetOrigin();
				vector pb = own[j].GetOwner().GetOrigin();
				float d = vector.DistanceXZ(pa, pb);
				if (d < 150.0 || d > 2500.0)
					continue;
				string k = own[i].GetLogName() + "|" + own[j].GetLogName();
				float at;
				if (m_mDCOLinkAt.Find(k, at) && worldTime - at < 900.0)
					continue;
				float score = d + vector.DistanceXZ(pa, objPos) * 0.5;
				if (score < best)
				{
					best = score;
					a = pa;
					b = pb;
					key = k;
				}
			}
		}
		return best < float.MAX;
	}

	protected bool DCO_LatestThreat(float now, out vector pos)
	{
		CMD_ThreatResponseComponent tr = GetThreatResponseComponent();
		if (!tr)
			return false;
		float best = -1;
		foreach (CMD_ThreatEntry t : tr.GetThreats())
		{
			if (!t || now - t.m_fLastUpdateTime > 600.0)
				continue;
			if (t.m_fLastUpdateTime > best)
			{
				best = t.m_fLastUpdateTime;
				pos = t.m_vBelievedPos;
			}
		}
		return best >= 0;
	}

	protected CMD_AICommanderObjectiveComponent DCO_OwnObjNearest(vector from)
	{
		CMD_AICommanderObjectiveComponent best;
		float bd = float.MAX;
		foreach (CMD_AICommanderObjectiveComponent o : DCO_AllObjectives())
		{
			if (!o || !o.GetOwner() || !o.IsCapturedBy(m_sFactionKey))
				continue;
			float d = vector.DistanceXZ(o.GetOwner().GetOrigin(), from);
			if (d < bd)
			{
				bd = d;
				best = o;
			}
		}
		return best;
	}

	protected DCO_GroupUtilityComponent DCO_NearestFree(array<DCO_GroupUtilityComponent> free, vector pos, out float dist)
	{
		DCO_GroupUtilityComponent best;
		dist = float.MAX;
		foreach (DCO_GroupUtilityComponent q : free)
		{
			float d = vector.DistanceXZ(q.GetOwner().GetOrigin(), pos);
			if (d < dist)
			{
				dist = d;
				best = q;
			}
		}
		return best;
	}

	protected void DCO_ReserveJobLog(string job, string grp, vector pos, string main, string extra)
	{
		DCO_Perf.Count("reserve:job_" + job);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("reserve_job cmd=%1 grp=%2 job=%3 pos=%4 main=%5 %6", m_sCommanderUID, grp, job, DCO_Util.XZ(pos), main, extra));
	}

	protected void DCO_ReserveJobs(float worldTime, bool defensive)
	{
		int mode = m_iReserveEmployment;
		if (mode <= 0 || !m_Ops || worldTime - m_fDCOReserveJobsAt < 60.0)
			return;
		m_fDCOReserveJobsAt = worldTime;
		CMD_AICommanderObjectiveComponent main;
		vector objPos;
		vector axis;
		vector rear;
		float objR = 100.0;
		string src = "staging";
		if (!defensive)
		{
			main = m_Ops.GetMainEffort();
			if (!main || !main.GetOwner())
				return;
			objPos = main.GetOwner().GetOrigin();
			objR = main.GetRadius();
			rear = TacticStagingPos(main);
			axis = objPos - rear;
		}
		else
		{
			vector threat;
			bool hasThreat = DCO_LatestThreat(worldTime, threat);
			if (hasThreat)
				main = DCO_OwnObjNearest(threat);
			else
				main = DCO_OwnObjNearest(GetOwner().GetOrigin());
			if (!main || !main.GetOwner())
				return;
			objPos = main.GetOwner().GetOrigin();
			objR = main.GetRadius();
			src = "self";
			axis = objPos - GetOwner().GetOrigin();
			if (hasThreat)
			{
				axis = threat - objPos;
				src = "threat";
			}
			axis[1] = 0;
			if (axis.Length() < 1.0)
				axis = Vector(1, 0, 0);
			axis.Normalize();
			rear = DCO_Util.Surface(objPos - axis * (objR + 150.0));
		}
		axis[1] = 0;
		if (axis.Length() < 1.0)
			return;
		axis.Normalize();
		vector side = Vector(-axis[2], 0, axis[0]);
		string mainName = main.GetLogName();
		array<DCO_GroupUtilityComponent> free = {};
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (DCO_ReserveFree(g, worldTime) && DCO_GroupJobs.Gate(this, g, DCO_EGroupJob.RESERVE, "reserve_jobs"))
				free.Insert(g);
		}
		if (free.IsEmpty())
			return;
		int useMax = Math.Ceil(free.Count() * 0.5);
		int used = 0;
		vector qpos = DCO_Util.Surface(rear - axis * 120.0);
		if (defensive)
			qpos = DCO_Util.Surface(objPos - axis * (objR + 60.0));
		float qd;
		DCO_GroupUtilityComponent qrf = DCO_NearestFree(free, qpos, qd);
		if (qrf)
		{
			if (qrf.DCO_ReserveJob() != 1)
			{
				if (qd > 150.0)
					TacticMove(qrf, qpos, worldTime);
				qrf.DCO_SetReserveJob(1, worldTime);
				DCO_ReserveJobLog("qrf", DCO_Log.IdGroup(AIGroup.Cast(qrf.GetOwner())), qpos, mainName, "d=" + Math.Round(qd).ToString());
			}
			free.RemoveItem(qrf);
			used++;
		}
		if (mode == 1)
			return;
		if (m_Defense && used < useMax)
		{
			vector opL = DCO_Util.Surface(rear + side * 300.0);
			vector opR = DCO_Util.Surface(rear - side * 300.0);
			vector watch = objPos;
			if (defensive)
			{
				opL = DCO_Util.Surface(objPos + side * (objR + 200.0));
				opR = DCO_Util.Surface(objPos - side * (objR + 200.0));
				watch = DCO_Util.Surface(objPos + axis * 300.0);
			}
			if (m_Defense.ReserveOP(this, opL, watch, "reserve_flank_l", worldTime))
			{
				used++;
				DCO_ReserveJobLog("op_flank_l", "-", opL, mainName, "src=" + src);
			}
			if (used < useMax && m_Defense.ReserveOP(this, opR, watch, "reserve_flank_r", worldTime))
			{
				used++;
				DCO_ReserveJobLog("op_flank_r", "-", opR, mainName, "src=" + src);
			}
			if (defensive && used < useMax)
			{
				vector rdir = DCO_CommanderOps.RotateXZ(axis, Math.RandomFloat(90.0, 270.0));
				vector opX = DCO_Util.Surface(objPos + rdir * (objR + 200.0));
				if (m_Defense.ReserveOP(this, opX, DCO_Util.Surface(objPos + rdir * 500.0), "reserve_random", worldTime))
				{
					used++;
					DCO_ReserveJobLog("op_random", "-", opX, mainName, "src=random");
				}
			}
		}
		foreach (CMD_AICommanderObjectiveComponent co : DCO_AllObjectives())
		{
			if (used >= useMax || free.IsEmpty())
				break;
			float capAt;
			if (!co || !co.GetOwner() || !m_mDCOCapturedAt.Find(co, capAt) || worldTime - capAt > 600.0)
				continue;
			if (!co.IsCapturedBy(m_sFactionKey) || DCO_DefShortfall.Defenders(this, co) > 0)
				continue;
			float dd;
			DCO_GroupUtilityComponent dg = DCO_NearestFree(free, co.GetOwner().GetOrigin(), dd);
			if (!dg)
				break;
			StartObjectiveGarrison(dg, co, worldTime);
			dg.DCO_SetReserveJob(3, worldTime);
			free.RemoveItem(dg);
			used++;
			DCO_ReserveJobLog("local_defend", DCO_Log.IdGroup(AIGroup.Cast(dg.GetOwner())), co.GetOwner().GetOrigin(), co.GetLogName(), "captured_s=" + Math.Round(worldTime - capAt).ToString());
		}
		foreach (DCO_GroupUtilityComponent lg : free)
		{
			if (used >= useMax)
				break;
			if (lg.DCO_ReserveJob() == 2 && worldTime - lg.DCO_ReserveJobAt() < 900.0)
				continue;
			vector a;
			vector b;
			string key;
			if (!DCO_LinkPair(objPos, worldTime, a, b, key))
				break;
			TacticMove(lg, a, worldTime);
			TacticAppendMove(lg, b, worldTime);
			TacticAppendMove(lg, a, worldTime);
			lg.SetTask(DCO_EGroupTask.PATROL, "reserve_link");
			lg.DCO_SetReserveJob(2, worldTime);
			m_mDCOLinkAt.Set(key, worldTime);
			used++;
			DCO_ReserveJobLog("link", DCO_Log.IdGroup(AIGroup.Cast(lg.GetOwner())), a, mainName, "to=" + DCO_Util.XZ(b) + " pair=" + key);
		}
		if (m_Defense && used < useMax && (m_fDCOReserveAmbushAt < 0 || worldTime - m_fDCOReserveAmbushAt > 900.0))
		{
			vector kill = DCO_Util.Surface(rear + axis * 250.0);
			if (defensive)
				kill = DCO_Util.Surface(objPos + axis * (objR + 250.0));
			if (m_Defense.ReserveAmbush(this, kill, -axis, "reserve_counter", worldTime))
			{
				m_fDCOReserveAmbushAt = worldTime;
				used++;
				DCO_ReserveJobLog("ambush", "-", kill, mainName, "src=" + src);
			}
		}
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("reserve_jobs cmd=%1 mode=%2 defensive=%3 free=%4 used=%5 max=%6 main=%7 src=%8", m_sCommanderUID, mode, DCO_Util.B01(defensive), free.Count(), used, useMax, mainName, src));
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
					m_Tactics.OnAborted(this, cand, "not_assigned");
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
			if (!own.CheckIsItLost(m_sFactionKey) || committed.Contains(own) || !HandleLostObjective(own))
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
		mgr.GetTopObjectivesOffensive(this, objCap + committed.Count() + m_iOffensiveRankExtra, ranked);
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
		DCO_ReserveJobs(worldTime, false);
		DCO_OpportunityGrab(mgr, worldTime);
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
			m_aDCOLostAt.Insert(DCO_Util.NowS());
			DCO_Perf.Count("balanced:lost_note");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("obj_lost_reset cmd=%1 obj=%2 state=%3 lost_recent=%4", m_sCommanderUID, obj.GetLogName(), typename.EnumToString(CMD_EObjectiveState, obj.GetObjectiveState(m_sFactionKey)), m_aDCOLostAt.Count()));
			ReleaseGroupsFromObjective(obj, true);
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

		DCO_QrfTick(allObjs, worldTime);
		DCO_ReserveJobs(worldTime, true);

		if (m_bDebugMode)
		{
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), hasAnyWork.ToString() + " < HAS DEFEND WORK FOR " + m_sCommanderUID + " " + m_sFactionKey);
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("cmd=%1 THINK DEFENSIVE", m_sCommanderUID));
		}
	}
}
