modded class SCR_AIGetAimErrorOffset
{
	static const float VERY_CLOSE_RANGE_THRESHOLD = 30.0;
	static const float CLOSE_RANGE_THRESHOLD = 60.0;
	static const float LONG_RANGE_THRESHOLD = 300.0;
	static const float AIMING_ERROR_SCALE = 1.0;

	static const float AIMING_ERROR_FACTOR_MAX = 3;
	
	// RESERVE FOR FINAL CALCULATION
	static const float MAXIMAL_TOLERANCE = 10.0;
	static const float MINIMAL_TOLERANCE = 0.003;
	
	// AIMING ERROR FACTOR GLOBAL
	static const float AIMING_ERROR_FACTOR_MIN = 0.12; 
	
	// AIMING ERROR FACTOR DISTANCE VARIANCE
	static const float AIMING_ERROR_VERY_CLOSE_RANGE_FACTOR_MIN = 0.005;
	static const float AIMING_ERROR_CLOSE_RANGE_FACTOR_MIN = 0.01;
	
	private SCR_AIInfoComponent m_InfoComponent;
	
	override float GetDistanceFactor(float distance)
	{
		float errorFactor = 0;
		
		if (distance < VERY_CLOSE_RANGE_THRESHOLD)
		{
			errorFactor = Math.Map(distance, 0, VERY_CLOSE_RANGE_THRESHOLD, AIMING_ERROR_VERY_CLOSE_RANGE_FACTOR_MIN, AIMING_ERROR_FACTOR_MIN);
			return errorFactor;
		} else if (distance < CLOSE_RANGE_THRESHOLD)
		{
			errorFactor = Math.Map(distance, VERY_CLOSE_RANGE_THRESHOLD, CLOSE_RANGE_THRESHOLD, AIMING_ERROR_CLOSE_RANGE_FACTOR_MIN, AIMING_ERROR_FACTOR_MIN);
			return errorFactor;
		}

		float distanceCl = Math.Clamp((distance - CLOSE_RANGE_THRESHOLD) / LONG_RANGE_THRESHOLD, 0, 3);
		return Math.Lerp(AIMING_ERROR_FACTOR_MIN, AIMING_ERROR_FACTOR_MAX, distanceCl);
	}
	
	override float GetTolerance(IEntity observer, IEntity target, float angularSize, float distance, EWeaponType weaponType)
	{
		float tolerance;
		bool setMaxTolerance;
	
		// Always use max tolerance in close range
		if (distance < VERY_CLOSE_RANGE_THRESHOLD)
			return MAXIMAL_TOLERANCE;
			
		tolerance = angularSize / 2; // half of the size
		// angular speed
		tolerance *= GetAngularSpeedFactor(observer, target, setMaxTolerance);
				
		if (setMaxTolerance)
			tolerance = MAXIMAL_TOLERANCE;
		else 
		{
			// weapon type tolerance modifier
			tolerance *= GetWeaponTypeFactor(weaponType);
		};
		return Math.Clamp(tolerance, MINIMAL_TOLERANCE, MAXIMAL_TOLERANCE * 1.5);
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
				return 3.0;
			}
			case EWeaponType.WT_HANDGUN:
			{
				return 1.2;
			}
			case EWeaponType.WT_FRAGGRENADE:
			{
				return 3.0;
			}
			case EWeaponType.WT_SMOKEGRENADE:
			{
				return 4.0;
			}
			case EWeaponType.WT_ROCKETLAUNCHER:
			{
				return 1.3;
			}
			case EWeaponType.WT_SNIPERRIFLE:
			{
				return 0.5;
			}
			case EWeaponType.WT_AUTOCANNON:
			{
				return 3.0;
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
				return 1.0;
			}
			case EWeaponType.WT_MACHINEGUN:
			{
				return 2.0;
			}
			case EWeaponType.WT_HANDGUN:
			{
				return 1.2;
			}
			case EWeaponType.WT_FRAGGRENADE:
			{
				return 1.2;
			}
			case EWeaponType.WT_SMOKEGRENADE:
			{
				return 1.0;
			}
			case EWeaponType.WT_ROCKETLAUNCHER:
			{
				return 0.2;
			}
			case EWeaponType.WT_SNIPERRIFLE:
			{
				return 0.3;
			}
			case EWeaponType.WT_AUTOCANNON:
			{
				return 3.5;
			}
		}
		
		return 1.0;
	}
	
	override float GetTargetIlluminationFactor(BaseTarget tgt)
	{
		PerceivableComponent perceivable = tgt.GetPerceivableComponent();
		if (!perceivable)
			return 1.5;
		
		if (perceivable.GetIlluminationFactor() < 0.2)
			return 4.0;
		
		if (perceivable.GetIlluminationFactor() < 0.5)
			return 2.0;
		
		return 1.0;
	}
	
	override float GetRandomFactor(EAISkill skill,float mu)
	{
		float sigma;
		switch (skill)
		{
			case EAISkill.NOOB :
			{
				sigma = 4;
				break;
			}
			case EAISkill.ROOKIE :
			{
				sigma = 2;
				break;
			}
			case EAISkill.REGULAR :
			{
				sigma = 1.1;
				break;
			}
			case EAISkill.VETERAN :
			{
				sigma = 0.75;
				break;
			}
			case EAISkill.EXPERT :
			{
				sigma = 0.45;
				break;
			}
			case EAISkill.CYLON :
			{
				return 0.1;
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
				return 2.5;
				break;
			}
			case EAIThreatState.ALERTED :
			{
				return 1.7;
				break;
			}
			case EAIThreatState.VIGILANT :
			{
				return 1.2;
				break;
			}
			case EAIThreatState.SAFE :
			{
				return 0.9;
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
		offsetX = GetRandomFactor(currentSkill, 0) * offsetX * AIMING_ERROR_SCALE * distanceFactor * offsetWeaponFactor * illuminationFactor * GetImprovement() * GetThreatFactor() * GetGlobalModifier();
		offsetY = GetRandomFactor(currentSkill, 0) * offsetY * AIMING_ERROR_SCALE * distanceFactor * offsetWeaponFactor * illuminationFactor * GetImprovement() * GetThreatFactor() * GetGlobalModifier();
		
		tolerance = GetTolerance(entity, targetEntity, angularSize, distance, weaponType);
		
		SetVariableOut(PORT_ERROR_OFFSET, offsetX + offsetY);
		SetVariableOut(PORT_AIM_POINT, aimPoint);
		SetVariableOut(PORT_TOLERANCE, tolerance);
		
#ifdef WORKBENCH
		// PrintFormat("Target size - used in tolerance: %1 target aimpointPosition: %2", distance * Math.Tan(tolerance * Math.DEG2RAD), aimPoint.GetPosition());
#endif
		
		return ENodeResult.SUCCESS;
	}
	
	float GetGlobalModifier()
	{
		float dcoAiSetting = m_CombatComponent.GetUtilityComponent().m_DCOConfig.GetAccuracy();
		float val = 1 / dcoAiSetting;	
		
		return val;
	}
	
	override void OnInit(AIAgent owner)
	{
		IEntity ent = owner.GetControlledEntity();
		if (ent)
			m_CombatComponent = SCR_AICombatComponent.Cast(ent.FindComponent(SCR_AICombatComponent));		
		m_InfoComponent = SCR_AIInfoComponent.Cast(owner.FindComponent(SCR_AIInfoComponent));
	}
}