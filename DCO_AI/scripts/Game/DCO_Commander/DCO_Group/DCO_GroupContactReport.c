[ComponentEditorProps(category: "GameScripted/Group", description: "Detects enemies and reports contact to Commander")]
class DCO_GroupContactReporterComponentClass : ScriptComponentClass {}

class DCO_GroupContactReporterComponent : ScriptComponent
{
	[Attribute("10.0", UIWidgets.EditBox, "Interval scan enemy (Second)", category: "Contact")]
	protected float m_fScanInterval;

	[Attribute("3", UIWidgets.EditBox, "Minimum enemy detected before report", category: "Contact")]
	protected int m_iMinEnemyToReport;

	[Attribute("5", UIWidgets.EditBox, "Minimun Unit Count for reporting", category: "Contact")]
	protected int m_iReinforcementThreshold;

	
	protected DCO_GroupUtilityComponent  m_GroupUtil;
	protected ref SCR_AIGroupPerception	 m_GroupPerc;
	protected SCR_AIGroup                m_Group;
	protected float                      m_fScanTimer        = 0.0;
	protected bool                       m_bReinfRequested   = false;

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

		/*Print(string.Format("[DCO_Reporter] %1 requesting reinforcement (%2 units left)",
			GetOwner().GetName(), m_GroupUtil.GetUnitCount()));*/
	}

	protected void ScanForEnemies(float worldTime)
	{
		if (!m_Group)
			return;

		Faction fc = m_Group.GetFaction();
		if (!fc)
			return;
		
		float m_iEnemyCount = 0;

		FactionKey myFk = fc.GetFactionKey();
		vector myPos    = GetOwner().GetOrigin();
		if (m_GroupUtil.perc.m_aTargets)
			m_iEnemyCount = m_GroupUtil.perc.m_aTargets.Count();
		
		vector reportPos = GetCentroidFromEntities(m_GroupUtil.perc.m_aTargetEntities);

		if (m_iEnemyCount < m_iMinEnemyToReport)
			return;
		
		ReportContact(reportPos, m_iEnemyCount, worldTime);
	}
	
	vector GetCentroidFromEntities(array<IEntity> entities)
	{
	    if (entities.IsEmpty())
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
		
		//Print(m_GroupUtil.perc.m_aTargets.Count().ToString() + " < KNOWN ENEMY");
		
		if (m_GroupUtil.perc.m_aTargets)
		{
			if (m_GroupUtil.perc.m_aTargets.Count() >= m_iReinforcementThreshold)
				RequestReinforcement(worldTime);		
		}
	}

	protected CMD_ThreatResponseComponent GetCommanderThreatComponent()
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return null;

		if (!m_GroupUtil)
			return null;

		return m_GroupUtil.GetMyCommander().GetThreatResponseComponent();
	}

	protected void CheckResetReinfFlag()
	{
		if (!m_GroupUtil)
			return;

		if (m_GroupUtil.GetGroupStatus() == DCOG_EGroupStatus.IDLE)
			m_bReinfRequested = false;
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer())
			return;

		m_fScanTimer += timeSlice;
		if (m_fScanTimer > m_fScanInterval && m_GroupUtil.perc)
		{
			m_fScanTimer = 0.0;
			float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
	
			ScanForEnemies(worldTime);
			CheckReinforcementNeed(worldTime);
			CheckResetReinfFlag();
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
		m_Group     = SCR_AIGroup.Cast(owner);
		m_GroupUtil = DCO_GroupUtilityComponent.Cast(owner.FindComponent(DCO_GroupUtilityComponent));
	}
}