modded class SCR_AIMedicHealBehavior : SCR_AIBehaviorBase
{
	protected const float MAX_THREAT_THRESHOLD = 0.05;

	override float CustomEvaluate()
	{
		EAIActionState st = GetActionState();
		if (st != EAIActionState.COMPLETED && st != EAIActionState.FAILED && m_Utility && m_EntityToHeal.m_Value != m_Utility.m_OwnerEntity && DCO_Util.IsBleeding(m_Utility.m_OwnerEntity))
		{
			DCO_Perf.Count("medic:helper_bleeding_abort");
			DCO_Log.Write(DCO_ELogChannel.MEDIC, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("medic_helper_bleeding_abort helper=%1 patient=%2", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), DCO_Log.IdUnit(m_EntityToHeal.m_Value)));
			Fail();
			return 0;
		}
		if (m_Utility && m_EntityToHeal.m_Value && m_EntityToHeal.m_Value != m_Utility.m_OwnerEntity && DCO_Stress.T(m_Utility, "heal.unseen_gate", 1.0) > 0 && DCO_CasualtyUnderFire(m_EntityToHeal.m_Value))
		{
			DCO_Perf.Count("heal:unseen_gate_medic");
			if (DCO_Log.On(DCO_ELogChannel.MEDIC, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.MEDIC, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("heal_defer who=medic medic=%1 patient=%2 reason=unseen_shooter_close",
					DCO_Log.IdUnit(m_Utility.m_OwnerEntity), DCO_Log.IdUnit(m_EntityToHeal.m_Value)));
			return 0;
		}
		return super.CustomEvaluate();
	}

	protected bool DCO_CasualtyUnderFire(IEntity patient)
	{
		AIControlComponent ctrl = AIControlComponent.Cast(patient.FindComponent(AIControlComponent));
		if (!ctrl)
			return false;
		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(ctrl.GetControlAIAgent());
		if (!agent || !agent.m_UtilityComponent || !agent.m_UtilityComponent.m_ThreatSystem)
			return false;
		SCR_AIUtilityComponent pu = agent.m_UtilityComponent;
		if (pu.m_AIInfo && pu.m_AIInfo.GetBleedTimeToUnconscious() <= 8.0)
			return false;
		float at = pu.m_ThreatSystem.DCO_UnseenCloseAt();
		return at >= 0 && GetGame().GetWorld().GetWorldTime() - at < DCO_Stress.T(m_Utility, "heal.unseen_s", 6.0) * 1000.0;
	}

	override void OnActionCompleted()
	{
		super.OnActionCompleted();
		if (!DCO_Util.Teardown("medic_completed"))
			DCO_MedicDispatcher.OnMedicFinished(m_Utility.m_OwnerEntity, m_EntityToHeal.m_Value, true);
	}

	override void OnActionFailed()
	{
		super.OnActionFailed();
		if (!DCO_Util.Teardown("medic_failed"))
			DCO_MedicDispatcher.OnMedicFinished(m_Utility.m_OwnerEntity, m_EntityToHeal.m_Value, false);
	}
};
