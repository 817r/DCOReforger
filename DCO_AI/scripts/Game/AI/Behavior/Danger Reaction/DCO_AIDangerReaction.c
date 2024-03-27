modded class SCR_AIDangerReaction_ProjectileHit : SCR_AIDangerReaction
{
	protected static const float BULLET_IMPACT_DISTANCE_MAX_SQ = 10*10;
	
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
		
		if (utility.m_CombatComponent.GetCurrentTarget() == null && distanceToShooter > SCR_AICombatComponent.LONG_RANGE_FIRE_DISTANCE && shooter)
		{
			utility.AddAction(new SCR_AIMoveFromUnknownFire(utility, null, shooterPos, shooter));
		}

		threatSystem.ThreatBulletImpact(dangerEvent.GetCount());
		
		if (threatSystem.GetThreatSuppression() > 0.4)
			utility.AddAction(new SCR_AIMoveFromUnknownFire(utility, null, shooterPos, shooter));
		
		return true;
	}	
};