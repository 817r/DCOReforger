modded class SCR_AIGetAimErrorOffset
{
	static const float VERY_CLOSE_RANGE_THRESHOLD = 20.0;
	static const float CLOSE_RANGE_THRESHOLD = 60.0;
	static const float LONG_RANGE_THRESHOLD = 250.0;
	static const float AIMING_ERROR_SCALE = 1.0;

	static const float AIMING_ERROR_FACTOR_MAX = 1.6;
	
	// RESERVE FOR FINAL CALCULATION
	static const float MAXIMAL_TOLERANCE = 10.0;
	static const float MINIMAL_TOLERANCE = 0.01;
	
	// AIMING ERROR FACTOR GLOBAL
	static const float AIMING_ERROR_FACTOR_MIN = 0.12; 
	
	// AIMING ERROR FACTOR DISTANCE VARIANCE
	static const float AIMING_ERROR_VERY_CLOSE_RANGE_FACTOR_MIN = 0.03;
	static const float AIMING_ERROR_CLOSE_RANGE_FACTOR_MIN = 0.12;
	
	private SCR_AIInfoComponent m_InfoComponent;
	private SCR_CharacterControllerComponent charCon;
	
	override float GetDistanceFactor(float distance)
	{
		float errorFactor = 0;
		float MinError = AIMING_ERROR_FACTOR_MIN / m_CombatComponent.GetUtilityComponent().m_DCOConfig.GetAccuracy();
		if (distance < VERY_CLOSE_RANGE_THRESHOLD)
		{
			errorFactor = Math.Map(distance, 0, VERY_CLOSE_RANGE_THRESHOLD, AIMING_ERROR_VERY_CLOSE_RANGE_FACTOR_MIN, AIMING_ERROR_CLOSE_RANGE_FACTOR_MIN);
			return errorFactor;
		} else if (distance < CLOSE_RANGE_THRESHOLD)
		{
			errorFactor = Math.Map(distance, VERY_CLOSE_RANGE_THRESHOLD, CLOSE_RANGE_THRESHOLD, AIMING_ERROR_CLOSE_RANGE_FACTOR_MIN, MinError);
			return errorFactor;
		}

		float distanceCl = Math.Clamp((distance - CLOSE_RANGE_THRESHOLD) / LONG_RANGE_THRESHOLD, 0, 1.5);
		return Math.Lerp(MinError, AIMING_ERROR_FACTOR_MAX, distanceCl);
	}
	
	override float GetTolerance(IEntity observer, IEntity target, float angularSize, float distance, EWeaponType weaponType)
	{
		float tolerance;
		bool setMaxTolerance;
	
		if (distance < VERY_CLOSE_RANGE_THRESHOLD)
			return Math.Map(distance, 0, VERY_CLOSE_RANGE_THRESHOLD, MAXIMAL_TOLERANCE, MAXIMAL_TOLERANCE / 10);
			
		tolerance = angularSize / 2;
		tolerance *= GetAngularSpeedFactor(observer, target, setMaxTolerance);
				
		if (setMaxTolerance)
			tolerance = MAXIMAL_TOLERANCE;
		else 
		{
			// weapon type tolerance modifier
			tolerance *= GetWeaponTypeFactor(weaponType);
		};
		return Math.Clamp(tolerance, MINIMAL_TOLERANCE, MAXIMAL_TOLERANCE);
	}	
	
	override float GetAngularSpeedFactor(IEntity observer, IEntity enemy, out bool setBigTolerance)
	{
		vector enemyVelocity;
		IEntity enemyRoot = enemy.GetRootParent();
		Physics enemyPhysics = enemyRoot.GetPhysics();
		if (enemyPhysics)
			enemyVelocity = enemyPhysics.GetVelocity();
		
		vector observerVelocity;
		vector observerAngularVelocity;
		IEntity observerRoot = observer.GetRootParent();
		Physics observerPhysics = observerRoot.GetPhysics();
		if (observerPhysics)
		{
			observerVelocity = observerPhysics.GetVelocity();
			observerAngularVelocity = observerPhysics.GetAngularVelocity();
		}
		
		vector relativeVelocity = enemyVelocity - observerVelocity;
		
		vector positionVector = enemy.GetOrigin() - observer.GetOrigin();
		vector targetLocalAngularVelocity = observerAngularVelocity + (positionVector * relativeVelocity / positionVector.LengthSq());  // omega = (r x v) / ||r||^2 
		float totalTargetLocalAngularVelocity = targetLocalAngularVelocity.Length();			
		
		if (totalTargetLocalAngularVelocity < 0.07) // rougly 4 degs in radians
			return 1.0;
		else if (totalTargetLocalAngularVelocity < 0.17) // roughly 10 degs in radians
			return 2;
		else if (totalTargetLocalAngularVelocity < 0.44) // 25 degs in radians
			return 3;
		else if (totalTargetLocalAngularVelocity < 0.78) // 45 degs in radians
			return 4;
		else if (totalTargetLocalAngularVelocity < 1.13) // 65 degs in radians
			return 5;
	
		setBigTolerance = true;
		return 0;
	}
	
	protected float GetSuppressionFactor()
	{
		if (!m_InfoComponent)
			return 1.0;
			
		float suppressionLevel = m_InfoComponent.GetThreatSystem().GetSuppressionMeasure(); 
		float maxSuppressionPenalty = 1.85; 
		float suppressionSpeedMultiplier = 1;
		
		float modifiedSuppression = suppressionLevel * suppressionSpeedMultiplier;

		modifiedSuppression = Math.Min(modifiedSuppression, 1.0);
		
		return Math.Lerp(1.0, maxSuppressionPenalty, modifiedSuppression);
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
				return 1.5;
			}
			case EWeaponType.WT_HANDGUN:
			{
				return 1.1;
			}
			case EWeaponType.WT_FRAGGRENADE:
			{
				return 1.0;
			}
			case EWeaponType.WT_SMOKEGRENADE:
			{
				return 1.0;
			}
			case EWeaponType.WT_ROCKETLAUNCHER:
			{
				return 1.1;
			}
			case EWeaponType.WT_SNIPERRIFLE:
			{
				return 0.5;
			}
			case EWeaponType.WT_AUTOCANNON:
			{
				return 2.6;
			}
		}
		return 1.0;
	}
	
	override float GetOffsetWeaponTypeFactor(EWeaponType weaponType)
	{
		switch(weaponType)
		{
			case EWeaponType.WT_RIFLE:
			{
				return 1.2;
			}
			case EWeaponType.WT_MACHINEGUN:
			{
				return 2.2;
			}
			case EWeaponType.WT_HANDGUN:
			{
				return 1.1;
			}
			case EWeaponType.WT_FRAGGRENADE:
			{
				return 1.5;
			}
			case EWeaponType.WT_SMOKEGRENADE:
			{
				return 1.3;
			}
			case EWeaponType.WT_ROCKETLAUNCHER:
			{
				return 0.5;
			}
			case EWeaponType.WT_SNIPERRIFLE:
			{
				return 0.2;
			}
			case EWeaponType.WT_AUTOCANNON:
			{
				return 2.6;
			}
		}
		
		return 1.0;
	}
	
	override float GetTargetIlluminationFactor(BaseTarget tgt)
	{
		PerceivableComponent perceivable = tgt.GetPerceivableComponent();
		if (!perceivable)
			return 10.0;
		
		if (perceivable.GetIlluminationFactor() < 0.2)
			return 6.0;
		
		if (perceivable.GetIlluminationFactor() < 0.5)
			return 3.0;
		
		return 1.0;
	}
	
	float GetRandomFactorDCOSkill(DCO_AISKILL skill, float mu)
	{
		float sigma;
		switch (skill)
		{
			case DCO_AISKILL.NOOB :
			{
				sigma = 3;
				break;
			}
			case DCO_AISKILL.ROOKIE :
			{
				sigma = 2.2;
				break;
			}
			case DCO_AISKILL.REGULAR :
			{
				sigma = 1.7;
				break;
			}
			case DCO_AISKILL.VETERAN :
			{
				sigma = 1.3;
				break;
			}
			case DCO_AISKILL.EXPERT :
			{
				sigma = 0.65;
				break;
			}
			case DCO_AISKILL.SPECIAL_OPS :
			{
				sigma = 0.2;
				break;
			}
			case DCO_AISKILL.TERMINATOR :
			{
				sigma = 0.0001;
				break;
			}
		}
		return Math.RandomGaussFloat(sigma,mu);
	}
	
	float GetThreatFactor()
	{
		switch (m_InfoComponent.GetThreatState())
		{
			case EAIThreatState.THREATENED :
			{
				return 2.7;
				break;
			}
			case EAIThreatState.ALERTED :
			{
				return 1.6;
				break;
			}
			case EAIThreatState.VIGILANT :
			{
				return 1.2;
				break;
			}
			case EAIThreatState.SAFE :
			{
				return 1;
				break;
			}
		}
		return 1;
	}

	float GetImprovement()
	{
		return m_CombatComponent.GetCurrentAimImprovement();
	}
	
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
    {
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
		
		if (!target)
		{
			ClearPorts();
			return ENodeResult.FAIL;
		}
		
		IEntity targetEntity = target.GetTargetEntity();
		if (!targetEntity)
		{
			ClearPorts();
			return ENodeResult.FAIL;
		}
		
		EAimPointType aimpointTypes[3];
		EAimPointType aimpointType0, aimpointType1;
		if (!GetVariableIn(PORT_AIMPOINT_TYPE_0, aimpointType0))
			aimpointType0 = -1;
		if (!GetVariableIn(PORT_AIMPOINT_TYPE_1, aimpointType1))
			aimpointType1 = -1;
		aimpointTypes[0] = aimpointType0;
		aimpointTypes[1] = aimpointType1;
		aimpointTypes[2] = m_eAimPointType;
		
		AimPoint aimPoint = GetAimPoint(target, aimpointTypes);
		
		if (!aimPoint)
		{
			ClearPorts();
			return ENodeResult.FAIL;
		}
		
		EWeaponType weaponType = m_CombatComponent.GetCurrentWeaponType();

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
		DCO_AISKILL dcoSkill = m_CombatComponent.GetUtilityComponent().m_DCOConfig.GetAISkill();
		offsetX = GetRandomFactor(currentSkill, 0) * offsetX * AIMING_ERROR_SCALE * distanceFactor * offsetWeaponFactor * illuminationFactor * GetImprovement() * GetThreatFactor() * GetSuppressionFactor() * StaminaFactor() * GetRandomFactorDCOSkill(dcoSkill, 0) * GetStanceFactor();
		offsetY = GetRandomFactor(currentSkill, 0) * offsetY * AIMING_ERROR_SCALE * distanceFactor * offsetWeaponFactor * illuminationFactor * GetImprovement() * GetThreatFactor() * GetSuppressionFactor() * StaminaFactor() * GetRandomFactorDCOSkill(dcoSkill, 0) * GetStanceFactor();
		
		tolerance = GetTolerance(entity, targetEntity, angularSize, distance, weaponType);
		
		SetVariableOut(PORT_ERROR_OFFSET, offsetX + offsetY);
		SetVariableOut(PORT_AIM_POINT, aimPoint);
		SetVariableOut(PORT_TOLERANCE, tolerance);
		
#ifdef WORKBENCH
		// PrintFormat("Target size - used in tolerance: %1 target aimpointPosition: %2", distance * Math.Tan(tolerance * Math.DEG2RAD), aimPoint.GetPosition());
#endif
		
		return ENodeResult.SUCCESS;
	}
	
	protected float StaminaFactor()
	{
		if (charCon.GetStamina() < 0.3)
			return 1.7;
		else if (charCon.GetStamina() < 0.6)
			return 1.4;
		else
			return 1;
	}
	
	protected float GetADSFactor()
	{
		if (charCon.IsWeaponADS())
			return 0.7;
		else
			return 1;
	}
	
	protected float GetStanceFactor()
	{
		switch (charCon.GetStance())
		{
			case ECharacterStance.PRONE:
			{
				return 0.7;
				break;
			}
			case ECharacterStance.CROUCH:
			{
				return 0.85;
				break;
			}
			case ECharacterStance.STAND:
			{
				return 1;
				break;
			}
			default:
			{
				return 1;
				break;
			}
		}
		
		return 1;
	}
	
	override void OnInit(AIAgent owner)
	{
		IEntity ent = owner.GetControlledEntity();
		if (ent)
			m_CombatComponent = SCR_AICombatComponent.Cast(ent.FindComponent(SCR_AICombatComponent));		
		m_InfoComponent = SCR_AIInfoComponent.Cast(owner.FindComponent(SCR_AIInfoComponent));
		charCon = SCR_CharacterControllerComponent.Cast(ent.FindComponent(SCR_CharacterControllerComponent));
	}
}