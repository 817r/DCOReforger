modded class SCR_AIUpdateTargetAttackData : AITaskScripted
{	
	// Inputs
	protected static const string BASE_TARGET_PORT = "BaseTarget";
	protected static const string WEAPON_IS_READY = "WeaponReady";
	
	// Outputs
	protected static const string PORT_LAST_SEEN_POSITION = "LastSeenPosition";
	protected static const string PORT_THREAT_POSITION = "ThreatPosition";
	protected static const string PORT_VISIBLE = "Visible";
	protected static const string PORT_FIRE_TREE_ID = "FireTreeId";
	protected static const string PORT_FIRE_RATE = "FireRate";
	static const string PORT_AIMPOINT_TYPE_0 = "AimpointType0";
	static const string PORT_AIMPOINT_TYPE_1 = "AimpointType1";
	
	
	// These IDs must match to actual trees in attack tree
	protected const int FIRE_TREE_INVALID 		= -1;	// No aiming or firing is allowed at all
	protected const int FIRE_TREE_LOOK			= 0;	// Looking at target without firing
	protected const int FIRE_TREE_BURST			= 1;
	protected const int FIRE_TREE_SINGLE		= 2;
	protected const int FIRE_TREE_SUPPRESSIVE	= 3;
	protected const int FIRE_TREE_MELEE			= 4;
	protected const int FIRE_TREE_LOOK_THREATS	= 5;	// Looking at data from threat system
	protected const int FIRE_TREE_RPG			= 6;
	
	protected const float MELEE_MAX_DISTANCE = 1.5;
	protected const float BURST_FIRE_MAX_DISTANCE = 55.0;
	
	protected SCR_ChimeraAIAgent m_Agent;
	protected SCR_AICombatComponent m_CombatComponent;
	protected CharacterControllerComponent m_CharacterController;
	protected PerceptionComponent m_PerceptionComponent;
	protected SCR_AIUtilityComponent m_UtilityComponent;
	protected SCR_AISectorThreatFilter m_ThreatFilter;
	protected BaseTarget m_Target;
	
	// Flag for executing some logic only once at start
	protected bool m_bFirstSimulate = true;
	
	protected bool m_bWeaponHasBurstOrAuto; // Cached on first run
	
	// State of looking at threat system
	protected bool m_bLookAtThreats;
	protected WorldTimestamp m_LookAtThreatsEndTime;
	protected vector m_vLookAtThreatsPos;

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
		
		if (targetDistance < weaponMinDist || targetDistance > weaponMaxDist)
		{
			// Outside weapon usage range
			// Look at target
			if (weaponType == EWeaponType.WT_MACHINEGUN)
				return FIRE_TREE_SUPPRESSIVE;
			
			return FIRE_TREE_SINGLE;
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
			else if (target.GetPerceivableComponent().IsInCompartment() && m_CombatComponent.HasWeaponOfType(EWeaponType.WT_ROCKETLAUNCHER) && targetDistance < 450)
				return FIRE_TREE_RPG;
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
			
			float threat = m_UtilityComponent.m_ThreatSystem.GetThreatMeasure();
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
				float maxFireRate = Math.Max(1, Math.Map(targetDistance, 0, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE, 2.5, 1));
				fireRate = maxFireRate * threat;
								
				return FIRE_TREE_SUPPRESSIVE;
			}
			else if (target.GetTraceFraction() > 0.35 && m_CombatComponent.HasWeaponOfType(EWeaponType.WT_ROCKETLAUNCHER) && targetDistance > 20 && targetDistance < 500)
			{
				return FIRE_TREE_RPG;
			}
			else if (target.GetTimeSinceSeen() < 6 && target.GetTraceFraction() > 0.5)
			{
				if (targetDistance < SCR_AICombatComponent.CLOSE_RANGE_COMBAT_DISTANCE && weaponType == EWeaponType.WT_MACHINEGUN)
					return FIRE_TREE_SUPPRESSIVE;
				else if (targetDistance < SCR_AICombatComponent.CLOSE_RANGE_COMBAT_DISTANCE && weaponType != EWeaponType.WT_MACHINEGUN)
					return FIRE_TREE_BURST;
				else if (weaponType == EWeaponType.WT_MACHINEGUN)
					return FIRE_TREE_BURST;
				else
					return FIRE_TREE_SINGLE;
			}
			else
				return FIRE_TREE_LOOK;
		}
		
		return FIRE_TREE_LOOK;
	}

	//-----------------------------------------------------------------------------------------------------
	protected override float CalculateLookAtThreatDuration(BaseTarget currentTarget, SCR_EAIThreatSectorFlags threatFlags)
	{
		// Time depends on how far our target is.
		// The further the target, the more time we can spare to look at something else.
		float tgtDist = currentTarget.GetDistance();
		float baseDuration = -6 + 4*Math.Log10(tgtDist + 21.2);
		
		if (threatFlags & SCR_EAIThreatSectorFlags.CAUSED_DAMAGE)
			baseDuration *= 2.0;
		else if (threatFlags & SCR_EAIThreatSectorFlags.DIRECTED_AT_ME)
			baseDuration *= 1.5;
		
		float duration = Math.RandomFloat(baseDuration, 1.5*baseDuration);
		
		return duration;
	}
	
	//-----------------------------------------------------------------------------------------------------
	protected static const float COS_VIEW_CONE = 0.814; // Cos 45, should be same as view cone as perception
	protected override bool PosSameDirectionAsTarget(vector threatPos)
	{
		IEntity controlledEntity = m_Agent.GetControlledEntity();
		
		if (!controlledEntity || !m_Target)
			return false;
		
		vector myPos = controlledEntity.GetOrigin();
		vector targetPos = m_Target.GetLastSeenPosition();
		
		vector dirToTarget = (targetPos - myPos).Normalized();
		vector dirToThreat = (threatPos - myPos).Normalized();
		float cosAngle = vector.Dot(dirToTarget, dirToThreat);
		
		return cosAngle > COS_VIEW_CONE;		
	}
}