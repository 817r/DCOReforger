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
	float m_fSOPShotAt = -1000;
	float m_fSOPFreeLogAt = -1000;
	float m_fSOPDetourAt = -1000;
	float m_fSOPObserveSince = -1;
	float m_fSOPObserveSeen = -1000;
	float m_fSOPEngageAt = -1000;

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

		DCO_EAILODMode mode = DCO_LodResolve(EffectiveLOD(), "apply");
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
		DCO_EAILODMode mode = DCO_LodResolve(EffectiveLOD(), "agent");
		DCO_GlobalAIComponent.ApplyLOD(agent, mode);
		PublishLOD(SCR_AIGroup.Cast(GetOwner()), mode);
	}

	protected float m_fDCOLodAt_s = -100;
	protected float m_fDCOLodWakeAt_s = -1000;
	protected float m_fDCOLodEventAt_s = -1000;
	protected string m_sDCOLodEvent;
	protected bool m_bDCOLodAwake = true;
	protected bool m_bDCOLodDecided;

	protected float DCO_LodTune(string key, float def)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(GetOwner());
		if (!cfg)
			return def;
		return cfg.DCO_TuneF(key, def);
	}

	protected bool DCO_LodTaskBusy()
	{
		if (m_Task.m_ePhase != DCO_ETaskPhase.HOLDING)
			return true;
		DCO_EGroupTask t = m_Task.m_eType;
		if (t == DCO_EGroupTask.PATROL || t == DCO_EGroupTask.RECON || t == DCO_EGroupTask.TRANSPORT)
			return true;
		return t == DCO_EGroupTask.REINFORCE || t == DCO_EGroupTask.FIRE_MISSION;
	}

	protected bool DCO_LodEnemyNear(float dist)
	{
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!pm || !m_UtilityComp || !m_UtilityComp.GetPercGroupComp())
			return false;
		float pmNow = pm.GetTime();
		vector pos = GetOwner().GetOrigin();
		foreach (SCR_AITargetInfo t : m_UtilityComp.GetPercGroupComp().m_aTargets)
		{
			if (t && pmNow - t.m_fTimestamp <= 30 && vector.DistanceXZ(t.m_vWorldPos, pos) < dist)
				return true;
		}
		return false;
	}

	protected bool DCO_LodWantAwake(float now, out string why)
	{
		if (now - m_fDCOLodEventAt_s < DCO_LodTune("lod.smart_hold_s", 60.0))
		{
			why = "event_" + m_sDCOLodEvent;
			return true;
		}
		if (DCO_LodTaskBusy())
		{
			why = "task_" + typename.EnumToString(DCO_EGroupTask, m_Task.m_eType) + "_" + typename.EnumToString(DCO_ETaskPhase, m_Task.m_ePhase);
			return true;
		}
		if (IsInContact())
		{
			why = "contact";
			return true;
		}
		if (DCO_LodEnemyNear(DCO_LodTune("lod.smart_enemy_m", 800.0)))
		{
			why = "enemy_near";
			return true;
		}
		if (DCO_PlayerProx.Within(GetOwner().GetOrigin(), DCO_LodTune("lod.smart_player_m", 1000.0)))
		{
			why = "player_near";
			return true;
		}
		why = "idle";
		return false;
	}

	protected DCO_EAILODMode DCO_LodResolve(DCO_EAILODMode mode, string src)
	{
		if (mode != DCO_EAILODMode.SMART)
			return mode;
		float now = DCO_Util.NowS();
		string why;
		bool want = DCO_LodWantAwake(now, why);
		if (!want && m_bDCOLodAwake && now - m_fDCOLodWakeAt_s < DCO_LodTune("lod.smart_hold_s", 60.0))
		{
			want = true;
			why = "hold";
		}
		if (want && (!m_bDCOLodAwake || !m_bDCOLodDecided))
			m_fDCOLodWakeAt_s = now;
		if (want != m_bDCOLodAwake || !m_bDCOLodDecided)
		{
			string state = "sleep";
			if (want)
				state = "wake";
			DCO_Perf.Count("lod_smart:" + state);
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("lod_smart grp=%1 state=%2 why=%3 src=%4 units=%5", GetOwner().GetName(), state, why, src, GetUnitCount()));
		}
		m_bDCOLodAwake = want;
		m_bDCOLodDecided = true;
		if (want)
			return DCO_EAILODMode.PREVENT_MAX_LOD;
		return DCO_EAILODMode.VANILLA;
	}

	void DCO_LodTick()
	{
		if (EffectiveLOD() != DCO_EAILODMode.SMART)
			return;
		float now = DCO_Util.NowS();
		if (now - m_fDCOLodAt_s < 5.0)
			return;
		m_fDCOLodAt_s = now;
		DCO_ApplyLOD();
	}

	void DCO_LodWake(string why)
	{
		if (EffectiveLOD() != DCO_EAILODMode.SMART || !Replication.IsServer())
			return;
		m_fDCOLodEventAt_s = DCO_Util.NowS();
		m_sDCOLodEvent = why;
		if (!m_bDCOLodAwake)
			DCO_ApplyLOD();
	}

	static void DCO_LodWakeFor(IEntity character, string why)
	{
		if (!character)
			return;
		AIControlComponent ctrl = AIControlComponent.Cast(character.FindComponent(AIControlComponent));
		if (!ctrl || !ctrl.GetAIAgent() || !ctrl.GetAIAgent().GetParentGroup())
			return;
		DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(ctrl.GetAIAgent().GetParentGroup().FindComponent(DCO_GroupUtilityComponent));
		if (g)
			g.DCO_LodWake(why);
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
	protected float m_fDCOCruiseGM;
	protected float m_fDCOCruiseWanted;
	protected float m_fDCOCruiseApplied = -1;
	protected IEntity m_DCOCruiseVeh;
	protected float m_fDCOCruiseAt_s;

	float GetCruiseGM() { return m_fDCOCruiseGM; }

	void SetCruiseGM(float kmh)
	{
		m_fDCOCruiseGM = kmh;
		DCO_ApplyCruise("gm");
	}

	float DCO_ResolveCruise(out string src)
	{
		float eff = m_fDCOCruiseWanted;
		src = "patrol";
		float fac = DCO_Ctx.TuneFor(GetOwner(), "veh.cruise_kmh", 0.0);
		if (fac > 0)
		{
			eff = fac;
			src = "faction";
		}
		if (m_fDCOCruiseGM > 0)
		{
			eff = m_fDCOCruiseGM;
			src = "squad";
		}
		bool convoyLower = eff <= 0 || m_fDCOCruiseConvoy < eff;
		if (m_fDCOCruiseConvoy > 0 && convoyLower)
		{
			eff = m_fDCOCruiseConvoy;
			src = "convoy";
		}
		if (eff <= 0)
			src = "vanilla";
		return eff;
	}

	protected float m_fDCOCruiseConvoy;

	void DCO_SetConvoyCruise(float kmh)
	{
		if (kmh == m_fDCOCruiseConvoy)
			return;
		m_fDCOCruiseConvoy = kmh;
		DCO_ApplyCruise("convoy");
	}

	void DCO_ConvoyCruiseExtra(IEntity veh, float kmh)
	{
		if (!veh)
			return;
		AICarMovementComponent car = AICarMovementComponent.Cast(veh.FindComponent(AICarMovementComponent));
		if (!car)
			return;
		float keep = m_fDCOCruiseConvoy;
		m_fDCOCruiseConvoy = kmh;
		string src;
		float eff = DCO_ResolveCruise(src);
		m_fDCOCruiseConvoy = keep;
		if (eff > 0)
			car.SetCruiseSpeed(eff);
		else
			car.ResetCruiseSpeed();
		DCO_Perf.Count("veh:cruise_convoy_extra");
	}

	void DCO_ApplyCruise(string by)
	{
		IEntity veh = GetGroupVehicle();
		if (!veh)
		{
			m_DCOCruiseVeh = null;
			m_fDCOCruiseApplied = -1;
			return;
		}
		AICarMovementComponent car = AICarMovementComponent.Cast(veh.FindComponent(AICarMovementComponent));
		if (!car)
			return;
		string src;
		float eff = DCO_ResolveCruise(src);
		if (veh == m_DCOCruiseVeh && eff == m_fDCOCruiseApplied)
			return;
		if (eff > 0)
			car.SetCruiseSpeed(eff);
		else
			car.ResetCruiseSpeed();
		m_bCruiseLimited = eff > 0;
		m_DCOCruiseVeh = veh;
		m_fDCOCruiseApplied = eff;
		if (by == "convoy")
		{
			DCO_Perf.Count("veh:cruise_convoy");
			return;
		}
		DCO_Perf.Count("veh:cruise_set");
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Group), string.Format("cruise_set grp=%1 kmh=%2 src=%3 by=%4 veh=%5", DCO_Log.IdGroup(m_Group), Math.Round(eff), src, by, DCO_Util.PrefabShort(veh)));
	}

	protected int m_iDCOSimMode;
	protected float m_fDCOSimAt_s;
	protected int m_iDCOReserveJob;
	protected float m_fDCOReserveJobAt_s = -1;

	int DCO_ReserveJob() { return m_iDCOReserveJob; }
	float DCO_ReserveJobAt() { return m_fDCOReserveJobAt_s; }

	void DCO_SetReserveJob(int job, float at_s)
	{
		m_iDCOReserveJob = job;
		m_fDCOReserveJobAt_s = at_s;
	}

	int GetSimMode() { return m_iDCOSimMode; }
	bool IsSimOff() { return m_iDCOSimMode > 0; }

	static bool SimOffFor(IEntity e)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(e);
		if (!ch)
			return false;
		AIControlComponent ctl = ch.GetAIControlComponent();
		if (!ctl || !ctl.GetAIAgent() || !ctl.GetAIAgent().GetParentGroup())
			return false;
		DCO_GroupUtilityComponent gu = DCO_GroupUtilityComponent.Cast(ctl.GetAIAgent().GetParentGroup().FindComponent(DCO_GroupUtilityComponent));
		return gu && gu.IsSimOff();
	}

	static string SimModeName(int mode)
	{
		if (mode == 2)
			return "hidden";
		if (mode == 1)
			return "off";
		return "auto";
	}

	void SetSimMode(int mode)
	{
		if (mode < 0 || mode > 2 || mode == m_iDCOSimMode)
			return;
		m_iDCOSimMode = mode;
		DCO_ApplySim("gm");
	}

	protected void DCO_StreamTo(IEntity e, bool enable)
	{
		RplComponent rpl = RplComponent.Cast(e.FindComponent(RplComponent));
		if (!rpl)
			return;
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;
		array<int> ids = {};
		pm.GetPlayers(ids);
		foreach (int id : ids)
		{
			PlayerController pc = pm.GetPlayerController(id);
			if (pc)
				rpl.EnableStreamingConNode(pc.GetRplIdentity(), enable);
		}
	}

	void DCO_ApplySim(string by)
	{
		if (!m_Group || !Replication.IsServer())
			return;
		bool off = m_iDCOSimMode > 0;
		bool hide = m_iDCOSimMode == 2;
		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);
		int n;
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			IEntity e = a.GetControlledEntity();
			if (!ca || !e)
				continue;
			AIControlComponent ctl = ca.GetControlComponent();
			if (ctl && off)
				ctl.DeactivateAI();
			else if (ctl)
				ctl.ActivateAI();
			if (hide)
				e.ClearFlags(EntityFlags.VISIBLE | EntityFlags.TRACEABLE, true);
			else
				e.SetFlags(EntityFlags.VISIBLE | EntityFlags.TRACEABLE, true);
			DCO_StreamTo(e, !hide);
			n++;
		}
		if (by == "tick")
			return;
		DCO_Perf.Count("ai_sim:" + SimModeName(m_iDCOSimMode));
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Group), string.Format("ai_sim grp=%1 mode=%2 by=%3 members=%4", DCO_Log.IdGroup(m_Group), SimModeName(m_iDCOSimMode), by, n));
	}

	void DCO_SimTick()
	{
		if (m_iDCOSimMode == 0)
			return;
		float now = DCO_Util.NowS();
		if (now - m_fDCOSimAt_s < 10.0)
			return;
		m_fDCOSimAt_s = now;
		DCO_ApplySim("tick");
	}

	protected SCR_AIWaypoint m_DCOMarkedDestWp;
	protected SCR_AIWaypoint m_DCOWpWatch;
	protected float m_fDCOWpLeaderInAt_s = -1;
	protected float m_fDCOWpAt_s;

	void DCO_MarkDestWp(SCR_AIWaypoint wp)
	{
		m_DCOMarkedDestWp = wp;
	}

	void DCO_WpTick()
	{
		float now = DCO_Util.NowS();
		if (now - m_fDCOWpAt_s < 5.0 || !m_Group)
			return;
		m_fDCOWpAt_s = now;
		SCR_AIWaypoint wp = SCR_AIWaypoint.Cast(m_Group.GetCurrentWaypoint());
		IEntity leader = m_Group.GetLeaderEntity();
		if (!wp || !leader || wp != m_DCOMarkedDestWp)
		{
			m_DCOWpWatch = null;
			m_fDCOWpLeaderInAt_s = -1;
			return;
		}
		float d = vector.DistanceXZ(leader.GetOrigin(), wp.GetOrigin());
		if (wp != m_DCOWpWatch || d > wp.GetCompletionRadius())
		{
			m_DCOWpWatch = wp;
			m_fDCOWpLeaderInAt_s = -1;
			if (d <= wp.GetCompletionRadius())
				m_fDCOWpLeaderInAt_s = now;
			return;
		}
		if (m_fDCOWpLeaderInAt_s < 0)
		{
			m_fDCOWpLeaderInAt_s = now;
			return;
		}
		if (now - m_fDCOWpLeaderInAt_s < 20.0)
			return;
		DCO_Perf.Count("wp:force_complete");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Group), string.Format("wp_force_complete grp=%1 leader_d=%2 radius=%3 wait_s=%4 why=stragglers", DCO_Log.IdGroup(m_Group), Math.Round(d), Math.Round(wp.GetCompletionRadius()), Math.Round(now - m_fDCOWpLeaderInAt_s)));
		m_Group.CompleteWaypoint(wp);
		m_DCOWpWatch = null;
		m_DCOMarkedDestWp = null;
		m_fDCOWpLeaderInAt_s = -1;
	}

	void DCO_CruiseTick()
	{
		float now = DCO_Util.NowS();
		if (now - m_fDCOCruiseAt_s < 10.0)
			return;
		m_fDCOCruiseAt_s = now;
		DCO_ApplyCruise("tick");
	}

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
		m_fDCOCruiseWanted = speedKmH;
		DCO_ApplyCruise("order");
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

	protected IEntity m_DCOSpawnVeh;

	void DCO_AddSpawnVehicle(IEntity vehicle, bool mount = false)
	{
		SCR_AIVehicleUsageComponent usage = FindVehicleUsage(vehicle, 3);
		if (usage && m_UtilityComp)
			m_UtilityComp.AddUsableVehicle(usage);
		if (!mount || !vehicle)
			return;
		m_OwnedVehicle = vehicle;
		m_DCOSpawnVeh = vehicle;
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

	bool DCO_NearOrderTarget(float within)
	{
		if (!m_bOrderActive)
			return true;
		return vector.DistanceXZ(GetOwner().GetOrigin(), m_vOrderTarget) <= within;
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
		return DCO_Dormant.Count(m_Group);
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
					DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, DCO_Log.IdGroup(m_Group), string.Format("veh_board_owned_skip grp=%1 reason=has_boarding_wp wp=%2", m_Group.GetName(), w.ClassName()));
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
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Group), string.Format("veh_board_owned grp=%1 veh=%2", m_Group.GetName(), m_OwnedVehicle));
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
		if (task != m_Task.m_eType)
			DCO_LodWake("task");
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
		DCO_Dev.OnTask(AIGroup.Cast(GetOwner()), task);
	}

	protected string m_sDCOForm;
	protected string m_sDCOFormPend;
	protected bool m_bDCOFormContact;
	protected float m_fDCOFormAt_s = -1;

	float DCO_FormAge()
	{
		if (m_fDCOFormAt_s < 0)
			return -1;
		return DCO_Util.NowS() - m_fDCOFormAt_s;
	}

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
		if (DCO_CivMode.CivGroup(m_Group))
			return;
		bool contact = IsInContact();
		string why;
		string f = DCO_Formation.Pick(m_Group, DCO_TaskFormation(), m_bRetreating, contact, m_Task.m_eType == DCO_EGroupTask.RECON, m_iDCOFormShape, m_iDCOFormSp, why);
		if (contact != m_bDCOFormContact)
		{
			if (contact)
				now = true;
			else
				DCO_Perf.Count("fm:contact_out_wait");
			m_bDCOFormContact = contact;
		}
		if (f == "" || f == m_sDCOForm)
		{
			m_sDCOFormPend = "";
			return;
		}
		float held = DCO_FormAge();
		if (!now && m_sDCOForm != "" && held >= 0 && held < DCO_Ctx.TuneFor(GetOwner(), "fm.hold_s", 20.0))
		{
			DCO_Perf.Count("fm:hold");
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
		m_fDCOFormAt_s = DCO_Util.NowS();
		DCO_Perf.Count("fm:set");
		DCO_Perf.Count("fm:set_" + f);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Group), string.Format("formation_set grp=%1 from=%2 to=%3 by=%4 task=%5 ", DCO_Log.IdGroup(m_Group), from, f, by, typename.EnumToString(DCO_EGroupTask, m_Task.m_eType)) + why + " held_s=" + held.ToString(-1, 0));
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
		if (m_bRetreating && !m_DCOBreak)
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
		if (phase != m_Task.m_ePhase && phase != DCO_ETaskPhase.HOLDING)
			DCO_LodWake("phase");
		m_Task.m_ePhase = phase;
		m_fStateCheck_ms = -1;
	}

	protected float m_fDCOHoldUntil;
	float m_fDCOArtyReadyAt;
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

	bool m_bDCOLowAmmo;
	protected float m_fDCOLowAmmoAt_s = -1000;

	void DCO_LowAmmoTick()
	{
		float now = DCO_Util.NowS();
		if (now - m_fDCOLowAmmoAt_s < 10.0 || !m_Group)
			return;
		m_fDCOLowAmmoAt_s = now;
		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);
		int total;
		int low;
		foreach (AIAgent a : agents)
		{
			if (!a)
				continue;
			IEntity ent = a.GetControlledEntity();
			if (!ent || !DCO_Util.IsAlive(ent))
				continue;
			SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(ent.FindComponent(SCR_AICombatComponent));
			typename well;
			int spare = DCO_Rearm.SpareMags(combat, ent, well);
			if (spare < 0)
				continue;
			total++;
			if (spare <= 1)
				low++;
		}
		bool lowNow = total > 0 && low * 2 >= total;
		if (lowNow == m_bDCOLowAmmo)
			return;
		m_bDCOLowAmmo = lowNow;
		if (lowNow)
			DCO_Perf.Count("grp:low_ammo_on");
		else
			DCO_Perf.Count("grp:low_ammo_off");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Group), string.Format("grp_low_ammo grp=%1 low=%2 total=%3 state=%4", DCO_Log.IdGroup(m_Group), low, total, DCO_Util.B01(lowNow)));
	}

	void DCO_ResetPeak(string why)
	{
		int from = m_iPeakStrength;
		int to = GetUnitCount();
		if (to >= from)
			return;
		m_iPeakStrength = to;
		DCO_Perf.Count("grp:peak_reset");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("peak_reset grp=%1 from=%2 to=%3 why=%4", GetOwner().GetName(), from, to, why));
	}

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

		if (myCommander && (m_iDCOWantedRole == 1 || m_iDCOWantedRole == 2) && now - m_fDCORoleTick_ms >= 15000.0)
		{
			m_fDCORoleTick_ms = now;
			DCO_RoleTick();
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

	protected float m_fDCOContactAt_ms = -1;
	protected bool m_bDCOFirstShot;

	protected void DCO_FirstShotCheck(float now)
	{
		if (m_fDCOContactAt_ms < 0)
		{
			m_fDCOContactAt_ms = now;
			return;
		}
		if (m_bDCOFirstShot || !m_Group)
			return;
		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			if (!a || !a.GetControlledEntity() || DCO_ShotCounter.LastShotAt(a.GetControlledEntity()) < m_fDCOContactAt_ms - 2000.0)
				continue;
			m_bDCOFirstShot = true;
			string roe = "-";
			IEntity leader = m_Group.GetLeaderEntity();
			SCR_AICombatComponent lcc;
			if (leader)
				lcc = SCR_AICombatComponent.Cast(leader.FindComponent(SCR_AICombatComponent));
			if (lcc)
				roe = typename.EnumToString(EAIGroupCombatMode, lcc.GetCombatMode());
			float delay = (now - m_fDCOContactAt_ms) / 1000.0;
			DCO_Perf.Count("contact:first_shot");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Group), string.Format("first_shot grp=%1 delay_s=%2 roe=%3", DCO_Log.IdGroup(m_Group), delay.ToString(-1, 1), roe));
			return;
		}
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
		{
			s |= DCO_EGroupState.IN_CONTACT;
			DCO_FirstShotCheck(now);
		}
		else
		{
			m_fDCOContactAt_ms = -1;
			m_bDCOFirstShot = false;
		}
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
		if (Replication.IsServer())
		{
			DCO_LowAmmoTick();
			DCO_CruiseTick();
			DCO_SimTick();
			DCO_WpTick();
			DCO_LodTick();
		}
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

	protected bool m_bDCOSupportLink;

	void DCO_MakeSupportOf(DCO_GroupUtilityComponent parent, AICommander_BaseComponent cmd)
	{
		m_DCOSupportOf = parent;
		if (!parent || !cmd)
			return;
		m_bDCOSupportLink = true;
		m_bAutoDetectDone = true;
		RegisterCommanderToGroup(cmd);
		DCO_Perf.Count("veh:crew_link");
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("veh_crew_link grp=%1 parent=%2 cmd=%3", DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), DCO_Log.IdGroup(AIGroup.Cast(parent.GetOwner())), cmd.GetCommanderUID()));
	}

	bool DCO_AdoptOrphanCrew()
	{
		if (!m_bDCOSupportLink || m_DCOSupportOf)
			return false;
		m_bDCOSupportLink = false;
		AICommander_BaseComponent cmd = myCommander;
		myCommander = null;
		bool ok = cmd && cmd.AssignGroup(this);
		DCO_Perf.Count("veh:crew_adopt");
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("veh_crew_adopt grp=%1 ok=%2 units=%3 why=parent_gone", DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), ok, GetUnitCount()));
		return true;
	}

	void SetDedicatedTransport(bool tf)
	{
		DedicatedTransport = DCO_TransportTeamComponent.Cast(GetOwner().FindComponent(DCO_TransportTeamComponent));
		if (!DedicatedTransport)
			return;
		if (tf)
			m_iDCOWantedRole = 2;
		else if (m_iDCOWantedRole == 2)
		{
			m_iDCOWantedRole = 0;
			m_iDCORoleState = 0;
		}
		if (tf == m_bIsDedicatedTransport)
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

	protected int m_iDCOWantedRole;
	protected int m_iDCORoleState;
	protected float m_fDCORoleTick_ms = -100000;
	protected float m_fDCORoleBoard_ms = -100000;
	protected IEntity m_DCORoleVeh;
	protected IEntity m_DCORoleFound;
	protected float m_fDCORoleFoundD;
	protected vector m_vDCORoleFrom;

	int DCO_GetWantedRole()			{ return m_iDCOWantedRole; }

	static string DCO_RoleName(int role)
	{
		if (role == 1)
			return "MORTAR";
		if (role == 2)
			return "TRANSPORT";
		if (role == 3)
			return "ARMOR";
		if (role == 4)
			return "MECHANIZED";
		if (role == 5)
			return "MOTORIZED";
		return "AUTO";
	}

	protected static string DCO_RoleWhyText(string why)
	{
		if (why == "no_free_mortar_40m")
			return "no free mortar within 40 m";
		if (why == "no_free_vehicle_40m")
			return "no free vehicle with cargo seats within 40 m";
		if (why == "no_free_cargo_seat")
			return "its vehicle has no free cargo seat";
		if (why == "boarding")
			return "it is boarding its vehicle";
		if (why == "busy")
			return "it boards when it has no task and no contact";
		return why;
	}

	void DCO_SetWantedRole(int role, int playerID)
	{
		role = Math.ClampInt(role, 0, 5);
		if (role == m_iDCOWantedRole)
			return;
		int old = m_iDCOWantedRole;
		if (role != 1 && IsMortar())
			DemoteMortar("role_change");
		if (role != 2 && m_bIsDedicatedTransport)
			SetDedicatedTransport(false);
		m_iDCOWantedRole = role;
		m_iDCORoleState = 0;
		m_DCORoleVeh = null;
		m_fDCORoleBoard_ms = -100000;
		m_fDCORoleTick_ms = -100000;
		m_bAutoDetectDone = role != 0;
		if (role == 3)
			m_eGroupRoleExternal = DCO_EGroupPreset.ARMOR;
		else if (role == 4)
			m_eGroupRoleExternal = DCO_EGroupPreset.MECHANIZED;
		else if (role == 5)
			m_eGroupRoleExternal = DCO_EGroupPreset.MOTORIZED;
		else if (!IsMortar())
			m_eGroupRoleExternal = DCO_EGroupPreset.AUTO;
		if (role == 0)
			m_fAssignedAt_ms = GetGame().GetWorld().GetWorldTime();
		m_fCapabilityCheck_ms = -1;
		string id = DCO_Log.IdGroup(AIGroup.Cast(GetOwner()));
		DCO_Perf.Count("role:set");
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, id, string.Format("role_set grp=%1 role=%2 old=%3 player=%4", id, DCO_RoleName(role), DCO_RoleName(old), playerID));
		if (role == 1 || role == 2)
			DCO_RoleTick();
	}

	protected void DCO_RoleTick()
	{
		if (!myCommander || !m_Group || IsPlayerGroup() || GetUnitCount() <= 0)
			return;
		int state = 2;
		string why;
		IEntity veh;
		if (m_iDCOWantedRole == 1)
		{
			if (!IsMortar())
				DetectMortar();
			if (IsMortar())
				state = 1;
			else
				why = "no_free_mortar_40m";
		}
		else
			state = DCO_RoleTransport(why, veh);

		if (state == m_iDCORoleState)
			return;
		m_iDCORoleState = state;
		string id = DCO_Log.IdGroup(AIGroup.Cast(GetOwner()));
		string call = DCO_PlayerComms.GetCallsign(m_Group);
		if (state == 1)
		{
			DCO_Perf.Count("role:active");
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, id, string.Format("role_active grp=%1 role=%2 veh=%3", id, DCO_RoleName(m_iDCOWantedRole), DCO_Log.IdVeh(veh)));
			if (m_iDCOWantedRole == 2)
				DCO_PlayerComms.SendToGameMasters("DCO", call + " is now a transport team.");
			return;
		}
		string label = "Mortar Team";
		if (m_iDCOWantedRole == 2)
			label = "Transport Team";
		DCO_Perf.Count("role:pending");
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, id, string.Format("role_pending grp=%1 role=%2 why=%3 veh=%4", id, DCO_RoleName(m_iDCOWantedRole), why, DCO_Log.IdVeh(veh)));
		DCO_PlayerComms.SendToGameMasters("DCO", string.Format("%1 is set to %2 but is not ready: %3.", call, label, DCO_RoleWhyText(why)));
	}

	protected int DCO_RoleTransport(out string why, out IEntity veh)
	{
		DCO_TransportTeamComponent team = DCO_TransportTeamComponent.Cast(GetOwner().FindComponent(DCO_TransportTeamComponent));
		if (!team)
		{
			why = "no_team_component";
			return 2;
		}
		if (m_bIsDedicatedTransport)
		{
			veh = team.ResolveVehicle();
			return 1;
		}

		veh = DCO_RoleDrivenVehicle();
		if (veh)
		{
			if (DCO_TransportTeamComponent.CountFreeCargo(veh) <= 0 && !DCO_Logistics.HasResupplyStation(veh))
			{
				why = "no_free_cargo_seat";
				return 2;
			}
			SetDedicatedTransport(true);
			if (m_bIsDedicatedTransport)
				return 1;
			why = "set_failed";
			return 2;
		}

		if (m_bInTransport || HasState(DCO_EGroupState.IN_CONTACT) || (m_Task.m_eType != DCO_EGroupTask.NONE && m_Task.m_eType != DCO_EGroupTask.PATROL))
		{
			why = "busy";
			return 4;
		}

		veh = GetGroupVehicle();
		if (!veh)
			veh = DCO_RoleClaimVehicle();
		if (!veh)
		{
			why = "no_free_vehicle_40m";
			return 2;
		}
		why = "boarding";
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fDCORoleBoard_ms >= 60000.0 && DCO_RoleBoard(veh))
			m_fDCORoleBoard_ms = now;
		return 3;
	}

	protected IEntity DCO_RoleDrivenVehicle()
	{
		array<AIAgent> agents = {};
		m_Group.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			if (!a)
				continue;
			ChimeraCharacter ch = ChimeraCharacter.Cast(a.GetControlledEntity());
			if (!ch || !ch.IsInVehicle())
				continue;
			CompartmentAccessComponent acc = ch.GetCompartmentAccessComponent();
			if (acc && PilotCompartmentSlot.Cast(acc.GetCompartment()))
				return acc.GetCompartment().GetOwner().GetRootParent();
		}
		return null;
	}

	protected IEntity DCO_RoleClaimVehicle()
	{
		if (m_DCORoleVeh)
		{
			DamageManagerComponent dmg = DamageManagerComponent.Cast(m_DCORoleVeh.FindComponent(DamageManagerComponent));
			if (!dmg || dmg.GetState() != EDamageState.DESTROYED)
				return m_DCORoleVeh;
			m_DCORoleVeh = null;
		}
		IEntity leader = m_Group.GetLeaderEntity();
		if (!leader || !m_UtilityComp)
			return null;
		m_DCORoleFound = null;
		m_fDCORoleFoundD = float.MAX;
		m_vDCORoleFrom = leader.GetOrigin();
		DCO_Perf.Count("q:DCO_GroupUtility");
		GetGame().GetWorld().QueryEntitiesBySphere(m_vDCORoleFrom, 40.0, DCO_RoleVehCb, null, EQueryEntitiesFlags.ALL);
		if (!m_DCORoleFound)
			return null;
		m_DCORoleVeh = m_DCORoleFound;
		m_DCORoleFound = null;
		SCR_AIVehicleUsageComponent usage = SCR_AIVehicleUsageComponent.Cast(m_DCORoleVeh.FindComponent(SCR_AIVehicleUsageComponent));
		if (usage)
			m_UtilityComp.AddUsableVehicle(usage);
		string id = DCO_Log.IdGroup(AIGroup.Cast(GetOwner()));
		DCO_Perf.Count("role:claim_vehicle");
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, id, string.Format("role_claim grp=%1 veh=%2 dist=%3 armed=%4", id, DCO_Log.IdVeh(m_DCORoleVeh), Math.Round(Math.Sqrt(m_fDCORoleFoundD)), DCO_Util.B01(DCO_VehicleCombat.HasTurret(m_DCORoleVeh))));
		return m_DCORoleVeh;
	}

	protected bool DCO_RoleVehCb(IEntity e)
	{
		if (!Vehicle.Cast(e))
			return true;
		SCR_AIVehicleUsageComponent usage = SCR_AIVehicleUsageComponent.Cast(e.FindComponent(SCR_AIVehicleUsageComponent));
		if (!usage || usage.GetDamageState() == EDamageState.DESTROYED)
			return true;
		if (usage.m_DCOOwnerGroup && usage.m_DCOOwnerGroup != m_Group && usage.DCO_OwnerAlive())
			return true;
		BaseCompartmentManagerComponent mgr = BaseCompartmentManagerComponent.Cast(e.FindComponent(BaseCompartmentManagerComponent));
		if (!mgr)
			return true;
		array<BaseCompartmentSlot> slots = {};
		mgr.GetCompartments(slots);
		bool pilot;
		int cargo;
		foreach (BaseCompartmentSlot slot : slots)
		{
			if (slot.GetOccupant())
				return true;
			if (PilotCompartmentSlot.Cast(slot))
				pilot = true;
			else if (CargoCompartmentSlot.Cast(slot))
				cargo++;
		}
		if (!pilot || cargo <= 0)
			return true;
		float d = vector.DistanceSq(e.GetOrigin(), m_vDCORoleFrom);
		if (d < m_fDCORoleFoundD)
		{
			m_fDCORoleFoundD = d;
			m_DCORoleFound = e;
		}
		return true;
	}

	protected bool DCO_RoleBoard(IEntity veh)
	{
		SCR_AIWaypoint wp = myCommander.SpawnGetInWP(veh.GetOrigin());
		SCR_BoardingEntityWaypoint board = SCR_BoardingEntityWaypoint.Cast(wp);
		if (!board)
		{
			if (wp)
				SCR_EntityHelper.DeleteEntityAndChildren(wp);
			return false;
		}
		SetTask(DCO_EGroupTask.NONE, "role_board");
		m_Group.CompleteAllWaypoints();
		board.SetEntity(veh);
		board.SetAllowance(true, true, true);
		board.SetCompletionType(EAIWaypointCompletionType.All);
		m_Group.AddWaypoint(wp);
		DCO_SetHold(DCO_Util.NowS() + 60.0);
		string id = DCO_Log.IdGroup(AIGroup.Cast(GetOwner()));
		DCO_Perf.Count("role:board");
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, id, string.Format("role_board grp=%1 veh=%2 armed=%3", id, DCO_Log.IdVeh(veh), DCO_Util.B01(DCO_VehicleCombat.HasTurret(veh))));
		return true;
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
	static void F_DCO_MortarsOf(AICommander_BaseComponent cmd, notnull array<IEntity> outMortars)
	{
		foreach (IEntity m, DCO_GroupUtilityComponent g : GetMortarCrew())
		{
			if (m && g && g.GetMyCommander() == cmd)
				outMortars.Insert(m);
		}
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
		if (m_eGroupRoleExternal == DCO_EGroupPreset.AUTO && m_iDCOWantedRole != 1 && AutoDetectSize() > 4)
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

		SCR_AIGroupPerception gpc = m_UtilityComp.GetPercGroupComp();
		if (!gpc)
			return false;

		foreach (SCR_AIGroupTargetCluster c : gpc.m_aTargetClusters)
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

		if (m_DCOSpawnVeh && myCommander && m_Group)
		{
			string id = DCO_Log.IdGroup(AIGroup.Cast(GetOwner()));
			bool mounted = DCO_RoleBoard(m_DCOSpawnVeh);
			DCO_Perf.Count("spawner:mount");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, id, string.Format("spawner_mount grp=%1 veh=%2 ok=%3", id, DCO_Log.IdVeh(m_DCOSpawnVeh), DCO_Util.B01(mounted)));
			m_DCOSpawnVeh = null;
		}
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
