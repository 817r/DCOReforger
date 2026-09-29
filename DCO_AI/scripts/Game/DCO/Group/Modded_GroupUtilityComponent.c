void SCR_AIOnTacticChange(SCR_AIInfoComponent agent, DCO_GroupTactics tactics);
typedef func SCR_AIOnTacticChange;

modded class SCR_AIGroupUtilityComponent
{
	const float SUPPRESS_MAX_CLUSTER_INFO_AGE_S = 120;
	const float SUPPRESS_OLD_CLUSTER_INFO_AGE_S = 20;
	const float SUPPRESS_MAX_DESTROYED_CLUSTER_INFO_AGE_S = 10;
	const float SUPPRESS_MIN_DIST_TO_CLUSTER_M = 20;
	const float SUPPRESS_MAX_DIST_TO_CLUSTER_M = 1500;

	protected const float CLOSE_VISIBLE_TRIGGER_DIST = 30.0;

	ref ScriptInvokerBase<SCR_AIOnTacticChange> m_OnTacticsChange = new ScriptInvokerBase<SCR_AIOnTacticChange>();
	protected DCO_GroupUtilityComponent utilDco;


	protected DCO_GroupTactics m_eDCOPostureOverride = DCO_GroupTactics.AUTOMATIC;
	protected DCO_GroupTactics m_eDCOPosture = DCO_GroupTactics.BALANCE;
	protected float m_fDCONextPostureEval_ms;
	protected float m_fDCOPostureChanged_ms = -1;
	protected float m_fDCOFirstContact_ms   = -1;

	protected float m_fDCONoWaypointSince_ms = -1;

	protected DCO_AIGarrisonActivity m_DCOGarrison;

	protected ref DCO_StragglerWatch m_DCOStragglers = new DCO_StragglerWatch();

	ref array<ref DCO_ShareTrack> m_aDCOShareTracks = {};
	ref map<IEntity, float> m_mDCOInjected = new map<IEntity, float>();

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

	override protected void OnAgentAdded(AIAgent agent)
	{
		super.OnAgentAdded(agent);
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

		return m_eDCOPosture;
	}

	DCO_GroupTactics DCO_GetPostureOverride()
	{
		return m_eDCOPostureOverride;
	}

	void DCO_SetPostureOverride(DCO_GroupTactics posture)
	{
		m_eDCOPostureOverride = posture;
	}

	override SCR_AIActionBase EvaluateActivity(out bool restartActivity)
	{
		SCR_AIActionBase activity = super.EvaluateActivity(restartActivity);
		if (!m_Perception)
			return activity;

		float now_ms = GetGame().GetWorld().GetWorldTime();

		if (m_Owner.IsSlave() || m_Owner.GetCurrentWaypoint())
			m_fDCONoWaypointSince_ms = -1;
		else if (m_fDCONoWaypointSince_ms < 0)
			m_fDCONoWaypointSince_ms = now_ms;

		if (now_ms >= m_fDCONextPostureEval_ms)
		{
			m_fDCONextPostureEval_ms = now_ms + 3000.0;
			int st = DCO_Perf.Begin();
			DCO_UpdatePosture(now_ms);
			DCO_Perf.End("grp_posture", st);
		}

		int ot = DCO_Perf.Begin();
		DCO_UpdateOvermatch(now_ms);
		DCO_Perf.End("grp_overmatch", ot);

		int ct = DCO_Perf.Begin();
		DCO_UpdateCQB(now_ms);
		DCO_Perf.End("grp_cqb", ct);

		int nt = DCO_Perf.Begin();
		DCO_UpdateNight(now_ms);
		DCO_Perf.End("grp_night", nt);

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

		return activity;
	}

	bool DCO_IsIdle()
	{
		if (m_fDCONoWaypointSince_ms < 0)
			return false;

		float enter_s = 30;
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
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
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (cfg)
			return cfg.GetSuperiorRatio();
		return 2;
	}

	bool DCO_IsSuperiorTo(SCR_AITargetClusterState s)
	{
		if (!s || s.m_iCountAlive <= 0 || m_fThreatMeasure >= 0.85)
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

		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
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
			else if (m_fThreatMeasure >= 0.85)
				DCO_EndOvermatch("suppressed");
			else if (DCO_OvermatchRatio(s) < threshold - 0.2)
				DCO_EndOvermatch("ratio");
			else if (DCO_HasOtherContact(s))
				DCO_EndOvermatch("contact");
			return;
		}

		if (m_fThreatMeasure >= 0.85)
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
			DCO_BenchmarkLoggerComponent.Event(string.Format("overmatch_start grp=%1 ratio=%2 need=%3 dist=%4",
				m_Owner, ratio.ToString(-1, 2), threshold.ToString(-1, 2), Math.Round(s.m_fDistMin)));
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
			DCO_BenchmarkLoggerComponent.Event(string.Format("overmatch_done grp=%1 dist=%2", m_Owner, Math.Round(dist)));
		else
			DCO_BenchmarkLoggerComponent.Event(string.Format("overmatch_abort grp=%1 reason=%2 dist=%3", m_Owner, reason, Math.Round(dist)));
	}


	protected ref DCO_CQBClear m_DCOCQB;
	protected float m_fDCONextCQB_ms;

	bool DCO_IsClearingBuilding()
	{
		return m_DCOCQB != null;
	}

	protected string DCO_FactionKey()
	{
		Faction f = m_Owner.GetFaction();
		if (!f)
			return string.Empty;
		return f.GetFactionKey();
	}

	protected void DCO_UpdateCQB(float now_ms)
	{
		if (m_DCOCQB)
		{
			if (now_ms >= m_fDCONextCQB_ms)
			{
				m_fDCONextCQB_ms = now_ms + 1000;
				if (!m_DCOCQB.Update(now_ms))
					m_DCOCQB = null;
			}
			return;
		}

		if (now_ms < m_fDCONextCQB_ms)
			return;
		m_fDCONextCQB_ms = now_ms + 2000.0;

		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (!cfg || !cfg.GetCQBEnabled() || !Replication.IsServer())
			return;

		string faction = DCO_FactionKey();
		if (faction.IsEmpty())
			return;

		IEntity building = DCO_ScanBuildingContacts(faction, now_ms);
		if (!building || !DCO_CQBTaskAllows())
			return;

		if (!DCO_BuildingClear.TryBook(building, faction, m_Owner, now_ms))
			return;

		m_DCOCQB = new DCO_CQBClear(this, building, faction, "contact", false, now_ms);
		m_fDCONextCQB_ms = now_ms + 1000;
	}

	bool DCO_StartSweep(IEntity building, float now_ms)
	{
		if (m_DCOCQB || !building || m_Owner.GetAgentsCount() < 2 || !DCO_OvermatchTaskAllows())
			return false;

		string faction = DCO_FactionKey();
		if (faction.IsEmpty() || !DCO_BuildingClear.TryBook(building, faction, m_Owner, now_ms))
			return false;

		m_DCOCQB = new DCO_CQBClear(this, building, faction, "sweep", true, now_ms);
		m_fDCONextCQB_ms = now_ms + 1000;
		return true;
	}

	protected bool DCO_CQBTaskAllows()
	{
		if (m_Owner.GetAgentsCount() < 2 || DCO_GetGarrisonMode() >= 0 || !DCO_OvermatchTaskAllows())
			return false;

		if (DCO_IsOvermatch())
			return true;

		if (!utilDco)
			return false;

		DCO_EGroupTask task = utilDco.GetTask();
		return task == DCO_EGroupTask.ATTACK || task == DCO_EGroupTask.FLANK;
	}

	protected IEntity DCO_ScanBuildingContacts(string faction, float now_ms)
	{
		PerceptionManager pm = GetGame().GetPerceptionManager();
		IEntity leader = m_Owner.GetLeaderEntity();
		if (!pm || !leader || !m_Perception)
			return null;

		float t = pm.GetTime();
		vector p = leader.GetOrigin();
		float bestSq = 100.0 * 100.0;
		IEntity best;
		foreach (SCR_AITargetInfo tgt : m_Perception.m_aTargets)
		{
			if (!tgt || !tgt.m_Entity || tgt.m_eCategory != EAITargetInfoCategory.IDENTIFIED || t - tgt.m_fTimestamp > 10.0)
				continue;

			IEntity building = SCR_CoverManagerComponent.DCO_GetBuildingAt(tgt.m_Entity);
			if (!building)
				continue;

			DCO_BuildingClear.MarkContact(building, faction, now_ms);
			float d = vector.DistanceSqXZ(building.GetOrigin(), p);
			if (d < bestSq)
			{
				bestSq = d;
				best = building;
			}
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

		if (!DCO_Night.IsActive() || m_Owner.IsSlave())
			return;

		SCR_AIGroup grp = SCR_AIGroup.Cast(m_Owner);
		if (!grp)
			return;

		bool stealth = DCO_IsStealthGroup();
		DCO_Night.ApplyLights(grp, stealth, m_DCOCQB);
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
			if (!s || s.m_iCountAlive <= 0 || s.GetTimeSinceLastNewInformation() > DCO_Night.CONTACT_FRESH_S)
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
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
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
		DCO_BenchmarkLoggerComponent.Event(string.Format("night_illum_request grp=%1 dist=%2", grp, Math.Round(bestDist)));
	}

	protected void DCO_UpdatePosture(float now_ms)
	{
		int friends = m_aInfoComponents.Count();
		if (friends == 0)
			return;

		m_DCOEnemyTotal.Reset();
		foreach (SCR_AIGroupTargetCluster c : m_Perception.m_aTargetClusters)
		{
			if (c.m_State && c.m_State.m_iCountAlive > 0)
				m_DCOEnemyTotal.Add(DCO_GetClusterStrength(c.m_State));
		}
		int enemies = m_DCOEnemyTotal.m_iCount;
		DCO_StrengthInfo mine = DCO_GetFriendlyStrength(m_DCOEnemyTotal.m_bHasVehicles);

		int motivated = 0;
		foreach (SCR_AIInfoComponent info : m_aInfoComponents)
		{
			if (!info || !info.GetUtilityComp())
				continue;

			DCO_AIMoraleSystem morale = info.GetUtilityComp().GetMoraleSystem();
			if (morale && (morale.GetState() == moraleState.MOTIVATED || morale.GetState() == moraleState.MANIAC))
				motivated++;
		}

		bool inCombat    = enemies > 0 || m_fThreatMeasure > 0.4;
		bool lowThreat   = m_fThreatMeasure < 0.33;
		bool winning     = mine.m_fStrength > m_DCOEnemyTotal.m_fStrength;
		bool outnumbered = mine.m_fStrength < m_DCOEnemyTotal.m_fStrength;
		bool dominant    = DCO_IsOvermatch() || (enemies > 0 && m_fThreatMeasure < 0.85
			&& DCO_Strength.IsSuperior(mine, m_DCOEnemyTotal, DCO_GetSuperiorRatio()));
		bool highMorale  = motivated * 2 > friends;

		DCO_GroupTactics next = DCO_GroupTactics.BALANCE;
		if (!inCombat)
		{
			m_fDCOFirstContact_ms = -1;
		}
		else
		{
			if (m_fDCOFirstContact_ms < 0)
				m_fDCOFirstContact_ms = now_ms;

			if (dominant)
				next = DCO_GroupTactics.AGGRESIVE;
			else if (now_ms - m_fDCOFirstContact_ms < 60000.0)
				next = DCO_GroupTactics.DEFENSIVE;
			else if ((winning || highMorale) && lowThreat)
				next = DCO_GroupTactics.AGGRESIVE;
			else if (winning)
				next = DCO_GroupTactics.BALANCE;
			else if (outnumbered && lowThreat)
				next = DCO_GroupTactics.DEFENSIVE;
			else if (outnumbered)
				next = DCO_GroupTactics.EVASIVE;
		}

		if (next == m_eDCOPosture)
			return;

		if (!dominant && m_fDCOPostureChanged_ms >= 0 && now_ms - m_fDCOPostureChanged_ms < 15000.0)
			return;

		m_eDCOPosture = next;
		m_fDCOPostureChanged_ms = now_ms;
	}

	override void EvaluateCombatMode()
	{
		if (!AICommander_ManagerComponent.GetInstance())
		{
			super.EvaluateCombatMode();
			return;
		}

		if (!utilDco)
			utilDco = DCO_GroupUtilityComponent.Cast(m_Owner.FindComponent(DCO_GroupUtilityComponent));

		if (!utilDco || !utilDco.GetMyCommander())
		{
			super.EvaluateCombatMode();
			return;
		}

	    if (m_eCombatModeExternal != EAIGroupCombatMode.RETURN_FIRE)
	    {
	        m_eCombatModeActual = m_eCombatModeExternal;
	        return;
	    }

	    DCO_GroupUtilityComponent groupUtil = DCO_GroupUtilityComponent.Cast(m_Owner.FindComponent(DCO_GroupUtilityComponent));
	    if (!groupUtil.GetMyCommander())
			return;

	    int targetCount = m_Perception.m_aTargetEntities.Count();
	    if (targetCount == 0)
	    {
	        m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;
	        return;
	    }

	    if (IsDirectlyThreatened())
	    {
	        m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
	        return;
	    }

		m_eCombatModeActual = DCO_ResolveROE(groupUtil);
	}

	protected EAIGroupCombatMode DCO_ResolveROE(DCO_GroupUtilityComponent groupUtil)
	{
		AICommander_BaseComponent cmd = groupUtil.GetMyCommander();
		DCO_ROETable roe = cmd.GetROETable();
		int state = groupUtil.GetState();

		if (state & DCO_EGroupState.RETREATING)
			return EAIGroupCombatMode.HOLD_FIRE;

		DCO_EGroupTask task = groupUtil.GetTask();
		if (groupUtil.IsMortar())
			task = DCO_EGroupTask.FIRE_MISSION;
		else if (groupUtil.IsArmor() && task == DCO_EGroupTask.NONE)
			task = DCO_EGroupTask.PATROL;

		DCO_ROEEntry entry = roe.Get(task);
		if (!entry)
			return EAIGroupCombatMode.HOLD_FIRE;

		DCO_EROEMode mode = entry.m_eMode;
		float dist = entry.m_fEngageDist;
		if (state & DCO_EGroupState.EN_ROUTE)
			dist = entry.m_fEngageDistEnRoute;
		dist *= roe.m_fDistScale;
		if (task == DCO_EGroupTask.DEFEND || task == DCO_EGroupTask.GARRISON)
			dist *= DCO_Night.EngageMultiplier();

		if (mode == DCO_EROEMode.HOLD && roe.m_fHoldEngageDist > 0)
		{
			mode = DCO_EROEMode.TIGHT;
			dist = roe.m_fHoldEngageDist;
		}

		if ((state & DCO_EGroupState.SUPPRESSED) && roe.m_bFireWhenSuppressed)
			mode = DCO_EROEMode.FREE;

		switch (mode)
		{
			case DCO_EROEMode.FREE:
				return EAIGroupCombatMode.FIRE_AT_WILL;

			case DCO_EROEMode.TIGHT:
			{
				if (IsAnyTargetRelevant(dist, !roe.m_bEngageDetected))
					return EAIGroupCombatMode.FIRE_AT_WILL;

				CMD_AICommanderObjectiveComponent obj = groupUtil.GetGroupObjective();
				if ((task == DCO_EGroupTask.DEFEND || task == DCO_EGroupTask.GARRISON) && obj
					&& IsTargetNear(obj.GetOwner().GetOrigin(), obj.GetRadius() + cmd.GetDefendObjectiveMargin()))
					return EAIGroupCombatMode.FIRE_AT_WILL;
				break;
			}
		}
		return EAIGroupCombatMode.HOLD_FIRE;
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

			bool isVeryClose = (c.m_State.m_fDistMin < CLOSE_VISIBLE_TRIGGER_DIST);
			if (isVeryClose)
				return true;
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

	bool DCO_IsBoundingTurn(AIAgent agent, out bool outBounding)
	{
		outBounding = false;
		SCR_AIGroupFireteam mine = FindFireteam(agent);
		if (!mine || mine.Type() != SCR_AIGroupFireteam)
			return true;

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
		int turn = (int)(GetGame().GetWorld().GetWorldTime() / 8000.0) % count;
		return myIndex == turn;
	}
}
