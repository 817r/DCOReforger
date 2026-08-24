modded class SCR_AIUpdateTargetAttackData : AITaskScripted
{	
	protected const int FIRE_TREE_INVALID 		= -1;
	protected const int FIRE_TREE_LOOK			= 0;
	protected const int FIRE_TREE_BURST			= 1;
	protected const int FIRE_TREE_SINGLE		= 2;
	protected const int FIRE_TREE_SUPPRESSIVE	= 3;
	protected const int FIRE_TREE_MELEE			= 4;
	protected const int FIRE_TREE_LOOK_THREATS	= 5;
	protected const int FIRE_TREE_THROW_GRENADE	= 6;
	protected const int FIRE_TREE_RPG			= 7;
	
	
	protected const float BURST_FIRE_MAX_DISTANCE = 70.0;
	
	protected const float CLOSE_DIRECT_THREAT_DIST = 60.0;
	
	protected const float GRENADE_MIN_THROW_DIST = 5.0;
	protected const float GRENADE_MAX_THROW_DIST = 25.0;

	override int ResolveFireTree(BaseTarget target, bool visible, bool weaponReady, out float fireRate)
	{
		SCR_AIBehaviorBase executedBehavior = SCR_AIBehaviorBase.Cast(m_UtilityComponent.GetExecutedAction());
		if (executedBehavior && executedBehavior.m_bUseCombatMove && !m_UtilityComponent.m_CombatMoveState.m_bAimAtTarget)
		{
			if (!IsCloseDirectThreat(target, visible))
				return FIRE_TREE_INVALID;
		}
		
		if (m_bLookAtThreats)
			return FIRE_TREE_LOOK_THREATS;
		
		if (!weaponReady)
			return FIRE_TREE_LOOK;
		
		BaseWeaponComponent selectedWeaponComp;
		int selectedMuzzleId;
		m_CombatComponent.GetSelectedWeapon(selectedWeaponComp, selectedMuzzleId);
		
		bool directDamage;
		float weaponMinDist, weaponMaxDist;
		m_CombatComponent.GetSelectedWeaponProperties(weaponMinDist, weaponMaxDist, directDamage);
		EWeaponType weaponType = selectedWeaponComp.GetWeaponType();
		
		if (!selectedWeaponComp)
			return FIRE_TREE_LOOK;
		

		if (m_CombatComponent.GetCombatMode() == EAIGroupCombatMode.HOLD_FIRE
			&& !IsCloseDirectThreat(target, visible)
			&& !ShouldReturnFireWhenEndangered())
		{
			if (!ShouldBreakDisciplineByChance(visible))
				return FIRE_TREE_LOOK;
		}
		
		
		float targetDistance = target.GetDistance();
		
		if (targetDistance < MELEE_MAX_DISTANCE &&
			!m_CharacterController.CanFire() &&
			m_CharacterController.GetStance() != ECharacterStance.PRONE)
		{
			return FIRE_TREE_MELEE;
		}
		
		if (m_PerceptionComponent.GetFriendlyInLineOfFire())
		{
			return FIRE_TREE_LOOK;
		}
		
		float threat = m_UtilityComponent.m_ThreatSystem.GetThreatMeasure();
		if (targetDistance < weaponMinDist || targetDistance > weaponMaxDist)
		{
			
			if (weaponType == EWeaponType.WT_MACHINEGUN)
			{
				if (DCO_AmmoUtility.ShouldAvoidSuppressiveFire(m_UtilityComponent, selectedWeaponComp))
					return FIRE_TREE_LOOK;
				
				float maxFireRate = Math.Max(1, Math.Map(targetDistance, 0, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE, 2, 1));
				fireRate = maxFireRate * threat;
				
				fireRate *= DCO_AmmoUtility.GetAmmoConservationScale(m_UtilityComponent, selectedWeaponComp);
				
				return FIRE_TREE_SUPPRESSIVE;
			}
			return FIRE_TREE_LOOK;
		}

		
		if (visible)
		{
			float effectiveBurstMaxDist = BURST_FIRE_MAX_DISTANCE * DCO_PersonalityCombatUtility.GetBurstDistanceScale(m_UtilityComponent);
			
			if (weaponType == EWeaponType.WT_MACHINEGUN)
				return FIRE_TREE_BURST;
			else if (targetDistance < effectiveBurstMaxDist && m_bWeaponHasBurstOrAuto)
				return FIRE_TREE_BURST;
			else
				return FIRE_TREE_SINGLE;
		}
		else
		{			
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
				if (DCO_AmmoUtility.ShouldAvoidSuppressiveFire(m_UtilityComponent, selectedWeaponComp))
					return FIRE_TREE_LOOK;
				
				float maxFireRate = Math.Max(1, Math.Map(targetDistance, 0, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE, 3, 1));
				fireRate = maxFireRate * threat;
				
				fireRate *= DCO_AmmoUtility.GetAmmoConservationScale(m_UtilityComponent, selectedWeaponComp);
								
				return FIRE_TREE_SUPPRESSIVE;
			}
			else if (target.GetTimeSinceSeen() > 2 && target.GetDistance() < 20 && target.GetTraceFraction() > 0.5
				&& m_CombatComponent.HasWeaponOfType(EWeaponType.WT_FRAGGRENADE)
				&& DCO_GrenadeUtility.CanThrowGrenadeNow(m_UtilityComponent))
			{
				SCR_AIThrowGrenadeToBehavior gren = new SCR_AIThrowGrenadeToBehavior(m_UtilityComponent, null, target.GetLastSeenPosition(), EWeaponType.WT_FRAGGRENADE, 1, SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE + 
				SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER);
				m_UtilityComponent.AddAction(gren);
				DCO_GrenadeUtility.NotifyGrenadeThrown(m_UtilityComponent);
				return FIRE_TREE_THROW_GRENADE;
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
				aimpointType1 = EAimPointType.NORMAL;
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
	
	protected bool IsCloseDirectThreat(BaseTarget target, bool visible)
	{
		if (!target || !visible)
			return false;
		
		return target.GetDistance() < CLOSE_DIRECT_THREAT_DIST;
	}

	protected bool ShouldReturnFireWhenEndangered()
	{
		if (!m_UtilityComponent || !m_UtilityComponent.m_ThreatSystem)
			return false;
		
		float threat = m_UtilityComponent.m_ThreatSystem.GetThreatMeasure();
		float threshold = DCO_PersonalityCombatUtility.GetEndangeredReturnFireThreshold(m_UtilityComponent);
		
		return threat >= threshold;
	}
	
	protected bool ShouldBreakDisciplineByChance(bool visible)
	{
		if (!visible)
			return false;
		
		float skillFactor       = DCO_PersonalityCombatUtility.GetSkillDisciplineFactor(m_UtilityComponent);
		float personalityScale  = DCO_PersonalityCombatUtility.GetDisciplineBreakChanceScale(m_UtilityComponent);
		
		float breakChance = 0.35 * skillFactor * personalityScale;
		breakChance = Math.Clamp(breakChance, 0.0, 0.4);
		
		return Math.RandomFloat01() < breakChance;
	}
}