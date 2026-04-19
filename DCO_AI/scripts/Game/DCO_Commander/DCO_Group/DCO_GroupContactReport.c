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

	[Attribute("120.0", UIWidgets.EditBox, "Cooldown antara artillery requests dari group ini (detik)", category: "Artillery")]
	protected float m_fArtilleryRequestCooldown;

	//--------------------------------------------------------------------
	protected DCO_GroupUtilityComponent  m_GroupUtil;
	protected ref SCR_AIGroupPerception  m_GroupPerc;
	protected SCR_AIGroup                m_Group;
	protected float                      m_fScanTimer          = 0.0;
	protected bool                       m_bReinfRequested     = false;
	protected float                      m_fLastArtilleryReqAt = -999.0;

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

	protected void RequestReinforcement(float worldTime)
	{
		if (m_bReinfRequested)
			return;

		if (!m_GroupUtil)
			return;

		m_bReinfRequested = true;
		m_GroupUtil.GetThreatResponseComponent().ReceiveReinforcementRequest(m_GroupUtil, worldTime);
	}

	//--------------------------------------------------------------------
	// Artillery request — group sends contact pos to ThreatResponse.
	// ThreatResponse decides ammo type + validates score before forwarding.
	//--------------------------------------------------------------------
	protected void RequestArtillerySupport(vector contactPos, float worldTime)
	{
		if (!m_GroupUtil)
			return;

		if (worldTime - m_fLastArtilleryReqAt < m_fArtilleryRequestCooldown)
			return;

		CMD_ThreatResponseComponent threatComp = m_GroupUtil.GetThreatResponseComponent();
		if (!threatComp)
			return;

		m_fLastArtilleryReqAt = worldTime;

		Print(string.Format("[DCO_Reporter] %1 requested artillery @ %2",
			GetOwner().GetName(), contactPos.ToString()));
	}

	//--------------------------------------------------------------------
	protected void ScanForEnemies(float worldTime)
	{
		if (!m_Group)
			return;

		Faction fc = m_Group.GetFaction();
		if (!fc)
			return;

		float enemyCount = 0;

		if (m_GroupUtil.perc && m_GroupUtil.perc.m_aTargets)
			enemyCount = m_GroupUtil.perc.m_aTargets.Count();

		if (enemyCount < m_iMinEnemyToReport)
			return;

		vector reportPos = GetCentroidFromEntities(m_GroupUtil.perc.m_aTargetEntities);

		ReportContact(reportPos, enemyCount, worldTime);

		// Request artillery when enemy count is high enough
		if (enemyCount >= m_iArtilleryEnemyThreshold)
			RequestArtillerySupport(reportPos, worldTime);
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
			if (m_GroupUtil.perc.m_aTargets.Count() >= m_iReinforcementThreshold)
				RequestReinforcement(worldTime);
		}
	}

	protected void CheckResetFlags()
	{
		if (!m_GroupUtil)
			return;

		if (m_GroupUtil.GetGroupStatus() == DCOG_EGroupStatus.IDLE)
			m_bReinfRequested = false;
	}

	//--------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer())
			return;

		m_fScanTimer += timeSlice;
		if (m_fScanTimer <= m_fScanInterval || !m_GroupUtil.perc)
			return;

		m_fScanTimer = 0.0;
		float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;

		ScanForEnemies(worldTime);
		CheckReinforcementNeed(worldTime);
		CheckResetFlags();
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

		m_Group     = SCR_AIGroup.Cast(owner);
		m_GroupUtil = DCO_GroupUtilityComponent.Cast(owner.FindComponent(DCO_GroupUtilityComponent));

		SetEventMask(owner, EntityEvent.FRAME);
	}
}