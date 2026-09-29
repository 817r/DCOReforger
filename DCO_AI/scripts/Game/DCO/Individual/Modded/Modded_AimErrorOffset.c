modded class SCR_AIGetAimErrorOffset
{
	static const float LONG_RANGE_THRESHOLD = 200.0;







	private SCR_AIInfoComponent m_InfoComponent;
	private SCR_CharacterControllerComponent charCon;



	protected IEntity         m_DCOAimTargetEnt;
	protected EAimPointType   m_eDCOAimType;
	protected int             m_iDCOAimIndex = -1;
	protected float           m_fDCOAimHoldUntil_ms;
	protected float           m_fDCOAimNextVisCheck_ms;
	protected ref array<ref AimPoint> m_aDCOAimBuffer = {};

	override float GetDistanceFactor(float distance)
	{
		float errorFactor = 0;
		float MinError = 0.12 / m_CombatComponent.GetUtilityComponent().m_DCOConfig.GetAccuracy();
		if (distance < 20.0)
		{
			errorFactor = Math.Map(distance, 0, 20.0, 0.03, 0.1);
			return errorFactor;
		} else if (distance < 60.0)
		{
			errorFactor = Math.Map(distance, 20.0, 60.0, 0.1, MinError);
			return errorFactor;
		}

		float distanceCl = Math.Clamp((distance - 60.0) / LONG_RANGE_THRESHOLD, 0, 1.5);
		return Math.Lerp(MinError, 1.4, distanceCl);
	}

	override float GetTolerance(IEntity observer, IEntity target, float angularSize, float distance, EWeaponType weaponType)
	{
		float tolerance;
		bool setMaxTolerance;

		if (distance < 20.0)
			return Math.Map(distance, 0, 20.0, 10.0, 10.0 / 10);

		tolerance = angularSize / 2;
		tolerance *= GetAngularSpeedFactor(observer, target, setMaxTolerance);

		if (setMaxTolerance)
			tolerance = 10.0;
		else
		{
			tolerance *= GetWeaponTypeFactor(weaponType);
		};
		return Math.Clamp(tolerance, 0.005, 10.0);
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
		vector targetLocalAngularVelocity = observerAngularVelocity + (positionVector * relativeVelocity / positionVector.LengthSq());
		float totalTargetLocalAngularVelocity = targetLocalAngularVelocity.Length();

		if (totalTargetLocalAngularVelocity < 0.07)
			return 1.0;
		else if (totalTargetLocalAngularVelocity < 0.17)
			return 2;
		else if (totalTargetLocalAngularVelocity < 0.44)
			return 3;
		else if (totalTargetLocalAngularVelocity < 0.78)
			return 4;
		else if (totalTargetLocalAngularVelocity < 1.13)
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
				return 0.1;
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
		return Math.RandomGaussFloat(DCO_AimUtility.SkillSigma(skill), mu);
	}

	float GetThreatFactor()
	{
		if (!m_InfoComponent)
			return 1;

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

	protected float GetTurretFactor(IEntity observer)
	{
		if (!m_InfoComponent || !m_InfoComponent.HasUnitState(EUnitState.IN_TURRET))
			return 1;

		IEntity vehicle = DCO_VehicleCombat.GetVehicle(observer);
		if (!vehicle || !vehicle.GetPhysics())
			return 0.8;

		float speed = vehicle.GetPhysics().GetVelocity().Length();
		if (speed < 1.0)
			return 0.8;

		return 1 + speed * 0.1;
	}

	protected float GetScopeFactor(IEntity observer, IEntity target, float distance)
	{
		float extraMag = DCO_AimUtility.GetSightMagnification(m_CombatComponent) - 1;
		if (extraMag <= 0)
			return 1;

		float factor;
		if (distance < 30.0)
			factor = 1 + extraMag * 0.3 * (1 - distance / 30.0);
		else
		{
			float farFactor = 1 / (1 + extraMag * 0.25);
			float t = Math.Clamp((distance - 30.0) / (100.0 - 30.0), 0, 1);
			factor = Math.Lerp(1, farFactor, t);
		}

		bool tooFast;
		float angular = GetAngularSpeedFactor(observer, target, tooFast);
		if (tooFast)
			angular = 5.0;

		return factor * (1 + (angular - 1) * extraMag * 0.1);
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

		AimPoint aimPoint = DCO_SelectAimPoint(entity, target, aimpointTypes);

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

		vector offsetX, offsetY;
		float angularSize, distance, tolerance;
		GetTargetAngularBounds(entity, aimPoint, offsetX, offsetY, angularSize, distance);

		float distanceFactor = GetDistanceFactor(distance);
		float offsetWeaponFactor = GetOffsetWeaponTypeFactor(weaponType);
		float illuminationFactor = GetTargetIlluminationFactor(target);

		EAISkill currentSkill = m_CombatComponent.GetAISkill();
		DCO_AISKILL dcoSkill = m_CombatComponent.GetUtilityComponent().m_DCOConfig.GetAISkill();
		float scopeFactor = GetScopeFactor(entity, targetEntity, distance) * GetTurretFactor(entity);
		offsetX = GetRandomFactor(currentSkill, 0) * offsetX * 1.0 * distanceFactor * offsetWeaponFactor * illuminationFactor * GetImprovement() * GetThreatFactor() * GetSuppressionFactor() * StaminaFactor() * GetRandomFactorDCOSkill(dcoSkill, 0) * GetStanceFactor() * scopeFactor;
		offsetY = GetRandomFactor(currentSkill, 0) * offsetY * 1.0 * distanceFactor * offsetWeaponFactor * illuminationFactor * GetImprovement() * GetThreatFactor() * GetSuppressionFactor() * StaminaFactor() * GetRandomFactorDCOSkill(dcoSkill, 0) * GetStanceFactor() * scopeFactor;

		tolerance = GetTolerance(entity, targetEntity, angularSize, distance, weaponType);

		SetVariableOut(PORT_ERROR_OFFSET, offsetX + offsetY);
		SetVariableOut(PORT_AIM_POINT, aimPoint);
		SetVariableOut(PORT_TOLERANCE, tolerance);

#ifdef WORKBENCH
#endif

		return ENodeResult.SUCCESS;
	}

	protected float StaminaFactor()
	{
		if (!charCon)
			return 1;

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
		if (!charCon)
			return 1;

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
		m_InfoComponent = SCR_AIInfoComponent.Cast(owner.FindComponent(SCR_AIInfoComponent));
		IEntity ent = owner.GetControlledEntity();
		if (!ent)
			return;

		m_CombatComponent = SCR_AICombatComponent.Cast(ent.FindComponent(SCR_AICombatComponent));
		charCon = SCR_CharacterControllerComponent.Cast(ent.FindComponent(SCR_CharacterControllerComponent));
	}

	protected AimPoint DCO_SelectAimPoint(IEntity self, BaseTarget target, EAimPointType aimpointTypes[3])
	{
		PerceivableComponent perceivable = target.GetPerceivableComponent();
		if (!perceivable)
			return null;

		IEntity targetEnt = target.GetTargetEntity();
		float now_ms = GetGame().GetWorld().GetWorldTime();

		if (targetEnt && targetEnt == m_DCOAimTargetEnt && m_iDCOAimIndex >= 0 && now_ms < m_fDCOAimHoldUntil_ms)
		{
			AimPoint held = DCO_GetAimPointByIndex(perceivable, m_eDCOAimType, m_iDCOAimIndex);
			if (held)
			{
				if (now_ms < m_fDCOAimNextVisCheck_ms)
					return held;

				m_fDCOAimNextVisCheck_ms = now_ms + 0.5 * 1000.0;

				if (DCO_IsAimPointVisible(self, targetEnt, held))
					return held;
			}
		}

		return DCO_PickNewAimPoint(self, target, perceivable, aimpointTypes, now_ms);
	}

	protected AimPoint DCO_PickNewAimPoint(IEntity self, BaseTarget target, PerceivableComponent perceivable, EAimPointType aimpointTypes[3], float now_ms)
	{
		IEntity targetEnt = target.GetTargetEntity();

		EAimPointType order[3];
		order[0] = aimpointTypes[0];
		order[1] = aimpointTypes[1];
		order[2] = aimpointTypes[2];

		if (order[0] == EAimPointType.NORMAL && order[1] == EAimPointType.WEAK && targetEnt)
		{
			float dist = vector.Distance(self.GetOrigin(), targetEnt.GetOrigin());
			if (Math.RandomFloat01() < DCO_GetHeadAimChance(dist))
			{
				order[0] = EAimPointType.WEAK;
				order[1] = EAimPointType.NORMAL;
			}
		}

		EAimPointType fallbackType = -1;

		for (int i = 0; i < 3; i++)
		{
			EAimPointType type = order[i];
			if (type == -1)
				continue;

			m_aDCOAimBuffer.Clear();
			perceivable.GetAimpointsOfType(m_aDCOAimBuffer, type);

			int count = m_aDCOAimBuffer.Count();
			if (count == 0)
				continue;

			if (fallbackType == -1)
				fallbackType = type;

			int startIdx = Math.RandomInt(0, count);
			for (int k = 0; k < count; k++)
			{
				int idx = (startIdx + k) % count;
				AimPoint candidate = m_aDCOAimBuffer[idx];
				if (!candidate)
					continue;

				if (!DCO_IsAimPointVisible(self, targetEnt, candidate))
					continue;

				DCO_StoreAimSelection(targetEnt, type, idx, now_ms, Math.RandomFloat(1.5, 3.0));
				return candidate;
			}
		}

		if (fallbackType == -1)
		{
			m_iDCOAimIndex  = -1;
			m_DCOAimTargetEnt = null;
			return null;
		}

		m_aDCOAimBuffer.Clear();
		perceivable.GetAimpointsOfType(m_aDCOAimBuffer, fallbackType);

		int fbCount = m_aDCOAimBuffer.Count();
		if (fbCount == 0)
		{
			m_iDCOAimIndex  = -1;
			m_DCOAimTargetEnt = null;
			return null;
		}

		int fbIdx = Math.RandomInt(0, fbCount);
		DCO_StoreAimSelection(targetEnt, fallbackType, fbIdx, now_ms, 0.5);
		return m_aDCOAimBuffer[fbIdx];
	}

	protected void DCO_StoreAimSelection(IEntity targetEnt, EAimPointType type, int idx, float now_ms, float hold_s)
	{
		m_DCOAimTargetEnt        = targetEnt;
		m_eDCOAimType            = type;
		m_iDCOAimIndex           = idx;
		m_fDCOAimHoldUntil_ms    = now_ms + hold_s * 1000.0;
		m_fDCOAimNextVisCheck_ms = now_ms + 0.5 * 1000.0;
	}

	protected AimPoint DCO_GetAimPointByIndex(PerceivableComponent perceivable, EAimPointType type, int idx)
	{
		m_aDCOAimBuffer.Clear();
		perceivable.GetAimpointsOfType(m_aDCOAimBuffer, type);

		if (idx < 0 || idx >= m_aDCOAimBuffer.Count())
			return null;

		return m_aDCOAimBuffer[idx];
	}

	protected bool DCO_IsAimPointVisible(IEntity self, IEntity targetEnt, AimPoint aimPoint)
	{
		if (!self || !aimPoint)
			return false;

		vector eye;
		ChimeraCharacter selfChar = ChimeraCharacter.Cast(self);
		if (selfChar)
			eye = selfChar.EyePosition();
		else
			eye = self.GetOrigin() + vector.Up * 1.6;

		TraceParam param = new TraceParam();
		param.Start     = eye;
		param.End       = aimPoint.GetPosition();
		param.Exclude   = self;
		param.Flags     = TraceFlags.WORLD | TraceFlags.ENTS;
		param.LayerMask = EPhysicsLayerDefs.Projectile;

		DCO_Perf.Count("t:Modded_AimErrorOffset");
		float frac = GetGame().GetWorld().TraceMove(param, null);
		if (frac >= 0.98)
			return true;

		if (targetEnt && param.TraceEnt && param.TraceEnt.GetRootParent() == targetEnt.GetRootParent())
			return true;

		return false;
	}

	protected float DCO_GetHeadAimChance(float distance)
	{
		float chance = 0.2;

		if (m_CombatComponent && m_CombatComponent.GetUtilityComponent() && m_CombatComponent.GetUtilityComponent().m_DCOConfig)
		{
			switch (m_CombatComponent.GetUtilityComponent().m_DCOConfig.GetAISkill())
			{
				case DCO_AISKILL.NOOB:        chance = 0.05; break;
				case DCO_AISKILL.ROOKIE:      chance = 0.10; break;
				case DCO_AISKILL.REGULAR:     chance = 0.20; break;
				case DCO_AISKILL.VETERAN:     chance = 0.35; break;
				case DCO_AISKILL.EXPERT:      chance = 0.50; break;
				case DCO_AISKILL.SPECIAL_OPS: chance = 0.65; break;
				case DCO_AISKILL.TERMINATOR:  chance = 0.80; break;
			}
		}

		float distScale = 1.0;
		if (distance > 100.0)
		{
			float t = Math.Clamp((distance - 100.0) / (300.0 - 100.0), 0.0, 1.0);
			distScale = Math.Lerp(1.0, 0.3, t);
		}

		return chance * distScale;
	}
}
