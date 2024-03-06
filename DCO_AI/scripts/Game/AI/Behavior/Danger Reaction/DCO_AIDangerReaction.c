modded class SCR_AIDangerReaction : SCR_AIReactionBase
{
	EAIDangerEventType m_eType;
	
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent) {}
	
};

modded class SCR_AIDangerReaction_ProjectileHit : SCR_AIDangerReaction
{
	protected static const float BULLET_IMPACT_DISTANCE_MAX_SQ = 15*15;
	
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent)
	{
		float distanceSq = vector.DistanceSq(utility.GetOrigin(), dangerEvent.GetPosition());
		if (distanceSq > BULLET_IMPACT_DISTANCE_MAX_SQ)
			return false;
		
		IEntity shooter = dangerEvent.GetObject();
		
		if (!shooter)
			return false;
		
		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
		IEntity shooterRoot = shooter.GetRootParent();
		if (!agent || !agent.IsEnemy(shooterRoot))
			return false;
		
		vector shooterPos = shooter.GetOrigin();
		
		float distanceToShooter = vector.Distance(utility.GetOrigin(), shooterPos);
		

		utility.AddAction(new SCR_AIMoveFromUnknownFire(utility, null, shooterPos, shooter));


		threatSystem.ThreatBulletImpact(dangerEvent.GetCount());
		
		return true;
	}	
};