void SCR_AIOnTacticChange(SCR_AIInfoComponent agent, DCO_GroupTactics tactics);
typedef func SCR_AIOnTacticChange;

modded class SCR_AIGroupUtilityComponent
{
	protected float m_fDCOLadderLog_s = -1000.0;
	const float SUPPRESS_MAX_CLUSTER_INFO_AGE_S = 120;
	const float SUPPRESS_OLD_CLUSTER_INFO_AGE_S = 20;
	const float SUPPRESS_MAX_DESTROYED_CLUSTER_INFO_AGE_S = 10;
	const float SUPPRESS_MIN_DIST_TO_CLUSTER_M = 20;
	const float SUPPRESS_MAX_DIST_TO_CLUSTER_M = 1500;


	ref ScriptInvokerBase<SCR_AIOnTacticChange> m_OnTacticsChange = new ScriptInvokerBase<SCR_AIOnTacticChange>();
	protected DCO_GroupUtilityComponent utilDco;
	DCO_GroupUtilityComponent DCO_GetUtil() { return utilDco; }
	protected SCR_AIActionBase m_DCOTraceAct;

	protected void DCO_TraceActivity(SCR_AIActionBase act)
	{
		string wp = "-";
		if (m_Owner.GetCurrentWaypoint())
			wp = m_Owner.GetCurrentWaypoint().ClassName();
		string task = "-";
		if (utilDco)
			task = typename.EnumToString(DCO_EGroupTask, utilDco.GetTask());
		DCO_TestCaseComponent.TraceGroup(m_Owner, m_DCOTraceAct, act, wp, task);
	}

	protected string DCO_TaskPhaseName()
	{
		if (!utilDco)
			return "-";
		return typename.EnumToString(DCO_ETaskPhase, utilDco.GetTaskData().m_ePhase);
	}

	protected float m_fDCOWpIdleSince_ms = -1;
	protected bool m_bDCOWpIdleLogged;

	protected void DCO_CheckWaypointIdle(float now_ms)
	{
		AIWaypoint wp = m_Owner.GetCurrentWaypoint();
		if (!wp || !SCR_AIIdleActivity.Cast(GetCurrentAction()))
		{
			m_fDCOWpIdleSince_ms = -1;
			m_bDCOWpIdleLogged = false;
			return;
		}
		if (m_fDCOWpIdleSince_ms < 0)
		{
			m_fDCOWpIdleSince_ms = now_ms;
			return;
		}
		if (m_bDCOWpIdleLogged || now_ms - m_fDCOWpIdleSince_ms < 15000.0)
			return;
		m_bDCOWpIdleLogged = true;
		string prefab = "-";
		if (wp.GetPrefabData())
			prefab = wp.GetPrefabData().GetPrefabName();
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("wp_idle grp=%1 wp=%2 prefab=%3 dist=%4 phase=%5", m_Owner.GetName(), wp.ClassName(), prefab, Math.Round(vector.Distance(m_Owner.GetOrigin(), wp.GetOrigin())), DCO_TaskPhaseName()));
	}

	protected void DCO_PublishTaskInfo()
	{
		string task = "None";
		vector dest;
		if (m_Owner.GetCurrentWaypoint())
			dest = m_Owner.GetCurrentWaypoint().GetOrigin();
		if (utilDco && utilDco.GetTask() != DCO_EGroupTask.NONE)
		{
			DCO_GroupTask td = utilDco.GetTaskData();
			if (dest == vector.Zero)
				dest = td.m_vTarget;
			task = typename.EnumToString(DCO_EGroupTask, td.m_eType) + " - " + typename.EnumToString(DCO_ETaskPhase, td.m_ePhase);
			if (td.m_Objective)
				task += " (" + td.m_Objective.GetDisplayName() + ")";
		}
		else if (m_Owner.GetCurrentWaypoint())
		{
			string wp = m_Owner.GetCurrentWaypoint().ClassName();
			wp.Replace("SCR_", "");
			wp.Replace("DCO_", "");
			wp.Replace("AIWaypoint", "Move");
			wp.Replace("Waypoint", "");
			task = "Waypoint " + wp;
		}

		string doing = "Idle";
		AIActionBase act = GetCurrentAction();
		if (act)
		{
			doing = act.ClassName();
			doing.Replace("SCR_AI", "");
			doing.Replace("DCO_", "");
			doing.Replace("Activity", "");
			doing.Replace("Behavior", "");
		}

		if (!utilDco || !utilDco.GetMyCommander())
			dest = vector.Zero;

		m_Owner.DCO_SetTaskInfo(task, doing, dest);
	}


	protected DCO_GroupTactics m_eDCOPostureOverride = DCO_GroupTactics.AUTOMATIC;
	protected DCO_GroupTactics m_eDCOPosture = DCO_GroupTactics.BALANCE;
	protected float m_fDCONextPostureEval_ms;
	protected float m_fDCOPostureChanged_ms = -1;
	protected float m_fDCOFirstContact_ms   = -1;
	protected DCO_GroupTactics m_eDCOForced = DCO_GroupTactics.AUTOMATIC;
	protected DCO_GroupTactics m_eDCOLastTable = DCO_GroupTactics.AUTOMATIC;
	protected int m_iDCOLadderBlocked;
	protected string m_sDCOBaseSrc = "table";
	protected float m_fDCOForcedUntil_ms;
	bool m_bDCOForcedUntilContact;
	float m_fDCOContactSeen_ms = -1;
	float m_fDCOLastRatio = 99;
	float m_fDCODelayHold_ms;
	float m_fDCOEvasiveAt_ms = -1;
	bool m_bDCOAmbSet;
	float m_fDCOAmbSprung_ms = -1;
	float m_fDCOAmbDecide_ms;
	float m_fDCOSneakSpotted_ms = -1;
	protected float m_fDCOPostureOpsTick_ms;
	bool m_bDCOPinGroup;
	float m_fDCOPinGroupAt_ms = -1;
	float m_fDCOPinLow_ms = -1;
	float m_fDCOPinSuppress_ms = -1;
	vector m_vDCOPinShooter;
	protected float m_fDCOPinTick_ms;
	protected float m_fDCOStatsTick_ms;
	protected float m_fDCOAwareTick_ms;
	protected bool m_bDCOTimersSeeded;
	float m_fDCOIdfStill_ms = -1;
	float m_fDCOBodySweep_ms;
	protected bool m_bDCORatioLow;
	protected bool m_bDCORatioVeryLow;
	protected bool m_bDCOConverging;
	protected int m_iDCOLastTask = -1;

	protected float m_fDCONoWaypointSince_ms = -1;

	protected DCO_AIGarrisonActivity m_DCOGarrison;

	protected ref DCO_StragglerWatch m_DCOStragglers = new DCO_StragglerWatch();

	ref array<ref DCO_ShareTrack> m_aDCOShareTracks = {};
	ref map<IEntity, float> m_mDCOInjected = new map<IEntity, float>();
	int m_iDCOSharedComp;
	vector m_vDCOSharedPos;
	vector m_vDCOSharedVel;
	float m_fDCOSharedAt = -1;

	bool DCO_IsStuckStraggler(AIAgent agent) { return m_DCOStragglers.IsStuck(agent); }

	DCO_AIGarrisonActivity DCO_GetGarrisonActivity() { return m_DCOGarrison; }
	void DCO_SetGarrisonActivity(DCO_AIGarrisonActivity activity) { m_DCOGarrison = activity; }
	SCR_AIWaypointState DCO_GetWaypointState() { return m_WaypointState; }

	int DCO_GetGarrisonMode()
	{
		if (!m_DCOGarrison || !m_DCOGarrison.IsActive())
			return -1;

		return m_DCOGarrison.GetMode();
	}

	ref SCR_AIGroupPerception GetPercGroupComp()
	{
		return m_Perception;
	}

	override protected void OnCurrentWaypointChanged(AIWaypoint currentWp, AIWaypoint prevWp)
	{
		SCR_BoardingEntityWaypoint board = SCR_BoardingEntityWaypoint.Cast(currentWp);
		IEntity boardVeh;
		if (board)
			boardVeh = board.GetEntity();
		if (boardVeh)
		{
			SCR_AIVehicleUsageComponent usage = SCR_AIVehicleUsageComponent.FindOnNearestParent(boardVeh, boardVeh);
			if (usage && !IsUsableVehicle(usage))
				AddUsableVehicle(usage);
		}
		super.OnCurrentWaypointChanged(currentWp, prevWp);
	}

	override void AddUsableVehicle(notnull SCR_AIVehicleUsageComponent vehicleUsageComp)
	{
		super.AddUsableVehicle(vehicleUsageComp);
		SCR_AIGroup prevOwner = vehicleUsageComp.m_DCOOwnerGroup;
		if (prevOwner == m_Owner || (prevOwner && vehicleUsageComp.DCO_OwnerAlive()))
			return;
		vehicleUsageComp.m_DCOOwnerGroup = m_Owner;
		string fromOwner = "none";
		if (prevOwner)
			fromOwner = DCO_Log.IdGroup(prevOwner) + "_dead";
		DCO_Perf.Count("vehicle_owner_change");
		string veh = "-";
		if (vehicleUsageComp.GetOwner() && vehicleUsageComp.GetOwner().GetPrefabData())
			veh = FilePath.StripExtension(FilePath.StripPath(vehicleUsageComp.GetOwner().GetPrefabData().GetPrefabName()));
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Owner), string.Format("vehicle_owner_change veh=%1 from=%2 to=%3", veh, fromOwner, DCO_Log.IdGroup(m_Owner)));
		TurretCompartmentSlot turret = vehicleUsageComp.GetTurretCompartmentSlot();
		if (turret && !turret.GetOccupant() && !turret.IsCompartmentAccessible())
			turret.SetCompartmentAccessible(true);
	}

	override protected void OnAgentAdded(AIAgent agent)
	{
		super.OnAgentAdded(agent);
		if (agent && m_Owner && Replication.IsServer())
		{
			DCO_GroupUtilityComponent groupLod = DCO_GroupUtilityComponent.Cast(m_Owner.FindComponent(DCO_GroupUtilityComponent));
			if (groupLod)
				groupLod.DCO_ApplyAgentLOD(agent);
		}
		SCR_ChimeraAIAgent chimeraAgent = SCR_ChimeraAIAgent.Cast(agent);
		if (!chimeraAgent)
			return;

		SCR_AIInfoComponent info = chimeraAgent.m_InfoComponent;

		if (!info)
			return;

		info.SetMyGroup(m_Owner);
	}

	EAIGroupCombatMode DCO_GetCombatModeExternal()
	{
		return m_eCombatModeExternal;
	}

	DCO_GroupTactics DCO_GetPosture()
	{
		if (m_eDCOPostureOverride != DCO_GroupTactics.AUTOMATIC)
			return m_eDCOPostureOverride;
		if (m_eDCOForced != DCO_GroupTactics.AUTOMATIC)
			return m_eDCOForced;

		return m_eDCOPosture;
	}

	DCO_GroupTactics DCO_GetPostureOverride()
	{
		return m_eDCOPostureOverride;
	}

	void DCO_SetPostureOverride(DCO_GroupTactics posture)
	{
		DCO_GroupTactics was = DCO_GetPosture();
		m_eDCOPostureOverride = posture;
		DCO_GroupTactics now = DCO_GetPosture();
		if (was != now)
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("posture_change grp=%1 from=%2 to=%3 reason=gm", DCO_Log.IdGroup(m_Owner),
				typename.EnumToString(DCO_GroupTactics, was), typename.EnumToString(DCO_GroupTactics, now)));
		if (posture != DCO_GroupTactics.AUTOMATIC && m_eDCOForced != DCO_GroupTactics.AUTOMATIC)
			DCO_GmWinsLog();
	}

	protected void DCO_GmWinsLog()
	{
		DCO_Perf.Count("pos:gm_wins");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("posture_gm_wins grp=%1 gm=%2 forced=%3", DCO_Log.IdGroup(m_Owner),
			typename.EnumToString(DCO_GroupTactics, m_eDCOPostureOverride), typename.EnumToString(DCO_GroupTactics, m_eDCOForced)));
	}

	void DCO_ForcePosture(DCO_GroupTactics posture, float now_ms, float dur_ms, bool untilContact, string why)
	{
		DCO_Perf.Count("posture_change:" + typename.EnumToString(DCO_GroupTactics, posture));
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("posture_change grp=%1 from=%2 to=%3 reason=forced:%4", DCO_Log.IdGroup(m_Owner),
			typename.EnumToString(DCO_GroupTactics, DCO_GetPosture()), typename.EnumToString(DCO_GroupTactics, posture), why));
		m_eDCOForced = posture;
		if (m_eDCOPostureOverride != DCO_GroupTactics.AUTOMATIC)
			DCO_GmWinsLog();
		m_fDCOForcedUntil_ms = now_ms + dur_ms;
		m_bDCOForcedUntilContact = untilContact;
		m_fDCOPostureChanged_ms = now_ms;
	}

	void DCO_ClearForced(string why)
	{
		if (m_eDCOForced == DCO_GroupTactics.AUTOMATIC)
			return;
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("posture_forced_end grp=%1 was=%2 reason=%3", DCO_Log.IdGroup(m_Owner), typename.EnumToString(DCO_GroupTactics, m_eDCOForced), why));
		m_eDCOForced = DCO_GroupTactics.AUTOMATIC;
		m_bDCOForcedUntilContact = false;
	}

	bool DCO_IsDirectlyThreatened()
	{
		return IsDirectlyThreatened();
	}

	override SCR_AIActionBase EvaluateActivity(out bool restartActivity)
	{
		SCR_AIActionBase activity = super.EvaluateActivity(restartActivity);
		if (!m_Perception)
			return activity;

		float now_ms = GetGame().GetWorld().GetWorldTime();
		if (!m_bDCOTimersSeeded)
		{
			m_bDCOTimersSeeded = true;
			m_fDCONextPostureEval_ms = Math.Max(m_fDCONextPostureEval_ms, now_ms + Math.RandomFloat(0, 3000.0));
			m_fDCOPostureOpsTick_ms = Math.Max(m_fDCOPostureOpsTick_ms, now_ms + Math.RandomFloat(0, 2000.0));
			m_fDCOPinTick_ms = Math.Max(m_fDCOPinTick_ms, now_ms + Math.RandomFloat(0, 1500.0));
			m_fDCOStatsTick_ms = Math.Max(m_fDCOStatsTick_ms, now_ms + Math.RandomFloat(0, 2000.0));
			m_fDCOAwareTick_ms = Math.Max(m_fDCOAwareTick_ms, now_ms + Math.RandomFloat(0, 3000.0));
			m_fDCONextSecurity_ms = Math.Max(m_fDCONextSecurity_ms, now_ms + Math.RandomFloat(0, 3000.0));
			m_fDCOArmorReact_ms = Math.Max(m_fDCOArmorReact_ms, now_ms + Math.RandomFloat(0, 2000.0));
			m_fDCOMountedTick_ms = Math.Max(m_fDCOMountedTick_ms, now_ms + Math.RandomFloat(0, 2000.0));
			m_fDCOPairTick_ms = Math.Max(m_fDCOPairTick_ms, now_ms + Math.RandomFloat(0, 4000.0));
			m_fDCOCETick_ms = Math.Max(m_fDCOCETick_ms, now_ms + Math.RandomFloat(0, 1000.0));
			m_fDCOSOPCheck_ms = Math.Max(m_fDCOSOPCheck_ms, now_ms + Math.RandomFloat(0, 2000.0));
			m_fDCOAmbushCheck_ms = Math.Max(m_fDCOAmbushCheck_ms, now_ms + Math.RandomFloat(0, 2000.0));
			m_fDCORetreatCheck_ms = Math.Max(m_fDCORetreatCheck_ms, now_ms + Math.RandomFloat(0, 2000.0));
			m_fDCOCmdLossCheck_ms = Math.Max(m_fDCOCmdLossCheck_ms, now_ms + Math.RandomFloat(0, 2000.0));
			m_fDCONextOvermatch_ms = Math.Max(m_fDCONextOvermatch_ms, now_ms + Math.RandomFloat(0, 1000.0));
			m_fDCONextCQB_ms = Math.Max(m_fDCONextCQB_ms, now_ms + Math.RandomFloat(0, 2000.0));
			m_fDCOPlanCheck_ms = Math.Max(m_fDCOPlanCheck_ms, now_ms + Math.RandomFloat(0, 5000.0));
			m_fDCOAntiCampTick_ms = Math.Max(m_fDCOAntiCampTick_ms, now_ms + Math.RandomFloat(0, 2000.0));
			m_fDCONextNight_ms = Math.Max(m_fDCONextNight_ms, now_ms + Math.RandomFloat(0, 2000.0));
			DCO_Perf.Count("grp:timer_seed");
		}

		if (m_Owner.IsSlave() || m_Owner.GetCurrentWaypoint())
			m_fDCONoWaypointSince_ms = -1;
		else if (m_fDCONoWaypointSince_ms < 0)
			m_fDCONoWaypointSince_ms = now_ms;

		if (now_ms >= m_fDCONextPostureEval_ms)
		{
			m_fDCONextPostureEval_ms = now_ms + 3000.0;
			int st = DCO_Perf.Begin();
			DCO_UpdatePosture(now_ms);
			DCO_AssaultTick(now_ms);
			DCO_Perf.End("grp_posture", st);
		}
		if (now_ms >= m_fDCOPostureOpsTick_ms && Replication.IsServer())
		{
			m_fDCOPostureOpsTick_ms = now_ms + 2000.0;
			int poT = DCO_Perf.Begin();
			DCO_PostureOps.Tick(this, m_Owner, utilDco, now_ms);
			DCO_Perf.End("grp_posture_ops", poT);
		}
		if (now_ms >= m_fDCOPinTick_ms && Replication.IsServer())
		{
			m_fDCOPinTick_ms = now_ms + 1500.0;
			int pinT = DCO_Perf.Begin();
			DCO_PinnedGroup.Tick(this, m_Owner, utilDco, now_ms);
			DCO_Perf.End("grp_pinned", pinT);
		}
		DCO_TickFlush(now_ms);
		DCO_TickSmoke(now_ms);
		DCO_TickLostGap(now_ms);
		if (now_ms >= m_fDCOStatsTick_ms && Replication.IsServer())
		{
			m_fDCOStatsTick_ms = now_ms + 2000.0;
			m_Owner.DCO_UpdateStats();
			DCO_PublishTaskInfo();
			DCO_CheckWaypointIdle(now_ms);
		}
		if (now_ms >= m_fDCOAwareTick_ms && Replication.IsServer())
		{
			m_fDCOAwareTick_ms = now_ms + 3000.0;
			int awT = DCO_Perf.Begin();
			DCO_Awareness.GroupTick(this, m_Owner, utilDco, now_ms);
			DCO_Statics.GroupTick(this, m_Owner, utilDco);
			DCO_Perf.End("grp_awareness", awT);
		}

		SCR_AIGroup dcoFeatGrp = SCR_AIGroup.Cast(m_Owner);
		if (!dcoFeatGrp || dcoFeatGrp.DCO_FeatureOn(DCO_EGroupFeature.OVERMATCH))
		{
			int ot = DCO_Perf.Begin();
			DCO_UpdateOvermatch(now_ms);
			DCO_Perf.End("grp_overmatch", ot);
		}

		int ct = DCO_Perf.Begin();
		DCO_UpdateCQB(now_ms);
		if (!dcoFeatGrp || dcoFeatGrp.DCO_FeatureOn(DCO_EGroupFeature.CQB))
		{
			int acT = DCO_Perf.Begin();
			DCO_UpdateAntiCamp(now_ms);
			DCO_Perf.End("cqb_anticamp", acT);
		}
		DCO_Perf.End("grp_cqb", ct);

		int nt = DCO_Perf.Begin();
		if (!dcoFeatGrp || dcoFeatGrp.DCO_FeatureOn(DCO_EGroupFeature.NIGHT))
			DCO_UpdateNight(now_ms);
		DCO_Perf.End("grp_night", nt);

		DCO_UpdateSecurity360(now_ms);
		DCO_UpdateHearing(now_ms);
		DCO_UpdateCommandLoss(now_ms);
		DCO_UpdateRetreatWhen(now_ms);
		DCO_UpdateAmbush(now_ms);
		if (utilDco && now_ms >= m_fDCOMountedTick_ms && Replication.IsServer())
		{
			m_fDCOMountedTick_ms = now_ms + 2000.0;
			int mT = DCO_Perf.Begin();
			DCO_MountedOps.Tick(utilDco, m_Owner, this);
			DCO_Perf.End("grp_mounted", mT);
		}
		if ((m_bDCOCommanderEval || m_bDCOCEWas) && now_ms >= m_fDCOCETick_ms && Replication.IsServer())
		{
			m_fDCOCETick_ms = now_ms + 1000.0;
			int ceT = DCO_Perf.Begin();
			DCO_CommanderEval.Tick(this, m_Owner, utilDco, m_bDCOCommanderEval);
			m_bDCOCEWas = m_bDCOCommanderEval;
			DCO_Perf.End("grp_commander_eval", ceT);
		}
		if (utilDco && now_ms >= m_fDCOPairTick_ms && Replication.IsServer())
		{
			m_fDCOPairTick_ms = now_ms + 4000.0;
			int pT = DCO_Perf.Begin();
			DCO_ArmorPair.Tick(utilDco, m_Owner, this);
			DCO_Perf.End("grp_armor_pair", pT);
		}
		if (utilDco && now_ms >= m_fDCOArmorReact_ms && Replication.IsServer())
		{
			int aaT = DCO_Perf.Begin();
			if (DCO_AntiArmor.Update(this, m_Owner, utilDco))
				m_fDCOArmorReact_ms = now_ms + 30000.0;
			else
				m_fDCOArmorReact_ms = now_ms + 2000.0;
			DCO_Perf.End("grp_anti_armor", aaT);
		}
		if (utilDco && now_ms >= m_fDCOSOPCheck_ms && Replication.IsServer())
		{
			m_fDCOSOPCheck_ms = now_ms + 2000.0;
			int sopT = DCO_Perf.Begin();
			DCO_ContactSOP.Update(this, utilDco, IsDirectlyThreatened() || utilDco.HasState(DCO_EGroupState.SUPPRESSED));
			DCO_Perf.End("grp_contact_sop", sopT);
		}

		foreach (SCR_AIGroupTargetCluster c : m_Perception.m_aTargetClusters)
		{
			if (!c.m_State)
				continue;

			SCR_AIInvestigateClusterActivity investigate = SCR_AIInvestigateClusterActivity.Cast(c.m_State.m_Activity);
			if (investigate)
				investigate.DCO_Update(now_ms);
		}

		if (m_DCOGarrison)
			m_DCOGarrison.Update(now_ms);

		int pt = DCO_Perf.Begin();
		m_DCOStragglers.Update(now_ms, m_Owner, DCO_GetGarrisonMode() >= 0);
		DCO_Perf.End("grp_straggler", pt);

		if (activity != m_DCOTraceAct)
		{
			if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.TRACE))
				DCO_TraceActivity(activity);
			m_DCOTraceAct = activity;
		}
		return activity;
	}

	protected float m_fDCONextSecurity_ms;

	protected void DCO_UpdateSecurity360(float now_ms)
	{
		if (now_ms < m_fDCONextSecurity_ms)
			return;
		m_fDCONextSecurity_ms = now_ms + 3000.0;
		if (!DCO_GlobalAIComponent.BehaviorOnFor(m_Owner, DCO_EBehavior.SECURITY_360) || !m_Perception.m_aTargetClusters.IsEmpty() || DCO_GetGarrisonMode() >= 0)
			return;

		IEntity leader = m_Owner.GetLeaderEntity();
		if (!leader || SCR_CharacterHelper.IsPlayer(leader))
			return;
		if (leader.GetPhysics() && leader.GetPhysics().GetVelocity().Length() > 0.3)
			return;

		int perfStart = DCO_Perf.Begin();
		vector front = leader.GetWorldTransformAxis(2);
		front[1] = 0;
		if (front.LengthSq() < 0.01)
			front = "0 0 1";
		front.Normalize();
		float baseYaw = Math.Atan2(front[0], front[2]) * Math.RAD2DEG;

		array<AIAgent> agents = {};
		m_Owner.GetAgents(agents);
		int slot = 0;
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent || !ca.m_UtilityComponent.m_LookAction)
				continue;
			IEntity ent = ca.GetControlledEntity();
			if (!ent || !DCO_Util.IsAlive(ent))
				continue;

			float sector = 0;
			if (ent != leader)
			{
				int s = slot % 3;
				if (s == 0)
					sector = -90;
				else if (s == 1)
					sector = 90;
				else
					sector = 180;
				slot++;
			}

			float yaw = (baseYaw + sector + Math.RandomFloat(-45, 45)) * Math.DEG2RAD;
			vector look = ent.GetOrigin() + Vector(Math.Sin(yaw) * 30.0, 1.6, Math.Cos(yaw) * 30.0);
			ca.m_UtilityComponent.m_LookAction.LookAt(look, 5.0, 3.0);
		}
		DCO_Perf.End("grp_security360", perfStart);
	}


	protected float m_fDCONextHearing_ms = -1;
	protected IEntity m_DCOHeardSource;
	protected float m_fDCOHeardAt_ms;
	protected ref DCO_VoiceState m_DCOVoice;
	protected float m_fDCOArmorReact_ms;
	protected float m_fDCOMountedTick_ms;
	protected float m_fDCOPairTick_ms;
	protected bool m_bDCOCommanderEval;
	protected bool m_bDCOCEWas;
	protected float m_fDCOCETick_ms;

	bool DCO_IsCommanderEval()
	{
		return m_bDCOCommanderEval;
	}

	void DCO_SetCommanderEval(bool on)
	{
		if (on)
			SetCombatMode(EAIGroupCombatMode.FIRE_AT_WILL);
		m_bDCOCommanderEval = on;
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("combat_mode_ce grp=%1 on=%2", m_Owner, on));
	}

	protected void DCO_UpdateHearing(float now_ms)
	{
		if (m_fDCONextHearing_ms < 0)
			m_fDCONextHearing_ms = now_ms + Math.RandomFloat(0, 1000);
		if (now_ms < m_fDCONextHearing_ms)
			return;
		bool contact = !m_Perception.m_aTargetClusters.IsEmpty();
		bool hearContact = DCO_Hearing.V2() && DCO_Ctx.TuneFor(m_Owner, "hear.contact", 0.0) > 0;
		if (!DCO_GlobalAIComponent.BehaviorOnFor(m_Owner, DCO_EBehavior.HEARING) || (contact && !hearContact && !DCO_GlobalAIComponent.BehaviorOnFor(m_Owner, DCO_EBehavior.PLAYER_VOICE)) || !Replication.IsServer())
		{
			m_fDCONextHearing_ms = now_ms + 1000.0;
			return;
		}
		if (!DCO_Hearing.TakeBudget())
			return;
		m_fDCONextHearing_ms = now_ms + 1000.0;

		IEntity leader = m_Owner.GetLeaderEntity();
		if (!leader || SCR_CharacterHelper.IsPlayer(leader))
			return;

		int perfStart = DCO_Perf.Begin();
		IEntity src;
		vector pos;
		Faction f;
		bool heard;
		if (DCO_Hearing.V2())
			heard = (!contact || hearContact) && DCO_HearV2(now_ms, contact);
		else if (!contact && DCO_Hearing.Listen(m_Owner, src, pos, f) && !(src && src == m_DCOHeardSource && now_ms - m_fDCOHeardAt_ms < 15000.0))
		{
			heard = true;
			m_DCOHeardSource = src;
			m_fDCOHeardAt_ms = now_ms;
			DCO_LookAtAll(pos + Vector(0, 1.2, 0), 40.0, 2.5);
			DCO_Perf.Count("ev:heard");
			if (src && !m_Owner.GetCurrentWaypoint() && (!utilDco || utilDco.GetTask() == DCO_EGroupTask.NONE))
			{
				PerceptionManager pm = GetGame().GetPerceptionManager();
				if (pm)
					m_Perception.AddOrUpdateGunshot(src, pos, f, pm.GetTime(), false);
			}
		}
		if (!heard && DCO_GlobalAIComponent.BehaviorOnFor(m_Owner, DCO_EBehavior.PLAYER_VOICE))
		{
			int voiceT = DCO_Perf.Begin();
			IEntity vsrc;
			IEntity vlistener;
			vector vpos;
			Faction vf;
			bool vbearing;
			float vnoise;
			if (DCO_Hearing.ListenVoice(m_Owner, contact, vsrc, vpos, vf, vbearing, vnoise, vlistener))
			{
				m_DCOHeardSource = vsrc;
				m_fDCOHeardAt_ms = now_ms;
				DCO_LookAtAll(vpos + Vector(0, 1.2, 0), 40.0, 2.5);
				PerceptionManager vpm = GetGame().GetPerceptionManager();
				if (vpm)
					m_Perception.AddOrUpdateGunshot(vsrc, vpos, vf, vpm.GetTime(), false);
				if (!m_DCOVoice)
					m_DCOVoice = new DCO_VoiceState();
				DCO_VoiceReact.OnHeard(this, m_Owner, m_DCOVoice, vsrc, vpos, vf, vbearing, vnoise, vlistener, contact, now_ms);
				float vd = 0;
				if (vlistener)
					vd = vector.DistanceXZ(vlistener.GetOrigin(), vpos);
				DCO_HeardEst.Report(m_Owner, vpos, Math.Max(vnoise, 3.0), 0.7, 4, vlistener, vd, "-", now_ms);
			}
			DCO_Perf.End("grp_hear_voice", voiceT);
		}
		DCO_Perf.End("grp_hearing", perfStart);
	}

	protected int m_iDCOHearScans;
	protected int m_iDCOHearHeard;
	protected int m_iDCOHearSkipKnown;
	protected float m_fDCOHearGapMin = 100000.0;
	protected float m_fDCOHearMaskMin = 1.0;
	protected float m_fDCOHearLogAt_ms;

	protected void DCO_HearSummary(float now_ms, bool contact)
	{
		if (now_ms - m_fDCOHearLogAt_ms < 30000.0)
			return;
		if (m_fDCOHearLogAt_ms > 0)
		{
			string gap = "none";
			if (m_fDCOHearGapMin < 99999.0)
				gap = Math.Round(m_fDCOHearGapMin).ToString();
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Owner), string.Format("hear_scan grp=%1 scans=%2 heard=%3 skip_known=%4 nearest_gap_m=%5 mask_min=%6 snapshot=%7 contact=%8", DCO_Log.IdGroup(m_Owner), m_iDCOHearScans, m_iDCOHearHeard, m_iDCOHearSkipKnown, gap, m_fDCOHearMaskMin.ToString(-1, 2), DCO_Hearing.SnapCount(), DCO_Util.B01(contact)));
		}
		m_fDCOHearLogAt_ms = now_ms;
		m_iDCOHearScans = 0;
		m_iDCOHearHeard = 0;
		m_iDCOHearSkipKnown = 0;
		m_fDCOHearGapMin = 100000.0;
		m_fDCOHearMaskMin = 1.0;
	}

	protected bool DCO_HearKnown(IEntity src, float pmNow)
	{
		if (!src)
			return false;
		int idx = m_Perception.m_aTargetEntities.Find(src);
		return idx >= 0 && pmNow - m_Perception.m_aTargets[idx].m_fTimestamp < 10.0;
	}

	protected bool DCO_HearV2(float now_ms, bool contact)
	{
		IEntity leader = m_Owner.GetLeaderEntity();
		Faction mine = DCO_Hearing.Fac(leader);
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!mine || !pm)
			return false;
		float pmNow = pm.GetTime();
		array<AIAgent> agents = {};
		m_Owner.GetAgents(agents);
		IEntity bestSrc;
		IEntity bestListener;
		SCR_AIUtilityComponent bestU;
		vector bestPos;
		Faction bestF;
		int bestType;
		float bestDist;
		float bestRange;
		float bestScore = 1.0;
		string bestMask = "-";
		int listeners;
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent)
				continue;
			IEntity me = ca.GetControlledEntity();
			if (!me || !DCO_Util.IsAlive(me) || SCR_CharacterHelper.IsPlayer(me))
				continue;
			listeners++;
			string mask;
			float m = DCO_Hearing.Mask(ca.m_UtilityComponent, me, mask);
			IEntity s;
			vector p;
			Faction fa;
			int t;
			float d;
			float r;
			float gap;
			m_iDCOHearScans++;
			m_fDCOHearMaskMin = Math.Min(m_fDCOHearMaskMin, m);
			bool got = DCO_Hearing.ListenAt(me, mine, m, s, p, fa, t, d, r, gap);
			m_fDCOHearGapMin = Math.Min(m_fDCOHearGapMin, gap);
			if (!got)
				continue;
			if (contact && DCO_HearKnown(s, pmNow))
			{
				m_iDCOHearSkipKnown++;
				continue;
			}
			if (s && s == m_DCOHeardSource && now_ms - m_fDCOHeardAt_ms < 15000.0)
				continue;
			float score = d / Math.Max(r, 0.1);
			if (score >= bestScore)
				continue;
			bestScore = score;
			bestSrc = s;
			bestListener = me;
			bestU = ca.m_UtilityComponent;
			bestPos = p;
			bestF = fa;
			bestType = t;
			bestDist = d;
			bestRange = r;
			bestMask = mask;
		}
		DCO_Perf.Count("hear_v2:scan");
		DCO_HearSummary(now_ms, contact);
		if (!bestListener)
			return false;
		m_iDCOHearHeard++;
		m_DCOHeardSource = bestSrc;
		m_fDCOHeardAt_ms = now_ms;
		vector look = bestPos + Vector(0, 1.2, 0);
		if (bestU.m_LookAction)
			bestU.m_LookAction.LookAt(look, 45.0, 3.0);
		DCO_LookAtAll(look, 40.0, 2.5);
		float conf = DCO_HeardEst.BaseConf(bestType) * Math.Clamp(1.0 - 0.7 * bestScore, 0.3, 1.0);
		float rad = DCO_HeardEst.BaseRadius(bestType, bestDist);
		DCO_HeardEst.Report(m_Owner, bestPos, rad, conf, bestType, bestListener, bestDist, bestMask, now_ms);
		DCO_Perf.Count("ev:heard");
		DCO_Perf.Count("ev:heard_" + DCO_Hearing.TypeName(bestType));
		bool idle = !m_Owner.GetCurrentWaypoint() && (!utilDco || utilDco.GetTask() == DCO_EGroupTask.NONE);
		bool inject = bestSrc && ((!contact && idle) || (contact && conf >= 0.5));
		if (inject)
			m_Perception.AddOrUpdateGunshot(bestSrc, bestPos, bestF, pmNow, false);
		if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, DCO_Log.IdGroup(m_Owner), string.Format("hear_v2 grp=%1 listener=%2 type=%3 dist=%4 range=%5 conf=%6 contact=%7 inject=%8 mask=%9", DCO_Log.IdGroup(m_Owner), DCO_Log.IdUnit(bestListener), DCO_Hearing.TypeName(bestType), Math.Round(bestDist), Math.Round(bestRange), conf.ToString(-1, 2), DCO_Util.B01(contact), DCO_Util.B01(inject), bestMask));
		return true;
	}

	void DCO_LookAtAll(vector pos, float priority, float duration)
	{
		array<AIAgent> agents = {};
		m_Owner.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (ca && ca.m_UtilityComponent && ca.m_UtilityComponent.m_LookAction)
				ca.m_UtilityComponent.m_LookAction.LookAt(pos, priority, duration);
			if (ca && ca.m_UtilityComponent && ca.m_UtilityComponent.m_CombatComponent)
				ca.m_UtilityComponent.m_CombatComponent.DCO_NotePeek(pos, 1, Math.RandomFloat(1000, 3000));
		}
	}


	protected ref array<IEntity> m_aDCONearBuildings;
	protected float m_fDCONearBuildings_ms = -1;

	array<IEntity> DCO_GetNearbyBuildings(float now_ms)
	{
		if (!m_aDCONearBuildings)
			m_aDCONearBuildings = {};
		if (m_fDCONearBuildings_ms >= 0 && now_ms - m_fDCONearBuildings_ms < 10000.0 && now_ms >= m_fDCONearBuildings_ms)
			return m_aDCONearBuildings;
		m_fDCONearBuildings_ms = now_ms;
		m_aDCONearBuildings.Clear();
		IEntity leader = m_Owner.GetLeaderEntity();
		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		if (leader && reg)
			reg.FindBuildings(leader.GetOrigin(), 60.0, m_aDCONearBuildings);
		return m_aDCONearBuildings;
	}


	protected IEntity m_DCOLastLeader;
	protected bool m_bDCOHadRTO;
	protected float m_fDCOAmbushCheck_ms;
	protected float m_fDCOSOPCheck_ms;
	protected float m_fDCOLastContact_ms = -1;
	protected float m_fDCOContactStart_ms = -1;
	protected int m_iDCOUnitsAtContact;
	protected bool m_bDCOMovingAtContact;

	protected void DCO_UpdateAmbush(float now_ms)
	{
		if (now_ms < m_fDCOAmbushCheck_ms || !utilDco || !Replication.IsServer())
			return;
		m_fDCOAmbushCheck_ms = now_ms + 1000.0;

		int st = utilDco.GetState();
		if (!(st & DCO_EGroupState.IN_CONTACT))
		{
			if (m_fDCOLastContact_ms < 0 || now_ms - m_fDCOLastContact_ms > 10000.0)
				m_fDCOContactStart_ms = -1;
			return;
		}

		int units = utilDco.GetUnitCount();
		if (m_fDCOContactStart_ms < 0)
		{
			bool quiet = m_fDCOLastContact_ms < 0 || now_ms - m_fDCOLastContact_ms > 30000.0;
			m_fDCOContactStart_ms = 0;
			if (quiet)
				m_fDCOContactStart_ms = now_ms;
			m_iDCOUnitsAtContact = units;
			m_bDCOMovingAtContact = (st & (DCO_EGroupState.EN_ROUTE | DCO_EGroupState.MOUNTED)) != 0;
		}
		m_fDCOLastContact_ms = now_ms;

		if (m_fDCOContactStart_ms <= 0 || now_ms - m_fDCOContactStart_ms > 5000.0 || !m_bDCOMovingAtContact)
			return;
		if (!DCO_GlobalAIComponent.BehaviorOnFor(m_Owner, DCO_EBehavior.AMBUSH_DRILL) || utilDco.IsPlayerGroup() || utilDco.IsBreakingContact() || utilDco.m_DCOAmbush)
			return;
		DCO_EGroupTask ambTask = utilDco.GetTask();
		if (utilDco.m_iDCOTRPhase >= 0 || ambTask == DCO_EGroupTask.ATTACK || ambTask == DCO_EGroupTask.SUPPORT_BY_FIRE)
		{
			m_fDCOContactStart_ms = 0;
			DCO_Perf.Count("amb:skip_attack_task");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, "-", string.Format("ambush_skip grp=%1 reason=attack_task task=%2 tr_phase=%3", m_Owner.GetName(), typename.EnumToString(DCO_EGroupTask, ambTask), utilDco.m_iDCOTRPhase));
			return;
		}
		if (m_fDCOSharedAt >= 0 && DCO_Util.NowS() - m_fDCOSharedAt < 60.0)
		{
			m_fDCOContactStart_ms = 0;
			return;
		}

		array<AIAgent> agents = {};
		m_Owner.GetAgents(agents);
		int threatened = 0;
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (ca && ca.m_UtilityComponent && ca.m_UtilityComponent.m_ThreatSystem && ca.m_UtilityComponent.m_ThreatSystem.GetState() == EAIThreatState.THREATENED)
				threatened++;
		}
		if (threatened < Math.Min(2, units) && units >= m_iDCOUnitsAtContact)
			return;

		IEntity leader = m_Owner.GetLeaderEntity();
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!leader || !pm)
			return;
		float pmNow = pm.GetTime();
		vector lp = leader.GetOrigin();
		vector sum;
		int n;
		float minYaw = 1000;
		float maxYaw = -1000;
		float baseYaw;
		foreach (SCR_AITargetInfo t : m_Perception.m_aTargets)
		{
			if (!t || pmNow - t.m_fTimestamp > 5.0)
				continue;
			sum = sum + t.m_vWorldPos;
			float yaw = Math.Atan2(t.m_vWorldPos[0] - lp[0], t.m_vWorldPos[2] - lp[2]) * Math.RAD2DEG;
			if (n == 0)
				baseYaw = yaw;
			float rel = yaw - baseYaw;
			if (rel > 180)
				rel -= 360;
			if (rel < -180)
				rel += 360;
			minYaw = Math.Min(minYaw, rel);
			maxYaw = Math.Max(maxYaw, rel);
			n++;
		}
		if (n == 0)
			return;

		m_fDCOContactStart_ms = 0;
		DCO_AmbushDrill.Start(utilDco, sum * (1.0 / n), maxYaw - minYaw > 70.0, n);
	}

	protected float m_fDCORetreatCheck_ms;

	protected void DCO_UpdateRetreatWhen(float now_ms)
	{
		if (now_ms < m_fDCORetreatCheck_ms || !utilDco || !Replication.IsServer())
			return;
		m_fDCORetreatCheck_ms = now_ms + 2000.0;
		DCO_ERetreatWhen mode = utilDco.GetRetreatWhen();
		if (mode == DCO_ERetreatWhen.NEVER || utilDco.IsBreakingContact() || utilDco.m_DCOAmbush || utilDco.IsPlayerGroup() || utilDco.IsInTransport() || utilDco.IsMortar())
			return;
		if (!DCO_GlobalAIComponent.BehaviorOnFor(m_Owner, DCO_EBehavior.BREAK_CONTACT))
			return;
		int st = utilDco.GetState();
		if (!(st & DCO_EGroupState.IN_CONTACT))
			return;
		IEntity leader = m_Owner.GetLeaderEntity();
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!leader || !pm)
			return;

		int pers = DCO_Radio.PersonalityOf(utilDco.GetMyCommander());
		DCO_GlobalAIComponent bcCfg = DCO_GlobalAIComponent.GetFor(m_Owner);
		float depletedAt = 0.4;
		if (bcCfg)
			depletedAt = bcCfg.DCO_TuneF("bc.keep_frac", 0.4);
		float overrunMul = 1.0;
		if (pers == 2)
		{
			depletedAt += 0.1;
			overrunMul = 0.75;
		}
		else if (pers == 1)
		{
			depletedAt -= 0.1;
			overrunMul = 1.5;
		}

		int units = utilDco.GetUnitCount();
		int peak = utilDco.DCO_GetPeakStrength();
		bool depleted = peak > 0 && units <= 1.0 * peak * depletedAt;
		int shakenN, aliveN;
		foreach (SCR_AIInfoComponent bcInfo : m_aInfoComponents)
		{
			if (!bcInfo || !bcInfo.GetUtilityComp())
				continue;
			DCO_AIMoraleSystem bcMorale = bcInfo.GetUtilityComp().GetMoraleSystem();
			if (!bcMorale)
				continue;
			aliveN++;
			if (bcMorale.GetState() >= moraleState.SHAKEN)
				shakenN++;
		}
		bool shook = aliveN >= 2 && shakenN * 2 > aliveN && (bcCfg && bcCfg.DCO_TuneF("bc.shaken", 1.0) > 0);

		float pmNow = pm.GetTime();
		vector lp = leader.GetOrigin();
		vector sum;
		int n, close;
		foreach (SCR_AITargetInfo t : m_Perception.m_aTargets)
		{
			if (!t || pmNow - t.m_fTimestamp > 10.0)
				continue;
			sum = sum + t.m_vWorldPos;
			n++;
			if (vector.DistanceSqXZ(lp, t.m_vWorldPos) < 2500.0)
				close++;
		}
		if (n == 0)
			return;
		bool overran = close >= Math.Max(2, 1.0 * units * overrunMul);

		bool trigger;
		string why;
		switch (mode)
		{
			case DCO_ERetreatWhen.DEFAULT:		trigger = overran || depleted || shook; break;
			case DCO_ERetreatWhen.OVERRAN:		trigger = overran; break;
			case DCO_ERetreatWhen.DEPLETED:		trigger = depleted; break;
			case DCO_ERetreatWhen.IN_CONTACT:	trigger = true; break;
			case DCO_ERetreatWhen.THREATENED:	trigger = (st & DCO_EGroupState.SUPPRESSED) != 0; break;
		}
		if (!trigger)
			return;
		why = "self_" + typename.EnumToString(DCO_ERetreatWhen, mode);
		if (overran)
			why += "_overran";
		else if (depleted)
			why += "_depleted";
		else if (shook)
			why += "_shaken";

		AICommander_BaseComponent cmd = utilDco.GetMyCommander();
		bool bcOk = DCO_BreakContact.Start(utilDco, sum * (1.0 / n), vector.Zero, why);
		if (bcOk && (depleted || shook))
		{
			float ineff = 150.0;
			if (bcCfg)
				ineff = bcCfg.DCO_TuneF("bc.ineffective_s", 150.0);
			utilDco.m_fDCOIneffUntil_s = DCO_Util.NowS() + ineff;
			DCO_Perf.Count("bc:self_ineffective");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("break_contact_self grp=%1 why=%2 units=%3 peak=%4 shaken=%5/%6 ineffective_s=%7", DCO_Log.IdGroup(m_Owner), why, units, peak, shakenN, aliveN, Math.Round(ineff)));
		}
		if (bcOk && cmd)
			cmd.DetachFromObjective(utilDco);
	}

	protected float m_fDCOCmdLossCheck_ms;

	protected void DCO_UpdateCommandLoss(float now_ms)
	{
		if (now_ms < m_fDCOCmdLossCheck_ms || !utilDco || !Replication.IsServer())
			return;
		m_fDCOCmdLossCheck_ms = now_ms + 1000.0;
		if (!DCO_GlobalAIComponent.BehaviorOnFor(m_Owner, DCO_EBehavior.LEADER_RTO_GRENADIER))
			return;

		IEntity leader = m_Owner.GetLeaderEntity();
		if (m_DCOLastLeader && leader != m_DCOLastLeader && !DCO_Util.IsAlive(m_DCOLastLeader) && leader && !SCR_CharacterHelper.IsPlayer(leader))
		{
			float dur = Math.RandomFloat(5000.0, 10000.0);
			utilDco.DCO_SetConfused(now_ms + dur);
			array<AIAgent> agents = {};
			m_Owner.GetAgents(agents);
			foreach (AIAgent a : agents)
			{
				SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
				if (ca && ca.m_UtilityComponent && ca.m_UtilityComponent.GetMoraleSystem())
					ca.m_UtilityComponent.GetMoraleSystem().AddStress(0.6, "leader_killed");
			}
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("leader_killed grp=%1 confused_s=%2", DCO_Log.IdGroup(m_Owner), Math.Round(dur / 1000.0)));
		}
		m_DCOLastLeader = leader;

		bool hasRTO = utilDco.HasRadio();
		if (m_bDCOHadRTO && !hasRTO)
		{
			float delay = Math.RandomFloat(30000.0, 60000.0);
			utilDco.DCO_SetRadioDelayed(now_ms + delay);
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("rto_lost grp=%1 report_delay_s=%2", m_Owner, Math.Round(delay / 1000.0)));
		}
		m_bDCOHadRTO = hasRTO;
	}


	bool DCO_IsIdle()
	{
		if (m_fDCONoWaypointSince_ms < 0)
			return false;

		float enter_s = 30;
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(m_Owner);
		if (cfg)
			enter_s = cfg.GetIdleEnterTime();

		return GetGame().GetWorld().GetWorldTime() - m_fDCONoWaypointSince_ms >= enter_s * 1000.0;
	}

	bool DCO_HasClusterActivity()
	{
		if (!m_Perception)
			return false;

		foreach (SCR_AIGroupTargetCluster c : m_Perception.m_aTargetClusters)
		{
			if (c.m_State && c.m_State.m_Activity)
				return true;
		}

		return false;
	}

	protected ref DCO_StrengthInfo m_DCOFriendly = new DCO_StrengthInfo();
	protected ref DCO_StrengthInfo m_DCOFriendlyVsVeh = new DCO_StrengthInfo();
	protected ref DCO_StrengthInfo m_DCOEnemyTotal = new DCO_StrengthInfo();
	protected float m_fDCOFriendlyAt_ms = -1;
	protected float m_fDCOFriendlyVsVehAt_ms = -1;

	DCO_StrengthInfo DCO_GetFriendlyStrength(bool enemyHasVehicles)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (enemyHasVehicles)
		{
			if (m_fDCOFriendlyVsVehAt_ms < 0 || now - m_fDCOFriendlyVsVehAt_ms >= 1000.0)
			{
				m_fDCOFriendlyVsVehAt_ms = now;
				DCO_Strength.OfGroup(SCR_AIGroup.Cast(m_Owner), true, m_DCOFriendlyVsVeh);
			}
			return m_DCOFriendlyVsVeh;
		}

		if (m_fDCOFriendlyAt_ms < 0 || now - m_fDCOFriendlyAt_ms >= 1000.0)
		{
			m_fDCOFriendlyAt_ms = now;
			DCO_Strength.OfGroup(SCR_AIGroup.Cast(m_Owner), false, m_DCOFriendly);
		}
		return m_DCOFriendly;
	}

	DCO_StrengthInfo DCO_GetClusterStrength(SCR_AITargetClusterState s)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (s.m_fDCOStrengthAt_ms < 0 || now - s.m_fDCOStrengthAt_ms >= 1000.0)
		{
			s.m_fDCOStrengthAt_ms = now;
			DCO_Strength.OfCluster(s.m_Cluster, s.m_DCOStrength);
		}
		return s.m_DCOStrength;
	}

	float DCO_GetSuperiorRatio()
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(m_Owner);
		if (!cfg)
			return 2;
		float sr = cfg.GetSuperiorRatio();
		if (cfg.DCO_TuneF("agg.profile", 1.0) > 0)
		{
			float sa = Math.Clamp(cfg.DCO_AggressionSlider(), 0.5, 4.0);
			if (sa != 1.0)
				sr = Math.Max(sr / Math.Pow(sa, 0.5), Math.Min(sr, 1.2));
		}
		return sr;
	}

	bool DCO_IsSuperiorTo(SCR_AITargetClusterState s)
	{
		if (!s || s.m_iCountAlive <= 0 || m_fThreatMeasure >= 2.0)
			return false;

		DCO_StrengthInfo enemy = DCO_GetClusterStrength(s);
		return DCO_Strength.IsSuperior(DCO_GetFriendlyStrength(enemy.m_bHasVehicles), enemy, DCO_GetSuperiorRatio());
	}


	protected SCR_AITargetClusterState m_DCOOvermatch;
	protected float m_fDCONextOvermatch_ms;

	bool DCO_IsOvermatch()
	{
		return m_DCOOvermatch != null;
	}

	protected float DCO_OvermatchThreshold()
	{
		float mult = 1.0;
		SCR_ChimeraAIAgent leader = SCR_ChimeraAIAgent.Cast(m_Owner.GetLeaderAgent());
		if (leader && leader.m_UtilityComponent)
		{
			switch (DCO_PersonalityCombatUtility.GetPersonalitySafe(leader.m_UtilityComponent))
			{
				case DCO_EAIPersonality.RECKLESS:	mult = 0.75; break;
				case DCO_EAIPersonality.AGGRESSIVE:	mult = 0.9; break;
				case DCO_EAIPersonality.CAUTIOUS:	mult = 1.5; break;
			}
		}
		return DCO_GetSuperiorRatio() * mult;
	}

	protected bool DCO_OvermatchTaskAllows()
	{
		if (m_Owner.IsSlave() || DCO_GetGarrisonMode() == DCO_EGarrisonMode.HOLD)
			return false;

		if (!utilDco)
			utilDco = DCO_GroupUtilityComponent.Cast(m_Owner.FindComponent(DCO_GroupUtilityComponent));
		if (!utilDco)
			return true;

		if (utilDco.IsMortar() || utilDco.IsDedicatedTransport() || utilDco.IsInTransport() || utilDco.IsHeldByGM() || utilDco.IsPlayerGroup())
			return false;

		DCO_EGroupTask task = utilDco.GetTask();
		return task != DCO_EGroupTask.RECON && task != DCO_EGroupTask.TRANSPORT && task != DCO_EGroupTask.FIRE_MISSION;
	}

	protected bool DCO_IsConfident(SCR_AITargetClusterState s)
	{
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!pm || !s.m_Cluster)
			return false;

		float t = pm.GetTime();
		foreach (SCR_AITargetInfo tgt : s.m_Cluster.m_aTargets)
		{
			if (!tgt)
				continue;

			EAITargetInfoCategory cat = tgt.m_eCategory;
			if (cat == EAITargetInfoCategory.DESTROYED || cat == EAITargetInfoCategory.DISARMED)
				continue;
			if (cat != EAITargetInfoCategory.IDENTIFIED || t - tgt.m_fTimestamp > 5.0)
				return false;
		}
		return true;
	}

	protected bool DCO_HasOtherContact(SCR_AITargetClusterState main)
	{
		IEntity leader = m_Owner.GetLeaderEntity();
		if (!leader)
			return false;

		vector p = leader.GetOrigin();
		vector mainDir = main.GetCenterPosition() - p;
		mainDir[1] = 0;
		mainDir.Normalize();

		foreach (SCR_AIGroupTargetCluster c : m_Perception.m_aTargetClusters)
		{
			SCR_AITargetClusterState s = c.m_State;
			if (!s || s == main || s.m_iCountAlive <= 0 || s.GetTimeSinceLastNewInformation() > 10.0)
				continue;

			bool firing = s.m_iCountEndangering > 0;
			if (!firing && (s.m_iCountDetected <= 0 || s.m_fDistMin > 150.0))
				continue;

			vector d = s.GetCenterPosition() - p;
			d[1] = 0;
			if (d.Length() < 1)
				continue;
			d.Normalize();

			if (vector.Dot(d, mainDir) < 0.707)
				return true;
		}
		return false;
	}

	protected float DCO_OvermatchRatio(SCR_AITargetClusterState s)
	{
		DCO_StrengthInfo enemy = DCO_GetClusterStrength(s);
		return DCO_Strength.Ratio(DCO_GetFriendlyStrength(enemy.m_bHasVehicles), enemy);
	}

	protected void DCO_UpdateOvermatch(float now_ms)
	{
		if (now_ms < m_fDCONextOvermatch_ms)
			return;
		m_fDCONextOvermatch_ms = now_ms + 1000.0;

		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(m_Owner);
		if (!cfg || !cfg.GetOvermatchAssault())
		{
			DCO_EndOvermatch("disabled");
			return;
		}

		if (!DCO_OvermatchTaskAllows())
		{
			DCO_EndOvermatch("task");
			return;
		}

		float threshold = DCO_OvermatchThreshold();
		if (m_DCOOvermatch)
		{
			SCR_AITargetClusterState s = m_DCOOvermatch;
			if (s.m_iCountAlive <= 0 || s.m_eState != EAITargetClusterState.ATTACKING || !s.m_Cluster || !m_Perception.m_aTargetClusters.Contains(s.m_Cluster))
				DCO_EndOvermatch("done");
			else if (m_fThreatMeasure >= 2.0)
				DCO_EndOvermatch("suppressed");
			else if (DCO_OvermatchRatio(s) < threshold - 0.2)
				DCO_EndOvermatch("ratio");
			else if (DCO_HasOtherContact(s))
				DCO_EndOvermatch("contact");
			return;
		}

		if (m_fThreatMeasure >= 2.0)
			return;

		float maxDist = cfg.GetOvermatchMaxDist();
		foreach (SCR_AIGroupTargetCluster c : m_Perception.m_aTargetClusters)
		{
			SCR_AITargetClusterState s = c.m_State;
			if (!s || s.m_eState != EAITargetClusterState.ATTACKING || s.m_iCountAlive <= 0 || s.m_fDistMin > maxDist)
				continue;

			float ratio = DCO_OvermatchRatio(s);
			if (ratio < threshold || !DCO_IsConfident(s) || DCO_HasOtherContact(s))
				continue;

			m_DCOOvermatch = s;
			m_eDCOPosture = DCO_GroupTactics.AGGRESIVE;
			m_fDCOPostureChanged_ms = now_ms;
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("overmatch_start grp=%1 ratio=%2 need=%3 dist=%4",
				DCO_Log.IdGroup(m_Owner), ratio.ToString(-1, 2), threshold.ToString(-1, 2), Math.Round(s.m_fDistMin)));
			return;
		}
	}

	protected void DCO_EndOvermatch(string reason)
	{
		if (!m_DCOOvermatch)
			return;

		float dist = m_DCOOvermatch.m_fDistMin;
		m_DCOOvermatch = null;
		if (reason == "done")
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("overmatch_done grp=%1 dist=%2", DCO_Log.IdGroup(m_Owner), Math.Round(dist)));
		else
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("overmatch_abort grp=%1 reason=%2 dist=%3", DCO_Log.IdGroup(m_Owner), reason, Math.Round(dist)));
	}


	protected DCO_CQBActivity m_DCOCQBAct;
	protected bool m_bDCORatioHoldLogged;
	protected float m_fDCONextCQB_ms;
	protected float m_fDCOLoaLog_ms;
	protected float m_fDCOPlanCheck_ms;
	protected bool m_bDCOAssaultLocked;

	bool DCO_IsClearingBuilding()
	{
		return m_DCOCQBAct && m_DCOCQBAct.IsRunning();
	}

	DCO_CQBActivity DCO_GetCQBActivity()
	{
		if (m_DCOCQBAct && m_DCOCQBAct.IsRunning())
			return m_DCOCQBAct;
		return null;
	}

	protected bool DCO_StartCQB(IEntity building, string faction, string trigger, bool sweep, float now_ms, string info, AIWaypoint wp = null)
	{
		DCO_CQBActivity act = DCO_CQBActivity.Create(this, building, faction, trigger, sweep, info, wp);
		if (!act)
			return false;
		m_DCOCQBAct = act;
		AddAction(act);
		m_fDCONextCQB_ms = now_ms + 1000;
		return true;
	}

	bool DCO_StartWaypointCQB(IEntity building, AIWaypoint wp, out string why)
	{
		why = "-";
		float now_ms = GetGame().GetWorld().GetWorldTime();
		string faction = DCO_FactionKey();
		if (faction.IsEmpty())
		{
			why = "no_faction";
			return false;
		}
		if (DCO_IsClearingBuilding())
		{
			why = "busy";
			return false;
		}
		if (!DCO_BuildingClear.TryBook(building, faction, m_Owner, now_ms))
		{
			why = "booked";
			return false;
		}
		if (!DCO_StartCQB(building, faction, "planned", false, now_ms, "src=waypoint", wp))
		{
			DCO_BuildingClear.Finish(building, faction, m_Owner, false, now_ms, 0);
			why = "no_entry";
			return false;
		}
		return true;
	}

	string DCO_FactionKey()
	{
		Faction f = m_Owner.GetFaction();
		if (!f)
			return string.Empty;
		return f.GetFactionKey();
	}

	protected void DCO_UpdateCQB(float now_ms)
	{
		if (m_DCOCQBAct)
		{
			if (m_DCOCQBAct.IsRunning())
				return;
			m_DCOCQBAct = null;
			m_fDCONextCQB_ms = now_ms + 2000.0;
		}
		if (now_ms < m_fDCONextCQB_ms)
			return;
		m_fDCONextCQB_ms = now_ms + 2000.0;

		SCR_AIGroup featGrp = SCR_AIGroup.Cast(m_Owner);
		if (featGrp && !featGrp.DCO_FeatureOn(DCO_EGroupFeature.CQB))
		{
			DCO_Perf.Count("cqb_skip:group_feature_off");
			return;
		}

		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(m_Owner);
		if (!cfg || !cfg.GetCQBEnabled() || !Replication.IsServer())
			return;

		string faction = DCO_FactionKey();
		if (faction.IsEmpty())
			return;

		string scanInfo;
		string trig = "contact";
		int csT = DCO_Perf.Begin();
		IEntity building = DCO_ScanBuildingContacts(faction, now_ms, scanInfo);
		DCO_Perf.End("cqb_scan", csT);
		if (!building)
		{
			int cpT = DCO_Perf.Begin();
			building = DCO_PlannedCQBTarget(faction, now_ms, scanInfo);
			DCO_Perf.End("cqb_plan", cpT);
			if (!building)
				return;
			trig = "planned";
		}
		else if (!DCO_CQBTaskAllows())
		{
			DCO_Perf.Count("cqb_skip:task");
			return;
		}

		string loaReason;
		if (!DCO_AssaultLock.LoaOk(this, building, loaReason))
		{
			DCO_Perf.Count("tr:assault_loa_" + loaReason);
			if (now_ms >= m_fDCOLoaLog_ms)
			{
				m_fDCOLoaLog_ms = now_ms + 30000.0;
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Owner), string.Format("tr_assault_loa grp=%1 target=%2 reason=%3 action=wait_sbf_bound", DCO_Log.IdGroup(m_Owner), building, loaReason));
			}
			return;
		}

		if (!DCO_BuildingClear.TryBook(building, faction, m_Owner, now_ms))
		{
			DCO_Perf.Count("cqb_skip:book");
			return;
		}

		int cstT = DCO_Perf.Begin();
		bool cqbStarted = DCO_StartCQB(building, faction, trig, false, now_ms, scanInfo);
		DCO_Perf.End("cqb_start", cstT);
		if (!cqbStarted)
		{
			DCO_BuildingClear.Finish(building, faction, m_Owner, false, now_ms, 0);
			m_fDCONextCQB_ms = now_ms + 30000.0;
			if (DCO_Log.On(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG, DCO_Log.IdGroup(m_Owner), string.Format("cqb_skip grp=%1 reason=no_entry agents=%2", DCO_Log.IdGroup(m_Owner), m_Owner.GetAgentsCount()));
			return;
		}
	}

	protected IEntity DCO_PlannedCQBTarget(string faction, float now_ms, out string info)
	{
		if (now_ms < m_fDCOPlanCheck_ms)
			return null;
		m_fDCOPlanCheck_ms = now_ms + 5000.0;
		IEntity leader = m_Owner.GetLeaderEntity();
		if (!leader || m_Owner.GetAgentsCount() < 2 || DCO_GetGarrisonMode() >= 0 || !DCO_OvermatchTaskAllows())
			return null;
		AIWaypoint wp = m_Owner.GetCurrentWaypoint();
		bool wpTask = SCR_SearchAndDestroyWaypoint.Cast(wp) && (!utilDco || !utilDco.GetMyCommander());
		if (!wpTask && !DCO_AssaultTask())
			return null;
		if (utilDco && utilDco.m_iDCOTRPhase >= 0 && utilDco.m_iDCOTRPhase != DCO_ETRPhase.EXECUTING && utilDco.m_iDCOTRPhase != DCO_ETRPhase.HOLDING)
		{
			DCO_Perf.Count("cqb_skip:planned_tr_phase");
			return null;
		}
		vector p = leader.GetOrigin();
		IEntity b;
		string reason = "-";
		if (utilDco && !wpTask)
		{
			CMD_AICommanderObjectiveComponent obj = utilDco.GetGroupObjective();
			if (obj && obj.GetOwner())
			{
				vector op = obj.GetOwner().GetOrigin();
				b = DCO_BuildingClear.FindContactBuilding(op, obj.GetRadius(), faction, 600000.0, now_ms);
				if (b)
					reason = "clear_registry";
				else
				{
					AICommander_BaseComponent cmd = utilDco.GetMyCommander();
					if (cmd && cmd.GetOps())
					{
						DCO_ObjectiveIntel intel = cmd.GetOps().GetIntel(obj);
						if (intel && intel.m_fDefenders > 0)
						{
							b = DCO_NearestBuilding(op, obj.GetRadius(), p);
							if (b)
								reason = "objective_intel";
						}
					}
				}
			}
		}
		if (!b)
		{
			b = DCO_BuildingClear.FindContactBuilding(p, 200.0, faction, 600000.0, now_ms);
			if (b)
				reason = "clear_registry";
		}
		if (!b && wp && wpTask)
		{
			b = DCO_NearestBuilding(wp.GetOrigin(), 15.0, wp.GetOrigin());
			if (b)
				reason = "waypoint_building";
		}
		else if (!b && wp)
			DCO_Perf.Count("cqb_skip:planned_wp_commander");
		if (!b)
			return null;
		float d = vector.DistanceXZ(p, b.GetOrigin());
		if (d > 200.0)
		{
			DCO_Perf.Count("cqb_skip:planned_far");
			return null;
		}
		info = string.Format("reason=%1 dist=%2", reason, Math.Round(d));
		return b;
	}

	IEntity DCO_NearestBuilding(vector center, float radius, vector from)
	{
		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		if (!reg)
			return null;
		array<IEntity> found = {};
		reg.FindBuildings(center, radius + 30.0, found);
		IEntity best;
		float bestD = float.MAX;
		foreach (IEntity cand : found)
		{
			if (!cand)
				continue;
			vector mins, maxs;
			cand.GetWorldBounds(mins, maxs);
			float dx = Math.Max(Math.Max(mins[0] - center[0], 0.0), center[0] - maxs[0]);
			float dz = Math.Max(Math.Max(mins[2] - center[2], 0.0), center[2] - maxs[2]);
			if (dx * dx + dz * dz > radius * radius)
				continue;
			float df = vector.DistanceXZ(cand.GetOrigin(), from);
			if (df < bestD)
			{
				bestD = df;
				best = cand;
			}
		}
		return best;
	}

	protected bool DCO_AttackingObjective()
	{
		if (!utilDco)
			return false;
		CMD_AICommanderObjectiveComponent obj = utilDco.GetGroupObjective();
		return obj && obj.GetOwner() && !obj.IsCapturedBy(DCO_FactionKey());
	}

	protected bool DCO_AssaultTask()
	{
		if (!utilDco)
			return false;
		DCO_EGroupTask task = utilDco.GetTask();
		if (task == DCO_EGroupTask.ATTACK || task == DCO_EGroupTask.FLANK)
			return true;
		return task == DCO_EGroupTask.REINFORCE && DCO_AttackingObjective();
	}

	protected float m_fDCOVehSbfLogAt;

	protected bool DCO_AssaultReleased()
	{
		if (!utilDco)
			return false;
		CMD_AICommanderObjectiveComponent obj = utilDco.GetGroupObjective();
		AICommander_BaseComponent cmd = utilDco.GetMyCommander();
		if (!obj || !cmd)
			return true;
		return cmd.IsAssaultReleased(obj);
	}

	protected float m_fDCOAntiCampTick_ms;
	protected ref map<AIAgent, float> m_mDCOCoverSince;
	protected ref map<AIAgent, float> m_mDCOCampLimit;

	protected void DCO_ResetAntiCamp()
	{
		if (m_mDCOCoverSince)
			m_mDCOCoverSince.Clear();
	}

	protected vector DCO_CampGoal(out string goalKind)
	{
		goalKind = "none";
		PerceptionManager pm = GetGame().GetPerceptionManager();
		IEntity leader = m_Owner.GetLeaderEntity();
		if (pm && leader && m_Perception)
		{
			float t = pm.GetTime();
			float bestSq = float.MAX;
			vector best;
			foreach (SCR_AITargetInfo tgt : m_Perception.m_aTargets)
			{
				if (!tgt || t - tgt.m_fTimestamp > 60.0)
					continue;
				float d = vector.DistanceSqXZ(tgt.m_vWorldPos, leader.GetOrigin());
				if (d < bestSq)
				{
					bestSq = d;
					best = tgt.m_vWorldPos;
				}
			}
			if (bestSq < float.MAX)
			{
				goalKind = "last_known";
				return best;
			}
		}
		if (utilDco)
		{
			CMD_AICommanderObjectiveComponent obj = utilDco.GetGroupObjective();
			if (obj && obj.GetOwner())
			{
				goalKind = "objective";
				return obj.GetOwner().GetOrigin();
			}
		}
		return vector.Zero;
	}

	protected void DCO_UpdateAntiCamp(float now_ms)
	{
		if (now_ms < m_fDCOAntiCampTick_ms || !Replication.IsServer())
			return;
		m_fDCOAntiCampTick_ms = now_ms + 5000.0;
		if (!DCO_AssaultTask())
		{
			DCO_ResetAntiCamp();
			return;
		}
		string skip;
		if (DCO_IsClearingBuilding())
			skip = "cqb";
		else if (!DCO_AssaultReleased())
			skip = "not_released";
		else
		{
			DCO_GroupTactics posture = DCO_GetPosture();
			if (posture == DCO_GroupTactics.DEFENSIVE || posture == DCO_GroupTactics.DELAY || posture == DCO_GroupTactics.AMBUSH)
				skip = "posture";
		}
		string goalKind;
		vector goal;
		if (skip.IsEmpty())
		{
			goal = DCO_CampGoal(goalKind);
			if (goal == vector.Zero)
				skip = "no_goal";
		}
		if (!skip.IsEmpty())
		{
			DCO_Perf.Count("anti_camp:skip_" + skip);
			DCO_ResetAntiCamp();
			return;
		}
		if (!m_mDCOCoverSince)
			m_mDCOCoverSince = new map<AIAgent, float>();
		if (!m_mDCOCampLimit)
			m_mDCOCampLimit = new map<AIAgent, float>();

		array<AIAgent> agents = {};
		m_Owner.GetAgents(agents);
		int bounding;
		array<ref AIActionBase> acts = {};
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent)
				continue;
			acts.Clear();
			ca.m_UtilityComponent.FindActionsOfType(DCO_BoundBehavior, acts);
			foreach (AIActionBase act : acts)
			{
				EAIActionState s = act.GetActionState();
				if (s != EAIActionState.COMPLETED && s != EAIActionState.FAILED)
				{
					bounding++;
					break;
				}
			}
		}
		int maxBound = agents.Count() / 2;
		foreach (AIAgent agent : agents)
		{
			SCR_ChimeraAIAgent chim = SCR_ChimeraAIAgent.Cast(agent);
			if (!chim || !chim.m_UtilityComponent || !chim.m_UtilityComponent.m_CombatMoveState)
				continue;
			SCR_AIUtilityComponent util = chim.m_UtilityComponent;
			IEntity ent = chim.GetControlledEntity();
			if (!ent)
				continue;
			if (util.m_CombatComponent && util.m_CombatComponent.DCO_IsPinned())
			{
				m_mDCOCoverSince.Remove(agent);
				DCO_Perf.Count("anti_camp:skip_pinned");
				continue;
			}
			bool sees = false;
			if (util.m_CombatComponent)
			{
				BaseTarget target = util.m_CombatComponent.GetCurrentTarget();
				sees = target && target.GetTimeSinceSeen() < 3.0;
			}
			if (!util.m_CombatMoveState.m_bInCover || sees)
			{
				m_mDCOCoverSince.Remove(agent);
				continue;
			}
			float since;
			if (!m_mDCOCoverSince.Find(agent, since))
			{
				m_mDCOCoverSince.Set(agent, now_ms);
				m_mDCOCampLimit.Set(agent, Math.RandomFloat(20.0, 30.0) * 1000.0);
				continue;
			}
			float limit = m_mDCOCampLimit.Get(agent);
			if (now_ms - since < limit)
				continue;
			if (bounding >= maxBound)
			{
				DCO_Perf.Count("anti_camp:skip_half");
				continue;
			}
			vector from = ent.GetOrigin();
			vector dir = goal - from;
			dir[1] = 0;
			if (dir.Length() < 8.0)
			{
				m_mDCOCoverSince.Remove(agent);
				continue;
			}
			dir.Normalize();
			float boundM = Math.RandomFloat(10.0, 25.0);
			vector boundGoal = DCO_Util.Surface(from + dir * boundM);
			util.AddAction(new DCO_BoundBehavior(util, null, boundGoal, goal, ECharacterStance.CROUCH, EMovementType.RUN, boundM * 0.8, boundM, 0.5, 1.5, 0, 20, "anti_camp"));
			bounding++;
			m_mDCOCoverSince.Remove(agent);
			DCO_Perf.Count("anti_camp:bound");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("anti_camp grp=%1 unit=%2 waited_s=%3 bound_m=%4 to=%5", DCO_Log.IdGroup(m_Owner), DCO_Log.IdUnit(ent), Math.Round((now_ms - since) / 1000), Math.Round(boundM), goalKind));
		}
	}

	bool DCO_StartSweep(IEntity building, float now_ms)
	{
		if (DCO_IsClearingBuilding() || !building || m_Owner.GetAgentsCount() < 2 || !DCO_OvermatchTaskAllows())
			return false;

		string faction = DCO_FactionKey();
		if (faction.IsEmpty() || !DCO_BuildingClear.TryBook(building, faction, m_Owner, now_ms))
			return false;

		if (!DCO_StartCQB(building, faction, "sweep", true, now_ms, ""))
		{
			DCO_BuildingClear.Finish(building, faction, m_Owner, false, now_ms, 0);
			return false;
		}
		return true;
	}

	protected bool DCO_CQBTaskAllows()
	{
		if (m_Owner.GetAgentsCount() < 2 || DCO_GetGarrisonMode() >= 0 || !DCO_OvermatchTaskAllows())
			return false;

		if (utilDco && utilDco.m_iDCOTRPhase >= 0 && utilDco.m_iDCOTRPhase != DCO_ETRPhase.EXECUTING && utilDco.m_iDCOTRPhase != DCO_ETRPhase.HOLDING)
		{
			DCO_Perf.Count("cqb_skip:tr_phase");
			if (DCO_Log.On(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG, DCO_Log.IdGroup(m_Owner), string.Format("cqb_skip grp=%1 reason=tr_phase tr_phase=%2", DCO_Log.IdGroup(m_Owner), typename.EnumToString(DCO_ETRPhase, utilDco.m_iDCOTRPhase)));
			return false;
		}

		if (DCO_IsOvermatch())
			return true;

		return DCO_AssaultTask();
	}

	protected float m_fDCOFullScan_ms;

	protected IEntity DCO_ScanBuildingContacts(string faction, float now_ms, out string info)
	{
		bool fullScan = now_ms >= m_fDCOFullScan_ms;
		if (fullScan)
			m_fDCOFullScan_ms = now_ms + 10000.0;
		PerceptionManager pm = GetGame().GetPerceptionManager();
		IEntity leader = m_Owner.GetLeaderEntity();
		if (!pm || !leader || !m_Perception)
			return null;

		float t = pm.GetTime();
		vector p = leader.GetOrigin();
		vector objPos;
		float objReach = -1;
		if (utilDco)
		{
			CMD_AICommanderObjectiveComponent obj = utilDco.GetGroupObjective();
			if (obj && obj.GetOwner())
			{
				objPos = obj.GetOwner().GetOrigin();
				objReach = obj.GetRadius() + 30.0;
			}
		}
		float bestSq = float.MAX;
		IEntity best;
		int fresh;
		int far;
		float lostWin = DCO_Ctx.TuneForGU(this, "cqb.lost_window_s", 0.0);
		foreach (SCR_AITargetInfo tgt : m_Perception.m_aTargets)
		{
			if (!tgt)
				continue;
			float age = t - tgt.m_fTimestamp;
			bool seenCat = tgt.m_eCategory == EAITargetInfoCategory.IDENTIFIED || tgt.m_eCategory == EAITargetInfoCategory.DETECTED;
			bool live = age <= 10.0 && seenCat;
			bool lostIn = !live && lostWin > 0 && age <= lostWin && (seenCat || tgt.m_eCategory == EAITargetInfoCategory.LOST) && tgt.m_Entity && DCO_Util.IsNotDead(tgt.m_Entity);
			if (!live && !lostIn)
				continue;
			if (lostIn)
				DCO_Perf.Count("cqb_scan:lost_candidate");
			fresh++;

			float td = vector.DistanceXZ(tgt.m_vWorldPos, p);
			if (!fullScan && td > 170.0)
			{
				far++;
				DCO_Perf.Count("cqb_scan:far_pre");
				continue;
			}
			if (!fullScan && bestSq < float.MAX && td - 20.0 > Math.Sqrt(bestSq))
			{
				DCO_Perf.Count("cqb_scan:worse_pre");
				continue;
			}

			string method;
			IEntity building = SCR_CoverManagerComponent.DCO_GetBuildingAtPos(tgt.m_vWorldPos, leader, method);
			if (!building)
				continue;

			DCO_BuildingClear.MarkContact(building, faction, now_ms);
			vector bp = building.GetOrigin();
			float d = vector.DistanceSqXZ(bp, p);
			string from = "leader";
			if (d > 100.0 * 100.0)
			{
				if (objReach > 0 && d <= 150.0 * 150.0 && vector.DistanceXZ(bp, objPos) <= objReach && DCO_AssaultReleased())
					from = "objective";
				else
				{
					far++;
					continue;
				}
			}
			if (d < bestSq)
			{
				bestSq = d;
				best = building;
				info = "cat=" + typename.EnumToString(EAITargetInfoCategory, tgt.m_eCategory) + " bld_method=" + method + " dist_from=" + from + " age_s=" + Math.Round(age) + " lost=" + lostIn;
			}
		}
		if (!best)
		{
			if (fresh == 0)
				DCO_Perf.Count("cqb_skip:no_target");
			else if (far > 0)
				DCO_Perf.Count("cqb_skip:far");
			else
				DCO_Perf.Count("cqb_skip:no_building");
		}
		return best;
	}

	protected float m_fDCONextNight_ms;
	protected float m_fDCONextFlare_ms;
	protected float m_fDCONextIllum_ms;

	protected bool DCO_IsStealthGroup()
	{
		if (!utilDco)
			return false;
		if (utilDco.GetTask() == DCO_EGroupTask.RECON)
			return true;

		AICommander_BaseComponent cmd = utilDco.GetMyCommander();
		return cmd && cmd.GetDefense() && cmd.GetDefense().IsOP(utilDco);
	}

	protected void DCO_UpdateNight(float now_ms)
	{
		if (now_ms < m_fDCONextNight_ms || !Replication.IsServer())
			return;
		m_fDCONextNight_ms = now_ms + 2000.0;

		SCR_AIGroup grp = SCR_AIGroup.Cast(m_Owner);
		if (!grp || m_Owner.IsSlave())
			return;

		if (!DCO_Night.IsActive())
		{
			if (DCO_GlobalAIComponent.BehaviorOnFor(m_Owner, DCO_EBehavior.AWARENESS))
			{
				DCO_Night.ApplyLights(grp, true, null);
				DCO_Night.ApplyHeadlights(grp, m_Perception);
			}
			return;
		}

		bool stealth = DCO_IsStealthGroup();
		DCO_Night.ApplyLights(grp, stealth, null);
		DCO_Night.ApplyHeadlights(grp, m_Perception);
		if (!stealth)
			DCO_TryFlare(grp, now_ms);
	}

	protected void DCO_TryFlare(SCR_AIGroup grp, float now_ms)
	{
		if (now_ms < m_fDCONextFlare_ms || !m_Perception)
			return;

		bool contact = m_fThreatMeasure > 0.33;
		if (utilDco)
			contact = utilDco.HasState(DCO_EGroupState.IN_CONTACT);
		if (!contact)
			return;

		SCR_AIGroupTargetCluster best;
		float bestDist = float.MAX;
		foreach (SCR_AIGroupTargetCluster c : m_Perception.m_aTargetClusters)
		{
			SCR_AITargetClusterState s = c.m_State;
			if (!s || s.m_iCountAlive <= 0 || s.GetTimeSinceLastNewInformation() > 15.0)
				continue;
			if (s.m_fDistMin < 100.0 || s.m_fDistMin > 400.0 || s.m_fDistMin >= bestDist)
				continue;
			bestDist = s.m_fDistMin;
			best = c;
		}
		if (!best || !DCO_Night.IsDark(best))
			return;

		vector center = best.m_State.GetCenterPosition();
		if (DCO_Night.FlareNear(center, now_ms))
			return;

		float cooldown = 75;
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(m_Owner);
		if (cfg)
			cooldown = cfg.GetNightFlareCooldown();
		m_fDCONextFlare_ms = now_ms + cooldown * 1000;

		AIAgent shooter;
		IEntity item;
		if (DCO_Night.FindFlare(grp, shooter, item) && DCO_Night.FireFlare(shooter, item, center, now_ms))
			return;

		if (!utilDco || now_ms < m_fDCONextIllum_ms)
			return;

		CMD_ThreatResponseComponent tr = utilDco.GetThreatResponseComponent();
		if (!tr)
			return;

		m_fDCONextIllum_ms = now_ms + 120000.0;
		CMD_FireMissionRequest req = new CMD_FireMissionRequest(center, SCR_EAIArtilleryAmmoType.ILLUMINATION, now_ms / 1000.0, 1);
		req.m_sSource = "squad_illum";
		req.m_sTier = "illum";
		req.m_Requester = utilDco;
		tr.ReceiveArtillerySupport(req, utilDco);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("night_illum_request grp=%1 dist=%2", grp, Math.Round(bestDist)));
	}

	protected void DCO_UpdatePosture(float now_ms)
	{
		int friends = m_aInfoComponents.Count();
		if (friends == 0)
			return;

		int lockMode = DCO_AssaultLock.Mode(m_Owner);
		if (lockMode > 0)
		{
			if (!m_bDCOAssaultLocked || m_eDCOForced != DCO_GroupTactics.AGGRESIVE)
			{
				string ev = "tr_posture_lock";
				if (lockMode == 1)
					ev = "cqb_posture_lock";
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Owner), string.Format("%1 grp=%2 from=%3 to=AGGRESIVE reason=assault_rule", ev, DCO_Log.IdGroup(m_Owner), typename.EnumToString(DCO_GroupTactics, DCO_GetPosture())));
				DCO_ForcePosture(DCO_GroupTactics.AGGRESIVE, now_ms, 600000.0, false, "assault_rule");
				m_bDCOAssaultLocked = true;
			}
			return;
		}
		if (m_bDCOAssaultLocked)
		{
			m_bDCOAssaultLocked = false;
			DCO_ClearForced("assault_rule_end");
		}

		m_DCOEnemyTotal.Reset();
		float nearestEnemy = 99999;
		foreach (SCR_AIGroupTargetCluster c : m_Perception.m_aTargetClusters)
		{
			if (c.m_State && c.m_State.m_iCountAlive > 0)
			{
				m_DCOEnemyTotal.Add(DCO_GetClusterStrength(c.m_State));
				nearestEnemy = Math.Min(nearestEnemy, c.m_State.m_fDistMin);
			}
		}
		int enemies = m_DCOEnemyTotal.m_iCount;
		DCO_StrengthInfo mine = DCO_GetFriendlyStrength(m_DCOEnemyTotal.m_bHasVehicles);

		int shaken, broken, spirited;
		foreach (SCR_AIInfoComponent info : m_aInfoComponents)
		{
			if (!info || !info.GetUtilityComp())
				continue;
			DCO_AIMoraleSystem morale = info.GetUtilityComp().GetMoraleSystem();
			if (!morale)
				continue;
			moraleState ms = morale.GetState();
			if (ms == moraleState.BREAK)
				broken++;
			if (ms == moraleState.SHAKEN || ms == moraleState.PANIC || ms == moraleState.BREAK)
				shaken++;
			if (morale.IsSpirited())
				spirited++;
		}

		int peak = friends;
		if (utilDco)
			peak = Math.Max(utilDco.DCO_GetPeakStrength(), friends);
		float losses = 1.0 - 1.0 * friends / Math.Max(peak, 1.0);
		float ratio = 99;
		if (enemies > 0)
			ratio = mine.m_fStrength / Math.Max(m_DCOEnemyTotal.m_fStrength, 0.1);
		m_fDCOLastRatio = ratio;

		if (m_eDCOForced != DCO_GroupTactics.AUTOMATIC && !m_bDCOForcedUntilContact && now_ms >= m_fDCOForcedUntil_ms)
			DCO_ClearForced("expired");

		DCO_GroupTactics eff = DCO_GetPosture();
		if (eff == DCO_GroupTactics.DEFENSIVE && enemies > 0 && (ratio < 0.5 || losses >= 0.5) && DCO_EnemyInArea())
		{
			DCO_GroupTactics fall = DCO_GroupTactics.DELAY;
			if (ratio < 0.3)
				fall = DCO_GroupTactics.EVASIVE;
			DCO_Perf.Count("pos:overrun");
			DCO_ForcePosture(fall, now_ms, 120000.0, false, "overrun");
			return;
		}
		if (eff == DCO_GroupTactics.EVASIVE && now_ms - Math.Max(m_fDCOContactSeen_ms, m_fDCOPostureChanged_ms) > 60000.0)
		{
			DCO_Perf.Count("pos:evasive_quiet");
			DCO_ForcePosture(DCO_GroupTactics.BALANCE, now_ms, 0, true, "evasive_quiet");
			return;
		}

		bool assault = DCO_IsAssaultPosture();
		DCO_GroupTactics next = DCO_BasePosture(assault);
		string why = "base";
		int curTask = -1;
		if (utilDco)
			curTask = utilDco.GetTask();
		bool baseChanged = curTask != m_iDCOLastTask;
		m_iDCOLastTask = curTask;
		if (utilDco && utilDco.m_eDCOTablePosture != m_eDCOLastTable)
		{
			baseChanged = true;
			m_eDCOLastTable = utilDco.m_eDCOTablePosture;
		}

		if (assault)
		{
			if (broken * 2 > friends)
			{
				next = DCO_GroupTactics.EVASIVE;
				why = "break";
			}
			else if (shaken * 2 > friends && next == DCO_GroupTactics.AGGRESIVE)
			{
				next = DCO_GroupTactics.BALANCE;
				why = "assault_shaken";
				DCO_Perf.Count("tm:assault_shaken_slow");
			}
			else if (utilDco && utilDco.m_iDCOPosStage == 1 && (shaken * 2 > friends || losses >= 0.5 || ratio < 0.5 || utilDco.HasState(DCO_EGroupState.SUPPRESSED)))
				DCO_Perf.Count("exec_lock:reaction_kept");
		}
		else
		{
			int step = 0;
			bool evasive = false;
			float aggr = 1;
			DCO_GlobalAIComponent aggrCfg = DCO_GlobalAIComponent.GetFor(m_Owner);
			if (aggrCfg)
			{
				aggr = aggrCfg.DCO_AggressionSlider();
				if (aggrCfg.DCO_TuneF("bc.ladder_slider", 1.0) <= 0)
					aggr = aggrCfg.DCO_Aggression();
				else
				{
					DCO_Perf.Count("bc:ladder_slider");
					float ladderNow = GetGame().GetWorld().GetWorldTime() * 0.001;
					if (ladderNow - m_fDCOLadderLog_s >= 60.0)
					{
						m_fDCOLadderLog_s = ladderNow;
						DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("bc_ladder grp=%1 slider=%2 aggression=%3 used=%4", DCO_Log.IdGroup(m_Owner), aggr.ToString(-1, 2), aggrCfg.DCO_Aggression().ToString(-1, 2), aggr.ToString(-1, 2)));
					}
				}
			}
			float aggrRatio = Math.Max(aggr, 0.5);
			if (losses >= Math.Clamp(0.7 * aggr, 0.35, 0.95))
			{
				evasive = true;
				why = "casualties70";
			}
			else if (losses >= Math.Clamp(0.5 * aggr, 0.25, 0.9))
			{
				step--;
				why = "casualties50";
			}

			if (enemies == 0)
			{
				m_bDCORatioVeryLow = false;
				m_bDCORatioLow = false;
			}
			else
			{
				m_bDCORatioVeryLow = ratio < 0.3 / aggrRatio || (m_bDCORatioVeryLow && ratio < 0.45 / aggrRatio);
				m_bDCORatioLow = ratio < 0.5 / aggrRatio || (m_bDCORatioLow && ratio < 0.7 / aggrRatio);
			}
			bool suppressedGrp = utilDco && utilDco.HasState(DCO_EGroupState.SUPPRESSED);
			string evWhy = why;
			if (m_bDCORatioVeryLow && (losses >= 0.25 || suppressedGrp || nearestEnemy < 100.0))
			{
				evasive = true;
				why = "ratio03";
			}
			else if ((m_bDCORatioVeryLow || m_bDCORatioLow) && (losses > 0.01 || suppressedGrp || nearestEnemy < 250.0))
			{
				step--;
				why = "ratio05";
			}
			else if (m_bDCORatioVeryLow || m_bDCORatioLow)
			{
				DCO_Perf.Count("pos:ratio_hold");
				if (!m_bDCORatioHoldLogged)
				{
					m_bDCORatioHoldLogged = true;
					DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("posture_ratio_hold grp=%1 ratio=%2 nearest=%3 losses=%4 suppressed=%5", DCO_Log.IdGroup(m_Owner), ratio.ToString(-1, 2), Math.Round(nearestEnemy), losses.ToString(-1, 2), DCO_Util.B01(suppressedGrp)));
				}
			}
			if (!m_bDCORatioVeryLow && !m_bDCORatioLow)
				m_bDCORatioHoldLogged = false;
			if (evasive)
				evWhy = why;

			if (broken * 2 > friends)
			{
				evasive = true;
				why = "break";
				evWhy = why;
			}
			else if (shaken * 2 > friends)
			{
				step--;
				why = "shaken";
			}

			int conv = DCO_FriendlyGroupsConverging();
			m_bDCOConverging = conv >= 2 || (m_bDCOConverging && conv >= 1);
			if (m_bDCOConverging)
			{
				step++;
				why = "converging";
			}
			if (spirited * 2 > friends)
			{
				step++;
				why = "enemy_wavering";
			}
			if (aggr >= 1.5)
			{
				step++;
				why = "aggression_high";
			}
			else if (aggr <= 0.5)
			{
				step--;
				why = "aggression_low";
			}

			if (!evasive && step != 0 && DCO_LadderImmune(next))
			{
				if (step != m_iDCOLadderBlocked)
				{
					m_iDCOLadderBlocked = step;
					DCO_Perf.Count("pos:ladder_immune");
					DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("posture_ladder_block grp=%1 posture=%2 step=%3 reason=%4", DCO_Log.IdGroup(m_Owner),
						typename.EnumToString(DCO_GroupTactics, next), step, why));
				}
				step = 0;
				why = "base";
			}
			else if (step == 0)
				m_iDCOLadderBlocked = 0;

			if (evasive)
			{
				next = DCO_GroupTactics.EVASIVE;
				why = evWhy;
			}
			else if (step != 0)
			{
				int idx = DCO_LadderIndex(next) + step;
				next = DCO_LadderPosture(Math.ClampInt(idx, 0, 3));
			}
		}

		if (next == m_eDCOPosture)
			return;

		float postureHyst = 60000.0;
		float postureA = DCO_Stress.AggrRelGlobal();
		if (postureA != 1.0)
			postureHyst = Math.Max(60000.0 * Math.Pow(postureA, -0.5), 30000.0);
		if (next != DCO_GroupTactics.EVASIVE && !baseChanged && m_fDCOPostureChanged_ms >= 0 && now_ms - m_fDCOPostureChanged_ms < postureHyst)
		{
			if (postureA != 1.0)
				DCO_Perf.Count("agg:p7_posture_hold");
			return;
		}

		if (why == "base")
			why = m_sDCOBaseSrc;
		else
			why = "ladder:" + why;
		DCO_Perf.Count("posture_change:" + typename.EnumToString(DCO_GroupTactics, next));
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("posture_change grp=%1 from=%2 to=%3 reason=%4", DCO_Log.IdGroup(m_Owner),
			typename.EnumToString(DCO_GroupTactics, m_eDCOPosture), typename.EnumToString(DCO_GroupTactics, next), why));
		m_eDCOPosture = next;
		m_fDCOPostureChanged_ms = now_ms;
	}

	protected bool DCO_IsAssaultPosture()
	{
		if (DCO_IsClearingBuilding() || DCO_IsOvermatch())
			return true;
		if (!utilDco)
			return false;
		if (utilDco.m_bDCOAssaultCommitted)
			return true;
		DCO_EGroupTask t = utilDco.GetTask();
		if (t == DCO_EGroupTask.REINFORCE && utilDco.m_iDCOThreatAct < 0 && utilDco.GetGroupObjective() && !utilDco.GetGroupObjective().IsCapturedBy(utilDco.GetFactionKey()))
			return true;
		return t == DCO_EGroupTask.ATTACK || t == DCO_EGroupTask.FLANK;
	}

	protected static bool DCO_LadderImmune(DCO_GroupTactics p)
	{
		return p == DCO_GroupTactics.AMBUSH || p == DCO_GroupTactics.SNEAK || p == DCO_GroupTactics.DELAY;
	}

	protected vector DCO_NearestTargetPos(vector pos)
	{
		vector best = pos;
		float bestSq = float.MAX;
		foreach (SCR_AIGroupTargetCluster c : m_Perception.m_aTargetClusters)
		{
			foreach (SCR_AITargetInfo t : c.m_aTargets)
			{
				if (t.m_eCategory != EAITargetInfoCategory.DETECTED && t.m_eCategory != EAITargetInfoCategory.IDENTIFIED)
					continue;
				float d = vector.DistanceSqXZ(t.m_vWorldPos, pos);
				if (d < bestSq)
				{
					bestSq = d;
					best = t.m_vWorldPos;
				}
			}
		}
		return best;
	}

	protected void DCO_AssaultStuckLog(float dist, string action, int shooters)
	{
		DCO_Perf.Count("assault_stuck:" + action);
		DCO_Log.Write(DCO_ELogChannel.TACTIC, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Owner), string.Format("assault_stuck grp=%1 tactic=%2 dist=%3 action=%4 shooters=%5", DCO_Log.IdGroup(m_Owner), utilDco.m_sDCOPosTactic, Math.Round(dist), action, shooters));
	}

	protected void DCO_AssaultTick(float now_ms)
	{
		if (!utilDco || utilDco.m_bDCOAssaultCommitted || utilDco.m_iDCOPosStage != 1 || utilDco.m_eDCOTablePosture != DCO_GroupTactics.AGGRESIVE)
			return;
		IEntity leader = m_Owner.GetLeaderEntity();
		if (!leader)
			return;
		vector lp = leader.GetOrigin();
		array<AIAgent> agents = {};
		m_Owner.GetAgents(agents);
		float lastThrow = DCO_AssaultCommit.LastThrow(agents);
		if (lastThrow > utilDco.m_fDCOPrepAt_ms)
		{
			utilDco.m_fDCOPrepAt_ms = lastThrow;
			utilDco.m_sDCOPrepKind = "grenade";
		}
		vector enemy = DCO_NearestTargetPos(lp);
		float dist = vector.DistanceXZ(lp, enemy);
		string trigger;
		if (IsTargetNear(lp, 40.0) && DCO_AssaultCommit.Ready(dist, true, DCO_AssaultCommit.NearMisses(agents, now_ms, 6500.0), now_ms - utilDco.m_fDCOPrepAt_ms, utilDco.m_sDCOPrepKind, trigger))
		{
			utilDco.m_bDCOAssaultCommitted = true;
			DCO_AssaultCommit.Log(m_Owner, utilDco.m_sDCOPosTactic, dist, trigger);
			return;
		}
		if (!IsTargetNear(lp, 66.0) || IsTargetNear(lp, 35.0))
		{
			utilDco.m_fDCOCommitBand_ms = -1;
			return;
		}
		if (utilDco.m_fDCOCommitBand_ms < 0)
			utilDco.m_fDCOCommitBand_ms = now_ms;
		if (utilDco.m_iDCOCommitStep == 0 && now_ms - utilDco.m_fDCOCommitBand_ms > 60000.0)
		{
			utilDco.m_iDCOCommitStep = 1;
			utilDco.m_fDCOCommitStepAt_ms = now_ms;
			int shooters = DCO_AssaultCommit.Volley(agents, enemy);
			if (shooters > 0)
			{
				utilDco.m_fDCOPrepAt_ms = now_ms;
				utilDco.m_sDCOPrepKind = "volley";
			}
			DCO_AssaultStuckLog(dist, "volley", shooters);
			return;
		}
		if (utilDco.m_iDCOCommitStep == 1 && now_ms - utilDco.m_fDCOCommitStepAt_ms > 10000.0)
		{
			utilDco.m_iDCOCommitStep = 2;
			utilDco.m_fDCOCommitStepAt_ms = now_ms;
			int smoke = 0;
			if (DCO_AssaultCommit.Smoke(agents, enemy))
			{
				smoke = 1;
				utilDco.m_fDCOPrepAt_ms = now_ms;
				utilDco.m_sDCOPrepKind = "smoke";
			}
			DCO_AssaultStuckLog(dist, "smoke", smoke);
			return;
		}
		if (utilDco.m_iDCOCommitStep == 2 && now_ms - utilDco.m_fDCOCommitStepAt_ms > 10000.0)
		{
			utilDco.m_iDCOCommitStep = 3;
			utilDco.m_bDCOAssaultCommitted = true;
			DCO_AssaultStuckLog(dist, "commit", 0);
			DCO_AssaultCommit.Log(m_Owner, utilDco.m_sDCOPosTactic, dist, "volley");
		}
	}

	protected DCO_GroupTactics DCO_BasePosture(bool assault)
	{
		if (DCO_IsClearingBuilding() || DCO_IsOvermatch())
		{
			m_sDCOBaseSrc = "cqb_overmatch";
			return DCO_GroupTactics.AGGRESIVE;
		}
		if (!utilDco)
			return DCO_GroupTactics.BALANCE;
		if (utilDco.m_bDCOAssaultCommitted)
		{
			m_sDCOBaseSrc = "assault_commit";
			return DCO_GroupTactics.AGGRESIVE;
		}
		DCO_GroupTactics p = utilDco.m_eDCOCmdPosture;
		m_sDCOBaseSrc = "cmd_pin";
		if (p == DCO_GroupTactics.AUTOMATIC)
		{
			p = DCO_PostureTable.Resolve(this, utilDco, false);
			m_sDCOBaseSrc = "table:" + utilDco.m_sDCOPosRow;
		}
		if (p == DCO_GroupTactics.SNEAK && DCO_TargetDist() > 300.0)
			return DCO_GroupTactics.BALANCE;
		return p;
	}

	protected float DCO_TargetDist()
	{
		IEntity leader = m_Owner.GetLeaderEntity();
		if (!leader)
			return 0;
		vector c;
		if (utilDco && utilDco.GetGroupObjective() && utilDco.GetGroupObjective().GetOwner())
			c = utilDco.GetGroupObjective().GetOwner().GetOrigin();
		else if (m_Owner.GetCurrentWaypoint())
			c = m_Owner.GetCurrentWaypoint().GetOrigin();
		else
			return 0;
		return vector.DistanceXZ(leader.GetOrigin(), c);
	}

	protected bool DCO_EnemyInArea()
	{
		vector c;
		float r;
		bool holdsArea = !utilDco || utilDco.GetTask() == DCO_EGroupTask.DEFEND || utilDco.GetTask() == DCO_EGroupTask.GARRISON || utilDco.GetTask() == DCO_EGroupTask.NONE;
		if (!holdsArea && m_Owner.GetLeaderEntity())
			return IsTargetNear(m_Owner.GetLeaderEntity().GetOrigin(), 40.0);
		if (utilDco && utilDco.GetGroupObjective() && utilDco.GetGroupObjective().GetOwner())
		{
			c = utilDco.GetGroupObjective().GetOwner().GetOrigin();
			r = utilDco.GetGroupObjective().GetRadius();
		}
		else if (m_Owner.GetCurrentWaypoint())
		{
			c = m_Owner.GetCurrentWaypoint().GetOrigin();
			r = Math.Max(m_Owner.GetCurrentWaypoint().GetCompletionRadius(), 30.0);
		}
		else if (m_Owner.GetLeaderEntity())
		{
			c = m_Owner.GetLeaderEntity().GetOrigin();
			r = 40.0;
		}
		else
			return false;
		return IsTargetNear(c, r);
	}

	protected static int DCO_LadderIndex(DCO_GroupTactics p)
	{
		switch (p)
		{
			case DCO_GroupTactics.EVASIVE: return 0;
			case DCO_GroupTactics.DEFENSIVE: return 1;
			case DCO_GroupTactics.AGGRESIVE: return 3;
		}
		return 2;
	}

	protected static DCO_GroupTactics DCO_LadderPosture(int idx)
	{
		switch (idx)
		{
			case 0: return DCO_GroupTactics.EVASIVE;
			case 1: return DCO_GroupTactics.DEFENSIVE;
			case 3: return DCO_GroupTactics.AGGRESIVE;
		}
		return DCO_GroupTactics.BALANCE;
	}

	protected int DCO_FriendlyGroupsConverging()
	{
		if (!utilDco || !utilDco.GetMyCommander())
			return 0;
		vector target;
		if (utilDco.GetGroupObjective() && utilDco.GetGroupObjective().GetOwner())
			target = utilDco.GetGroupObjective().GetOwner().GetOrigin();
		else if (m_Owner.GetCurrentWaypoint())
			target = m_Owner.GetCurrentWaypoint().GetOrigin();
		else
			return 0;

		int n;
		foreach (DCO_GroupUtilityComponent o : utilDco.GetMyCommander().GetOwnedGroups())
		{
			if (!o || o == utilDco || !o.GetOwner() || o.GetUnitCount() <= 0)
				continue;
			vector ot;
			SCR_AIGroup og = SCR_AIGroup.Cast(o.GetOwner());
			if (o.GetGroupObjective() && o.GetGroupObjective().GetOwner())
				ot = o.GetGroupObjective().GetOwner().GetOrigin();
			else if (og && og.GetCurrentWaypoint())
				ot = og.GetCurrentWaypoint().GetOrigin();
			else
				continue;
			if (vector.DistanceXZ(ot, target) <= 150.0 && vector.DistanceXZ(o.GetOwner().GetOrigin(), target) <= 400.0)
				n++;
		}
		return n;
	}

	protected string m_sDCOROEReason = "-";
	protected string m_sDCOROESource = "commander";
	protected EAIGroupCombatMode m_eDCOROELogged = -1;

	string DCO_GetROEReason() { return m_sDCOROEReason; }

	override void EvaluateCombatMode()
	{
		m_sDCOROEReason = "vanilla";
		m_sDCOROESource = "vanilla";
		DCO_EvaluateCombatModeInner();
		if (m_eCombatModeActual == m_eDCOROELogged)
			return;
		m_eDCOROELogged = m_eCombatModeActual;
		DCO_Perf.Count("roe:" + m_sDCOROEReason);
		if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("roe_mode grp=%1 mode=%2 reason=%3 source=%4", DCO_Log.IdGroup(m_Owner), typename.EnumToString(EAIGroupCombatMode, m_eCombatModeActual), m_sDCOROEReason, m_sDCOROESource));
	}

	protected void DCO_EvaluateCombatModeInner()
	{
		EAIGroupCombatMode postureMode;
		if (DCO_PostureOps.ROE(this, utilDco, postureMode))
		{
			m_sDCOROEReason = "posture";
			m_sDCOROESource = "dco";
			m_eCombatModeActual = postureMode;
			return;
		}

		if (!utilDco)
			utilDco = DCO_GroupUtilityComponent.Cast(m_Owner.FindComponent(DCO_GroupUtilityComponent));

		if (!AICommander_ManagerComponent.GetInstance() || !utilDco || !utilDco.GetMyCommander())
		{
			if (!utilDco || utilDco.GetTask() == DCO_EGroupTask.NONE)
			{
				super.EvaluateCombatMode();
				return;
			}
			DCO_EvaluateNoCommander();
			return;
		}
		m_sDCOROESource = "commander";

	    if (m_eCombatModeExternal != EAIGroupCombatMode.RETURN_FIRE)
	    {
	        m_sDCOROEReason = "external";
	        m_eCombatModeActual = m_eCombatModeExternal;
	        return;
	    }

	    DCO_GroupUtilityComponent groupUtil = DCO_GroupUtilityComponent.Cast(m_Owner.FindComponent(DCO_GroupUtilityComponent));
	    if (!groupUtil.GetMyCommander())
			return;

	    int targetCount = m_Perception.m_aTargetEntities.Count();
	    if (targetCount == 0)
	    {
	        DCO_UpdateContact(groupUtil, false);
	        m_sDCOROEReason = "no_targets";
	        m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;
	        return;
	    }

	    bool threatened = IsDirectlyThreatened();
	    DCO_UpdateContact(groupUtil, threatened);
	    if (threatened)
	    {
	        m_sDCOROEReason = "threatened";
	        m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
	        return;
	    }

		m_eCombatModeActual = DCO_ResolveROE(groupUtil);
	}

	protected void DCO_EvaluateNoCommander()
	{
		m_sDCOROESource = "no_commander_task";
		if (m_eCombatModeExternal != EAIGroupCombatMode.RETURN_FIRE)
		{
			m_sDCOROEReason = "external";
			m_eCombatModeActual = m_eCombatModeExternal;
			return;
		}
		bool threatened = m_Perception.m_aTargetEntities.Count() > 0 && IsDirectlyThreatened();
		DCO_UpdateContact(utilDco, threatened);
		string block;
		if (threatened)
			m_sDCOROEReason = "threatened";
		else if (DCO_InContact())
			m_sDCOROEReason = "contact_active";
		else if (m_Perception.m_aTargetEntities.Count() > 0 && IsAnyTargetRelevant(DCO_OpenFireRange(utilDco, false, false, block), false))
			m_sDCOROEReason = "roe_tight_relevant";
		else
		{
			m_sDCOROEReason = "roe_tight_no_relevant";
			m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;
			return;
		}
		m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
	}

	protected EAIGroupCombatMode DCO_ResolveROE(DCO_GroupUtilityComponent groupUtil)
	{
		AICommander_BaseComponent cmd = groupUtil.GetMyCommander();
		DCO_ROETable roe = cmd.GetROETable();
		int state = groupUtil.GetState();

		if (groupUtil.IsBreakingContact())
		{
			m_sDCOROEReason = "breaking_contact";
			return EAIGroupCombatMode.FIRE_AT_WILL;
		}
		float voiceNow = DCO_Util.NowS();
		if (groupUtil.m_fDCOVoiceReconFireUntil > voiceNow)
		{
			m_sDCOROEReason = "voice_recon";
			return EAIGroupCombatMode.FIRE_AT_WILL;
		}
		if (groupUtil.m_fDCOVoiceAmbushUntil > voiceNow)
		{
			IEntity vl = m_Owner.GetLeaderEntity();
			float nearest = float.MAX;
			if (vl)
			{
				foreach (SCR_AITargetInfo vt : m_Perception.m_aTargets)
				{
					if (vt)
						nearest = Math.Min(nearest, vector.DistanceXZ(vl.GetOrigin(), vt.m_vWorldPos));
				}
			}
			if (nearest >= 50.0)
			{
				m_sDCOROEReason = "voice_ambush_wait";
				return EAIGroupCombatMode.HOLD_FIRE;
			}
			groupUtil.m_fDCOVoiceAmbushUntil = 0;
			DCO_Perf.Count("voice:ambush_sprung");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("voice_ambush_sprung grp=%1 dist=%2", m_Owner, Math.Round(nearest)));
			m_sDCOROEReason = "voice_ambush_sprung";
			return EAIGroupCombatMode.FIRE_AT_WILL;
		}
		switch (groupUtil.m_eSOPAction)
		{
			case DCO_ESOPAction.ENGAGE:
			{
				m_sDCOROEReason = "sop_engage";
				return EAIGroupCombatMode.FIRE_AT_WILL;
			}
			case DCO_ESOPAction.OBSERVE:
			case DCO_ESOPAction.BYPASS:
			{
				m_sDCOROEReason = "sop_observe_bypass";
				return EAIGroupCombatMode.HOLD_FIRE;
			}
			case DCO_ESOPAction.AMBUSH:
			{
				if (groupUtil.m_bSOPSprung)
				{
					m_sDCOROEReason = "sop_ambush_sprung";
					return EAIGroupCombatMode.FIRE_AT_WILL;
				}
				m_sDCOROEReason = "sop_ambush_wait";
				return EAIGroupCombatMode.HOLD_FIRE;
			}
		}
		if (state & DCO_EGroupState.RETREATING)
		{
			m_sDCOROEReason = "retreating";
			return EAIGroupCombatMode.HOLD_FIRE;
		}

		if (DCO_InContact())
		{
			m_sDCOROEReason = "contact_active";
			return EAIGroupCombatMode.FIRE_AT_WILL;
		}

		DCO_EGroupTask task = groupUtil.GetTask();
		if (groupUtil.IsMortar())
			task = DCO_EGroupTask.FIRE_MISSION;
		else if (groupUtil.IsArmor() && task == DCO_EGroupTask.NONE)
			task = DCO_EGroupTask.PATROL;

		DCO_ROEEntry entry = roe.Get(task);
		if (!entry)
		{
			m_sDCOROEReason = "no_roe_entry";
			return EAIGroupCombatMode.HOLD_FIRE;
		}

		DCO_EROEMode mode = entry.m_eMode;
		string rangeBlock;
		float dist = DCO_OpenFireRange(groupUtil, false, false, rangeBlock);
		if (mode == DCO_EROEMode.HOLD)
			mode = DCO_EROEMode.TIGHT;

		if ((state & DCO_EGroupState.SUPPRESSED) && roe.m_bFireWhenSuppressed)
			mode = DCO_EROEMode.FREE;

		if (DCO_Air.On() && task != DCO_EGroupTask.RECON)
		{
			float airNear = float.MAX;
			foreach (SCR_AIGroupTargetCluster ac : m_Perception.m_aTargetClusters)
			{
				if (ac)
					airNear = Math.Min(airNear, DCO_Air.ClusterAirDist(ac.m_State, 5.0));
			}
			if (airNear < DCO_Air.OpenRange())
			{
				DCO_Air.Note("roe_target", DCO_Log.IdGroup(m_Owner), airNear);
				m_sDCOROEReason = "air_target";
				return EAIGroupCombatMode.FIRE_AT_WILL;
			}
		}

		switch (mode)
		{
			case DCO_EROEMode.FREE:
			{
				m_sDCOROEReason = "roe_free";
				return EAIGroupCombatMode.FIRE_AT_WILL;
			}

			case DCO_EROEMode.TIGHT:
			{
				if (IsAnyTargetRelevant(dist, !roe.m_bEngageDetected))
				{
					m_sDCOROEReason = "roe_tight_relevant";
					return EAIGroupCombatMode.FIRE_AT_WILL;
				}

				CMD_AICommanderObjectiveComponent obj = groupUtil.GetGroupObjective();
				if (task == DCO_EGroupTask.SUPPORT_BY_FIRE && obj && groupUtil.IsArmor() && IsAnyTargetRelevant(800.0, !roe.m_bEngageDetected)
					&& IsTargetNear(obj.GetOwner().GetOrigin(), obj.GetRadius() + 150.0))
				{
					float vnow = DCO_Util.NowS();
					if (vnow >= m_fDCOVehSbfLogAt)
					{
						m_fDCOVehSbfLogAt = vnow + 30.0;
						DCO_Perf.Count("veh_sbf_engage");
						DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("veh_sbf_engage grp=%1 dist=%2 tgt=objective_area range=800", m_Owner.GetName(), Math.Round(vector.DistanceXZ(m_Owner.GetOrigin(), obj.GetOwner().GetOrigin()))));
					}
					m_sDCOROEReason = "veh_sbf";
					return EAIGroupCombatMode.FIRE_AT_WILL;
				}
				if ((task == DCO_EGroupTask.DEFEND || task == DCO_EGroupTask.GARRISON) && obj
					&& IsTargetNear(obj.GetOwner().GetOrigin(), obj.GetRadius() + cmd.GetDefendObjectiveMargin()))
				{
					m_sDCOROEReason = "roe_tight_defend_near";
					return EAIGroupCombatMode.FIRE_AT_WILL;
				}
				break;
			}
		}
		m_sDCOROEReason = "roe_tight_no_relevant";
		return EAIGroupCombatMode.HOLD_FIRE;
	}

	protected float m_fDCOContactUntil = -1;
	protected float m_fDCOContactLastInfo = -1;
	protected float m_fDCORecentUntil = -1;
	protected float m_fDCOContactExtendLog;
	protected float m_fDCOHoldBlockedLog;
	protected string m_sDCOPendingCause;
	protected IEntity m_DCOSeenShooter;
	protected IEntity m_DCOSeenTarget;
	protected float m_fDCOSeenDist;
	protected string m_sDCOSeenBlock;

	void DCO_NoteContact(string cause)
	{
		m_sDCOPendingCause = cause;
	}

	bool DCO_InContact()
	{
		return m_fDCOContactUntil > 0 && DCO_Util.NowS() < m_fDCOContactUntil;
	}

	protected DCO_EGroupTask DCO_ROETask(DCO_GroupUtilityComponent g)
	{
		DCO_EGroupTask task = g.GetTask();
		if (g.IsMortar())
			task = DCO_EGroupTask.FIRE_MISSION;
		else if (g.IsArmor() && task == DCO_EGroupTask.NONE)
			task = DCO_EGroupTask.PATROL;
		return task;
	}

	protected float DCO_OpenFireRange(DCO_GroupUtilityComponent g, bool mg, bool crewTarget, out string block)
	{
		block = "";
		DCO_EGroupTask task = DCO_ROETask(g);
		AICommander_BaseComponent rc = g.GetMyCommander();
		DCO_ROETable roe;
		float r = 50.0;
		if (rc)
		{
			roe = rc.GetROETable();
			DCO_ROEEntry entry = roe.Get(task);
			if (entry)
				r = entry.m_fEngageDist;
			if (!entry || entry.m_eMode == DCO_EROEMode.HOLD)
				r = Math.Max(50.0, roe.m_fHoldEngageDist);
		}
		else if (task == DCO_EGroupTask.ATTACK || task == DCO_EGroupTask.FLANK || task == DCO_EGroupTask.REINFORCE || task == DCO_EGroupTask.DEFEND || task == DCO_EGroupTask.GARRISON)
			r = 300.0;
		else if (task == DCO_EGroupTask.SUPPORT_BY_FIRE)
			r = 600.0;
		else if (task == DCO_EGroupTask.PATROL)
			r = 200.0;
		switch (task)
		{
			case DCO_EGroupTask.ATTACK:
			case DCO_EGroupTask.FLANK:
			case DCO_EGroupTask.REINFORCE:
			{
				if (g.m_iDCOTRPhase == DCO_ETRPhase.MOVING || g.m_iDCOTRPhase == DCO_ETRPhase.STAGING)
				{
					block = "pre_go";
					r = 50.0;
				}
				else if (mg)
					r = Math.Min(r * 2.0, 800.0);
				break;
			}
			case DCO_EGroupTask.SUPPORT_BY_FIRE:
			{
				if (mg)
					r = Math.Min(r * 1.35, 800.0);
				break;
			}
			case DCO_EGroupTask.DEFEND:
			case DCO_EGroupTask.GARRISON:
			{
				if (mg)
					r = Math.Min(r * 2.0, 800.0);
				r *= DCO_Night.EngageMultiplier();
				break;
			}
			case DCO_EGroupTask.RECON:
			{
				block = "recon";
				break;
			}
			case DCO_EGroupTask.PATROL:
				break;
			default:
			{
				block = "self_defense";
				break;
			}
		}
		if (crewTarget && task != DCO_EGroupTask.RECON && r < 300.0)
		{
			r = 300.0;
			block = "";
		}
		if (roe)
			r *= roe.m_fDistScale;
		if (DCO_Util.NowS() < m_fDCORecentUntil)
			r *= 1.2;
		return r;
	}

	protected bool DCO_IsCrewTarget(IEntity tgt)
	{
		if (Vehicle.Cast(tgt))
			return true;
		ChimeraCharacter tc = ChimeraCharacter.Cast(tgt);
		return tc && tc.IsInVehicle();
	}

	protected bool DCO_MemberSeesEnemy(DCO_GroupUtilityComponent g)
	{
		m_DCOSeenShooter = null;
		m_DCOSeenTarget = null;
		m_sDCOSeenBlock = "";
		array<AIAgent> agents = {};
		m_Owner.GetAgents(agents);
		float bestHeld = float.MAX;
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent || !ca.m_UtilityComponent.m_CombatComponent)
				continue;
			BaseTarget t = ca.m_UtilityComponent.m_CombatComponent.GetCurrentTarget();
			if (!t || t.GetTimeSinceSeen() > 2.0)
				continue;
			IEntity tgt = t.GetTargetEntity();
			IEntity me = a.GetControlledEntity();
			if (!tgt || !me || !DCO_Util.IsAlive(tgt))
				continue;
			float d = vector.Distance(me.GetOrigin(), tgt.GetOrigin());
			string block;
			float r = DCO_OpenFireRange(g, DCO_Util.IsMGShooter(me), DCO_IsCrewTarget(tgt), block);
			if (DCO_Air.On() && DCO_ROETask(g) != DCO_EGroupTask.RECON && DCO_Air.IsAir(tgt) && r < DCO_Air.OpenRange())
			{
				r = DCO_Air.OpenRange();
				block = "";
				DCO_Perf.Count("air:roe_range");
			}
			if (d <= r)
			{
				m_DCOSeenShooter = me;
				m_DCOSeenTarget = tgt;
				m_fDCOSeenDist = d;
				return true;
			}
			if (d < bestHeld)
			{
				bestHeld = d;
				m_DCOSeenShooter = me;
				m_DCOSeenTarget = tgt;
				m_fDCOSeenDist = d;
				m_sDCOSeenBlock = block;
				if (block.IsEmpty())
					m_sDCOSeenBlock = "out_of_range";
			}
		}
		return false;
	}

	protected float DCO_NearestClusterDist(out bool fresh)
	{
		fresh = false;
		float nearest = float.MAX;
		foreach (SCR_AIGroupTargetCluster c : m_Perception.m_aTargetClusters)
		{
			if (!c.m_State || c.m_State.m_iCountAlive <= 0)
				continue;
			if (c.m_State.GetTimeSinceLastNewInformation() < 5.0)
				fresh = true;
			nearest = Math.Min(nearest, c.m_State.m_fDistMin);
		}
		return nearest;
	}

	protected void DCO_UpdateContact(DCO_GroupUtilityComponent g, bool threatened)
	{
		float now = DCO_Util.NowS();
		bool active = DCO_InContact();
		bool fresh;
		float nearest = DCO_NearestClusterDist(fresh);
		string cause;
		if (threatened)
			cause = "fired_upon";
		else if (DCO_MemberSeesEnemy(g))
			cause = "visual";
		else if (!m_sDCOPendingCause.IsEmpty())
			cause = m_sDCOPendingCause;
		else if (active && fresh)
			cause = "info";
		m_sDCOPendingCause = "";

		if (cause.IsEmpty() && m_DCOSeenTarget && now >= m_fDCOHoldBlockedLog)
		{
			m_fDCOHoldBlockedLog = now + 10.0;
			DCO_Perf.Count("roe_hold_blocked:" + m_sDCOSeenBlock);
			if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO))
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_DCOSeenShooter), string.Format("roe_hold_blocked grp=%1 tgt=%2 dist=%3 reason=%4 task=%5",
					DCO_Log.IdGroup(m_Owner), DCO_Log.IdUnit(m_DCOSeenTarget), Math.Round(m_fDCOSeenDist), m_sDCOSeenBlock, typename.EnumToString(DCO_EGroupTask, DCO_ROETask(g))));
		}

		if (cause.IsEmpty())
		{
			if (m_fDCOContactUntil > 0 && now >= m_fDCOContactUntil)
			{
				m_fDCOContactUntil = -1;
				m_fDCORecentUntil = now + 180.0;
				DCO_Perf.Count("roe_contact_end");
				if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO))
					DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("roe_contact_end grp=%1 silent_s=%2 recent_s=180", DCO_Log.IdGroup(m_Owner), Math.Round(now - m_fDCOContactLastInfo)));
			}
			return;
		}

		float hold = 90.0;
		if (nearest < 100.0 || (cause == "visual" && m_fDCOSeenDist < 100.0))
			hold = 60.0;
		if (DCO_ROETask(g) == DCO_EGroupTask.RECON)
			hold = 30.0;
		m_fDCOContactUntil = now + hold;
		m_fDCOContactLastInfo = now;

		if (!active)
		{
			DCO_Perf.Count("roe_contact_start:" + cause);
			if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO))
			{
				float d = nearest;
				if (cause == "visual")
					d = m_fDCOSeenDist;
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("roe_contact_start grp=%1 cause=%2 dist=%3 task=%4 hold_s=%5", DCO_Log.IdGroup(m_Owner), cause, Math.Round(d), typename.EnumToString(DCO_EGroupTask, DCO_ROETask(g)), hold));
				if (cause == "visual")
					DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_DCOSeenShooter), string.Format("roe_open_fire grp=%1 tgt=%2 dist=%3 crew_target=%4", DCO_Log.IdGroup(m_Owner), DCO_Log.IdUnit(m_DCOSeenTarget), Math.Round(m_fDCOSeenDist), DCO_IsCrewTarget(m_DCOSeenTarget)));
			}
		}
		else if (now >= m_fDCOContactExtendLog)
		{
			m_fDCOContactExtendLog = now + 15.0;
			if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, "-", string.Format("roe_contact_extend grp=%1 cause=%2 hold_s=%3", DCO_Log.IdGroup(m_Owner), cause, hold));
		}
	}

	protected bool IsDirectlyThreatened()
	{
		foreach (SCR_AIGroupTargetCluster c : m_Perception.m_aTargetClusters)
		{
			if (!c.m_State)
				continue;

			bool hasThreat = (c.m_State.m_iCountEndangering > 0 && c.m_State.m_iCountAlive > 0);
			bool isFresh   = (c.m_State.GetTimeSinceLastNewInformation() < 5.0);

			if (hasThreat && isFresh)
				return true;

			bool isVeryClose = (c.m_State.m_fDistMin < 30.0);
			if (isVeryClose)
				return true;

			if (DCO_Air.On())
			{
				float airD = DCO_Air.ClusterAirDist(c.m_State, 5.0);
				if (airD < DCO_Ctx.TuneFor(m_Owner, "air.contact_m", 400.0))
				{
					DCO_Air.Note("roe_threat", DCO_Log.IdGroup(m_Owner), airD);
					return true;
				}
			}
		}

		return false;
	}

	protected bool IsTargetNear(vector pos, float radius)
	{
		float radiusSq = radius * radius;
		foreach (SCR_AIGroupTargetCluster c : m_Perception.m_aTargetClusters)
		{
			foreach (SCR_AITargetInfo t : c.m_aTargets)
			{
				if (t.m_eCategory != EAITargetInfoCategory.DETECTED && t.m_eCategory != EAITargetInfoCategory.IDENTIFIED)
					continue;

				if (vector.DistanceSqXZ(t.m_vWorldPos, pos) <= radiusSq)
					return true;
			}
		}
		return false;
	}

	protected bool IsAnyTargetRelevant(float maxRelevanceDistance = 150.0, bool identifiedOnly = false)
	{
		foreach (SCR_AIGroupTargetCluster c : m_Perception.m_aTargetClusters)
		{
			if (!c.m_State)
				continue;

			if (identifiedOnly && !DCO_HasIdentifiedWithin(c, maxRelevanceDistance))
			{
				if (c.m_State.m_iCountEndangering > 0 && c.m_State.m_iCountAlive > 0 && c.m_State.GetTimeSinceLastNewInformation() < 5.0)
					return true;
				continue;
			}

			bool hasThreat = (c.m_State.m_iCountEndangering > 0 && c.m_State.m_iCountAlive > 0);

			bool isFresh = (c.m_State.GetTimeSinceLastNewInformation() < 5.0);

			bool isRelevantDistance = (c.m_State.m_fDistMin < maxRelevanceDistance);

			if ((hasThreat && isFresh) || isRelevantDistance)
				return true;
		}

		return false;
	}

	protected bool DCO_HasIdentifiedWithin(SCR_AIGroupTargetCluster c, float dist)
	{
		IEntity leader = m_Owner.GetLeaderEntity();
		if (!leader)
			return false;

		float distSq = dist * dist;
		vector p = leader.GetOrigin();
		foreach (SCR_AITargetInfo t : c.m_aTargets)
		{
			if (t.m_eCategory == EAITargetInfoCategory.IDENTIFIED && vector.DistanceSq(t.m_vWorldPos, p) <= distSq)
				return true;
		}
		return false;
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		SCR_AIGroupTacticsComponent tacticsComp = SCR_AIGroupTacticsComponent.Cast(owner.FindComponent(SCR_AIGroupTacticsComponent));
		if (tacticsComp)
			m_eDCOPostureOverride = tacticsComp.GetGroupTactics();
		utilDco = DCO_GroupUtilityComponent.Cast(owner.FindComponent(DCO_GroupUtilityComponent));
		if (utilDco)
			utilDco.perc = m_Perception;
		DCO_ContactSharing.Register(this);
	}
}
modded class SCR_AIGroupFireteamManager
{
	array<ref SCR_AIGroupFireteam> DCO_Fireteams()
	{
		return m_aFireteams;
	}

	bool DCO_IsBoundingTurn(AIAgent agent, out bool outBounding, float period_ms = 8000.0)
	{
		outBounding = false;
		if (m_Group && m_Group.GetGroupUtilityComponent() && m_Group.GetGroupUtilityComponent().DCO_GetUtil() && m_Group.GetGroupUtilityComponent().DCO_GetUtil().m_bDCOAssaultCommitted)
			return true;
		SCR_AIGroupFireteam mine = FindFireteam(agent);
		if (!mine || mine.Type() != SCR_AIGroupFireteam)
			return true;
		if (DCO_TeamBound.On(m_Group))
			return DCO_TeamBound.Turn(this, m_Group, m_aFireteams, mine, outBounding);

		int myIndex = -1;
		int count = 0;
		foreach (SCR_AIGroupFireteam ft : m_aFireteams)
		{
			if (ft.Type() != SCR_AIGroupFireteam)
				continue;

			if (ft == mine)
				myIndex = count;
			count++;
		}

		if (count < 2)
			return true;

		outBounding = true;
		int turn = (int)(GetGame().GetWorld().GetWorldTime() / period_ms) % count;
		return myIndex == turn;
	}
}
