modded class SCR_AIUtilityComponent
{
	IEntity m_DCOLastAtkEnt;
	float m_fDCOLastAtkAt_ms = -100000;
}

modded class SCR_AIAttackBehavior
{
	override protected void InitWaitTime(SCR_AIUtilityComponent utility)
	{
		super.InitWaitTime(utility);
		if (!utility || !utility.m_OwnerEntity || !m_Target.m_Value || DCO_Stress.T(utility, "rt.fast", 1.0) <= 0)
			return;
		BaseTarget t = m_Target.m_Value;
		IEntity me = utility.m_OwnerEntity;
		float dist = vector.Distance(me.GetOrigin(), t.GetLastSeenPosition());
		float now = GetGame().GetWorld().GetWorldTime();
		float before = m_fWaitTime.m_Value;
		string why = "";
		if (t.GetTargetEntity() && t.GetTargetEntity() == utility.m_DCOLastAtkEnt && now - utility.m_fDCOLastAtkAt_ms < DCO_Stress.T(utility, "rt.reacquire_s", 10.0) * 1000.0)
		{
			m_fWaitTime.m_Value = 0;
			why = "reacquire";
		}
		else if (dist < DCO_Stress.T(utility, "rt.close_m", 30.0) && utility.m_ThreatSystem)
		{
			float threat = utility.m_ThreatSystem.GetThreatMeasure();
			float w = 0;
			if (threat < SCR_AIThreatSystem.ATTACK_DELAYED_THRESHOLD)
				w = DCO_Stress.T(utility, "rt.close_surprise_s", 0.15);
			else if (threat >= SCR_AIThreatSystem.THREATENED_THRESHOLD)
				w = DCO_Stress.T(utility, "rt.close_threat_s", 0.2);
			m_fWaitTime.m_Value = w + (1.4 * dist) / (300.0 + dist);
			why = "close";
		}
		if (why == "")
			return;
		DCO_Perf.Count("rt:wait_" + why);
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("attack_wait unit=%1 tgt=%2 dist=%3 why=%4 vanilla_s=%5 dco_s=%6", DCO_Log.IdUnit(me), DCO_Log.IdUnit(t.GetTargetEntity()), Math.Round(dist), why, before.ToString(-1, 2), m_fWaitTime.m_Value.ToString(-1, 2)));
	}

	override float CustomEvaluate()
	{
		float p = super.CustomEvaluate();
		if (m_bSelected && m_Utility && m_Target.m_Value && m_Target.m_Value.GetTargetEntity())
		{
			m_Utility.m_DCOLastAtkEnt = m_Target.m_Value.GetTargetEntity();
			m_Utility.m_fDCOLastAtkAt_ms = GetGame().GetWorld().GetWorldTime();
		}
		return p;
	}
}
