enum DCO_EGarrisonMode
{
	HOLD,
	DEFEND,
	RELEASE
}

enum DCO_EGarrisonFillOrder
{
	BUILDING_FIRST,
	WINDOWS_FIRST
}

class DCO_GarrisonWaypointClass : SCR_DefendWaypointClass
{
}

class DCO_GarrisonWaypoint : SCR_DefendWaypoint
{
	[Attribute("1", UIWidgets.ComboBox, "Hold: never leaves the building. Defend: a small flank team may go out. Release: garrison until a trigger, then free.", category: "Garrison", enums: ParamEnumArray.FromEnum(DCO_EGarrisonMode))]
	protected DCO_EGarrisonMode m_eMode;

	[Attribute("1", UIWidgets.Slider, "Share of slots filled in each building. Smaller = the group spreads over more buildings.", category: "Garrison", params: "0.25 1 0.05")]
	protected float m_fFillRatio;

	[Attribute("0", UIWidgets.Slider, "Maximum buildings per group. 0 = automatic from member count.", category: "Garrison", params: "0 10 1")]
	protected int m_iMaxBuildings;

	[Attribute("0", UIWidgets.ComboBox, "Fill order when more than one building is used.", category: "Garrison", enums: ParamEnumArray.FromEnum(DCO_EGarrisonFillOrder))]
	protected DCO_EGarrisonFillOrder m_eFillOrder;

	[Attribute("0.7", UIWidgets.Slider, "Fire direction weight: 1 = all windows face the threat, 0 = even 360.", category: "Garrison", params: "0 1 0.05")]
	protected float m_fThreatWeight;

	[Attribute("0.5", UIWidgets.Slider, "Spread members over different floors. 0 = ignore floors, 1 = spread strongly.", category: "Garrison", params: "0 1 0.05")]
	protected float m_fFloorSpread;

	[Attribute("30", UIWidgets.Slider, "Move to another window (other floor first) if no enemy is seen for this long (s) while the group is in contact. 0 = off.", category: "Garrison", params: "0 120 5")]
	protected float m_fRotateIdleTime;

	[Attribute("1", UIWidgets.CheckBox, "Move to another window or floor when under constant fire for 8 s.", category: "Garrison")]
	protected bool m_bRotateWhenSuppressed;

	[Attribute("1", UIWidgets.CheckBox, "Allow roof and balcony slots.", category: "Garrison")]
	protected bool m_bAllowRoof;

	[Attribute("5", UIWidgets.Slider, "Minimum delay (s) before an empty slot (member killed) is refilled.", category: "Garrison", params: "0 60 1")]
	protected float m_fRefillDelayMin;

	[Attribute("15", UIWidgets.Slider, "Maximum delay (s) before an empty slot is refilled.", category: "Garrison", params: "0 60 1")]
	protected float m_fRefillDelayMax;

	[Attribute("1", UIWidgets.CheckBox, "If only one soldier is left in a building, pull them to the group's main building.", category: "Garrison")]
	protected bool m_bPullLoneToMain;

	[Attribute("0", UIWidgets.CheckBox, "Draw the building slots in use (red window, orange roof, blue door, grey interior).", category: "Garrison")]
	protected bool m_bDebugDraw;

	[Attribute("50", UIWidgets.Slider, "Defend: the flank team may go out to the waypoint radius + this (m).", category: "Garrison", params: "0 300 5")]
	protected float m_fDefendLeashExtra;

	[Attribute("4", UIWidgets.Slider, "Defend: flank team size (1 fireteam). 0 = no flank team.", category: "Garrison", params: "0 8 1")]
	protected int m_iFlankTeamSize;

	[Attribute("30", UIWidgets.Slider, "Release: an enemy this close (m) to a garrison member -> leash released.", category: "Garrison", params: "0 200 5")]
	protected float m_fReleaseEnemyDist;

	[Attribute("50", UIWidgets.Slider, "Release: casualties (percent of garrison members) reach this -> leash released. 0 = casualty trigger off.", category: "Garrison", params: "0 100 5")]
	protected float m_fReleaseCasualtyPct;

	[Attribute("20", UIWidgets.Slider, "Release: most members stay suppressed (THREATENED) for this long (s) -> leash released. 0 = off.", category: "Garrison", params: "0 120 1")]
	protected float m_fReleaseSuppressedTime;

	[Attribute("1", UIWidgets.CheckBox, "If there is no building: pick field fighting positions (cover, view toward the threat, off the skyline, in pairs). Off = vanilla Defend.", category: "Garrison")]
	protected bool m_bFieldPositions;

	[Attribute("0", UIWidgets.CheckBox, "Release: return to garrison once it is safe again.", category: "Garrison")]
	protected bool m_bReturnAfterRelease;

	override SCR_AIWaypointState CreateWaypointState(SCR_AIGroupUtilityComponent groupUtilityComp)
	{
		return new DCO_GarrisonWaypointState(groupUtilityComp, this);
	}

	override bool DCO_WantsSetupDialog() { return true; }

	override string DCO_SetupInfo()
	{
		return string.Format("type=garrison mode=%1 fill=%2 maxb=%3 floor_spread=%4 rotate_idle=%5 rotate_supp=%6", m_eMode, m_fFillRatio, m_iMaxBuildings, m_fFloorSpread, m_fRotateIdleTime, m_bRotateWhenSuppressed);
	}

	vector GetThreatDirection()
	{
		vector end = SCR_AIDefendActivity.GetDefendDirection(this, 10);
		vector dir = end - GetOrigin();
		dir[1] = 0;
		return dir.Normalized();
	}

	DCO_EGarrisonMode GetMode() { return m_eMode; }
	void SetMode(DCO_EGarrisonMode mode) { m_eMode = mode; }
	float GetFillRatio() { return m_fFillRatio; }
	void SetFillRatio(float v) { m_fFillRatio = Math.Clamp(v, 0.25, 1); }
	int GetMaxBuildings() { return m_iMaxBuildings; }
	void SetMaxBuildings(int v) { m_iMaxBuildings = Math.ClampInt(v, 0, 10); }
	DCO_EGarrisonFillOrder GetFillOrder() { return m_eFillOrder; }
	void SetFillOrder(DCO_EGarrisonFillOrder v) { m_eFillOrder = v; }
	float GetThreatWeight() { return m_fThreatWeight; }
	void SetThreatWeight(float v) { m_fThreatWeight = Math.Clamp(v, 0, 1); }
	float GetFloorSpread() { return m_fFloorSpread; }
	void SetFloorSpread(float v) { m_fFloorSpread = Math.Clamp(v, 0, 1); }
	float GetRotateIdleTime() { return m_fRotateIdleTime; }
	void SetRotateIdleTime(float v) { m_fRotateIdleTime = Math.Clamp(v, 0, 120); }
	bool GetRotateWhenSuppressed() { return m_bRotateWhenSuppressed; }
	void SetRotateWhenSuppressed(bool v) { m_bRotateWhenSuppressed = v; }
	bool GetAllowRoof() { return m_bAllowRoof; }
	void SetAllowRoof(bool v) { m_bAllowRoof = v; }
	bool GetPullLoneToMain() { return m_bPullLoneToMain; }
	void SetPullLoneToMain(bool v) { m_bPullLoneToMain = v; }
	bool GetDebugDraw() { return m_bDebugDraw && DCO_TestCaseComponent.RunOn(); }
	void SetDebugDraw(bool v) { m_bDebugDraw = v; }
	float GetDefendLeashExtra() { return m_fDefendLeashExtra; }
	void SetDefendLeashExtra(float v) { m_fDefendLeashExtra = Math.Clamp(v, 0, 300); }
	int GetFlankTeamSize() { return m_iFlankTeamSize; }
	void SetFlankTeamSize(int v) { m_iFlankTeamSize = Math.ClampInt(v, 0, 8); }
	float GetReleaseEnemyDist() { return m_fReleaseEnemyDist; }
	void SetReleaseEnemyDist(float v) { m_fReleaseEnemyDist = Math.Clamp(v, 0, 200); }
	float GetReleaseCasualtyPct() { return m_fReleaseCasualtyPct; }
	void SetReleaseCasualtyPct(float v) { m_fReleaseCasualtyPct = Math.Clamp(v, 0, 100); }
	float GetReleaseSuppressedTime() { return m_fReleaseSuppressedTime; }
	void SetReleaseSuppressedTime(float v) { m_fReleaseSuppressedTime = Math.Clamp(v, 0, 120); }
	bool GetFieldPositions() { return m_bFieldPositions; }
	void SetFieldPositions(bool v) { m_bFieldPositions = v; }
	bool GetReturnAfterRelease() { return m_bReturnAfterRelease; }
	void SetReturnAfterRelease(bool v) { m_bReturnAfterRelease = v; }

	float RollRefillDelay_ms()
	{
		float lo = Math.Min(m_fRefillDelayMin, m_fRefillDelayMax);
		float hi = Math.Max(m_fRefillDelayMin, m_fRefillDelayMax);
		return Math.RandomFloat(lo, hi) * 1000;
	}

	float GetRefillDelayMax() { return m_fRefillDelayMax; }
	void SetRefillDelayMax(float v) { m_fRefillDelayMax = Math.Clamp(v, 0, 60); m_fRefillDelayMin = Math.Min(m_fRefillDelayMin, m_fRefillDelayMax); }
}

class DCO_GarrisonWaypointState : SCR_AIDefendWaypointState
{
	void StartGarrison()
	{
		DCO_GarrisonWaypoint wp = DCO_GarrisonWaypoint.Cast(m_Waypoint);
		if (!wp || !m_Utility)
			return;

		m_Utility.CancelActivitiesRelatedToWaypoint(wp, SCR_AIMoveActivity);

		DCO_AIGarrisonActivity existing = m_Utility.DCO_GetGarrisonActivity();
		if (existing && existing.m_RelatedWaypoint == wp)
		{
			EAIActionState state = existing.GetActionState();
			if (state != EAIActionState.FAILED && state != EAIActionState.COMPLETED)
				return;
		}

		m_Utility.AddAction(new DCO_AIGarrisonActivity(m_Utility, wp));
	}
}

class DCO_AIStartGarrison : AITaskScripted
{
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		SCR_AIGroup group = SCR_AIGroup.Cast(owner);
		if (!group)
			return ENodeResult.FAIL;

		SCR_AIGroupUtilityComponent utility = SCR_AIGroupUtilityComponent.Cast(group.FindComponent(SCR_AIGroupUtilityComponent));
		if (!utility)
			return ENodeResult.FAIL;

		DCO_GarrisonWaypointState state = DCO_GarrisonWaypointState.Cast(utility.DCO_GetWaypointState());
		if (!state)
			return ENodeResult.FAIL;

		DCO_GarrisonWaypoint wp = DCO_GarrisonWaypoint.Cast(state.GetWaypoint());
		if (wp && wp.DCO_SetupPending())
		{
			DCO_Perf.Count("garrison:start_wait_setup");
			return ENodeResult.RUNNING;
		}

		state.StartGarrison();
		return ENodeResult.SUCCESS;
	}

	static override bool VisibleInPalette() { return true; }

	static override string GetOnHoverDescription() { return "DCO: start the garrison activity for the group's active garrison waypoint."; }
}
