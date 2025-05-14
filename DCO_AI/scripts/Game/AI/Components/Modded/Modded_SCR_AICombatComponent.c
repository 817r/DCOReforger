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
	protected const float PERCEPTION_FACTOR_THREATENED = 0.8;	// We are suppressed and are bad at recognizing enemies
	
	protected const float PERCEPTION_FACTOR_EQUIPMENT_BINOCULARS = 3.0;	//!< Looking through binoculars
	protected const float PERCEPTION_FACTOR_EQUIPMENT_NONE = 1.0;		//!< Not using any special equipment, same recognition ability as usual
	
	//! Within this distance AI considers combat as 'close range', used in firing times
	static const float CLOSE_RANGE_COMBAT_DISTANCE = 40.0;

	//! Beyond this distance AI considers combat as 'long range', used for danger events and firing times
	static const float LONG_RANGE_COMBAT_DISTANCE = 300.0;
	
	// AIM Improvement Factors
	protected const float AIM_IMPROVEMENT_BASE_IMPROVEMENT = 0.0055;
	protected const float AIM_IMPROVEMENT_EQUIPMENT = 0.002;
	protected const float AIM_IMPROVEMENT_INTERVAL_S = 1;
	
	// AIM Decrement Value
	protected const float AIM_SUPPRESSED_DECREMENT = 0.01;
	protected const float AIM_LOW_MORALE_DECREMENT = 0.003;
	protected const float AIM_IMPROVEMENT_DECAY_NO_TARGET = 0.003;
	protected const float AIM_IMPROVEMENT_DECAY = 0.03;
	
	float m_fImprovementTimer;
	
	// AI Aim Improvement from threat states if its enabled
	protected const float AIM_IMPROVEMENT_SAFE = 1.0;
	protected const float AIM_IMPROVEMENT_VIGILANT = 5;
	protected const float AIM_IMPROVEMENT_ALERTED = 4;
	protected const float AIM_IMPROVEMENT_THREATENED = 3;
	
	protected const float TAKE_COVER_CHANCES_DEFAULT = 80;
	protected float TAKE_COVER_CHANCES;
	
	protected float AIM_IMPROVEMENT = 1;
	protected static const float TARGET_MAX_DISTANCE_DISARMED = 1.2;

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
		super.Update(timeSliceMs);
		// Evaluate if we must dismount turret - only if we are already in turret
		if (m_CurrentTurretController)
			EvaluateDismountTurret(timeSliceMs);
		if (m_SelectedTarget)
			CalculateOverallAimImprovement(timeSliceMs);
		
		CalculateCoverChances();
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
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

	override void UpdatePerceptionFactor(PerceptionComponent perceptionComp, SCR_AIThreatSystem threatSystem)
	{
		super.UpdatePerceptionFactor(m_Perception,threatSystem);
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
		perceptionFactor *= SkillPerception();
		
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
				return 0.8;
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
	
	float SkillPerception()
	{
		switch(m_Utility.DCO_ConfComponent.GetSkillLevel())
		{
			case DCO_SKILL.CONSCPRIT:
			{
				return 0.3;
				break;
			}
			case DCO_SKILL.GREEN:
			{
				return 0.7;
				break;
			}
			case DCO_SKILL.REGULAR:
			{
				return 1;
				break;
			}
			case DCO_SKILL.VETERAN:
			{
				return 1.3;
				break;
			}
			case DCO_SKILL.CRACK:
			{
				return 1.6;
				break;
			}
			case DCO_SKILL.ELITE:
			{
				return 2.0;
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
			AIM_IMPROVEMENT = 1 * GetSkillAimFactor();
		}
		else
		{
			bool Visible = IsTargetVisible(GetCurrentTarget());
			float Distt = Math.Clamp(GetCurrentTarget().GetDistance(), 10, m_Utility.DCO_ConfComponent.GetAimMaxRangeEffect());
			float DistanceImprovementMultiplier = Math.Map(Distt, 10, m_Utility.DCO_ConfComponent.GetAimMaxRangeEffect(), m_Utility.DCO_ConfComponent.GetMaxAimImprovement(), 1);			
			
			m_fImprovementTimer += timeSliceMs;
			
			if (isNewTarget)
			{
				float random = Math.RandomFloat(1, 1.2);
				AIM_IMPROVEMENT = Math.Clamp(AIM_IMPROVEMENT - (AIM_IMPROVEMENT_DECAY * random), 0.5, 10);
			}
			
			if (m_fImprovementTimer > AIM_IMPROVEMENT_INTERVAL_S * 1000)
			{
				float random = Math.RandomFloat(1, 2);
				if (GetCurrentTarget().GetTimeSinceSeen() > TARGET_MAX_LAST_SEEN)
				{
					AIM_IMPROVEMENT = Math.Clamp(AIM_IMPROVEMENT - (AIM_IMPROVEMENT_DECAY_NO_TARGET * random), 0.5, 100);
				} else
				{
					float eqMul;
					float suppressionVal = Math.Map(m_Utility.m_ThreatSystem.GetSuppressionMeasure(), 0, 1, 0, AIM_SUPPRESSED_DECREMENT);
					float moraleVal = Math.Map(m_Utility.DCO_MoraleSystem.GetMoraleValue(), 0, 100, AIM_LOW_MORALE_DECREMENT, 0);				
					if (m_Utility.GetCharacterController().IsWeaponADS()) eqMul = AIM_IMPROVEMENT_EQUIPMENT;
					else eqMul = 0;
					
					float Improvement = ((AIM_IMPROVEMENT_BASE_IMPROVEMENT + eqMul) - (moraleVal + suppressionVal)) * random;
					if (Improvement > 0) Improvement *= DistanceImprovementMultiplier; 
					
					AIM_IMPROVEMENT = Math.Clamp((AIM_IMPROVEMENT + Improvement) * GetSkillAimFactor(), 0.5, 100);
				}
				
				
				m_fImprovementTimer = 0;
			}
			

			
			//SCR_AIDebugVisualization.VisualizeMessage(m_Utility.m_OwnerEntity, AIM_IMPROVEMENT.ToString(), EAIDebugCategory.COMBAT, 1.4, Color.White);	
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
	
	protected void CalculateCoverChances()
	{
		float coverMorale = Math.Map(m_Morale.GetMoraleValue(), 0, 100, 50, 0);
		float coverThreat = Math.Map(m_Utility.m_ThreatSystem.GetThreatMeasure(), 0, 1, 0, 50);
		float coverDefChances = TAKE_COVER_CHANCES_DEFAULT;
		
		switch(m_Utility.DCO_ConfComponent.GetSkillLevel())
		{
			case DCO_SKILL.CONSCPRIT:
			{
				coverDefChances *= 0.1;
				break;
			}
			case DCO_SKILL.GREEN:
			{
				coverDefChances *= 0.25;
				break;
			}
			case DCO_SKILL.REGULAR:
			{
				coverDefChances *= 0.4;
				break;
			}
			case DCO_SKILL.VETERAN:
			{
				coverDefChances *= 0.55;
				break;
			}
			case DCO_SKILL.CRACK:
			{
				coverDefChances *= 0.7;
				break;
			}
			case DCO_SKILL.ELITE:
			{
				coverDefChances *= 0.85;
				break;
			}
		}
		
		TAKE_COVER_CHANCES = Math.Clamp((coverThreat + coverThreat + coverDefChances) / 2,0,100);
	}
	
	float GetCoverChances()
	{
		return TAKE_COVER_CHANCES;
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
	
	float GetSkillAimFactor()
	{
		switch(m_Utility.DCO_ConfComponent.GetSkillLevel())
		{
			case DCO_SKILL.CONSCPRIT:
			{
				return 0.3;
				break;
			}
			case DCO_SKILL.GREEN:
			{
				return 0.7;
				break;
			}
			case DCO_SKILL.REGULAR:
			{
				return 1.0;
				break;
			}
			case DCO_SKILL.VETERAN:
			{
				return 1.4;
				break;
			}
			case DCO_SKILL.CRACK:
			{
				return 1.9;
				break;
			}
			case DCO_SKILL.ELITE:
			{
				return 2.5;
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
	
	SCR_InventoryStorageManagerComponent getInventoryStorageMan()
	{
		return m_InventoryManager;
	}
	
	SCR_CharacterControllerComponent getCharacterController()
	{
		return m_CharacterController;
	}
}