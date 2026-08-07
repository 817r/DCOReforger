[ComponentEditorProps(category: "GameScripted/Group", description: "Detects enemies and reports contact to Commander")]
class DCO_GroupContactReporterComponentClass : ScriptComponentClass {}

class DCO_GroupContactReporterComponent : ScriptComponent
{
	[Attribute("10.0", UIWidgets.EditBox, "Interval scan enemy (Second)", category: "Contact")]
	protected float m_fScanInterval;

	[Attribute("3", UIWidgets.EditBox, "Minimum enemy detected before report", category: "Contact")]
	protected int m_iMinEnemyToReport;

	[Attribute("5", UIWidgets.EditBox, "Minimum Unit Count for reinforcement request", category: "Contact")]
	protected int m_iReinforcementThreshold;

	[Attribute("6", UIWidgets.EditBox, "Minimum enemy visible untuk request artillery support", category: "Artillery")]
	protected int m_iArtilleryEnemyThreshold;

	[Attribute("180.0", UIWidgets.EditBox, "Cooldown antara artillery requests dari group ini (detik)", category: "Artillery")]
	protected float m_fArtilleryRequestCooldown;

	[Attribute("90.0", UIWidgets.EditBox, "Detik sebelum reinforcement request flag di-reset paksa walau grup belum IDLE (nyegah stuck permanen)", category: "Contact")]
	protected float m_fReinfRequestTimeout;

	[Attribute("0.15", UIWidgets.Range, "Chance artillery request DITOLAK kalau role grup bukan RECON/FLANK (0 = gak pernah ditolak, 1 = selalu ditolak)", params: "0 1 0.01", category: "Artillery")]
	protected float m_fNonFavoredRoleArtyRejectChance;

	//--------------------------------------------------------------------
	protected DCO_GroupUtilityComponent  m_GroupUtil;
	protected IEntity					 m_MyEntity;
	protected SCR_AIGroup                m_Group;
	protected float                      m_fScanTimer          = 0.0;
	protected bool                       m_bReinfRequested     = false;
	protected float                      m_fLastArtilleryReqAt = -999.0;
	protected float                      m_fLastReinfRequestAt = -999.0;

	//--------------------------------------------------------------------
	protected void ReportContact(vector contactPos, int enemyCount, float worldTime)
	{
		CMD_ContactReport report = new CMD_ContactReport(
			contactPos,
			enemyCount,
			worldTime,
			GetOwner().GetName());

		m_GroupUtil.GetThreatResponseComponent().ReceiveContactReport(report, m_GroupUtil);
	}
	
	bool CanChangeRole()
	{
		return m_GroupUtil.CanCommanderOverrideRole();
	}

	protected void RequestReinforcement(float worldTime)
	{
		if (m_bReinfRequested)
			return;

		if (!m_GroupUtil)
			return;
		
		if (!m_GroupUtil.CanCallReinforcement())
			return;

		m_bReinfRequested = true;
		m_fLastReinfRequestAt = worldTime; // === ADDED: A ===
		m_GroupUtil.GetThreatResponseComponent().ReceiveReinforcementRequest(m_GroupUtil, worldTime);
	}

	// === MODIFIED: nambah param enemyCount buat nge-skala shellCount, dipake buat
	// ngelengkapin constructor CMD_FireMissionRequest yang butuh 4 argumen ===
	protected void RequestArtillerySupport(vector contactPos, int enemyCount, float worldTime)
	{
	    if (!m_GroupUtil)
	        return;
		
		if (!m_GroupUtil.CanCallArty())
			return;

	    if (worldTime - m_fLastArtilleryReqAt < m_fArtilleryRequestCooldown)
	        return;

	    CMD_EGroupRole role = m_GroupUtil.GetGroupRole();
	    if (role != CMD_EGroupRole.RECON)
	    {
	        if (Math.RandomFloat01() < m_fNonFavoredRoleArtyRejectChance)
	        {
	            Print(string.Format("[DCO_Reporter] Artillery request DITOLAK -- role %1 bukan prioritas buat call-in artillery", role));
	            return;
	        }
	    }
	
	    CMD_ThreatResponseComponent threatComp = m_GroupUtil.GetThreatResponseComponent();
	    if (!threatComp)
	        return;
	
	    m_fLastArtilleryReqAt = worldTime;

	    // Skala shellCount dari jumlah musuh yang keliatan -- clamp biar gak ekstrem
	    int shellCount = Math.Clamp(Math.Round(enemyCount * 0.5), 2, 8);
	
	    CMD_FireMissionRequest request = new CMD_FireMissionRequest(
	        contactPos,
	        DetermineShellType(worldTime),
	        worldTime,
	        shellCount
	    );
	
	    threatComp.ReceiveArtillerySupport(request, m_GroupUtil);
	
	    Print(string.Format("[DCO_Reporter] %1 requested artillery @ %2 (%3 shell)",
	        GetOwner().GetName(), contactPos.ToString(), shellCount));
	}
	// === END MODIFIED ===

	// === MODIFIED: fix return type mismatch (true/false -> enum yang bener), fix
	// "protected" invalid di local variable, fix logic malam (&& -> ||). Parameter
	// contactPos dihapus karena emang gak pernah dipake di dalam function-nya. ===
	SCR_EAIArtilleryAmmoType DetermineShellType(float worldTime)
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetOwner().GetWorld());
		if (!world)
			return SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE; // fallback aman kalau world gak ketemu

		TimeAndWeatherManagerEntity manager = world.GetTimeAndWeatherManager();
		if (!manager)
			return SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE; // fallback aman kalau manager gak ketemu

		float sunriseTime;
		float sunsetTime;

		float currentTime = manager.GetTimeOfTheDay();
		if (!manager.GetSunriseHour(sunriseTime) || !manager.GetSunsetHour(sunsetTime))
			return SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE; // fallback aman kalau lookup sunrise/sunset gagal

		// MODIFIED: && -> || -- sebelumnya kondisi ini mustahil true
		if (currentTime < sunriseTime || currentTime > sunsetTime)
			return SCR_EAIArtilleryAmmoType.ILLUMINATION;

		return SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE;
	}
	// === END MODIFIED ===

	// === ADDED: D -- personality Squad Leader (unit leader grup, bukan rata-rata
	// semua anggota) ngaruh ke threshold kapan grup minta bantuan. CAUTIOUS SL lebih
	// gampang cemas -> threshold diturunin (minta bantuan lebih cepet). AGGRESSIVE/
	// RECKLESS SL pede sendiri -> threshold dinaikin (baru minta kalau beneran parah).
	protected float GetPersonalityThresholdScale()
	{
		if (!m_Group)
			return 1.0;

		IEntity leaderEntity = m_Group.GetLeaderEntity();
		if (!leaderEntity)
			return 1.0;

		SCR_AICombatComponent leaderCombat = SCR_AICombatComponent.Cast(leaderEntity.FindComponent(SCR_AICombatComponent));
		if (!leaderCombat)
			return 1.0;

		SCR_AIUtilityComponent leaderUtil = leaderCombat.GetUtilityComponent();
		if (!leaderUtil || !leaderUtil.m_DCOConfig)
			return 1.0;

		switch (leaderUtil.m_DCOConfig.GetPersonality())
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 0.7;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 1.2;
			case DCO_EAIPersonality.RECKLESS:
				return 1.4;
			default:
				return 1.0; // STANDARD
		}
		
		return 1.0;
	}
	// === END ADDED ===

	//--------------------------------------------------------------------
	protected void ScanForEnemies(float worldTime)
	{
		if (!m_Group)
			return;

		Faction fc = m_Group.GetFaction();
		if (!fc)
			return;

		// === MODIFIED: fix type -- Count() natively int, sebelumnya di-declare float
		// yang beresiko implicit narrowing pas dioper ke parameter int ===
		int enemyCount = 0;
		// === END MODIFIED ===

		if (m_GroupUtil.perc && m_GroupUtil.perc.m_aTargets)
			enemyCount = m_GroupUtil.perc.m_aTargets.Count();

		if (enemyCount < m_iMinEnemyToReport)
			return;

		vector reportPos = GetCentroidFromEntities(m_GroupUtil.perc.m_aTargetEntities);

		ReportContact(reportPos, enemyCount, worldTime);

		// === MODIFIED: D -- threshold artillery di-scale personality SL ===
		int effectiveArtyThreshold = Math.Max(1, Math.Round(m_iArtilleryEnemyThreshold * GetPersonalityThresholdScale()));
		if (enemyCount >= effectiveArtyThreshold)
			RequestArtillerySupport(reportPos, enemyCount, worldTime);
		// === END MODIFIED ===
	}

	vector GetCentroidFromEntities(array<IEntity> entities)
	{
		if (!entities || entities.IsEmpty())
			return vector.Zero;

		vector sum = vector.Zero;
		int count  = 0;

		foreach (IEntity ent : entities)
		{
			if (!ent)
				continue;
			sum   = sum + ent.GetOrigin();
			count = count + 1;
		}

		if (count == 0)
			return vector.Zero;

		float inv = 1.0 / count;
		return sum * inv;
	}

	protected void CheckReinforcementNeed(float worldTime)
	{
		if (!m_GroupUtil)
			return;

		if (m_GroupUtil.GetGroupRole() != CMD_EGroupRole.RECON
			&& m_GroupUtil.GetGroupRole() != CMD_EGroupRole.FLANK)
			return;

		if (m_GroupUtil.perc && m_GroupUtil.perc.m_aTargets)
		{
			// === MODIFIED: D -- threshold reinforcement di-scale personality SL ===
			int effectiveReinfThreshold = Math.Max(1, Math.Round(m_iReinforcementThreshold * GetPersonalityThresholdScale()));
			if (m_GroupUtil.perc.m_aTargets.Count() >= effectiveReinfThreshold)
				RequestReinforcement(worldTime);
			// === END MODIFIED ===
		}
	}

	// === MODIFIED: A -- tambah fallback timeout, sekarang butuh worldTime ===
	protected void CheckResetFlags(float worldTime)
	{
		if (!m_GroupUtil)
			return;

		if (m_GroupUtil.GetGroupStatus() == DCOG_EGroupStatus.IDLE)
		{
			m_bReinfRequested = false;
			return;
		}

		// Fallback: kalau udah lama minta reinforcement dan grup masih belum IDLE
		// (mungkin reinforcement-nya sendiri gak pernah kekirim gara-gara gating di
		// commander, atau grup stuck di status lain), reset paksa biar grup ini bisa
		// nyoba minta lagi -- daripada stuck gak bisa minta bantuan selamanya.
		if (m_bReinfRequested && (worldTime - m_fLastReinfRequestAt) > m_fReinfRequestTimeout)
			m_bReinfRequested = false;
	}
	// === END MODIFIED ===

	//--------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer())
			return;

		// === ADDED: null-check m_GroupUtil sebelum akses .perc -- sebelumnya bisa
		// null-deref crash kalau EOnInit ninggalin m_GroupUtil null ===
		if (!m_GroupUtil)
			return;
		// === END ADDED ===
		
		if (!m_GroupUtil.GetMyCommander())
			return;

		m_fScanTimer += timeSlice;
		if (m_fScanTimer <= m_fScanInterval || !m_GroupUtil.perc)
			return;

		m_fScanTimer = 0.0;
		float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;

		ScanForEnemies(worldTime);
		CheckReinforcementNeed(worldTime);
		CheckResetFlags(worldTime);
	}

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		// Behavior ini DIPERTAHANKAN sesuai instruksi -- kalau gak ada
		// AICommander_ManagerComponent, komponen ini emang harus diem total, gak
		// usah ngeproses apapun. Ini bukan bug.
		if (!AICommander_ManagerComponent.GetInstance())
			return;
		
		m_MyEntity = owner;
		m_Group     = SCR_AIGroup.Cast(owner);
		m_GroupUtil = DCO_GroupUtilityComponent.Cast(owner.FindComponent(DCO_GroupUtilityComponent));

		// === MODIFIED: sebelumnya FRAME event CUMA di-enable lewat
		// InitializeContactReport() yang gak dipanggil di manapun dalam file ini --
		// kalau gak ada kode eksternal yang manggil itu, EOnFrame gak PERNAH jalan.
		// Sekarang di-enable LANGSUNG di sini, tapi cuma kalau m_GroupUtil beneran
		// ketemu (kalau gak ketemu, percuma juga di-enable karena EOnFrame bakal
		// early-return terus lewat null-check di atas). ===
		if (m_GroupUtil)
			SetEventMask(owner, EntityEvent.FRAME);
		// === END MODIFIED ===
	}
	
	//! Dipertahankan sebagai public method buat kode lain yang mau re-trigger FRAME
	//! secara manual (misal abis group di-reset/reassign) -- tapi bukan lagi
	//! satu-satunya jalan buat ngaktifin scanning, EOnInit udah self-sufficient.
	void InitializeContactReport()
	{
		SetEventMask(m_MyEntity, EntityEvent.FRAME);	
	}
}