[BaseContainerProps()]
modded class SCR_AIDangerReaction_ProjectileHit : SCR_AIDangerReaction
{
	protected static const float BULLET_IMPACT_DISTANCE_SQ_MAX = 10*10;
	
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent)
	{
		float distanceSq = vector.DistanceSq(utility.GetOrigin(), dangerEvent.GetPosition());
		
		if (distanceSq < BULLET_IMPACT_DISTANCE_SQ_MAX)
		{
			if (utility.m_OwnerEntity == dangerEvent.GetVictim())
				return false;
		}

		if (distanceSq > BULLET_IMPACT_DISTANCE_SQ_MAX)
			return false;
		
		IEntity shooter = dangerEvent.GetObject();
		
		if (shooter)
		{
			SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
			
			if (agent && agent.IsEnemy(shooter))
			{				
				vector shooterPos = shooter.GetOrigin();
				
				SCR_AIInfoComponent aiInfoComponent = utility.m_AIInfo;
				
				ECharacterStance stance = aiInfoComponent.GetStance();

				float threatSuppression = utility.m_ThreatSystem.GetThreatSuppression();
				
				float distanceToDanger = vector.Distance(utility.GetOrigin(), dangerEvent.GetPosition());
				
				if (threatSuppression > 0.8)	
					utility.AddAction(new SCR_AIMoveFromUnknownFire(utility, null, shooterPos, shooter));
				
				float distanceToShooter = vector.Distance(shooterPos, utility.GetOrigin());
				
				bool isNullTarget = utility.m_CombatComponent.GetCurrentTarget() == null;

				if (isNullTarget || distanceToDanger < 10 || distanceToDanger < 5)
					utility.AddAction(new SCR_AIMoveFromUnknownFire(utility, null, shooterPos, shooter));
			}
		}
		
		return true;
	}
};