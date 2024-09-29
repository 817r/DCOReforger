modded class SCR_AIGetAimErrorOffset: AITaskScripted
{
	static const string PORT_ERROR_OFFSET = "ErrorOffset";
	static const string PORT_BASE_TARGET = "BaseTargetIn";
	static const string PORT_AIM_POINT = "AimPoint";
	static const string PORT_TOLERANCE = "AimingTolerance";
	static const float CLOSE_RANGE_THRESHOLD = 10.0;
	static const float MEDIUM_RANGE_THRESHOLD = 95.0;
	static const float LONG_RANGE_THRESHOLD = 200.0;
	
	static const float AIMING_ERROR_SCALE = 1.0; // TODO: game master and server option
	static const float AIMING_ERROR_FACTOR_MIN = 0.2; 
	static const float AIMING_ERROR_CLOSE_RANGE_FACTOR_MIN = 0.02;
	static const float AIMING_ERROR_FACTOR_MAX = 1.2;
	
	static const float MAXIMAL_TOLERANCE = 8.5;	
	static const float MINIMAL_TOLERANCE = 0.002;

	DCO_CUSTOMRANK ranks;
	float threatFactor;
	SCR_AIInfoComponent m_InfoComponent;
	CharacterControllerComponent m_char;
	
	override void OnInit(AIAgent owner)
	{
		IEntity ent = owner.GetControlledEntity();
		if (ent)
			m_CombatComponent = SCR_AICombatComponent.Cast(ent.FindComponent(SCR_AICombatComponent));		
		m_InfoComponent = SCR_AIInfoComponent.Cast(owner.FindComponent(SCR_AIInfoComponent));
		m_char = m_InfoComponent.getCharCont();
		ranks = DCO_SkillComponent.GetCharacterRank(ent);
	}
	
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
    {
		//if (!m_CombatComponent || !m_InfoComponent)
		if (!m_CombatComponent)
			return ENodeResult.FAIL;
		
		IEntity entity = owner.GetControlledEntity();
		if (!entity)
			return ENodeResult.FAIL;
		
		if (!m_InfoComponent)
			return ENodeResult.FAIL;
		
#ifdef AI_DEBUG
		m_aDebugShapes.Clear();
		
#endif
		
		BaseTarget target;
		GetVariableIn(PORT_BASE_TARGET, target);
		
		// Bail if target is invalid
		if (!target)
		{
			ClearPorts();
			return ENodeResult.FAIL;
		}
		
		// Bail if target is invalid
		IEntity targetEntity = target.GetTargetEntity();
		if (!targetEntity)
		{
			ClearPorts();
			return ENodeResult.FAIL;
		}
		
		// Resolve which aimpoint types to use
		EAimPointType aimpointTypes[3];
		EAimPointType aimpointType0, aimpointType1;
		if (!GetVariableIn(PORT_AIMPOINT_TYPE_0, aimpointType0))
			aimpointType0 = -1;
		if (!GetVariableIn(PORT_AIMPOINT_TYPE_1, aimpointType1))
			aimpointType1 = -1;
		aimpointTypes[0] = aimpointType0;
		aimpointTypes[1] = aimpointType1;
		aimpointTypes[2] = m_eAimPointType;
		
		// Try to find aimpoint
		AimPoint aimPoint = GetAimPoint(target, aimpointTypes);
		
		// Bail if aimpoint was not found
		if (!aimPoint)
		{
			ClearPorts();
			return ENodeResult.FAIL;
		}
		
		EWeaponType weaponType = m_CombatComponent.GetCurrentWeaponType();
		ECharacterStance stances = m_CombatComponent.getCharacterStance();
		

#ifdef AI_DEBUG
		if (DiagMenu.GetBool(SCR_DebugMenuID.DEBUGUI_AI_SHOW_TARGET_AIMPOINT))
			m_aDebugShapes.Insert(Shape.CreateSphere(COLOR_YELLOW_A, ShapeFlags.NOZBUFFER | ShapeFlags.TRANSP, aimPoint.GetPosition(),aimPoint.GetDimension()));
#endif
		
		// Calculate angular bounds
		vector offsetX, offsetY;
		float angularSize, distance, tolerance;		
		GetTargetAngularBounds(entity, aimPoint, offsetX, offsetY, angularSize, distance);

		// Correct aim point size based on factors
		float distanceFactor = GetDistanceFactor(distance);
		float offsetWeaponFactor = GetOffsetWeaponTypeFactor(weaponType);
		float illuminationFactor = GetTargetIlluminationFactor(target);	
					
		
		EAISkill currentSkill = m_CombatComponent.GetAISkill();
		EAIThreatState currentThreat = m_InfoComponent.GetThreatState();
		float threats = GetSkillFromThreat(currentSkill, currentThreat) / 20;
		
		offsetX = GetRandomFactor(currentSkill, 0.2) * offsetX * AIMING_ERROR_SCALE * distanceFactor * offsetWeaponFactor * illuminationFactor;
		offsetY = GetRandomFactor(currentSkill, 0.2) * offsetY * AIMING_ERROR_SCALE * distanceFactor * offsetWeaponFactor * illuminationFactor;
		
		tolerance = GetTolerances(entity, targetEntity, angularSize, distance, weaponType, stances);
		
		tolerance = Math.Clamp(tolerance + threats, MINIMAL_TOLERANCE, MAXIMAL_TOLERANCE);
		
		SetVariableOut(PORT_ERROR_OFFSET, offsetX + offsetY);
		SetVariableOut(PORT_AIM_POINT, aimPoint);
		SetVariableOut(PORT_TOLERANCE, tolerance);
		
#ifdef WORKBENCH
		// PrintFormat("Target size - used in tolerance: %1 target aimpointPosition: %2", distance * Math.Tan(tolerance * Math.DEG2RAD), aimPoint.GetPosition());
#endif
		
		return ENodeResult.SUCCESS;
	}
	
	override float GetDistanceFactor(float distance)
	{
		if (distance < CLOSE_RANGE_THRESHOLD)
			return Math.Map(distance, 0, CLOSE_RANGE_THRESHOLD, AIMING_ERROR_CLOSE_RANGE_FACTOR_MIN, AIMING_ERROR_FACTOR_MIN);

		float distanceCl = Math.Clamp((distance - CLOSE_RANGE_THRESHOLD) / LONG_RANGE_THRESHOLD, 0, 1);
		return Math.Lerp(AIMING_ERROR_FACTOR_MIN, AIMING_ERROR_FACTOR_MAX, distanceCl);
	}
	
	override float GetTargetIlluminationFactor(BaseTarget tgt)
	{
		PerceivableComponent perceivable = tgt.GetPerceivableComponent();
		if (!perceivable)
			return 1.0;
		
		if (perceivable.GetIlluminationFactor() < 0.7)
			return 1.5;
		
		else if (perceivable.GetIlluminationFactor() < 0.5)
			return 2.3;
		
		else if (perceivable.GetIlluminationFactor() < 0.3)
			return 2.7;
		
		return 1.0;
	} 
	
	float GetTolerances(IEntity observer, IEntity target, float angularSize, float distance, EWeaponType weaponType, ECharacterStance stance)
	{
		float tolerance;
		bool setMaxTolerance;
		float maxTOl;
		float minTOl;
		float ADSFactor;
		float minTOlEx = MINIMAL_TOLERANCE;
		float maxTOlEx = MAXIMAL_TOLERANCE;
	
		// Always use max tolerance in close range
		if (distance > LONG_RANGE_THRESHOLD)
		{
			switch(weaponType)
			{
				case EWeaponType.WT_RIFLE:
				{
					maxTOl = MAXIMAL_TOLERANCE;
					minTOl = MINIMAL_TOLERANCE;
					break;
				}
				case EWeaponType.WT_MACHINEGUN:
				{
					maxTOl = MAXIMAL_TOLERANCE * 2;
					minTOl = MINIMAL_TOLERANCE;
					break;
				}
				case EWeaponType.WT_ROCKETLAUNCHER:
				{
					maxTOl = MAXIMAL_TOLERANCE * 2;
					minTOl = MINIMAL_TOLERANCE * 5;
					break;
				}
				case EWeaponType.WT_SNIPERRIFLE:
				{
					maxTOl = MAXIMAL_TOLERANCE / 10;
					minTOl = MINIMAL_TOLERANCE;
					break;
				}
				case EWeaponType.WT_GRENADELAUNCHER:
				{
					maxTOl = MAXIMAL_TOLERANCE * 1.1;
					minTOl = MINIMAL_TOLERANCE * 55;
					break;
				}
				default:
				{
					maxTOl = MAXIMAL_TOLERANCE;
					minTOl = MINIMAL_TOLERANCE;
					break;
				}
			}
		}
		
		else if (distance > MEDIUM_RANGE_THRESHOLD)
		{
			switch(weaponType)
			{
				case EWeaponType.WT_RIFLE:
				{
					maxTOl = MAXIMAL_TOLERANCE;
					minTOl = MINIMAL_TOLERANCE;
					break;
				}
				case EWeaponType.WT_MACHINEGUN:
				{
					maxTOl = MAXIMAL_TOLERANCE * 1.5;
					minTOl = MINIMAL_TOLERANCE;
					break;
				}
				case EWeaponType.WT_ROCKETLAUNCHER:
				{
					maxTOl = MAXIMAL_TOLERANCE;
					minTOl = MINIMAL_TOLERANCE;
					break;
				}
				case EWeaponType.WT_SNIPERRIFLE:
				{
					maxTOl = MAXIMAL_TOLERANCE / 10;
					minTOl = MINIMAL_TOLERANCE;
					break;
				}
				case EWeaponType.WT_GRENADELAUNCHER:
				{
					maxTOl = MAXIMAL_TOLERANCE * 1.1;
					minTOl = MINIMAL_TOLERANCE * 55;
					break;
				}
				default:
				{
					maxTOl = MAXIMAL_TOLERANCE;
					minTOl = MINIMAL_TOLERANCE;
					break;
				}
			}
		}
		
		else if (distance > CLOSE_RANGE_THRESHOLD)
		{
			switch(weaponType)
			{
				case EWeaponType.WT_RIFLE:
				{
					maxTOl = MAXIMAL_TOLERANCE * 1;
					minTOl = MINIMAL_TOLERANCE;
					break;
				}
				case EWeaponType.WT_MACHINEGUN:
				{
					maxTOl = MAXIMAL_TOLERANCE * 1;
					minTOl = MINIMAL_TOLERANCE ;
					break;
				}
				case EWeaponType.WT_ROCKETLAUNCHER:
				{
					maxTOl = MAXIMAL_TOLERANCE * 1;
					minTOl = MINIMAL_TOLERANCE;
					break;
				}
				case EWeaponType.WT_SNIPERRIFLE:
				{
					maxTOl = MAXIMAL_TOLERANCE / 10;
					minTOl = MINIMAL_TOLERANCE;
					break;
				}
				case EWeaponType.WT_GRENADELAUNCHER:
				{
					maxTOl = MAXIMAL_TOLERANCE * 1.1;
					minTOl = MINIMAL_TOLERANCE * 45;
					break;
				}
				default:
				{
					maxTOl = MAXIMAL_TOLERANCE;
					minTOl = MINIMAL_TOLERANCE;
					break;
				}
			}
		}
		
		else if (distance < CLOSE_RANGE_THRESHOLD)
		{
			switch(weaponType)
			{
				case EWeaponType.WT_GRENADELAUNCHER:
				{
					maxTOl = MAXIMAL_TOLERANCE * 1.1;
					minTOl = MINIMAL_TOLERANCE * 12;
					break;
				}
				default:
				{
					if (ranks == DCO_CUSTOMRANK.RECRUIT)
					{
						maxTOl = MAXIMAL_TOLERANCE;
						minTOl = MINIMAL_TOLERANCE;
					}
					else if (ranks == DCO_CUSTOMRANK.PRIVATE)
					{
						maxTOl = MAXIMAL_TOLERANCE / 2;
						minTOl = MINIMAL_TOLERANCE;
					}
					else
					{
						maxTOl = MAXIMAL_TOLERANCE / 6;
						minTOl = MINIMAL_TOLERANCE;
					}
					break;
				}
			}
		}
			
		tolerance = angularSize / 2; // half of the size
		// angular speed
		tolerance *= GetAngularSpeedFactor(observer, target, setMaxTolerance);
				
		if (setMaxTolerance)
			return getMaximumTolerance(ranks) - getADSFactor();
		else 
		{
			// weapon type tolerance modifier
			if(m_CombatComponent.GetCurrentWeapon())
				ADSFactor = getADSFactor();
			
			tolerance = (tolerance * (GetWeaponTypeFactor(weaponType) + GetStanceTypeFactor(stance) + GetHealthTypeFactor())) - (GetAimImprovement() + ADSFactor);
		};
		
		switch(ranks)
		{
			case DCO_CUSTOMRANK.RECRUIT:
			{
				minTOlEx = minTOl * 15;
				maxTOlEx = maxTOl * 1.5;
				break;
			}
			case DCO_CUSTOMRANK.PRIVATE:
			{
				minTOlEx = minTOl;
				maxTOlEx = maxTOl;
				break;
			}
			case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
			{
				minTOlEx = minTOl;
				maxTOlEx = maxTOl - 1;
				break;
			}
			case DCO_CUSTOMRANK.SPECIALIST:
			{
				minTOlEx = minTOl;
				maxTOlEx = maxTOl / 10;
				break;
			}
		}
		
		tolerance = Math.Clamp(tolerance, minTOlEx, maxTOlEx);
		
		return tolerance;
	}	
	
	float getMaximumTolerance(DCO_CUSTOMRANK ranking)
	{
		switch(ranking)
		{
			case DCO_CUSTOMRANK.RECRUIT:
			{
				return MAXIMAL_TOLERANCE * 2;
				break;
			}
			case DCO_CUSTOMRANK.PRIVATE:
			{
				return MAXIMAL_TOLERANCE;
				break;
			}
			case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
			{
				return MAXIMAL_TOLERANCE / 1.2;
				break;
			}
			case DCO_CUSTOMRANK.SPECIALIST:
			{
				return MAXIMAL_TOLERANCE / 10;
				break;
			}
		}
		
		return MAXIMAL_TOLERANCE;
	}
	
	override float GetAngularSpeedFactor(IEntity observer, IEntity enemy, out bool setBigTolerance)
	{
		IEntity parent = enemy.GetParent(); // getting the vehicle for character inside vehicle
		if (parent)
		{
			enemy = parent;					// case of driver
			parent = enemy.GetParent();
			if (parent)						// case of turret
				enemy = parent;
		}
		Physics ph = enemy.GetPhysics();
		if (ph)
		{
			vector positionVector = enemy.GetOrigin() - observer.GetOrigin();
			vector angularVelocity = positionVector * ph.GetVelocity() / positionVector.LengthSq();  // omega = (r x v) / ||r||^2 
			float angularSpeed = angularVelocity.Length();			
			
			if (angularSpeed < 0.07) // rougly 4 degs in radians
				return 1.0;
			else if (angularSpeed < 0.17) // roughly 10 degs in radians
				return 2;
		}	
		setBigTolerance = true;
		return 0;
	}
	
	//------------------------------------------------------------------------------------------------
	// returns random factor based on AI skill
	override float GetRandomFactor(EAISkill skill,float mu)
	{
		float sigma;
		switch (skill)
		{
			case EAISkill.RECRUIT :
			{
				sigma = 2.0;
				break;
			}
			case EAISkill.ROOKIE :
			{
				sigma = 1.4;
				break;
			}
			case EAISkill.REGULAR :
			{
				sigma = 1.0;
				break;
			}
			case EAISkill.TRAINED :
			{
				sigma = 0.8;
				break;
			}
			case EAISkill.VETERAN :
			{
				sigma = 0.5;
				break;
			}
			case EAISkill.EXPERT :
			{
				sigma = 0.3;
				break;
			}
			case EAISkill.CYLON :
			{
				return 0.2;
			}
		}
		
		return Math.RandomGaussFloat(sigma,mu);
	}
	
	//------------------------------------------------------------------------------------------------
	// returns skill corrected by current threat level and if AI can shoot under such suppression
	float GetSkillFromThreat(EAISkill inSkill, EAIThreatState threat)
	{
		float sigma;
		
		switch (threat)
		{
			case EAIThreatState.EXHAUSTED :
			{
				switch (inSkill)
				{
					case EAISkill.ROOKIE :
					{
						return 2;
					}
					case EAISkill.REGULAR :
					{
						return 1.5;
					}
					case EAISkill.VETERAN :
					{
						return 1.3;
					}
					case EAISkill.EXPERT :
					{
						return 1;
					}
					case EAISkill.CYLON :
					{
						return 0.5;
					}
				};
				break;
			}
			case EAIThreatState.PINNED :
			{
				switch (inSkill)
				{
					case EAISkill.ROOKIE :
					{
						return 1.5;
					}
					case EAISkill.REGULAR :
					{
						return 1.3;
					}
					case EAISkill.VETERAN :
					{
						return 1;
					}
					case EAISkill.EXPERT :
					{
						return 0.6;
					}
					case EAISkill.CYLON :
					{
						return 0.3;
					}
				};
				break;
			}
			case EAIThreatState.THREATENED : 
			{		 
				switch (inSkill)
				{
					case EAISkill.ROOKIE :
					{
						return 1.1;
					}
					case EAISkill.REGULAR :
					{
						return 0.9;
					}
					case EAISkill.VETERAN :
					{
						return 0.7;
					}
					case EAISkill.EXPERT :
					{
						return 0.5;
					}
					case EAISkill.CYLON :
					{
						return 0.3;
					}
				};
				break;
			}
			case EAIThreatState.ALERTED :
			{
				switch (inSkill)
				{
					case EAISkill.ROOKIE :
					{
						return 1;
					}
					case EAISkill.REGULAR :
					{
						return 0.8;
					}
					case EAISkill.VETERAN :
					{
						return 0.6;
					}
					case EAISkill.EXPERT :
					{
						return 0.4;
					}
					case EAISkill.CYLON :
					{
						return 0.2;
					}
				};
				break;
			}
			default :
			{
				return 0;
				break;
			}	
		}	
		
		return 0;
	}
	
	override float GetWeaponTypeFactor(EWeaponType weaponType)
	{
		switch(weaponType)
		{
			case EWeaponType.WT_RIFLE:
			{
				return 1;
			}
			case EWeaponType.WT_MACHINEGUN:
			{
				return 2.5;
			}
			case EWeaponType.WT_HANDGUN:
			{
				return 1;
			}
			case EWeaponType.WT_FRAGGRENADE:
			{
				return 5.0;
			}
			case EWeaponType.WT_SMOKEGRENADE:
			{
				return 5.0;
			}
			case EWeaponType.WT_ROCKETLAUNCHER:
			{
				return 1.1;
			}
			case EWeaponType.WT_SNIPERRIFLE:
			{
				return 0.001;
			}
			case EWeaponType.WT_GRENADELAUNCHER:
			{
				return 3.2;
			}
		}
		
		return 1.2;
	}
	
	float GetStanceTypeFactor(ECharacterStance stance)
	{
		switch(stance)
		{
			case ECharacterStance.STAND:
			{
				return 1.0;
			}
			case ECharacterStance.CROUCH:
			{
				return 0.7;
			}
			case ECharacterStance.PRONE:
			{
				return 0.5;
			}
		}
		return 1.0;
	}
	
	float GetHealthTypeFactor()
	{
		float rightArm = m_CombatComponent.GetAIInfoComponent().getCharDamageComp().GetGroupHealthScaled(ECharacterHitZoneGroup.RIGHTARM);
		float leftArm = m_CombatComponent.GetAIInfoComponent().getCharDamageComp().GetGroupHealthScaled(ECharacterHitZoneGroup.LEFTARM);
		return 2 - (rightArm + leftArm);
	}
	
	float GetAimImprovement()
	{
		float improvement;
		
		switch(ranks)
		{
			case DCO_CUSTOMRANK.RECRUIT:
			{
				improvement = m_CombatComponent.getImprovement();
				break;
			}
			case DCO_CUSTOMRANK.PRIVATE:
			{
				improvement = m_CombatComponent.getImprovement() * 2;
				break;
			}
			case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
			{
				improvement = m_CombatComponent.getImprovement() * 7;
				break;
			}
			case DCO_CUSTOMRANK.SPECIALIST:
			{
				improvement = m_CombatComponent.getImprovement() * 80;
				break;
			}
		}
		
		SCR_AIGroup myGrp = m_InfoComponent.m_UtilityComponent.getMyGroup();
		float distToLead = vector.Distance(m_InfoComponent.m_UtilityComponent.GetOrigin(), myGrp.GetLeaderEntity().GetOrigin());
		
		if (distToLead < 50)
			improvement = improvement * 10;
		
		return improvement;
	}
	
	float getADSFactor()
	{
		bool adsActive;
		adsActive = m_char.IsWeaponADS();
		
		if (adsActive)
			return 10;
		
		return 0;
	}
};

