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
	protected DCO_AIInfoComponent m_DCO_AIInfoComponent;
	
	protected static const float ASSIGNED_TARGETS_SCORE_INCREMENT = 15.0;
	protected static const float ENDANGERING_TARGETS_SCORE_INCREMENT = 30.0;
	static const float			 ENDANGERING_TARGET_SCORE_MULTIPLIER = 2.0;

	protected static const float TARGET_MAX_LAST_SEEN_DIRECT_ATTACK = 1.0;
			  static const float TARGET_MAX_LAST_SEEN_INDIRECT_ATTACK = 5.0;
			  static const float TARGET_MAX_LAST_SEEN_INDIRECT_ATTACK_MG = 10.0;
			  static const float TARGET_MAX_LAST_SEEN = 60.0;
	
	static const float TARGET_SCORE_HIGH_PRIORITY_ATTACK = 98.0;
	static const float TARGET_MAX_LAST_SEEN_VISIBLE = 0.8;
	protected static const float TARGET_MIN_INDIRECT_TRACE_FRACTION_MIN = 0.48;
	
	protected const float PERCEPTION_FACTOR_SAFE = 1.2;
	protected const float PERCEPTION_FACTOR_VIGILANT = 7.0;
	protected const float PERCEPTION_FACTOR_ALERTED = 6.5; 
	protected const float PERCEPTION_FACTOR_THREATENED = 5.5;
	protected const float PERCEPTION_FACTOR_PINNED = 5.0;
	protected const float PERCEPTION_FACTOR_EXHAUSTED = 4.5;

	protected const float PERCEPTION_FACTOR_EQUIPMENT_BINOCULARS = 2.5;
	protected const float PERCEPTION_FACTOR_EQUIPMENT_NONE = 1.0;
	
	static const float LONG_RANGE_FIRE_DISTANCE = 200.0;
	
	protected const float DISMOUNT_TURRET_TIMER_MS = 1500;
	protected static const float TURRET_TARGET_EXCESS_ANGLE_THRESHOLD_DEG = 5.0;
	
	private bool LOW_AMMO = false;
	
	override protected void EOnInit(IEntity owner)
	{
		GetAiAgent();
		
		super.EOnInit(owner);
		
		if (m_Agent)
		{
			m_ControlledEntity = m_Agent.GetControlledEntity();
			
			m_SCR_ChimeraAIAgent = SCR_ChimeraAIAgent.Cast(m_Agent);
			
			m_DCO_AIInfoComponent = DCO_AIInfoComponent.Cast(m_Agent.FindComponent(DCO_AIInfoComponent));
		}
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
			case EAIThreatState.PINNED:
				perceptionFactor = PERCEPTION_FACTOR_PINNED; break;
			case EAIThreatState.EXHAUSTED:
				perceptionFactor = PERCEPTION_FACTOR_EXHAUSTED; break;
		}
		
		perceptionFactor *= m_fEquipmentPerceptionFactor;
		
		perceptionComp.SetPerceptionFactor(perceptionFactor);
	}
	
	//------------------------------------------------------------------------------------------------
	protected static const float DISTANCE_MAX = 500; 
	protected static const float DISTANCE_MIN = 5; // Minimal distance when movement is allowed
	private static const float NEAR_PROXIMITY = 10;
	// TODO: add possibility to get cover towards custom position
	//------------------------------------------------------------------------------------------------
	override vector FindNextCoverPosition()
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
	
	override void SetCombatType(EAICombatType combatType)
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
				SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,true);
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
				SetActionAllowed(EAICombatActions.SUPPRESSIVE_FIRE,true);
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
		
		int lowMagThreshold = 1;
		
		// Decide how many remainiing magazines is enough to complain
		switch (weaponComp.GetWeaponType())
		{
			case EWeaponType.WT_RIFLE: lowMagThreshold = 1; break;
			case EWeaponType.WT_GRENADELAUNCHER: lowMagThreshold = 2; break; // todo now it won't work when we are out of UGL ammo because weapons are not marked with WT_GRENADELAUNCHER
			case EWeaponType.WT_SNIPERRIFLE: lowMagThreshold = 1; break;
			case EWeaponType.WT_ROCKETLAUNCHER: lowMagThreshold = 1; break;
			case EWeaponType.WT_MACHINEGUN: lowMagThreshold = 1; break;
			case EWeaponType.WT_HANDGUN: lowMagThreshold = 1; break;
			default: lowMagThreshold = 1;
		}
		
		if( magCount < lowMagThreshold )
		{
			LOW_AMMO = true;
			return true;
		}
		
		LOW_AMMO = false;
		
		return false;
	}
	
	override protected void Event_OnDamageOverTimeAdded(EDamageType dType, float dps, HitZone hz)
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
	
	override bool DismountTurretCondition(inout vector targetPos, bool targetPosProvided)
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
};