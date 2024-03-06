[BaseContainerProps()]
modded class SCR_AIDangerReaction_WeaponFired : SCR_AIDangerReaction
{
	//------------------------------------------------------------------------------------------------
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent)
	{
		IEntity shooter = dangerEvent.GetObject();
		
		if (shooter)
		{
			shooter = shooter.GetRootParent();
			
			SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
			
			if (agent && agent.IsEnemy(shooter))
			{
				vector min, max;
				shooter.GetBounds(min, max);
				
				AIGroup parentGroup = agent.GetParentGroup();
				
				vector lookPosition = shooter.GetOrigin() + (min + max) * 0.5;
				
				float distance = vector.Distance(lookPosition, utility.GetOrigin());
				
				threatSystem.ThreatShotFired(distance, dangerEvent.GetCount());
				
				int weaponFiredReactionDistance = AI_WEAPONFIRED_REACTION_DISTANCE;
				
				if (parentGroup)
				{
					DCO_AIInfoGroupComponent aiGroupInfoComponent = DCO_AIInfoGroupComponent.Cast(parentGroup.FindComponent(DCO_AIInfoGroupComponent));
					
					if (aiGroupInfoComponent)
					{
						AIAgent leaderAgent = parentGroup.GetLeaderAgent();
						
						IEntity leaderEntity = parentGroup.GetLeaderEntity();
						
						weaponFiredReactionDistance = aiGroupInfoComponent.GetWeaponFiredReactionDistance();
						
						float distanceToShooter = vector.Distance(shooter.GetOrigin(), leaderEntity.GetOrigin());

						if (agent == leaderAgent)
							NotifyGroup(parentGroup, shooter, lookPosition);
						
					}
				}
				
				if (distance < weaponFiredReactionDistance)
				{
					utility.m_LookAction.LookAt(lookPosition, utility.m_LookAction.PRIO_DANGER_EVENT, 3.0);
					
					if (utility.m_CombatComponent.GetCurrentTarget() == null)
					{
						bool addObserveBehavior = false;
						
						vector origin = utility.m_OwnerEntity.GetOrigin();
						
						vector turretDismountCheckPosition = lookPosition;
						
						bool mustDismountTurret = utility.m_CombatComponent.DismountTurretCondition(turretDismountCheckPosition, true);
						
						if (mustDismountTurret)
						{							
							utility.m_CombatComponent.TryAddDismountTurretActions(turretDismountCheckPosition);
						}

						SCR_AIObserveUnknownFireBehavior oldObserveBehavior = SCR_AIObserveUnknownFireBehavior.Cast(utility.FindActionOfType(SCR_AIObserveUnknownFireBehavior));
						SCR_AIMoveAndInvestigateBehavior investigateBehavior = SCR_AIMoveAndInvestigateBehavior.Cast(utility.FindActionOfType(SCR_AIMoveAndInvestigateBehavior));
						
						if (investigateBehavior)
						{
							if (SCR_AIObserveUnknownFireBehavior.IsNewPositionMoreRelevant(origin, investigateBehavior.m_vPosition.m_Value, lookPosition))
								addObserveBehavior = true;
						}
						else if (oldObserveBehavior)
						{
							if (SCR_AIObserveUnknownFireBehavior.IsNewPositionMoreRelevant(origin, oldObserveBehavior.m_vPosition.m_Value, lookPosition))
								addObserveBehavior = true;
						}
						else if (!oldObserveBehavior)
							addObserveBehavior = true;
						
						if (addObserveBehavior)
						{
							SCR_AIObservePositionBehavior observeBehavior = new SCR_AIObserveUnknownFireBehavior(utility, null,	posWorld: lookPosition);
							utility.AddAction(observeBehavior);
						}
						
						float threatTotal = utility.m_ThreatSystem.GetThreatTotal();
						
						EAIThreatState threatState = utility.m_AIInfo.GetThreatState();
						
						if (threatTotal < 0.1000 && threatState == EAIThreatState.SAFE)
						{
							float dangerReactionChance;
							
							vector shooterPos = shooter.GetOrigin();
							
							float distanceToShooter = vector.Distance(shooterPos, utility.GetOrigin());
							
							EWeaponType currentWeaponType = utility.m_CombatComponent.GetCurrentWeaponType();
							
							if (currentWeaponType == EWeaponType.WT_MACHINEGUN)
								return true;
							

							float dangerReactionWeaponFiredDelay = distanceToShooter;
								
							GetGame().GetCallqueue().CallLater(DangerReactionWeaponFiredDelayed, dangerReactionWeaponFiredDelay, false, utility, shooter, shooterPos);
							
						}
					}
				}
			}
			else
			{
				if (agent)
					DangerReactionWeaponFiredFriendly(agent, shooter, utility);
			}
		}
		
		return true;
	}

	void DangerReactionWeaponFiredDelayed(SCR_AIUtilityComponent utility, IEntity shooter, vector shooterPos)
	{
		SCR_AIMoveFromDangerBehavior aiMoveFromDangerBehavior = SCR_AIMoveFromDangerBehavior.Cast(utility.FindActionOfType(SCR_AIMoveFromDangerBehavior));
		
		if (aiMoveFromDangerBehavior)
			return;
		
		utility.AddAction(new SCR_AIMoveFromDangerBehavior(utility, null, shooterPos, shooter));
	}

	bool DangerReactionWeaponFiredFriendly(AIAgent agent, IEntity shooter, SCR_AIUtilityComponent utility)
	{
		if (utility.m_CombatComponent.GetCurrentTarget() == null)
		{
			Vehicle vehicle = Vehicle.Cast(shooter);
			
			if (vehicle)
				return false;
			
			if (utility.HasActionOfType(SCR_AIObserveUnknownFireBehavior))
				return false;
			
			SCR_AICombatComponent aiCombatComponent = SCR_AICombatComponent.Cast(shooter.FindComponent(SCR_AICombatComponent));
			
			if (aiCombatComponent)
			{
				AIAgent shooterAgent = aiCombatComponent.GetAiAgent();
				
				if (shooterAgent)
				{
					BaseTarget currentTarget = aiCombatComponent.GetCurrentTarget();
					
					if (currentTarget)
					{
						IEntity targetEntity = currentTarget.GetTargetEntity();
						
						if (targetEntity)
						{
							IEntity ownerEntity = utility.m_OwnerEntity;
							
							AIGroup parentGroup = agent.GetParentGroup();
							
							int weaponFiredReactionDistance = AI_WEAPONFIRED_REACTION_DISTANCE;
							
							float distanceToShooter = vector.Distance(ownerEntity.GetOrigin(), targetEntity.GetOrigin());
							
							if (parentGroup)
							{
								SCR_AIConfigComponent aiConfigComponent = SCR_AIConfigComponent.Cast(parentGroup.FindComponent(SCR_AIConfigComponent));
								
								weaponFiredReactionDistance = 800;
							}
							
							if (distanceToShooter < weaponFiredReactionDistance)
							{
								vector min, max;
								
								bool addObserveBehavior = false;
								
								targetEntity.GetBounds(min, max);
								
								vector origin = utility.m_OwnerEntity.GetOrigin();
								
								vector targetLookPosition = targetEntity.GetOrigin() + (min + max) * 0.5;
								
								utility.m_LookAction.LookAt(targetLookPosition, utility.m_LookAction.PRIO_DANGER_EVENT, 3.0);
								
								SCR_AIObserveUnknownFireBehavior oldObserveBehavior = SCR_AIObserveUnknownFireBehavior.Cast(utility.FindActionOfType(SCR_AIObserveUnknownFireBehavior));
								SCR_AIMoveAndInvestigateBehavior investigateBehavior = SCR_AIMoveAndInvestigateBehavior.Cast(utility.FindActionOfType(SCR_AIMoveAndInvestigateBehavior));
								
								if (oldObserveBehavior)
								{
									if (SCR_AIObserveUnknownFireBehavior.IsNewPositionMoreRelevant(origin, oldObserveBehavior.m_vPosition.m_Value, targetLookPosition))
										addObserveBehavior = true;
								}
								
								if (addObserveBehavior)
								{
									if (true)
									{
										SCR_AIObservePositionBehavior observeBehavior = new SCR_AIObserveUnknownFireBehavior(utility, null,	posWorld: targetLookPosition);
										utility.AddAction(observeBehavior);
									}
								}							
							}
						}
					}
				}
			}
		}
		
		return true;
	}
};