[BaseContainerProps()]
modded class SCR_AIDangerReaction_WeaponFired : SCR_AIDangerReaction
{
	protected static const float PROJECTILE_FLYBY_RADIUS = 13;
	protected static const float PROJECTILE_FLYBY_RADIUS_SQ = PROJECTILE_FLYBY_RADIUS * PROJECTILE_FLYBY_RADIUS;
	protected static const float AI_WEAPONFIRED_REACTION_DISTANCE = 500;
	
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent)
	{
		IEntity shooter = dangerEvent.GetObject();
		
		if (!shooter)
			return false;
		
		// Get root entity of shooter, in case it is turret in vehicle hierarchy
		IEntity shooterRoot = shooter.GetRootParent();
		
		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
		if (!agent || !agent.IsEnemy(shooterRoot))
			return false;
		
		vector min, max;
		shooter.GetBounds(min, max);
		vector lookPosition = shooter.GetOrigin() + (min + max) * 0.5;
		
		vector myOrigin = utility.m_OwnerEntity.GetOrigin();
		float distance = vector.Distance(myOrigin, shooter.GetOrigin());
		
		bool flyby = IsFlyby(myOrigin, distance, shooter);
		
		if (flyby)
			threatSystem.ThreatProjectileFlyby(dangerEvent.GetCount());
		else
			threatSystem.ThreatShotFired(distance, dangerEvent.GetCount());

		// Look at shooting position. Even though we add an observe behavior, we can't guarantee that
		// some other behavior doesn't override observe behavior, in which case we might want to look at shooter in parallel.
		utility.m_LookAction.LookAt(lookPosition, utility.m_LookAction.PRIO_DANGER_EVENT, 5.0);
		
		// Notify our group
		// ! Only if we are a leader
		AIGroup myGroup = AIGroup.Cast(utility.GetOwner().GetParentGroup());
		NotifyGroup(myGroup, shooterRoot, lookPosition);
		
		// Ignore if we have selected a target
		// Ignore if target is too far
		if (utility.m_CombatComponent.GetCurrentTarget() != null ||
			distance > AI_WEAPONFIRED_REACTION_DISTANCE)
			return false;
		
		// Check if we must dismount the turret
		vector turretDismountCheckPosition = lookPosition;
		bool mustDismountTurret = utility.m_CombatComponent.DismountTurretCondition(turretDismountCheckPosition, true);
		if (mustDismountTurret)
		{
			utility.m_CombatComponent.TryAddDismountTurretActions(turretDismountCheckPosition);
		}
		
		// Stare at gunshot origin
		bool addObserveBehavior = false;
		SCR_AIMoveAndInvestigateBehavior investigateBehavior = SCR_AIMoveAndInvestigateBehavior.Cast(utility.FindActionOfType(SCR_AIMoveAndInvestigateBehavior));
		SCR_AIObserveUnknownFireBehavior oldObserveBehavior = SCR_AIObserveUnknownFireBehavior.Cast(utility.FindActionOfType(SCR_AIObserveUnknownFireBehavior));
		if (investigateBehavior && investigateBehavior.GetActionState() == EAIActionState.RUNNING)
		{
			if (SCR_AIObserveUnknownFireBehavior.IsNewPositionMoreRelevant(myOrigin, investigateBehavior.m_vPosition.m_Value, lookPosition))
				addObserveBehavior = true;
		}
		else if (oldObserveBehavior)
		{
			if (SCR_AIObserveUnknownFireBehavior.IsNewPositionMoreRelevant(myOrigin, oldObserveBehavior.m_vPosition.m_Value, lookPosition))
				addObserveBehavior = true;
		}
		else if (!oldObserveBehavior)
			addObserveBehavior = true;
			
		if (addObserveBehavior)
		{
			// !!! It's important that priority of this is higher than priority of move and investigate,
			// !!! So first we look at gunshot origin, then investigate it
			SCR_AIObserveUnknownFireBehavior observeBehavior = new SCR_AIObserveUnknownFireBehavior(utility, null,	posWorld: lookPosition, useMovement: flyby);
			utility.AddAction(observeBehavior);
		}
		else if (oldObserveBehavior && flyby)
		{
			// Notify the existing observe behavior, make it execute movement from now on.
			// Otherwise if first behavior was created without movement, and then a bullet flies by,
			// the AI does not move.
			oldObserveBehavior.SetUseMovement(true);
		}
		
		return true;
	}
	
	override bool IsFlyby(vector myPos, float distanceToShooter, IEntity shooter)
	{
		// !!! Important for turrets - WeaponMgr is on turret entity, not vehicle root
		BaseWeaponManagerComponent weaponMgr = BaseWeaponManagerComponent.Cast(shooter.FindComponent(BaseWeaponManagerComponent));
		if (!weaponMgr)
			return false;
		
		vector muzzleTransform[4];
		weaponMgr.GetCurrentMuzzleTransform(muzzleTransform); // todo Ideally this should come from danger event, together with muzzle dir.
		vector muzzlePos = muzzleTransform[3];
		bool flyby = true;
		if ( distanceToShooter > PROJECTILE_FLYBY_RADIUS &&
			Math3D.IntersectionPointCylinder(myPos, muzzleTransform[3], muzzleTransform[2], PROJECTILE_FLYBY_RADIUS) )
		{
			// Within cylinder, but far enough, react as if projectile flew by
			return true;
		}
		
		return true;
	}
};
