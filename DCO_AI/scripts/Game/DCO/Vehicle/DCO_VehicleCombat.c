class DCO_VehicleATThreat
{
	IEntity m_Entity;
	vector m_vPos;
	float m_fTime_ms;
}

class DCO_VehicleCombat
{
	static const float CHECK_INTERVAL_MS = 1000;
	static const float BAIL_HEALTH       = 0.25;
	static const float TURRET_BROKEN     = 0.9;
	static const float AT_SCAN_RANGE     = 350;
	static const float AT_SEEN_MAX_S     = 6;
	static const float AT_MEMORY_MS      = 10000;
	static const float AT_EVADE_DIST     = 250;

	protected static ref map<IEntity, float> s_mNextCheck = new map<IEntity, float>();
	protected static ref map<IEntity, ref DCO_VehicleATThreat> s_mATThreat = new map<IEntity, ref DCO_VehicleATThreat>();
	protected static ref array<BaseTarget> s_aTargets = {};

	static IEntity GetVehicle(IEntity character)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(character);
		if (!ch || !ch.IsInVehicle())
			return null;

		CompartmentAccessComponent access = ch.GetCompartmentAccessComponent();
		if (!access || !access.GetCompartment())
			return null;

		return access.GetCompartment().GetOwner().GetRootParent();
	}

	static bool IsArmored(IEntity vehicle)
	{
		if (!vehicle)
			return false;

		PerceivableComponent perc = PerceivableComponent.Cast(vehicle.FindComponent(PerceivableComponent));
		if (!perc)
			return false;

		EAIUnitType type = perc.GetUnitType();
		return type == EAIUnitType.UnitType_VehicleMedium || type == EAIUnitType.UnitType_VehicleHeavy;
	}

	static bool IsProtectedFromBullets(IEntity character)
	{
		return IsArmored(GetVehicle(character));
	}

	static void UpdateCrew(SCR_AIUtilityComponent utility)
	{
		IEntity self = utility.m_OwnerEntity;
		float now = GetGame().GetWorld().GetWorldTime();
		float next;
		if (s_mNextCheck.Find(self, next) && now < next)
			return;
		s_mNextCheck.Set(self, now + CHECK_INTERVAL_MS);

		IEntity vehicle = GetVehicle(self);
		if (!vehicle)
			return;

		if (utility.m_DCOConfig && utility.m_DCOConfig.IsHoldPosition())
		{
			SetHandBrakeIfDriver(self, true);
			return;
		}

		if (TryBailOut(utility, vehicle))
			return;

		ScanATThreats(utility, vehicle, now);
	}

	static void SetHandBrakeIfDriver(IEntity character, bool engaged)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(character);
		if (!ch || !ch.IsInVehicle())
			return;

		CompartmentAccessComponent access = ch.GetCompartmentAccessComponent();
		if (!access || !PilotCompartmentSlot.Cast(access.GetCompartment()))
			return;

		CarControllerComponent car = CarControllerComponent.Cast(access.GetCompartment().GetOwner().GetRootParent().FindComponent(CarControllerComponent));
		if (car)
			car.SetPersistentHandBrake(engaged);
	}

	protected static bool TryBailOut(SCR_AIUtilityComponent utility, IEntity vehicle)
	{
		if (utility.FindActionOfType(SCR_AIGetOutVehicle))
			return true;

		SCR_VehicleDamageManagerComponent dmg = SCR_VehicleDamageManagerComponent.Cast(vehicle.FindComponent(SCR_VehicleDamageManagerComponent));
		if (!dmg || dmg.IsDestroyed())
			return false;

		bool gunner = utility.m_AIInfo && utility.m_AIInfo.HasUnitState(EUnitState.IN_TURRET);
		string reason;
		if (dmg.IsOnFire())
			reason = "fire";
		else if (dmg.GetHealthScaled() < BAIL_HEALTH)
			reason = "hp";
		else if (!dmg.GetEngineFunctional())
		{
			if (gunner && IsArmored(vehicle) && dmg.GetAimingDamage() < TURRET_BROKEN)
				return false;
			reason = "immobile";
		}
		else if (gunner && !IsArmored(vehicle) && dmg.GetAimingDamage() >= TURRET_BROKEN)
			reason = "turret";

		if (reason.IsEmpty())
			return false;

		utility.AddAction(new SCR_AIGetOutVehicle(utility, null, vehicle, priority: SCR_AIActionBase.PRIORITY_BEHAVIOR_GET_OUT_VEHICLE_HIGH_PRIORITY));
		DCO_BenchmarkLoggerComponent.Event(string.Format("crew_bailout unit=%1 reason=%2 gunner=%3 hp=%4",
			utility.m_OwnerEntity, reason, gunner, dmg.GetHealthScaled().ToString(1, 2)));
		return true;
	}

	protected static void ScanATThreats(SCR_AIUtilityComponent utility, IEntity vehicle, float now)
	{
		PerceptionComponent perception = utility.m_PerceptionComponent;
		if (!perception)
			return;

		s_aTargets.Clear();
		perception.GetTargetsList(s_aTargets, ETargetCategory.ENEMY);

		vector vehPos = vehicle.GetOrigin();
		float bestDist = AT_SCAN_RANGE;
		BaseTarget best;
		foreach (BaseTarget target : s_aTargets)
		{
			if (!target || !target.GetTargetEntity() || target.GetTimeSinceSeen() > AT_SEEN_MAX_S)
				continue;

			float dist = vector.Distance(vehPos, target.GetLastSeenPosition());
			if (dist < bestDist && HasLauncherInHands(target))
			{
				best = target;
				bestDist = dist;
			}
		}

		if (best)
			ReportATThreat(vehicle, best.GetTargetEntity(), best.GetLastSeenPosition());
	}

	static const float NO_AT_STANDOFF = 60;
	static const float NO_AT_MIN_DIST = 25;

	static bool IsATKnown(IEntity vehicle, SCR_AIUtilityComponent utility)
	{
		if (!vehicle)
			return false;

		IEntity shooter;
		vector pos;
		if (GetATThreat(vehicle, shooter, pos))
			return true;

		vector vp = vehicle.GetOrigin();
		float rangeSq = AT_SCAN_RANGE * AT_SCAN_RANGE;
		if (utility && utility.m_PerceptionComponent)
		{
			s_aTargets.Clear();
			utility.m_PerceptionComponent.GetTargetsList(s_aTargets, ETargetCategory.ENEMY);
			foreach (BaseTarget t : s_aTargets)
			{
				if (t && vector.DistanceSq(vp, t.GetLastSeenPosition()) < rangeSq && HasLauncherInHands(t))
				{
					s_aTargets.Clear();
					return true;
				}
			}
			s_aTargets.Clear();
		}

		if (!utility || !utility.GetAIAgent())
			return false;

		SCR_AIGroup grp = SCR_AIGroup.Cast(utility.GetAIAgent().GetParentGroup());
		if (!grp || !grp.GetGroupUtilityComponent() || !grp.GetGroupUtilityComponent().GetPercGroupComp())
			return false;

		foreach (SCR_AITargetInfo info : grp.GetGroupUtilityComponent().GetPercGroupComp().m_aTargets)
		{
			if (info && info.m_Entity && vector.DistanceSq(vp, info.m_vWorldPos) < rangeSq
				&& DCO_Strength.HasWeaponInHands(info.m_Entity, EWeaponType.WT_ROCKETLAUNCHER))
				return true;
		}
		return false;
	}

	static bool HasLauncherInHands(BaseTarget target)
	{
		BaseWeaponManagerComponent wm = target.GetWeaponManagerComponent();
		if (!wm || !wm.GetCurrentWeapon())
			return false;

		return wm.GetCurrentWeapon().GetWeaponType() == EWeaponType.WT_ROCKETLAUNCHER;
	}

	static void ReportATThreat(IEntity vehicle, IEntity shooter, vector pos)
	{
		if (!Vehicle.Cast(vehicle))
			return;

		DCO_VehicleATThreat threat = s_mATThreat.Get(vehicle);
		if (!threat)
		{
			threat = new DCO_VehicleATThreat();
			s_mATThreat.Set(vehicle, threat);
		}

		if (threat.m_Entity != shooter)
			DCO_BenchmarkLoggerComponent.Event(string.Format("at_threat veh=%1 shooter=%2 dist=%3",
				vehicle, shooter, Math.Round(vector.Distance(vehicle.GetOrigin(), pos))));

		threat.m_Entity = shooter;
		threat.m_vPos = pos;
		threat.m_fTime_ms = GetGame().GetWorld().GetWorldTime();
	}

	static bool GetATThreat(IEntity vehicle, out IEntity shooter, out vector pos)
	{
		if (!vehicle)
			return false;

		DCO_VehicleATThreat threat = s_mATThreat.Get(vehicle);
		if (!threat || !threat.m_Entity)
			return false;

		if (GetGame().GetWorld().GetWorldTime() - threat.m_fTime_ms > AT_MEMORY_MS)
			return false;

		SCR_CharacterDamageManagerComponent dmg = SCR_CharacterDamageManagerComponent.Cast(threat.m_Entity.FindComponent(SCR_CharacterDamageManagerComponent));
		if (dmg && dmg.GetState() == EDamageState.DESTROYED)
			return false;

		shooter = threat.m_Entity;
		pos = threat.m_vPos;
		return true;
	}

	static SCR_AICombatMoveRequest_Move CreateEvadeRequest(IEntity vehicle, vector atPos)
	{
		float dist = vector.Distance(vehicle.GetOrigin(), atPos);

		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_TARGET;
		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		rq.m_vMovePos = atPos;
		rq.m_vTargetPos = atPos;
		rq.m_eMovementType = EMovementType.RUN;
		rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
		rq.m_fMoveDuration_s = Math.Clamp((AT_EVADE_DIST - dist + 50) / SCR_AICombatMoveUtils.GROUND_VEHICLE_GENERIC_SPEED, 3, 12);
		rq.m_bAimAtTarget = false;
		rq.m_bAimAtTargetEnd = false;
		return rq;
	}

	static bool CanPushEvade(SCR_AICombatMoveState driverState)
	{
		if (!driverState)
			return false;

		SCR_AICombatMoveRequest_Move rq = SCR_AICombatMoveRequest_Move.Cast(driverState.GetRequest());
		if (!rq || rq.m_eReason != SCR_EAICombatMoveReason.MOVE_FROM_TARGET)
			return true;

		return rq.m_eState != SCR_EAICombatMoveRequestState.IDLE && rq.m_eState != SCR_EAICombatMoveRequestState.EXECUTING;
	}
}
