[ComponentEditorProps(category: "GameScripted/Commander", description: "Koordinasi fire mission artillery untuk AI Commander")]
class CMD_ArtillerySupportClass : ScriptComponentClass 
{
	
}

// === REWRITE NOTES ===
// Bug yang dibenerin dari versi sebelumnya:
// 1. ProcessQueue() gak ada null-check m_Commander -- padahal dipake langsung
//    (GetOwner().GetOrigin(), SpawnArtilleryWP). Kalau EOnInit gagal nemu
//    AICommander_BaseComponent, ini bisa null-deref crash begitu ada request masuk
//    queue. Sekarang di-guard di awal ProcessQueue.
// 2. Loop utama ProcessQueue pake "foreach + RemoveItem" di array yang sama --
//    modifying array while iterating, request kedua+ bisa ke-skip kalau ada 2+
//    dalam 1 cycle. Diganti index-based while loop, konsisten sama pola
//    PurgeExpiredRequests/PurgeDeadUnits yang di file yang sama udah bener.
// 3. Global cooldown check di-comment out sebelumnya (dihitung tapi gak pernah
//    dienforce) -- diaktifin lagi.
// 4. m_fBaseRejectionChance dideklarasi tapi gak pernah dipake -- sekarang beneran
//    di-roll di RequestShellImpact().
// 5. FindClosestAvailableUnit() (exclude EXECUTING_COMMAND) vs HasAnyAvailableUnit()
//    (harus IDLE) -- dua definisi "available" yang beda. Disamain, both require IDLE.
// === END REWRITE NOTES ===
class CMD_ArtillerySupport : ScriptComponent
{ 
	[Attribute("0.4", UIWidgets.EditBox, "Interval (detik) dispatch tiap shell dari queue", category: "Firing")]
	protected float m_fDispatchInterval;
 
	[Attribute("90.0", UIWidgets.EditBox, "Detik sebelum request di queue dianggap kadaluarsa", category: "Firing")]
	protected float m_fQueueExpiry;
 
	[Attribute("25.0", UIWidgets.EditBox, "Radius cek friendly (meter) sebelum fire mission diizinkan", category: "Safety")]
	protected float m_fFriendlySafeRadius;
	
	[Attribute("0.3", UIWidgets.Range, "Chance request ditolak secara random (0 = tidak pernah, 1 = selalu)", params: "0 1 0.01", category: "Firing")]
	protected float m_fBaseRejectionChance;
	
	[Attribute("15.0", UIWidgets.EditBox, "Cooldown base (detik) per shell yang ditembak", category: "Firing")]
	protected float m_fCooldownPerShell;
	
	[Attribute("30.0", UIWidgets.EditBox, "Minimum cooldown global artillery (detik)", category: "Firing")]
	protected float m_fMinGlobalCooldown;
	
	[Attribute("300.0", UIWidgets.EditBox, "Maximum cooldown global artillery (detik)", category: "Firing")]
	protected float m_fMaxGlobalCooldown;

	[Attribute("30.0", UIWidgets.EditBox, "Minimum dispersion radius (meter)", category: "Artillery Accuracy")]
	protected float m_fMinDispersion;
	
	[Attribute("250.0", UIWidgets.EditBox, "Maximum dispersion radius (meter)", category: "Artillery Accuracy")]
	protected float m_fMaxDispersion;
	
	[Attribute("0.2", UIWidgets.Range, "Dispersion scale per meter jarak (range * scale = base dispersion)", params: "0.01 0.5 0.01", category: "Artillery Accuracy")]
	protected float m_fDispersionRangeScale;
	
	[Attribute("0.5", UIWidgets.Range, "Base accuracy sebelum modifier", params: "0.0 1.0 0.01", category: "Artillery Accuracy")]
	protected float m_fBaseAccuracy;
	
	[Attribute("1.5", UIWidgets.Range, "Global dispersion multiplier — naikan buat lebih spray, turunin buat lebih presisi", params: "0.1 3.0 0.1", category: "Artillery Accuracy")]
	protected float m_fDispersionMultiplier;
	
	[Attribute("0.1", UIWidgets.Range, "Cluster ratio — 0 = flat random, 1 = sangat clustering di center", params: "0.0 1.0 0.01", category: "Artillery Accuracy")]
	protected float m_fClusterRatio;
	
	// === ADDED: Walking Rounds In -- tembakan berulang ke area yang sama (dalem radius
	// & window waktu tertentu) makin akurat, sampe plateau di koreksi ke-N. Ini
	// nyimulasiin spotter/observer ngasih koreksi buat tembakan berikutnya. Bagian 2
	// dari sistem "intel accuracy" -- digabung sama plateau + auto-expire.
	[Attribute("50.0", UIWidgets.EditBox, "Radius (meter) buat nganggep fire mission baru itu 'nembak tempat yang sama' kayak sebelumnya", category: "Artillery Accuracy")]
	protected float m_fCorrectionRadius;
	
	[Attribute("300.0", UIWidgets.EditBox, "Detik -- kalau gak ada fire mission baru ke area itu dalem waktu ini, tracking koreksi di-reset (dianggap situasi baru)", category: "Artillery Accuracy")]
	protected float m_fCorrectionExpiry;
	
	[Attribute("3", UIWidgets.EditBox, "Maximum jumlah koreksi berturut-turut sebelum bonus akurasi plateau (gak nambah lagi)", category: "Artillery Accuracy")]
	protected int m_iMaxCorrections;
	
	[Attribute("0.08", UIWidgets.Range, "Bonus akurasi PER TINGKAT koreksi (tingkat 1 = panggilan pertama, gak dapet bonus)", params: "0.0 0.2 0.01", category: "Artillery Accuracy")]
	protected float m_fCorrectionBonusPerStep;
	
	protected ref array<ref CMD_ArtilleryCorrectionTrack> m_aCorrectionTracks = new array<ref CMD_ArtilleryCorrectionTrack>();
	// === END ADDED ===
	
	[Attribute("5", UIWidgets.EditBox, "Maximum request di queue sebelum kena penalty cooldown", category: "Cooldown")]
	protected int m_iMaxQueueSize;
	
	[Attribute("10.0", UIWidgets.EditBox, "Penalty cooldown tambahan per request saat queue penuh (detik)", category: "Cooldown")]
	protected float m_fQueueOverloadPenalty;
	
	[Attribute("60.0", UIWidgets.EditBox, "Cooldown global artillery setelah fire mission selesai (detik)", category: "Cooldown")]
	protected float m_fGlobalCooldown;
	
	[Attribute("5.0", UIWidgets.EditBox, "Waktu proses per request di queue (detik) sebelum request berikutnya bisa di-process", category: "Cooldown")]
	protected float m_fQueueProcessTime;
	
	protected float m_fGlobalCooldownUntil  = 0.0;
	protected float m_fNextProcessTime      = 0.0;
	protected int   m_iTotalShellsFired     = 0;
 
	//--------------------------------------------------------------------
	protected AICommander_BaseComponent               m_Commander;
	protected ref array<DCO_GroupUtilityComponent>        m_aUnits        = new array<DCO_GroupUtilityComponent>();
	protected ref array<ref CMD_FireMissionRequest>   m_aQueue        = new array<ref CMD_FireMissionRequest>();
	protected CMD_ThreatResponseComponent m_ThreatResponseComponent;
	protected float                                   m_fDispatchTimer = 0.0;
 
	// State untuk friendly query callback — tidak thread-safe tapi Reforger single-threaded
	protected bool   m_bFriendlyFound   = false;
	protected string m_sFriendlyFaction = string.Empty;
	
	float GetGlobalCooldownUntil()     { return m_fGlobalCooldownUntil; }
	int   GetTotalShellsFired()        { return m_iTotalShellsFired; }
	
	void AddGlobalCooldown(float worldTime, float penalty)
	{
	    m_fGlobalCooldownUntil = Math.Clamp(
	        Math.Max(m_fGlobalCooldownUntil, worldTime) + penalty,
	        worldTime,
	        worldTime + m_fMaxGlobalCooldown
	    );
	}

	void RegisterArtilleryGroup(DCO_GroupUtilityComponent grp)
	{
		if (!grp)
			return;
		
		if (m_aUnits.Contains(grp))
			return;
		
		m_aUnits.Insert(grp);
	}
 
	// Hapus unit dari registry — dipanggil saat group mati/dissolved
	void UnregisterArtilleryGroup(DCO_GroupUtilityComponent grp)
	{
		if (!grp)
			return;
		
		if (m_aUnits.Contains(grp))
		{
			m_aUnits.RemoveItemOrdered(grp);
		}
	}
	
	// === ADDED: Artillery Availability Check -- public getter, biar caller (misal
	// CommanderThreatResponse.c) bisa cek "ada gak artillery yang beneran terdaftar"
	// SEBELUM masukin request ke queue -- daripada request nyangkut di queue sampe
	// expired tanpa feedback apapun ke pemohon.
	bool HasRegisteredUnits()
	{
		return !m_aUnits.IsEmpty();
	}
	// === END ADDED ===
 
	// === MODIFIED: sekarang beneran roll m_fBaseRejectionChance sebelum queue ===
	void RequestShellImpact(vector impactPos, SCR_EAIArtilleryAmmoType shellType, float worldTime, int shellCount)
	{
	    if (!Replication.IsServer())
	        return;
	    
	    // === ADDED: rejection chance yang sebelumnya dideklarasi tapi gak pernah dipake ===
	    if (m_fBaseRejectionChance > 0 && Math.RandomFloat01() < m_fBaseRejectionChance)
	    {
	        Print(string.Format("[CMD_ArtillerySupport] Request DITOLAK (random rejection roll) @ %1", impactPos.ToString()));
	        return;
	    }
	    // === END ADDED ===
	    
	    // Queue penuh — tambah penalty cooldown dan tolak
	    if (m_aQueue.Count() >= m_iMaxQueueSize)
	    {
	        int overflow       = m_aQueue.Count() - m_iMaxQueueSize + 1;
	        float penalty      = m_fQueueOverloadPenalty * overflow;
	        m_fGlobalCooldownUntil = Math.Clamp(
	            Math.Max(m_fGlobalCooldownUntil, worldTime) + penalty,
	            worldTime,
	            worldTime + m_fMaxGlobalCooldown
	        );
	        return;
	    }
	    
	    CMD_FireMissionRequest req = new CMD_FireMissionRequest(impactPos, shellType, worldTime, shellCount);
	    m_aQueue.Insert(req);
	}
	// === END MODIFIED ===
 
	// Safety check — return true jika ada friendly dalam radius dari pos.
	// Dipanggil CMD_ThreatResponseComponent sebelum fire mission dikirim.
	bool HasFriendlyNearPos(vector pos, float radius)
	{
		if (!m_Commander)
			return false;
 
		m_bFriendlyFound   = false;
		m_sFriendlyFaction = m_Commander.GetCommanderFactionKey();
 
		GetGame().GetWorld().QueryEntitiesBySphere(
			pos,
			radius,
			FriendlySafetyCallback,
			null,
			EQueryEntitiesFlags.ALL);
 
		return m_bFriendlyFound;
	}
 
	// Shorthand — pakai radius default dari attribute
	bool HasFriendlyNearPosDefault(vector pos)
	{
		return HasFriendlyNearPos(pos, m_fFriendlySafeRadius);
	}
 
	// === MODIFIED: samain definisi "available" sama FindClosestAvailableUnit --
	// harus IDLE eksplisit, bukan cuma "bukan EXECUTING_COMMAND" ===
	bool HasAnyAvailableUnit()
	{
		foreach (DCO_GroupUtilityComponent unit : m_aUnits)
		{
			if (unit && unit.GetGroupStatus() == DCOG_EGroupStatus.IDLE)
				return true;
		}
		return false;
	}
 
	protected void ProcessQueue(float worldTime)
	{
	    PurgeExpiredRequests(worldTime);
	    PurgeDeadUnits();
	    
	    // === ADDED: Walking Rounds In -- cleanup tracking yang expired, piggyback
	    // cycle ProcessQueue yang udah ada biar gak nambah tick/interval baru.
	    CleanupExpiredCorrectionTracks(worldTime);
	    // === END ADDED ===
	
	    // === ADDED: null-check m_Commander -- sebelumnya gak ada, padahal dipake
	    // langsung di bawah (GetOwner().GetOrigin(), SpawnArtilleryWP). Kalau
	    // AICommander_BaseComponent gak ketemu pas EOnInit, ini bisa null-deref
	    // crash begitu ada request masuk queue. ===
	    if (!m_Commander)
	        return;
	    // === END ADDED ===
	    
	    if (m_aQueue.IsEmpty())
	        return;
	    if (m_aUnits.IsEmpty())
	        return;
	
	    // === MODIFIED: global cooldown check diaktifin lagi (sebelumnya di-comment,
	    // dihitung tapi gak pernah dienforce) ===
	    if (worldTime < m_fGlobalCooldownUntil)
	        return;
	    // === END MODIFIED ===
	
	    if (worldTime < m_fNextProcessTime)
	        return;
		
		int shellsFired = 0;
		
		// === MODIFIED: sebelumnya "foreach + RemoveItem" di array yang sama --
		// modifying array while iterating, request ke-2+ dalam 1 cycle bisa
		// ke-skip karena index geser abis remove. Diganti index-based while loop,
		// konsisten sama pola PurgeExpiredRequests/PurgeDeadUnits. ===
		int qi = 0;
		while (qi < m_aQueue.Count())
	    {
		    CMD_FireMissionRequest req = m_aQueue[qi];
		    if (!req)
	    	{
		    	m_aQueue.Remove(qi);
		    	continue;
	    	}
			
		    DCO_GroupUtilityComponent unit = FindClosestAvailableUnit(req.m_eShellType, req.m_vImpactPos, worldTime);
		    if (!unit)
		    {
		        qi++;
		        continue;
		    }
		
		    array<vector> artyPoint = GenerateArtilleryImpactPoints(req, m_Commander.GetOwner().GetOrigin(), worldTime, req.m_iShellCount);
		    
		    foreach (vector v : artyPoint)
		    {
		        SCR_AIWaypoint wp = m_Commander.SpawnArtilleryWP(v);
		        if (!wp)
		            continue;
		
		        SCR_AIWaypointArtillerySupport wps = SCR_AIWaypointArtillerySupport.Cast(wp);
		        wps.SetTargetShotCount(1);
		        wps.SetActive(true);
		
		        unit.ShootMortar(wps, worldTime);
		        shellsFired++;
		    }
		
		    m_aQueue.Remove(qi);
		}
		// === END MODIFIED ===

	    m_iTotalShellsFired = m_iTotalShellsFired + shellsFired;
	
	    m_fNextProcessTime = worldTime + m_fQueueProcessTime;
	
	    float shellCooldown = Math.Clamp(
	        m_fGlobalCooldown + (shellsFired * m_fCooldownPerShell),
	        m_fMinGlobalCooldown,
	        m_fMaxGlobalCooldown
	    );
	
	    float queueRatio = 0.0;
	    if (m_iMaxQueueSize > 0)
	        queueRatio = Math.Clamp(m_aQueue.Count() / m_iMaxQueueSize, 0.0, 1.0);
	
	    float finalCooldown = Math.Lerp(shellCooldown, m_fMinGlobalCooldown, queueRatio * 0.5);
	    m_fGlobalCooldownUntil = worldTime + finalCooldown;
	}
	
	array<vector> GenerateArtilleryImpactPoints(CMD_FireMissionRequest request, vector commanderPos, float worldTime, int shells = 1)
	{
	    float dispersion         = CalculateArtilleryDispersion(commanderPos, request.m_vImpactPos, request.m_eShellType);
	    float accuracy           = CalculateArtilleryAccuracy(request, commanderPos, worldTime);
	    float effectiveDispersion = dispersion * (1.0 - Math.Clamp(accuracy, 0.0, 1.0));
	    float minRadius          = Math.Max(effectiveDispersion * 0.1, 1.0);
	
	    array<vector> impactPoints = new array<vector>();
	    RandomGenerator rand       = new RandomGenerator();
	
	    for (int i = 0; i < shells; i++)
	    {
	        float radius2;
	
	        if (m_fClusterRatio >= 0.5)
	        {
	            // Lebih clustering — average beberapa sample biar distribusi mengerucut ke center
	            int sampleCount = Math.Round(Math.Lerp(1.0, 4.0, m_fClusterRatio));
	            float sum = 0.0;
	            for (int s = 0; s < sampleCount; s++)
	                sum += rand.RandFloatXY(minRadius, effectiveDispersion);
	            radius2 = sum / sampleCount;
	        }
	        else
	        {
	            // Lebih flat/spread — pure random
	            radius2 = rand.RandFloatXY(minRadius, effectiveDispersion);
	        }
	
	        float angleDeg = rand.RandFloatXY(0.0, 360.0);
	        float angleRad = angleDeg * Math.DEG2RAD;
	
	        float px = request.m_vImpactPos[0] + Math.Cos(angleRad) * radius2;
	        float pz = request.m_vImpactPos[2] + Math.Sin(angleRad) * radius2;
	        float py = GetGame().GetWorld().GetSurfaceY(px, pz);
	
	        impactPoints.Insert(Vector(px, py, pz));
	    }
	
	    return impactPoints;
	}
	
	float CalculateArtilleryRadius(vector commanderPos, vector impactPos, SCR_EAIArtilleryAmmoType shellType)
	{
	    float rangeToTarget = vector.Distance(commanderPos, impactPos);
	
	    float baseRadius = Math.Clamp(rangeToTarget * 0.08, 25.0, 120.0);
	
	    switch (shellType)
	    {
	        case SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE:
	            baseRadius *= 1.0;
	            break;
	        case SCR_EAIArtilleryAmmoType.SMOKE:
	            baseRadius *= 1.3;
	            break;
	        case SCR_EAIArtilleryAmmoType.ILLUMINATION:
	            baseRadius *= 1.8;
	            break;
	        case SCR_EAIArtilleryAmmoType.PRACTICE:
	            baseRadius *= 0.7;
	            break;
	    }
	
	    return baseRadius;
	}
	
	// === Satu-satunya kalkulasi dispersion yang beneran dipake buat tembakan --
	// CommanderThreatResponse.c sekarang gak punya versi sendiri lagi. ===
	float CalculateArtilleryDispersion(vector commanderPos, vector impactPos, SCR_EAIArtilleryAmmoType shellType)
	{
	    float rangeToTarget  = vector.Distance(commanderPos, impactPos);
	    float baseDispersion = Math.Clamp(rangeToTarget * m_fDispersionRangeScale, m_fMinDispersion, m_fMaxDispersion);
	
	    switch (shellType)
	    {
	        case SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE:
	            baseDispersion *= 1.0;
	            break;
	        case SCR_EAIArtilleryAmmoType.SMOKE:
	            baseDispersion *= 1.2;
	            break;
	        case SCR_EAIArtilleryAmmoType.ILLUMINATION:
	            baseDispersion *= 1.5;
	            break;
	        case SCR_EAIArtilleryAmmoType.PRACTICE:
	            baseDispersion *= 0.8;
	            break;
	    }
	
	    return Math.Clamp(baseDispersion * m_fDispersionMultiplier, m_fMinDispersion, m_fMaxDispersion);
	}
	
	float CalculateArtilleryAccuracy(CMD_FireMissionRequest request, vector commanderPos, float worldTime)
	{
	    float accuracy       = m_fBaseAccuracy;
	    float rangeToTarget  = vector.Distance(commanderPos, request.m_vImpactPos);
	    
	    // === MODIFIED: BUG FIX -- sebelumnya dataAge pake m_fRequestedTime (waktu
	    // DISPATCH request, hampir selalu ~0 karena CalculateArtilleryAccuracy dipanggil
	    // gak lama setelah request dibikin). Sekarang pake m_fTargetLastSeenTime (kapan
	    // target BENERAN terakhir dikonfirmasi ada di posisi itu, dari threat entry) --
	    // ini yang beneran ngukur "seberapa basi intel-nya", bukan "seberapa cepet
	    // commander mutusin nembak".
	    float dataAge = worldTime - request.m_fTargetLastSeenTime;
	    // === END MODIFIED ===

	    if (dataAge < 30.0)
	        accuracy += 0.10;

	    if (request.m_eShellType == SCR_EAIArtilleryAmmoType.PRACTICE)
	        accuracy += 0.20;

	    if (dataAge > 30.0)
	        accuracy -= Math.Clamp((dataAge - 30.0) * 0.01, 0.0, 0.25);

	    accuracy -= Math.Clamp(rangeToTarget * 0.0001, 0.0, 0.20);

	    if (request.m_eShellType == SCR_EAIArtilleryAmmoType.ILLUMINATION)
	        accuracy -= 0.15;

	    if (request.m_eShellType == SCR_EAIArtilleryAmmoType.SMOKE)
	        accuracy -= 0.10;
	    
	    // === ADDED: Bagian 1 -- Report Quality. Laporan yang dibuat dari jarak deket
	    // (spotter jelas liat) nambah akurasi, laporan dari jauh (perkiraan kasar)
	    // ngurangin. Quality 0.7 = netral (gak nambah gak ngurangin), di bawah/atas
	    // itu geser proporsional.
	    accuracy += (request.m_fReportQuality - 0.7) * 0.5;
	    // === END ADDED ===
	    
	    // === ADDED: Bagian 2 -- Walking Rounds In. Tembakan berulang ke area yang
	    // sama (radius m_fCorrectionRadius, dalem window m_fCorrectionExpiry) dapet
	    // bonus akurasi tambahan, plateau di koreksi ke-m_iMaxCorrections.
	    accuracy += GetCorrectionBonus(request.m_vImpactPos, worldTime);
	    // === END ADDED ===

	    return Math.Clamp(accuracy, 0.05, 0.95);
	}
	
	// === ADDED: Walking Rounds In ===
	//! Cari/bikin tracking koreksi buat posisi ini, update counter-nya, return bonus
	//! akurasi yang berlaku. Dipanggil SEKALI per fire mission (dari
	//! CalculateArtilleryAccuracy), jadi counter naik tiap kali ada request baru ke
	//! area yang sama.
	protected float GetCorrectionBonus(vector impactPos, float worldTime)
	{
	    CMD_ArtilleryCorrectionTrack found = null;
	    
	    foreach (CMD_ArtilleryCorrectionTrack t : m_aCorrectionTracks)
	    {
	        if (!t)
	            continue;
	        
	        if ((worldTime - t.m_fLastCallTime) > m_fCorrectionExpiry)
	            continue;
	        
	        if (vector.Distance(t.m_vPosition, impactPos) <= m_fCorrectionRadius)
	        {
	            found = t;
	            break;
	        }
	    }
	    
	    if (!found)
	    {
	        found = new CMD_ArtilleryCorrectionTrack();
	        found.m_vPosition       = impactPos;
	        found.m_iCorrectionStep = 0;
	        m_aCorrectionTracks.Insert(found);
	    }
	    
	    found.m_iCorrectionStep = Math.Min(found.m_iCorrectionStep + 1, m_iMaxCorrections);
	    found.m_fLastCallTime   = worldTime;
	    found.m_vPosition       = impactPos;
	    
	    return (found.m_iCorrectionStep - 1) * m_fCorrectionBonusPerStep;
	}
	
	protected void CleanupExpiredCorrectionTracks(float worldTime)
	{
	    for (int i = m_aCorrectionTracks.Count() - 1; i >= 0; i--)
	    {
	        CMD_ArtilleryCorrectionTrack t = m_aCorrectionTracks[i];
	        if (!t || (worldTime - t.m_fLastCallTime) > m_fCorrectionExpiry)
	            m_aCorrectionTracks.Remove(i);
	    }
	}
	// === END ADDED ===

 
	// === MODIFIED: sekarang require IDLE eksplisit, samain sama HasAnyAvailableUnit
	// -- sebelumnya cuma exclude EXECUTING_COMMAND, bisa nyolong unit yang statusnya
	// bukan IDLE tapi juga bukan EXECUTING_COMMAND (misal lagi assigned ke role lain) ===
	protected DCO_GroupUtilityComponent FindClosestAvailableUnit(SCR_EAIArtilleryAmmoType shellType, vector targetPos, float worldTime)
	{
		DCO_GroupUtilityComponent bestUnit   = null;
		float             bestDistSq = -1.0;
 
		foreach (DCO_GroupUtilityComponent unit : m_aUnits)
		{
			if (!unit)
				continue;
			
			if (unit.GetGroupStatus() != DCOG_EGroupStatus.IDLE)
				continue;
 
			float distSq = vector.DistanceSq(unit.GetOwner().GetOrigin(), targetPos);
 
			if (bestDistSq < 0.0 || distSq < bestDistSq)
			{
				bestDistSq = distSq;
				bestUnit   = unit;
			}
		}
 
		return bestUnit;
	}
	// === END MODIFIED ===
 
	protected void PurgeExpiredRequests(float worldTime)
	{
		int i = 0;
		while (i < m_aQueue.Count())
		{
			CMD_FireMissionRequest req = m_aQueue[i];
			if (!req || worldTime - req.m_fRequestedTime > m_fQueueExpiry)
			{
				m_aQueue.Remove(i);
				continue;
			}
			i++;
		}
	}
 
	protected void PurgeDeadUnits()
	{
		int i = 0;
		while (i < m_aUnits.Count())
		{
			DCO_GroupUtilityComponent unit = m_aUnits[i];
			if (!unit)
			{
				m_aUnits.Remove(i);
				continue;
			}
			i++;
		}
	}
 
	// Callback untuk QueryEntitiesBySphere — return false = stop query
	protected bool FriendlySafetyCallback(IEntity ent)
	{
		if (!ent)
			return true; // lanjut iterasi
 
		FactionAffiliationComponent facComp = FactionAffiliationComponent.Cast(
			ent.FindComponent(FactionAffiliationComponent));
 
		if (!facComp)
			return true;
 
		Faction faction = facComp.GetAffiliatedFaction();
		if (!faction)
			return true;
 
		if (faction.GetFactionKey() == m_sFriendlyFaction)
		{
			m_bFriendlyFound = true;
			return false; // stop — friendly ditemukan
		}
 
		return true;
	}
 
	//====================================================================
	// LIFECYCLE
	//====================================================================
 
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer())
			return;
 
		m_fDispatchTimer = m_fDispatchTimer + timeSlice;
		if (m_fDispatchTimer < m_fDispatchInterval)
			return;
 
		m_fDispatchTimer = 0.0;
		float worldTime  = GetGame().GetWorld().GetWorldTime() / 1000.0;
		ProcessQueue(worldTime);
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
		if (!Replication.IsServer())
			return;
 
		m_Commander = AICommander_BaseComponent.Cast(owner.FindComponent(AICommander_BaseComponent));
 
		if (!m_Commander)
		{
			Print("[CMD_ArtillerySupport] PERINGATAN — AICommander_BaseComponent tidak ditemukan. " +
				"Friendly safety check dan ProcessQueue tidak akan berfungsi (di-guard, gak crash, tapi artillery gak akan pernah nembak).");
		}
		else
		{
			Print("[CMD_ArtillerySupport] Initialized, menunggu unit artillery daftar.");
		}
	}
}

// === ADDED: Walking Rounds In -- top-level class, konsisten sama pola
// CMD_ThreatCluster/CMD_ContactReport/dll di file lain. Nyimpen state "udah
// berapa kali koreksi berturut-turut" buat 1 area tembakan (radius+window waktu
// tertentu, lihat m_fCorrectionRadius/m_fCorrectionExpiry).
class CMD_ArtilleryCorrectionTrack
{
	vector m_vPosition;
	int    m_iCorrectionStep;
	float  m_fLastCallTime;
}
// === END ADDED ===