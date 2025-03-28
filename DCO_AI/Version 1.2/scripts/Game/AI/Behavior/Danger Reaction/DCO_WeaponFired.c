[BaseContainerProps()]
modded class SCR_AIDangerReaction_WeaponFired : SCR_AIDangerReaction
{
	protected static const float PROJECTILE_FLYBY_RADIUS = 13;
	protected static const float PROJECTILE_FLYBY_RADIUS_SQ = PROJECTILE_FLYBY_RADIUS * PROJECTILE_FLYBY_RADIUS;
	protected static const float AUDIBLE_DISTANCE_NORMAL = 500.0; // Audible distance for normal gunshots
	protected static const float AUDIBLE_DISTANCE_SUPPRESSED = 100.0; // Audible distance for suppressed gunshots
	
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{
		AIDangerEventWeaponFire eventWeaponFire = AIDangerEventWeaponFire.Cast(dangerEvent);		
		IEntity shooter = eventWeaponFire.GetObject();
		
		if (!shooter || !eventWeaponFire)
			return false;
		
		IEntity instigatorEntity = FindInstigatorEntity(shooter);
		if (!instigatorEntity)
			return false;
		
		// Check faction relations, ignore if not enemy or there is no faction
		Faction instigatorFaction = SCR_ChimeraAIAgent.GetFaction(instigatorEntity);
		
		if (!instigatorFaction)
			return false;
		
		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
		
		bool myFactionIsMilitary = utility.IsMilitary();
		if (myFactionIsMilitary && !agent.IsEnemy(instigatorFaction))
			return false;
		
		// Get root entity of shooter, in case it is turret in vehicle hierarchy
		vector shotPos = eventWeaponFire.GetPosition();
		vector shotDir = eventWeaponFire.GetDirection();
		bool isShotSuppressed = eventWeaponFire.IsSuppressed();
		
		vector myOrigin = utility.m_OwnerEntity.GetOrigin();
		float distance = vector.Distance(myOrigin, shotPos);
		
		// Is it a flyby?
		bool isFlyby = IsFlyby(myOrigin, shotPos, shotDir, distance);
		
		// Is it audible?
		float maxAudibleDistance;
		if (isShotSuppressed)
			maxAudibleDistance = AUDIBLE_DISTANCE_SUPPRESSED;
		else
			maxAudibleDistance = AUDIBLE_DISTANCE_NORMAL;
		bool isAudible = distance < maxAudibleDistance;
		
		if (isFlyby)
			threatSystem.ThreatProjectileFlyby(dangerEventCount);
		else if (isAudible)
			threatSystem.ThreatShotFired(distance, dangerEventCount);

		// Look at shooting position. Even though we add an observe behavior, we can't guarantee that
		// some other behavior doesn't override observe behavior, in which case we might want to look at shooter in parallel.
		if (isFlyby || isAudible)
		utility.m_LookAction.LookAt(shotPos, utility.m_LookAction.PRIO_DANGER_EVENT, 3.0);
		
		// Notify our group
		// ! Only if we are a leader
		if (isFlyby || isAudible)
		{
			AIGroup myGroup = utility.GetOwner().GetParentGroup();
			if (myGroup && myGroup.GetLeaderAgent() == agent)
			{
				bool endangeringForGroup = isFlyby || distance < PROJECTILE_FLYBY_RADIUS;
				NotifyGroup(myGroup, shooter, instigatorEntity, instigatorFaction, shotPos, endangeringForGroup);
			}
		}
		
		// Ignore if we are a driver inside vehicle
		if (utility.m_AIInfo.HasUnitState(EUnitState.PILOT))
			return false;
			
		// Ignore if we have selected a target
		// Ignore if target is too far
		if (utility.m_CombatComponent.GetCurrentTarget() != null)
			return false;
		
		// Check if we must dismount the turret
		vector turretDismountCheckPosition = shotPos;
		bool mustDismountTurret = utility.m_CombatComponent.DismountTurretCondition(turretDismountCheckPosition, true);
		if (mustDismountTurret)
		{
			utility.m_CombatComponent.TryAddDismountTurretActions(turretDismountCheckPosition);
		}
		
		// Stare at gunshot origin
		if (isAudible || isFlyby)
		{
			bool addObserveBehavior = false;
			SCR_AIMoveAndInvestigateBehavior investigateBehavior = SCR_AIMoveAndInvestigateBehavior.Cast(utility.FindActionOfType(SCR_AIMoveAndInvestigateBehavior));
			SCR_AIObserveUnknownFireBehavior oldObserveBehavior = SCR_AIObserveUnknownFireBehavior.Cast(utility.FindActionOfType(SCR_AIObserveUnknownFireBehavior));
			SCR_AISuppressGroupClusterBehavior suppressGroupClusterBehavior = SCR_AISuppressGroupClusterBehavior.Cast(utility.FindActionOfType(SCR_AISuppressGroupClusterBehavior));
			if (investigateBehavior && investigateBehavior.GetActionState() == EAIActionState.RUNNING)
			{
				if (SCR_AIObserveUnknownFireBehavior.IsNewPositionMoreRelevant(myOrigin, investigateBehavior.m_vPosition.m_Value, shotPos))
					addObserveBehavior = true;
			}
			else if (suppressGroupClusterBehavior && suppressGroupClusterBehavior.m_SuppressionVolume.m_Value)
			{
				if (SCR_AIObserveUnknownFireBehavior.IsNewPositionMoreRelevant(myOrigin, suppressGroupClusterBehavior.m_SuppressionVolume.m_Value.GetCenterPosition(), shotPos))
					addObserveBehavior = true;
			}
			else if (oldObserveBehavior)
			{
				if (SCR_AIObserveUnknownFireBehavior.IsNewPositionMoreRelevant(myOrigin, oldObserveBehavior.m_vPosition.m_Value, shotPos))
					addObserveBehavior = true;
			}
			else if (!oldObserveBehavior)
				addObserveBehavior = true;
				
			if (addObserveBehavior)
			{
				// !!! It's important that priority of this is higher than priority of move and investigate,
				// !!! So first we look at gunshot origin, then investigate it
				bool useMovement = isFlyby && !utility.m_AIInfo.HasUnitState(EUnitState.IN_TURRET) && !utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE);
				SCR_AIObserveUnknownFireBehavior observeBehavior = new SCR_AIObserveUnknownFireBehavior(utility, null,	posWorld: shotPos, useMovement: useMovement);
				utility.AddAction(observeBehavior);
			}
			else if (oldObserveBehavior && isFlyby)
			{
				// Notify the existing observe behavior, make it execute movement from now on.
				// Otherwise if first behavior was created without movement, and then a bullet flies by,
				// the AI does not move.
				if (!utility.m_AIInfo.HasUnitState(EUnitState.IN_TURRET) && !utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
					oldObserveBehavior.SetUseMovement(true);
			}
		}
		
		return true;
	}
};
