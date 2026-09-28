modded class SCR_AIMedicHealBehavior : SCR_AIBehaviorBase
{
	protected const float MAX_THREAT_THRESHOLD = 0.05;

	override void OnActionCompleted()
	{
		super.OnActionCompleted();
		DCO_MedicDispatcher.OnMedicFinished(m_Utility.m_OwnerEntity, m_EntityToHeal.m_Value, true);
	}

	override void OnActionFailed()
	{
		super.OnActionFailed();
		DCO_MedicDispatcher.OnMedicFinished(m_Utility.m_OwnerEntity, m_EntityToHeal.m_Value, false);
	}
};
