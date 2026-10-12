enum DCO_ESpawnerTarget
{
	SPECIFIC_UID,
	WEAKEST_OF_FACTION,
	NEAREST_OF_FACTION
}

enum DCO_ESpawnerObjectiveEvent
{
	NONE,
	LOST,
	CAPTURED
}

enum DCO_ESpawnerRole
{
	AUTO,
	ASSAULT,
	FIRE_TEAM,
	AT,
	RECON,
	MOTORIZED
}

enum DCO_ESpawnerExperience
{
	RECRUIT,
	REGULAR,
	VETERAN,
	ELITE
}

enum DCO_ESpawnerMorale
{
	LOW,
	NORMAL,
	HIGH
}

enum DCO_ESpawnerShape
{
	RESTED,
	REDUCED
}

enum DCO_ESpawnerStock
{
	NONE,
	MEN,
	POINTS
}

enum DCO_ESpawnerState
{
	READY,
	WAITING,
	COOLDOWN,
	OFF,
	STOPPED
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleField("m_sPrefab")]
class DCO_SpawnerGroupEntry
{
	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Group prefab.", params: "et")]
	ResourceName m_sPrefab;

	[Attribute("", UIWidgets.ResourcePickerThumbnail, "Optional vehicle prefab used by this group (spawned near the group, registered as the group vehicle).", params: "et")]
	ResourceName m_sVehicle;

	[Attribute("1", UIWidgets.EditBox, "Selection weight.")]
	int m_iWeight;

	[Attribute("-1", UIWidgets.EditBox, "Budget: how many times this group may be sent. -1 = unlimited.")]
	int m_iBudget;

	[Attribute("0", UIWidgets.ComboBox, "Group role for the commander's choice. AUTO = from group size and vehicle.", enums: ParamEnumArray.FromEnum(DCO_ESpawnerRole))]
	DCO_ESpawnerRole m_eRole;

	[Attribute("1", UIWidgets.ComboBox, "Experience: Recruit / Regular / Veteran / Elite = DCO skill Rookie / Regular / Veteran / Expert.", enums: ParamEnumArray.FromEnum(DCO_ESpawnerExperience))]
	DCO_ESpawnerExperience m_eExperience;

	[Attribute("1", UIWidgets.ComboBox, "Morale: Low (morale threshold x0.8, gets tense sooner) / Normal / High (x1.2).", enums: ParamEnumArray.FromEnum(DCO_ESpawnerMorale))]
	DCO_ESpawnerMorale m_eMorale;

	[Attribute("0", UIWidgets.ComboBox, "Condition: Rested = full. Reduced = 50% stamina and 60% spare magazines.", enums: ParamEnumArray.FromEnum(DCO_ESpawnerShape))]
	DCO_ESpawnerShape m_eShape;

	[Attribute("0", UIWidgets.EditBox, "Arrival time (min) after ordering: 0 / 10 / 25 / 40. The group does not exist until it arrives.")]
	int m_iArrivalMin;

	[Attribute("-1", UIWidgets.EditBox, "Manual point cost. -1 = automatic (soldiers x experience x morale x condition + vehicle class).")]
	int m_iCostOverride;

	[Attribute("", UIWidgets.EditBox, "Group preset name (a server file in $profile:DCO/GroupPresets or a built-in preset). When set, the group is built from that preset instead of the group prefab.")]
	string m_sPresetName;

	int m_iUsed;
	int m_iSoldiers = -1;
	string m_sMembers;

	bool IsPreset()
	{
		return !m_sMembers.IsEmpty();
	}

	bool HasBudget()
	{
		return m_iBudget < 0 || m_iUsed < m_iBudget;
	}

	int Left()
	{
		if (m_iBudget < 0)
			return -1;
		return Math.Max(m_iBudget - m_iUsed, 0);
	}

	DCO_ESpawnerRole ResolveRole()
	{
		if (m_eRole != DCO_ESpawnerRole.AUTO)
			return m_eRole;
		if (IsPreset())
			return DCO_GroupPresets.AutoRole(m_sMembers, !m_sVehicle.IsEmpty());
		if (!m_sVehicle.IsEmpty())
			return DCO_ESpawnerRole.MOTORIZED;
		if (m_iSoldiers >= 0 && m_iSoldiers <= 4)
			return DCO_ESpawnerRole.FIRE_TEAM;
		return DCO_ESpawnerRole.ASSAULT;
	}

	string Label()
	{
		string name = FilePath.StripExtension(FilePath.StripPath(m_sPrefab));
		if (IsPreset())
			name = m_sPresetName;
		if (name.IsEmpty())
			name = "catalog";
		if (m_eExperience != DCO_ESpawnerExperience.REGULAR)
			name += " · " + DCO_CommanderSpawnerComponent.ExperienceName(m_eExperience);
		return name;
	}
}

class DCO_SpawnerOrder
{
	ref DCO_SpawnerGroupEntry m_Entry;
	ResourceName m_sPrefab;
	float m_fDue;
	float m_fOrdered;
	int m_iMen;
	int m_iPoints;
	string m_sCmdUID;
	string m_sTrigger;
	bool m_bFromPool;
	bool m_bWaitLogged;
	float m_fDueReached = -1;
	ref DCO_ReinfRequest m_Req;
	float m_fArrival_s;
}

[ComponentEditorProps(category: "GameScripted/Commander", description: "DCO Commander Spawner: spawns groups and hands them straight to the AI Commander.")]
class DCO_CommanderSpawnerComponentClass : ScriptComponentClass {}

class DCO_CommanderSpawnerComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.ComboBox, "Target commander.", enums: ParamEnumArray.FromEnum(DCO_ESpawnerTarget), category: "Target")]
	protected DCO_ESpawnerTarget m_eTarget;

	[Attribute("", UIWidgets.EditBox, "Commander UID (SPECIFIC_UID mode).", category: "Target")]
	protected string m_sCommanderUID;

	[Attribute("", UIWidgets.EditBox, "Faction key (WEAKEST / NEAREST modes). Empty = all factions.", category: "Target")]
	protected FactionKey m_sFaction;

	[Attribute("", UIWidgets.Object, "Weighted list of group prefabs (force pool). Empty = random groups from the commander faction catalog.", category: "Content")]
	protected ref array<ref DCO_SpawnerGroupEntry> m_aGroups;

	[Attribute("1", UIWidgets.CheckBox, "The commander picks group types to fit its needs (AT, assault, recon, motorized). OFF = weighted random.", category: "Content")]
	protected bool m_bCommanderChooses;

	[Attribute("1", UIWidgets.EditBox, "Groups per spawn.", category: "Content")]
	protected int m_iGroupsPerSpawn;

	[Attribute("30", UIWidgets.EditBox, "Spawn radius around the spawner (m).", category: "Content")]
	protected float m_fSpawnRadius;

	[Attribute("0", UIWidgets.CheckBox, "Vehicles: ON = Crewed (vehicles come with their own crew: driver + gunner, the group rides as passengers). OFF = Group mounts (the group drives itself).", category: "Content")]
	protected bool m_bCrewed;

	[Attribute("1", UIWidgets.ComboBox, "Default experience for catalog groups (empty pool).", enums: ParamEnumArray.FromEnum(DCO_ESpawnerExperience), category: "Content")]
	protected DCO_ESpawnerExperience m_eDefExperience;

	[Attribute("1", UIWidgets.ComboBox, "Default morale for catalog groups.", enums: ParamEnumArray.FromEnum(DCO_ESpawnerMorale), category: "Content")]
	protected DCO_ESpawnerMorale m_eDefMorale;

	[Attribute("0", UIWidgets.ComboBox, "Default condition for catalog groups.", enums: ParamEnumArray.FromEnum(DCO_ESpawnerShape), category: "Content")]
	protected DCO_ESpawnerShape m_eDefShape;

	[Attribute("0", UIWidgets.EditBox, "Default arrival time (min) for catalog groups.", category: "Content")]
	protected int m_iDefArrivalMin;

	[Attribute("0", UIWidgets.CheckBox, "Spawn once when the mission starts.", category: "Modes")]
	protected bool m_bSpawnAtStart;

	[Attribute("300", UIWidgets.EditBox, "Cooldown between trigger spawns (s).", category: "Modes")]
	protected float m_fCooldown;

	[Attribute("", UIWidgets.EditBox, "Name of the trigger objective entity (empty = off).", category: "Modes")]
	protected string m_sTriggerObjective;

	[Attribute("0", UIWidgets.ComboBox, "Trigger objective event (seen from the target commander's faction).", enums: ParamEnumArray.FromEnum(DCO_ESpawnerObjectiveEvent), category: "Modes")]
	protected DCO_ESpawnerObjectiveEvent m_eObjectiveEvent;

	[Attribute("0", UIWidgets.EditBox, "Spawn when the target commander's manpower drops below this number (0 = off).", category: "Modes")]
	protected int m_iManpowerBelow;

	[Attribute("0", UIWidgets.CheckBox, "The commander asks this spawner for reinforcements when its forces are worn down, its strength ratio is too low for the mission, or it lacks a special capability (AT, recon).", category: "Modes")]
	protected bool m_bCommanderRequest;
	protected DCO_ReinfRequest m_DCOClaim;
	protected float m_fDCOReinfRetry;

	[Attribute("0", UIWidgets.CheckBox, "Legacy: use the commander's soldier stock. Used when Stock Mode = NONE.", category: "Manpower")]
	protected bool m_bUseStock;

	[Attribute("0", UIWidgets.ComboBox, "Draw from commander stock: NONE / MEN (soldiers) / POINTS. When stock runs out the spawner stops.", enums: ParamEnumArray.FromEnum(DCO_ESpawnerStock), category: "Manpower")]
	protected DCO_ESpawnerStock m_eStockMode;

	[Attribute("300", UIWidgets.EditBox, "Safe radius (m): no spawning while a player or enemy is inside it.", category: "Safety")]
	protected float m_fSafeRadius;

	[Attribute("1", UIWidgets.CheckBox, "Stop when overrun: more enemies than friendlies (min 2) inside the safe radius for Hold Time.", category: "Stop")]
	protected bool m_bStopOverrun;

	[Attribute("1", UIWidgets.CheckBox, "Stop when the home objective is lost by the commander's faction.", category: "Stop")]
	protected bool m_bStopHomeLost;

	[Attribute("1", UIWidgets.CheckBox, "Stop when there is no target commander.", category: "Stop")]
	protected bool m_bStopCommanderGone;

	[Attribute("0", UIWidgets.CheckBox, "Stop permanently after Late Minutes of mission time.", category: "Stop")]
	protected bool m_bStopLate;

	[Attribute("90", UIWidgets.EditBox, "Mission time (min) for the Late in the mission condition.", category: "Stop")]
	protected float m_fLateMin;

	[Attribute("1", UIWidgets.CheckBox, "When the stop reason clears (overrun, home, commander): ON = turn back on, OFF = stay off.", category: "Stop")]
	protected bool m_bBackOn;

	[Attribute("60", UIWidgets.EditBox, "Hold time (s) for overrun and for turning back on.", category: "Stop")]
	protected float m_fHoldTime;

	[Attribute("", UIWidgets.EditBox, "Home objective: objective entity name. Empty = automatic (nearest within 600 m owned by the target faction at mission start). '-' = none.", category: "Stop")]
	protected string m_sHomeObjective;

	protected string m_sPending;
	protected float m_fNextAllowed;
	protected bool m_bStartDone;
	protected bool m_bStopped;
	protected string m_sStopReason;
	protected int m_iLastObjState = -1;
	protected float m_fStartTime;
	protected string m_sLastDefer;
	protected ref array<ref DCO_SpawnerOrder> m_aOrders = {};
	protected bool m_bGMOff;
	protected bool m_bOff;
	protected string m_sOffReason;
	protected string m_sOffDetail;
	protected float m_fOverrunSince = -1;
	protected float m_fClearSince = -1;
	protected float m_fCmdGoneSince = -1;
	protected bool m_bHomeResolved;
	protected string m_sHomeResolved;
	protected float m_fHomeBackSince = -1;
	protected bool m_bForceOnce;
	protected string m_sWaitReason;
	protected string m_sWaitDetail;
	protected DCO_ESpawnerState m_eState = -1;
	protected string m_sStateReason;
	protected int m_iSpawnedGroups;
	protected int m_iSpawnedMen;
	protected float m_fLastSpawnAt = -1;
	protected string m_sLastSpawnTrigger;
	protected string m_sLastCmdUID;
	protected int m_iEnemyCount;
	protected int m_iFriendCount;
	[Attribute("4", UIWidgets.ComboBox, "Spawner preset. Not Custom = the preset values replace the matching settings at mission start (triggers, groups per spawn, cooldown, arrival). Custom = use the settings as they are.", enums: { ParamEnum("One-shot", "0"), ParamEnum("Counterattack wave", "1"), ParamEnum("Steady top-up", "2"), ParamEnum("Commander on call", "3"), ParamEnum("Custom", "4") }, category: "Spawner")]
	protected int m_iPreset;
	protected string m_sPresetSnap;

	[RplProp()]
	protected int m_iRplState = -1;
	[RplProp()]
	protected string m_sRplStatus;
	[RplProp()]
	protected string m_sRplPool;
	[RplProp()]
	protected vector m_vRplFeed;
	[RplProp()]
	protected vector m_vRplTrigger;
	[RplProp()]
	protected vector m_vRplHome;
	[RplProp()]
	protected float m_fRplSafe;
	[RplProp()]
	protected float m_fRplSpawnR;
	[RplProp()]
	protected string m_sRplFaction;

	protected static ref array<DCO_CommanderSpawnerComponent> s_aAll;
	protected static array<DCO_CommanderSpawnerComponent> GetAllStore()
	{
		if (!s_aAll)
			s_aAll = {};
		return s_aAll;
	}

	static array<DCO_CommanderSpawnerComponent> GetAll()	{ return GetAllStore(); }
	bool IsPending()							{ return !m_sPending.IsEmpty() || !m_aOrders.IsEmpty(); }

	DCO_ESpawnerTarget GetTargetMode()			{ return m_eTarget; }
	string GetCommanderUID()					{ return m_sCommanderUID; }
	void SetTarget(DCO_ESpawnerTarget mode, string uid)
	{
		m_eTarget = mode;
		m_sCommanderUID = uid;
		LogEdit("target", typename.EnumToString(DCO_ESpawnerTarget, mode) + ":" + uid);
	}
	FactionKey GetFaction()						{ return m_sFaction; }
	void SetFaction(FactionKey fk)				{ m_sFaction = fk; LogEdit("faction", fk); }
	int GetGroupsPerSpawn()						{ return m_iGroupsPerSpawn; }
	void SetGroupsPerSpawn(int n)				{ m_iGroupsPerSpawn = Math.Clamp(n, 1, 20); LogEdit("groups_per_spawn", m_iGroupsPerSpawn.ToString()); }
	float GetSpawnRadius()						{ return m_fSpawnRadius; }
	void SetSpawnRadius(float r)				{ m_fSpawnRadius = Math.Clamp(r, 5, 500); LogEdit("spawn_radius", m_fSpawnRadius.ToString()); }
	bool GetSpawnAtStart()						{ return m_bSpawnAtStart; }
	void SetSpawnAtStart(bool b)				{ m_bSpawnAtStart = b; LogEdit("at_start", b.ToString()); }
	float GetCooldown()							{ return m_fCooldown; }
	void SetCooldown(float s)					{ m_fCooldown = Math.Clamp(s, 0, 7200); LogEdit("cooldown", m_fCooldown.ToString()); }
	string GetTriggerObjective()				{ return m_sTriggerObjective; }
	void SetTriggerObjective(string name)
	{
		m_sTriggerObjective = name;
		m_iLastObjState = -1;
		LogEdit("trigger_objective", name);
	}
	DCO_ESpawnerObjectiveEvent GetObjectiveEvent()	{ return m_eObjectiveEvent; }
	void SetObjectiveEvent(DCO_ESpawnerObjectiveEvent e)	{ m_eObjectiveEvent = e; LogEdit("objective_event", typename.EnumToString(DCO_ESpawnerObjectiveEvent, e)); }
	int GetManpowerBelow()						{ return Math.Max(m_iManpowerBelow, 0); }
	void SetManpowerBelow(int n)				{ if (m_iManpowerBelow < 0 && n == 0) return; m_iManpowerBelow = Math.Max(n, 0); LogEdit("manpower_below", m_iManpowerBelow.ToString()); }
	bool GetUseStock()							{ return StockMode() == DCO_ESpawnerStock.MEN; }
	void SetUseStock(bool b)
	{
		if (b)
			SetStockMode(DCO_ESpawnerStock.MEN);
		else
			SetStockMode(DCO_ESpawnerStock.NONE);
	}
	DCO_ESpawnerStock StockMode()
	{
		if (m_eStockMode == DCO_ESpawnerStock.NONE && m_bUseStock)
			return DCO_ESpawnerStock.MEN;
		return m_eStockMode;
	}
	void SetStockMode(DCO_ESpawnerStock mode)
	{
		m_eStockMode = mode;
		m_bUseStock = false;
		if (m_bStopped && m_sStopReason == "stock")
		{
			m_bStopped = false;
			m_sStopReason = string.Empty;
		}
		LogEdit("stock_mode", typename.EnumToString(DCO_ESpawnerStock, mode));
	}
	bool GetCrewed()							{ return m_bCrewed; }
	void SetCrewed(bool b)						{ m_bCrewed = b; LogEdit("vehicles", b.ToString()); }
	bool GetCommanderChooses()					{ return m_bCommanderChooses; }
	void SetCommanderChooses(bool b)			{ m_bCommanderChooses = b; LogEdit("commander_chooses", b.ToString()); }
	bool GetCommanderRequest()					{ return m_bCommanderRequest; }
	void SetCommanderRequest(bool b)			{ m_bCommanderRequest = b; LogEdit("commander_asks", b.ToString()); }
	float GetSafeRadius()						{ return m_fSafeRadius; }
	void SetSafeRadius(float r)					{ m_fSafeRadius = Math.Clamp(r, 0, 2000); LogEdit("safe_radius", m_fSafeRadius.ToString()); }
	DCO_ESpawnerExperience GetDefExperience()	{ return m_eDefExperience; }
	void SetDefExperience(DCO_ESpawnerExperience v)	{ m_eDefExperience = v; LogEdit("experience", ExperienceName(v)); }
	DCO_ESpawnerMorale GetDefMorale()			{ return m_eDefMorale; }
	void SetDefMorale(DCO_ESpawnerMorale v)		{ m_eDefMorale = v; LogEdit("morale", typename.EnumToString(DCO_ESpawnerMorale, v)); }
	DCO_ESpawnerShape GetDefShape()				{ return m_eDefShape; }
	void SetDefShape(DCO_ESpawnerShape v)		{ m_eDefShape = v; LogEdit("shape", typename.EnumToString(DCO_ESpawnerShape, v)); }
	int GetDefArrivalMin()						{ return m_iDefArrivalMin; }
	void SetDefArrivalMin(int v)				{ m_iDefArrivalMin = Math.Clamp(v, 0, 120); LogEdit("arrival_min", m_iDefArrivalMin.ToString()); }
	bool GetStopOverrun()						{ return m_bStopOverrun; }
	void SetStopOverrun(bool b)					{ m_bStopOverrun = b; LogEdit("stop_overrun", b.ToString()); }
	bool GetStopHomeLost()						{ return m_bStopHomeLost; }
	void SetStopHomeLost(bool b)				{ m_bStopHomeLost = b; LogEdit("stop_home_lost", b.ToString()); }
	bool GetStopCommanderGone()					{ return m_bStopCommanderGone; }
	void SetStopCommanderGone(bool b)			{ m_bStopCommanderGone = b; LogEdit("stop_commander_gone", b.ToString()); }
	bool GetStopLate()							{ return m_bStopLate; }
	void SetStopLate(bool b)					{ m_bStopLate = b; LogEdit("stop_late", b.ToString()); }
	float GetLateMin()							{ return m_fLateMin; }
	void SetLateMin(float v)					{ m_fLateMin = Math.Clamp(v, 5, 600); LogEdit("late_min", m_fLateMin.ToString()); }
	bool GetBackOn()							{ return m_bBackOn; }
	void SetBackOn(bool b)						{ m_bBackOn = b; LogEdit("back_on", b.ToString()); }
	float GetHoldTime()							{ return m_fHoldTime; }
	void SetHoldTime(float v)					{ m_fHoldTime = Math.Clamp(v, 10, 600); LogEdit("hold_time", m_fHoldTime.ToString()); }
	string GetHomeObjectiveSetting()			{ return m_sHomeObjective; }
	void SetHomeObjectiveSetting(string name)
	{
		m_sHomeObjective = name;
		m_bHomeResolved = false;
		LogEdit("home_objective", name);
	}
	bool IsGMOff()								{ return m_bGMOff; }
	int GetPreset()								{ return m_iPreset; }
	DCO_ESpawnerState GetState()
	{
		if (Replication.IsServer())
			return m_eState;
		return m_iRplState;
	}
	string GetStateReason()						{ return m_sStateReason; }
	array<ref DCO_SpawnerOrder> GetOrders()		{ return m_aOrders; }
	int GetSpawnedGroups()						{ return m_iSpawnedGroups; }
	int GetSpawnedMen()							{ return m_iSpawnedMen; }
	string Name()
	{
		string n = GetOwner().GetName();
		if (n.IsEmpty())
			n = "Spawner " + GridName();
		return n;
	}

	protected string GridName()
	{
		vector p = GetOwner().GetOrigin();
		return Pad3(Math.Floor(p[0] / 100.0)) + "-" + Pad3(Math.Floor(p[2] / 100.0));
	}

	protected static string Pad3(int v)
	{
		string t = Math.AbsInt(v).ToString();
		while (t.Length() < 3)
			t = "0" + t;
		return t;
	}

	protected void InitClamp()
	{
		string was = string.Format("%1/%2/%3/%4/%5/%6", m_fSpawnRadius, m_fSafeRadius, m_fCooldown, m_fLateMin, m_fHoldTime, m_iGroupsPerSpawn);
		m_fSpawnRadius = Math.Clamp(m_fSpawnRadius, 5, 500);
		m_fSafeRadius = Math.Clamp(m_fSafeRadius, 0, 2000);
		m_fCooldown = Math.Clamp(m_fCooldown, 0, 7200);
		m_fLateMin = Math.Clamp(m_fLateMin, 5, 600);
		m_fHoldTime = Math.Clamp(m_fHoldTime, 10, 600);
		m_iGroupsPerSpawn = Math.ClampInt(m_iGroupsPerSpawn, 1, 20);
		string now = string.Format("%1/%2/%3/%4/%5/%6", m_fSpawnRadius, m_fSafeRadius, m_fCooldown, m_fLateMin, m_fHoldTime, m_iGroupsPerSpawn);
		if (was != now)
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("spawner_init_clamped spawner=%1 was=%2 now=%3 fields=spawn_r/safe_r/cooldown/late_min/hold/groups", LogName(), was, now));
	}

	string Title()
	{
		string n = Name();
		if (n.StartsWith("Spawner"))
			return n;
		return "Spawner " + n;
	}

	string LogName()
	{
		string n = Name();
		n.Replace(" ", "_");
		return n;
	}

	static string EntryLabel(DCO_SpawnerGroupEntry e)
	{
		if (!e)
			return "?";
		return e.Label();
	}

	static string Plural(int n, string one, string many)
	{
		if (n == 1)
			return one;
		return many;
	}

	static string Cap(string t)
	{
		if (t.IsEmpty())
			return t;
		string first = t.Substring(0, 1);
		first.ToUpper();
		return first + t.Substring(1, t.Length() - 1);
	}

	static string ExperienceName(DCO_ESpawnerExperience e)
	{
		if (e == DCO_ESpawnerExperience.RECRUIT)
			return "Recruit";
		if (e == DCO_ESpawnerExperience.VETERAN)
			return "Veteran";
		if (e == DCO_ESpawnerExperience.ELITE)
			return "Elite";
		return "Regular";
	}

	protected void Card(int kind, string title, string text, AICommander_BaseComponent cmd)
	{
		text = Cap(text);
		int sid = -1;
		SCR_EditableEntityComponent ed = SCR_EditableEntityComponent.Cast(GetOwner().FindComponent(SCR_EditableEntityComponent));
		if (ed)
			sid = Replication.FindItemId(ed);
		int cid = -1;
		if (cmd && cmd.GetOwner())
		{
			SCR_EditableEntityComponent ced = SCR_EditableEntityComponent.Cast(cmd.GetOwner().FindComponent(SCR_EditableEntityComponent));
			if (ced)
				cid = Replication.FindItemId(ced);
		}
		DCO_PlayerComms.SendSpawnerCard(sid, cid, kind, title, text);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_card spawner=%1 kind=%2 title=%3", LogName(), kind, title));
	}

	protected void CancelDue(int playerID)
	{
		float now = DCO_Util.NowS();
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		int n;
		for (int i = m_aOrders.Count() - 1; i >= 0; i--)
		{
			DCO_SpawnerOrder o = m_aOrders[i];
			if (now < o.m_fDue)
				continue;
			AICommander_BaseComponent cmd;
			if (mgr)
				cmd = mgr.FindCommanderByUID(o.m_sCmdUID);
			if (cmd && o.m_iMen > 0)
				cmd.RefundStock(o.m_iMen);
			if (cmd && o.m_iPoints > 0)
				cmd.RefundPoints(o.m_iPoints);
			if (o.m_bFromPool && o.m_Entry)
				o.m_Entry.m_iUsed = Math.Max(o.m_Entry.m_iUsed - 1, 0);
			DCO_Perf.Count("spawner:refund_gm_cancel");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_cancel spawner=%1 group=%2 refund_men=%3 refund_pts=%4 reason=gm_cancel gm=%5", LogName(), EntryLabel(o.m_Entry), o.m_iMen, o.m_iPoints, playerID));
			m_aOrders.Remove(i);
			n++;
		}
		m_sPending = string.Empty;
		if (n > 0)
			Card(0, Title(), string.Format("%1 waiting %2 cancelled by the GM. Stock returned.", n, Plural(n, "group was", "groups were")), ResolveCommander());
		UpdateState();
	}

	protected void LogEdit(string field, string value)
	{
		if (!GetOwner())
			return;
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_edit spawner=%1 field=%2 value=%3", LogName(), field, value));
	}

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!GetAllStore().Contains(this))
			GetAllStore().Insert(this);
		if (!Replication.IsServer() || !GetGame().InPlayMode())
			return;
		m_fStartTime = DCO_Util.NowS();
		InitClamp();
		GetGame().GetCallqueue().CallLater(Tick, 5000, true);
		GetGame().GetCallqueue().CallLater(PublishPresets, 3000, false);
		GetGame().GetCallqueue().CallLater(InitWorkbenchSetup, 2500, false);
	}

	protected void InitWorkbenchSetup()
	{
		if (m_aGroups)
		{
			foreach (DCO_SpawnerGroupEntry e : m_aGroups)
			{
				if (!e || e.m_sPresetName.IsEmpty() || !e.m_sMembers.IsEmpty())
					continue;
				DCO_GroupPreset p = DCO_GroupPresets.Find(e.m_sPresetName, OwnFaction(ResolveCommander()));
				if (!p)
				{
					DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("spawner_pool spawner=%1 op=resolve group=%2 result=not_found", LogName(), e.m_sPresetName));
					continue;
				}
				e.m_sMembers = p.m_sMembers;
				if (e.m_sVehicle.IsEmpty())
					e.m_sVehicle = p.m_sVehicle;
				if (e.m_eRole == DCO_ESpawnerRole.AUTO)
					e.m_eRole = p.m_iRole;
				e.m_iSoldiers = p.MemberCount();
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_pool spawner=%1 op=resolve group=%2 men=%3 result=ok", LogName(), e.m_sPresetName, e.m_iSoldiers));
			}
		}
		if (m_iPreset >= 0 && m_iPreset <= 3)
		{
			bool askedInWorkbench = m_bCommanderRequest;
			ApplyPreset(m_iPreset);
			LogEdit("preset_workbench", m_iPreset.ToString());
			if (m_eObjectiveEvent != DCO_ESpawnerObjectiveEvent.NONE && m_sTriggerObjective.IsEmpty())
			{
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("spawner_trigger_none spawner=%1 preset=%2 event=%3 gm_notice=1 (no trigger objective and no home objective held within 600 m at start, so this objective trigger can never fire; set Trigger Objective)", LogName(), m_iPreset, typename.EnumToString(DCO_ESpawnerObjectiveEvent, m_eObjectiveEvent)));
				DCO_PlayerComms.SendToGameMasters("DCO", Title() + " can never use its objective trigger: it has no Trigger Objective and holds no objective nearby. Set Trigger Objective, or use the Commander on call preset.");
			}
			if (askedInWorkbench && !m_bCommanderRequest)
			{
				m_bCommanderRequest = true;
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_preset_keep spawner=%1 preset=%2 field=commander_asks value=1 reason=workbench_explicit (the preset turns it off, but the mission maker ticked Commander asks)", LogName(), m_iPreset));
			}
		}
		else
			m_iPreset = 4;
	}

	override void OnDelete(IEntity owner)
	{
		GetAllStore().RemoveItem(this);
		if (GetGame())
			GetGame().GetCallqueue().Remove(Tick);
		super.OnDelete(owner);
	}

	protected void PublishPresets()
	{
		DCO_GroupPresets.Publish();
	}

	void SpawnNow(int playerID)
	{
		if (m_bGMOff || m_bStopped)
		{
			Card(0, Title(), string.Format("Cannot spawn: %1.", StateSentence()), ResolveCommander());
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_spawn_now spawner=%1 gm=%2 result=refused reason=%3", LogName(), playerID, m_sStateReason));
			return;
		}
		if (m_bOff)
			Card(0, Title(), string.Format("%1 is off (%2). Spawn now waits until it is back on.", Title(), m_sOffDetail), ResolveCommander());
		m_sPending = "gm";
		m_fNextAllowed = 0;
		Tick();
		string result = "ordered";
		if (m_bOff || !m_sWaitReason.IsEmpty())
			result = "waiting";
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_spawn_now spawner=%1 gm=%2 result=%3 reason=%4", LogName(), playerID, result, m_sStateReason));
	}

	void SpawnAnyway(int playerID)
	{
		if (m_bGMOff || m_bStopped)
		{
			string why = "stopped";
			if (m_bGMOff)
				why = "gm_off";
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_force spawner=%1 gm=%2 result=refused reason=%3", LogName(), playerID, why));
			return;
		}
		m_bForceOnce = true;
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_force spawner=%1 gm=%2", LogName(), playerID));
		Tick();
	}

	void SetGMOff(bool off, int playerID)
	{
		if (off == m_bGMOff)
			return;
		m_bGMOff = off;
		if (off)
		{
			m_iGMOffCancelled = CancelOrders("gm_off");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_off spawner=%1 reason=gm enemy=%2 friendly=%3 gm=%4", LogName(), m_iEnemyCount, m_iFriendCount, playerID));
			DCO_Events.Spawner("spawner_off", string.Format("spawner=%1 reason=gm", Name()));
		}
		else
		{
			m_sOnReason = "gm";
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_on spawner=%1 reason=gm gm=%2", LogName(), playerID));
		}
		UpdateState();
		PublishStatus(ResolveCommander(), DCO_Util.NowS());
	}

	protected string m_sOnReason;
	protected int m_iGMOffCancelled;

	protected int PoolIndex(int row)
	{
		if (!m_aGroups || row < 0)
			return -1;
		int n = 0;
		foreach (int i, DCO_SpawnerGroupEntry pe : m_aGroups)
		{
			if (!pe)
				continue;
			if (n == row)
				return i;
			n++;
		}
		return -1;
	}

	void GMAction(int action, int a, int b, string s, int playerID)
	{
		if (action == 0)
		{
			SpawnNow(playerID);
			return;
		}
		if (action == 1)
		{
			SpawnAnyway(playerID);
			return;
		}
		if (action == 2)
		{
			SetGMOff(!m_bGMOff, playerID);
			return;
		}
		if (action == 3)
		{
			CancelDue(playerID);
			return;
		}
		if (action == 10)
		{
			PoolAdd(s, playerID);
			return;
		}
		if (action >= 21 && action <= 25)
		{
			PresetAction(action, s, playerID);
			return;
		}
		int ri = PoolIndex(a);
		if (ri < 0)
		{
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("spawner_pool_bad_index spawner=%1 action=%2 row=%3 gm=%4", LogName(), action, a, playerID));
			return;
		}
		DCO_SpawnerGroupEntry e = m_aGroups[ri];
		string op = "edit";
		if (action == 11)
		{
			if (e.m_iBudget < 0)
				e.m_iBudget = Math.Max(e.m_iUsed + 1, 1);
			e.m_iBudget = Math.Clamp(e.m_iBudget + b, Math.Max(e.m_iUsed, 1), 99);
		}
		else if (action == 20)
		{
			if (e.m_iBudget < 0)
				e.m_iBudget = Math.Max(e.m_iUsed + 1, 1);
			else
				e.m_iBudget = -1;
		}
		else if (action == 12)
			e.m_eRole = Math.ClampInt(b, 0, 5);
		else if (action == 13)
			e.m_eExperience = Math.ClampInt(b, 0, 3);
		else if (action == 14)
			e.m_eMorale = Math.ClampInt(b, 0, 2);
		else if (action == 15)
			e.m_eShape = Math.ClampInt(b, 0, 1);
		else if (action == 16)
			e.m_iArrivalMin = Math.Clamp(b, 0, 120);
		else if (action == 17)
			e.m_iCostOverride = Math.Clamp(b, -1, 999);
		else if (action == 19)
			e.m_iWeight = Math.Clamp(b, 1, 20);
		else if (action == 18)
		{
			op = "remove";
			m_aGroups.RemoveOrdered(ri);
		}
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_pool spawner=%1 op=%2 group=%3 count=%4 quality=%5/%6/%7 arrival=%8 cost=%9",
			LogName(), op, e.Label(), e.m_iBudget, ExperienceName(e.m_eExperience), typename.EnumToString(DCO_ESpawnerMorale, e.m_eMorale), typename.EnumToString(DCO_ESpawnerShape, e.m_eShape), e.m_iArrivalMin, CostOf(e, Math.Max(e.m_iSoldiers, 0))) + " gm=" + playerID.ToString());
		PublishStatus(ResolveCommander(), DCO_Util.NowS());
	}

	protected void PresetAction(int action, string s, int playerID)
	{
		string gm = GetGame().GetPlayerManager().GetPlayerName(playerID);
		if ((action == 22 || action == 24 || action == 25) && !DCO_GroupPresets.CanUpload(playerID))
		{
			DCO_Perf.Count("gm_rpc:preset_blocked");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("group_preset op=blocked action=%1 name=%2 gm=%3 why=no_rights rule=%4", action, s, playerID, DCO_GroupPresets.UploadMode()));
			string uploadMode = DCO_GroupPresets.UploadMode();
			string uploadText = "You may not change group presets on this server.";
			if (uploadMode == "admin")
				uploadText = "Only admins may change group presets on this server.";
			else if (uploadMode == "off")
				uploadText = "Nobody may change group presets on this server (server setting groupPresetUploads).";
			DCO_PlayerComms.SendToGameMasters("DCO", uploadText);
			return;
		}
		if (action == 22)
		{
			DCO_GroupPreset lp = DCO_GroupPreset.Unpack(s);
			if (lp && !DCO_GroupPresets.Exists(lp.m_sName) && DCO_GroupPresets.CountFiles() >= DCO_GroupPresets.UploadLimit())
			{
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("group_preset op=blocked action=22 name=%1 gm=%2 why=limit limit=%3", lp.m_sName, playerID, DCO_GroupPresets.UploadLimit()));
				DCO_PlayerComms.SendToGameMasters("DCO", string.Format("This server already has %1 presets, the most it allows. Delete one first.", DCO_GroupPresets.UploadLimit()));
				return;
			}
		}
		if (action == 21)
		{
			DCO_GroupPreset p = DCO_GroupPresets.Load(s);
			if (p)
				PoolAddPreset(p, playerID);
			return;
		}
		if (action == 22 || action == 23)
		{
			DCO_GroupPreset np = DCO_GroupPreset.Unpack(s);
			if (!np || np.m_sName.IsEmpty() || np.MemberCount() == 0 || np.MemberCount() > 12)
			{
				DCO_PlayerComms.SendToGameMasters("DCO", "Group preset not saved: it needs a name and 1 to 12 men.");
				return;
			}
			np.m_sMembers = DCO_GroupPresets.LeaderFirst(np.m_sMembers);
			if (np.m_sFaction.IsEmpty())
				np.m_sFaction = OwnFaction(ResolveCommander());
			np.m_sMaker = gm;
			np.m_iCreated = System.GetUnixTime();
			if (action == 22)
			{
				string op = "save";
				if (DCO_GroupPresets.Exists(np.m_sName))
					op = "replace";
				bool ok = DCO_GroupPresets.Save(np);
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("group_preset op=%1 name=%2 from=builder men=%3 vehicle=%4 gm=%5 ok=%6", op, np.m_sName, np.MemberCount(), np.m_sVehicle, playerID, ok));
				if (ok)
					DCO_PlayerComms.SendToGameMasters("DCO", string.Format("Group preset saved: %1 (%2 %3).", np.m_sName, np.MemberCount(), Plural(np.MemberCount(), "man", "men")));
				DCO_GroupPresets.Publish();
				return;
			}
			PoolAddPreset(np, playerID);
			return;
		}
		if (action == 24)
		{
			bool del = DCO_GroupPresets.Delete(s);
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("group_preset op=delete name=%1 from=builder men=0 vehicle=none gm=%2 ok=%3", s, playerID, del));
			DCO_GroupPresets.Publish();
			return;
		}
		if (action == 25)
		{
			array<string> pair = {};
			s.Split("|", pair, false);
			if (pair.Count() < 2 || pair[1].Trim().IsEmpty())
				return;
			DCO_GroupPreset rp = DCO_GroupPresets.Load(pair[0]);
			if (!rp)
				return;
			rp.m_sName = pair[1].Trim();
			if (DCO_GroupPresets.Save(rp))
				DCO_GroupPresets.Delete(pair[0]);
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("group_preset op=rename name=%1 from=builder men=%2 vehicle=%3 gm=%4 old=%5", rp.m_sName, rp.MemberCount(), rp.m_sVehicle, playerID, pair[0]));
			DCO_GroupPresets.Publish();
		}
	}

	protected void PoolAddPreset(DCO_GroupPreset p, int playerID)
	{
		if (!m_aGroups)
			m_aGroups = {};
		DCO_SpawnerGroupEntry e = new DCO_SpawnerGroupEntry();
		e.m_sMembers = p.m_sMembers;
		e.m_sPresetName = p.m_sName;
		e.m_sVehicle = p.m_sVehicle;
		e.m_iWeight = 1;
		e.m_iBudget = 1;
		e.m_eRole = p.m_iRole;
		e.m_eExperience = p.m_iExperience;
		e.m_eMorale = p.m_iMorale;
		e.m_eShape = p.m_iShape;
		e.m_iArrivalMin = p.m_iArrival;
		e.m_iCostOverride = -1;
		e.m_iSoldiers = p.MemberCount();
		m_aGroups.Insert(e);
		if (m_bStopped && m_sStopReason == "budget")
		{
			m_bStopped = false;
			m_sStopReason = string.Empty;
		}
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_pool spawner=%1 op=add group=%2 count=%3 men=%4 cost=%5 gm=%6 from=preset", LogName(), e.Label(), e.m_iBudget, e.m_iSoldiers, CostOf(e, e.m_iSoldiers), playerID));
		PublishStatus(ResolveCommander(), DCO_Util.NowS());
	}

	protected void PoolAdd(string prefab, int playerID)
	{
		if (prefab.IsEmpty())
			return;
		Resource r = Resource.Load(prefab);
		if (!r || !r.IsValid())
			return;
		if (!m_aGroups)
			m_aGroups = {};
		DCO_SpawnerGroupEntry e = new DCO_SpawnerGroupEntry();
		e.m_sPrefab = prefab;
		e.m_iWeight = 1;
		e.m_iBudget = 1;
		e.m_eRole = DCO_ESpawnerRole.AUTO;
		e.m_eExperience = m_eDefExperience;
		e.m_eMorale = m_eDefMorale;
		e.m_eShape = m_eDefShape;
		e.m_iArrivalMin = m_iDefArrivalMin;
		e.m_iCostOverride = -1;
		e.m_iSoldiers = PrefabSoldiers(r);
		m_aGroups.Insert(e);
		if (m_bStopped && m_sStopReason == "budget")
		{
			m_bStopped = false;
			m_sStopReason = string.Empty;
		}
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_pool spawner=%1 op=add group=%2 count=%3 men=%4 cost=%5 gm=%6", LogName(), e.Label(), e.m_iBudget, e.m_iSoldiers, CostOf(e, e.m_iSoldiers), playerID));
		PublishStatus(ResolveCommander(), DCO_Util.NowS());
	}

	protected string Snap()
	{
		return string.Format("%1|%2|%3|%4|%5|%6|%7|%8|%9", m_bSpawnAtStart, m_eObjectiveEvent, m_iManpowerBelow, m_bCommanderRequest, m_iGroupsPerSpawn, m_bCommanderChooses, Math.Round(m_fCooldown), m_iDefArrivalMin, m_bStopOverrun);
	}

	string PresetLabel()
	{
		array<string> names = {"One-shot", "Counterattack wave", "Steady top-up", "Commander on call", "Custom"};
		int p = Math.ClampInt(m_iPreset, 0, 4);
		if (p == 4 || m_sPresetSnap.IsEmpty())
			return names[p];
		array<string> a = {};
		array<string> b = {};
		Snap().Split("|", a, false);
		m_sPresetSnap.Split("|", b, false);
		int changed;
		for (int i = 0; i < a.Count() && i < b.Count(); i++)
		{
			if (a[i] != b[i])
				changed++;
		}
		return string.Format("%1 · %2 rows changed", names[p], changed);
	}

	void ApplyPreset(int preset)
	{
		m_iPreset = preset;
		if (preset == 0)
		{
			m_bSpawnAtStart = true;
			m_eObjectiveEvent = DCO_ESpawnerObjectiveEvent.NONE;
			m_iManpowerBelow = 0;
			m_bCommanderRequest = false;
			int rows = 0;
			if (m_aGroups)
				rows = m_aGroups.Count();
			m_iGroupsPerSpawn = Math.Clamp(Math.Max(rows, 1), 1, 20);
			m_bStopOverrun = true;
			SetAllArrival(0);
		}
		else if (preset == 1)
		{
			m_bSpawnAtStart = false;
			m_eObjectiveEvent = DCO_ESpawnerObjectiveEvent.LOST;
			if (m_sTriggerObjective.IsEmpty())
				m_sTriggerObjective = ResolveHomeName();
			m_iManpowerBelow = 0;
			m_bCommanderRequest = false;
			m_iGroupsPerSpawn = 2;
			m_bCommanderChooses = true;
			m_fCooldown = 600;
			SetAllArrival(10);
		}
		else if (preset == 2)
		{
			m_bSpawnAtStart = false;
			m_eObjectiveEvent = DCO_ESpawnerObjectiveEvent.NONE;
			m_bCommanderRequest = false;
			m_iManpowerBelow = -1;
			m_iSteadyPeak = 0;
			m_iGroupsPerSpawn = 1;
			m_fCooldown = 300;
			SetAllArrival(10);
		}
		else if (preset == 3)
		{
			m_bSpawnAtStart = false;
			m_eObjectiveEvent = DCO_ESpawnerObjectiveEvent.NONE;
			m_iManpowerBelow = 0;
			m_bCommanderRequest = true;
			m_iGroupsPerSpawn = 1;
			m_bCommanderChooses = true;
			m_fCooldown = 300;
			SetAllArrival(0);
		}
		m_sPresetSnap = Snap();
		LogEdit("preset", preset.ToString());
	}

	protected void SetAllArrival(int min)
	{
		m_iDefArrivalMin = min;
		if (!m_aGroups)
			return;
		foreach (DCO_SpawnerGroupEntry e : m_aGroups)
		{
			if (e)
				e.m_iArrivalMin = min;
		}
	}

	protected void Tick()
	{
		float now = DCO_Util.NowS();
		AICommander_BaseComponent cmd = ResolveCommander();
		if (cmd)
			m_sLastCmdUID = cmd.GetCommanderUID();

		UpdateStopConditions(cmd, now);

		if (m_bSpawnAtStart && !m_bStartDone && now - m_fStartTime >= 10.0)
		{
			m_bStartDone = true;
			Request("start", true);
		}

		if (cmd)
		{
			if (m_iManpowerBelow < 0)
				SteadyLine(cmd, now);
			if (m_iManpowerBelow > 0 && cmd.GetTotalManpower() < m_iManpowerBelow)
				Request("manpower", false);
			if (m_bCommanderRequest)
				DCO_ReinfClaim(cmd);
			CheckObjective(cmd.GetCommanderFactionKey());
			DCO_ESpawnerStock peakMode = StockMode();
			if (peakMode == DCO_ESpawnerStock.POINTS)
				m_iDefStockPeak = Math.Max(m_iDefStockPeak, cmd.GetPointsStock());
			else if (peakMode != DCO_ESpawnerStock.NONE)
				m_iDefStockPeak = Math.Max(m_iDefStockPeak, cmd.GetReinforcementStock());
		}

		m_sWaitReason = string.Empty;
		m_sWaitDetail = string.Empty;
		bool canOrder = !m_bGMOff && !m_bStopped && (!m_bOff || m_sPending == "gm");
		if (!m_sPending.IsEmpty() && canOrder)
			CreateOrders(cmd, now);
		else if (!m_sPending.IsEmpty() && !canOrder && m_sPending != "gm")
			m_sPending = string.Empty;

		ProcessOrders(now);
		UpdateState();
		PublishStatus(cmd, now);
		IdleReport(cmd, now);
	}

	protected float m_fIdleLogAt = -1;
	protected SCR_AIGroup m_LastSpawnGrp;
	protected int m_iDefStockPeak;

	static float BestEtaS(AICommander_BaseComponent cmd, vector pos, int prio)
	{
		if (!cmd)
			return -1;
		float best = -1;
		foreach (DCO_CommanderSpawnerComponent s : GetAllStore())
		{
			if (!s || !s.GetOwner() || !s.DCO_CanFill(cmd))
				continue;
			float arrMin = -1;
			if (s.m_aGroups && !s.m_aGroups.IsEmpty())
			{
				foreach (DCO_SpawnerGroupEntry e : s.m_aGroups)
				{
					if (!s.Usable(e) || e.Left() == 0)
						continue;
					if (arrMin < 0 || e.m_iArrivalMin < arrMin)
						arrMin = e.m_iArrivalMin;
				}
				if (arrMin < 0)
					continue;
			}
			else
				arrMin = s.m_iDefArrivalMin;
			float eta = arrMin * 60.0 * DCO_ReinfRequest.PrioMul(prio) + vector.DistanceXZ(s.GetOwner().GetOrigin(), pos) / 2.0;
			if (best < 0 || eta < best)
				best = eta;
		}
		return best;
	}

	protected bool DefenseCanFill(AICommander_BaseComponent cmd, DCO_ReinfRequest req, int count, float now)
	{
		DCO_ESpawnerStock mode = StockMode();
		if (mode != DCO_ESpawnerStock.NONE && req.m_iPrio < 2)
		{
			int stock = cmd.GetReinforcementStock();
			if (mode == DCO_ESpawnerStock.POINTS)
				stock = cmd.GetPointsStock();
			m_iDefStockPeak = Math.Max(m_iDefStockPeak, stock);
			float reserve = DCO_Ctx.TuneForCmd(cmd, "reinf.def_stock_reserve", 0.35) * m_iDefStockPeak;
			if (stock < reserve)
			{
				DCO_Perf.Count("def:reinf_skip_stock_reserve");
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("def_reinf_skip spawner=%1 id=%2 obj=%3 reason=stock_reserve stock=%4 reserve=%5 prio=%6", LogName(), req.m_iId, req.m_sObj, stock, Math.Round(reserve), DCO_ReinfRequest.PrioName(req.m_iPrio)));
				return false;
			}
		}
		if (m_aGroups && !m_aGroups.IsEmpty())
		{
			int avail;
			foreach (DCO_SpawnerGroupEntry e : m_aGroups)
			{
				if (!Usable(e))
					continue;
				int left = e.Left();
				if (left < 0)
					avail += count;
				else
					avail += left;
			}
			if (avail < count)
			{
				DCO_Perf.Count("reinf:defense_unfillable");
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("def_reinf_skip spawner=%1 id=%2 obj=%3 reason=piecemeal need=%4 avail=%5 (pool cannot cover the gap, nothing sent)", LogName(), req.m_iId, req.m_sObj, count, avail));
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("reinf_unfillable spawner=%1 cmd=%2 id=%3 type=DEFENSE reason=budget", LogName(), cmd.GetCommanderUID(), req.m_iId));
				return false;
			}
		}
		return true;
	}

	protected void DefAssign(SCR_AIGroup grp, CMD_AICommanderObjectiveComponent obj, int reqId)
	{
		if (!grp || !obj)
			return;
		DCO_GroupUtilityComponent util = DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));
		AICommander_BaseComponent cmd;
		if (util)
			cmd = util.GetMyCommander();
		if (!util || !cmd)
		{
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("def_reinf_assign grp=%1 obj=%2 id=%3 ok=0 reason=no_commander", DCO_Log.IdGroup(grp), obj.GetLogName(), reqId));
			return;
		}
		cmd.StartObjectiveGarrison(util, obj, DCO_Util.NowS());
		DCO_Perf.Count("reinf:defense_fill");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("def_reinf_assign grp=%1 obj=%2 id=%3 cmd=%4 ok=1 order=garrison", DCO_Log.IdGroup(grp), obj.GetLogName(), reqId, cmd.GetCommanderUID()));
	}

	protected void IdleReport(AICommander_BaseComponent cmd, float now)
	{
		if (!m_aOrders.IsEmpty() || !m_sPending.IsEmpty() || now - m_fStartTime < 30.0)
			return;
		if (m_fIdleLogAt >= 0 && now - m_fIdleLogAt < 120.0)
			return;
		m_fIdleLogAt = now;
		string state = "ready";
		if (m_bStopped)
			state = "stopped";
		else if (m_bGMOff)
			state = "gm_off";
		else if (m_bOff)
			state = "off_" + m_sOffReason;
		else if (now < m_fNextAllowed)
			state = "cooldown";
		else if (!m_sWaitReason.IsEmpty())
			state = "waiting_" + m_sWaitReason;
		string start = "-";
		if (m_bSpawnAtStart)
		{
			start = "pending";
			if (m_bStartDone)
				start = "done";
		}
		string ev = "-";
		if (m_eObjectiveEvent != DCO_ESpawnerObjectiveEvent.NONE)
		{
			string trig = m_sTriggerObjective;
			if (trig.IsEmpty())
				trig = "NONE";
			ev = typename.EnumToString(DCO_ESpawnerObjectiveEvent, m_eObjectiveEvent) + "@" + trig;
		}
		string man = "-";
		if (m_iManpowerBelow < 0)
			man = "line_pending";
		else if (m_iManpowerBelow > 0 && cmd)
			man = string.Format("%1/%2", cmd.GetTotalManpower(), m_iManpowerBelow);
		string asks = "0";
		if (m_bCommanderRequest)
			asks = "1";
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_idle spawner=%1 state=%2 start=%3 event=%4 manpower_now_line=%5 asks=%6 fillers=%7 (no spawn: no trigger condition holds)", LogName(), state, start, ev, man, asks, FillReport(cmd)));
	}

	static string Clock(float s)
	{
		int t = Math.Max(Math.Round(s), 0);
		int m = t / 60;
		int sec = t % 60;
		string ss = sec.ToString();
		if (sec < 10)
			ss = "0" + ss;
		return m.ToString() + ":" + ss;
	}

	protected void PublishStatus(AICommander_BaseComponent cmd, float now)
	{
		if (!Replication.IsServer())
			return;
		array<string> f = {};
		f.Insert(Cap(StateSentence()));
		string feeds = "-";
		if (cmd)
			feeds = cmd.GetCommanderUID();
		f.Insert(feeds);
		f.Insert(string.Format("%1 %2 · %3 men", m_iSpawnedGroups, Plural(m_iSpawnedGroups, "group", "groups"), m_iSpawnedMen));
		string last = "-";
		if (m_fLastSpawnAt >= 0)
			last = Clock(m_fLastSpawnAt - m_fStartTime) + " · " + m_sLastSpawnTrigger;
		f.Insert(last);
		string next = "now";
		float nextLeft = m_fNextAllowed - now;
		if (nextLeft > 0 && nextLeft < 60)
			next = "in under a minute";
		else if (nextLeft >= 60)
		{
			int nextMin = Math.Ceil(nextLeft / 60.0);
			next = string.Format("in about %1 min", nextMin);
		}
		f.Insert(next);
		string orders;
		foreach (DCO_SpawnerOrder o : m_aOrders)
		{
			if (!orders.IsEmpty())
				orders += ";";
			orders += EntryLabel(o.m_Entry) + " · " + Clock(o.m_fDue - m_fStartTime);
		}
		f.Insert(orders);
		f.Insert(PoolTotalText());
		string men;
		string pts;
		int shared;
		if (cmd)
		{
			if (cmd.GetReinforcementStock() >= 0)
				men = string.Format("%1 of %2 men", cmd.GetReinforcementStock(), cmd.GetStockStart());
			else
				men = "unlimited men";
			if (cmd.GetPointsStock() >= 0)
				pts = string.Format("%1 of %2 points", cmd.GetPointsStock(), cmd.GetPointsStart());
			else
				pts = "unlimited points";
			foreach (DCO_CommanderSpawnerComponent o2 : GetAllStore())
			{
				if (o2 && o2 != this && o2.m_sLastCmdUID == cmd.GetCommanderUID() && o2.StockMode() != DCO_ESpawnerStock.NONE)
					shared++;
			}
		}
		f.Insert(men);
		f.Insert(pts);
		f.Insert(StockMode().ToString());
		string blocker = m_sWaitReason;
		if (m_bOff)
			blocker = m_sOffReason;
		f.Insert(blocker);
		f.Insert(shared.ToString());
		f.Insert(m_sStateReason);
		f.Insert(GetHomeName());
		f.Insert(PresetLabel());
		string reinf = "-";
		if (cmd && m_bCommanderRequest)
			reinf = cmd.DCO_Reinf().StatusText();
		f.Insert(reinf);
		if (m_bCommanderChooses)
			f.Insert("1");
		else
			f.Insert("0");
		string status;
		foreach (int fi, string fv : f)
		{
			if (fi > 0)
				status += "\n";
			status += fv;
		}

		string pool;
		if (m_aGroups)
		{
			foreach (DCO_SpawnerGroupEntry e : m_aGroups)
			{
				if (!e)
					continue;
				if (!pool.IsEmpty())
					pool += ";";
				int men2 = Math.Max(e.m_iSoldiers, 0);
				pool += string.Format("%1|%2|%3|%4|%5|%6|%7|%8|%9", e.Label(), typename.EnumToString(DCO_ESpawnerRole, e.m_eRole), e.m_iWeight, e.m_eExperience, e.m_eMorale, e.m_eShape, e.m_iArrivalMin, CostOf(e, men2), e.m_iCostOverride >= 0) + "|" + e.Left().ToString() + "|" + e.m_iBudget.ToString() + "|" + men2.ToString() + "|" + typename.EnumToString(DCO_ESpawnerRole, e.ResolveRole());
			}
		}

		string fk = OwnFaction(cmd);
		if (fk != m_sRplFaction)
		{
			m_sRplFaction = fk;
			Replication.BumpMe();
		}
		vector feed;
		if (cmd && cmd.GetOwner())
			feed = cmd.GetOwner().GetOrigin();
		vector trig;
		CMD_AICommanderObjectiveComponent tobj = FindObjective(m_sTriggerObjective);
		if (tobj && m_eObjectiveEvent != DCO_ESpawnerObjectiveEvent.NONE)
			trig = tobj.GetOwner().GetOrigin();
		vector home;
		CMD_AICommanderObjectiveComponent hobj = FindObjective(GetHomeName());
		if (hobj && m_bStopHomeLost)
			home = hobj.GetOwner().GetOrigin();

		int st = m_eState;
		if (st == m_iRplState && status == m_sRplStatus && pool == m_sRplPool && feed == m_vRplFeed && trig == m_vRplTrigger && home == m_vRplHome && m_fSafeRadius == m_fRplSafe && m_fSpawnRadius == m_fRplSpawnR)
			return;
		m_iRplState = st;
		m_sRplStatus = status;
		m_sRplPool = pool;
		m_vRplFeed = feed;
		m_vRplTrigger = trig;
		m_vRplHome = home;
		m_fRplSafe = m_fSafeRadius;
		m_fRplSpawnR = m_fSpawnRadius;
		Replication.BumpMe();
	}

	protected string PoolTotalText()
	{
		if (!m_aGroups || m_aGroups.IsEmpty())
			return "Pool empty: random catalog groups";
		int groups;
		int menLeft;
		int menTotal;
		int ptsLeft;
		int ptsTotal;
		bool unlimited;
		foreach (DCO_SpawnerGroupEntry e : m_aGroups)
		{
			if (!e || (e.m_sPrefab.IsEmpty() && !e.IsPreset()))
				continue;
			if (e.m_iSoldiers < 0)
			{
				Resource r = Resource.Load(e.m_sPrefab);
				if (r && r.IsValid())
					e.m_iSoldiers = PrefabSoldiers(r);
				else
					e.m_iSoldiers = 0;
			}
			int cost = CostOf(e, e.m_iSoldiers);
			if (e.m_iBudget < 0)
			{
				unlimited = true;
				continue;
			}
			groups += e.Left();
			menLeft += e.Left() * e.m_iSoldiers;
			menTotal += e.m_iBudget * e.m_iSoldiers;
			ptsLeft += e.Left() * cost;
			ptsTotal += e.m_iBudget * cost;
		}
		string s = string.Format("Pool left: %1 %2 · %3 of %4 men · %5 of %6 points", groups, Plural(groups, "group", "groups"), menLeft, menTotal, ptsLeft, ptsTotal);
		if (unlimited)
			s += " · some rows unlimited";
		return s;
	}

	string GetRplFaction()					{ return m_sRplFaction; }
	int GetRplState()						{ return m_iRplState; }
	string GetRplStatus()					{ return m_sRplStatus; }
	string GetRplPool()						{ return m_sRplPool; }
	vector GetRplFeed()						{ return m_vRplFeed; }
	vector GetRplTrigger()					{ return m_vRplTrigger; }
	vector GetRplHome()						{ return m_vRplHome; }
	float GetRplSafe()						{ return m_fRplSafe; }
	float GetRplSpawnRadius()				{ return m_fRplSpawnR; }

	string StatusField(int i)
	{
		array<string> parts = {};
		m_sRplStatus.Split("\n", parts, false);
		if (i < 0 || i >= parts.Count())
			return string.Empty;
		return parts[i];
	}

	protected void Request(string reason, bool ignoreCooldown)
	{
		if (m_bStopped || m_bGMOff || !m_sPending.IsEmpty())
			return;
		if (m_bOff)
			return;
		if (!ignoreCooldown && DCO_Util.NowS() < m_fNextAllowed)
			return;
		m_sPending = reason;
	}

	protected void CheckObjective(FactionKey fk)
	{
		if (m_sTriggerObjective.IsEmpty() || m_eObjectiveEvent == DCO_ESpawnerObjectiveEvent.NONE)
			return;

		CMD_AICommanderObjectiveComponent obj = FindObjective(m_sTriggerObjective);
		if (!obj)
			return;

		int state = 0;
		if (obj.IsCapturedBy(fk))
			state = 1;

		int last = m_iLastObjState;
		m_iLastObjState = state;
		if (last < 0 || last == state)
			return;

		if (m_eObjectiveEvent == DCO_ESpawnerObjectiveEvent.CAPTURED && state == 1)
			Request("objective_captured", false);
		else if (m_eObjectiveEvent == DCO_ESpawnerObjectiveEvent.LOST && state == 0)
			Request("objective_lost", false);
	}

	static CMD_AICommanderObjectiveComponent FindObjective(string name)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr || name.IsEmpty())
			return null;
		foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
		{
			if (o && o.GetOwner() && (o.GetOwner().GetName() == name || (!name.IsEmpty() && o.GetLogName() == name)))
				return o;
		}
		return null;
	}

	AICommander_BaseComponent ResolveCommander()
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return null;

		if (m_eTarget == DCO_ESpawnerTarget.SPECIFIC_UID)
			return mgr.FindCommanderByUID(m_sCommanderUID);

		vector origin = GetOwner().GetOrigin();
		AICommander_BaseComponent best;
		float bestScore = float.MAX;
		foreach (AICommander_BaseComponent c : mgr.m_aCommander)
		{
			if (!c || (!m_sFaction.IsEmpty() && c.GetCommanderFactionKey() != m_sFaction))
				continue;

			float score;
			if (m_eTarget == DCO_ESpawnerTarget.NEAREST_OF_FACTION)
				score = vector.DistanceXZ(c.GetOwner().GetOrigin(), origin);
			else
				score = 1.0 * c.GetTotalManpower() / Math.Max(c.GetObjectiveList().Count(), 1.0);

			if (score < bestScore)
			{
				bestScore = score;
				best = c;
			}
		}
		return best;
	}

	protected FactionKey OwnFaction(AICommander_BaseComponent cmd)
	{
		if (cmd)
			return cmd.GetCommanderFactionKey();
		return m_sFaction;
	}

	protected string ResolveHomeName()
	{
		if (m_sHomeObjective == "-")
			return string.Empty;
		if (!m_sHomeObjective.IsEmpty())
			return m_sHomeObjective;
		if (m_bHomeResolved)
			return m_sHomeResolved;
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return string.Empty;
		FactionKey fk = OwnFaction(ResolveCommander());
		if (fk.IsEmpty())
			return string.Empty;
		vector origin = GetOwner().GetOrigin();
		float best = 600.0;
		string name;
		foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
		{
			if (!o || !o.GetOwner() || o.GetOwner().GetName().IsEmpty() || !o.IsCapturedBy(fk))
				continue;
			float d = vector.DistanceXZ(o.GetOwner().GetOrigin(), origin);
			if (d <= best)
			{
				best = d;
				name = o.GetOwner().GetName();
			}
		}
		m_bHomeResolved = true;
		m_sHomeResolved = name;
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_home spawner=%1 home=%2 source=auto", LogName(), name));
		return name;
	}

	string GetHomeName()
	{
		return ResolveHomeName();
	}

	protected void UpdateStopConditions(AICommander_BaseComponent cmd, float now)
	{
		if (m_bStopped)
			return;

		if (m_bStopLate && now - m_fStartTime >= m_fLateMin * 60.0)
		{
			StopForGood("time", string.Format("the mission passed %1 min", Math.Round(m_fLateMin)));
			return;
		}

		FactionKey fk = OwnFaction(cmd);
		CountArea(fk);

		string reason;
		string detail;
		bool condition = false;

		if (m_bStopCommanderGone && !cmd)
		{
			if (m_fCmdGoneSince < 0)
				m_fCmdGoneSince = now;
			if (now - m_fCmdGoneSince >= 10.0)
			{
				condition = true;
				reason = "commander_gone";
				detail = "no commander to feed";
				if (!m_sFaction.IsEmpty())
					detail = string.Format("no %1 commander left", m_sFaction);
			}
		}
		else
			m_fCmdGoneSince = -1;

		if (!condition && m_bStopOverrun && m_fSafeRadius > 0 && !fk.IsEmpty())
		{
			bool overrun = m_iEnemyCount > m_iFriendCount && m_iEnemyCount >= 2;
			if (overrun)
			{
				if (m_fOverrunSince < 0)
					m_fOverrunSince = now;
				m_fClearSince = -1;
			}
			else
			{
				m_fOverrunSince = -1;
				if (m_iFriendCount > m_iEnemyCount)
				{
					if (m_fClearSince < 0)
						m_fClearSince = now;
				}
				else
					m_fClearSince = -1;
			}
			bool wasOverrun = m_bOff && m_sOffReason == "overrun";
			if ((overrun && now - m_fOverrunSince >= m_fHoldTime) || (wasOverrun && (m_fClearSince < 0 || now - m_fClearSince < m_fHoldTime)))
			{
				condition = true;
				reason = "overrun";
				detail = string.Format("%1 %2 against %3 %4 inside its %5 m area", m_iEnemyCount, EnemyLabel(fk), m_iFriendCount, fk, Math.Round(m_fSafeRadius));
			}
		}

		if (!condition && m_bStopHomeLost && cmd)
		{
			string home = ResolveHomeName();
			CMD_AICommanderObjectiveComponent obj = FindObjective(home);
			if (obj)
			{
				FactionKey homeOwner = obj.GetOwnerFaction();
				bool enemyHolds = !homeOwner.IsEmpty() && homeOwner != fk && !FriendlyKeys(homeOwner, fk);
				bool held = !enemyHolds;
				string homeState = "own";
				if (enemyHolds)
					homeState = "enemy";
				else if (!obj.IsCapturedBy(fk))
					homeState = "neutral";
				if (homeState != m_sHomeState)
				{
					DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_home_state spawner=%1 home=%2 owner=%3 state=%4 lost=%5 (only an enemy capture counts as lost)", LogName(), home, homeOwner, homeState, DCO_Util.B01(enemyHolds)));
					m_sHomeState = homeState;
				}
				bool wasLost = m_bOff && m_sOffReason == "home_lost";
				if (held)
				{
					if (m_fHomeBackSince < 0)
						m_fHomeBackSince = now;
				}
				else
					m_fHomeBackSince = -1;
				if (!held || (wasLost && now - m_fHomeBackSince < m_fHoldTime))
				{
					condition = true;
					reason = "home_lost";
					detail = string.Format("home objective %1 was taken by %2", home, homeOwner);
				}
			}
		}

		if (condition && !m_bOff)
		{
			m_bOff = true;
			m_sOffReason = reason;
			m_sOffDetail = detail;
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_off spawner=%1 reason=%2 enemy=%3 friendly=%4", LogName(), reason, m_iEnemyCount, m_iFriendCount));
			DCO_Events.Spawner("spawner_off", string.Format("spawner=%1 reason=%2", Name(), reason));
			if (!m_bBackOn)
				StopForGood(reason, detail);
		}
		else if (!condition && m_bOff)
		{
			string onReason = "area_retaken";
			if (m_sOffReason == "home_lost")
				onReason = "home_retaken";
			else if (m_sOffReason == "commander_gone")
				onReason = "commander_back";
			m_bOff = false;
			m_sOnReason = onReason;
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_on spawner=%1 reason=%2", LogName(), onReason));
			m_sOffReason = string.Empty;
		}
	}

	protected string m_sHomeState;

	protected static bool FriendlyKeys(FactionKey a, FactionKey b)
	{
		FactionManager fm = GetGame().GetFactionManager();
		if (!fm)
			return false;
		Faction fa = fm.GetFactionByKey(a);
		Faction fb = fm.GetFactionByKey(b);
		return fa && fb && fa.IsFactionFriendly(fb);
	}

	static string FillReport(AICommander_BaseComponent cmd)
	{
		if (!cmd)
			return "-";
		string rep;
		int ok;
		foreach (DCO_CommanderSpawnerComponent s : GetAllStore())
		{
			if (!s)
				continue;
			string why = "ok";
			if (!s.m_bCommanderRequest)
				why = "asks_off";
			else if (s.m_bStopped)
				why = "stopped";
			else if (s.m_bGMOff)
				why = "gm_off";
			else if (s.m_bOff)
				why = "off_" + s.m_sOffReason;
			else if (DCO_Util.NowS() < s.m_fDCOReinfRetry)
				why = "retry";
			else if (!s.AnyBudget())
				why = "no_budget";
			else if (s.m_sLastCmdUID != cmd.GetCommanderUID())
				why = "feeds_other";
			else
				ok++;
			if (!rep.IsEmpty())
				rep += ",";
			rep += s.LogName() + ":" + why;
		}
		if (rep.IsEmpty())
			return "none";
		return string.Format("%1[%2]", ok, rep);
	}

	protected string EnemyLabel(FactionKey fk)
	{
		if (m_sEnemyKey.IsEmpty() || m_sEnemyKey == "mixed")
			return Plural(m_iEnemyCount, "enemy", "enemies");
		return m_sEnemyKey;
	}

	protected void SteadyLine(AICommander_BaseComponent cmd, float now)
	{
		m_iSteadyPeak = Math.Max(m_iSteadyPeak, cmd.GetTotalManpower());
		if (now - m_fStartTime < 60.0 || m_iSteadyPeak <= 0)
			return;
		m_iManpowerBelow = Math.Max(Math.Round(0.6 * m_iSteadyPeak), 1);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_steady_line spawner=%1 cmd=%2 peak=%3 line=%4 window_s=60", LogName(), cmd.GetCommanderUID(), m_iSteadyPeak, m_iManpowerBelow));
	}

	protected int m_iSteadyPeak;
	protected string m_sEnemyKey;

	string ClaimKey()
	{
		vector p = GetOwner().GetOrigin();
		return string.Format("%1@%2,%3", LogName(), Math.Round(p[0]), Math.Round(p[2]));
	}

	protected void CountArea(FactionKey fk)
	{
		m_iEnemyCount = 0;
		m_iFriendCount = 0;
		m_sEnemyKey = string.Empty;
		if (fk.IsEmpty() || m_fSafeRadius <= 0)
			return;
		FactionManager fm = GetGame().GetFactionManager();
		if (!fm)
			return;
		Faction own = fm.GetFactionByKey(fk);
		if (!own)
			return;
		vector origin = GetOwner().GetOrigin();
		float safeSq = m_fSafeRadius * m_fSafeRadius;

		array<IEntity> chars = {};
		AIWorld world = GetGame().GetAIWorld();
		if (world)
		{
			array<AIAgent> agents = {};
			world.GetAIAgents(agents);
			foreach (AIAgent a : agents)
			{
				IEntity e = a.GetControlledEntity();
				if (e && !chars.Contains(e))
					chars.Insert(e);
			}
		}
		array<int> players = {};
		GetGame().GetPlayerManager().GetPlayers(players);
		foreach (int pid : players)
		{
			IEntity p = GetGame().GetPlayerManager().GetPlayerControlledEntity(pid);
			if (p && !chars.Contains(p))
				chars.Insert(p);
		}

		array<IEntity> armorSeen = {};
		foreach (IEntity e : chars)
		{
			ChimeraCharacter ch = ChimeraCharacter.Cast(e);
			if (!ch || !DCO_Util.IsAlive(ch) || vector.DistanceSqXZ(ch.GetOrigin(), origin) > safeSq)
				continue;
			FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(ch.FindComponent(FactionAffiliationComponent));
			if (!fac || !fac.GetAffiliatedFaction())
				continue;
			Faction f = fac.GetAffiliatedFaction();
			if (f == own || f.IsFactionFriendly(own))
			{
				m_iFriendCount++;
				continue;
			}
			m_iEnemyCount++;
			string ek = f.GetFactionKey();
			if (m_sEnemyKey.IsEmpty())
				m_sEnemyKey = ek;
			else if (m_sEnemyKey != ek)
				m_sEnemyKey = "mixed";
			if (ch.IsInVehicle())
			{
				IEntity veh = DCO_VehicleCombat.GetVehicle(ch);
				if (veh && !armorSeen.Contains(veh) && DCO_VehicleCombat.IsArmored(veh))
				{
					armorSeen.Insert(veh);
					m_iEnemyCount += 4;
				}
			}
		}
	}

	protected void StopForGood(string reason, string detail)
	{
		if (m_bStopped)
			return;
		m_bStopped = true;
		m_sStopReason = reason;
		m_sOffDetail = detail;
		CancelOrders("stopped");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("spawner_stop spawner=%1 reason=%2 detail=%3", LogName(), reason, detail));
		DCO_Events.Spawner("spawner_stop", string.Format("spawner=%1 reason=%2 detail=%3", Name(), reason, detail));
	}

	protected void RefundOrder(DCO_SpawnerOrder o, string reason)
	{
		AICommander_BaseComponent cmd;
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
			cmd = mgr.FindCommanderByUID(o.m_sCmdUID);
		if (cmd && o.m_iMen > 0)
			cmd.RefundStock(o.m_iMen);
		if (cmd && o.m_iPoints > 0)
			cmd.RefundPoints(o.m_iPoints);
		if (o.m_bFromPool && o.m_Entry)
			o.m_Entry.m_iUsed = Math.Max(o.m_Entry.m_iUsed - 1, 0);
		DCO_Perf.Count("spawner:refund_" + reason);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_cancel spawner=%1 group=%2 refund_men=%3 refund_pts=%4 reason=%5", LogName(), EntryLabel(o.m_Entry), o.m_iMen, o.m_iPoints, reason));
	}

	protected void DCO_ReinfClaim(AICommander_BaseComponent cmd)
	{
		if (!DCO_CanFill(cmd))
			return;
		DCO_ReinfRequest r = cmd.DCO_Reinf().Claimable(ClaimKey());
		if (!r)
			return;
		if (r.m_sSpawner.IsEmpty())
		{
			float myD = vector.DistanceXZ(GetOwner().GetOrigin(), r.m_vPos);
			foreach (DCO_CommanderSpawnerComponent s : GetAllStore())
			{
				if (!s || s == this || !s.DCO_CanFill(cmd))
					continue;
				if (vector.DistanceXZ(s.GetOwner().GetOrigin(), r.m_vPos) < myD)
					return;
			}
			r.m_sSpawner = ClaimKey();
		}
		m_DCOClaim = r;
		Request("reinf:" + r.KindName(), r.m_iPrio >= 2);
		if (m_sPending.IndexOf("reinf:") != 0)
			m_DCOClaim = null;
	}

	bool DCO_CanFill(AICommander_BaseComponent cmd)
	{
		if (!cmd || !m_bCommanderRequest || m_bStopped || m_bGMOff || m_bOff || DCO_Util.NowS() < m_fDCOReinfRetry || !AnyBudget())
			return false;
		return m_sLastCmdUID == cmd.GetCommanderUID();
	}

	protected static int DCO_ReinfNeedBit(string type)
	{
		if (type == "AT")
			return 1;
		return 4;
	}

	protected DCO_SpawnerGroupEntry DCO_ReinfPick(Faction faction, DCO_ReinfRequest req, int needs, out string why)
	{
		if (req.m_iKind != 2)
		{
			if (req.m_sStyle == "USSR" && m_aGroups && !m_aGroups.IsEmpty())
			{
				DCO_SpawnerGroupEntry big;
				foreach (DCO_SpawnerGroupEntry e : m_aGroups)
				{
					if (Usable(e) && (!big || e.m_iSoldiers > big.m_iSoldiers))
						big = e;
				}
				if (big)
				{
					why = "ussr_whole_unit";
					return big;
				}
			}
			return PickEntry(faction, needs, why);
		}
		why = "reinf_" + req.m_sType;
		if (m_aGroups && !m_aGroups.IsEmpty())
		{
			array<DCO_SpawnerGroupEntry> fit = {};
			foreach (DCO_SpawnerGroupEntry c : m_aGroups)
			{
				if (Usable(c) && NeedBit(c.ResolveRole()) == needs)
					fit.Insert(c);
			}
			if (fit.IsEmpty() && req.m_sType == "RECON" && DCO_Ctx.TuneFor(GetOwner(), "reinf.cap_substitute", 0) > 0)
			{
				DCO_SpawnerGroupEntry small;
				foreach (DCO_SpawnerGroupEntry sc : m_aGroups)
				{
					if (Usable(sc) && sc.m_iSoldiers > 0 && sc.m_iSoldiers <= 6 && (!small || sc.m_iSoldiers < small.m_iSoldiers))
						small = sc;
				}
				if (small)
				{
					why = "reinf_RECON_substitute";
					DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("reinf_fill_substitute spawner=%1 type=RECON used=%2 men=%3 reason=no_recon_in_pool", LogName(), EntryLabel(small), small.m_iSoldiers));
					return small;
				}
			}
			if (fit.IsEmpty())
				return null;
			return fit.GetRandomElement();
		}
		SCR_Faction scrFaction = SCR_Faction.Cast(faction);
		if (!scrFaction)
			return null;
		SCR_EntityCatalog catalog = scrFaction.GetFactionEntityCatalogOfType(EEntityCatalogType.GROUP, false);
		if (!catalog)
			return null;
		array<SCR_EntityCatalogEntry> list = {};
		catalog.GetEntityList(list);
		array<ResourceName> fitc = {};
		foreach (SCR_EntityCatalogEntry ce : list)
		{
			string n = ce.GetPrefab();
			n.ToLower();
			bool ok;
			if (needs == 1)
				ok = n.Contains("_at") || n.Contains("antitank") || n.Contains("anti_tank");
			else
				ok = n.Contains("recon") || n.Contains("sniper") || n.Contains("scout") || n.Contains("fireteam") || n.Contains("fire_team");
			if (ok)
				fitc.Insert(ce.GetPrefab());
		}
		if (fitc.IsEmpty())
			return null;
		DCO_SpawnerGroupEntry pick = new DCO_SpawnerGroupEntry();
		pick.m_sPrefab = fitc.GetRandomElement();
		pick.m_iWeight = 1;
		pick.m_iBudget = -1;
		pick.m_eRole = DCO_ESpawnerRole.RECON;
		if (needs == 1)
			pick.m_eRole = DCO_ESpawnerRole.AT;
		pick.m_eExperience = m_eDefExperience;
		pick.m_eMorale = m_eDefMorale;
		pick.m_eShape = m_eDefShape;
		pick.m_iArrivalMin = m_iDefArrivalMin;
		pick.m_iCostOverride = -1;
		why = "catalog_" + req.m_sType;
		return pick;
	}

	protected int CancelOrders(string reason)
	{
		if (m_aOrders.IsEmpty())
			return 0;
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		int n = m_aOrders.Count();
		foreach (DCO_SpawnerOrder o : m_aOrders)
		{
			RefundOrder(o, reason);
		}
		m_aOrders.Clear();
		if (reason != "gm_off")
			Card(0, Title() + ": orders cancelled", string.Format("%1 %2 on the way %3 cancelled (spawner stopped). Stock returned.", n, Plural(n, "group", "groups"), Plural(n, "was", "were")), ResolveCommander());
		return n;
	}

	protected void CreateOrders(AICommander_BaseComponent cmd, float now)
	{
		if (!cmd)
		{
			SetWait("no_commander", "there is no commander to feed");
			return;
		}

		FactionManager fm = GetGame().GetFactionManager();
		Faction faction;
		if (fm)
			faction = fm.GetFactionByKey(cmd.GetCommanderFactionKey());
		if (!faction)
		{
			SetWait("no_faction", "the commander has no faction");
			return;
		}

		DCO_ESpawnerStock stockMode = StockMode();
		int needs = ReadNeeds(cmd);
		int ordered;
		string list;
		DCO_ReinfRequest req;
		if (m_sPending.IndexOf("reinf:") == 0)
			req = m_DCOClaim;
		m_DCOClaim = null;
		int count = Math.Clamp(m_iGroupsPerSpawn, 1, 20);
		if (req)
		{
			count = Math.Max(req.m_iGroups - req.m_iOrdered, 0);
			if (req.m_iKind == 2)
				needs = DCO_ReinfNeedBit(req.m_sType);
			if (req.m_iKind == 3 && !DefenseCanFill(cmd, req, count, now))
			{
				m_fDCOReinfRetry = now + 60.0;
				count = 0;
			}
		}
		for (int i = 0; i < count; i++)
		{
			if (!AnyBudget())
			{
				StopForGood("budget", "every group in the force pool is used up");
				break;
			}

			string why;
			DCO_SpawnerGroupEntry entry;
			if (req)
				entry = DCO_ReinfPick(faction, req, needs, why);
			else
				entry = PickEntry(faction, needs, why);
			if (!entry && req && req.m_iKind == 2)
			{
				DCO_Perf.Count("reinf:unfillable");
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("reinf_unfillable spawner=%1 cmd=%2 id=%3 type=%4", LogName(), cmd.GetCommanderUID(), req.m_iId, req.m_sType));
				if (cmd.DCO_Reinf())
					cmd.DCO_Reinf().MarkNoSource(req.m_sType, LogName(), now);
				break;
			}
			if (!entry)
			{
				SetWait("no_prefab", "no group prefab available");
				break;
			}

			int soldiers;
			if (entry.IsPreset())
				soldiers = entry.m_iSoldiers;
			else
			{
				Resource res = Resource.Load(entry.m_sPrefab);
				if (!res || !res.IsValid())
				{
					SetWait("bad_prefab", "a group prefab cannot be loaded");
					break;
				}
				soldiers = PrefabSoldiers(res);
			}
			entry.m_iSoldiers = soldiers;
			int crew = CrewCount(entry);
			soldiers += crew;
			int points = CostOf(entry, entry.m_iSoldiers) + crew;
			int men;
			int pts;
			if (stockMode == DCO_ESpawnerStock.MEN)
			{
				if (!cmd.ConsumeStock(soldiers))
				{
					StopForGood("stock", string.Format("%1's reinforcement stock is empty", cmd.GetCommanderUID()));
					break;
				}
				men = soldiers;
			}
			else if (stockMode == DCO_ESpawnerStock.POINTS)
			{
				if (!cmd.ConsumePoints(points))
				{
					StopForGood("stock", string.Format("%1's reinforcement points are used up", cmd.GetCommanderUID()));
					break;
				}
				pts = points;
			}

			bool fromPool = m_aGroups && m_aGroups.Contains(entry);
			if (fromPool)
				entry.m_iUsed++;

			DCO_SpawnerOrder o = new DCO_SpawnerOrder();
			o.m_Entry = entry;
			o.m_sPrefab = entry.m_sPrefab;
			o.m_fOrdered = now;
			o.m_fArrival_s = Math.Max(entry.m_iArrivalMin, 0) * 60.0;
			o.m_fDue = now + o.m_fArrival_s;
			if (req)
			{
				o.m_Req = req;
				o.m_fDue = now + o.m_fArrival_s * DCO_ReinfRequest.PrioMul(req.m_iPrio);
				req.m_iOrdered++;
			}
			o.m_iMen = men;
			o.m_iPoints = pts;
			o.m_sCmdUID = cmd.GetCommanderUID();
			o.m_sTrigger = m_sPending;
			o.m_bFromPool = fromPool;
			m_aOrders.Insert(o);
			ordered++;

			DCO_ESpawnerRole role = entry.ResolveRole();
			needs = needs & ~NeedBit(role);
			if (!list.IsEmpty())
				list += ", ";
			list += entry.Label();
			if (entry.m_iArrivalMin > 0)
				list += string.Format(" (arrives in %1 min)", entry.m_iArrivalMin);

			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_choice spawner=%1 cmd=%2 prefab=%3 role=%4 why=%5 left=%6",
				LogName(), cmd.GetCommanderUID(), entry.Label(), typename.EnumToString(DCO_ESpawnerRole, role), why, entry.Left()));
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_order spawner=%1 cmd=%2 group=%3 arrival_s=%4 cost_men=%5 cost_pts=%6 stock_left=%7 trigger=%8",
				LogName(), cmd.GetCommanderUID(), entry.Label(), Math.Round(o.m_fDue - now), soldiers, points, StockLeftText(cmd), m_sPending));
		}

		if (req)
		{
			if (ordered > 0)
			{
				DCO_Perf.Count("reinf:claim");
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("reinf_claim spawner=%1 cmd=%2 id=%3 kind=%4 groups=%5 prio=%6 mul=%7 why=%8", LogName(), cmd.GetCommanderUID(), req.m_iId, req.KindName(), ordered, DCO_ReinfRequest.PrioName(req.m_iPrio), DCO_ReinfRequest.PrioMul(req.m_iPrio), req.m_sWhy));
				Card(0, string.Format("%1 requests reinforcements", cmd.GetCommanderUID()), string.Format("%1. %2. %3 sends %4 %5.", req.Text(), cmd.DCO_Reinf().StatusText(), Title(), ordered, Plural(ordered, "group", "groups")), cmd);
			}
			else
			{
				if (req.m_sSpawner == ClaimKey())
					req.m_sSpawner = string.Empty;
				m_fDCOReinfRetry = now + 60.0;
			}
			m_sPending = string.Empty;
			m_sLastDefer = string.Empty;
		}
		if (ordered > 0)
		{
			Card(0, string.Format("%1 → %2", Title(), cmd.GetCommanderUID()), string.Format("%1 %2 ordered: %3.%4", ordered, Plural(ordered, "group", "groups"), list, StockSentence(cmd)), cmd);
			m_fNextAllowed = now + m_fCooldown;
		}
		if (ordered > 0 || m_bStopped)
		{
			m_sPending = string.Empty;
			m_sLastDefer = string.Empty;
		}
	}

	protected string StockLeftText(AICommander_BaseComponent cmd)
	{
		DCO_ESpawnerStock mode = StockMode();
		if (mode == DCO_ESpawnerStock.MEN)
			return cmd.GetReinforcementStock().ToString() + "men";
		if (mode == DCO_ESpawnerStock.POINTS)
			return cmd.GetPointsStock().ToString() + "pts";
		return "-";
	}

	protected string StockSentence(AICommander_BaseComponent cmd)
	{
		DCO_ESpawnerStock mode = StockMode();
		if (mode == DCO_ESpawnerStock.MEN && cmd.GetReinforcementStock() >= 0)
			return string.Format(" Stock %1 of %2 men.", cmd.GetReinforcementStock(), cmd.GetStockStart());
		if (mode == DCO_ESpawnerStock.POINTS && cmd.GetPointsStock() >= 0)
			return string.Format(" Stock %1 of %2 points.", cmd.GetPointsStock(), cmd.GetPointsStart());
		return string.Empty;
	}

	protected void ProcessOrders(float now)
	{
		if (m_aOrders.IsEmpty() || m_bGMOff || m_bStopped)
			return;

		bool force = m_bForceOnce;
		for (int i = 0; i < m_aOrders.Count(); i++)
		{
			DCO_SpawnerOrder o = m_aOrders[i];
			if (o.m_Req && o.m_Req.m_bCancelled && o.m_Req.m_sCancelWhy != "filled")
			{
				RefundOrder(o, "reinf_" + o.m_Req.m_sCancelWhy);
				m_aOrders.RemoveOrdered(i);
				i--;
				continue;
			}
			if (o.m_Req)
			{
				float rdue = o.m_fOrdered + o.m_fArrival_s * DCO_ReinfRequest.PrioMul(o.m_Req.m_iPrio);
				if (rdue < o.m_fDue - 1.0)
				{
					DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("reinf_due_advance spawner=%1 id=%2 prio=%3 due_in_old_s=%4 due_in_s=%5", LogName(), o.m_Req.m_iId, DCO_ReinfRequest.PrioName(o.m_Req.m_iPrio), Math.Round(o.m_fDue - now), Math.Round(rdue - now)));
					o.m_fDue = rdue;
				}
			}
			if (now < o.m_fDue)
				continue;
			if (o.m_fDueReached < 0)
				o.m_fDueReached = now;
			if (m_bOff)
			{
				SetWait(m_sOffReason, m_sOffDetail);
				return;
			}

			AICommander_BaseComponent cmd;
			AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
			if (mgr)
				cmd = mgr.FindCommanderByUID(o.m_sCmdUID);
			if (!cmd && m_eTarget != DCO_ESpawnerTarget.SPECIFIC_UID)
				cmd = ResolveCommander();
			if (!cmd)
			{
				SetWait("no_commander", "there is no commander to feed");
				return;
			}

			FactionManager fm = GetGame().GetFactionManager();
			Faction faction;
			if (fm)
				faction = fm.GetFactionByKey(cmd.GetCommanderFactionKey());
			if (!faction)
			{
				SetWait("no_faction", "the commander has no faction");
				return;
			}

			string detail;
			string block = SafetyBlock(faction, force, detail);
			if (!block.IsEmpty())
			{
				SetWait(block, detail);
				return;
			}

			Resource res;
			int soldiers = o.m_Entry.m_iSoldiers;
			if (!o.m_Entry.IsPreset())
			{
				res = Resource.Load(o.m_sPrefab);
				if (!res || !res.IsValid())
				{
					m_aOrders.RemoveOrdered(i);
					i--;
					continue;
				}
				soldiers = PrefabSoldiers(res);
			}
			if (!SpawnGroup(cmd, o, res, soldiers))
				continue;

			m_iSpawnedGroups++;
			m_iSpawnedMen += soldiers;
			if (o.m_Req)
				o.m_Req.m_iArrived++;
			if (o.m_Req && o.m_Req.m_iKind == 3 && o.m_Req.m_DefObj && m_LastSpawnGrp)
				GetGame().GetCallqueue().CallLater(DefAssign, 1500, false, m_LastSpawnGrp, o.m_Req.m_DefObj, o.m_Req.m_iId);
			m_fLastSpawnAt = now;
			m_sLastSpawnTrigger = o.m_sTrigger;
			float waited = now - o.m_fDueReached;
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_arrive spawner=%1 cmd=%2 group=%3 waited_s=%4 forced=%5", LogName(), cmd.GetCommanderUID(), EntryLabel(o.m_Entry), Math.Round(waited), force));
			if (o.m_fDue > o.m_fOrdered + 1.0)
				Card(0, string.Format("%1 → %2", Title(), cmd.GetCommanderUID()), EntryLabel(o.m_Entry) + " arrived.", cmd);
			m_aOrders.RemoveOrdered(i);
			i--;
		}
		if (force)
			m_bForceOnce = false;
	}

	protected void SetWait(string reason, string detail)
	{
		m_sWaitReason = reason;
		m_sWaitDetail = detail;
		if (reason == m_sLastDefer)
			return;
		m_sLastDefer = reason;
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_defer spawner=%1 reason=%2 trigger=%3 detail=%4", LogName(), reason, m_sPending, detail));
	}

	protected void UpdateState()
	{
		DCO_ESpawnerState st = DCO_ESpawnerState.READY;
		string reason = "-";
		float now = DCO_Util.NowS();
		if (m_bStopped)
		{
			st = DCO_ESpawnerState.STOPPED;
			reason = m_sStopReason;
		}
		else if (m_bGMOff)
		{
			st = DCO_ESpawnerState.OFF;
			reason = "gm";
		}
		else if (m_bOff)
		{
			st = DCO_ESpawnerState.OFF;
			reason = m_sOffReason;
		}
		else if (!m_sWaitReason.IsEmpty())
		{
			st = DCO_ESpawnerState.WAITING;
			reason = m_sWaitReason;
		}
		else if (now < m_fNextAllowed)
		{
			st = DCO_ESpawnerState.COOLDOWN;
			reason = "cooldown";
		}

		if (st == m_eState && reason == m_sStateReason)
			return;
		DCO_ESpawnerState prev = m_eState;
		m_eState = st;
		m_sStateReason = reason;
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_status spawner=%1 state=%2 reason=%3", LogName(), typename.EnumToString(DCO_ESpawnerState, st), reason));
		if (prev < 0)
			return;
		if (st == DCO_ESpawnerState.WAITING && reason == "faction_cap")
		{
			if (prev != DCO_ESpawnerState.WAITING)
				Card(0, Title() + " is waiting", m_sWaitDetail + ". It spawns when there are fewer.", ResolveCommander());
		}
		else if (st == DCO_ESpawnerState.WAITING)
		{
			if (prev != DCO_ESpawnerState.WAITING)
				Card(1, Title() + " is waiting", m_sWaitDetail + ". It spawns as soon as the area is clear. Spawn anyway skips the safe radius for this one spawn, never the faction AI cap.", ResolveCommander());
		}
		else if (st == DCO_ESpawnerState.OFF && reason == "gm")
		{
			string cancelled;
			if (m_iGMOffCancelled > 0)
				cancelled = string.Format(" %1 %2 on the way %3 cancelled. Stock returned.", m_iGMOffCancelled, Plural(m_iGMOffCancelled, "group", "groups"), Plural(m_iGMOffCancelled, "was", "were"));
			m_iGMOffCancelled = 0;
			Card(0, Title() + " is off", "Turned off by the GM." + cancelled + " Only the GM can turn it back on.", ResolveCommander());
		}
		else if (st == DCO_ESpawnerState.OFF && reason == "overrun")
			Card(2, Title() + " is off: overrun", string.Format("%1. It comes back on when %2 holds the area again for %3 s.", m_sOffDetail, OwnFaction(ResolveCommander()), Math.Round(m_fHoldTime)), ResolveCommander());
		else if (st == DCO_ESpawnerState.OFF)
			Card(2, Title() + " is off", m_sOffDetail + ". It comes back on when the reason is gone.", ResolveCommander());
		else if (st == DCO_ESpawnerState.STOPPED && reason == "stock")
			Card(3, Title() + " stopped", m_sOffDetail + ". Add stock on the commander or set Take From Stock to No.", ResolveCommander());
		else if (st == DCO_ESpawnerState.STOPPED)
			Card(2, Title() + " stopped", m_sOffDetail + ".", ResolveCommander());
		else if (prev == DCO_ESpawnerState.OFF && st != DCO_ESpawnerState.OFF)
		{
			string back = OwnFaction(ResolveCommander()) + " holds the area again.";
			if (m_sOnReason == "gm")
				back = "Turned on by the GM.";
			else if (m_sOnReason == "home_retaken")
				back = OwnFaction(ResolveCommander()) + " holds its home objective again.";
			else if (m_sOnReason == "commander_back")
			{
				AICommander_BaseComponent bc = ResolveCommander();
				string cn = "its commander";
				if (bc)
					cn = bc.GetCommanderUID();
				back = "It feeds " + cn + " now.";
			}
			m_sOnReason = string.Empty;
			Card(0, Title() + " is back on", back, ResolveCommander());
		}
	}

	string StateSentence()
	{
		if (m_bStopped)
			return "stopped, " + m_sOffDetail;
		if (m_bGMOff)
			return "turned off by the GM";
		if (m_bOff)
			return "off, " + m_sOffDetail;
		if (!m_sWaitReason.IsEmpty())
			return "waiting, " + m_sWaitDetail;
		float left = m_fNextAllowed - DCO_Util.NowS();
		if (left > 0)
		{
			if (left < 60)
				return "cooldown, next spawn in under a minute";
			int mins = Math.Ceil(left / 60.0);
			return string.Format("cooldown, next spawn in about %1 %2", mins, Plural(mins, "minute", "minutes"));
		}
		return "ready";
	}

	protected string SafetyBlock(Faction faction, bool force, out string detail)
	{
		vector origin = GetOwner().GetOrigin();
		float safeSq = m_fSafeRadius * m_fSafeRadius;

		if (m_fSafeRadius > 0 && !force)
		{
			array<int> players = {};
			GetGame().GetPlayerManager().GetPlayers(players);
			foreach (int pid : players)
			{
				IEntity p = GetGame().GetPlayerManager().GetPlayerControlledEntity(pid);
				if (p && vector.DistanceSqXZ(p.GetOrigin(), origin) <= safeSq)
				{
					detail = string.Format("Player %1 is %2 m away, inside the %3 m safe radius", GetGame().GetPlayerManager().GetPlayerName(pid), Math.Round(vector.DistanceXZ(p.GetOrigin(), origin)), Math.Round(m_fSafeRadius));
					return "player_near";
				}
			}
		}

		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetForFactionObj(faction);
		int cap = 0;
		if (cfg)
			cap = cfg.GetSpawnerFactionAICap();
		if ((m_fSafeRadius <= 0 || force) && cap <= 0)
			return string.Empty;

		AIWorld world = GetGame().GetAIWorld();
		if (!world)
			return string.Empty;

		array<AIAgent> agents = {};
		world.GetAIAgents(agents);
		int count;
		int enemies;
		foreach (AIAgent a : agents)
		{
			ChimeraCharacter ch = ChimeraCharacter.Cast(a.GetControlledEntity());
			if (!ch)
				continue;
			FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(ch.FindComponent(FactionAffiliationComponent));
			if (!fac || !fac.GetAffiliatedFaction())
				continue;
			Faction f = fac.GetAffiliatedFaction();
			if (f == faction)
			{
				count++;
				continue;
			}
			if (!force && m_fSafeRadius > 0 && !f.IsFactionFriendly(faction) && vector.DistanceSqXZ(ch.GetOrigin(), origin) <= safeSq)
				enemies++;
		}

		if (enemies > 0)
		{
			detail = string.Format("%1 %2 inside the %3 m safe radius", enemies, Plural(enemies, "enemy is", "enemies are"), Math.Round(m_fSafeRadius));
			return "enemy_near";
		}
		if (cap > 0 && count >= cap)
		{
			detail = string.Format("%1 of %2 AI soldiers of this faction are already in play", count, cap);
			return "faction_cap";
		}
		return string.Empty;
	}

	protected static bool Usable(DCO_SpawnerGroupEntry e)
	{
		return e && (!e.m_sPrefab.IsEmpty() || e.IsPreset()) && e.m_iWeight > 0 && e.HasBudget();
	}

	protected bool AnyBudget()
	{
		if (!m_aGroups || m_aGroups.IsEmpty())
			return true;
		foreach (DCO_SpawnerGroupEntry e : m_aGroups)
		{
			if (Usable(e))
				return true;
		}
		return false;
	}

	static int ExperienceCostPct(DCO_ESpawnerExperience e)
	{
		if (e == DCO_ESpawnerExperience.RECRUIT)
			return 70;
		if (e == DCO_ESpawnerExperience.VETERAN)
			return 140;
		if (e == DCO_ESpawnerExperience.ELITE)
			return 180;
		return 100;
	}

	protected static ref map<string, int> s_mVehCost;

	static int VehicleCost(ResourceName vehicle)
	{
		if (vehicle.IsEmpty())
			return 0;
		if (!s_mVehCost)
			s_mVehCost = new map<string, int>();
		int cached;
		if (s_mVehCost.Find(vehicle, cached))
			return cached;
		int byLabel = VehicleCostByLabel(vehicle);
		if (byLabel > 0)
		{
			s_mVehCost.Set(vehicle, byLabel);
			return byLabel;
		}
		int byName = VehicleCostByName(vehicle);
		s_mVehCost.Set(vehicle, byName);
		return byName;
	}

	protected static int VehicleCostByLabel(ResourceName vehicle)
	{
		FactionManager fm = GetGame().GetFactionManager();
		if (!fm)
			return 0;
		array<Faction> factions = {};
		fm.GetFactionsList(factions);
		foreach (Faction f : factions)
		{
			SCR_Faction sf = SCR_Faction.Cast(f);
			if (!sf)
				continue;
			SCR_EntityCatalog cat = sf.GetFactionEntityCatalogOfType(EEntityCatalogType.VEHICLE, false);
			if (!cat)
				continue;
			array<SCR_EntityCatalogEntry> list = {};
			cat.GetEntityList(list);
			foreach (SCR_EntityCatalogEntry e : list)
			{
				if (!e || e.GetPrefab() != vehicle)
					continue;
				string n = vehicle;
				n.ToLower();
				bool armor = e.HasEditableEntityLabel(EEditableEntityLabel.VEHICLE_APC) || e.HasEditableEntityLabel(EEditableEntityLabel.TRAIT_ARMOR);
				if (armor && (n.Contains("tank") || n.Contains("t72") || n.Contains("m1a")))
					return 16;
				if (armor)
					return 8;
				if (e.HasEditableEntityLabel(EEditableEntityLabel.TRAIT_ARMED) || e.HasEditableEntityLabel(EEditableEntityLabel.VEHICLE_TURRET))
					return 4;
				return 2;
			}
		}
		return 0;
	}

	protected static int VehicleCostByName(ResourceName vehicle)
	{
		string n = vehicle;
		n.ToLower();
		if (n.Contains("t72") || n.Contains("t-72") || n.Contains("t55") || n.Contains("m1a") || n.Contains("m60a") || n.Contains("tank"))
			return 16;
		if (n.Contains("btr") || n.Contains("bmp") || n.Contains("brdm") || n.Contains("m113") || n.Contains("lav") || n.Contains("m2a") || n.Contains("apc"))
			return 8;
		if (n.Contains("pkm") || n.Contains("dshkm") || n.Contains("_m2") || n.Contains("mg") || n.Contains("armed") || n.Contains("m60") || n.Contains("spg"))
			return 4;
		return 2;
	}

	protected int CrewCount(DCO_SpawnerGroupEntry e)
	{
		if (!m_bCrewed || e.m_sVehicle.IsEmpty())
			return 0;
		if (VehicleCost(e.m_sVehicle) >= 4)
			return 2;
		return 1;
	}

	static int CostOf(DCO_SpawnerGroupEntry e, int soldiers)
	{
		if (e.m_iCostOverride >= 0)
			return e.m_iCostOverride;
		float c = soldiers;
		c = c * ExperienceCostPct(e.m_eExperience) * 0.01;
		if (e.m_eMorale == DCO_ESpawnerMorale.LOW)
			c *= 0.8;
		if (e.m_eShape == DCO_ESpawnerShape.REDUCED)
			c *= 0.8;
		return Math.Round(c) + VehicleCost(e.m_sVehicle);
	}

	protected static int NeedBit(DCO_ESpawnerRole role)
	{
		if (role == DCO_ESpawnerRole.AT)
			return 1;
		if (role == DCO_ESpawnerRole.ASSAULT)
			return 2;
		if (role == DCO_ESpawnerRole.FIRE_TEAM || role == DCO_ESpawnerRole.RECON)
			return 4;
		if (role == DCO_ESpawnerRole.MOTORIZED)
			return 8;
		return 0;
	}

	protected static string NeedName(int bit)
	{
		if (bit == 1)
			return "enemy_armor";
		if (bit == 2)
			return "attack_objective";
		if (bit == 4)
			return "unclear_contacts";
		if (bit == 8)
			return "far_objective";
		return "default";
	}

	protected int ReadNeeds(AICommander_BaseComponent cmd)
	{
		int needs;
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (tr)
		{
			int armor;
			int unclear;
			foreach (CMD_ThreatEntry t : tr.GetThreats())
			{
				if (!t)
					continue;
				if (t.m_iArmor > 0 || t.m_bArmorSeen)
					armor++;
				if (t.m_bNeedsRecon && !t.m_bReconSent)
					unclear++;
			}

			int antiArmor;
			foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
			{
				if (g && !g.IsPlayerGroup() && (g.HasAT() || g.IsArmor()))
					antiArmor++;
			}
			if (armor > antiArmor)
				needs |= 1;
			if (unclear > 0)
				needs |= 4;
		}

		FactionKey fk = cmd.GetCommanderFactionKey();
		vector origin = GetOwner().GetOrigin();
		float nearest = float.MAX;
		int enemyObj;
		foreach (CMD_AICommanderObjectiveComponent o : cmd.GetObjectiveList())
		{
			if (!o || !o.GetOwner() || o.IsCapturedBy(fk))
				continue;
			enemyObj++;
			nearest = Math.Min(nearest, vector.DistanceXZ(o.GetOwner().GetOrigin(), origin));
		}
		if (enemyObj > 0)
			needs |= 2;
		if (enemyObj > 0 && nearest > 1500.0)
			needs |= 8;

		return needs;
	}

	protected static float ChoiceScore(DCO_SpawnerGroupEntry e, int needs, out string why)
	{
		DCO_ESpawnerRole role = e.ResolveRole();
		int bit = NeedBit(role);
		float score = 1;
		why = "default";
		if (role == DCO_ESpawnerRole.ASSAULT)
			score = 1.2;
		if (bit != 0 && (needs & bit) != 0)
		{
			score += 3;
			why = NeedName(bit);
			if (role == DCO_ESpawnerRole.RECON)
				score += 0.5;
		}
		if ((needs & 16) != 0)
		{
			score += (12 - Math.Max(e.m_iSoldiers, 1)) * 0.25;
			if (why == "default")
				why = "low_manpower";
		}
		return score + 0.25 * e.m_iWeight + Math.RandomFloat(0, 0.3);
	}

	protected DCO_SpawnerGroupEntry PickEntry(Faction faction, int needs, out string why)
	{
		why = "random";
		if (m_aGroups && !m_aGroups.IsEmpty())
		{
			if (m_bCommanderChooses)
			{
				DCO_SpawnerGroupEntry best;
				float bestScore = -1;
				string bestWhy;
				foreach (DCO_SpawnerGroupEntry c : m_aGroups)
				{
					if (!Usable(c))
						continue;
					if (c.m_iSoldiers < 0)
					{
						Resource cres = Resource.Load(c.m_sPrefab);
						if (cres && cres.IsValid())
							c.m_iSoldiers = PrefabSoldiers(cres);
						else
							c.m_iSoldiers = 0;
					}
					string cw;
					float s = ChoiceScore(c, needs, cw);
					if (s > bestScore)
					{
						bestScore = s;
						best = c;
						bestWhy = cw;
					}
				}
				why = bestWhy;
				return best;
			}

			int total;
			foreach (DCO_SpawnerGroupEntry e : m_aGroups)
			{
				if (Usable(e))
					total += e.m_iWeight;
			}
			if (total <= 0)
				return null;

			int roll = Math.RandomInt(0, total);
			foreach (DCO_SpawnerGroupEntry e2 : m_aGroups)
			{
				if (!Usable(e2))
					continue;
				roll -= e2.m_iWeight;
				if (roll < 0)
					return e2;
			}
			return null;
		}

		why = "catalog";

		SCR_Faction scrFaction = SCR_Faction.Cast(faction);
		if (!scrFaction)
			return null;
		SCR_EntityCatalog catalog = scrFaction.GetFactionEntityCatalogOfType(EEntityCatalogType.GROUP, false);
		if (!catalog)
			return null;
		array<SCR_EntityCatalogEntry> list = {};
		catalog.GetEntityList(list);
		if (list.IsEmpty())
			return null;

		DCO_SpawnerGroupEntry pick = new DCO_SpawnerGroupEntry();
		pick.m_sPrefab = list.GetRandomElement().GetPrefab();
		pick.m_iWeight = 1;
		pick.m_iBudget = -1;
		pick.m_eExperience = m_eDefExperience;
		pick.m_eMorale = m_eDefMorale;
		pick.m_eShape = m_eDefShape;
		pick.m_iArrivalMin = m_iDefArrivalMin;
		pick.m_iCostOverride = -1;
		return pick;
	}

	protected static int PrefabSoldiers(Resource res)
	{
		IEntitySource src = res.GetResource().ToEntitySource();
		array<ResourceName> slots = {};
		if (src && src.Get("m_aUnitPrefabSlots", slots) && !slots.IsEmpty())
			return slots.Count();
		return 1;
	}

	protected bool SpawnGroup(AICommander_BaseComponent cmd, DCO_SpawnerOrder order, Resource res, int soldiers)
	{
		DCO_SpawnerGroupEntry entry = order.m_Entry;
		vector origin = GetOwner().GetOrigin();
		vector pos;
		if (!SCR_WorldTools.FindEmptyTerrainPosition(pos, origin, m_fSpawnRadius, 2, 2))
			pos = origin;

		SCR_AIGroup grp;
		if (entry.IsPreset())
			grp = SpawnFromMembers(entry, cmd.GetCommanderFactionKey(), pos);
		else
		{
			EntitySpawnParams params = new EntitySpawnParams();
			params.TransformMode = ETransformMode.WORLD;
			params.Transform[3] = pos;
			grp = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(res, GetOwner().GetWorld(), params));
		}
		if (!grp)
			return false;
		m_LastSpawnGrp = grp;

		IEntity vehicle;
		if (!entry.m_sVehicle.IsEmpty())
		{
			Resource vres = Resource.Load(entry.m_sVehicle);
			vector vpos;
			if (!SCR_WorldTools.FindEmptyTerrainPosition(vpos, pos, 25, 4, 3))
				vpos = pos + Vector(8, 0, 0);
			if (vres && vres.IsValid())
			{
				EntitySpawnParams vparams = new EntitySpawnParams();
				vparams.TransformMode = ETransformMode.WORLD;
				vparams.Transform[3] = vpos;
				vehicle = GetGame().SpawnEntityPrefab(vres, GetOwner().GetWorld(), vparams);
			}
		}

		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_spawn spawner=%1 cmd=%2 group=%3 prefab=%4 soldiers=%5 vehicle=%6 trigger=%7 stock=%8",
			LogName(), cmd.GetCommanderUID(), grp, entry.m_sPrefab, soldiers, vehicle != null, order.m_sTrigger, StockLeftText(cmd)));
		DCO_Events.Spawner("spawner_spawn", string.Format("spawner=%1 cmd=%2 prefab=%3 soldiers=%4 vehicle=%5", Name(), cmd.GetCommanderUID(), entry.m_sPrefab, soldiers, vehicle != null));
		GetGame().GetCallqueue().CallLater(AssignLater, 500, false, grp, cmd.GetCommanderUID(), vehicle);
		if (vehicle && m_bCrewed)
			GetGame().GetCallqueue().CallLater(CrewAndBoard, 1500, false, grp, vehicle, cmd.GetCommanderFactionKey(), 0);
		GetGame().GetCallqueue().CallLater(ApplyQuality, 2000, false, grp, entry.m_eExperience, entry.m_eMorale, entry.m_eShape, soldiers, 0);
		return true;
	}

	protected SCR_AIGroup SpawnFromMembers(DCO_SpawnerGroupEntry e, FactionKey fk, vector pos)
	{
		Resource gres = Resource.Load("{000CD338713F2B5A}Prefabs/AI/Groups/Group_Base.et");
		if (!gres || !gres.IsValid())
			return null;
		EntitySpawnParams gp = new EntitySpawnParams();
		gp.TransformMode = ETransformMode.WORLD;
		gp.Transform[3] = pos;
		SCR_AIGroup grp = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(gres, GetOwner().GetWorld(), gp));
		if (!grp)
			return null;
		grp.SetDeleteWhenEmpty(false);
		FactionManager fm = GetGame().GetFactionManager();
		if (fm && fm.GetFactionByKey(fk))
			grp.SetFaction(fm.GetFactionByKey(fk));
		array<string> types = {};
		string ordered = DCO_GroupPresets.LeaderFirst(e.m_sMembers);
		ordered.Split(",", types, true);
		int added;
		foreach (int i, string type : types)
		{
			ResourceName pr = DCO_GroupPresets.MemberPrefab(fk, type);
			if (pr.IsEmpty())
				continue;
			Resource cres = Resource.Load(pr);
			if (!cres || !cres.IsValid())
				continue;
			vector cpos;
			if (!SCR_WorldTools.FindEmptyTerrainPosition(cpos, pos, 8, 1, 2))
				cpos = pos + Vector(1.2 * i, 0, 0);
			EntitySpawnParams cp = new EntitySpawnParams();
			cp.TransformMode = ETransformMode.WORLD;
			cp.Transform[3] = cpos;
			IEntity ch = GetGame().SpawnEntityPrefab(cres, GetOwner().GetWorld(), cp);
			if (ch && grp.AddAIEntityToGroup(ch))
				added++;
		}
		grp.SetDeleteWhenEmpty(true);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_preset_spawn spawner=%1 preset=%2 members=%3 added=%4 faction=%5", LogName(), e.m_sPresetName, types.Count(), added, fk));
		return grp;
	}

	protected void CrewAndBoard(SCR_AIGroup grp, IEntity vehicle, FactionKey fk, int attempt)
	{
		if (!grp || !vehicle)
			return;
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		if (agents.IsEmpty() && attempt < 4)
		{
			GetGame().GetCallqueue().CallLater(CrewAndBoard, 1500, false, grp, vehicle, fk, attempt + 1);
			return;
		}
		BaseCompartmentManagerComponent cm = BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(BaseCompartmentManagerComponent));
		int turrets;
		if (cm)
		{
			array<BaseCompartmentSlot> slots = {};
			cm.GetCompartments(slots);
			foreach (BaseCompartmentSlot sl : slots)
			{
				if (TurretCompartmentSlot.Cast(sl))
					turrets++;
			}
		}
		ResourceName crewPrefab = DCO_GroupPresets.CrewPrefab(fk);
		int crewMade;
		array<ECompartmentType> seats = {ECompartmentType.PILOT};
		if (turrets > 0)
			seats.Insert(ECompartmentType.TURRET);
		Resource cres;
		if (!crewPrefab.IsEmpty())
			cres = Resource.Load(crewPrefab);
		foreach (ECompartmentType seat : seats)
		{
			if (!cres || !cres.IsValid())
				break;
			EntitySpawnParams cp = new EntitySpawnParams();
			cp.TransformMode = ETransformMode.WORLD;
			cp.Transform[3] = vehicle.GetOrigin() + Vector(3 + crewMade, 0, 2);
			IEntity ch = GetGame().SpawnEntityPrefab(cres, GetOwner().GetWorld(), cp);
			if (!ch)
				continue;
			grp.AddAIEntityToGroup(ch);
			SCR_CompartmentAccessComponent acc = SCR_CompartmentAccessComponent.Cast(ch.FindComponent(SCR_CompartmentAccessComponent));
			if (acc && acc.MoveInVehicle(vehicle, seat))
				crewMade++;
		}
		int boarded;
		int walking;
		foreach (AIAgent a : agents)
		{
			IEntity m = a.GetControlledEntity();
			if (!m)
				continue;
			SCR_CompartmentAccessComponent macc = SCR_CompartmentAccessComponent.Cast(m.FindComponent(SCR_CompartmentAccessComponent));
			if (macc && macc.MoveInVehicle(vehicle, ECompartmentType.CARGO))
				boarded++;
			else
				walking++;
		}
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_crewed spawner=%1 grp=%2 vehicle=%3 crew=%4 turrets=%5 boarded=%6 walking=%7 crew_prefab=%8",
			LogName(), DCO_Log.IdGroup(grp), vehicle.GetPrefabData().GetPrefabName(), crewMade, turrets, boarded, walking, crewPrefab));
	}

	protected void AssignLater(SCR_AIGroup grp, string uid, IEntity vehicle)
	{
		if (!grp)
			return;

		DCO_GroupUtilityComponent util = DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));
		if (!util)
		{
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_assign group=%1 cmd=%2 ok=0 reason=no_utility", grp, uid));
			return;
		}

		if (vehicle)
			util.DCO_AddSpawnVehicle(vehicle);
		util.DCO_AssignFromSpawner(uid);
		bool ok = util.GetMyCommander() != null;
		string how = "direct";
		if (!ok)
			how = "retry";
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_assign group=%1 cmd=%2 ok=%3 mode=%4", grp, uid, ok, how));
	}

	protected static DCO_AISKILL SkillOf(DCO_ESpawnerExperience e)
	{
		if (e == DCO_ESpawnerExperience.RECRUIT)
			return DCO_AISKILL.ROOKIE;
		if (e == DCO_ESpawnerExperience.VETERAN)
			return DCO_AISKILL.VETERAN;
		if (e == DCO_ESpawnerExperience.ELITE)
			return DCO_AISKILL.EXPERT;
		return DCO_AISKILL.REGULAR;
	}

	protected void ApplyQuality(SCR_AIGroup grp, DCO_ESpawnerExperience exp, DCO_ESpawnerMorale morale, DCO_ESpawnerShape shape, int expected, int attempt)
	{
		if (!grp)
			return;
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		if (agents.Count() < expected && attempt < 4)
		{
			GetGame().GetCallqueue().CallLater(ApplyQuality, 2000, false, grp, exp, morale, shape, expected, attempt + 1);
			return;
		}

		float moraleMul = 1.0;
		float moraleBase = 0;
		if (morale == DCO_ESpawnerMorale.LOW)
		{
			moraleMul = 0.8;
			moraleBase = 0.5;
		}
		else if (morale == DCO_ESpawnerMorale.HIGH)
			moraleMul = 1.2;

		int magsRemoved;
		foreach (AIAgent a : agents)
		{
			DCO_AIConfigComponent conf = DCO_AIConfigComponent.Cast(a.FindComponent(DCO_AIConfigComponent));
			if (conf)
				conf.SetAISkill(SkillOf(exp));
			SCR_AIUtilityComponent u = SCR_AIUtilityComponent.Cast(a.FindComponent(SCR_AIUtilityComponent));
			if (u && u.GetMoraleSystem())
				u.GetMoraleSystem().DCO_SetMoraleLevel(moraleMul, moraleBase);
			if (shape == DCO_ESpawnerShape.REDUCED)
				magsRemoved += ReduceShape(a.GetControlledEntity());
		}

		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("spawner_quality grp=%1 exp=%2 morale=%3 shape=%4 members=%5 mags_removed=%6",
			DCO_Log.IdGroup(grp), ExperienceName(exp), typename.EnumToString(DCO_ESpawnerMorale, morale), typename.EnumToString(DCO_ESpawnerShape, shape), agents.Count(), magsRemoved));
	}

	protected int ReduceShape(IEntity ch)
	{
		if (!ch)
			return 0;
		CharacterStaminaComponent stamina = CharacterStaminaComponent.Cast(ch.FindComponent(CharacterStaminaComponent));
		if (stamina)
			stamina.AddStamina(-0.5);

		SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(ch.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inv)
			return 0;
		array<IEntity> items = {};
		inv.GetItems(items);
		array<IEntity> mags = {};
		foreach (IEntity it : items)
		{
			if (!it || !it.FindComponent(MagazineComponent))
				continue;
			if (it.GetParent() && it.GetParent().FindComponent(BaseWeaponComponent))
				continue;
			string n;
			if (it.GetPrefabData())
				n = it.GetPrefabData().GetPrefabName();
			n.ToLower();
			if (n.Contains("rocket") || n.Contains("grenade") || n.Contains("smoke") || n.Contains("flare"))
				continue;
			mags.Insert(it);
		}
		int remove = Math.Floor(0.4 * mags.Count());
		if (mags.Count() - remove < 1)
			remove = Math.Max(mags.Count() - 1, 0);
		int removed;
		for (int i = 0; i < remove; i++)
		{
			if (inv.TryDeleteItem(mags[i]))
				removed++;
		}
		return removed;
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class DCO_SpawnerSpawnNowContextAction : SCR_SelectedEntitiesContextAction
{
	override int GetParam()
	{
		return GetGame().GetPlayerController().GetPlayerId();
	}

	override bool CanBeShown(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return selectedEntity && selectedEntity.GetOwner() && selectedEntity.GetOwner().FindComponent(DCO_CommanderSpawnerComponent) != null;
	}

	override bool CanBePerformed(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return CanBeShown(selectedEntity, cursorWorldPosition, flags);
	}

	override void Perform(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{
		foreach (SCR_EditableEntityComponent entity : selectedEntities)
		{
			if (!entity || !entity.GetOwner())
				continue;
			DCO_CommanderSpawnerComponent spawner = DCO_CommanderSpawnerComponent.Cast(entity.GetOwner().FindComponent(DCO_CommanderSpawnerComponent));
			if (spawner)
				spawner.SpawnNow(param);
		}
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class DCO_SpawnerSpawnAnywayContextAction : DCO_SpawnerSpawnNowContextAction
{
	override bool CanBeShown(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		if (!selectedEntity || !selectedEntity.GetOwner())
			return false;
		DCO_CommanderSpawnerComponent s = DCO_CommanderSpawnerComponent.Cast(selectedEntity.GetOwner().FindComponent(DCO_CommanderSpawnerComponent));
		return s && s.GetState() == DCO_ESpawnerState.WAITING;
	}

	override void Perform(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{
		foreach (SCR_EditableEntityComponent entity : selectedEntities)
		{
			if (!entity || !entity.GetOwner())
				continue;
			DCO_CommanderSpawnerComponent spawner = DCO_CommanderSpawnerComponent.Cast(entity.GetOwner().FindComponent(DCO_CommanderSpawnerComponent));
			if (spawner)
				spawner.SpawnAnyway(param);
		}
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class DCO_SpawnerToggleContextAction : DCO_SpawnerSpawnNowContextAction
{
	override bool CanBeShown(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return selectedEntity && selectedEntity.GetOwner() && selectedEntity.GetOwner().FindComponent(DCO_CommanderSpawnerComponent) != null;
	}

	override void Perform(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{
		foreach (SCR_EditableEntityComponent entity : selectedEntities)
		{
			if (!entity || !entity.GetOwner())
				continue;
			DCO_CommanderSpawnerComponent spawner = DCO_CommanderSpawnerComponent.Cast(entity.GetOwner().FindComponent(DCO_CommanderSpawnerComponent));
			if (spawner)
				spawner.SetGMOff(!spawner.IsGMOff(), param);
		}
	}
}
