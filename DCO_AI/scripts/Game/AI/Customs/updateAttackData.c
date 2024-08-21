modded class SCR_AIUpdateTargetAttackData : AITaskScripted
{	
	// Inputs
	protected static const string BASE_TARGET_PORT = "BaseTarget";
	protected static const string WEAPON_IS_READY = "WeaponReady";
	
	// Outputs
	protected static const string PORT_LAST_SEEN_POSITION = "LastSeenPosition";
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
	
	protected const float MELEE_MAX_DISTANCE = 2.0;
	protected const float BURST_FIRE_MAX_DISTANCE = 50.0;
	
	protected SCR_ChimeraAIAgent m_Agent;
	protected SCR_AICombatComponent m_CombatComponent;
	protected CharacterControllerComponent m_CharacterController;
	protected PerceptionComponent m_PerceptionComponent;
	protected SCR_AIUtilityComponent m_UtilityComponent;
	
	// Flag for executing some logic only once at start
	protected bool m_bFirstSimulate = true;
	
	protected bool m_bWeaponHasBurstOrAuto; // Cached on first run
	
	//-----------------------------------------------------------------------------------------------------
	override void OnInit(AIAgent owner)
	{
		m_UtilityComponent = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
		m_Agent = SCR_ChimeraAIAgent.Cast(owner);
		
		IEntity myEntity = owner.GetControlledEntity();
		if (myEntity)
		{
			m_CombatComponent = SCR_AICombatComponent.Cast(myEntity.FindComponent(SCR_AICombatComponent));
			
			m_PerceptionComponent = PerceptionComponent.Cast(myEntity.FindComponent(PerceptionComponent));
			m_CharacterController = CharacterControllerComponent.Cast(myEntity.FindComponent(CharacterControllerComponent));
		}
		
		m_bFirstSimulate = true;
	}
	
	//-----------------------------------------------------------------------------------------------------
	override void OnAbort(AIAgent owner, Node nodeCausingAbort)
	{
		m_bFirstSimulate = true;
	}
	
	//-----------------------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		BaseTarget target;
		GetVariableIn(BASE_TARGET_PORT, target);
		
		bool weaponReady;
		GetVariableIn(WEAPON_IS_READY, weaponReady);
		
		if (!target || !m_CombatComponent || !m_PerceptionComponent || !m_CharacterController || !m_UtilityComponent)
			return ENodeResult.FAIL;
		
		if (m_bFirstSimulate)
		{
			BaseWeaponComponent selectedWeaponComp;
			int selectedMuzzleId;
			m_CombatComponent.GetSelectedWeapon(selectedWeaponComp, selectedMuzzleId);
			m_bWeaponHasBurstOrAuto = WeaponHasBurstOrAutoMode(selectedWeaponComp, selectedMuzzleId);
		}
		
		// Last seen pos
		SetVariableOut(PORT_LAST_SEEN_POSITION, target.GetLastSeenPosition());
		
		// Visible?
		bool visible = m_CombatComponent.IsTargetVisible(target);
		SetVariableOut(PORT_VISIBLE, visible);
		
		// Fire rate modifier (modified in ResolveFireTree)
		float fireRate = 1;
		
		// Which fire tree to use?
		// This must be reevaluated periodically
		int fireTreeId = ResolveFireTree(target, visible, weaponReady, fireRate);
		SetVariableOut(PORT_FIRE_TREE_ID, fireTreeId);
		SetVariableOut(PORT_FIRE_RATE, fireRate);
		
		// Which aimpoints to use?
		// We run this only once, since it's not going to change much
		if (m_bFirstSimulate)
		{
			EAimPointType aimpointType0;
			EAimPointType aimpointType1;
			ResolveAimpointTypes(target, aimpointType0, aimpointType1);
			SetVariableOut(PORT_AIMPOINT_TYPE_0, aimpointType0);
			SetVariableOut(PORT_AIMPOINT_TYPE_1, aimpointType1);
		}
		
		// Reset m_bFirstSimulate flag
		m_bFirstSimulate = false;
		
		return ENodeResult.SUCCESS;
	}
	
	//-----------------------------------------------------------------------------------------------------
	// Evaluates which fire tree should be used
	override int ResolveFireTree(BaseTarget target, bool visible, bool weaponReady, out float fireRate)
	{
		// Is aiming forbidden by combat move?
		SCR_AIBehaviorBase executedBehavior = SCR_AIBehaviorBase.Cast(m_UtilityComponent.GetExecutedAction());
		if (executedBehavior && executedBehavior.m_bUseCombatMove && !m_UtilityComponent.m_CombatMoveState.m_bAimAtTarget)
			return FIRE_TREE_INVALID;
		
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
		
		float threat = m_UtilityComponent.m_ThreatSystem.GetThreatTotal();
		
		if (targetDistance < weaponMinDist || targetDistance > weaponMaxDist)
		{
			// Outside weapon usage range
			// Look at target
			
			if(threat > 3.5)
				return FIRE_TREE_BURST;
			
			
			return FIRE_TREE_LOOK;
		}

		// Within weapon usage range
		
		if (visible)
		{
			// Visible
			// If machinegun, always use burst at any range
			// For regular weapons, use burst at short range if available, otherwise single
			if (weaponType == EWeaponType.WT_MACHINEGUN && targetDistance < 50)
				return FIRE_TREE_BURST;
			else if (weaponType == EWeaponType.WT_MACHINEGUN)
				return FIRE_TREE_SUPPRESSIVE;
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
				target.GetTraceFraction() > 0.4)
			{
				float maxFireRate = Math.Max(1, Math.Map(targetDistance, 0, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE * 2, 3, 1));
				if (threat > 3.5)	fireRate = maxFireRate * (threat - 0.5);
				else fireRate = maxFireRate * threat;
								
				return FIRE_TREE_SUPPRESSIVE;
			}
			else if (targetDistance < 50 && target.GetTimeSinceSeen() < lastSeenThreshold)
				return FIRE_TREE_BURST;
			else
				return FIRE_TREE_LOOK;
		}
		
		return FIRE_TREE_LOOK;
	}
		
	//-----------------------------------------------------------------------------------------------------
	protected ref TStringArray s_aVarsIn = {
		BASE_TARGET_PORT,
		WEAPON_IS_READY
	};
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
	
	protected ref TStringArray s_aVarsOut = {
		PORT_VISIBLE,
		PORT_LAST_SEEN_POSITION,
		PORT_FIRE_TREE_ID,
		PORT_AIMPOINT_TYPE_0,
		PORT_AIMPOINT_TYPE_1,
		PORT_FIRE_RATE
	};
	override TStringArray GetVariablesOut() { return s_aVarsOut; }
	
	override bool VisibleInPalette() { return true; }
	
	override string GetOnHoverDescription() { return "Special node which is used in attack behavior"; };

}