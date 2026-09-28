class DCO_AIIdleWait : AITaskScripted
{
	protected static const float RETRY_MS = 20000;

	protected SCR_AIUtilityComponent m_Utility;
	protected float m_fNextTry_ms;

	override void OnInit(AIAgent owner)
	{
		m_Utility = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
	}

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!m_Utility || !m_Utility.m_CombatMoveState || m_Utility.m_CombatMoveState.m_bInCover)
			return ENodeResult.RUNNING;

		float now_ms = GetGame().GetWorld().GetWorldTime();
		if (now_ms < m_fNextTry_ms)
			return ENodeResult.RUNNING;

		SCR_AIGroup group = SCR_AIGroup.Cast(owner.GetParentGroup());
		if (!group || !group.GetGroupUtilityComponent() || !group.GetGroupUtilityComponent().DCO_IsIdle())
			return ENodeResult.RUNNING;

		m_fNextTry_ms = now_ms + RETRY_MS;
		return ENodeResult.SUCCESS;
	}

	static override bool VisibleInPalette() { return true; }
	static override string GetOnHoverDescription() { return "RUNNING sampai grup IDLE dan AI belum di cover, lalu SUCCESS."; }
}

class DCO_FindIndoorPositionIdle : DCO_FindIndoorPosition
{
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (cfg)
			m_fRadius = cfg.GetIdleCoverSearchDist();

		return super.EOnTaskSimulate(owner, dt);
	}
}

class DCO_AICreateIdleCoverQueryProps : SCR_AICreateBasicCoverQueryProps
{
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (cfg)
			m_fRadius = cfg.GetIdleCoverSearchDist();

		return super.EOnTaskSimulate(owner, dt);
	}
}
