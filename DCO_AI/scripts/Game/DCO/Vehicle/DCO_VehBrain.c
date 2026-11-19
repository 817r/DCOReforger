class DCO_VehBrainRec
{
	string m_sState = "IDLE";
	string m_sWhy = "-";
	float m_fStateAt_ms;
	float m_fTick_ms = -1;
	vector m_vGoal;
	string m_sGoalSrc = "-";
	float m_fGoalAt_ms = -1;
	float m_fArriveAt_ms = -1;
	string m_sHeld = "-";
	float m_fHeldAt_ms = -1;
	bool m_bImmobile;
	int m_iWheels;
	int m_iWheelsDead;
	bool m_bAT;
	bool m_bRelentless;
	string m_sPosture = "-";
	vector m_vTgt;
	float m_fTgtD = -1;
	float m_fTgtSeen_s = -1;
	bool m_bTgtVeh;
	float m_fFireAt_ms = -1;
}

class DCO_VehBrain
{
	protected static ref map<IEntity, ref DCO_VehBrainRec> s_mRec;
	protected static map<IEntity, ref DCO_VehBrainRec> GetRecs()
	{
		if (!s_mRec)
			s_mRec = new map<IEntity, ref DCO_VehBrainRec>();
		return s_mRec;
	}
	protected static ref array<HitZone> s_aZones;
	protected static ref array<IEntity> s_aQuery;
	protected static array<IEntity> GetQuery()
	{
		if (!s_aQuery)
			s_aQuery = {};
		return s_aQuery;
	}

	static bool On(SCR_AIUtilityComponent u)
	{
		return DCO_VehMove.On(u);
	}

	static DCO_VehBrainRec Find(IEntity veh)
	{
		if (!veh)
			return null;
		return GetRecs().Get(veh);
	}

	static DCO_VehBrainRec Get(IEntity veh)
	{
		DCO_VehBrainRec r = GetRecs().Get(veh);
		if (r)
			return r;
		if (GetRecs().Count() > 128)
		{
			array<IEntity> stale = {};
			foreach (IEntity k, DCO_VehBrainRec v : GetRecs())
			{
				if (!k)
					stale.Insert(k);
			}
			foreach (IEntity sk : stale)
				GetRecs().Remove(sk);
		}
		r = new DCO_VehBrainRec();
		r.m_fStateAt_ms = GetGame().GetWorld().GetWorldTime();
		GetRecs().Set(veh, r);
		return r;
	}

	static void Tick(SCR_AIUtilityComponent u, IEntity veh)
	{
		if (!u || !veh || (!On(u) && !DCO_VehDbg.IsOn()))
			return;
		DCO_VehBrainRec r = Get(veh);
		if (u.m_AIInfo && u.m_AIInfo.HasUnitState(EUnitState.IN_TURRET) && u.m_CombatComponent)
			NoteGunner(r, u, veh);
		float now = GetGame().GetWorld().GetWorldTime();
		if (r.m_fTick_ms >= 0 && now - r.m_fTick_ms < 900.0)
			return;
		r.m_fTick_ms = now;
		r.m_bAT = DCO_VehicleCombat.IsATKnown(veh, u);
		r.m_bImmobile = Immobile(u, veh, r);
		SCR_AICombatMoveState ds = DCO_VehicleActive.DriverState(veh);
		bool moving = ds && ds.IsExecutingRequest();
		if (r.m_fGoalAt_ms >= 0 && r.m_fArriveAt_ms < 0 && !moving && now - r.m_fGoalAt_ms > 1500.0)
			r.m_fArriveAt_ms = now;
		Classify(r, veh, moving, now);
	}

	protected static void NoteGunner(DCO_VehBrainRec r, SCR_AIUtilityComponent u, IEntity veh)
	{
		r.m_bRelentless = DCO_Relentless.Unit(u);
		r.m_sPosture = typename.EnumToString(DCO_GroupTactics, DCO_VehiclePositioning.GetPosture(u));
		BaseTarget t = u.m_CombatComponent.GetCurrentTarget();
		if (!t || !t.GetTargetEntity())
		{
			r.m_fTgtD = -1;
			return;
		}
		r.m_vTgt = t.GetLastSeenPosition();
		r.m_fTgtD = vector.DistanceXZ(veh.GetOrigin(), r.m_vTgt);
		r.m_fTgtSeen_s = t.GetTimeSinceSeen();
		r.m_bTgtVeh = DCO_VehicleCombat.GetVehicle(t.GetTargetEntity()) != null || Vehicle.Cast(t.GetTargetEntity()) != null;
	}

	static bool Immobile(SCR_AIUtilityComponent u, IEntity veh, DCO_VehBrainRec r)
	{
		SCR_VehicleDamageManagerComponent dmg = SCR_VehicleDamageManagerComponent.Cast(veh.FindComponent(SCR_VehicleDamageManagerComponent));
		if (!dmg)
			return false;
		if (!s_aZones)
			s_aZones = {};
		s_aZones.Clear();
		dmg.GetAllHitZonesInHierarchy(s_aZones);
		int wheels = 0;
		int dead = 0;
		foreach (HitZone hz : s_aZones)
		{
			if (!SCR_WheelHitZone.Cast(hz))
				continue;
			wheels++;
			if (hz.GetDamageState() == EDamageState.DESTROYED)
				dead++;
		}
		s_aZones.Clear();
		if (r)
		{
			r.m_iWheels = wheels;
			r.m_iWheelsDead = dead;
		}
		if (!dmg.GetEngineFunctional())
			return true;
		float limit = DCO_Stress.T(u, "va.immobile_wheels", 2.0);
		return wheels > 0 && dead >= limit;
	}

	protected static void Classify(DCO_VehBrainRec r, IEntity veh, bool moving, float now)
	{
		string st;
		string why = "-";
		DCO_VehActState va = DCO_VehicleActive.Find(veh);
		if (DCO_VehicleActive.CrewAlive(veh) == 0)
			st = "EMPTY";
		else if (r.m_bImmobile)
		{
			st = "IMMOBILE";
			why = string.Format("wheels %1/%2", r.m_iWheelsDead, r.m_iWheels);
		}
		else if (va && va.m_bWithdrawn)
			st = "WITHDRAW";
		else if (DCO_VehiclePositioning.IsHiding(veh))
			st = "HIDE";
		else if (moving)
		{
			st = "MOVE";
			why = r.m_sGoalSrc;
		}
		else if (DriverTravelling(veh))
			st = "TRAVEL";
		else if (va && va.m_fLastLos_ms >= 0 && now - va.m_fLastLos_ms < 3000.0)
			st = "ENGAGE";
		else if (r.m_fTgtD >= 0)
			st = "OVERWATCH";
		else
			st = "IDLE";
		r.m_sWhy = why;
		if (st == r.m_sState)
			return;
		if (DCO_Log.On(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, DCO_Log.IdVeh(veh), string.Format("veh_state veh=%1 from=%2 to=%3 why=%4 after_s=%5 tgt_d=%6 at=%7",
				DCO_Log.IdVeh(veh), r.m_sState, st, why, Math.Round((now - r.m_fStateAt_ms) / 1000.0), Math.Round(r.m_fTgtD), DCO_Util.B01(r.m_bAT)));
		DCO_Perf.Count("vbrain:state_" + st);
		r.m_sState = st;
		r.m_fStateAt_ms = now;
	}

	static SCR_AIUtilityComponent DriverUtility(IEntity veh)
	{
		Vehicle v = Vehicle.Cast(veh);
		if (!v || !v.GetPilot())
			return null;
		AIControlComponent ctrl = AIControlComponent.Cast(v.GetPilot().FindComponent(AIControlComponent));
		if (!ctrl)
			return null;
		SCR_ChimeraAIAgent ag = SCR_ChimeraAIAgent.Cast(ctrl.GetAIAgent());
		if (!ag)
			return null;
		return ag.m_UtilityComponent;
	}

	protected static bool DriverTravelling(IEntity veh)
	{
		SCR_AIUtilityComponent du = DriverUtility(veh);
		if (!du)
			return false;
		SCR_AIBehaviorBase b = SCR_AIBehaviorBase.Cast(du.GetExecutedAction());
		return b && (SCR_AIMoveInFormationBehavior.Cast(b) || SCR_AIMoveIndividuallyBehavior.Cast(b));
	}

	static float StandoffFloor(SCR_AIUtilityComponent u, IEntity veh)
	{
		int cls = DCO_VehicleActive.VehClass(veh);
		if (cls == 0)
			return 0;
		float d = DCO_Stress.T(u, "veh.standoff_m", 200.0);
		if (cls == 2)
			d *= DCO_Stress.T(u, "va.standoff_apc_x", 1.25);
		else if (cls == 3)
			d *= DCO_Stress.T(u, "va.standoff_tank_x", 1.5);
		DCO_VehBrainRec r = Find(veh);
		if (r && r.m_bAT)
			d = Math.Max(d, DCO_VehicleActive.ATStandoff(cls));
		switch (DCO_VehiclePositioning.GetPosture(u))
		{
			case DCO_GroupTactics.AGGRESIVE: return d * 0.6;
			case DCO_GroupTactics.DEFENSIVE: return d * 1.3;
			case DCO_GroupTactics.EVASIVE: return d * 1.5;
		}
		return d;
	}

	static string Accept(SCR_AIUtilityComponent u, SCR_AICombatMoveRequestBase request, string src)
	{
		SCR_AICombatMoveRequest_Move rq = SCR_AICombatMoveRequest_Move.Cast(request);
		if (!rq || !u)
			return "";
		IEntity veh = DCO_VehicleCombat.GetVehicle(u.m_OwnerEntity);
		if (!veh)
			return "";
		DCO_VehBrainRec r = Get(veh);
		float now = GetGame().GetWorld().GetWorldTime();
		string held = Judge(u, veh, r, rq, src, now);
		if (!held.IsEmpty())
		{
			r.m_sHeld = held + " " + src;
			r.m_fHeldAt_ms = now;
			return held;
		}
		vector dest = PredictDest(veh, rq);
		r.m_vGoal = dest;
		r.m_sGoalSrc = src;
		r.m_fGoalAt_ms = now;
		r.m_fArriveAt_ms = -1;
		if (DCO_Log.On(DCO_ELogChannel.VEH, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.DEBUG, DCO_Log.IdVeh(veh), string.Format("veh_brain_accept veh=%1 src=%2 reason=%3 move_d=%4", DCO_Log.IdVeh(veh), src,
				typename.EnumToString(SCR_EAICombatMoveReason, rq.m_eReason), Math.Round(vector.DistanceXZ(veh.GetOrigin(), dest))));
		return "";
	}

	protected static string Judge(SCR_AIUtilityComponent u, IEntity veh, DCO_VehBrainRec r, SCR_AICombatMoveRequest_Move rq, string src, float now)
	{
		if (r.m_bImmobile)
			return "immobile";
		bool urgent = rq.m_eReason == SCR_EAICombatMoveReason.MOVE_FROM_TARGET || rq.m_eReason == SCR_EAICombatMoveReason.MOVE_FROM_DANGER || rq.m_eReason == SCR_EAICombatMoveReason.FF_AVOIDANCE;
		if (src == "va_unstick")
			urgent = true;
		if (urgent)
			return "";
		SCR_EAICombatMoveDirection dir = rq.m_eDirection;
		if (dir == SCR_EAICombatMoveDirection.LEFT || dir == SCR_EAICombatMoveDirection.RIGHT || dir == SCR_EAICombatMoveDirection.ANYWHERE)
			return "random_dir";
		vector tgt = rq.m_vTargetPos;
		if (tgt == vector.Zero && r.m_fTgtD >= 0)
			tgt = r.m_vTgt;
		if (tgt != vector.Zero && !r.m_bRelentless)
		{
			vector dest = PredictDest(veh, rq);
			float floorD = StandoffFloor(u, veh);
			float dNow = vector.DistanceXZ(veh.GetOrigin(), tgt);
			float dNew = vector.DistanceXZ(dest, tgt);
			if (dNew < floorD && dNew < dNow - 5.0 && !EscortNear(u.m_OwnerEntity, dest, DCO_Stress.T(u, "va.escort_m", 80.0)))
				return "too_close";
		}
		return Hold(u, veh, r, now);
	}

	static string WouldHold(SCR_AIUtilityComponent u, IEntity veh)
	{
		if (!u || !veh || !On(u))
			return "";
		DCO_VehBrainRec r = Find(veh);
		if (!r)
			return "";
		if (r.m_bImmobile)
			return "immobile";
		return Hold(u, veh, r, GetGame().GetWorld().GetWorldTime());
	}

	protected static string Hold(SCR_AIUtilityComponent u, IEntity veh, DCO_VehBrainRec r, float now)
	{
		if (!r.m_bAT && r.m_fArriveAt_ms >= 0 && now - r.m_fArriveAt_ms < DCO_Stress.T(u, "va.dwell_s", 20.0) * 1000.0 && !LosLost(veh, now))
			return "dwell";
		if (r.m_fGoalAt_ms >= 0 && now - r.m_fGoalAt_ms < DCO_Stress.T(u, "va.move_gap_s", 8.0) * 1000.0)
			return "gap";
		return "";
	}

	protected static bool LosLost(IEntity veh, float now)
	{
		DCO_VehActState va = DCO_VehicleActive.Find(veh);
		if (!va || va.m_fLastLos_ms < 0)
			return true;
		return now - va.m_fLastLos_ms > 8000.0;
	}

	static vector PredictDest(IEntity veh, SCR_AICombatMoveRequest_Move rq)
	{
		vector here = veh.GetOrigin();
		if (rq.m_eDirection == SCR_EAICombatMoveDirection.CUSTOM_POS)
			return rq.m_vMovePos;
		if (rq.m_eDirection != SCR_EAICombatMoveDirection.FORWARD)
			return here;
		vector dir = rq.m_vMovePos - here;
		dir[1] = 0;
		float full = dir.Length();
		if (full < 1.0)
			return here;
		float step = Math.Min(full, rq.m_fMoveDuration_s * SCR_AICombatMoveUtils.GROUND_VEHICLE_GENERIC_SPEED);
		return here + dir * (step / full);
	}

	protected static bool QueryCb(IEntity ent)
	{
		if (ent && ChimeraCharacter.Cast(ent))
			GetQuery().Insert(ent);
		return true;
	}

	static bool EscortNear(IEntity self, vector pos, float radius)
	{
		if (!self || radius <= 0)
			return false;
		FactionAffiliationComponent selfFac = FactionAffiliationComponent.Cast(self.FindComponent(FactionAffiliationComponent));
		if (!selfFac || !selfFac.GetAffiliatedFaction())
			return false;
		Faction mine = selfFac.GetAffiliatedFaction();
		GetQuery().Clear();
		DCO_Perf.Count("q:DCO_VehEscort");
		GetGame().GetWorld().QueryEntitiesBySphere(pos, radius, null, QueryCb, EQueryEntitiesFlags.DYNAMIC);
		bool found = false;
		foreach (IEntity ent : GetQuery())
		{
			ChimeraCharacter ch = ChimeraCharacter.Cast(ent);
			if (!ch || ch == self || ch.IsInVehicle() || !DCO_Util.IsAlive(ch))
				continue;
			FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(ch.FindComponent(FactionAffiliationComponent));
			if (fac && fac.GetAffiliatedFaction() == mine)
			{
				found = true;
				break;
			}
		}
		GetQuery().Clear();
		return found;
	}

	static bool KeepCrew(SCR_AIUtilityComponent u, IEntity veh, string cause)
	{
		if (!u || !veh || !On(u) || !Vehicle.Cast(veh) || !DCO_VehicleCombat.HasTurret(veh))
			return false;
		if (cause != "group_getout" && cause != "msg_dismount" && cause != "vehicle_combat" && cause != "turret_dismount" && cause != "bail_immobile")
			return false;
		string seat = DCO_VehExit.Seat(u.m_OwnerEntity);
		if (seat != "driver" && seat != "gunner")
			return false;
		SCR_VehicleDamageManagerComponent dmg = SCR_VehicleDamageManagerComponent.Cast(veh.FindComponent(SCR_VehicleDamageManagerComponent));
		if (!dmg || dmg.IsDestroyed() || dmg.IsOnFire() || dmg.GetHealthScaled() < DCO_Stress.T(u, "va.bail_hp", 0.25) || dmg.GetAimingDamage() >= 0.9)
			return false;
		bool armored = DCO_VehicleCombat.IsArmored(veh);
		if (cause == "turret_dismount" && !armored)
			return false;
		if (seat == "driver" && !armored && Immobile(u, veh, null))
			return false;
		DCO_Perf.Count("vbrain:keep_crew_" + cause);
		return true;
	}

	static float FireWait_ms(SCR_AIUtilityComponent u, IEntity veh)
	{
		if (DCO_VehicleCombat.IsArmored(veh))
			return DCO_Stress.T(u, "va.fire_wait_armor_s", 15.0) * 1000.0;
		return DCO_Stress.T(u, "va.fire_wait_soft_s", 5.0) * 1000.0;
	}
}
