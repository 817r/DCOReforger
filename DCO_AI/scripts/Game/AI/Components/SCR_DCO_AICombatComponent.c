modded enum EAISkill
{
	RECRUIT	= 10,
	TRAINED	= 30,
};

modded class SCR_AICombatComponent : ScriptComponent
{
	protected DCO_Group_Info m_SCR_AIGroup;
	protected IEntity m_ControlledEntity;
	protected SCR_ChimeraAIAgent m_SCR_ChimeraAIAgent;
	protected DCO_AIMoraleSystem m_DCO_MoraleSystem;
	protected DCO_AIInfoComponent m_DCO_AIInfoComponent;
	protected SCR_CharacterDamageManagerComponent damageManager;
	private DCO_SkillComponent m_DCO_Skill;
	private DCO_CUSTOMRANK rank;	
	
	DCO_GroupTactic m_Tac;
	DCO_GroupTacticComponent m_GroupTacticComponent;
	
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
	
	protected const float PERCEPTION_FACTOR_SAFE = 0.5;
	protected const float PERCEPTION_FACTOR_VIGILANT = 3.0;
	protected const float PERCEPTION_FACTOR_ALERTED = 2.8; 
	protected const float PERCEPTION_FACTOR_THREATENED = 2.5;
	protected const float PERCEPTION_FACTOR_PINNED = 1.2;
	protected const float PERCEPTION_FACTOR_EXHAUSTED = 1.0;

	protected const float PERCEPTION_FACTOR_EQUIPMENT_BINOCULARS = 2.5;
	protected const float PERCEPTION_FACTOR_EQUIPMENT_NONE = 1.0;
	
	static const float LONG_RANGE_FIRE_DISTANCE = 200.0;
	
	protected const float DISMOUNT_TURRET_TIMER_MS = 1000;
	protected static const float TURRET_TARGET_EXCESS_ANGLE_THRESHOLD_DEG = 4.0;
	
	private int groupNumber;
	private int nowGroupNumber;
	bool alreadyGetMemberCount = false;
	
	private float AimImprovement;
	
	private bool LOW_AMMO = false;
	bool selectedTargetChanged = false;
	
	override protected void EOnInit(IEntity owner)
	{		
		super.EOnInit(owner);
		GetAiAgent();
		
		if (m_Agent)
		{
			m_ControlledEntity = m_Agent.GetControlledEntity();
			
			rank = m_DCO_Skill.GetCharacterRank(m_Utility.m_OwnerEntity);
			
			damageManager = SCR_CharacterDamageManagerComponent.Cast(m_ControlledEntity.FindComponent(SCR_CharacterDamageManagerComponent));
			
			m_SCR_AIGroup = m_Utility.m_DCO_GroupInfo;
						
			m_DCO_Skill = m_Utility.m_DCO_Skill.GetCharacterSkillRankComponent(m_Utility.m_OwnerEntity);
			
			m_SCR_ChimeraAIAgent = m_Agent;
			
			m_DCO_AIInfoComponent = DCO_AIInfoComponent.Cast(m_Agent.FindComponent(DCO_AIInfoComponent));
			
			if (m_DCO_AIInfoComponent)
			{
				groupNumber = m_DCO_AIInfoComponent.getMemberNumber();
			}
			
			m_GroupTacticComponent = DCO_GroupTacticComponent.Cast(owner.FindComponent(DCO_GroupTacticComponent));
			
			m_Tac = m_GroupTacticComponent.GetGroupTactic(owner);
		}
	}
	
	override void Update(float timeSliceMs)
	{
		// Evaluate if we must dismount turret - only if we are already in turret
		if (m_CurrentTurretController)
			EvaluateDismountTurret(timeSliceMs);
				
		#ifdef WORKBENCH
		SCR_AIDebugVisualization.VisualizeMessage(m_Utility.m_OwnerEntity, "Group Number Init : " + groupNumber.ToString() + ", Group Number Now : " + nowGroupNumber.ToString(), EAIDebugCategory.COMBAT, 1.4, Color.White);
		#endif
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
						perceptionFactor = PERCEPTION_FACTOR_SAFE * 1.2; 
						break;
					}
					case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
					{
						perceptionFactor = PERCEPTION_FACTOR_SAFE * 1.4;
						break;
					}
					case DCO_CUSTOMRANK.SPECIALIST:
					{
						perceptionFactor = PERCEPTION_FACTOR_SAFE * 1.6; 
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
						perceptionFactor = PERCEPTION_FACTOR_VIGILANT * 1.2; 
						break;
					}
					case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
					{
						perceptionFactor = PERCEPTION_FACTOR_VIGILANT * 1.45;
						break;
					}
					case DCO_CUSTOMRANK.SPECIALIST:
					{
						perceptionFactor = PERCEPTION_FACTOR_VIGILANT * 1.7; 
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
						perceptionFactor = PERCEPTION_FACTOR_ALERTED * 1.2; 
						break;
					}
					case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
					{
						perceptionFactor = PERCEPTION_FACTOR_ALERTED * 1.45;
						break;
					}
					case DCO_CUSTOMRANK.SPECIALIST:
					{
						perceptionFactor = PERCEPTION_FACTOR_ALERTED * 1.7; 
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
						perceptionFactor = PERCEPTION_FACTOR_THREATENED * 1.2; 
						break;
					}
					case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
					{
						perceptionFactor = PERCEPTION_FACTOR_THREATENED * 1.45;
						break;
					}
					case DCO_CUSTOMRANK.SPECIALIST:
					{
						perceptionFactor = PERCEPTION_FACTOR_THREATENED * 1.7; 
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
						perceptionFactor = PERCEPTION_FACTOR_PINNED * 1.2; 
						break;
					}
					case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
					{
						perceptionFactor = PERCEPTION_FACTOR_PINNED * 1.45;
						break;
					}
					case DCO_CUSTOMRANK.SPECIALIST:
					{
						perceptionFactor = PERCEPTION_FACTOR_PINNED * 1.7; 
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
						perceptionFactor = PERCEPTION_FACTOR_EXHAUSTED * 1.2; 
						break;
					}
					case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
					{
						perceptionFactor = PERCEPTION_FACTOR_EXHAUSTED * 1.45;
						break;
					}
					case DCO_CUSTOMRANK.SPECIALIST:
					{
						perceptionFactor = PERCEPTION_FACTOR_EXHAUSTED * 1.7; 
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
		
		perceptionFactor *= m_fEquipmentPerceptionFactor;
		
		perceptionComp.SetPerceptionFactor(perceptionFactor);
	}
	
	//------------------------------------------------------------------------------------------------
	protected static const float DISTANCE_MAX = 1500; 
	protected static const float DISTANCE_MIN = 0; // Minimal distance when movement is allowed
	private static const float NEAR_PROXIMITY = 10;
	
	protected const float m_StopDistance = 30 + Math.RandomFloat(0, 12); 
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
	
	float getCurrentHealth()
	{
		return damageManager.GetHealth();
	}
	
	float GetMaxHealth()
	{
		return damageManager.GetMaxHealth();
	}
	
	void improvement(float improvement)
	{
		AimImprovement = improvement;
	}
	
	float getImprovement()
	{		
		return AimImprovement;
	}
	
	float resetImprovement()
	{
		AimImprovement = 0;
		
		return 0;
	}
};