modded class SCR_AIGetAimErrorOffset: AITaskScripted
{	
    //------------------------------------------------------------------------------------------------
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
		offsetX = (GetRandomFactor(currentSkill, 0) * offsetX * AIMING_ERROR_SCALE * distanceFactor * offsetWeaponFactor * illuminationFactor / GetAimImprovement()) * MoraleFactor();
		offsetY = GetRandomFactor(currentSkill, 0) * offsetY * AIMING_ERROR_SCALE * distanceFactor * offsetWeaponFactor * illuminationFactor / GetAimImprovement();
		
		tolerance = GetTolerance(entity, targetEntity, angularSize, distance, weaponType);
		
		SetVariableOut(PORT_ERROR_OFFSET, offsetX + offsetY);
		SetVariableOut(PORT_AIM_POINT, aimPoint);
		SetVariableOut(PORT_TOLERANCE, tolerance);
		
#ifdef WORKBENCH
		// PrintFormat("Target size - used in tolerance: %1 target aimpointPosition: %2", distance * Math.Tan(tolerance * Math.DEG2RAD), aimPoint.GetPosition());
#endif
		
		return ENodeResult.SUCCESS;
	}

	
	float GetAimImprovement()
	{
		return m_CombatComponent.GetOverallAimImprovement();
	}
	
	float MoraleFactor()
	{
		return m_CombatComponent.MoraleAimFactor();
	}
	
	//------------------------------------------------------------------------------------------------
    static override bool VisibleInPalette() {return true;}
	
	//------------------------------------------------------------------------------------------------
	protected static override string GetOnHoverDescription() {return "Get aiming error position from the center of the target";}
};

