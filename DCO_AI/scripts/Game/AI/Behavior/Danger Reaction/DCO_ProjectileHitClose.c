[BaseContainerProps()]
modded class SCR_AIDangerReaction_ProjectileHit : SCR_AIDangerReaction
{
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
				float dangerReactionChance;
				
				vector shooterPos = shooter.GetOrigin();
				
				SCR_AIInfoComponent aiInfoComponent = utility.m_AIInfo;
				
				ECharacterStance stance = aiInfoComponent.GetStance();
				
				threatSystem.ThreatBulletImpact(dangerEvent.GetCount());
				
				float threatSuppressionIsDanger = Math.RandomFloat(0.5,1.0);
				
				float threatSuppression = utility.m_ThreatSystem.GetThreatSuppression();
				
				float distanceToDanger = vector.Distance(utility.GetOrigin(), dangerEvent.GetPosition());
				
				{					
					if (distanceToDanger < 3)

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
						else
						{
							ECharacterStanceChange stanceChange;
							
							IEntity controlledEntity = agent.GetControlledEntity();
							
							CharacterControllerComponent characterControllerComponent = CharacterControllerComponent.Cast(controlledEntity.FindComponent(CharacterControllerComponent));
							
							if (characterControllerComponent)
							{
								if (stance == ECharacterStance.STAND)
								{
									stance = ECharacterStance.CROUCH;
									stanceChange = ECharacterStanceChange.STANCECHANGE_TOCROUCH;
								}
								else if (stance == ECharacterStance.CROUCH)
								{
									stance = ECharacterStance.PRONE;
									stanceChange = ECharacterStanceChange.STANCECHANGE_TOPRONE;
								}
								
								if (stanceChange > 0)
								{
									aiInfoComponent.SetStance(stance);
									characterControllerComponent.SetStanceChange(stanceChange);
								}
							}
						}
					}

					else
					{
						return true;
						
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