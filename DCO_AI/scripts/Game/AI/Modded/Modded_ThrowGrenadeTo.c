modded class SCR_AIThrowGrenadeToBehavior : SCR_AIBehaviorBase
{
	protected static const float THROW_TIMEOUT_MS = 8000.0;
	
	override float CustomEvaluate()
	{
		if ((GetGame().GetWorld().GetWorldTime() - m_fStartTime) > THROW_TIMEOUT_MS)
			return 0;
		
		return super.CustomEvaluate();
	}
};