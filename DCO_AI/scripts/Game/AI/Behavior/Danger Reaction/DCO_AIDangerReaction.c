/*modded class SCR_AIDangerReaction_ProjectileHit : SCR_AIDangerReaction
{
	protected static const float BULLET_IMPACT_DISTANCE_MAX_SQ = 10*10;
	
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent)
	{
		float distanceSq = vector.DistanceSq(utility.GetOrigin(), dangerEvent.GetPosition());
		
		IEntity shooter = dangerEvent.GetObject();
		
		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
		IEntity shooterRoot = shooter.GetRootParent();
		if (!agent || !agent.IsEnemy(shooterRoot))
			return false;
		
		vector shooterPos = shooter.GetOrigin();
		
		float distanceToShooter = vector.Distance(utility.GetOrigin(), shooterPos);
		
		if (utility.m_CombatComponent.GetCurrentTarget() == null && distanceToShooter > SCR_AICombatComponent.LONG_RANGE_FIRE_DISTANCE && shooter)
			utility.AddAction(new SCR_AIMoveFromUnknownFire(utility, null, shooterPos, shooter));
		
		threatSystem.ThreatBulletImpact(dangerEvent.GetCount());
		
		float distanceDanger = vector.Distance(utility.GetOrigin(), dangerEvent.GetPosition());
		
		int bulletFly = dangerEvent.GetCount();
		
		if (shooter)
		{			
			if (agent && agent.IsEnemy(shooter))
			{
				if (distanceDanger < 2 && bulletFly > 5)
				{
					int randomChances = Math.RandomInt(1,10);
					
					if (randomChances > 4)
						utility.AddAction(new SCR_AIMoveFromDangerBehavior(utility, null, shooterPos, shooter));
					
				}
			}
		
		}
		
		return true;
	}	
};*/

[BaseContainerProps()]
modded class SCR_AIDangerReaction_ProjectileHit : SCR_AIDangerReaction
{
	protected static const float BULLET_IMPACT_DISTANCE_SQ_MAX = 5*5;
	protected SCR_AICombatMoveState m_State;
	protected SCR_AICombatComponent m_CombatComp;
	protected SCR_AICombatMoveLogic_Attack m_CombatLogic;
	
	
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent)
	{
		float distanceSq = vector.DistanceSq(utility.GetOrigin(), dangerEvent.GetPosition());
				
		if (distanceSq > BULLET_IMPACT_DISTANCE_SQ_MAX)
			return false;
		
		IEntity shooter = dangerEvent.GetObject();
		
		if (!shooter)
			return false;
		
		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
		IEntity shooterRoot = shooter.GetRootParent();
		vector shooterPos = shooter.GetOrigin();
		float distanceToDanger = vector.Distance(utility.GetOrigin(), dangerEvent.GetPosition());
		bool isNullTarget = utility.m_CombatComponent.GetCurrentTarget() == null;
		
		if (!agent || !agent.IsEnemy(shooterRoot))
			return false;

		float distanceToShooter = vector.Distance(utility.GetOrigin(), shooterPos);
		
		if (shooter)
		{
			if (agent && agent.IsEnemy(shooter))
			{
				if (distanceToDanger < 3)
					utility.AddAction(new SCR_AIMoveFromUnknownFire(utility, null, shooterPos, shooter));
				else if (distanceToDanger < 2 && distanceToShooter < 5)
					utility.AddAction(new SCR_AIMoveFromUnknownFire(utility, null, shooterPos, shooter));
				else if (isNullTarget && distanceToDanger < 2)
					utility.AddAction(new SCR_AIMoveFromUnknownFire(utility, null, shooterPos, shooter));
				
				return true;
			}
		}
		
		return true;
	}
};