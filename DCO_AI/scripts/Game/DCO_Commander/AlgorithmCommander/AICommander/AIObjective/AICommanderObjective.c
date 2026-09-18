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
	[Attribute("50.0", UIWidgets.EditBox, "Base strategic value (0–100). Makin tinggi makin penting.", category: "Objective")]
	protected float m_fBaseValue;

	[Attribute("", UIWidgets.EditBox, "Nama Objective", category: "Objective")]
	protected string m_sObjectiveName;

	[Attribute("150.0", UIWidgets.EditBox, "Radius (meter) untuk deteksi musuh di sekitar objective ini.", category: "Objective")]
	protected float m_fThreatRadius;

	[Attribute("200.0", UIWidgets.EditBox, "Radius (meter) untuk cek friendly presence.", category: "Objective")]
	protected float m_fFriendlyRadius;

	[Attribute("30.0", UIWidgets.Slider, "Radius of the Objective", "1.0 300.0 1.0")]
	protected float m_fRadius;

	[Attribute("400.0", UIWidgets.EditBox, "Radius (meter) intel coverage yang di-provide objective ini ke objective LAIN di sekitarnya. Cuma relevan kalau ObjectiveType == RECON.", category: "Intel")]
	protected float m_fIntelCoverageRadius;

	[Attribute("2500.0", UIWidgets.EditBox, "Jarak (meter) dari commander di mana proximity bonus abis (0 di jarak ini, penuh di jarak 0).", category: "Priority")]
	protected float m_fMaxRelevantDistance;

	[Attribute("20.0", UIWidgets.EditBox, "Penalti maksimum buat objective di luar Max Relevant Distance. Makin besar makin ogah commander ngambil objective jauh. 0 = jarak jauh gak dibedain sama sekali.", category: "Priority")]
	protected float m_fDistantFalloffWeight;

	[Attribute("100.0", UIWidgets.EditBox, "Poin maksimum dari kedekatan, didapat waktu jarak ke commander nol. Ini penentu utama urutan objective. Naikin kalau objective dekat masih kalah sama objective jauh yang base value-nya besar.", category: "Priority")]
	protected float m_fProximityWeight;

	[Attribute("40.0", UIWidgets.EditBox, "Poin per unit musuh, SEBELUM diskala jarak. Ancaman sifatnya pengubah -- kalau angkanya bikin objective bermusuh selalu menang tanpa peduli jarak, berarti kegedean.", category: "Priority")]
	protected float m_fThreatWeight;

	[Attribute("2.0", UIWidgets.EditBox, "Pangkat peluruhan ancaman terhadap kedekatan. 1 = ancaman luruh sama cepat dengan kedekatan. 2 = jauh lebih cepat (di setengah radius relevan, kedekatan 50% tapi ancaman 25%). Makin besar makin cuek sama musuh jauh.", category: "Priority")]
	protected float m_fThreatFalloffPower;

	[Attribute("0.05", UIWidgets.Range, "Lantai skala ancaman. Mencegah musuh di luar radius relevan dianggap nol sama sekali -- kalau nol, dua objective jauh yang setara jadi gak bisa dibedain walaupun satunya penuh musuh.", params: "0 1 0.01", category: "Priority")]
	protected float m_fThreatFalloffFloor;

	[Attribute("8", UIWidgets.EditBox, "Batas jumlah musuh yang dihitung buat skor. Tanpa batas, lantai peluruhan ancaman (yang niatnya cuma pemecah seri) dikali jumlah yang gak berhingga: objective bergarnisun 50 unit ngasih commander di seberang peta ~90 poin dari jarak tak terhingga, dan itu ngalahin objective dekat. Naikin kalau pertempuran besar jadi kurang diprioritaskan; turunin kalau commander masih ketarik ke garnisun jauh.", category: "Priority")]
	protected int m_iThreatCountCap;

	[Attribute("0", UIWidgets.ComboBox, "Tipe objective ini", "", ParamEnumArray.FromEnum(CMD_EObjectiveType))]
	CMD_EObjectiveType m_eObjectiveType;

	[Attribute("60.0", UIWidgets.EditBox, "Detik yang dibutuhkan untuk capture (groups harus di area)", category: "Objective")]
	protected float m_fCaptureHoldDuration;

	[Attribute("20.0", UIWidgets.EditBox, "Detik radius harus bersih dari musuh sebelum objective dianggap uncontested. Commander pakai ini buat mutusin perlu staging atau langsung masuk.", category: "Objective")]
	protected float m_fUncontestedDwell;

	[Attribute("5.0", UIWidgets.EditBox, "Interval (detik) objective nyensus isi radius-nya sendiri. Makin kecil makin responsif, makin sering juga spatial query-nya.", category: "Objective")]
	protected float m_fPresenceScanInterval;

	[Attribute("8", UIWidgets.EditBox, "Berapa unit yang dibutuhkan buat laju geser kontrol PENUH. Di bawah ini lajunya proporsional -- 1 unit dari 8 berarti 1/8 laju. Naikin kalau mau objective besar butuh pasukan besar.", category: "Objective Control")]
	protected int m_iFullSpeedUnits;

	[Attribute("100.0", UIWidgets.Range, "Persen kontrol yang dibutuhkan buat jadi OWNER objective ini.", params: "0 100 1", category: "Objective Control")]
	protected float m_fOwnershipGainThreshold;

	[Attribute("50.0", UIWidgets.Range, "Persen kontrol di mana status owner HILANG. Jarak ke Gain Threshold di atas adalah histeresisnya -- tanpa jarak itu status owner kedip-kedip tiap ada gangguan kecil.", params: "0 100 1", category: "Objective Control")]
	protected float m_fOwnershipLossThreshold;

	[Attribute("1.0", UIWidgets.EditBox, "Persen per detik kontrol bergeser balik ke pemilik terakhir waktu radius kosong. 0 = kontrol beku waktu gak ada siapa-siapa.", category: "Objective Control")]
	protected float m_fEmptyDecayRate;

	[Attribute("1", UIWidgets.CheckBox, "Semua faction di Faction Manager boleh contest objective ini.", category: "Objective Control")]
	protected bool m_bAllowAllFactions;

	[Attribute(desc: "Kalau Allow All Factions dimatiin: cuma faction key di daftar ini yang boleh contest. Faction lain dianggap GAK ADA -- unitnya gak ngeblok dan gak ikut ngitung.", category: "Objective Control")]
	protected ref array<string> m_aAllowedFactions;

	[Attribute("5.0", UIWidgets.EditBox, "Stalemate: kontrol berubah kurang dari sekian persen...", category: "Objective Control")]
	protected float m_fStalematePercentThreshold;

	[Attribute("120.0", UIWidgets.EditBox, "...selama sekian detik, baru dianggap stalemate.", category: "Objective Control")]
	protected float m_fStalemateSeconds;

	[Attribute(desc: "Commander UID yang megang objective ini di awal scenario. Kosongin buat Neutral. Cuma boleh SATU commander.", category: "Objective Control")]
	protected string m_sInitialOwnerCommanderUID;

	[Attribute("2", UIWidgets.EditBox, "Berapa group yang di-assign untuk defend setelah captured", category: "Objective")]
	protected int m_iDefendGroupCount;

	[Attribute("4", UIWidgets.EditBox, "Berapa group yang di-assign untuk attack objective", category: "Objective")]
	protected int m_iMaxGroupCount;

	[Attribute("", UIWidgets.Auto, "Blacklisted Commander to not process this objective", category: "Objective")]
	protected ref array<string> m_sBlacklistedCo;

	[Attribute("", UIWidgets.Auto, "Set Captured by Commander for this objective", category: "Objective")]
	protected ref array<string> m_sCapturedCo;

	[Attribute("0", UIWidgets.EditBox, "Debug Buat Objective ini", category: "Objective Debug")]
	protected bool m_bDebugMode;

	protected ref map<FactionKey, ref array<ref DCO_SectorGarrison>> m_mSectorGarrison = new map<FactionKey, ref array<ref DCO_SectorGarrison>>();
	protected ref map<FactionKey, int>   m_mSectorCount  = new map<FactionKey, int>();
	protected ref map<FactionKey, float> m_mSectorOffset = new map<FactionKey, float>();

	protected ref array<ref Shape> m_aDebugShapes = new array<ref Shape>();

	protected ref array<ref DebugTextWorldSpace> m_aDebugTexts = new array<ref DebugTextWorldSpace>();

	[Attribute("0.5", UIWidgets.EditBox, "Interval (detik) gambar ulang overlay debug objective ini.", category: "Objective Debug")]
	protected float m_fDebugRefreshInterval;

	protected float m_fDebugTimer = 0.0;

	protected float m_fLastProgressTime   = 0.0;
	protected float m_fStaleStartTime     = 0.0;

	[Attribute("300.0", UIWidgets.EditBox, "Detik tanpa progress sebelum dianggap stalemate", category: "Objective")]
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
	protected float m_fControlAtLastCheck    = 0.0;
	protected bool  m_bInitialOwnerResolved  = false;

	protected ref map<FactionKey, int> m_mFactionPresence = new map<FactionKey, int>();
	protected int   m_iTotalPresence  = 0;
	protected float m_fLastScanTime   = 0.0;
	protected float m_fScanTimer      = 0.0;
	protected bool  m_bHasScanned     = false;

	// === ADDED: Census Cache + Active Gate ===
	// Satu spatial query per objective per m_fPresenceScanInterval, di radius terbesar
	// yang dipakai siapapun (GetCensusRadius). Semua count (commander maupun
	// ScanPresence) baca dari hasil yang sama dan cuma nge-filter jarak.
	[Attribute("60.0", UIWidgets.EditBox, "Detik objective tetap ACTIVE setelah terakhir diproses commander. Lewat dari ini objective IDLE: scan presence dan geser kontrol berhenti total. Harus lebih besar dari think interval commander terlama (default 30s x personality sampai 1.5 = 45s). 0 = gate mati, objective selalu active.", category: "Objective")]
	protected float m_fIdleTimeout;

	protected ref array<FactionKey> m_aCensusFaction = {};
	protected ref array<float>      m_aCensusDistSq  = {};
	protected float m_fCensusTime   = 0.0;
	protected float m_fCensusRadius = 0.0;
	protected bool  m_bCensusValid  = false;

	// Target sementara buat QueryCallbackCensus -- cuma hidup selama CollectCensus jalan.
	protected array<FactionKey> m_aCollectFaction;
	protected array<float>      m_aCollectDistSq;
	protected ref set<IEntity>  m_CollectSeen = new set<IEntity>();
	protected vector            m_vCollectCenter;

	protected ref map<FactionKey, int> m_mCountScratch = new map<FactionKey, int>();

	protected float m_fLastProcessedTime     = 0.0;
	protected bool  m_bEverProcessed         = false;
	protected bool  m_bSuppressProcessedMark = false;
	// === END ADDED ===

	ref map<FactionKey, bool>  m_mIsCaptured       = new map<FactionKey, bool>();

	ref map<FactionKey, CMD_EObjectiveState> m_mObjectiveState = new map<FactionKey, CMD_EObjectiveState>();
	ref map<FactionKey, int> m_mObjectiveAssignedGroup = new map<FactionKey, int>();
	protected ref map<FactionKey, DCO_GroupUtilityComponent> m_mReconGroup = new map<FactionKey, DCO_GroupUtilityComponent>();

	protected ref map<FactionKey, bool> m_mLostStatus = new map<FactionKey, bool>();

	IEntity m_OwnerEntity;

	protected ref map<string, float> m_mCachedScore   = new map<string, float>();
	protected ref map<string, float> m_mScoreCacheAge = new map<string, float>();

	protected ref map<string, string> m_mScoreBreakdown = new map<string, string>();
	static const float CACHE_DURATION = 3.0;

	protected float m_fLastContestedTime = 0.0;

	float GetRadius()
	{
		return m_fRadius;
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
	    // === ADDED ===
	    MarkProcessed(worldTime);
	    // === END ADDED ===

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
	    // === ADDED ===
	    MarkProcessed(worldTime);
	    // === END ADDED ===

	    if (!m_bHasScanned)
	        return GetCaptureStatus(fk);

	    int friendlyCount = GetFactionPresence(fk);
	    int enemyCount    = 0;

	    foreach (FactionKey pk, int pu : m_mFactionPresence)
	    {
	        if (pk == fk || !CanFactionContest(pk))
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

		if (Math.AbsFloat(now - m_fControlAtLastCheck) >= m_fStalematePercentThreshold)
		{
			m_fControlAtLastCheck    = now;
			m_fLastControlChangeTime = worldTime;
			return false;
		}

		if (m_fLastControlChangeTime <= 0.0)
		{
			m_fLastControlChangeTime = worldTime;
			return false;
		}

		return (worldTime - m_fLastControlChangeTime) >= m_fStalemateSeconds;
	}

	void ResetStalemateTracking()
	{
	    m_fLastProgressTime = 0.0;
	}

	bool IsCommanderBlackListed(string cuid)
	{
		if (m_sBlacklistedCo.Contains(cuid))
			return true;
		return false;
	}

	bool IsScouted(FactionKey fk)
	{
	    CMD_EObjectiveState state = GetObjectiveState(fk);
	    return state != CMD_EObjectiveState.PENDING;
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

	CMD_EObjectiveAction GetObjectiveAction(FactionKey fk)
	{
		if (m_eObjectiveType == CMD_EObjectiveType.RECON)
			return CMD_EObjectiveAction.RECON;

		if (IsCapturedBy(fk, string.Empty))
			return CMD_EObjectiveAction.DEFEND;

		return CMD_EObjectiveAction.CAPTURE;
	}

	protected void InitializeObjective()
	{
		if (!AICommander_ManagerComponent.GetInstance())
			return;
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

	    // === ADDED: di-score = sedang diproses commander, termasuk kalau hasilnya dari cache ===
	    MarkProcessed(worldTime);
	    // === END ADDED ===

	    string cacheKey = forFaction + "|" + commanderUID;

	    float cacheAge;
	    if (m_mScoreCacheAge.Find(cacheKey, cacheAge) && (worldTime - cacheAge) < CACHE_DURATION)
	    {
	        float cachedScore;
	        if (m_mCachedScore.Find(cacheKey, cachedScore))
	            return cachedScore;
	    }

	    // === MODIFIED: baca dari census, bukan sphere query sendiri ===
	    int enemyCount;
	    int friendlyAtThreatRadius;
	    CountNearbyUnitsCached(m_fThreatRadius, forFaction, friendlyAtThreatRadius, enemyCount);
	    // === END MODIFIED ===

	    float contestedRaw = 0.0;

	    if (m_fLastContestedTime > 0.0)
	    {
	        float elapsed = worldTime - m_fLastContestedTime;
	        if (elapsed < 120.0)
	            contestedRaw = Math.Lerp(25.0, 0.0, elapsed / 120.0);
	    }

	    // === MODIFIED: baca dari census, bukan sphere query sendiri ===
	    int friendlyCount;
	    int enemyAtFriendlyRadius;
	    CountNearbyUnitsCached(m_fFriendlyRadius, forFaction, friendlyCount, enemyAtFriendlyRadius);
	    // === END MODIFIED ===
	    float friendlyPenalty = Math.Clamp(friendlyCount * 5.0, 0.0, 30.0);

	    float assignedPenalty = 0.0;
	    if (currentState == CMD_EObjectiveState.ASSIGNED)
	        assignedPenalty = 15.0;

	    float score = m_fBaseValue - friendlyPenalty - assignedPenalty;

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

	    float threatTerm = cappedEnemyCount * m_fThreatWeight * threatFactor * focusMod;

	    score += threatTerm;

	    float finalScore = Math.Max(score, 0.0);

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

		// === MODIFIED: ResolveInitialOwner dipindah ke atas timer. Objective IDLE gak
		// pernah scan, tapi owner awal tetap harus ke-set -- tanpa itu IsCapturedBy()
		// false terus, commander defensive gak pernah nyentuh objective ini, dan
		// objective-nya gak pernah jadi ACTIVE. Setelah resolved fungsinya langsung return.
		ResolveInitialOwner();

		m_fScanTimer += timeSlice;

		if (m_fScanTimer < m_fPresenceScanInterval)
			return;

		// Timer tetap di-reset walau IDLE -- elapsed gak pernah numpuk jadi lonjakan
		// waktu objective aktif lagi. Waktu selama IDLE memang gak dihitung.
		float elapsed  = m_fScanTimer;
		m_fScanTimer   = 0.0;

		float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;

		bool active = IsProcessedActive(worldTime);

		if (active)
			ScanPresenceFromCensus(worldTime);
		else if (m_bDebugMode)
			m_aDebugShapes.Clear(); // dulu di-clear ScanPresence; tanpa ini shape numpuk tiap interval

		UpdateObjectiveDebug(worldTime);

		if (active)
			AdvanceControl(worldTime, elapsed);
		// === END MODIFIED ===

	}

	protected void ResolveInitialOwner()
	{
		if (m_bInitialOwnerResolved)
			return;

		if (m_sInitialOwnerCommanderUID.IsEmpty())
		{
			m_bInitialOwnerResolved = true;
			return;
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

	//------------------------------------------------------------------------------------------------
	// === ADDED: Census Cache ===
	//! Radius census = radius terbesar yang dipakai caller. Intel coverage cuma ikut
	//! kalau objective ini RECON (default-nya 400m -- gak perlu dibayar objective biasa).
	//! Caller yang minta radius lebih besar dari ini jatuh ke query langsung.
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

	//! Satu sphere query, hasilnya faction + jarak kuadrat per karakter.
	protected void CollectCensus(float radius, notnull array<FactionKey> outFaction, notnull array<float> outDistSq)
	{
		outFaction.Clear();
		outDistSq.Clear();
		m_CollectSeen.Clear();

		m_aCollectFaction = outFaction;
		m_aCollectDistSq  = outDistSq;
		m_vCollectCenter  = GetOwner().GetOrigin();

		GetGame().GetWorld().QueryEntitiesBySphere(m_vCollectCenter, radius, QueryCallbackCensus);

		m_aCollectFaction = null;
		m_aCollectDistSq  = null;
		m_CollectSeen.Clear();
	}

	//! Rebuild kalau census sudah berumur >= m_fPresenceScanInterval.
	//! Interval <= 0 = cache mati, tiap panggilan query (sama kayak perilaku lama).
	protected void EnsureCensus(float worldTime)
	{
		float radius = GetCensusRadius();

		if (m_bCensusValid
			&& m_fPresenceScanInterval > 0.0
			&& m_fCensusRadius == radius
			&& (worldTime - m_fCensusTime) < m_fPresenceScanInterval)
			return;

		CollectCensus(radius, m_aCensusFaction, m_aCensusDistSq);

		m_fCensusRadius = radius;
		m_fCensusTime   = worldTime;
		m_bCensusValid  = true;
	}

	//! Friendly & enemy sekaligus dari census.
	//! Semantik: faction sendiri atau yang allied = friendly, sisanya = enemy.
	void CountNearbyUnitsCached(float radius, FactionKey factionKey, out int friendlyCount, out int enemyCount)
	{
		friendlyCount = 0;
		enemyCount    = 0;

		float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
		MarkProcessed(worldTime);

		// Diambil sekali, bukan per entity. Kalau gagal: 0/0, sama kayak versi lama
		// (dulu tiap entity di-skip kalau faction sendiri gak ketemu).
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
			EnsureCensus(worldTime);
			factions = m_aCensusFaction;
			distSq   = m_aCensusDistSq;
		}
		else
		{
			factions = new array<FactionKey>();
			distSq   = new array<float>();
			CollectCensus(radius, factions, distSq);
		}

		// Kelompokkan per faction dulu -- relasi faction cukup di-resolve per faction
		// (biasanya 2-4), bukan per karakter.
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

	//! Isi m_mFactionPresence dari census yang di-filter ke m_fRadius.
	//! Satu-satunya penulis presence (dipanggil EOnFrame dan MarkProcessed).
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
			Print(total.ToString() + " < Number of counted Entity Inside > " + m_sObjectiveName);
			int flags = ShapeFlags.TRANSP | ShapeFlags.WIREFRAME;
			m_aDebugShapes.Insert(Shape.CreateSphere(Color.BLUE, flags, GetOwner().GetOrigin(), m_fRadius));
		}

		m_iTotalPresence = total;
		m_fLastScanTime  = worldTime;
		m_bHasScanned    = true;
	}
	// === END ADDED ===

	//------------------------------------------------------------------------------------------------
	// === ADDED: Active Gate ===
	//! Dipanggil tiap kali commander memproses objective ini. Kalau objective baru
	//! bangun dari IDLE, presence langsung di-refresh -- data lama bisa berumur menit.
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
	// === END ADDED ===

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

	void ReportCommanderPresence(string cuid, FactionKey fk, int groupCount, float dt)
	{
		if (cuid.IsEmpty() || groupCount <= 0 || dt <= 0.0)
			return;

		if (!CanFactionContest(fk))
			return;

		float score;
		if (!m_mCommanderContribution.Find(cuid, score))
			score = 0.0;

		m_mCommanderContribution.Set(cuid, score + (groupCount * dt));
		m_mCommanderFaction.Set(cuid, fk);
	}

	void AdvanceControl(float worldTime, float dt)
	{
		if (!m_bHasScanned || dt <= 0.0)
			return;

		FactionKey winner    = "";
		int winnerUnits      = 0;
		int runnerUpUnits    = 0;

		foreach (FactionKey k, int units : m_mFactionPresence)
		{
			if (units <= 0 || !CanFactionContest(k))
				continue;

			if (units > winnerUnits)
			{
				runnerUpUnits = winnerUnits;
				winnerUnits   = units;
				winner        = k;
			}
			else if (units > runnerUpUnits)
			{
				runnerUpUnits = units;
			}
		}

		if (winnerUnits <= 0)
		{

			DecayTowardLastOwner(dt);
			ResolveOwnership(worldTime);
			return;
		}

		float baseRate       = 100.0 / Math.Max(m_fCaptureHoldDuration, 1.0);
		float unitScale      = Math.Clamp(winnerUnits / Math.Max(m_iFullSpeedUnits, 1), 0.0, 1.0);
		float contestPenalty = (winnerUnits - runnerUpUnits) / winnerUnits;

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

		if (m_sOwnerFaction.IsEmpty())
		{
			foreach (FactionKey k, float v : m_mControl)
			{
				if (v >= m_fOwnershipGainThreshold)
				{
					m_sOwnerFaction     = k;
					m_sLastOwnerFaction = k;
					break;
				}
			}
		}
		else if (GetControl(m_sOwnerFaction) < m_fOwnershipLossThreshold)
		{
			m_sOwnerFaction = previousOwner;
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

	bool TransferOwnerCommander(string newUID)
	{
		if (newUID.IsEmpty() || m_sOwnerFaction.IsEmpty())
			return false;

		FactionKey cfk;
		if (!m_mCommanderFaction.Find(newUID, cfk) || cfk != m_sOwnerFaction)
			return false;

		m_sOwnerCommanderUID = newUID;
		return true;
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

		m_aDebugShapes.Insert(Shape.CreateSphere(color, flags, pos, DCO_DebugDraw.MARKER_BIG));

		DrawDebugControlBar(Vector(pos[0], pos[1] + 8.0, pos[2]), flags);

		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
			Vector(pos[0], pos[1] + 30.0, pos[2]), BuildDebugHeader(),   21.0, color));

		// === MODIFIED: overlay manggil ComputePriorityScore -- jangan sampai itu
		// dianggap "commander memproses" dan objective IDLE kebangun cuma gara-gara debug.
		m_bSuppressProcessedMark = true;
		string scoreText = BuildDebugCommanderScores(worldTime);
		m_bSuppressProcessedMark = false;

		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
			Vector(pos[0], pos[1] + 41.0, pos[2]), scoreText, 16.0,
			DCO_DebugDraw.COLOR_COMMANDER));
		// === END MODIFIED ===

		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
			Vector(pos[0], pos[1] + 21.0, pos[2]), BuildDebugControl(),  16.0, color));

		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
			Vector(pos[0], pos[1] + 12.0, pos[2]), BuildDebugPresence(worldTime), 16.0, color));

		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
			Vector(pos[0], pos[1] +  2.0, pos[2]), BuildDebugTuning(),   15.0, color));
	}

	protected void DrawDebugControlBar(vector origin, int flags)
	{
		const int   SLOTS   = 20;
		const float SPACING = 1.3;
		const float RADIUS  = 0.5;

		array<int> slotColors = {};
		for (int s = 0; s < SLOTS; s++)
			slotColors.Insert(DCO_DebugDraw.COLOR_NEUTRAL);

		int cursor = 0;
		foreach (FactionKey k, float v : m_mControl)
		{
			int take = Math.Round(v / 100.0 * SLOTS);
			int col  = DCO_DebugDraw.FactionColor(k);

			for (int n = 0; n < take; n++)
			{
				if (cursor >= SLOTS)
					break;

				slotColors.Set(cursor, col);
				cursor = cursor + 1;
			}
		}

		float startX = origin[0] - ((SLOTS - 1) * SPACING * 0.5);

		for (int i = 0; i < SLOTS; i++)
		{
			vector p = Vector(startX + (i * SPACING), origin[1], origin[2]);
			m_aDebugShapes.Insert(Shape.CreateSphere(slotColors.Get(i), flags, p, RADIUS));
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

		// === ADDED: status gate + census ===
		if (IsProcessedActive(worldTime))
			body = body + "\nstate ACTIVE";
		else
			body = body + string.Format("\nstate IDLE (gak diproses > %1s)", DCO_DebugDraw.F1(m_fIdleTimeout));

		body = body + string.Format("\ncensus r %1   %2 entries",
			DCO_DebugDraw.M(GetCensusRadius()),
			m_aCensusFaction.Count());
		// === END ADDED ===

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

		float toleranceDist = Math.Max(m_fRadius * 2.0, 60.0);
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
		// === ADDED: commander defensive ngecek objective miliknya = memproses ===
		MarkProcessed(GetGame().GetWorld().GetWorldTime() / 1000.0);
		// === END ADDED ===

		return CheckIsItLost(fk);
	}

	bool CheckIsItLost(FactionKey fk)
	{
		return !m_sLastOwnerFaction.IsEmpty()
			&& m_sLastOwnerFaction == fk
			&& !IsOwnedBy(fk);
	}

	protected void MarkLost(FactionKey fk)
	{
		if (!m_mLostStatus.Contains(fk))
			m_mLostStatus.Insert(fk, true);
		else
			m_mLostStatus.Set(fk, true);

		SetCapturedBy(fk, false);

		ClearCaptureTimer(fk);

		Print(string.Format("[CMD_Objective] %1 LOST by %2", GetOwner().GetName(), fk));
	}

	void ResetLostStatus(FactionKey fk)
	{
		if (m_mLostStatus.Contains(fk))
			m_mLostStatus.Set(fk, false);
	}

	bool IsLost(FactionKey fk)
	{
		bool val;
		if (m_mLostStatus.Find(fk, val))
			return val;
		return false;
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
			m_sOwnerFaction = fk;
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