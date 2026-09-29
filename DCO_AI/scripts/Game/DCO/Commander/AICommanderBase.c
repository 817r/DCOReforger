[ComponentEditorProps(category: "GameScripted/Commander")]
class AICommander_BaseComponentClass : ScriptComponentClass
{
	[Attribute("{35BD6541CBB8AC08}Prefabs/AI/Waypoints/AIWaypoint_Cycle.et", UIWidgets.ResourceNamePicker, desc: "Cycle waypoint to be used for waypoints in hierarchy.", "et", category: "Commander Waypoint Setting")]
	protected ResourceName m_sCycleWaypointPrefab;

	[Attribute("{FFF9518F73279473}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_Move.et", UIWidgets.ResourceNamePicker, desc: "Waypoint to be used Move.", "et", category: "Commander Waypoint Setting")]
	protected ResourceName m_sDefaultMoveWaypointPrefab;

	[Attribute("{D9C14ECEC9772CC6}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_Defend.et", UIWidgets.ResourceNamePicker, desc: "Waypoint to be used Defend.", "et", category: "Commander Waypoint Setting")]
	protected ResourceName m_sDefaultDefendWaypointPrefab;

	[Attribute("{2E6D3ABB8094159A}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_GetIn.et", UIWidgets.ResourceNamePicker, desc: "Waypoint to be used Get In.", "et", category: "Commander Waypoint Setting")]
	protected ResourceName m_sDefaultGetInWaypointPrefab;

	[Attribute("{2602CAB8AB74FBBF}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_GetOut.et", UIWidgets.ResourceNamePicker, desc: "Waypoint to be used Get Out.", "et", category: "Commander Waypoint Setting")]
	protected ResourceName m_sDefaultGetOutWaypointPrefab;

	[Attribute("{2602CAB8AB74FBBF}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_GetOut.et", UIWidgets.ResourceNamePicker, desc: "Tasking For Player.", "et", category: "Commander Player Tasking Setting")]
	protected ResourceName m_sDefaultTaskPlayerMovePrefab;

	[Attribute("{6ED320498A60081C}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_ArtillerySupport.et", UIWidgets.ResourceNamePicker, desc: "Tasking For Player.", "et", category: "Commander Player Tasking Setting")]
	protected ResourceName m_sDefaultShootArtilleryPrefab;

	[Attribute("{B2A2A69B5F42EC49}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_Suppress_Editor.et", UIWidgets.ResourceNamePicker, desc: "Waypoint To Suppress Area.", "et", category: "Commander Player Tasking Setting")]
	protected ResourceName m_sDefaultSuppressPrefab;

	ResourceName GetCycleWaypointPrefab()
	{
		return m_sCycleWaypointPrefab;
	}

	ResourceName GetDefaultMoveWaypointPrefab()
	{
		return m_sDefaultMoveWaypointPrefab;
	}

	ResourceName GetDefaultDefendWaypointPrefab()
	{
		return m_sDefaultDefendWaypointPrefab;
	}

	ResourceName GetDefaultGetInWaypointPrefab()
	{
		return m_sDefaultGetInWaypointPrefab;
	}

	ResourceName GetDefaultGetOutWaypointPrefab()
	{
		return m_sDefaultGetOutWaypointPrefab;
	}

	ResourceName GetDefaultMoveTaskPrefab()
	{
		return m_sDefaultTaskPlayerMovePrefab;
	}

	ResourceName GetShootArtilleryWaypointPrefab()
	{
		return m_sDefaultShootArtilleryPrefab;
	}

	ResourceName GetDefaultSuppressPrefab()
	{
		return m_sDefaultSuppressPrefab;
	}
}

class CMD_FrontlineReconTrack
{
	DCO_GroupUtilityComponent m_Squad;
	float m_fExpireTime;
}

class DCO_FrontlineSegment
{
	vector m_vStart;
	vector m_vEnd;

	vector m_vFacing;

	float m_fPressure;

	CMD_AICommanderObjectiveComponent m_Owned;
	CMD_AICommanderObjectiveComponent m_Threat;

	vector Center()
	{
		return (m_vStart + m_vEnd) * 0.5;
	}

	vector NearestPointTo(vector p)
	{
		vector ab = m_vEnd - m_vStart;
		float  len2 = ab.LengthSq();

		if (len2 < 0.001)
			return m_vStart;

		vector ap = p - m_vStart;
		float  t  = Math.Clamp(vector.Dot(ap, ab) / len2, 0.0, 1.0);

		return m_vStart + (ab * t);
	}
}

enum DCO_EPatrolPattern
{
	RING    = 0,
	LANE    = 1,
	ARC     = 2,
	ADVANCE = 3
}

class AICommander_BaseComponent : ScriptComponent
{
	protected float m_fBaseThinkInterval;
	protected int m_iBaseRetreatThreshold;
	protected float m_fBaseStalemateResponseCooldown;

	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Prefab entity yang punya AICommander_ManagerComponent. Di-spawn otomatis (server) kalau entity ini init sebelum ada manager di world. Kosong = gak auto-spawn.", "et", category: "Commander Manager")]
	protected ResourceName m_sManagerPrefab;

	[Attribute("", UIWidgets.Font, desc: "UID of the Commander.", category: "Commander General Setting"), RplProp()]
	protected string m_sCommanderUID;

	[Attribute("", UIWidgets.Auto, desc: "Faction Key of the Commander.", category: "Commander General Setting")]
	protected FactionKey m_sFactionKey;

	[Attribute("3.0", UIWidgets.Auto, "Number of the Objective Can be processed at the same time", category: "Commander Objective Setting")]
	protected int m_fObjectiveAtTheSameTime;

	[Attribute("1", UIWidgets.CheckBox, "Pake Synchronized Attack (grup ngumpul dulu di staging sampe full/timeout, baru nyerang bareng) -- kalau dimatiin, balik ke behavior lama (grup langsung ke objective satu-satu begitu dapet slot).", category: "Commander Objective Setting")]
	protected bool m_bUseSynchronizedAttack;

	[Attribute("60.0", UIWidgets.EditBox, "Waktu maksimum (detik) nunggu slot ASSAULT penuh sebelum synchronized attack di-release paksa dengan grup yang udah ngumpul (walau belum full).", category: "Commander Objective Setting")]
	protected float m_fSyncAttackMaxWaitTime;

	[Attribute("30.0", UIWidgets.EditBox, "Interval think cycle dalam detik.", category: "Commander Setting")]
	protected float m_fThinkInterval;

	[Attribute("60.0", UIWidgets.EditBox, "Delay Before Start First Iteration", category: "Commander Setting")]
	protected float m_fDelayFirstIteration;

	[Attribute("120.0", UIWidgets.EditBox, "Cooldown (detik) sebelum stalemate response di-trigger lagi per objective", category: "Commander Setting")]
	protected float m_fStalemateResponseCooldown;

	[Attribute("2", UIWidgets.EditBox, "Unit count minimum group sebelum dipaksa retreat.", category: "Commander Setting")]
	protected int m_iRetreatThreshold;

	[Attribute("15.0", UIWidgets.EditBox, "Interval cek capture progress (detik)", category: "Commander Setting")]
	protected float m_fCaptureCheckInterval;

	[Attribute("250.0", UIWidgets.EditBox, "Radius patrol scouting buat grup recon yang dikirim ke frontline (bukan ke objective spesifik).", category: "Commander Setting")]
	protected float m_fFrontlineReconRadius;

	[Attribute("180.0", UIWidgets.EditBox, "Berapa lama (detik) grup ditugasin frontline recon sebelum dilepas balik ke pool (RESERVE).", category: "Commander Setting")]
	protected float m_fFrontlineReconDuration;

	[Attribute("1", UIWidgets.EditBox, "Jumlah grup defend per objective yang masuk gedung (garrison). Sisanya cincin sektor outdoor. 0 = gak pakai garrison.", category: "Commander Setting")]
	protected int m_iGarrisonGroupsPerObjective;

	[Attribute("{F22E21BBBF46819E}Prefabs/AI/Waypoints/AIWaypoint_Garrison.et", UIWidgets.ResourceNamePicker, "Prefab waypoint garrison yang dipakai commander", "et", category: "Commander Setting")]
	protected ResourceName m_sGarrisonWaypointPrefab;

	[Attribute("1", UIWidgets.ComboBox, "Mode garrison dari commander", category: "Commander Setting", enums: ParamEnumArray.FromEnum(DCO_EGarrisonMode))]
	protected DCO_EGarrisonMode m_eGarrisonMode;

	protected ref map<CMD_AICommanderObjectiveComponent, ref array<DCO_GroupUtilityComponent>> m_mObjGarrison = new map<CMD_AICommanderObjectiveComponent, ref array<DCO_GroupUtilityComponent>>();

	[Attribute("250", UIWidgets.Slider, "DEFEND/ARMORED: nembak bebas kalau ada musuh sedekat ini (m) dari grup.", params: "0 1000 10", category: "Commander Setting")]
	protected float m_fDefendEngageDist;

	[Attribute("50", UIWidgets.Slider, "DEFEND: juga nembak bebas kalau musuh masuk radius objective grup + margin ini (m).", params: "0 300 5", category: "Commander Setting")]
	protected float m_fDefendObjectiveMargin;

	float GetDefendEngageDist() { return m_fDefendEngageDist; }
	float GetDefendObjectiveMargin() { return m_fDefendObjectiveMargin; }

	[Attribute("1", UIWidgets.CheckBox, "Grup ARMORED yang idle dan grup yang punya kendaraan sendiri patroli naik kendaraan (radius lebih besar, pelan, lewat jalan).", category: "Commander Setting")]
	protected bool m_bVehiclePatrol;

	[Attribute("2.5", UIWidgets.EditBox, "Pengali radius patroli kendaraan dibanding patroli jalan kaki.", category: "Commander Setting")]
	protected float m_fVehiclePatrolRadiusMul;

	[Attribute("2", UIWidgets.EditBox, "Maksimum grup frontline recon bersamaan. Dulu tanpa batas (dipanggil tiap Think), ~6 grup nyangkut di frontline terus.", category: "Commander Setting")]
	protected int m_iMaxFrontlineRecon;

	protected ref array<ref CMD_FrontlineReconTrack> m_aFrontlineReconTracks = new array<ref CMD_FrontlineReconTrack>();

	[Attribute("500.0", UIWidgets.EditBox, "Jarak maksimum (meter) grup idle boleh ditarik buat patrol. Kalau kandidat terdekat (HQ/objective captured) lebih jauh dari ini, patrol lokal di posisi sekarang aja.", category: "Commander Setting")]
	protected float m_fMaxPatrolPullDistance;

	[Attribute("80.0", UIWidgets.EditBox, "Berapa meter pusat patroli digeser ke arah frontline tiap Think cycle. 0 = pusat diam di anchor (perilaku lama).", category: "Commander Patrol")]
	protected float m_fPatrolFrontlineDrift;

	[Attribute("400.0", UIWidgets.EditBox, "Rem: pusat patroli berhenti merangkak kalau jaraknya ke objective musuh terdekat sudah di bawah nilai ini (ditambah radius objective). Nyegah cadangan nyelonong sendirian ke pertempuran.", category: "Commander Patrol")]
	protected float m_fPatrolFrontlineStandoff;

	[Attribute("1", UIWidgets.CheckBox, desc: "Titik patroli digeser ke tempat dengan ketinggian dan garis pandang lebih baik. Butuh raycast -- matiin kalau kerasa berat.", category: "Commander Patrol")]
	protected bool m_bPatrolTerrainScoring;

	[Attribute("12", UIWidgets.EditBox, "Jatah titik yang boleh dinilai medannya per Think cycle, dibagi ke SEMUA grup idle. Begitu habis, sisanya pakai titik geometri biasa. Ini yang jaga raycast gak meledak waktu grup idle-nya banyak.", category: "Commander Patrol")]
	protected int m_iPatrolSmartBudget;

	[Attribute("150.0", UIWidgets.EditBox, "Pusat patroli baru dianggap terlalu mirip kalau jaraknya di bawah ini dari pusat yang baru dipakai.", category: "Commander Patrol")]
	protected float m_fPatrolMemoryRadius;

	[Attribute("5", UIWidgets.EditBox, "Berapa pusat patroli terakhir yang diingat per grup.", category: "Commander Patrol")]
	protected int m_iPatrolMemorySize;

	[Attribute("0.5", UIWidgets.Range, "Letak simpul frontline di sepanjang pasangan (objective kita -> ancaman terdekat). 0.5 = tanah tak bertuan. Di bawah 0.5 merapat ke wilayah kita (postur bertahan), di atas 0.5 merapat ke musuh (agresif).", params: "0.1 0.9 0.05", category: "Commander Frontline")]
	protected float m_fFrontlineNodeBias;

	[Attribute("220.0", UIWidgets.EditBox, "Bentang busur (derajat) waktu kita cuma pegang SATU objective. Lebih dari 180 supaya sisi sampingnya ikut tertutup; yang terbuka cuma arah belakang.", category: "Commander Frontline")]
	protected float m_fEnvelopeArcDeg;

	[Attribute("2.0", UIWidgets.EditBox, "Radius busur selubung, sebagai pengali radius objective.", category: "Commander Frontline")]
	protected float m_fEnvelopeRadiusMul;

	[Attribute("250.0", UIWidgets.EditBox, "Radius minimum busur selubung, buat objective yang radiusnya kecil.", category: "Commander Frontline")]
	protected float m_fEnvelopeMinRadius;

	[Attribute("5", UIWidgets.EditBox, "Jumlah simpul di busur selubung. Makin banyak makin halus lengkungannya.", category: "Commander Frontline")]
	protected int m_iEnvelopeNodes;

	protected ref array<ref DCO_FrontlineSegment> m_aFrontline = new array<ref DCO_FrontlineSegment>();

	protected string m_sFrontlineReason = "not built yet";

	[Attribute("0.6", UIWidgets.Range, "Pengali radius patroli buat grup paling kecil.", params: "0.2 2 0.05", category: "Commander Patrol")]
	protected float m_fPatrolRadiusMulMin;

	[Attribute("1.6", UIWidgets.Range, "Pengali radius patroli buat grup penuh (12 orang).", params: "0.2 3 0.05", category: "Commander Patrol")]
	protected float m_fPatrolRadiusMulMax;

	protected int m_iPatrolSmartRemaining = 0;

	protected ref map<DCO_GroupUtilityComponent, ref array<vector>> m_mPatrolMemory = new map<DCO_GroupUtilityComponent, ref array<vector>>();

	[Attribute("0.25", UIWidgets.Range, "Titik kumpul ditaruh sejauh (jarak commander->objective x nilai ini) DARI OBJECTIVE, lalu di-clamp ke Min/Max di bawah.", params: "0.05 1 0.01", category: "Commander Staging")]
	protected float m_fStagingLegFraction;

	[Attribute("200.0", UIWidgets.EditBox, "Jarak minimum titik kumpul dari objective. Otomatis dinaikin kalau lebih kecil dari radius objective + margin -- staging DI DALAM radius bikin objective nilai dirinya CONTESTED terus dan capture-nya gak pernah mulai.", category: "Commander Staging")]
	protected float m_fStagingMinDistance;

	[Attribute("600.0", UIWidgets.EditBox, "Jarak maksimum titik kumpul dari objective. Batas atas ini yang bikin perjalanan terakhir tetap pendek berapapun jauhnya commander.", category: "Commander Staging")]
	protected float m_fStagingMaxDistance;

	[Attribute("60.0", UIWidgets.EditBox, "Margin di luar radius objective buat titik kumpul.", category: "Commander Staging")]
	protected float m_fStagingMargin;

	[Attribute("200.0", UIWidgets.EditBox, "Tiap sekian meter perjalanan, dibuat satu waypoint antara. Bikin grup ngikutin rute bertahap, bukan garis lurus panjang. 0 = matiin, balik ke satu waypoint tujuan.", category: "Commander Staging")]
	protected float m_fWaypointLegDistance;

	[Attribute("120.0", UIWidgets.EditBox, "Seberapa jauh titik antara boleh digeser ke samping buat nyari tempat aman (hindari air dan radius objective musuh).", category: "Commander Staging")]
	protected float m_fWaypointNudgeRadius;

	[Attribute("15", UIWidgets.EditBox, "Batas jumlah waypoint sapuan yang dibuat per grup di dalam objective. Tiap waypoint itu entity yang di-spawn, jadi ini yang jaga objective besar gak bikin ratusan entity sekali serang. Kepadatannya dikecilkan proporsional, bentuk sapuannya tetap.", category: "Commander Staging")]
	protected int m_iMaxSearchWaypoints;

	[Attribute("90.0", UIWidgets.EditBox, "Berapa lama (detik) grup suppress support nutupin assault force begitu di-release, sebelum dilepas balik ke pool.", category: "Commander Setting")]
	protected float m_fAssaultSuppressDuration;

	[Attribute("45.0", UIWidgets.EditBox, "Berapa lama (detik) grup suppress cover nutupin arah kontak begitu ada grup lain retreat.", category: "Commander Setting")]
	protected float m_fRetreatCoverDuration;

	[Attribute("50.0", UIWidgets.EditBox, "Radius suppress buat nutupin retreat.", category: "Commander Setting")]
	protected float m_fRetreatCoverRadius;

	[Attribute("400.0", UIWidgets.EditBox, "Jarak minimum sebelum cari transport", category: "Commander Setting")]
	protected float m_fTransportDistanceThreshold;

	[Attribute("175.0", UIWidgets.EditBox, "Radius pencarian kendaraan transport dari grup (m). Dulu 50 m, hampir gak pernah ketemu.", category: "Commander Setting")]
	protected float m_fVehicleSearchRadius;

	float GetVehicleSearchRadius() { return m_fVehicleSearchRadius; }
	void SetVehicleSearchRadius(float r) { m_fVehicleSearchRadius = Math.Clamp(r, 25, 1000); }

	[Attribute("50.0", UIWidgets.EditBox, "Radius Close To Commander", category: "Commander Setting")]
	protected float m_fBaseRadius;

	[Attribute("20", UIWidgets.Slider, "Reserve policy: persen pasukan yang selalu disisain sebagai cadangan (gak dikomit ke serangan / recon / flank). Juga nentuin porsi grup idle buat defend di mode BALANCED. 0 = gak ada cadangan.", params: "0 50 1", category: "Commander Reserve Policy")]
	protected float m_fReservePercent;

	[Attribute("0", UIWidgets.CheckBox, "All-in: commit SEMUA pasukan ke serangan, tanpa cadangan dan tanpa jatah defend di BALANCED. Defender objective yang lagi direbut tetap ditambal.", category: "Commander Reserve Policy")]
	protected bool m_bAllIn;

	[Attribute("0", UIWidgets.CheckBox, "All-in termasuk defender objective belakang (yang aman, jauh dari frontline): mereka dilepas buat ikut serang.", category: "Commander Reserve Policy")]
	protected bool m_bAllInRearDefenders;

	int GetObjectivesAtOnce()				{ return m_fObjectiveAtTheSameTime; }
	void SetObjectivesAtOnce(float value)	{ m_fObjectiveAtTheSameTime = Math.Clamp(Math.Round(value), 1, 10); }
	float GetReservePercent()				{ return m_fReservePercent; }
	void SetReservePercent(float value)		{ m_fReservePercent = Math.Clamp(value, 0, 50); }
	bool IsAllIn()							{ return m_bAllIn; }
	void SetAllIn(bool value)				{ m_bAllIn = value; }
	bool IsAllInRearDefenders()				{ return m_bAllInRearDefenders; }
	void SetAllInRearDefenders(bool value)	{ m_bAllInRearDefenders = value; }

	protected void ReleaseRearDefenders()
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;

		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (!g || g.IsPlayerGroup() || g.GetTask() != DCO_EGroupTask.DEFEND)
				continue;
			CMD_AICommanderObjectiveComponent obj = g.GetGroupObjective();
			if (!obj || !obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID) || IsObjectiveContested(obj))
				continue;

			float front = float.MAX;
			foreach (CMD_AICommanderObjectiveComponent other : mgr.m_aObjective)
			{
				if (other && !other.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
					front = Math.Min(front, vector.Distance(obj.GetOwner().GetOrigin(), other.GetOwner().GetOrigin()));
			}
			if (front < m_fLogiHubRearDist)
				continue;

			DetachFromObjective(g);
			g.CompleteAllWaypoints();
			Print(string.Format("[%1] All-in: defender belakang %2 dilepas dari %3", m_sCommanderUID, g.GetOwner().GetName(), obj.GetOwner().GetName()));
		}
	}

	protected float GetDefendShare()
	{
		if (m_bAllIn)
			return 0;
		return Math.Clamp(m_fReservePercent / 100.0 * 2.5, 0.1, 1);
	}

	[Attribute("1", UIWidgets.CheckBox, desc: "Kalau true, defend group dievaluasi posisinya (dataran tinggi + arah hadap musuh) bukan random murni.", category: "Commander Defend Setting")]
	protected bool m_bEnableKeyDefendSpotEvaluation;

	[Attribute("2", UIWidgets.EditBox, "Maksimum jumlah defend group per objective yang dikirim ke 'key position' hasil evaluasi (sisanya tetap random/patrol seperti biasa).", category: "Commander Defend Setting")]
	protected int m_iMaxKeyDefendPositions;

	[Attribute("2", UIWidgets.ComboBox, "Commander Mode", "", ParamEnumArray.FromEnum(CMD_ECommanderMode), category: "Commander Personality" )]
	protected CMD_ECommanderMode m_eCommanderModeExternal;

	[Attribute("0.5", UIWidgets.Range, "Agresivitas/Eagerness: seberapa cepat commit assault tanpa tunggu recon.\n0 = tunggu recon tiba dulu | 1 = langsung serang tanpa recon", params: "0 1 0.01", category: "Commander Personality")]
	protected float m_fAggression;

	[Attribute("0.5", UIWidgets.Range, "Adaptabilitas: kecepatan switching mode dan reaktivitas commander.\n0 = lambat bereaksi | 1 = sangat responsif", params: "0 1 0.01", category: "Commander Personality")]
	protected float m_fAdaptability;

	[Attribute("0.5", UIWidgets.Range, "Risk Taking: seberapa berani commit force ke objective yang FOGGY (gak ke-cover intel RECON).\n0 = nolak komit sampe ada intel jelas | 1 = tetep maksa nyerang walau buta", params: "0 1 0.01", category: "Commander Personality")]
	protected float m_fRiskTaking;

	[Attribute("0.5", UIWidgets.Range, "Resilience: seberapa tahan commander ngirim grup bertarung sebelum retreat.\n0 = gampang retreat (threshold tinggi) | 1 = tahan banting (threshold rendah, hold sampe abis)", params: "0 1 0.01", category: "Commander Personality")]
	protected float m_fResilience;

	[Attribute("0.5", UIWidgets.Range, "Patience: seberapa lama commander nunggu sebelum reallocate grup dari objective yang stalemate.\n0 = cepet nyerah/realokasi | 1 = sabar nungguin lama", params: "0 1 0.01", category: "Commander Personality")]
	protected float m_fPatience;

	[Attribute("0.5", UIWidgets.Range, "Combat Focus: lebih mentingin ngejar/reinforce musuh yang kedeteksi, atau stay fokus ke objective capture.\n0 = fokus objective, cuekin ancaman kecil | 1 = ngejar musuh kemanapun, reinforce gampang ke-trigger", params: "0 1 0.01", category: "Commander Personality")]
	protected float m_fCombatFocus;

	float GetCombatFocus() { return m_fCombatFocus; }

	[Attribute("8191", UIWidgets.Flags, "Taktik yang boleh dipilih commander. Kalau gak ada yang cocok: Fire and Maneuver (kalau diizinkan), selain itu tunggu.", enums: ParamEnumArray.FromEnum(DCO_ETacticFlag), category: "Commander Tactics")]
	protected int m_iAllowedTactics;

	int GetAllowedTactics()					{ return m_iAllowedTactics; }
	void SetAllowedTactics(int mask)		{ m_iAllowedTactics = mask & 8191; }

	protected ref DCO_CommanderTactics m_Tactics = new DCO_CommanderTactics();
	protected bool m_bTacticAborting;
	DCO_CommanderTactics GetTactics()		{ return m_Tactics; }

	vector TacticStagingPos(CMD_AICommanderObjectiveComponent obj)
	{
		return GetOrCreateStagingPos(obj);
	}

	bool TacticCommit(DCO_GroupUtilityComponent g, CMD_AICommanderObjectiveComponent obj, DCO_EGroupTask task, vector pos, float worldTime)
	{
		if (!g || !obj)
			return false;

		g.CompleteAllWaypoints();
		bool viaTransport = TryAssignTransport(g, pos, worldTime, task);
		if (!viaTransport && !SpawnMoveRoute(g, g.GetOwner().GetOrigin(), pos, worldTime))
			return false;

		if (!viaTransport)
			g.SetTask(task);
		if (g.GetGroupObjective() != obj)
		{
			g.SetGroupObjective(obj);
			if (TaskCountsSlot(task))
				obj.SetObjectiveGroup(m_sFactionKey, 1);
		}
		return true;
	}

	void TacticAppendMove(DCO_GroupUtilityComponent g, vector pos, float worldTime)
	{
		SCR_AIWaypoint wp = SpawnMoveWP(pos);
		if (wp)
			g.MoveTo(wp, worldTime);
	}

	void TacticMove(DCO_GroupUtilityComponent g, vector pos, float worldTime)
	{
		if (!g || g.IsInTransport())
			return;

		g.CompleteAllWaypoints();
		if (!SpawnMoveRoute(g, g.GetOwner().GetOrigin(), pos, worldTime))
			TacticAppendMove(g, pos, worldTime);
	}

	void TacticAssault(DCO_GroupUtilityComponent g, CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		if (!g || !obj || g.IsInTransport())
			return;

		g.CompleteAllWaypoints();
		vector center = obj.GetOwner().GetOrigin();
		vector from = g.GetOwner().GetOrigin();
		vector entry = GetEntryPoint(obj, from);

		array<SCR_AIWaypoint> wps = {};
		GenerateSearchWaypoints(center, obj.GetRadius(), wps, 50.0, ApproachAngle(from, center) * Math.RAD2DEG - 45.0, 90.0);
		SpawnMoveRoute(g, from, entry, worldTime);
		if (!wps.IsEmpty())
			g.MoveToRoute(wps, worldTime);
		g.SetTask(DCO_EGroupTask.ATTACK);

		if (g.GetGroupObjective() != obj)
		{
			g.SetGroupObjective(obj);
			obj.SetObjectiveGroup(m_sFactionKey, 1);
		}
	}

	void TacticRelease(DCO_GroupUtilityComponent g, CMD_AICommanderObjectiveComponent obj)
	{
		if (!g)
			return;

		if (g.GetGroupObjective() == obj && TaskCountsSlot(g.GetTask()) && obj.GetCurrentAssignedGroupCount(m_sFactionKey) > 0)
			obj.SetObjectiveGroup(m_sFactionKey, -1);
		if (!g.IsInTransport())
			g.CompleteAllWaypoints();
		g.SetTask(DCO_EGroupTask.NONE);
		g.SetGroupObjective(null);
	}

	void TacticAbort(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		m_bTacticAborting = true;
		AbortUndermannedAttack(obj, worldTime);
		m_bTacticAborting = false;
	}

	void TacticRestart(CMD_AICommanderObjectiveComponent obj)
	{
		ResetSyncState(obj);
		obj.SetObjectiveState(m_sFactionKey, CMD_EObjectiveState.PENDING);
	}

	[Attribute("", UIWidgets.Object, "Tabel ROE dasar per tugas (kosong = default). Personality memodifikasi tabel ini; nanti doctrine faction yang ngisi.", category: "Commander ROE")]
	protected ref array<ref DCO_ROEEntry> m_aROEBase;

	protected ref DCO_ROETable m_ROE;

	DCO_ROETable GetROETable()
	{
		if (!m_ROE)
		{
			m_ROE = BuildBaseROETable();
			ApplyPersonalityToROE(m_ROE);
		}
		return m_ROE;
	}

	protected DCO_ROETable BuildBaseROETable()
	{
		DCO_ROETable t = new DCO_ROETable();
		t.Set(DCO_ROEEntry.Create(DCO_EGroupTask.NONE,				DCO_EROEMode.HOLD,	0, 0));
		t.Set(DCO_ROEEntry.Create(DCO_EGroupTask.PATROL,			DCO_EROEMode.HOLD,	0, 0));
		t.Set(DCO_ROEEntry.Create(DCO_EGroupTask.TRANSPORT,			DCO_EROEMode.HOLD,	0, 0));
		t.Set(DCO_ROEEntry.Create(DCO_EGroupTask.RECON,				DCO_EROEMode.HOLD,	0, 0));
		t.Set(DCO_ROEEntry.Create(DCO_EGroupTask.ATTACK,			DCO_EROEMode.TIGHT,	160, 100));
		t.Set(DCO_ROEEntry.Create(DCO_EGroupTask.REINFORCE,			DCO_EROEMode.TIGHT,	160, 100));
		t.Set(DCO_ROEEntry.Create(DCO_EGroupTask.FLANK,				DCO_EROEMode.TIGHT,	170, 50));
		t.Set(DCO_ROEEntry.Create(DCO_EGroupTask.SUPPORT_BY_FIRE,	DCO_EROEMode.TIGHT,	300, 300));
		t.Set(DCO_ROEEntry.Create(DCO_EGroupTask.DEFEND,			DCO_EROEMode.TIGHT,	m_fDefendEngageDist, m_fDefendEngageDist));
		t.Set(DCO_ROEEntry.Create(DCO_EGroupTask.GARRISON,			DCO_EROEMode.TIGHT,	m_fDefendEngageDist, m_fDefendEngageDist));
		t.Set(DCO_ROEEntry.Create(DCO_EGroupTask.FIRE_MISSION,		DCO_EROEMode.TIGHT,	40, 40));

		if (m_aROEBase)
		{
			foreach (DCO_ROEEntry e : m_aROEBase)
			{
				if (e)
					t.Set(e);
			}
		}
		return t;
	}

	protected void ApplyPersonalityToROE(DCO_ROETable t)
	{
		t.m_fDistScale = Math.Lerp(0.8, 1.3, m_fAggression);
		t.m_bEngageDetected = m_fRiskTaking >= 0.5;
		t.m_fHoldEngageDist = 0;
		if (m_fCombatFocus > 0.5)
			t.m_fHoldEngageDist = Math.Lerp(0, 80, (m_fCombatFocus - 0.5) * 2);
		t.m_bFireWhenSuppressed = m_fResilience >= 0.6;
	}

	[Attribute("30.0", UIWidgets.EditBox, "Interval (detik) recon standing ngereveal musuh di sekitarnya ke threat response.", category: "Intel")]
	protected float m_fReconRevealInterval;

	[Attribute("600.0", UIWidgets.EditBox, "Radius (meter) buat nyari objective captured LAIN yang bisa di-link jadi 1 rute patrol.", category: "Patrol")]
	protected float m_fPatrolLinkRadius;

	[Attribute("0.4", UIWidgets.Range, "Chance grup defend dapet Objective-Link Patrol (roaming antar objective) ketimbang Perimeter Patrol (muter di 1 objective doang).", params: "0 1 0.01", category: "Patrol")]
	protected float m_fLinkPatrolChance;

    [Attribute("false", UIWidgets.CheckBox, desc: "Random Personality Every Playthough?", category: "Commander Personality")]
    bool m_bRandomPersonality;

	[Attribute("3", UIWidgets.EditBox, "Jumlah sektor MINIMUM per objective. Clamp bawah -- ngegigit di radius kecil (< ~29m dengan arc 60).", category: "Commander Defend Setting")]
	protected int m_iMinSector;

	[Attribute("8", UIWidgets.EditBox, "Jumlah sektor MAKSIMUM per objective. Ini praktis satu-satunya plafon manpower garnisun -- naikin dengan hati-hati.", category: "Commander Defend Setting")]
	protected int m_iMaxSector;

	[Attribute("60.0", UIWidgets.EditBox, "Panjang busur (meter) yang diwakili satu sektor. Makin kecil = makin banyak sektor. 60 bikin radius default (30m) jatuh di 3 sektor.", category: "Commander Defend Setting")]
	protected float m_fArcPerSector;

	[Attribute("1", UIWidgets.CheckBox, desc: "Skalakan jumlah sektor pakai personality commander?", category: "Commander Defend Setting")]
	protected bool m_bScaleSectorByPersonality;

	[Attribute("0.75", UIWidgets.Range, "Pengali sektor waktu Eagerness = 1 (agresif -- sektor lebih sedikit, manpower disimpan buat offense).", params: "0.25 2.0 0.05", category: "Commander Defend Setting")]
	protected float m_fSectorPersonalityMin;

	[Attribute("1.25", UIWidgets.Range, "Pengali sektor waktu Eagerness = 0 (hati-hati -- sektor lebih banyak, garnisun lebih rapat).", params: "0.25 2.0 0.05", category: "Commander Defend Setting")]
	protected float m_fSectorPersonalityMax;

	[Attribute("15.0", UIWidgets.EditBox, "Plafon completion radius defend waypoint (meter). Nilai efektif = min(radius x 0.25, ini).", category: "Commander Defend Setting")]
	protected float m_fMaxCompletionRadius;

	[Attribute("1", UIWidgets.CheckBox, desc: "Gate PERTUMBUHAN garnisun ke budget manpower? Replenish tidak pernah di-gate.", category: "Commander Defend Setting")]
	protected bool m_bGateDefendByManpower;

	[Attribute("400.0", UIWidgets.EditBox, "Radius (meter) pencarian kontak buat nentuin arah ancaman sebuah objective.", category: "Commander Defend Setting")]
	protected float m_fThreatDirectionRadius;

	protected int m_iPhaseBudget = -1;

	[Attribute("0", UIWidgets.CheckBox, desc: "Gambar overlay commander ini sebagai shape 3D. Cuma kelihatan waktu Game Master kebuka. Objective, manager, dan grup punya checkbox sendiri-sendiri.", category: "Commander Debug")]
	protected bool m_bDebugMode;

	[Attribute("0.5", UIWidgets.EditBox, "Interval (detik) gambar ulang overlay commander.", category: "Commander Debug")]
	protected float m_fDebugRefreshInterval;

	[Attribute("1", UIWidgets.CheckBox, desc: "Boleh nyomot grup yang lagi ngerjain tugas lain kalau tugas itu kalah penting? Kalau false, commander cuma narik dari grup idle/reserve (behavior lama).", category: "Commander Preemption")]
	protected bool m_bEnablePreemption;

	[Attribute("1.3", UIWidgets.EditBox, "Objective baru harus berapa kali lebih berharga dari tugas grup sekarang sebelum boleh nyomot. 1.0 = comot asal lebih tinggi (bikin grup bolak-balik). Naikin kalau grup keliatan gak konsisten.", category: "Commander Preemption")]
	protected float m_fPreemptionMargin;

	[Attribute("60.0", UIWidgets.EditBox, "Jeda minimum (detik) sebelum grup yang sama boleh dicomot lagi. Nyegah satu grup jadi bola pingpong antar objective.", category: "Commander Preemption")]
	protected float m_fPreemptionCooldown;

	protected ref map<DCO_GroupUtilityComponent, float> m_mLastPreemptTime = new map<DCO_GroupUtilityComponent, float>();

	protected ref array<ref Shape> m_aDebugShapes = new array<ref Shape>();
	protected ref array<ref DebugTextWorldSpace> m_aDebugTexts = new array<ref DebugTextWorldSpace>();
	protected float m_fDebugTimer = 0.0;

	protected float m_fCaptureCheckTimer = 0.0;

	protected float m_fDefensiveTriggerCooldown = 0.0;
	static float DEFENSIVE_COOLDOWN = 180.0;

	protected ref array<CMD_AICommanderObjectiveComponent> m_aObjective = {};

	protected CMD_ECommanderState m_eCommanderState = CMD_ECommanderState.IDLE;
	protected CMD_ECommanderMode m_eCommanderMode = CMD_ECommanderMode.OFFENSIVE;

	protected CMD_ThreatResponseComponent threatComp;
	protected CMD_ArtillerySupport		  artySupport;

	protected ref array<DCO_GroupUtilityComponent> m_aOwnedGroup = {};
	protected ref array<IEntity> m_aVehicle = {};

	protected ref array<DCO_TransportTeamComponent> m_aTransportTeams = {};

	protected IEntity m_MyEnt;

	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mStalemateResponseTime = new map<CMD_AICommanderObjectiveComponent, float>();

	protected ref map<CMD_AICommanderObjectiveComponent, bool>  m_mAssaultReleased    = new map<CMD_AICommanderObjectiveComponent, bool>();
	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mStagingStartTime   = new map<CMD_AICommanderObjectiveComponent, float>();
	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mFirstArrivalTime   = new map<CMD_AICommanderObjectiveComponent, float>();
	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mSyncDeadline       = new map<CMD_AICommanderObjectiveComponent, float>();
	protected const float SYNC_TRAVEL_SPEED_MPS = 1.5;

	protected ref map<CMD_AICommanderObjectiveComponent, vector> m_mStagingPos = new map<CMD_AICommanderObjectiveComponent, vector>();

	[Attribute("1", UIWidgets.CheckBox, "Grup pemain faction ini dapet task (peta + daftar tugas) buat objective yang lagi diserang / dipertahankan commander.", category: "Commander Player Tasking Setting")]
	protected bool m_bPlayerTasking;

	[Attribute("1", UIWidgets.CheckBox, "Grup pemain dihitung penuh di rencana (makan slot objective, ditunggu di sinkronisasi serangan). OFF = cuma bonus.", category: "Commander Player Tasking Setting")]
	protected bool m_bPlayerTaskingCounted;

	[Attribute("2", UIWidgets.EditBox, "Maksimal grup pemain yang dihitung per objective (sisanya bonus).", category: "Commander Player Tasking Setting")]
	protected int m_iPlayerMaxPerObjective;

	[Attribute("60", UIWidgets.EditBox, "Detik grup pemain harus mulai bergerak ke target (acceptance) sebelum diganti grup AI.", category: "Commander Player Tasking Setting")]
	protected float m_fPlayerAcceptTime;

	[Attribute("1.5", UIWidgets.EditBox, "Pengali ETA grup pemain (jarak / kecepatan x pengali + margin).", category: "Commander Player Tasking Setting")]
	protected float m_fPlayerEtaFactor;

	[Attribute("60", UIWidgets.EditBox, "Margin ETA (detik).", category: "Commander Player Tasking Setting")]
	protected float m_fPlayerEtaMargin;

	[Attribute("8", UIWidgets.Slider, "Menit maksimal perjalanan grup pemain (tanpa margin). Lebih dari ini commander nawarin transport; kalau gak ada, role jadi bonus.", params: "2 30 1", category: "Commander Player Tasking Setting")]
	protected float m_fPlayerMaxEtaMin;

	[Attribute("-1", UIWidgets.EditBox, "Stok bala bantuan (jumlah prajurit) buat Commander Spawner yang pakai stok. -1 = tanpa batas. Server JSON commanderStock >= 0 menimpa nilai ini.", category: "Commander Reinforcement")]
	protected int m_iReinforcementStock;

	protected int m_iStockLeft;
	protected bool m_bStockInit;

	[Attribute("1", UIWidgets.CheckBox, "Grup pemain masuk objective sebelum release -> semua grup dirilis lebih awal buat dukung mereka.", category: "Commander Player Tasking Setting")]
	protected bool m_bPlayerEarlyRelease;

	[Attribute("2", UIWidgets.EditBox, "Gagal acceptance/ETA berturut-turut sebanyak ini -> grup cuma bonus (gak ditunggu).", category: "Commander Player Tasking Setting")]
	protected int m_iPlayerFailLimit;

	[Attribute("10", UIWidgets.EditBox, "Menit grup pemain jadi bonus setelah kena batas gagal.", category: "Commander Player Tasking Setting")]
	protected float m_fPlayerBonusMinutes;

	protected ref DCO_PlayerTasking m_PlayerTasking = new DCO_PlayerTasking();

	[Attribute("1", UIWidgets.CheckBox, "MEDEVAC otomatis: korban pingsan (tanpa medic) diangkut ke hub, pulih jadi manpower.", category: "Commander Logistics")]
	protected bool m_bLogiMedevac;

	[Attribute("1", UIWidgets.CheckBox, "EXTRACT otomatis: grup gak bisa bertempur / mundur dijemput ke hub.", category: "Commander Logistics")]
	protected bool m_bLogiExtract;

	[Attribute("1", UIWidgets.CheckBox, "RESUPPLY otomatis pakai kendaraan ber-stasiun resupply. Gak jalan kalau Magic Ammo global ON.", category: "Commander Logistics")]
	protected bool m_bLogiResupply;

	[Attribute("1", UIWidgets.CheckBox, "MEDEVAC / EXTRACT panas dikawal armor nganggur.", category: "Commander Logistics")]
	protected bool m_bLogiEscort;

	[Attribute("6", UIWidgets.EditBox, "Prioritas MEDEVAC", category: "Commander Logistics")]
	protected int m_iLogiPrioMedevac;
	[Attribute("5", UIWidgets.EditBox, "Prioritas EXTRACT", category: "Commander Logistics")]
	protected int m_iLogiPrioExtract;
	[Attribute("4", UIWidgets.EditBox, "Prioritas REINFORCE", category: "Commander Logistics")]
	protected int m_iLogiPrioReinforce;
	[Attribute("3", UIWidgets.EditBox, "Prioritas DEPLOY (serang / flank / recon / suppress)", category: "Commander Logistics")]
	protected int m_iLogiPrioDeploy;
	[Attribute("2", UIWidgets.EditBox, "Prioritas REDEPLOY / patroli jauh", category: "Commander Logistics")]
	protected int m_iLogiPrioRedeploy;
	[Attribute("1", UIWidgets.EditBox, "Prioritas RESUPPLY", category: "Commander Logistics")]
	protected int m_iLogiPrioResupply;
	[Attribute("5", UIWidgets.EditBox, "Prioritas minimal permintaan pemain (transport / extract / medevac)", category: "Commander Logistics")]
	protected int m_iLogiPrioPlayer;

	[Attribute("0.3", UIWidgets.Slider, "Transport cuma dipakai kalau ETA-nya lebih cepat dari jalan kaki minimal segini (0.3 = 30%).", params: "0 0.9 0.05", category: "Commander Logistics")]
	protected float m_fLogiEtaMargin;

	[Attribute("150", UIWidgets.EditBox, "Batching: jarak titik jemput & tujuan (m) biar grup / korban digabung satu trip.", category: "Commander Logistics")]
	protected float m_fLogiBatchRadius;

	[Attribute("20", UIWidgets.EditBox, "Batching: detik sejak trip dimulai grup lain masih boleh ikut.", category: "Commander Logistics")]
	protected float m_fLogiBatchWindow;

	[Attribute("250", UIWidgets.EditBox, "Jarak aman LZ / rute dari ancaman (m, ditambah ketidakpastian posisi ancaman).", category: "Commander Logistics")]
	protected float m_fLogiLZSafeDist;

	[Attribute("180", UIWidgets.EditBox, "Detik rawat korban di hub sebelum pulih.", category: "Commander Logistics")]
	protected float m_fLogiTreatTime;

	[Attribute("400", UIWidgets.EditBox, "EXTRACT cuma kalau grup lebih jauh dari ini ke hub (m).", category: "Commander Logistics")]
	protected float m_fLogiExtractMinDist;

	[Attribute("2", UIWidgets.EditBox, "RESUPPLY: magazen cadangan di bawah ini = tipis (setengah grup tipis -> resupply).", category: "Commander Logistics")]
	protected int m_iLogiLowAmmoMags;

	[Attribute("500", UIWidgets.EditBox, "Hub otomatis: objective sendiri minimal sejauh ini dari objective yang bukan milik sendiri (m).", category: "Commander Logistics")]
	protected float m_fLogiHubRearDist;

	protected ref DCO_Logistics m_Logistics = new DCO_Logistics();

	[Attribute("2.0", UIWidgets.EditBox, "Rasio minimum kekuatan serang vs perkiraan musuh untuk commander PALING agresif (gak bisa diturunin Aggression).", category: "Commander Attack")]
	protected float m_fAttackRatioAggressive;

	[Attribute("3.0", UIWidgets.EditBox, "Rasio minimum kekuatan serang vs perkiraan musuh untuk commander paling hati-hati (Aggression 0).", category: "Commander Attack")]
	protected float m_fAttackRatioCautious;

	[Attribute("300", UIWidgets.EditBox, "Serangan dibatalin karena pasukan kurang sampai batas waktu kumpul -> objective ini dilewati selama segini (detik), commander pilih objective lain.", category: "Commander Attack")]
	protected float m_fAttackAbortCooldown;

	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mAttackCooldown = new map<CMD_AICommanderObjectiveComponent, float>();

	protected ref DCO_CommanderOps m_Ops = new DCO_CommanderOps();
	DCO_CommanderOps GetOps()						{ return m_Ops; }

	[Attribute("1", UIWidgets.CheckBox, "Ambush di jalur pendekatan ke objective sendiri (proyeksi arah ancaman / layar depan objective paling depan).", category: "Commander Defense")]
	protected bool m_bAmbush;

	[Attribute("1", UIWidgets.CheckBox, "Ambush interdiksi MSR: jalan yang KELIHATAN dilewati kendaraan musuh berulang (laporan kontak). Wajib grup ber-AT.", category: "Commander Defense")]
	protected bool m_bAmbushMSR;

	[Attribute("2", UIWidgets.EditBox, "Maksimum ambush aktif bersamaan.", category: "Commander Defense")]
	protected int m_iMaxAmbushes;

	[Attribute("75", UIWidgets.EditBox, "Radius zona tembak ambush (m).", category: "Commander Defense")]
	protected float m_fAmbushKillRadius;

	[Attribute("600", UIWidgets.EditBox, "Ambush tanpa kontak segini lama (detik) -> batal, mundur.", category: "Commander Defense")]
	protected float m_fAmbushMaxWait;

	[Attribute("60", UIWidgets.EditBox, "Ambush putus kontak setelah menembak segini lama (detik).", category: "Commander Defense")]
	protected float m_fAmbushFireTime;

	[Attribute("1", UIWidgets.CheckBox, "Pos pengamatan (OP/LP): tim kecil diam di ketinggian, tahan tembakan, lapor kontak (kualitas laporan lebih tinggi).", category: "Commander Defense")]
	protected bool m_bOP;

	[Attribute("2", UIWidgets.EditBox, "Maksimum OP aktif.", category: "Commander Defense")]
	protected int m_iMaxOP;

	[Attribute("150", UIWidgets.EditBox, "OP mundur kalau musuh lebih dekat dari ini (m).", category: "Commander Defense")]
	protected float m_fOPWithdrawDist;

	[Attribute("600", UIWidgets.EditBox, "OP dirotasi (ditarik) setelah segini lama (detik).", category: "Commander Defense")]
	protected float m_fOPRotateTime;

	[Attribute("1", UIWidgets.CheckBox, "Serangan balik segera saat objective sendiri jatuh, kalau rasio kekuatan cadangan terdekat cukup.", category: "Commander Defense")]
	protected bool m_bCounterattack;

	[Attribute("120", UIWidgets.EditBox, "Batas waktu serangan balik setelah objective jatuh (detik) -- sebelum musuh selesai konsolidasi. Lewat = rencana operasi biasa.", category: "Commander Defense")]
	protected float m_fCounterattackWindow;

	[Attribute("1", UIWidgets.CheckBox, "Counter-battery: perkiraan posisi artileri musuh dari hantaman (makin akurat tiap hantaman), balas tembak atau kirim grup pemburu.", category: "Commander Defense")]
	protected bool m_bCounterBattery;

	[Attribute("250", UIWidgets.EditBox, "Error perkiraan posisi artileri musuh di hantaman pertama (m); mengecil dibagi akar jumlah hantaman.", category: "Commander Defense")]
	protected float m_fCBInitialError;

	[Attribute("1", UIWidgets.CheckBox, "Pengelabuan: serangan pura-pura upaya pendukung dan/atau asap di sisi lain sebelum upaya utama release.", category: "Commander Defense")]
	protected bool m_bFeint;

	[Attribute("60", UIWidgets.EditBox, "Pengelabuan jalan segini lama (detik) sebelum upaya utama release.", category: "Commander Defense")]
	protected float m_fFeintLead;

	protected ref DCO_CommanderDefense m_Defense = new DCO_CommanderDefense();
	DCO_CommanderDefense GetDefense()			{ return m_Defense; }
	bool IsAmbushEnabled()						{ return m_bAmbush; }
	bool IsMSRAmbushEnabled()					{ return m_bAmbushMSR; }
	int GetMaxAmbushes()						{ return m_iMaxAmbushes; }
	float GetAmbushKillRadius()					{ return m_fAmbushKillRadius; }
	float GetAmbushMaxWait()					{ return m_fAmbushMaxWait; }
	float GetAmbushFireTime()					{ return m_fAmbushFireTime; }
	bool IsOPEnabled()							{ return m_bOP; }
	int GetMaxOP()								{ return m_iMaxOP; }
	float GetOPWithdrawDist()					{ return m_fOPWithdrawDist; }
	float GetOPRotateTime()						{ return m_fOPRotateTime; }
	bool IsCounterattackEnabled()				{ return m_bCounterattack; }
	float GetCounterattackWindow()				{ return m_fCounterattackWindow; }
	bool IsCounterBatteryEnabled()				{ return m_bCounterBattery; }
	float GetCBInitialError()					{ return m_fCBInitialError; }
	bool IsFeintEnabled()						{ return m_bFeint; }
	float GetFeintLead()						{ return m_fFeintLead; }
	void SetAmbushEnabled(bool b)				{ m_bAmbush = b; }
	void SetMSRAmbushEnabled(bool b)			{ m_bAmbushMSR = b; }
	void SetOPEnabled(bool b)					{ m_bOP = b; }
	void SetCounterattackEnabled(bool b)		{ m_bCounterattack = b; }
	void SetCounterBatteryEnabled(bool b)		{ m_bCounterBattery = b; }
	void SetFeintEnabled(bool b)				{ m_bFeint = b; }

	void SetObjectiveCooldown(CMD_AICommanderObjectiveComponent obj, float seconds)
	{
		m_mAttackCooldown.Set(obj, GetGame().GetWorld().GetWorldTime() / 1000.0 + seconds);
	}

	void StartObjectiveGarrison(DCO_GroupUtilityComponent grp, CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		if (TaskCountsSlot(grp.GetTask()) && grp.GetGroupObjective() == obj && obj.GetCurrentAssignedGroupCount(m_sFactionKey) > 0)
			obj.SetObjectiveGroup(m_sFactionKey, -1);
		DCO_GarrisonWaypoint wp = SpawnGarrisonWP(obj.GetOwner().GetOrigin(), ComputeThreatAngle(obj), obj.GetRadius());
		grp.CompleteAllWaypoints();
		grp.SetTask(DCO_EGroupTask.DEFEND);
		grp.SetGroupObjective(obj);
		if (wp)
			grp.MoveTo(wp, worldTime);
	}

	float GetMinAttackRatio()
	{
		return Math.Max(1, Math.Lerp(m_fAttackRatioCautious, m_fAttackRatioAggressive, m_fAggression));
	}

	int EstimateEnemyAt(CMD_AICommanderObjectiveComponent obj)
	{
		if (!threatComp || !obj)
			return 0;
		vector p = obj.GetOwner().GetOrigin();
		float r = obj.GetRadius() + 150;
		int n;
		foreach (CMD_ThreatEntry t : threatComp.GetThreats())
		{
			if (!t || vector.DistanceXZ(t.m_vBelievedPos, p) > r)
				continue;
			int c = t.m_iEstimatedEnemyCount;
			if (t.m_bArmorSeen)
				c *= 4;
			n += c;
		}
		return n;
	}

	float GetAttackStrength(CMD_AICommanderObjectiveComponent obj)
	{
		float s;
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (!g || g.GetGroupObjective() != obj || !TaskCountsSlot(g.GetTask()))
				continue;
			float w = g.GetUnitCount();
			if (g.IsArmor())
				w *= 3;
			s += w;
		}
		return s;
	}

	bool IsAttackRatioMet(CMD_AICommanderObjectiveComponent obj)
	{
		return m_Ops.IsForceSufficient(this, obj);
	}

	int GetRequiredAttackGroups(CMD_AICommanderObjectiveComponent obj)
	{
		int floor = obj.GetRequiredGroupCount();

		float total;
		int groups;
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (!g || g.IsPlayerGroup() || g.IsDedicatedTransport() || g.IsMortar())
				continue;
			total += g.GetUnitCount();
			groups++;
		}
		float avg = 6;
		if (groups > 0)
			avg = Math.Max(total / groups, 2);

		return Math.Max(floor, m_Ops.RequiredGroups(this, obj, avg));
	}

	bool IsObjectiveOnCooldown(CMD_AICommanderObjectiveComponent obj)
	{
		float until;
		return m_mAttackCooldown.Find(obj, until) && GetGame().GetWorld().GetWorldTime() / 1000.0 < until;
	}

	protected void AbortUndermannedAttack(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		int released;
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (!g || g.IsPlayerGroup() || g.GetGroupObjective() != obj || !TaskCountsSlot(g.GetTask()))
				continue;
			if (obj.GetCurrentAssignedGroupCount(m_sFactionKey) > 0)
				obj.SetObjectiveGroup(m_sFactionKey, -1);
			if (!g.IsInTransport())
				g.CompleteAllWaypoints();
			g.SetTask(DCO_EGroupTask.NONE);
			g.SetGroupObjective(null);
			released++;
		}
		ResetSyncState(obj);
		m_mAttackCooldown.Set(obj, worldTime + m_fAttackAbortCooldown);
		if (!m_bTacticAborting)
			m_Tactics.OnAborted(obj, "undermanned");

		string line = string.Format("attack_abort cmd=%1 obj=%2 strength=%3 enemy_est=%4 ratio_needed=%5 groups_released=%6",
			m_sCommanderUID, obj.GetOwner().GetName(), GetAttackStrength(obj), EstimateEnemyAt(obj), GetMinAttackRatio().ToString(-1, 1), released);
		Print("[" + m_sCommanderUID + "] " + line);
		DCO_BenchmarkLoggerComponent.Event(line);
	}

	DCO_Logistics GetLogistics()		{ return m_Logistics; }
	bool IsLogiMedevac()				{ return m_bLogiMedevac; }
	bool IsLogiExtract()				{ return m_bLogiExtract; }
	bool IsLogiResupply()				{ return m_bLogiResupply; }
	bool IsLogiEscort()					{ return m_bLogiEscort; }
	void SetLogiMedevac(bool value)		{ m_bLogiMedevac = value; }
	void SetLogiExtract(bool value)		{ m_bLogiExtract = value; }
	void SetLogiResupply(bool value)	{ m_bLogiResupply = value; }
	void SetLogiEscort(bool value)		{ m_bLogiEscort = value; }
	int GetLogiPlayerPriority()			{ return m_iLogiPrioPlayer; }
	float GetLogiEtaMargin()			{ return m_fLogiEtaMargin; }
	float GetLogiBatchRadius()			{ return m_fLogiBatchRadius; }
	float GetLogiBatchWindow()			{ return m_fLogiBatchWindow; }
	float GetLogiLZSafeDist()			{ return m_fLogiLZSafeDist; }
	float GetLogiTreatTime()			{ return m_fLogiTreatTime; }
	float GetLogiExtractMinDist()		{ return m_fLogiExtractMinDist; }
	int GetLogiLowAmmoMags()			{ return m_iLogiLowAmmoMags; }
	array<DCO_TransportTeamComponent> GetTransportTeams()	{ return m_aTransportTeams; }

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
				source = "auto " + pick.GetOwner().GetName();
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
		grp.SetTask(DCO_EGroupTask.NONE);
		grp.SetGroupObjective(null);
	}

	[Attribute("1.5", UIWidgets.EditBox, "Moral grup (rata-rata, 0 segar - 3.7 break) di atas ini = gak dikasih tugas serang/flank. Resilience menaikkan toleransi (+/-0.6).", category: "Commander Personality")]
	protected float m_fMoraleAttackThreshold;

	float GetMoraleAttackThreshold()
	{
		return m_fMoraleAttackThreshold + Math.Lerp(-0.6, 0.6, m_fResilience);
	}

	bool IsMoraleFitForAttack(DCO_GroupUtilityComponent grp)
	{
		return grp.GetGroupMorale() <= GetMoraleAttackThreshold();
	}

	array<DCO_GroupUtilityComponent> GetOwnedGroups()	{ return m_aOwnedGroup; }
	bool IsPlayerTaskingEnabled()		{ return m_bPlayerTasking; }
	bool IsPlayerTaskingCounted()		{ return m_bPlayerTaskingCounted; }
	int GetPlayerMaxPerObjective()		{ return m_iPlayerMaxPerObjective; }
	float GetPlayerAcceptTime()			{ return m_fPlayerAcceptTime; }
	void SetPlayerAcceptTime(float value)	{ m_fPlayerAcceptTime = Math.Clamp(value, 15, 300); }
	float GetPlayerEtaFactor()			{ return m_fPlayerEtaFactor; }
	float GetPlayerEtaMargin()			{ return m_fPlayerEtaMargin; }
	float GetPlayerMaxEtaMin()			{ return m_fPlayerMaxEtaMin; }

	int GetReinforcementStock()
	{
		if (!m_bStockInit)
		{
			m_bStockInit = true;
			m_iStockLeft = m_iReinforcementStock;
			DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
			if (cfg && cfg.GetCommanderStockDefault() >= 0)
				m_iStockLeft = cfg.GetCommanderStockDefault();
		}
		return m_iStockLeft;
	}

	void SetReinforcementStock(int value)
	{
		m_bStockInit = true;
		m_iStockLeft = Math.Max(value, -1);
	}

	bool ConsumeStock(int soldiers)
	{
		int stock = GetReinforcementStock();
		if (stock < 0)
			return true;
		if (stock < soldiers)
			return false;
		m_iStockLeft = stock - soldiers;
		return true;
	}
	void SetPlayerMaxEtaMin(float value)	{ m_fPlayerMaxEtaMin = Math.Clamp(value, 2, 30); }
	bool GetPlayerEarlyRelease()		{ return m_bPlayerEarlyRelease; }
	int GetPlayerFailLimit()			{ return m_iPlayerFailLimit; }
	float GetPlayerBonusMinutes()		{ return m_fPlayerBonusMinutes; }
	DCO_PlayerTasking GetPlayerTasking()	{ return m_PlayerTasking; }

	bool IsAssaultReleased(CMD_AICommanderObjectiveComponent obj)
	{
		bool released;
		return m_mAssaultReleased.Find(obj, released) && released;
	}

	bool ShiftStaging(CMD_AICommanderObjectiveComponent obj, vector threatPos, float dist, float worldTime)
	{
		vector st;
		if (!m_mStagingPos.Find(obj, st) || IsAssaultReleased(obj))
			return false;

		vector away = st - threatPos;
		away[1] = 0;
		if (away.LengthSq() < 1)
			away = GetOwner().GetOrigin() - st;
		away[1] = 0;
		away.Normalize();
		vector np = st + away * dist;
		np[1] = GetGame().GetWorld().GetSurfaceY(np[0], np[2]);
		m_mStagingPos.Set(obj, np);

		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (!g || g.GetGroupObjective() != obj || !TaskCountsSlot(g.GetTask()) || g.IsInTransport() || g.IsPlayerGroup())
				continue;
			g.CompleteAllWaypoints();
			SpawnMoveRoute(g, g.GetOwner().GetOrigin(), np, worldTime);
		}
		Print(string.Format("[%1] Staging %2 digeser %3 m menjauhi ancaman", m_sCommanderUID, obj.GetOwner().GetName(), Math.Round(dist)));
		return true;
	}

	bool GetStagingPos(CMD_AICommanderObjectiveComponent obj, out vector pos)
	{
		return m_mStagingPos.Find(obj, pos);
	}

	bool SendSupportSquad(DCO_GroupUtilityComponent grp, DCO_EGroupTask task, vector movePos, vector suppressPos)
	{
		float now = GetGame().GetWorld().GetWorldTime() / 1000.0;
		grp.CompleteAllWaypoints();
		grp.SetGroupObjective(null);
		grp.SetTask(task);

		if (TryAssignTransport(grp, movePos, now, task))
			return true;

		if (!SpawnMoveRoute(grp, grp.GetOwner().GetOrigin(), movePos, now))
			return false;

		if (task == DCO_EGroupTask.SUPPORT_BY_FIRE && suppressPos != vector.Zero)
		{
			SCR_AIWaypoint wp = SpawnSuppressWP(suppressPos);
			SCR_AIGroup aiGrp = SCR_AIGroup.Cast(grp.GetOwner());
			if (wp && aiGrp)
				aiGrp.AddWaypoint(wp);
		}
		return true;
	}

	float GetAssaultCountdown(CMD_AICommanderObjectiveComponent obj)
	{
		if (IsAssaultReleased(obj))
			return 0;

		float first;
		if (!m_mFirstArrivalTime.Find(obj, first))
			return -1;

		float now = GetGame().GetWorld().GetWorldTime() / 1000.0;
		return Math.Max(0, first + m_fSyncAttackMaxWaitTime - now);
	}

	string DescribeOperationGroups(CMD_AICommanderObjectiveComponent obj)
	{
		string s;
		vector objPos = obj.GetOwner().GetOrigin();
		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || grp.GetGroupObjective() != obj || !TaskCountsSlot(grp.GetTask()))
				continue;

			if (!s.IsEmpty())
				s += ", ";
			string what = "assault";
			if (grp.GetTask() == DCO_EGroupTask.FLANK)
				what = "flank";
			s += string.Format("%1 (%2 from the %3)", DCO_PlayerComms.GetCallsign(SCR_AIGroup.Cast(grp.GetOwner())), what,
				DCO_PlayerComms.Bearing(objPos, grp.GetOwner().GetOrigin()));
		}
		return s;
	}

	protected void NotifyPlayersReleased(CMD_AICommanderObjectiveComponent obj)
	{
		if (m_bPlayerTasking && obj)
			m_PlayerTasking.OnAssaultReleased(this, obj);
	}

	void ReleaseAssaultEarly(CMD_AICommanderObjectiveComponent obj)
	{
		if (!obj || IsAssaultReleased(obj) || !m_mStagingStartTime.Contains(obj))
			return;

		Print(string.Format("[%1] Grup pemain masuk %2 duluan -- serangan dirilis lebih awal", m_sCommanderUID, obj.GetOwner().GetName()));
		m_mAssaultReleased.Set(obj, true);
		ReleaseSynchronizedAssault(obj, GetGame().GetWorld().GetWorldTime() / 1000.0);
	}

	bool GetPlayerTaskTarget(vector fromPos, out CMD_AICommanderObjectiveComponent outObj, out DCO_EPlayerTaskType outType, out vector outPos, array<CMD_AICommanderObjectiveComponent> skip = null)
	{
		float best = float.MAX;
		foreach (CMD_AICommanderObjectiveComponent obj : m_aObjective)
		{
			if (!obj || obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID) || (skip && skip.Contains(obj)))
				continue;

			float d = vector.DistanceSq(fromPos, obj.GetOwner().GetOrigin());
			if (d < best)
			{
				best = d;
				outObj = obj;
			}
		}

		if (outObj)
		{
			bool released;
			vector staging;
			m_mAssaultReleased.Find(outObj, released);
			if (m_bUseSynchronizedAttack && !released && m_mStagingStartTime.Contains(outObj) && m_mStagingPos.Find(outObj, staging))
			{
				outType = DCO_EPlayerTaskType.MOVE;
				outPos = staging;
			}
			else
			{
				outType = DCO_EPlayerTaskType.ATTACK;
				outPos = outObj.GetOwner().GetOrigin();
			}
			return true;
		}

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return false;

		foreach (CMD_AICommanderObjectiveComponent own : mgr.m_aObjective)
		{
			if (!own || !own.IsCapturedBy(m_sFactionKey, m_sCommanderUID) || !IsObjectiveContested(own))
				continue;

			float d = vector.DistanceSq(fromPos, own.GetOwner().GetOrigin());
			if (d < best)
			{
				best = d;
				outObj = own;
			}
		}

		if (!outObj)
			return false;

		outType = DCO_EPlayerTaskType.DEFEND;
		outPos = outObj.GetOwner().GetOrigin();
		return true;
	}

	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mAssignedTime = new map<CMD_AICommanderObjectiveComponent, float>();

	[Attribute("45.0", UIWidgets.EditBox, "Waktu maksimum (detik) nunggu recon konfirmasi sebelum SKIP dan tetep nyerang. Recon opsional -- ini nyegah commander stuck nunggu recon yang gak kunjung dateng/gak available.", category: "Commander Objective Setting")]
	protected float m_fReconWaitTimeout;

	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mLastReconRevealTime = new map<CMD_AICommanderObjectiveComponent, float>();

	protected float m_fThinkTimer = 0 - m_fDelayFirstIteration;

	int GetOwnedGroupCount() { return m_aOwnedGroup.Count(); }
	CMD_ArtillerySupport GetArtySupport() { return artySupport; }

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
	int GetOwnedVehicle()	{return m_aVehicle.Count(); }

	protected int m_iManpowerTotalCache = 0;

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
		return m_iManpowerTotalCache * m_fReservePercent / 100.0;
	}

	bool CanCommitGroup(DCO_GroupUtilityComponent grp)
	{
		if (m_bAllIn || m_fReservePercent <= 0)
			return grp != null;

		if (!grp)
			return false;

		float reserveAfterCommit = GetReserveManpower() - grp.GetUnitCount();
		return reserveAfterCommit >= GetReserveFloor();
	}

	bool send = true;

	bool RegisterGroup(DCO_GroupUtilityComponent grp)
	{
		if (!grp)
			return false;

		if (grp.IsMortar())
		{
			if (!artySupport)
				return false;
			artySupport.RegisterArtilleryGroup(grp);
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

		m_mPatrolMemory.Remove(grp);

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

		Print(string.Format("[%1] AssignGroup: %2", m_sCommanderUID, grp.GetOwner().GetName()));
		return true;
	}

	void ReleaseGroup(DCO_GroupUtilityComponent grp)
	{
		if (!Replication.IsServer() || !grp)
			return;

		if (!GetGame() || !GetGame().GetWorld())
			return;

		float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
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

		if (m_mPatrolMemory.Contains(grp))
			m_mPatrolMemory.Remove(grp);

		for (int i = m_aFrontlineReconTracks.Count() - 1; i >= 0; i--)
		{
			CMD_FrontlineReconTrack track = m_aFrontlineReconTracks[i];
			if (track && track.m_Squad == grp)
				m_aFrontlineReconTracks.Remove(i);
		}

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

		Print(string.Format("[%1] ReleaseGroup: %2", m_sCommanderUID, grp.GetOwner().GetName()));
	}

	bool AssignVehicle(IEntity veh)
	{
		if (!Replication.IsServer() || !veh)
			return false;

		DCO_TransportMissionComponent mission = DCO_TransportMissionComponent.Cast(veh.FindComponent(DCO_TransportMissionComponent));
		if (!mission)
			return false;

		SCR_VehicleFactionAffiliationComponent fac = SCR_VehicleFactionAffiliationComponent.Cast(veh.FindComponent(SCR_VehicleFactionAffiliationComponent));
		if (!fac)
			return false;

		FactionKey vehFk;
		if (fac.GetAffiliatedFaction())
			vehFk = fac.GetAffiliatedFactionKey();
		else
			vehFk = fac.GetDefaultFactionKey();

		if (vehFk != m_sFactionKey)
			return false;

		AICommander_BaseComponent current = mission.GetCommanderOwner();
		if (current == this)
			return true;

		if (current)
			current.ReleaseVehicle(veh);

		RegisterVehicle(veh);
		mission.ActivateForCommander(this);

		Print(string.Format("[%1] AssignVehicle: %2", m_sCommanderUID, veh.GetName()));
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
			mission.ReleaseFromCommander(GetGame().GetWorld().GetWorldTime() / 1000.0);

		Print(string.Format("[%1] ReleaseVehicle: %2", m_sCommanderUID, veh.GetName()));
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

		float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
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

	protected void InitializeCommander()
	{
		AICommander_ManagerComponent.GetOrSpawnInstance(m_sManagerPrefab, m_MyEnt);
		if (!AICommander_ManagerComponent.GetInstance()) return;
		AICommander_ManagerComponent.GetInstance().RegisterCommander(this);
		threatComp = CMD_ThreatResponseComponent.Cast(m_MyEnt.FindComponent(CMD_ThreatResponseComponent));

		m_eCommanderMode = m_eCommanderModeExternal;
		if (m_bRandomPersonality)
		{
			m_fAggression  = Math.RandomFloat01();
			m_fAdaptability = Math.RandomFloat01();
			m_fRiskTaking  = Math.RandomFloat01();
			m_fResilience  = Math.RandomFloat01();
			m_fPatience    = Math.RandomFloat01();
			m_fCombatFocus = Math.RandomFloat01();
		}
		m_fBaseThinkInterval             = m_fThinkInterval;
		m_iBaseRetreatThreshold          = m_iRetreatThreshold;
		m_fBaseStalemateResponseCooldown = m_fStalemateResponseCooldown;

		float adaptMod   = Math.Lerp(1.5, 0.5, m_fAdaptability);
		m_fThinkInterval = m_fThinkInterval * adaptMod;
		float resilienceMod = Math.Lerp(2.0, 0.5, m_fResilience);
		m_iRetreatThreshold = Math.Max(1, Math.Round(m_iRetreatThreshold * resilienceMod));
		float patienceMod = Math.Lerp(0.4, 2.5, m_fPatience);
		m_fStalemateResponseCooldown = m_fStalemateResponseCooldown * patienceMod;
		m_fThinkTimer = m_fThinkInterval - m_fDelayFirstIteration;

		m_fThinkTimer = m_fThinkTimer - Math.RandomFloat(0.0, m_fThinkInterval);

		artySupport = CMD_ArtillerySupport.Cast(m_MyEnt.FindComponent(CMD_ArtillerySupport));

		Print(string.Format("[%1] INITIALIZED", m_sCommanderUID));
		Print(string.Format("[%1] < Think Timer | > Think Interval [%2] | [%3] < Commander ", m_fThinkTimer, m_fThinkInterval, m_sCommanderUID));
	}

	float GetAggression()                         { return m_fAggression; }
	float GetAdaptability()                        { return m_fAdaptability; }
	float GetRiskTaking()                          { return m_fRiskTaking; }
	float GetResilience()                          { return m_fResilience; }
	float GetPatience()                            { return m_fPatience; }

	void SetCommanderMode(CMD_ECommanderMode mode)
	{
		m_eCommanderModeExternal = mode;
		m_eCommanderMode = mode;
	}

	void SetAggression(float value)  { m_fAggression  = Math.Clamp(value, 0, 1); m_ROE = null; }
	void SetRiskTaking(float value)  { m_fRiskTaking  = Math.Clamp(value, 0, 1); m_ROE = null; }
	void SetCombatFocus(float value) { m_fCombatFocus = Math.Clamp(value, 0, 1); m_ROE = null; }
	bool  GetVehiclePatrol()                    { return m_bVehiclePatrol; }
	void  SetVehiclePatrol(bool value)          { m_bVehiclePatrol = value; }
	float GetVehiclePatrolRadiusMul()           { return m_fVehiclePatrolRadiusMul; }
	void  SetVehiclePatrolRadiusMul(float value){ m_fVehiclePatrolRadiusMul = Math.Clamp(value, 1, 5); }
	int  GetMaxFrontlineRecon()          { return m_iMaxFrontlineRecon; }
	void SetMaxFrontlineRecon(int value) { m_iMaxFrontlineRecon = Math.Max(value, 0); }

	void SetAdaptability(float value)
	{
		m_fAdaptability = Math.Clamp(value, 0, 1);
		ApplyPersonalityDerived();
	}

	void SetResilience(float value)
	{
		m_fResilience = Math.Clamp(value, 0, 1);
		ApplyPersonalityDerived();
	}

	void SetPatience(float value)
	{
		m_fPatience = Math.Clamp(value, 0, 1);
		ApplyPersonalityDerived();
	}

	protected void ApplyPersonalityDerived()
	{
		m_ROE = null;
		if (m_fBaseThinkInterval <= 0)
			return;

		m_fThinkInterval             = m_fBaseThinkInterval * Math.Lerp(1.5, 0.5, m_fAdaptability);
		m_iRetreatThreshold          = Math.Max(1, Math.Round(m_iBaseRetreatThreshold * Math.Lerp(2.0, 0.5, m_fResilience)));
		m_fStalemateResponseCooldown = m_fBaseStalemateResponseCooldown * Math.Lerp(0.4, 2.5, m_fPatience);
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

	CMD_ThreatResponseComponent GetThreatResponseComponent()
	{
		return threatComp;
	}

	FactionKey GetCommanderFactionKey()
	{
		return m_sFactionKey;
	}

	array<CMD_AICommanderObjectiveComponent> GetObjectiveList()
	{
		return m_aObjective;
	}

	void PromoteToArtillery(DCO_GroupUtilityComponent grp)
	{
		if (!grp || !artySupport)
			return;

		m_aOwnedGroup.RemoveItem(grp);
		grp.CompleteAllWaypoints();
		grp.SetTask(DCO_EGroupTask.NONE);
		artySupport.RegisterArtilleryGroup(grp);
		DCO_BenchmarkLoggerComponent.Event(string.Format("arty_unit_registered grp=%1 cmd=%2", grp.GetOwner(), m_sCommanderUID));
	}

	void DemoteFromArtillery(DCO_GroupUtilityComponent grp, string reason)
	{
		if (!grp)
			return;

		if (artySupport)
			artySupport.UnregisterArtilleryGroup(grp);
		grp.CompleteAllWaypoints();
		grp.SetTask(DCO_EGroupTask.NONE);
		RegisterGroup(grp);
		DCO_BenchmarkLoggerComponent.Event(string.Format("arty_unit_demoted grp=%1 cmd=%2 reason=%3", grp.GetOwner(), m_sCommanderUID, reason));
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

	string GetCommanderUID()
	{
		return m_sCommanderUID;
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

	CMD_ECommanderMode GetCommanderMode()      { return m_eCommanderMode; }

	vector ComputeFlankPosition(vector from, vector objective, float distance)
	{
		float angle = Math.RandomFloat(55.0, 80.0);

		vector left  = FlankPoint(from, objective, distance, angle, 1);
		vector right = FlankPoint(from, objective, distance, angle, -1);

		if (EvaluateFlankCandidate(left, objective) >= EvaluateFlankCandidate(right, objective))
			return left;

		return right;
	}

	vector FlankPoint(vector from, vector objective, float distance, float angleDeg, int side)
	{
		float baseAng = ApproachAngle(from, objective);
		float a = baseAng + side * angleDeg * Math.DEG2RAD;

		vector p = objective + Vector(Math.Cos(a) * distance, 0.0, Math.Sin(a) * distance);
		p[1] = GetGame().GetWorld().GetSurfaceY(p[0], p[2]);
		return p;
	}

	protected float ApproachAngle(vector from, vector objective)
	{
		vector d = from - objective;
		if (d[0] * d[0] + d[2] * d[2] < 1.0)
			d = GetOwner().GetOrigin() - objective;

		return Math.Atan2(d[2], d[0]);
	}

	protected vector GetEntryPoint(CMD_AICommanderObjectiveComponent obj, vector from)
	{
		return FlankPoint(from, obj.GetOwner().GetOrigin(), obj.GetRadius() + 60.0, 0.0, 1);
	}

	protected float EvaluateFlankCandidate(vector candidate, vector objective)
	{
		if (!IsRoutePointUsable(candidate))
			return -1.0;

		return Math.Clamp((candidate[1] - objective[1]) / 20.0, -1.0, 1.0) + Math.RandomFloat(0.0, 0.3);
	}

	protected vector GetOrCreateStagingPos(CMD_AICommanderObjectiveComponent obj)
	{
		vector cached;
		if (m_mStagingPos.Find(obj, cached))
			return cached;

		vector objPos = obj.GetOwner().GetOrigin();
		vector base   = GetOwner().GetOrigin();

		vector axis = objPos - base;
		axis        = Vector(axis[0], 0.0, axis[2]);
		axis        = axis.Normalized();
		axis        = m_Ops.AdjustApproachAxis(obj, axis);

		float dist = vector.Distance(base, objPos);

		float minDist = Math.Max(m_fStagingMinDistance, obj.GetRadius() + m_fStagingMargin);
		float maxDist = Math.Max(m_fStagingMaxDistance, minDist);

		float standoff = Math.Clamp(dist * m_fStagingLegFraction, minDist, maxDist);

		if (standoff >= dist)
			standoff = dist * 0.5;

		vector pos = PickCoveredStagingPoint(objPos, axis, standoff, dist);

		pos = PullStagingClearOfHostileObjectives(obj, base, axis, dist, standoff, pos);
		pos[1] = GetGame().GetWorld().GetSurfaceY(pos[0], pos[2]);

		m_mStagingPos.Insert(obj, pos);

		return pos;
	}

	protected vector PickCoveredStagingPoint(vector objPos, vector axis, float standoff, float legDist)
	{
		BaseWorld world = GetGame().GetWorld();
		float baseAng = Math.Atan2(-axis[2], -axis[0]);

		vector best = objPos - axis * standoff;
		float bestScore = -1000.0;

		for (int di = 0; di < 3; di++)
		{
			float d = standoff + di * 100.0;
			if (d >= legDist)
				break;

			for (int ai = -2; ai <= 2; ai++)
			{
				float a = baseAng + ai * 20.0 * Math.DEG2RAD;
				vector cand = objPos + Vector(Math.Cos(a) * d, 0, Math.Sin(a) * d);
				cand[1] = world.GetSurfaceY(cand[0], cand[2]);

				if (!IsRoutePointUsable(cand))
					continue;

				TraceParam trace = new TraceParam();
				trace.Start = Vector(cand[0], cand[1] + 1.7, cand[2]);
				trace.End   = Vector(objPos[0], objPos[1] + 2.0, objPos[2]);
				trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
				DCO_Perf.Count("t:AICommanderBase");
				float hit = world.TraceMove(trace, null);

				float score = 0.0;
				if (hit < 0.85)
					score = score + 2.0;
				score = score - Math.AbsFloat(ai) * 0.15 - di * 0.25;

				if (score > bestScore)
				{
					bestScore = score;
					best = cand;
				}
			}
		}

		return best;
	}

	protected void ClearStagingPos(CMD_AICommanderObjectiveComponent obj)
	{
		if (m_mStagingPos.Contains(obj))
			m_mStagingPos.Remove(obj);
	}

	protected vector PullStagingClearOfHostileObjectives(
		CMD_AICommanderObjectiveComponent target,
		vector base, vector axis, float legDist, float standoff, vector candidate)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return candidate;

		const float MARGIN    = 30.0;
		const int   MAX_PULLS = 6;
		const float PULL_STEP = 80.0;

		vector targetPos = target.GetOwner().GetOrigin();

		for (int attempt = 0; attempt < MAX_PULLS; attempt++)
		{
			bool clashes = false;

			foreach (CMD_AICommanderObjectiveComponent other : mgr.m_aObjective)
			{
				if (!other || !other.GetOwner() || other == target)
					continue;

				if (other.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
					continue;

				float keepOut = other.GetRadius() + MARGIN;

				if (vector.DistanceSq(candidate, other.GetOwner().GetOrigin()) < keepOut * keepOut)
				{
					clashes = true;
					break;
				}
			}

			if (!clashes)
				return candidate;

			standoff = standoff + PULL_STEP;

			if (standoff >= legDist)
				return candidate;

			candidate = targetPos - axis * standoff;
		}

		return candidate;
	}

	protected void RecordAssignedTime(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		if (!m_mAssignedTime.Contains(obj))
			m_mAssignedTime.Insert(obj, worldTime);
		else
			m_mAssignedTime.Set(obj, worldTime);
	}

	protected void AssignRolesToObjective(CMD_AICommanderObjectiveComponent obj, float worldTime, CMD_ObjectiveContextCache contextCache = null)
	{
	    if (obj.GetObjectiveType() == CMD_EObjectiveType.RECON)
	    {
	        AssignReconOnlyToObjective(obj, worldTime);
	        return;
	    }

	    CMD_EObjectiveState objState = obj.GetObjectiveState(m_sFactionKey);

	    if (objState == CMD_EObjectiveState.COMPLETED || objState == CMD_EObjectiveState.FAILED)
	    {
	        ResetSyncState(obj);
	        return;
	    }

	    AICommander_ManagerComponent mgrCheck = AICommander_ManagerComponent.GetInstance();
	    if (objState == CMD_EObjectiveState.PENDING)
	    {
	        ResetSyncState(obj);

	        bool isFoggy;
	        if (mgrCheck && contextCache)
	            isFoggy = !mgrCheck.IsObjectiveIntelCoveredCached(obj, contextCache);
	        else
	            isFoggy = mgrCheck && !mgrCheck.IsObjectiveIntelCovered(obj, m_sFactionKey);

	        if (isFoggy && m_fRiskTaking < Math.RandomFloat01())
	        {
	            TrySendRecon(obj);

	            RecordAssignedTime(obj, worldTime);

	            return;
	        }
	    }

	    if (objState == CMD_EObjectiveState.PENDING)
	    {
	    	if (!m_Tactics.Begin(this, obj, worldTime))
	    		return;

	    	if (m_Tactics.ControlsObjective(obj))
	    	{
	    		obj.MarkAssigned(m_sFactionKey);
	    		RecordAssignedTime(obj, worldTime);
	    		return;
	    	}

	    	if (m_fAggression >= Math.RandomFloat01())
	    	{
	    		TrySendToStaging(obj, worldTime);
	    		obj.MarkAssigned(m_sFactionKey);
	    		RecordAssignedTime(obj, worldTime);
	    		return;
	    	}

	        TrySendRecon(obj);
	        TrySendToStaging(obj, worldTime);
	        obj.MarkAssigned(m_sFactionKey);
	        RecordAssignedTime(obj, worldTime);
	        return;
	    }

	    if (objState == CMD_EObjectiveState.ASSIGNED)
	    {
	        if (m_Tactics.ControlsObjective(obj))
	            return;

	        bool wasReleased;
	        if (m_mAssaultReleased.Find(obj, wasReleased) && wasReleased && !HasCommittedGroups(obj))
	            ResetSyncState(obj);

	        float assignedTime;
	        bool hasAssignedRecord = m_mAssignedTime.Find(obj, assignedTime);
	        bool reconTimedOut = !hasAssignedRecord || (worldTime - assignedTime) > m_fReconWaitTimeout;

	        if (!reconTimedOut && m_fAggression < Math.RandomFloat01() && !obj.IsReconArrived(m_sFactionKey, worldTime))
	        {
	            return;
	        }

	        if (obj.IsUncontested(m_sFactionKey, worldTime))
	        {
	            bool alreadyReleased;
	            if (!m_mAssaultReleased.Find(obj, alreadyReleased))
	                alreadyReleased = false;

	            if (!alreadyReleased && m_mStagingStartTime.Contains(obj))
	            {
	                ReleaseSynchronizedAssault(obj, worldTime);
	                m_mAssaultReleased.Set(obj, true);
	            }

	            TrySendAssaultWithSlots(obj, worldTime);
	            return;
	        }

	        if (m_bUseSynchronizedAttack)
	        {
	            bool released;
	            if (!m_mAssaultReleased.Find(obj, released))
	                released = false;

	            if (!released)
	            {
	                if (!m_mStagingStartTime.Contains(obj))
	                    m_mStagingStartTime.Insert(obj, worldTime);

	                int   stagedHere, stagedTotal;
	                float stagedMaxDist;
	                CountStagingGroups(obj, stagedHere, stagedTotal, stagedMaxDist);

	                if (stagedHere > 0 && !m_mFirstArrivalTime.Contains(obj))
	                {
	                    m_mFirstArrivalTime.Insert(obj, worldTime);
	                    DCO_PlayerAwareness.BroadcastOperation(this, obj, "op_assault_soon", DCO_Radio.P("obj", obj.GetOwner().GetName(), "sec", DCO_Radio.N(m_fSyncAttackMaxWaitTime), "groups", DescribeOperationGroups(obj)));
	                }

	                if (stagedTotal > 0 && !m_mSyncDeadline.Contains(obj))
	                    m_mSyncDeadline.Insert(obj, worldTime + stagedMaxDist / SYNC_TRAVEL_SPEED_MPS + m_fSyncAttackMaxWaitTime);

	                bool timedOut = false;
	                if (m_mFirstArrivalTime.Contains(obj))
	                    timedOut = (worldTime - m_mFirstArrivalTime.Get(obj)) > m_fSyncAttackMaxWaitTime;
	                if (m_mSyncDeadline.Contains(obj) && worldTime > m_mSyncDeadline.Get(obj))
	                    timedOut = true;

	                bool isFull   = obj.IsGroupSlotFull(m_sFactionKey) && obj.GetCurrentAssignedGroupCount(m_sFactionKey) >= GetRequiredAttackGroups(obj);
	                bool arrived  = AreStagedGroupsArrived(obj);
	                bool ratioOk  = IsAttackRatioMet(obj);

	                if (timedOut && !ratioOk)
	                {
	                    AbortUndermannedAttack(obj, worldTime);
	                    return;
	                }

	                if ((isFull && arrived && ratioOk) || timedOut)
	                {
	                    if (!m_Ops.ReadyToRelease(this, obj, worldTime))
	                        return;

	                    if (m_Tactics.HoldRelease(this, obj, worldTime))
	                        return;

	                    ReleaseSynchronizedAssault(obj, worldTime);

	                    if (!m_mAssaultReleased.Contains(obj))
	                        m_mAssaultReleased.Insert(obj, true);
	                    else
	                        m_mAssaultReleased.Set(obj, true);

	                    return;
	                }

	                TryGatherForSynchronizedAssault(obj, worldTime);

	                return;
	            }
	        }

	        TrySendAssaultWithSlots(obj, worldTime);
	    }
	}

	protected void AssignReconOnlyToObjective(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
	    if (obj.IsReconObjectiveActive(m_sFactionKey))
	    {
	        TryReconRevealEnemies(obj, worldTime);
	        return;
	    }

	    TrySendRecon(obj);
	}

	protected void TryReconRevealEnemies(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		if (!threatComp)
			return;

		float lastReveal;
		if (m_mLastReconRevealTime.Find(obj, lastReveal))
		{
			if ((worldTime - lastReveal) < m_fReconRevealInterval)
				return;
		}

		int reconFriendlyCount;
		int enemyCount;
		obj.CountNearbyUnitsCached(obj.GetIntelCoverageRadius(), m_sFactionKey, reconFriendlyCount, enemyCount);

		if (!m_mLastReconRevealTime.Contains(obj))
			m_mLastReconRevealTime.Insert(obj, worldTime);
		else
			m_mLastReconRevealTime.Set(obj, worldTime);

		if (enemyCount <= 0)
			return;

		DCO_GroupUtilityComponent reconGrp = obj.GetReconGroup(m_sFactionKey);

		CMD_ContactReport report = new CMD_ContactReport(
			obj.GetOwner().GetOrigin(),
			enemyCount,
			worldTime,
			"RECON:" + obj.GetOwner().GetName());

		threatComp.ReceiveContactReport(report, reconGrp);
	}

	protected void TrySendRecon(CMD_AICommanderObjectiveComponent obj)
	{
		DCO_GroupUtilityComponent reconGrp = FindBestIdleGroupForTask(DCO_EGroupTask.RECON, obj.GetOwner().GetOrigin());
		if (!reconGrp)
		{
			return;
		}

		if (reconGrp.IsPlayerGroup())
		{
			return;
		}

		if (!CanCommitGroup(reconGrp))
		{
			return;
		}

		reconGrp.CompleteAllWaypoints();

		float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;

		vector objCenter = obj.GetOwner().GetOrigin();
		float  minDist   = obj.GetRadius() + 100.0;
	    vector reconPos  = CMD_ReconSpotFinder.FindBestReconSpot(reconGrp.GetOwner().GetOrigin(), objCenter, minDist, minDist + 200.0, 16);
		if (reconPos == vector.Zero)
			return;

		reconGrp.SetGroupObjective(obj);
 		obj.SetReconGroup(m_sFactionKey, reconGrp);
		obj.MarkAssigned(m_sFactionKey);

		if (TryAssignTransport(reconGrp, reconPos, worldTime, DCO_EGroupTask.RECON))
    		return;

		reconGrp.SetTask(DCO_EGroupTask.RECON);
		SpawnMoveRoute(reconGrp, reconGrp.GetOwner().GetOrigin(), reconPos, worldTime);
	}

	protected void BuildFrontline()
	{
		m_aFrontline.Clear();

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;

		array<CMD_AICommanderObjectiveComponent> own   = {};
		array<CMD_AICommanderObjectiveComponent> hostile = {};

		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj || !obj.GetOwner())
				continue;

			if (obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
				own.Insert(obj);
			else
				hostile.Insert(obj);
		}

		if (hostile.IsEmpty())
		{
			m_sFrontlineReason = "no hostile objectives";
			return;
		}

		if (own.IsEmpty())
		{
			vector groupCentroid;
			if (!TryGetOwnGroupCentroid(groupCentroid))
			{
				m_sFrontlineReason = "no territory and no groups";
				return;
			}

			BuildEnvelopeAt(groupCentroid, 0.0, null, hostile);

			if (m_aFrontline.IsEmpty())
				m_sFrontlineReason = "envelope failed from group centroid";
			else
				m_sFrontlineReason = "from group centroid (no objective held yet)";

			return;
		}

		if (own.Count() == 1)
		{
			BuildEnvelopeFrontline(own[0], hostile);

			if (m_aFrontline.IsEmpty())
				m_sFrontlineReason = "envelope failed";
			else
				m_sFrontlineReason = "envelope around single held objective";

			return;
		}

		BuildPairedFrontline(own, hostile);

		if (m_aFrontline.IsEmpty())
			m_sFrontlineReason = "paired build produced no segments";
		else
			m_sFrontlineReason = "paired from held objectives";
	}

	protected bool TryGetOwnGroupCentroid(out vector centroid)
	{
		vector sum = vector.Zero;
		int    n   = 0;

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || !grp.GetOwner() || grp.IsPlayerGroup() || grp.IsDedicatedTransport())
				continue;

			sum = sum + grp.GetOwner().GetOrigin();
			n   = n + 1;
		}

		if (n <= 0)
			return false;

		centroid    = sum / n;
		centroid[1] = GetGame().GetWorld().GetSurfaceY(centroid[0], centroid[2]);

		return true;
	}

	protected void BuildPairedFrontline(
		notnull array<CMD_AICommanderObjectiveComponent> own,
		notnull array<CMD_AICommanderObjectiveComponent> hostile)
	{
		vector centroid = vector.Zero;
		foreach (CMD_AICommanderObjectiveComponent o : own)
			centroid = centroid + o.GetOwner().GetOrigin();

		centroid = centroid / own.Count();

		array<ref DCO_FrontlineSegment> nodes = {};

		foreach (CMD_AICommanderObjectiveComponent o : own)
		{
			vector oPos = o.GetOwner().GetOrigin();

			CMD_AICommanderObjectiveComponent nearest = null;
			float nearestSq = float.MAX;

			foreach (CMD_AICommanderObjectiveComponent h : hostile)
			{
				float dSq = vector.DistanceSq(oPos, h.GetOwner().GetOrigin());
				if (dSq < nearestSq)
				{
					nearestSq = dSq;
					nearest   = h;
				}
			}

			if (!nearest)
				continue;

			vector hPos = nearest.GetOwner().GetOrigin();
			vector node = oPos + ((hPos - oPos) * m_fFrontlineNodeBias);
			node[1]     = GetGame().GetWorld().GetSurfaceY(node[0], node[2]);

			DCO_FrontlineSegment seg = new DCO_FrontlineSegment();
			seg.m_vStart   = node;
			seg.m_Owned    = o;
			seg.m_Threat   = nearest;
			seg.m_fPressure = ComputeSegmentPressure(nearest, node);

			nodes.Insert(seg);
		}

		if (nodes.Count() < 2)
			return;

		SortNodesByAngle(nodes, centroid);

		for (int i = 0; i < nodes.Count() - 1; i++)
		{
			DCO_FrontlineSegment seg = new DCO_FrontlineSegment();
			seg.m_vStart    = nodes[i].m_vStart;
			seg.m_vEnd      = nodes[i + 1].m_vStart;
			seg.m_Owned     = nodes[i].m_Owned;
			seg.m_Threat    = nodes[i].m_Threat;
			seg.m_fPressure = (nodes[i].m_fPressure + nodes[i + 1].m_fPressure) * 0.5;
			seg.m_vFacing   = ComputeFacing(seg.Center(), centroid);

			m_aFrontline.Insert(seg);
		}
	}

	protected void BuildEnvelopeFrontline(
		CMD_AICommanderObjectiveComponent center,
		notnull array<CMD_AICommanderObjectiveComponent> hostile)
	{
		BuildEnvelopeAt(center.GetOwner().GetOrigin(), center.GetRadius(), center, hostile);
	}

	protected void BuildEnvelopeAt(
		vector cPos,
		float anchorRadius,
		CMD_AICommanderObjectiveComponent anchorObj,
		notnull array<CMD_AICommanderObjectiveComponent> hostile)
	{
		CMD_AICommanderObjectiveComponent nearest = null;
		float nearestSq = float.MAX;

		foreach (CMD_AICommanderObjectiveComponent h : hostile)
		{
			float dSq = vector.DistanceSq(cPos, h.GetOwner().GetOrigin());
			if (dSq < nearestSq)
			{
				nearestSq = dSq;
				nearest   = h;
			}
		}

		if (!nearest)
			return;

		vector facing = nearest.GetOwner().GetOrigin() - cPos;
		facing        = Vector(facing[0], 0.0, facing[2]);

		if (facing.LengthSq() < 1.0)
			return;

		facing = facing.Normalized();

		float baseAngle = Math.Atan2(facing[2], facing[0]) * Math.RAD2DEG;

		float radius;
		if (anchorRadius > 0.0)
		{
			radius = Math.Max(anchorRadius * m_fEnvelopeRadiusMul, m_fEnvelopeMinRadius);
		}
		else
		{
			float toThreat = vector.Distance(cPos, nearest.GetOwner().GetOrigin());
			radius = Math.Clamp(toThreat * 0.4, m_fEnvelopeMinRadius, Math.Max(toThreat - 100.0, m_fEnvelopeMinRadius));
		}

		int nodeCount = Math.Max(3, m_iEnvelopeNodes);

		array<vector> arc = {};

		for (int i = 0; i < nodeCount; i++)
		{
			float t        = i / (float)(nodeCount - 1);
			float angleDeg = baseAngle - (m_fEnvelopeArcDeg * 0.5) + (m_fEnvelopeArcDeg * t);
			float angleRad = angleDeg * Math.DEG2RAD;

			vector p = cPos + Vector(Math.Cos(angleRad) * radius, 0.0, Math.Sin(angleRad) * radius);
			p[1]     = GetGame().GetWorld().GetSurfaceY(p[0], p[2]);

			arc.Insert(p);
		}

		float pressure = ComputeSegmentPressure(nearest, cPos);

		for (int s = 0; s < arc.Count() - 1; s++)
		{
			DCO_FrontlineSegment seg = new DCO_FrontlineSegment();
			seg.m_vStart    = arc[s];
			seg.m_vEnd      = arc[s + 1];
			seg.m_Owned     = anchorObj;
			seg.m_Threat    = nearest;
			seg.m_fPressure = pressure;
			seg.m_vFacing   = ComputeFacing(seg.Center(), cPos);

			m_aFrontline.Insert(seg);
		}
	}

	protected vector ComputeFacing(vector segCenter, vector rearRef)
	{
		vector away = segCenter - rearRef;
		away        = Vector(away[0], 0.0, away[2]);

		if (away.LengthSq() < 1.0)
			return Vector(1.0, 0.0, 0.0);

		return away.Normalized();
	}

	protected float ComputeSegmentPressure(CMD_AICommanderObjectiveComponent threat, vector node)
	{
		if (!threat || !threat.GetOwner())
			return 0.0;

		int segFriendly;
		int segEnemies;
		threat.CountNearbyUnitsCached(threat.GetRadius(), m_sFactionKey, segFriendly, segEnemies);
		float enemies = segEnemies;
		float dist    = Math.Max(vector.Distance(node, threat.GetOwner().GetOrigin()), 1.0);

		return enemies * (1000.0 / dist);
	}

	protected void SortNodesByAngle(notnull array<ref DCO_FrontlineSegment> nodes, vector centroid)
	{
		for (int i = 1; i < nodes.Count(); i++)
		{
			ref DCO_FrontlineSegment key = nodes[i];
			float keyAngle = AngleAround(key.m_vStart, centroid);

			int j = i - 1;
			while (j >= 0 && AngleAround(nodes[j].m_vStart, centroid) > keyAngle)
			{
				nodes.Set(j + 1, nodes[j]);
				j = j - 1;
			}

			nodes.Set(j + 1, key);
		}
	}

	protected float AngleAround(vector p, vector centroid)
	{
		return Math.Atan2(p[2] - centroid[2], p[0] - centroid[0]);
	}

	protected bool TryGetFrontlinePosition(out vector frontlinePos)
	{
		if (m_aFrontline.IsEmpty())
			return false;

		DCO_FrontlineSegment best = null;
		float bestPressure = -1.0;

		foreach (DCO_FrontlineSegment seg : m_aFrontline)
		{
			if (seg.m_fPressure > bestPressure)
			{
				bestPressure = seg.m_fPressure;
				best         = seg;
			}
		}

		if (!best)
			return false;

		frontlinePos = best.Center();
		return true;
	}

	protected bool GetNearestFrontlinePoint(vector from, out vector pos, out vector facing)
	{
		if (m_aFrontline.IsEmpty())
			return false;

		float  bestSq = float.MAX;
		bool   found  = false;

		foreach (DCO_FrontlineSegment seg : m_aFrontline)
		{
			vector p    = seg.NearestPointTo(from);
			float  dSq  = vector.DistanceSq(from, p);

			if (dSq < bestSq)
			{
				bestSq = dSq;
				pos    = p;
				facing = seg.m_vFacing;
				found  = true;
			}
		}

		return found;
	}

	protected bool GetFrontlineSegmentForRecon(out vector pos)
	{
		if (m_aFrontline.IsEmpty())
			return false;

		DCO_FrontlineSegment best = null;
		float bestScore = -1.0;

		foreach (DCO_FrontlineSegment seg : m_aFrontline)
		{
			vector c = seg.Center();

			float score = seg.m_fPressure + 1.0;

			foreach (CMD_FrontlineReconTrack t : m_aFrontlineReconTracks)
			{
				if (!t || !t.m_Squad || !t.m_Squad.GetOwner())
					continue;

				if (vector.DistanceSq(t.m_Squad.GetOwner().GetOrigin(), c) < m_fFrontlineReconRadius * m_fFrontlineReconRadius)
					score = score * 0.25;
			}

			if (score > bestScore)
			{
				bestScore = score;
				best      = seg;
			}
		}

		if (!best)
			return false;

		pos = best.Center();
		return true;
	}

	protected void TrySendFrontlineRecon(float worldTime)
	{
		int activeRecon = 0;
		foreach (CMD_FrontlineReconTrack t : m_aFrontlineReconTracks)
		{
			if (t && t.m_Squad)
				activeRecon++;
		}
		if (activeRecon >= m_iMaxFrontlineRecon)
			return;

		vector frontlinePos;
		if (!GetFrontlineSegmentForRecon(frontlinePos))
			return;

		DCO_GroupUtilityComponent reconGrp = FindBestIdleGroupForTask(DCO_EGroupTask.RECON, frontlinePos);
		if (!reconGrp)
			return;

		if (reconGrp.IsPlayerGroup())
			return;

		if (!CanCommitGroup(reconGrp))
			return;

		reconGrp.CompleteAllWaypoints();
		reconGrp.SetTask(DCO_EGroupTask.RECON);
		GeneratePatrolRoute(reconGrp, frontlinePos, m_fFrontlineReconRadius, worldTime);

		CMD_FrontlineReconTrack track = new CMD_FrontlineReconTrack();
		track.m_Squad = reconGrp;
		track.m_fExpireTime = worldTime + m_fFrontlineReconDuration;
		m_aFrontlineReconTracks.Insert(track);

		if (m_bDebugMode)
			Print(string.Format("[%1] Frontline Recon: %2 -> scouting deket %3",
				m_sCommanderUID, reconGrp.GetOwner().GetName(), frontlinePos.ToString()));
	}

	protected void UpdateFrontlineReconTracks(float worldTime)
	{
		if (!Replication.IsServer())
			return;

		for (int i = m_aFrontlineReconTracks.Count() - 1; i >= 0; i--)
		{
			CMD_FrontlineReconTrack track = m_aFrontlineReconTracks[i];
			if (!track || !track.m_Squad)
			{
				m_aFrontlineReconTracks.Remove(i);
				continue;
			}

			if (worldTime >= track.m_fExpireTime)
			{
				if (track.m_Squad.GetTask() == DCO_EGroupTask.RECON)
				{
					track.m_Squad.CompleteAllWaypoints();
					track.m_Squad.SetTask(DCO_EGroupTask.NONE);
					if (m_bDebugMode)
						Print(string.Format("[%1] Frontline Recon selesai -- %2 dilepas balik ke RESERVE",
							m_sCommanderUID, track.m_Squad.GetOwner().GetName()));
				}
				m_aFrontlineReconTracks.Remove(i);
			}
		}
	}

	protected vector GetNearestSafePosition(vector fromPos)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();

		array<vector> candidatePositions = new array<vector>();
		if (mgr)
		{
			foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
			{
				if (!obj)
					continue;
				if (!obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
					continue;
				candidatePositions.Insert(obj.GetOwner().GetOrigin());
			}
		}
		if (candidatePositions.IsEmpty())
			candidatePositions.Insert(GetOwner().GetOrigin());

		array<vector> sorted = new array<vector>();
		array<float>  sortedDist = new array<float>();
		foreach (vector cand : candidatePositions)
		{
			float d = vector.DistanceSq(fromPos, cand);
			int insertAt = sorted.Count();
			for (int i = 0; i < sorted.Count(); i++)
			{
				if (d < sortedDist[i])
				{
					insertAt = i;
					break;
				}
			}
			sorted.InsertAt(cand, insertAt);
			sortedDist.InsertAt(d, insertAt);
		}

		int pickIndex = Math.Round((sorted.Count() - 1) * (1.0 - m_fResilience));
		pickIndex = Math.Clamp(pickIndex, 0, sorted.Count() - 1);

		return sorted[pickIndex];
	}

	[Attribute("100.0", UIWidgets.EditBox, "Jarak minimum (meter) buat nganggep 2 grup sendiri 'ngecluster'.", category: "Commander Setting")]
	protected float m_fMinGroupClusterDistance;

	protected bool IsPositionClusteredWithOwnedGroups(vector pos, DCO_GroupUtilityComponent excludeGroup = null)
	{
		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || grp == excludeGroup)
				continue;

			if (vector.Distance(pos, grp.GetOwner().GetOrigin()) < m_fMinGroupClusterDistance)
				return true;
		}
		return false;
	}

	protected float ComputeReserveFrontlineScore(vector cand, AICommander_ManagerComponent mgr)
	{
	    float nearestNonOwnedDistSq = float.MAX;

	    if (mgr)
	    {
	        foreach (CMD_AICommanderObjectiveComponent fobj : mgr.m_aObjective)
	        {
	            if (!fobj || fobj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
	                continue;

	            float d = vector.DistanceSq(cand, fobj.GetOwner().GetOrigin());
	            if (d < nearestNonOwnedDistSq)
	                nearestNonOwnedDistSq = d;
	        }
	    }

	    if (nearestNonOwnedDistSq == float.MAX)
	        return 0.5;

	    return 1.0 / (1.0 + Math.Sqrt(nearestNonOwnedDistSq) / 500.0);
	}

	protected void SendIdleGroupsToReserve()
	{
	    AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
	    if (!mgr)
	        return;

	    array<vector> capturedObjPositions = new array<vector>();
	    foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
	    {
	        if (!obj)
	            continue;

	        if (!obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
	            continue;

	        capturedObjPositions.Insert(obj.GetOwner().GetOrigin());
	    }

	    array<float> capturedFrontlineScore = new array<float>();
	    foreach (vector capPos : capturedObjPositions)
	        capturedFrontlineScore.Insert(ComputeReserveFrontlineScore(capPos, mgr));

	    float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;

	    m_iPatrolSmartRemaining = m_iPatrolSmartBudget;

	    foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
	    {
	        if (!grp)
	            continue;

	        if (grp.IsMoving())
	            continue;

	        if (grp.IsDedicatedTransport() || grp.IsPlayerGroup())
	            continue;

	        if (grp.IsMortar() || !grp.IsAvailableReserve() || !grp.CanPatrol())
	            continue;
	        if (grp.IsArmor() && !m_bVehiclePatrol)
	            continue;

			if (!grp.CanItHaveOrder())
				continue;

	        array<vector> candidatePositions = capturedObjPositions;
	        array<float>  candidateFrontline = capturedFrontlineScore;

	        if (capturedObjPositions.IsEmpty())
	        {
	            vector fallbackPos;
	            vector frontline, frontFacing;
	            if (GetNearestFrontlinePoint(grp.GetOwner().GetOrigin(), frontline, frontFacing))
	                fallbackPos = frontline;
	            else
	                fallbackPos = grp.GetOwner().GetOrigin();

	            candidatePositions = new array<vector>();
	            candidateFrontline = new array<float>();
	            candidatePositions.Insert(fallbackPos);
	            candidateFrontline.Insert(ComputeReserveFrontlineScore(fallbackPos, mgr));
	        }

	        vector nearestCandidate = candidatePositions[0];
	        float bestCombinedScore = -1.0;

	        int candCount = candidatePositions.Count();
	        for (int c = 0; c < candCount; c++)
	        {
	            vector cand          = candidatePositions[c];
	            float frontlineScore = candidateFrontline[c];

	            float practicalScore = 1.0 / (1.0 + vector.Distance(grp.GetOwner().GetOrigin(), cand) / 500.0);

	            float combined = (m_fAggression * frontlineScore) + ((1.0 - m_fAggression) * practicalScore);

	            if (combined > bestCombinedScore)
	            {
	                bestCombinedScore = combined;
	                nearestCandidate  = cand;
	            }
	        }
	        float nearestDistSq = vector.DistanceSq(grp.GetOwner().GetOrigin(), nearestCandidate);

	        if (nearestDistSq > (m_fMaxPatrolPullDistance * m_fMaxPatrolPullDistance))
	            nearestCandidate = grp.GetOwner().GetOrigin();

	        float spreadAngle = Math.RandomFloat(0.0, 360.0) * Math.DEG2RAD;
	        vector frontPt, frontPtFacing;
	        if (GetNearestFrontlinePoint(nearestCandidate, frontPt, frontPtFacing))
	            spreadAngle = ApproachAngle(frontPt, nearestCandidate) + Math.RandomFloat(-90.0, 90.0) * Math.DEG2RAD;
	        float spreadDist  = Math.RandomFloatInclusive(m_fBaseRadius * 2.0, m_fBaseRadius * 6.0);
	        vector patrolCenter = nearestCandidate + Vector(Math.Cos(spreadAngle) * spreadDist, 0.0, Math.Sin(spreadAngle) * spreadDist);
	        patrolCenter[1] = GetGame().GetWorld().GetSurfaceY(patrolCenter[0], patrolCenter[2]);

	        EWaterSurfaceType waterType = EWaterSurfaceType.WST_NONE;
	        float lakeArea = 0;
	        float waterY = SCR_WorldTools.GetWaterSurfaceY(null, patrolCenter, waterType, lakeArea);

	        if (patrolCenter[1] < waterY && waterType == EWaterSurfaceType.WST_OCEAN)
	        {
	            patrolCenter = nearestCandidate;
	            patrolCenter[1] = GetGame().GetWorld().GetSurfaceY(patrolCenter[0], patrolCenter[2]);
	        }

	        if (IsPositionClusteredWithOwnedGroups(patrolCenter, grp))
	        {
	            float retryAngle = Math.RandomFloat(0.0, 360.0) * Math.DEG2RAD;
	            float retryDist  = Math.RandomFloatInclusive(m_fBaseRadius * 2.0, m_fBaseRadius * 6.0);
	            vector retryCenter = nearestCandidate + Vector(Math.Cos(retryAngle) * retryDist, 0.0, Math.Sin(retryAngle) * retryDist);
	            retryCenter[1] = GetGame().GetWorld().GetSurfaceY(retryCenter[0], retryCenter[2]);

	            if (!IsPositionClusteredWithOwnedGroups(retryCenter, grp))
	                patrolCenter = retryCenter;
	        }

	        IEntity patrolVeh = null;
	        if (m_bVehiclePatrol)
	        {
	            IEntity veh = grp.GetGroupVehicle();
	            if (veh && vector.Distance(veh.GetOrigin(), grp.GetOwner().GetOrigin()) <= VEHICLE_PATROL_MAX_WALK && grp.CanAllFitIn(veh))
	                patrolVeh = veh;
	        }

	        GeneratePatrolRoute(grp, patrolCenter, m_fBaseRadius, worldTime, patrolVeh);

	        grp.SetTask(DCO_EGroupTask.PATROL);
	    }
	}

	protected bool IsGroupBusy(DCO_GroupUtilityComponent grp)
	{
		if (grp.IsInTransport() || grp.GetSupportPlayerGroup() >= 0 || grp.DCO_IsHeld())
			return true;

		DCO_EGroupTask t = grp.GetTask();
		if (grp.GetGroupObjective() && t != DCO_EGroupTask.NONE && t != DCO_EGroupTask.PATROL)
			return true;

		return IsFrontlineReconGroup(grp);
	}

	static bool TaskCountsSlot(DCO_EGroupTask task)
	{
		return task == DCO_EGroupTask.ATTACK || task == DCO_EGroupTask.FLANK;
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

	protected DCO_GroupUtilityComponent FindBestIdleGroupForTask(DCO_EGroupTask task, vector targetPos, bool canTakeDefend = false, CMD_AICommanderObjectiveComponent targetObj = null, int wantedCaps = -1, bool armor = false, int purpose = 0)
	{
		if (wantedCaps < 0)
			wantedCaps = DefaultCapabilitiesForTask(task);

		if (m_iPhaseBudget == 0)
			return null;

		array<DCO_EGroupTask> tiers;
		if (armor)
			tiers = {DCO_EGroupTask.NONE, DCO_EGroupTask.PATROL};
		else if (canTakeDefend)
			tiers = {task, DCO_EGroupTask.NONE, DCO_EGroupTask.PATROL, DCO_EGroupTask.RECON, DCO_EGroupTask.REINFORCE, DCO_EGroupTask.DEFEND};
		else
			tiers = {task, DCO_EGroupTask.NONE, DCO_EGroupTask.PATROL, DCO_EGroupTask.RECON, DCO_EGroupTask.REINFORCE};

		int tierCount = tiers.Count();

		array<DCO_GroupUtilityComponent> bestPerTier   = {};
		array<float>                     bestScorePerTier  = {};
		array<float>                     bestDistSqPerTier = {};
		for (int t = 0; t < tierCount; t++)
		{
			bestPerTier.Insert(null);
			bestScorePerTier.Insert(-1000.0);
			bestDistSqPerTier.Insert(-1.0);
		}

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp)
				continue;

			bool isReservePatrol = grp.GetTask() == DCO_EGroupTask.PATROL || (grp.IsArmor() && grp.GetTask() == DCO_EGroupTask.NONE);
			if (grp.IsMoving() && !isReservePatrol)
				continue;

			if (grp.IsDedicatedTransport() || grp.IsMortar())
				continue;

			if (armor != grp.IsArmor())
				continue;

			if (!grp.CanCommanderOverrideRole())
				continue;

			if (!grp.CanItHaveOrder())
				continue;

			if (grp.IsPlayerGroup())
				continue;

			if (IsGroupBusy(grp))
				continue;

			if ((purpose == 1 || task == DCO_EGroupTask.REINFORCE) && !grp.CanReinforce())
				continue;
			if (purpose == 2 && !grp.CanSupportPlayers())
				continue;

			if ((task == DCO_EGroupTask.ATTACK || task == DCO_EGroupTask.FLANK) && !IsMoraleFitForAttack(grp))
				continue;

			int tierIdx = tiers.Find(grp.GetTask());
			if (tierIdx < 0)
				continue;

			int unitCount = grp.GetUnitCount();
			float strengthPct = Math.Clamp(unitCount / 12.0 * 100.0, 0.0, 100.0);

			float score = 0.0;
			switch (task)
			{
				case DCO_EGroupTask.RECON:
					score = 100.0 - strengthPct;
					break;
				case DCO_EGroupTask.ATTACK:
					score = strengthPct;
					break;
				case DCO_EGroupTask.FLANK:
					score = 100.0 - Math.AbsFloat(strengthPct - 50.0);
					break;
				default:
					score = strengthPct;
					break;
			}

			float distSq = vector.DistanceSq(grp.GetOwner().GetOrigin(), targetPos);

			score = score - Math.Min(Math.Sqrt(distSq) / 25.0, 80.0);
			score = score + CountCapabilityMatches(grp, wantedCaps) * 30.0;

			bool better = false;

			if (score > bestScorePerTier[tierIdx])
				better = true;
			else if (score == bestScorePerTier[tierIdx]
				&& (bestDistSqPerTier[tierIdx] < 0.0 || distSq < bestDistSqPerTier[tierIdx]))
				better = true;

			if (better)
			{
				bestScorePerTier[tierIdx]  = score;
				bestDistSqPerTier[tierIdx] = distSq;
				bestPerTier[tierIdx]       = grp;
			}
		}

		for (int t = 0; t < tierCount; t++)
		{
			if (bestPerTier[t])
			{
				if (m_iPhaseBudget > 0)
					m_iPhaseBudget = m_iPhaseBudget - 1;

				return bestPerTier[t];
			}
		}

		float preemptTime = GetGame().GetWorld().GetWorldTime() / 1000.0;

		DCO_GroupUtilityComponent preempted = TryFindPreemptableGroup(task, targetPos, targetObj, preemptTime, armor);
		if (preempted)
		{
			if (m_iPhaseBudget > 0)
				m_iPhaseBudget = m_iPhaseBudget - 1;

			return preempted;
		}

		return null;
	}

	protected bool IsGroupGatheringForAssault(CMD_AICommanderObjectiveComponent curObj)
	{
		if (!curObj)
			return false;

		if (!m_mStagingStartTime.Contains(curObj))
			return false;

		bool released;
		if (m_mAssaultReleased.Find(curObj, released) && released)
			return false;

		return true;
	}

	protected void ReleasePreemptedGroup(DCO_GroupUtilityComponent grp, float worldTime)
	{
		if (!grp)
			return;

		CMD_AICommanderObjectiveComponent oldObj = grp.GetGroupObjective();
		if (oldObj)
		{
			if (TaskCountsSlot(grp.GetTask()) && oldObj.GetCurrentAssignedGroupCount(m_sFactionKey) > 0)
				oldObj.SetObjectiveGroup(m_sFactionKey, -1);

			if (oldObj.GetReconGroup(m_sFactionKey) == grp)
				oldObj.SetReconGroup(m_sFactionKey, null);

			if (oldObj.GetObjectiveState(m_sFactionKey) == CMD_EObjectiveState.ASSIGNED
				&& oldObj.GetCurrentAssignedGroupCount(m_sFactionKey) <= 0)
			{
				oldObj.SetObjectiveState(m_sFactionKey, CMD_EObjectiveState.PENDING);
			}
		}

		grp.CompleteAllWaypoints();
		grp.SetGroupObjective(null);
		grp.SetTask(DCO_EGroupTask.NONE);

		m_mLastPreemptTime.Set(grp, worldTime);
	}

	protected DCO_GroupUtilityComponent TryFindPreemptableGroup(DCO_EGroupTask task, vector targetPos, CMD_AICommanderObjectiveComponent targetObj, float worldTime, bool armor = false)
	{
		if (!m_bEnablePreemption)
			return null;

		if (!targetObj || !targetObj.GetOwner())
			return null;

		float newScore = targetObj.ComputePriorityScore(m_sFactionKey, worldTime, GetOwner().GetOrigin(), m_fCombatFocus, m_sCommanderUID);
		if (newScore <= 0.0)
			return null;

		DCO_GroupUtilityComponent best = null;
		float bestValue  = -1.0;
		float bestDistSq = -1.0;

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || !grp.GetOwner())
				continue;

			if (grp.IsPlayerGroup())
				continue;

			if (grp.IsDedicatedTransport())
				continue;

			if (!grp.CanCommanderOverrideRole())
				continue;

			if (!grp.CanItHaveOrder())
				continue;

			if (DCO_PreemptionUtility.IsHardProtected(grp))
				continue;

			if (armor != grp.IsArmor())
				continue;

			if (grp.IsInContact())
				continue;

			CMD_AICommanderObjectiveComponent curObj = grp.GetGroupObjective();

			if (curObj == targetObj)
				continue;

			if (IsGroupGatheringForAssault(curObj))
				continue;

			float lastPreempt;
			if (m_mLastPreemptTime.Find(grp, lastPreempt) && (worldTime - lastPreempt) < m_fPreemptionCooldown)
				continue;

			float curScore = 0.0;
			if (curObj && curObj.GetOwner())
				curScore = curObj.ComputePriorityScore(m_sFactionKey, worldTime, GetOwner().GetOrigin(), m_fCombatFocus, m_sCommanderUID);

			float taskValue = DCO_PreemptionUtility.ComputeTaskValue(curScore, grp);

			if (!DCO_PreemptionUtility.IsWorthPreempting(newScore, taskValue, m_fPreemptionMargin))
				continue;

			float distSq = vector.DistanceSq(grp.GetOwner().GetOrigin(), targetPos);

			bool better = false;

			if (!best)
				better = true;
			else if (taskValue < bestValue)
				better = true;
			else if (taskValue == bestValue && distSq < bestDistSq)
				better = true;

			if (better)
			{
				best       = grp;
				bestValue  = taskValue;
				bestDistSq = distSq;
			}
		}

		if (!best)
			return null;

		ReleasePreemptedGroup(best, worldTime);
		return best;
	}

	protected int CountDefendDemand()
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return 0;

		int demand = 0;

		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj || !obj.GetOwner())
				continue;

			if (!obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
				continue;

			if (obj.CheckIsItLost(m_sFactionKey))
				continue;

			float radius = obj.GetRadius();
			if (radius <= 0.0)
				continue;

			int cap = obj.GetDefendGroupCount();
			if (cap <= 0)
				cap = 1;

			if (!obj.HasSectorGrid(m_sFactionKey))
			{
				int estimate = DCO_SectorMath.ComputeSectorCount(radius, m_fArcPerSector, GetSectorPersonalityMod(), m_iMinSector, m_iMaxSector);
				if (estimate > cap)
					estimate = cap;
				demand = demand + estimate;
				continue;
			}

			int staffed = obj.GetStaffedSectorCount(m_sFactionKey);
			int missing = cap - staffed;
			if (missing > 0)
				demand = demand + missing;

			array<ref DCO_SectorGarrison> sectors = obj.GetSectorGarrison(m_sFactionKey);
			if (sectors)
			{
				foreach (DCO_SectorGarrison sec : sectors)
				{
					if (sec && sec.NeedsReplenish())
						demand = demand + 1;
				}
			}
		}

		return demand;
	}

	protected int CountIdleCommittableGroups()
	{
		int count = 0;

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || grp.IsPlayerGroup())
				continue;

			if (grp.IsDedicatedTransport() || !grp.CanCommanderOverrideRole() || !grp.CanItHaveOrder())
				continue;

			bool isReservePatrol = grp.GetTask() == DCO_EGroupTask.PATROL || (grp.IsArmor() && grp.GetTask() == DCO_EGroupTask.NONE);
			if (grp.IsMoving() && !isReservePatrol)
				continue;

			if (IsGroupBusy(grp))
				continue;

			count = count + 1;
		}

		return count;
	}

	protected void ThinkBalanced(AICommander_ManagerComponent mgr, float worldTime)
	{
		int defendDemand = CountDefendDemand();
		int idleAvail    = CountIdleCommittableGroups();

		int defendBudget = 0;
		if (defendDemand > 0 && idleAvail > 0)
		{
			float share = GetDefendShare();
			defendBudget = Math.Round(idleAvail * share);

			if (defendBudget > defendDemand)
				defendBudget = defendDemand;

			if (defendBudget < 1 && !m_bAllIn)
				defendBudget = 1;
		}

		m_iPhaseBudget = defendBudget;
		ThinkDefensive(worldTime);

		m_iPhaseBudget = -1;
		ThinkOffensive(mgr, worldTime);
	}

	protected void Think(float worldTime)
	{
		if (!Replication.IsServer())
			return;

		m_eCommanderState = CMD_ECommanderState.PLANNING;

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;

		ReclaimStaleAssignments(worldTime);

		m_iManpowerTotalCache = GetTotalManpower();

		if (m_bAllIn && m_bAllInRearDefenders && m_eCommanderMode != CMD_ECommanderMode.DEFENSIVE)
			ReleaseRearDefenders();

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp)
				continue;

			if (!grp.CanItHaveOrder())
				continue;

			grp.CheckOrderComplete(worldTime);
		}

		BuildFrontline();

		m_iPhaseBudget = -1;

		switch (m_eCommanderMode)
		{
			case CMD_ECommanderMode.OFFENSIVE:
			{
				ThinkOffensive(mgr, worldTime);
				break;
			}
			case CMD_ECommanderMode.DEFENSIVE:
			{
				ThinkDefensive(worldTime);
				break;
			}
			case CMD_ECommanderMode.BALANCED:
			{
				ThinkBalanced(mgr, worldTime);
				break;
			}
		}

		m_iPhaseBudget = -1;

		TrySendFrontlineRecon(worldTime);

		SendIdleGroupsToReserve();

		if (m_bPlayerTasking)
			m_PlayerTasking.Update(this);
		else
			m_PlayerTasking.Clear();

		m_eCommanderState = CMD_ECommanderState.COMMANDING;
	}

	protected void ThinkOffensive(AICommander_ManagerComponent mgr, float worldTime)
	{
		array<CMD_AICommanderObjectiveComponent> committed = {};
		foreach (CMD_AICommanderObjectiveComponent cand : mgr.m_aObjective)
		{
			if (cand && IsObjectiveCommitted(cand))
				committed.Insert(cand);
		}

		array<CMD_AICommanderObjectiveComponent> recapture = {};
		array<float> recaptureDist = {};
		vector basePos = GetOwner().GetOrigin();
		foreach (CMD_AICommanderObjectiveComponent own : mgr.m_aObjective)
		{
			if (!own)
				continue;

			if (own.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
			{
				if (m_eCommanderMode == CMD_ECommanderMode.OFFENSIVE && IsObjectiveContested(own))
					AssignDefendToObjective(own, worldTime);
				continue;
			}

			if (!own.CheckIsItLost(m_sFactionKey) || !HandleLostObjective(own) || committed.Contains(own))
				continue;

			float d = vector.DistanceSq(own.GetOwner().GetOrigin(), basePos);
			int at = 0;
			while (at < recaptureDist.Count() && recaptureDist[at] <= d)
				at++;
			recapture.InsertAt(own, at);
			recaptureDist.InsertAt(d, at);
		}

		array<CMD_AICommanderObjectiveComponent> ranked = {};
		mgr.GetTopObjectivesOffensive(this, m_fObjectiveAtTheSameTime + committed.Count() + 4, ranked);
		m_Ops.PlanAxis(this, ranked, worldTime);

		m_aObjective.Clear();
		m_aObjective.InsertAll(committed);
		foreach (CMD_AICommanderObjectiveComponent rc : recapture)
		{
			if (m_aObjective.Count() >= m_fObjectiveAtTheSameTime)
				break;
			m_aObjective.Insert(rc);
		}
		foreach (CMD_AICommanderObjectiveComponent r : ranked)
		{
			if (m_aObjective.Count() >= m_fObjectiveAtTheSameTime)
				break;
			if (!m_aObjective.Contains(r))
				m_aObjective.Insert(r);
		}

		if (m_aObjective.IsEmpty())
		{
			m_eCommanderState = CMD_ECommanderState.IDLE;
			return;
		}

		CMD_ObjectiveContextCache contextCache = mgr.BuildObjectiveContext(m_sFactionKey);

		for (int i = 0; i < m_aObjective.Count(); i++)
		{
			CMD_AICommanderObjectiveComponent obj = m_aObjective[i];
			if (!obj)
				continue;

			int friendlyNear;
			int enemyNear;
			obj.CountNearbyUnitsCached(obj.GetRadius(), m_sFactionKey, friendlyNear, enemyNear);

			if (friendlyNear > 0 && enemyNear >= friendlyNear * 3)
				continue;

			AssignRolesToObjective(obj, worldTime, contextCache);
		}

		if (m_bDebugMode)
			Print(string.Format("[%1] THINK OFFENSIVE", m_sCommanderUID));
	}

	bool IsObjectiveContested(CMD_AICommanderObjectiveComponent obj)
	{
		int friendlyNear;
		int enemyNear;
		obj.CountNearbyUnitsCached(obj.GetRadius(), m_sFactionKey, friendlyNear, enemyNear);
		return enemyNear > 0 && enemyNear >= friendlyNear;
	}

	protected bool HandleLostObjective(CMD_AICommanderObjectiveComponent obj)
	{
		if (obj.GetObjectiveState(m_sFactionKey) != CMD_EObjectiveState.PENDING && obj.CheckAndMarkIfLost(m_sFactionKey))
		{
			ReleaseGroupsFromObjective(obj);
			obj.ResetAssignedGroupCount(m_sFactionKey);
			obj.SetObjectiveState(m_sFactionKey, CMD_EObjectiveState.PENDING);
		}
		return obj.CheckIsItLost(m_sFactionKey);
	}

	protected void ThinkDefensive(float worldTime)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;

		array<CMD_AICommanderObjectiveComponent> allObjs = mgr.m_aObjective;

		bool hasAnyWork = false;

		foreach (CMD_AICommanderObjectiveComponent obj : allObjs)
		{
			if (!obj)
				continue;

			if (!obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
			{
				HandleLostObjective(obj);
				continue;
			}

			AssignDefendToObjective(obj, worldTime);
			hasAnyWork = true;
		}

		if (m_bDebugMode)
		{
			Print(hasAnyWork.ToString() + " < HAS DEFEND WORK FOR " + m_sCommanderUID + " " + m_sFactionKey);
			Print(string.Format("[%1] THINK DEFENSIVE", m_sCommanderUID));
		}
	}

	protected void TrySendToStaging(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
	    vector objPos  = obj.GetOwner().GetOrigin();
	    vector base    = GetOwner().GetOrigin();

	    vector stagingPos = GetOrCreateStagingPos(obj);

	    DCO_GroupUtilityComponent assaultGrp = FindBestIdleGroupForTask(DCO_EGroupTask.ATTACK, objPos);
	    if (assaultGrp)
	    {
			if (assaultGrp.IsPlayerGroup())
			{
				return;
			}
			if (!CanCommitGroup(assaultGrp))
			{
			}
			else
			{
				assaultGrp.CompleteAllWaypoints();
				bool viaTransport = TryAssignTransport(assaultGrp, stagingPos, worldTime, DCO_EGroupTask.ATTACK);
		        if (viaTransport || SpawnMoveRoute(assaultGrp, assaultGrp.GetOwner().GetOrigin(), stagingPos, worldTime))
		        {
		            if (!viaTransport)
		                assaultGrp.SetTask(DCO_EGroupTask.ATTACK);
		            if (assaultGrp.GetGroupObjective() != obj)
		            {
		                assaultGrp.SetGroupObjective(obj);
		                obj.SetObjectiveGroup(m_sFactionKey, 1);
		            }
		            TryAttachArmorToAssault(obj, stagingPos, worldTime);
		        }
			}
	    }

	    if (obj.GetRequiredGroupCount() >= 2 && !ObjectiveHasSuppressGroup(obj))
	    {
	        DCO_GroupUtilityComponent sbf = FindBestIdleGroupForTask(DCO_EGroupTask.SUPPORT_BY_FIRE, objPos, false, obj);
	        if (sbf && !sbf.IsPlayerGroup() && CanCommitGroup(sbf))
	        {
	            vector sbfPos = m_Ops.FindOverwatch(obj, sbf.GetOwner().GetOrigin());
	            if (sbfPos != vector.Zero)
	            {
	                sbf.CompleteAllWaypoints();
	                sbf.SetGroupObjective(obj);
	                if (!TryAssignTransport(sbf, sbfPos, worldTime, DCO_EGroupTask.SUPPORT_BY_FIRE))
	                {
	                    sbf.SetTask(DCO_EGroupTask.SUPPORT_BY_FIRE);
	                    SpawnMoveRoute(sbf, sbf.GetOwner().GetOrigin(), sbfPos, worldTime);
	                }
	                SCR_AIWaypoint sup = SpawnSuppressWP(objPos);
	                if (sup)
	                    sbf.MoveTo(sup, worldTime);
	            }
	        }
	    }

	    DCO_GroupUtilityComponent flankGrp = FindBestIdleGroupForTask(DCO_EGroupTask.FLANK, objPos);
	    if (flankGrp)
	    {
			if (flankGrp.IsPlayerGroup())
			{
				return;
			}
			if (!CanCommitGroup(flankGrp))
			{
				return;
			}
			flankGrp.CompleteAllWaypoints();

			float  stagingDist  = vector.Distance(stagingPos, objPos);
			bool   swing        = m_Tactics.AllowsFlankSwing(obj);
			int    pincerSide   = m_Tactics.NextFlankSide(obj);
	        vector flankStaging;
			if (!swing)
				flankStaging = FlankPoint(stagingPos, objPos, stagingDist, 25.0, 1);
			else if (pincerSide != 0)
				flankStaging = FlankPoint(stagingPos, objPos, stagingDist, 90.0, pincerSide);
			else
				flankStaging = ComputeFlankPosition(stagingPos, objPos, stagingDist);

			DCO_EGroupTask flankTask = DCO_EGroupTask.FLANK;
			if (!swing)
				flankTask = DCO_EGroupTask.ATTACK;

			bool flankViaTransport = TryAssignTransport(flankGrp, flankStaging, worldTime, flankTask);
			if (flankViaTransport || SpawnMoveRoute(flankGrp, flankGrp.GetOwner().GetOrigin(), flankStaging, worldTime))
	        {
	            if (!flankViaTransport)
	                flankGrp.SetTask(flankTask);
	            if (flankGrp.GetGroupObjective() != obj)
	            {
	                flankGrp.SetGroupObjective(obj);
	                obj.SetObjectiveGroup(m_sFactionKey, 1);
	            }
	        }
	    }
	}

	protected void GenerateSearchWaypoints(vector center, float radius, array<SCR_AIWaypoint> outWaypoints, float wpSpacing = 50.0, float angleOffset = 0.0, float angleSpan = 360.0)
	{
	    if (!outWaypoints)
	        return;

	    outWaypoints.Clear();

	    if (angleSpan <= 0.0)
	        angleSpan = 360.0;

	    int budget = Math.Max(2, m_iMaxSearchWaypoints);

	    int rings = Math.Max(1, (int)Math.Round(radius / wpSpacing));

	    int maxRings = Math.Max(1, budget / 3);
	    if (rings > maxRings)
	        rings = maxRings;

	    int baseSectorsPerRing = Math.Max(2, (int)Math.Round((radius * 2 * Math.PI) / (wpSpacing * 1.5)));

	    float ringStep  = radius / rings;
	    float spanRatio = angleSpan / 360.0;

	    array<int> sectorsPerRing = {};
	    int wanted = 0;

	    for (int ringCalc = rings; ringCalc >= 1; ringCalc--)
	    {
	        float ringRatioCalc = (float)ringCalc / rings;
	        int   cs = Math.Max(2, (int)Math.Round(baseSectorsPerRing * ringRatioCalc * spanRatio));

	        sectorsPerRing.Insert(cs);
	        wanted = wanted + cs;
	    }

	    if (wanted > budget)
	    {
	        float scale = budget / (float)wanted;

	        for (int si = 0; si < sectorsPerRing.Count(); si++)
	            sectorsPerRing.Set(si, Math.Max(1, (int)Math.Round(sectorsPerRing.Get(si) * scale)));
	    }

	    int ringIndex = 0;

	    for (int ring = rings; ring >= 1; ring--)
	    {
	        float radiusInner = ringStep * (ring - 1);
	        float radiusOuter = ringStep * ring;

	        int currentSectors = sectorsPerRing.Get(ringIndex);
	        ringIndex          = ringIndex + 1;

	        float sectorAngle = angleSpan / currentSectors;

	        for (int sector = 0; sector < currentSectors; sector++)
	        {
	            if (outWaypoints.Count() >= budget)
	                return;

	            float angleMin = angleOffset + sectorAngle * sector;
	            float angleMax = angleOffset + sectorAngle * (sector + 1);
	            float angleDeg = Math.RandomFloat(angleMin, angleMax);
	            float angleRad = angleDeg * Math.DEG2RAD;

	            float dist = Math.RandomFloat(radiusInner + 1.0, radiusOuter);

	            float px = center[0] + Math.Cos(angleRad) * dist;
	            float pz = center[2] + Math.Sin(angleRad) * dist;
	            float py = GetGame().GetWorld().GetSurfaceY(px, pz);

	            vector p = Vector(px, py, pz);

	            EWaterSurfaceType waterType = EWaterSurfaceType.WST_NONE;
	            float lakeArea = 0;
	            float waterY   = SCR_WorldTools.GetWaterSurfaceY(null, p, waterType, lakeArea);

	            if (py < waterY && waterType != EWaterSurfaceType.WST_NONE)
	                continue;

	            SCR_AIWaypoint wp = SpawnMoveWP(p);
	            if (wp)
	                outWaypoints.Insert(wp);
	        }
	    }
	}

	bool SpawnMoveRoute(DCO_GroupUtilityComponent grp, vector from, vector to, float worldTime)
	{
		if (!grp)
			return false;

		float total = vector.Distance(from, to);
		array<SCR_AIWaypoint> addedLegs = {};

		if (m_fWaypointLegDistance > 0.0 && total > m_fWaypointLegDistance)
		{
			int legs = Math.Floor(total / m_fWaypointLegDistance);

			for (int i = 1; i < legs; i++)
			{
				float t = i / (float)legs;

				vector leg = from + ((to - from) * t);

				vector safeLeg;
				if (!FindSafeRoutePoint(leg, to, safeLeg))
					continue;

				SCR_AIWaypoint legWp = SpawnMoveWP(safeLeg);
				if (legWp)
				{
					grp.MoveTo(legWp, worldTime);
					addedLegs.Insert(legWp);
				}
			}
		}

		SCR_AIWaypoint destWp = SpawnMoveWP(to);
		if (!destWp)
		{
			AIGroup aiGrp = AIGroup.Cast(grp.GetOwner());
			foreach (SCR_AIWaypoint leg : addedLegs)
			{
				if (!leg)
					continue;
				if (aiGrp)
					aiGrp.RemoveWaypoint(leg);
				SCR_EntityHelper.DeleteEntityAndChildren(leg);
			}
			if (!addedLegs.IsEmpty() && !grp.IsGroupHaveWaypoint())
				grp.SetPhase(DCO_ETaskPhase.HOLDING);
			return false;
		}

		grp.MoveTo(destWp, worldTime);
		return true;
	}

	protected bool FindSafeRoutePoint(vector candidate, vector heading, out vector result)
	{
		vector dir = heading - candidate;
		dir        = Vector(dir[0], 0.0, dir[2]);

		if (dir.LengthSq() < 1.0)
			dir = Vector(1.0, 0.0, 0.0);
		else
			dir = dir.Normalized();

		vector side = Vector(-dir[2], 0.0, dir[0]);

		const int STEPS = 4;

		for (int step = 0; step <= STEPS; step++)
		{
			float offset = (step / (float)STEPS) * m_fWaypointNudgeRadius;

			for (int s = 0; s < 2; s++)
			{
				float signedOffset = offset;
				if (s == 1)
					signedOffset = -offset;

				vector test = candidate + (side * signedOffset);
				test[1]     = GetGame().GetWorld().GetSurfaceY(test[0], test[2]);

				if (IsRoutePointUsable(test))
				{
					result = test;
					return true;
				}
			}

			if (offset <= 0.0)
				continue;
		}

		return false;
	}

	protected bool IsRoutePointUsable(vector pos)
	{
		EWaterSurfaceType waterType = EWaterSurfaceType.WST_NONE;
		float lakeArea = 0;
		float waterY   = SCR_WorldTools.GetWaterSurfaceY(null, pos, waterType, lakeArea);

		if (pos[1] < waterY && waterType != EWaterSurfaceType.WST_NONE)
			return false;

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return true;

		foreach (CMD_AICommanderObjectiveComponent other : mgr.m_aObjective)
		{
			if (!other || !other.GetOwner())
				continue;

			if (other.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
				continue;

			float keepOut = other.GetRadius();

			if (vector.DistanceSq(pos, other.GetOwner().GetOrigin()) < keepOut * keepOut)
				return false;
		}

		return true;
	}

	SCR_AIWaypoint SpawnMoveWP(vector pos, EMovementType moveType = EMovementType.RUN)
	{
	    AICommander_BaseComponentClass data = AICommander_BaseComponentClass.Cast(GetComponentData(GetOwner()));
	    if (!data)
	        return null;

	    Resource res = Resource.Load(data.GetDefaultMoveWaypointPrefab());
	    if (!res || !res.IsValid())
	        return null;

	    BaseWorld world = GetGame().GetWorld();
	    if (!world)
	        return null;

		EWaterSurfaceType waterType = EWaterSurfaceType.WST_NONE;

	    float surfaceY = world.GetSurfaceY(pos[0], pos[2]);
		float lakeArea = 0;

	    float waterY = SCR_WorldTools.GetWaterSurfaceY(null, pos, waterType, lakeArea);
	    if (surfaceY < waterY)
	    {
			if (waterType == EWaterSurfaceType.WST_OCEAN || waterType == EWaterSurfaceType.WST_RIVER || waterType == EWaterSurfaceType.WST_POND)
	        	return null;
	    }

	    pos[1] = surfaceY;
	    EntitySpawnParams params = EntitySpawnParams();
	    params.TransformMode = ETransformMode.WORLD;
	    Math3D.MatrixIdentity4(params.Transform);
	    params.Transform[3] = pos;

		SCR_AIWaypoint wp = SCR_AIWaypoint.Cast(GetGame().SpawnEntityPrefab(res, null, params));
		if (!wp)
			return null;

		if (SCR_WorldTools.IsObjectUnderwater(wp))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(wp);
			return null;
		}

		SCR_AIGroupCharactersMovementSpeedSetting mspeed = SCR_AIGroupCharactersMovementSpeedSetting.Create(SCR_EAISettingOrigin.BEHAVIOR, moveType);
		wp.AddSetting(mspeed);

	    return wp;
	}

	SCR_AIWaypoint SpawnArtilleryWP(vector pos)
	{
	    AICommander_BaseComponentClass data = AICommander_BaseComponentClass.Cast(GetComponentData(GetOwner()));
	    if (!data)
	        return null;

	    Resource res = Resource.Load(data.GetShootArtilleryWaypointPrefab());
	    if (!res || !res.IsValid())
	        return null;

	    BaseWorld world = GetGame().GetWorld();
	    if (!world)
	        return null;

		EWaterSurfaceType waterType = EWaterSurfaceType.WST_NONE;

	    float surfaceY = world.GetSurfaceY(pos[0], pos[2]);
		float lakeArea = 0;

	    float waterY = SCR_WorldTools.GetWaterSurfaceY(null, pos, waterType, lakeArea);
	    if (surfaceY < waterY)
	    {
			if (waterType == EWaterSurfaceType.WST_OCEAN)
	        	return null;
	    }

	    pos[1] = surfaceY;
	    EntitySpawnParams params = EntitySpawnParams();
	    params.TransformMode = ETransformMode.WORLD;
	    Math3D.MatrixIdentity4(params.Transform);
	    params.Transform[3] = pos;

	    return SCR_AIWaypoint.Cast(GetGame().SpawnEntityPrefab(res, null, params));
	}

	SCR_AIWaypoint SpawnSuppressWP(vector pos)
	{
	    AICommander_BaseComponentClass data = AICommander_BaseComponentClass.Cast(GetComponentData(GetOwner()));
	    if (!data)
	        return null;

	    Resource res = Resource.Load(data.GetDefaultSuppressPrefab());
	    if (!res || !res.IsValid())
	        return null;

	    EntitySpawnParams params = EntitySpawnParams();
	    params.TransformMode = ETransformMode.WORLD;
	    Math3D.MatrixIdentity4(params.Transform);
	    params.Transform[3] = pos;

	    return SCR_AIWaypoint.Cast(GetGame().SpawnEntityPrefab(res, null, params));
	}

	SCR_AIWaypoint SpawnDefendWP(vector pos)
	{
		AICommander_BaseComponentClass data = AICommander_BaseComponentClass.Cast(GetComponentData(GetOwner()));
		if (!data)
			return null;

		Resource res = Resource.Load(data.GetDefaultDefendWaypointPrefab());
		if (!res || !res.IsValid())
			return null;

		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = pos;

		return SCR_AIWaypoint.Cast(GetGame().SpawnEntityPrefab(res, null, params));
	}

	protected void HandleStalemateObjective(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
	    float lastResponse;
	    if (m_mStalemateResponseTime.Find(obj, lastResponse))
	    {
	        if ((worldTime - lastResponse) < m_fStalemateResponseCooldown)
	            return;
	    }

	    if (!m_mStalemateResponseTime.Contains(obj))
	        m_mStalemateResponseTime.Insert(obj, worldTime);
	    else
	        m_mStalemateResponseTime.Set(obj, worldTime);

	    obj.NotifyContested(worldTime);

	    bool slotFull = obj.IsGroupSlotFull(m_sFactionKey);

	    if (!slotFull)
	    {
	        TrySendAssaultWithSlots(obj, worldTime);
	        return;
	    }

	    ReallocateGroupFromStalemateObjective(obj, worldTime);
	}

	protected void ReallocateGroupFromStalemateObjective(CMD_AICommanderObjectiveComponent stalemateObj, float worldTime)
	{
	    AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
	    if (!mgr)
	        return;

	    array<CMD_AICommanderObjectiveComponent> candidates = {};
	    mgr.GetTopObjectivesOffensive(this, m_fObjectiveAtTheSameTime + 2, candidates);

	    CMD_AICommanderObjectiveComponent altObj = null;
	    foreach (CMD_AICommanderObjectiveComponent c : candidates)
	    {
	        if (!c || c == stalemateObj)
	            continue;
	        if (c.IsStalemate(m_sFactionKey, worldTime))
	            continue;
	        altObj = c;
	        break;
	    }

	    if (!altObj)
	    {
	        if (m_bDebugMode)
	            Print(string.Format("[%1] STALEMATE %2: gak ada objective alternatif -- tahan posisi",
	                m_sCommanderUID, stalemateObj.GetOwner().GetName()));
	        return;
	    }

	    foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
	    {
	        if (!grp)
	            continue;

	        if (grp.GetGroupObjective() != stalemateObj)
	            continue;

	        if (grp.IsMoving())
	            continue;

	        if (grp.IsInContact())
	            continue;

	        if (!TaskCountsSlot(grp.GetTask()) || grp.IsInTransport())
	            continue;

	        grp.SetGroupObjective(altObj);
	        grp.SetTask(DCO_EGroupTask.ATTACK);

	        RandomGenerator rand = new RandomGenerator();
	        vector altPos = rand.GenerateRandomPointInRadius(5, Math.Max(altObj.GetRadius(), 6), altObj.GetOwner().GetOrigin(), false);
	        altPos[1] = GetGame().GetWorld().GetSurfaceY(altPos[0], altPos[2]);

	        grp.CompleteAllWaypoints();
	        if (!TryAssignTransport(grp, GetEntryPoint(altObj, grp.GetOwner().GetOrigin()), worldTime, DCO_EGroupTask.ATTACK))
	            SpawnMoveRoute(grp, grp.GetOwner().GetOrigin(), altPos, worldTime);

	        stalemateObj.SetObjectiveGroup(m_sFactionKey, -1);
	        altObj.SetObjectiveGroup(m_sFactionKey, 1);

	        break;
	    }
	}

	protected void ReleaseGroupsFromObjective(CMD_AICommanderObjectiveComponent obj)
	{
		if (!obj)
			return;

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || grp.IsPlayerGroup())
				continue;

			if (grp.GetGroupObjective() != obj)
				continue;

			if (!grp.CanCommanderOverrideRole() || grp.IsDedicatedTransport())
				continue;

			grp.CompleteAllWaypoints();
			grp.SetGroupObjective(null);
			grp.SetTask(DCO_EGroupTask.NONE);
		}

		ResetSyncState(obj);
	}

	protected void ReclaimStaleAssignments(float worldTime)
	{
		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || grp.IsPlayerGroup())
				continue;

			if (!grp.CanCommanderOverrideRole() || grp.IsDedicatedTransport())
				continue;

			DCO_EGroupTask task = grp.GetTask();

			if (task == DCO_EGroupTask.NONE
			 || task == DCO_EGroupTask.PATROL
			 || grp.GetSupportPlayerGroup() >= 0
			 || grp.IsInTransport()
			 || grp.HasState(DCO_EGroupState.RETREATING))
				continue;

			if (task == DCO_EGroupTask.DEFEND && !grp.GetGroupObjective())
				continue;

			if (grp.IsMoving())
				continue;

			if (grp.IsGroupHaveWaypoint())
				continue;

			CMD_AICommanderObjectiveComponent obj = grp.GetGroupObjective();

			bool stale;
			if (task == DCO_EGroupTask.DEFEND)
				stale = !obj || !obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID);
			else
				stale = !obj
					|| obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID)
					|| obj.CheckIsItLost(m_sFactionKey);

			if (!stale)
				continue;

			if (obj && TaskCountsSlot(task) && obj.GetCurrentAssignedGroupCount(m_sFactionKey) > 0)
				obj.SetObjectiveGroup(m_sFactionKey, -1);

			grp.SetGroupObjective(null);
			grp.SetTask(DCO_EGroupTask.NONE);
		}
	}

	protected float GetSectorPersonalityMod()
	{
		if (!m_bScaleSectorByPersonality)
			return 1.0;

		float mod = Math.Lerp(m_fSectorPersonalityMax, m_fSectorPersonalityMin, m_fAggression);

		mod = mod * Math.Lerp(0.95, 1.10, m_fResilience);

		return mod;
	}

	protected float AngleDelta(float a, float b)
	{
		float twoPi = 2.0 * Math.PI;
		float d = a - b;

		while (d > Math.PI)
			d -= twoPi;

		while (d < -Math.PI)
			d += twoPi;

		return Math.AbsFloat(d);
	}

	protected float ComputeThreatAngle(CMD_AICommanderObjectiveComponent obj)
	{
		vector objPos = obj.GetOwner().GetOrigin();
		vector dir    = vector.Zero;

		if (threatComp)
		{
			array<ref CMD_ThreatEntry> threats = threatComp.GetThreats();
			if (threats)
			{
				foreach (CMD_ThreatEntry t : threats)
				{
					if (!t)
						continue;

					float dist = vector.Distance(t.m_vPosition, objPos);
					if (dist > m_fThreatDirectionRadius || dist < 1.0)
						continue;

					vector toThreat = t.m_vPosition - objPos;
					toThreat = Vector(toThreat[0], 0.0, toThreat[2]);
					toThreat = toThreat.Normalized();

					float weight = t.m_iEstimatedEnemyCount;
					if (weight < 1.0)
						weight = 1.0;

					dir = dir + toThreat * weight;
				}
			}
		}

		if (dir.LengthSq() < 0.001)
		{
			AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
			if (mgr)
			{
				float bestDistSq = float.MAX;
				foreach (CMD_AICommanderObjectiveComponent other : mgr.m_aObjective)
				{
					if (!other || other == obj)
						continue;

					if (other.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
						continue;

					vector otherPos = other.GetOwner().GetOrigin();
					float dsq = vector.DistanceSq(otherPos, objPos);
					if (dsq < bestDistSq)
					{
						bestDistSq = dsq;
						dir = otherPos - objPos;
					}
				}
			}
		}

		if (dir.LengthSq() < 0.001)
			dir = objPos - GetOwner().GetOrigin();

		dir = Vector(dir[0], 0.0, dir[2]);
		if (dir.LengthSq() < 0.001)
			return 0.0;

		return Math.Atan2(dir[2], dir[0]);
	}

	protected DCO_SectorGarrison PickNextSector(notnull array<ref DCO_SectorGarrison> sectors, int sectorCount, float sectorOffset, float threatAngle)
	{
		DCO_SectorGarrison best = null;
		float bestGap    = -1.0;
		float bestFacing = -2.0;

		foreach (DCO_SectorGarrison cand : sectors)
		{
			if (!cand || cand.IsStaffed())
				continue;

			float candAngle = DCO_SectorMath.GetSectorMidAngle(cand.m_iSectorIndex, sectorCount, sectorOffset);

			float gap = Math.PI;
			foreach (DCO_SectorGarrison other : sectors)
			{
				if (!other || !other.IsStaffed())
					continue;

				float otherAngle = DCO_SectorMath.GetSectorMidAngle(other.m_iSectorIndex, sectorCount, sectorOffset);
				float d = AngleDelta(candAngle, otherAngle);
				if (d < gap)
					gap = d;
			}

			float facing = Math.Cos(candAngle - threatAngle);

			bool better = false;
			if (gap > bestGap + 0.001)
				better = true;
			else if (Math.AbsFloat(gap - bestGap) <= 0.001 && facing > bestFacing)
				better = true;

			if (better)
			{
				best       = cand;
				bestGap    = gap;
				bestFacing = facing;
			}
		}

		return best;
	}

	protected bool IsFrontlineReconGroup(DCO_GroupUtilityComponent grp)
	{
		if (!grp)
			return false;

		foreach (CMD_FrontlineReconTrack t : m_aFrontlineReconTracks)
		{
			if (t && t.m_Squad == grp)
				return true;
		}

		return false;
	}

	protected void AssignGroupToSector(DCO_GroupUtilityComponent grp, CMD_AICommanderObjectiveComponent obj, DCO_SectorGarrison sec, float worldTime)
	{
		grp.CompleteAllWaypoints();
		grp.SetTask(DCO_EGroupTask.DEFEND);

		grp.SetGroupObjective(obj);

		grp.MoveTo(sec.m_Waypoint, worldTime);
		sec.m_Group = grp;
	}

	protected void UpdateObjectiveGarrison(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		array<DCO_GroupUtilityComponent> garrison;
		if (!m_mObjGarrison.Find(obj, garrison))
		{
			garrison = {};
			m_mObjGarrison.Insert(obj, garrison);
		}

		for (int i = garrison.Count() - 1; i >= 0; i--)
		{
			DCO_GroupUtilityComponent g = garrison[i];
			if (!g || g.GetGroupObjective() != obj || g.GetTask() != DCO_EGroupTask.DEFEND || !g.GetGarrisonActivity())
				garrison.Remove(i);
		}

		int friendlyNear, enemyNear;
		obj.CountNearbyUnitsCached(obj.GetRadius(), m_sFactionKey, friendlyNear, enemyNear);
		if (enemyNear > 0 && enemyNear >= friendlyNear)
		{
			foreach (DCO_GroupUtilityComponent rg : garrison)
			{
				DCO_AIGarrisonActivity act = rg.GetGarrisonActivity();
				if (act && act.IsActive())
					act.ForceRelease();
			}
			return;
		}

		if (garrison.Count() >= m_iGarrisonGroupsPerObjective)
			return;

		DCO_GarrisonRegistry registry = DCO_GarrisonRegistry.GetInstance();
		if (!registry)
			return;

		vector objPos = obj.GetOwner().GetOrigin();
		DCO_GroupUtilityComponent grp = FindBestIdleGroupForTask(DCO_EGroupTask.DEFEND, objPos);
		if (!grp || grp.IsPlayerGroup() || IsFrontlineReconGroup(grp))
			return;

		if (m_bGateDefendByManpower && !CanCommitGroup(grp))
			return;

		if (registry.CountFreeBuildings(objPos, obj.GetRadius(), AIGroup.Cast(grp.GetOwner())) <= 0)
			return;

		DCO_GarrisonWaypoint wp = SpawnGarrisonWP(objPos, ComputeThreatAngle(obj), obj.GetRadius());
		if (!wp)
			return;

		grp.CompleteAllWaypoints();
		grp.SetTask(DCO_EGroupTask.DEFEND);
		grp.SetGroupObjective(obj);
		grp.MoveTo(wp, worldTime);
		garrison.Insert(grp);
	}

	DCO_GarrisonWaypoint SpawnGarrisonWP(vector pos, float threatAngle, float radius, bool holdMode = false)
	{
		Resource res = Resource.Load(m_sGarrisonWaypointPrefab);
		if (!res || !res.IsValid())
			return null;

		vector dir = Vector(Math.Cos(threatAngle), 0, Math.Sin(threatAngle));
		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.AnglesToMatrix(Vector(dir.ToYaw(), 0, 0), params.Transform);
		pos[1] = GetGame().GetWorld().GetSurfaceY(pos[0], pos[2]);
		params.Transform[3] = pos;

		DCO_GarrisonWaypoint wp = DCO_GarrisonWaypoint.Cast(GetGame().SpawnEntityPrefab(res, null, params));
		if (!wp)
			return null;

		wp.SetCompletionRadius(Math.Max(radius, 30));
		if (holdMode)
			wp.SetMode(DCO_EGarrisonMode.HOLD);
		else
			wp.SetMode(m_eGarrisonMode);
		return wp;
	}

	protected void AssignDefendToObjective(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		if (!obj || !obj.GetOwner())
			return;

		float radius  = obj.GetRadius();
		if (radius <= 0.0)
			return;

		vector objPos = obj.GetOwner().GetOrigin();

		UpdateObjectiveGarrison(obj, worldTime);

		if (!obj.HasSectorGrid(m_sFactionKey))
		{
			float initialThreat = ComputeThreatAngle(obj);
			int   count  = DCO_SectorMath.ComputeSectorCount(radius, m_fArcPerSector, GetSectorPersonalityMod(), m_iMinSector, m_iMaxSector);
			float offset = DCO_SectorMath.ComputeSectorOffset(initialThreat, count);
			obj.InitSectorGrid(m_sFactionKey, count, offset);
		}

		array<ref DCO_SectorGarrison> sectors = obj.GetSectorGarrison(m_sFactionKey);
		if (!sectors || sectors.IsEmpty())
			return;

		int   sectorCount  = obj.GetSectorCount(m_sFactionKey);
		float sectorOffset = obj.GetSectorOffset(m_sFactionKey);
		float threatAngle  = ComputeThreatAngle(obj);

		float completionRadius = DCO_SectorMath.ComputeCompletionRadius(radius, m_fMaxCompletionRadius);
		float minSep           = completionRadius * 2.0;

		array<vector> taken = {};
		foreach (DCO_SectorGarrison s : sectors)
		{
			if (s && s.IsStaffed())
				taken.Insert(s.m_vPosition);
		}

		foreach (DCO_SectorGarrison sec : sectors)
		{
			if (!sec || !sec.NeedsReplenish())
				continue;

			DCO_GroupUtilityComponent grp = FindBestIdleGroupForTask(DCO_EGroupTask.DEFEND, sec.m_vPosition);

			if (!grp)
				break;

			if (grp.IsPlayerGroup() || IsFrontlineReconGroup(grp))
				continue;

			AssignGroupToSector(grp, obj, sec, worldTime);
		}

		int garrisonCap = obj.GetDefendGroupCount();
		if (garrisonCap <= 0)
			garrisonCap = 1;

		int staffedNow = obj.GetStaffedSectorCount(m_sFactionKey);

		while (staffedNow < garrisonCap)
		{
			DCO_SectorGarrison target = PickNextSector(sectors, sectorCount, sectorOffset, threatAngle);
			if (!target)
				break;

			DCO_GroupUtilityComponent grp = FindBestIdleGroupForTask(DCO_EGroupTask.DEFEND, objPos);
			if (!grp || grp.IsPlayerGroup() || IsFrontlineReconGroup(grp))
				break;

			if (m_bGateDefendByManpower && !CanCommitGroup(grp))
				break;

			vector pos;
			DCO_SectorMath.SamplePosition(objPos, radius, target.m_iSectorIndex, sectorCount, sectorOffset, minSep, taken, pos);

			SCR_AIWaypoint wp = SpawnDefendWP(pos);
			if (!wp)
				break;

			wp.SetCompletionRadius(completionRadius);

			target.m_vPosition = pos;
			target.m_Waypoint  = wp;
			taken.Insert(pos);

			AssignGroupToSector(grp, obj, target, worldTime);

			staffedNow = staffedNow + 1;
		}
	}

	protected void GeneratePatrolRoute(DCO_GroupUtilityComponent grp, vector center, float radius, float worldTime, IEntity mountedVehicle = null)
	{
		if (!grp)
			return;

		float patrolRadius = radius * 1.6 * PatrolRadiusMultiplier(grp);
		if (mountedVehicle)
			patrolRadius = patrolRadius * m_fVehiclePatrolRadiusMul;

		vector patrolCenter = ApplyFrontlineDrift(center);

		patrolCenter = AvoidRecentPatrolCenters(grp, patrolCenter, patrolRadius);
		RememberPatrolCenter(grp, patrolCenter);

		vector lookAt;
		bool   hasThreat = TryGetFrontlinePosition(lookAt);

		int pattern = PickPatrolPattern(grp, hasThreat);

		array<vector> points = {};

		switch (pattern)
		{
			case DCO_EPatrolPattern.LANE:    BuildLanePatrol(patrolCenter, patrolRadius, hasThreat, lookAt, points); break;
			case DCO_EPatrolPattern.ARC:     BuildArcPatrol(patrolCenter, patrolRadius, lookAt, points);             break;
			case DCO_EPatrolPattern.ADVANCE: BuildAdvancePatrol(patrolCenter, patrolRadius, lookAt, points);         break;
			default:                         BuildRingPatrol(patrolCenter, patrolRadius, points);                    break;
		}

		grp.CompleteAllWaypoints();

		EMovementType moveType = EMovementType.WALK;
		RoadNetworkManager roads;
		if (mountedVehicle)
		{
			moveType = EMovementType.RUN;
			if (!grp.IsMountedIn(mountedVehicle))
			{
				SCR_BoardingEntityWaypoint board = SCR_BoardingEntityWaypoint.Cast(SpawnGetInWP(mountedVehicle.GetOrigin()));
				if (board)
				{
					board.SetEntity(mountedVehicle);
					board.SetAllowance(true, true, true);
					board.SetCompletionType(EAIWaypointCompletionType.All);
					grp.MoveTo(board, worldTime);
				}
			}

			SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
			if (aiWorld)
				roads = aiWorld.GetRoadNetworkManager();
		}

		foreach (vector raw : points)
		{
			vector p = raw;
			p[1] = GetGame().GetWorld().GetSurfaceY(p[0], p[2]);

			if (hasThreat && !mountedVehicle)
				p = RefinePatrolPoint(p, lookAt);

			if (roads)
				p = SnapToRoad(roads, p);

			EWaterSurfaceType waterType = EWaterSurfaceType.WST_NONE;
			float lakeArea = 0;
			float waterY   = SCR_WorldTools.GetWaterSurfaceY(null, p, waterType, lakeArea);

			if (p[1] < waterY && waterType != EWaterSurfaceType.WST_NONE)
				continue;

			SCR_AIWaypoint wp = SpawnMoveWP(p, moveType);
			if (wp)
				grp.MoveTo(wp, worldTime);
		}

		if (mountedVehicle)
			grp.SetVehicleCruiseSpeed(VEHICLE_PATROL_SPEED_KMH);
	}

	protected const float VEHICLE_PATROL_SPEED_KMH = 25.0;
	protected const float VEHICLE_PATROL_MAX_WALK = 150.0;
	protected const float ROAD_SNAP_MAX = 120.0;

	protected vector SnapToRoad(RoadNetworkManager roads, vector p)
	{
		BaseRoad road;
		float dist;
		roads.GetClosestRoad(p, road, dist, true);
		if (!road || dist > ROAD_SNAP_MAX)
			return p;

		array<vector> roadPts = {};
		road.GetPoints(roadPts);
		vector best = p;
		float bestSq = float.MAX;
		foreach (vector rp : roadPts)
		{
			float dSq = vector.DistanceSqXZ(rp, p);
			if (dSq < bestSq)
			{
				bestSq = dSq;
				best = rp;
			}
		}
		best[1] = GetGame().GetWorld().GetSurfaceY(best[0], best[2]);
		return best;
	}

	protected float PatrolRadiusMultiplier(DCO_GroupUtilityComponent grp)
	{
		float units    = grp.GetUnitCount();
		float strength = Math.Clamp(units / 12.0, 0.0, 1.0);

		return Math.Lerp(m_fPatrolRadiusMulMin, m_fPatrolRadiusMulMax, strength);
	}

	protected vector ApplyFrontlineDrift(vector center)
	{
		if (m_fPatrolFrontlineDrift <= 0.0)
			return center;

		vector frontline, facing;
		if (!GetNearestFrontlinePoint(center, frontline, facing))
			return center;

		if (facing.LengthSq() < 0.001)
			return center;

		vector drifted = center + (facing * m_fPatrolFrontlineDrift);
		drifted[1]     = GetGame().GetWorld().GetSurfaceY(drifted[0], drifted[2]);

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return drifted;

		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj || !obj.GetOwner())
				continue;

			if (obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
				continue;

			float standoff = m_fPatrolFrontlineStandoff + obj.GetRadius();

			if (vector.DistanceSq(drifted, obj.GetOwner().GetOrigin()) < standoff * standoff)
				return center;
		}

		return drifted;
	}

	protected vector AvoidRecentPatrolCenters(DCO_GroupUtilityComponent grp, vector candidate, float patrolRadius)
	{
		array<vector> memory;
		if (!m_mPatrolMemory.Find(grp, memory) || memory.IsEmpty())
			return candidate;

		const int MAX_TRIES = 5;

		for (int attempt = 0; attempt < MAX_TRIES; attempt++)
		{
			bool tooClose = false;

			foreach (vector old : memory)
			{
				if (vector.DistanceSq(candidate, old) < m_fPatrolMemoryRadius * m_fPatrolMemoryRadius)
				{
					tooClose = true;
					break;
				}
			}

			if (!tooClose)
				return candidate;

			float angle = Math.RandomFloat(0.0, 360.0) * Math.DEG2RAD;
			float dist  = m_fPatrolMemoryRadius * Math.RandomFloatInclusive(1.1, 1.8);

			candidate = candidate + Vector(Math.Cos(angle) * dist, 0.0, Math.Sin(angle) * dist);
			candidate[1] = GetGame().GetWorld().GetSurfaceY(candidate[0], candidate[2]);
		}

		return candidate;
	}

	protected void RememberPatrolCenter(DCO_GroupUtilityComponent grp, vector center)
	{
		array<vector> memory;
		if (!m_mPatrolMemory.Find(grp, memory))
		{
			memory = new array<vector>();
			m_mPatrolMemory.Insert(grp, memory);
		}

		memory.Insert(center);

		while (memory.Count() > m_iPatrolMemorySize)
			memory.Remove(0);
	}

	protected int PickPatrolPattern(DCO_GroupUtilityComponent grp, bool hasThreat)
	{
		if (!hasThreat)
		{
			if (Math.RandomFloat01() < 0.5)
				return DCO_EPatrolPattern.RING;

			return DCO_EPatrolPattern.LANE;
		}

		float roll = Math.RandomFloat01();

		if (roll < m_fAggression * 0.5)
			return DCO_EPatrolPattern.ADVANCE;

		if (roll < 0.5 + (m_fAggression * 0.2))
			return DCO_EPatrolPattern.ARC;

		if (roll < 0.8)
			return DCO_EPatrolPattern.LANE;

		return DCO_EPatrolPattern.RING;
	}

	protected void BuildRingPatrol(vector center, float radius, out array<vector> outPoints)
	{
		int   count      = Math.RandomInt(4, 7);
		float startAngle = Math.RandomFloat(0.0, 360.0);

		for (int i = 0; i < count; i++)
		{
			float angleRad = (startAngle + (360.0 / count) * i) * Math.DEG2RAD;
			float dist     = radius * Math.RandomFloatInclusive(0.85, 1.15);

			outPoints.Insert(center + Vector(Math.Cos(angleRad) * dist, 0.0, Math.Sin(angleRad) * dist));
		}
	}

	protected void BuildLanePatrol(vector center, float radius, bool hasThreat, vector lookAt, out array<vector> outPoints)
	{
		vector axis;

		if (hasThreat)
		{
			vector toThreat = lookAt - center;
			toThreat        = Vector(toThreat[0], 0.0, toThreat[2]);

			if (toThreat.LengthSq() < 1.0)
				toThreat = Vector(1.0, 0.0, 0.0);
			else
				toThreat = toThreat.Normalized();

			axis = Vector(-toThreat[2], 0.0, toThreat[0]);
		}
		else
		{
			float a = Math.RandomFloat(0.0, 360.0) * Math.DEG2RAD;
			axis    = Vector(Math.Cos(a), 0.0, Math.Sin(a));
		}

		vector side = Vector(-axis[2], 0.0, axis[0]);

		int   legs   = Math.RandomInt(4, 7);
		float length = radius * 1.8;

		for (int i = 0; i < legs; i++)
		{
			float t = (i / (float)Math.Max(legs - 1, 1)) * 2.0 - 1.0;

			float lateral = radius * 0.35;
			if (i % 2 == 1)
				lateral = -lateral;

			outPoints.Insert(center + (axis * (t * length)) + (side * lateral));
		}
	}

	protected void BuildArcPatrol(vector center, float radius, vector lookAt, out array<vector> outPoints)
	{
		vector toThreat = lookAt - center;
		toThreat        = Vector(toThreat[0], 0.0, toThreat[2]);

		float baseAngle;
		if (toThreat.LengthSq() < 1.0)
			baseAngle = Math.RandomFloat(0.0, 360.0);
		else
			baseAngle = Math.Atan2(toThreat[2], toThreat[0]) * Math.RAD2DEG;

		float span  = Math.RandomFloatInclusive(120.0, 160.0);
		int   count = Math.RandomInt(4, 7);

		for (int i = 0; i < count; i++)
		{
			float t        = i / (float)Math.Max(count - 1, 1);
			float angleDeg = baseAngle - (span * 0.5) + (span * t);
			float angleRad = angleDeg * Math.DEG2RAD;

			float dist = radius * Math.RandomFloatInclusive(0.8, 1.2);

			outPoints.Insert(center + Vector(Math.Cos(angleRad) * dist, 0.0, Math.Sin(angleRad) * dist));
		}
	}

	protected void BuildAdvancePatrol(vector center, float radius, vector lookAt, out array<vector> outPoints)
	{
		vector dir = lookAt - center;
		dir        = Vector(dir[0], 0.0, dir[2]);

		if (dir.LengthSq() < 1.0)
			dir = Vector(1.0, 0.0, 0.0);
		else
			dir = dir.Normalized();

		vector side = Vector(-dir[2], 0.0, dir[0]);

		int   count  = Math.RandomInt(4, 7);
		float length = radius * 2.2;

		for (int i = 0; i < count; i++)
		{
			float t = (i + 1) / (float)count;

			float lateral = radius * Math.RandomFloatInclusive(0.25, 0.6);
			if (i % 2 == 1)
				lateral = -lateral;

			outPoints.Insert(center + (dir * (t * length)) + (side * lateral));
		}
	}

	protected vector RefinePatrolPoint(vector candidate, vector lookAt)
	{
		if (!m_bPatrolTerrainScoring || m_iPatrolSmartRemaining <= 0)
			return candidate;

		m_iPatrolSmartRemaining = m_iPatrolSmartRemaining - 1;

		const int   SAMPLES = 5;
		const float SPREAD  = 35.0;

		vector best      = candidate;
		float  bestScore = ScorePatrolPoint(candidate, lookAt);

		for (int i = 0; i < SAMPLES; i++)
		{
			float angle = Math.RandomFloat(0.0, 360.0) * Math.DEG2RAD;
			float dist  = Math.RandomFloatInclusive(SPREAD * 0.3, SPREAD);

			vector test = candidate + Vector(Math.Cos(angle) * dist, 0.0, Math.Sin(angle) * dist);
			test[1]     = GetGame().GetWorld().GetSurfaceY(test[0], test[2]);

			float score = ScorePatrolPoint(test, lookAt);

			if (score > bestScore)
			{
				bestScore = score;
				best      = test;
			}
		}

		return best;
	}

	protected float ScorePatrolPoint(vector pos, vector lookAt)
	{
		float heightDiff = pos[1] - lookAt[1];
		float elevScore  = Math.Clamp(heightDiff / 15.0, 0.0, 1.0);

		vector eye     = Vector(pos[0], pos[1] + 1.2, pos[2]);
		vector target  = Vector(lookAt[0], lookAt[1] + 1.0, lookAt[2]);

		TraceParam trace = new TraceParam();
		trace.Start = eye;
		trace.End   = target;
		trace.Flags = TraceFlags.ANY_CONTACT;

		DCO_Perf.Count("t:AICommanderBase");
		float hitFraction = GetGame().GetWorld().TraceMove(trace, null);

		float losScore;
		if (hitFraction >= 1.0)
			losScore = 1.0;
		else if (hitFraction >= 0.85)
			losScore = 0.5;
		else
			losScore = 0.0;

		return (losScore * 0.6) + (elevScore * 0.4);
	}

	protected void AssignPatrolAroundObjective(DCO_GroupUtilityComponent grp, vector center, float radius, float worldTime)
	{
		GeneratePatrolRoute(grp, center, radius, worldTime);
		grp.SetTask(DCO_EGroupTask.DEFEND);
	}

	protected void AssignObjectiveLinkPatrol(DCO_GroupUtilityComponent grp, CMD_AICommanderObjectiveComponent homeObj, float worldTime)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
		{
			AssignPatrolAroundObjective(grp, homeObj.GetOwner().GetOrigin(), homeObj.GetRadius(), worldTime);
			return;
		}

		vector homePos = homeObj.GetOwner().GetOrigin();
		array<vector> linkPoints = new array<vector>();
		linkPoints.Insert(homePos);

		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj || obj == homeObj)
				continue;

			if (!obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
				continue;

			float dist = vector.Distance(obj.GetOwner().GetOrigin(), homePos);
			if (dist <= m_fPatrolLinkRadius)
				linkPoints.Insert(obj.GetOwner().GetOrigin());
		}

		if (linkPoints.Count() < 2)
		{
			AssignPatrolAroundObjective(grp, homePos, homeObj.GetRadius(), worldTime);
			return;
		}

		for (int i = linkPoints.Count() - 1; i > 0; i--)
		{
			int j = Math.RandomInt(0, i + 1);
			vector tmp   = linkPoints[i];
			linkPoints[i] = linkPoints[j];
			linkPoints[j] = tmp;
		}

		grp.CompleteAllWaypoints();
		RandomGenerator rand = new RandomGenerator();

		foreach (vector p : linkPoints)
		{
			vector patrolPos = rand.GenerateRandomPointInRadius(0, Math.Max(homeObj.GetRadius() * 0.8, 1), p, false);
			patrolPos[1] = GetGame().GetWorld().GetSurfaceY(patrolPos[0], patrolPos[2]);

			SCR_AIWaypoint wp = SpawnMoveWP(patrolPos, EMovementType.WALK);
			if (wp)
				grp.MoveTo(wp, worldTime);
		}

		grp.SetTask(DCO_EGroupTask.DEFEND);
	}

	protected void AssignDefensivePatrol(DCO_GroupUtilityComponent grp, CMD_AICommanderObjectiveComponent homeObj, float worldTime)
	{
		if (Math.RandomFloat01() < m_fLinkPatrolChance)
			AssignObjectiveLinkPatrol(grp, homeObj, worldTime);
		else
			AssignPatrolAroundObjective(grp, homeObj.GetOwner().GetOrigin(), homeObj.GetRadius(), worldTime);
	}

	protected bool ObjectiveHasSuppressGroup(CMD_AICommanderObjectiveComponent obj)
	{
		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp)
				continue;

			if (grp.GetTask() == DCO_EGroupTask.SUPPORT_BY_FIRE && grp.GetGroupObjective() == obj)
				return true;
		}
		return false;
	}

	protected void TryGatherForSynchronizedAssault(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		vector objPos = obj.GetOwner().GetOrigin();

		vector stagingPos = GetOrCreateStagingPos(obj);

		foreach (DCO_GroupUtilityComponent staged : m_aOwnedGroup)
		{
			if (!staged || staged.GetGroupObjective() != obj || staged.GetTask() != DCO_EGroupTask.ATTACK)
				continue;

			if (staged.IsMoving() || staged.IsGroupHaveWaypoint())
				continue;

			if (vector.Distance(staged.GetOwner().GetOrigin(), stagingPos) <= STAGING_ARRIVE_RADIUS)
				continue;

			if (!TryAssignTransport(staged, stagingPos, worldTime, DCO_EGroupTask.ATTACK))
				SpawnMoveRoute(staged, staged.GetOwner().GetOrigin(), stagingPos, worldTime);
		}

		int required = GetRequiredAttackGroups(obj);

		while (obj.GetCurrentAssignedGroupCount(m_sFactionKey) < required)
		{
			DCO_GroupUtilityComponent assaultGrp = FindBestIdleGroupForTask(DCO_EGroupTask.ATTACK, objPos);
			if (!assaultGrp)
				break;

			if (assaultGrp.IsPlayerGroup())
				break;

			if (!CanCommitGroup(assaultGrp))
				break;

			assaultGrp.CompleteAllWaypoints();

			if (TryAssignTransport(assaultGrp, stagingPos, worldTime, DCO_EGroupTask.ATTACK))
			{
				if (assaultGrp.GetGroupObjective() != obj)
				{
					assaultGrp.SetGroupObjective(obj);
					obj.SetObjectiveGroup(m_sFactionKey, 1);
				}
				continue;
			}

			if (!SpawnMoveRoute(assaultGrp, assaultGrp.GetOwner().GetOrigin(), stagingPos, worldTime))
				break;

			assaultGrp.SetTask(DCO_EGroupTask.ATTACK);
			if (assaultGrp.GetGroupObjective() != obj)
			{
				assaultGrp.SetGroupObjective(obj);
				obj.SetObjectiveGroup(m_sFactionKey, 1);
			}
		}

		TryAttachArmorToAssault(obj, stagingPos, worldTime);
	}

	protected const int MAX_ARMOR_PER_OBJECTIVE = 2;

	protected void TryAttachArmorToAssault(CMD_AICommanderObjectiveComponent obj, vector stagingPos, float worldTime)
	{
		int attached = 0;
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (g && g.IsArmor() && g.GetGroupObjective() == obj)
				attached++;
		}

		if (attached >= MAX_ARMOR_PER_OBJECTIVE)
			return;

		DCO_GroupUtilityComponent armor = FindBestIdleGroupForTask(DCO_EGroupTask.ATTACK, obj.GetOwner().GetOrigin(), false, null, -1, true);
		if (!armor || armor.IsPlayerGroup())
			return;

		vector overwatch = m_Ops.FindOverwatch(obj, armor.GetOwner().GetOrigin());
		if (overwatch == vector.Zero)
			overwatch = stagingPos;

		armor.CompleteAllWaypoints();
		if (!SpawnMoveRoute(armor, armor.GetOwner().GetOrigin(), overwatch, worldTime))
			return;

		armor.SetTask(DCO_EGroupTask.SUPPORT_BY_FIRE);
		armor.SetGroupObjective(obj);
		obj.SetObjectiveGroup(m_sFactionKey, 1);
		m_Ops.RegisterArmor(this, armor, obj, overwatch);
	}

	protected const float STAGING_ARRIVE_RADIUS = 80.0;

	protected void CountStagingGroups(CMD_AICommanderObjectiveComponent obj, out int here, out int total, out float maxDist)
	{
		here = 0;
		total = 0;
		maxDist = 0;
		vector stagingPos = GetOrCreateStagingPos(obj);

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || grp.GetGroupObjective() != obj)
				continue;

			if (grp.GetTask() != DCO_EGroupTask.ATTACK)
				continue;

			total++;
			float d = vector.Distance(grp.GetOwner().GetOrigin(), stagingPos);
			maxDist = Math.Max(maxDist, d);
			if (!grp.IsInTransport() && d <= STAGING_ARRIVE_RADIUS)
				here++;
		}
	}

	protected void ResetSyncState(CMD_AICommanderObjectiveComponent obj)
	{
		m_mAssaultReleased.Remove(obj);
		m_mStagingStartTime.Remove(obj);
		m_mFirstArrivalTime.Remove(obj);
		m_mSyncDeadline.Remove(obj);
		m_mAssignedTime.Remove(obj);
		ClearStagingPos(obj);
	}

	protected bool IsObjectiveCommitted(CMD_AICommanderObjectiveComponent obj)
	{
		if (obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID) || obj.IsCommanderBlackListed(m_sCommanderUID))
			return false;

		if (obj.GetObjectiveState(m_sFactionKey) != CMD_EObjectiveState.ASSIGNED)
			return false;

		return m_mStagingStartTime.Contains(obj) || m_mAssaultReleased.Contains(obj) || HasCommittedGroups(obj);
	}

	protected bool HasCommittedGroups(CMD_AICommanderObjectiveComponent obj)
	{
		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (grp && grp.GetGroupObjective() == obj && TaskCountsSlot(grp.GetTask()))
				return true;
		}
		return false;
	}

	protected bool AreStagedGroupsArrived(CMD_AICommanderObjectiveComponent obj)
	{
		int found = 0;
		vector stagingPos = GetOrCreateStagingPos(obj);

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || grp.GetGroupObjective() != obj)
				continue;

			if (grp.GetTask() != DCO_EGroupTask.ATTACK)
				continue;

			found++;

			if (grp.IsMoving())
				return false;

			if (vector.Distance(grp.GetOwner().GetOrigin(), stagingPos) > STAGING_ARRIVE_RADIUS)
				return false;
		}

		foreach (DCO_GroupUtilityComponent riding : m_aOwnedGroup)
		{
			if (riding && riding.GetGroupObjective() == obj && riding.IsInTransport())
				return false;
		}

		if (m_bPlayerTasking && m_PlayerTasking.IsBlockingSync(this, obj))
			return false;

		if (threatComp && threatComp.IsHoldingRelease(obj, GetGame().GetWorld().GetWorldTime() / 1000.0))
			return false;

		return found > 0;
	}

	protected void RequestAssaultIllumination(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		if (!DCO_Night.IsActive() || !artySupport || !artySupport.HasRegisteredUnits())
			return;

		CMD_FireMissionRequest req = new CMD_FireMissionRequest(obj.GetOwner().GetOrigin(), SCR_EAIArtilleryAmmoType.ILLUMINATION, worldTime, 1);
		req.m_sSource = "assault_illum";
		req.m_sTier = "illum";
		artySupport.RequestShellImpact(req, worldTime, 1);
	}

	protected void ReleaseSynchronizedAssault(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		if (m_bPlayerTasking)
			GetGame().GetCallqueue().CallLater(NotifyPlayersReleased, 100, false, obj);

		vector stagingPos = GetOrCreateStagingPos(obj);
		m_Ops.OnAssaultReleased(this, obj, stagingPos, worldTime);
		ClearStagingPos(obj);
		RequestAssaultIllumination(obj, worldTime);

		int releasedCount = 0;

		array<DCO_GroupUtilityComponent> toRelease = {};
		array<DCO_GroupUtilityComponent> toFlank   = {};

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || grp.GetGroupObjective() != obj || grp.IsInTransport())
				continue;

			if (grp.GetTask() == DCO_EGroupTask.ATTACK && vector.Distance(grp.GetOwner().GetOrigin(), stagingPos) <= STAGING_ARRIVE_RADIUS)
				toRelease.Insert(grp);
			else if (grp.GetTask() == DCO_EGroupTask.FLANK)
				toFlank.Insert(grp);
		}

		int totalEntering = toRelease.Count() + toFlank.Count();
		if (totalEntering <= 0)
			return;

		vector objPos    = obj.GetOwner().GetOrigin();

		vector approachFrom = vector.Zero;
		foreach (DCO_GroupUtilityComponent ag : toRelease)
			approachFrom = approachFrom + ag.GetOwner().GetOrigin();
		if (toRelease.IsEmpty())
			approachFrom = GetOwner().GetOrigin();
		else
			approachFrom = approachFrom * (1.0 / toRelease.Count());

		float approachDeg = ApproachAngle(approachFrom, objPos) * Math.RAD2DEG;
		float sectorSize  = 180.0 / Math.Max(1, toRelease.Count());

		for (int gi = 0; gi < toRelease.Count(); gi++)
		{
			DCO_GroupUtilityComponent grp = toRelease[gi];
			grp.CompleteAllWaypoints();

			array<SCR_AIWaypoint> searchWPs = {};
			GenerateSearchWaypoints(objPos, obj.GetRadius(), searchWPs, 50.0, approachDeg - 90.0 + sectorSize * gi, sectorSize);

			if (searchWPs.IsEmpty())
				continue;

			grp.MoveToRoute(searchWPs, worldTime);

			releasedCount++;
		}

		for (int fi = 0; fi < toFlank.Count(); fi++)
		{
			DCO_GroupUtilityComponent fgrp = toFlank[fi];
			fgrp.CompleteAllWaypoints();

			float flankOwnDeg = ApproachAngle(fgrp.GetOwner().GetOrigin(), objPos) * Math.RAD2DEG;
			float sideSign    = 1.0;
			float rel = flankOwnDeg - approachDeg;
			while (rel > 180.0)
				rel -= 360.0;
			while (rel < -180.0)
				rel += 360.0;
			if (rel < 0.0)
				sideSign = -1.0;

			float flankSpan = 70.0;
			float arcCenter = approachDeg + sideSign * 125.0;
			float arcStart  = arcCenter - flankSpan * 0.5;

			float approachAngle = arcCenter * Math.DEG2RAD;
			float approachDist  = obj.GetRadius() * 1.4;

			vector approach = objPos + Vector(
				Math.Cos(approachAngle) * approachDist,
				0.0,
				Math.Sin(approachAngle) * approachDist);

			approach[1] = GetGame().GetWorld().GetSurfaceY(approach[0], approach[2]);

			SCR_AIWaypoint approachWp = SpawnMoveWP(approach);
			if (approachWp)
				fgrp.MoveTo(approachWp, worldTime);

			array<SCR_AIWaypoint> flankWPs = {};
			GenerateSearchWaypoints(objPos, obj.GetRadius(), flankWPs, 50.0, arcStart, flankSpan);

			if (flankWPs.IsEmpty())
				continue;

			fgrp.MoveToRoute(flankWPs, worldTime);

			releasedCount++;
		}
	}

	protected void PushStalledAssaultGroups(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		if (IsGroupGatheringForAssault(obj))
			return;

		vector objPos = obj.GetOwner().GetOrigin();
		float  radius = obj.GetRadius();

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || grp.IsPlayerGroup() || grp.GetGroupObjective() != obj)
				continue;

			if (!TaskCountsSlot(grp.GetTask()))
				continue;

			if (grp.IsMoving() || grp.IsGroupHaveWaypoint())
				continue;

			vector pos = grp.GetOwner().GetOrigin();
			if (vector.Distance(Vector(pos[0], 0, pos[2]), Vector(objPos[0], 0, objPos[2])) <= radius)
				continue;

			float ownDeg = ApproachAngle(pos, objPos) * Math.RAD2DEG;

			array<SCR_AIWaypoint> wps = {};
			GenerateSearchWaypoints(objPos, radius, wps, 50.0, ownDeg - 45.0, 90.0);
			if (!wps.IsEmpty())
				grp.MoveToRoute(wps, worldTime);
		}
	}

	protected void TrySendAssaultWithSlots(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		PushStalledAssaultGroups(obj, worldTime);

		if (obj.IsGroupSlotFull(m_sFactionKey))
			return;

		vector objCenter = obj.GetOwner().GetOrigin();
		float  objRad    = obj.GetRadius();
		int    required  = obj.GetRequiredGroupCount();
		int    slotsLeft = required - obj.GetCurrentAssignedGroupCount(m_sFactionKey);

		if (required >= 2 && !ObjectiveHasSuppressGroup(obj))
		{
			DCO_GroupUtilityComponent suppressGrp = FindBestIdleGroupForTask(DCO_EGroupTask.SUPPORT_BY_FIRE, objCenter, false, obj);
			if (suppressGrp && !suppressGrp.IsPlayerGroup() && CanCommitGroup(suppressGrp))
			{
				vector suppressPos = m_Ops.FindOverwatch(obj, suppressGrp.GetOwner().GetOrigin());
				if (suppressPos != vector.Zero)
				{
					suppressGrp.CompleteAllWaypoints();
					suppressGrp.SetGroupObjective(obj);
					if (!TryAssignTransport(suppressGrp, suppressPos, worldTime, DCO_EGroupTask.SUPPORT_BY_FIRE))
					{
						suppressGrp.SetTask(DCO_EGroupTask.SUPPORT_BY_FIRE);
						SpawnMoveRoute(suppressGrp, suppressGrp.GetOwner().GetOrigin(), suppressPos, worldTime);
					}
				}
			}
		}

		if (slotsLeft > 0)
		{
			DCO_GroupUtilityComponent assaultGrp = FindBestIdleGroupForTask(DCO_EGroupTask.ATTACK, objCenter, false, obj);
			if (assaultGrp && !assaultGrp.IsPlayerGroup() && CanCommitGroup(assaultGrp))
			{
				assaultGrp.CompleteAllWaypoints();

				vector from  = assaultGrp.GetOwner().GetOrigin();
				vector entry = GetEntryPoint(obj, from);

				int   arcCount = Math.Max(1, required);
				int   arcIndex = obj.GetCurrentAssignedGroupCount(m_sFactionKey) % arcCount;
				float arcSpan  = 180.0 / arcCount;
				float startDeg = ApproachAngle(from, objCenter) * Math.RAD2DEG - 90.0 + arcSpan * arcIndex;

				bool sent = TryAssignTransport(assaultGrp, entry, worldTime, DCO_EGroupTask.ATTACK);
				if (!sent)
				{
					array<SCR_AIWaypoint> searchWPs = {};
					GenerateSearchWaypoints(objCenter, objRad, searchWPs, 50.0, startDeg, arcSpan);
					if (!searchWPs.IsEmpty())
					{
						SpawnMoveRoute(assaultGrp, from, entry, worldTime);
						assaultGrp.MoveToRoute(searchWPs, worldTime);
						assaultGrp.SetTask(DCO_EGroupTask.ATTACK);
						sent = true;
					}
				}

				if (sent && assaultGrp.GetGroupObjective() != obj)
				{
					assaultGrp.SetGroupObjective(obj);
					obj.SetObjectiveGroup(m_sFactionKey, 1);
					slotsLeft = slotsLeft - 1;
				}
			}
		}

		if (slotsLeft > 0 && m_Tactics.AllowsFlankSwing(obj))
		{
			DCO_GroupUtilityComponent flankGrp = FindBestIdleGroupForTask(DCO_EGroupTask.FLANK, objCenter, false, obj);
			if (flankGrp && !flankGrp.IsPlayerGroup() && CanCommitGroup(flankGrp))
			{
				flankGrp.CompleteAllWaypoints();

				vector ffrom   = flankGrp.GetOwner().GetOrigin();
				int    pSide   = m_Tactics.NextFlankSide(obj);
				vector swingPt;
				if (pSide != 0)
					swingPt = FlankPoint(ffrom, objCenter, objRad + 220.0, 110.0, pSide);
				else
					swingPt = ComputeFlankPosition(ffrom, objCenter, objRad + 220.0);

				float approachDeg = ApproachAngle(ffrom, objCenter) * Math.RAD2DEG;
				float rel = ApproachAngle(swingPt, objCenter) * Math.RAD2DEG - approachDeg;
				while (rel > 180.0)
					rel -= 360.0;
				while (rel < -180.0)
					rel += 360.0;
				int side = 1;
				if (rel < 0.0)
					side = -1;

				bool fsent = TryAssignTransport(flankGrp, swingPt, worldTime, DCO_EGroupTask.FLANK);
				if (!fsent && SpawnMoveRoute(flankGrp, ffrom, swingPt, worldTime))
				{
					SCR_AIWaypoint edgeWp = SpawnMoveWP(FlankPoint(ffrom, objCenter, objRad + 60.0, 100.0, side));
					if (edgeWp)
						flankGrp.MoveTo(edgeWp, worldTime);

					array<SCR_AIWaypoint> flankWPs = {};
					GenerateSearchWaypoints(objCenter, objRad, flankWPs, 50.0, approachDeg + side * 110.0 - 35.0, 70.0);
					if (!flankWPs.IsEmpty())
						flankGrp.MoveToRoute(flankWPs, worldTime);

					flankGrp.SetTask(DCO_EGroupTask.FLANK);
					fsent = true;
				}

				if (fsent && flankGrp.GetGroupObjective() != obj)
				{
					flankGrp.SetGroupObjective(obj);
					obj.SetObjectiveGroup(m_sFactionKey, 1);
				}
			}
		}
	}

	protected void UpdateCommanderDebug(float timeSlice)
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

		int    flags = DCO_DebugDraw.Flags();
		vector p     = GetOwner().GetOrigin();
		float  now   = GetGame().GetWorld().GetWorldTime() / 1000.0;

		m_aDebugShapes.Insert(Shape.CreateSphere(DCO_DebugDraw.COLOR_COMMANDER, flags, p, 2.0));

		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(Vector(p[0], p[1] + 46.0, p[2]), BuildDebugHeader(now),   22.0, DCO_DebugDraw.COLOR_COMMANDER));
		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(Vector(p[0], p[1] + 37.0, p[2]), BuildDebugForce(),       17.0, DCO_DebugDraw.COLOR_COMMANDER));
		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(Vector(p[0], p[1] + 27.0, p[2]), BuildDebugPersonality(), 17.0, DCO_DebugDraw.COLOR_COMMANDER));
		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(Vector(p[0], p[1] + 16.0, p[2]), BuildDebugBudget(),      17.0, DCO_DebugDraw.COLOR_COMMANDER));
		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(Vector(p[0], p[1] +  4.0, p[2]), BuildDebugTuning(),      16.0, DCO_DebugDraw.COLOR_COMMANDER));

		DrawDebugObjectiveLinks(p, flags, now);
		DrawDebugFrontline(p, flags);
		m_Logistics.DrawDebug(m_aDebugShapes, m_aDebugTexts, flags);
	}

	protected string BuildDebugHeader(float now)
	{
		return string.Format(
			"%1  [%2]\nMODE %3   state: %4\nnext think in %5s of %6s",
			m_sCommanderUID,
			m_sFactionKey,
			DCO_DebugDraw.ModeName(m_eCommanderMode),
			DCO_DebugDraw.CommanderStateName(m_eCommanderState),
			DCO_DebugDraw.F1(Math.Max(m_fThinkInterval - m_fThinkTimer, 0.0)),
			DCO_DebugDraw.F1(m_fThinkInterval));
	}

	protected string BuildDebugForce()
	{
		int defendDemand = -1;
		if (m_eCommanderMode != CMD_ECommanderMode.OFFENSIVE)
			defendDemand = CountDefendDemand();

		return string.Format(
			"FORCE\nmanpower %1   reserve %2   floor %3   all-in %4\ngroups %5   idle/committable %6\nobjectives worked %7 of %8 max   garrison needed %9",
			GetTotalManpower(),
			GetReserveManpower(),
			Math.Round(GetReserveFloor()),
			m_bAllIn,
			m_aOwnedGroup.Count(),
			CountIdleCommittableGroups(),
			m_aObjective.Count(),
			m_fObjectiveAtTheSameTime,
			defendDemand);
	}

	protected string BuildDebugPersonality()
	{
		return string.Format(
			"PERSONALITY (0..1, drives the gates below)\naggression %1 -- tempo: staging vs recon roll\nrisk %2 -- commit into fog     patience %3 -- stalemate wait\nresilience %4 -- retreat depth  adaptability %5 -- think rate\ncombat focus %6 -- tier threshold, small-threat response",
			m_fAggression,
			m_fRiskTaking,
			m_fPatience,
			m_fResilience,
			m_fAdaptability,
			m_fCombatFocus);
	}

	protected string BuildDebugBudget()
	{
		string head = string.Format(
			"BUDGET (reserve policy)\nreserve floor = total %1 x %2 pct   all-in %3 (rear defenders %4)\nattack ratio >= %5 : 1",
			GetTotalManpower(),
			m_fReservePercent,
			m_bAllIn,
			m_bAllInRearDefenders,
			GetMinAttackRatio().ToString(-1, 1));

		if (m_eCommanderMode != CMD_ECommanderMode.BALANCED)
			return head + "\ndefend share n/a -- only BALANCED splits the idle pool";

		int   idle  = CountIdleCommittableGroups();
		float share = GetDefendShare();

		return head + string.Format(
			"\ndefend share %1 (from reserve policy)\n-> up to %2 of %3 idle groups reserved for defend",
			share, Math.Round(idle * share), idle);
	}

	protected string BuildDebugTuning()
	{
		string a = string.Format(
			"TUNING\nsync attack %1   sync max wait %2s   recon wait %3s",
			m_bUseSynchronizedAttack, m_fSyncAttackMaxWaitTime, m_fReconWaitTimeout);

		string b = string.Format(
			"\nstalemate cooldown %1s (patience-scaled)   retreat threshold %2\nsectors %3..%4 at %5m arc   sector personality mod %6",
			Math.Round(m_fStalemateResponseCooldown), m_iRetreatThreshold,
			m_iMinSector, m_iMaxSector, m_fArcPerSector, GetSectorPersonalityMod());

		string c = string.Format(
			"\ntransport beyond %1m   patrol pull max %2m   cluster min %3m\ngate defend by manpower %4   completion radius cap %5m",
			m_fTransportDistanceThreshold, m_fMaxPatrolPullDistance,
			m_fMinGroupClusterDistance, m_bGateDefendByManpower, m_fMaxCompletionRadius);

		return a + b + c;
	}

	protected void DrawDebugObjectiveLinks(vector cmdPos, int flags, float now)
	{
		for (int i = 0; i < m_aObjective.Count(); i++)
		{
			CMD_AICommanderObjectiveComponent obj = m_aObjective.Get(i);
			if (!obj || !obj.GetOwner())
				continue;

			vector objPos = obj.GetOwner().GetOrigin();

			int color;
			if (i == 0)
				color = DCO_DebugDraw.COLOR_TARGET;
			else
				color = DCO_DebugDraw.COLOR_QUEUED;

			m_aDebugShapes.Insert(Shape.CreateArrow(
				Vector(cmdPos[0], cmdPos[1] + 2.0, cmdPos[2]),
				Vector(objPos[0], objPos[1] + 2.0, objPos[2]),
				3.0, color, flags));

			float score = obj.ComputePriorityScore(
				m_sFactionKey, now, cmdPos, m_fCombatFocus, m_sCommanderUID);

			bool released;
			if (!m_mAssaultReleased.Find(obj, released))
				released = false;

			string phase;
			if (released)
				phase = "assault released";
			else if (m_mStagingStartTime.Contains(obj))
				phase = "gathering at staging";
			else
				phase = DCO_DebugDraw.ObjectiveStateName(obj.GetObjectiveState(m_sFactionKey));

			m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
				Vector(objPos[0], objPos[1] + 38.0, objPos[2]),
				string.Format("#%1 for %2\nscore %3   %4",
					i + 1, m_sCommanderUID, Math.Round(score), phase),
				18.0, color));

			vector staging;
			if (m_mStagingPos.Find(obj, staging))
			{
				m_aDebugShapes.Insert(Shape.CreateSphere(DCO_DebugDraw.COLOR_STAGING, flags, staging, 1.5));

				m_aDebugShapes.Insert(Shape.CreateArrow(
					Vector(staging[0], staging[1] + 2.0, staging[2]),
					Vector(objPos[0], objPos[1] + 2.0, objPos[2]),
					2.0, DCO_DebugDraw.COLOR_STAGING, flags));

				m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
					Vector(staging[0], staging[1] + 6.0, staging[2]),
					string.Format("STAGING\n%1 to objective   %2 from commander",
						DCO_DebugDraw.M(vector.Distance(staging, objPos)),
						DCO_DebugDraw.M(vector.Distance(staging, cmdPos))),
					16.0, DCO_DebugDraw.COLOR_STAGING));
			}
		}
	}

	protected void DrawDebugFrontline(vector cmdPos, int flags)
	{
		if (m_aFrontline.IsEmpty())
		{
			m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
				Vector(cmdPos[0], cmdPos[1] + 52.0, cmdPos[2]),
				"FRONTLINE: none\n" + m_sFrontlineReason,
				17.0, DCO_DebugDraw.COLOR_FRONTLINE));

			return;
		}

		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
			Vector(cmdPos[0], cmdPos[1] + 52.0, cmdPos[2]),
			string.Format("FRONTLINE: %1 segments\n%2", m_aFrontline.Count(), m_sFrontlineReason),
			17.0, DCO_DebugDraw.COLOR_FRONTLINE));

		const float DOT_SPACING = 12.0;

		for (int i = 0; i < m_aFrontline.Count(); i++)
		{
			DCO_FrontlineSegment seg = m_aFrontline.Get(i);
			if (!seg)
				continue;

			float segLen = vector.Distance(seg.m_vStart, seg.m_vEnd);
			int   dots   = Math.Max(2, (int)Math.Round(segLen / DOT_SPACING));

			for (int d = 0; d <= dots; d++)
			{
				float t = d / (float)dots;
				vector p = seg.m_vStart + ((seg.m_vEnd - seg.m_vStart) * t);
				p[1]     = GetGame().GetWorld().GetSurfaceY(p[0], p[2]) + 1.0;

				m_aDebugShapes.Insert(Shape.CreateSphere(
					DCO_DebugDraw.COLOR_FRONTLINE, flags, p, 0.6));
			}

			vector c = seg.Center();
			c[1]     = GetGame().GetWorld().GetSurfaceY(c[0], c[2]) + 2.0;

			m_aDebugShapes.Insert(Shape.CreateArrow(
				c, c + (seg.m_vFacing * 35.0), 3.0, DCO_DebugDraw.COLOR_FRONTLINE, flags));

			string own  = "?";
			string threat = "?";

			if (seg.m_Owned && seg.m_Owned.GetOwner())
				own = seg.m_Owned.GetOwner().GetName();

			if (seg.m_Threat && seg.m_Threat.GetOwner())
				threat = seg.m_Threat.GetOwner().GetName();

			m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
				Vector(c[0], c[1] + 8.0, c[2]),
				string.Format("FRONTLINE %1/%2\npressure %3\n%4 -> %5",
					i + 1,
					m_aFrontline.Count(),
					DCO_DebugDraw.F1(seg.m_fPressure),
					own,
					threat),
				15.0, DCO_DebugDraw.COLOR_FRONTLINE));
		}
	}
	protected void ThinkCaptureProgress(float worldTime)
	{
	    if (!Replication.IsServer())
	        return;

	    AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
	    if (!mgr)
	        return;

	    foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
	    {
	        if (!obj)
	            continue;

	        CMD_EObjectiveState state = obj.GetObjectiveState(m_sFactionKey);

	        if (state == CMD_EObjectiveState.COMPLETED || state == CMD_EObjectiveState.FAILED)
	            continue;

	        if (obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
	            continue;

	        int friendlyNear;
	        int enemyNear;
	        obj.CountNearbyUnitsCached(obj.GetRadius(), m_sFactionKey, friendlyNear, enemyNear);

	        if (friendlyNear < enemyNear && obj.GetCurrentAssignedGroupCount(m_sFactionKey) > 0)
	        {
	            obj.SetObjectiveState(m_sFactionKey, CMD_EObjectiveState.ASSIGNED);
	            state = CMD_EObjectiveState.ASSIGNED;
	        }

	        if (state != CMD_EObjectiveState.ASSIGNED)
	            continue;

	        if (!obj.IsCaptureTimerRunning(m_sFactionKey))
	        {
	            if (friendlyNear > 0 && friendlyNear >= enemyNear)
	                obj.StartCaptureTimer(m_sFactionKey, worldTime);

	            continue;
	        }

	        obj.AssessObjective(m_sFactionKey, worldTime);

	        if (obj.IsStalemate(m_sFactionKey, worldTime))
	            HandleStalemateObjective(obj, worldTime);

	        if (obj.IsCaptureTimerComplete(m_sFactionKey, worldTime))
	        {
	            if (CQBBlocksCapture(obj, worldTime))
	                continue;

	            obj.SetCapturedBy(m_sFactionKey, true);
	            obj.ResetAssignedGroupCount(m_sFactionKey);
	            obj.ResetStalemateTracking();
	            SweepObjective(obj);
	            m_Tactics.OnCaptured(this, obj, worldTime);
	        }
	    }
	}

	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mCaptureBlockLog = new map<CMD_AICommanderObjectiveComponent, float>();

	protected bool CQBBlocksCapture(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (!cfg || !cfg.GetCQBEnabled() || !cfg.GetCQBCaptureWaits())
			return false;

		float now_ms = GetGame().GetWorld().GetWorldTime();
		IEntity building = DCO_BuildingClear.FindContactBuilding(obj.GetOwner().GetOrigin(), obj.GetRadius(), m_sFactionKey, cfg.GetCQBContactTimeout() * 1000, now_ms);
		if (!building)
			return false;

		SweepBuilding(obj, building, now_ms);
		if (worldTime - m_mCaptureBlockLog.Get(obj) > 30.0)
		{
			m_mCaptureBlockLog.Set(obj, worldTime);
			DCO_BenchmarkLoggerComponent.Event(string.Format("capture_blocked obj=%1 building=%2 faction=%3", obj.GetOwner().GetName(), building, m_sFactionKey));
		}
		return true;
	}

	protected void SweepObjective(CMD_AICommanderObjectiveComponent obj)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		if (!cfg || !cfg.GetCQBEnabled() || !reg)
			return;

		float now_ms = GetGame().GetWorld().GetWorldTime();
		array<IEntity> buildings = {};
		reg.FindBuildings(obj.GetOwner().GetOrigin(), obj.GetRadius(), buildings);
		foreach (IEntity b : buildings)
		{
			if (DCO_BuildingClear.State(b, m_sFactionKey) == 3)
				continue;
			if (!SweepBuilding(obj, b, now_ms))
				return;
		}
	}

	protected bool SweepBuilding(CMD_AICommanderObjectiveComponent obj, IEntity building, float now_ms)
	{
		vector p = obj.GetOwner().GetOrigin();
		float r = obj.GetRadius() + 50.0;
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (!g || g.GetGroupObjective() != obj || g.IsPlayerGroup() || vector.DistanceXZ(g.GetOwner().GetOrigin(), p) > r)
				continue;

			SCR_AIGroup grp = SCR_AIGroup.Cast(g.GetOwner());
			if (!grp || !grp.GetGroupUtilityComponent())
				continue;
			if (grp.GetGroupUtilityComponent().DCO_StartSweep(building, now_ms))
				return true;
		}
		return false;
	}

	protected IEntity TryAssignTransports(DCO_GroupUtilityComponent passengerGroup)
	{
		if (!passengerGroup)
			return null;

		int unitCount = passengerGroup.GetUnitCount();
		if (unitCount <= 0)
			return null;

		IEntity ownedVeh = passengerGroup.GetOwnedVehicle();
		if (ownedVeh)
		{
			DamageManagerComponent dmg = DamageManagerComponent.Cast(ownedVeh.FindComponent(DamageManagerComponent));
			bool vehicleDestroyed = dmg && dmg.GetState() == EDamageState.DESTROYED;

			DCO_TransportMissionComponent ownedMission = DCO_TransportMissionComponent.Cast(ownedVeh.FindComponent(DCO_TransportMissionComponent));

			float ownedDist = vector.Distance(passengerGroup.GetOwner().GetOrigin(), ownedVeh.GetOrigin());
			if (!vehicleDestroyed && ownedMission && !ownedMission.IsActiveVehicle() && ownedDist <= m_fVehicleSearchRadius * 3)
				return ownedVeh;

			if (vehicleDestroyed || !ownedMission)
			{
				if (ownedMission)
					ownedMission.ReleaseOwnership();
				passengerGroup.SetOwnedVehicle(null);
			}
		}

		vector groupPos = passengerGroup.GetOwner().GetOrigin();
		IEntity vehicle  = null;
		float   bestScore = float.MAX;

		foreach (IEntity cand : m_aVehicle)
		{
			if (!CMD_VehicleFinder.FindNearestVehicle(cand, groupPos, unitCount, passengerGroup))
				continue;

			float d = vector.Distance(groupPos, cand.GetOrigin());
			if (d > m_fVehicleSearchRadius)
				continue;

			float score = d + CMD_VehicleFinder.SeatFitPenalty(cand, unitCount);
			if (score < bestScore)
			{
				bestScore = score;
				vehicle   = cand;
			}
		}

		return vehicle;
	}

	protected void BeginTransportMission(
		DCO_GroupUtilityComponent passengerGroup,
		IEntity                   vehicle,
		vector                    destination,
		float                     worldTime)
	{
		DCO_TransportMissionComponent mission =
			DCO_TransportMissionComponent.Cast(vehicle.FindComponent(DCO_TransportMissionComponent));

		if (!mission)
		{
			return;
		}

		if (mission.IsActiveVehicle())
		{
			return;
		}

		mission.ClaimOwnership(passengerGroup);
		passengerGroup.SetOwnedVehicle(vehicle);

		passengerGroup.BeginTransport(DCO_EGroupTask.NONE);

		SCR_AIGroup grp = SCR_AIGroup.Cast(passengerGroup.GetOwner());
		if (!grp)
			return;

		grp.CompleteAllWaypoints();

		SCR_AIWaypoint wpToVehicle = SpawnMoveWP(vehicle.GetOrigin());
		if (wpToVehicle)
			passengerGroup.MoveTo(wpToVehicle, worldTime);

		mission.StartMission(passengerGroup, destination, m_sFactionKey, worldTime, this);
	}

	SCR_AIWaypoint SpawnGetInWP(vector pos)
	{
		AICommander_BaseComponentClass data = AICommander_BaseComponentClass.Cast(GetComponentData(GetOwner()));
		if (!data)
			return null;

		Resource res = Resource.Load(data.GetDefaultGetInWaypointPrefab());
		if (!res || !res.IsValid())
			return null;

		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = pos;

		return SCR_AIWaypoint.Cast(GetGame().SpawnEntityPrefab(res, null, params));
	}

	SCR_AIWaypoint SpawnGetOutWP(vector pos)
	{
		AICommander_BaseComponentClass data = AICommander_BaseComponentClass.Cast(GetComponentData(GetOwner()));
		if (!data)
			return null;

		Resource res = Resource.Load(data.GetDefaultGetOutWaypointPrefab());
		if (!res || !res.IsValid())
			return null;

		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = pos;

		return SCR_AIWaypoint.Cast(GetGame().SpawnEntityPrefab(res, null, params));
	}

	DCO_GroupUtilityComponent FindBestIdleGroupForTask_Public(DCO_EGroupTask task, vector pos, int wantedCaps = -1, bool armor = false, int purpose = 1)
	{
	    return FindBestIdleGroupForTask(task, pos, false, null, wantedCaps, armor, purpose);
	}

	void RegisterTransportTeam(DCO_TransportTeamComponent team)
	{
		if (!team || m_aTransportTeams.Contains(team))
			return;

				team.SetRallyPoint(GetOwner().GetOrigin());
		team.SetCommander(this);
		m_aTransportTeams.Insert(team);
	}

	bool TryAssignTransport(DCO_GroupUtilityComponent passengerGroup, vector destination, float worldTime, DCO_EGroupTask taskAfter = DCO_EGroupTask.NONE)
	{
		if (!passengerGroup || !passengerGroup.CanBeTransported())
			return false;

		float dist = vector.Distance(passengerGroup.GetOwner().GetOrigin(), destination);
		if (dist < m_fTransportDistanceThreshold)
			return false;

		int unitCount = passengerGroup.GetUnitCount();
		if (unitCount <= 0)
			return false;

		if (m_Logistics.RequestTransport(this, passengerGroup, destination, taskAfter, worldTime))
			return true;

		vector groupPos = passengerGroup.GetOwner().GetOrigin();
		IEntity vehicle = TryAssignTransports(passengerGroup);

		if (!vehicle)
        	return false;

		passengerGroup.SetTask(taskAfter);
		BeginTransportMission(passengerGroup, vehicle, destination, worldTime);
		return true;
	}

	void UnregisterTransportTeam(DCO_TransportTeamComponent team)
	{
		m_aTransportTeams.RemoveItem(team);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		 m_fThinkTimer += timeSlice;
		 m_fCaptureCheckTimer += timeSlice;

		UpdateCommanderDebug(timeSlice);

		if (Replication.IsServer())
		{
			int pt = DCO_Perf.Begin();
			m_Logistics.Update(this, timeSlice);
			DCO_Perf.End("cmd_logistics", pt);
			pt = DCO_Perf.Begin();
			m_Ops.Tick(this, timeSlice);
			DCO_Perf.End("cmd_ops", pt);
			pt = DCO_Perf.Begin();
			m_Defense.Tick(this, timeSlice);
			DCO_Perf.End("cmd_defense", pt);
			pt = DCO_Perf.Begin();
			m_Tactics.Tick(this, timeSlice);
			DCO_Perf.End("cmd_tactics", pt);
		}

		if (m_fThinkTimer >= m_fThinkInterval)
		{
			m_fThinkTimer = 0.0;
			float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
			int tt = DCO_Perf.Begin();
			Think(worldTime);
			DCO_Perf.End("cmd_think", tt);
		}

		if (m_fCaptureCheckTimer >= m_fCaptureCheckInterval)
		{
		    m_fCaptureCheckTimer = 0.0;
		    ThinkCaptureProgress(GetGame().GetWorld().GetWorldTime() / 1000.0);

		    UpdateFrontlineReconTracks(GetGame().GetWorld().GetWorldTime() / 1000.0);

		    if (m_bPlayerTasking && Replication.IsServer())
		        m_PlayerTasking.CheckCompletion(this);
		}
	}

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
		SetEventMask(owner, EntityEvent.FRAME);
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		m_MyEnt = owner;
		InitializeCommander();
	}

	override void OnDelete(IEntity owner)
	{
		if (Replication.IsServer() && GetGame() && GetGame().GetWorld())
			ReleaseEverything();

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
			mgr.UnregisterCommander(this);

		super.OnDelete(owner);
	}
}
