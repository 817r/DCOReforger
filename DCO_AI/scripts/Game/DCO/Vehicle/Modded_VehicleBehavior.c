modded class SCR_AIVehicleBehavior : SCR_AIBehaviorBase
{
	override float CustomEvaluate()
	{
		if (m_Utility && m_Utility.m_DCOConfig && m_Utility.m_DCOConfig.IsHoldPosition())
			return 0;

		return super.CustomEvaluate();
	}
}
