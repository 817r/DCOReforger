[BaseContainerProps()]
class SCR_AIDangerEvent_Killzone : AIDangerEvent
{
	protected float m_fRadius;
	
	protected EMovementType m_eMovementType = EMovementType.RUN;
	
	//------------------------------------------------------------------------------------------------
	void SetRadius(float radius)
	{
		m_fRadius = radius;
	}
	
	//------------------------------------------------------------------------------------------------
	float GetRadius()
	{
		return m_fRadius;
	}
}

[BaseContainerProps()]
class SCR_AIDangerReaction_Killzone : SCR_AIDangerReaction
{
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{	
		SCR_AIDangerEvent_Killzone unsafeAreaEvent = SCR_AIDangerEvent_Killzone.Cast(dangerEvent);
		if (!unsafeAreaEvent)
			return false;
		
		vector unsafePos = unsafeAreaEvent.GetPosition();
		float unsafeRadius = unsafeAreaEvent.GetRadius() * 1.2;
		float distance = vector.Distance(unsafePos, utility.GetOrigin());
		
		if (distance > unsafeRadius)
			return false;
		
		float distanceToMove = (unsafeRadius - distance) * -1;		
		
		SCR_AIMoveFromUnsafeAreaBehavior moveBehavior = new SCR_AIMoveFromUnsafeAreaBehavior(utility, null, unsafePos, null, distanceToMove);
		utility.AddAction(moveBehavior);		
		
		return true;
	}
};
