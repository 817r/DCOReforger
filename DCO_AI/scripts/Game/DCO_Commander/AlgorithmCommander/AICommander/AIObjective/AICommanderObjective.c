[ComponentEditorProps(category: "GameScripted/AI/AICommander", description: "Component for AI Commander Objective")]
class CMD_AICommanderObjectiveComponentClass : ScriptComponentClass
{
}

class CMD_AICommanderObjectiveComponent : ScriptComponent
{
	[Attribute("50.0", UIWidgets.EditBox, "Base strategic value (0–100). Makin tinggi makin penting.", category: "Objective")]
	protected float m_fBaseValue;
	
	[Attribute("150.0", UIWidgets.EditBox, "Radius (meter) untuk deteksi musuh di sekitar objective ini.", category: "Objective")]
	protected float m_fThreatRadius;
 
	[Attribute("200.0", UIWidgets.EditBox, "Radius (meter) untuk cek friendly presence.", category: "Objective")]
	protected float m_fFriendlyRadius;
	
	[Attribute("30.0", UIWidgets.Slider, "Radius of the Objective", "1.0 300.0 1.0")]
	protected float m_fRadius;
	
	// === ADDED: Intel Fog System ===
	[Attribute("400.0", UIWidgets.EditBox, "Radius (meter) intel coverage yang di-provide objective ini ke objective LAIN di sekitarnya. Cuma relevan kalau ObjectiveType == RECON.", category: "Intel")]
	protected float m_fIntelCoverageRadius;
	// === END ADDED ===
	
	// === REMOVED: Proximity & Relevance -- dicabut total dari scoring objective
	// non-RECON. Priority sekarang murni base value + enemy count + contested +
	// friendly penalty + assigned penalty.
	// === END REMOVED ===
	
	// === ADDED: Priority -- Proximity ===
	[Attribute("2500.0", UIWidgets.EditBox, "Jarak (meter) dari commander di mana proximity bonus abis (0 di jarak ini, penuh di jarak 0).", category: "Priority")]
	protected float m_fMaxRelevantDistance;
	// === END ADDED ===
	
	
	[Attribute("0", UIWidgets.ComboBox, "Tipe objective ini", "", ParamEnumArray.FromEnum(CMD_EObjectiveType))]
	CMD_EObjectiveType m_eObjectiveType;
 
	[Attribute("60.0", UIWidgets.EditBox, "Detik yang dibutuhkan untuk capture (groups harus di area)", category: "Objective")]
	protected float m_fCaptureHoldDuration;
 
	[Attribute("2", UIWidgets.EditBox, "Berapa group yang di-assign untuk defend setelah captured", category: "Objective")]
	protected int m_iDefendGroupCount;
	
	[Attribute("4", UIWidgets.EditBox, "Berapa group yang di-assign untuk attack objective", category: "Objective")]
	protected int m_iMaxGroupCount;
	
	[Attribute("", UIWidgets.Auto, "Blacklisted Commander to not process this objective", category: "Objective")]
	protected ref array<string> m_sBlacklistedCo;
	
	[Attribute("", UIWidgets.Auto, "Set Captured by Commander for this objective", category: "Objective")]
	protected ref array<string> m_sCapturedCo;
	
	protected float m_fLastProgressTime   = 0.0;
	protected float m_fStaleStartTime     = 0.0;
	
	[Attribute("300.0", UIWidgets.EditBox, "Detik tanpa progress sebelum dianggap stalemate", category: "Objective")]
	protected float m_fStalemateThreshold;
 
	ref map<FactionKey, float> m_mCaptureStartTime = new map<FactionKey, float>();
	ref map<FactionKey, bool>  m_mIsCaptured       = new map<FactionKey, bool>();
	
	ref map<FactionKey, CMD_EObjectiveState> m_mObjectiveState = new map<FactionKey, CMD_EObjectiveState>();
	ref map<FactionKey, int> m_mObjectiveAssignedGroup = new map<FactionKey, int>();
	protected ref map<FactionKey, DCO_GroupUtilityComponent> m_mReconGroup = new map<FactionKey, DCO_GroupUtilityComponent>();
	
	protected ref map<FactionKey, bool> m_mLostStatus = new map<FactionKey, bool>();
	
	IEntity m_OwnerEntity;
	
	protected float m_fCachedScore    = 0.0;
	protected float m_fScoreCacheAge  = 0.0;
	static const float CACHE_DURATION = 3.0;
 
	protected float m_fLastContestedTime = 0.0;
	
	float GetRadius()
	{
		return m_fRadius;
	}
	
	float GetCaptureProgress(FactionKey fk, float worldTime)
	{
	    float startTime;
	    if (!m_mCaptureStartTime.Find(fk, startTime))
	        return 0.0;
	
	    float elapsed = worldTime - startTime;
	    
	    // === MODIFIED: Optimasi -- 1 query buat friendly+enemy sekaligus (radius sama) ===
	    int friendlyCount, enemyCount;
	    CountNearbyUnitsBoth(m_fRadius, fk, friendlyCount, enemyCount);
	    
	    if (friendlyCount < enemyCount)
	    {
	        elapsed = 0;
	    }
	    else
	    {
	        // Ada progress nyata — update timestamp
	        m_fLastProgressTime = worldTime;
	    }
	    // === END MODIFIED ===
	
	    // === REVERTED: Intel Fog gak jadi ngaruh ke capture SPEED. Efeknya murni di
	    // KEPUTUSAN commander mau komit apa enggak (RiskTaking gate, di AssignRolesToObjective
	    // di AICommanderBase.c) -- objective RECON bikin commander lebih percaya diri buat
	    // ngecapture objective sekitarnya, bukan bikin capture-nya sendiri lebih cepet/lambat
	    // secara mekanik begitu udah komit.
	    // === END REVERTED ===

	    return Math.Clamp(elapsed / m_fCaptureHoldDuration, 0.0, 1.0);
	}
	
	bool IsStalemate(FactionKey fk, float worldTime)
	{
	    // Hanya relevan kalau capture timer udah jalan
	    if (!IsCaptureTimerRunning(fk))
	        return false;
	
	    // Kalau belum pernah ada progress, hitung dari start timer
	    float referenceTime = m_fLastProgressTime;
	    if (referenceTime <= 0.0)
	    {
	        float startTime;
	        if (!m_mCaptureStartTime.Find(fk, startTime))
	            return false;
	        referenceTime = startTime;
	    }
	
	    return (worldTime - referenceTime) >= m_fStalemateThreshold;
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
		m_fScoreCacheAge     = 0.0; // invalidate cache
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
	float ComputePriorityScore(FactionKey forFaction, float worldTime, vector commanderPos)
	{
	    CMD_EObjectiveState currentState = GetObjectiveState(forFaction);
	
	    if (currentState == CMD_EObjectiveState.COMPLETED || currentState == CMD_EObjectiveState.FAILED)
	        return 0.0;
	
	    if ((worldTime - m_fScoreCacheAge) < CACHE_DURATION)
	        return m_fCachedScore;
	
	    float score = m_fBaseValue;
	
	    int enemyCount = CountNearbyUnits(m_fThreatRadius, forFaction, false);
	    if (enemyCount > 0)
	        score += Math.Clamp(enemyCount * 8.0, 0.0, 50.0);
	
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
	
	    // === ADDED: Proximity bonus -- jarak ke commander + base value objective ini
	    // sendiri. Makin deket (relatif ke m_fMaxRelevantDistance), makin gede bonusnya,
	    // diskalain sama base value (0 di jarak >= m_fMaxRelevantDistance, sampe
	    // m_fBaseValue penuh di jarak 0).
	    float distToCommander  = vector.Distance(commanderPos, GetOwner().GetOrigin());
	    float proximityFactor  = Math.Clamp(1.0 - (distToCommander / m_fMaxRelevantDistance), 0.0, 1.0);
	    float proximityBonus   = proximityFactor * m_fBaseValue;
	    score += proximityBonus;
	    // === END ADDED ===

	    m_fCachedScore   = Math.Max(score, 0.0);
	    m_fScoreCacheAge = worldTime;
	
	    return m_fCachedScore;
	}
	
	bool QueryCallback(IEntity e)
	{
		if (!nearby.Contains(e))
			nearby.Insert(e);
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
		GetGame().GetWorld().QueryEntitiesBySphere(pos, radius, null, QueryCallback, EQueryEntitiesFlags.ALL);
		
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
 
			bool sameFaction = (fc.GetFactionKey() == factionKey);
 
			if (isFriendly && sameFaction)
				count++;
			else if (!isFriendly && !sameFaction)
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
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		InitializeObjective();
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
	}

	void StartCaptureTimer(FactionKey fk, float worldTime)
	{
		if (!m_mCaptureStartTime.Contains(fk))
			m_mCaptureStartTime.Insert(fk, worldTime);
		else
			m_mCaptureStartTime.Set(fk, worldTime);
 
		//Print(string.Format("[CMD_Objective] %1 | %2 capture timer started (hold %3s)",
			//GetOwner().GetName(), fk, m_fCaptureHoldDuration.ToString()));
	}
 
	bool IsCaptureTimerRunning(FactionKey fk)
	{
		return m_mCaptureStartTime.Contains(fk);
	}
 
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
	
	//! Faction key yang UDAH BERHASIL capture (owner objective ini SEKARANG).
	//! Return string kosong kalau belum ada faction manapun yang capture.
	FactionKey GetOwningFaction()
	{
		foreach (FactionKey key, bool captured : m_mIsCaptured)
		{
			if (captured)
				return key;
		}
		return string.Empty;
	}
	
	//! Shortcut buat UI -- progress 0-100 (persen) buat faction yang LAGI capture
	//! SAAT INI (GetActiveCapturingFaction()). Return 0 kalau gak ada yang lagi capture.
	float GetActiveCaptureProgressPercent(float worldTime)
	{
		FactionKey activeFaction = GetActiveCapturingFaction();
		if (activeFaction.IsEmpty())
			return 0.0;
		
		return GetCaptureProgress(activeFaction, worldTime) * 100.0;
	}
	// === END ADDED ===
	
	bool IsCapturedBy(FactionKey fk, string cuid = "")
	{
		bool captured;
		if (!cuid.IsEmpty())
		{
			if (m_sCapturedCo.Contains(cuid))
			{
				//Print("FOUND " + cuid + " IN OBJECTIVE ");
				if (!m_mIsCaptured.Contains(fk))
				{
					SetCapturedBy(fk, true);
					return true;
				}
				else if (m_mIsCaptured.Find(fk, captured))
				{
					//Print("IS " + cuid + " CAPTURE? " + captured.ToString() + " < IsCaptured Map Found");
					return captured;
						
				}
			}
		} else
			//Print("NO CUID FOUND IN OBJECTIVE : " + cuid);
		
		bool refre = false;
		
		//Print("IS " + fk + " CAPTURE UPDATED ? " + m_mIsCaptured.Find(fk, refre).ToString());
		
		if (m_mIsCaptured.Find(fk, captured))
			return captured;
		return false;
	}

	bool CheckAndMarkIfLost(FactionKey fk)
	{
		bool alreadyLost;
		if (m_mLostStatus.Find(fk, alreadyLost) && alreadyLost)
			return true;
	
		// === MODIFIED: Optimasi -- 1 query buat friendly+enemy sekaligus (radius sama) ===
		int friendlyCount, enemyCount;
		CountNearbyUnitsBoth(m_fRadius, fk, friendlyCount, enemyCount);
		// === END MODIFIED ===
		
		if (friendlyCount == 0 && IsCapturedBy(fk, string.Empty))
		{
			MarkLost(fk);
			return true;
		}
	
		if (friendlyCount > 0 && enemyCount >= friendlyCount * 3)
		{
			MarkLost(fk);
			return true;
		}
	
		return false;
	}
	
	bool CheckIsItLost(FactionKey fk)
	{
		bool alreadyLost;
		if (m_mLostStatus.Find(fk, alreadyLost) && alreadyLost)
			return true;	
		return false;
	}
	
	protected void MarkLost(FactionKey fk)
	{
		if (!m_mLostStatus.Contains(fk))
			m_mLostStatus.Insert(fk, true);
		else
			m_mLostStatus.Set(fk, true);
	
		// Invalidate captured status
		SetCapturedBy(fk, false);
	
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
		if (!m_mIsCaptured.Contains(fk))
			m_mIsCaptured.Insert(fk, val);
		else
			m_mIsCaptured.Set(fk, val);

		//Print("SET " + fk + " CAPTURE " + val);
	}
 
	bool IsGroupSlotFull(FactionKey fk)
	{
		return GetCurrentAssignedGroupCount(fk) >= GetRequiredGroupCount();
	}
}