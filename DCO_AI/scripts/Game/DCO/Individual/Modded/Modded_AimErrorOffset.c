modded class SCR_AIGetAimErrorOffset
{
	static const float CLOSE_RANGE_THRESHOLD = 60.0;
	static const float AIMING_ERROR_SCALE = 1.0;
	static const float AIMING_ERROR_FACTOR_MAX = 1.4;
	static const float MAXIMAL_TOLERANCE = 10.0;
	static const float MINIMAL_TOLERANCE = 0.005;
	static const float AIMING_ERROR_FACTOR_MIN = 0.12;
	static const float AIMING_ERROR_CLOSE_RANGE_FACTOR_MIN = 0.1;
	static const float LONG_RANGE_THRESHOLD = 200.0;







	private SCR_AIInfoComponent m_InfoComponent;
	private SCR_CharacterControllerComponent charCon;



	protected IEntity         m_DCOAimTargetEnt;
	protected EAimPointType   m_eDCOAimType;
	protected int             m_iDCOAimIndex = -1;
	protected float           m_fDCOAimHoldUntil_ms;
	protected float           m_fDCOAimNextVisCheck_ms;
	protected ref array<ref AimPoint> m_aDCOAimBuffer = {};
	protected BaseWeaponComponent m_DCOZeroWeapon;
	protected float m_fDCOZero = 300.0;
	protected static int s_iDCOVertSample;
	protected float m_fDCOCalibShot_ms = -1;
	protected float m_fDCOLastLat;
	protected float m_fDCOLastVert;
	protected float m_fDCOLastDist;
	protected EWeaponType m_eDCOLastWeapon;
	protected IEntity m_DCOLastTarget;
	protected static int s_iDCOCalib100;
	protected static int s_iDCOCalib200;
	protected static int s_iDCOCalib300;
	protected static int s_iDCOCalibAll;


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

	protected float DCO_StressFactor()
	{
		if (!m_InfoComponent)
			return 1.0;

		float base = 1.0;
		EAIThreatState state = m_InfoComponent.GetThreatState();
		if (state == EAIThreatState.THREATENED)
			base = 1.4;
		else if (state == EAIThreatState.ALERTED)
			base = 1.2;

		float supp = Math.Clamp(m_InfoComponent.GetThreatSystem().GetSuppressionMeasure(), 0, 1);
		float stress = base * Math.Lerp(1.0, 2.3, supp);
		if (stress > 3.2)
		{
			stress = 3.2;
			DCO_Perf.Count("aim:pinned_cap");
		}
		return stress;
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
			case EWeaponType.WT_HANDGUN:
				return 1.8;
			case EWeaponType.WT_SNIPERRIFLE:
				return 0.6;
			case EWeaponType.WT_AUTOCANNON:
				return 2.6;
		}
		return 1.0;
	}

	override float GetTargetIlluminationFactor(BaseTarget tgt)
	{
		PerceivableComponent perceivable = tgt.GetPerceivableComponent();
		if (!perceivable)
			return 2.0;

		if (perceivable.GetIlluminationFactor() < 0.2)
			return 2.0;

		if (perceivable.GetIlluminationFactor() < 0.5)
			return 1.5;

		return 1.0;
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
		float mag = DCO_AimUtility.GetSightMagnification(m_CombatComponent);
		if (mag < 1.5)
			return 1;

		float farFactor = 0.85;
		if (mag >= 3.0)
			farFactor = 0.65;
		float factor = Math.Lerp(1, farFactor, Math.Clamp((distance - 50.0) / 100.0, 0, 1));

		bool tooFast;
		float angular = GetAngularSpeedFactor(observer, target, tooFast);
		if (tooFast)
			angular = 5.0;

		return factor * (1 + (angular - 1) * (mag - 1) * 0.1);
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
		DCO_AimCalibSample(entity);

		SCR_AIUtilityComponent aimUtil = m_CombatComponent.GetUtilityComponent();
		float accuracy = Math.Max(DCO_AIConfigComponent.AccuracyOf(aimUtil), 0.02);
		float shake = Math.Max(DCO_AIConfigComponent.AimShakeOf(aimUtil), 0.25);
		int posture;
		bool supported, moving;
		float sway = DCO_PostureFactor(posture, supported) * StaminaFactor();
		sway = 1 + (sway - 1) / shake;
		float stress = DCO_StressFactor();
		float light = GetTargetIlluminationFactor(target);
		float optic = GetScopeFactor(entity, targetEntity, distance);
		float ramp = GetImprovement();
		float move = DCO_MoveFactor(entity, moving);
		float sigma = 0.45 * DCO_AimUtility.SkillSigma(DCO_AIConfigComponent.SkillOf(aimUtil)) * sway * stress * move * light * optic
			* GetOffsetWeaponTypeFactor(weaponType) * GetTurretFactor(entity) * ramp / accuracy;
		sigma *= DCO_SmokeField.AimFactor(entity.GetOrigin() + vector.Up * 1.6, aimPoint.GetPosition());
		DCO_CtxState ctx = DCO_Ctx.Get(aimUtil);
		if (ctx)
		{
			float ctxAcc = Math.Clamp(DCO_Ctx.P(ctx, DCO_ECtxParam.ACC), 0.5, 2.0);
			sigma /= ctxAcc;
			DCO_Perf.Count("ctx:aim");
		}
		if (sigma < 0.2)
		{
			sigma = 0.2;
			DCO_Perf.Count("aim:floor_hit");
		}
		m_CombatComponent.DCO_SetAimDebug(sigma, posture, supported, stress, ramp, light, optic, moving);

		float lat = sigma * 0.001 * distance;
		float sv = DCO_VerticalSigma(distance, weaponType);
		float vert = Math.Sqrt(lat * lat + sv * sv);
		vector aimLos = aimPoint.GetPosition() - entity.GetOrigin();
		vector right = Vector(aimLos[2], 0, -aimLos[0]);
		if (right.LengthSq() > 0.0001)
			right.Normalize();
		offsetX = right * Math.RandomGaussFloat(1, 0) * lat;
		offsetY = Vector(0, 1, 0) * Math.RandomGaussFloat(1, 0) * vert;

		tolerance = GetTolerance(entity, targetEntity, angularSize, distance, weaponType);

		if (target && target.GetTimeSinceSeen() > 0.5 && DCO_GlobalAIComponent.BehaviorOnFor(owner, DCO_EBehavior.SUPPRESS_LAST_KNOWN))
		{
			vector los = aimPoint.GetPosition() - entity.GetOrigin();
			los[1] = 0;
			if (los.Length() > 1)
			{
				los.Normalize();
				offsetX = offsetX + Vector(los[2], 0, -los[0]) * Math.RandomFloat(-2.0, 2.0);
			}
		}

		m_fDCOLastLat = offsetX.Length();
		m_fDCOLastVert = offsetY.Length();
		m_fDCOLastDist = distance;
		m_eDCOLastWeapon = weaponType;
		m_DCOLastTarget = targetEntity;

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
			return 1.5;
		else if (charCon.GetStamina() < 0.6)
			return 1.25;
		return 1;
	}


	protected float DCO_PostureFactor(out int posture, out bool supported)
	{
		posture = 2;
		supported = m_CombatComponent.DCO_GetRestMul() > 1.0;
		if (!charCon)
			return 2.75;

		ECharacterStance stance = charCon.GetStance();
		float f = 2.75;
		if (supported)
			f = 1.5;
		if (stance == ECharacterStance.PRONE)
		{
			posture = 0;
			f = 1.3;
			if (supported)
				f = 1.0;
		}
		else if (stance == ECharacterStance.CROUCH)
		{
			posture = 1;
			f = 1.8;
			if (supported)
				f = 1.3;
		}

		if (!supported && posture != 0 && m_CombatComponent.DCO_GetAimHold() > 7000.0)
		{
			f *= 1.15;
			DCO_Perf.Count("aim:hold_shake");
		}
		return f;
	}

	protected float DCO_MoveFactor(IEntity observer, out bool moving)
	{
		moving = false;
		if (!observer || !observer.GetPhysics() || (m_InfoComponent && m_InfoComponent.HasUnitState(EUnitState.IN_TURRET)))
			return 1.0;

		float speed = observer.GetPhysics().GetVelocity().Length();
		if (speed <= 0.5)
			return 1.0;

		moving = true;
		if (speed > 3.0)
		{
			DCO_Perf.Count("aim:running_fire");
			return 8.0;
		}
		return 4.0;
	}

	protected void DCO_AimCalibSample(IEntity shooter)
	{
		if (!m_DCOLastTarget || m_fDCOLastDist <= 0 || m_fDCOLastDist > 400.0 || !DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
			return;
		float shot = DCO_ShotCounter.LastShotAt(shooter);
		if (shot <= m_fDCOCalibShot_ms)
			return;
		m_fDCOCalibShot_ms = shot;

		int band = 300;
		int bandN;
		if (m_fDCOLastDist < 150.0)
		{
			band = 100;
			s_iDCOCalib100++;
			bandN = s_iDCOCalib100;
		}
		else if (m_fDCOLastDist < 250.0)
		{
			band = 200;
			s_iDCOCalib200++;
			bandN = s_iDCOCalib200;
		}
		else
		{
			s_iDCOCalib300++;
			bandN = s_iDCOCalib300;
		}
		s_iDCOCalibAll++;

		string pick = "debug";
		if (bandN <= 30)
			pick = "band_quota";
		else if (s_iDCOCalibAll % 20 == 0)
			pick = "1in20";
		else if (!DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
			return;

		string weapon = "rifle";
		if (m_eDCOLastWeapon == EWeaponType.WT_MACHINEGUN)
			weapon = "mg";
		else if (m_eDCOLastWeapon == EWeaponType.WT_SNIPERRIFLE)
			weapon = "dmr";
		else if (m_eDCOLastWeapon == EWeaponType.WT_HANDGUN)
			weapon = "pistol";
		string posture = "stand";
		if (m_CombatComponent.m_iDCOAimPosture == 0)
			posture = "prone";
		else if (m_CombatComponent.m_iDCOAimPosture == 1)
			posture = "crouch";

		string head = string.Format("miss_calib skill=%1 weapon=%2 band=%3 lat_m=%4 dist=%5 sigma_mrad=%6 posture=%7 supported=%8 stress=%9",
			typename.EnumToString(DCO_AISKILL, DCO_AIConfigComponent.SkillOf(m_CombatComponent.GetUtilityComponent())), weapon, band, m_fDCOLastLat.ToString(-1, 2), Math.Round(m_fDCOLastDist),
			m_CombatComponent.m_fDCOAimSigma.ToString(-1, 2), posture, m_CombatComponent.m_bDCOAimSupported, m_CombatComponent.m_fDCOAimStress.ToString(-1, 2));
		string tail = string.Format(" ramp=%1 light=%2 optic=%3 moving=%4 target=%5 src=aim pick=%6 band_n=%7 vert_m=%8",
			m_CombatComponent.m_fDCOAimRamp.ToString(-1, 2), m_CombatComponent.m_fDCOAimLight, m_CombatComponent.m_fDCOAimOptic.ToString(-1, 2), m_CombatComponent.m_bDCOAimMoving,
			DCO_Log.IdUnit(m_DCOLastTarget), pick, bandN, m_fDCOLastVert.ToString(-1, 2));
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(shooter), head + tail);
	}

	protected float DCO_VerticalSigma(float distance, EWeaponType weaponType)
	{
		if (weaponType == EWeaponType.WT_SNIPERRIFLE || weaponType == EWeaponType.WT_HANDGUN || distance <= 300.0)
			return 0;

		BaseWeaponComponent weapon = m_CombatComponent.GetCurrentWeapon();
		if (!weapon)
			return 0;

		if (weapon != m_DCOZeroWeapon)
		{
			m_DCOZeroWeapon = weapon;
			m_fDCOZero = 300.0;
			EntityPrefabData pd = weapon.GetOwner().GetPrefabData();
			if (pd)
			{
				string name = pd.GetPrefabName();
				name.ToLower();
				if (name.Contains("ak74") || name.Contains("rpk74") || name.Contains("ak-74") || name.Contains("rpk-74"))
					m_fDCOZero = 440.0;
			}
		}

		if (distance <= m_fDCOZero)
			return 0;

		float tz = m_fDCOZero / 900.0;
		float td = distance / 900.0;
		float sv = 0.15 * 4.9 * (td * td - tz * tz);
		DCO_Perf.Count("aim:vertical");
		s_iDCOVertSample++;
		if (s_iDCOVertSample % 20 == 0 && DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_CombatComponent.GetOwner()), string.Format("aim_vertical dist=%1 zero=%2 v=900 sigma_v_m=%3", Math.Round(distance), m_fDCOZero, sv.ToString(-1, 3)));
		return sv;
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
		int perfStart = DCO_Perf.Begin();
		AimPoint picked = DCO_SelectAimPointTimed(self, target, aimpointTypes);
		DCO_Perf.End("aim_pick", perfStart);
		return picked;
	}

	protected AimPoint DCO_SelectAimPointTimed(IEntity self, BaseTarget target, EAimPointType aimpointTypes[3])
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
				DCO_Perf.Count("aim:head");
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

		if (DCO_Util.TraceClear(eye, aimPoint.GetPosition(), self, 0.98, TraceFlags.WORLD | TraceFlags.ENTS, EPhysicsLayerDefs.Projectile, targetEnt, "t:Modded_AimErrorOffset"))
			return true;

		return false;
	}

	protected float DCO_GetHeadAimChance(float distance)
	{
		float chance = 0.1;

		if (m_CombatComponent && m_CombatComponent.GetUtilityComponent() && m_CombatComponent.GetUtilityComponent().m_DCOConfig)
		{
			switch (m_CombatComponent.GetUtilityComponent().m_DCOConfig.GetAISkill())
			{
				case DCO_AISKILL.NOOB:        chance = 0.02; break;
				case DCO_AISKILL.ROOKIE:      chance = 0.05; break;
				case DCO_AISKILL.REGULAR:     chance = 0.10; break;
				case DCO_AISKILL.VETERAN:     chance = 0.18; break;
				case DCO_AISKILL.EXPERT:      chance = 0.25; break;
				case DCO_AISKILL.SPECIAL_OPS: chance = 0.33; break;
				case DCO_AISKILL.TERMINATOR:  chance = 0.40; break;
			}
		}
		if (m_CombatComponent)
			chance *= DCO_Stress.T(m_CombatComponent.GetUtilityComponent(), "aim.head_mul", 1.0);

		float distScale = 1.0;
		if (distance > 100.0)
		{
			float t = Math.Clamp((distance - 100.0) / (300.0 - 100.0), 0.0, 1.0);
			distScale = Math.Lerp(1.0, 0.3, t);
		}

		return Math.Clamp(chance * distScale, 0, 1);
	}
}
