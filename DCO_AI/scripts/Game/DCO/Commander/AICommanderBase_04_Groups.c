modded class AICommander_BaseComponent
{
	int GetLogiPriority(DCO_ELogiJob kind)
	{
		switch (kind)
		{
			case DCO_ELogiJob.MEDEVAC:		return m_iLogiPrioMedevac;
			case DCO_ELogiJob.EXTRACT:		return m_iLogiPrioExtract;
			case DCO_ELogiJob.REINFORCE:	return m_iLogiPrioReinforce;
			case DCO_ELogiJob.DEPLOY:		return m_iLogiPrioDeploy;
			case DCO_ELogiJob.REDEPLOY:		return m_iLogiPrioRedeploy;
		}
		return m_iLogiPrioResupply;
	}

	vector GetHubFor(vector pos, out string source)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		float best = float.MAX;
		vector hub;
		foreach (DCO_LogisticsHubComponent h : DCO_LogisticsHubComponent.GetHubs())
		{
			if (!h)
				continue;
			vector hp = h.GetOwner().GetOrigin();
			FactionKey fk = h.GetFactionKey();
			if (fk != string.Empty && fk != m_sFactionKey)
				continue;
			if (fk == string.Empty && mgr && NearestCommander(mgr, hp) != this)
				continue;

			float d = vector.DistanceSq(pos, hp);
			if (d < best)
			{
				best = d;
				hub = hp;
			}
		}
		if (best < float.MAX)
		{
			source = "GM";
			return hub;
		}

		if (mgr)
		{
			CMD_AICommanderObjectiveComponent pick, deepest;
			float pickFront = float.MAX;
			float deepestFront = -1;
			foreach (CMD_AICommanderObjectiveComponent own : mgr.m_aObjective)
			{
				if (!own || !own.IsCapturedBy(m_sFactionKey, m_sCommanderUID) || IsObjectiveContested(own))
					continue;

				float front = float.MAX;
				foreach (CMD_AICommanderObjectiveComponent other : mgr.m_aObjective)
				{
					if (other && !other.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
						front = Math.Min(front, vector.Distance(own.GetOwner().GetOrigin(), other.GetOwner().GetOrigin()));
				}

				if (front >= m_fLogiHubRearDist && front < pickFront)
				{
					pickFront = front;
					pick = own;
				}
				if (front > deepestFront)
				{
					deepestFront = front;
					deepest = own;
				}
			}
			if (!pick)
				pick = deepest;
			if (pick)
			{
				source = "auto " + pick.GetLogName();
				return pick.GetOwner().GetOrigin();
			}
		}

		source = "commander";
		return GetOwner().GetOrigin();
	}

	protected static AICommander_BaseComponent NearestCommander(AICommander_ManagerComponent mgr, vector pos)
	{
		AICommander_BaseComponent best;
		float bestD = float.MAX;
		foreach (AICommander_BaseComponent c : mgr.m_aCommander)
		{
			if (!c)
				continue;
			float d = vector.DistanceSq(pos, c.GetOwner().GetOrigin());
			if (d < bestD)
			{
				bestD = d;
				best = c;
			}
		}
		return best;
	}

	void DetachFromObjective(DCO_GroupUtilityComponent grp)
	{
		CMD_AICommanderObjectiveComponent obj = grp.GetGroupObjective();
		DCO_EGroupTask task = grp.GetTask();
		if (obj)
		{
			if (task == DCO_EGroupTask.DEFEND)
			{
				array<ref DCO_SectorGarrison> sectors = obj.GetSectorGarrison(m_sFactionKey);
				if (sectors)
				{
					foreach (DCO_SectorGarrison sec : sectors)
					{
						if (sec && sec.m_Group == grp)
							sec.m_Group = null;
					}
				}
			}
			else if (TaskCountsSlot(task) && obj.GetCurrentAssignedGroupCount(m_sFactionKey) > 0)
			{
				obj.SetObjectiveGroup(m_sFactionKey, -1);
			}
		}
		grp.SetTask(DCO_EGroupTask.NONE, "AICommanderBase:1708");
		grp.SetGroupObjective(null);
	}

	float GetMoraleAttackThreshold()
	{
		return m_fMoraleAttackThreshold + Math.Lerp(-m_fMoraleThresholdSpread, m_fMoraleThresholdSpread, m_fResilience);
	}

	bool IsMoraleFitForAttack(DCO_GroupUtilityComponent grp)
	{
		return grp.GetGroupMorale() <= GetMoraleAttackThreshold();
	}

	int GetReinforcementStock()
	{
		if (!m_bStockInit)
		{
			m_bStockInit = true;
			m_iStockLeft = m_iReinforcementStock;
			DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetForFaction(m_sFactionKey);
			if (cfg && cfg.GetCommanderStockDefault() >= 0)
				m_iStockLeft = cfg.GetCommanderStockDefault();
		}
		return m_iStockLeft;
	}

	void SetReinforcementStock(int value)
	{
		m_bStockInit = true;
		m_iStockLeft = Math.Max(value, -1);
		m_iStockStart = m_iStockLeft;
	}

	bool ConsumeStock(int soldiers)
	{
		int stock = GetReinforcementStock();
		if (m_iStockStart == -2)
			m_iStockStart = stock;
		if (stock < 0)
			return true;
		if (stock < soldiers)
			return false;
		m_iStockLeft = stock - soldiers;
		return true;
	}

	void RefundStock(int soldiers)
	{
		int stock = GetReinforcementStock();
		if (stock >= 0)
			m_iStockLeft = stock + soldiers;
	}

	int GetStockStart()
	{
		if (m_iStockStart == -2)
			return GetReinforcementStock();
		return m_iStockStart;
	}

	int GetPointsStock()
	{
		return m_iPointsStock;
	}

	void SetPointsStock(int value)
	{
		m_iPointsStock = Math.Max(value, -1);
		m_iPointsStart = m_iPointsStock;
	}

	int GetPointsStart()
	{
		if (m_iPointsStart == -2)
			return m_iPointsStock;
		return m_iPointsStart;
	}

	bool ConsumePoints(int points)
	{
		if (m_iPointsStart == -2)
			m_iPointsStart = m_iPointsStock;
		if (m_iPointsStock < 0)
			return true;
		if (m_iPointsStock < points)
			return false;
		m_iPointsStock -= points;
		return true;
	}

	void RefundPoints(int points)
	{
		if (m_iPointsStock >= 0)
			m_iPointsStock += points;
	}

	int GetAllGroups(notnull out array<DCO_GroupUtilityComponent> outGroups)
	{
		outGroups.Clear();
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (g)
				outGroups.Insert(g);
		}

		if (artySupport)
		{
			array<DCO_GroupUtilityComponent> arty = {};
			artySupport.GetRegisteredUnits(arty);
			foreach (DCO_GroupUtilityComponent a : arty)
			{
				if (!outGroups.Contains(a))
					outGroups.Insert(a);
			}
		}
		return outGroups.Count();
	}

	int GetTotalManpower()
	{
		int total = 0;
		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp)
				continue;
			total += grp.GetUnitCount();
		}
		if (m_PlayerTasking)
			total += m_PlayerTasking.GetCountedManpower();
		return total;
	}

	int GetReserveManpower()
	{
		int reserve = 0;
		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp)
				continue;

			if (grp.GetTask() == DCO_EGroupTask.NONE || grp.GetTask() == DCO_EGroupTask.PATROL)
				reserve += grp.GetUnitCount();
		}
		return reserve;
	}

	float GetReserveFloor()
	{
		if (m_bAllIn)
			return 0;
		float floor = 1.0 * m_iManpowerTotalCache * m_fReservePercent / 100.0;
		if (GetDoctrine() && m_DoctrineRt.GetPlanner().ReserveRequired(this))
			floor = Math.Max(floor, m_iManpowerTotalCache * GetDoctrine().m_fReserve);
		return floor;
	}

	bool DCO_PlanGuard(DCO_GroupUtilityComponent g, string path, CMD_AICommanderObjectiveComponent forObj = null)
	{
		if (!g || !m_Tactics || !DCO_TacticsCompat.OwnsGroup(m_Tactics, g))
			return false;
		CMD_AICommanderObjectiveComponent planObj = g.GetGroupObjective();
		if (forObj && planObj == forObj)
			return false;
		DCO_Perf.Count("plan_guard:" + path);
		if (DCO_Log.On(DCO_ELogChannel.CMD, DCO_ELogLevel.DEBUG))
		{
			string objName = "-";
			if (planObj)
				objName = planObj.GetLogName();
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.DEBUG, DCO_Log.IdCmd(this), "plan_guard_skip grp=" + g.GetOwner().GetName() + " path=" + path + " plan_obj=" + objName + " task=" + typename.EnumToString(DCO_EGroupTask, g.GetTask()));
		}
		return true;
	}

	bool DCO_MedicTeamBlocked(DCO_GroupUtilityComponent g, int task, string path)
	{
		if (!g || !g.IsMedicTeam())
			return false;
		if (task != DCO_EGroupTask.RECON && task != DCO_EGroupTask.ATTACK && task != DCO_EGroupTask.FLANK && task != DCO_EGroupTask.SUPPORT_BY_FIRE
			&& task != DCO_EGroupTask.PATROL && task != DCO_EGroupTask.DEFEND && task != DCO_EGroupTask.GARRISON && task != DCO_EGroupTask.REINFORCE)
			return false;
		DCO_Perf.Count("idle_pick:skip_medic");
		float nowS = DCO_Util.NowS();
		if (m_mDCOMedicLogAt.Get(g) > nowS)
			return true;
		m_mDCOMedicLogAt.Set(g, nowS + 60.0);
		if (DCO_Log.On(DCO_ELogChannel.CMD, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.DEBUG, DCO_Log.IdCmd(this), "idle_pick_skip grp=" + g.GetOwner().GetName() + " reason=medic_team task=" + typename.EnumToString(DCO_EGroupTask, task) + " path=" + path);
		return true;
	}

	bool CanCommitGroup(DCO_GroupUtilityComponent grp)
	{
		if (DCO_PlanGuard(grp, "commit"))
			return false;

		if (DCO_MedicTeamBlocked(grp, DCO_EGroupTask.ATTACK, "commit"))
			return false;

		if (m_bAllIn || m_fReservePercent <= 0)
			return grp != null;

		if (!grp)
			return false;

		float reserveAfterCommit = GetReserveManpower() - grp.GetUnitCount();
		return reserveAfterCommit >= GetReserveFloor();
	}

	bool RegisterGroup(DCO_GroupUtilityComponent grp)
	{
		if (!grp)
			return false;

		if (grp.IsMortar())
		{
			if (!artySupport)
				return false;
			artySupport.RegisterArtilleryGroup(grp);
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("group_owned_skip cmd=%1 grp=%2 reason=mortar_to_artillery -- mortar crew is held by artillery support, not in the owned group list", m_sCommanderUID, grp.GetOwner().GetName()));
			return true;
		}

		if (!m_aOwnedGroup.Contains(grp))
			m_aOwnedGroup.Insert(grp);

		return true;
	}

	bool RegisterVehicle(IEntity grp)
	{
		if (!m_aVehicle.Contains(grp))
			m_aVehicle.Insert(grp);

		return true;
	}

	bool IsGroupHere(DCO_GroupUtilityComponent grp)
	{
		return m_aOwnedGroup.Contains(grp);
	}

	bool UnregisterGroup(DCO_GroupUtilityComponent grp)
	{
		if (m_aOwnedGroup.Contains(grp))
			m_aOwnedGroup.RemoveItem(grp);

		if (m_mLastPreemptTime.Contains(grp))
			m_mLastPreemptTime.Remove(grp);

		m_Patrol.ForgetGroup(grp);

		return true;
	}

	bool AssignGroup(DCO_GroupUtilityComponent grp)
	{
		if (!Replication.IsServer() || !grp)
			return false;

		grp.EnsureDormantInit();

		if (grp.GetFactionKey().IsEmpty() || grp.GetFactionKey() != m_sFactionKey)
			return false;

		AICommander_BaseComponent current = grp.GetMyCommander();
		if (current == this)
			return true;

		if (current)
			current.ReleaseGroup(grp);

		grp.ActivateForCommander(this);
		RegisterGroup(grp);

		DCO_TransportTeamComponent selfTeam = DCO_TransportTeamComponent.Cast(grp.GetOwner().FindComponent(DCO_TransportTeamComponent));
		if (selfTeam && grp.IsDedicatedTransport())
			RegisterTransportTeam(selfTeam);

		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("cmd=%1 AssignGroup: %2", m_sCommanderUID, grp.GetOwner().GetName()));
		return true;
	}

	void ReleaseGroup(DCO_GroupUtilityComponent grp)
	{
		if (!Replication.IsServer() || !grp)
			return;

		if (!GetGame() || !GetGame().GetWorld())
			return;

		float worldTime = DCO_Util.NowS();
		DCO_EGroupTask task = grp.GetTask();
		CMD_AICommanderObjectiveComponent obj = grp.GetGroupObjective();

		if (obj)
		{
			if (task == DCO_EGroupTask.DEFEND)
			{
				array<ref DCO_SectorGarrison> sectors = obj.GetSectorGarrison(m_sFactionKey);
				if (sectors)
				{
					foreach (DCO_SectorGarrison sec : sectors)
					{
						if (sec && sec.m_Group == grp)
							sec.m_Group = null;
					}
				}
			}
			else if (TaskCountsSlot(task))
			{
				if (obj.GetCurrentAssignedGroupCount(m_sFactionKey) > 0)
					obj.SetObjectiveGroup(m_sFactionKey, -1);
			}
		}

		UnregisterGroup(grp);

		if (artySupport)
			artySupport.UnregisterArtilleryGroup(grp);

		m_Patrol.ForgetGroup(grp);
		m_Frontline.ForgetGroup(grp);

				foreach (DCO_TransportTeamComponent team : m_aTransportTeams)
		{
			if (team && team.HasPassenger(grp))
				team.OnPassengerReleased(grp, worldTime);
		}
		m_Logistics.OnGroupReleased(grp);

		DCO_TransportTeamComponent selfTeam = DCO_TransportTeamComponent.Cast(grp.GetOwner().FindComponent(DCO_TransportTeamComponent));
		if (selfTeam)
		{
			selfTeam.ReleaseFromCommander(worldTime);
			m_aTransportTeams.RemoveItem(selfTeam);
		}

		foreach (IEntity veh : m_aVehicle)
		{
			if (!veh)
				continue;

			DCO_TransportMissionComponent mission = DCO_TransportMissionComponent.Cast(veh.FindComponent(DCO_TransportMissionComponent));
			if (mission && mission.GetPassengerGroup() == grp && mission.IsActiveVehicle())
				mission.AbortMission(worldTime);
		}

		grp.ReleaseFromCommander();

		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("cmd=%1 ReleaseGroup: %2", m_sCommanderUID, grp.GetOwner().GetName()));
	}

	bool AssignVehicle(IEntity veh)
	{
		if (!Replication.IsServer() || !veh)
			return false;

		DCO_TransportMissionComponent mission = DCO_TransportMissionComponent.Cast(veh.FindComponent(DCO_TransportMissionComponent));
		if (!mission)
			return false;

		FactionKey vehFk = DCO_Util.FactionKeyOf(veh, true);

		if (vehFk != m_sFactionKey)
			return false;

		AICommander_BaseComponent current = mission.GetCommanderOwner();
		if (current == this)
			return true;

		if (current)
			current.ReleaseVehicle(veh);

		RegisterVehicle(veh);
		mission.ActivateForCommander(this);

		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("cmd=%1 AssignVehicle: %2", m_sCommanderUID, veh.GetName()));
		return true;
	}

	void ReleaseVehicle(IEntity veh)
	{
		if (!Replication.IsServer() || !veh)
			return;

		if (!GetGame() || !GetGame().GetWorld())
			return;

		if (m_aVehicle.Contains(veh))
			m_aVehicle.RemoveItem(veh);

		DCO_TransportMissionComponent mission = DCO_TransportMissionComponent.Cast(veh.FindComponent(DCO_TransportMissionComponent));
		if (mission)
			mission.ReleaseFromCommander(DCO_Util.NowS());

		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("cmd=%1 ReleaseVehicle: %2", m_sCommanderUID, veh.GetName()));
	}

	protected void ReleaseEverything()
	{
		array<DCO_GroupUtilityComponent> groups = {};
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (g)
				groups.Insert(g);
		}

		if (artySupport)
		{
			array<DCO_GroupUtilityComponent> artyUnits = {};
			artySupport.GetRegisteredUnits(artyUnits);
			foreach (DCO_GroupUtilityComponent a : artyUnits)
			{
				if (!groups.Contains(a))
					groups.Insert(a);
			}
		}

		foreach (DCO_GroupUtilityComponent grp : groups)
		{
			if (grp && grp.GetMyCommander() == this)
				ReleaseGroup(grp);
		}

		float worldTime = DCO_Util.NowS();
		foreach (DCO_TransportTeamComponent team : m_aTransportTeams)
		{
			if (team)
				team.ReleaseFromCommander(worldTime);
		}
		m_aTransportTeams.Clear();

		array<IEntity> vehicles = {};
		foreach (IEntity v : m_aVehicle)
		{
			if (v)
				vehicles.Insert(v);
		}

		foreach (IEntity veh : vehicles)
			ReleaseVehicle(veh);
	}

	bool RenameCommanderFromGM(string newUID)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr || !mgr.RenameCommander(this, newUID))
			return false;

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (grp)
				grp.SetDedicatedCommanderUID(newUID);
		}

		if (artySupport)
		{
			array<DCO_GroupUtilityComponent> artyUnits = {};
			artySupport.GetRegisteredUnits(artyUnits);
			foreach (DCO_GroupUtilityComponent arty : artyUnits)
			{
				if (arty)
					arty.SetDedicatedCommanderUID(newUID);
			}
		}

		return true;
	}

	void PromoteToArtillery(DCO_GroupUtilityComponent grp)
	{
		if (!grp || !artySupport)
			return;

		m_aOwnedGroup.RemoveItem(grp);
		grp.CompleteAllWaypoints();
		grp.SetTask(DCO_EGroupTask.NONE, "AICommanderBase:2529");
		artySupport.RegisterArtilleryGroup(grp);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("arty_unit_registered grp=%1 cmd=%2", grp.GetOwner(), m_sCommanderUID));
		DCO_Perf.Count("arty_register");
		DCO_Log.Write(DCO_ELogChannel.ARTY, DCO_ELogLevel.INFO, "-", string.Format("arty_register grp=%1 cmd=%2 tubes=%3 range=%4 units=%5", grp.GetOwner().GetName(), m_sCommanderUID, DCO_MortarTubes(grp), artySupport.DCO_RangeText(), artySupport.DCO_UnitCount()));
	}

	bool DCO_IsArtyRegistered(DCO_GroupUtilityComponent grp)
	{
		return artySupport && artySupport.DCO_HasUnit(grp);
	}

	protected int DCO_MortarTubes(DCO_GroupUtilityComponent grp)
	{
		SCR_AIGroupUtilityComponent gu = grp.GetGroupUtilityComponent();
		if (!gu || !gu.m_VehicleMgr)
			return 0;
		array<ref SCR_AIGroupVehicle> vehicles = {};
		gu.m_VehicleMgr.GetAllVehicles(vehicles);
		int n = 0;
		foreach (SCR_AIGroupVehicle v : vehicles)
		{
			if (v && v.GetVehicleUsageComponent() && v.GetVehicleUsageComponent().GetVehicleType() == EAIVehicleType.STATIC_ARTILLERY)
				n++;
		}
		return n;
	}

	void DemoteFromArtillery(DCO_GroupUtilityComponent grp, string reason)
	{
		if (!grp)
			return;

		if (artySupport)
			artySupport.UnregisterArtilleryGroup(grp);
		grp.CompleteAllWaypoints();
		grp.SetTask(DCO_EGroupTask.NONE, "AICommanderBase:2565");
		RegisterGroup(grp);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("arty_unit_demoted grp=%1 cmd=%2 reason=%3", grp.GetOwner(), m_sCommanderUID, reason));
	}

	void SetCommanderFactionKey(FactionKey fk)
	{
		if (!Replication.IsServer() || fk == m_sFactionKey)
			return;

		ReleaseEverything();
		m_sFactionKey = fk;

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
			mgr.OnCommanderFactionChanged(this);
	}

	void SetCommanderUID(string uid)
	{
		if (!Replication.IsServer())
			return;

		m_sCommanderUID = uid;
		Replication.BumpMe();
	}

	void SwitchToDefensive(float worldTime)
	{
		m_eCommanderMode            = CMD_ECommanderMode.DEFENSIVE;
		m_fDefensiveTriggerCooldown = worldTime;
	}

	void SwitchToOffensive()
	{
		m_eCommanderMode = CMD_ECommanderMode.OFFENSIVE;
	}

	void SwitchToBalanced()
	{
		m_eCommanderMode = CMD_ECommanderMode.BALANCED;
	}

	void ForceDefensiveMode(float worldTime)   { SwitchToDefensive(worldTime); }

	void ForceOffensiveMode()                  { SwitchToOffensive(); }

	protected bool IsGroupBusy(DCO_GroupUtilityComponent grp)
	{
		if (grp.IsInTransport() || grp.GetSupportPlayerGroup() >= 0 || grp.DCO_IsHeld())
			return true;

		DCO_EGroupTask t = grp.GetTask();
		if (grp.GetGroupObjective() && t != DCO_EGroupTask.NONE && t != DCO_EGroupTask.PATROL)
			return true;

		return m_Frontline.IsReconGroup(grp);
	}

	int DCO_GroupsOnObj(CMD_AICommanderObjectiveComponent obj)
	{
		int n;
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (!g || g.GetGroupObjective() != obj)
				continue;
			DCO_EGroupTask t = g.GetTask();
			if (t == DCO_EGroupTask.ATTACK || t == DCO_EGroupTask.FLANK || t == DCO_EGroupTask.SUPPORT_BY_FIRE)
				n++;
		}
		return n;
	}

	static int DefaultCapabilitiesForTask(DCO_EGroupTask task)
	{
		switch (task)
		{
			case DCO_EGroupTask.RECON:           return DCO_EAIGroupCapabilities.COVERT | DCO_EAIGroupCapabilities.LONG_RANGE;
			case DCO_EGroupTask.ATTACK:          return DCO_EAIGroupCapabilities.CLOSE_RANGE | DCO_EAIGroupCapabilities.LEADING;
			case DCO_EGroupTask.FLANK:           return DCO_EAIGroupCapabilities.CLOSE_RANGE | DCO_EAIGroupCapabilities.COVERT;
			case DCO_EGroupTask.SUPPORT_BY_FIRE: return DCO_EAIGroupCapabilities.SUPPRESSING | DCO_EAIGroupCapabilities.LONG_RANGE;
			case DCO_EGroupTask.REINFORCE:       return DCO_EAIGroupCapabilities.SUPPRESSING | DCO_EAIGroupCapabilities.MEDIUM_RANGE;
			case DCO_EGroupTask.DEFEND:          return DCO_EAIGroupCapabilities.SUPPRESSING | DCO_EAIGroupCapabilities.MEDIUM_RANGE;
		}
		return 0;
	}

	static int CountCapabilityMatches(DCO_GroupUtilityComponent grp, int wanted)
	{
		if (wanted == 0)
			return 0;

		DCO_GroupConfigComponent cfg = DCO_GroupConfigComponent.Cast(grp.GetOwner().FindComponent(DCO_GroupConfigComponent));
		if (!cfg)
			return 0;

		int matched = cfg.GetCapabilities() & wanted;
		int count = 0;
		while (matched)
		{
			count += matched & 1;
			matched = matched >> 1;
		}
		return count;
	}

	string DCO_ReconExclude(DCO_GroupUtilityComponent grp)
	{
		if (!grp || !grp.GetOwner())
			return "invalid";
		if (grp.IsDedicatedTransport())
			return "transport";
		if (grp.IsBreakingContact())
			return "breaking_contact";
		if (m_Logistics && m_Logistics.HasJobFor(grp))
			return "logistics";
		if (grp.IsMortar())
			return "mortar";
		if (grp.IsMedicTeam())
			return "medic";
		IEntity veh = grp.GetGroupVehicle();
		if (veh && !DCO_VehicleCombat.HasTurret(veh))
			return "unarmed_vehicle";
		return "";
	}

	string DCO_ReconKind(DCO_GroupUtilityComponent grp)
	{
		if (!grp)
			return "none";
		if (CountCapabilityMatches(grp, DCO_EAIGroupCapabilities.COVERT | DCO_EAIGroupCapabilities.LONG_RANGE) > 0)
			return "recon_cap";
		IEntity veh = grp.GetGroupVehicle();
		if (veh)
		{
			if (DCO_VehicleCombat.HasTurret(veh) && !grp.IsArmor())
				return "armed_vehicle";
			return "vehicle";
		}
		int n = grp.GetUnitCount();
		if (n >= m_iLightInfMin && n <= m_iLightInfMax)
			return "light_inf";
		return "squad";
	}

	protected void DCO_ReconSkip(DCO_GroupUtilityComponent grp, string reason)
	{
		DCO_Perf.Count("recon_skip:" + reason);
		float now = DCO_Util.NowS();
		float at;
		if (m_mDCOReconSkipAt.Find(grp, at) && now - at < 60.0)
			return;
		m_mDCOReconSkipAt.Set(grp, now);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("recon_skip cmd=%1 grp=%2 reason=%3", m_sCommanderUID, grp.GetOwner().GetName(), reason));
	}

	int DCO_ThreatReconCount()
	{
		int n = 0;
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (g && g.GetTask() == DCO_EGroupTask.RECON && !g.GetGroupObjective())
				n++;
		}
		return n;
	}

	bool DCO_NearEnemyObjective(vector pos, float radius)
	{
		foreach (CMD_AICommanderObjectiveComponent o : GetObjectiveList())
		{
			if (!o || !o.GetOwner())
				continue;
			FactionKey owner = o.GetOwnerFaction();
			if (owner.IsEmpty() || owner == m_sFactionKey)
				continue;
			if (vector.DistanceXZ(o.GetOwner().GetOrigin(), pos) <= radius + o.GetRadius())
				return true;
		}
		return false;
	}

	void RegisterTransportTeam(DCO_TransportTeamComponent team)
	{
		if (!team || m_aTransportTeams.Contains(team))
			return;

				team.SetRallyPoint(GetOwner().GetOrigin());
		team.SetCommander(this);
		m_aTransportTeams.Insert(team);
	}

	void UnregisterTransportTeam(DCO_TransportTeamComponent team)
	{
		m_aTransportTeams.RemoveItem(team);
	}
}
