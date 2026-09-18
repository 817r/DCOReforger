[ComponentEditorProps(category: "GameScripted/Commander")]
class AICommander_BaseComponentClass : ScriptComponentClass
{
	// TO-DO GANTI SEMUA JADI YANG GA EDITABLE
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

//! Pola rute patroli. Dulu cuma ada satu bentuk (lingkaran) dan itu yang bikin
//! gerakan grup idle gampang ditebak.
//! Satu ruas garis frontline. Frontline bukan lagi satu titik, tapi rangkaian
//! segmen yang dibentuk dari batas antara wilayah kita dan wilayah lawan.
class DCO_FrontlineSegment
{
	vector m_vStart;
	vector m_vEnd;

	//! Normal segmen, menghadap menjauhi wilayah kita. Dipakai drift patroli supaya
	//! grup merangkak tegak lurus ke perbatasan.
	vector m_vFacing;

	//! Jumlah musuh di objective ancaman dibagi jarak. Recon pergi ke yang tertinggi.
	float m_fPressure;

	CMD_AICommanderObjectiveComponent m_Owned;
	CMD_AICommanderObjectiveComponent m_Threat;

	vector Center()
	{
		return (m_vStart + m_vEnd) * 0.5;
	}

	//! Proyeksi titik ke ruas ini, dijepit ke ujung-ujungnya. Ini yang bikin grup bisa
	//! menyebar sepanjang perbatasan alih-alih semuanya menuju satu titik.
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
	RING    = 0,   //!< lingkaran penuh -- pola lama, sekarang salah satu pilihan
	LANE    = 1,   //!< bolak-balik melintang, tegak lurus arah ancaman
	ARC     = 2,   //!< busur 120-160 derajat menghadap ancaman
	ADVANCE = 3    //!< netto maju ke arah ancaman, bergoyang menyamping
}

class AICommander_BaseComponent : ScriptComponent
{
	[Attribute("", UIWidgets.Font, desc: "UID of the Commander.", category: "Commander General Setting")]
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
	
	protected ref array<ref CMD_FrontlineReconTrack> m_aFrontlineReconTracks = new array<ref CMD_FrontlineReconTrack>();

	[Attribute("500.0", UIWidgets.EditBox, "Jarak maksimum (meter) grup idle boleh ditarik buat patrol. Kalau kandidat terdekat (HQ/objective captured) lebih jauh dari ini, patrol lokal di posisi sekarang aja.", category: "Commander Setting")]
	protected float m_fMaxPatrolPullDistance;

	// === ADDED: Commander Patrol ===
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

	//! Kenapa frontline kosong, kalau memang kosong. Overlay menampilkannya supaya
	//! garis yang tidak muncul tidak pernah jadi kegagalan diam-diam.
	protected string m_sFrontlineReason = "not built yet";

	[Attribute("0.6", UIWidgets.Range, "Pengali radius patroli buat grup paling kecil.", params: "0.2 2 0.05", category: "Commander Patrol")]
	protected float m_fPatrolRadiusMulMin;

	[Attribute("1.6", UIWidgets.Range, "Pengali radius patroli buat grup penuh (12 orang).", params: "0.2 3 0.05", category: "Commander Patrol")]
	protected float m_fPatrolRadiusMulMax;

	//! Sisa jatah penilaian medan di cycle ini. Di-reset tiap SendIdleGroupsToReserve.
	protected int m_iPatrolSmartRemaining = 0;

	//! Pusat patroli terakhir per grup. Kunci komponen -- pola yang sama dipakai
	//! m_mAssaultReleased, jadi sudah kebukti jalan di codebase ini.
	protected ref map<DCO_GroupUtilityComponent, ref array<vector>> m_mPatrolMemory = new map<DCO_GroupUtilityComponent, ref array<vector>>();
	// === END ADDED ===

	// === ADDED: Staging & Route ===
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
	// === END ADDED ===

	[Attribute("90.0", UIWidgets.EditBox, "Berapa lama (detik) grup suppress support nutupin assault force begitu di-release, sebelum dilepas balik ke pool.", category: "Commander Setting")]
	protected float m_fAssaultSuppressDuration;
	
	[Attribute("45.0", UIWidgets.EditBox, "Berapa lama (detik) grup suppress cover nutupin arah kontak begitu ada grup lain retreat.", category: "Commander Setting")]
	protected float m_fRetreatCoverDuration;
	
	[Attribute("50.0", UIWidgets.EditBox, "Radius suppress buat nutupin retreat.", category: "Commander Setting")]
	protected float m_fRetreatCoverRadius;
	
	[Attribute("400.0", UIWidgets.EditBox, "Jarak minimum sebelum cari transport", category: "Commander Setting")]
	protected float m_fTransportDistanceThreshold;
	
	[Attribute("50.0", UIWidgets.EditBox, "Radius pencarian kendaraan", category: "Commander Setting")]
	protected float m_fVehicleSearchRadius;
	
	[Attribute("50.0", UIWidgets.EditBox, "Radius Close To Commander", category: "Commander Setting")]
	protected float m_fBaseRadius;
	
	[Attribute("0.6", UIWidgets.Range, "Defend Chances Instead Patrol Around", params: "0 1 0.01", category: "Commander Setting")]
	protected float m_fDefendChance;
	
	[Attribute("1", UIWidgets.CheckBox, desc: "Aktifkan Manpower Budgeting? Kalau false, commander bisa komit semua grup ke offense tanpa reserve (behavior lama).", category: "Commander Manpower Budget")]
	protected bool m_bEnableManpowerBudget;
	
	[Attribute("0.2", UIWidgets.Range, "Minimum persentase total manpower yang WAJIB tetap jadi reserve (tidak dikomit ke offense). 0.2 = 20% pasukan selalu disisain.", params: "0 1 0.01", category: "Commander Manpower Budget")]
	protected float m_fReserveMinimumPct;
	
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

	// === ADDED: jatah defend di mode BALANCED, sebagai fraksi grup idle yang
	// tersedia. Digeser sama Eagerness: agresif -> Min (nahan sedikit buat defend),
	// hati-hati -> Max (nahan banyak). Cuma dipakai mode BALANCED; OFFENSIVE dan
	// DEFENSIVE gak kena batas ini sama sekali.
	[Attribute("0.2", UIWidgets.Range, "BALANCED: fraksi grup idle yang disisihkan buat defend waktu Eagerness = 1 (paling agresif).", params: "0 1 0.01", category: "Commander Personality")]
	protected float m_fDefendShareMin;

	[Attribute("0.7", UIWidgets.Range, "BALANCED: fraksi grup idle yang disisihkan buat defend waktu Eagerness = 0 (paling hati-hati).", params: "0 1 0.01", category: "Commander Personality")]
	protected float m_fDefendShareMax;

	//! Batas berapa grup lagi yang boleh ditarik FindBestIdleGroupForRole() di fase
	//! ini. -1 = tanpa batas (perilaku lama, dipakai OFFENSIVE & DEFENSIVE murni).
	protected int m_iPhaseBudget = -1;
	// === END ADDED ===

	// === ADDED: Commander Debug ===
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
	// === END ADDED ===
	
 
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
	
	// === ADDED: Synchronized Attack ===
	// Assault groups gak lagi langsung diarahin ke objective satu-satu begitu dapet
	// slot (piecemeal). Sekarang mereka ngumpul dulu di staging position sampai FULL
	// (m_fSyncAttackMaxWaitTime detik timeout kalau gak nyampe penuh juga), baru
	// SEMUANYA di-release bareng ke objective di cycle yang sama.
	protected ref map<CMD_AICommanderObjectiveComponent, bool>  m_mAssaultReleased    = new map<CMD_AICommanderObjectiveComponent, bool>();
	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mStagingStartTime   = new map<CMD_AICommanderObjectiveComponent, float>();

	// === ADDED: posisi staging per objective.
	// PERUBAHAN PERILAKU, bukan cuma debug. Dulu stagingPos dihitung inline di
	// TrySendToStaging DAN di TryGatherForSynchronizedAssault, dua-duanya pakai
	// Math.RandomFloatInclusive(0.15, 0.4) yang di-ROLL ULANG tiap pemanggilan.
	// Akibatnya tiap grup dikirim ke titik kumpul yang BERBEDA -- untuk objective jauh
	// selisihnya bisa ratusan meter, jadi "synchronized" assault-nya gak sinkron sama
	// sekali. Sekarang dihitung SEKALI per objective lalu dipakai ulang.
	protected ref map<CMD_AICommanderObjectiveComponent, vector> m_mStagingPos = new map<CMD_AICommanderObjectiveComponent, vector>();
	// === END ADDED ===
	
	// === ADDED: Recon Wait Timeout ===
	// Recon itu OPSIONAL, bukan syarat wajib buat nyerang -- tergantung availability
	// (ada grup RECON apa enggak) dan personality (Eagerness). Commander harus SELALU
	// advancing, gak boleh nunggu recon selamanya. m_mAssignedTime nyatet kapan objective
	// ini di-mark ASSIGNED (independen dari Synchronized Attack toggle), dipake buat
	// nge-timeout recon-wait di bawah.
	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mAssignedTime = new map<CMD_AICommanderObjectiveComponent, float>();
	
	[Attribute("45.0", UIWidgets.EditBox, "Waktu maksimum (detik) nunggu recon konfirmasi sebelum SKIP dan tetep nyerang. Recon opsional -- ini nyegah commander stuck nunggu recon yang gak kunjung dateng/gak available.", category: "Commander Objective Setting")]
	protected float m_fReconWaitTimeout;
	// === END ADDED ===
	
	// === ADDED: Recon Reveal ===
	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mLastReconRevealTime = new map<CMD_AICommanderObjectiveComponent, float>();
	// === END ADDED ===
	
	protected float m_fThinkTimer = 0 - m_fDelayFirstIteration;
	
	protected const float m_fFlankAngleMin = 15;
	protected const float m_fFlankAngleMax = 315;
	
	int GetOwnedGroupCount() { return m_aOwnedGroup.Count(); }
	int GetOwnedVehicle()	{return m_aVehicle.Count(); }
	
	// === ADDED: Manpower Budget System ===
	protected int m_iManpowerTotalCache = 0; // diisi ulang tiap Think() cycle, dipakai GetReserveFloor()
	
	// Total manpower = jumlah unit dari semua grup yang dimiliki commander ini.
	int GetTotalManpower()
	{
		int total = 0;
		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp)
				continue;
			total += grp.GetUnitCount();
		}
		return total;
	}
	
	// Reserve manpower = unit dari grup yang belum dikomit (role NONE atau RESERVE).
	int GetReserveManpower()
	{
		int reserve = 0;
		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp)
				continue;
			
			CMD_EGroupRole role = grp.GetGroupRole();
			if (role == CMD_EGroupRole.NONE || role == CMD_EGroupRole.RESERVE)
				reserve += grp.GetUnitCount();
		}
		return reserve;
	}
	
	// Berapa manpower minimum yang wajib tetap nganggur (gak boleh dikomit ke offense).
	// === OPTIMIZED: pakai total manpower yang di-cache sekali per Think() cycle,
	// bukan re-scan m_aOwnedGroup tiap kali CanCommitGroup() dipanggil (bisa berkali-kali
	// per cycle -- 1x per grup yang dicoba commit ke offense). Total force gak berubah
	// dalam satu Think() cycle (cuma role yang berubah), jadi aman di-cache per-cycle.
	float GetReserveFloor()
	{
		// === ADDED: Combat Focus -- ngaruh juga ke manpower budget. CombatFocus
		// tinggi lebih berani komit banyak pasukan ke fight/task (reserve floor turun
		// sampe 0.5x -- nahan lebih sedikit di belakang), CombatFocus rendah lebih
		// konservatif (reserve floor naik sampe 1.5x -- nahan lebih banyak cadangan).
		float combatFocusMod = Math.Lerp(1.5, 0.5, m_fCombatFocus);
		return m_iManpowerTotalCache * m_fReserveMinimumPct * combatFocusMod;
		// === END ADDED ===
	}
	
	// Cek apakah grup ini boleh dikomit ke offense (RECON/ASSAULT/FLANK) tanpa
	// nembus reserve floor. Dipanggil SEBELUM grup di-assign role offensive.
	// Defend tidak lewat gate ini karena itu memang tujuan reserve.
	bool CanCommitGroup(DCO_GroupUtilityComponent grp)
	{
		if (!m_bEnableManpowerBudget)
			return true;
		
		if (!grp)
			return false;
		
		float reserveAfterCommit = GetReserveManpower() - grp.GetUnitCount();
		return reserveAfterCommit >= GetReserveFloor();
	}
	
	// Buat debug overlay / mission maker visibility (opsional dipanggil dari luar).
	string GetManpowerBudgetStatus()
	{
		return string.Format("[%1] Manpower Total:%2 Reserve:%3 Floor:%4 Enabled:%5",
			m_sCommanderUID, GetTotalManpower(), GetReserveManpower(), GetReserveFloor(), m_bEnableManpowerBudget);
	}
	// === END ADDED ===
	
	bool send = true;
	
	bool RegisterGroup(DCO_GroupUtilityComponent grp)
	{
		
		if (grp.GetGroupRole() == CMD_EGroupRole.ARTILLERY)
		{
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

		return true;
	}
	
	protected void InitializeCommander()
	{
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
	
	CMD_ThreatResponseComponent GetThreatResponseComponent()
	{
		return threatComp;
	}
	
	FactionKey GetCommanderFactionKey()
	{
		return m_sFactionKey;
	}
	
	string GetCommanderUID()
	{
		return m_sCommanderUID;
	}
	
	void SwitchToDefensive(float worldTime)
	{
		m_eCommanderMode            = CMD_ECommanderMode.DEFENSIVE;
		m_fDefensiveTriggerCooldown = worldTime;
	 
		//Print(string.Format("[%1] SWITCHING TO DEFENSIVE MODE", m_sCommanderUID));
	}
	 
	void SwitchToOffensive()
	{
		m_eCommanderMode = CMD_ECommanderMode.OFFENSIVE;
	 
		//Print(string.Format("[%1] SWITCHING TO OFFENSIVE MODE", m_sCommanderUID));
	}
	 
	// === ADDED: dulu gak ada jalur apapun yang naruh BALANCED ke m_eCommanderMode,
	// jadi cabang BALANCED di Think() itu dead code. Sekarang tiga mode punya
	// override runtime yang setara.
	void SwitchToBalanced()
	{
		m_eCommanderMode = CMD_ECommanderMode.BALANCED;
	}
	// === END ADDED ===
	 
	// Manual override dari luar (misalnya game mode bisa force defensive)
	void ForceDefensiveMode(float worldTime)   { SwitchToDefensive(worldTime); }
	void ForceOffensiveMode()                  { SwitchToOffensive(); }
	 
	CMD_ECommanderMode GetCommanderMode()      { return m_eCommanderMode; }
	
	vector ComputeFlankPosition(vector base, vector objective, float distance)
	{
		vector axis = objective - base;
		axis = Vector(axis[0], 0.0, axis[2]);
		axis = axis.Normalized();
	 
		float angleDeg = Math.RandomFloat(m_fFlankAngleMin, m_fFlankAngleMax);
		float angleRad = angleDeg * Math.DEG2RAD;
	 
		float cosA = Math.Cos(angleRad);
		float sinA = Math.Sin(angleRad);

		vector dirLeft  = Vector(
			axis[0] * cosA - axis[2] * sinA,
			0.0,
			axis[0] * sinA + axis[2] * cosA);
	 
		vector dirRight = Vector(
			axis[0] * cosA + axis[2] * sinA,
			0.0,
			axis[0] * (-sinA) + axis[2] * cosA);
	 
		float axisRatio = Math.RandomFloat(0.45, 0.70);
		float totalDist = vector.Distance(base, objective);
		vector midpoint = base + axis * (totalDist * axisRatio);
	 
		vector candidateLeft  = midpoint + dirLeft  * distance;
		vector candidateRight = midpoint + dirRight * distance;
	 
		candidateLeft[1]  = GetGame().GetWorld().GetSurfaceY(candidateLeft[0],  candidateLeft[2]);
		candidateRight[1] = GetGame().GetWorld().GetSurfaceY(candidateRight[0], candidateRight[2]);
	 
		float scoreLeft  = EvaluateFlankCandidate(candidateLeft,  objective);
		float scoreRight = EvaluateFlankCandidate(candidateRight, objective);
	 
		vector chosen;
	 
		if (scoreLeft >= scoreRight)
			chosen = candidateLeft;
		else
			chosen = candidateRight;
	 
		chosen[1] = GetGame().GetWorld().GetSurfaceY(chosen[0], chosen[2]);
	 
		return chosen;
	}
	
	protected float EvaluateFlankCandidate(vector candidate, vector objective)
	{
		vector toObjective = objective - candidate;
		toObjective = Vector(toObjective[0], 0.0, toObjective[2]);
		toObjective = toObjective.Normalized();
	 
		vector toBase = GetOwner().GetOrigin() - candidate;
		toBase = Vector(toBase[0], 0.0, toBase[2]);
		toBase = toBase.Normalized();
	 
		float dot         = vector.Dot(toObjective, toBase);
		float angleScore  = 1.0 - Math.AbsFloat(dot);
	 
		return angleScore;
	}
 
	// === ADDED: satu titik kumpul per objective, dihitung sekali lalu dipakai ulang.
	// CATATAN: rumusnya masih SAMA PERSIS kayak sebelumnya -- diukur dari posisi
	// COMMANDER, 15-40% jalan menuju objective. Yang berubah cuma dia gak di-roll ulang
	// tiap pemanggilan.
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

		float dist = vector.Distance(base, objPos);

		// === MODIFIED: titik kumpul sekarang diukur DARI OBJECTIVE, bukan dari
		// commander. Rumus lama (base + axis * dist * 0.15..0.4) naruh titik kumpul
		// deket rumah: buat objective 8 km, grup ngumpul 1,2-3,2 km dari markas lalu
		// jalan SENDIRI-SENDIRI 5-7 km lagi. Selama perjalanan itu mereka gak sinkron
		// dan bisa kena kontak satu per satu -- seluruh guna synchronized assault
		// hilang persis di kasus yang paling butuh.
		//
		// Sekarang jaraknya di-clamp, jadi titik kumpul selalu 200-600 m dari target
		// BERAPAPUN jauhnya commander. Grup naik transport, turun deket target,
		// ngumpul, baru serentak masuk.
		//
		// Batas bawah dinaikin otomatis kalau radius objective lebih besar: staging
		// DI DALAM radius bikin objective nilai dirinya CONTESTED terus, dan capture
		// -nya gak akan pernah mulai.
		float minDist = Math.Max(m_fStagingMinDistance, obj.GetRadius() + m_fStagingMargin);
		float maxDist = Math.Max(m_fStagingMaxDistance, minDist);

		float standoff = Math.Clamp(dist * m_fStagingLegFraction, minDist, maxDist);

		// Kalau commander lebih deket dari standoff-nya, jangan naruh titik kumpul di
		// belakang commander -- pakai titik tengah aja.
		if (standoff >= dist)
			standoff = dist * 0.5;

		vector pos = objPos - axis * standoff;

		pos = PullStagingClearOfHostileObjectives(obj, base, axis, dist, standoff, pos);
		pos[1] = GetGame().GetWorld().GetSurfaceY(pos[0], pos[2]);
		// === END MODIFIED ===

		m_mStagingPos.Insert(obj, pos);

		return pos;
	}

	protected void ClearStagingPos(CMD_AICommanderObjectiveComponent obj)
	{
		if (m_mStagingPos.Contains(obj))
			m_mStagingPos.Remove(obj);
	}

	//! Geser titik staging mundur sampai keluar dari radius objective manapun yang
	//! bukan punya faction kita -- termasuk objective TUJUAN itu sendiri.
	//!
	//! Selain alasan taktis (grup kena kontak sebelum siap), ada alasan mekanis yang
	//! lebih penting: grup yang ngumpul di dalam radius objective tujuan bikin
	//! objective itu nilai dirinya CONTESTED terus, jadi capture-nya GAK PERNAH mulai.
	// === MODIFIED: objective TUJUAN sekarang dikecualikan. Titik kumpul memang
	// sengaja ditaruh dekat target -- jarak amannya udah dijaga standoff di
	// GetOrCreateStagingPos. Yang dihindari cuma objective musuh LAIN yang kebetulan
	// kelewatan di jalur. Penggeserannya juga mundur menjauhi target (nambah
	// standoff), bukan mundur ke arah commander pakai fraksi.
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

			// Jangan mundur sampai lewat commander -- lebih baik titik kumpul kurang
			// ideal daripada ada di belakang garis berangkat.
			if (standoff >= legDist)
				return candidate;

			candidate = targetPos - axis * standoff;
		}

		return candidate;
	}
	// === END ADDED ===

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
	        return;
	
	    AICommander_ManagerComponent mgrCheck = AICommander_ManagerComponent.GetInstance();
	    if (objState == CMD_EObjectiveState.PENDING)
	    {
	        bool isFoggy;
	        if (mgrCheck && contextCache)
	            isFoggy = !mgrCheck.IsObjectiveIntelCoveredCached(obj, contextCache);
	        else
	            isFoggy = mgrCheck && !mgrCheck.IsObjectiveIntelCovered(obj, m_sFactionKey);

	        if (isFoggy && m_fRiskTaking < Math.RandomFloat01())
	        {
	            TrySendRecon(obj);

	            // === ADDED: TrySendRecon() manggil obj.MarkAssigned() di ujungnya, jadi
	            // state pindah ke ASSIGNED. Tanpa RecordAssignedTime, cycle berikutnya
	            // m_mAssignedTime.Find() gagal -> reconTimedOut langsung true -> guard
	            // tunggu-recon dilewati. Commander ngirim recon lalu nyerang cycle
	            // berikutnya tanpa peduli recon-nya sampai atau belum, bikin seluruh
	            // m_fReconWaitTimeout gak ada artinya di jalur ini.
	            RecordAssignedTime(obj, worldTime);
	            // === END ADDED ===

	            return;
	        }
	    }
	
	    if (objState == CMD_EObjectiveState.PENDING)
	    {
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
	        float assignedTime;
	        bool hasAssignedRecord = m_mAssignedTime.Find(obj, assignedTime);
	        bool reconTimedOut = !hasAssignedRecord || (worldTime - assignedTime) > m_fReconWaitTimeout;

	        if (!reconTimedOut && m_fAggression < Math.RandomFloat01() && !obj.IsReconArrived(m_sFactionKey, worldTime))
	        {
	            return;
	        }

	        // === ADDED: objective yang radius-nya udah bersih gak perlu serangan
	        // serentak sama sekali. Dulu commander tetap nyuruh grup ngumpul di staging
	        // dan nunggu slot penuh / timeout m_fSyncAttackMaxWaitTime -- padahal gak ada
	        // yang mau diserang. Itu yang bikin release kerasa kelamaan: waktunya habis
	        // buat persiapan tempur di tempat yang kosong.
	        //
	        // Objective yang nentuin dia bersih atau enggak (IsUncontested), bukan
	        // commander. Kalau bersih: lepas apapun yang lagi ngumpul, terus kirim
	        // langsung. Kalau musuh muncul lagi nanti, statusnya balik CONTESTED dan
	        // objective ngereset akumulator capture-nya sendiri.
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
	        // === END ADDED ===

	        if (m_bUseSynchronizedAttack)
	        {
	            bool released;
	            if (!m_mAssaultReleased.Find(obj, released))
	                released = false;

	            if (!released)
	            {
	                float stagingStart;
	                if (!m_mStagingStartTime.Find(obj, stagingStart))
	                {
	                    stagingStart = worldTime;
	                    m_mStagingStartTime.Insert(obj, stagingStart);
	                }

	                bool isFull   = obj.IsGroupSlotFull(m_sFactionKey);
	                bool arrived  = AreStagedGroupsArrived(obj);
	                bool timedOut = (worldTime - stagingStart) > m_fSyncAttackMaxWaitTime;

	                if ((isFull && arrived) || timedOut)
	                {
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
		
		// === MODIFIED: census objective (satu query dipakai bareng) ===
		int reconFriendlyCount;
		int enemyCount;
		obj.CountNearbyUnitsCached(obj.GetIntelCoverageRadius(), m_sFactionKey, reconFriendlyCount, enemyCount);
		// === END MODIFIED ===
		
		if (!m_mLastReconRevealTime.Contains(obj))
			m_mLastReconRevealTime.Insert(obj, worldTime);
		else
			m_mLastReconRevealTime.Set(obj, worldTime);
		
		if (enemyCount <= 0)
			return; // gak ada yang di-reveal, tapi timer tetep di-update (nyegah re-check tiap cycle)
		
		DCO_GroupUtilityComponent reconGrp = obj.GetReconGroup(m_sFactionKey);
		
		CMD_ContactReport report = new CMD_ContactReport(
			obj.GetOwner().GetOrigin(),
			enemyCount,
			worldTime,
			"RECON:" + obj.GetOwner().GetName());
		
		threatComp.ReceiveContactReport(report, reconGrp);
	}
	// === END ADDED ===
 
	protected void TrySendRecon(CMD_AICommanderObjectiveComponent obj)
	{
		DCO_GroupUtilityComponent reconGrp = FindBestIdleGroupForRole(CMD_EGroupRole.RECON, obj.GetOwner().GetOrigin());
		if (!reconGrp)
		{
			return;
		}
		
		if (reconGrp.IsPlayerGroup())
		{
			return;
		}
		
		// === ADDED: Manpower Budget gate ===
		if (!CanCommitGroup(reconGrp))
		{
			//Print(string.Format("[%1] Manpower budget insufficient — skip RECON to %2", m_sCommanderUID, obj.GetOwner().GetName()));
			return;
		}
		// === END ADDED ===
		
		reconGrp.CompleteAllWaypoints();
		
		float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
		
		RandomGenerator rand = new RandomGenerator();
		vector objPos    = rand.GenerateRandomPointInRadius(obj.GetRadius() / 5, obj.GetRadius(), obj.GetOwner().GetOrigin(), false);
		objPos[1]		 = GetGame().GetWorld().GetSurfaceY(objPos[0], objPos[2]);
				
	    vector reconPos = CMD_ReconSpotFinder.FindBestReconSpot(reconGrp.GetOwner().GetOrigin(), objPos, 300.0, 80.0, 16);
		if (reconPos == vector.Zero)
			return;
		
		if (TryAssignTransport(reconGrp, reconPos, worldTime))
    		return;
 
		SCR_AIWaypoint wp = SpawnMoveWP(reconPos);
		if (!wp)
			return;
 
		
		reconGrp.SetGroupRole(CMD_EGroupRole.RECON);
		reconGrp.MoveTo(wp, worldTime);
 		obj.SetReconGroup(m_sFactionKey, reconGrp);
		obj.MarkAssigned(m_sFactionKey);
	}
 
	// === ADDED: Frontline Recon ===
	//! Cari posisi "frontline" -- titik tengah antara base commander dan objective
	//! terdekat yang BUKAN milik kita (PENDING atau musuh). Proxy sederhana buat
	//! "kira-kira di mana kontak bakal kejadian duluan", tanpa perlu analisa posisi
	//! musuh yang kompleks (kita gak selalu punya info musuh yang reliable).
	//------------------------------------------------------------------------------------------------
	// === MODIFIED: Frontline ===
	//! Frontline dulu CUMA SATU TITIK: `(posisiCommander + objectiveMusuhTerdekat) * 0.5`.
	//! Tiga masalahnya:
	//!   1. cuma memakai SATU objective musuh -- yang terdekat. Musuh yang memegang tiga
	//!      objective tersebar tetap menghasilkan satu titik di dekat salah satunya;
	//!   2. objective milik kita cuma dipakai sebagai filter, tidak ikut membentuk apa pun,
	//!      padahal justru wilayah kita yang menentukan di mana perbatasannya;
	//!   3. separuh perhitungannya posisi commander, yang belum punya entity fisik.
	//!
	//! Sekarang frontline adalah GARIS -- rangkaian segmen. Bentuknya dua kasus:
	//!
	//!   Punya >= 2 objective: tiap objective kita dipasangkan dengan objective non-milik
	//!   terdekat, titik tengah pasangan jadi simpul. Simpul diurutkan menurut sudut
	//!   mengelilingi titik berat wilayah kita, lalu dihubungkan berurutan.
	//!
	//!   Punya 1 objective: satu simpul tidak membentuk garis, jadi dibuat BUSUR yang
	//!   memeluk objective itu -- apex di sisi menghadap ancaman, lengannya menarik balik
	//!   ke belakang. Itu selubung pertahanan satu titik pegangan, bukan garis pemisah.
	//!
	//!   Tidak punya objective sama sekali: tidak ada wilayah, tidak ada perbatasan.
	//!   Mengembalikan false, bukan mengarang titik dari koordinat commander.
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

		// === MODIFIED: dulu `if (own.IsEmpty() || hostile.IsEmpty()) return;` --
		// satu baris yang bikin frontline TIDAK PERNAH ada di awal skenario, karena
		// belum ada objective yang kita pegang. Justru di saat itulah garisnya paling
		// berguna dilihat: commander sudah punya pasukan dan sudah punya sasaran, cuma
		// belum punya wilayah.
		//
		// Sekarang kalau belum punya objective, wilayah diwakili titik berat grup kita
		// sendiri. Itu memang di mana kekuatan kita berada, dan perbatasannya tetap
		// berarti walaupun belum ada yang direbut.
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

	//! Titik berat grup yang kita miliki. Dipakai sebagai pengganti wilayah waktu
	//! commander belum memegang objective apa pun. Grup pemain dan transport khusus
	//! tidak ikut -- keduanya bukan cerminan posisi kekuatan yang commander kendalikan.
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

	//! Kasus >= 2 objective. Simpul = titik tengah pasangan (milik kita, ancaman
	//! terdekatnya). m_fFrontlineNodeBias menggeser simpul di sepanjang pasangan itu:
	//! 0.5 = tanah tak bertuan, di bawah 0.5 = merapat ke wilayah kita (postur bertahan),
	//! di atas 0.5 = merapat ke musuh (postur agresif).
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
			seg.m_vStart   = node;   // sementara: simpul disimpan di Start
			seg.m_Owned    = o;
			seg.m_Threat   = nearest;
			seg.m_fPressure = ComputeSegmentPressure(nearest, node);

			nodes.Insert(seg);
		}

		if (nodes.Count() < 2)
			return;

		SortNodesByAngle(nodes, centroid);

		// Simpul berurutan jadi segmen. Sengaja TIDAK ditutup jadi lingkaran --
		// perbatasan itu busur menghadap musuh, bukan cincin mengelilingi kita.
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

	//! Kasus 1 objective. Busur memeluk objective: apex menghadap ancaman terdekat,
	//! lengan menarik balik ke belakang. Bentangnya lebih dari setengah lingkaran
	//! supaya sisi sampingnya ikut tertutup -- yang terbuka cuma arah belakang.
	protected void BuildEnvelopeFrontline(
		CMD_AICommanderObjectiveComponent center,
		notnull array<CMD_AICommanderObjectiveComponent> hostile)
	{
		BuildEnvelopeAt(center.GetOwner().GetOrigin(), center.GetRadius(), center, hostile);
	}

	//! === MODIFIED: selubung dipisah dari objective supaya bisa dipakai dua kasus --
	//! memeluk objective yang kita pegang, ATAU memeluk titik berat grup waktu kita
	//! belum memegang apa pun. Bentuknya sama: apex menghadap ancaman terdekat, lengan
	//! terbuka ke belakang.
	//!
	//! anchorRadius 0 berarti tidak ada objective sebagai acuan; radiusnya diturunkan
	//! dari jarak ke ancaman supaya selubungnya duduk di depan kita, bukan menempel di
	//! musuh.
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
			// Tanpa objective acuan: pakai sebagian jarak ke ancaman, tapi jangan sampai
			// menempel di musuh -- disisakan 100 m supaya garisnya tetap terbaca sebagai
			// perbatasan kita, bukan lingkaran di sekitar objective mereka.
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

	//! Arah hadap segmen = menjauhi wilayah kita. Dipakai buat drift patroli supaya
	//! grup merangkak TEGAK LURUS ke perbatasan, bukan menyerong ke satu titik jauh.
	protected vector ComputeFacing(vector segCenter, vector rearRef)
	{
		vector away = segCenter - rearRef;
		away        = Vector(away[0], 0.0, away[2]);

		if (away.LengthSq() < 1.0)
			return Vector(1.0, 0.0, 0.0);

		return away.Normalized();
	}

	//! Tekanan = jumlah musuh di objective ancaman, dibagi jarak (dinormalisasi per
	//! 1000 m). Ini yang bikin recon pergi ke segmen paling panas duluan, bukan ke
	//! segmen pertama di daftar.
	protected float ComputeSegmentPressure(CMD_AICommanderObjectiveComponent threat, vector node)
	{
		if (!threat || !threat.GetOwner())
			return 0.0;

		// === MODIFIED: census objective ===
		int segFriendly;
		int segEnemies;
		threat.CountNearbyUnitsCached(threat.GetRadius(), m_sFactionKey, segFriendly, segEnemies);
		float enemies = segEnemies;
		// === END MODIFIED ===
		float dist    = Math.Max(vector.Distance(node, threat.GetOwner().GetOrigin()), 1.0);

		return enemies * (1000.0 / dist);
	}

	//! Urutkan simpul menurut sudut mengelilingi titik berat wilayah kita. Tanpa ini
	//! garisnya bisa zig-zag menyilang karena urutan objective di manager sembarang.
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

	//------------------------------------------------------------------------------------------------
	//! Titik acuan frontline tunggal: pusat segmen dengan tekanan tertinggi.
	//! Nama lama dipertahankan supaya pemanggil yang ada tidak perlu diubah semua.
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

	//! Titik terdekat PADA garis dari sebuah posisi, beserta arah hadap segmennya.
	//! Ini yang dipakai patroli dan drift supaya grup menyebar sepanjang perbatasan
	//! alih-alih menumpuk di satu titik.
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

	//! Segmen dengan tekanan tertinggi yang belum dipegang recon manapun. Dipakai buat
	//! menyebar recon sepanjang garis, bukan menumpuk semuanya di satu titik.
	protected bool GetFrontlineSegmentForRecon(out vector pos)
	{
		if (m_aFrontline.IsEmpty())
			return false;

		DCO_FrontlineSegment best = null;
		float bestScore = -1.0;

		foreach (DCO_FrontlineSegment seg : m_aFrontline)
		{
			vector c = seg.Center();

			// Segmen yang sudah ada recon-nya diturunkan, bukan dicoret -- kalau semua
			// segmen sudah terisi, recon berikutnya tetap dapat tempat.
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
	// === END MODIFIED ===
	
	//! Kirim grup RECON yang masih nganggur (gak kepake buat objective manapun) buat
	//! scouting ke arah frontline -- bukan buat objective spesifik, tapi buat nentuin
	//! dari arah mana musuh bakal dateng / ngasih early warning. Contact report udah
	//! otomatis jalan sendiri (tiap grup punya DCO_GroupContactReporterComponent),
	//! jadi cukup POSISIIN mereka di frontline, gak perlu logic laporan baru.
	protected void TrySendFrontlineRecon(float worldTime)
	{
		// === MODIFIED: dulu semua grup recon dikirim ke titik frontline yang SAMA --
		// layarnya menumpuk di satu tempat alih-alih melebar. Sekarang tiap grup dapat
		// segmen yang tekanannya paling tinggi DAN belum ada recon-nya.
		vector frontlinePos;
		if (!GetFrontlineSegmentForRecon(frontlinePos))
			return;
		// === END MODIFIED ===

		DCO_GroupUtilityComponent reconGrp = FindBestIdleGroupForRole(CMD_EGroupRole.RECON, frontlinePos);
		if (!reconGrp)
			return;
		
		if (reconGrp.IsPlayerGroup())
			return;
		
		if (!CanCommitGroup(reconGrp))
			return;
		
		reconGrp.CompleteAllWaypoints();
		reconGrp.SetGroupRole(CMD_EGroupRole.RECON);
		GeneratePatrolRoute(reconGrp, frontlinePos, m_fFrontlineReconRadius, worldTime);
		
		// === ADDED: BUG FIX -- catat tracking durasi, biar grup ini ke-release balik
		// ke pool nanti (lihat UpdateFrontlineReconTracks), gak permanen kesedot.
		CMD_FrontlineReconTrack track = new CMD_FrontlineReconTrack();
		track.m_Squad = reconGrp;
		track.m_fExpireTime = worldTime + m_fFrontlineReconDuration;
		m_aFrontlineReconTracks.Insert(track);
		// === END ADDED ===
		
		if (m_bDebugMode)
			Print(string.Format("[%1] Frontline Recon: %2 -> scouting deket %3",
				m_sCommanderUID, reconGrp.GetOwner().GetName(), frontlinePos.ToString()));
	}
	
	//! Lepas grup frontline recon yang udah expired balik ke RESERVE, biar pool
	//! grup buat assault/defend/flank gak abis kesedot permanen. Piggyback siklus
	//! ThinkCaptureProgress yang sama kayak Suppress Mission.
	protected void UpdateFrontlineReconTracks(float worldTime)
	{
		// === ADDED: dipanggil dari EOnFrame yang guard server-nya dikomentar. ===
		if (!Replication.IsServer())
			return;
		// === END ADDED ===

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
				if (track.m_Squad.GetGroupRole() == CMD_EGroupRole.RECON)
				{
					track.m_Squad.CompleteAllWaypoints();
					track.m_Squad.SetGroupRole(CMD_EGroupRole.RESERVE);
					if (m_bDebugMode)
						Print(string.Format("[%1] Frontline Recon selesai -- %2 dilepas balik ke RESERVE",
							m_sCommanderUID, track.m_Squad.GetOwner().GetName()));
				}
				m_aFrontlineReconTracks.Remove(i);
			}
		}
	}
	// === END ADDED ===
	
	
	// === ADDED: Retreat System -- helper reusable, extract dari logic yang tadinya
	// nempel di dalem SendIdleGroupsToReserve doang. Nyari posisi "aman" terdekat
	// (objective captured atau HQ fallback) dari suatu titik, divalidasi navmesh
	// reachability (sama pola kayak fix patrol center kemarin). Dipake buat patrol
	// candidate DAN rally point retreat -- 2 kebutuhan yang sama persis konsepnya:
	// "titik aman terdekat yang beneran bisa dipijak".
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
		// === END MODIFIED ===
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
	// === END ADDED ===
	
	// === ADDED: dipakai 2 tempat di SendIdleGroupsToReserve (kandidat captured
	// yang di-precompute, dan kandidat fallback per grup). Isinya persis logic lama.
	//! Makin dekat kandidat ke objective yang BELUM kita pegang, makin tinggi (0..1).
	//! 0.5 kalau semua objective sudah milik kita.
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
	// === END ADDED ===

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

	    // === MODIFIED: skor frontline tiap objective captured gak bergantung sama grup,
	    // jadi dihitung SEKALI di sini -- dulu diulang per grup idle
	    // (O(grup x captured x semua objective)). ===
	    array<float> capturedFrontlineScore = new array<float>();
	    foreach (vector capPos : capturedObjPositions)
	        capturedFrontlineScore.Insert(ComputeReserveFrontlineScore(capPos, mgr));
	    // === END MODIFIED ===

	    float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;

	    m_iPatrolSmartRemaining = m_iPatrolSmartBudget;
	
	    foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
	    {
	        if (!grp)
	            continue;
	
	        if (grp.GetGroupStatus() == DCOG_EGroupStatus.EXECUTING_COMMAND)
	            continue;

	        bool isNeverAssigned         = (grp.GetGroupRole() == CMD_EGroupRole.NONE);
	        bool isFinishedReservePatrol = (grp.GetGroupRole() == CMD_EGroupRole.RESERVE && grp.GetGroupStatus() == DCOG_EGroupStatus.IDLE);
	        
	        if (!isNeverAssigned && !isFinishedReservePatrol)
	            continue;
	        // === END MODIFIED ===
	
			
			if (!grp.CanItHaveOrder())
				continue;
	
	        // === MODIFIED: pakai list captured + skor yang udah dihitung di atas, tanpa
	        // copy per grup. Kandidat fallback (gak punya objective) dihitung di tempat. ===
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
	            // === END MODIFIED ===
	            
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
	        // === END ADDED ===

	        GeneratePatrolRoute(grp, patrolCenter, m_fBaseRadius, worldTime);
	        // === END MODIFIED ===

	        grp.SetGroupRole(CMD_EGroupRole.RESERVE);
	    }
	}
	
		protected DCO_GroupUtilityComponent FindBestIdleGroupForRole(CMD_EGroupRole role, vector targetPos, bool canTakeDefend = false, CMD_AICommanderObjectiveComponent targetObj = null)
	{
		// === ADDED: satu-satunya pintu akuisisi grup di seluruh commander, jadi ini
		// tempat yang benar buat batas jatah fase (dipakai BALANCED). -1 = tanpa batas.
		if (m_iPhaseBudget == 0)
			return null;
		// === END ADDED ===

		array<CMD_EGroupRole> tiers = {role};
		if (canTakeDefend)
			tiers = {role, CMD_EGroupRole.NONE, CMD_EGroupRole.RESERVE, CMD_EGroupRole.RECON, CMD_EGroupRole.REINFORNCE, CMD_EGroupRole.DEFEND};
		else
			tiers = {role, CMD_EGroupRole.NONE, CMD_EGroupRole.RESERVE, CMD_EGroupRole.RECON, CMD_EGroupRole.REINFORNCE};
		
		int tierCount = tiers.Count();
		if (role == CMD_EGroupRole.ARMORED)
			tierCount = 1;
		
		array<DCO_GroupUtilityComponent> bestPerTier   = {};
		array<float>                     bestScorePerTier  = {};
		array<float>                     bestDistSqPerTier = {};
		for (int t = 0; t < tierCount; t++)
		{
			bestPerTier.Insert(null);
			bestScorePerTier.Insert(-1.0);
			bestDistSqPerTier.Insert(-1.0);
		}
		
		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp)
				continue;

			bool isReservePatrol = (grp.GetGroupRole() == CMD_EGroupRole.RESERVE);
			if (grp.GetGroupStatus() == DCOG_EGroupStatus.EXECUTING_COMMAND && !isReservePatrol)
				continue;

			if (grp.IsDedicatedTransport())
				continue;

			if (!grp.CanCommanderOverrideRole())
				continue;

			if (!grp.CanItHaveOrder())
				continue;

			if (grp.IsPlayerGroup())
				continue;

			CMD_EGroupRole grpRole = grp.GetGroupRole();

			int tierIdx = -1;
			for (int t = 0; t < tierCount; t++)
			{
				if (tiers[t] == grpRole)
				{
					tierIdx = t;
					break;
				}
			}

			if (tierIdx < 0)
				continue;

			int unitCount = grp.GetUnitCount();
			float strengthPct = Math.Clamp(unitCount / 12.0 * 100.0, 0.0, 100.0);

			float score = 0.0;
			switch (role)
			{
				case CMD_EGroupRole.RECON:
					score = 100.0 - strengthPct;
					break;
				case CMD_EGroupRole.ASSAULT:
					score = strengthPct;
					break;
				case CMD_EGroupRole.FLANK:
					score = 100.0 - Math.AbsFloat(strengthPct - 50.0);
					break;
				default:
					score = strengthPct;
					break;
			}

			float distSq = vector.DistanceSq(grp.GetOwner().GetOrigin(), targetPos);

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

		DCO_GroupUtilityComponent preempted = TryFindPreemptableGroup(role, targetPos, targetObj, preemptTime);
		if (preempted)
		{
			if (m_iPhaseBudget > 0)
				m_iPhaseBudget = m_iPhaseBudget - 1;

			return preempted;
		}
		// === END ADDED ===

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
			oldObj.SetObjectiveGroup(m_sFactionKey, -1);

			// Objective lama balik PENDING kalau grup terakhirnya baru aja ditarik --
			// biar dia masuk antrean penilaian lagi, bukan nyangkut di ASSIGNED
			// dengan nol grup.
			if (oldObj.GetObjectiveState(m_sFactionKey) == CMD_EObjectiveState.ASSIGNED
				&& oldObj.GetCurrentAssignedGroupCount(m_sFactionKey) <= 0)
			{
				oldObj.SetObjectiveState(m_sFactionKey, CMD_EObjectiveState.PENDING);
			}
		}

		grp.CompleteAllWaypoints();
		grp.SetGroupObjective(null);
		grp.SetGroupRole(CMD_EGroupRole.RESERVE);

		m_mLastPreemptTime.Set(grp, worldTime);
	}

	protected DCO_GroupUtilityComponent TryFindPreemptableGroup(CMD_EGroupRole role, vector targetPos, CMD_AICommanderObjectiveComponent targetObj, float worldTime)
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

			// Gerbang kepemilikan yang sama persis dipakai FindBestIdleGroupForRole.
			if (grp.IsDedicatedTransport())
				continue;

			if (!grp.CanCommanderOverrideRole())
				continue;

			if (!grp.CanItHaveOrder())
				continue;

			CMD_EGroupRole grpRole = grp.GetGroupRole();

			if (DCO_PreemptionUtility.IsRoleHardProtected(grpRole))
				continue;

			// Lagi bertindak atas inisiatif sendiri = lagi kontak. Narik grup dari
			// tengah baku tembak bikin dia jalan sambil ditembakin.
			if (grp.GetGroupStatus() == DCOG_EGroupStatus.INITIATIVE)
				continue;

			CMD_AICommanderObjectiveComponent curObj = grp.GetGroupObjective();

			// Udah di objective yang sama -- gak ada gunanya dicomot.
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

			float taskValue = DCO_PreemptionUtility.ComputeTaskValue(curScore, grpRole);

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

	//! Berapa grup yang realistis masih bisa ditarik cycle ini. Filternya sengaja
	//! dibikin mirror FindBestIdleGroupForRole biar angkanya nyambung.
	protected int CountIdleCommittableGroups()
	{
		int count = 0;

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || grp.IsPlayerGroup())
				continue;

			if (grp.IsDedicatedTransport() || !grp.CanCommanderOverrideRole() || !grp.CanItHaveOrder())
				continue;

			bool isReservePatrol = (grp.GetGroupRole() == CMD_EGroupRole.RESERVE);
			if (grp.GetGroupStatus() == DCOG_EGroupStatus.EXECUTING_COMMAND && !isReservePatrol)
				continue;

			count = count + 1;
		}

		return count;
	}

	//! BALANCED yang sebenarnya. Yang lama cuma ngundi URUTAN panggil
	//! ThinkOffensive/ThinkDefensive -- karena dua-duanya narik dari kolam idle yang
	//! sama, yang jalan duluan nyaplok semua grup dan yang kedua cuma dapet sisa.
	//! Efeknya commander flip-flop tiap cycle antara "hampir semua nyerang" dan
	//! "hampir semua defend" tanpa alasan taktis. Sekarang defend dijatah DULU
	//! sebanyak yang benar-benar dia butuh (terbatas, bisa dihitung), sisanya baru
	//! bebas buat offense (rakus, gak ada batas alami).
	protected void ThinkBalanced(AICommander_ManagerComponent mgr, float worldTime)
	{
		int defendDemand = CountDefendDemand();
		int idleAvail    = CountIdleCommittableGroups();

		int defendBudget = 0;
		if (defendDemand > 0 && idleAvail > 0)
		{
			float share = Math.Lerp(m_fDefendShareMax, m_fDefendShareMin, m_fAggression);
			defendBudget = Math.Round(idleAvail * share);

			if (defendBudget > defendDemand)
				defendBudget = defendDemand;

			// Kalau memang ada lubang garnisun, defend selalu dapet minimal satu --
			// tanpa ini commander agresif dengan sedikit grup idle bisa dapet
			// pembulatan nol dan gak pernah nambal defense sama sekali.
			if (defendBudget < 1)
				defendBudget = 1;
		}

		m_iPhaseBudget = defendBudget;
		ThinkDefensive(worldTime);

		m_iPhaseBudget = -1;
		ThinkOffensive(mgr, worldTime);
	}
	// === END ADDED ===
	
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
	 
		// === MODIFIED: blok `if (unitCount <= m_iRetreatThreshold ...)` badannya KOSONG
		// -- m_iRetreatThreshold diskalain personality di InitializeCommander tapi gak
		// dipakai apa-apa. `continue` di baris terakhir juga no-op (posisinya di ujung
		// badan loop), CheckOrderComplete() cuma dipanggil buat efek sampingnya. Loop
		// ini sekarang menyatakan maksudnya apa adanya.
		// CATATAN: panggilan GetTotalManpower() yang kedua (dulu tepat di bawah loop
		// ini) sudah ikut hilang di Stage 1 -- nilainya gak berubah di antara dua
		// panggilan itu, jadi yang di atas loop sudah cukup.
		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp)
				continue;
			
			if (!grp.CanItHaveOrder())
				continue;
	 
			grp.CheckOrderComplete(worldTime);
		}
		// === END MODIFIED ===
	 
		// === MODIFIED: routing mode. Dulu di sini ada tiga masalah bertumpuk:
		//  1. gate EvaluateCommanderMode() bikin mode default (BALANCED) gak pernah
		//     nyampe ke m_eCommanderMode sama sekali;
		//  2. EnsureAbsoluteDefend() itu duplikat ThinkDefensive() -- di mode
		//     DEFENSIVE dua-duanya jalan di cycle yang sama;
		//  3. cabang BALANCED cuma ngundi urutan panggil, bukan bagi-bagi pasukan.
		// Sekarang: satu switch, satu jalur per mode, tanpa duplikasi.
		// === ADDED: garis frontline dibangun ulang sekali per cycle, SEBELUM mode
		// routing jalan -- patroli, drift, dan recon semuanya membacanya. ===
		BuildFrontline();
		// === END ADDED ===

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
		// === END MODIFIED ===
		
		TrySendFrontlineRecon(worldTime);
		
		SendIdleGroupsToReserve();
	 
		m_eCommanderState = CMD_ECommanderState.COMMANDING;
	}
	
	protected void ThinkOffensive(AICommander_ManagerComponent mgr, float worldTime)
	{
		mgr.GetTopObjectivesOffensive(this, m_fObjectiveAtTheSameTime, m_aObjective);
	 
		if (m_aObjective.IsEmpty())
		{
			m_eCommanderState = CMD_ECommanderState.IDLE;
			return;
		}
		
		// === ADDED: Optimasi -- context dibangun SEKALI, dipake bareng buat RiskTaking
		// gate di AssignRolesToObjective (bukan tiap objective query manager dari nol) ===
		CMD_ObjectiveContextCache contextCache = mgr.BuildObjectiveContext(m_sFactionKey);
		// === END ADDED ===
	 
		for (int i = 0; i < m_aObjective.Count(); i++)
		{
			CMD_AICommanderObjectiveComponent obj = m_aObjective[i];
			if (!obj)
				continue;
			
			// === MODIFIED: dulu di sini CheckAndMarkIfLost() dipanggil lalu hasilnya
			// LANGSUNG dibatalin ResetLostStatus() di baris berikutnya -- status lost
			// hidup satu baris, jadi satu-satunya efek nyata adalah `continue`-nya.
			// Efek sampingnya jelek: karena flag lost gak pernah bertahan,
			// CheckIsItLost() di ThinkDefensive dan di ReclaimStaleAssignments SELALU
			// false, bikin dua jalur itu mati. Kepemilikan status lost sekarang
			// dipegang sisi defensive; di sini cukup cek langsung tanpa mutasi.
			// === MODIFIED: satu panggilan census, bukan dua sphere query ===
			int friendlyNear;
			int enemyNear;
			obj.CountNearbyUnitsCached(obj.GetRadius(), m_sFactionKey, friendlyNear, enemyNear);
			// === END MODIFIED ===

			if (friendlyNear > 0 && enemyNear >= friendlyNear * 3)
				continue; // lagi kalah telak di situ -- jangan tambah komit cycle ini
			// === END MODIFIED ===

			AssignRolesToObjective(obj, worldTime, contextCache);
		}
		
		// === MODIFIED: log per-cycle cuma waktu debug ===
		if (m_bDebugMode)
			Print(string.Format("[%1] THINK OFFENSIVE", m_sCommanderUID));
		// === END MODIFIED ===
	}
	
	protected void ThinkDefensive(float worldTime)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;

		// === OPTIMIZED ===
		// Dulu manggil mgr.GetTopObjectives() yang trigger ComputePriorityScore()+QueryEntitiesBySphere
		// buat SEMUA objective di map, padahal urutan priority gak ngaruh di sini (loop di bawah gak
		// ada break, semua captured objective tetap diproses). Pakai mgr.m_aObjective langsung.
		array<CMD_AICommanderObjectiveComponent> allObjs = mgr.m_aObjective;
		// === END OPTIMIZED ===
	 
		bool hasAnyWork = false;
	 
		foreach (CMD_AICommanderObjectiveComponent obj : allObjs)
		{
			if (!obj)
				continue;
	 
			if (!obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
				continue;
			
			// === MODIFIED: dulu pakai CheckIsItLost() yang cuma MEMBACA map lost.
			// Satu-satunya penulis map itu ada di ThinkOffensive dan langsung di-reset,
			// jadi di mode DEFENSIVE nilainya selalu false -- commander defensive gak
			// punya jalur apapun buat nyadar dia kehilangan objective. Sekarang dia
			// yang MENGEVALUASI sendiri, dan hasilnya gak dibatalin.
			if (obj.CheckAndMarkIfLost(m_sFactionKey))
			{
				// Urutan penting: lepas grupnya DULU (CompleteAllWaypoints) sebelum
				// ResetAssignedGroupCount -- fungsi itu manggil ClearSectorGrid yang
				// ngehapus entity waypoint yang lagi dituju grup-grup ini.
				ReleaseGroupsFromObjective(obj);
				obj.ResetAssignedGroupCount(m_sFactionKey);
				obj.SetObjectiveState(m_sFactionKey, CMD_EObjectiveState.PENDING);
				continue;
			}
			// === END MODIFIED ===
			
	 
			AssignDefendToObjective(obj, worldTime);
			hasAnyWork = true;
		}
		
		// === MODIFIED: log per-cycle cuma waktu debug ===
		if (m_bDebugMode)
		{
			Print(hasAnyWork.ToString() + " < HAS DEFEND WORK FOR " + m_sCommanderUID + " " + m_sFactionKey);
			Print(string.Format("[%1] THINK DEFENSIVE", m_sCommanderUID));
		}
		// === END MODIFIED ===
		// === MODIFIED: SendIdleGroupsToReserve() dicabut dari sini -- sekarang dipanggil
		// terpusat 1x per Think() cycle di Think() sendiri, gak lagi gated hasAnyWork ===
	}
	
	protected void TrySendToStaging(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
	    vector objPos  = obj.GetOwner().GetOrigin();
	    vector base    = GetOwner().GetOrigin();
	
	    // === MODIFIED: titik kumpul dihitung sekali per objective lewat
	    // GetOrCreateStagingPos. `base` masih dipakai blok FLANK di bawah. ===
	    vector stagingPos = GetOrCreateStagingPos(obj);
	
	    DCO_GroupUtilityComponent assaultGrp = FindBestIdleGroupForRole(CMD_EGroupRole.ASSAULT, objPos);
	    if (assaultGrp)
	    {
			if (assaultGrp.IsPlayerGroup())
			{
				//CMD_TaskNotifier.Notify(assaultGrp.GetOwner(), "STAGING " + obj.GetOwner().GetName(), obj.GetOwner().GetOrigin(), CMD_ETaskType.MOVE);
				return;
			}
			// === ADDED: Manpower Budget gate ===
			if (!CanCommitGroup(assaultGrp))
			{
				//Print(string.Format("[%1] Manpower budget insufficient — skip ASSAULT staging to %2", m_sCommanderUID, obj.GetOwner().GetName()));
			}
			else
			{
				assaultGrp.CompleteAllWaypoints();
				if (TryAssignTransport(assaultGrp, stagingPos, worldTime))
	    			return;
		        // === MODIFIED: rute bertahap, bukan satu waypoint tujuan. ===
		        if (SpawnMoveRoute(assaultGrp, assaultGrp.GetOwner().GetOrigin(), stagingPos, worldTime))
		        {
		            assaultGrp.SetGroupRole(CMD_EGroupRole.ASSAULT);
		            if (assaultGrp.GetGroupObjective() != obj)
		            {
		                assaultGrp.SetGroupObjective(obj);
		                obj.SetObjectiveGroup(m_sFactionKey, 1);
		            }
		        }
			}
	    }
	
	    DCO_GroupUtilityComponent flankGrp = FindBestIdleGroupForRole(CMD_EGroupRole.FLANK, objPos);
	    if (flankGrp)
	    {
			if (flankGrp.IsPlayerGroup())
			{
				//CMD_TaskNotifier.Notify(flankGrp.GetOwner(), "STAGING " + obj.GetOwner().GetName(), obj.GetOwner().GetOrigin(), CMD_ETaskType.MOVE);
				return;
			}
			// === ADDED: Manpower Budget gate ===
			if (!CanCommitGroup(flankGrp))
			{
				//Print(string.Format("[%1] Manpower budget insufficient — skip FLANK staging to %2", m_sCommanderUID, obj.GetOwner().GetName()));
				return;
			}
			// === END ADDED ===
			flankGrp.CompleteAllWaypoints();
			if (TryAssignTransport(flankGrp, stagingPos, worldTime))
    			return;
	        vector flankStaging = ComputeFlankPosition(base, stagingPos, 150);
	        SCR_AIWaypoint wp   = SpawnMoveWP(flankStaging);
	        if (wp)
	        {
	            flankGrp.SetGroupRole(CMD_EGroupRole.FLANK);
	            flankGrp.MoveTo(wp, worldTime);
	            flankGrp.SetGroupObjective(obj);
	            //Print(string.Format("[%1] FLANK → STAGING: %2", m_sCommanderUID, flankGrp.GetOwner().GetName()));
	        }
	    }
	}
	
	//! Bikin rute sapuan di DALAM objective buat satu grup.
	//!
	//! === MODIFIED: tiga hal berubah.
	//!
	//! 1. angleSpan. Dulu tiap grup dikasih angleOffset yang beda, tapi loop sektornya
	//!    tetap `0 .. currentSectors-1` dengan sectorAngle = 360/currentSectors --
	//!    jadi SETIAP grup tetap menyapu 360 derajat penuh, cuma titik mulainya geser.
	//!    Komentar lamanya bilang tiap grup nyari di sektor berbeda; kenyataannya
	//!    liputannya rangkap sebanyak jumlah grup. Sekarang span-nya dilempar dari
	//!    pemanggil, jadi 3 grup beneran dapat 120 derajat masing-masing.
	//!
	//!    Jumlah sektor per cincin ikut diskala span, supaya kerapatan titik per
	//!    busur tetap sama -- bukan 21 titik dijejalkan ke 120 derajat.
	//!
	//! 2. Urutan cincin. Dulu: cincin terluar, lalu cincin tengah DIACAK, lalu cincin
	//!    terluar LAGI. Grup melompat masuk-keluar tanpa pola dan cincin termahal
	//!    (yang terluar, titiknya paling banyak) dikerjakan dua kali. Sekarang menyempit
	//!    dari luar ke dalam, sekali jalan. Variasinya tetap ada dari sudut acak per
	//!    sektor dan angleOffset per grup.
	//!
	//! 3. Cek air. Dulu cuma ngandelin SpawnMoveWP yang CUMA nolak WST_OCEAN -- danau
	//!    dan kolam di dalam objective tetap dapat waypoint. Sekarang semua tipe air
	//!    dilewati, sama seperti di GeneratePatrolRoute.
	protected void GenerateSearchWaypoints(vector center, float radius, array<SCR_AIWaypoint> outWaypoints, float wpSpacing = 50.0, float angleOffset = 0.0, float angleSpan = 360.0)
	{
	    if (!outWaypoints)
	        return;

	    outWaypoints.Clear();

	    if (angleSpan <= 0.0)
	        angleSpan = 360.0;

	    int budget = Math.Max(2, m_iMaxSearchWaypoints);

	    int rings = Math.Max(1, (int)Math.Round(radius / wpSpacing));

	    // === ADDED: batas jumlah waypoint per grup.
	    // Cincin dibatasi dulu SEBELUM jatah dibagi. Kalau enggak, objective besar
	    // menghasilkan 5 cincin yang masing-masing cuma kebagian 3 titik -- bentuk
	    // sapuannya jadi cincin-cincin tipis yang tidak berarti apa-apa. Lebih baik
	    // cincin sedikit tapi tiap cincin terisi layak.
	    int maxRings = Math.Max(1, budget / 3);
	    if (rings > maxRings)
	        rings = maxRings;
	    // === END ADDED ===

	    int baseSectorsPerRing = Math.Max(2, (int)Math.Round((radius * 2 * Math.PI) / (wpSpacing * 1.5)));

	    float ringStep  = radius / rings;
	    float spanRatio = angleSpan / 360.0;

	    // === ADDED: hitung jatah ideal tiap cincin dulu, baru dikecilkan proporsional
	    // kalau totalnya lewat batas. Dengan cara ini bentuk sapuannya tetap sama --
	    // cincin luar tetap lebih rapat dari cincin dalam -- cuma seluruhnya jadi lebih
	    // renggang. Kalau dipotong begitu saja di tengah jalan, cincin dalam tidak akan
	    // pernah kebagian sama sekali karena urutannya dari luar ke dalam.
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
	    // === END ADDED ===

	    int ringIndex = 0;

	    // Dari luar ke dalam: kepung dulu, baru menyempit. Ini juga yang bikin jalur
	    // tempuhnya jauh lebih pendek daripada urutan acak.
	    for (int ring = rings; ring >= 1; ring--)
	    {
	        float radiusInner = ringStep * (ring - 1);
	        float radiusOuter = ringStep * ring;

	        int currentSectors = sectorsPerRing.Get(ringIndex);
	        ringIndex          = ringIndex + 1;

	        float sectorAngle = angleSpan / currentSectors;

	        for (int sector = 0; sector < currentSectors; sector++)
	        {
	            // Pengaman terakhir: pembulatan ke atas per cincin bisa bikin totalnya
	            // lewat sedikit dari batas.
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
	// === END MODIFIED ===
	
	// === ADDED: rute bertahap.
	//! Kirim grup dari `from` ke `to` lewat rantai waypoint tiap m_fWaypointLegDistance
	//! meter, bukan satu waypoint tujuan yang jauh.
	//!
	//! Kenapa: satu waypoint jauh bikin AI narik garis lurus ke tujuan dan nembus
	//! apapun yang ada di antaranya. Dengan titik antara, tiap potong perjalanan bisa
	//! digeser ke tempat yang lebih masuk akal -- keluar dari air, keluar dari radius
	//! objective musuh yang kebetulan kelewatan.
	//!
	//! Titik antara yang gak ketemu tempat aman DILEWATI, bukan bikin seluruh rute
	//! gagal. Rutenya jadi lebih kasar di bagian itu, tapi grup tetap jalan.
	//!
	//! Return true kalau waypoint TUJUAN berhasil dipasang -- itu yang menentukan
	//! order-nya sah atau enggak. Titik antara sifatnya penyempurnaan.
	bool SpawnMoveRoute(DCO_GroupUtilityComponent grp, vector from, vector to, float worldTime)
	{
		if (!grp)
			return false;

		float total = vector.Distance(from, to);

		if (m_fWaypointLegDistance > 0.0 && total > m_fWaypointLegDistance)
		{
			int legs = Math.Floor(total / m_fWaypointLegDistance);

			// Potongan terakhir digabung ke tujuan -- kalau enggak, ada waypoint
			// antara yang nempel banget sama tujuan dan grup keliatan berhenti dua
			// kali di tempat yang sama.
			for (int i = 1; i < legs; i++)
			{
				float t = i / (float)legs;

				vector leg = from + ((to - from) * t);

				vector safeLeg;
				if (!FindSafeRoutePoint(leg, to, safeLeg))
					continue;

				SCR_AIWaypoint legWp = SpawnMoveWP(safeLeg);
				if (legWp)
					grp.MoveTo(legWp, worldTime);
			}
		}

		SCR_AIWaypoint destWp = SpawnMoveWP(to);
		if (!destWp)
			return false;

		grp.MoveTo(destWp, worldTime);
		return true;
	}

	//! Cari titik yang bisa dilewati di sekitar `candidate`. Diuji titik aslinya dulu,
	//! baru geseran tegak lurus ke kiri/kanan dengan jarak nambah bertahap.
	//!
	//! Geserannya tegak lurus terhadap arah jalan supaya rutenya melebar ke samping
	//! (muter halangan), bukan maju-mundur di sumbu perjalanan.
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
				continue; // step 0 cuma sekali, gak perlu diuji dua sisi
		}

		return false;
	}

	//! Titik dianggap bisa dilewati kalau bukan air apapun dan gak di dalam radius
	//! objective yang bukan milik kita.
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
	// === END ADDED ===

	SCR_AIWaypoint SpawnMoveWP(vector pos, EMovementType moveType = EMovementType.RUN)
	{
	    AICommander_BaseComponentClass data = AICommander_BaseComponentClass.Cast(GetComponentData(GetOwner()));
	    if (!data)
	        return null;
	
	    Resource res = Resource.Load(data.GetDefaultMoveWaypointPrefab());
	    if (!res || !res.IsValid())
	        return null;
	
	    // --- Snap ke surface & validasi bukan air ---
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
		SCR_AIGroupCharactersMovementSpeedSetting mspeed = SCR_AIGroupCharactersMovementSpeedSetting.Create(SCR_EAISettingOrigin.BEHAVIOR, moveType);
		wp.AddSetting(mspeed);
		
		if (SCR_WorldTools.IsObjectUnderwater(wp))
			return null;
	
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
	        stalemateObj.ResetAssignedGroupCount(m_sFactionKey);
	        TrySendAssaultWithSlots(stalemateObj, worldTime);
	        return;
	    }
	
	    foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
	    {
	        if (!grp)
	            continue;
	
	        if (grp.GetGroupObjective() != stalemateObj)
	            continue;
	
	        if (grp.GetGroupStatus() == DCOG_EGroupStatus.EXECUTING_COMMAND)
	            continue;
	
	        grp.SetGroupObjective(altObj);
	        grp.SetGroupRole(CMD_EGroupRole.ASSAULT);
	
	        RandomGenerator rand = new RandomGenerator();
	        vector altPos = rand.GenerateRandomPointInRadius(5, altObj.GetRadius(), altObj.GetOwner().GetOrigin(), false);
	        altPos[1] = GetGame().GetWorld().GetSurfaceY(altPos[0], altPos[2]);
	
	        if (!TryAssignTransport(grp, altPos, worldTime))
	        {
	            SCR_AIWaypoint wp = SpawnMoveWP(altPos);
	            if (wp)
	                grp.MoveTo(wp, worldTime);
	        }
	
	        stalemateObj.SetObjectiveGroup(m_sFactionKey, -1);
	        altObj.SetObjectiveGroup(m_sFactionKey, 1);
	
	        break;
	    }
	}

	// === ADDED: lepas semua grup yang masih nempel ke sebuah objective balik ke
	// RESERVE. Dipakai waktu objective hilang: tanpa ini commander berhenti ngirim
	// orang baru, tapi yang udah di sana tetap nyangkut di objective yang bukan
	// punya kita lagi -- gak defend, gak nyerang.
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
			grp.SetGroupRole(CMD_EGroupRole.RESERVE);
		}
	}
	// === END ADDED ===

	protected void ReclaimStaleAssignments(float worldTime)
	{
		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || grp.IsPlayerGroup())
				continue;

			if (!grp.CanCommanderOverrideRole() || grp.IsDedicatedTransport())
				continue;

			CMD_EGroupRole role = grp.GetGroupRole();

			// === MODIFIED: DEFEND dulu di-skip tanpa syarat, jadi grup garnisun di
			// objective yang udah direbut musuh nyangkut selamanya. Sekarang DEFEND
			// ikut diproses -- tapi definisi "basi"-nya KEBALIK dari role lain (lihat
			// di bawah): buat DEFEND, objective yang masih milik kita justru berarti
			// grupnya lagi kerja dengan benar.
			if (role == CMD_EGroupRole.NONE
			 || role == CMD_EGroupRole.RESERVE
			 || role == CMD_EGroupRole.TRANSPORT
			 || role == CMD_EGroupRole.RETREAT)
				continue;
			// === END MODIFIED ===

			// Masih jalan / masih punya order -- jangan diganggu.
			if (grp.GetGroupStatus() != DCOG_EGroupStatus.IDLE)
				continue;

			if (grp.IsGroupHaveWaypoint())
				continue;

			CMD_AICommanderObjectiveComponent obj = grp.GetGroupObjective();

			// === MODIFIED: definisi "basi" dipisah per role.
			// Role ofensif: objective yang UDAH kita rebut = kerjaan selesai -> basi.
			// Role DEFEND: kebalikannya -- objective yang masih milik kita = grupnya
			// lagi menjalankan tugas. Yang basi justru kalau objective-nya udah BUKAN
			// punya kita lagi.
			bool stale;
			if (role == CMD_EGroupRole.DEFEND)
				stale = !obj || !obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID);
			else
				stale = !obj
					|| obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID)
					|| obj.CheckIsItLost(m_sFactionKey);

			if (!stale)
				continue;

			// DEFEND gak pernah NAMBAH counter (AssignGroupToSector cuma nyetel
			// sector), jadi dia juga gak boleh NGURANGI -- itu bikin counter drift
			// ke bawah dan commander over-commit.
			if (obj && role != CMD_EGroupRole.DEFEND && obj.GetCurrentAssignedGroupCount(m_sFactionKey) > 0)
				obj.SetObjectiveGroup(m_sFactionKey, -1);
			// === END MODIFIED ===

			grp.SetGroupObjective(null);
			grp.SetGroupRole(CMD_EGroupRole.RESERVE);
		}
	}
	
	// === REMOVED: EnsureAbsoluteDefend() -- duplikat persis ThinkDefensive():
	// loop mgr.m_aObjective, filter IsCapturedBy, panggil AssignDefendToObjective.
	// Bedanya cuma ThinkDefensive juga nangani status lost. Dulu keduanya jalan di
	// cycle yang sama waktu mode DEFENSIVE.
	// === END REMOVED ===

	protected float GetSectorPersonalityMod()
	{
		if (!m_bScaleSectorByPersonality)
			return 1.0;

		float mod = Math.Lerp(m_fSectorPersonalityMax, m_fSectorPersonalityMin, m_fAggression);

		mod = mod * Math.Lerp(0.95, 1.10, m_fResilience);

		return mod;
	}

	//------------------------------------------------------------------------------------------------
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

	//------------------------------------------------------------------------------------------------
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

	//------------------------------------------------------------------------------------------------
	// === ADDED: growth loop garnisun manggil FindBestIdleGroupForRole(RESERVE, ...),
	// dan tier fallback fungsi itu mencakup RECON. Akibatnya garnisun bisa nyomot grup
	// yang lagi ditugasin frontline recon. Waktu track-nya expired,
	// UpdateFrontlineReconTracks ngecek role == RECON, ketemu DEFEND, jadi gak
	// di-release -- sementara grupnya udah terlanjur ditarik dari frontline.
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
	// === END ADDED ===

	protected void AssignGroupToSector(DCO_GroupUtilityComponent grp, CMD_AICommanderObjectiveComponent obj, DCO_SectorGarrison sec, float worldTime)
	{
		grp.CompleteAllWaypoints();
		grp.SetGroupRole(CMD_EGroupRole.DEFEND);

		grp.SetGroupObjective(obj);

		grp.MoveTo(sec.m_Waypoint, worldTime);
		sec.m_Group = grp;
	}

	//------------------------------------------------------------------------------------------------
	protected void AssignDefendToObjective(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		if (!obj || !obj.GetOwner())
			return;

		float radius  = obj.GetRadius();
		if (radius <= 0.0)
			return;

		vector objPos = obj.GetOwner().GetOrigin();

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

			DCO_GroupUtilityComponent grp = FindBestIdleGroupForRole(CMD_EGroupRole.RESERVE, sec.m_vPosition);

			// === MODIFIED: dulu satu kondisi `if (!grp || grp.IsPlayerGroup()) break;`.
			// Kalau kandidat terbaik kebetulan player group, SELURUH loop replenish
			// berhenti -- sector lain yang juga butuh isi ulang gak kesentuh sama
			// sekali cycle itu. Dua kasus itu beda: "gak ada grup sama sekali" memang
			// alasan berhenti, "kandidat ini gak cocok" enggak.
			if (!grp)
				break;

			if (grp.IsPlayerGroup() || IsFrontlineReconGroup(grp))
				continue;
			// === END MODIFIED ===

			AssignGroupToSector(grp, obj, sec, worldTime);
		}

		// === ADDED: growth loop dulu jalan sampai SEMUA sector keisi -- ukuran
		// garnisun sepenuhnya ditentukan m_iMinSector/m_iMaxSector. Atribut
		// "Defend Group Count" di objective (GetDefendGroupCount) nol pemanggil di
		// seluruh repo, jadi mission maker yang nyetel angka itu gak dapet efek apapun.
		// Sekarang jumlah sector jadi BENTUK garnisun, DefendGroupCount jadi PLAFONnya.
		int garrisonCap = obj.GetDefendGroupCount();
		if (garrisonCap <= 0)
			garrisonCap = 1;

		int staffedNow = obj.GetStaffedSectorCount(m_sFactionKey);
		// === END ADDED ===

		while (staffedNow < garrisonCap)
		{
			DCO_SectorGarrison target = PickNextSector(sectors, sectorCount, sectorOffset, threatAngle);
			if (!target)
				break;

			DCO_GroupUtilityComponent grp = FindBestIdleGroupForRole(CMD_EGroupRole.RESERVE, objPos);
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
	
	//------------------------------------------------------------------------------------------------
	// === MODIFIED: Patrol Route ===
	//! Rute patroli dulu SELALU lingkaran: 4-6 titik dibagi rata di sekitar pusat,
	//! jitter radius +-15%, sudut awal acak. Tiga hal yang bikin gampang ditebak:
	//!   1. cuma ada satu generator -- strukturnya identik tiap kali,
	//!   2. titiknya murni geometri (cos/sin), gak pernah lihat medan,
	//!   3. pusatnya nempel di anchor, jadi grup muter di area yang sama terus.
	//!
	//! Sekarang: empat pola, pusat yang merangkak ke depan, titik yang dipilih dari
	//! medan, ingatan supaya gak balik ke tempat yang sama, dan ukuran yang ikut
	//! kekuatan grup.
	protected void GeneratePatrolRoute(DCO_GroupUtilityComponent grp, vector center, float radius, float worldTime)
	{
		if (!grp)
			return;

		// --- #6: ukuran ikut kekuatan grup ---
		// Grup 8 orang nyisir lebih luas, grup 3 orang lebih rapat. Dulu semuanya
		// dapat radius sama, jadi grup kecil kelihatan kewalahan nutup area besar.
		float patrolRadius = radius * 1.6 * PatrolRadiusMultiplier(grp);

		// --- #2: pusat merangkak ke arah frontline ---
		vector patrolCenter = ApplyFrontlineDrift(center);

		// --- #4: hindari area yang baru saja dipatroli grup ini ---
		patrolCenter = AvoidRecentPatrolCenters(grp, patrolCenter, patrolRadius);
		RememberPatrolCenter(grp, patrolCenter);

		// Arah ancaman dipakai dua kali: buat milih pola, dan buat nentuin ke mana
		// titik-titiknya harus "menghadap" waktu dinilai medannya.
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

		foreach (vector raw : points)
		{
			vector p = raw;
			p[1] = GetGame().GetWorld().GetSurfaceY(p[0], p[2]);

			// --- #3: geser ke tempat yang lebih masuk akal secara medan ---
			if (hasThreat)
				p = RefinePatrolPoint(p, lookAt);

			// Air jenis apapun dilewati. Ini yang dulu udah ada dan tetap dipertahanin --
			// SpawnMoveWP sendiri cuma nolak WST_OCEAN, danau tetap lolos.
			EWaterSurfaceType waterType = EWaterSurfaceType.WST_NONE;
			float lakeArea = 0;
			float waterY   = SCR_WorldTools.GetWaterSurfaceY(null, p, waterType, lakeArea);

			if (p[1] < waterY && waterType != EWaterSurfaceType.WST_NONE)
				continue;

			SCR_AIWaypoint wp = SpawnMoveWP(p);
			if (wp)
				grp.MoveTo(wp, worldTime);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! #6 -- 0.6x buat grup kecil sampai 1.6x buat grup penuh (12 orang).
	protected float PatrolRadiusMultiplier(DCO_GroupUtilityComponent grp)
	{
		float units    = grp.GetUnitCount();
		float strength = Math.Clamp(units / 12.0, 0.0, 1.0);

		return Math.Lerp(m_fPatrolRadiusMulMin, m_fPatrolRadiusMulMax, strength);
	}

	//------------------------------------------------------------------------------------------------
	//! #2 -- geser pusat patroli ke arah frontline sejauh m_fPatrolFrontlineDrift.
	//!
	//! Remnya penting: cadangan yang merangkak terus akhirnya nyampe ke objective
	//! musuh sendirian dan mati konyol. Kalau pusat hasil geseran udah lebih dekat
	//! dari m_fPatrolFrontlineStandoff ke objective musuh terdekat, geserannya
	//! dibatalin -- grup nunggu di jarak aman, bukan nyelonong.
	protected vector ApplyFrontlineDrift(vector center)
	{
		if (m_fPatrolFrontlineDrift <= 0.0)
			return center;

		// === MODIFIED: arah geser sekarang NORMAL segmen terdekat, bukan garis lurus
		// menuju satu titik acuan. Grup merangkak tegak lurus ke perbatasan -- kalau
		// pakai titik, grup yang ada di ujung garis akan menyerong jauh ke tengah. ===
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
				return center; // kedeketan -- batalin geseran cycle ini
		}

		return drifted;
	}

	//------------------------------------------------------------------------------------------------
	//! #4 -- ingatan pusat patroli. Kalau pusat baru jatuh dekat salah satu pusat
	//! yang baru dipakai, dia diputar ke sisi lain. Ini yang menghapus kesan grup
	//! balik ke tempat yang sama terus-terusan.
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

			// Putar ke sekitar titik semula dengan jarak sedikit lebih jauh dari
			// radius ingatan, supaya sekali putar biasanya udah cukup.
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

	//------------------------------------------------------------------------------------------------
	//! #1 -- pemilihan pola. Bukan acak murni: konteks yang nentuin peluangnya.
	//!
	//! Tanpa arah ancaman yang diketahui, ARC dan ADVANCE gak ada artinya -- dua-duanya
	//! butuh tahu ke mana harus menghadap. Jadi kalau frontline belum ada, cuma RING
	//! dan LANE yang mungkin.
	protected int PickPatrolPattern(DCO_GroupUtilityComponent grp, bool hasThreat)
	{
		if (!hasThreat)
		{
			if (Math.RandomFloat01() < 0.5)
				return DCO_EPatrolPattern.RING;

			return DCO_EPatrolPattern.LANE;
		}

		// Eagerness menggeser bobot: commander agresif lebih sering ngirim cadangan
		// yang bergerak maju, commander hati-hati lebih sering nyuruh nutup busur
		// menghadap ancaman.
		float roll = Math.RandomFloat01();

		if (roll < m_fAggression * 0.5)
			return DCO_EPatrolPattern.ADVANCE;

		if (roll < 0.5 + (m_fAggression * 0.2))
			return DCO_EPatrolPattern.ARC;

		if (roll < 0.8)
			return DCO_EPatrolPattern.LANE;

		return DCO_EPatrolPattern.RING;
	}

	//------------------------------------------------------------------------------------------------
	//! Pola lama: lingkaran penuh 4-6 titik. Dipertahankan sebagai salah satu pilihan,
	//! bukan satu-satunya.
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

	//! Jalur bolak-balik antara dua ujung. Sumbunya TEGAK LURUS arah ancaman kalau
	//! diketahui -- jadi grup menyapu melintang di depan ancaman, bukan mondar-mandir
	//! mendekat lalu menjauh.
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
			// t bergerak dari -1 ke +1 sepanjang sumbu; sisi kiri-kanan diselang-seling
			// supaya jalurnya zig-zag, bukan garis lurus.
			float t = (i / (float)Math.Max(legs - 1, 1)) * 2.0 - 1.0;

			float lateral = radius * 0.35;
			if (i % 2 == 1)
				lateral = -lateral;

			outPoints.Insert(center + (axis * (t * length)) + (side * lateral));
		}
	}

	//! Busur menghadap ancaman. Cuma menutupi 120-160 derajat ke arah lookAt --
	//! sisi belakang gak dipatroli karena memang bukan dari sana datangnya.
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

	//! Rute yang secara netto MAJU ke arah ancaman, dengan goyangan menyamping supaya
	//! gak jadi garis lurus. Ini yang bikin cadangan kelihatan bergerak ke depan,
	//! bukan menunggu di tempat.
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

	//------------------------------------------------------------------------------------------------
	//! #3 -- geser titik ke tempat yang lebih masuk akal secara medan.
	//!
	//! Beberapa kandidat di sekitar titik asli dinilai pakai keunggulan ketinggian dan
	//! garis pandang ke arah ancaman -- pola yang sama dengan CMD_ReconSpotFinder.
	//! Fungsi itu sendiri GAK dipakai langsung karena dia nyari di titik tengah antara
	//! pengamat dan target; buat patroli kita mau nyari di sekitar titik yang udah ada.
	//!
	//! Raycast itu mahal, jadi ada jatah per Think cycle (m_iPatrolSmartBudget) yang
	//! dibagi ke semua grup idle. Begitu habis, sisanya pakai titik geometri apa
	//! adanya -- lebih baik sebagian grup dapat titik bagus daripada semua grup bikin
	//! frame drop.
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

	//! Ketinggian 40%, garis pandang 60%. Titik tinggi yang gak bisa lihat apa-apa
	//! gak berguna buat patroli, tapi titik rendah dengan pandangan bagus masih oke.
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
	// === END MODIFIED ===
	
	protected void AssignPatrolAroundObjective(DCO_GroupUtilityComponent grp, vector center, float radius, float worldTime)
	{
		GeneratePatrolRoute(grp, center, radius, worldTime);
		grp.SetGroupRole(CMD_EGroupRole.DEFEND);
	}
	// === END MODIFIED ===
	
	// === ADDED: Objective-Link Patrol ===
	//! Patroli ANTAR objective yang udah captured dan saling berdekatan (bukan cuma
	//! muter di 1 objective doang) -- lebih natural buat area yang punya beberapa
	//! captured objective berdekatan (garis depan yang udah stabil), dibanding tiap
	//! grup defend cuma muter sendiri-sendiri di objective masing-masing.
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
		
		// Gak ada objective lain yang deket buat di-link -- fallback ke perimeter patrol biasa
		if (linkPoints.Count() < 2)
		{
			AssignPatrolAroundObjective(grp, homePos, homeObj.GetRadius(), worldTime);
			return;
		}
		
		// Shuffle biar urutan rute gak selalu sama
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
			vector patrolPos = rand.GenerateRandomPointInRadius(0, homeObj.GetRadius() * 0.8, p, false);
			patrolPos[1] = GetGame().GetWorld().GetSurfaceY(patrolPos[0], patrolPos[2]);
			
			SCR_AIWaypoint wp = SpawnMoveWP(patrolPos);
			if (wp)
				grp.MoveTo(wp, worldTime);
		}
		
		grp.SetGroupRole(CMD_EGroupRole.DEFEND);
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
			
			if (grp.GetGroupRole() == CMD_EGroupRole.SUPPRESS && grp.GetGroupObjective() == obj)
				return true;
		}
		return false;
	}

	protected void TryGatherForSynchronizedAssault(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		vector objPos = obj.GetOwner().GetOrigin();

		// === MODIFIED: titik kumpul yang SAMA dengan yang dipakai TrySendToStaging.
		// Sebelumnya dua fungsi ini ngitung sendiri-sendiri dengan random terpisah. ===
		vector stagingPos = GetOrCreateStagingPos(obj);

		int required = obj.GetRequiredGroupCount();

		while (obj.GetCurrentAssignedGroupCount(m_sFactionKey) < required)
		{
			DCO_GroupUtilityComponent assaultGrp = FindBestIdleGroupForRole(CMD_EGroupRole.ASSAULT, objPos);
			if (!assaultGrp)
				break; // gak ada grup available lagi -- coba lagi cycle Think() berikutnya

			if (assaultGrp.IsPlayerGroup())
				break; // jangan otomatis narik grup pemain ke staging

			if (!CanCommitGroup(assaultGrp))
				break; // manpower budget gak cukup -- stop gathering buat cycle ini

			assaultGrp.CompleteAllWaypoints();

			if (TryAssignTransport(assaultGrp, stagingPos, worldTime))
			{
				assaultGrp.SetGroupRole(CMD_EGroupRole.ASSAULT);
				if (assaultGrp.GetGroupObjective() != obj)
				{
					assaultGrp.SetGroupObjective(obj);
					obj.SetObjectiveGroup(m_sFactionKey, 1);
				}
				continue;
			}

			SCR_AIWaypoint wp = SpawnMoveWP(stagingPos);
			if (!wp)
				break;

			assaultGrp.SetGroupRole(CMD_EGroupRole.ASSAULT);
			assaultGrp.MoveTo(wp, worldTime);
			if (assaultGrp.GetGroupObjective() != obj)
			{
				assaultGrp.SetGroupObjective(obj);
				obj.SetObjectiveGroup(m_sFactionKey, 1);
			}
		}
	}
	
	protected bool AreStagedGroupsArrived(CMD_AICommanderObjectiveComponent obj)
	{
		int found = 0;

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || grp.GetGroupObjective() != obj)
				continue;

			if (grp.GetGroupRole() != CMD_EGroupRole.ASSAULT)
				continue;

			found++;

			if (grp.GetGroupStatus() == DCOG_EGroupStatus.EXECUTING_COMMAND)
				return false;
		}

		return found > 0;
	}

	protected void ReleaseSynchronizedAssault(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		// === ADDED: fase ngumpul selesai, titik kumpulnya udah gak relevan. Ini juga
		// yang ngasih lifetime buat m_mStagingPos supaya gak numpuk selamanya. ===
		ClearStagingPos(obj);
		// === END ADDED ===

		int releasedCount = 0;

		array<DCO_GroupUtilityComponent> toRelease = {};
		array<DCO_GroupUtilityComponent> toFlank   = {};

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || grp.GetGroupObjective() != obj)
				continue;

			if (grp.GetGroupRole() == CMD_EGroupRole.ASSAULT)
				toRelease.Insert(grp);
			else if (grp.GetGroupRole() == CMD_EGroupRole.FLANK)
				toFlank.Insert(grp);
		}

		// === MODIFIED: busur dibagi ke SEMUA grup yang masuk, assault maupun flank.
		//
		// Dulu cuma grup ASSAULT yang dapat jatah busur (dan itupun tidak berfungsi --
		// lihat GenerateSearchWaypoints), sementara grup FLANK cuma dapat SATU waypoint
		// di radius x 1.2 -- di LUAR objective -- dan tidak ada apapun yang menyuruhnya
		// masuk setelah itu. Jadi di mode sync, flank parkir di luar selamanya.
		//
		// Anehnya di jalur non-sync (TrySendAssaultWithSlots) flank justru dapat tiga
		// kaki menyusur sumbu lalu masuk ke dalam. Dua jalur, dua perilaku berbeda
		// untuk role yang sama.
		//
		// Sekarang keduanya masuk dan sama-sama dapat busur sendiri. Bedanya cuma cara
		// mendekat: assault lurus, flank lewat titik pendekatan menyamping dulu.
		int totalEntering = toRelease.Count() + toFlank.Count();
		if (totalEntering <= 0)
			return;

		float sectorSize = 360.0 / totalEntering;
		vector objPos    = obj.GetOwner().GetOrigin();
		vector base      = GetOwner().GetOrigin();

		for (int gi = 0; gi < toRelease.Count(); gi++)
		{
			DCO_GroupUtilityComponent grp = toRelease[gi];
			grp.CompleteAllWaypoints();

			array<SCR_AIWaypoint> searchWPs = {};
			GenerateSearchWaypoints(objPos, obj.GetRadius(), searchWPs, 50.0, sectorSize * gi, sectorSize);

			if (searchWPs.IsEmpty())
				continue;

			grp.MoveToRoute(searchWPs, worldTime);

			releasedCount++;
		}

		for (int fi = 0; fi < toFlank.Count(); fi++)
		{
			DCO_GroupUtilityComponent fgrp = toFlank[fi];
			fgrp.CompleteAllWaypoints();

			// Indeks busurnya melanjutkan indeks assault, jadi flank tidak menimpa
			// sektor yang sudah dipegang grup assault.
			int   arcIndex = toRelease.Count() + fi;
			float arcStart = sectorSize * arcIndex;

			// Titik pendekatan di luar radius, di sisi busur yang bakal dia sapu.
			// Ini yang bikin flank terlihat memutar dulu, bukan ikut masuk dari arah
			// yang sama dengan assault.
			float approachAngle = (arcStart + sectorSize * 0.5) * Math.DEG2RAD;
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
			GenerateSearchWaypoints(objPos, obj.GetRadius(), flankWPs, 50.0, arcStart, sectorSize);

			if (flankWPs.IsEmpty())
				continue;

			fgrp.MoveToRoute(flankWPs, worldTime);

			releasedCount++;
		}
		// === END MODIFIED ===
	}
	
	protected void TrySendAssaultWithSlots(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		if (obj.IsGroupSlotFull(m_sFactionKey))
		{
			//Print(string.Format("[%1] [%5] Objective %2 slot penuh (%3 required) Terisi : %4",
				//m_sCommanderUID, obj.GetOwner().GetName(), obj.GetRequiredGroupCount(), obj.GetCurrentAssignedGroupCount(m_sFactionKey), m_sFactionKey));
			return;
		}
		
		RandomGenerator rand = new RandomGenerator();
 
		vector objPos    = rand.GenerateRandomPointInRadius(5, obj.GetRadius(), obj.GetOwner().GetOrigin(), false);
		objPos[1]		 = GetGame().GetWorld().GetSurfaceY(objPos[0], objPos[2]);
		
		float objRad	 = obj.GetRadius();
		int required     = obj.GetRequiredGroupCount();
		int slotsLeft    = required - obj.GetCurrentAssignedGroupCount(m_sFactionKey);

		if (required >= 2 && !ObjectiveHasSuppressGroup(obj))
		{
			DCO_GroupUtilityComponent suppressGrp = FindBestIdleGroupForRole(CMD_EGroupRole.SUPPRESS, objPos, false, obj);
			if (suppressGrp && !suppressGrp.IsPlayerGroup() && CanCommitGroup(suppressGrp))
			{
				vector suppressPos = CMD_ReconSpotFinder.FindBestReconSpot(objPos, objPos, 180.0, objRad * 1.5, 12);
				if (suppressPos != vector.Zero)
				{
					suppressGrp.CompleteAllWaypoints();
					SCR_AIWaypoint suppressWp = SpawnMoveWP(suppressPos);
					if (suppressWp)
					{
						suppressGrp.SetGroupRole(CMD_EGroupRole.SUPPRESS);
						suppressGrp.MoveTo(suppressWp, worldTime);
						if (suppressGrp.GetGroupObjective() != obj)
							suppressGrp.SetGroupObjective(obj);
						
						//Print(string.Format("[%1] SUPPRESS → %2: %3", m_sCommanderUID, obj.GetOwner().GetName(), suppressGrp.GetOwner().GetName()));
					}
				}
			}
		}
 
		if (slotsLeft > 0)
		{
			DCO_GroupUtilityComponent assaultGrp = FindBestIdleGroupForRole(CMD_EGroupRole.ASSAULT, objPos, false, obj);
			if (assaultGrp)
			{
				if (assaultGrp.IsPlayerGroup())
				{
					return;
				}
				if (!CanCommitGroup(assaultGrp))
				{
					//Print(string.Format("[%1] Manpower budget insufficient — skip ASSAULT slot di %2", m_sCommanderUID, obj.GetOwner().GetName()));
				}
				else
				{
					assaultGrp.CompleteAllWaypoints();
					if (TryAssignTransport(assaultGrp, objPos, worldTime))
	    				return;
					// === MODIFIED: offset lama `360/6 * assignedGroupCount` punya dua
					// masalah. Angka 6 dihardcode dan tidak ada hubungannya dengan jumlah
					// grup yang sebenarnya masuk; dan waktu hitungannya persis 6, offset
					// jadi 360 derajat alias sama saja dengan nol.
					//
					// Sekarang busurnya dibagi berdasarkan berapa grup yang MEMANG
					// dibutuhkan objective ini, dan grup ini mengambil irisan berikutnya
					// yang belum terpakai.
					array<SCR_AIWaypoint> searchWPs = {};

					int   arcCount = Math.Max(1, obj.GetRequiredGroupCount());
					int   arcIndex = obj.GetCurrentAssignedGroupCount(m_sFactionKey) % arcCount;
					float arcSpan  = 360.0 / arcCount;

					GenerateSearchWaypoints(obj.GetOwner().GetOrigin(), obj.GetRadius(), searchWPs, 50.0, arcSpan * arcIndex, arcSpan);
					// === END MODIFIED ===
					if (searchWPs.Count() > 0)
					{
						assaultGrp.MoveToRoute(searchWPs, worldTime);
						
						assaultGrp.SetGroupRole(CMD_EGroupRole.ASSAULT);
						if (assaultGrp.GetGroupObjective() != obj)
						{
							assaultGrp.SetGroupObjective(obj);
							obj.SetObjectiveGroup(m_sFactionKey, 1);
							slotsLeft = slotsLeft - 1;
						}
						
						objPos    		 = rand.GenerateRandomPointInRadius(5, obj.GetRadius(), obj.GetOwner().GetOrigin(), false);
						objPos[1]		 = GetGame().GetWorld().GetSurfaceY(objPos[0], objPos[2]);
					}
				}
			}
		}
 
		if (slotsLeft > 0)
		{
			DCO_GroupUtilityComponent flankGrp = FindBestIdleGroupForRole(CMD_EGroupRole.FLANK, objPos);
			if (flankGrp)
			{
				if (flankGrp.IsPlayerGroup())
				{
					//CMD_TaskNotifier.Notify(flankGrp.GetOwner(), "FLANK " + obj.GetOwner().GetName(), obj.GetOwner().GetOrigin(), CMD_ETaskType.DESTROY);
					return;
				}
				// === ADDED: Manpower Budget gate ===
				if (!CanCommitGroup(flankGrp))
				{
					//Print(string.Format("[%1] Manpower budget insufficient — skip FLANK slot di %2", m_sCommanderUID, obj.GetOwner().GetName()));
					return;
				}
				// === END ADDED ===
				flankGrp.CompleteAllWaypoints();
				if (TryAssignTransport(flankGrp, objPos, worldTime))
    				return;
				
				for(int wpNum = 1; wpNum < 4; wpNum++)
				{
				    float axisRatio = 0.40 + (wpNum - 1) * 0.25;
				
				    float sideOffset = 80.0 - (wpNum - 1) * 25.0;
				
				    vector axis    = objPos - GetOwner().GetOrigin();
				    axis           = Vector(axis[0], 0.0, axis[2]);
				    axis           = axis.Normalized();
				    float totalDist = vector.Distance(GetOwner().GetOrigin(), objPos);
				
				    vector alongPoint = GetOwner().GetOrigin() + axis * (totalDist * axisRatio);
				    vector flankPos   = ComputeFlankPosition(alongPoint, objPos, sideOffset);
				
				    SCR_AIWaypoint wp = SpawnMoveWP(flankPos);
				    if (wp)
				    {
				        flankGrp.SetGroupRole(CMD_EGroupRole.FLANK);
				        flankGrp.MoveTo(wp, worldTime);
				
				        /*Print(string.Format("[%1] FLANK WP %2/3 (ratio: %3 offset: %4m): %5",
				            m_sCommanderUID,
				            wpNum,
				            axisRatio.ToString(),
				            sideOffset.ToString(),
				            flankGrp.GetOwner().GetName()));*/
				    }
				}
				
				SCR_AIWaypoint Objwp = SpawnMoveWP(objPos);
				flankGrp.MoveTo(Objwp, worldTime);
				
				if (flankGrp.GetGroupObjective() != obj)
				{
					flankGrp.SetGroupObjective(obj);
					obj.SetObjectiveGroup(m_sFactionKey, 1);
					slotsLeft = slotsLeft - 1;
				}
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// === ADDED: Commander Debug ===
	//! Overlay milik commander. Isinya CUMA hal yang commander ini yang tau:
	//! mode, personality, budget, tuning, layar frontline, dan daftar objective yang
	//! lagi digarap beserta skor dan rank-nya.
	//!
	//! Yang GAK di sini: isi objective (kontrol, presence, slot) digambar objective-nya
	//! sendiri; tally global digambar manager; role dan status grup digambar grupnya.
	//! Gak ada snapshot yang nyalin state komponen lain ke sini -- tiap komponen baca
	//! dirinya sendiri, live.
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

		m_aDebugShapes.Insert(Shape.CreateSphere(DCO_DebugDraw.COLOR_COMMANDER, flags, p, DCO_DebugDraw.MARKER_BIG));

		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(Vector(p[0], p[1] + 46.0, p[2]), BuildDebugHeader(now),   22.0, DCO_DebugDraw.COLOR_COMMANDER));
		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(Vector(p[0], p[1] + 37.0, p[2]), BuildDebugForce(),       17.0, DCO_DebugDraw.COLOR_COMMANDER));
		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(Vector(p[0], p[1] + 27.0, p[2]), BuildDebugPersonality(), 17.0, DCO_DebugDraw.COLOR_COMMANDER));
		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(Vector(p[0], p[1] + 16.0, p[2]), BuildDebugBudget(),      17.0, DCO_DebugDraw.COLOR_COMMANDER));
		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(Vector(p[0], p[1] +  4.0, p[2]), BuildDebugTuning(),      16.0, DCO_DebugDraw.COLOR_COMMANDER));

		DrawDebugObjectiveLinks(p, flags, now);
		DrawDebugFrontline(p, flags);
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
			"FORCE\nmanpower %1   reserve %2   floor %3   budgeting %4\ngroups %5   idle/committable %6\nobjectives worked %7 of %8 max   garrison needed %9",
			GetTotalManpower(),
			GetReserveManpower(),
			Math.Round(GetReserveFloor()),
			m_bEnableManpowerBudget,
			m_aOwnedGroup.Count(),
			CountIdleCommittableGroups(),
			m_aObjective.Count(),
			m_fObjectiveAtTheSameTime,
			defendDemand);
	}

	protected string BuildDebugPersonality()
	{
		return string.Format(
			"PERSONALITY (0..1, drives the gates below)\naggression %1 -- staging vs recon roll, defend share\nrisk %2 -- commit into fog     patience %3 -- stalemate wait\nresilience %4 -- retreat depth  adaptability %5 -- think rate\ncombat focus %6 -- tier threshold, reserve floor mod",
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
			"BUDGET\nreserve floor = total %1 x pct %2 x focusMod %3",
			GetTotalManpower(),
			m_fReserveMinimumPct,
			Math.Lerp(1.5, 0.5, m_fCombatFocus));

		if (m_eCommanderMode != CMD_ECommanderMode.BALANCED)
			return head + "\ndefend share n/a -- only BALANCED splits the idle pool";

		int   idle  = CountIdleCommittableGroups();
		float share = Math.Lerp(m_fDefendShareMax, m_fDefendShareMin, m_fAggression);

		return head + string.Format(
			"\ndefend share %1 = lerp(%2, %3) by aggression\n-> up to %4 of %5 idle groups reserved for defend",
			share, m_fDefendShareMax, m_fDefendShareMin,
			Math.Round(idle * share), idle);
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

	//! Panah ke objective yang lagi digarap, plus rank dan skor. Rank dan skor itu
	//! fakta COMMANDER tentang objective -- dihitung dari posisi dan personality
	//! commander ini -- jadi tempatnya di sini, bukan di overlay objective.
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

			// Titik kumpul: penanda kecil, jaraknya ditulis. Bukan bola sebesar area.
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

	//! Layar penyaring frontline: titik kecil + radius DITULIS, bukan bola seukuran
	//! radiusnya yang nutupin layar.
	//! === MODIFIED: dulu cuma satu bola kecil di titik frontline. Sekarang seluruh
	//! garisnya digambar -- itu yang bikin kamu bisa menilai apakah commander paham
	//! bentuk perbatasannya atau tidak. Ruas digambar sebagai rantai bola kecil karena
	//! Shape.CreateSphere satu-satunya primitif yang sudah kebukti jalan di codebase
	//! ini; panah dipakai buat arah hadap, yang memang butuh ujung runcing.
	protected void DrawDebugFrontline(vector cmdPos, int flags)
	{
		// === MODIFIED: dulu langsung return kalau kosong -- garis yang tidak muncul
		// tidak bisa dibedakan dari sistem yang mati. Sekarang alasannya ditulis. ===
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
		// === END MODIFIED ===

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

			// Arah hadap: panah pendek dari tengah ruas.
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
	// === END ADDED ===
	protected void ThinkCaptureProgress(float worldTime)
	{
	    // === ADDED: Think() punya guard server di baris atasnya, tapi fungsi ini
	    // enggak -- dan guard di EOnFrame dikomentar. Akibatnya di multiplayer TIAP
	    // CLIENT ngejalanin capture timer, SetCapturedBy(), dan
	    // ResetAssignedGroupCount() secara lokal.
	    if (!Replication.IsServer())
	        return;
	    // === END ADDED ===

	    // === MODIFIED: dulu iterate m_aObjective (milik commander) yang CUMA diisi
	    // ThinkOffensive(). Di mode DEFENSIVE array itu kosong selamanya, jadi capture
	    // progress mati total. Sekarang baca daftar objective dari manager, sama kayak
	    // ThinkDefensive.
	    AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
	    if (!mgr)
	        return;

	    foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
	    // === END MODIFIED ===
	    {
	        if (!obj)
	            continue;
	
	        CMD_EObjectiveState state = obj.GetObjectiveState(m_sFactionKey);

	        // === MODIFIED: seluruh badan loop dirombak. Empat hal yang berubah:
	        //
	        // 1. `state` dulu dibaca SEBELUM SetObjectiveState(ASSIGNED) ditulis, jadi
	        //    paksaannya baru kerasa cycle berikutnya. Sekarang local ikut di-update.
	        //
	        // 2. Paksaan ke ASSIGNED itu dulu gak dijaga apa-apa. Sejak loop ini pindah
	        //    ke mgr.m_aObjective (biar capture jalan di mode DEFENSIVE), dia berlaku
	        //    ke SEMUA objective di map -- objective yang gak digarap siapa-siapa bisa
	        //    keflip ke ASSIGNED cuma karena musuh lagi lebih banyak di situ. Sekarang
	        //    dibatasi ke objective yang memang punya grup terkomit dari kita.
	        //
	        // 3. Spatial query dulu dipanggil 4x per objective per tick (2 buat cek
	        //    paksaan, 2 lagi buat cek start timer). Sekarang sekali, hasilnya dipakai
	        //    bareng.
	        //
	        // 4. Penilaian dipindah ke objective (AssessObjective). Objective yang
	        //    nentuin sendiri dia CONTESTED atau CAPTURING dari isi radius-nya --
	        //    commander gak lagi nyimpulin itu dari fase assault yang lagi jalan.
	        if (state == CMD_EObjectiveState.COMPLETED || state == CMD_EObjectiveState.FAILED)
	            continue;

	        if (obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
	            continue;

	        // === MODIFIED: satu panggilan census, bukan dua sphere query ===
	        int friendlyNear;
	        int enemyNear;
	        obj.CountNearbyUnitsCached(obj.GetRadius(), m_sFactionKey, friendlyNear, enemyNear);
	        // === END MODIFIED ===

	        if (friendlyNear < enemyNear && obj.GetCurrentAssignedGroupCount(m_sFactionKey) > 0)
	        {
	            obj.SetObjectiveState(m_sFactionKey, CMD_EObjectiveState.ASSIGNED);
	            state = CMD_EObjectiveState.ASSIGNED;
	        }

	        if (state != CMD_EObjectiveState.ASSIGNED)
	            continue;

	        if (!obj.IsCaptureTimerRunning(m_sFactionKey))
	        {
	            // Syaratnya sekarang murni kondisi lapangan: kita hadir dan gak kalah
	            // jumlah. Dulu juga nuntut GetCurrentAssignedGroupCount() > 0 -- padahal
	            // counter itu bisa bocor (grup mati tanpa ngurangin slot), jadi objective
	            // bisa gagal mulai capture walaupun pasukannya jelas-jelas berdiri di
	            // sana. Yang ada di tanah lebih dipercaya daripada pembukuan.
	            if (friendlyNear > 0 && friendlyNear >= enemyNear)
	                obj.StartCaptureTimer(m_sFactionKey, worldTime);

	            continue;
	        }

	        obj.AssessObjective(m_sFactionKey, worldTime);

	        if (obj.IsStalemate(m_sFactionKey, worldTime))
	            HandleStalemateObjective(obj, worldTime);

	        if (obj.IsCaptureTimerComplete(m_sFactionKey, worldTime))
	        {
	            obj.SetCapturedBy(m_sFactionKey, true);
	            obj.ResetAssignedGroupCount(m_sFactionKey);
	            obj.ResetStalemateTracking();
	        }
	        // === END MODIFIED ===
	    }
	}
	
	protected IEntity TryAssignTransports(DCO_GroupUtilityComponent passengerGroup)
	{
		if (!passengerGroup)
			return null;
	 
		int unitCount = passengerGroup.GetUnitCount();
		if (unitCount <= 0)
			return null;
		
		// === ADDED: Vehicle Ownership fast-path ===
		// Kalau grup ini udah punya vehicle sendiri, langsung pake itu lagi -- gak perlu
		// search dari nol tiap kali mau transport. Cek dulu vehicle-nya masih hidup &
		// lagi gak dipake orang lain sebelum dipake ulang.
		IEntity ownedVeh = passengerGroup.GetOwnedVehicle();
		if (ownedVeh)
		{
			DamageManagerComponent dmg = DamageManagerComponent.Cast(ownedVeh.FindComponent(DamageManagerComponent));
			bool vehicleDestroyed = dmg && dmg.GetState() == EDamageState.DESTROYED;
	
			DCO_TransportMissionComponent ownedMission = DCO_TransportMissionComponent.Cast(ownedVeh.FindComponent(DCO_TransportMissionComponent));
	
			if (!vehicleDestroyed && ownedMission && !ownedMission.IsActiveVehicle())
				return ownedVeh;
	
			if (vehicleDestroyed || !ownedMission)
			{
				// Vehicle hancur / component ilang -- lepas klaim biar bisa cari pengganti.
				if (ownedMission)
					ownedMission.ReleaseOwnership();
				passengerGroup.SetOwnedVehicle(null);
			}
		}
		// === END ADDED ===
	 
		vector groupPos = passengerGroup.GetOwner().GetOrigin();
		IEntity vehicle = null;
		
		for(int i = 0; i < m_aVehicle.Count(); i++)
		{
			vehicle = CMD_VehicleFinder.FindNearestVehicle(m_aVehicle[i], groupPos, unitCount, passengerGroup);
			if (vehicle) break;
		}
	 
		if (!vehicle)
		{
			return null;
		}
		
		return vehicle;
	}
	
	array<vector> GenerateArtilleryImpactPoints(vector center, float range, float dispersion, float accuracy, float numberOfShell = 3)
	{
	    array<vector> impactPoints = new array<vector>();
	    RandomGenerator rand = new RandomGenerator();
	
	    float effectiveDispersion = dispersion * (1.0 - Math.Clamp(accuracy, 0.0, 1.0));
	    float minRadius           = effectiveDispersion * 0.1;
	
	    for (int i = 0; i < numberOfShell; i++)
	    {
	        float r1     = rand.RandFloatXY(minRadius, effectiveDispersion);
	        float r2     = rand.RandFloatXY(minRadius, effectiveDispersion);
	        float radius = (r1 + r2) * 0.5;
	
	        float angleDeg = rand.RandFloatXY(0.0, 360.0);
	        float angleRad = angleDeg * Math.DEG2RAD;
	
	        float px = center[0] + Math.Cos(angleRad) * radius;
	        float pz = center[2] + Math.Sin(angleRad) * radius;
	        float py = GetGame().GetWorld().GetSurfaceY(px, pz);
	
	        impactPoints.Insert(Vector(px, py, pz));
	    }
	
	    return impactPoints;
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
			//Print(string.Format("[%1] Vehicle %2 tidak punya DCO_TransportMissionComponent — skip", m_sCommanderUID, vehicle.GetName()), LogLevel.WARNING);
			return;
		}
	 
		if (mission.IsActiveVehicle())
		{
			//Print(string.Format("[%1] Vehicle %2 sudah dipakai transport lain", m_sCommanderUID, vehicle.GetName()));
			return;
		}
	 
		// === ADDED: Vehicle Ownership ===
		// Klaim vehicle ini buat grup ini secara permanen (bukan cuma buat 1 trip),
		// jadi trip berikutnya grup ini langsung pake vehicle yang sama, gak search ulang.
		mission.ClaimOwnership(passengerGroup);
		passengerGroup.SetOwnedVehicle(vehicle);
		// === END ADDED ===
	 
		passengerGroup.SetGroupRole(CMD_EGroupRole.TRANSPORT);
		
		SCR_AIGroup grp = SCR_AIGroup.Cast(passengerGroup.GetOwner());
		if (!grp)
			return;
		
		grp.CompleteAllWaypoints();		
	 
		// Kirim group jalan kaki ke vehicle dulu
		SCR_AIWaypoint wpToVehicle = SpawnMoveWP(vehicle.GetOrigin());
		if (wpToVehicle)
			passengerGroup.MoveTo(wpToVehicle, worldTime);
	 
		// Pass `this` supaya mission bisa spawn GetIn/GetOut WP
		mission.StartMission(passengerGroup, destination, m_sFactionKey, worldTime, this);
	 
		/*Print(string.Format("[%1] TRANSPORT assigned | group: %2 | vehicle: %3 | dest: %4",
			m_sCommanderUID,
			passengerGroup.GetOwner().GetName(),
			vehicle.GetName(),
			destination.ToString()));*/
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
	
	DCO_GroupUtilityComponent FindBestIdleGroupForRole_Public(CMD_EGroupRole role, vector pos)
	{
	    return FindBestIdleGroupForRole(role, pos);
	}
	
	void RegisterTransportTeam(DCO_TransportTeamComponent team)
	{
		if (!team || m_aTransportTeams.Contains(team))
			return;
	 
		team.SetRallyPoint(GetOwner().GetOrigin());
		m_aTransportTeams.Insert(team);
	 
		//Print(string.Format("[%1] Transport team registered: %2", m_sCommanderUID, team.GetOwner().GetName()));
	}
	
	bool TryAssignTransport(DCO_GroupUtilityComponent passengerGroup, vector destination, float worldTime)
	{
		if (!passengerGroup)
			return false;
	 
		float dist = vector.Distance(passengerGroup.GetOwner().GetOrigin(), destination);
		if (dist < m_fTransportDistanceThreshold)
			return false;
	 
		int unitCount = passengerGroup.GetUnitCount();
		if (unitCount <= 0)
			return false;
	 
		// === ADDED: Dedicated Transport Team dispatch ===
		// Dulu blok ini di-comment total, jadi dedicated team yang udah ke-register di
		// m_aTransportTeams gak pernah dipanggil -- commander selalu fallback ke general
		// vehicle pool. Sekarang: cek dulu ada dedicated team yang available, pakai kalau ada.
		// (Pakai FindAvailableTransportTeam() yang baru, BUKAN GetAvailableTransportTeam()
		// lama -- itu ada bug null-return + logic yang nyampur vehicle-pool-search buat
		// grup transport-nya sendiri, jangan dipakai.)
		DCO_TransportTeamComponent dedicatedTeam = FindAvailableTransportTeam();
		if (dedicatedTeam)
		{
			dedicatedTeam.AssignJob(passengerGroup, destination, this, worldTime);
			if (m_bDebugMode)
				Print(string.Format("[%1] TRANSPORT via dedicated team: %2 carrying %3",
					m_sCommanderUID,
					dedicatedTeam.GetOwner().GetName(),
					passengerGroup.GetOwner().GetName()));
			return true;
		}
		// === END ADDED ===
	 
		// Fallback: cari vehicle biasa
		vector groupPos = passengerGroup.GetOwner().GetOrigin();
		IEntity vehicle = TryAssignTransports(passengerGroup);
	 
		if (!vehicle)
        	return false;
		
		BeginTransportMission(passengerGroup, vehicle, destination, worldTime);
		return true;
	}
	
	// === ADDED: Dedicated Transport Team dispatch ===
	// Cari dedicated transport team pertama yang lagi available (belum ada job).
	// Sengaja dipisah dari GetAvailableTransportTeam() lama karena itu punya bug
	// (null-return di akhir walau berhasil, dan null-deref kalau vehicle gak ketemu).
	protected DCO_TransportTeamComponent FindAvailableTransportTeam()
	{
		foreach (DCO_TransportTeamComponent team : m_aTransportTeams)
		{
			if (team && team.IsAvailable())
				return team;
		}
		return null;
	}
	// === END ADDED ===
	 
	DCO_TransportTeamComponent GetAvailableTransportTeam(vector TakeAt, float wt)
	{
		IEntity vehicle = null;
		DCO_TransportTeamComponent t = null;
		foreach (DCO_TransportTeamComponent team : m_aTransportTeams)
		{
			if (team && team.IsAvailable())
			{
				vehicle = TryAssignTransports(team.GetDCOGroupUtility());
				t = team;
				break;
			}
		}
		DCO_TransportMissionComponent mission = DCO_TransportMissionComponent.Cast(vehicle.FindComponent(DCO_TransportMissionComponent));
	 
		if (!mission)
		{
			//Print(string.Format("[%1] Vehicle %2 tidak punya DCO_TransportMissionComponent — skip", m_sCommanderUID, vehicle.GetName()), LogLevel.WARNING);
			return null;
		}
	 
		if (mission.IsActiveVehicle())
		{
			//Print(string.Format("[%1] Vehicle %2 sudah dipakai transport lain", m_sCommanderUID, vehicle.GetName()));
			return null;
		}
		

		
		SCR_AIWaypoint wpToVehicle = SpawnMoveWP(vehicle.GetOrigin());
		if (wpToVehicle)
			t.GetDCOGroupUtility().MoveTo(wpToVehicle, wt);
		
		mission.StartMission(t.GetDCOGroupUtility(), TakeAt, m_sFactionKey, wt, this);
		
		return null;
	}
	
	void ComputeArtillerySpreadAndDispersion(vector center, float range, float dispersion, float accuracy, float numberOfShell = 3)
	{
	    RandomGenerator rand = new RandomGenerator();
	    
	    float effectiveDispersion = dispersion * (1.0 - Math.Clamp(accuracy, 0.0, 1.0));
	    float minRadius           = effectiveDispersion * 0.1;
	    
	    for (int i = 0; i < numberOfShell; i++)
	    {
	        float r1 = rand.RandFloatXY(minRadius, effectiveDispersion);
	        float r2 = rand.RandFloatXY(minRadius, effectiveDispersion);
	        float radius = (r1 + r2) * 0.5;
	        
	        float angleDeg = rand.RandFloatXY(0.0, 360.0);
	        float angleRad = angleDeg * Math.DEG2RAD;
	        
	        float px = center[0] + Math.Cos(angleRad) * radius;
	        float pz = center[2] + Math.Sin(angleRad) * radius;
	        float py = GetGame().GetWorld().GetSurfaceY(px, pz);
	        
	        vector shellImpact = Vector(px, py, pz);
	        
	        if (m_bDebugMode)
	            Print(string.Format("[Artillery] Shell %1 impact at %2 (radius: %3m from center)",
	                i + 1, shellImpact.ToString(), radius.ToString()));
	        
	        // TODO: spawn explosion / effect di shellImpact
	    }
	}
	
	
	
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		//if (!Replication.IsServer())
		//	return;
 
		 m_fThinkTimer += timeSlice;
		 m_fCaptureCheckTimer += timeSlice;

		// === ADDED: sengaja DI LUAR guard server -- shape dirender lokal. Di
		// hosted/single-player mesinnya sama jadi kelihatan. ===
		UpdateCommanderDebug(timeSlice);
		// === END ADDED ===

		if (m_fThinkTimer >= m_fThinkInterval)
		{
			m_fThinkTimer = 0.0;
			float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
			Think(worldTime);
		}
		
		if (m_fCaptureCheckTimer >= m_fCaptureCheckInterval)
		{
		    m_fCaptureCheckTimer = 0.0;
		    ThinkCaptureProgress(GetGame().GetWorld().GetWorldTime() / 1000.0);
		    
		    // === REMOVED: Order Squad Suppress -- di-nonaktifin sementara bareng
		    // trigger-nya (di ReleaseSynchronizedAssault), sambil investigasi waypoint
		    // leak. Dikomentar dulu, gampang diaktifin lagi nanti.
		    //UpdateSuppressMissions(GetGame().GetWorld().GetWorldTime() / 1000.0);
		    // === END REMOVED ===
		    
		    // === ADDED: Frontline Recon -- piggyback siklus yang sama ===
		    UpdateFrontlineReconTracks(GetGame().GetWorld().GetWorldTime() / 1000.0);
		    // === END ADDED ===
		}
		
		//Print(string.Format("[%1] < Think Timer | > Think Interval [%2] | [%3] < Commander ", m_fThinkTimer, m_fThinkInterval, m_sCommanderUID));
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
}