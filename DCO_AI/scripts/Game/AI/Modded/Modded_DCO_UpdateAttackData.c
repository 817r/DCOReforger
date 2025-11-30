modded class SCR_AIUpdateTargetAttackData : AITaskScripted
{	
	protected const int FIRE_TREE_INVALID 		= -1;	// No aiming or firing is allowed at all
	protected const int FIRE_TREE_LOOK			= 0;	// Looking at target without firing
	protected const int FIRE_TREE_BURST			= 1;
	protected const int FIRE_TREE_SINGLE		= 2;
	protected const int FIRE_TREE_SUPPRESSIVE	= 3;
	protected const int FIRE_TREE_MELEE			= 4;
	protected const int FIRE_TREE_LOOK_THREATS	= 5;	// Looking at data from threat system
	
	protected const float BURST_FIRE_MAX_DISTANCE = 70.0;

	//-----------------------------------------------------------------------------------------------------
	// Evaluates which fire tree should be used
	override int ResolveFireTree(BaseTarget target, bool visible, bool weaponReady, out float fireRate)
	{
		// Is aiming forbidden by combat move?
		SCR_AIBehaviorBase executedBehavior = SCR_AIBehaviorBase.Cast(m_UtilityComponent.GetExecutedAction());
		if (executedBehavior && executedBehavior.m_bUseCombatMove && !m_UtilityComponent.m_CombatMoveState.m_bAimAtTarget)
			return FIRE_TREE_INVALID;
		
		// Is looking at threats activated?
		if (m_bLookAtThreats)
			return FIRE_TREE_LOOK_THREATS;
		
		// Is weapon not ready?
		if (!weaponReady)
			return FIRE_TREE_LOOK;
		
		BaseWeaponComponent selectedWeaponComp;
		int selectedMuzzleId;
		m_CombatComponent.GetSelectedWeapon(selectedWeaponComp, selectedMuzzleId);
		
		bool directDamage
		float weaponMinDist, weaponMaxDist;
		m_CombatComponent.GetSelectedWeaponProperties(weaponMinDist, weaponMaxDist, directDamage);
		
		if (!selectedWeaponComp)
			return FIRE_TREE_LOOK;
		
		// Hold fire?
		if (m_CombatComponent.GetCombatMode() == EAIGroupCombatMode.HOLD_FIRE)
			return FIRE_TREE_LOOK;
		
		EWeaponType weaponType = selectedWeaponComp.GetWeaponType();
		
		float targetDistance = target.GetDistance();
		
		// Use melee?
		if (targetDistance < MELEE_MAX_DISTANCE &&
			!m_CharacterController.CanFire() &&
			m_CharacterController.GetStance() != ECharacterStance.PRONE)
		{
			return FIRE_TREE_MELEE;
		}
		
		// Friendly in aim?
		if (m_PerceptionComponent.GetFriendlyInLineOfFire())
		{
			return FIRE_TREE_LOOK;
		}
	
		
		// Not melee
		float threat = m_UtilityComponent.m_ThreatSystem.GetThreatMeasure();
		if (targetDistance < weaponMinDist || targetDistance > weaponMaxDist)
		{
			// Outside weapon usage range
			// Look at target
			
			if (weaponType == EWeaponType.WT_MACHINEGUN)
			{
				float maxFireRate = Math.Max(1, Math.Map(targetDistance, 0, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE, 4, 1));
				fireRate = maxFireRate * threat;
				return FIRE_TREE_SUPPRESSIVE;
			}
			return FIRE_TREE_LOOK;
		}

		// Within weapon usage range
		
		if (visible)
		{
			// Visible
			// If machinegun, always use burst at any range
			// For regular weapons, use burst at short range if available, otherwise single
			if (weaponType == EWeaponType.WT_MACHINEGUN)
				return FIRE_TREE_BURST;
			else if (targetDistance < BURST_FIRE_MAX_DISTANCE && m_bWeaponHasBurstOrAuto)
				return FIRE_TREE_BURST;
			else
				return FIRE_TREE_SINGLE;
			
		}
		else
		{
			// Invisible
			// Use suppressive fire or don't fire at all
			
			// If weapon is configured to deal indirect damage, then use it for invisible target
			// Otherwise regular weapons can also be used to fire at hidden target,
			// except for rocket launchers, their ammo is too valuable
			// Also ensure we don't do suppressive fire into a wall in front of us
			
			float lastSeenThreshold;
			if (weaponType == EWeaponType.WT_MACHINEGUN)
				lastSeenThreshold = SCR_AICombatComponent.TARGET_MAX_LAST_SEEN_INDIRECT_ATTACK_MG;
			else
			{
				if (targetDistance < SCR_AICombatComponent.CLOSE_RANGE_COMBAT_DISTANCE)
					lastSeenThreshold = SCR_AICombatComponent.TARGET_MAX_LAST_SEEN_INDIRECT_ATTACK_CLOSE;
				else
					lastSeenThreshold = SCR_AICombatComponent.TARGET_MAX_LAST_SEEN_INDIRECT_ATTACK;
			}
			
			lastSeenThreshold = Math.Max(SCR_AICombatComponent.TARGET_MIN_LAST_SEEN_INDIRECT_ATTACK, lastSeenThreshold * threat);
			
			if ((!directDamage || weaponType != EWeaponType.WT_ROCKETLAUNCHER) &&
				target.GetTimeSinceSeen() < lastSeenThreshold &&
				target.GetTraceFraction() > 0.5)
			{
				float maxFireRate = Math.Max(1, Math.Map(targetDistance, 0, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE, 4, 1));
				fireRate = maxFireRate * threat;
								
				return FIRE_TREE_SUPPRESSIVE;
			}
			else
				return FIRE_TREE_LOOK;
		}
		
		return FIRE_TREE_LOOK;
	}
	
	//-----------------------------------------------------------------------------------------------------
	override void ResolveAimpointTypes(notnull BaseTarget target, out EAimPointType aimpointType0, out EAimPointType aimpointType1)
	{
		IEntity targetEntity = target.GetTargetEntity();
		if (!targetEntity)
		{
			aimpointType0 = -1;
			aimpointType1 = -1;
			return;
		}
		
		EWeaponType weaponType = m_CombatComponent.GetSelectedWeaponType();
		ChimeraCharacter character = ChimeraCharacter.Cast(targetEntity);
		if (character)
		{
			// Characters
			
			if (character.IsInVehicle())
			{
				// Aim at head
				aimpointType0 = EAimPointType.WEAK;
				aimpointType1 = EAimPointType.NORMAL;
				return;
			}
			
			if (weaponType == EWeaponType.WT_SNIPERRIFLE)
			{
				aimpointType0 = EAimPointType.INCAPACITATE;
				aimpointType1 = EAimPointType.WEAK;
				return;
			}
			
			aimpointType0 = EAimPointType.NORMAL;
			aimpointType1 = EAimPointType.WEAK;
			return;
		}
		else
		{
			if (weaponType == EWeaponType.WT_ROCKETLAUNCHER)
			{
				// Rocket launcher, aim at weak point
				aimpointType0 = EAimPointType.WEAK;
				aimpointType1 = EAimPointType.NORMAL;
				return;
			}
			
			aimpointType0 = EAimPointType.NORMAL;
			aimpointType1 = EAimPointType.WEAK;
			return;
		}
	}
}