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
				
				threatSystem.ThreatBulletImpact(dangerEvent.GetCount());
				
				float threatSuppression = utility.m_ThreatSystem.GetThreatSuppression();
				
				float distanceToDanger = vector.Distance(utility.GetOrigin(), dangerEvent.GetPosition());
				
				{					
					if (distanceToDanger < 5)

					{
						SCR_AIMoveFromDangerBehavior aiMoveFromDangerBehavior = SCR_AIMoveFromDangerBehavior.Cast(utility.FindActionOfType(SCR_AIMoveFromDangerBehavior));
						
						if (aiMoveFromDangerBehavior)
							return true;
						
						utility.AddAction(new SCR_AIMoveFromDangerBehavior(utility, null, shooterPos, shooter));

					}
				}
				
				return true;
				
				float distanceToShooter = vector.Distance(shooterPos, utility.GetOrigin());
				
					bool isNullTarget = utility.m_CombatComponent.GetCurrentTarget() == null;

					if (isNullTarget && distanceToDanger < 5 || distanceToDanger < 3)

					{
						EUnitAIState aiState = aiInfoComponent.GetAIState();
						
						if (isNullTarget || stance == ECharacterStance.PRONE)
						{							
							if (isNullTarget || distanceToDanger < 1)
							{
								SCR_AIMoveFromDangerBehavior aiMoveFromDangerBehavior = SCR_AIMoveFromDangerBehavior.Cast(utility.FindActionOfType(SCR_AIMoveFromDangerBehavior));
								
								if (aiMoveFromDangerBehavior)
									return true;
								
								utility.AddAction(new SCR_AIMoveFromDangerBehavior(utility, null, shooterPos, shooter));
								
							}
						}
					}

					else
					{						
						SCR_AIMoveFromDangerBehavior aiMoveFromDangerBehavior = SCR_AIMoveFromDangerBehavior.Cast(utility.FindActionOfType(SCR_AIMoveFromDangerBehavior));
						
						if (aiMoveFromDangerBehavior)
							return true;
						
						return true;
					}
				}
			
		}
		
		return true;
	}
};