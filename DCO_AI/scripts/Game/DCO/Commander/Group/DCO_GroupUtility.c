[ComponentEditorProps(category: "GameScripted/Group")]
class DCO_GroupUtilityComponentClass : ScriptComponentClass
{
}

class DCO_GroupUtilityComponent : ScriptComponent
{
	[Attribute("0", UIWidgets.CheckBox, desc: "Gambar overlay grup ini (role, tugas, status order) sebagai teks 3D. Cuma kelihatan waktu Game Master kebuka.", category: "Group Debug")]
	protected bool m_bDebugMode;

	[Attribute("0.5", UIWidgets.EditBox, "Interval (detik) gambar ulang overlay grup.", category: "Group Debug")]
	protected float m_fDebugRefreshInterval;

	protected ref array<ref Shape> m_aDebugShapes = new array<ref Shape>();
	protected ref array<ref DebugTextWorldSpace> m_aDebugTexts = new array<ref DebugTextWorldSpace>();
	protected float m_fDebugTimer = 0.0;

	[Attribute("0", UIWidgets.CheckBox, "Dedicated transport team: grup ini cuma ngerjain job logistik (butuh DCO_TransportTeamComponent).", category: "Commander")]
	protected bool m_bIsDedicatedTransport;

	[Attribute("1", UIWidgets.EditBox, "Can it be used By Commander?", category: "Commander")]
	protected bool m_bIsProcessedByCommander;

	[Attribute("1", UIWidgets.EditBox, "Can it Have By Commander?", category: "Commander")]
	protected bool m_bCanHaveCommander;

	[Attribute("1", UIWidgets.EditBox, "Can it call Artillery?", category: "Support")]
	protected bool m_bCanCallArtillery;

	[Attribute("1", UIWidgets.EditBox, "Can it call Reinforcement?", category: "Support")]
	protected bool m_bCanCallReinforcement;

	[Attribute("1", UIWidgets.EditBox, "Can it role be override by Commander?", category: "Group Role")]
	protected bool m_bCanCommanderOverrideRole;

	[Attribute("1", UIWidgets.CheckBox, "Commander boleh ngirim squad ini sebagai bala bantuan / respons ancaman.", category: "Commander")]
	protected bool m_bAllowReinforce;

	[Attribute("1", UIWidgets.CheckBox, "Commander boleh ngangkut squad ini pakai kendaraan / tim transport.", category: "Commander")]
	protected bool m_bAllowTransport;

	[Attribute("1", UIWidgets.CheckBox, "Squad ini boleh dipakai patroli cadangan.", category: "Commander")]
	protected bool m_bAllowPatrol;

	[Attribute("1", UIWidgets.CheckBox, "Squad ini ngirim laporan kontak ke Commander.", category: "Commander")]
	protected bool m_bAllowContactReport;

	[Attribute("1", UIWidgets.CheckBox, "Squad ini boleh dikirim bantu permintaan pemain (suppress / support / flank).", category: "Commander")]
	protected bool m_bAllowPlayerSupport;

	bool CanReinforce()					{ return m_bAllowReinforce; }
	bool CanBeTransported()				{ return m_bAllowTransport; }
	bool CanPatrol()					{ return m_bAllowPatrol; }
	bool CanReportContacts()			{ return m_bAllowContactReport; }
	bool CanSupportPlayers()			{ return m_bAllowPlayerSupport; }
	void SetCanReinforce(bool b)		{ m_bAllowReinforce = b; }
	void SetCanBeTransported(bool b)	{ m_bAllowTransport = b; }
	void SetCanPatrol(bool b)			{ m_bAllowPatrol = b; }
	void SetCanReportContacts(bool b)	{ m_bAllowContactReport = b; }
	void SetCanSupportPlayers(bool b)	{ m_bAllowPlayerSupport = b; }
	void SetCanCallArty(bool b)			{ m_bCanCallArtillery = b; }
	void SetCanCallReinforcement(bool b)	{ m_bCanCallReinforcement = b; }
	void SetCanCommanderOverrideRole(bool b)	{ m_bCanCommanderOverrideRole = b; }

	protected SCR_AIGroup m_Group;
	protected SCR_AIGroupUtilityComponent m_UtilityComp;
	ref SCR_AIGroupPerception perc;
	protected AIFormationComponent m_FormationComponent;
	protected AICommander_BaseComponent myCommander;
	protected CMD_ThreatResponseComponent threatComp;
	protected DCO_TransportTeamComponent DedicatedTransport;
	protected DCO_GroupContactReporterComponent contactReportComponent;

	protected EAIGroupCombatMode m_eCombatModeBeforeCommander = EAIGroupCombatMode.FIRE_AT_WILL;

	[Attribute("0", UIWidgets.SearchComboBox, "Preset kemampuan grup (AUTO = dari isi grup)", enums: ParamEnumArray.FromEnum(DCO_EGroupPreset))]
	protected DCO_EGroupPreset m_eGroupRoleExternal;

	[Attribute("", UIWidgets.Auto, "Blacklisted Commander to not process this Group", category: "Commander")]
	protected ref array<string> m_sBlacklistedCo;

	[Attribute("", UIWidgets.Auto, "Assign This Unit To Commander", category: "Commander")]
	protected string m_sDedicatedCo;

	[Attribute("0", UIWidgets.CheckBox, "Auto-assign ke commander waktu scenario mulai (tanpa GM). Dedicated Co kosong = commander random yang faction-nya sama.", category: "Commander")]
	protected bool m_bAutoAssign;

	protected int m_iAutoAssignTries;
	protected const int AUTO_ASSIGN_MAX_TRIES = 12;
	protected const int AUTO_ASSIGN_RETRY_MS = 5000;

	[Attribute("", UIWidgets.Auto, "Usable Mortar For This Group In The Inital", category: "Artillery Group")]
	protected ref array<string> m_sUsableVehicle;

	protected ref DCO_GroupTask m_Task = new DCO_GroupTask();
	protected bool m_bInTransport;
	protected bool m_bRetreating;
	protected int m_iPeakStrength;

	protected DCO_EGroupCapability m_eCapability = DCO_EGroupCapability.INFANTRY;
	protected int m_iUnitRoles;
	protected bool m_bHasRadio;
	protected float m_fCapabilityCheck_ms = -1;
	protected int m_iStateCache;
	protected float m_fStateCheck_ms = -1;


	protected FactionKey fk;

	protected bool IsPlayerGroup = false;

	protected vector m_vOrderTarget    = vector.Zero;
	protected float  m_fOrderStartTime = 0.0;
	protected float  m_fOrderTimeout   = 0.0;
	protected bool   m_bOrderActive    = false;

	static float AVG_MOVE_SPEED_MPS = 2.5;
	static float ORDER_BASE_BUFFER  = 10.0;
	static float ARRIVAL_THRESHOLD  = 3.0;

	static float SQUAD_SPREAD_THRESHOLD = 30.0;

	protected vector m_vLastCheckPos = vector.Zero;
	protected float m_fLastMoveTime = 0;
	protected const float STUCK_DIST_THRESHOLD = 2.5;
	protected const float STUCK_TIME_THRESHOLD = 15.0;

	protected IEntity m_OwnedVehicle;

	IEntity GetOwnedVehicle()
	{
		return m_OwnedVehicle;
	}

	void SetOwnedVehicle(IEntity veh)
	{
		m_OwnedVehicle = veh;
	}

	bool HasOwnedVehicle()
	{
		return m_OwnedVehicle != null;
	}

	protected bool m_bCruiseLimited;

	IEntity GetGroupVehicle()
	{
		if (m_OwnedVehicle)
		{
			DamageManagerComponent dmg = DamageManagerComponent.Cast(m_OwnedVehicle.FindComponent(DamageManagerComponent));
			if (!dmg || dmg.GetState() != EDamageState.DESTROYED)
				return m_OwnedVehicle;
		}

		if (!m_Group)
			return null;
		return DCO_VehicleCombat.GetVehicle(m_Group.GetLeaderEntity());
	}

	bool IsMountedIn(IEntity veh)
	{
		return m_Group && veh && DCO_VehicleCombat.GetVehicle(m_Group.GetLeaderEntity()) == veh;
	}

	bool CanAllFitIn(IEntity veh)
	{
		if (!m_Group || !veh)
			return false;

		int outside = 0;
		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			ChimeraCharacter c = ChimeraCharacter.Cast(a.GetControlledEntity());
			if (!c || DCO_VehicleCombat.GetVehicle(c) == veh)
				continue;

			CharacterControllerComponent ctrl = c.GetCharacterController();
			if (ctrl && ctrl.GetLifeState() != ECharacterLifeState.DEAD)
				outside++;
		}

		SCR_BaseCompartmentManagerComponent mgr = SCR_BaseCompartmentManagerComponent.Cast(veh.FindComponent(SCR_BaseCompartmentManagerComponent));
		return outside <= CMD_VehicleFinder.CountFreeSeats(mgr);
	}

	void SetVehicleCruiseSpeed(float speedKmH)
	{
		IEntity veh = GetGroupVehicle();
		if (!veh)
			return;

		AICarMovementComponent car = AICarMovementComponent.Cast(veh.FindComponent(AICarMovementComponent));
		if (!car)
			return;

		if (speedKmH > 0)
			car.SetCruiseSpeed(speedKmH);
		else
			car.ResetCruiseSpeed();
		m_bCruiseLimited = speedKmH > 0;
	}

	void SetDedicatedCommanderUID(string uid)
	{
		m_sDedicatedCo = uid;
	}

	void DCO_AssignFromSpawner(string commanderUID)
	{
		m_sDedicatedCo = commanderUID;
		m_bAutoAssign = true;
		m_iAutoAssignTries = 0;
		if (fk.IsEmpty())
			EnsureDormantInit();
		else
			TryAutoAssign();
	}

	void DCO_AddSpawnVehicle(IEntity vehicle)
	{
		SCR_AIVehicleUsageComponent usage = FindVehicleUsage(vehicle, 3);
		if (usage && m_UtilityComp)
			m_UtilityComp.AddUsableVehicle(usage);
	}

	void EnsureDormantInit()
	{
		if (!fk.IsEmpty())
			return;

		if (!m_bIsProcessedByCommander && !m_bCanHaveCommander)
			return;

		if (!contactReportComponent)
			contactReportComponent = DCO_GroupContactReporterComponent.Cast(GetOwner().FindComponent(DCO_GroupContactReporterComponent));

		GetGame().GetCallqueue().Remove(delayedInit);
		delayedInit(GetOwner());
	}

	bool IsCommanderEligible()
	{
		return m_bIsProcessedByCommander || m_bCanHaveCommander;
	}

	bool CanCommanderOverrideRole()
	{
		return m_bCanCommanderOverrideRole;
	}

	bool CanCallReinforcement()
	{
		return m_bCanCallReinforcement;
	}

	bool CanCallArty()
	{
		return m_bCanCallArtillery;
	}

	void CompleteAllWaypoints()
	{
		m_Group.CompleteAllWaypoints();
	}

	bool IsCommanderBlacklisted(string cuid)
	{
		return m_sBlacklistedCo.Contains(cuid);
	}

	int GetGroupID()
	{
		return m_Group.GetID();
	}

	AICommander_BaseComponent GetMyCommander()
	{
		return myCommander;
	}

	string DedicatedCommander()
	{
		return m_sDedicatedCo;
	}

	bool IsOrderActive()
	{
	    return m_bOrderActive;
	}

	bool IsDedicatedTransport()
	{
		return m_bIsDedicatedTransport;
	}

	void DCO_SetDedicatedFlag(bool value)
	{
		m_bIsDedicatedTransport = value;
	}

	CMD_ThreatResponseComponent GetThreatResponseComponent()
	{
		return threatComp;
	}

	SCR_AIGroupUtilityComponent GetGroupUtilityComponent()
	{
		return m_UtilityComp;
	}

	int GetUnitCount()
	{
		if (!m_Group)
			return 0;
		return m_Group.GetAgentsCount();
	}

	void MoveTo(SCR_AIWaypoint wp, float worldTime)
	{
		if (!m_Group || !wp)
			return;

		m_Group.AddWaypoint(wp);
		SetPhase(DCO_ETaskPhase.MOVING);
		m_Task.m_vTarget = wp.GetOrigin();
		BeginOrderTracking(wp.GetOrigin(), worldTime);
	}

	void CheckGroupIsHaveOrder()
	{
		if (IsMoving() && IsGroupHaveWaypoint())
			return;

		SetPhase(DCO_ETaskPhase.HOLDING);
		if (m_Task.m_eType == DCO_EGroupTask.FIRE_MISSION)
			m_Task.m_eType = DCO_EGroupTask.NONE;
	}

	void ShootMortar(SCR_AIWaypoint wp, float worldTime)
	{
		if (!IsMortar())
			return;

		m_Group.AddWaypoint(wp);
		m_Task.m_eType = DCO_EGroupTask.FIRE_MISSION;
		m_Task.m_vTarget = wp.GetOrigin();
		SetPhase(DCO_ETaskPhase.MOVING);
	}

	bool IsGroupHaveWaypoint()
	{
		array<AIWaypoint> wp ={};
		m_Group.GetWaypoints(wp);

		if (wp.Count() > 0)
			return true;
		else
			return false;
	}

	void SetGroupObjective(CMD_AICommanderObjectiveComponent obj)
	{
		m_Task.m_Objective = obj;
	}

	CMD_AICommanderObjectiveComponent GetGroupObjective()
	{
		return m_Task.m_Objective;
	}

	void SetTask(DCO_EGroupTask task)
	{
		if (task == DCO_EGroupTask.FIRE_MISSION && !IsMortar())
			task = DCO_EGroupTask.NONE;
		if (task == DCO_EGroupTask.TRANSPORT && !GetGroupVehicle() && !m_bIsDedicatedTransport)
			task = DCO_EGroupTask.NONE;
		if (IsMortar() && task != DCO_EGroupTask.FIRE_MISSION)
			task = DCO_EGroupTask.NONE;

		if (m_bCruiseLimited && task != DCO_EGroupTask.PATROL && task != DCO_EGroupTask.NONE)
			SetVehicleCruiseSpeed(0);

		m_Task.m_eType = task;
		m_Task.m_fStart_s = GetGame().GetWorld().GetWorldTime() / 1000.0;
		m_Task.m_Issuer = myCommander;
		if (task != DCO_EGroupTask.NONE)
			m_bRetreating = false;
		ApplyTaskStance();
	}

	protected void ApplyTaskStance()
	{
		if (!m_FormationComponent || !m_UtilityComp)
			return;

		string formation = "Wedge";
		switch (m_Task.m_eType)
		{
			case DCO_EGroupTask.RECON:			formation = "Column"; break;
			case DCO_EGroupTask.FLANK:			formation = "Line"; break;
			case DCO_EGroupTask.SUPPORT_BY_FIRE:	formation = "Line"; break;
			case DCO_EGroupTask.PATROL:			formation = "StaggeredColumn"; break;
		}
		if (m_bRetreating)
			formation = "StaggeredColumn";
		m_FormationComponent.SetFormation(formation);

		EAIGroupCombatMode mode = EAIGroupCombatMode.RETURN_FIRE;
		if (IsMortar() || m_bRetreating)
			mode = EAIGroupCombatMode.HOLD_FIRE;
		m_UtilityComp.SetCombatMode(mode);
	}

	DCO_EGroupTask GetTask()			{ return m_Task.m_eType; }
	DCO_GroupTask GetTaskData()			{ return m_Task; }
	DCO_ETaskPhase GetPhase()			{ return m_Task.m_ePhase; }
	bool IsMoving()						{ return m_Task.m_ePhase == DCO_ETaskPhase.MOVING; }

	void SetPhase(DCO_ETaskPhase phase)
	{
		m_Task.m_ePhase = phase;
		m_fStateCheck_ms = -1;
	}

	protected float m_fDCOHoldUntil;
	void DCO_SetHold(float untilTime)	{ m_fDCOHoldUntil = untilTime; }
	bool DCO_IsHeld()					{ return m_fDCOHoldUntil > 0 && GetGame().GetWorld().GetWorldTime() / 1000.0 < m_fDCOHoldUntil; }
	int DCO_GetPeakStrength()			{ return m_iPeakStrength; }

	bool IsAvailableReserve()
	{
		if (m_bInTransport)
			return false;
		if (m_fDCOHoldUntil > 0 && GetGame().GetWorld().GetWorldTime() / 1000.0 < m_fDCOHoldUntil)
			return false;
		if (m_bRetreating && IsMoving())
			return false;
		if (m_Task.m_eType == DCO_EGroupTask.NONE)
			return true;
		return m_Task.m_eType == DCO_EGroupTask.PATROL && !IsMoving();
	}

	void SetRetreating(bool retreating)
	{
		m_bRetreating = retreating;
		if (retreating)
			m_Task.m_eType = DCO_EGroupTask.NONE;
		m_fStateCheck_ms = -1;
		ApplyTaskStance();
	}

	DCO_EGroupCapability GetCapability()
	{
		RefreshCapability();
		return m_eCapability;
	}

	bool IsArmor()		{ return GetCapability() == DCO_EGroupCapability.ARMOR; }
	bool IsMortar()		{ return m_eGroupRoleExternal == DCO_EGroupPreset.MORTAR; }
	bool IsInfantry()	{ DCO_EGroupCapability c = GetCapability(); return c == DCO_EGroupCapability.INFANTRY || c == DCO_EGroupCapability.MOTORIZED; }
	bool HasAT()		{ RefreshCapability(); return (m_iUnitRoles & EUnitRole.AT_SPECIALIST) != 0; }
	bool HasMG()		{ RefreshCapability(); return (m_iUnitRoles & EUnitRole.MACHINEGUNNER) != 0; }
	bool HasMedic()		{ RefreshCapability(); return (m_iUnitRoles & EUnitRole.MEDIC) != 0; }
	bool HasRadio()		{ RefreshCapability(); return m_bHasRadio; }

	protected void RefreshCapability()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (m_fCapabilityCheck_ms >= 0 && now - m_fCapabilityCheck_ms < 5000)
			return;
		m_fCapabilityCheck_ms = now;

		if (myCommander && m_eGroupRoleExternal == DCO_EGroupPreset.AUTO && !m_bAutoDetectDone && now - m_fMortarRetry_ms >= 15000.0)
		{
			m_fMortarRetry_ms = now;
			string stop;
			if (now - m_fAssignedAt_ms > 60000.0)
				stop = "timeout";
			else if (m_Task.m_eType != DCO_EGroupTask.NONE)
				stop = "tasked";

			if (stop.IsEmpty())
			{
				DetectMortar();
				DetectTransport();
			}
			else
			{
				m_bAutoDetectDone = true;
				DCO_BenchmarkLoggerComponent.Event(string.Format("auto_detect_stop grp=%1 reason=%2", GetOwner().GetName(), stop));
			}
		}

		if (myCommander && m_bAutoMortar && m_eGroupRoleExternal == DCO_EGroupPreset.MORTAR)
		{
			if (GetUnitCount() <= 0)
				DemoteMortar("crew_lost");
			else if (!HasUsableMortar())
				DemoteMortar("weapon_lost");
		}

		if (m_eGroupRoleExternal == DCO_EGroupPreset.ARMOR)
			m_eCapability = DCO_EGroupCapability.ARMOR;
		else if (m_eGroupRoleExternal == DCO_EGroupPreset.MORTAR)
			m_eCapability = DCO_EGroupCapability.MORTAR;
		else if (GetGroupVehicle())
			m_eCapability = DCO_EGroupCapability.MOTORIZED;
		else
			m_eCapability = DCO_EGroupCapability.INFANTRY;

		m_iUnitRoles = 0;
		m_bHasRadio = false;
		if (!m_Group)
			return;

		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);
		m_iPeakStrength = Math.Max(m_iPeakStrength, agents.Count());
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (ca && ca.m_InfoComponent && !ca.m_InfoComponent.HasUnitState(EUnitState.UNCONSCIOUS))
			{
				SCR_AIInfoComponent info = ca.m_InfoComponent;
				if (info.HasRole(EUnitRole.AT_SPECIALIST))
					m_iUnitRoles |= EUnitRole.AT_SPECIALIST;
				if (info.HasRole(EUnitRole.MACHINEGUNNER))
					m_iUnitRoles |= EUnitRole.MACHINEGUNNER;
				if (info.HasRole(EUnitRole.MEDIC))
					m_iUnitRoles |= EUnitRole.MEDIC;
			}

			IEntity member = a.GetControlledEntity();
			if (!m_bHasRadio && member)
			{
				SCR_GadgetManagerComponent gadgets = SCR_GadgetManagerComponent.Cast(member.FindComponent(SCR_GadgetManagerComponent));
				m_bHasRadio = gadgets != null && gadgets.GetGadgetByType(EGadgetType.RADIO) != null;
			}
		}

		PublishRoleInfo();
	}

	protected void PublishRoleInfo()
	{
		SCR_AIGroup grp = SCR_AIGroup.Cast(GetOwner());
		if (!grp || !Replication.IsServer())
			return;

		if (!myCommander)
		{
			grp.DCO_SetRoleInfo(-1, 0);
			return;
		}

		int caps = 0;
		if (m_iUnitRoles & EUnitRole.AT_SPECIALIST)
			caps |= 1;
		if (m_iUnitRoles & EUnitRole.MACHINEGUNNER)
			caps |= 2;
		if (m_iUnitRoles & EUnitRole.MEDIC)
			caps |= 4;
		if (m_bHasRadio)
			caps |= 8;

		int role = m_eCapability;
		if (m_bIsDedicatedTransport)
			role = DCO_EGroupCapability.TRANSPORT;
		grp.DCO_SetRoleInfo(role, caps);
	}

	int GetState()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (m_fStateCheck_ms >= 0 && now - m_fStateCheck_ms < 1000)
			return m_iStateCache;
		m_fStateCheck_ms = now;

		int s = DCO_EGroupState.READY;
		if (IsMoving() || m_bOrderActive)
			s |= DCO_EGroupState.EN_ROUTE;
		if (IsInContact())
			s |= DCO_EGroupState.IN_CONTACT;
		if (IsSuppressed())
			s |= DCO_EGroupState.SUPPRESSED;
		if (m_bRetreating)
			s |= DCO_EGroupState.RETREATING;
		if (m_bInTransport || (m_Group && DCO_VehicleCombat.GetVehicle(m_Group.GetLeaderEntity())))
			s |= DCO_EGroupState.MOUNTED;

		int units = GetUnitCount();
		m_iPeakStrength = Math.Max(m_iPeakStrength, units);
		if (m_iPeakStrength > 0 && units < m_iPeakStrength * 0.4)
			s |= DCO_EGroupState.COMBAT_INEFFECTIVE;

		m_iStateCache = s;
		return s;
	}

	bool HasState(DCO_EGroupState flag)
	{
		return (GetState() & flag) != 0;
	}

	protected float m_fMorale;
	protected float m_fMoraleCheck_ms = -1;

	float GetGroupMorale()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (m_fMoraleCheck_ms >= 0 && now - m_fMoraleCheck_ms < 1000 * 3)
			return m_fMorale;
		m_fMoraleCheck_ms = now;

		m_fMorale = 0;
		if (!m_Group)
			return 0;

		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);
		int n, threatened;
		float sum;
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent)
				continue;

			DCO_AIMoraleSystem ms = ca.m_UtilityComponent.GetMoraleSystem();
			if (!ms)
				continue;

			n++;
			sum += ms.GetMoraleMeasure();
			if (ca.m_UtilityComponent.m_ThreatSystem && ca.m_UtilityComponent.m_ThreatSystem.GetState() == EAIThreatState.THREATENED)
				threatened++;
		}
		if (n > 0)
			m_fMorale = sum / n + 0.5 * threatened / n;
		return m_fMorale;
	}

	protected bool IsSuppressed()
	{
		if (!m_Group)
			return false;

		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);
		int alive, threatened;
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent || !ca.m_UtilityComponent.m_ThreatSystem)
				continue;
			alive++;
			if (ca.m_UtilityComponent.m_ThreatSystem.GetState() == EAIThreatState.THREATENED)
				threatened++;
		}
		return alive > 0 && threatened * 2 > alive;
	}

	string GetTaskLabel()
	{
		string s = typename.EnumToString(DCO_EGroupTask, m_Task.m_eType);
		if (!IsInfantry())
			s += " [" + typename.EnumToString(DCO_EGroupCapability, GetCapability()) + "]";
		int st = GetState();
		if (st & DCO_EGroupState.MOUNTED)
			s += " mounted";
		if (st & DCO_EGroupState.RETREATING)
			s += " retreating";
		if (st & DCO_EGroupState.IN_CONTACT)
			s += " contact";
		if (st & DCO_EGroupState.SUPPRESSED)
			s += " suppressed";
		if (st & DCO_EGroupState.COMBAT_INEFFECTIVE)
			s += " ineffective";
		return s;
	}

	protected bool IsSquadSpreadTooFar(vector leaderPos)
	{
		if (!m_Group)
			return false;

		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);
		IEntity leaderVeh = DCO_VehicleCombat.GetVehicle(m_Group.GetLeaderEntity());

		foreach (AIAgent agent : agents)
		{
			ChimeraCharacter controlled = ChimeraCharacter.Cast(agent.GetControlledEntity());
			if (!controlled)
				continue;

			CharacterControllerComponent ctrl = controlled.GetCharacterController();
			if (ctrl && ctrl.GetLifeState() != ECharacterLifeState.ALIVE)
				continue;

			IEntity veh = DCO_VehicleCombat.GetVehicle(controlled);
			if (veh && veh != leaderVeh)
				continue;

			if (m_UtilityComp && m_UtilityComp.DCO_IsStuckStraggler(agent))
				continue;

			if (vector.Distance(controlled.GetOrigin(), leaderPos) > SQUAD_SPREAD_THRESHOLD)
				return true;
		}

		return false;
	}

	protected void BeginOrderTracking(vector targetPos, float worldTime)
	{
		m_vOrderTarget    = targetPos;
		m_fOrderStartTime = worldTime;
		m_bOrderActive    = true;

		float initialDist = vector.Distance(GetOwner().GetOrigin(), targetPos);
		m_fOrderTimeout   = (initialDist / AVG_MOVE_SPEED_MPS) + ORDER_BASE_BUFFER;
	}

	bool CheckOrderComplete(float worldTime)
	{
		if (!m_bOrderActive)
			return true;

		if (!m_Group)
		{
			ResetOrderTracking();
			return true;
		}

		vector currentPos = GetOwner().GetOrigin();
		float distToTarget = vector.Distance(currentPos, m_vOrderTarget);

		if (distToTarget <= ARRIVAL_THRESHOLD)
		{
			if (!IsSquadSpreadTooFar(currentPos))
			{
				SetPhase(DCO_ETaskPhase.HOLDING);
				ResetOrderTracking();
				return true;
			}
		}

		if (m_vLastCheckPos == vector.Zero)
		{
			m_vLastCheckPos = currentPos;
			m_fLastMoveTime = worldTime;
		}
		else
		{
			float distMoved = vector.Distance(currentPos, m_vLastCheckPos);

			if (distMoved > STUCK_DIST_THRESHOLD)
			{
				m_vLastCheckPos = currentPos;
				m_fLastMoveTime = worldTime;
			}
			else
			{
				float stuckElapsed = worldTime - m_fLastMoveTime;
				if (stuckElapsed >= STUCK_TIME_THRESHOLD)
				{
					SetPhase(DCO_ETaskPhase.HOLDING);
					ResetOrderTracking();
					return true;
				}
			}
		}

		float elapsed = worldTime - m_fOrderStartTime;
		if (elapsed >= m_fOrderTimeout)
		{
			SetPhase(DCO_ETaskPhase.HOLDING);
			ResetOrderTracking();
			return true;
		}

		return false;
	}

	protected void ResetOrderTracking()
	{
		m_vOrderTarget    = vector.Zero;
		m_fOrderStartTime = 0.0;
		m_fOrderTimeout   = 0.0;
		m_bOrderActive    = false;
		m_vLastCheckPos = vector.Zero;
		m_fLastMoveTime = 0;
	}

	void ClearAssignment()
	{
		m_Task.m_eType = DCO_EGroupTask.NONE;
		m_bInTransport = false;
		m_bRetreating = false;
	}

	void BeginTransport(DCO_EGroupTask taskAfter)
	{
		if (taskAfter != DCO_EGroupTask.NONE)
			SetTask(taskAfter);
		m_bInTransport = true;
		SetPhase(DCO_ETaskPhase.MOVING);
	}

	bool IsInTransport()	{ return m_bInTransport; }

	protected int m_iSupportPlayerGroup = -1;
	int GetSupportPlayerGroup()			{ return m_iSupportPlayerGroup; }
	void SetSupportPlayerGroup(int id)	{ m_iSupportPlayerGroup = id; }

	void EndTransport()
	{
		m_bInTransport = false;
		m_fStateCheck_ms = -1;
		SetPhase(DCO_ETaskPhase.HOLDING);

		if (m_Task.m_eType == DCO_EGroupTask.NONE)
			m_Task.m_Objective = null;

		ApplyTaskStance();
	}

	bool ShouldUseBounding()
	{
		return m_Task.m_eType == DCO_EGroupTask.ATTACK;
	}

	FactionKey GetFactionKey()
	{
		return fk;
	}

	void OnGroupRemoved()
	{
		if (AICommander_ManagerComponent.GetInstance())
			AICommander_ManagerComponent.GetInstance().UnregisterGroup(this);

		if (m_OwnedVehicle)
		{
			DCO_TransportMissionComponent mission = DCO_TransportMissionComponent.Cast(m_OwnedVehicle.FindComponent(DCO_TransportMissionComponent));
			if (mission)
				mission.ReleaseOwnership();
			m_OwnedVehicle = null;
		}
	}

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_Group = SCR_AIGroup.Cast(owner);
		m_UtilityComp = SCR_AIGroupUtilityComponent.Cast(owner.FindComponent(SCR_AIGroupUtilityComponent));
		m_FormationComponent = AIFormationComponent.Cast(owner.FindComponent(AIFormationComponent));
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (myCommander && Replication.IsServer())
			RefreshCapability();
		UpdateGroupDebug(timeSlice);
	}

	protected void UpdateGroupDebug(float timeSlice)
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

		vector g     = GetOwner().GetOrigin();
		int    color = DCO_DebugDraw.TaskColor(m_Task.m_eType);

		CMD_AICommanderObjectiveComponent obj = GetGroupObjective();

		string taskLine;
		if (obj && obj.GetOwner())
			taskLine = string.Format("task: %1  (%2 away)",
				obj.GetOwner().GetName(),
				DCO_DebugDraw.M(vector.Distance(g, obj.GetOwner().GetOrigin())));
		else
			taskLine = "task: none";

		string cmdLine;
		if (myCommander)
			cmdLine = "commander: " + myCommander.GetCommanderUID();
		else
			cmdLine = "commander: none";

		int   units       = GetUnitCount();
		float strengthPct = Math.Clamp(units / 12.0 * 100.0, 0.0, 100.0);

		string label = string.Format(
			"%1  x%2  (id %3)\n%4\n%5\n%6",
			GetTaskLabel(),
			units,
			GetGroupID(),
			DCO_DebugDraw.PhaseName(GetPhase()),
			taskLine,
			cmdLine);

		string flagsLine = string.Format(
			"strength %1%% | morale %9\nwp %2 | order %3 | override %4 | orderable %5\nvehicle %6 | transport %7 | player %8",
			Math.Round(strengthPct),
			DCO_DebugDraw.YesNo(IsGroupHaveWaypoint()),
			DCO_DebugDraw.YesNo(IsOrderActive()),
			DCO_DebugDraw.YesNo(CanCommanderOverrideRole()),
			DCO_DebugDraw.YesNo(CanItHaveOrder()),
			DCO_DebugDraw.YesNo(HasOwnedVehicle()),
			DCO_DebugDraw.YesNo(IsDedicatedTransport()),
			DCO_DebugDraw.YesNo(IsPlayerGroup()),
			GetGroupMorale().ToString(-1, 2));

		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(Vector(g[0], g[1] + 12.0, g[2]), label,     17.0, color));
		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(Vector(g[0], g[1] +  4.0, g[2]), flagsLine, 15.0, color));

		if (obj && obj.GetOwner())
		{
			vector t = obj.GetOwner().GetOrigin();
			m_aDebugShapes.Insert(Shape.CreateArrow(
				Vector(g[0], g[1] + 1.5, g[2]),
				Vector(t[0], t[1] + 1.5, t[2]),
				1.2, color, DCO_DebugDraw.Flags()));
		}
	}

	void RegisterCommanderToGroup(AICommander_BaseComponent cmd)
	{
		myCommander = cmd;
		Print(myCommander.GetCommanderUID() + " < MY COMMANDER | MY GROUP > " + GetTaskLabel());
		threatComp = myCommander.GetThreatResponseComponent();
	}

	void SetDedicatedTransport(bool tf)
	{
		DedicatedTransport = DCO_TransportTeamComponent.Cast(GetOwner().FindComponent(DCO_TransportTeamComponent));
		if (!DedicatedTransport || tf == m_bIsDedicatedTransport)
			return;

		m_bAutoTransport = false;
		AICommander_BaseComponent cmd = myCommander;
		if (!cmd)
		{
			m_bIsDedicatedTransport = tf;
			return;
		}

		cmd.ReleaseGroup(this);
		m_bIsDedicatedTransport = tf;
		cmd.AssignGroup(this);
		m_bAutoDetectDone = true;
		DCO_BenchmarkLoggerComponent.Event(string.Format("transport_team_set grp=%1 dedicated=%2 by=GM", GetOwner().GetName(), tf));
	}

	bool IsAutoTransport()			{ return m_bAutoTransport; }

	void DemoteAutoTransport(string reason)
	{
		m_bAutoTransport = false;
		m_bAutoDetectDone = true;
		m_bIsDedicatedTransport = false;
		DCO_BenchmarkLoggerComponent.Event(string.Format("transport_team_demoted grp=%1 reason=%2", GetOwner().GetName(), reason));
		DCO_PlayerComms.SendToGameMasters("DCO", DCO_PlayerComms.GetCallsign(m_Group) + " is no longer a transport team (" + reason + ")");
	}

	bool IsPlayerGroup()
	{
		SCR_AIGroup grp = SCR_AIGroup.Cast(GetOwner());
		if (!grp)
			return false;

		IEntity leader = grp.GetLeaderEntity();
		IsPlayerGroup = grp.GetLeaderID() > 0 || SCR_CharacterHelper.IsPlayer(leader) || (!leader && grp.GetTotalPlayerCount() > 0);
		return IsPlayerGroup;
	}

	void MoveToRoute(notnull array<SCR_AIWaypoint> wps, float worldTime)
	{
		if (!m_Group || wps.IsEmpty())
			return;

		float routeLength = 0.0;
		vector prev  = GetOwner().GetOrigin();
		vector final = prev;
		int added = 0;

		foreach (SCR_AIWaypoint wp : wps)
		{
			if (!wp)
				continue;

			m_Group.AddWaypoint(wp);

			vector wpPos = wp.GetOrigin();
			routeLength += vector.Distance(prev, wpPos);
			prev  = wpPos;
			final = wpPos;
			added++;
		}

		if (added == 0)
			return;

		SetPhase(DCO_ETaskPhase.MOVING);

		m_vOrderTarget    = final;
		m_fOrderStartTime = worldTime;
		m_bOrderActive    = true;
		m_fOrderTimeout   = (routeLength / AVG_MOVE_SPEED_MPS) + ORDER_BASE_BUFFER;
		m_vLastCheckPos   = vector.Zero;
		m_fLastMoveTime   = 0.0;
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (!AICommander_ManagerComponent.GetInstance() && !m_bAutoAssign)
			return;

		if (!m_bIsProcessedByCommander && !m_bCanHaveCommander)
			return;

		SCR_AIGroup grp = SCR_AIGroup.Cast(owner);

		contactReportComponent = DCO_GroupContactReporterComponent.Cast(owner.FindComponent(DCO_GroupContactReporterComponent));
		GetGame().GetCallqueue().CallLater(delayedInit, 5000, false, owner);
	}

	bool CanItHaveOrder()
	{
		return m_bIsProcessedByCommander && !IsHeldByGM() && !IsIdleIgnoringCommander();
	}

	protected bool IsIdleIgnoringCommander()
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (!cfg || cfg.GetIdleFollowCommander())
			return false;

		return m_UtilityComp && m_UtilityComp.DCO_IsIdle();
	}

	bool IsHeldByGM()
	{
		AIGroup group = AIGroup.Cast(GetOwner());
		if (!group || !group.GetLeaderEntity())
			return false;

		SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(group.GetLeaderEntity().FindComponent(SCR_AICombatComponent));
		if (!combat || !combat.GetUtilityComponent() || !combat.GetUtilityComponent().m_DCOConfig)
			return false;

		return combat.GetUtilityComponent().m_DCOConfig.IsHoldPosition();
	}

	protected void delayedInit(IEntity owner)
	{
		SCR_AIGroup grp = SCR_AIGroup.Cast(owner);
		Faction fc = grp.GetFaction();
		if (fc)
			fk = grp.GetFaction().GetFactionKey();

		foreach(string s : m_sUsableVehicle)
		{
			IEntity e = GetGame().GetWorld().FindEntityByName(s);

			SCR_AIVehicleUsageComponent aiveh = FindVehicleUsage(e, 3);
			if (aiveh)
				m_UtilityComp.AddUsableVehicle(aiveh);
		}

		if (m_bAutoAssign && Replication.IsServer())
			TryAutoAssign();
	}

	protected static SCR_AIVehicleUsageComponent FindVehicleUsage(IEntity e, int depth)
	{
		if (!e)
			return null;

		SCR_AIVehicleUsageComponent usage = SCR_AIVehicleUsageComponent.Cast(e.FindComponent(SCR_AIVehicleUsageComponent));
		if (usage || depth <= 0)
			return usage;

		for (IEntity child = e.GetChildren(); child; child = child.GetSibling())
		{
			usage = FindVehicleUsage(child, depth - 1);
			if (usage)
				return usage;
		}

		return null;
	}

	protected float m_fMortarRetry_ms = -15000;
	protected static ref map<IEntity, DCO_GroupUtilityComponent> s_mMortarCrew = new map<IEntity, DCO_GroupUtilityComponent>();
	protected SCR_AIVehicleUsageComponent m_FoundMortar;
	protected SCR_AIVehicleUsageComponent m_AutoMortarUsage;
	protected bool m_bAutoMortar;
	protected bool m_bAutoTransport;
	protected bool m_bAutoDetectDone;
	protected float m_fAssignedAt_ms;

	protected int AutoDetectSize()
	{
		return Math.Max(m_iPeakStrength, GetUnitCount());
	}

	protected bool HasUsableMortar()
	{
		return m_AutoMortarUsage && m_AutoMortarUsage.GetOwner() && m_AutoMortarUsage.GetDamageState() != EDamageState.DESTROYED;
	}

	protected void DemoteMortar(string reason)
	{
		if (m_AutoMortarUsage && m_AutoMortarUsage.GetOwner())
			s_mMortarCrew.Remove(m_AutoMortarUsage.GetOwner());
		m_AutoMortarUsage = null;
		m_bAutoMortar = false;
		m_bAutoDetectDone = true;
		m_eGroupRoleExternal = DCO_EGroupPreset.AUTO;
		GetGame().GetCallqueue().Remove(CheckGroupIsHaveOrder);
		if (myCommander)
			myCommander.DemoteFromArtillery(this, reason);
		DCO_PlayerComms.SendToGameMasters("DCO", DCO_PlayerComms.GetCallsign(m_Group) + " is no longer a mortar team (" + reason + ")");
	}

	protected void DetectMortar()
	{
		if (!m_Group || !m_UtilityComp || !m_Group.GetLeaderEntity())
			return;
		if (m_eGroupRoleExternal != DCO_EGroupPreset.AUTO && m_eGroupRoleExternal != DCO_EGroupPreset.MORTAR)
			return;
		if (m_eGroupRoleExternal == DCO_EGroupPreset.AUTO && AutoDetectSize() > 4)
			return;

		array<ref SCR_AIGroupVehicle> vehicles = {};
		if (m_UtilityComp.m_VehicleMgr)
			m_UtilityComp.m_VehicleMgr.GetAllVehicles(vehicles);
		foreach (SCR_AIGroupVehicle v : vehicles)
		{
			SCR_AIVehicleUsageComponent usage = null;
			if (v)
				usage = v.GetVehicleUsageComponent();
			if (usage && usage.GetVehicleType() == EAIVehicleType.STATIC_ARTILLERY && usage.GetDamageState() != EDamageState.DESTROYED)
			{
				s_mMortarCrew.Set(v.GetEntity(), this);
				bool wasMortar = m_eGroupRoleExternal == DCO_EGroupPreset.MORTAR;
				if (!wasMortar)
				{
					m_bAutoMortar = true;
					m_AutoMortarUsage = usage;
				}
				m_eGroupRoleExternal = DCO_EGroupPreset.MORTAR;
				m_fCapabilityCheck_ms = -1;
				if (myCommander && !wasMortar)
				{
					myCommander.PromoteToArtillery(this);
					GetGame().GetCallqueue().Remove(CheckGroupIsHaveOrder);
					GetGame().GetCallqueue().CallLater(CheckGroupIsHaveOrder, 10000, true);
				}
				return;
			}
		}

		m_FoundMortar = null;
		DCO_Perf.Count("q:DCO_GroupUtility");
		GetGame().GetWorld().QueryEntitiesBySphere(m_Group.GetLeaderEntity().GetOrigin(), 40.0, MortarQueryCallback, null, EQueryEntitiesFlags.ALL);
		if (!m_FoundMortar)
			return;

		s_mMortarCrew.Set(m_FoundMortar.GetOwner(), this);
		m_UtilityComp.AddUsableVehicle(m_FoundMortar);
		if (m_eGroupRoleExternal == DCO_EGroupPreset.AUTO)
		{
			m_bAutoMortar = true;
			m_AutoMortarUsage = m_FoundMortar;
		}
		m_eGroupRoleExternal = DCO_EGroupPreset.MORTAR;
		m_fCapabilityCheck_ms = -1;
		Print(string.Format("[DCO_Group] %1 -> tim mortir otomatis (%2)", GetOwner().GetName(), m_FoundMortar.GetOwner()));
		if (myCommander)
		{
			myCommander.PromoteToArtillery(this);
			GetGame().GetCallqueue().Remove(CheckGroupIsHaveOrder);
			GetGame().GetCallqueue().CallLater(CheckGroupIsHaveOrder, 10000, true);
		}
		DCO_PlayerComms.SendToGameMasters("DCO", DCO_PlayerComms.GetCallsign(SCR_AIGroup.Cast(GetOwner())) + " is now a mortar team");
	}


	protected void DetectTransport()
	{
		if (m_bIsDedicatedTransport || !m_Group || !myCommander || m_OwnedVehicle || IsPlayerGroup() || !IsAvailableReserve())
			return;
		if (m_eGroupRoleExternal != DCO_EGroupPreset.AUTO || AutoDetectSize() > 3)
			return;

		DCO_TransportTeamComponent team = DCO_TransportTeamComponent.Cast(GetOwner().FindComponent(DCO_TransportTeamComponent));
		IEntity veh = DCO_VehicleCombat.GetVehicle(m_Group.GetLeaderEntity());
		if (!team || !veh || DCO_TransportTeamComponent.CountFreeCargo(veh) < 4)
			return;

		SCR_BaseCompartmentManagerComponent mgr = SCR_BaseCompartmentManagerComponent.Cast(veh.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!mgr)
			return;

		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);
		array<IEntity> crew = {};
		foreach (AIAgent a : agents)
		{
			IEntity ent = a.GetControlledEntity();
			if (!ent || DCO_VehicleCombat.GetVehicle(ent) != veh)
				return;
			crew.Insert(ent);
		}

		array<BaseCompartmentSlot> slots = {};
		mgr.GetCompartments(slots);
		bool driving = false;
		foreach (BaseCompartmentSlot slot : slots)
		{
			if (TurretCompartmentSlot.Cast(slot))
				return;
			if (PilotCompartmentSlot.Cast(slot) && crew.Contains(slot.GetOccupant()))
				driving = true;
		}
		if (!driving)
			return;

		m_bIsDedicatedTransport = true;
		m_bAutoTransport = true;
		SetTask(DCO_EGroupTask.NONE);
		myCommander.RegisterTransportTeam(team);
		Print(string.Format("[DCO_Group] %1 -> tim transport otomatis (%2)", GetOwner().GetName(), veh));
		DCO_PlayerComms.SendToGameMasters("DCO", DCO_PlayerComms.GetCallsign(m_Group) + " is now a transport team");
	}

	protected bool MortarQueryCallback(IEntity e)
	{
		SCR_AIVehicleUsageComponent usage = SCR_AIVehicleUsageComponent.Cast(e.FindComponent(SCR_AIVehicleUsageComponent));
		if (!usage || usage.GetVehicleType() != EAIVehicleType.STATIC_ARTILLERY || usage.GetDamageState() == EDamageState.DESTROYED)
			return true;

		DCO_GroupUtilityComponent owner = s_mMortarCrew.Get(e);
		if (owner && owner != this && owner.GetMyCommander())
			return true;

		m_FoundMortar = usage;
		return false;
	}

	protected void TryAutoAssign()
	{
		if (myCommander)
			return;

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
		{
			AICommander_BaseComponent cmd = mgr.PickCommanderForFaction(fk, m_sDedicatedCo);
			if (cmd && cmd.AssignGroup(this))
				return;
		}

		m_iAutoAssignTries++;
		if (m_iAutoAssignTries < AUTO_ASSIGN_MAX_TRIES)
			GetGame().GetCallqueue().CallLater(TryAutoAssign, AUTO_ASSIGN_RETRY_MS, false);
		else
			Print(string.Format("[DCO_Group] Auto-assign %1 gagal: gak ada commander '%2' faction %3", GetOwner().GetName(), m_sDedicatedCo, fk), LogLevel.WARNING);
	}

	DCO_AIGarrisonActivity GetGarrisonActivity()
	{
		if (!m_UtilityComp)
			return null;
		return m_UtilityComp.DCO_GetGarrisonActivity();
	}

	protected const float CONTACT_FRESH_S = 15.0;
	protected const float CONTACT_CLOSE_DIST = 30.0;

	bool IsInContact()
	{
		if (!m_UtilityComp)
			return false;

		SCR_AIGroupPerception perc = m_UtilityComp.GetPercGroupComp();
		if (!perc)
			return false;

		foreach (SCR_AIGroupTargetCluster c : perc.m_aTargetClusters)
		{
			if (!c.m_State || c.m_State.m_iCountAlive <= 0)
				continue;

			if (c.m_State.m_iCountEndangering > 0 && c.m_State.GetTimeSinceLastNewInformation() < CONTACT_FRESH_S)
				return true;

			if (c.m_State.m_fDistMin < CONTACT_CLOSE_DIST)
				return true;
		}
		return false;
	}

	void ActivateForCommander(AICommander_BaseComponent cmd)
	{
		if (!cmd)
			return;

		if (m_UtilityComp)
			m_eCombatModeBeforeCommander = m_UtilityComp.DCO_GetCombatModeExternal();

		m_fAssignedAt_ms = GetGame().GetWorld().GetWorldTime();
		m_bAutoDetectDone = false;
		DetectMortar();
		m_sDedicatedCo = cmd.GetCommanderUID();
		RegisterCommanderToGroup(cmd);

		SetTask(DCO_EGroupTask.NONE);

		if (contactReportComponent)
			contactReportComponent.InitializeContactReport();

		if (IsMortar())
			GetGame().GetCallqueue().CallLater(CheckGroupIsHaveOrder, 10000, true);

		SetEventMask(GetOwner(), EntityEvent.FRAME);
	}

	void ReleaseFromCommander()
	{
		GetGame().GetCallqueue().Remove(CheckGroupIsHaveOrder);

		if (m_Group)
			m_Group.CompleteAllWaypoints();

		if (m_OwnedVehicle)
		{
			DCO_TransportMissionComponent mission = DCO_TransportMissionComponent.Cast(m_OwnedVehicle.FindComponent(DCO_TransportMissionComponent));
			if (mission && mission.IsOwnedBy(this))
				mission.ReleaseOwnership();
			m_OwnedVehicle = null;
		}

		m_Task.m_Objective = null;
		ClearAssignment();
		SetPhase(DCO_ETaskPhase.HOLDING);
		ResetOrderTracking();

		myCommander = null;
		threatComp = null;
		m_sDedicatedCo = string.Empty;
		PublishRoleInfo();

		if (contactReportComponent)
			contactReportComponent.DeactivateContactReport();

		if (m_UtilityComp)
			m_UtilityComp.SetCombatMode(m_eCombatModeBeforeCommander);

		ClearEventMask(GetOwner(), EntityEvent.FRAME);
		m_aDebugShapes.Clear();
		m_aDebugTexts.Clear();
	}
}
