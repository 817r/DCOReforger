modded class SCR_AICombatComponent : ScriptComponent
{
	// Constants
	static const int			 TARGET_ENDANGERED_TIMEOUT_S = 10;				//!< How long after last time target was endangering we stop treating it as such
	static const float			 ENDANGERING_TARGET_SCORE_MULTIPLIER = 1.05;	//!< Multiplier of target score if target is considered endangering
	
	// Score increments for assigned targets and endangering targets
	protected static const float ASSIGNED_TARGETS_SCORE_INCREMENT = 0.5;
	protected static const float ENDANGERING_TARGETS_SCORE_INCREMENT = 25.0;

	// Perception factors
	protected const float PERCEPTION_FACTOR_SAFE = 1.0;			//!< We are safe and are good at recognising enemies
	protected const float PERCEPTION_FACTOR_VIGILANT = 2.5;		//!< When vigilant and alert we are very good at recognising enemies
	protected const float PERCEPTION_FACTOR_ALERTED = 2.5;
	protected const float PERCEPTION_FACTOR_THREATENED = 0.4;	// We are suppressed and are bad at recognizing enemies
	
	protected const float PERCEPTION_FACTOR_EQUIPMENT_BINOCULARS = 3.0;	//!< Looking through binoculars
	protected const float PERCEPTION_FACTOR_EQUIPMENT_NONE = 1.0;		//!< Not using any special equipment, same recognition ability as usual
	
	//! Within this distance AI considers combat as 'close range', used in firing times
	static const float CLOSE_RANGE_COMBAT_DISTANCE = 40.0;

	//! Beyond this distance AI considers combat as 'long range', used for danger events and firing times
	static const float LONG_RANGE_COMBAT_DISTANCE = 300.0;
	
	// AIM Improvement Factors
	protected const float AIM_IMPROVEMENT_BASE_IMPROVEMENT = 0.004;
	protected const float AIM_IMPROVEMENT_EQUIPMENT = 0.005;
	protected const float AIM_IMPROVEMENT_INTERVAL_S = 2;
	
	// AIM Decrement Value
	protected const float AIM_SUPPRESSED_DECREMENT = 0.0038;
	protected const float AIM_IMPROVEMENT_DECAY = 0.3;
	
	float m_fImprovementTimer;
	
	// AI Aim Improvement from threat states if its enabled
	protected const float AIM_IMPROVEMENT_SAFE = 1.0;
	protected const float AIM_IMPROVEMENT_VIGILANT = 5;
	protected const float AIM_IMPROVEMENT_ALERTED = 4;
	protected const float AIM_IMPROVEMENT_THREATENED = 3;
	
	protected float AIM_IMPROVEMENT = 1;

	protected SCR_ChimeraAIAgent m_Agent;
	protected SCR_CharacterControllerComponent		m_CharacterController;
	protected SCR_InventoryStorageManagerComponent	m_InventoryManager;
	protected BaseWeaponManagerComponent			m_WpnManager;
	protected SCR_CompartmentAccessComponent		m_CompartmentAccess;
	protected SCR_ExtendedDamageManagerComponent	m_DamageManager;
	protected PerceptionComponent 					m_Perception;
	protected SCR_AIInfoComponent					m_AIInfo;
	protected SCR_AIUtilityComponent				m_Utility;
	protected DCO_AIMoraleSystemComponent			m_Morale;
	
	protected SCR_AIConfigComponent m_ConfigComponent;
	
	protected float m_TimeLastUpdate;
	
	protected bool isNewTarget;
	
	//------------------------------------------------------------------------------------------------
	//!
	//! \param[in] timeSliceMs
	override void Update(float timeSliceMs)
	{
		// Evaluate if we must dismount turret - only if we are already in turret
		if (m_CurrentTurretController)
			EvaluateDismountTurret(timeSliceMs);
		if (m_SelectedTarget)
			CalculateOverallAimImprovement(timeSliceMs);
	}

	//------------------------------------------------------------------------------------------------
	override protected void Event_OnDamage(BaseDamageContext damageContext)
	{
		if (damageContext.damageType != EDamageType.FIRE || !m_Utility || !m_CurrentVehicle || m_bCurrentVehicleEvac)
			return;
		
		// Fire damage inside a vehicle - evac vehicle
		m_bCurrentVehicleEvac = true;
		
		SCR_AIGetOutVehicle behaviorGetOut = new SCR_AIGetOutVehicle(m_Utility, null, m_CurrentVehicle, priority: SCR_AIActionBase.PRIORITY_BEHAVIOR_GET_OUT_VEHICLE_HIGH_PRIORITY);
		m_Utility.AddAction(behaviorGetOut);
		
		SCR_AIMoveFromDangerBehavior behaviorMoveFromDanger = new SCR_AIMoveFromDangerBehavior(m_Utility, null, m_CurrentVehicle.GetOrigin(), m_CurrentVehicle);
		m_Utility.AddAction(behaviorMoveFromDanger);
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		m_eAISkill = m_eAISkillDefault;
		
		ChimeraCharacter character = ChimeraCharacter.Cast(owner);
		if (character)
		{	
			m_CharacterController = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
		if (m_CharacterController)
		{
			m_CharacterController.m_OnGadgetStateChangedInvoker.Insert(Event_OnGadgetStateChanged);
			m_CharacterController.m_OnGadgetFocusStateChangedInvoker.Insert(Event_OnGadgetFocusStateChanged);
		}
		
			m_WpnManager = m_CharacterController.GetWeaponManagerComponent();
			
			m_InventoryManager = SCR_InventoryStorageManagerComponent.Cast(m_CharacterController.GetInventoryStorageManager());
			if (m_InventoryManager)
			{
				m_InventoryManager.m_OnItemAddedInvoker.Insert(Event_OnInventoryChanged);
				m_InventoryManager.m_OnItemRemovedInvoker.Insert(Event_OnInventoryChanged);
			}
			
			m_CompartmentAccess = SCR_CompartmentAccessComponent.Cast(character.GetCompartmentAccessComponent());
		if (m_CompartmentAccess)
		{
			m_CompartmentAccess.GetOnCompartmentEntered().Insert(Event_OnCompartmentEntered);
			m_CompartmentAccess.GetOnCompartmentLeft().Insert(Event_OnCompartmentLeft);
		}
		}	
			
		m_Perception = PerceptionComponent.Cast(owner.FindComponent(PerceptionComponent));
		
		BaseWorld world = GetGame().GetWorld();
		if (world)
		{
			float worldTime = world.GetWorldTime();
			m_fNextWeaponTargetEvaluation_ms = worldTime + Math.RandomFloat(0, WEAPON_TARGET_UPDATE_PERIOD_MS);
		}

		if (owner)
		{
			m_Morale = DCO_AIMoraleSystemComponent.Cast(owner.FindComponent(DCO_AIMoraleSystemComponent));
			InitWeaponTargetSelector(owner);
		}

		m_DamageManager = SCR_ExtendedDamageManagerComponent.Cast(owner.FindComponent(SCR_ExtendedDamageManagerComponent));
		if (m_DamageManager)
		{
			m_DamageManager.GetOnDamageEffectAdded().Insert(Event_OnDamageEffectAdded);
			m_DamageManager.GetOnDamage().Insert(Event_OnDamage);
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
	
	override bool EvaluateLowAmmo(BaseWeaponComponent weaponComp, int muzzleId)
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
		int lowMagThreshold = GetWeaponLowMagThreshold(weaponComp);
		
		return magCount < lowMagThreshold;
	}
	
	override int GetWeaponLowMagThreshold(BaseWeaponComponent weapon)
	{
		EWeaponType wpType = SCR_AIWeaponHandling.GetWeaponType(weapon, true);
		SCR_AIWeaponTypeHandlingConfig config = m_Utility.m_ConfigComponent.GetWeaponTypeHandlingConfig(wpType);
		if (!config)
			return SCR_AIWeaponTypeHandlingConfig.DEFAULT_LOW_MAG_THRESHOLD;
		
		return 2;
	}
	
	override void UpdatePerceptionFactor(PerceptionComponent perceptionComp, SCR_AIThreatSystem threatSystem)
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
		perceptionFactor *= m_fPerceptionFactor;
		perceptionFactor *= MoralePerception();
		
		perceptionComp.SetPerceptionFactor(perceptionFactor);
	}
	
	float MoralePerception()
	{
		switch(m_Morale.GetMoraleStates())
		{
			case MoraleState.FRESH:
			{
				return 2.0;
				break;
			}
			case MoraleState.NORMAL:
			{
				return 1.5;
				break;
			}
			case MoraleState.STRESSED:
			{
				return 1.2;
				break;
			}
			case MoraleState.PRESSURED:
			{
				return 0.5;
				break;
			}
			case MoraleState.BREAK:
			{
				return 0.5;
				break;
			}
		}
			
		return 1.0;
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
			m_DamageManager.GetOnDamageEffectAdded().Remove(Event_OnDamageEffectAdded);
			m_DamageManager.GetOnDamage().Remove(Event_OnDamage);
		}
	}
	
	void CalculateOverallAimImprovement(float timeSliceMs)
	{		
		if (!m_Utility.DCO_ConfComponent.EnableAimImprovement())
		{
			AIM_IMPROVEMENT = 1;
		}
		else
		{
			bool Visible = IsTargetVisible(GetCurrentTarget());
			float Distt = Math.Clamp(GetCurrentTarget().GetDistance(), 0, m_Utility.DCO_ConfComponent.GetAimMaxRangeEffect());
			float DistanceImprovementMultiplier = Math.Map(Distt, 0, m_Utility.DCO_ConfComponent.GetAimMaxRangeEffect(), m_Utility.DCO_ConfComponent.GetMaxAimImprovement(), 1);			
			
			m_fImprovementTimer += timeSliceMs;
			
			if (isNewTarget)
			{
				float random = Math.RandomFloat(1, 2);
				AIM_IMPROVEMENT = Math.Clamp(AIM_IMPROVEMENT - (AIM_IMPROVEMENT_DECAY * random), 1, 10);
			} else if (m_fImprovementTimer > AIM_IMPROVEMENT_INTERVAL_S * 1000)
			{
				m_fImprovementTimer = 0;
				float random = Math.RandomFloat(1, 1.5);
				float suppressionVal = Math.Map(m_Utility.m_ThreatSystem.GetSuppressionMeasure(), 0, 1, 0, AIM_SUPPRESSED_DECREMENT);
				AIM_IMPROVEMENT = Math.Clamp((AIM_IMPROVEMENT + AIM_IMPROVEMENT_BASE_IMPROVEMENT * DistanceImprovementMultiplier - suppressionVal) * random, 1, 10);
			}
			
			if (GetCurrentTarget().GetTimeSinceSeen() > 6)
			{
				AIM_IMPROVEMENT = 1;
			}
		}
	}
	
	override void EvaluateWeaponAndTarget(out bool outWeaponEvent, out bool outSelectedTargetChanged,
		out BaseTarget outPrevTarget, out BaseTarget outCurrentTarget,
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
		
		SCR_ChimeraAIAgent myAgent = GetAiAgent();
		float agentThreat = m_Utility.m_ThreatSystem.GetThreatMeasure();

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
		// Grenade throw is synchronized via group
		array<EWeaponType> weaponBlacklist;
		if (groupInfoComp)
		{
			if (agentThreat > FRAG_GRENADE_MAX_THREAT || !groupInfoComp.IsGrenadeThrowAllowed(myAgent))
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
		
		BaseTarget prevTarget = m_SelectedTarget;
		if (newTarget != m_SelectedTarget)
		{
			#ifdef AI_DEBUG
			AddDebugMessage(string.Format("Target has changed. New: %1, Previous: %2", newTarget, m_SelectedTarget));
			#endif
			m_SelectedTarget = newTarget;
			selectedTargetChanged = true;
			isNewTarget = true;
		} else isNewTarget = false;
		
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
		
		// Reset last velocity if target changed
		if (selectedTargetChanged)
		{
			m_SelectedTargetVisible = false;
			m_SelectedTargetDestinationPos = vector.Zero;
		}
			
		if (newTarget)
		{
			
			bool visible = IsTargetVisible(newTarget);
			IEntity targetEntity = newTarget.GetTargetEntity();
			
			if (visible != m_SelectedTargetVisible)
			{
				m_SelectedTargetVisible = visible;
				
				// Save position (destination) of target pos at the time we figured we've lost LOS
				// Note: this solution is dependent on update rate of EvaluateWeaponAndTarget
				if (!visible && targetEntity)
					m_SelectedTargetDestinationPos = targetEntity.GetOrigin();
			}
		}
				
		outWeaponEvent = weaponEvent;
		outSelectedTargetChanged = selectedTargetChanged;
		outRetreatTargetChanged = retreatTargetChanged;
		outCompartmentChanged = compartmentChanged;
		outCurrentTarget = newTarget;
		outPrevTarget = prevTarget;
	}
	
	float MoraleAimFactor()
	{
		switch(m_Morale.GetMoraleStates())
		{
			case MoraleState.FRESH:
			{
				return 1.0;
				break;
			}
			case MoraleState.NORMAL:
			{
				return 1.4;
				break;
			}
			case MoraleState.STRESSED:
			{
				return 1.9;
				break;
			}
			case MoraleState.PRESSURED:
			{
				return 2.5;
				break;
			}
			case MoraleState.BREAK:
			{
				return 3.2;
				break;
			}
		}
		
		return 1.0;
	}
	
	DCO_AIMoraleSystemComponent GetMoraleComponent()
	{
		return m_Morale;
	}
	
	float GetOverallAimImprovement()
	{
		return AIM_IMPROVEMENT;
	}	
	
	SCR_AITargetClusterState GetAITargetClusterState()
	{
		return m_TargetClusterState;
	}
}