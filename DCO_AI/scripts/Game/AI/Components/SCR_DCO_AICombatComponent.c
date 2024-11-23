modded enum EAISkill
{
	RECRUIT	= 10,
	TRAINED	= 30,
};

modded class SCR_AICombatComponent : ScriptComponent
{
	protected SCR_AIGroup m_SCR_AIGroup;
	protected IEntity m_ControlledEntity;
	protected SCR_ChimeraAIAgent m_SCR_ChimeraAIAgent;
	protected DCO_AIMoraleSystem m_DCO_MoraleSystem;
	protected SCR_CharacterDamageManagerComponent damageManager;
	private DCO_SkillComponent m_DCO_Skill;
	private DCO_CUSTOMRANK rank;	
	
	protected SCR_CompartmentAccessComponent m_CompartmentAccessComponent;
	protected BaseWeaponManagerComponent m_WeaponManagerComponent;
	protected TurretComponent m_TurretComponent;
	
	protected IEntity m_MyEntity;
	
	DCO_GroupTactic m_Tac;
	DCO_GroupTacticComponent m_GroupTacticComponent;
	
	protected static const float ASSIGNED_TARGETS_SCORE_INCREMENT = 15.0;
	
	protected static const float ENDANGERING_TARGETS_SCORE_INCREMENT = 30.0;
	static const float			 ENDANGERING_TARGET_SCORE_MULTIPLIER = 1.5;

	protected static const float TARGET_MAX_LAST_SEEN_DIRECT_ATTACK = 1.3;
	protected static const float TARGET_MAX_LAST_SEEN_DIRECT_ATTACK_CLOSE = 4.5;
			  static const float TARGET_MIN_LAST_SEEN_INDIRECT_ATTACK = 2.0;
	          static const float TARGET_MAX_LAST_SEEN_INDIRECT_ATTACK = 7.0;
	          static const float TARGET_MAX_LAST_SEEN_INDIRECT_ATTACK_MG = 12.0;
	          static const float TARGET_MAX_LAST_SEEN_INDIRECT_ATTACK_CLOSE = 10.0;
	
	static const float TARGET_SCORE_HIGH_PRIORITY_ATTACK = 90.0;
	static const float TARGET_MAX_LAST_SEEN_VISIBLE = 0.8;
	
	static const float TARGET_MAX_LAST_SEEN = 8.0;
	
	protected const float PERCEPTION_FACTOR_SAFE = 0.3;
	protected const float PERCEPTION_FACTOR_VIGILANT = 3.0;
	protected const float PERCEPTION_FACTOR_ALERTED = 2.8; 
	protected const float PERCEPTION_FACTOR_THREATENED = 2.5;
	protected const float PERCEPTION_FACTOR_PINNED = 2.0;
	protected const float PERCEPTION_FACTOR_EXHAUSTED = 1.2;
	protected const float PERCEPTION_OVERALL_BASE = 0.25;
		
	protected static const float TARGET_MAX_DISTANCE_INFANTRY = 700.0;
	protected static const float TARGET_MAX_DISTANCE_VEHICLE = 1000.0;

	//! Beyond this distance AI considers combat as 'long range', used for danger events and firing times
	static const float LONG_RANGE_COMBAT_DISTANCE = 250.0;
	
	protected const float FRAG_GRENADE_MAX_THREAT = 3.2;
	
	private int groupNumber;
	private int nowGroupNumber;
	
	private float AimImprovement;
	
	private bool LOW_AMMO = false;
	bool selectedTargetChanged = false;
	
	override protected void EOnInit(IEntity owner)
	{		
		vanilla.EOnInit(owner);
		
		ChimeraCharacter character = ChimeraCharacter.Cast(owner);
		if (m_Agent)
		{
			m_ControlledEntity = m_Agent.GetControlledEntity();
			
			rank = m_DCO_Skill.GetCharacterRank(m_Utility.m_OwnerEntity);
			
			m_SCR_AIGroup = m_Utility.getMyGroup();
			
			damageManager = m_Utility.m_AIInfo.getCharDamageComp();
						
			m_DCO_Skill = m_Utility.m_DCO_Skill.GetCharacterSkillRankComponent(m_Utility.m_OwnerEntity);
			
			m_SCR_ChimeraAIAgent = m_Agent;
			
			m_GroupTacticComponent = DCO_GroupTacticComponent.Cast(owner.FindComponent(DCO_GroupTacticComponent));
			
			m_Tac = m_GroupTacticComponent.GetGroupTactic(owner);
			
			m_eAISkill = m_eAISkillDefault;
		}
	}
	
	override void Update(float timeSliceMs)
	{
		// Evaluate if we must dismount turret - only if we are already in turret
		if (m_CurrentTurretController)
			EvaluateDismountTurret(timeSliceMs);
		
		rank = m_DCO_Skill.GetCharacterRank(m_Utility.m_OwnerEntity);
	}
	
	int getGroupNumber()
	{
		return groupNumber;
	}

	override void UpdatePerceptionFactor(PerceptionComponent perceptionComp, SCR_AIThreatSystem threatSystem)
	{
		EAIThreatState threatState = threatSystem.GetState();
		
		float perceptionFactor;
		switch (threatState)
		{
			case EAIThreatState.SAFE:
			{
				switch (rank)
				{
					case DCO_CUSTOMRANK.RECRUIT:
					{
						perceptionFactor = PERCEPTION_FACTOR_SAFE; 
						break;
					}
					case DCO_CUSTOMRANK.PRIVATE:
					{
						perceptionFactor = PERCEPTION_FACTOR_SAFE * 1.3; 
						break;
					}
					case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
					{
						perceptionFactor = PERCEPTION_FACTOR_SAFE * 1.5;
						break;
					}
					case DCO_CUSTOMRANK.SPECIALIST:
					{
						perceptionFactor = PERCEPTION_FACTOR_SAFE * 1.8; 
						break;
					}
					default :
					{
						perceptionFactor = PERCEPTION_FACTOR_SAFE;
						break;
					}
				}
				break;
			}
			case EAIThreatState.VIGILANT:
			{
				switch (rank)
				{
					case DCO_CUSTOMRANK.RECRUIT:
					{
						perceptionFactor = PERCEPTION_FACTOR_VIGILANT; 
						break;
					}
					case DCO_CUSTOMRANK.PRIVATE:
					{
						perceptionFactor = PERCEPTION_FACTOR_VIGILANT * 1.3; 
						break;
					}
					case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
					{
						perceptionFactor = PERCEPTION_FACTOR_VIGILANT * 1.5;
						break;
					}
					case DCO_CUSTOMRANK.SPECIALIST:
					{
						perceptionFactor = PERCEPTION_FACTOR_VIGILANT * 1.8; 
						break;
					}
					default :
					{
						perceptionFactor = PERCEPTION_FACTOR_VIGILANT;
						break;
					}
				}
				break;
			}
			case EAIThreatState.ALERTED:
			{
				switch (rank)
				{
					case DCO_CUSTOMRANK.RECRUIT:
					{
						perceptionFactor = PERCEPTION_FACTOR_ALERTED; 
						break;
					}
					case DCO_CUSTOMRANK.PRIVATE:
					{
						perceptionFactor = PERCEPTION_FACTOR_ALERTED * 1.3; 
						break;
					}
					case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
					{
						perceptionFactor = PERCEPTION_FACTOR_ALERTED * 1.5;
						break;
					}
					case DCO_CUSTOMRANK.SPECIALIST:
					{
						perceptionFactor = PERCEPTION_FACTOR_ALERTED * 1.8; 
						break;
					}
					default :
					{
						perceptionFactor = PERCEPTION_FACTOR_ALERTED;
						break;
					}
				}
				break;
			}
			case EAIThreatState.THREATENED:
			{
				switch (rank)
				{
					case DCO_CUSTOMRANK.RECRUIT:
					{
						perceptionFactor = PERCEPTION_FACTOR_THREATENED; 
						break;
					}
					case DCO_CUSTOMRANK.PRIVATE:
					{
						perceptionFactor = PERCEPTION_FACTOR_THREATENED * 1.3; 
						break;
					}
					case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
					{
						perceptionFactor = PERCEPTION_FACTOR_THREATENED * 1.5;
						break;
					}
					case DCO_CUSTOMRANK.SPECIALIST:
					{
						perceptionFactor = PERCEPTION_FACTOR_THREATENED * 1.8; 
						break;
					}
					default :
					{
						perceptionFactor = PERCEPTION_FACTOR_THREATENED;
						break;
					}
				}
				break;
			}
			case EAIThreatState.PINNED:
			{
				switch (rank)
				{
					case DCO_CUSTOMRANK.RECRUIT:
					{
						perceptionFactor = PERCEPTION_FACTOR_PINNED; 
						break;
					}
					case DCO_CUSTOMRANK.PRIVATE:
					{
						perceptionFactor = PERCEPTION_FACTOR_PINNED * 1.3; 
						break;
					}
					case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
					{
						perceptionFactor = PERCEPTION_FACTOR_PINNED * 1.5;
						break;
					}
					case DCO_CUSTOMRANK.SPECIALIST:
					{
						perceptionFactor = PERCEPTION_FACTOR_PINNED * 1.8; 
						break;
					}
					default :
					{
						perceptionFactor = PERCEPTION_FACTOR_PINNED;
						break;
					}
				}
				break;
			}
			case EAIThreatState.EXHAUSTED:
			{
				switch (rank)
				{
					case DCO_CUSTOMRANK.RECRUIT:
					{
						perceptionFactor = PERCEPTION_FACTOR_EXHAUSTED; 
						break;
					}
					case DCO_CUSTOMRANK.PRIVATE:
					{
						perceptionFactor = PERCEPTION_FACTOR_EXHAUSTED * 1.3; 
						break;
					}
					case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
					{
						perceptionFactor = PERCEPTION_FACTOR_EXHAUSTED * 1.5;
						break;
					}
					case DCO_CUSTOMRANK.SPECIALIST:
					{
						perceptionFactor = PERCEPTION_FACTOR_EXHAUSTED * 1.8; 
						break;
					}
					default :
					{
						perceptionFactor = PERCEPTION_FACTOR_EXHAUSTED;
						break;
					}
				}
				break;
			}
		}
		
		switch (m_eAISkill)
		{
			case EAISkill.RECRUIT :
			{
				perceptionFactor += PERCEPTION_OVERALL_BASE / 4;
				break;
			}
			case EAISkill.ROOKIE :
			{
				perceptionFactor += PERCEPTION_OVERALL_BASE / 2;
				break;
			}
			case EAISkill.REGULAR :
			{
				perceptionFactor += PERCEPTION_OVERALL_BASE;
				break;
			}
			case EAISkill.TRAINED :
			{
				perceptionFactor += PERCEPTION_OVERALL_BASE * 1.2;
				break;
			}
			case EAISkill.VETERAN :
			{
				perceptionFactor += PERCEPTION_OVERALL_BASE * 1.4;
				break;
			}
			case EAISkill.EXPERT :
			{
				perceptionFactor += PERCEPTION_OVERALL_BASE * 1.6;
				break;
			}
			case EAISkill.CYLON :
			{
				perceptionFactor += PERCEPTION_OVERALL_BASE * 1.8;
				break;
			}
		}
		
		perceptionFactor *= m_fEquipmentPerceptionFactor;
		perceptionFactor *= m_fPerceptionFactor;
		
		#ifdef WORKBENCH
		SCR_AIDebugVisualization.VisualizeMessage(m_Utility.m_OwnerEntity, "Perception : " + perceptionFactor.ToString(), EAIDebugCategory.COMBAT, 1.4, Color.White);
		#endif
		
		perceptionComp.SetPerceptionFactor(perceptionFactor);
	}
	
	//------------------------------------------------------------------------------------------------
	protected static const float DISTANCE_MAX = 1500; 
	protected static const float DISTANCE_MIN = 0; // Minimal distance when movement is allowed
	private static const float NEAR_PROXIMITY = 10;
	
	protected const float m_StopDistance = 30 + Math.RandomFloat(0, 12); 
	// TODO: add possibility to get cover towards custom position
	//------------------------------------------------------------------------------------------------

	override void SetCombatType(EAICombatType combatType)
	{
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("SetCombatType: %1", typename.EnumToString(EAICombatType, combatType)));
		#endif
		
		switch (combatType)
		{
			case EAICombatType.NONE:
			{
				SetActionAllowed(EAICombatActions.HOLD_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_WHEN_FIRE,true);
				SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,true);
				SetActionAllowed(EAICombatActions.MOVEMENT_TO_LAST_SEEN,true);
				break;
			}
			case EAICombatType.NORMAL:
			{
				SetActionAllowed(EAICombatActions.HOLD_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_WHEN_FIRE,true);
				SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,true);
				SetActionAllowed(EAICombatActions.MOVEMENT_TO_LAST_SEEN,true);
				break;
			}
			case EAICombatType.SUPPRESSIVE:
			{
				SetActionAllowed(EAICombatActions.HOLD_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_WHEN_FIRE,true);
				SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,true);
				SetActionAllowed(EAICombatActions.MOVEMENT_TO_LAST_SEEN,false);
				break;
			}
			case EAICombatType.RETREAT:
			{
				SetActionAllowed(EAICombatActions.HOLD_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_WHEN_FIRE,true);
				SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,true);
				SetActionAllowed(EAICombatActions.MOVEMENT_TO_LAST_SEEN,false);
				break;
			}
			case EAICombatType.SINGLE_SHOT:
			{
				SetActionAllowed(EAICombatActions.HOLD_FIRE,false);
				SetActionAllowed(EAICombatActions.MOVEMENT_WHEN_FIRE,true);
				SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,true);
				SetActionAllowed(EAICombatActions.MOVEMENT_TO_LAST_SEEN,true);
				break;
			}
		}
		m_eCombatType = combatType;
	}
	
	protected override void EvaluateDismountTurret(float timeSliceMs)
	{
		vector targetPos;
		bool mustDismount = DismountTurretCondition(targetPos, false);
		bool outofMagDismount = turretOutOfMag();
		bool noGunner = noGunnerOut();
		
		if (noGunner)
		{
			if (m_fDismountTurretTimer == -1.0)
				return;
			
			m_fDismountTurretTimer += timeSliceMs;
			
			if (m_fDismountTurretTimer > DISMOUNT_TURRET_TIMER_MS)
			{
				m_fDismountTurretTimer = -1.0;
				
				TryAddDismountTurretActions(targetPos);
			}
		}
		else if (mustDismount || outofMagDismount)
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
		else
		{
			m_fDismountTurretTimer = 0;
		}
	}
	
	bool noGunnerOut()
	{
		if (!m_CurrentVehicle)
			return false;
		
		SCR_AIUtilityComponent m_DriverUtility;
		SCR_AIUtilityComponent m_GunnerUtility;
		Vehicle m_MyVehicle;
		
		if (m_ControlledEntity)
		{
			m_CompartmentAccessComponent = SCR_CompartmentAccessComponent.Cast(m_ControlledEntity.FindComponent(SCR_CompartmentAccessComponent));
			
			if (m_CompartmentAccessComponent && m_CompartmentAccessComponent.IsInCompartment())
			{
				IEntity turretEnt = m_CompartmentAccessComponent.GetCompartment().GetOwner();
				if (turretEnt)
				{
					TurretControllerComponent contr = TurretControllerComponent.Cast(turretEnt.FindComponent(TurretControllerComponent));
					if (contr)
						m_TurretComponent = contr.GetTurretComponent();
					m_WeaponManagerComponent = BaseWeaponManagerComponent.Cast(turretEnt.FindComponent(BaseWeaponManagerComponent));
				}	
				m_MyVehicle = Vehicle.Cast(m_CompartmentAccessComponent.GetVehicle());	
			}	
		}
		
		if (!m_MyVehicle) return false;
		
		IEntity driverEntity = m_MyVehicle.GetPilot();
		if (!driverEntity)
			return false;
		AIControlComponent controlComp = AIControlComponent.Cast(driverEntity.FindComponent(AIControlComponent));
		if (!controlComp)
			return false;
		AIAgent driverAgent = controlComp.GetAIAgent();
		if (!driverAgent)
			return false;
		m_DriverUtility = SCR_AIUtilityComponent.Cast(driverAgent.FindComponent(SCR_AIUtilityComponent));
		if (!m_DriverUtility)
			return false;
		
		if (m_GunnerUtility.m_AIInfo.HasUnitState(EUnitState.UNCONSCIOUS) || m_GunnerUtility.m_AIInfo.getCharDamageComp().IsDestroyed())
			return true;
		
		return false;
	}
	
	bool turretOutOfMag()
	{
		// False if not in turret
		if (!m_CurrentTurretController)
			return false;
		TurretComponent turretComp = m_CurrentTurretController.GetTurretComponent();
		if (!turretComp)
			return false;
		
		// False if we have a driver in the vehicle
		array<BaseCompartmentSlot> compartments = {};
		m_CurrentVehicleCompartmentManager.GetCompartments(compartments);
		foreach (BaseCompartmentSlot slot : compartments)
		{
			if (PilotCompartmentSlot.Cast(slot) && slot.GetOccupant())
				return false;
			
			if (TurretCompartmentSlot.Cast(slot))
			{
				IEntity turrets = slot.GetOccupant();
				if (turrets)
				{
					DamageManagerComponent dmg = DamageManagerComponent.Cast(turrets.FindComponent(DamageManagerComponent));
					if (dmg.IsDestroyed())
						return true;
				} if(!turrets) return true;
			}
		}
		
		// False if we are in a vehicle and we should not leave turret of this vehicle type
		// Note that static turrets are not of Vehicle class.
		//Vehicle vehicle = Vehicle.Cast(m_CurrentVehicle);
		//if (vehicle && s_aForbidDismountTurretsOfVehicleTypes.Find(vehicle.m_eVehicleType) != -1)
		//	return false;
		
		return m_CurrentTurretController.GetWeaponManager().GetCurrentWeapon().GetCurrentMagazine().GetAmmoCount() == 0;
	}
	
	float improvementCalcuation()
	{
		if (!m_SelectedTarget && selectedTargetChanged)
			return 0;
		
		vector targetPosition = m_SelectedTarget.GetTargetEntity().GetOrigin();

		return 0;
	}
	
	bool lowAmmo()
	{
		return LOW_AMMO;
	}
	
	ECharacterStance getCharacterStance()
	{
		return m_AIInfo.GetStance();
	}
	
	int getTargetCount()
	{
		if(m_aAssignedTargets.IsEmpty())
			return 0;
		else
			return m_aAssignedTargets.Count();
	}
	
	void improvement(float improvement)
	{
		AimImprovement = improvement;
	}
	
	float getImprovement()
	{		
		return AimImprovement;
	}	
	
	DCO_GroupTactic getTactics()
	{
		return m_Tac;
	}
	
	TurretControllerComponent GetTurretComponent()
	{
		return m_CurrentTurretController;
	}
	
	static EAISkill setSkill(IEntity unit, EAISkill skills)
	{
		if (!unit)
			return EAISkill.REGULAR;
		
		SCR_AICombatComponent comp = GetCharacterSkillsComponent(unit);
		
		if (!comp)
			return EAISkill.REGULAR;
		
		return comp.SetCharacterRank(skills);
	}

	static EAISkill GetCharacterRank(IEntity unit)
	{
		if (!unit)
			return EAISkill.REGULAR;
		
		SCR_AICombatComponent comp = GetCharacterSkillsComponent(unit);
		
		if (!comp)
			return EAISkill.REGULAR;
		
		return comp.GetCharacterRank();
	}
	
	static SCR_AICombatComponent GetCharacterSkillsComponent(IEntity unit)
	{
		return SCR_AICombatComponent.Cast(unit.FindComponent(SCR_AICombatComponent));
	}

	protected EAISkill SetCharacterRank(EAISkill skills)
	{
		m_eAISkill = skills;
		
		return skills;
	}
	
	protected EAISkill GetCharacterRank()
	{
		return m_eAISkill;
	}
	
	void SCR_AICombatComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_MyEntity = ent;
	}
};