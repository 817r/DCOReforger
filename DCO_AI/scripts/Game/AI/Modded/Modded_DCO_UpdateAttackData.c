modded class SCR_AIUpdateTargetAttackData : AITaskScripted
{	
	protected const int FIRE_TREE_INVALID 		= -1;	// No aiming or firing is allowed at all
	protected const int FIRE_TREE_LOOK			= 0;	// Looking at target without firing
	protected const int FIRE_TREE_BURST			= 1;
	protected const int FIRE_TREE_SINGLE		= 2;
	protected const int FIRE_TREE_SUPPRESSIVE	= 3;
	protected const int FIRE_TREE_MELEE			= 4;
	protected const int FIRE_TREE_LOOK_THREATS	= 5;	// Looking at data from threat system
	protected const int FIRE_TREE_THROW_GRENADE	= 6;
	protected const int FIRE_TREE_RPG			= 7;
	
	
	protected const float BURST_FIRE_MAX_DISTANCE = 70.0;

	//-----------------------------------------------------------------------------------------------------
	// Evaluates which fire tree should be used
	override int ResolveFireTree(BaseTarget target, bool visible, bool weaponReady, out float fireRate)
	{
		// Is aiming forbidden by combat move?
		SCR_AIBehaviorBase executedBehavior = SCR_AIBehaviorBase.Cast(m_UtilityComponent.GetExecutedAction());
		if (executedBehavior && executedBehavior.m_bUseCombatMove && !m_UtilityComponent.m_CombatMoveState.m_bAimAtTarget)
			return FIRE_TREE_LOOK_THREATS;
		
		// Is looking at threats activated?
		if (m_bLookAtThreats)
			return FIRE_TREE_LOOK_THREATS;
		
		// Is weapon not ready?
		if (!weaponReady)
			return FIRE_TREE_LOOK;
		
		
		
		BaseWeaponComponent selectedWeaponComp;
		int selectedMuzzleId;
		m_CombatComponent.GetSelectedWeapon(selectedWeaponComp, selectedMuzzleId);
		
		bool directDamage; // FIX: pre-existing syntax bug, titik-koma kelewat di sini (sebelum ada perubahan apapun dari kita)
		float weaponMinDist, weaponMaxDist;
		m_CombatComponent.GetSelectedWeaponProperties(weaponMinDist, weaponMaxDist, directDamage);
		EWeaponType weaponType = selectedWeaponComp.GetWeaponType();
		
		if (!selectedWeaponComp)
			return FIRE_TREE_LOOK;
		
		// Hold fire?
		if (m_CombatComponent.GetCombatMode() == EAIGroupCombatMode.HOLD_FIRE)
			return FIRE_TREE_LOOK;
		
		
		
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
	
		/*if (target.GetUnitType() == EAIUnitType.UnitType_Aircraft)
		{
			if (weaponType == EWeaponType.WT_MACHINEGUN)
			{
				if (target.GetDistance() > 50)
					return FIRE_TREE_LOOK;
				else if (target.GetTraceFraction() > 0.8)
				{
					return FIRE_TREE_SUPPRESSIVE;
				}
			} else if (!weaponType == EWeaponType.WT_ROCKETLAUNCHER || weaponType == EWeaponType.WT_MACHINEGUN)
			{
				return FIRE_TREE_LOOK;
			}
		}*/
		
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
				
				// === ADDED: High-Value Target prioritization ===
				if (IsHighValueTarget(target, visible))
					fireRate *= 1.5;
				// === END ADDED ===
				
				return FIRE_TREE_SUPPRESSIVE;
			}
			return FIRE_TREE_LOOK;
		}

		// Within weapon usage range
		
		if (visible)
		{
			// === ADDED: High-Value Target prioritization ===
			bool isHVT = IsHighValueTarget(target, visible);
			// === END ADDED ===
			
			// Visible
			// If machinegun, always use burst at any range
			// For regular weapons, use burst at short range if available, otherwise single
			if (weaponType == EWeaponType.WT_MACHINEGUN)
				return FIRE_TREE_BURST;
			else if ((targetDistance < BURST_FIRE_MAX_DISTANCE || isHVT) && m_bWeaponHasBurstOrAuto) // MODIFIED: "|| isHVT" -- HVT dapet burst walau di luar jarak burst normal
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
				
				// === ADDED: High-Value Target prioritization ===
				if (IsHighValueTarget(target, false))
					fireRate *= 1.5;
				// === END ADDED ===
								
				return FIRE_TREE_SUPPRESSIVE;
			}
			//else if (target.GetTimeSinceSeen() > 3 && (target.GetDistance() > 50 && m_CombatComponent.HasWeaponOfType(EWeaponType.WT_ROCKETLAUNCHER)))
			//{
			//	return FIRE_TREE_RPG;
			//}
			else if (target.GetTimeSinceSeen() > 2 && target.GetDistance() < 15 && target.GetTraceFraction() > 0.5)
			{
				SCR_AIThrowGrenadeToBehavior gren = new SCR_AIThrowGrenadeToBehavior(m_UtilityComponent, null, target.GetLastSeenPosition(), EWeaponType.WT_FRAGGRENADE, 1, SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE + 
				SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER);
				m_UtilityComponent.AddAction(gren);
				return FIRE_TREE_LOOK;
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
	
	// === ADDED: High-Value Target prioritization ===
	// Cek apakah target ini "bernilai tinggi" (bawa MG/AT/sniper) berdasarkan senjata
	// yang dia pegang saat ini. Dipakai buat bikin respon (burst-fire, suppressive fire
	// rate) lebih agresif -- ini BUKAN target re-selection (SCR_AIWeaponTargetSelector-nya
	// vanilla, gak bisa kita intervensi milih target). Ini cuma "begitu ngunci HVT, abis-
	// abisan", bukan "selalu tembak HVT duluan".
	//
	// Sengaja gak omniscient penuh: kalau target lagi gak visible DAN udah lama gak keliatan
	// (>5 detik), kita anggap "belum tau" loadout-nya, return false.
	protected static bool IsHighValueTarget(BaseTarget target, bool visible)
	{
		if (!target)
			return false;
		
		if (!visible && target.GetTimeSinceSeen() > 5.0)
			return false;
		
		IEntity targetEntity = target.GetTargetEntity();
		if (!targetEntity)
			return false;
		
		BaseWeaponManagerComponent wpnMgr = BaseWeaponManagerComponent.Cast(targetEntity.FindComponent(BaseWeaponManagerComponent));
		if (!wpnMgr)
			return false;
		
		array<BaseWeaponComponent> weapons = {};
		wpnMgr.GetWeapons(weapons);
		
		foreach (BaseWeaponComponent w : weapons)
		{
			if (!w)
				continue;
			
			EWeaponType wt = w.GetWeaponType();
			if (wt == EWeaponType.WT_MACHINEGUN || wt == EWeaponType.WT_ROCKETLAUNCHER || wt == EWeaponType.WT_SNIPERRIFLE)
				return true;
		}
		
		return false;
	}
	// === END ADDED ===
}