class DCO_ContactPauseBehavior : SCR_AIWaitBehavior
{
	protected float m_fDCOStart_ms;
	protected float m_fDCOUntil_ms;

	void DCO_ContactPauseBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, float duration_ms, float priorityLevel)
	{
		SetPriority(59);
		m_fPriorityLevel.m_Value = priorityLevel;
		m_fDCOStart_ms = GetGame().GetWorld().GetWorldTime();
		m_fDCOUntil_ms = m_fDCOStart_ms + duration_ms;
	}

	override int GetCause()
	{
		return SCR_EAIBehaviorCause.DANGER_LOW;
	}

	override float CustomEvaluate()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		bool target = m_Utility && m_Utility.m_CombatComponent && m_Utility.m_CombatComponent.GetCurrentTarget();
		if (now < m_fDCOUntil_ms && !target)
			return 59;
		string why = "timeout";
		if (target)
			why = "target";
		DCO_Perf.Count("unk:pause_end_" + why);
		if (m_Utility && m_Utility.m_OwnerEntity && DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("unknown_pause_end unit=%1 why=%2 held_s=%3", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), why, ((now - m_fDCOStart_ms) / 1000.0).ToString(-1, 1)));
		Complete();
		return 0;
	}
}
