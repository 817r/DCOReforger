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
	[Attribute("1", UIWidgets.ComboBox, "Hold: gak pernah keluar gedung. Defend: tim flank kecil boleh keluar. Release: garrison sampai trigger lalu bebas.", category: "Garrison", enums: ParamEnumArray.FromEnum(DCO_EGarrisonMode))]
	protected DCO_EGarrisonMode m_eMode;

	[Attribute("1", UIWidgets.Slider, "Porsi slot tiap gedung yang diisi. Kecil = grup nyebar ke lebih banyak gedung.", category: "Garrison", params: "0.25 1 0.05")]
	protected float m_fFillRatio;

	[Attribute("0", UIWidgets.Slider, "Maksimum gedung per grup. 0 = otomatis dari jumlah anggota.", category: "Garrison", params: "0 10 1")]
	protected int m_iMaxBuildings;

	[Attribute("0", UIWidgets.ComboBox, "Urutan isi kalau pakai lebih dari satu gedung.", category: "Garrison", enums: ParamEnumArray.FromEnum(DCO_EGarrisonFillOrder))]
	protected DCO_EGarrisonFillOrder m_eFillOrder;

	[Attribute("0.7", UIWidgets.Slider, "Bobot arah tembak: 1 = semua jendela menghadap ancaman, 0 = rata 360.", category: "Garrison", params: "0 1 0.05")]
	protected float m_fThreatWeight;

	[Attribute("1", UIWidgets.CheckBox, "Boleh pakai slot atap/balkon.", category: "Garrison")]
	protected bool m_bAllowRoof;

	[Attribute("5", UIWidgets.Slider, "Delay minimum (detik) sebelum slot kosong (anggota mati) diisi ulang.", category: "Garrison", params: "0 60 1")]
	protected float m_fRefillDelayMin;

	[Attribute("15", UIWidgets.Slider, "Delay maksimum (detik) sebelum slot kosong diisi ulang.", category: "Garrison", params: "0 60 1")]
	protected float m_fRefillDelayMax;

	[Attribute("1", UIWidgets.CheckBox, "Kalau di satu gedung tinggal satu orang, tarik dia ke gedung utama grup.", category: "Garrison")]
	protected bool m_bPullLoneToMain;

	[Attribute("0", UIWidgets.CheckBox, "Gambar slot gedung yang dipakai (merah jendela, oranye atap, biru pintu, abu interior).", category: "Garrison")]
	protected bool m_bDebugDraw;

	[Attribute("50", UIWidgets.Slider, "Defend: tim flank boleh keluar sampai radius waypoint + ini (meter).", category: "Garrison", params: "0 300 5")]
	protected float m_fDefendLeashExtra;

	[Attribute("4", UIWidgets.Slider, "Defend: jumlah orang tim flank (1 fireteam). 0 = gak ada tim flank.", category: "Garrison", params: "0 8 1")]
	protected int m_iFlankTeamSize;

	[Attribute("30", UIWidgets.Slider, "Release: musuh sedekat ini (meter) dari anggota garrison -> leash dilepas.", category: "Garrison", params: "0 200 5")]
	protected float m_fReleaseEnemyDist;

	[Attribute("50", UIWidgets.Slider, "Release: korban (persen dari anggota garrison) sebanyak ini -> leash dilepas. 0 = trigger korban mati.", category: "Garrison", params: "0 100 5")]
	protected float m_fReleaseCasualtyPct;

	[Attribute("20", UIWidgets.Slider, "Release: mayoritas anggota ditekan (THREATENED) terus selama ini (detik) -> leash dilepas. 0 = mati.", category: "Garrison", params: "0 120 1")]
	protected float m_fReleaseSuppressedTime;

	[Attribute("0", UIWidgets.CheckBox, "Release: kembali garrison setelah aman lagi.", category: "Garrison")]
	protected bool m_bReturnAfterRelease;

	override SCR_AIWaypointState CreateWaypointState(SCR_AIGroupUtilityComponent groupUtilityComp)
	{
		return new DCO_GarrisonWaypointState(groupUtilityComp, this);
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
	bool GetAllowRoof() { return m_bAllowRoof; }
	void SetAllowRoof(bool v) { m_bAllowRoof = v; }
	bool GetPullLoneToMain() { return m_bPullLoneToMain; }
	void SetPullLoneToMain(bool v) { m_bPullLoneToMain = v; }
	bool GetDebugDraw() { return m_bDebugDraw; }
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

		state.StartGarrison();
		return ENodeResult.SUCCESS;
	}

	static override bool VisibleInPalette() { return true; }

	static override string GetOnHoverDescription() { return "DCO: mulai activity garrison untuk waypoint garrison aktif grup."; }
}
