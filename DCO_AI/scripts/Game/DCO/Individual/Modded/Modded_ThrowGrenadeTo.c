modded class SCR_AIThrowGrenadeToBehavior : SCR_AIBehaviorBase
{

	override float CustomEvaluate()
	{
		if ((GetGame().GetWorld().GetWorldTime() - m_fStartTime) > 3000.0)
			return 0;

		return super.CustomEvaluate();
	}
};