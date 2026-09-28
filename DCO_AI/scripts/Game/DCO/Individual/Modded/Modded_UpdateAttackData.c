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
	protected const int FIRE_TREE_GL			= 8;

	protected const float BURST_FIRE_MAX_DISTANCE = 70.0;

	protected const float CLOSE_DIRECT_THREAT_DIST = 80.0;

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

		if (!selectedWeaponComp)
			return FIRE_TREE_LOOK;

		EWeaponType weaponType = selectedWeaponComp.GetWeaponType();

		if (m_CombatComponent.GetCombatMode() == EAIGroupCombatMode.HOLD_FIRE && IsCloseDirectThreat(target, visible))
		{
			if (ShouldBreakDisciplineByChance(visible))
			{
				if (!ShouldReturnFireWhenEndangered())
					return FIRE_TREE_LOOK;
			}
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

		bool longHold = targetDistance > DCO_LongRange.EffectiveRange(weaponType) && DCO_LongRange.ShouldHold(m_UtilityComponent);
		bool farRifle = longHold && weaponType != EWeaponType.WT_MACHINEGUN;
		if (longHold)
			DCO_LongRange.ApplyStance(m_UtilityComponent, visible);

		if (targetDistance < weaponMinDist || targetDistance > weaponMaxDist)
		{
			if (weaponType == EWeaponType.WT_MACHINEGUN)
			{
				if (DCO_AmmoUtility.ShouldAvoidSuppressiveFire(m_UtilityComponent, selectedWeaponComp))
					return FIRE_TREE_LOOK;

				float maxFireRate = Math.Max(1, Math.Map(targetDistance, 0, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE, 2, 1));
				fireRate = DCO_LongRange.CapFireRate(maxFireRate * threat, targetDistance, weaponType);

				fireRate *= DCO_AmmoUtility.GetAmmoConservationScale(m_UtilityComponent, selectedWeaponComp);

				return FIRE_TREE_SUPPRESSIVE;
			}
			return FIRE_TREE_LOOK;
		}

		if (visible)
		{
			float effectiveBurstMaxDist = BURST_FIRE_MAX_DISTANCE * DCO_PersonalityCombatUtility.GetBurstDistanceScale(m_UtilityComponent);

			if (weaponType == EWeaponType.WT_MACHINEGUN)
			{
				if (targetDistance > DCO_LongRange.EffectiveRange(weaponType))
				{
					fireRate = DCO_LongRange.MG_FAR_FIRE_RATE;
					return FIRE_TREE_SUPPRESSIVE;
				}
				return FIRE_TREE_BURST;
			}
			else if (farRifle)
			{
				if (!DCO_LongRange.IsShotWindow(m_UtilityComponent.m_OwnerEntity))
					return FIRE_TREE_LOOK;
				return FIRE_TREE_SINGLE;
			}
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

			lastSeenThreshold = Math.Max(SCR_AICombatComponent.TARGET_MIN_LAST_SEEN_INDIRECT_ATTACK * 2, lastSeenThreshold * threat * ThreatRememberFromPersonality());

			vector grenadeThrowPos;

			if (target.GetTimeSinceSeen() < lastSeenThreshold
				&& DCO_UGLUtility.ShouldUseGL(m_UtilityComponent, selectedWeaponComp, target.GetLastSeenPosition()))
			{
				return FIRE_TREE_GL;
			}

			if ((!directDamage || weaponType != EWeaponType.WT_ROCKETLAUNCHER) &&
				target.GetTimeSinceSeen() < lastSeenThreshold &&
				target.GetTraceFraction() > 0.4)
			{
				if (DCO_AmmoUtility.ShouldAvoidSuppressiveFire(m_UtilityComponent, selectedWeaponComp))
					return FIRE_TREE_LOOK;

				if (farRifle && !DCO_LongRange.IsShotWindow(m_UtilityComponent.m_OwnerEntity))
					return FIRE_TREE_LOOK;

				float maxFireRate = Math.Max(1, Math.Map(targetDistance, 0, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE, 3, 1));
				fireRate = DCO_LongRange.CapFireRate(maxFireRate * threat, targetDistance, weaponType);

				fireRate *= DCO_AmmoUtility.GetAmmoConservationScale(m_UtilityComponent, selectedWeaponComp);

				return FIRE_TREE_SUPPRESSIVE;
			}
			else if (target.GetTimeSinceSeen() > 2
				&& m_CombatComponent.HasWeaponOfType(EWeaponType.WT_FRAGGRENADE)
				&& DCO_GrenadeUtility.CanThrowGrenadeNow(m_UtilityComponent)
				&& DCO_GrenadeUtility.ResolveThrowPos(m_UtilityComponent, DCO_AimUtility.ScatterExplosive(m_UtilityComponent, target.GetLastSeenPosition(), false), grenadeThrowPos))
			{
				SCR_AIThrowGrenadeToBehavior gren = new SCR_AIThrowGrenadeToBehavior(m_UtilityComponent, null, grenadeThrowPos, EWeaponType.WT_FRAGGRENADE, 1, SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE +
				SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER);
				m_UtilityComponent.AddAction(gren);
				DCO_GrenadeUtility.NotifyGrenadeThrown(m_UtilityComponent);

				return FIRE_TREE_LOOK;
			}
			else if ((!directDamage || weaponType == EWeaponType.WT_ROCKETLAUNCHER) &&
				target.GetTimeSinceSeen() < lastSeenThreshold &&
				target.GetTraceFraction() > 0.4)
			{
				return FIRE_TREE_RPG;
			}
			else
				return FIRE_TREE_LOOK;
		}

		return FIRE_TREE_LOOK;
	}

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
			if (character.IsInVehicle())
			{
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

	protected bool ThreatRememberFromPersonality()
	{
		float personalityScale  = DCO_PersonalityCombatUtility.GetThreatRememberFromPersonalityScale(m_UtilityComponent);
		return personalityScale;
	}
}