modded class SCR_AIHealWaitBehavior : SCR_AIBehaviorBase
{
	protected float m_fDCOStart_ms = -1;
	protected float m_fDCONextCheck_ms;

	override float CustomEvaluate()
	{
		EAIActionState st = GetActionState();
		if (st == EAIActionState.COMPLETED || st == EAIActionState.FAILED)
			return 0;
		if (!m_Utility || !m_Utility.m_OwnerEntity)
			return GetPriority();

		float now = GetGame().GetWorld().GetWorldTime();
		if (m_fDCOStart_ms < 0)
			m_fDCOStart_ms = now;
		if (now < m_fDCONextCheck_ms)
			return GetPriority();
		m_fDCONextCheck_ms = now + 2000.0;

		IEntity me = m_Utility.m_OwnerEntity;
		if (!SCR_AIDamageHandling.IsCharacterWounded(me))
		{
			DCO_End("not_wounded", true, now);
			return 0;
		}

		IEntity medic = m_HealProvider.m_Value;
		if (!medic || !SCR_AIDamageHandling.IsAlive(medic))
		{
			DCO_End("no_medic", false, now);
			return 0;
		}

		float waited = now - m_fDCOStart_ms;
		float medicDist = vector.Distance(medic.GetOrigin(), me.GetOrigin());
		if (waited > 90000.0 && medicDist > 5.0)
		{
			DCO_End("timeout", false, now);
			return 0;
		}

		return GetPriority();
	}

	protected void DCO_End(string reason, bool ok, float now)
	{
		DCO_Perf.Count("heal_wait_end:" + reason);
		IEntity me = m_Utility.m_OwnerEntity;
		float medicDist = -1;
		if (m_HealProvider.m_Value && me)
			medicDist = vector.Distance(m_HealProvider.m_Value.GetOrigin(), me.GetOrigin());
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("heal_wait_end who=%1 reason=%2 waited_s=%3 medic=%4 medic_dist=%5",
			DCO_Log.IdUnit(me), reason, Math.Round((now - m_fDCOStart_ms) / 1000.0), DCO_Log.IdUnit(m_HealProvider.m_Value), Math.Round(medicDist)));
		if (ok)
			Complete();
		else
			Fail();
	}
};
