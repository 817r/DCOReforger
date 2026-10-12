class DCO_VehicleATThreat
{
	IEntity m_Entity;
	vector m_vPos;
	float m_fTime_ms;
}

class DCO_VehicleCombat
{

	protected static ref map<IEntity, float> s_mNextCheck;
	protected static map<IEntity, float> GetNextCheck()
	{
		if (!s_mNextCheck)
			s_mNextCheck = new map<IEntity, float>();
		return s_mNextCheck;
	}
	protected static ref map<IEntity, ref DCO_VehicleATThreat> s_mATThreat;
	protected static map<IEntity, ref DCO_VehicleATThreat> GetATThreatStore()
	{
		if (!s_mATThreat)
			s_mATThreat = new map<IEntity, ref DCO_VehicleATThreat>();
		return s_mATThreat;
	}
	protected static ref map<IEntity, float> s_mFireAt;
	protected static map<IEntity, float> GetFireAt()
	{
		if (!s_mFireAt)
			s_mFireAt = new map<IEntity, float>();
		return s_mFireAt;
	}
	protected static ref map<IEntity, float> s_mTurretFillNext;
	protected static map<IEntity, float> GetTurretFillNext()
	{
		if (!s_mTurretFillNext)
			s_mTurretFillNext = new map<IEntity, float>();
		return s_mTurretFillNext;
	}
	protected static ref array<BaseTarget> s_aTargets;
	protected static array<BaseTarget> GetTargets()
	{
		if (!s_aTargets)
			s_aTargets = {};
		return s_aTargets;
	}

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
		if (GetNextCheck().Find(self, next) && now < next)
			return;
		GetNextCheck().Set(self, now + 1000.0);

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

		if (TryFillTurret(utility, vehicle))
			return;

		if (TryDismountUnarmed(utility, vehicle))
			return;

		ScanATThreats(utility, vehicle, now);
		DCO_VehicleActive.Tick(utility, vehicle);
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

	static bool HasTurret(IEntity vehicle)
	{
		BaseCompartmentManagerComponent mgr = BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(BaseCompartmentManagerComponent));
		if (!mgr)
			return false;
		array<BaseCompartmentSlot> slots = {};
		mgr.GetCompartments(slots);
		foreach (BaseCompartmentSlot slot : slots)
		{
			if (TurretCompartmentSlot.Cast(slot))
				return true;
		}
		return false;
	}

	protected static bool TryFillTurret(SCR_AIUtilityComponent utility, IEntity vehicle)
	{
		ChimeraCharacter selfCh = ChimeraCharacter.Cast(utility.m_OwnerEntity);
		if (!selfCh)
			return false;
		SCR_CompartmentAccessComponent acc = SCR_CompartmentAccessComponent.Cast(selfCh.GetCompartmentAccessComponent());
		if (!acc || !CargoCompartmentSlot.Cast(acc.GetCompartment()))
			return false;
		float now = GetGame().GetWorld().GetWorldTime();
		float next;
		if (GetTurretFillNext().Find(vehicle, next) && now < next)
			return false;
		BaseCompartmentManagerComponent mgr = BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(BaseCompartmentManagerComponent));
		if (!mgr)
			return false;
		SCR_AIGroup grp = DCO_GroupFeatures.GroupOf(selfCh);
		array<BaseCompartmentSlot> slots = {};
		mgr.GetCompartments(slots);
		TurretCompartmentSlot empty;
		bool ownCrew;
		foreach (BaseCompartmentSlot slot : slots)
		{
			IEntity occ = slot.GetOccupant();
			bool alive = occ && DCO_Util.IsAlive(occ);
			if (alive && grp && (PilotCompartmentSlot.Cast(slot) || TurretCompartmentSlot.Cast(slot)) && DCO_GroupFeatures.GroupOf(occ) == grp)
				ownCrew = true;
			TurretCompartmentSlot ts = TurretCompartmentSlot.Cast(slot);
			if (ts && !alive && !empty && ts.IsCompartmentAccessible())
				empty = ts;
		}
		if (!empty || !ownCrew)
			return false;
		GetTurretFillNext().Set(vehicle, now + 10000.0);
		bool reserved = empty.IsReserved();
		bool ok = acc.MoveInVehicle(vehicle, ECompartmentType.TURRET, false, empty);
		if (ok)
			DCO_Perf.Count("veh:turret_fill_ok");
		else
			DCO_Perf.Count("veh:turret_fill_fail");
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, DCO_Log.IdVeh(vehicle), string.Format("veh_turret_fill veh=%1 unit=%2 grp=%3 ok=%4 reserved=%5 dead_occ=%6",
			DCO_Log.IdVeh(vehicle), DCO_Log.IdUnit(selfCh), DCO_Log.IdGroup(grp), DCO_Util.B01(ok), DCO_Util.B01(reserved), DCO_Util.B01(empty.GetOccupant() != null)));
		return ok;
	}

	protected static bool TryDismountUnarmed(SCR_AIUtilityComponent utility, IEntity vehicle)
	{
		if (IsArmored(vehicle) || HasTurret(vehicle))
			return false;
		ChimeraCharacter selfCh = ChimeraCharacter.Cast(utility.m_OwnerEntity);
		if (selfCh && selfCh.GetCompartmentAccessComponent() && TurretCompartmentSlot.Cast(selfCh.GetCompartmentAccessComponent().GetCompartment()))
		{
			DCO_Perf.Count("crew_dismount_unarmed:skip_turret");
			return false;
		}
		Physics ph = vehicle.GetPhysics();
		if (ph && ph.GetVelocity().Length() > 1.0)
			return false;
		AIAgent agent = utility.GetAIAgent();
		if (!agent)
			return false;
		AIGroup grp = agent.GetParentGroup();
		if (!grp)
			return false;
		AIWaypoint wp = grp.GetCurrentWaypoint();
		SCR_BoardingEntityWaypoint bw = SCR_BoardingEntityWaypoint.Cast(wp);
		if (bw && bw.GetEntity())
			return false;
		if (wp && vector.DistanceXZ(wp.GetOrigin(), vehicle.GetOrigin()) > 60.0)
			return false;

		bool threat = utility.m_ThreatSystem && utility.m_ThreatSystem.GetState() >= EAIThreatState.ALERTED;
		if (!threat)
		{
			SCR_AIGroupUtilityComponent gu = SCR_AIGroupUtilityComponent.Cast(grp.FindComponent(SCR_AIGroupUtilityComponent));
			if (gu && gu.m_Perception)
			{
				foreach (SCR_AIGroupTargetCluster c : gu.m_Perception.m_aTargetClusters)
				{
					if (c.m_State && c.m_State.m_iCountAlive > 0 && c.m_State.m_fDistMin < 300.0 && c.m_State.GetTimeSinceLastNewInformation() < 30.0)
					{
						threat = true;
						break;
					}
				}
			}
		}
		if (!threat)
			return false;

		utility.AddAction(new SCR_AIGetOutVehicle(utility, null, vehicle));
		DCO_Perf.Count("crew_dismount_unarmed");
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("crew_dismount_unarmed unit=%1 grp=%2", utility.m_OwnerEntity, grp.GetName()));
		return true;
	}

	static bool DCO_IsPlanVehFix(SCR_AIUtilityComponent utility)
	{
		if (!utility)
			return false;
		AIAgent agent = utility.GetAIAgent();
		if (!agent || !agent.GetParentGroup())
			return false;
		return false;
	}

	static string DCO_GroupName(SCR_AIUtilityComponent utility)
	{
		AIAgent agent = utility.GetAIAgent();
		if (!agent || !agent.GetParentGroup())
			return "-";
		return agent.GetParentGroup().GetName();
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
		float hp = dmg.GetHealthScaled();
		if (!dmg.IsOnFire())
			GetFireAt().Remove(vehicle);
		if (dmg.IsOnFire())
		{
			float now = GetGame().GetWorld().GetWorldTime();
			float fireAt;
			if (!GetFireAt().Find(vehicle, fireAt))
			{
				fireAt = now;
				GetFireAt().Set(vehicle, now);
			}
			if (IsArmored(vehicle) && hp >= 0.5 && now - fireAt < 10000.0)
			{
				DCO_Perf.Count("veh:bailout_fire_wait");
				DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("crew_bailout_wait unit=%1 reason=fire hp=%2 burn_s=%3", DCO_Log.IdUnit(utility.m_OwnerEntity), hp.ToString(1, 2), (0.001 * (now - fireAt)).ToString(1, 1)));
				return false;
			}
			reason = "fire";
		}
		else if (dmg.GetHealthScaled() < 0.25)
			reason = "hp";
		else if (!dmg.GetEngineFunctional())
		{
			if (gunner && IsArmored(vehicle) && dmg.GetAimingDamage() < 0.9)
				return false;
			reason = "immobile";
		}
		else if (gunner && !IsArmored(vehicle) && dmg.GetAimingDamage() >= 0.9)
			reason = "turret";

		if (reason.IsEmpty())
			return false;
		if ((reason == "immobile" || reason == "turret") && DCO_IsPlanVehFix(utility))
		{
			DCO_Perf.Count("veh_fix_getout_block:bailout_" + reason);
			DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("veh_fix_getout_block grp=%1 unit=%2 by=bailout_%3", DCO_GroupName(utility), utility.m_OwnerEntity, reason));
			return false;
		}

		utility.AddAction(new SCR_AIGetOutVehicle(utility, null, vehicle, priority: SCR_AIActionBase.PRIORITY_BEHAVIOR_GET_OUT_VEHICLE_HIGH_PRIORITY));
		DCO_Perf.Count("veh:bailout_" + reason);
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("crew_bailout unit=%1 reason=%2 gunner=%3 hp=%4",
			DCO_Log.IdUnit(utility.m_OwnerEntity), reason, gunner, hp.ToString(1, 2)));
		return true;
	}

	protected static void ScanATThreats(SCR_AIUtilityComponent utility, IEntity vehicle, float now)
	{
		PerceptionComponent perception = utility.m_PerceptionComponent;
		if (!perception)
			return;

		GetTargets().Clear();
		GetTargets();
		perception.GetTargetsList(s_aTargets, ETargetCategory.ENEMY);

		vector vehPos = vehicle.GetOrigin();
		float bestDist = 350.0;
		BaseTarget best;
		foreach (BaseTarget target : GetTargets())
		{
			if (!target || !target.GetTargetEntity() || target.GetTimeSinceSeen() > 6.0)
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


	protected static ref map<IEntity, float> s_mATActiveUntil;
	protected static map<IEntity, float> GetATActiveUntil()
	{
		if (!s_mATActiveUntil)
			s_mATActiveUntil = new map<IEntity, float>();
		return s_mATActiveUntil;
	}

	static bool IsATKnown(IEntity vehicle, SCR_AIUtilityComponent utility)
	{
		if (!vehicle)
			return false;

		float now = GetGame().GetWorld().GetWorldTime();
		float until;
		if (GetATActiveUntil().Find(vehicle, until) && now < until)
			return true;
		if (!DetectATNow(vehicle, utility))
			return false;

		if (GetATActiveUntil().Count() > 64)
		{
			array<IEntity> stale = {};
			foreach (IEntity v, float u : GetATActiveUntil())
			{
				if (!v || u < now)
					stale.Insert(v);
			}
			foreach (IEntity sv : stale)
				GetATActiveUntil().Remove(sv);
		}
		GetATActiveUntil().Set(vehicle, now + 20000.0);
		DCO_Perf.Count("veh:at_active");
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.DEBUG, "-", string.Format("veh_at_known veh=%1 latch_s=20", DCO_Log.IdVeh(vehicle)));
		return true;
	}

	protected static bool DetectATNow(IEntity vehicle, SCR_AIUtilityComponent utility)
	{
		IEntity shooter;
		vector pos;
		if (GetATThreat(vehicle, shooter, pos))
			return true;

		vector vp = vehicle.GetOrigin();
		float rangeSq = 350.0 * 350.0;
		if (utility && utility.m_PerceptionComponent)
		{
			GetTargets().Clear();
			GetTargets();
			utility.m_PerceptionComponent.GetTargetsList(s_aTargets, ETargetCategory.ENEMY);
			foreach (BaseTarget t : GetTargets())
			{
				if (!t || t.GetTimeSinceSeen() > 5.0 || vector.DistanceSq(vp, t.GetLastSeenPosition()) >= rangeSq)
					continue;
				if (HasLauncherInHands(t) || IsArmored(TargetVehicle(t.GetTargetEntity())))
				{
					GetTargets().Clear();
					return true;
				}
			}
			GetTargets().Clear();
		}

		if (!utility || !utility.GetAIAgent())
			return false;

		SCR_AIGroup grp = SCR_AIGroup.Cast(utility.GetAIAgent().GetParentGroup());
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (!pm || !grp || !grp.GetGroupUtilityComponent() || !grp.GetGroupUtilityComponent().GetPercGroupComp())
			return false;

		float pmNow = pm.GetTime();
		foreach (SCR_AITargetInfo info : grp.GetGroupUtilityComponent().GetPercGroupComp().m_aTargets)
		{
			if (!info || !info.m_Entity || pmNow - info.m_fTimestamp > 5.0 || vector.DistanceSq(vp, info.m_vWorldPos) >= rangeSq)
				continue;
			if (DCO_Strength.HasWeaponInHands(info.m_Entity, EWeaponType.WT_ROCKETLAUNCHER) || IsArmored(TargetVehicle(info.m_Entity)))
				return true;
		}
		return false;
	}

	static IEntity TargetVehicle(IEntity ent)
	{
		if (!ent)
			return null;
		IEntity veh = GetVehicle(ent);
		if (veh)
			return veh;
		return ent.GetRootParent();
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

		DCO_VehicleATThreat threat = GetATThreatStore().Get(vehicle);
		if (!threat)
		{
			threat = new DCO_VehicleATThreat();
			GetATThreatStore().Set(vehicle, threat);
		}

		if (threat.m_Entity != shooter)
			DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, DCO_Log.IdVeh(vehicle), string.Format("at_threat veh=%1 shooter=%2 dist=%3",
				DCO_Log.IdVeh(vehicle), DCO_Log.IdUnit(shooter), Math.Round(vector.Distance(vehicle.GetOrigin(), pos))));

		threat.m_Entity = shooter;
		threat.m_vPos = pos;
		threat.m_fTime_ms = GetGame().GetWorld().GetWorldTime();
	}

	static bool GetATThreat(IEntity vehicle, out IEntity shooter, out vector pos)
	{
		if (!vehicle)
			return false;

		DCO_VehicleATThreat threat = GetATThreatStore().Get(vehicle);
		if (!threat || !threat.m_Entity)
			return false;

		if (GetGame().GetWorld().GetWorldTime() - threat.m_fTime_ms > 10000.0)
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
		rq.m_fMoveDuration_s = Math.Clamp((250.0 - dist + 50) / SCR_AICombatMoveUtils.GROUND_VEHICLE_GENERIC_SPEED, 3, 12);
		rq.m_bAimAtTarget = false;
		rq.m_bAimAtTargetEnd = false;
		return rq;
	}

	protected static ref map<IEntity, float> s_mATHoldAt;

	static bool ATReact(IEntity vehicle, SCR_AIUtilityComponent utility, SCR_AICombatMoveState driverState, out SCR_AICombatMoveRequest_Move rq)
	{
		rq = null;
		IEntity shooter;
		vector atPos;
		if (!GetATThreat(vehicle, shooter, atPos))
			return false;
		float d = vector.Distance(atPos, vehicle.GetOrigin());
		int cls = DCO_VehicleActive.VehClass(vehicle);
		float hideD = 250.0;
		if (cls > 0)
			hideD = DCO_VehicleActive.HideDist(utility, cls);
		if (d >= hideD)
			return false;
		bool armored = IsArmored(vehicle);
		bool aggr = DCO_VehiclePositioning.Aggr(utility);
		if (cls > 0 || !armored || !aggr || d < 150.0)
		{
			if (CanPushEvade(driverState))
			{
				float hide_s = -1;
				if (cls > 0)
					hide_s = DCO_VehicleActive.HideTime_s(utility);
				rq = DCO_VehiclePositioning.CreateATEvadeRequest(vehicle, atPos, hide_s);
				DCO_Perf.Count("veh:at_evade");
				DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, DCO_Log.IdVeh(vehicle), string.Format("at_evade veh=%1 class=%2 dist=%3 hide_d=%4 hide_s=%5 aggr=%6", DCO_Log.IdVeh(vehicle), DCO_VehicleActive.ClassName(cls), Math.Round(d), Math.Round(hideD), Math.Round(hide_s), aggr));
			}
			return true;
		}
		if (driverState && driverState.IsExecutingRequest())
			return true;
		float now = GetGame().GetWorld().GetWorldTime();
		if (!s_mATHoldAt)
			s_mATHoldAt = new map<IEntity, float>();
		float last = -100000.0;
		s_mATHoldAt.Find(vehicle, last);
		if (now - last < 8000.0)
			return true;
		s_mATHoldAt.Set(vehicle, now);
		rq = new SCR_AICombatMoveRequest_Move();
		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		rq.m_vMovePos = atPos;
		rq.m_vTargetPos = atPos;
		if (Math.RandomInt(0, 2) == 0)
			rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
		else
			rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
		rq.m_fMoveDuration_s = Math.RandomFloat(3.0, 5.0);
		rq.m_bAimAtTarget = false;
		rq.m_bAimAtTargetEnd = false;
		DCO_Perf.Count("veh:at_hold");
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("veh_at_hold veh=%1 dist=%2 shooter=%3 action=lateral_fire", DCO_Log.IdVeh(vehicle), Math.Round(d), DCO_Log.IdUnit(shooter)));
		return true;
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
