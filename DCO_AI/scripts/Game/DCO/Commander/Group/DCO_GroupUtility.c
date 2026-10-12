enum DCO_EGroupLOD
{
	INHERIT,
	VANILLA,
	PREVENT_MAX_LOD,
	FULL_DETAIL
}

enum DCO_ESOPAction
{
	DYNAMIC,
	ENGAGE,
	OBSERVE,
	AMBUSH,
	BYPASS,
	WITHDRAW
}

enum DCO_EAmbushReaction
{
	DYNAMIC,
	SUPPRESS,
	FAN_OUT,
	GO_PRONE
}

enum DCO_ERetreatWhen
{
	DEFAULT,
	NEVER,
	OVERRAN,
	DEPLETED,
	IN_CONTACT,
	THREATENED
}

[ComponentEditorProps(category: "GameScripted/Group")]
class DCO_GroupUtilityComponentClass : ScriptComponentClass
{
}

class DCO_GroupUtilityComponent : ScriptComponent
{
	[Attribute("0", UIWidgets.ComboBox, "AI LOD for this group. INHERIT = commander groups follow the global lodMode, groups without a commander use vanilla. Other values always apply, with or without a commander.", enums: ParamEnumArray.FromEnum(DCO_EGroupLOD), category: "Performance")]
	protected DCO_EGroupLOD m_eLODOverride;

	[Attribute("0", UIWidgets.ComboBox, "AI Detail for this squad. FULL = all DCO features. BALANCED = no lean, window fire, awareness or rearm. LIGHT = only specialists, contact drill and explosion reactions (lightest). CUSTOM = use Features Off below.", enums: ParamEnumArray.FromEnum(DCO_EGroupDetail), category: "Performance")]
	protected DCO_EGroupDetail m_eDCODetail;

	[Attribute("0", UIWidgets.Flags, "DCO features turned off for this squad (used when AI Detail = CUSTOM).", enums: ParamEnumArray.FromEnum(DCO_EGroupFeature), category: "Performance")]
	protected DCO_EGroupFeature m_eDCOFeaturesOff;

	[Attribute("0", UIWidgets.ComboBox, "When the squad withdraws on its own (break contact). DEFAULT = overwhelmed or 40% or less left.", enums: ParamEnumArray.FromEnum(DCO_ERetreatWhen), category: "Behavior")]
	protected DCO_ERetreatWhen m_eRetreatWhen;

	[Attribute("0", UIWidgets.ComboBox, "Squad reaction when ambushed. DYNAMIC = assault if close; if far, take cover + smoke, then flank or break contact.", enums: ParamEnumArray.FromEnum(DCO_EAmbushReaction), category: "Behavior")]
	protected DCO_EAmbushReaction m_eAmbushReaction;

	ref DCO_BreakContact m_DCOBreak;
	float m_fDCOIneffUntil_s = -1;
	ref DCO_AmbushDrill m_DCOAmbush;
	ref array<vector> m_aDCOImpactPos;
	ref array<float> m_aDCOImpactAt;
	float m_fDCODisplacedAt = -1000;
	float m_fDCOIDFRadioAt = -1000;
	[Attribute("0", UIWidgets.ComboBox, "Contact SOP when we see the enemy first. DYNAMIC = from task + phase (recon observes, patrol hasty ambush, attack on the move bypasses, etc.).", enums: ParamEnumArray.FromEnum(DCO_ESOPAction), category: "Behavior")]
	protected DCO_ESOPAction m_eSOPOverride;

	DCO_ESOPAction m_eSOPAction;
	bool m_bSOPSprung;
	float m_fDCOVoiceAmbushUntil;
	float m_fDCOVoiceReconFireUntil;
	ref DCO_MountedState m_DCOMounted;
	DCO_GroupUtilityComponent m_DCOSupportOf;
	ref DCO_PairState m_DCOPair;
	DCO_GroupTactics m_eDCOCmdPosture;
	bool m_bDCOCmdAmbush;
	DCO_GroupTactics m_eDCOTablePosture;
	string m_sDCOPosRow;
	string m_sDCOPosKey;
	string m_sDCOPosTactic = "-";
	int m_iDCOPosStage;
	bool m_bDCOPosNight;
	bool m_bDCOPosClosed;
	bool m_bDCOPosFront;
	bool m_bDCOPosSprung;
	bool m_bDCOPosVarLogged;
	float m_fDCOPosRoll;
	int m_iDCOThreatAct = -1;
	bool m_bDCOPursuitCons;
	bool m_bDCOAssaultCommitted;
	int m_iDCOCommitStep;
	float m_fDCOCommitBand_ms = -1;
	float m_fDCOCommitStepAt_ms;
	float m_fDCOPrepAt_ms = -100000.0;
	string m_sDCOPrepKind = "grenade";
	int m_iDCODefRole;
	float m_fDCODefHoldLog;
	bool m_bSOPReturnFire;
	float m_fSOPDetourAt = -1000;

	DCO_ESOPAction GetSOPOverride()			{ return m_eSOPOverride; }
	void SetSOPOverride(DCO_ESOPAction v)	{ m_eSOPOverride = v; }
	float m_fDCOAmbushAt = -1;

	DCO_EAmbushReaction GetAmbushReaction()		{ return m_eAmbushReaction; }
	void SetAmbushReaction(DCO_EAmbushReaction v)	{ m_eAmbushReaction = v; }

	DCO_ERetreatWhen GetRetreatWhen()		{ return m_eRetreatWhen; }

	[Attribute("0", UIWidgets.CheckBox, "Use the vehicle exit rule below for this squad instead of the global one.", category: "Behavior")]
	protected bool m_bDCOOwnVehicleExit;

	[Attribute("15", UIWidgets.Flags, "When this squad may leave its vehicles. Used only when the box above is ticked.", enums: ParamEnumArray.FromEnum(DCO_EVehicleExit), category: "Behavior")]
	protected int m_iDCOVehicleExit;

	int DCO_VehicleExit()
	{
		if (!m_bDCOOwnVehicleExit)
			return -1;
		return m_iDCOVehicleExit & 31;
	}

	void DCO_SetVehicleExit(int flags)
	{
		if (flags < 0)
		{
			m_bDCOOwnVehicleExit = false;
			return;
		}
		m_bDCOOwnVehicleExit = true;
		m_iDCOVehicleExit = flags & 31;
	}
	void SetRetreatWhen(DCO_ERetreatWhen v)	{ m_eRetreatWhen = v; }
	bool IsBreakingContact()				{ return m_DCOBreak != null; }

	DCO_EGroupLOD GetLODOverride()			{ return m_eLODOverride; }

	void SetLODOverride(DCO_EGroupLOD value)
	{
		m_eLODOverride = value;
		DCO_ApplyLOD();
	}

	DCO_EAILODMode EffectiveLOD()
	{
		if (m_eLODOverride == DCO_EGroupLOD.VANILLA)
			return DCO_EAILODMode.VANILLA;
		if (m_eLODOverride == DCO_EGroupLOD.PREVENT_MAX_LOD)
			return DCO_EAILODMode.PREVENT_MAX_LOD;
		if (m_eLODOverride == DCO_EGroupLOD.FULL_DETAIL)
			return DCO_EAILODMode.FULL_DETAIL;
		if (!myCommander)
			return DCO_EAILODMode.VANILLA;
		return DCO_GlobalAIComponent.GlobalLODMode();
	}

	void DCO_ApplyLOD()
	{
		if (!Replication.IsServer())
			return;
		SCR_AIGroup lodGroup = SCR_AIGroup.Cast(GetOwner());
		if (!lodGroup)
			return;

		DCO_EAILODMode mode = EffectiveLOD();
		array<AIAgent> lodAgents = {};
		lodGroup.GetAgents(lodAgents);
		foreach (AIAgent a : lodAgents)
		{
			if (a)
				DCO_GlobalAIComponent.ApplyLOD(a, mode);
		}
		PublishLOD(lodGroup, mode);
	}

	void DCO_ApplyAgentLOD(AIAgent agent)
	{
		if (!agent || !Replication.IsServer())
			return;
		DCO_EAILODMode mode = EffectiveLOD();
		DCO_GlobalAIComponent.ApplyLOD(agent, mode);
		PublishLOD(SCR_AIGroup.Cast(GetOwner()), mode);
	}

	protected void PublishLOD(SCR_AIGroup lodGroup, DCO_EAILODMode mode)
	{
		if (!lodGroup)
			return;
		int source = 0;
		if (m_eLODOverride != DCO_EGroupLOD.INHERIT)
			source = 2;
		else if (myCommander)
			source = 1;
		lodGroup.DCO_SetLODInfo(mode + source * 4);
	}

	[Attribute("0", UIWidgets.CheckBox, desc: "Draw this group's overlay (role, task, order status) as 3D text. Only visible while Game Master is open.", category: "Group Debug")]
	protected bool m_bDebugMode;

	[Attribute("0.5", UIWidgets.EditBox, "Interval (s) between group overlay redraws.", category: "Group Debug")]
	protected float m_fDebugRefreshInterval;

	protected ref array<ref Shape> m_aDebugShapes = new array<ref Shape>();
	protected ref array<ref DebugTextWorldSpace> m_aDebugTexts = new array<ref DebugTextWorldSpace>();
	protected float m_fDebugTimer = 0.0;

	[Attribute("0", UIWidgets.CheckBox, "Dedicated transport team: this group only does logistics jobs (needs DCO_TransportTeamComponent).", category: "Commander")]
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

	[Attribute("1", UIWidgets.CheckBox, "The commander may send this squad as reinforcements or a threat response.", category: "Commander")]
	protected bool m_bAllowReinforce;

	[Attribute("1", UIWidgets.CheckBox, "The commander may move this squad by vehicle or transport team.", category: "Commander")]
	protected bool m_bAllowTransport;

	[Attribute("1", UIWidgets.CheckBox, "This squad may be used for reserve patrols.", category: "Commander")]
	protected bool m_bAllowPatrol;

	[Attribute("1", UIWidgets.CheckBox, "This squad sends contact reports to the Commander.", category: "Commander")]
	protected bool m_bAllowContactReport;

	[Attribute("1", UIWidgets.CheckBox, "This squad may be sent to help with player requests (suppress, support, flank).", category: "Commander")]
	protected bool m_bAllowPlayerSupport;

	[Attribute("511", UIWidgets.Flags, "Jobs the AI commander may give this squad.", enums: ParamEnumArray.FromEnum(DCO_EGroupJob), category: "Commander")]
	protected int m_iDCOAllowedJobs;

	[Attribute("0", UIWidgets.CheckBox, "The commander picks this squad first when it builds an attack.", category: "Commander")]
	protected bool m_bDCOFirstToFight;

	int GetAllowedJobs()				{ return m_iDCOAllowedJobs; }
	void SetAllowedJobs(int mask)		{ m_iDCOAllowedJobs = mask & 511; }
	bool AllowsJobBits(int mask)		{ return (m_iDCOAllowedJobs & mask) != 0; }
	bool IsFirstToFight()				{ return m_bDCOFirstToFight; }
	void SetFirstToFight(bool b)		{ m_bDCOFirstToFight = b; }

	bool CanReinforce()					{ return m_bAllowReinforce; }
	bool CanBeTransported()				{ return m_bAllowTransport; }
	bool CanPatrol()					{ return (m_iDCOAllowedJobs & 64) != 0; }
	bool CanReportContacts()			{ return m_bAllowContactReport; }
	bool CanSupportPlayers()			{ return m_bAllowPlayerSupport; }
	void SetCanReinforce(bool b)		{ m_bAllowReinforce = b; }
	void SetCanBeTransported(bool b)	{ m_bAllowTransport = b; }
	void SetCanPatrol(bool b)
	{
		m_bAllowPatrol = b;
		if (b)
			m_iDCOAllowedJobs = m_iDCOAllowedJobs | 64;
		else
			m_iDCOAllowedJobs = m_iDCOAllowedJobs & 447;
		DCO_GroupJobs.LogSet(this, -1);
	}

	protected void DCO_MigratePatrol()
	{
		if (m_bAllowPatrol || (m_iDCOAllowedJobs & 64) == 0)
			return;
		m_iDCOAllowedJobs = m_iDCOAllowedJobs & 447;
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("grp_flags_migrated grp=%1 from=allow_patrol_off cleared=patrol mask=%2", GetOwner().GetName(), m_iDCOAllowedJobs));
	}
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

	[Attribute("0", UIWidgets.SearchComboBox, "Group capability preset (AUTO = from group contents).", enums: ParamEnumArray.FromEnum(DCO_EGroupPreset))]
	protected DCO_EGroupPreset m_eGroupRoleExternal;

	[Attribute("", UIWidgets.Auto, "Blacklisted Commander to not process this Group", category: "Commander")]
	protected ref array<string> m_sBlacklistedCo;

	[Attribute("", UIWidgets.Auto, "Assign This Unit To Commander", category: "Commander")]
	protected string m_sDedicatedCo;

	[Attribute("0", UIWidgets.CheckBox, "Auto-assign to a commander when the scenario starts (no GM). Empty Dedicated Co = a random commander of the same faction.", category: "Commander")]
	protected bool m_bAutoAssign;

	protected int m_iAutoAssignTries;

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

	static float AVG_MOVE_SPEED_MPS() { return 2.5; }
	static float ORDER_BASE_BUFFER() { return 10.0; }
	static float ARRIVAL_THRESHOLD() { return 3.0; }

	static float SQUAD_SPREAD_THRESHOLD() { return 30.0; }

	protected vector m_vLastCheckPos = vector.Zero;
	protected float m_fLastMoveTime = 0;

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

			if (DCO_Util.IsNotDead(c))
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

	bool DCO_TransportPending()
	{
		return myCommander && m_eGroupRoleExternal == DCO_EGroupPreset.AUTO && !m_bAutoDetectDone && (GetGroupVehicle() || m_OwnedVehicle);
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
		SCR_BoardingEntityWaypoint bwp = SCR_BoardingEntityWaypoint.Cast(wp);
		if (!bwp || !bwp.GetEntity())
			DCO_BoardOwned(wp.GetOrigin());
		SetPhase(DCO_ETaskPhase.MOVING);
		m_Task.m_vTarget = wp.GetOrigin();
		BeginOrderTracking(wp.GetOrigin(), worldTime);
	}

	bool DCO_BoardOwned(vector target)
	{
		if (!m_Group || !m_OwnedVehicle || IsMortar() || m_bInTransport)
			return false;
		if (GetGroupVehicle() != m_OwnedVehicle || IsMountedIn(m_OwnedVehicle))
			return false;
		vector gp = GetOwner().GetOrigin();
		if (m_Group.GetLeaderEntity())
			gp = m_Group.GetLeaderEntity().GetOrigin();
		if (vector.Distance(m_OwnedVehicle.GetOrigin(), gp) > 200.0)
			return false;
		if (!IsArmor() && target != vector.Zero && vector.Distance(gp, target) < 300.0)
			return false;
		array<AIWaypoint> wps = {};
		m_Group.GetWaypoints(wps);
		foreach (AIWaypoint w : wps)
		{
			if (SCR_BoardingWaypoint.Cast(w))
			{
				DCO_Perf.Count("veh:board_owned_skip_has_boarding");
				if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG))
					DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, "-", string.Format("veh_board_owned_skip grp=%1 reason=has_boarding_wp wp=%2", m_Group.GetName(), w.ClassName()));
				return false;
			}
		}
		Resource res = Resource.Load("{712F4795CF8B91C7}Prefabs/AI/Waypoints/AIWaypoint_GetIn.et");
		if (!res || !res.IsValid())
			return false;
		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = m_OwnedVehicle.GetOrigin();
		SCR_BoardingEntityWaypoint board = SCR_BoardingEntityWaypoint.Cast(GetGame().SpawnEntityPrefab(res, null, params));
		if (!board)
			return false;
		board.SetCompletionRadius(25);
		board.SetEntity(m_OwnedVehicle);
		board.SetAllowance(true, true, true);
		board.SetCompletionType(EAIWaypointCompletionType.All);
		m_Group.AddWaypointAt(board, 0);
		DCO_Perf.Count("veh:board_owned");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("veh_board_owned grp=%1 veh=%2", m_Group.GetName(), m_OwnedVehicle));
		return true;
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

	static string DCO_TaskBy;

	static string DCO_TaskByText(string by)
	{
		if (!by.IsEmpty())
			return by;
		if (DCO_TaskBy.IsEmpty())
			return "-";
		return DCO_TaskBy;
	}

	void SetTask(DCO_EGroupTask task, string by = "")
	{
		if (task == DCO_EGroupTask.FIRE_MISSION && !IsMortar())
			task = DCO_EGroupTask.NONE;
		if (task == DCO_EGroupTask.TRANSPORT && !GetGroupVehicle() && !m_bIsDedicatedTransport)
			task = DCO_EGroupTask.NONE;
		if (IsMortar() && task != DCO_EGroupTask.FIRE_MISSION)
			task = DCO_EGroupTask.NONE;

		if (m_bCruiseLimited && task != DCO_EGroupTask.PATROL && task != DCO_EGroupTask.NONE)
			SetVehicleCruiseSpeed(0);

		if (task != m_Task.m_eType && myCommander && myCommander.GetTactics() && DCO_TacticsCompat.OwnsGroup(myCommander.GetTactics(), this))
		{
			string objName = "-";
			if (GetGroupObjective())
				objName = GetGroupObjective().GetLogName();
			string act = "-";
			if (m_UtilityComp && m_UtilityComp.GetCurrentAction())
				act = m_UtilityComp.GetCurrentAction().ClassName();
			DCO_Perf.Count("task_change_plan_group");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("task_change grp=%1 from=%2 to=%3 obj=%4 by=%5 activity=%6", GetOwner().GetName(), typename.EnumToString(DCO_EGroupTask, m_Task.m_eType), typename.EnumToString(DCO_EGroupTask, task), objName, DCO_TaskByText(by), act));
		}

		if (task != m_Task.m_eType)
			DCO_Events.SquadTask(AIGroup.Cast(GetOwner()), m_Task.m_eType, task, DCO_TaskByText(by));
		m_Task.m_eType = task;
		m_Task.m_fStart_s = DCO_Util.NowS();
		m_Task.m_Issuer = myCommander;
		m_eDCOCmdPosture = DCO_GroupTactics.AUTOMATIC;
		m_bDCOCmdAmbush = false;
		m_sDCOPosKey = "";
		m_iDCOThreatAct = -1;
		m_bDCOPursuitCons = false;
		m_bDCOAssaultCommitted = false;
		m_iDCOCommitStep = 0;
		m_fDCOCommitBand_ms = -1;
		if (task != DCO_EGroupTask.NONE)
			m_bRetreating = false;
		ApplyTaskStance();
		DCO_Battle.OnTask(AIGroup.Cast(GetOwner()), task);
	}

	protected string m_sDCOForm;
	protected string m_sDCOFormPend;
	protected bool m_bDCOFormContact;

	protected int m_iDCOFormShape;
	protected int m_iDCOFormSp;

	string DCO_GetFormation()			{ return m_sDCOForm; }
	int DCO_GetFormShape()				{ return m_iDCOFormShape; }
	int DCO_GetFormSpacing()			{ return m_iDCOFormSp; }

	void DCO_SetFormOverride(int shape, int spacing, string by)
	{
		if (shape == m_iDCOFormShape && spacing == m_iDCOFormSp)
			return;
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Group), string.Format("formation_override grp=%1 shape=%2 spacing=%3 from_shape=%4 from_spacing=%5 by=%6", DCO_Log.IdGroup(m_Group), DCO_Formation.ShapeName(shape), spacing, DCO_Formation.ShapeName(m_iDCOFormShape), m_iDCOFormSp, by));
		m_iDCOFormShape = shape;
		m_iDCOFormSp = spacing;
		DCO_Perf.Count("fm:override_" + by);
		if (m_FormationComponent && m_Group)
		{
			if (shape > 0 || spacing > 0 || DCO_Formation.On())
				DCO_UpdateFormation(true, by);
			else
				DCO_FormationTick();
		}
	}

	protected bool DCO_FormActive()
	{
		return DCO_Formation.On() || m_iDCOFormShape > 0 || m_iDCOFormSp > 0;
	}

	protected string DCO_TaskFormation()
	{
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
		return formation;
	}

	protected float m_fDCOGarArriveAt = -1;
	protected float m_fDCOWpArriveAt = -1;
	protected AIWaypoint m_DCOWpArrive;

	protected void DCO_AttackArriveTick()
	{
		if (!m_Group || DCO_Ctx.TuneFor(GetOwner(), "wp.attack_arrive", 0) <= 0)
			return;
		DCO_EGroupTask task = GetTask();
		AIWaypoint wp = m_Group.GetCurrentWaypoint();
		bool assault = task == DCO_EGroupTask.ATTACK || task == DCO_EGroupTask.FLANK;
		bool anyTask = DCO_Ctx.TuneFor(GetOwner(), "wp.arrive_any", 0) > 0 && task != DCO_EGroupTask.DEFEND && task != DCO_EGroupTask.GARRISON && task != DCO_EGroupTask.TRANSPORT && task != DCO_EGroupTask.FIRE_MISSION;
		if ((!assault && !anyTask) || !wp || wp.Type() != SCR_AIWaypoint)
		{
			m_fDCOWpArriveAt = -1;
			m_DCOWpArrive = null;
			return;
		}
		if (wp != m_DCOWpArrive)
		{
			m_DCOWpArrive = wp;
			m_fDCOWpArriveAt = -1;
		}
		float r = wp.GetCompletionRadius() + 10.0;
		vector c = wp.GetOrigin();
		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);
		int n = 0;
		int inside = 0;
		foreach (AIAgent a : agents)
		{
			IEntity e;
			if (a)
				e = a.GetControlledEntity();
			if (!e)
				continue;
			n++;
			if (vector.DistanceXZ(e.GetOrigin(), c) <= r)
				inside++;
		}
		if (n == 0 || inside * 2 < n)
		{
			m_fDCOWpArriveAt = -1;
			return;
		}
		float now = GetGame().GetWorld().GetWorldTime();
		if (m_fDCOWpArriveAt < 0)
		{
			m_fDCOWpArriveAt = now;
			return;
		}
		float waited = now - m_fDCOWpArriveAt;
		if (waited < DCO_Ctx.TuneFor(GetOwner(), "wp.attack_arrive_s", 8.0) * 1000.0)
			return;
		m_fDCOWpArriveAt = -1;
		m_DCOWpArrive = null;
		DCO_Perf.Count("wp:attack_arrive_complete");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Group), string.Format("wp_arrive_complete grp=%1 task=%2 wp=%3 inside=%4/%5 radius=%6 waited_s=%7", DCO_Log.IdGroup(m_Group), typename.EnumToString(DCO_EGroupTask, task), DCO_Util.XZ(c), inside, n, Math.Round(r), Math.Round(waited / 1000.0)));
		m_Group.CompleteWaypoint(wp);
	}

	void DCO_GarrisonArriveTick()
	{
		DCO_AttackArriveTick();
		if (!m_Group || DCO_Ctx.TuneFor(GetOwner(), "garrison.arrive_start", 1) <= 0)
			return;
		SCR_AIGroupUtilityComponent u = SCR_AIGroupUtilityComponent.Cast(m_Group.FindComponent(SCR_AIGroupUtilityComponent));
		if (!u)
			return;
		DCO_GarrisonWaypointState st = DCO_GarrisonWaypointState.Cast(u.DCO_GetWaypointState());
		DCO_GarrisonWaypoint wp;
		if (st)
			wp = DCO_GarrisonWaypoint.Cast(st.GetWaypoint());
		DCO_AIGarrisonActivity act = u.DCO_GetGarrisonActivity();
		if (!wp || wp.DCO_SetupPending() || (act && act.IsActive()))
		{
			m_fDCOGarArriveAt = -1;
			return;
		}
		float r = wp.GetCompletionRadius() + 10.0;
		vector c = wp.GetOrigin();
		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);
		int n = 0;
		int inside = 0;
		foreach (AIAgent a : agents)
		{
			IEntity e;
			if (a)
				e = a.GetControlledEntity();
			if (!e)
				continue;
			n++;
			if (vector.DistanceXZ(e.GetOrigin(), c) <= r)
				inside++;
		}
		if (n == 0 || inside * 2 < n)
		{
			m_fDCOGarArriveAt = -1;
			return;
		}
		float now = GetGame().GetWorld().GetWorldTime();
		if (m_fDCOGarArriveAt < 0)
		{
			m_fDCOGarArriveAt = now;
			return;
		}
		float waited = now - m_fDCOGarArriveAt;
		if (waited < DCO_Ctx.TuneFor(GetOwner(), "garrison.arrive_s", 8.0) * 1000.0)
			return;
		m_fDCOGarArriveAt = -1;
		DCO_Perf.Count("garrison:arrive_start");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Group), string.Format("garrison_arrive_start grp=%1 inside=%2/%3 radius=%4 waited_s=%5", DCO_Log.IdGroup(m_Group), inside, n, Math.Round(r), Math.Round(waited / 1000.0)));
		st.StartGarrison();
	}

	void DCO_FormationTick()
	{
		if (!m_FormationComponent || !m_Group)
			return;
		if (!DCO_FormActive())
		{
			if (m_sDCOForm != "")
			{
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Group), string.Format("formation_auto_off grp=%1 from=%2 to=%3", DCO_Log.IdGroup(m_Group), m_sDCOForm, DCO_TaskFormation()));
				m_sDCOForm = "";
				m_FormationComponent.SetFormation(DCO_TaskFormation());
			}
			return;
		}
		DCO_UpdateFormation(false, "tick");
	}

	protected void DCO_UpdateFormation(bool now, string by)
	{
		bool contact = IsInContact();
		string why;
		string f = DCO_Formation.Pick(m_Group, DCO_TaskFormation(), m_bRetreating, contact, m_Task.m_eType == DCO_EGroupTask.RECON, m_iDCOFormShape, m_iDCOFormSp, why);
		if (contact != m_bDCOFormContact)
		{
			now = true;
			m_bDCOFormContact = contact;
		}
		if (f == "" || f == m_sDCOForm)
		{
			m_sDCOFormPend = "";
			return;
		}
		if (!now && m_sDCOForm != "" && f != m_sDCOFormPend)
		{
			m_sDCOFormPend = f;
			DCO_Perf.Count("fm:pending");
			return;
		}
		m_sDCOFormPend = "";
		string from = m_sDCOForm;
		if (!DCO_Formation.Apply(m_Group, f))
		{
			DCO_Perf.Count("fm:fail");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.WARN, DCO_Log.IdGroup(m_Group), string.Format("formation_fail grp=%1 name=%2 fallback=%3", DCO_Log.IdGroup(m_Group), f, DCO_TaskFormation()));
			f = DCO_TaskFormation();
			DCO_Formation.Apply(m_Group, f);
		}
		m_sDCOForm = f;
		DCO_Perf.Count("fm:set");
		DCO_Perf.Count("fm:set_" + f);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Group), string.Format("formation_set grp=%1 from=%2 to=%3 by=%4 task=%5 ", DCO_Log.IdGroup(m_Group), from, f, by, typename.EnumToString(DCO_EGroupTask, m_Task.m_eType)) + why);
	}

	protected void ApplyTaskStance()
	{
		if (!m_FormationComponent || !m_UtilityComp)
			return;

		if (DCO_FormActive())
			DCO_UpdateFormation(true, "task");
		else
			m_FormationComponent.SetFormation(DCO_TaskFormation());

		EAIGroupCombatMode mode = EAIGroupCombatMode.RETURN_FIRE;
		if (IsMortar() || (m_bRetreating && !m_DCOBreak))
			mode = EAIGroupCombatMode.HOLD_FIRE;
		else if (m_DCOBreak)
			mode = EAIGroupCombatMode.FIRE_AT_WILL;
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
	bool m_bDCOManStatics;
	int m_iDCOTRPhase = -1;
	bool m_bDCOTRReady;
	bool m_bDCOTRFarEdge;
	bool m_bDCOTRRefixReq;
	bool m_bDCOTRMayBound = true;
	bool m_bDCOTRBoundDone;
	bool m_bDCOTRCloseIn;
	float m_fDCOTRFireAt = -1;
	float m_fDCOTRLastFire = -1;
	bool m_bDCOTRMasked;
	bool m_bDCOTRBackAtAP;
	bool m_bDCOTRStuck;
	bool m_bDCOTRCommitted;
	bool m_bDCOTRForceReady;
	bool m_bDCOTRNoLos;
	bool m_bDCOTRSmokeReq;
	vector m_vDCOTRRelocate;
	vector m_vDCOTRRally;
	protected ref array<int> m_aDCOTRSignals = {};

	void DCO_TRSignal(int sig)
	{
		if (m_aDCOTRSignals.Contains(sig))
			return;
		m_aDCOTRSignals.Insert(sig);
		DCO_Perf.Count("tacrole:signal");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("tr_signal_sent grp=%1 signal=%2", GetOwner().GetName(), typename.EnumToString(DCO_ETRSignal, sig)));
	}

	bool DCO_TRTake(int sig)
	{
		int i = m_aDCOTRSignals.Find(sig);
		if (i < 0)
			return false;
		m_aDCOTRSignals.Remove(i);
		return true;
	}

	void DCO_TRReset()
	{
		m_aDCOTRSignals.Clear();
		m_bDCOTRReady = false;
		m_bDCOTRFarEdge = false;
		m_bDCOTRRefixReq = false;
		m_bDCOTRMayBound = true;
		m_bDCOTRBoundDone = false;
		m_bDCOTRCloseIn = false;
		m_fDCOTRFireAt = -1;
		m_fDCOTRLastFire = -1;
		m_bDCOTRMasked = false;
		m_bDCOTRBackAtAP = false;
		m_bDCOTRStuck = false;
		m_bDCOTRCommitted = false;
		m_bDCOTRForceReady = false;
		m_bDCOTRNoLos = false;
		m_bDCOTRSmokeReq = false;
		m_vDCOTRRelocate = vector.Zero;
		m_vDCOTRRally = vector.Zero;
	}
	void DCO_SetHold(float untilTime)	{ m_fDCOHoldUntil = untilTime; }
	bool DCO_IsHeld()					{ return m_fDCOHoldUntil > 0 && DCO_Util.NowS() < m_fDCOHoldUntil; }
	int DCO_GetPeakStrength()			{ return m_iPeakStrength; }

	bool IsAvailableReserve()
	{
		if (m_bInTransport)
			return false;
		if (m_fDCOHoldUntil > 0 && DCO_Util.NowS() < m_fDCOHoldUntil)
			return false;
		if (m_bRetreating && IsMoving())
			return false;
		if (m_DCOBreak && DCO_Ctx.TuneFor(GetOwner(), "grp.bc_not_free", 1) > 0)
		{
			DCO_Perf.Count("reserve:skip_breaking_contact");
			return false;
		}
		if (m_Task.m_eType == DCO_EGroupTask.NONE)
			return true;
		return m_Task.m_eType == DCO_EGroupTask.PATROL && !IsMoving();
	}

	bool IsRetreating()
	{
		return m_bRetreating;
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
	bool IsInfantry()	{ DCO_EGroupCapability c = GetCapability(); return c == DCO_EGroupCapability.INFANTRY || c == DCO_EGroupCapability.MOTORIZED || c == DCO_EGroupCapability.MECHANIZED; }
	bool HasAT()		{ RefreshCapability(); return (m_iUnitRoles & EUnitRole.AT_SPECIALIST) != 0; }
	bool HasMG()		{ RefreshCapability(); return (m_iUnitRoles & EUnitRole.MACHINEGUNNER) != 0; }
	bool HasMedic()		{ RefreshCapability(); return (m_iUnitRoles & EUnitRole.MEDIC) != 0; }
	bool IsMedicTeam()	{ RefreshCapability(); return m_bMedicTeam; }
	protected bool m_bMedicTeam;
	protected bool m_bMedicLogged;
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
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("auto_detect_stop grp=%1 reason=%2", GetOwner().GetName(), stop));
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
		else if (m_eGroupRoleExternal == DCO_EGroupPreset.MOTORIZED)
			m_eCapability = DCO_EGroupCapability.MOTORIZED;
		else if (m_eGroupRoleExternal == DCO_EGroupPreset.MECHANIZED)
			m_eCapability = DCO_EGroupCapability.MECHANIZED;
		else if (GetGroupVehicle())
			m_eCapability = DCO_MountedOps.Classify(m_Group, GetGroupVehicle());
		else
			m_eCapability = DCO_EGroupCapability.INFANTRY;

		m_iUnitRoles = 0;
		m_bHasRadio = false;
		if (!m_Group)
			return;

		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);
		m_iPeakStrength = Math.Max(m_iPeakStrength, agents.Count());
		int livingCount = 0;
		int medicCount = 0;
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (ca && ca.m_InfoComponent && !ca.m_InfoComponent.HasUnitState(EUnitState.UNCONSCIOUS))
			{
				SCR_AIInfoComponent info = ca.m_InfoComponent;
				livingCount++;
				if (info.HasRole(EUnitRole.AT_SPECIALIST))
					m_iUnitRoles |= EUnitRole.AT_SPECIALIST;
				if (info.HasRole(EUnitRole.MACHINEGUNNER))
					m_iUnitRoles |= EUnitRole.MACHINEGUNNER;
				if (info.HasRole(EUnitRole.MEDIC) && DCO_Util.IsRealMedic(a.GetControlledEntity()))
				{
					m_iUnitRoles |= EUnitRole.MEDIC;
					medicCount++;
				}
			}

			IEntity member = a.GetControlledEntity();
			if (!m_bHasRadio && member)
			{
				m_bHasRadio = DCO_Util.HasManpackRadio(member);
			}
		}
		bool prevMedicTeam = m_bMedicTeam;
		m_bMedicTeam = livingCount > 0 && medicCount * 2 >= livingCount && (m_iUnitRoles & (EUnitRole.MACHINEGUNNER | EUnitRole.AT_SPECIALIST)) == 0 && m_eCapability == DCO_EGroupCapability.INFANTRY && !GetOwnedVehicle() && !GetGroupVehicle() && !m_bIsDedicatedTransport;
		bool vehicleCrew = m_eCapability != DCO_EGroupCapability.INFANTRY || GetOwnedVehicle() != null || GetGroupVehicle() != null || m_bIsDedicatedTransport;
		if (prevMedicTeam != m_bMedicTeam || (medicCount > 0 && !m_bMedicLogged))
		{
			m_bMedicLogged = true;
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("capability_medic grp=%1 medics=%2 units=%3 vehicle_crew=%4 result=%5", GetOwner().GetName(), medicCount, livingCount, vehicleCrew, m_bMedicTeam));
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
		if ((m_iPeakStrength > 0 && units * 10 < m_iPeakStrength * 4) || DCO_Util.NowS() < m_fDCOIneffUntil_s)
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

			if (!DCO_Util.IsAlive(controlled))
				continue;

			IEntity veh = DCO_VehicleCombat.GetVehicle(controlled);
			if (veh && veh != leaderVeh)
				continue;

			if (m_UtilityComp && m_UtilityComp.DCO_IsStuckStraggler(agent))
				continue;

			if (vector.Distance(controlled.GetOrigin(), leaderPos) > SQUAD_SPREAD_THRESHOLD())
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
		m_fOrderTimeout   = (initialDist / AVG_MOVE_SPEED_MPS()) + ORDER_BASE_BUFFER();
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

		if (distToTarget <= ARRIVAL_THRESHOLD())
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

			if (distMoved > 2.5)
			{
				m_vLastCheckPos = currentPos;
				m_fLastMoveTime = worldTime;
			}
			else
			{
				float stuckElapsed = worldTime - m_fLastMoveTime;
				if (stuckElapsed >= 15.0)
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

	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(DCO_FormationTick);
		super.OnDelete(owner);
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
				obj.GetDisplayName(),
				DCO_DebugDraw.M(vector.Distance(g, obj.GetOwner().GetOrigin())));
		else
			taskLine = "task: none";

		string cmdLine;
		if (myCommander)
			cmdLine = "commander: " + myCommander.GetCommanderUID();
		else
			cmdLine = "commander: none";

		int   units       = GetUnitCount();
		float strengthPct = Math.Clamp(1.0 * units / 12.0 * 100.0, 0.0, 100.0);

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
		if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), myCommander.GetCommanderUID() + " < MY COMMANDER | MY GROUP > " + GetTaskLabel());
		threatComp = myCommander.GetThreatResponseComponent();
		DCO_ApplyLOD();
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
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("transport_team_set grp=%1 dedicated=%2 by=GM", GetOwner().GetName(), tf));
	}

	bool IsAutoTransport()			{ return m_bAutoTransport; }

	void DemoteAutoTransport(string reason)
	{
		m_bAutoTransport = false;
		m_bAutoDetectDone = true;
		m_bIsDedicatedTransport = false;
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("transport_team_demoted grp=%1 reason=%2", GetOwner().GetName(), reason));
		string transportTail = ".";
		if (reason == "vehicle_lost")
			transportTail = ": its vehicle was lost.";
		DCO_PlayerComms.SendToGameMasters("DCO", DCO_PlayerComms.GetCallsign(m_Group) + " is no longer a transport team" + transportTail);
	}

	bool IsPlayerGroup()
	{
		SCR_AIGroup grp = SCR_AIGroup.Cast(GetOwner());
		if (!grp)
			return false;

		IsPlayerGroup = DCO_Util.IsPlayerGroup(grp);
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

		if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG))
		{
			vector firstPos = vector.Zero;
			foreach (SCR_AIWaypoint fw : wps)
			{
				if (fw)
				{
					firstPos = fw.GetOrigin();
					break;
				}
			}
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, "-", string.Format("grp_route grp=%1 n=%2 len=%3 first=%4 last=%5", DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), added, Math.Round(routeLength), DCO_Util.XZ(firstPos), DCO_Util.XZ(final)));
		}

		DCO_BoardOwned(final);
		SetPhase(DCO_ETaskPhase.MOVING);

		m_vOrderTarget    = final;
		m_fOrderStartTime = worldTime;
		m_bOrderActive    = true;
		m_fOrderTimeout   = (routeLength / AVG_MOVE_SPEED_MPS()) + ORDER_BASE_BUFFER();
		m_vLastCheckPos   = vector.Zero;
		m_fLastMoveTime   = 0.0;
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		DCO_MigratePatrol();
		SCR_AIGroup featGrp = SCR_AIGroup.Cast(owner);
		if (featGrp && Replication.IsServer())
		{
			GetGame().GetCallqueue().CallLater(DCO_FormationTick, 5000 + Math.RandomInt(0, 1000), true);
			GetGame().GetCallqueue().CallLater(DCO_GarrisonArriveTick, 3000 + Math.RandomInt(0, 1000), true);
		}
		if (featGrp && Replication.IsServer())
		{
			int off = m_eDCOFeaturesOff;
			if (m_eDCODetail != DCO_EGroupDetail.CUSTOM)
				off = DCO_GroupFeatures.PresetOff(m_eDCODetail);
			if (off != 0)
				featGrp.DCO_SetFeatureOff(off, "prefab");
		}
		if (!AICommander_ManagerComponent.GetInstance() && !m_bAutoAssign)
		{
			DCO_ScheduleUsableVehicles(owner);
			return;
		}

		if (!m_bIsProcessedByCommander && !m_bCanHaveCommander)
		{
			DCO_ScheduleUsableVehicles(owner);
			return;
		}

		SCR_AIGroup grp = SCR_AIGroup.Cast(owner);

		contactReportComponent = DCO_GroupContactReporterComponent.Cast(owner.FindComponent(DCO_GroupContactReporterComponent));
		GetGame().GetCallqueue().CallLater(delayedInit, 5000, false, owner);
	}

	protected float m_fDCOConfusedUntil_ms;
	protected float m_fDCORadioDelayUntil_ms;

	void DCO_SetConfused(float untilMs)		{ m_fDCOConfusedUntil_ms = untilMs; }
	void DCO_SetRadioDelayed(float untilMs)	{ m_fDCORadioDelayUntil_ms = untilMs; }
	bool IsConfused()						{ return GetGame().GetWorld().GetWorldTime() < m_fDCOConfusedUntil_ms; }
	bool IsRadioDelayed()					{ return GetGame().GetWorld().GetWorldTime() < m_fDCORadioDelayUntil_ms; }

	protected float m_fDCOBcSkipLogAt = -1000;

	bool CanItHaveOrder()
	{
		if (m_DCOBreak && DCO_Ctx.TuneFor(GetOwner(), "bc.no_retask", 0) > 0)
		{
			float now = DCO_Util.NowS();
			if (now - m_fDCOBcSkipLogAt >= 30.0)
			{
				m_fDCOBcSkipLogAt = now;
				DCO_Perf.Count("tactic_pick_skip:break_contact");
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("tactic_pick_skip grp=%1 reason=break_contact task=%2", GetOwner().GetName(), typename.EnumToString(DCO_EGroupTask, m_Task.m_eType)));
			}
			return false;
		}
		return m_bIsProcessedByCommander && !IsHeldByGM() && !IsIdleIgnoringCommander() && !IsConfused();
	}

	protected bool IsIdleIgnoringCommander()
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(GetOwner());
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

	protected bool m_bDCOUsableRegistered;

	protected void DCO_ScheduleUsableVehicles(IEntity owner)
	{
		if (!m_sUsableVehicle || m_sUsableVehicle.IsEmpty() || !Replication.IsServer())
			return;
		GetGame().GetCallqueue().CallLater(DCO_RegisterUsableVehiclesLater, 5000, false, owner);
	}

	protected void DCO_RegisterUsableVehiclesLater(IEntity owner)
	{
		DCO_RegisterUsableVehicles(owner, "no_commander");
	}

	protected void DCO_RegisterUsableVehicles(IEntity owner, string via)
	{
		if (m_bDCOUsableRegistered || !m_sUsableVehicle || !m_UtilityComp)
			return;
		m_bDCOUsableRegistered = true;
		int n;
		int missing;
		foreach (string s : m_sUsableVehicle)
		{
			IEntity e = GetGame().GetWorld().FindEntityByName(s);
			SCR_AIVehicleUsageComponent aiveh = FindVehicleUsage(e, 3);
			if (!aiveh)
			{
				missing++;
				continue;
			}
			m_UtilityComp.AddUsableVehicle(aiveh);
			n++;
			if (!m_OwnedVehicle && !IsMortar() && Vehicle.Cast(aiveh.GetOwner()))
				m_OwnedVehicle = aiveh.GetOwner();
		}
		if (m_sUsableVehicle.IsEmpty())
			return;
		DCO_Perf.Count("usable_vehicle:" + via);
		AIGroup g = AIGroup.Cast(owner);
		DCO_ELogLevel lvl = DCO_ELogLevel.INFO;
		if (missing > 0)
			lvl = DCO_ELogLevel.WARN;
		DCO_Log.Write(DCO_ELogChannel.VEH, lvl, DCO_Log.IdGroup(g), string.Format("usable_vehicle_registered grp=%1 n=%2 missing=%3 via=%4", DCO_Log.IdGroup(g), n, missing, via));
	}

	protected void delayedInit(IEntity owner)
	{
		SCR_AIGroup grp = SCR_AIGroup.Cast(owner);
		Faction fc = grp.GetFaction();
		if (fc)
			fk = grp.GetFaction().GetFactionKey();

		DCO_RegisterUsableVehicles(owner, "commander");
		if (m_OwnedVehicle && Replication.IsServer())
			DCO_BoardOwned(vector.Zero);

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
	protected static ref map<IEntity, DCO_GroupUtilityComponent> s_mMortarCrew;
	protected static map<IEntity, DCO_GroupUtilityComponent> GetMortarCrew()
	{
		if (!s_mMortarCrew)
			s_mMortarCrew = new map<IEntity, DCO_GroupUtilityComponent>();
		return s_mMortarCrew;
	}
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
			GetMortarCrew().Remove(m_AutoMortarUsage.GetOwner());
		m_AutoMortarUsage = null;
		m_bAutoMortar = false;
		m_bAutoDetectDone = true;
		m_eGroupRoleExternal = DCO_EGroupPreset.AUTO;
		GetGame().GetCallqueue().Remove(CheckGroupIsHaveOrder);
		if (myCommander)
			myCommander.DemoteFromArtillery(this, reason);
		string mortarTail = ".";
		if (reason == "crew_lost")
			mortarTail = ": its crew was lost.";
		else if (reason == "weapon_lost")
			mortarTail = ": its mortar was lost.";
		DCO_PlayerComms.SendToGameMasters("DCO", DCO_PlayerComms.GetCallsign(m_Group) + " is no longer a mortar team" + mortarTail);
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
				GetMortarCrew().Set(v.GetEntity(), this);
				bool wasMortar = m_eGroupRoleExternal == DCO_EGroupPreset.MORTAR;
				if (!wasMortar)
				{
					m_bAutoMortar = true;
					m_AutoMortarUsage = usage;
				}
				m_eGroupRoleExternal = DCO_EGroupPreset.MORTAR;
				m_fCapabilityCheck_ms = -1;
				if (myCommander && (!wasMortar || !myCommander.DCO_IsArtyRegistered(this)))
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

		GetMortarCrew().Set(m_FoundMortar.GetOwner(), this);
		m_UtilityComp.AddUsableVehicle(m_FoundMortar);
		if (m_eGroupRoleExternal == DCO_EGroupPreset.AUTO)
		{
			m_bAutoMortar = true;
			m_AutoMortarUsage = m_FoundMortar;
		}
		m_eGroupRoleExternal = DCO_EGroupPreset.MORTAR;
		m_fCapabilityCheck_ms = -1;
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), string.Format("%1 -> automatic mortar team (%2)", GetOwner().GetName(), m_FoundMortar.GetOwner()));
		if (myCommander)
		{
			myCommander.PromoteToArtillery(this);
			GetGame().GetCallqueue().Remove(CheckGroupIsHaveOrder);
			GetGame().GetCallqueue().CallLater(CheckGroupIsHaveOrder, 10000, true);
		}
		DCO_PlayerComms.SendToGameMasters("DCO", DCO_PlayerComms.GetCallsign(SCR_AIGroup.Cast(GetOwner())) + " is now a mortar team.");
	}


	protected bool m_bDCOTransportSkipLogged;

	protected void DetectTransport()
	{
		if (m_bIsDedicatedTransport || !m_Group || !myCommander || m_OwnedVehicle || IsPlayerGroup() || !IsAvailableReserve())
			return;
		int jobs = GetAllowedJobs();
		if ((jobs & DCO_EGroupJob.TRANSPORT) == 0)
		{
			if (!m_bDCOTransportSkipLogged)
			{
				m_bDCOTransportSkipLogged = true;
				DCO_Perf.Count("grpjob:auto_transport_skip");
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Group), string.Format("auto_transport_skip grp=%1 reason=transport_not_allowed mask=%2", DCO_Log.IdGroup(m_Group), jobs));
			}
			return;
		}
		bool transportOnly = (jobs & DCO_GroupJobs.AnyCombat()) == 0;
		if (m_eGroupRoleExternal != DCO_EGroupPreset.AUTO || (AutoDetectSize() > 3 && !transportOnly))
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
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), string.Format("%1 -> automatic transport team (%2)", GetOwner().GetName(), veh));
		DCO_PlayerComms.SendToGameMasters("DCO", DCO_PlayerComms.GetCallsign(m_Group) + " is now a transport team.");
	}

	protected bool MortarQueryCallback(IEntity e)
	{
		SCR_AIVehicleUsageComponent usage = SCR_AIVehicleUsageComponent.Cast(e.FindComponent(SCR_AIVehicleUsageComponent));
		if (!usage || usage.GetVehicleType() != EAIVehicleType.STATIC_ARTILLERY || usage.GetDamageState() == EDamageState.DESTROYED)
			return true;

		DCO_GroupUtilityComponent owner = GetMortarCrew().Get(e);
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
		if (m_iAutoAssignTries < 12)
			GetGame().GetCallqueue().CallLater(TryAutoAssign, 5000, false);
		else
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.WARN, DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), string.Format("Auto-assign %1 failed: no commander '%2' for faction %3", GetOwner().GetName(), m_sDedicatedCo, fk));
	}

	DCO_AIGarrisonActivity GetGarrisonActivity()
	{
		if (!m_UtilityComp)
			return null;
		return m_UtilityComp.DCO_GetGarrisonActivity();
	}

	protected const float CONTACT_FRESH_S = 15.0;

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

			if (c.m_State.m_fDistMin < 30.0)
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
		if (IsMortar() && myCommander && !myCommander.DCO_IsArtyRegistered(this))
			myCommander.PromoteToArtillery(this);

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
		DCO_ApplyLOD();
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
