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

	[Attribute("0.5", UIWidgets.Range, "Chance artillery request DITOLAK kalau role grup bukan RECON/FLANK (0 = gak pernah ditolak, 1 = selalu ditolak)", params: "0 1 0.01", category: "Artillery")]
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
		m_fLastReinfRequestAt = worldTime;
		m_GroupUtil.GetThreatResponseComponent().ReceiveReinforcementRequest(m_GroupUtil, worldTime);
	}

	protected void RequestArtillerySupport(vector contactPos, int enemyCount, float worldTime)
	{
	    if (!m_GroupUtil)
	        return;
		
		if (!m_GroupUtil.CanCallArty())
			return;

	    if (worldTime - m_fLastArtilleryReqAt < m_fArtilleryRequestCooldown)
	        return;

	    CMD_EGroupRole role = m_GroupUtil.GetGroupRole();
	    if (role != CMD_EGroupRole.RECON && role != CMD_EGroupRole.FLANK)
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

	    int shellCount = Math.Clamp(Math.Round(enemyCount * 0.5), 2, 8);

	    float spotDistance  = vector.Distance(m_GroupUtil.GetOwner().GetOrigin(), contactPos);
	    float reportQuality = CMD_ThreatResponseComponent.ComputeReportQuality(spotDistance);
	
	    CMD_FireMissionRequest request = new CMD_FireMissionRequest(
	        contactPos,
	        DetermineShellType(worldTime),
	        worldTime,
	        shellCount,
	        worldTime,
	        reportQuality
	    );
	
	    threatComp.ReceiveArtillerySupport(request, m_GroupUtil);
	
	    Print(string.Format("[DCO_Reporter] %1 requested artillery @ %2 (%3 shell)",
	        GetOwner().GetName(), contactPos.ToString(), shellCount));
	}

	SCR_EAIArtilleryAmmoType DetermineShellType(float worldTime)
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetOwner().GetWorld());
		if (!world)
			return SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE;

		TimeAndWeatherManagerEntity manager = world.GetTimeAndWeatherManager();
		if (!manager)
			return SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE;

		float sunriseTime;
		float sunsetTime;

		float currentTime = manager.GetTimeOfTheDay();
		if (!manager.GetSunriseHour(sunriseTime) || !manager.GetSunsetHour(sunsetTime))
			return SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE;

		if (currentTime < sunriseTime || currentTime > sunsetTime)
			return SCR_EAIArtilleryAmmoType.ILLUMINATION;

		return SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE;
	}

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
				return 1.0;
		}
		return 1.0;
	}

	//--------------------------------------------------------------------
	protected void ScanForEnemies(float worldTime)
	{
		if (!m_Group)
			return;

		Faction fc = m_Group.GetFaction();
		if (!fc)
			return;

		int enemyCount = 0;

		if (m_GroupUtil.perc && m_GroupUtil.perc.m_aTargets)
			enemyCount = m_GroupUtil.perc.m_aTargets.Count();

		if (enemyCount < m_iMinEnemyToReport)
			return;

		vector reportPos = GetCentroidFromEntities(m_GroupUtil.perc.m_aTargetEntities);

		ReportContact(reportPos, enemyCount, worldTime);

		int effectiveArtyThreshold = Math.Max(1, Math.Round(m_iArtilleryEnemyThreshold * GetPersonalityThresholdScale()));
		if (enemyCount >= effectiveArtyThreshold)
			RequestArtillerySupport(reportPos, enemyCount, worldTime);
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
			int effectiveReinfThreshold = Math.Max(1, Math.Round(m_iReinforcementThreshold * GetPersonalityThresholdScale()));
			if (m_GroupUtil.perc.m_aTargets.Count() >= effectiveReinfThreshold)
				RequestReinforcement(worldTime);
		}
	}

	protected void CheckResetFlags(float worldTime)
	{
		if (!m_GroupUtil)
			return;

		if (m_GroupUtil.GetGroupStatus() == DCOG_EGroupStatus.IDLE)
		{
			m_bReinfRequested = false;
			return;
		}

		if (m_bReinfRequested && (worldTime - m_fLastReinfRequestAt) > m_fReinfRequestTimeout)
			m_bReinfRequested = false;
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer())
			return;

		if (!m_GroupUtil)
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

		if (!AICommander_ManagerComponent.GetInstance())
			return;
		
		m_MyEntity = owner;
		m_Group     = SCR_AIGroup.Cast(owner);
		m_GroupUtil = DCO_GroupUtilityComponent.Cast(owner.FindComponent(DCO_GroupUtilityComponent));

		if (m_GroupUtil)
			SetEventMask(owner, EntityEvent.FRAME);
	}
	
	void InitializeContactReport()
	{
		SetEventMask(m_MyEntity, EntityEvent.FRAME);	
	}
}