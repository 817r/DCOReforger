enum DCO_ECaptureStatus
{
	IDLE      = 0,
	CAPTURING = 1,
	CONTESTED = 2
}

[ComponentEditorProps(category: "GameScripted/AI/AICommander", description: "Component for AI Commander Objective")]
class CMD_AICommanderObjectiveComponentClass : ScriptComponentClass
{
}

class CMD_AICommanderObjectiveComponent : ScriptComponent
{
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Entity prefab with AICommander_ManagerComponent. Spawned automatically (server) if this entity initialises before a manager exists in the world. Empty = no auto spawn.", "et", category: "Commander Manager")]
	protected ResourceName m_sManagerPrefab;

	[Attribute("50.0", UIWidgets.EditBox, "Base strategic value (0-100). Higher = more important.", category: "Objective")]
	protected float m_fBaseValue;

	[Attribute("", UIWidgets.EditBox, "Objective name.", category: "Objective"), RplProp()]
	protected string m_sObjectiveName;

	protected string m_sDCOFallbackName;

	[Attribute("150.0", UIWidgets.EditBox, "Radius (m) for detecting enemies around this objective.", category: "Objective")]
	protected float m_fThreatRadius;

	[Attribute("200.0", UIWidgets.EditBox, "Radius (m) for checking friendly presence.", category: "Objective")]
	protected float m_fFriendlyRadius;

	[Attribute("30.0", UIWidgets.Slider, "Radius of the Objective", "1.0 300.0 1.0"), RplProp(onRplName: "OnRadiusChanged")]
	protected float m_fRadius;

	[Attribute("400.0", UIWidgets.EditBox, "Radius (m) of intel coverage this objective gives to OTHER nearby objectives. Only relevant if ObjectiveType == RECON.", category: "Intel")]
	protected float m_fIntelCoverageRadius;

	[Attribute("2500.0", UIWidgets.EditBox, "Distance (m) from the commander at which the proximity bonus runs out (0 at this distance, full at distance 0).", category: "Priority")]
	protected float m_fMaxRelevantDistance;

	[Attribute("20.0", UIWidgets.EditBox, "Maximum penalty for objectives beyond Max Relevant Distance. Larger = commander less willing to take far objectives. 0 = distance makes no difference.", category: "Priority")]
	protected float m_fDistantFalloffWeight;

	[Attribute("100.0", UIWidgets.EditBox, "Maximum points from proximity, reached at zero distance to the commander. This mainly decides objective order. Raise if near objectives still lose to far ones with a large base value.", category: "Priority")]
	protected float m_fProximityWeight;

	[Attribute("40.0", UIWidgets.EditBox, "Points per enemy unit, BEFORE distance scaling. Threat is a modifier; if it makes hostile objectives always win regardless of distance, it is too high.", category: "Priority")]
	protected float m_fThreatWeight;

	[Attribute("2.0", UIWidgets.EditBox, "Exponent of threat decay relative to proximity. 1 = threat decays as fast as proximity. 2 = much faster (at half the relevant radius, proximity 50% but threat 25%). Larger = cares less about far enemies.", category: "Priority")]
	protected float m_fThreatFalloffPower;

	[Attribute("0.05", UIWidgets.Range, "Floor of the threat scale. Keeps enemies beyond the relevant radius from counting as zero; otherwise two equal far objectives could not be told apart even if one is full of enemies.", params: "0 1 0.01", category: "Priority")]
	protected float m_fThreatFalloffFloor;

	[Attribute("8", UIWidgets.EditBox, "Cap on the enemy count used for scoring. Without it the threat decay floor (meant only as a tie breaker) is multiplied by an unlimited count, so a 50 unit garrison across the map can outscore a near objective. Raise if large battles get too little priority; lower if the commander is still pulled toward far garrisons.", category: "Priority")]
	protected int m_iThreatCountCap;

	[Attribute("0", UIWidgets.ComboBox, "Type of this objective.", "", ParamEnumArray.FromEnum(CMD_EObjectiveType))]
	CMD_EObjectiveType m_eObjectiveType;

	[Attribute("60.0", UIWidgets.EditBox, "Seconds needed to capture (groups must be in the area).", category: "Objective")]
	protected float m_fCaptureHoldDuration;

	[Attribute("20.0", UIWidgets.EditBox, "Seconds the radius must be clear of enemies before the objective counts as uncontested. The commander uses this to decide between staging and moving straight in.", category: "Objective")]
	protected float m_fUncontestedDwell;

	[Attribute("5.0", UIWidgets.EditBox, "Interval (s) at which the objective counts the units in its own radius. Smaller = more responsive but more spatial queries.", category: "Objective")]
	protected float m_fPresenceScanInterval;

	[Attribute("8", UIWidgets.EditBox, "Units needed for the FULL control shift rate. Below this the rate is proportional: 1 unit of 8 gives 1/8 of the rate. Raise if large objectives should need large forces.", category: "Objective Control")]
	protected int m_iFullSpeedUnits;

	[Attribute("100.0", UIWidgets.Range, "Control percent needed to become OWNER of this objective.", params: "0 100 1", category: "Objective Control")]
	protected float m_fOwnershipGainThreshold;

	[Attribute("0", UIWidgets.CheckBox, "Retain: hold this objective until overrun. Doctrine withdrawal rules (Delay, Fallback, Withdraw) are ignored.", category: "Objective")]
	protected bool m_bRetain;

	[Attribute("0", UIWidgets.CheckBox, "Operation primary: an anchor objective. When the commander picks one of its secondaries, the operation is planned around this primary.", category: "Operation")]
	protected bool m_bDCOIsPrimary;

	[Attribute("600", UIWidgets.EditBox, "Objectives within this radius (m) of this primary are its secondaries when Linked Secondaries is empty.", category: "Operation")]
	protected float m_fDCOSecondaryRadius;

	[Attribute(desc: "Entity names of objectives that are this primary's secondaries. When not empty this list is used instead of the radius.", category: "Operation")]
	protected ref array<string> m_aDCOLinkedSecondaries;

	bool DCO_IsPrimary()						{ return m_bDCOIsPrimary; }
	void DCO_SetIsPrimary(bool v)				{ m_bDCOIsPrimary = v; }
	float DCO_SecondaryRadius()					{ return m_fDCOSecondaryRadius; }
	void DCO_SetSecondaryRadius(float v)		{ m_fDCOSecondaryRadius = Math.Max(0, v); }
	array<string> DCO_LinkedSecondaries()		{ return m_aDCOLinkedSecondaries; }

	[Attribute("1", UIWidgets.ComboBox, "Strategic value: 0 High (x1.67 base value), 1 Moderate (unchanged), 2 Low (x0.5), 3 No (not a goal on its own; only threats near it still count).", enums: { ParamEnum("High", "0"), ParamEnum("Moderate", "1"), ParamEnum("Low", "2"), ParamEnum("No", "3") }, category: "Objective")]
	protected int m_iStrategicValue;

	[Attribute("0", UIWidgets.ComboBox, "Category: what kind of place this objective is.", enums: { ParamEnum("Town", "0"), ParamEnum("Military installation", "1"), ParamEnum("Supply depot", "2"), ParamEnum("Terrain", "3") }, category: "Objective")]
	protected int m_iCategory;

	[Attribute("50.0", UIWidgets.Range, "Control percent at which owner status is LOST. The gap to the Gain Threshold above is the hysteresis; without it owner status flickers on every small disturbance.", params: "0 100 1", category: "Objective Control")]
	protected float m_fOwnershipLossThreshold;

	[Attribute("1.0", UIWidgets.EditBox, "Percent per second that control drifts back to the last owner while the radius is empty. 0 = control freezes when nobody is there.", category: "Objective Control")]
	protected float m_fEmptyDecayRate;

	[Attribute("1", UIWidgets.CheckBox, "All factions in the Faction Manager may contest this objective.", category: "Objective Control")]
	protected bool m_bAllowAllFactions;

	[Attribute(desc: "If Allow All Factions is off, only faction keys in this list may contest. Other factions are treated as ABSENT: their units do not block and are not counted.", category: "Objective Control")]
	protected ref array<string> m_aAllowedFactions;

	[Attribute("5.0", UIWidgets.EditBox, "Stalemate: control changes by less than this percent...", category: "Objective Control")]
	protected float m_fStalematePercentThreshold;

	[Attribute("120.0", UIWidgets.EditBox, "...for this many seconds before it counts as a stalemate.", category: "Objective Control")]
	protected float m_fStalemateSeconds;

	[Attribute(desc: "Commander UID that holds this objective at scenario start. Leave empty for Neutral. Only ONE commander allowed.", category: "Objective Control")]
	protected string m_sInitialOwnerCommanderUID;

	[Attribute("2", UIWidgets.EditBox, "Number of groups assigned to defend after capture.", category: "Objective")]
	protected int m_iDefendGroupCount;

	[Attribute("4", UIWidgets.EditBox, "Number of groups assigned to attack the objective.", category: "Objective")]
	protected int m_iMaxGroupCount;

	[Attribute("", UIWidgets.Auto, "Old list of commander UIDs that ignore this objective. Kept for old worlds; it behaves like DCO Assign Mode 2 with these UIDs. New worlds use DCO Assign Mode + DCO Assign List.", category: "Objective")]
	protected ref array<string> m_sBlacklistedCo;

	[Attribute("0", UIWidgets.EditBox, "Which AI commanders may plan this objective (attack, defend, patrol, recon, operation secondaries). 0 = all commanders, 1 = only the commanders in DCO Assign List, 2 = every commander except those in DCO Assign List.", category: "Objective", params: "0 2 1")]
	protected int m_iDCOAssignMode;

	[Attribute("", UIWidgets.Auto, "Commander UIDs used by DCO Assign Mode.", category: "Objective")]
	protected ref array<string> m_aDCOAssignList;

	[Attribute("", UIWidgets.Auto, "Commander UIDs that only ATTACK this objective (tune obj.role_on = 1). After the capture they hold it until a commander allowed to defend has squads on it, then they leave.", category: "Objective")]
	protected ref array<string> m_aDCOAttackOnly;

	[Attribute("", UIWidgets.Auto, "Commander UIDs that only DEFEND this objective (tune obj.role_on = 1). They defend and retake it after a loss, but never attack it while the enemy holds it from the start.", category: "Objective")]
	protected ref array<string> m_aDCODefendOnly;

	int DCO_RoleOf(string cuid)
	{
		bool atk = m_aDCOAttackOnly && m_aDCOAttackOnly.Contains(cuid);
		bool def = m_aDCODefendOnly && m_aDCODefendOnly.Contains(cuid);
		if (atk == def)
			return 0;
		if (atk)
			return 1;
		return 2;
	}

	void DCO_GetRoleList(int role, notnull array<string> outList)
	{
		outList.Clear();
		if (role == 1 && m_aDCOAttackOnly)
			outList.Copy(m_aDCOAttackOnly);
		else if (role == 2 && m_aDCODefendOnly)
			outList.Copy(m_aDCODefendOnly);
	}

	void DCO_SetRoleList(int role, notnull array<string> list, string by, int playerId)
	{
		if (role == 1)
		{
			if (!m_aDCOAttackOnly)
				m_aDCOAttackOnly = {};
			m_aDCOAttackOnly.Copy(list);
		}
		else if (role == 2)
		{
			if (!m_aDCODefendOnly)
				m_aDCODefendOnly = {};
			m_aDCODefendOnly.Copy(list);
		}
		else
			return;
		string roleName = "attack_only";
		if (role == 2)
			roleName = "defend_only";
		DCO_Perf.Count("obj_role:set");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("obj_role_set obj=%1 role=%2 list=%3 by=%4 player=%5", GetLogName(), roleName, DCO_JoinList(list), by, playerId));
	}

	[Attribute("", UIWidgets.Auto, "Old field. Used as Initial Owner Commander UID (first entry) only when that field is empty. Use Initial Owner Commander UID instead.", category: "Objective")]
	protected ref array<string> m_sCapturedCo;

	[Attribute("0", UIWidgets.EditBox, "Debug for this objective.", category: "Objective Debug")]
	protected bool m_bDebugMode;

	protected ref map<FactionKey, ref array<ref DCO_SectorGarrison>> m_mSectorGarrison = new map<FactionKey, ref array<ref DCO_SectorGarrison>>();
	protected ref map<FactionKey, int>   m_mSectorCount  = new map<FactionKey, int>();
	protected ref map<FactionKey, float> m_mSectorOffset = new map<FactionKey, float>();

	protected ref array<ref Shape> m_aDebugShapes = new array<ref Shape>();

	protected ref array<ref DebugTextWorldSpace> m_aDebugTexts = new array<ref DebugTextWorldSpace>();

	[Attribute("0.5", UIWidgets.EditBox, "Interval (s) between debug overlay redraws for this objective.", category: "Objective Debug")]
	protected float m_fDebugRefreshInterval;

	protected float m_fDebugTimer = 0.0;

	protected float m_fLastProgressTime   = 0.0;
	protected float m_fStaleStartTime     = 0.0;

	[Attribute("300.0", UIWidgets.EditBox, "Seconds without progress before it counts as a stalemate.", category: "Objective")]
	protected float m_fStalemateThreshold;

	ref map<FactionKey, float> m_mCaptureStartTime = new map<FactionKey, float>();

	protected ref map<FactionKey, float> m_mCaptureHeld     = new map<FactionKey, float>();
	protected ref map<FactionKey, float> m_mCaptureLastTick = new map<FactionKey, float>();

	protected ref map<FactionKey, int>   m_mCaptureStatus     = new map<FactionKey, int>();

	protected ref map<FactionKey, float> m_mLastEnemySeenTime = new map<FactionKey, float>();

	protected ref map<FactionKey, float> m_mControl = new map<FactionKey, float>();

	protected FactionKey m_sOwnerFaction;
	protected FactionKey m_sLastOwnerFaction;
	protected string     m_sOwnerCommanderUID = string.Empty;

	protected ref map<string, float>      m_mCommanderContribution = new map<string, float>();
	protected ref map<string, FactionKey> m_mCommanderFaction      = new map<string, FactionKey>();

	protected float m_fLastControlChangeTime = 0.0;
	protected ref map<FactionKey, float> m_mStalemateControl    = new map<FactionKey, float>();
	protected ref map<FactionKey, float> m_mStalemateChangeTime = new map<FactionKey, float>();
	protected bool  m_bInitialOwnerResolved  = false;

	protected ref map<FactionKey, int> m_mFactionPresence = new map<FactionKey, int>();
	protected int   m_iTotalPresence  = 0;
	protected float m_fLastScanTime   = 0.0;
	protected float m_fScanTimer      = 0.0;
	protected bool  m_bHasScanned     = false;

	[Attribute("60.0", UIWidgets.EditBox, "Seconds the objective stays ACTIVE after the commander last processed it. After that it is IDLE: presence scans and control shift stop completely. Must be longer than the slowest commander think interval (default 30 s x personality up to 1.5 = 45 s). 0 = gate off, objective always active.", category: "Objective")]
	protected float m_fIdleTimeout;

	protected ref array<FactionKey> m_aCensusFaction = {};
	protected ref array<float>      m_aCensusDistSq  = {};
	protected float m_fCensusTime   = 0.0;
	protected float m_fCensusRadius = 0.0;
	protected bool  m_bCensusValid  = false;

	protected array<FactionKey> m_aCollectFaction;
	protected array<float>      m_aCollectDistSq;
	protected ref set<IEntity>  m_CollectSeen = new set<IEntity>();
	protected vector            m_vCollectCenter;

	protected ref map<FactionKey, int> m_mCountScratch = new map<FactionKey, int>();

	protected float m_fLastProcessedTime     = 0.0;
	protected bool  m_bEverProcessed         = false;
	protected bool  m_bSuppressProcessedMark = false;

	ref map<FactionKey, bool>  m_mIsCaptured       = new map<FactionKey, bool>();

	ref map<FactionKey, CMD_EObjectiveState> m_mObjectiveState = new map<FactionKey, CMD_EObjectiveState>();
	ref map<FactionKey, int> m_mObjectiveAssignedGroup = new map<FactionKey, int>();
	protected ref map<FactionKey, DCO_GroupUtilityComponent> m_mReconGroup = new map<FactionKey, DCO_GroupUtilityComponent>();

	IEntity m_OwnerEntity;

	protected ref map<string, float> m_mCachedScore   = new map<string, float>();
	protected ref map<string, float> m_mScoreCacheAge = new map<string, float>();

	protected ref map<string, string> m_mScoreBreakdown = new map<string, string>();

	protected float m_fLastContestedTime = 0.0;

	float GetRadius()
	{
		return m_fRadius;
	}

	bool IsRetain()
	{
		return m_bRetain;
	}

	int GetStrategicValue()
	{
		return m_iStrategicValue;
	}

	void SetStrategicValue(int value)
	{
		m_iStrategicValue = Math.ClampInt(value, 0, 3);
		m_mScoreCacheAge.Clear();
	}

	int GetCategory()
	{
		return m_iCategory;
	}

	void SetCategory(int value)
	{
		m_iCategory = Math.ClampInt(value, 0, 3);
	}

	float StrategicScale()
	{
		switch (m_iStrategicValue)
		{
			case 0: return 1.0 / 0.6;
			case 2: return 0.3 / 0.6;
			case 3: return 0;
		}
		return 1;
	}

	void SetRetain(bool retain)
	{
		m_bRetain = retain;
	}

	void SetRadius(float radius)
	{
		if (!Replication.IsServer())
			return;

		m_fRadius = Math.Clamp(radius, 1, 300);
		Replication.BumpMe();
		OnRadiusChanged();
	}

	protected void OnRadiusChanged()
	{
		SCR_BaseAreaMeshComponent mesh = SCR_BaseAreaMeshComponent.Cast(GetOwner().FindComponent(SCR_BaseAreaMeshComponent));
		if (mesh)
			mesh.GenerateAreaMesh();
	}

	float GetCaptureProgress(FactionKey fk, float worldTime)
	{
	    return Math.Clamp(GetControl(fk) / 100.0, 0.0, 1.0);
	}

	float GetCaptureHeldSeconds(FactionKey fk)
	{
	    float held;
	    if (m_mCaptureHeld.Find(fk, held))
	        return held;

	    return 0.0;
	}

	int GetCaptureStatus(FactionKey fk)
	{
	    int status;
	    if (m_mCaptureStatus.Find(fk, status))
	        return status;

	    return DCO_ECaptureStatus.IDLE;
	}

	float GetSecondsSinceEnemySeen(FactionKey fk, float worldTime)
	{
	    float lastSeen;
	    if (!m_mLastEnemySeenTime.Find(fk, lastSeen))
	        return -1.0;

	    return worldTime - lastSeen;
	}

	bool IsUncontested(FactionKey fk, float worldTime)
	{
	    MarkProcessed(worldTime);

	    float lastSeen;
	    if (!m_mLastEnemySeenTime.Find(fk, lastSeen))
	        return false;

	    return (worldTime - lastSeen) >= m_fUncontestedDwell;
	}

	float GetUncontestedDwell()
	{
	    return m_fUncontestedDwell;
	}

	int AssessObjective(FactionKey fk, float worldTime)
	{
	    MarkProcessed(worldTime);

	    if (!m_bHasScanned)
	        return GetCaptureStatus(fk);

	    int friendlyCount = GetFactionPresence(fk);
	    int enemyCount    = 0;

	    foreach (FactionKey pk, int pu : m_mFactionPresence)
	    {
	        if (!CanFactionContest(pk) || AreFactionsFriendly(fk, pk))
	            continue;

	        enemyCount = enemyCount + pu;
	    }

	    if (enemyCount > 0)
	        m_mLastEnemySeenTime.Set(fk, worldTime);
	    else if (!m_mLastEnemySeenTime.Contains(fk))
	        m_mLastEnemySeenTime.Set(fk, worldTime);

	    if (!m_mCaptureStartTime.Contains(fk))
	    {
	        m_mCaptureStatus.Set(fk, DCO_ECaptureStatus.IDLE);
	        return DCO_ECaptureStatus.IDLE;
	    }

	    float lastTick;
	    if (!m_mCaptureLastTick.Find(fk, lastTick))
	        lastTick = worldTime;

	    float dt = worldTime - lastTick;
	    m_mCaptureLastTick.Set(fk, worldTime);

	    if (dt <= 0.0)
	        return GetCaptureStatus(fk);

	    if (enemyCount > 0)
	    {
	        m_mCaptureHeld.Set(fk, 0.0);
	        m_mCaptureStatus.Set(fk, DCO_ECaptureStatus.CONTESTED);
	        return DCO_ECaptureStatus.CONTESTED;
	    }

	    if (friendlyCount <= 0)
	    {
	        float heldIdle;
	        if (!m_mCaptureHeld.Find(fk, heldIdle))
	            heldIdle = 0.0;

	        heldIdle = heldIdle - dt;

	        if (heldIdle <= 0.0)
	        {
	            ClearCaptureTimer(fk);
	            return DCO_ECaptureStatus.IDLE;
	        }

	        m_mCaptureHeld.Set(fk, heldIdle);
	        m_mCaptureStatus.Set(fk, DCO_ECaptureStatus.IDLE);
	        return DCO_ECaptureStatus.IDLE;
	    }

	    float held;
	    if (!m_mCaptureHeld.Find(fk, held))
	        held = 0.0;

	    m_mCaptureHeld.Set(fk, held + dt);
	    m_fLastProgressTime = worldTime;
	    m_mCaptureStatus.Set(fk, DCO_ECaptureStatus.CAPTURING);

	    return DCO_ECaptureStatus.CAPTURING;
	}

	bool IsStalemate(FactionKey fk, float worldTime)
	{
		if (m_fStalemateSeconds <= 0.0)
			return false;

		float now = GetControl(fk);
		float lastControl;
		float lastChange;
		bool known = m_mStalemateControl.Find(fk, lastControl) && m_mStalemateChangeTime.Find(fk, lastChange);

		if (!known || lastChange < m_fLastControlChangeTime || Math.AbsFloat(now - lastControl) >= m_fStalematePercentThreshold)
		{
			m_mStalemateControl.Set(fk, now);
			m_mStalemateChangeTime.Set(fk, Math.Max(worldTime, m_fLastControlChangeTime));
			return false;
		}

		return (worldTime - lastChange) >= m_fStalemateSeconds;
	}

	void ResetStalemateTracking()
	{
	    m_fLastProgressTime = 0.0;
	    m_mStalemateControl.Clear();
	    m_mStalemateChangeTime.Clear();
	}

	bool IsCommanderBlackListed(string cuid)
	{
		if (m_sBlacklistedCo && m_sBlacklistedCo.Contains(cuid))
			return true;
		return !DCO_CanProcess(cuid);
	}

	bool DCO_CanProcess(string cuid)
	{
		if (m_sBlacklistedCo && m_sBlacklistedCo.Contains(cuid))
			return false;
		if (m_iDCOAssignMode == 1)
			return m_aDCOAssignList && m_aDCOAssignList.Contains(cuid);
		if (m_iDCOAssignMode == 2)
			return !m_aDCOAssignList || !m_aDCOAssignList.Contains(cuid);
		return true;
	}

	int DCO_AssignModeEff()
	{
		if (m_iDCOAssignMode == 0 && m_sBlacklistedCo && !m_sBlacklistedCo.IsEmpty())
			return 2;
		return m_iDCOAssignMode;
	}

	void DCO_GetAssignListEff(notnull array<string> outList)
	{
		outList.Clear();
		if (m_iDCOAssignMode == 0 && m_sBlacklistedCo && !m_sBlacklistedCo.IsEmpty())
		{
			outList.Copy(m_sBlacklistedCo);
			return;
		}
		if (m_aDCOAssignList)
			outList.Copy(m_aDCOAssignList);
	}

	bool DCO_AssignEmpty()
	{
		if (m_iDCOAssignMode == 1 && (!m_aDCOAssignList || m_aDCOAssignList.IsEmpty()))
			return true;
		int effMode = DCO_AssignModeEff();
		if (effMode != 1 && effMode != 2)
			return false;
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr || mgr.m_aCommander.IsEmpty())
			return false;
		foreach (AICommander_BaseComponent c : mgr.m_aCommander)
		{
			if (c && DCO_CanProcess(c.GetCommanderUID()))
				return false;
		}
		return true;
	}

	void DCO_SetAssign(int mode, notnull array<string> list, string by, int playerId)
	{
		string before = string.Format("%1:%2", DCO_AssignModeEff(), DCO_JoinList(m_aDCOAssignList));
		m_iDCOAssignMode = Math.ClampInt(mode, 0, 2);
		if (!m_aDCOAssignList)
			m_aDCOAssignList = {};
		m_aDCOAssignList.Copy(list);
		if (m_sBlacklistedCo && !m_sBlacklistedCo.IsEmpty())
		{
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("obj_assign_migrate obj=%1 old_blacklist=%2", GetLogName(), DCO_JoinList(m_sBlacklistedCo)));
			m_sBlacklistedCo.Clear();
		}
		string modeName = "all";
		if (m_iDCOAssignMode == 1)
			modeName = "only";
		else if (m_iDCOAssignMode == 2)
			modeName = "except";
		DCO_Perf.Count("obj_assign:set");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("obj_assign_set obj=%1 mode=%2 list=%3 by=%4 player=%5 before=%6", GetLogName(), modeName, DCO_JoinList(m_aDCOAssignList), by, playerId, before));
		GetGame().GetCallqueue().Remove(DCO_CheckAssignEmpty);
		GetGame().GetCallqueue().CallLater(DCO_CheckAssignEmpty, 200);
	}

	protected void DCO_CheckAssignEmpty()
	{
		if (!DCO_AssignEmpty())
			return;
		string modeName = "only";
		if (m_iDCOAssignMode == 2)
			modeName = "except";
		DCO_Perf.Count("obj_assign:empty");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("obj_assign_empty obj=%1 mode=%2 -- nobody handles this objective until a commander is allowed", GetLogName(), modeName));
		string nm = GetDisplayName();
		if (nm.IsEmpty())
			nm = GetLogName();
		DCO_PlayerComms.SendToGameMasters("DCO", string.Format("Nobody handles %1. It is ignored until a commander is checked.", nm));
	}

	void DCO_CommanderGone(string cuid)
	{
		int effMode = DCO_AssignModeEff();
		if (effMode != 1 && effMode != 2)
			return;
		DCO_Perf.Count("obj_assign:cmd_gone");
		if (m_aDCOAssignList && m_aDCOAssignList.Contains(cuid))
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("obj_assign_cmd_gone cmd=%1 obj=%2 mode=%3 list=%4 -- listed commander deleted; list kept (a new commander with the same UID takes its place)", cuid, GetLogName(), m_iDCOAssignMode, DCO_JoinList(m_aDCOAssignList)));
		GetGame().GetCallqueue().Remove(DCO_CheckAssignEmpty);
		GetGame().GetCallqueue().CallLater(DCO_CheckAssignEmpty, 200);
	}

	static string DCO_JoinList(array<string> list)
	{
		if (!list || list.IsEmpty())
			return "-";
		string s;
		foreach (string e : list)
		{
			if (!s.IsEmpty())
				s += ",";
			s += e;
		}
		return s;
	}

	void MarkCompleted(FactionKey fk) { SetObjectiveState(fk, CMD_EObjectiveState.COMPLETED); }
	void MarkFailed(FactionKey fk)    { SetObjectiveState(fk, CMD_EObjectiveState.FAILED); }
	void MarkAssigned(FactionKey fk)  { SetObjectiveState(fk, CMD_EObjectiveState.ASSIGNED); }

	void NotifyContested(float worldTime)
	{
		m_fLastContestedTime = worldTime;

		m_mCachedScore.Clear();
		m_mScoreCacheAge.Clear();
		m_mScoreBreakdown.Clear();
	}

	CMD_EObjectiveState GetObjectiveState(FactionKey fk)
	{
		CMD_EObjectiveState state;
		if (m_mObjectiveState.Find(fk, state))
			return state;
		return CMD_EObjectiveState.PENDING;
	}

	protected void InitializeObjective()
	{
		AICommander_ManagerComponent.GetOrSpawnInstance(m_sManagerPrefab, m_OwnerEntity);
		if (!AICommander_ManagerComponent.GetInstance())
			return;

		AICommander_ManagerComponent.GetInstance().InitializeCommanderManager();

		AICommander_ManagerComponent.GetInstance().RegisterObjective(this);
		for(int i = 0; i < AICommander_ManagerComponent.GetInstance().m_aAvailableFactions.Count(); i++)
		{
			m_mObjectiveState.Insert(AICommander_ManagerComponent.GetInstance().m_aAvailableFactions[i], CMD_EObjectiveState.PENDING);
			m_mObjectiveAssignedGroup.Insert(AICommander_ManagerComponent.GetInstance().m_aAvailableFactions[i], 0);
		}
	}


	float ComputePriorityScore(FactionKey forFaction, float worldTime, vector commanderPos, float combatFocus = 0.5, string commanderUID = "")
	{
	    CMD_EObjectiveState currentState = GetObjectiveState(forFaction);

	    if (currentState == CMD_EObjectiveState.COMPLETED || currentState == CMD_EObjectiveState.FAILED)
	        return 0.0;

	    string cacheKey = forFaction + "|" + commanderUID;

	    float cacheAge;
	    if (m_mScoreCacheAge.Find(cacheKey, cacheAge) && (worldTime - cacheAge) < 3.0)
	    {
	        float cachedScore;
	        if (m_mCachedScore.Find(cacheKey, cachedScore))
	            return cachedScore;
	    }

	    bool prevSuppress = m_bSuppressProcessedMark;
	    m_bSuppressProcessedMark = true;

	    int enemyCount;
	    int friendlyAtThreatRadius;
	    CountNearbyUnitsCached(m_fThreatRadius, forFaction, friendlyAtThreatRadius, enemyCount, 30.0);

	    float contestedRaw = 0.0;

	    if (m_fLastContestedTime > 0.0)
	    {
	        float elapsed = worldTime - m_fLastContestedTime;
	        if (elapsed < 120.0)
	            contestedRaw = Math.Lerp(25.0, 0.0, elapsed / 120.0);
	    }

	    int friendlyCount;
	    int enemyAtFriendlyRadius;
	    CountNearbyUnitsCached(m_fFriendlyRadius, forFaction, friendlyCount, enemyAtFriendlyRadius, 30.0);
	    m_bSuppressProcessedMark = prevSuppress;
	    float friendlyPenalty = Math.Clamp(friendlyCount * 5.0, 0.0, 30.0);

	    float assignedPenalty = 0.0;
	    if (currentState == CMD_EObjectiveState.ASSIGNED)
	        assignedPenalty = 15.0;

	    float score = m_fBaseValue * StrategicScale() - friendlyPenalty - assignedPenalty;

	    float distToCommander = vector.Distance(commanderPos, GetOwner().GetOrigin());

	    float proximityFactor = 0.0;
	    float proximityBonus  = 0.0;

	    if (m_fMaxRelevantDistance > 0.0)
	    {
	        if (distToCommander <= m_fMaxRelevantDistance)
	        {
	            proximityFactor = 1.0 - (distToCommander / m_fMaxRelevantDistance);
	            proximityBonus  = proximityFactor * m_fProximityWeight;
	        }
	        else
	        {
	            float excess   = (distToCommander - m_fMaxRelevantDistance) / m_fMaxRelevantDistance;
	            proximityBonus = -m_fDistantFalloffWeight * (excess / (1.0 + excess));
	        }
	    }

	    float contestedBonus = contestedRaw * proximityFactor;

	    score += proximityBonus + contestedBonus;

	    float threatFactor = Math.Max(
	        Math.Pow(proximityFactor, m_fThreatFalloffPower),
	        m_fThreatFalloffFloor);

	    float focusMod = Math.Lerp(0.5, 1.5, combatFocus);

	    int cappedEnemyCount = enemyCount;
	    if (m_iThreatCountCap > 0 && cappedEnemyCount > m_iThreatCountCap)
	        cappedEnemyCount = m_iThreatCountCap;

	    float threatTerm = 1.0 * cappedEnemyCount * m_fThreatWeight * threatFactor * focusMod;

	    score += threatTerm;

	    float finalScore = Math.Max(score, 0.0);
	    if (m_iStrategicValue == 3)
	        finalScore = Math.Max(threatTerm, 0.0);

	    m_mCachedScore.Set(cacheKey, finalScore);
	    m_mScoreCacheAge.Set(cacheKey, worldTime);

	    if (m_bDebugMode)
	    {
	        m_mScoreBreakdown.Set(cacheKey, string.Format(
	            "base %1  contested %2  prox %3  threat %4 (%5 of %6 enemies)  friendly -%7  assigned -%8",
	            DCO_DebugDraw.F1(m_fBaseValue),
	            DCO_DebugDraw.F1(contestedBonus),
	            DCO_DebugDraw.F1(proximityBonus),
	            DCO_DebugDraw.F1(threatTerm),
	            cappedEnemyCount,
	            enemyCount,
	            DCO_DebugDraw.F1(friendlyPenalty),
	            DCO_DebugDraw.F1(assignedPenalty)));
	    }

	    return finalScore;
	}

	void SetObjectiveState(FactionKey fk, CMD_EObjectiveState state)
	{
		if (m_mObjectiveState.Contains(fk))
		{
			m_mObjectiveState[fk] = state;
		}
	}

	void SetObjectiveGroup(FactionKey fk, int number)
	{
		if (m_mObjectiveAssignedGroup.Contains(fk))
		{
			int num = m_mObjectiveAssignedGroup[fk] + number;
			m_mObjectiveAssignedGroup[fk] = num;
		}
	}

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_OwnerEntity = owner;
		SetEventMask(owner, EntityEvent.INIT);

		SetEventMask(owner, EntityEvent.FRAME);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer())
			return;

		if (m_fPresenceScanInterval <= 0.0)
			return;

		ResolveInitialOwner();

		m_fScanTimer += timeSlice;

		if (m_fScanTimer < m_fPresenceScanInterval)
			return;

		float elapsed  = m_fScanTimer;
		m_fScanTimer   = 0.0;

		float worldTime = DCO_Util.NowS();

		bool active = IsProcessedActive(worldTime);

		if (active)
			ScanPresenceFromCensus(worldTime);
		else if (m_bDebugMode)
			m_aDebugShapes.Clear();

		UpdateObjectiveDebug(worldTime);

		if (active)
			AdvanceControl(worldTime, elapsed);
	}

	protected void ResolveInitialOwner()
	{
		if (m_bInitialOwnerResolved)
			return;

		if (m_sInitialOwnerCommanderUID.IsEmpty())
		{
			if (!m_sCapturedCo || m_sCapturedCo.IsEmpty() || m_sCapturedCo[0].IsEmpty())
			{
				m_bInitialOwnerResolved = true;
				return;
			}
			m_sInitialOwnerCommanderUID = m_sCapturedCo[0];
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("obj_owner_legacy obj=%1 uid=%2 field=m_sCapturedCo (old field read as Initial Owner Commander UID; set that field instead)", GetLogName(), m_sInitialOwnerCommanderUID));
		}

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;

		foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
		{
			if (!cmd || cmd.GetCommanderUID() != m_sInitialOwnerCommanderUID)
				continue;

			FactionKey fk = cmd.GetCommanderFactionKey();

			m_mControl.Set(fk, 100.0);
			m_sOwnerFaction      = fk;
			m_sLastOwnerFaction  = fk;
			m_sOwnerCommanderUID = m_sInitialOwnerCommanderUID;

			m_bInitialOwnerResolved = true;
			return;
		}
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		InitializeObjective();
	}

	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(DCO_CheckAssignEmpty);
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
			mgr.UnregisterObjective(this);

		super.OnDelete(owner);
	}

	float GetCensusRadius()
	{
		float r = Math.Max(m_fRadius, Math.Max(m_fThreatRadius, m_fFriendlyRadius));

		if (m_eObjectiveType == CMD_EObjectiveType.RECON)
			r = Math.Max(r, m_fIntelCoverageRadius);

		return r;
	}

	bool QueryCallbackCensus(IEntity e)
	{
		if (!e || !m_aCollectFaction || !m_aCollectDistSq)
			return true;

		SCR_ChimeraCharacter chr = SCR_ChimeraCharacter.Cast(e);
		if (!chr)
			return true;

		if (!DCO_Util.IsNotDead(chr))
			return true;

		if (m_CollectSeen.Contains(e))
			return true;

		m_CollectSeen.Insert(e);

		SCR_CharacterPerceivableComponent percive = SCR_CharacterPerceivableComponent.Cast(e.FindComponent(SCR_CharacterPerceivableComponent));
		if (!percive)
			return true;

		Faction fc = percive.GetPerceivedFaction();
		if (!fc)
			return true;

		m_aCollectFaction.Insert(fc.GetFactionKey());
		m_aCollectDistSq.Insert(vector.DistanceSq(m_vCollectCenter, e.GetOrigin()));

		return true;
	}

	protected void CollectCensus(float radius, notnull array<FactionKey> outFaction, notnull array<float> outDistSq)
	{
		outFaction.Clear();
		outDistSq.Clear();
		m_CollectSeen.Clear();

		m_aCollectFaction = outFaction;
		m_aCollectDistSq  = outDistSq;
		m_vCollectCenter  = GetOwner().GetOrigin();

		DCO_Perf.Count("q:AICommanderObjective");
		GetGame().GetWorld().QueryEntitiesBySphere(m_vCollectCenter, radius, QueryCallbackCensus, null, EQueryEntitiesFlags.DYNAMIC);

		m_aCollectFaction = null;
		m_aCollectDistSq  = null;
		m_CollectSeen.Clear();
	}

	protected void EnsureCensus(float worldTime, float maxAge = -1)
	{
		float radius = GetCensusRadius();
		if (maxAge < 0)
			maxAge = m_fPresenceScanInterval;

		if (m_bCensusValid
			&& m_fPresenceScanInterval > 0.0
			&& m_fCensusRadius == radius
			&& (worldTime - m_fCensusTime) < maxAge)
			return;

		CollectCensus(radius, m_aCensusFaction, m_aCensusDistSq);

		m_fCensusRadius = radius;
		m_fCensusTime   = worldTime;
		m_bCensusValid  = true;
	}

	void CountNearbyUnitsCached(float radius, FactionKey factionKey, out int friendlyCount, out int enemyCount, float maxCensusAge = -1)
	{
		friendlyCount = 0;
		enemyCount    = 0;

		float worldTime = DCO_Util.NowS();
		MarkProcessed(worldTime);

		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return;

		SCR_Faction myFaction = SCR_Faction.Cast(factionManager.GetFactionByKey(factionKey));
		if (!myFaction)
			return;

		array<FactionKey> factions;
		array<float>      distSq;

		if (radius <= GetCensusRadius())
		{
			EnsureCensus(worldTime, maxCensusAge);
			factions = m_aCensusFaction;
			distSq   = m_aCensusDistSq;
		}
		else
		{
			factions = new array<FactionKey>();
			distSq   = new array<float>();
			CollectCensus(radius, factions, distSq);
		}

		float radiusSq = radius * radius;
		m_mCountScratch.Clear();

		int entryCount = factions.Count();
		for (int i = 0; i < entryCount; i++)
		{
			if (distSq[i] > radiusSq)
				continue;

			FactionKey entryKey = factions[i];

			int entryUnits;
			if (!m_mCountScratch.Find(entryKey, entryUnits))
				entryUnits = 0;

			m_mCountScratch.Set(entryKey, entryUnits + 1);
		}

		foreach (FactionKey groupKey, int groupUnits : m_mCountScratch)
		{
			if (groupKey == factionKey)
			{
				friendlyCount += groupUnits;
				continue;
			}

			Faction other = factionManager.GetFactionByKey(groupKey);
			if (other && myFaction.IsFactionFriendly(other))
				friendlyCount += groupUnits;
			else
				enemyCount += groupUnits;
		}
	}

	void ScanPresenceFromCensus(float worldTime)
	{
		EnsureCensus(worldTime);

		m_mFactionPresence.Clear();
		int total = 0;

		float radiusSq = m_fRadius * m_fRadius;

		int entryCount = m_aCensusFaction.Count();
		for (int i = 0; i < entryCount; i++)
		{
			if (m_aCensusDistSq[i] > radiusSq)
				continue;

			FactionKey key = m_aCensusFaction[i];

			int current;
			if (!m_mFactionPresence.Find(key, current))
				current = 0;

			m_mFactionPresence.Set(key, current + 1);
			total = total + 1;
		}

		if (m_bDebugMode)
		{
			m_aDebugShapes.Clear();
			if (DCO_Log.On(DCO_ELogChannel.CMD, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.DEBUG, "-", total.ToString() + " < Number of counted Entity Inside > " + m_sObjectiveName);
			int flags = ShapeFlags.TRANSP | ShapeFlags.WIREFRAME;
			m_aDebugShapes.Insert(Shape.CreateSphere(Color.BLUE, flags, GetOwner().GetOrigin(), m_fRadius));
		}

		m_iTotalPresence = total;
		m_fLastScanTime  = worldTime;
		m_bHasScanned    = true;
	}

	void MarkProcessed(float worldTime)
	{
		if (m_bSuppressProcessedMark)
			return;

		bool wasActive = IsProcessedActive(worldTime);

		m_fLastProcessedTime = worldTime;
		m_bEverProcessed     = true;

		if (wasActive)
			return;

		if (!Replication.IsServer() || m_fPresenceScanInterval <= 0.0)
			return;

		ScanPresenceFromCensus(worldTime);
	}

	bool IsProcessedActive(float worldTime)
	{
		if (m_fIdleTimeout <= 0.0)
			return true;

		if (!m_bEverProcessed)
			return false;

		return (worldTime - m_fLastProcessedTime) <= m_fIdleTimeout;
	}

	float GetControl(FactionKey fk)
	{
		float v;
		if (m_mControl.Find(fk, v))
			return v;

		return 0.0;
	}

	float GetNeutralControl()
	{
		float total = 0.0;

		foreach (FactionKey k, float v : m_mControl)
			total = total + v;

		return Math.Clamp(100.0 - total, 0.0, 100.0);
	}

	FactionKey GetOwnerFaction()      { return m_sOwnerFaction; }
	FactionKey GetLastOwnerFaction()  { return m_sLastOwnerFaction; }
	string     GetOwnerCommanderUID() { return m_sOwnerCommanderUID; }

	bool IsOwnedBy(FactionKey fk)
	{
		return !m_sOwnerFaction.IsEmpty() && m_sOwnerFaction == fk;
	}

	bool AreFactionsFriendly(FactionKey a, FactionKey b)
	{
		if (a == b)
			return true;

		FactionManager fm = GetGame().GetFactionManager();
		if (!fm)
			return false;

		Faction fa = fm.GetFactionByKey(a);
		Faction fb = fm.GetFactionByKey(b);
		return fa && fb && fa.IsFactionFriendly(fb);
	}

	protected int GetSideStrength(FactionKey fk)
	{
		int side = 0;
		foreach (FactionKey k, int units : m_mFactionPresence)
		{
			if (units > 0 && CanFactionContest(k) && AreFactionsFriendly(fk, k))
				side += units;
		}
		return side;
	}

	bool CanFactionContest(FactionKey fk)
	{
		if (fk.IsEmpty())
			return false;

		if (m_bAllowAllFactions)
			return true;

		return m_aAllowedFactions.Contains(fk);
	}

	void GetControlSnapshot(out array<FactionKey> outKeys, out array<float> outValues)
	{
		outKeys.Clear();
		outValues.Clear();

		foreach (FactionKey k, float v : m_mControl)
		{
			outKeys.Insert(k);
			outValues.Insert(v);
		}
	}

	void AdvanceControl(float worldTime, float dt)
	{
		if (!m_bHasScanned || dt <= 0.0)
			return;

		FactionKey winner    = "";
		int winnerSide       = 0;
		int winnerOwn        = 0;

		foreach (FactionKey k, int units : m_mFactionPresence)
		{
			if (units <= 0 || !CanFactionContest(k))
				continue;

			int side = GetSideStrength(k);
			if (side > winnerSide || (side == winnerSide && units > winnerOwn))
			{
				winnerSide = side;
				winnerOwn  = units;
				winner     = k;
			}
		}

		if (winnerSide <= 0)
		{
			DecayTowardLastOwner(dt);
			ResolveOwnership(worldTime);
			return;
		}

		int runnerUpSide = 0;
		foreach (FactionKey k, int units : m_mFactionPresence)
		{
			if (units <= 0 || !CanFactionContest(k) || AreFactionsFriendly(winner, k))
				continue;

			runnerUpSide = Math.Max(runnerUpSide, GetSideStrength(k));
		}

		float winnerF        = winnerSide;
		float runnerUpF      = runnerUpSide;
		float baseRate       = 100.0 / Math.Max(m_fCaptureHoldDuration, 1.0);
		float unitScale      = Math.Clamp(winnerF / Math.Max(m_iFullSpeedUnits, 1), 0.0, 1.0);
		float contestPenalty = (winnerF - runnerUpF) / winnerF;

		float delta = baseRate * unitScale * contestPenalty * dt;

		if (delta > 0.0)
			PushControlToward(winner, delta);

		ResolveOwnership(worldTime);
	}

	protected void PushControlToward(FactionKey winner, float delta)
	{
		float remaining = delta;

		float othersTotal = 0.0;
		foreach (FactionKey k, float v : m_mControl)
		{
			if (k != winner)
				othersTotal = othersTotal + v;
		}

		if (othersTotal > 0.0)
		{
			float drain = Math.Min(remaining, othersTotal);
			float ratio = drain / othersTotal;

			array<FactionKey> keys = {};
			foreach (FactionKey k, float v : m_mControl)
				keys.Insert(k);

			foreach (FactionKey k : keys)
			{
				if (k == winner)
					continue;

				float v = GetControl(k);
				m_mControl.Set(k, Math.Max(v - (v * ratio), 0.0));
			}

			remaining = remaining - drain;
		}

		if (remaining <= 0.0)
			return;

		float mine = GetControl(winner);
		m_mControl.Set(winner, Math.Clamp(mine + remaining, 0.0, 100.0));
	}

	protected void DecayTowardLastOwner(float dt)
	{
		if (m_sLastOwnerFaction.IsEmpty())
			return;

		if (GetControl(m_sLastOwnerFaction) >= 100.0)
			return;

		PushControlToward(m_sLastOwnerFaction, m_fEmptyDecayRate * dt);
	}

	protected void ResolveOwnership(float worldTime)
	{
		FactionKey previousOwner = m_sOwnerFaction;

		if (!m_sOwnerFaction.IsEmpty() && GetControl(m_sOwnerFaction) < m_fOwnershipLossThreshold)
			m_sOwnerFaction = "";

		if (m_sOwnerFaction.IsEmpty())
		{
			float best = m_fOwnershipGainThreshold;
			foreach (FactionKey k, float v : m_mControl)
			{
				if (v >= best)
				{
					best                = v;
					m_sOwnerFaction     = k;
				}
			}

			if (!m_sOwnerFaction.IsEmpty())
				m_sLastOwnerFaction = m_sOwnerFaction;
		}

		if (previousOwner != m_sOwnerFaction)
		{
			m_mCommanderContribution.Clear();
			m_mCommanderFaction.Clear();
			m_sOwnerCommanderUID = string.Empty;
			m_fLastControlChangeTime = worldTime;
		}

		ResolveOwnerCommander();
	}

	protected void ResolveOwnerCommander()
	{
		if (m_sOwnerFaction.IsEmpty())
		{
			m_sOwnerCommanderUID = string.Empty;
			return;
		}

		string bestUID  = string.Empty;
		float  bestScore = 0.0;

		foreach (string cuid, float score : m_mCommanderContribution)
		{
			FactionKey cfk;
			if (!m_mCommanderFaction.Find(cuid, cfk) || cfk != m_sOwnerFaction)
				continue;

			if (score > bestScore)
			{
				bestScore = score;
				bestUID   = cuid;
			}
		}

		m_sOwnerCommanderUID = bestUID;
	}

	protected void UpdateObjectiveDebug(float worldTime)
	{
		if (!m_bDebugMode)
		{
			if (!m_aDebugTexts.IsEmpty())
				m_aDebugTexts.Clear();

			return;
		}

		m_aDebugTexts.Clear();

		if (!DCO_DebugDraw.IsLocalPlayerInGM())
		{
			m_aDebugShapes.Clear();
			return;
		}

		int    flags = DCO_DebugDraw.Flags();
		vector pos   = GetOwner().GetOrigin();

		int color;
		if (m_sOwnerFaction.IsEmpty())
			color = DCO_DebugDraw.COLOR_NEUTRAL;
		else
			color = DCO_DebugDraw.FactionColor(m_sOwnerFaction);

		m_aDebugShapes.Insert(Shape.CreateSphere(color, flags, pos, 2.0));

		DrawDebugControlBar(Vector(pos[0], pos[1] + 8.0, pos[2]), flags);

		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
			Vector(pos[0], pos[1] + 30.0, pos[2]), BuildDebugHeader(),   21.0, color));

		m_bSuppressProcessedMark = true;
		string scoreText = BuildDebugCommanderScores(worldTime);
		m_bSuppressProcessedMark = false;

		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
			Vector(pos[0], pos[1] + 41.0, pos[2]), scoreText, 16.0,
			DCO_DebugDraw.COLOR_COMMANDER));

		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
			Vector(pos[0], pos[1] + 21.0, pos[2]), BuildDebugControl(),  16.0, color));

		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
			Vector(pos[0], pos[1] + 12.0, pos[2]), BuildDebugPresence(worldTime), 16.0, color));

		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
			Vector(pos[0], pos[1] +  2.0, pos[2]), BuildDebugTuning(),   15.0, color));
	}

	protected void DrawDebugControlBar(vector origin, int flags)
	{

		array<int> slotColors = {};
		for (int s = 0; s < 20; s++)
			slotColors.Insert(DCO_DebugDraw.COLOR_NEUTRAL);

		int cursor = 0;
		foreach (FactionKey k, float v : m_mControl)
		{
			int take = Math.Round(v / 100.0 * 20);
			int col  = DCO_DebugDraw.FactionColor(k);

			for (int n = 0; n < take; n++)
			{
				if (cursor >= 20)
					break;

				slotColors.Set(cursor, col);
				cursor = cursor + 1;
			}
		}

		float startX = origin[0] - ((20 - 1) * 1.3 * 0.5);

		for (int i = 0; i < 20; i++)
		{
			vector p = Vector(startX + (1.0 * i * 1.3), origin[1], origin[2]);
			m_aDebugShapes.Insert(Shape.CreateSphere(slotColors.Get(i), flags, p, 0.5));
		}
	}

	protected string BuildDebugHeader()
	{
		string label = m_sObjectiveName;
		if (label.IsEmpty())
			label = GetOwner().GetName();

		string ownerText;
		if (m_sOwnerFaction.IsEmpty())
		{
			ownerText = "Neutral";
		}
		else
		{
			string cmdText;
			if (m_sOwnerCommanderUID.IsEmpty())
				cmdText = "no commander yet";
			else
				cmdText = m_sOwnerCommanderUID;

			ownerText = string.Format("%1 (%2)", m_sOwnerFaction, cmdText);
		}

		return string.Format("%1  [%2]\nowner: %3",
			label,
			DCO_DebugDraw.ObjectiveTypeName(m_eObjectiveType),
			ownerText);
	}

	protected string BuildDebugControl()
	{
		string body = "CONTROL";

		foreach (FactionKey k, float v : m_mControl)
		{
			if (v <= 0.0)
				continue;

			body = body + string.Format("\n  %1 : %2%%", k, Math.Round(v));
		}

		float neutral = GetNeutralControl();
		if (neutral > 0.0)
			body = body + string.Format("\n  Neutral : %1%%", Math.Round(neutral));

		if (!m_sLastOwnerFaction.IsEmpty())
			body = body + string.Format("\n  last owner %1, empty decay %2%%/s",
				m_sLastOwnerFaction, DCO_DebugDraw.F1(m_fEmptyDecayRate));

		return body;
	}

	protected string BuildDebugPresence(float worldTime)
	{
		string body = string.Format("PRESENCE  scan every %1s   last %2s ago\n%3 units, %4 factions",
			DCO_DebugDraw.F1(m_fPresenceScanInterval),
			DCO_DebugDraw.F1(GetPresenceScanAge(worldTime)),
			m_iTotalPresence,
			m_mFactionPresence.Count());

		if (IsProcessedActive(worldTime))
			body = body + "\nstate ACTIVE";
		else
			body = body + string.Format("\nstate IDLE (not processed > %1s)", DCO_DebugDraw.F1(m_fIdleTimeout));

		body = body + string.Format("\ncensus r %1   %2 entries",
			DCO_DebugDraw.M(GetCensusRadius()),
			m_aCensusFaction.Count());

		if (!m_bHasScanned)
			return body + "\n(not scanned yet)";

		if (m_mFactionPresence.IsEmpty())
			return body + "\nradius empty";

		foreach (FactionKey k, int units : m_mFactionPresence)
		{
			string mark;
			if (!CanFactionContest(k))
				mark = "  (cannot contest)";
			else
				mark = "";

			body = body + string.Format("\n  %1 : %2%3", k, units, mark);
		}

		return body;
	}

	protected string BuildDebugCommanderScores(float worldTime)
	{
		string body = "COMMANDER SCORES";

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return body + "\n  (no manager)";

		if (mgr.m_aCommander.IsEmpty())
			return body + "\n  (no commander registered)";

		foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
		{
			if (!cmd || !cmd.GetOwner())
				continue;

			FactionKey fk  = cmd.GetCommanderFactionKey();
			string     uid = cmd.GetCommanderUID();

			float score = ComputePriorityScore(fk, worldTime, cmd.GetOwner().GetOrigin(), cmd.GetCombatFocus(), uid);

			string marks = "";

			if (IsCommanderBlackListed(uid))
				marks = marks + "  [BLACKLIST]";

			if (IsCapturedBy(fk, uid))
				marks = marks + "  [CAPTURED]";

			body = body + string.Format("\n  %1 (%2)  score %3   %4   slots %5%6",
				uid,
				fk,
				DCO_DebugDraw.F1(score),
				DCO_DebugDraw.ObjectiveStateName(GetObjectiveState(fk)),
				GetCurrentAssignedGroupCount(fk),
				marks);

			string breakdown;
			if (m_mScoreBreakdown.Find(fk + "|" + uid, breakdown))
				body = body + "\n       " + breakdown;
		}

		return body;
	}

	protected string BuildDebugTuning()
	{
		string a = string.Format(
			"TUNING\nradius %1   base value %2   threat %3   friendly %4",
			DCO_DebugDraw.M(m_fRadius),
			DCO_DebugDraw.F1(m_fBaseValue),
			DCO_DebugDraw.M(m_fThreatRadius),
			DCO_DebugDraw.M(m_fFriendlyRadius));

		string b = string.Format(
			"\nhold %1s   full speed at %2 units   own %3%% / lose %4%%",
			DCO_DebugDraw.F1(m_fCaptureHoldDuration),
			m_iFullSpeedUnits,
			Math.Round(m_fOwnershipGainThreshold),
			Math.Round(m_fOwnershipLossThreshold));

		string c = string.Format(
			"\nslots %1 of %2 (cap %3)   sectors %4 of %5 defendGroupCount\nuncontested dwell %6s   stalemate %7%% over %8s",
			GetCurrentAssignedGroupCount(m_sOwnerFaction),
			GetRequiredGroupCount(),
			m_iMaxGroupCount,
			GetStaffedSectorCount(m_sOwnerFaction),
			m_iDefendGroupCount,
			DCO_DebugDraw.F1(m_fUncontestedDwell),
			DCO_DebugDraw.F1(m_fStalematePercentThreshold),
			DCO_DebugDraw.F1(m_fStalemateSeconds));

		return a + b + c;
	}

	int GetFactionPresence(FactionKey fk)
	{
		int count;
		if (m_mFactionPresence.Find(fk, count))
			return count;

		return 0;
	}

	int GetTotalPresence()
	{
		return m_iTotalPresence;
	}

	int GetPresenceFactionCount()
	{
		return m_mFactionPresence.Count();
	}

	void GetPresenceFactionKeys(out array<FactionKey> outKeys)
	{
		outKeys.Clear();

		foreach (FactionKey key, int count : m_mFactionPresence)
			outKeys.Insert(key);
	}

	bool HasScannedPresence()
	{
		return m_bHasScanned;
	}

	float GetPresenceScanAge(float worldTime)
	{
		if (!m_bHasScanned)
			return -1.0;

		return worldTime - m_fLastScanTime;
	}

	float GetPresenceScanInterval()
	{
		return m_fPresenceScanInterval;
	}

	string GetOwnerFactionLabel()
	{
		FactionKey owner = GetOwningFaction();

		if (owner.IsEmpty())
			return "Neutral";

		return owner;
	}

	int GetRequiredGroupCount()
	{
	    int radiusGroups;
	    if (m_fRadius >= 70.0)
	        radiusGroups = 2;
	    else
	        radiusGroups = 1;

	    int priorityBonus;
	    if (m_fBaseValue >= 60.0)
	        priorityBonus = 1;
	    else
	        priorityBonus = 0;

	    int typeBonus;
	    switch (m_eObjectiveType)
	    {
	        case CMD_EObjectiveType.CAPTURE:
	        case CMD_EObjectiveType.DESTROY:
	        case CMD_EObjectiveType.DEFEND:
	        case CMD_EObjectiveType.HOLD_UNTIL:
	            typeBonus = 1;
	            break;
	        default:
	            typeBonus = 0;
	            break;
	    }

	    int total = radiusGroups + priorityBonus + typeBonus;
	    return Math.ClampInt(total, 1, m_iMaxGroupCount);
	}

	CMD_EObjectiveType GetObjectiveType()    { return m_eObjectiveType; }
	string GetObjectiveName()                { return m_sObjectiveName; }

	string GetDisplayName()
	{
		if (!m_sObjectiveName.IsEmpty())
			return m_sObjectiveName;
		if (!m_sDCOFallbackName.IsEmpty())
			return m_sDCOFallbackName;
		return BuildFallbackName();
	}

	string GetLogName()
	{
		string n = GetDisplayName();
		n.Replace(" ", "_");
		return n;
	}

	protected string BuildFallbackName()
	{
		IEntity owner = GetOwner();
		string n = owner.GetName();
		if (!n.IsEmpty())
		{
			m_sDCOFallbackName = n;
			DCO_Perf.Count("obj_name_fallback:entity");
			return n;
		}

		vector pos = owner.GetOrigin();
		SCR_EditableEntityCore core = SCR_EditableEntityCore.Cast(SCR_EditableEntityCore.GetInstance(SCR_EditableEntityCore));
		if (!core)
			return "Objective " + SCR_MapEntity.GetGridLabel(pos);

		SCR_EditableEntityComponent loc = core.FindNearestEntity(pos, EEditableEntityType.COMMENT, EEditableEntityFlag.LOCAL);
		vector locPos;
		if (loc && loc.GetInfo() && loc.GetPos(locPos) && vector.DistanceXZ(locPos, pos) <= 600)
			n = WidgetManager.Translate(loc.GetInfo().GetName());

		if (!n.IsEmpty() && !n.StartsWith("#"))
		{
			DCO_Perf.Count("obj_name_fallback:location");
		}
		else
		{
			n = "Objective " + SCR_MapEntity.GetGridLabel(pos);
			DCO_Perf.Count("obj_name_fallback:grid");
		}
		m_sDCOFallbackName = n;
		return n;
	}

	void SetObjectiveName(string objectiveName)
	{
		if (!Replication.IsServer())
			return;

		string before = GetLogName();
		m_sObjectiveName = objectiveName;
		Replication.BumpMe();
		DCO_Perf.Count("obj_rename:gm");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("obj_rename from=%1 to=%2", before, GetLogName()));
		AICommander_ManagerComponent rmgr = AICommander_ManagerComponent.GetInstance();
		if (rmgr)
			rmgr.DCO_RebuildObjectiveRoster();
	}

	void DCO_ApplyPersisted(string objectiveName, float radius, string ownerUid)
	{
		if (!Replication.IsServer())
			return;

		m_sObjectiveName = objectiveName;
		m_fRadius = Math.Clamp(radius, 1, 300);
		Replication.BumpMe();
		OnRadiusChanged();

		DCO_SetController(null);
		if (!ownerUid.IsEmpty())
		{
			m_sInitialOwnerCommanderUID = ownerUid;
			m_bInitialOwnerResolved = false;
			ResolveInitialOwner();
		}
		DCO_Perf.Count("obj_persist:load");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("obj_persist_load obj=%1 radius=%2 owner=%3", GetLogName(), Math.Round(m_fRadius), ownerUid));
		AICommander_ManagerComponent pmgr = AICommander_ManagerComponent.GetInstance();
		if (pmgr)
			pmgr.DCO_RebuildObjectiveRoster();
	}
	int GetDefendGroupCount()                { return m_iDefendGroupCount; }

	float GetBaseValue()                     { return m_fBaseValue; }
	float GetThreatRadius()                  { return m_fThreatRadius; }
	float GetFriendlyRadius()                { return m_fFriendlyRadius; }
	float GetMaxRelevantDistance()           { return m_fMaxRelevantDistance; }
	float GetDistantFalloffWeight()          { return m_fDistantFalloffWeight; }
	float GetCaptureHoldDuration()           { return m_fCaptureHoldDuration; }
	int   GetMaxGroupCount()                 { return m_iMaxGroupCount; }

	int GetCurrentAssignedGroupCount(FactionKey fk)
	{
		int count;
		if (m_mObjectiveAssignedGroup.Find(fk, count))
			return count;
		return 0;
	}

	void SetReconGroup(FactionKey fk, DCO_GroupUtilityComponent grp)
	{
	    if (!m_mReconGroup.Contains(fk))
	        m_mReconGroup.Insert(fk, grp);
	    else
	        m_mReconGroup.Set(fk, grp);
	}

	DCO_GroupUtilityComponent GetReconGroup(FactionKey fk)
	{
		DCO_GroupUtilityComponent grp;
		if (m_mReconGroup.Find(fk, grp))
			return grp;
		return null;
	}

	bool IsReconArrived(FactionKey fk, float worldTime)
	{
	    DCO_GroupUtilityComponent reconGrp;
	    if (!m_mReconGroup.Find(fk, reconGrp))
	        return false;

	    if (!reconGrp)
	        return false;

	    return reconGrp.CheckOrderComplete(worldTime);
	}

	bool IsReconObjectiveActive(FactionKey fk)
	{
		if (m_eObjectiveType != CMD_EObjectiveType.RECON)
			return false;

		DCO_GroupUtilityComponent reconGrp;
		if (!m_mReconGroup.Find(fk, reconGrp))
			return false;

		if (!reconGrp || !reconGrp.GetOwner())
			return false;

		float toleranceDist = Math.Max(m_fRadius * 2.0, m_fRadius + 350.0);
		return vector.DistanceSq(reconGrp.GetOwner().GetOrigin(), GetOwner().GetOrigin()) <= toleranceDist * toleranceDist;
	}

	float GetIntelCoverageRadius()
	{
		return m_fIntelCoverageRadius;
	}

	void ResetAssignedGroupCount(FactionKey fk)
	{
		int current = GetCurrentAssignedGroupCount(fk);
		SetObjectiveGroup(fk, -current);
		ClearSectorGrid(fk);
	}

	void StartCaptureTimer(FactionKey fk, float worldTime)
	{
		if (!m_mCaptureStartTime.Contains(fk))
			m_mCaptureStartTime.Insert(fk, worldTime);
		else
			m_mCaptureStartTime.Set(fk, worldTime);

		m_mCaptureHeld.Set(fk, 0.0);
		m_mCaptureLastTick.Set(fk, worldTime);
	}

	bool IsCaptureTimerRunning(FactionKey fk)
	{
		return m_mCaptureStartTime.Contains(fk);
	}

	void ClearCaptureTimer(FactionKey fk)
	{
		if (m_mCaptureStartTime.Contains(fk))
			m_mCaptureStartTime.Remove(fk);

		if (m_mCaptureHeld.Contains(fk))
			m_mCaptureHeld.Remove(fk);

		if (m_mCaptureLastTick.Contains(fk))
			m_mCaptureLastTick.Remove(fk);

		if (m_mCaptureStatus.Contains(fk))
			m_mCaptureStatus.Remove(fk);
	}

	void ClearAllCaptureTimers()
	{
		m_mCaptureStartTime.Clear();
		m_mCaptureHeld.Clear();
		m_mCaptureLastTick.Clear();
		m_mCaptureStatus.Clear();
	}

	bool IsCaptureTimerComplete(FactionKey fk, float worldTime)
	{
		if (!m_mCaptureStartTime.Contains(fk))
			return false;

		return GetCaptureProgress(fk, worldTime) >= 1.0;
	}

	FactionKey GetActiveCapturingFaction()
	{
		foreach (FactionKey key, float startTime : m_mCaptureStartTime)
		{
			return key;
		}
		return string.Empty;
	}

	FactionKey GetOwningFaction()
	{
		return m_sOwnerFaction;
	}

	float GetActiveCaptureProgressPercent(float worldTime)
	{
		FactionKey activeFaction = GetActiveCapturingFaction();
		if (activeFaction.IsEmpty())
			return 0.0;

		return GetCaptureProgress(activeFaction, worldTime) * 100.0;
	}

	bool IsCapturedBy(FactionKey fk, string cuid = "")
	{
		return IsOwnedBy(fk);
	}

	bool CheckAndMarkIfLost(FactionKey fk)
	{
		MarkProcessed(DCO_Util.NowS());

		return CheckIsItLost(fk);
	}

	bool CheckIsItLost(FactionKey fk)
	{
		return !m_sLastOwnerFaction.IsEmpty()
			&& m_sLastOwnerFaction == fk
			&& !IsOwnedBy(fk);
	}

	void DCO_SetController(AICommander_BaseComponent cmd)
	{
		if (!Replication.IsServer())
			return;

		m_bInitialOwnerResolved = true;
		m_mCommanderContribution.Clear();
		m_mCommanderFaction.Clear();
		array<FactionKey> keys = {};
		foreach (FactionKey k, float v : m_mControl)
			keys.Insert(k);
		foreach (FactionKey k : keys)
			m_mControl.Set(k, 0.0);

		if (!cmd)
		{
			m_sOwnerFaction = "";
			m_sOwnerCommanderUID = string.Empty;
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("%1 set to Neutral (GM)", GetOwner().GetName()));
			return;
		}

		FactionKey fk = cmd.GetCommanderFactionKey();
		m_mControl.Set(fk, 100.0);
		m_sOwnerFaction = fk;
		m_sLastOwnerFaction = fk;
		m_mCommanderContribution.Set(cmd.GetCommanderUID(), 1000000);
		m_mCommanderFaction.Set(cmd.GetCommanderUID(), fk);
		m_sOwnerCommanderUID = cmd.GetCommanderUID();
		ResetAssignedGroupCount(fk);
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("%1 held by %2 (%3) -- GM", GetOwner().GetName(), m_sOwnerCommanderUID, fk));
	}

	void SetCapturedBy(FactionKey fk, bool val)
	{
		if (val)
		{
			array<FactionKey> keys = {};
			foreach (FactionKey k, float v : m_mControl)
				keys.Insert(k);

			foreach (FactionKey k : keys)
			{
				if (k != fk)
					m_mControl.Set(k, 0.0);
			}

			m_mControl.Set(fk, 100.0);
			m_sOwnerFaction     = fk;
			m_sLastOwnerFaction = fk;
		}
		else if (IsOwnedBy(fk))
		{
			m_mControl.Set(fk, 0.0);
			m_sOwnerFaction = "";
		}

		ResolveOwnerCommander();
	}

	bool IsGroupSlotFull(FactionKey fk)
	{
		return GetCurrentAssignedGroupCount(fk) >= GetRequiredGroupCount();
	}

	bool HasSectorGrid(FactionKey fk)
	{
		return m_mSectorGarrison.Contains(fk);
	}

	array<ref DCO_SectorGarrison> GetSectorGarrison(FactionKey fk)
	{
		array<ref DCO_SectorGarrison> sectors;
		if (m_mSectorGarrison.Find(fk, sectors))
			return sectors;

		return null;
	}

	int GetSectorCount(FactionKey fk)
	{
		int count;
		if (m_mSectorCount.Find(fk, count))
			return count;

		return 0;
	}

	float GetSectorOffset(FactionKey fk)
	{
		float offset;
		if (m_mSectorOffset.Find(fk, offset))
			return offset;

		return 0.0;
	}

	void InitSectorGrid(FactionKey fk, int count, float offset)
	{
		if (count <= 0)
			return;

		array<ref DCO_SectorGarrison> sectors = new array<ref DCO_SectorGarrison>();
		for (int i = 0; i < count; i++)
			sectors.Insert(new DCO_SectorGarrison(i));

		m_mSectorGarrison.Set(fk, sectors);
		m_mSectorCount.Set(fk, count);
		m_mSectorOffset.Set(fk, offset);
	}

	int GetStaffedSectorCount(FactionKey fk)
	{
		array<ref DCO_SectorGarrison> sectors = GetSectorGarrison(fk);
		if (!sectors)
			return 0;

		int n = 0;
		foreach (DCO_SectorGarrison sec : sectors)
		{
			if (sec && sec.m_Group)
				n++;
		}

		return n;
	}

	void ClearSectorGrid(FactionKey fk)
	{
		array<ref DCO_SectorGarrison> sectors = GetSectorGarrison(fk);
		if (sectors)
		{
			foreach (DCO_SectorGarrison sec : sectors)
			{
				if (sec && sec.m_Waypoint)
					SCR_EntityHelper.DeleteEntityAndChildren(sec.m_Waypoint);
			}
		}

		m_mSectorGarrison.Remove(fk);
		m_mSectorCount.Remove(fk);
		m_mSectorOffset.Remove(fk);
	}
}
