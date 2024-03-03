modded class SCR_AIDangerReaction : SCR_AIReactionBase
{
	EAIDangerEventType m_eType;
	
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent) {}
	
	bool IsDangerChanceByDistance(float distanceToDanger, out float dangerReactionChance)
	{
		dangerReactionChance = 90;
		
		if (Math.RandomFloat(0,100) < dangerReactionChance)
			return true;
		
		return false;
	}

	bool IsMoveFromDangerChanceBySetting(SCR_AIUtilityComponent utility)
	{
		DCO_AIInfoComponent aiInfoComponent = utility.m_DCO_AIInfoComponent;
		
		if (aiInfoComponent)
		{
			float threatSuppressionIsDanger = 0.8;
			
			float threatSuppression = utility.m_ThreatSystem.GetThreatSuppression();
			
			if (utility.m_CombatComponent.GetCurrentTarget() == null)
			{				
				threatSuppressionIsDanger = 1;				
			}
			
			if (Math.RandomInt(0,100) < 85)
				return true;
		}
		
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
			//TODO: change combat type from SILENT
		}

		threatSystem.ThreatBulletImpact(dangerEvent.GetCount());
		
		return true;
	}	
};