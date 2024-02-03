class SCR_DCOAICombatComponentClass: ScriptComponentClass
{
};

enum DCO_EAISkill
{
	NONE,
	ROOKIE	= 20,
	REGULAR	= 50,
	VETERAN	= 70,
	EXPERT 	= 80,
	CYLON  	= 100
};


// be aware it is used for bitmask
enum DCO_EAICombatActions
{
	HOLD_FIRE = 1,
	BURST_FIRE = 2,
	SUPPRESSIVE_FIRE = 4,
	MOVEMENT_WHEN_FIRE = 8,
	MOVEMENT_TO_LAST_SEEN = 16,
};

enum DCO_EAICombatType
{
	NONE,
	NORMAL,
	SUPPRESSIVE,
	RETREAT,
	SINGLE_SHOT
};

//------------------------------------------------------------------------------------------------
class SCR_DCOAICombatComponent : ScriptComponent
{
	//-------------------------------------------------------------------------------------------------
	// Constants
	protected static const int	ENEMIES_SIMPLIFY_THRESHOLD = 12;
	protected static const float	RIFLE_BURST_RANGE_SQ = 50*50;
	
	protected static const float ASSIGNED_TARGETS_SCORE_INCREMENT = 30.0;
	protected static const float ENDANGERING_TARGETS_SCORE_INCREMENT = 15.0;
	
	// The object wrapping the weapon&target selection algorithm
	protected static const float TARGET_MAX_LAST_SEEN_DIRECT_ATTACK = 1.6;
	protected static const float TARGET_MAX_DISTANCE_INFANTRY = 500.0;
	protected static const float TARGET_MAX_DISTANCE_VEHICLE = 700.0;
	protected static const float TARGET_MAX_DISTANCE_DISARMED = 0.0;		// Max distance at which disarmed targets are considered for attack. Now it's 0, so they never shoot disarmed targets
	protected static const float TARGET_MAX_TIME_SINCE_ENDANGERED = 5.0; 	// Max time since we were endangered to consider the enemy target endangering
	protected static const float TARGET_INVISIBLE_TIME = 5.0; 				// how long in until invisible target becomes logically invisible
	protected static const float TARGET_INVESTIGATE_TIME = 7.0;				// how long before investigating the target's location 
	protected static const float TARGET_FORGET_TIME = 60;					// how long before forgetting the target 
	protected static const float TARGET_SCORE_RETREAT = 75.0;				// Threshold for target score for retreating from that target
			  static const float TARGET_SCORE_HIGH_PRIORITY_ATTACK = 96.0;	// Threshold for high priority attack
	
	protected static ref array<EWeaponType> s_aWeaponBlacklistFragGrenades = {EWeaponType.WT_FRAGGRENADE};
	
	// Perception factors
	protected const float PERCEPTION_FACTOR_SAFE = 2.0;			// We are safe and are good at recognizing enemies
	protected const float PERCEPTION_FACTOR_VIGILANT = 5.0;		// When vigilant and alert we are very good at recognizing enemies
	protected const float PERCEPTION_FACTOR_ALERTED = 5.0;
	protected const float PERCEPTION_FACTOR_THREATENED = 0.8;	// We are suppressed and are bad at recognizing enemies
	
	protected const float PERCEPTION_FACTOR_EQUIPMENT_BINOCULARS = 6.0;	// Looking through binoculars
	protected const float PERCEPTION_FACTOR_EQUIPMENT_NONE = 2.0;		// Not using any special equipment, same recognition ability as usual
	
	
	protected SCR_ChimeraAIAgent m_Agent;
	protected SCR_CharacterControllerComponent		m_CharacterController;
	protected SCR_InventoryStorageManagerComponent	m_InventoryManager;
	protected EventHandlerManagerComponent			m_EventHandlerManagerComponent;
	protected BaseWeaponManagerComponent			m_WpnManager;
	protected SCR_CompartmentAccessComponent		m_CompartmentAccess;
	protected ScriptedDamageManagerComponent		m_DamageManager;
	protected PerceptionComponent 					m_Perception;
	protected SCR_AIInfoComponent					m_AIInfo;
	protected SCR_AIUtilityComponent				m_Utility;
	
	// Cached data about current vehicle and compartment
	protected IEntity m_CurrentVehicle;								// Current vehicle. It's not always same as compartment owner!
	protected SCR_BaseCompartmentManagerComponent m_CurrentVehicleCompartmentManager;	// Vehicle's compartment manager
	protected BaseCompartmentSlot m_CurrentCompartmentSlot;			// Current compartment slot
	protected TurretControllerComponent m_CurrentTurretController;	// Current turret controller (if we are in a turret)

	protected EAICombatActions	m_iAllowedActions; //will be initialized with default combat type
	protected EAICombatType		m_eCombatType = EAICombatType.NORMAL;
	protected EAICombatType		m_eDefaultCombatType = EAICombatType.NORMAL;

	
	[Attribute("50", UIWidgets.ComboBox, "AI skill in combat", "", ParamEnumArray.FromEnum(EAISkill) )]
	protected EAISkill m_eAISkillDefault;
	protected EAISkill m_eAISkill;
	
	
	//-------------------------------------------------------------------------------------------------
	// Belongs to friendly in aim check
	protected bool m_bFriendlyAimLastResult;
	protected ref array<BaseTarget> m_FriendlyTargets = {};
	#ifdef WORKBENCH
	protected ref Shape m_FriendlyAimShape;
	#endif
	
	//-------------------------------------------------------------------------------------------------	
	// Weapon and target selection
	ref AIWeaponTargetSelector m_WeaponTargetSelector = new AIWeaponTargetSelector();
	private ref BaseTarget	m_SelectedTarget;
	protected ref BaseTarget m_SelectedRetreatTarget;				// Target we should retreat from
	protected ref array<IEntity> m_aAssignedTargets = {};			// Array with assigned targets. Their score is increased.
	protected SCR_AITargetClusterState m_TargetClusterState;		// Assigned target cluster from our group
	protected EAIUnitType m_eExpectedEnemyType = EAIUnitType.UnitType_Infantry;	// Enemy type we expect to fight
	protected ResourceName m_SelectedWeaponResource;				// selected weapon handle tree read from config

	// Weapon selection against a specific target and its properties
	protected BaseWeaponComponent m_SelectedWeaponComp; // Weapon, muzzle and magazine which we want to equip.
	protected BaseMagazineComponent m_SelectedMagazineComp; // Character might currently have a different one though.
	protected int m_iSelectedMuzzle;
	protected float m_fSelectedWeaponMinDist;
	protected float m_fSelectedWeaponMaxDist;
	protected bool m_bSelectedWeaponDirectDamage;
	protected EAIUnitType m_eUnitTypesCanAttack;
	protected BaseCompartmentSlot m_WeaponEvaluationCompartment; // Compartment at previous weapon evaluation
	
	// Weapon selection updates
	protected float m_fNextWeaponTargetEvaluation_ms = 0;
	protected const float WEAPON_TARGET_UPDATE_PERIOD_MS = 500;
	
	protected SCR_AIConfigComponent m_ConfigComponent;
	
	
	// Turret dismounting
	protected float m_fDismountTurretTimer;
	protected static const float DISMOUNT_TURRET_TARGET_LAST_SEEN_MAX_S = 100;
	protected static const float TURRET_TARGET_EXCESS_ANGLE_THRESHOLD_DEG = 3.0; // We will dismount turret when target's angle exceeds turret limits by this value
	protected const float DISMOUNT_TURRET_TIMER_MS = 1200; // How much time must pass till we decide to dismount a turret if enemy is out of turret limits.
	static ref array<EVehicleType> s_aForbidDismountTurretsOfVehicleTypes = {EVehicleType.APC}; // Array of vehicle types we should not dismount when turret can't shoot the target
	
	// Perception
	protected float m_fEquipmentPerceptionFactor = 1.0; // How good we can spot targets based on our current equipment. Updated by RecalculateEquipmentPerceptionFactor().
	
	// Gadget state. Used for detection when we use binoculars.
	EGadgetType m_eCurrentGadgetType = -1;
	bool m_bGadgetFocus = false; // True when we are in gadget focus mode (like looking through binoculars)
	
	//------------------------------------------------------------------------------------------------
	EAISkill GetAISkill()
	{
		return m_eAISkill;
	}
	
	//------------------------------------------------------------------------------------------------
	// use this to change AIskill dynamically
	void SetAISkill(EAISkill skill)
	{
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("SetAISkill: %1", typename.EnumToString(EAISkill, skill)));
		#endif
		
		m_eAISkill = skill;
	}
	
	//------------------------------------------------------------------------------------------------
	// use this to reset AI skill to default
	void ResetAISkill()
	{
		#ifdef AI_DEBUG
		AddDebugMessage("ResetAISkill");
		#endif
		
		m_eAISkill = m_eAISkillDefault;
	}
	
	//------------------------------------------------------------------------------------------------
	EWeaponType GetCurrentWeaponType()
	{
		BaseWeaponComponent currentWeapon = null;
		if (m_CurrentTurretController)
		{
			// If we are in turret, find turret weapon
			BaseWeaponManagerComponent turretWpnMgr = m_CurrentTurretController.GetWeaponManager();
			if (turretWpnMgr)
				currentWeapon = turretWpnMgr.GetCurrentWeapon();
		}
		else if (m_WpnManager)
		{
			// If not in turret, use character's weapon
			currentWeapon = m_WpnManager.GetCurrentWeapon();
		}
		
		if (currentWeapon)
			return currentWeapon.GetWeaponType();
		
		return EWeaponType.WT_NONE;
	}
	
	//------------------------------------------------------------------------------------------------
	BaseTarget GetCurrentTarget()
	{
		return m_SelectedTarget;
	}
	
	//------------------------------------------------------------------------------------------------
	BaseTarget GetRetreatTarget()
	{
		return m_SelectedRetreatTarget;
	}
	
	//------------------------------------------------------------------------------------------------
	BaseTarget GetLastSeenEnemy()
	{
		return m_Perception.GetLastSeenTarget(ETargetCategory.ENEMY, float.MAX);
	}	
	
	//------------------------------------------------------------------------------------------------
	// Finds out if some of my known enemies is aimng at me. 
	BaseTarget GetEndangeringEnemy()
	{
		array<BaseTarget> targets = {};
		m_Perception.GetTargetsList(targets, ETargetCategory.ENEMY);
		
		foreach (BaseTarget tgt : targets)
		{
			if (tgt.IsEndangering())
				return tgt;
		}
		
		return null;
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsEnemyKnown(IEntity ent)
	{
		bool knownAsEnemy = m_Perception.GetTargetPerceptionObject(ent, ETargetCategory.ENEMY);
		bool knownAsDetected = m_Perception.GetTargetPerceptionObject(ent, ETargetCategory.DETECTED);
		return knownAsEnemy || knownAsDetected;
	}
	
	//------------------------------------------------------------------------------------------------
	//! selectedWeaponChanged - when true, selected weapon, muzzle or magazine has changed
	//! selectedTargetChanged - when true, selected target has changed
	//! outRetreatTargetChanged - when true, the most dangerous target we should retreat from has changed since last evaluation
	//! outCompartmentChanged - when true, compartment has changed since last evaluation
	void EvaluateWeaponAndTarget(out bool outWeaponEvent, out bool outSelectedTargetChanged,
		out bool outRetreatTargetChanged, out bool outCompartmentChanged)
	{
		float worldTime = GetGame().GetWorld().GetWorldTime();
		if (worldTime < m_fNextWeaponTargetEvaluation_ms)
		{
			outWeaponEvent = false;
			outSelectedTargetChanged = false;
			return;
		}
		
		m_fNextWeaponTargetEvaluation_ms = worldTime + WEAPON_TARGET_UPDATE_PERIOD_MS;
		
		AIAgent myAgent = GetAiAgent();
		AIGroup myGroup = myAgent.GetParentGroup();
		SCR_AIGroupInfoComponent groupInfoComp;
		if (myGroup)
			groupInfoComp = SCR_AIGroupInfoComponent.Cast(myGroup.FindComponent(SCR_AIGroupInfoComponent));
		
		BaseTarget newTarget = null;
		bool weaponEvent = false;
		bool selectedTargetChanged = false;
		bool retreatTargetChanged = false;
		bool compartmentChanged = false;
		
		// Resolve if we want to think of throwing grenade
		array<EWeaponType> weaponBlacklist;
		if (groupInfoComp)
		{
			if (!groupInfoComp.IsGrenadeThrowAllowed(myAgent))
				weaponBlacklist = s_aWeaponBlacklistFragGrenades;
		}
		
		bool useCompartmentWeapons = m_AIInfo.HasUnitState(EUnitState.IN_TURRET); // True when we are in a turret
		
		// Which assigned targets array to use?
		array<IEntity> assignedTargets;
		if (m_TargetClusterState && m_TargetClusterState.m_Cluster && m_TargetClusterState.m_Cluster.m_aEntities)
			assignedTargets = m_TargetClusterState.m_Cluster.m_aEntities;
		else
			assignedTargets = m_aAssignedTargets;
		
		bool selectedWpnTarget = m_WeaponTargetSelector.SelectWeaponAndTarget(assignedTargets,
			ASSIGNED_TARGETS_SCORE_INCREMENT, ENDANGERING_TARGETS_SCORE_INCREMENT,
			useCompartmentWeapons, weaponTypesBlacklist: weaponBlacklist);
		
		m_eUnitTypesCanAttack = m_WeaponTargetSelector.GetUnitTypesCanAttack();
		if (selectedWpnTarget)
		{
			BaseWeaponComponent newWeaponComp;
			BaseMagazineComponent newMagazineComp;
			int newMuzzleId;
			
			newTarget = m_WeaponTargetSelector.GetSelectedTarget();
			m_WeaponTargetSelector.GetSelectedWeapon(newWeaponComp, newMuzzleId, newMagazineComp);
			m_WeaponTargetSelector.GetSelectedWeaponProperties(m_fSelectedWeaponMinDist, m_fSelectedWeaponMaxDist, m_bSelectedWeaponDirectDamage);
			
			
			weaponEvent = newWeaponComp != m_SelectedWeaponComp ||
							newMuzzleId != m_iSelectedMuzzle ||
							newMagazineComp != m_SelectedMagazineComp;
			
			if (newTarget && newWeaponComp)
			{
				SelectWeaponFireMode(newTarget, newWeaponComp);
			}
			
			bool weaponOrMuzzleChanged = newWeaponComp != m_SelectedWeaponComp ||
									newMuzzleId != m_iSelectedMuzzle;
			
			if (weaponOrMuzzleChanged)
			{
				ref array<BaseMuzzleComponent> muzzles = {};
				newWeaponComp.GetMuzzlesList(muzzles);
				if (newMuzzleId >= muzzles.Count() || newMuzzleId < 0)
					m_SelectedWeaponResource = m_ConfigComponent.GetTreeNameForWeaponType(newWeaponComp.GetWeaponType(),0);	
				else 
					m_SelectedWeaponResource = m_ConfigComponent.GetTreeNameForWeaponType(newWeaponComp.GetWeaponType(),muzzles[newMuzzleId].GetMuzzleType());
				
				if (newWeaponComp)
				{
					EWeaponType weaponType = newWeaponComp.GetWeaponType();
					if (groupInfoComp && weaponType == EWeaponType.WT_FRAGGRENADE)
					{
						// We want to throw a grenade
						// Notify group immediately
						groupInfoComp.OnAgentSelectedGrenade(myAgent);
					}
				}
			}
			
			m_SelectedWeaponComp = newWeaponComp;
			m_iSelectedMuzzle = newMuzzleId;
			m_SelectedMagazineComp = newMagazineComp;
		}
		
		if (SelectTarget(newTarget))
			selectedTargetChanged = true;
		
		// Check if we must retreat from some target
		BaseTarget targetCantAttack;
		float targetCantAttackScore;
		m_WeaponTargetSelector.GetMostRelevantTargetCantAttack(targetCantAttack, targetCantAttackScore);
		if (targetCantAttackScore < TARGET_SCORE_RETREAT)
			targetCantAttack = null;
		if (targetCantAttack != m_SelectedRetreatTarget)
		{
			m_SelectedRetreatTarget = targetCantAttack;
			retreatTargetChanged = true;
		}
		
		// Check if compartment has changed
		BaseCompartmentSlot currentCompartment = m_CompartmentAccess.GetCompartment();
		if (currentCompartment != m_WeaponEvaluationCompartment)
		{
			compartmentChanged = true;
			m_WeaponEvaluationCompartment = currentCompartment;
		}
				
		outWeaponEvent = weaponEvent;
		outSelectedTargetChanged = selectedTargetChanged;
		outRetreatTargetChanged = retreatTargetChanged;
		outCompartmentChanged = compartmentChanged;
	}
	
	//------------------------------------------------------------------------------------------------
	bool SelectTarget(BaseTarget newTarget)
	{
		
		if (newTarget == m_SelectedTarget)
			return false;
		
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("Target has changed. New: %1, Previous: %2", newTarget, m_SelectedTarget));
		#endif
		
		m_SelectedTarget = newTarget;
		
		// suppressive regime is cleared when target is changed
		if (GetCombatType() == EAICombatType.SUPPRESSIVE)
			ResetCombatType();
		
		return true;
	}
	
	//------------------------------------------------------------------------------------------------
	void SelectWeaponFireMode(BaseTarget newTarget, BaseWeaponComponent newWeaponComp)
	{
		if (!newTarget || !newWeaponComp)
			return;
		
		float targetDistanceSq = vector.DistanceSq(GetOwner().GetOrigin(), newTarget.GetTargetEntity().GetOrigin());
		
		EWeaponType wt = newWeaponComp.GetWeaponType();
		
		bool burstFire = false;
		if (wt == EWeaponType.WT_MACHINEGUN)
			burstFire = true;
		else if (targetDistanceSq < RIFLE_BURST_RANGE_SQ)
			burstFire = WeaponHasBurstOrAutoMode(newWeaponComp);
		else
			burstFire = false;
		
		SetActionAllowed(EAICombatActions.BURST_FIRE, burstFire);
	}
	
	//------------------------------------------------------------------------------------------------
	// Checks amount of magazines for given weapon, and if low on ammo, reports that to group
	bool EvaluateLowAmmo(BaseWeaponComponent weaponComp, int muzzleId)
	{
		if (!weaponComp)
			return false;
		array<BaseMuzzleComponent> muzzles = {};
		weaponComp.GetMuzzlesList(muzzles);
		if (muzzleId >= muzzles.Count() || muzzleId < 0)
			return false;
		
		BaseMuzzleComponent muzzleComp = muzzles[muzzleId];
		if (!muzzleComp)
			return false;
				
		// Ignore disposable weapons
		if (muzzleComp.IsDisposable())
			return false;
		
		int magCount = m_InventoryManager.GetMagazineCountByWeapon(weaponComp);
		
		int lowMagThreshold = 0;
		
		// Decide how many remainiing magazines is enough to complain
		switch (weaponComp.GetWeaponType())
		{
			case EWeaponType.WT_RIFLE: lowMagThreshold = 3; break;
			case EWeaponType.WT_GRENADELAUNCHER: lowMagThreshold = 3; break; // todo now it won't work when we are out of UGL ammo because weapons are not marked with WT_GRENADELAUNCHER
			case EWeaponType.WT_SNIPERRIFLE: lowMagThreshold = 1; break;
			case EWeaponType.WT_ROCKETLAUNCHER: lowMagThreshold = 1; break;
			case EWeaponType.WT_MACHINEGUN: lowMagThreshold = 1; break;
			case EWeaponType.WT_HANDGUN: lowMagThreshold = 1; break;
			default: lowMagThreshold = 0;
		}
		
		return magCount < lowMagThreshold;
	}
	
	//------------------------------------------------------------------------------------------------
	void Update(float timeSliceMs)
	{
		// Evaluate if we must dismount turret - only if we are already in turret
		if (m_CurrentTurretController)
			EvaluateDismountTurret(timeSliceMs);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Condition to dismount turret
	//! Returns true when we should dismount it immediately
	bool DismountTurretCondition(inout vector targetPos, bool targetPosProvided)
	{
		// False if not in turret
		if (!m_CurrentTurretController)
			return false;
		TurretComponent turretComp = m_CurrentTurretController.GetTurretComponent();
		if (!turretComp)
			return false;
		
		// False if we have a valid target to attack
		if (m_SelectedTarget)
			return false;
		
		// False if we have a driver in the vehicle
		array<BaseCompartmentSlot> compartments = {};
		m_CurrentVehicleCompartmentManager.GetCompartments(compartments);
		foreach (BaseCompartmentSlot slot : compartments)
		{
			if (PilotCompartmentSlot.Cast(slot) && slot.GetOccupant())
				return false;
		}
		
		// False if we are in a vehicle and we should not leave turret of this vehicle type
		// Note that static turrets are not of Vehicle class.
		Vehicle vehicle = Vehicle.Cast(m_CurrentVehicle);
		if (vehicle && s_aForbidDismountTurretsOfVehicleTypes.Find(vehicle.m_eVehicleType) != -1)
			return false;
		
		// If target pos is not provided, find a target which we are going to check against
		if (!targetPosProvided)
		{
			BaseTarget target = m_Perception.GetClosestTarget(ETargetCategory.DETECTED, DISMOUNT_TURRET_TARGET_LAST_SEEN_MAX_S, DISMOUNT_TURRET_TARGET_LAST_SEEN_MAX_S);
			if (target)
				targetPos = target.GetLastDetectedPosition();
			else
			{
				target = m_Perception.GetClosestTarget(ETargetCategory.ENEMY, DISMOUNT_TURRET_TARGET_LAST_SEEN_MAX_S, DISMOUNT_TURRET_TARGET_LAST_SEEN_MAX_S);
				if (target)
					targetPos = target.GetLastSeenPosition();
			}
			
			// False if there is no target which would cause us to dismount
			if (!target)
				return false;
			else
			{
				IEntity targetEntity = target.GetTargetEntity();
				if (!targetEntity)
					return false;
				else
				{
					vector bmin, bmax;
					targetEntity.GetBounds(bmin, bmax);
					targetPos = targetPos + 0.5 * (bmin + bmax);
				}
			}
		}
			
		// Check angle excess of the target's position
		vector angleExcess = turretComp.GetAimingAngleExcess(targetPos);
		
		//PrintFormat("Excess angle: %1", angleExcess);
		
		return angleExcess.Length() > TURRET_TARGET_EXCESS_ANGLE_THRESHOLD_DEG;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void EvaluateDismountTurret(float timeSliceMs)
	{
		vector targetPos;
		bool mustDismount = DismountTurretCondition(targetPos, false);
		
		if (!mustDismount)
		{
			m_fDismountTurretTimer = 0;
		}
		else
		{
			// Do nothing if already requested to dismount
			if (m_fDismountTurretTimer == -1.0)
				return;
			
			m_fDismountTurretTimer += timeSliceMs;
			
			if (m_fDismountTurretTimer > DISMOUNT_TURRET_TIMER_MS)
			{
				m_fDismountTurretTimer = -1.0;
				
				TryAddDismountTurretActions(targetPos);
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Adds action to dismount turret, investigate, and go back
	void TryAddDismountTurretActions(vector targetPos, bool addGetOut = true, bool addInvestigate = true, bool addGetIn = true)
	{
		BaseCompartmentSlot compartmentSlot = m_CurrentCompartmentSlot;
		if (!compartmentSlot)
			return;
		
		if (addGetOut)
		{
			AIActionBase prevGetOutAction = m_Utility.FindActionOfType(SCR_AIGetOutVehicle);
			if (prevGetOutAction)
				return;
			
			SCR_AIGetOutVehicle getOutAction = new SCR_AIGetOutVehicle(m_Utility, null, compartmentSlot.GetOwner(), SCR_AIActionBase.PRIORITY_BEHAVIOR_DISMOUNT_TURRET);
			m_Utility.AddAction(getOutAction);
		}
		
		if (addInvestigate)
		{
			AIActionBase prevInvestigate = m_Utility.FindActionOfType(SCR_AIMoveAndInvestigateBehavior);
			if (!prevInvestigate)
			{
				SCR_AIMoveAndInvestigateBehavior moveAndInvestigateAction = new SCR_AIMoveAndInvestigateBehavior(m_Utility, null, targetPos, SCR_AIActionBase.PRIORITY_BEHAVIOR_DISMOUNT_TURRET_INVESTIGATE, true);
				m_Utility.AddAction(moveAndInvestigateAction);
			}
		}
		
		if (addGetIn)
		{	
			AIActionBase prevGetInAction = m_Utility.FindActionOfType(SCR_AIGetInVehicle);
			if (!prevGetInAction)
			{
				SCR_AIGetInVehicle getInAction = new SCR_AIGetInVehicle(m_Utility, null, compartmentSlot.GetVehicle(), ECompartmentType.Turret, SCR_AIActionBase.PRIORITY_BEHAVIOR_DISMOUNT_TURRET_GET_IN);
				m_Utility.AddAction(getInAction);
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected static const float DISTANCE_MAX = 22; 
	protected static const float DISTANCE_MIN = 6; // Minimal distance when movement is allowed
	private static const float NEAR_PROXIMITY = 2;
	
	protected const float m_StopDistance = 30 + Math.RandomFloat(0, 10); 
	// TODO: add possibility to get cover towards custom position
	//------------------------------------------------------------------------------------------------
	vector FindNextCoverPosition()
	{
		if (!m_SelectedTarget)
			return vector.Zero;
		
		vector ownerPos = GetOwner().GetOrigin();
		vector lastSeenPos = m_SelectedTarget.GetLastSeenPosition();
		float distanceToTarget = vector.Distance(ownerPos, lastSeenPos);

		if (m_StopDistance > distanceToTarget)
			return vector.Zero;
		
		// Create randomized position
		SCR_ChimeraAIAgent agent = GetAiAgent();
		SCR_DefendWaypoint defendWp = SCR_DefendWaypoint.Cast(agent.m_GroupWaypoint);
		vector direction;
		bool standardAttack = true;
		float nextCoverDistance;
		
		// If target is outside defend waypoint, run towards center of it
		if (defendWp)
		{
			if (!defendWp.IsWithinCompletionRadius(lastSeenPos) &&
				!defendWp.IsWithinCompletionRadius(ownerPos))
			{
				direction = vector.Direction(ownerPos, defendWp.GetOrigin());	// Direction towards center of defend wp
				
				if (vector.Distance(defendWp.GetOrigin(), ownerPos) < DISTANCE_MIN)
					nextCoverDistance = 0;
				else	
					nextCoverDistance = DISTANCE_MIN;
				
				standardAttack = false;
			}
		}
		
		if (standardAttack)
		{
			nextCoverDistance = Math.RandomFloat(DISTANCE_MIN, DISTANCE_MAX);

			// If close enough, get directly to the target
			if (nextCoverDistance > (distanceToTarget - DISTANCE_MIN))
				nextCoverDistance = distanceToTarget - DISTANCE_MIN;
			
			direction = vector.Direction(ownerPos, m_SelectedTarget.GetLastSeenPosition());
		}
			
		direction.Normalize();
		vector newPositionCenter = direction * nextCoverDistance + ownerPos, newPosition;
		// yes possibly it could lead to end up in target position but lets ignore it for now
		
		newPosition = s_AIRandomGenerator.GenerateRandomPointInRadius(0, NEAR_PROXIMITY, newPositionCenter, true);
		newPosition[1] = newPositionCenter[1];
		return newPosition;
	}
	
	//------------------------------------------------------------------------------------------------
	static bool WeaponHasBurstOrAutoMode(notnull BaseWeaponComponent weaponComp)
	{
		if (!weaponComp)
			return false;
		
		BaseMuzzleComponent muzzle = weaponComp.GetCurrentMuzzle();
		if (!muzzle)
			return false;
		
		array<BaseFireMode> fireModes = new array<BaseFireMode>();
		int count = muzzle.GetFireModesList(fireModes);
		foreach (BaseFireMode mode : fireModes)
		{
			EWeaponFiremodeType modeType = mode.GetFiremodeType();
			if (modeType == EWeaponFiremodeType.Burst || modeType == EWeaponFiremodeType.Auto)
			{
				return true;
			}
		}
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void Event_OnInventoryChanged(IEntity item, BaseInventoryStorageComponent storageOwner)
	{
		// This event spams so much, especially at start, we want to process it only once
		//ScriptCallQueue callQueue = GetGame().GetCallqueue();
		//callQueue.Remove(Event_OnTimerAfterInventoryChanged);
		//callQueue.CallLater(Event_OnTimerAfterInventoryChanged, 2500, false);
	}
	protected void Event_OnTimerAfterInventoryChanged()
	{
		//EvaluateAndReportOutOfAmmo();
	}
	
	//------------------------------------------------------------------------------------------------
	void Event_OnCompartmentEntered( IEntity vehicle, BaseCompartmentManagerComponent manager, int mgrID, int slotID )
	{
		m_CurrentCompartmentSlot = manager.FindCompartment(slotID, mgrID);
		IEntity compartmentEntity = m_CurrentCompartmentSlot.GetOwner();
		m_CurrentTurretController = TurretControllerComponent.Cast(compartmentEntity.FindComponent(TurretControllerComponent));
		m_CurrentVehicle = m_CurrentCompartmentSlot.GetVehicle();
		m_CurrentVehicleCompartmentManager = SCR_BaseCompartmentManagerComponent.Cast(m_CurrentVehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
	}
	
	//------------------------------------------------------------------------------------------------
	void Event_OnCompartmentLeft( IEntity vehicle, BaseCompartmentManagerComponent manager, int mgrID, int slotID )
	{
		m_CurrentVehicle = null;
		m_CurrentCompartmentSlot = null;
		m_CurrentTurretController = null;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void Event_OnGadgetStateChanged(IEntity gadget, bool isInHand, bool isOnGround)
	{
		if (isInHand)
		{
			SCR_GadgetComponent gadgetComp = SCR_GadgetComponent.Cast(gadget.FindComponent(SCR_GadgetComponent));
			if (gadgetComp)
				m_eCurrentGadgetType = gadgetComp.GetType();
			else
				m_eCurrentGadgetType = -1;
		}
		else
			m_eCurrentGadgetType = -1;
		
		RecalculateEquipmentPerceptionFactor();
	}
	
	//------------------------------------------------------------------------------------------------
	protected void Event_OnGadgetFocusStateChanged(IEntity gadget, bool isFocused)
	{
		m_bGadgetFocus = isFocused;
		RecalculateEquipmentPerceptionFactor();
	}
	
	//------------------------------------------------------------------------------------------------
	protected void Event_OnDamageOverTimeAdded(EDamageType dType, float dps, HitZone hz)
	{
		if (dType != EDamageType.BLEEDING || !m_Utility || !m_Utility.m_AIInfo)
			return;
		
		SCR_AIActionBase currentAction = SCR_AIActionBase.Cast(m_Utility.GetCurrentAction());
		if (!currentAction)
			return;
		float priorityLevelClamped = currentAction.GetRestrictedPriorityLevel();
		
		if (m_Utility.m_AIInfo.HasRole(EUnitRole.MEDIC))
		{
			if (!m_Utility.HasActionOfType(SCR_AIHealBehavior))
			{
				// If we can heal ourselves, add Heal Behavior.
				SCR_AIHealBehavior behavior = new SCR_AIHealBehavior(m_Utility, null, m_Utility.m_OwnerEntity, true, priorityLevel: priorityLevelClamped);
				m_Utility.AddAction(behavior);
			}
		}
		else if (m_Agent)
		{
			// If we immediately know that we can't heal ourselves, report to group
			AIGroup myGroup = m_Agent.GetParentGroup();
			if (myGroup)
			{
				SCR_MailboxComponent myMailbox = SCR_MailboxComponent.Cast(m_Agent.FindComponent(SCR_MailboxComponent));
				SCR_AIMessage_Wounded msg = SCR_AIMessage_Wounded.Create(m_Utility.m_OwnerEntity);
				myMailbox.RequestBroadcast(msg, myGroup);
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Checks if unit is allowed to do action from the allowed actions enum
	bool IsActionAllowed(EAICombatActions action)
	{
		return (m_iAllowedActions & action);
	}
	
	//------------------------------------------------------------------------------------------------	
	private void SetActionAllowed(EAICombatActions action, bool isAllowed)
	{
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("SetActionAllowed: %1, %2", typename.EnumToString(EAICombatActions, action), isAllowed));
		#endif
		
		if ((m_iAllowedActions & action) != isAllowed)
		{
			if (isAllowed)
				m_iAllowedActions = m_iAllowedActions | action;
			else
				m_iAllowedActions = m_iAllowedActions & ~action;
		}
	}
	
	//------------------------------------------------------------------------------------------------	
	void SetHoldFire(bool isHoldFire)
	{
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("SetHoldFire: %1", isHoldFire));
		#endif
		
		if (isHoldFire)
		{ 
			SetActionAllowed(EAICombatActions.HOLD_FIRE,true);
			SetActionAllowed(EAICombatActions.BURST_FIRE,false);
			SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,false);
		}
		else
		{
			SetCombatType(m_eCombatType);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	EAICombatType GetCombatType()
	{
		return m_eCombatType;
	}
	
	// For current AI combat type can be changed during the behaviors
	// If you want AI to come bac to your state you have to set also default
	//------------------------------------------------------------------------------------------------
	void SetCombatType(EAICombatType combatType)
	{
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("SetCombatType: %1", typename.EnumToString(EAICombatType, combatType)));
		#endif
		
		switch (combatType)
		{
			case EAICombatType.NONE:
			{
				SetActionAllowed(EAICombatActions.HOLD_FIRE,true);
				SetActionAllowed(EAICombatActions.MOVEMENT_WHEN_FIRE,false);
				SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_TO_LAST_SEEN,false);
				break;
			}
			case EAICombatType.NORMAL:
			{
				SetActionAllowed(EAICombatActions.HOLD_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_WHEN_FIRE,true);
				SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_TO_LAST_SEEN,true);
				break;
			}
			case EAICombatType.SUPPRESSIVE:
			{
				SetActionAllowed(EAICombatActions.HOLD_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_WHEN_FIRE,false);
				SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,true);
				SetActionAllowed(EAICombatActions.MOVEMENT_TO_LAST_SEEN,true);
				break;
			}
			case EAICombatType.RETREAT:
			{
				SetActionAllowed(EAICombatActions.HOLD_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_WHEN_FIRE,true);
				SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_TO_LAST_SEEN,false);
				break;
			}
			case EAICombatType.SINGLE_SHOT:
			{
				SetActionAllowed(EAICombatActions.HOLD_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_WHEN_FIRE,false);
				SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_TO_LAST_SEEN,false);
				break;
			}
		}
		m_eCombatType = combatType;
#ifdef WORKBENCH
		SCR_AIDebugVisualization.VisualizeMessage(GetOwner(), typename.EnumToString(EAICombatType,m_eCombatType), EAIDebugCategory.COMBAT, 5);
#endif
	}
	
	// When AI use reset e.g. after bounding overwatch it is reseted to default combat type
	// You have to also call SetCombatType to have immediate effect.
	//------------------------------------------------------------------------------------------------
	void SetDefaultCombatType(EAICombatType combatType)
	{
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("SetDefaultCombatType: %1", typename.EnumToString(EAICombatType, combatType)));
		#endif
		
		m_eDefaultCombatType = combatType;
	}
	
	//------------------------------------------------------------------------------------------------
	void ResetCombatType()
	{
		#ifdef AI_DEBUG
		AddDebugMessage("ResetCombatType");
		#endif
		
		SetCombatType(m_eDefaultCombatType);
	}
	
	//------------------------------------------------------------------------------------------------
	bool IsFriendlyInAim()
	{
		IEntity friendlyEntInAim = m_Perception.GetFriendlyInLineOfFire();
#ifdef WORKBENCH
		if (friendlyEntInAim && DiagMenu.GetBool(SCR_DebugMenuID.DEBUGUI_AI_SHOW_FRIENDLY_IN_AIM))
			m_FriendlyAimShape = Shape.CreateSphere(COLOR_RED, ShapeFlags.NOOUTLINE|ShapeFlags.NOZBUFFER|ShapeFlags.TRANSP, friendlyEntInAim.GetOrigin() + Vector(0, 2, 0), 0.1);	
		else 
			m_FriendlyAimShape = null;
#endif		
		m_bFriendlyAimLastResult = friendlyEntInAim != null;
				
		return m_bFriendlyAimLastResult;
	}
		
	//------------------------------------------------------------------------------------------------
	//! Either array of targets or target cluster must be provided
	void SetAssignedTargets(array<IEntity> assignedTargets, SCR_AITargetClusterState clusterState)
	{
		m_aAssignedTargets.Clear();
		
		if (assignedTargets)
			m_aAssignedTargets.Copy(assignedTargets);
		else if (clusterState)
			m_TargetClusterState = clusterState;
	}
	
	//------------------------------------------------------------------------------------------------
	void ResetAssignedTargets()
	{
		m_aAssignedTargets.Clear();
		m_TargetClusterState = null;
	}
	
	//------------------------------------------------------------------------------------------------
	void SetExpectedEnemyType(EAIUnitType expectedEnemyType)
	{
		m_eExpectedEnemyType = expectedEnemyType;
	}
	
	//------------------------------------------------------------------------------------------------
	void ResetExpectedEnemyType()
	{
		m_eExpectedEnemyType = EAIUnitType.UnitType_Infantry;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Returns the weapon which combat component thinks is best against current target
	//! If there is no current target, then there is no selected weapon as well!
	//! In that case use GetExpectedWeapon
	void GetSelectedWeapon(out BaseWeaponComponent outWeaponComp, out int outMuzzleId, out BaseMagazineComponent outMagazineComp)
	{
		if (m_SelectedWeaponComp)
		{
			outWeaponComp = m_SelectedWeaponComp;
			outMagazineComp = m_SelectedMagazineComp;
			outMuzzleId = m_iSelectedMuzzle;
		}
		else
		{
			outWeaponComp = null;
			outMuzzleId = -1;
			outMagazineComp = null;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void GetSelectedWeaponProperties(out float outMinDistance, out float outMaxDistance, out bool outDirectDamage)
	{
		outMinDistance = m_fSelectedWeaponMinDist;
		outMaxDistance = m_fSelectedWeaponMaxDist;
		outDirectDamage = m_bSelectedWeaponDirectDamage;
	}
	
	//------------------------------------------------------------------------------------------------
	ResourceName GetSelectedWeaponResource()
	{
		return m_SelectedWeaponResource;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Returns weapon which combat component thinks is expected to be used this situation
	//! This should be used for selecting fallback weapon in a situation without an actively attacked target
	void EvaluateExpectedWeapon(out BaseWeaponComponent outWeaponComp, out int outMuzzleId, out BaseMagazineComponent outMagazineComp)
	{
		bool useCompartmentWeapons = m_AIInfo.HasUnitState(EUnitState.IN_TURRET);
		bool success = m_WeaponTargetSelector.SelectWeaponAgainstUnitType(m_eExpectedEnemyType, useCompartmentWeapons);
		
		if (success)
		{
			BaseWeaponComponent weaponComp;
			int muzzleId;
			BaseMagazineComponent magazineComp;
			
			m_WeaponTargetSelector.GetSelectedWeapon(weaponComp, muzzleId, magazineComp);
			
			outWeaponComp = weaponComp;
			outMuzzleId = muzzleId;
			outMagazineComp = magazineComp;
		}
		else
		{
			outWeaponComp = null;
			outMuzzleId = null;
			outMagazineComp = null;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	bool HasWeaponOfType(EWeaponType weaponType)
	{
		return m_WeaponTargetSelector.CharacterHasWeaponOfType(weaponType);
	}
	
	//------------------------------------------------------------------------------------------------
	int GetMagazineCount(typename magazineWell, bool countMagazineInWeapon = false)
	{
		int count = m_WeaponTargetSelector.GetCharacterMagazineCount(magazineWell);
		return count - 1 + countMagazineInWeapon;
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		m_EventHandlerManagerComponent = EventHandlerManagerComponent.Cast(owner.FindComponent(EventHandlerManagerComponent));
		m_eAISkill = m_eAISkillDefault;
		
		m_CharacterController = SCR_CharacterControllerComponent.Cast(owner.FindComponent(SCR_CharacterControllerComponent));
		if (m_CharacterController)
		{
			m_CharacterController.m_OnGadgetStateChangedInvoker.Insert(Event_OnGadgetStateChanged);
			m_CharacterController.m_OnGadgetFocusStateChangedInvoker.Insert(Event_OnGadgetFocusStateChanged);
		}
		
		m_WpnManager = BaseWeaponManagerComponent.Cast(owner.FindComponent(BaseWeaponManagerComponent));
		m_InventoryManager = SCR_InventoryStorageManagerComponent.Cast(owner.FindComponent(SCR_InventoryStorageManagerComponent));
		m_CompartmentAccess = SCR_CompartmentAccessComponent.Cast(owner.FindComponent(SCR_CompartmentAccessComponent));
		if (m_CompartmentAccess)
		{
			m_CompartmentAccess.GetOnCompartmentEntered().Insert(Event_OnCompartmentEntered);
			m_CompartmentAccess.GetOnCompartmentLeft().Insert(Event_OnCompartmentLeft);
		}
		m_Perception = PerceptionComponent.Cast(owner.FindComponent(PerceptionComponent));
		
		auto world = GetGame().GetWorld();
		if (world)
		{
			float worldTime = world.GetWorldTime();
			m_fNextWeaponTargetEvaluation_ms = worldTime + Math.RandomFloat(0, WEAPON_TARGET_UPDATE_PERIOD_MS);
		}
		
		SetCombatType(m_eDefaultCombatType);
		
		m_InventoryManager.m_OnItemAddedInvoker.Insert(Event_OnInventoryChanged);
		m_InventoryManager.m_OnItemRemovedInvoker.Insert(Event_OnInventoryChanged);
		
		if (owner)
		{
			InitWeaponTargetSelector(owner);
		}

		m_DamageManager = ScriptedDamageManagerComponent.Cast(owner.FindComponent(ScriptedDamageManagerComponent));
		if (m_DamageManager)
		{
			m_DamageManager.GetOnDamageOverTimeAdded().Insert(Event_OnDamageOverTimeAdded);
		}
		
		AIControlComponent ctrl = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		if (ctrl)
		{
			SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(ctrl.GetAIAgent());
			if (agent)
			{
				m_AIInfo = agent.m_InfoComponent;
				m_ConfigComponent = SCR_AIConfigComponent.Cast(agent.FindComponent(SCR_AIConfigComponent));
				m_Utility = SCR_AIUtilityComponent.Cast(agent.FindComponent(SCR_AIUtilityComponent));
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (m_CompartmentAccess)
		{
			m_CompartmentAccess.GetOnCompartmentEntered().Remove(Event_OnCompartmentEntered);
			m_CompartmentAccess.GetOnCompartmentLeft().Remove(Event_OnCompartmentLeft);
		}
		
		if (m_CharacterController)
		{
			m_CharacterController.m_OnGadgetStateChangedInvoker.Remove(Event_OnGadgetStateChanged);
			m_CharacterController.m_OnGadgetFocusStateChangedInvoker.Remove(Event_OnGadgetFocusStateChanged);
		}
		
		if (m_DamageManager)
		{
			m_DamageManager.GetOnDamageOverTimeAdded().Remove(Event_OnDamageOverTimeAdded);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	protected void InitWeaponTargetSelector(IEntity owner)
	{
		m_WeaponTargetSelector.Init(owner);
		
		// maxLastSeenIndirect must be consistent with conditions in ShouldAttackEndForTarget, therefore we use TARGET_INVISIBLE_TIME here
		m_WeaponTargetSelector.SetSelectionProperties(TARGET_MAX_LAST_SEEN_DIRECT_ATTACK, TARGET_INVISIBLE_TIME,
		TARGET_MAX_DISTANCE_INFANTRY, TARGET_MAX_DISTANCE_VEHICLE, TARGET_MAX_TIME_SINCE_ENDANGERED, TARGET_MAX_DISTANCE_DISARMED);
		
		m_WeaponTargetSelector.SetTargetScoreConstants(EAIUnitType.UnitType_Infantry,			100.0,	-0.1); // At short range we prefer to shoot enemies in vehicle
		m_WeaponTargetSelector.SetTargetScoreConstants(EAIUnitType.UnitType_VehicleUnarmored,	99.0,	-0.08);
		m_WeaponTargetSelector.SetTargetScoreConstants(EAIUnitType.UnitType_VehicleMedium,		150.0,	-0.15);
		m_WeaponTargetSelector.SetTargetScoreConstants(EAIUnitType.UnitType_VehicleHeavy,		200.0,	-0.15);
		m_WeaponTargetSelector.SetTargetScoreConstants(EAIUnitType.UnitType_Aircraft,			90.0,	-0.015);
		m_WeaponTargetSelector.SetTargetScoreConstants(EAIUnitType.UnitType_Fortification,		40.0,	-0.1); // Fortifications are not used ATM
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdatePerceptionFactor(PerceptionComponent perceptionComp, SCR_AIThreatSystem threatSystem)
	{
		EAIThreatState threatState = threatSystem.GetState();
		float perceptionFactor;
		switch (threatState)
		{
			case EAIThreatState.SAFE:
				perceptionFactor = PERCEPTION_FACTOR_SAFE; break; 
			case EAIThreatState.VIGILANT:
				perceptionFactor = PERCEPTION_FACTOR_VIGILANT; break;
			case EAIThreatState.ALERTED:
				perceptionFactor = PERCEPTION_FACTOR_ALERTED; break; 
			case EAIThreatState.THREATENED:
				perceptionFactor = PERCEPTION_FACTOR_THREATENED; break; 
		}
		
		perceptionFactor *= m_fEquipmentPerceptionFactor;
		
		perceptionComp.SetPerceptionFactor(perceptionFactor);
	}
	
	//------------------------------------------------------------------------------------------------
	protected void RecalculateEquipmentPerceptionFactor()
	{
		if (m_eCurrentGadgetType == EGadgetType.BINOCULARS && m_bGadgetFocus)
			m_fEquipmentPerceptionFactor = PERCEPTION_FACTOR_EQUIPMENT_BINOCULARS;
		else
			m_fEquipmentPerceptionFactor = PERCEPTION_FACTOR_EQUIPMENT_NONE;
	}
	
	//------------------------------------------------------------------------------------------------
	void ~SCR_DCOAICombatComponent()
	{
		GetGame().GetCallqueue().Remove(Event_OnTimerAfterInventoryChanged);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Used by AIDebugInfoComponent
	void DebugPrintToWidget(TextWidget w)
	{
		string str = string.Format("\n%1", m_SelectedTarget.ToString());
		if (m_SelectedTarget)
			str = str + string.Format("\n%1", m_SelectedTarget.GetTimeSinceSeen());
		else
			str = str + "\n";
		w.SetText(str);
	}
	
	SCR_ChimeraAIAgent GetAiAgent()
	{
		if (m_Agent)
			return m_Agent;
		
		AIControlComponent controlComp = AIControlComponent.Cast(GetOwner().FindComponent(AIControlComponent));
		m_Agent = SCR_ChimeraAIAgent.Cast(controlComp.GetAIAgent());
		return m_Agent;
	}
	
	//------------------------------------------------------------------------------------------------
	BaseTarget FindTargetByEntity(IEntity ent)
	{
		BaseTarget tgt = m_Perception.FindTargetPerceptionObject(ent);
		if (tgt)
			return tgt;
		
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("FindTargetByEntity: did not find target: %1", ent), LogLevel.WARNING);
		Print(string.Format("FindTargetByEntity: did not find target: %1", ent), LogLevel.WARNING);
		#endif
		return null;
	}
	
	//--------------------------------------------------------------------------------------------
	void UpdateLastSeenPosition(IEntity entity, notnull SCR_AITargetInfo targetInfo)
	{
		BaseTarget tgt = m_Perception.FindTargetPerceptionObject(entity);
		if (!tgt)
			return;
		
		if (targetInfo.m_fTimestamp > tgt.GetTimeLastSeen())
			tgt.UpdateLastSeenPosition(targetInfo.m_vWorldPos, targetInfo.m_fTimestamp);
	}
	
	//--------------------------------------------------------------------------------------------
	void UpdateLastSeenPosition(notnull BaseTarget tgt, notnull SCR_AITargetInfo targetInfo)
	{		
		if (targetInfo.m_fTimestamp > tgt.GetTimeLastSeen())
			tgt.UpdateLastSeenPosition(targetInfo.m_vWorldPos, targetInfo.m_fTimestamp);
	}
	
	#ifdef AI_DEBUG
	//--------------------------------------------------------------------------------------------
	protected void AddDebugMessage(string str, LogLevel logLevel = LogLevel.NORMAL)
	{
		AIControlComponent controlComp = AIControlComponent.Cast(GetOwner().FindComponent(AIControlComponent));
		AIAgent agent = controlComp.GetAIAgent();
		if (!agent)
			return;
		SCR_AIInfoBaseComponent infoComp = SCR_AIInfoBaseComponent.Cast(agent.FindComponent(SCR_AIInfoBaseComponent));
		infoComp.AddDebugMessage(str, msgType: EAIDebugMsgType.COMBAT, logLevel);
	}
	#endif
	
	//------------------------------------------------------------------------------------------------
	// Contains logic wheather or not should attack end or not based on timeouts, returns true if target is obsolete
	bool ShouldAttackEndForTarget(BaseTarget enemyTarget, out bool shouldInvestigateFurther = false, out string context = string.Empty)
	{
		float timeLastSeen = enemyTarget.GetTimeSinceSeen();
		IEntity targetEntity = enemyTarget.GetTargetEntity();
		ETargetCategory targetCategory = enemyTarget.GetTargetCategory();
		
		// End if no longer enemy (the case for vehicle which was occupied and became empty)
		if (targetCategory != ETargetCategory.ENEMY)
		{
#ifdef AI_DEBUG
			AddDebugMessage(string.Format("Ending attack for target: %1. Target category is no longer ETargetCategory.ENEMY", enemyTarget));
			context = "Target category is no longer ETargetCategory.ENEMY";
#endif
			return true;
		}
		
		// End if target entity is deleted
		if (!targetEntity)
		{
#ifdef AI_DEBUG
			AddDebugMessage(string.Format("Ending attack for target: %1. Target entity is null.", enemyTarget));
			context = "Target entity is null";
#endif
			return true;
		};
		
		// End if not seen for a long time
		if (timeLastSeen > TARGET_INVISIBLE_TIME)
		{
			if (timeLastSeen > TARGET_FORGET_TIME)
			{
#ifdef AI_DEBUG
				AddDebugMessage(string.Format("Ending attack for target: %1. Target is invisible for too long.", enemyTarget));
				context = "Target is invisible for too long";
#endif
				return true;
			};
			// investigate or use suppressive fire
			if (timeLastSeen > TARGET_INVESTIGATE_TIME)
			{
				// if static -> forget about target
				if (m_AIInfo && (m_AIInfo.HasUnitState(EUnitState.IN_TURRET) || !IsActionAllowed(EAICombatActions.MOVEMENT_TO_LAST_SEEN)))
				{
#ifdef AI_DEBUG
					AddDebugMessage(string.Format("Ending attack for target: %1. Agent not allowed to investigate a lost enemy.", enemyTarget));
					context = "Agent not allowed to investigate a lost enemy";
#endif
					return true;
				}	
				// if normal combat mode and allow investigate -> move individually
				else if (GetCombatType() == EAICombatType.NORMAL && IsActionAllowed(EAICombatActions.MOVEMENT_TO_LAST_SEEN))
				{
#ifdef AI_DEBUG
					AddDebugMessage(string.Format("Ending attack for target: %1. Agent will investigate a lost enemy.", enemyTarget));
					context = "Agent will investigate a lost enemy";
#endif
					shouldInvestigateFurther = true;
					return true;
				}
			}
		}
		
		// End if we have no weapons to attack this target
		if ((enemyTarget.GetUnitType() & m_eUnitTypesCanAttack) == 0)
		{
#ifdef AI_DEBUG
			AddDebugMessage(string.Format("Ending attack for target: %1. No weapons to attack this target.", enemyTarget));
			context = "No weapons to attack this target";
#endif
			shouldInvestigateFurther = false;
			return true;
		}
		
		return false;
	}
};