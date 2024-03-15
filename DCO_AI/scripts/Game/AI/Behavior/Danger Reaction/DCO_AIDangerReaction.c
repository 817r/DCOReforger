modded class SCR_AIDangerReaction : SCR_AIReactionBase
{
	EAIDangerEventType m_eType;
	
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent) {}
	
<<<<<<< HEAD
	bool IsDangerChanceByDistance(float distanceToDanger, out float dangerReactionChance)
	{
		dangerReactionChance = 90;
		
		if (Math.RandomFloat(0,100) < dangerReactionChance)
			return true;
		
		return false;
	}
=======
};
>>>>>>> Reforger_1.1

modded class SCR_AIDangerReaction_ProjectileHit : SCR_AIDangerReaction
{
	protected static const float BULLET_IMPACT_DISTANCE_MAX_SQ = 15*15;
	
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent)
	{
		float distanceSq = vector.DistanceSq(utility.GetOrigin(), dangerEvent.GetPosition());
		if (distanceSq > BULLET_IMPACT_DISTANCE_MAX_SQ)
			return false;
		
		IEntity shooter = dangerEvent.GetObject();
		
<<<<<<< HEAD
		return false;
	}
	
	
};

modded class SCR_AIDangerReaction_ProjectileHit : SCR_AIDangerReaction
{
	protected static const float BULLET_IMPACT_DISTANCE_MAX_SQ = 10*10;
	
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent)
	{
		float distanceSq = vector.DistanceSq(utility.GetOrigin(), dangerEvent.GetPosition());
		if (distanceSq > BULLET_IMPACT_DISTANCE_MAX_SQ)
			return false;
		
		IEntity shooter = dangerEvent.GetObject();
		
=======
>>>>>>> Reforger_1.1
		if (!shooter)
			return false;
		
		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
		IEntity shooterRoot = shooter.GetRootParent();
		if (!agent || !agent.IsEnemy(shooterRoot))
			return false;
		
		vector shooterPos = shooter.GetOrigin();
		
		float distanceToShooter = vector.Distance(utility.GetOrigin(), shooterPos);
		
<<<<<<< HEAD
		if (utility.m_CombatComponent.GetCurrentTarget() == null && distanceToShooter > SCR_AICombatComponent.LONG_RANGE_FIRE_DISTANCE && shooter)
		{
			utility.AddAction(new SCR_AIMoveFromUnknownFire(utility, null, shooterPos, shooter));
			//TODO: change combat type from SILENT
		}
=======

		utility.AddAction(new SCR_AIMoveFromUnknownFire(utility, null, shooterPos, shooter));

>>>>>>> Reforger_1.1

		threatSystem.ThreatBulletImpact(dangerEvent.GetCount());
		
		return true;
	}	
};