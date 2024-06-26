modded class SCR_AIGetAimErrorOffset: AITaskScripted
{
	static const string PORT_ERROR_OFFSET = "ErrorOffset";
	static const string PORT_BASE_TARGET = "BaseTargetIn";
	static const string PORT_AIM_POINT = "AimPoint";
	static const string PORT_TOLERANCE = "AimingTolerance";
	static const float CLOSE_RANGE_THRESHOLD = 10.0;
	static const float MEDIUM_RANGE_THRESHOLD = 80.0;
	static const float LONG_RANGE_THRESHOLD = 200.0;
	
	static const float AIMING_ERROR_SCALE = 1.0; // TODO: game master and server option
	static const float AIMING_ERROR_FACTOR_MIN = 0.45; 
	static const float AIMING_ERROR_CLOSE_RANGE_FACTOR_MIN = 0.2;
	static const float AIMING_ERROR_FACTOR_MAX = 2.0;
	
	static const float MAXIMAL_TOLERANCE = 12.0;	
	static const float MINIMAL_TOLERANCE = 0.35;
	
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
    {
		//if (!m_CombatComponent || !m_InfoComponent)
		if (!m_CombatComponent)
			return ENodeResult.FAIL;
		
		IEntity entity = owner.GetControlledEntity();
		if (!entity)
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
		
		EAISkill currentSkill = m_CombatComponent.GetAISkill();
		offsetX = GetRandomFactor(currentSkill, 0) * offsetX * AIMING_ERROR_SCALE * distanceFactor * offsetWeaponFactor;
		offsetY = GetRandomFactor(currentSkill, 0) * offsetY * AIMING_ERROR_SCALE * distanceFactor * offsetWeaponFactor;
		
		tolerance = GetTolerances(entity, targetEntity, angularSize, distance, weaponType, stances);
		
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
	
	float GetTolerances(IEntity observer, IEntity target, float angularSize, float distance, EWeaponType weaponType, ECharacterStance stance)
	{
		float tolerance;
		bool setMaxTolerance;
		float maxTOl;
		float minTOl;
	
		// Always use max tolerance in close range
		if (distance < CLOSE_RANGE_THRESHOLD)
		{
			switch(weaponType)
			{
				case EWeaponType.WT_GRENADELAUNCHER:
				{
					maxTOl = MAXIMAL_TOLERANCE * 1.1;
					minTOl = MINIMAL_TOLERANCE * 300;
					break;
				}
				default:
				{
					maxTOl = MAXIMAL_TOLERANCE - 2.0;
					minTOl = MINIMAL_TOLERANCE * 30;
					break;
				}
			}
		}
		
		if (distance > CLOSE_RANGE_THRESHOLD)
		{
			switch(weaponType)
			{
				case EWeaponType.WT_RIFLE:
				{
					maxTOl = MAXIMAL_TOLERANCE * 1.1;
					minTOl = MINIMAL_TOLERANCE * 30;
					break;
				}
				case EWeaponType.WT_MACHINEGUN:
				{
					maxTOl = MAXIMAL_TOLERANCE * 1.25;
					minTOl = MINIMAL_TOLERANCE * 50;
					break;
				}
				case EWeaponType.WT_ROCKETLAUNCHER:
				{
					maxTOl = MAXIMAL_TOLERANCE * 1.2;
					minTOl = MINIMAL_TOLERANCE * 50;
					break;
				}
				case EWeaponType.WT_SNIPERRIFLE:
				{
					maxTOl = MAXIMAL_TOLERANCE / 5;
					minTOl = MINIMAL_TOLERANCE * 30;
					break;
				}
				case EWeaponType.WT_GRENADELAUNCHER:
				{
					maxTOl = MAXIMAL_TOLERANCE * 1.1;
					minTOl = MINIMAL_TOLERANCE * 50;
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
			
		
		if (distance > MEDIUM_RANGE_THRESHOLD)
		{
			switch(weaponType)
			{
				case EWeaponType.WT_RIFLE:
				{
					maxTOl = MAXIMAL_TOLERANCE * 1.1;
					minTOl = MINIMAL_TOLERANCE * 300;
					break;
				}
				case EWeaponType.WT_MACHINEGUN:
				{
					maxTOl = MAXIMAL_TOLERANCE * 1.25;
					minTOl = MINIMAL_TOLERANCE * 350;
					break;
				}
				case EWeaponType.WT_ROCKETLAUNCHER:
				{
					maxTOl = MAXIMAL_TOLERANCE * 1.2;
					minTOl = MINIMAL_TOLERANCE * 320;
					break;
				}
				case EWeaponType.WT_SNIPERRIFLE:
				{
					maxTOl = MAXIMAL_TOLERANCE / 4;
					minTOl = MINIMAL_TOLERANCE * 200;
					break;
				}
				case EWeaponType.WT_GRENADELAUNCHER:
				{
					maxTOl = MAXIMAL_TOLERANCE * 1.1;
					minTOl = MINIMAL_TOLERANCE * 350;
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
			
			
		tolerance = angularSize / 2; // half of the size
		// angular speed
		tolerance *= GetAngularSpeedFactor(observer, target, setMaxTolerance);
				
		if (setMaxTolerance)
			return MAXIMAL_TOLERANCE;
		else 
		{
			// weapon type tolerance modifier
			tolerance = (tolerance * (GetWeaponTypeFactor(weaponType) + GetStanceTypeFactor(stance) + GetHealthTypeFactor())) - GetAimImprovement();
		};
		return Math.Clamp(tolerance, minTOl, maxTOl);
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
				sigma = 2.75;
				break;
			}
			case EAISkill.ROOKIE :
			{
				sigma = 1.45;
				break;
			}
			case EAISkill.REGULAR :
			{
				sigma = 1.05;
				break;
			}
			case EAISkill.TRAINED :
			{
				sigma = 1.35;
				break;
			}
			case EAISkill.VETERAN :
			{
				sigma = 0.77;
				break;
			}
			case EAISkill.EXPERT :
			{
				sigma = 0.52;
				break;
			}
			case EAISkill.CYLON :
			{
				return 0.42;
			}
		}
		
		return Math.RandomGaussFloat(sigma,mu);
	}
	
	//------------------------------------------------------------------------------------------------
	// returns skill corrected by current threat level and if AI can shoot under such suppression
	EAISkill GetSkillFromThreat(EAISkill inSkill, EAIThreatState threat)
	{
		switch (threat)
		{
			case EAIThreatState.EXHAUSTED :
			{
				switch (inSkill)
				{
					case EAISkill.ROOKIE :
					{
						return EAISkill.ROOKIE;
					}
					case EAISkill.REGULAR :
					{
						return EAISkill.ROOKIE;
					}
					case EAISkill.VETERAN :
					{
						return EAISkill.ROOKIE;
					}
					case EAISkill.EXPERT :
					{
						return EAISkill.REGULAR;
					}
					case EAISkill.CYLON :
					{
						return EAISkill.VETERAN;
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
						return EAISkill.ROOKIE;
					}
					case EAISkill.REGULAR :
					{
						return EAISkill.ROOKIE;
					}
					case EAISkill.VETERAN :
					{
						return EAISkill.REGULAR;
					}
					case EAISkill.EXPERT :
					{
						return EAISkill.REGULAR;
					}
					case EAISkill.CYLON :
					{
						return EAISkill.VETERAN;
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
						return EAISkill.ROOKIE;
					}
					case EAISkill.REGULAR :
					{
						return EAISkill.ROOKIE;
					}
					case EAISkill.VETERAN :
					{
						return EAISkill.REGULAR;
					}
					case EAISkill.EXPERT :
					{
						return EAISkill.VETERAN;
					}
					case EAISkill.CYLON :
					{
						return EAISkill.EXPERT;
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
						return EAISkill.ROOKIE;
					}
					case EAISkill.REGULAR :
					{
						return EAISkill.REGULAR;
					}
					case EAISkill.VETERAN :
					{
						return EAISkill.REGULAR;
					}
					case EAISkill.EXPERT :
					{
						return EAISkill.VETERAN;
					}
					case EAISkill.CYLON :
					{
						return EAISkill.EXPERT;
					}
				};
				break;
			}
			default :
			{
				return inSkill;
				break;
			}	
		}	
		return EAISkill.NONE;
	}
	
	override float GetWeaponTypeFactor(EWeaponType weaponType)
	{
		switch(weaponType)
		{
			case EWeaponType.WT_RIFLE:
			{
				return 1.1;
			}
			case EWeaponType.WT_MACHINEGUN:
			{
				return 3.1;
			}
			case EWeaponType.WT_HANDGUN:
			{
				return 1.35;
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
				return 1.3;
			}
			case EWeaponType.WT_SNIPERRIFLE:
			{
				return 0.15;
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
				return 1.4;
			}
			case ECharacterStance.CROUCH:
			{
				return 1.2;
			}
			case ECharacterStance.PRONE:
			{
				return 2.0;
			}
		}
		return 1.0;
	}
	
	float GetHealthTypeFactor()
	{
		float currHealth = m_CombatComponent.getCurrentHealth();
		float maxHealth = m_CombatComponent.GetMaxHealth();
		
		if (currHealth < maxHealth)
		{
			return 0.5;
		} 
		else if (currHealth < (maxHealth - maxHealth/4))
		{
			return 1.1;
		} 
		else if (currHealth < maxHealth/2)
		{
			return 1.4;
		}

		return 0;
	}
	
	float GetAimImprovement()
	{
		return m_CombatComponent.getImprovement();
	}
};

