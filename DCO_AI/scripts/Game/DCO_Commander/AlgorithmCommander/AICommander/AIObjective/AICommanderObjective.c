//! Hasil penilaian objective terhadap satu faction. Objective yang nentuin ini
//! sendiri dari kondisi radius-nya -- commander cuma membacanya, gak nyimpulin
//! dari fase assault yang lagi dia jalanin.
enum DCO_ECaptureStatus
{
	IDLE      = 0,   //!< timer mati atau gak ada siapa-siapa di radius
	CAPTURING = 1,   //!< radius bersih dari musuh, akumulator maju
	CONTESTED = 2    //!< ada musuh di radius, akumulator di-reset tiap tick
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
	
	// === ADDED: bobot penalti buat objective DI LUAR m_fMaxRelevantDistance. Penalti
	// ini asimptotik -- makin jauh makin mendekat ke nilai ini tapi gak pernah lewat,
	// jadi objective jauh tetap punya urutan relatif sesamanya dan tetap mungkin
	// kepilih. Set 0 kalau mau balik ke perilaku lama (semua yang jauh dianggap sama).
	[Attribute("20.0", UIWidgets.EditBox, "Penalti maksimum buat objective di luar Max Relevant Distance. Makin besar makin ogah commander ngambil objective jauh. 0 = jarak jauh gak dibedain sama sekali.", category: "Priority")]
	protected float m_fDistantFalloffWeight;
	// === END ADDED ===

	// === ADDED: bobot skor prioritas ===
	// Dipisah dari m_fBaseValue supaya "seberapa berharga objective ini" dan
	// "seberapa penting dia dekat" jadi dua knob terpisah. Dulu keduanya nempel:
	// proximityBonus dikali base value, jadi objective bernilai rendah cuma bisa
	// dapat sedikit dari posisinya yang dekat.
	[Attribute("100.0", UIWidgets.EditBox, "Poin maksimum dari kedekatan, didapat waktu jarak ke commander nol. Ini penentu utama urutan objective. Naikin kalau objective dekat masih kalah sama objective jauh yang base value-nya besar.", category: "Priority")]
	protected float m_fProximityWeight;

	[Attribute("40.0", UIWidgets.EditBox, "Poin per unit musuh, SEBELUM diskala jarak. Ancaman sifatnya pengubah -- kalau angkanya bikin objective bermusuh selalu menang tanpa peduli jarak, berarti kegedean.", category: "Priority")]
	protected float m_fThreatWeight;

	[Attribute("2.0", UIWidgets.EditBox, "Pangkat peluruhan ancaman terhadap kedekatan. 1 = ancaman luruh sama cepat dengan kedekatan. 2 = jauh lebih cepat (di setengah radius relevan, kedekatan 50% tapi ancaman 25%). Makin besar makin cuek sama musuh jauh.", category: "Priority")]
	protected float m_fThreatFalloffPower;

	[Attribute("0.05", UIWidgets.Range, "Lantai skala ancaman. Mencegah musuh di luar radius relevan dianggap nol sama sekali -- kalau nol, dua objective jauh yang setara jadi gak bisa dibedain walaupun satunya penuh musuh.", params: "0 1 0.01", category: "Priority")]
	protected float m_fThreatFalloffFloor;
	// === END ADDED ===
	
	[Attribute("0", UIWidgets.ComboBox, "Tipe objective ini", "", ParamEnumArray.FromEnum(CMD_EObjectiveType))]
	CMD_EObjectiveType m_eObjectiveType;
 
	[Attribute("60.0", UIWidgets.EditBox, "Detik yang dibutuhkan untuk capture (groups harus di area)", category: "Objective")]
	protected float m_fCaptureHoldDuration;

	// === ADDED: berapa lama radius harus bersih sebelum objective dianggap aman.
	// Dwell-nya ada supaya kontak yang kedip-kedip (musuh ilang sebentar dari query
	// lalu muncul lagi) gak bikin commander bolak-balik ngebatalin persiapan.
	// CATATAN: penilaian jalan di irama m_fCaptureCheckInterval commander (default
	// 15 detik), jadi nilai di bawah itu efektif membulat ke satu tick.
	[Attribute("20.0", UIWidgets.EditBox, "Detik radius harus bersih dari musuh sebelum objective dianggap uncontested. Commander pakai ini buat mutusin perlu staging atau langsung masuk.", category: "Objective")]
	protected float m_fUncontestedDwell;

	[Attribute("5.0", UIWidgets.EditBox, "Interval (detik) objective nyensus isi radius-nya sendiri. Makin kecil makin responsif, makin sering juga spatial query-nya.", category: "Objective")]
	protected float m_fPresenceScanInterval;
	// === END ADDED ===

	// === ADDED: Control Model ===
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
	// === END ADDED ===
 
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

	// === ADDED: teks dipisah dari shape karena lifetime-nya sama tapi tipenya beda --
	// dua-duanya hidup selama referensinya dipegang, jadi harus di-Clear bareng tiap
	// refresh atau gambarnya numpuk.
	protected ref array<ref DebugTextWorldSpace> m_aDebugTexts = new array<ref DebugTextWorldSpace>();

	[Attribute("0.5", UIWidgets.EditBox, "Interval (detik) gambar ulang overlay debug objective ini.", category: "Objective Debug")]
	protected float m_fDebugRefreshInterval;

	protected float m_fDebugTimer = 0.0;
	
	protected float m_fLastProgressTime   = 0.0;
	protected float m_fStaleStartTime     = 0.0;
	
	[Attribute("300.0", UIWidgets.EditBox, "Detik tanpa progress sebelum dianggap stalemate", category: "Objective")]
	protected float m_fStalemateThreshold;
 
	ref map<FactionKey, float> m_mCaptureStartTime = new map<FactionKey, float>();

	// === ADDED: detik hold yang BENERAN terkumpul, plus kapan terakhir dimajuin.
	// m_mCaptureStartTime sekarang cuma berperan sebagai penanda "timer aktif".
	protected ref map<FactionKey, float> m_mCaptureHeld     = new map<FactionKey, float>();
	protected ref map<FactionKey, float> m_mCaptureLastTick = new map<FactionKey, float>();

	//! Arah gerak progress di tick terakhir: 1 maju, 0 beku (kalah jumlah),
	//! -1 mundur (gak ada siapa-siapa). Disimpen supaya overlay debug bisa
	//! nampilin trennya tanpa ngulang spatial query sendiri.
	//! Status hasil penilaian terakhir per faction (DCO_ECaptureStatus).
	protected ref map<FactionKey, int>   m_mCaptureStatus     = new map<FactionKey, int>();

	//! Kapan musuh terakhir kelihatan di radius. Jalan terus, terlepas dari timer
	//! capture aktif atau enggak -- commander butuh ini SEBELUM ada timer sama sekali
	//! buat mutusin objective ini perlu staging atau langsung dimasukin.
	protected ref map<FactionKey, float> m_mLastEnemySeenTime = new map<FactionKey, float>();

	// === ADDED: hasil sensus berkala. Diisi ScanPresence(), dibaca AssessObjective
	// dan overlay debug. Ini satu-satunya sumber angka kehadiran sekarang -- gak ada
	// lagi yang manggil QueryEntitiesBySphere buat nanya "berapa orang di sini".
	// === ADDED: Control Model state ===
	//! Persen kontrol per faction, 0..100. Totalnya <= 100; sisanya Neutral.
	protected ref map<FactionKey, float> m_mControl = new map<FactionKey, float>();

	protected FactionKey m_sOwnerFaction;
	protected FactionKey m_sLastOwnerFaction;
	protected string     m_sOwnerCommanderUID = string.Empty;

	//! Skor grup x detik per commander. Yang tertinggi jadi owner commander waktu
	//! faction-nya nyampe ambang -- "banyak DAN lama", bukan sekadar duluan.
	protected ref map<string, float>      m_mCommanderContribution = new map<string, float>();
	protected ref map<string, FactionKey> m_mCommanderFaction      = new map<string, FactionKey>();

	protected float m_fLastControlChangeTime = 0.0;
	protected float m_fControlAtLastCheck    = 0.0;
	protected bool  m_bInitialOwnerResolved  = false;
	// === END ADDED ===

	protected ref map<FactionKey, int> m_mFactionPresence = new map<FactionKey, int>();
	protected int   m_iTotalPresence  = 0;
	protected float m_fLastScanTime   = 0.0;
	protected float m_fScanTimer      = 0.0;
	protected bool  m_bHasScanned     = false;
	// === END ADDED ===
	// === END ADDED ===
	ref map<FactionKey, bool>  m_mIsCaptured       = new map<FactionKey, bool>();
	
	ref map<FactionKey, CMD_EObjectiveState> m_mObjectiveState = new map<FactionKey, CMD_EObjectiveState>();
	ref map<FactionKey, int> m_mObjectiveAssignedGroup = new map<FactionKey, int>();
	protected ref map<FactionKey, DCO_GroupUtilityComponent> m_mReconGroup = new map<FactionKey, DCO_GroupUtilityComponent>();
	
	protected ref map<FactionKey, bool> m_mLostStatus = new map<FactionKey, bool>();
	
	IEntity m_OwnerEntity;
	
	// === MODIFIED: cache skor dulu cuma 2 field tunggal buat SELURUH objective,
	// padahal skornya dihitung dari commanderPos + forFaction yang beda tiap pemanggil.
	// Commander A (deket) ngitung duluan, commander B (jauh) yang nanya dalam window
	// CACHE_DURATION dapet skor punya A -- lengkap sama proximity bonus posisi A.
	// Beda faction juga saling nimpa. Sekarang cache di-key per (faction|commanderUID).
	protected ref map<string, float> m_mCachedScore   = new map<string, float>();
	protected ref map<string, float> m_mScoreCacheAge = new map<string, float>();
	static const float CACHE_DURATION = 3.0;
	// === END MODIFIED ===
 
	protected float m_fLastContestedTime = 0.0;
	
	float GetRadius()
	{
		return m_fRadius;
	}
	
	// === MODIFIED: penilaian objective sekarang jadi urusan OBJECTIVE-nya sendiri,
	// bukan disimpulin commander dari fase assault-nya.
	//
	// Aturannya cuma satu pertanyaan: ADA MUSUH DI RADIUS ATAU ENGGAK.
	//   ada musuh (berapapun)  -> CONTESTED, akumulator DI-RESET ke nol
	//   gak ada musuh + ada kita -> CAPTURING, akumulator maju
	//   kosong dua-duanya      -> IDLE, timer dimatiin
	//
	// Ini beda dari versi sebelumnya yang mbandingin JUMLAH (friendly >= enemy ->
	// maju, kalah jumlah -> beku). Perbandingan jumlah bikin capture bisa jalan
	// sambil baku tembak, dan progress-nya awet walaupun lagi didorong mundur.
	// Sekarang kehadiran satu musuh aja langsung ngenolin -- kalau mau ngerebut,
	// harus dibersihin dulu, dan hasil pembersihannya kelihatan langsung.
	//
	// Konsekuensi yang disengaja: commander gak perlu lagi "assault sampai bersih
	// baru capture". Begitu radius bersih, statusnya otomatis CAPTURING -- gak
	// peduli synchronized assault-nya udah di-release atau belum.

	// === MODIFIED: sekarang cuma pembungkus tabel kontrol. Dipertahanin namanya
	// supaya call site lama gak perlu disentuh; 0..1 di sini = 0..100 persen kontrol.
	float GetCaptureProgress(FactionKey fk, float worldTime)
	{
	    return Math.Clamp(GetControl(fk) / 100.0, 0.0, 1.0);
	}

	//! Detik hold yang udah terkumpul. Buat overlay debug.
	float GetCaptureHeldSeconds(FactionKey fk)
	{
	    float held;
	    if (m_mCaptureHeld.Find(fk, held))
	        return held;

	    return 0.0;
	}

	//! Status hasil penilaian terakhir. Lihat DCO_ECaptureStatus.
	int GetCaptureStatus(FactionKey fk)
	{
	    int status;
	    if (m_mCaptureStatus.Find(fk, status))
	        return status;

	    return DCO_ECaptureStatus.IDLE;
	}

	//! Berapa detik sejak musuh terakhir kelihatan di radius. -1 kalau belum pernah
	//! dinilai sama sekali.
	float GetSecondsSinceEnemySeen(FactionKey fk, float worldTime)
	{
	    float lastSeen;
	    if (!m_mLastEnemySeenTime.Find(fk, lastSeen))
	        return -1.0;

	    return worldTime - lastSeen;
	}

	//! Objective dianggap bersih kalau udah lewat m_fUncontestedDwell detik sejak
	//! musuh terakhir kelihatan. Dwell-nya ada supaya kontak yang kedip-kedip (musuh
	//! ilang sebentar dari query lalu muncul lagi) gak langsung bikin commander
	//! ngebatalin persiapan serangannya.
	//!
	//! Ini yang dipakai commander buat mutusin perlu staging atau enggak. Objective
	//! yang bersih gak perlu serangan serentak -- tinggal dimasukin.
	bool IsUncontested(FactionKey fk, float worldTime)
	{
	    float lastSeen;
	    if (!m_mLastEnemySeenTime.Find(fk, lastSeen))
	        return false; // belum pernah dinilai -- jangan berasumsi aman

	    return (worldTime - lastSeen) >= m_fUncontestedDwell;
	}

	float GetUncontestedDwell()
	{
	    return m_fUncontestedDwell;
	}

	//! Nilai kondisi objective ini buat satu faction dan majuin/reset akumulatornya.
	//! Dipanggil TEPAT SEKALI per tick dari ThinkCaptureProgress. Aman kalau kepanggil
	//! dua kali di worldTime yang sama (dt jadi nol, langsung balik).
	//!
	//! Return status hasil penilaian.
	int AssessObjective(FactionKey fk, float worldTime)
	{
	    // === MODIFIED: dulu manggil CountNearbyUnitsBoth di sini -- satu
	    // QueryEntitiesBySphere tiap kali dipanggil, per faction yang nanya. Sekarang
	    // baca hasil sensus berkala yang udah dikerjain ScanPresence sekali buat semua
	    // faction. Kalau sensusnya belum pernah jalan, jangan berasumsi apa-apa:
	    // tabel kosong bakal kebaca "gak ada musuh" dan itu bisa bikin objective
	    // ke-capture cuma karena datanya belum siap.
	    if (!m_bHasScanned)
	        return GetCaptureStatus(fk);

	    // Cuma faction yang boleh contest yang dihitung sebagai musuh. Yang gak boleh
	    // dianggap gak ada -- warga sipil gak bikin objective mustahil direbut.
	    int friendlyCount = GetFactionPresence(fk);
	    int enemyCount    = 0;

	    foreach (FactionKey pk, int pu : m_mFactionPresence)
	    {
	        if (pk == fk || !CanFactionContest(pk))
	            continue;

	        enemyCount = enemyCount + pu;
	    }
	    // === END MODIFIED ===

	    // Jam "kapan musuh terakhir kelihatan" jalan terus, terlepas dari timer
	    // capture aktif atau enggak -- commander butuh ini buat mutusin staging
	    // sebelum ada timer sama sekali.
	    if (enemyCount > 0)
	        m_mLastEnemySeenTime.Set(fk, worldTime);
	    else if (!m_mLastEnemySeenTime.Contains(fk))
	        m_mLastEnemySeenTime.Set(fk, worldTime); // titik nol, dwell mulai dihitung

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
	        // Di-reset, BUKAN dibekukan. Musuh yang bertahan di objective bikin
	        // progress balik nol tiap tick -- kalau mau ngerebut, bersihin dulu.
	        m_mCaptureHeld.Set(fk, 0.0);
	        m_mCaptureStatus.Set(fk, DCO_ECaptureStatus.CONTESTED);
	        return DCO_ECaptureStatus.CONTESTED;
	    }

	    if (friendlyCount <= 0)
	    {
	        // Gak ada musuh, tapi kita juga gak di sana. Progress luruh; timer mati
	        // kalau habis. Peluruhan (bukan reset instan) ngasih grace period alami
	        // buat grup yang sekejap gak kedeteksi, misalnya lagi naik kendaraan.
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
	// === END MODIFIED ===
	
	// === MODIFIED: stalemate sekarang diukur dari PERGERAKAN KONTROL, bukan dari
	// timer capture yang udah gak ada. Dua ambangnya bisa diatur di editor.
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
 
		// === MODIFIED: dulu invalidasi cache = nolin m_fScoreCacheAge (satu field global).
		// Sekarang cache-nya per pemanggil, jadi harus dikosongin semuanya -- kontak baru
		// di objective ini ngubah skor buat SEMUA commander, bukan cuma satu.
		m_mCachedScore.Clear();
		m_mScoreCacheAge.Clear();
		// === END MODIFIED ===
	}
	
	CMD_EObjectiveState GetObjectiveState(FactionKey fk)
	{
		CMD_EObjectiveState state;
		if (m_mObjectiveState.Find(fk, state))
			return state;
		return CMD_EObjectiveState.PENDING;
	}
	
	// === MODIFIED: BUG FIX -- sebelumnya baca dari m_mObjectiveAssignedGroup (map
	// JUMLAH grup yang di-assign, int), bukan dari state action beneran. Efeknya
	// return value-nya itu int-count di-cast paksa ke enum CMD_EObjectiveAction --
	// gak ada hubungannya sama konsep "objective action" apapun (kalau assigned
	// group count = 2, ini bakal return member enum ke-2, apapun itu artinya).
	// Gak ada tracking CMD_EObjectiveAction beneran di file ini sama sekali, jadi
	// gue gak bisa nebak state apa yang harusnya di-return kapan -- sementara
	// dikonsistenin return NONE, daripada ngasih data yang keliatan valid tapi
	// sebenernya ngaco. Kalau emang butuh tracking action beneran, perlu desain
	// state-nya dulu (kapan di-set jadi apa) sebelum bisa diimplement bener.
	// === MODIFIED: implementasi beneran sekarang -- sebelumnya (sesi lalu) return
	// NONE konsisten karena gak ada tracking state yang jelas. Sekarang bisa dihitung
	// deterministic dari type + captured status, gak butuh state tersimpen terpisah:
	// - RECON type -> selalu RECON (fungsinya emang gitu, gak pernah "diserang")
	// - CAPTURE/DESTROY, udah dikuasain faction ini -> DEFEND
	// - CAPTURE/DESTROY, belum dikuasain -> CAPTURE (attack/take it)
	CMD_EObjectiveAction GetObjectiveAction(FactionKey fk)
	{
		if (m_eObjectiveType == CMD_EObjectiveType.RECON)
			return CMD_EObjectiveAction.RECON;
		
		if (IsCapturedBy(fk, string.Empty))
			return CMD_EObjectiveAction.DEFEND;
		
		return CMD_EObjectiveAction.CAPTURE;
	}
	// === END MODIFIED ===
	
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
	
	// === MODIFIED: Priority sekarang compute jarak + base value lagi -- makin deket
	// makin tinggi bonus-nya, diskalain sama base value objective itu sendiri (objective
	// penting + deket = bonus paling gede). Simpel: gak ada normalisasi/importance-
	// weighting kayak sistem relevance sebelumnya, cuma proximity x base value doang.
	float ComputePriorityScore(FactionKey forFaction, float worldTime, vector commanderPos, float combatFocus = 0.5, string commanderUID = "")
	{
	    CMD_EObjectiveState currentState = GetObjectiveState(forFaction);

	    if (currentState == CMD_EObjectiveState.COMPLETED || currentState == CMD_EObjectiveState.FAILED)
	        return 0.0;

	    string cacheKey = forFaction + "|" + commanderUID;

	    float cacheAge;
	    if (m_mScoreCacheAge.Find(cacheKey, cacheAge) && (worldTime - cacheAge) < CACHE_DURATION)
	    {
	        float cachedScore;
	        if (m_mCachedScore.Find(cacheKey, cachedScore))
	            return cachedScore;
	    }

	    // === MODIFIED: 2-Tier (Assault vs Capture) -- sebelumnya enemyCount cuma
	    // NAMBAH skor (max +50), jadi objective kosong yang base value/proximity-nya
	    // tinggi bisa ngalahin objective yang lagi ada kontak aktif. Itu salah secara
	    // taktis -- ada musuh HARUS didahuluin (assault), bukan sekadar "nambah
	    // pertimbangan". Sekarang enemyCount jadi PENENTU TIER, bukan cuma tambahan:
	    // objective ber-musuh (Tier 1) SELALU menang lawan objective kosong (Tier 2)
	    // lewat offset besar (10000), apapun kombinasi value/proximity Tier 2.
	    int enemyCount = CountNearbyUnits(m_fThreatRadius, forFaction, false);

	    float score = m_fBaseValue;

	    if (m_fLastContestedTime > 0.0)
	    {
	        float elapsed = worldTime - m_fLastContestedTime;
	        if (elapsed < 120.0)
	            score += Math.Lerp(25.0, 0.0, elapsed / 120.0);
	    }

	    int friendlyCount = CountNearbyUnits(m_fFriendlyRadius, forFaction, true);
	    score -= Math.Clamp(friendlyCount * 5.0, 0.0, 30.0);

	    if (currentState == CMD_EObjectiveState.ASSIGNED)
	        score -= 15.0;

	    // === MODIFIED: struktur skor dibalik.
	    //
	    // DULU dua hal bikin urutannya gak masuk akal:
	    //
	    //   1. proximityBonus dikali m_fBaseValue. Artinya keuntungan maksimum sebuah
	    //      objective dari kedekatan SAMA DENGAN base value-nya sendiri. Objective
	    //      kecil di 300 m atapnya cuma 2 x base value (40), sementara lantai
	    //      objective besar di jarak berapapun adalah base value dikurangi falloff
	    //      (80). Selisih base value lebih dari dua kali gak akan pernah terkejar
	    //      jarak -- itu sebabnya objective kecil di dekat rumah dikacangin.
	    //
	    //   2. tier gate `score = 10000 + enemyCount * 300` MENGHAPUS jarak justru di
	    //      kasus yang paling butuh. Objective bermusuh 8 km dan objective bermusuh
	    //      300 m dianggap sama mendesaknya. Efek sampingnya semua objective
	    //      bermusuh berskor 10xxx dan bedanya tipis -- di overlay debug praktis
	    //      gak bisa dibedain.
	    //
	    // SEKARANG hierarkinya: base value + jarak jadi penentu utama, ancaman jadi
	    // PENGUBAH yang luruh lebih tajam dari kedekatan. Pangkat luruh itu yang bikin
	    // musuh jauh hampir tak berarti sementara kedekatan masih terasa: di setengah
	    // radius relevan, kedekatan bernilai 50% tapi ancaman tinggal 25%.
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
	            // Di luar radius relevan: bonus habis, masuk penalti landai yang
	            // TERBATAS (asimptotik ke m_fDistantFalloffWeight). Objective jauh tetap
	            // bisa diurutin sesamanya dan tetap mungkin kepilih -- gak pernah jatuh
	            // ke skor yang bikin dia mustahil direbut selamanya.
	            float excess   = (distToCommander - m_fMaxRelevantDistance) / m_fMaxRelevantDistance;
	            proximityBonus = -m_fDistantFalloffWeight * (excess / (1.0 + excess));
	        }
	    }

	    score += proximityBonus;

	    // Lantai luruh mencegah ancaman jauh jadi NOL. Kalau nol, dua objective jauh
	    // yang base value-nya sama jadi gak bisa dibedain sama sekali walaupun satunya
	    // penuh musuh -- dan commander gak punya alasan buat milih yang mana.
	    float threatFactor = Math.Max(
	        Math.Pow(proximityFactor, m_fThreatFalloffPower),
	        m_fThreatFalloffFloor);

	    // combatFocus dulu cuma menggeser tierThreshold, sebuah ambang integer yang
	    // efektifnya cuma punya tiga nilai. Sekarang dia jadi pengali langsung, jadi
	    // commander yang fokus tempur bereaksi lebih keras ke musuh lewat knob yang
	    // berarti, bukan lewat lompatan diskrit.
	    float focusMod = Math.Lerp(0.5, 1.5, combatFocus);

	    score += enemyCount * m_fThreatWeight * threatFactor * focusMod;
	    // === END MODIFIED ===
	    float finalScore = Math.Max(score, 0.0);

	    m_mCachedScore.Set(cacheKey, finalScore);
	    m_mScoreCacheAge.Set(cacheKey, worldTime);

	    return finalScore;
	}
	
	bool QueryCallback(IEntity e)
	{
		SCR_ChimeraCharacter chr = SCR_ChimeraCharacter.Cast(e);
		if (!chr)
			return true;

		SCR_CharacterPerceivableComponent percive = SCR_CharacterPerceivableComponent.Cast(e.FindComponent(SCR_CharacterPerceivableComponent));
		if (!percive)
			return true;
		
		if (!nearby.Contains(e))
		{
			nearby.Insert(e);
		}
		
		return true;
	}
	
	ref array<IEntity> nearby = {};
	
	// === MODIFIED: BUG FIX -- nearby.Clear() sebelumnya gak pernah dipanggil.
	// QueryCallback cuma NAMBAH entity ke array (field instance, bukan local var),
	// jadi tiap kali CountNearbyUnits dipanggil, entity lama yang udah pindah/mati
	// tetep numpuk dan ke-count selama pointer-nya masih valid. Makin lama scenario
	// jalan, makin gak akurat friendlyCount/enemyCount yang dihasilin. ===
	int CountNearbyUnits(float radius, FactionKey factionKey, bool isFriendly)
	{
		int count  = 0;
		vector pos = GetOwner().GetOrigin();
 
		nearby.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(pos, radius, QueryCallback);
		
		foreach (IEntity ent : nearby)
		{
			if (!ent)
				continue;
 
			SCR_ChimeraCharacter grp = SCR_ChimeraCharacter.Cast(ent);
			if (!grp)
				continue;
			
			SCR_CharacterPerceivableComponent percive = SCR_CharacterPerceivableComponent.Cast(ent.FindComponent(SCR_CharacterPerceivableComponent));
			if (!percive)
				continue;
			
			Faction fc = percive.GetPerceivedFaction();
			if (!fc)
				continue;
			
			SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
			if (!SCR_FactionManager)
				continue;
 
			Faction myfac = factionManager.GetFactionByKey(factionKey);
			if (!myfac)
				continue;
			
			SCR_Faction f = SCR_Faction.Cast(myfac);
			bool sameFaction = (fc.GetFactionKey() == factionKey);
			bool IsFriendlyFaction = f.IsFactionFriendly(fc);
 
			if (isFriendly && (sameFaction || IsFriendlyFaction))
				count++;
			else if (!isFriendly && (!sameFaction || !IsFriendlyFaction))
				count++;
		}
 		//Print(string.Format("[CMD_Objective] %1 | %2 Is Friendly %3 Count %4",
			//GetOwner().GetName(), factionKey, isFriendly, count));		
		return count;
	}
	// === END MODIFIED ===
	
	// === ADDED: Optimasi -- versi combined buat titik yang butuh friendly DAN enemy
	// count SEKALIGUS di radius yang SAMA (GetCaptureProgress, IsCaptureTimerComplete,
	// CheckAndMarkIfLost). Sebelumnya masing-masing manggil QueryEntitiesBySphere
	// terpisah padahal posisi+radius identik -- sekarang 1 query, 1 pass. ===
	void CountNearbyUnitsBoth(float radius, FactionKey factionKey, out int friendlyCount, out int enemyCount)
	{
		friendlyCount = 0;
		enemyCount    = 0;
		vector pos    = GetOwner().GetOrigin();
		
		nearby.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(pos, radius, null, QueryCallback, EQueryEntitiesFlags.ALL);
		
		foreach (IEntity ent : nearby)
		{
			if (!ent)
				continue;
			
			SCR_ChimeraCharacter chr = SCR_ChimeraCharacter.Cast(ent);
			if (!chr)
				continue;
			
			SCR_CharacterPerceivableComponent percive = SCR_CharacterPerceivableComponent.Cast(ent.FindComponent(SCR_CharacterPerceivableComponent));
			if (!percive)
				continue;
			
			Faction fc = percive.GetPerceivedFaction();
			if (!fc)
				continue;
			
			if (fc.GetFactionKey() == factionKey)
				friendlyCount++;
			else
				enemyCount++;
		}
	}
	// === END ADDED ===
	
	void SetObjectiveState(FactionKey fk, CMD_EObjectiveState state)
	{
		if (m_mObjectiveState.Contains(fk))
		{
			m_mObjectiveState[fk] = state;
			//Print("SET OBJ STATE TO > " + typename.EnumToString(CMD_EObjectiveState, m_mObjectiveState[fk]) + " FK : " + fk);
		}
		
	}
	
	void SetObjectiveGroup(FactionKey fk, int number)
	{
		if (m_mObjectiveAssignedGroup.Contains(fk))
		{
			int num = m_mObjectiveAssignedGroup[fk] + number;
			m_mObjectiveAssignedGroup[fk] = num;
			//Print(m_mObjectiveAssignedGroup[fk].ToString() + " < Number of assigned Group | Num > " + num);
			
		}
	}
	
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_OwnerEntity = owner;
		SetEventMask(owner, EntityEvent.INIT);

		// === ADDED: objective butuh tick sendiri buat sensus berkala. Dulu dia pasif
		// total -- cuma bereaksi kalau commander nanya. ===
		SetEventMask(owner, EntityEvent.FRAME);
		// === END ADDED ===
	}

	// === ADDED: Presence Scan tick ===
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		// Data otoritatif -- dibangun di server. Di client murni tabelnya tetap kosong
		// dan semua getter-nya balikin nol.
		if (!Replication.IsServer())
			return;

		if (m_fPresenceScanInterval <= 0.0)
			return;

		m_fScanTimer += timeSlice;

		if (m_fScanTimer < m_fPresenceScanInterval)
			return;

		float elapsed  = m_fScanTimer;
		m_fScanTimer   = 0.0;

		float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;

		ScanPresence(worldTime);

		// === ADDED: overlay digambar di tick objective sendiri. Objective yang tau
		// isi dirinya, jadi dia juga yang nggambar dirinya -- commander gak perlu
		// nyalin state-nya ke snapshot dulu.
		UpdateObjectiveDebug(worldTime);
		// === END ADDED ===

		// === ADDED: kontrol dimajuin di tick objective sendiri, tepat setelah sensus.
		// Objective yang nilai dirinya -- commander cuma baca hasilnya. ===
		ResolveInitialOwner();
		AdvanceControl(worldTime, elapsed);
		// === END ADDED ===
	}

	// === ADDED: kepemilikan awal dari editor. Diselesaikan LAZY, bukan di
	// InitializeObjective -- waktu objective init, commander belum tentu udah
	// registrasi ke manager, jadi UID-nya belum bisa diresolusi jadi faction.
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
			return; // manager belum ada -- coba lagi tick berikutnya

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
	// === END ADDED ===
	// === END ADDED ===
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		InitializeObjective();
	}

	//------------------------------------------------------------------------------------------------
	// === ADDED: Presence Scan ===
	//! Objective nyensus isi radius-nya sendiri secara berkala, bukan nunggu ditanya
	//! commander. Satu query per interval, hasilnya diember per FactionKey.
	//!
	//! Ini sekaligus NGURANGIN kerja: AssessObjective dulu manggil CountNearbyUnitsBoth
	//! (satu QueryEntitiesBySphere) tiap tick per faction yang nanya. Sekarang query-nya
	//! sekali per interval buat SEMUA faction, dan penilaian tinggal baca tabel.
	//!
	//! Jalan di server doang -- ini data otoritatif. Overlay debug bacanya lokal, yang
	//! artinya kelihatan di hosted/single-player; di client murni tabelnya kosong.
	
	bool QueryCallbackScan(IEntity e)
	{
		SCR_ChimeraCharacter chr = SCR_ChimeraCharacter.Cast(e);
		if (!chr)
			return true;;

		SCR_CharacterPerceivableComponent percive = SCR_CharacterPerceivableComponent.Cast(e.FindComponent(SCR_CharacterPerceivableComponent));
		if (!percive)
			return true;;
		
		if (!nearbyScan.Contains(e))
		{
			nearbyScan.Insert(e);
		}
		
		return true;
	}
	
	ref array<IEntity> nearbyScan = {};
	
	void ScanPresence(float worldTime)
	{
		m_mFactionPresence.Clear();
		float m_iTotalPresenceTemp = 0;

		vector pos = GetOwner().GetOrigin();

		nearbyScan.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(pos, m_fRadius, QueryCallbackScan);
		
		if (m_bDebugMode)
		{
			m_aDebugShapes.Clear();
			Print(nearbyScan.Count().ToString() + " < Number of counted Entity Inside > " + m_sObjectiveName);
			int flags = ShapeFlags.TRANSP | ShapeFlags.WIREFRAME;
			m_aDebugShapes.Insert(Shape.CreateSphere(Color.BLUE, flags, pos, m_fRadius));
			
		}
			

		foreach (IEntity ent : nearbyScan)
		{
			if (!ent)
				continue;

			SCR_ChimeraCharacter chr = SCR_ChimeraCharacter.Cast(ent);
			if (!chr)
				continue;

			SCR_CharacterPerceivableComponent percive = SCR_CharacterPerceivableComponent.Cast(ent.FindComponent(SCR_CharacterPerceivableComponent));
			if (!percive)
				continue;

			Faction fc = percive.GetPerceivedFaction();
			if (!fc)
				continue;

			FactionKey key = fc.GetFactionKey();

			int current;
			if (!m_mFactionPresence.Find(key, current))
				current = 0;

			m_mFactionPresence.Set(key, current + 1);
			m_iTotalPresenceTemp = m_iTotalPresenceTemp + 1;
		}

		m_iTotalPresence = m_iTotalPresenceTemp;
		m_fLastScanTime = worldTime;
		m_bHasScanned   = true;
	}

	//------------------------------------------------------------------------------------------------
	// === ADDED: Control Model ===
	//! Kepemilikan objective sekarang persentase per faction, bukan boolean.
	//! m_mControl[fk] = 0..100, totalnya <= 100, sisanya Neutral.
	//!
	//! LAJU GESER, diturunkan dari tiga syarat sekaligus:
	//!   delta = baseRate x unitScale x contestPenalty x dt
	//!     baseRate       = 100 / m_fCaptureHoldDuration      (persen per detik)
	//!     unitScale      = clamp(unitTerbanyak / m_iFullSpeedUnits, 0, 1)
	//!     contestPenalty = (unitTerbanyak - runnerUp) / unitTerbanyak
	//!
	//!   1 unit sendirian  -> 1/8 x 1.0   = 12.5% laju, jalan tapi lambat
	//!   8 unit sendirian  -> laju penuh
	//!   2 lawan 1         -> 2/8 x 0.5   = 12.5%, yang dua naik tapi diperlambat
	//!   1 lawan 1         -> penalty 0, mandek total
	//!
	//! URUTAN: kenaikan HARUS ngambil dari kontrol faction lain dulu sampai nol, baru
	//! sisanya ngisi ruang netral. Jadi ngerebut objective yang dipegang penuh lawan
	//! butuh dua kali waktu -- sekali buat ngosongin punya dia, sekali buat ngisi
	//! punya kita.

	float GetControl(FactionKey fk)
	{
		float v;
		if (m_mControl.Find(fk, v))
			return v;

		return 0.0;
	}

	//! Porsi yang belum dipegang faction manapun.
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

	//! Faction yang gak boleh contest dianggap GAK ADA sama sekali -- unitnya gak
	//! ngeblok siapa-siapa dan gak ikut ngitung. Tanpa ini, warga sipil di dalam
	//! radius bisa bikin objective mustahil direbut selamanya.
	bool CanFactionContest(FactionKey fk)
	{
		if (fk.IsEmpty())
			return false;

		if (m_bAllowAllFactions)
			return true;

		return m_aAllowedFactions.Contains(fk);
	}

	//! Salinan tabel kontrol buat overlay debug.
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

	//! Commander ngelaporin kehadiran grupnya di sini. Skornya grup x detik --
	//! itu yang bikin "banyak DAN lama" menang, sementara "lama tapi cuma satu grup"
	//! kalah sama "sebentar tapi banyak". Dipakai buat nentuin commander mana yang
	//! jadi owner waktu faction-nya nyampe ambang.
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

	//------------------------------------------------------------------------------------------------
	//! Majuin kontrol satu tick. Dipanggil dari EOnFrame objective sendiri, tepat
	//! setelah ScanPresence -- objective yang nilai dirinya, bukan commander.
	void AdvanceControl(float worldTime, float dt)
	{
		if (!m_bHasScanned || dt <= 0.0)
			return;

		// --- cari faction terkuat yang boleh contest ---
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
			// Radius kosong. Kontrol luruh balik ke pemilik terakhir, bukan ke netral --
			// wilayah yang udah direbut tetap kerasa milik yang ngerebut walaupun
			// pasukannya lagi gak di situ.
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

	//! Naikin kontrol satu faction sebanyak delta, dengan aturan "turunin lawan dulu".
	protected void PushControlToward(FactionKey winner, float delta)
	{
		float remaining = delta;

		// 1. Ambil dari faction lain, proporsional terhadap kontrol mereka. Proporsional
		//    supaya waktu ada tiga faction, yang pegang paling banyak juga turun paling
		//    banyak -- bukan faction pertama di iterasi map yang kena habis duluan.
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

		// 2. Sisanya baru ngisi ruang netral.
		float mine = GetControl(winner);
		m_mControl.Set(winner, Math.Clamp(mine + remaining, 0.0, 100.0));
	}

	//! Radius kosong: geser kontrol balik ke pemilik terakhir.
	protected void DecayTowardLastOwner(float dt)
	{
		if (m_sLastOwnerFaction.IsEmpty())
			return;

		if (GetControl(m_sLastOwnerFaction) >= 100.0)
			return;

		PushControlToward(m_sLastOwnerFaction, m_fEmptyDecayRate * dt);
	}

	//------------------------------------------------------------------------------------------------
	//! Tentuin owner faction dan owner commander dari tabel kontrol.
	protected void ResolveOwnership(float worldTime)
	{
		FactionKey previousOwner = m_sOwnerFaction;

		// Naik jadi owner butuh ambang penuh; kehilangan status baru terjadi di ambang
		// yang jauh lebih rendah. Jarak antara keduanya itu histeresisnya -- tanpa itu
		// satu musuh nyasar bisa bikin status owner kedip-kedip tiap tick.
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
			// Ganti pemilik faction: kontribusi commander di-reset supaya perebutan
			// berikutnya dinilai dari nol, bukan diwarisi skor pertempuran sebelumnya.
			m_mCommanderContribution.Clear();
			m_mCommanderFaction.Clear();
			m_sOwnerCommanderUID = string.Empty;
			m_fLastControlChangeTime = worldTime;
		}

		ResolveOwnerCommander();
	}

	//! Commander mana yang megang objective ini. Cuma boleh satu.
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

		// Objective boleh punya faction tapi belum punya commander -- misalnya direbut
		// grup yang gak ditugasin commander manapun. Commander sefaction yang non-
		// blacklist tetap nganggep objective ini udah kita pegang.
		m_sOwnerCommanderUID = bestUID;
	}

	//! Serahin kepemilikan ke commander lain sefaction. Dipakai waktu owner lama
	//! ninggalin objective -- ini yang bikin handoff offensive -> defensive mungkin.
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
	// === END ADDED ===
	//------------------------------------------------------------------------------------------------
	// === ADDED: Objective Debug ===
	//! Overlay milik objective. Semuanya dibaca LIVE dari state objective sendiri --
	//! gak ada snapshot, gak ada commander yang harus nyalin apa-apa ke sini.
	//!
	//! Yang GAK ditampilin di sini: skor prioritas dan rank. Dua itu fakta
	//! COMMANDER tentang objective (dihitung dari posisi dan personality commander),
	//! bukan properti objective -- tempatnya di overlay commander.
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

		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
			Vector(pos[0], pos[1] + 21.0, pos[2]), BuildDebugControl(),  16.0, color));

		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
			Vector(pos[0], pos[1] + 12.0, pos[2]), BuildDebugPresence(worldTime), 16.0, color));

		m_aDebugTexts.Insert(DCO_DebugDraw.SpawnText(
			Vector(pos[0], pos[1] +  2.0, pos[2]), BuildDebugTuning(),   15.0, color));
	}

	//! Bar kontrol: deretan bola kecil, tiap bola satu potong persentase. Dipakai bola
	//! karena Shape.CreateSphere satu-satunya primitif yang udah kebukti jalan di
	//! codebase ini. Slot diisi berurutan per faction lalu sisanya netral, jadi
	//! 62/30/8 kebaca sebagai blok warna bersambung -- bukan bola selang-seling.
	protected void DrawDebugControlBar(vector origin, int flags)
	{
		const int   SLOTS   = 20;   // 5% per slot
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

	//! Angka mentah yang dipakai buat ngitung. Waktu nge-tune di Workbench yang
	//! berguna itu ngeliat nilai efektifnya, bukan cuma hasil akhirnya.
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
	// === END ADDED ===
	//! Berapa unit dari faction ini di radius, menurut sensus terakhir.
	int GetFactionPresence(FactionKey fk)
	{
		int count;
		if (m_mFactionPresence.Find(fk, count))
			return count;

		return 0;
	}

	//! Semua unit di radius, faction apapun.
	int GetTotalPresence()
	{
		return m_iTotalPresence;
	}

	//! Berapa faction BERBEDA yang lagi ada di radius.
	int GetPresenceFactionCount()
	{
		return m_mFactionPresence.Count();
	}

	//! Daftar FactionKey yang punya unit di radius. Buat overlay debug.
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

	//! Faction pemilik objective ini, atau "Neutral" kalau belum dipegang siapa-siapa.
	//! Dipakai overlay debug; sengaja ngembaliin string biar gak ada pemanggil yang
	//! keliru nganggep string kosong itu FactionKey yang sah.
	string GetOwnerFactionLabel()
	{
		FactionKey owner = GetOwningFaction();

		if (owner.IsEmpty())
			return "Neutral";

		return owner;
	}
	// === END ADDED ===
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

	// === ADDED: getter properti mentah. Semua field ini protected dan cuma dipakai
	// di dalam ComputePriorityScore/CountNearbyUnits, jadi dari luar gak ada cara
	// ngeliat ANGKA APA yang lagi dipakai commander buat mutusin. Overlay debug niat
	// commander yang jadi konsumennya -- biar pas nge-tune di Workbench keliatan
	// nilai efektifnya, bukan cuma hasil akhirnya.
	float GetBaseValue()                     { return m_fBaseValue; }
	float GetThreatRadius()                  { return m_fThreatRadius; }
	float GetFriendlyRadius()                { return m_fFriendlyRadius; }
	float GetMaxRelevantDistance()           { return m_fMaxRelevantDistance; }
	float GetDistantFalloffWeight()          { return m_fDistantFalloffWeight; }
	float GetCaptureHoldDuration()           { return m_fCaptureHoldDuration; }
	int   GetMaxGroupCount()                 { return m_iMaxGroupCount; }
	// === END ADDED ===
 
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
	
	// === ADDED: getter buat GetReconGroup -- dipake buat Recon Reveal (grup RECON
	// yang beneran jadi "reporter" pas ngirim contact report reveal-nya) ===
	DCO_GroupUtilityComponent GetReconGroup(FactionKey fk)
	{
		DCO_GroupUtilityComponent grp;
		if (m_mReconGroup.Find(fk, grp))
			return grp;
		return null;
	}
	// === END ADDED ===
	
	bool IsReconArrived(FactionKey fk, float worldTime)
	{
	    DCO_GroupUtilityComponent reconGrp;
	    if (!m_mReconGroup.Find(fk, reconGrp))
	        return false;
	
	    if (!reconGrp)
	        return false;
	
	    return reconGrp.CheckOrderComplete(worldTime);
	}
	
	// === ADDED: Intel Fog System ===
	//! Cuma relevan buat objective ber-type RECON. True kalau ada grup RECON yang
	//! masih idup dan beneran DI SEKITAR objective ini sekarang (bukan cuma pernah
	//! dikirim dulu) -- reuse m_mReconGroup, direpurpose sebagai "standing presence"
	//! buat objective RECON (beda sama pemakaian di objective normal yang cuma buat
	//! pre-assault scouting sekali jalan).
	bool IsReconObjectiveActive(FactionKey fk)
	{
		if (m_eObjectiveType != CMD_EObjectiveType.RECON)
			return false;
		
		DCO_GroupUtilityComponent reconGrp;
		if (!m_mReconGroup.Find(fk, reconGrp))
			return false;
		
		if (!reconGrp || !reconGrp.GetOwner())
			return false;
		
		// Toleransi longgar di sekitar objective -- grup gak perlu presisi di titik
		// tengah, cukup "masih di area ini secara umum"
		float toleranceDist = Math.Max(m_fRadius * 2.0, 60.0);
		return vector.DistanceSq(reconGrp.GetOwner().GetOrigin(), GetOwner().GetOrigin()) <= toleranceDist * toleranceDist;
	}
	
	float GetIntelCoverageRadius()
	{
		return m_fIntelCoverageRadius;
	}
	
	//! Multiplier speed capture progress berdasarkan intel coverage. 1.0 = objective
	//! RECON itu sendiri (gak kena fog-nya sendiri) ATAU ke-cover recon aktif di
	//! sekitarnya. Di bawah 1.0 = foggy, capture melambat.
	// === REMOVED: GetIntelFogMultiplier -- gak jadi dipake, Intel Fog cuma ngaruh ke
	// keputusan komit commander (RiskTaking gate di AICommanderBase.c), bukan ke
	// capture speed. IsReconObjectiveActive/GetIntelCoverageRadius di atas dan
	// IsObjectiveIntelCovered di AICommanderManager.c tetap dipake buat itu.
	// === END REMOVED ===
 
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

		// === ADDED: akumulator dinolin di sini, bukan diturunin dari selisih jam. ===
		m_mCaptureHeld.Set(fk, 0.0);
		m_mCaptureLastTick.Set(fk, worldTime);
		// === END ADDED ===
 
		//Print(string.Format("[CMD_Objective] %1 | %2 capture timer started (hold %3s)",
			//GetOwner().GetName(), fk, m_fCaptureHoldDuration.ToString()));
	}
 
	bool IsCaptureTimerRunning(FactionKey fk)
	{
		return m_mCaptureStartTime.Contains(fk);
	}
 
	// === ADDED: dulu m_mCaptureStartTime cuma pernah di-Insert/Set, GAK PERNAH di-Remove
	// di manapun. Akibatnya sekali sebuah faction mulai capture di objective ini,
	// IsCaptureTimerRunning(fk) return true SELAMANYA -- walaupun grupnya udah mundur,
	// mati, atau objective-nya udah kelar direbut. GetActiveCapturingFaction() juga ikut
	// ngasih faction basi terus. Dua method di bawah ini yang ngasih jalan keluarnya.
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
	// === END ADDED ===
 
	// === MODIFIED: satuin duplikasi logic -- sebelumnya isi fungsi ini hampir identik
	// sama GetCaptureProgress (reset-elapsed-kalau-outnumbered, query sphere sendiri).
	// Sekarang delegate ke GetCaptureProgress, cukup cek udah nyampe 1.0 (100%) apa
	// belum. Bonus: sebelumnya IsCaptureTimerComplete gak pernah update
	// m_fLastProgressTime (cuma GetCaptureProgress yang update), jadi kalau cuma ini
	// yang dipanggil, stalemate-detection bisa salah baca. Sekarang konsisten. ===
	bool IsCaptureTimerComplete(FactionKey fk, float worldTime)
	{
		if (!m_mCaptureStartTime.Contains(fk))
			return false;
		
		return GetCaptureProgress(fk, worldTime) >= 1.0;
	}
	// === END MODIFIED ===
	
	// === ADDED: UI Data Getters ===
	//! Faction key yang LAGI PUNYA capture timer aktif (proses capture lagi
	//! berjalan) di objective ini SAAT INI. Beda sama GetOwningFaction() -- ini
	//! buat "siapa yang LAGI ngerebut", bukan "siapa yang UDAH punya". Return
	//! string kosong kalau gak ada faction manapun yang lagi proses capture.
	FactionKey GetActiveCapturingFaction()
	{
		foreach (FactionKey key, float startTime : m_mCaptureStartTime)
		{
			return key; // ambil yang pertama ketemu -- normalnya cuma 1 faction yang lagi capture di 1 waktu
		}
		return string.Empty;
	}

	// === MODIFIED: dulu scan m_mIsCaptured dan ngembaliin match PERTAMA -- non
	// deterministik kalau dua faction sama-sama true, yang memang mungkin terjadi
	// karena gak ada yang jamin eksklusivitas. Sekarang satu field otoritatif.
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
	// === END ADDED ===
	
	// === MODIFIED: sekarang cuma nanya "faction ini owner atau bukan" ke model
	// kontrol. Parameter cuid DIABAIKAN -- kepemilikan commander sekarang field
	// eksplisit sendiri (GetOwnerCommanderUID), bukan disimpulin dari array editor.
	// Nama lama dipertahanin supaya 13 call site di AICommanderBase gak perlu diubah
	// di commit yang sama dengan perombakan model kontrolnya.
	bool IsCapturedBy(FactionKey fk, string cuid = "")
	{
		return IsOwnedBy(fk);
	}


	// === MODIFIED: gak ada lagi yang perlu "di-mark" -- statusnya dihitung dari
	// tabel kontrol tiap kali ditanya. Fungsinya jadi identik dengan CheckIsItLost;
	// nama lama dipertahanin buat call site yang udah ada.
	bool CheckAndMarkIfLost(FactionKey fk)
	{
		return CheckIsItLost(fk);
	}

	
	// === MODIFIED: "lost" sekarang turunan murni dari kontrol -- kita pernah pegang
	// objective ini, sekarang enggak. m_mLostStatus gak dipakai lagi buat mutusin.
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
	
		// Invalidate captured status
		SetCapturedBy(fk, false);
 
		// === ADDED: faction ini udah gak punya kehadiran di sini, jadi progres
		// capture-nya (kalau ada) harus ikut hangus. Tanpa ini timernya nyangkut dan
		// dianggap masih jalan pas mereka balik lagi nanti.
		ClearCaptureTimer(fk);
		// === END ADDED ===
	
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
 
	// === MODIFIED: sekarang nulis ke tabel kontrol, bukan ke boolean. val=true
	// artinya "paksa jadi 100% milik faction ini" -- dipakai jalur seed/skenario,
	// bukan jalur capture normal (capture normal jalan lewat AdvanceControl).
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