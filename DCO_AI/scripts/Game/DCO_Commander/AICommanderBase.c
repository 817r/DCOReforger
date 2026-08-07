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
	
	[Attribute("{6ED320498A60081C}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_ArtillerySupport.et", UIWidgets.ResourceNamePicker, desc: "Waypoint To Suppress Area.", "et", category: "Commander Player Tasking Setting")]
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

class AICommander_BaseComponent : ScriptComponent
{
	[Attribute("", UIWidgets.Font, desc: "UID of the Commander.", category: "Commander General Setting")]
	protected string m_sCommanderUID;
	
	[Attribute("", UIWidgets.Auto, desc: "Faction Key of the Commander.", category: "Commander General Setting")]
	protected FactionKey m_sFactionKey;	
	
	[Attribute("3.0", UIWidgets.Auto, "Number of the Objective Can be processed at the same time", category: "Commander Objective Setting")]
	protected int m_fObjectiveAtTheSameTime;
	
	// === ADDED: Synchronized Attack ===
	[Attribute("1", UIWidgets.CheckBox, "Pake Synchronized Attack (grup ngumpul dulu di staging sampe full/timeout, baru nyerang bareng) -- kalau dimatiin, balik ke behavior lama (grup langsung ke objective satu-satu begitu dapet slot).", category: "Commander Objective Setting")]
	protected bool m_bUseSynchronizedAttack;
	
	[Attribute("60.0", UIWidgets.EditBox, "Waktu maksimum (detik) nunggu slot ASSAULT penuh sebelum synchronized attack di-release paksa dengan grup yang udah ngumpul (walau belum full).", category: "Commander Objective Setting")]
	protected float m_fSyncAttackMaxWaitTime;
	// === END ADDED ===
	
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
	
	[Attribute("400.0", UIWidgets.EditBox, "Jarak minimum sebelum cari transport", category: "Commander Setting")]
	protected float m_fTransportDistanceThreshold;
	
	[Attribute("50.0", UIWidgets.EditBox, "Radius pencarian kendaraan", category: "Commander Setting")]
	protected float m_fVehicleSearchRadius;
	
	[Attribute("50.0", UIWidgets.EditBox, "Radius Close To Commander", category: "Commander Setting")]
	protected float m_fBaseRadius;
	
	[Attribute("0.6", UIWidgets.Range, "Defend Chances Instead Patrol Around", params: "0 1 0.01", category: "Commander Setting")]
	protected float m_fDefendChance;
	
	// === ADDED: Manpower Budget System ===
	[Attribute("1", UIWidgets.CheckBox, desc: "Aktifkan Manpower Budgeting? Kalau false, commander bisa komit semua grup ke offense tanpa reserve (behavior lama).", category: "Commander Manpower Budget")]
	protected bool m_bEnableManpowerBudget;
	
	[Attribute("0.2", UIWidgets.Range, "Minimum persentase total manpower yang WAJIB tetap jadi reserve (tidak dikomit ke offense). 0.2 = 20% pasukan selalu disisain.", params: "0 1 0.01", category: "Commander Manpower Budget")]
	protected float m_fReserveMinimumPct;
	// === END ADDED ===
	
	// === ADDED: Absolute Defend & Key Position Evaluation ===
	[Attribute("1", UIWidgets.CheckBox, desc: "Kalau true, defend group dievaluasi posisinya (dataran tinggi + arah hadap musuh) bukan random murni.", category: "Commander Defend Setting")]
	protected bool m_bEnableKeyDefendSpotEvaluation;
	
	[Attribute("2", UIWidgets.EditBox, "Maksimum jumlah defend group per objective yang dikirim ke 'key position' hasil evaluasi (sisanya tetap random/patrol seperti biasa).", category: "Commander Defend Setting")]
	protected int m_iMaxKeyDefendPositions;
	// === END ADDED ===
	
	[Attribute("2", UIWidgets.ComboBox, "Commander Mode", "", ParamEnumArray.FromEnum(CMD_ECommanderMode), category: "Commander Personality" )]
	protected CMD_ECommanderMode m_eCommanderModeExternal;
	
	[Attribute("0.5", UIWidgets.Range, "Agresivitas/Eagerness: seberapa cepat commit assault tanpa tunggu recon.\n0 = tunggu recon tiba dulu | 1 = langsung serang tanpa recon", params: "0 1 0.01", category: "Commander Personality")]
	protected float m_fAggression;
	
	[Attribute("0.5", UIWidgets.Range, "Adaptabilitas: kecepatan switching mode dan reaktivitas commander.\n0 = lambat bereaksi | 1 = sangat responsif", params: "0 1 0.01", category: "Commander Personality")]
	protected float m_fAdaptability;
	
	// === ADDED: Commander Personality System (expansion) ===
	[Attribute("0.5", UIWidgets.Range, "Risk Taking: seberapa berani commit force ke objective yang FOGGY (gak ke-cover intel RECON).\n0 = nolak komit sampe ada intel jelas | 1 = tetep maksa nyerang walau buta", params: "0 1 0.01", category: "Commander Personality")]
	protected float m_fRiskTaking;
	
	[Attribute("0.5", UIWidgets.Range, "Resilience: seberapa tahan commander ngirim grup bertarung sebelum retreat.\n0 = gampang retreat (threshold tinggi) | 1 = tahan banting (threshold rendah, hold sampe abis)", params: "0 1 0.01", category: "Commander Personality")]
	protected float m_fResilience;
	
	[Attribute("0.5", UIWidgets.Range, "Patience: seberapa lama commander nunggu sebelum reallocate grup dari objective yang stalemate.\n0 = cepet nyerah/realokasi | 1 = sabar nungguin lama", params: "0 1 0.01", category: "Commander Personality")]
	protected float m_fPatience;
	// === END ADDED ===
	
	// === ADDED: Recon Reveal ===
	[Attribute("30.0", UIWidgets.EditBox, "Interval (detik) recon standing ngereveal musuh di sekitarnya ke threat response.", category: "Intel")]
	protected float m_fReconRevealInterval;
	// === END ADDED ===
	
	// === ADDED: Defensive Patrol System ===
	[Attribute("600.0", UIWidgets.EditBox, "Radius (meter) buat nyari objective captured LAIN yang bisa di-link jadi 1 rute patrol.", category: "Patrol")]
	protected float m_fPatrolLinkRadius;
	
	[Attribute("0.4", UIWidgets.Range, "Chance grup defend dapet Objective-Link Patrol (roaming antar objective) ketimbang Perimeter Patrol (muter di 1 objective doang).", params: "0 1 0.01", category: "Patrol")]
	protected float m_fLinkPatrolChance;
	// === END ADDED ===
	
    [Attribute("false", UIWidgets.CheckBox, desc: "Random Personality Every Playthough?", category: "Commander Personality")]
    bool m_bRandomPersonality;
 
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
		return m_iManpowerTotalCache * m_fReserveMinimumPct;
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
		
		return true;
	}
	
	protected void InitializeCommander()
	{
		if (!AICommander_ManagerComponent.GetInstance()) return;
		AICommander_ManagerComponent.GetInstance().RegisterCommander(this);
		threatComp = CMD_ThreatResponseComponent.Cast(m_MyEnt.FindComponent(CMD_ThreatResponseComponent));
		// === MODIFIED: Commander Personality System -- randomize semua trait (bukan
		// cuma Aggression/Adaptability lagi) kalau m_bRandomPersonality aktif ===
		if (m_bRandomPersonality)
		{
			m_fAggression  = Math.RandomFloat01();
			m_fAdaptability = Math.RandomFloat01();
			m_fRiskTaking  = Math.RandomFloat01();
			m_fResilience  = Math.RandomFloat01();
			m_fPatience    = Math.RandomFloat01();
		}
		// === END MODIFIED ===
		float adaptMod   = Math.Lerp(1.5, 0.5, m_fAdaptability);
		m_fThinkInterval = m_fThinkInterval * adaptMod;
		
		// === ADDED: Resilience -- scale retreat threshold. Resilience tinggi = commander
		// tahan banting, biarin grup bertarung sampe unit-nya beneran sedikit (threshold
		// rendah). Resilience rendah = gampang retreat, threshold-nya dinaikin. ===
		float resilienceMod = Math.Lerp(2.0, 0.5, m_fResilience);
		m_iRetreatThreshold = Math.Max(1, Math.Round(m_iRetreatThreshold * resilienceMod));
		// === END ADDED ===
		
		// === ADDED: Patience -- scale stalemate response cooldown. Patience tinggi =
		// commander sabar, nunggu lebih lama sebelum reallocate grup dari objective
		// yang stalemate. Patience rendah = cepet nyerah/realokasi. ===
		float patienceMod = Math.Lerp(0.4, 2.5, m_fPatience);
		m_fStalemateResponseCooldown = m_fStalemateResponseCooldown * patienceMod;
		// === END ADDED ===
		
		// === ADDED: BUG FIX -- m_fThinkTimer sebelumnya diinisialisasi
		// "0 - m_fDelayFirstIteration" (field initializer), dan m_fDelayFirstIteration
		// itu attribute FIXED (default 60.0, sama buat semua commander). Efeknya:
		// trigger PERTAMA semua commander SELALU bareng persis di detik ke-60, dan
		// kalau m_fAdaptability antar commander kebetulan mirip (atau gak di-random),
		// mereka bisa tetep nempel bareng terus-menerus setelahnya juga. Ini
		// kemungkinan penyebab periodic freeze -- Think() cycle SEMUA commander
		// (masing-masing bisa nge-trigger banyak QueryEntitiesBySphere lewat
		// ComputePriorityScore per objective) numpuk di frame yang sama. Fix: kasih
		// jitter random per-instance ke starting timer, di atas delay awal yang udah
		// ada (bukan gantiin -- combine keduanya).
		m_fThinkTimer = m_fThinkTimer - Math.RandomFloat(0.0, m_fThinkInterval);
		// === END ADDED ===
		
		artySupport = CMD_ArtillerySupport.Cast(m_MyEnt.FindComponent(CMD_ArtillerySupport));
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
 
	// === MODIFIED: Optimasi -- tambah optional contextCache, dipass dari ThinkOffensive
	// (dibangun sekali per cycle). Backward compatible -- kalau null, fallback ke
	// jalur lama (query manager per objective).
	// === ADDED: Recon Wait Timeout ===
	//! Catat kapan objective ini pertama kali di-mark ASSIGNED buat faction ini.
	//! Dipake nge-gate timeout recon-wait, independen dari Synchronized Attack
	//! toggle (biar tetep jalan walau m_bUseSynchronizedAttack dimatiin).
	protected void RecordAssignedTime(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		if (!m_mAssignedTime.Contains(obj))
			m_mAssignedTime.Insert(obj, worldTime);
		else
			m_mAssignedTime.Set(obj, worldTime);
	}
	// === END ADDED ===
	
	protected void AssignRolesToObjective(CMD_AICommanderObjectiveComponent obj, float worldTime, CMD_ObjectiveContextCache contextCache = null)
	{
	    // === ADDED: RECON-type objective gating -- objective RECON gak diserang/
	    // dipertahanin kayak biasa, cuma butuh standing recon presence. Logic
	    // attack/defend di bawah sama sekali gak relevan buat tipe ini.
	    if (obj.GetObjectiveType() == CMD_EObjectiveType.RECON)
	    {
	        AssignReconOnlyToObjective(obj, worldTime);
	        return;
	    }
	    // === END ADDED ===
	    
	    CMD_EObjectiveState objState = obj.GetObjectiveState(m_sFactionKey);
	
	    if (objState == CMD_EObjectiveState.COMPLETED || objState == CMD_EObjectiveState.FAILED)
	        return;
	
	    // === MODIFIED: BUG FIX -- RiskTaking gate sebelumnya jalan UNCONDITIONAL tiap
	    // cycle buat SEMUA state (PENDING maupun ASSIGNED). Kalau map gak punya objective
	    // RECON sama sekali, objective manapun bakal SELAMANYA foggy (gak ada yang bisa
	    // nge-cover), dan gate ini bakal keroll TIAP cycle -- kalau kalah roll (RiskTaking
	    // rendah), early-return dan SKIP logic gathering/release Synchronized Attack yang
	    // udah jalan buat objective yang UDAH ASSIGNED. Efeknya keliatan kayak "commander
	    // gak mau nyerang, diem doang" karena progress yang udah dimulai ke-stall
	    // berulang-ulang. Sekarang gate ini CUMA berlaku buat keputusan AWAL (state masih
	    // PENDING) -- begitu commander komit (ASSIGNED), dia jalan terus, gak mikir ulang
	    // tiap cycle.
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
	            return;
	        }
	    }
	    // === END MODIFIED ===
	
	    if (objState == CMD_EObjectiveState.PENDING)
	    {
	    	if (m_fAggression >= Math.RandomFloat01())
	    	{
	    		TrySendToStaging(obj, worldTime);
	    		obj.MarkAssigned(m_sFactionKey);
	    		RecordAssignedTime(obj, worldTime);
	    		return;
	    	}
	        // === MODIFIED: BUG FIX -- branch low-aggression ini sebelumnya GAK PERNAH
	        // manggil MarkAssigned(). Efeknya objective STUCK di state PENDING SELAMANYA
	        // kalau roll Eagerness kalah -- tiap cycle cuma re-send recon+staging berulang,
	        // gak pernah naik ke ASSIGNED, gak pernah nyampe logic assault/gathering.
	        // Commander dengan Eagerness rendah (personality) jadi kayak "gak mau nyerang
	        // objective manapun" -- padahal maksud desainnya emang cuma nunggu recon
	        // konfirmasi dulu SEBELUM nyerang (logic itu udah ada di branch ASSIGNED,
	        // tunggu IsReconArrived -- tapi logic itu gak pernah kesampean karena
	        // objective-nya gak pernah ke-mark ASSIGNED). Sekarang MarkAssigned() dipanggil
	        // di sini juga, biar transisi ke ASSIGNED beneran kejadian dan logic tunggu-
	        // recon di branch ASSIGNED bisa jalan sesuai desain aslinya.
	        TrySendRecon(obj);
	        TrySendToStaging(obj, worldTime);
	        obj.MarkAssigned(m_sFactionKey);
	        RecordAssignedTime(obj, worldTime);
	        return;
	        // === END MODIFIED ===
	    }
	
	    if (objState == CMD_EObjectiveState.ASSIGNED)
	    {
	        // === MODIFIED: Recon opsional -- commander harus SELALU advancing, gak boleh
	        // nunggu recon selamanya. Timeout m_fReconWaitTimeout detik sejak objective
	        // di-ASSIGNED -- abis itu SKIP recon-wait apapun hasilnya (available atau
	        // enggak, arrived atau enggak), tetep lanjut nyerang. Personality (Eagerness)
	        // masih ngaruh ke KEMUNGKINAN nunggu di awal, timeout ini cuma jaring pengaman
	        // biar gak stuck selamanya kalau recon emang gak available/gak kunjung dateng.
	        float assignedTime;
	        bool hasAssignedRecord = m_mAssignedTime.Find(obj, assignedTime);
	        bool reconTimedOut = !hasAssignedRecord || (worldTime - assignedTime) > m_fReconWaitTimeout;

	        if (!reconTimedOut && m_fAggression < Math.RandomFloat01() && !obj.IsReconArrived(m_sFactionKey, worldTime))
	        {
	            return;
	        }
	        // === END MODIFIED ===

	        // === MODIFIED: Synchronized Attack -- sebelumnya langsung TrySendAssaultWithSlots
	        // yang isi 1 slot per cycle dan LANGSUNG ngirim grup ke objective (piecemeal).
	        // Sekarang (kalau m_bUseSynchronizedAttack aktif): selama belum "released",
	        // grup-grup ngumpul dulu di staging sampe FULL atau timeout
	        // (m_fSyncAttackMaxWaitTime), baru semuanya dilepas bareng ke objective di
	        // cycle yang sama. Setelah released, reinforcement susulan (kalau ada slot
	        // kosong lagi belakangan) tetap pake TrySendAssaultWithSlots yang lama --
	        // langsung ke objective, gak perlu nunggu-nunggu lagi.
	        //
	        // Kalau m_bUseSynchronizedAttack DIMATIKAN di Workbench, skip semua gathering/
	        // release, balik ke behavior lama sepenuhnya (langsung TrySendAssaultWithSlots).
	        if (m_bUseSynchronizedAttack)
	        {
	            bool released;
	            if (!m_mAssaultReleased.Find(obj, released))
	                released = false;

	            if (!released)
	            {
	                TryGatherForSynchronizedAssault(obj, worldTime);

	                float stagingStart;
	                if (!m_mStagingStartTime.Find(obj, stagingStart))
	                {
	                    stagingStart = worldTime;
	                    m_mStagingStartTime.Insert(obj, stagingStart);
	                }

	                bool isFull    = obj.IsGroupSlotFull(m_sFactionKey);
	                bool timedOut  = (worldTime - stagingStart) > m_fSyncAttackMaxWaitTime;

	                if (isFull || timedOut)
	                {
	                    ReleaseSynchronizedAssault(obj, worldTime);

	                    if (!m_mAssaultReleased.Contains(obj))
	                        m_mAssaultReleased.Insert(obj, true);
	                    else
	                        m_mAssaultReleased.Set(obj, true);
	                }

	                return;
	            }
	        }
	        // === END MODIFIED ===

	        TrySendAssaultWithSlots(obj, worldTime);
	    }
	}
	
	// === ADDED: RECON objective standing-presence logic ===
	//! Objective RECON gak pernah “selesai” -- cuma butuh ADA grup RECON standing di
	//! situ terus-menerus. Kalau belum ada/grup lama mati-pergi, kirim penggantinya.
	//! IsReconObjectiveActive udah ngecek grup masih idup DAN masih beneran di area,
	//! jadi ini otomatis “refill” begitu presence-nya kosong.
	protected void AssignReconOnlyToObjective(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
	    if (obj.IsReconObjectiveActive(m_sFactionKey))
	    {
	        // === ADDED: Recon Reveal -- selama recon standing di sini, berkala
	        // scan+report kontak musuh di sekitarnya ke threat response, biar reveal
	        // ini beneran ngefek ke sistem lain (cluster/reinforcement/artillery),
	        // bukan cuma dekoratif.
	        TryReconRevealEnemies(obj, worldTime);
	        // === END ADDED ===
	        return;
	    }
	    
	    TrySendRecon(obj);
	}
	
	// === ADDED: Recon Reveal ===
	//! Selama grup RECON masih idup & standing di objective RECON, tiap
	//! m_fReconRevealInterval detik dia "reveal" musuh di sekitarnya (dalam radius
	//! intel-coverage objective itu) dengan ngirim contact report ke threat response
	//! component -- persis kayak laporan kontak dari grup manapun, cuma sumbernya
	//! recon yang standing, bukan grup yang aktif engage.
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
		
		int enemyCount = obj.CountNearbyUnits(obj.GetIntelCoverageRadius(), m_sFactionKey, false);
		
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
 
	protected void SendIdleGroupsToReserve()
	{
	    AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
	    if (!mgr)
	        return;
	
	    // === OPTIMIZED ===
	    // Sebelumnya: mgr.GetTopObjectives() dipanggil ULANG per idle group DI DALAM loop.
	    // GetTopObjectives() men-trigger ComputePriorityScore() (isinya QueryEntitiesBySphere,
	    // spatial query beneran mahal) buat SETIAP objective di map, terus hasilnya CUMA dipakai
	    // buat filter IsCapturedBy lalu di-random-shuffle lagi -- urutan priority-nya kebuang
	    // percuma. Kalau ada 10 grup idle & 15 objective, itu ratusan spatial query per Think()
	    // cycle. Sekarang: list posisi objective captured dihitung SEKALI di luar loop, langsung
	    // pakai mgr.m_aObjective (unsorted -- gapapa karena toh langsung di-shuffle per grup).
	    array<vector> capturedObjPositions = new array<vector>();
	    foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
	    {
	        if (!obj)
	            continue;
	
	        if (!obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
	            continue;
	
	        capturedObjPositions.Insert(obj.GetOwner().GetOrigin());
	    }
	
	    // === MODIFIED: "rand" dihapus dari sini -- GenerateRandomPointInRadius yang
	    // dulu manggilnya udah gak dipake lagi, digantiin GeneratePatrolRoute() yang
	    // punya RandomGenerator sendiri di dalemnya. ===
	    float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
	    // === END OPTIMIZED / MODIFIED ===
	
	    foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
	    {
	        if (!grp)
	            continue;
	
	        if (grp.GetGroupStatus() == DCOG_EGroupStatus.EXECUTING_COMMAND)
	            continue;
	
	        if (grp.GetGroupRole() != CMD_EGroupRole.NONE)
	            continue;
			
			if (!grp.CanItHaveOrder())
				continue;
	
	        array<vector> candidatePositions = new array<vector>();
	        foreach (vector cp : capturedObjPositions)
	            candidatePositions.Insert(cp);
	
	        // === MODIFIED: REVERT -- sebelumnya (kalau captured <2) fallback narik posisi
	        // objective PENDING sebagai kandidat, biar gak numpuk di 1 titik. Tapi itu bikin
	        // masalah baru: reserve grup jadi muter-muter NGERUBUNGIN objective yang lagi
	        // digarap assault/recon grup lain -- aneh, reserve gak ada urusan ke situ.
	        //
	        // Ternyata gak perlu -- GeneratePatrolRoute() (dipake di bawah) UDAH otomatis
	        // nyebar sendiri (4-6 titik random, radius jitter, starting angle acak) buat
	        // SATU center manapun yang dikasih. Jadi "numpuk di 1 titik" yang jadi alasan
	        // fallback ini sebenernya udah keselesein sendiri sama GeneratePatrolRoute --
	        // gak perlu narik ke objective pending lagi. Reserve sekarang balik jaga
	        // perimeter HQ begitu belum ada territory captured (early game) -- itu emang
	        // tugas reserve yang bener (jaga markas, siap di-deploy), bukan ngerecokin
	        // target orang lain.
	        if (candidatePositions.IsEmpty())
	            candidatePositions.Insert(GetOwner().GetOrigin());
	        // === END MODIFIED ===
	
	        // === MODIFIED: Kesibukan grup idle -- sebelumnya shuffle SEMUA kandidat terus
	        // nyamperin satu-satu sekali jalan (abis itu diem lagi sampe Think() cycle
	        // berikutnya, ~45 detik default). Sekarang: pilih kandidat TERDEKAT dari grup
	        // ini, kasih patrol LOOP beneran di situ (GeneratePatrolRoute, 4-6 titik
	        // berulang) -- grup keliatan "sibuk" terus-menerus, bukan cuma numpang lewat.
	        vector nearestCandidate = candidatePositions[0];
	        float nearestDistSq = vector.DistanceSq(grp.GetOwner().GetOrigin(), nearestCandidate);
	        foreach (vector cand : candidatePositions)
	        {
	            float distSq = vector.DistanceSq(grp.GetOwner().GetOrigin(), cand);
	            if (distSq < nearestDistSq)
	            {
	                nearestDistSq = distSq;
	                nearestCandidate = cand;
	            }
	        }

	        GeneratePatrolRoute(grp, nearestCandidate, m_fBaseRadius, worldTime);
	        // === END MODIFIED ===

	        grp.SetGroupRole(CMD_EGroupRole.RESERVE);
	    }
	}
	
	protected DCO_GroupUtilityComponent FindBestIdleGroupForRole(CMD_EGroupRole role, vector targetPos)
	{
		// Susunan tier fallback (index 0 = prioritas tertinggi), persis urutan lama
		array<CMD_EGroupRole> tiers = {role, CMD_EGroupRole.NONE, CMD_EGroupRole.RESERVE, CMD_EGroupRole.RECON, CMD_EGroupRole.REINFORNCE, CMD_EGroupRole.DEFEND};
		
		// ARMORED exception -- sebelumnya cuma nyoba tier pertama (role itu sendiri), gak fallback
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

			if (grp.GetGroupStatus() == DCOG_EGroupStatus.EXECUTING_COMMAND)
				continue;

			if (grp.IsDedicatedTransport())
				continue;

			if (!grp.CanCommanderOverrideRole())
				continue;

			if (!grp.CanItHaveOrder())
				continue;

			// === ADDED: BUG FIX -- sebelumnya player group gak di-exclude di sini,
			// jadi bisa kepilih jadi "kandidat terbaik" buat auto-assign (ASSAULT/FLANK/
			// dll). Caller-caller (TrySendToStaging, TryGatherForSynchronizedAssault, dll)
			// ngecek IsPlayerGroup() abis dapet hasil dan ada yang RETURN dari SELURUH
			// fungsi kalau ketemu player group -- kalau grup player itu KONSISTEN jadi
			// kandidat terbaik (deket/status pas), objective bisa stuck SELAMANYA gak
			// pernah dapet grup AI karena selalu "ketemu player duluan, terus di-abort".
			// Filter di sini -- player group emang gak boleh di-auto-assign, tapi caller
			// harus tetep bisa nemuin grup AI LAIN yang available, bukan nyerah total.
			if (grp.IsPlayerGroup())
				continue;
			// === END ADDED ===

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

			if (score > bestScorePerTier[tierIdx])
			{
				if (bestDistSqPerTier[tierIdx] < 0.0 || distSq < bestDistSqPerTier[tierIdx])
				{
					bestScorePerTier[tierIdx]  = score;
					bestDistSqPerTier[tierIdx] = distSq;
					bestPerTier[tierIdx]       = grp;
				}
			}
		}

		for (int t = 0; t < tierCount; t++)
		{
			if (bestPerTier[t])
				return bestPerTier[t];
		}

		return null;
	}
	
	
	protected void EvaluateCommanderMode(float worldTime)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;
		
		// === MODIFIED: BUG FIX -- sebelumnya switch di m_eCommanderMode (mode SAAT
		// INI), padahal harusnya switch di m_eCommanderModeExternal (mode yang
		// DIMINTA). Efeknya: kalau external minta DEFENSIVE tapi commander lagi
		// OFFENSIVE, switch malah ke-hit case OFFENSIVE (reinforce mode lama, gak
		// pernah pindah). Kalau current mode BALANCED, gak match case manapun sama
		// sekali -- mode gak pernah keganti walau diminta eksternal. ===
		if (m_eCommanderModeExternal != CMD_ECommanderMode.BALANCED)
		{
			switch (m_eCommanderModeExternal)
			{
				case CMD_ECommanderMode.DEFENSIVE:
				{
					SwitchToDefensive(worldTime);
					return;
				}
				case CMD_ECommanderMode.OFFENSIVE:
				{
					SwitchToOffensive();
					return;
				}
			}
		} else
		{
			m_eCommanderMode = m_eCommanderModeExternal;
			return;
		}
		// === END MODIFIED ===
	 	/*
		bool anyLost = false;
		bool haveObjectiveToDefend = false;
	 
		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj)
				continue;
	 
			if (obj.IsCapturedBy(m_sFactionKey))
			{
				haveObjectiveToDefend = true;
				if (obj.GetObjectiveState(m_sFactionKey) == CMD_EObjectiveState.FAILED)
					anyLost = true;
				break;
			}
		}
	 
		if ((haveObjectiveToDefend || anyLost) && m_eCommanderMode == CMD_ECommanderMode.OFFENSIVE)
		{
			SwitchToDefensive(worldTime);
			return;
		}
	 
		if (m_eCommanderMode == CMD_ECommanderMode.DEFENSIVE && !anyLost)
		{
			float elapsed = worldTime - m_fDefensiveTriggerCooldown;
			if (elapsed >= DEFENSIVE_COOLDOWN)
				SwitchToOffensive();
		}*/
	}
	
	protected void Think(float worldTime)
	{
		if (!Replication.IsServer())
			return;
	 
		m_eCommanderState = CMD_ECommanderState.PLANNING;
	 
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;
	 
		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp)
				continue;
			
			if (!grp.CanItHaveOrder())
				continue;
	 
			if (grp.GetUnitCount() <= m_iRetreatThreshold
				&& grp.GetGroupStatus() != DCOG_EGroupStatus.IDLE)
			{
				//SCR_AIWaypoint rallyWP = SpawnMoveWP(GetOwner().GetOrigin());
				//if (rallyWP)
					//grp.ForceRetreat(rallyWP, worldTime);
			}
	 
			if (!grp.CheckOrderComplete(worldTime))
				continue;
		}
	 
		// === ADDED: Manpower cache refresh (1x per Think cycle, lihat GetReserveFloor) ===
		m_iManpowerTotalCache = GetTotalManpower();
		// === END ADDED ===
	 
		EvaluateCommanderMode(worldTime);
	 
		// === ADDED: Absolute Defend Guarantee ===
		// Captured objective SELALU dicek butuh reinforcement defend, terlepas dari
		// commander mode saat ini (dulu ini cuma jalan pas mode DEFENSIVE/BALANCED,
		// jadi kalau commander lagi full OFFENSIVE, objective yang udah captured
		// gak pernah direinforce). Dipanggil SEBELUM offense supaya defend dapat
		// prioritas pertama ambil grup dari reserve pool.
		// PENGECUALIAN: commander bertipe OFFENSIVE murni (full aggression, personality
		// yang gak peduli hold ground) SENGAJA di-skip — semua manpower dialokasikan
		// buat push, gak ada yang dikorbankan buat defend captured objective.
		if (m_eCommanderMode != CMD_ECommanderMode.OFFENSIVE)
			EnsureAbsoluteDefend(worldTime);
		// === END ADDED ===
	 
		if (m_eCommanderMode == CMD_ECommanderMode.DEFENSIVE)
			ThinkDefensive(worldTime);
		else if (m_eCommanderMode == CMD_ECommanderMode.OFFENSIVE)
			ThinkOffensive(mgr, worldTime);
		else if (m_eCommanderMode == CMD_ECommanderMode.BALANCED)
		{
			// === MODIFIED: urutan think tergantung Eagerness (m_fAggression).
			// Commander eager mikirin OFFENSE duluan (assault dapet prioritas rebutan
			// grup idle sebelum defend kebagian), commander kurang eager mikirin
			// DEFENSE duluan (jaga yang udah ada dulu, baru comit sisa manpower buat
			// nyerang). Order ini beneran ngaruh karena dua-duanya rebutan dari pool
			// grup idle yang sama (m_aOwnedGroup) -- yang duluan jalan dapet pilihan
			// pertama.
			if (m_fAggression >= 0.5)
			{
				ThinkOffensive(mgr, worldTime);
				ThinkDefensive(worldTime);
			}
			else
			{
				ThinkDefensive(worldTime);
				ThinkOffensive(mgr, worldTime);
			}
			// === END MODIFIED ===
		}
		
		// === MODIFIED: BUG FIX -- sebelumnya SendIdleGroupsToReserve() CUMA dipanggil
		// sebagai fallback terakhir (m_aObjective.IsEmpty() di ThinkOffensive / !hasAnyWork
		// di ThinkDefensive). Selama ada objective aktif (yang hampir selalu ada), fungsi
		// itu gak pernah kepanggil -- akibatnya grup yang ketahan manpower reserve floor
		// (CanCommitGroup) atau kelebihan dari slot objective yang udah penuh cuma
		// nyangkut di role NONE, status IDLE, selamanya. Gak dikasih patrol/standby
		// apapun -- keliatan kayak "AFK". Sekarang dipanggil SEKALI tiap akhir Think()
		// cycle, unconditional -- internal loop-nya sendiri udah filter cuma proses grup
		// yang MASIH role NONE, jadi aman dipanggil tiap cycle, gak ganggu grup yang
		// udah ke-assign kemana pun. ===
		SendIdleGroupsToReserve();
		// === END MODIFIED ===
	 
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
			
			if (obj.CheckAndMarkIfLost(m_sFactionKey))
			{
				obj.ResetLostStatus(m_sFactionKey);
				continue;
			}
	 
			AssignRolesToObjective(obj, worldTime, contextCache);
		}
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
			
			if (obj.CheckIsItLost(m_sFactionKey))
			{
				obj.ResetAssignedGroupCount(m_sFactionKey);
				obj.SetObjectiveState(m_sFactionKey, CMD_EObjectiveState.PENDING);
				continue;
			}
			
	 
			AssignDefendToObjective(obj, worldTime);
			hasAnyWork = true;
		}
		
		Print(hasAnyWork.ToString() + " < HAS DEFEND WORK FOR " + m_sCommanderUID + " " + m_sFactionKey);
		// === MODIFIED: SendIdleGroupsToReserve() dicabut dari sini -- sekarang dipanggil
		// terpusat 1x per Think() cycle di Think() sendiri, gak lagi gated hasAnyWork ===
	}
	
	protected void TrySendToStaging(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
	    vector objPos  = obj.GetOwner().GetOrigin();
	    vector base    = GetOwner().GetOrigin();
	    vector axis    = objPos - base;
	    axis           = Vector(axis[0], 0.0, axis[2]);
	    axis           = axis.Normalized();
	    float dist     = vector.Distance(base, objPos);
	
	    vector stagingPos   = base + axis * (dist * Math.RandomFloatInclusive(0.15, 0.4));
	    stagingPos[1]       = GetGame().GetWorld().GetSurfaceY(stagingPos[0], stagingPos[2]);
	
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
			// === END ADDED ===
			assaultGrp.CompleteAllWaypoints();
			if (TryAssignTransport(assaultGrp, stagingPos, worldTime))
    			return;
	        SCR_AIWaypoint wp = SpawnMoveWP(stagingPos);
	        if (wp)
	        {
	            assaultGrp.SetGroupRole(CMD_EGroupRole.ASSAULT);
	            assaultGrp.MoveTo(wp, worldTime);
	            //Print(string.Format("[%1] ASSAULT → STAGING: %2", m_sCommanderUID, assaultGrp.GetOwner().GetName()));
	        }
			} // === ADDED: closes CanCommitGroup else block ===
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
	            //Print(string.Format("[%1] FLANK → STAGING: %2", m_sCommanderUID, flankGrp.GetOwner().GetName()));
	        }
	    }
	}
	
	protected void GenerateSearchWaypoints(vector center, float radius, array<SCR_AIWaypoint> outWaypoints, float wpSpacing = 50.0)
	{
	    if (!outWaypoints)
	        return;
	
	    outWaypoints.Clear();
	
	    int rings = Math.Max(1, (int)Math.Round(radius / wpSpacing));
	    
	    int baseSectorsPerRing = Math.Max(2, (int)Math.Round((radius * 2 * Math.PI) / (wpSpacing * 1.5))); 
	
	    RandomGenerator rand = new RandomGenerator();
	    float ringStep = radius / rings;
	
	    array<int> middleRings = new array<int>();
	    for (int r = 1; r < rings; r++)
	        middleRings.Insert(r);
	
	    for (int r = middleRings.Count() - 1; r > 0; r--)
	    {
	        int swapIdx = Math.RandomInt(0, r + 1);
	        int tmp = middleRings[r];
	        middleRings[r] = middleRings[swapIdx];
	        middleRings[swapIdx] = tmp;
	    }
	
	    array<int> ringOrder = new array<int>();
	    ringOrder.Insert(rings);
	    foreach (int r : middleRings)
	        ringOrder.Insert(r);
	    
	    if (rings > 1) 
	        ringOrder.Insert(rings);
	
	    foreach (int ring : ringOrder)
	    {
	        float radiusInner = ringStep * (ring - 1);
	        float radiusOuter = ringStep * ring;
	        
	        int currentSectors = Math.Max(2, (int)Math.Round(baseSectorsPerRing * ((float)ring / rings)));
	        float sectorAngle = 360.0 / currentSectors;
	
	        for (int sector = 0; sector < currentSectors; sector++)
	        {
	            float angleMin = sectorAngle * sector;
	            float angleMax = sectorAngle * (sector + 1);
	            float angleDeg = Math.RandomFloat(angleMin, angleMax);
	            float angleRad = angleDeg * Math.DEG2RAD;
	
	            float dist = Math.RandomFloat(radiusInner + 1.0, radiusOuter);
	
	            float px = center[0] + Math.Cos(angleRad) * dist;
	            float pz = center[2] + Math.Sin(angleRad) * dist;
	            float py = GetGame().GetWorld().GetSurfaceY(px, pz);
	
	            SCR_AIWaypoint wp = SpawnMoveWP(Vector(px, py, pz));
	            if (wp)
	                outWaypoints.Insert(wp);
	        }
	    }
	}
	
	SCR_AIWaypoint SpawnMoveWP(vector pos)
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
	
	// === ADDED: Absolute Defend Guarantee ===
	// Loop semua objective yang udah captured commander ini dan pastikan defend
	// slot-nya keisi, terlepas dari commander mode. Dipanggil setiap Think() cycle.
	protected void EnsureAbsoluteDefend(float worldTime)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;
	
		// === OPTIMIZED: pakai mgr.m_aObjective langsung, gak perlu GetTopObjectives()
		// (priority sort + ComputePriorityScore/QueryEntitiesBySphere buat semua objective)
		// karena di sini urutan gak ngaruh -- semua captured objective diproses tanpa break.
		array<CMD_AICommanderObjectiveComponent> allObjs = mgr.m_aObjective;
	
		foreach (CMD_AICommanderObjectiveComponent obj : allObjs)
		{
			if (!obj)
				continue;
	
			if (!obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
				continue;
	
			if (obj.CheckIsItLost(m_sFactionKey))
				continue;
	
			AssignDefendToObjective(obj, worldTime);
		}
	}
	
	// Evaluasi titik defend di sekitar objective berdasarkan dua faktor heuristik:
	// 1. Elevasi permukaan (dataran tinggi relatif terhadap rata-rata ring) — defend dari atas lebih unggul.
	// 2. Arah hadap terhadap axis base→objective — titik yang "menghadap" menjauhi base
	//    diasumsikan lebih dekat ke arah datangnya musuh (karena base = rear friendly).
	// CATATAN: ini heuristik dari GetSurfaceY doang, BUKAN analisis line-of-sight/navmesh
	// sungguhan (engine ini gak expose API buat itu ke script). Cukup buat menghindari
	// full-random placement, bukan pengganti recon manusia.
	protected void EvaluateDefendPositions(vector center, float radius, int count, out array<vector> outPositions)
	{
		outPositions = {};
		if (count <= 0)
			return;
	
		int sampleCount = 16;
		array<vector> candidates = {};
		float sampleRadius = radius * 1.3;
		float totalElevation = 0.0;
	
		for (int i = 0; i < sampleCount; i++)
		{
			float angleDeg = (360.0 / sampleCount) * i;
			float angleRad = angleDeg * Math.DEG2RAD;
	
			float px = center[0] + Math.Cos(angleRad) * sampleRadius;
			float pz = center[2] + Math.Sin(angleRad) * sampleRadius;
			float py = GetGame().GetWorld().GetSurfaceY(px, pz);
	
			vector candidate = Vector(px, py, pz);
			candidates.Insert(candidate);
			totalElevation += py;
		}
	
		float avgElevation = totalElevation / sampleCount;
	
		vector baseToObj = center - GetOwner().GetOrigin();
		baseToObj = Vector(baseToObj[0], 0.0, baseToObj[2]);
		baseToObj = baseToObj.Normalized();
	
		array<float> scores = {};
		for (int i = 0; i < candidates.Count(); i++)
		{
			float elevationScore = candidates[i][1] - avgElevation;
	
			vector toCandidate = candidates[i] - center;
			toCandidate = Vector(toCandidate[0], 0.0, toCandidate[2]);
			toCandidate = toCandidate.Normalized();
	
			float facingScore = vector.Dot(toCandidate, baseToObj) * 10.0;
	
			scores.Insert(elevationScore * 2.0 + facingScore);
		}
	
		array<vector> sortedCandidates = {};
		array<float> sortedScores = {};
		for (int i = 0; i < candidates.Count(); i++)
		{
			float score = scores[i];
			bool inserted = false;
			for (int j = 0; j < sortedCandidates.Count(); j++)
			{
				if (score > sortedScores[j])
				{
					sortedCandidates.InsertAt(candidates[i], j);
					sortedScores.InsertAt(score, j);
					inserted = true;
					break;
				}
			}
	
			if (!inserted)
			{
				sortedCandidates.Insert(candidates[i]);
				sortedScores.Insert(score);
			}
		}
	
		// Ambil top-N sambil jaga jarak minimum biar defend spot gak numpuk di 1 sisi objective.
		array<vector> picked = {};
		foreach (vector cand : sortedCandidates)
		{
			if (picked.Count() >= count)
				break;
	
			bool tooClose = false;
			foreach (vector p : picked)
			{
				if (vector.Distance(p, cand) < radius * 0.6)
				{
					tooClose = true;
					break;
				}
			}
	
			if (!tooClose)
				picked.Insert(cand);
		}
	
		// Fallback kalau filter jarak ketat dan belum cukup, isi sisa tanpa filter.
		if (picked.Count() < count)
		{
			foreach (vector cand : sortedCandidates)
			{
				if (picked.Count() >= count)
					break;
				if (!picked.Contains(cand))
					picked.Insert(cand);
			}
		}
	
		outPositions = picked;
	}
	// === END ADDED ===
	
	protected void AssignDefendToObjective(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
	    int needed = obj.GetDefendGroupCount();
	    int current = obj.GetCurrentAssignedGroupCount(m_sFactionKey);
	    int toSend = needed - current;
	
	    if (toSend <= 0)
	        return;
	
	    vector objPos = obj.GetOwner().GetOrigin();
	
	    array<int> roleList = new array<int>();
	
	    if (toSend > 2)
	    {
	        roleList.Insert(1);
	        for (int i = 1; i < toSend; i++)
	        {
				float qrfChance = 0.2 / i;
				float roll = Math.RandomFloat(0.0, 1.0);
				
				int role = 0;
				if (roll < qrfChance)
				    role = 1;
				
				roleList.Insert(role);
	        }
	    }
	    else
	    {
	        for (int i = 0; i < toSend; i++)
	            roleList.Insert(0);
	    }
	
	    for (int i = roleList.Count() - 1; i > 0; i--)
	    {
	        int j = Math.RandomInt(0, i + 1);
	        int tmp = roleList[i];
	        roleList[i] = roleList[j];
	        roleList[j] = tmp;
	    }
	
	    // === ADDED: Key Defend Position Evaluation ===
	    array<vector> keyDefendPositions = {};
	    int keyPosUsed = 0;
	    if (m_bEnableKeyDefendSpotEvaluation)
	    {
	        int keyPosCount = Math.Min(toSend, m_iMaxKeyDefendPositions);
	        EvaluateDefendPositions(objPos, obj.GetRadius(), keyPosCount, keyDefendPositions);
	    }
	    // === END ADDED ===
	
	    // === OPTIMIZED: RandomGenerator dibuat SEKALI di sini, bukan per-grup di dalam loop ===
	    RandomGenerator rand = new RandomGenerator();
	    // === END OPTIMIZED ===
	
	    for (int i = 0; i < toSend; i++)
	    {
	        DCO_GroupUtilityComponent defGrp = FindBestIdleGroupForRole(CMD_EGroupRole.RESERVE, obj.GetOwner().GetOrigin());
	        if (!defGrp)
	            break;
			
			defGrp.CompleteAllWaypoints();
	
	        if (roleList[i] == 1)
	        {
	            vector qrfPos = ComputeFlankPosition(GetOwner().GetOrigin(), objPos, obj.GetRadius() * 0.8);
	            if (!TryAssignTransport(defGrp, qrfPos, worldTime))
	            {
	                SCR_AIWaypoint wp = SpawnMoveWP(qrfPos);
	                if (wp)
	                {
	                    defGrp.SetGroupRole(CMD_EGroupRole.RESERVE);
	                    defGrp.MoveTo(wp, worldTime);
	                    obj.SetObjectiveGroup(m_sFactionKey, 1);
	                }
	            }
	        }
	        else
	        {
	            float roll = Math.RandomFloat(0.0, 1.0);
	            bool bDoDefend = (roll < m_fDefendChance);
	
	            if (bDoDefend)
	            {
	                // (rand dipakai dari instance yang di-hoist di atas loop, gak dibuat ulang di sini)
	                vector defendPos = rand.GenerateRandomPointInRadius(0, obj.GetRadius(), obj.GetOwner().GetOrigin(), false);
	                defendPos[1] = GetGame().GetWorld().GetSurfaceY(defendPos[0], defendPos[2]);
	
	                // === ADDED: pakai key defend position hasil evaluasi kalau tersedia ===
	                if (keyPosUsed < keyDefendPositions.Count())
	                {
	                    defendPos = keyDefendPositions[keyPosUsed];
	                    keyPosUsed++;
	                }
	                // === END ADDED ===
	
	                SCR_AIWaypoint wp = SpawnDefendWP(defendPos);
	                if (wp)
	                {
	                    defGrp.SetGroupRole(CMD_EGroupRole.DEFEND);
	                    defGrp.MoveTo(wp, worldTime);
	                    wp.SetCompletionRadius(obj.GetRadius());
	                    obj.SetObjectiveGroup(m_sFactionKey, 1);
	                }
	            }
	            else
	            {
	                AssignDefensivePatrol(defGrp, obj, worldTime);
	                obj.SetObjectiveGroup(m_sFactionKey, 1);
	            }
	        }
	    }
	
	    Print("Assigning Number Of Squad to Defend : " + toSend.ToString());
	}
	
	// === MODIFIED: Perimeter Patrol -- sebelumnya selalu 4 titik simetris persis di
	// angle yang sama tiap dipanggil (robotic, dan kalau ada 2+ grup defend di
	// objective yang sama, rutenya bakal identik persis). Sekarang titik-nya
	// bervariasi (4-6), radius per titik di-jitter dikit, dan starting angle random
	// -- biar beda grup di objective yang sama gak jalan di rute yang persis sama. ===
	// === MODIFIED: logic patrol-nya di-extract ke GeneratePatrolRoute() (role-agnostic),
	// dipake bareng sama SendIdleGroupsToReserve() juga sekarang -- biar grup idle
	// dapet patrol loop beneran (4-6 titik terus-menerus), bukan cuma nyamperin
	// sederet titik sekali terus diem lagi sampe Think() cycle berikutnya. ===
	protected void GeneratePatrolRoute(DCO_GroupUtilityComponent grp, vector center, float radius, float worldTime)
	{
		RandomGenerator rand = new RandomGenerator();
		
		int patrolPoints   = Math.RandomInt(4, 7); // 4-6 titik
		float patrolRadius = radius * 1.6;
		float startAngle   = Math.RandomFloat(0.0, 360.0);
		
		grp.CompleteAllWaypoints();
		
		for (int p = 0; p < patrolPoints; p++)
		{
			float angleDeg = startAngle + (360.0 / patrolPoints) * p;
			float angleRad = angleDeg * Math.DEG2RAD;
			
			// Jitter radius dikit (±15%) biar gak keliatan muter di lingkaran sempurna
			float radiusJitter = rand.RandFloatXY(patrolRadius * 0.85, patrolRadius * 1.15);
			
			float px = center[0] + Math.Cos(angleRad) * radiusJitter;
			float pz = center[2] + Math.Sin(angleRad) * radiusJitter;
			float py = GetGame().GetWorld().GetSurfaceY(px, pz);
			
			SCR_AIWaypoint wp = SpawnMoveWP(Vector(px, py, pz));
			if (wp)
				grp.MoveTo(wp, worldTime);
		}
	}
	
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
	
	//! Dispatcher -- pilih strategi patrol defensif. Kalau ada objective captured lain
	//! yang berdekatan, ada kesempatan (m_fLinkPatrolChance) buat patrol objective-link
	//! ketimbang muter di 1 objective doang. Kalau gak ada yang deket, otomatis fallback
	//! ke perimeter (AssignObjectiveLinkPatrol sendiri udah handle fallback ini).
	protected void AssignDefensivePatrol(DCO_GroupUtilityComponent grp, CMD_AICommanderObjectiveComponent homeObj, float worldTime)
	{
		if (Math.RandomFloat01() < m_fLinkPatrolChance)
			AssignObjectiveLinkPatrol(grp, homeObj, worldTime);
		else
			AssignPatrolAroundObjective(grp, homeObj.GetOwner().GetOrigin(), homeObj.GetRadius(), worldTime);
	}
	// === END ADDED ===
	
	// === ADDED: Dedicated Suppress Group helper ===
	// Cek apakah objective ini udah punya grup SUPPRESS yang di-assign, biar gak
	// nyoba assign ulang tiap Think() cycle.
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
	// === END ADDED ===
	
	// === ADDED: Synchronized Attack ===
	//! Fase GATHERING -- isi slot ASSAULT ke STAGING position (bukan langsung ke
	//! objective), loop sampe SEMUA slot required keisi atau gak ada grup available
	//! lagi. Beda sama TrySendAssaultWithSlots yang cuma isi 1 slot per panggilan --
	//! di sini kita mau ngumpulin secepat mungkin dalam 1 cycle biar readiness check
	//! di caller bisa kejadian lebih cepet.
	protected void TryGatherForSynchronizedAssault(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		vector objPos = obj.GetOwner().GetOrigin();
		vector base   = GetOwner().GetOrigin();
		vector axis   = objPos - base;
		axis          = Vector(axis[0], 0.0, axis[2]);
		axis          = axis.Normalized();
		float dist    = vector.Distance(base, objPos);

		vector stagingPos = base + axis * (dist * Math.RandomFloatInclusive(0.15, 0.4));
		stagingPos[1]      = GetGame().GetWorld().GetSurfaceY(stagingPos[0], stagingPos[2]);

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

	//! Fase RELEASE -- semua grup ASSAULT yang lagi staging buat objective ini
	//! (GetGroupObjective() == obj) dikasih waypoint SEARCH ke objective beneran,
	//! SEMUANYA di cycle Think() yang sama -- ini yang bikin efek "nyerang bareng"
	//! (bukan piecemeal kayak sebelumnya).
	protected void ReleaseSynchronizedAssault(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		int releasedCount = 0;

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp)
				continue;

			if (grp.GetGroupObjective() != obj)
				continue;

			if (grp.GetGroupRole() != CMD_EGroupRole.ASSAULT)
				continue; // flank/suppress udah punya posisi sendiri, gak perlu di-release ke objective

			grp.CompleteAllWaypoints();

			array<SCR_AIWaypoint> searchWPs = {};
			GenerateSearchWaypoints(obj.GetOwner().GetOrigin(), obj.GetRadius(), searchWPs);
			foreach (SCR_AIWaypoint wp : searchWPs)
				grp.MoveTo(wp, worldTime);

			releasedCount++;
		}

		Print(string.Format("[%1] SYNCHRONIZED ASSAULT RELEASED -> %2 (%3 grup)",
			m_sCommanderUID, obj.GetOwner().GetName(), releasedCount));
	}
	// === END ADDED ===
	
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
	 
		// === ADDED: Dedicated Suppress Group ===
		// Kalau objective butuh >1 grup, sisihin 1 grup (di LUAR hitungan required slot
		// assault -- ini bonus/support, bukan gantiin manpower assault) buat diem di
		// posisi ber-LOS ke objective dan suppress, sementara grup lain push masuk.
		// Posisi dicari pakai CMD_ReconSpotFinder yang udah ada scoring LOS-nya (dipake
		// juga di TrySendRecon), jadi gak perlu bikin LOS-scoring baru dari nol.
		if (required >= 2 && !ObjectiveHasSuppressGroup(obj))
		{
			DCO_GroupUtilityComponent suppressGrp = FindBestIdleGroupForRole(CMD_EGroupRole.SUPPRESS, objPos);
			if (suppressGrp && !suppressGrp.IsPlayerGroup() && CanCommitGroup(suppressGrp))
			{
				vector suppressPos = CMD_ReconSpotFinder.FindBestReconSpot(GetOwner().GetOrigin(), objPos, 180.0, objRad * 1.5, 12);
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
		// === END ADDED ===
 
		if (slotsLeft > 0)
		{
			DCO_GroupUtilityComponent assaultGrp = FindBestIdleGroupForRole(CMD_EGroupRole.ASSAULT, objPos);
			if (assaultGrp)
			{
				if (assaultGrp.IsPlayerGroup())
				{
					//CMD_TaskNotifier.Notify(assaultGrp.GetOwner(), "ASSAULT " + obj.GetOwner().GetName(), obj.GetOwner().GetOrigin(), CMD_ETaskType.CAPTURE);
					return;
				}
				// === ADDED: Manpower Budget gate ===
				if (!CanCommitGroup(assaultGrp))
				{
					//Print(string.Format("[%1] Manpower budget insufficient — skip ASSAULT slot di %2", m_sCommanderUID, obj.GetOwner().GetName()));
				}
				else
				{
				// === END ADDED ===
				assaultGrp.CompleteAllWaypoints();
				if (TryAssignTransport(assaultGrp, objPos, worldTime))
    				return;
				array<SCR_AIWaypoint> searchWPs = {};
				GenerateSearchWaypoints(obj.GetOwner().GetOrigin(), obj.GetRadius(), searchWPs);
				if (searchWPs.Count() > 0)
				{
					foreach (SCR_AIWaypoint wp : searchWPs)
    					assaultGrp.MoveTo(wp, worldTime);
					
					assaultGrp.SetGroupRole(CMD_EGroupRole.ASSAULT);
					if (assaultGrp.GetGroupObjective() != obj)
					{
						assaultGrp.SetGroupObjective(obj);
						obj.SetObjectiveGroup(m_sFactionKey, 1);
						slotsLeft = slotsLeft - 1;
					}
					
					objPos    		 = rand.GenerateRandomPointInRadius(5, obj.GetRadius(), obj.GetOwner().GetOrigin(), false);
					objPos[1]		 = GetGame().GetWorld().GetSurfaceY(objPos[0], objPos[2]);

					/*Print(string.Format("[%1] ASSAULT (slotted require %3): %2 → %4 Slots Left %6",
						m_sCommanderUID,
						obj.GetCurrentAssignedGroupCount(m_sFactionKey),
						required,
						assaultGrp.GetOwner().GetName(),
						obj.GetOwner().GetName(),
						slotsLeft));*/
				}
				} // === ADDED: closes CanCommitGroup else block ===
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
	
	protected void ThinkCaptureProgress(float worldTime)
	{
	    foreach (CMD_AICommanderObjectiveComponent obj : m_aObjective)
	    {
	        if (!obj)
	            continue;
	
	        CMD_EObjectiveState state = obj.GetObjectiveState(m_sFactionKey);
	
	        if (obj.CountNearbyUnits(obj.GetRadius(), m_sFactionKey, true) < obj.CountNearbyUnits(obj.GetRadius(), m_sFactionKey, false))
	            obj.SetObjectiveState(m_sFactionKey, CMD_EObjectiveState.ASSIGNED);
	
	        if (state == CMD_EObjectiveState.COMPLETED || state == CMD_EObjectiveState.FAILED)
	            continue;
	
	        if (obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
	            continue;
	
	        if (state != CMD_EObjectiveState.ASSIGNED)
	            continue;
	
	        if (!obj.IsCaptureTimerRunning(m_sFactionKey))
	        {
	            if (obj.GetCurrentAssignedGroupCount(m_sFactionKey) > 0)
	            {
	                if (obj.CountNearbyUnits(obj.GetRadius(), m_sFactionKey, true) > 0)
	                    obj.StartCaptureTimer(m_sFactionKey, worldTime);
	            }
	            continue;
	        }
	
	        float progress = obj.GetCaptureProgress(m_sFactionKey, worldTime);
	
	        if (obj.IsStalemate(m_sFactionKey, worldTime))
	        {
	            HandleStalemateObjective(obj, worldTime);
	        }
	
	        if (obj.IsCaptureTimerComplete(m_sFactionKey, worldTime))
	        {
	            obj.SetCapturedBy(m_sFactionKey, true);
	            obj.ResetAssignedGroupCount(m_sFactionKey);
	            obj.ResetStalemateTracking(); // ← reset tracking setelah captured
	        }
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
	        
	        Print(string.Format("[Artillery] Shell %1 impact at %2 (radius: %3m from center)",
	            i + 1, shellImpact.ToString(), radius.ToString()));
	        
	        // TODO: spawn explosion / effect di shellImpact
	    }
	}
	
	
	
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer())
			return;
 
		m_fThinkTimer += timeSlice;
		 m_fCaptureCheckTimer += timeSlice;

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
}