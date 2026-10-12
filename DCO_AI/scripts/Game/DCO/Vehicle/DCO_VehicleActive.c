class DCO_VehActState
{
	float m_fLastLos_ms = -1;
	float m_fFireSince_ms = -1;
	float m_fExpose_s = 20;
	vector m_vLastFire;
	vector m_vAnchor;
	float m_fNextSeek_ms;
	float m_fNextRecon_ms;
	ref array<float> m_aRecon = {};
	ref array<float> m_aRockets = {};
	bool m_bWithdrawn;
	bool m_bHid;
	float m_fHideStart_ms = -1;
	float m_fIdleSince_ms = -1;
	float m_fNextDiag_ms;
	int m_iScoots;
	string m_sIdle = "-";
}

class DCO_VehicleActive
{
	protected static ref map<IEntity, ref DCO_VehActState> s_mState;
	protected static map<IEntity, ref DCO_VehActState> GetState()
	{
		if (!s_mState)
			s_mState = new map<IEntity, ref DCO_VehActState>();
		return s_mState;
	}
	protected static ref array<BaseTarget> s_aT;
	protected static array<BaseTarget> GetT()
	{
		if (!s_aT)
			s_aT = {};
		return s_aT;
	}

	static int VehClass(IEntity veh)
	{
		if (!veh || !DCO_VehicleCombat.HasTurret(veh))
			return 0;
		PerceivableComponent perc = PerceivableComponent.Cast(veh.FindComponent(PerceivableComponent));
		if (perc && perc.GetUnitType() == EAIUnitType.UnitType_VehicleHeavy)
			return 3;
		if (DCO_VehicleCombat.IsArmored(veh))
			return 2;
		return 1;
	}

	static string ClassName(int cls)
	{
		switch (cls)
		{
			case 1: return "SOFT";
			case 2: return "APC";
			case 3: return "TANK";
		}
		return "NONE";
	}

	static float HideDist(SCR_AIUtilityComponent u, int cls)
	{
		float d = DCO_Stress.T(u, "va.hide_soft_m", 120.0);
		if (cls == 2)
			d = DCO_Stress.T(u, "va.hide_apc_m", 80.0);
		else if (cls == 3)
			d = DCO_Stress.T(u, "va.hide_tank_m", 50.0);
		DCO_GroupTactics p = DCO_VehiclePositioning.GetPosture(u);
		if (p == DCO_GroupTactics.DEFENSIVE || p == DCO_GroupTactics.EVASIVE)
			d *= 1.3;
		return d;
	}

	static float ScootMax(SCR_AIUtilityComponent u, int cls)
	{
		if (cls == 2)
			return DCO_Stress.T(u, "va.scoot_apc_m", 250.0);
		if (cls == 3)
			return DCO_Stress.T(u, "va.scoot_tank_m", 200.0);
		return DCO_Stress.T(u, "va.scoot_soft_m", 300.0);
	}

	static float ExposeTime_s(SCR_AIUtilityComponent u, int cls)
	{
		float c = DCO_Stress.T(u, "va.expose_soft_s", 15.0);
		if (cls == 2)
			c = DCO_Stress.T(u, "va.expose_apc_s", 12.0);
		else if (cls == 3)
			c = DCO_Stress.T(u, "va.expose_tank_s", 15.0);
		return c * Math.RandomFloat(0.75, 1.25);
	}

	static float HideTime_s(SCR_AIUtilityComponent u)
	{
		float lo = DCO_Stress.T(u, "va.hide_min_s", 20.0);
		float hi = DCO_Stress.T(u, "va.hide_max_s", 40.0);
		return Math.RandomFloat(Math.Min(lo, hi), Math.Max(lo, hi));
	}

	static float ATStandoff(int cls)
	{
		if (cls == 2)
			return 250.0;
		if (cls == 3)
			return 300.0;
		return 150.0;
	}

	static float EffMin(int cls)
	{
		if (cls == 2)
			return 150.0;
		if (cls == 3)
			return 200.0;
		return 100.0;
	}

	static float EffMax(int cls)
	{
		if (cls == 2)
			return 600.0;
		if (cls == 3)
			return 1200.0;
		return 400.0;
	}

	static DCO_VehActState Get(IEntity veh)
	{
		DCO_VehActState st = GetState().Get(veh);
		if (!st)
		{
			if (GetState().Count() > 128)
			{
				array<IEntity> stale = {};
				foreach (IEntity k, DCO_VehActState v : GetState())
				{
					if (!k)
						stale.Insert(k);
				}
				foreach (IEntity sk : stale)
					GetState().Remove(sk);
			}
			st = new DCO_VehActState();
			GetState().Set(veh, st);
		}
		return st;
	}

	static SCR_AICombatMoveState DriverState(IEntity veh)
	{
		BaseCompartmentManagerComponent mgr = BaseCompartmentManagerComponent.Cast(veh.FindComponent(BaseCompartmentManagerComponent));
		if (!mgr)
			return null;
		array<BaseCompartmentSlot> slots = {};
		mgr.GetCompartments(slots);
		foreach (BaseCompartmentSlot slot : slots)
		{
			if (!PilotCompartmentSlot.Cast(slot))
				continue;
			IEntity occ = slot.GetOccupant();
			if (!occ || !DCO_Util.IsAlive(occ))
				return null;
			AIControlComponent ctrl = AIControlComponent.Cast(occ.FindComponent(AIControlComponent));
			if (!ctrl)
				return null;
			SCR_ChimeraAIAgent ag = SCR_ChimeraAIAgent.Cast(ctrl.GetAIAgent());
			if (!ag || !ag.m_UtilityComponent)
				return null;
			return ag.m_UtilityComponent.m_CombatMoveState;
		}
		return null;
	}

	static int CrewAlive(IEntity veh)
	{
		BaseCompartmentManagerComponent mgr = BaseCompartmentManagerComponent.Cast(veh.FindComponent(BaseCompartmentManagerComponent));
		if (!mgr)
			return 0;
		array<BaseCompartmentSlot> slots = {};
		mgr.GetCompartments(slots);
		int n;
		foreach (BaseCompartmentSlot slot : slots)
		{
			if (!PilotCompartmentSlot.Cast(slot) && !TurretCompartmentSlot.Cast(slot))
				continue;
			IEntity occ = slot.GetOccupant();
			if (occ && DCO_Util.IsAlive(occ))
				n++;
		}
		return n;
	}

	static bool CarriesTroops(IEntity veh, IEntity crew)
	{
		SCR_AIGroup crewGrp = DCO_GroupFeatures.GroupOf(crew);
		bool own;
		BaseCompartmentManagerComponent mgr = BaseCompartmentManagerComponent.Cast(veh.FindComponent(BaseCompartmentManagerComponent));
		if (!mgr)
			return false;
		array<BaseCompartmentSlot> slots = {};
		mgr.GetCompartments(slots);
		foreach (BaseCompartmentSlot slot : slots)
		{
			if (!CargoCompartmentSlot.Cast(slot))
				continue;
			IEntity occ = slot.GetOccupant();
			if (!occ || !DCO_Util.IsAlive(occ))
				continue;
			if (crewGrp && DCO_GroupFeatures.GroupOf(occ) == crewGrp)
			{
				own = true;
				continue;
			}
			return true;
		}
		if (own)
			DCO_Perf.Count("veh:active_own_cargo");
		return false;
	}

	static void NoteRocket(IEntity veh)
	{
		if (!veh)
			return;
		DCO_VehActState st = Get(veh);
		float now = GetGame().GetWorld().GetWorldTime();
		if (!st.m_aRockets.IsEmpty() && now - st.m_aRockets[st.m_aRockets.Count() - 1] < 2000.0)
			return;
		st.m_aRockets.Insert(now);
		if (st.m_aRockets.Count() > 6)
			st.m_aRockets.Remove(0);
	}

	static void NoteFirePos(IEntity veh, vector pos)
	{
		DCO_VehActState st = Get(veh);
		st.m_vLastFire = pos;
		st.m_fFireSince_ms = -1;
	}

	protected static BaseTarget BestKnown(SCR_AIUtilityComponent u, IEntity veh)
	{
		if (!u.m_PerceptionComponent)
			return null;
		GetT().Clear();
		GetT();
		u.m_PerceptionComponent.GetTargetsList(s_aT, ETargetCategory.ENEMY);
		BaseTarget best;
		float bestD = float.MAX;
		vector vp = veh.GetOrigin();
		foreach (BaseTarget t : GetT())
		{
			if (!t || !t.GetTargetEntity() || t.GetTimeSinceSeen() > 60.0 || !DCO_Util.IsAlive(t.GetTargetEntity()))
				continue;
			float d = vector.DistanceXZ(vp, t.GetLastSeenPosition());
			if (t.GetTimeSinceSeen() < 2.0)
				d -= 10000.0;
			if (d < bestD)
			{
				bestD = d;
				best = t;
			}
		}
		return best;
	}

	protected static bool GroupIntel(SCR_AIUtilityComponent u, IEntity veh, out vector pos, out float age)
	{
		AIAgent ag = u.GetAIAgent();
		if (!ag || !ag.GetParentGroup())
			return false;
		SCR_AIGroupUtilityComponent gu = SCR_AIGroupUtilityComponent.Cast(ag.GetParentGroup().FindComponent(SCR_AIGroupUtilityComponent));
		if (!gu || !gu.m_Perception)
			return false;
		float bestD = float.MAX;
		vector vp = veh.GetOrigin();
		foreach (SCR_AIGroupTargetCluster c : gu.m_Perception.m_aTargetClusters)
		{
			if (!c || !c.m_State || c.m_State.m_iCountAlive <= 0 || c.m_State.GetTimeSinceLastNewInformation() > 60.0)
				continue;
			vector cp = c.m_State.GetCenterPosition();
			float d = vector.DistanceXZ(vp, cp);
			if (d < bestD)
			{
				bestD = d;
				pos = cp;
				age = c.m_State.GetTimeSinceLastNewInformation();
			}
		}
		return bestD < float.MAX;
	}

	protected static vector FriendPos(SCR_AIUtilityComponent u)
	{
		AIAgent ag = u.GetAIAgent();
		if (!ag || !ag.GetParentGroup())
			return vector.Zero;
		DCO_GroupUtilityComponent dg = DCO_GroupUtilityComponent.Cast(ag.GetParentGroup().FindComponent(DCO_GroupUtilityComponent));
		if (!dg || !dg.m_DCOPair || dg.m_DCOPair.m_aInf.IsEmpty() || !dg.m_DCOPair.m_aInf[0])
			return vector.Zero;
		SCR_AIGroup ig = SCR_AIGroup.Cast(dg.m_DCOPair.m_aInf[0].GetOwner());
		if (!ig || !ig.GetLeaderEntity())
			return vector.Zero;
		return ig.GetLeaderEntity().GetOrigin();
	}

	protected static bool Confirmed(BaseTarget t)
	{
		IEntity e = t.GetTargetEntity();
		if (!e)
			return false;
		if (DCO_VehicleCombat.TargetVehicle(e))
			return true;
		if (SCR_CoverManagerComponent.IsEntityInsideBuilding(e))
			return true;
		return DCO_Util.IsMGShooter(e);
	}

	static float AmmoFrac(SCR_AIUtilityComponent u)
	{
		BaseWeaponComponent w;
		int muzzle;
		u.m_CombatComponent.GetSelectedWeapon(w, muzzle);
		if (!w)
			return 1.0;
		BaseMagazineComponent mag = w.GetCurrentMagazine();
		if (!mag || mag.GetMaxAmmoCount() <= 0)
			return 1.0;
		float ammo = mag.GetAmmoCount();
		return ammo / mag.GetMaxAmmoCount();
	}

	protected static void Bounds(SCR_AIUtilityComponent u, IEntity veh, DCO_VehActState st, out vector c, out float r)
	{
		float extra = DCO_Stress.T(u, "va.bound_extra_m", 250.0);
		c = st.m_vAnchor;
		r = 300.0;
		AIAgent ag = u.GetAIAgent();
		if (!ag || !ag.GetParentGroup())
			return;
		AIGroup g = ag.GetParentGroup();
		DCO_GroupUtilityComponent dg = DCO_GroupUtilityComponent.Cast(g.FindComponent(DCO_GroupUtilityComponent));
		if (dg && dg.m_DCOPair && g.GetCurrentWaypoint())
		{
			c = g.GetCurrentWaypoint().GetOrigin();
			r = DCO_Stress.T(u, "va.pair_radius_m", 80.0);
			return;
		}
		if (dg && dg.GetGroupObjective() && dg.GetGroupObjective().GetOwner())
		{
			c = dg.GetGroupObjective().GetOwner().GetOrigin();
			r = dg.GetGroupObjective().GetRadius() + extra;
			return;
		}
		if (g.GetCurrentWaypoint())
		{
			c = g.GetCurrentWaypoint().GetOrigin();
			r = Math.Max(g.GetCurrentWaypoint().GetCompletionRadius(), 60.0) + extra;
		}
	}

	static void Tick(SCR_AIUtilityComponent u, IEntity veh)
	{
		if (!u || !veh || !u.m_CombatComponent || DCO_Stress.T(u, "va.enabled", 1.0) < 0.5)
			return;
		if (!u.m_AIInfo || !u.m_AIInfo.HasUnitState(EUnitState.IN_TURRET))
			return;
		int cls = VehClass(veh);
		if (cls == 0)
			return;
		SCR_AICombatMoveState ds = DriverState(veh);
		if (!ds)
			return;
		if (CarriesTroops(veh, u.m_OwnerEntity))
		{
			DCO_Perf.Count("veh:active_skip_troops");
			return;
		}
		DCO_VehActState st = Get(veh);
		float now = GetGame().GetWorld().GetWorldTime();

		BaseTarget cur = u.m_CombatComponent.GetCurrentTarget();
		bool vis = cur && cur.GetTimeSinceSeen() < 2.0;
		if (vis)
		{
			st.m_fLastLos_ms = now;
			if (st.m_fFireSince_ms < 0)
				st.m_fFireSince_ms = now;
			st.m_fIdleSince_ms = -1;
		}

		BaseTarget known = BestKnown(u, veh);
		if (!known)
		{
			st.m_fFireSince_ms = -1;
			vector gpos;
			float gage;
			if (!GroupIntel(u, veh, gpos, gage))
			{
				st.m_fIdleSince_ms = -1;
				return;
			}
			float gd = vector.DistanceXZ(veh.GetOrigin(), gpos);
			if (st.m_vAnchor == vector.Zero)
				st.m_vAnchor = veh.GetOrigin();
			if (gd <= EffMax(cls) * 1.5 && now >= st.m_fNextSeek_ms && !st.m_bWithdrawn && !ds.IsExecutingRequest() && !DCO_VehiclePositioning.IsHiding(veh))
			{
				DCO_Perf.Count("veh:seek_group_intel");
				Seek(u, veh, ds, st, cls, gpos, true, "group_intel", now);
				return;
			}
			string gwhy = "group_intel_far";
			if (gd <= EffMax(cls) * 1.5)
				gwhy = "group_intel_wait";
			if (!ds.IsExecutingRequest())
				Recon(u, veh, st, gpos, gage, false, "group", cls, now);
			Idle(u, veh, st, now, gwhy, gd, cls);
			return;
		}
		vector lkp = known.GetLastSeenPosition();
		float d = vector.DistanceXZ(veh.GetOrigin(), lkp);
		if (st.m_vAnchor == vector.Zero)
			st.m_vAnchor = veh.GetOrigin();

		if (!st.m_bWithdrawn && Withdraw(u, veh, ds, st, cls, lkp, now))
			return;

		if (DCO_VehiclePositioning.IsHiding(veh))
		{
			if (!st.m_bHid)
			{
				st.m_bHid = true;
				st.m_fHideStart_ms = now;
			}
			st.m_fFireSince_ms = -1;
			Idle(u, veh, st, now, "hidden", d, cls);
			return;
		}

		IEntity atShooter;
		vector atPos;
		bool at = DCO_VehicleCombat.GetATThreat(veh, atShooter, atPos) && vector.DistanceXZ(atPos, veh.GetOrigin()) < ScootMax(u, cls);

		if (st.m_bHid && !ds.IsExecutingRequest())
		{
			st.m_bHid = false;
			float hidden_s = (now - st.m_fHideStart_ms) / 1000.0;
			bool ok = Seek(u, veh, ds, st, cls, lkp, at, "reemerge", now);
			DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, DCO_Log.IdVeh(veh), string.Format("veh_reemerge veh=%1 class=%2 hidden_s=%3 found=%4 tgt_d=%5", DCO_Log.IdVeh(veh), ClassName(cls), Math.Round(hidden_s), DCO_Util.B01(ok), Math.Round(d)));
			return;
		}

		if (ds.IsExecutingRequest())
			return;

		if (at && vis && st.m_fFireSince_ms > 0 && now - st.m_fFireSince_ms > st.m_fExpose_s * 1000.0)
		{
			float held_s = (now - st.m_fFireSince_ms) / 1000.0;
			if (Seek(u, veh, ds, st, cls, lkp, true, "scoot", now))
			{
				st.m_iScoots++;
				DCO_Perf.Count("veh:scoot");
				DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, DCO_Log.IdVeh(veh), string.Format("veh_scoot veh=%1 class=%2 held_s=%3 at_d=%4 scoots=%5", DCO_Log.IdVeh(veh), ClassName(cls), Math.Round(held_s), Math.Round(vector.DistanceXZ(atPos, veh.GetOrigin())), st.m_iScoots));
			}
			return;
		}

		if (!vis && (st.m_fLastLos_ms < 0 || now - st.m_fLastLos_ms > 8000.0) && d <= EffMax(cls) * 1.5 && now >= st.m_fNextSeek_ms && !st.m_bWithdrawn)
		{
			Seek(u, veh, ds, st, cls, lkp, at, "no_los", now);
			return;
		}

		if (!vis)
		{
			Recon(u, veh, st, known.GetLastSeenPosition(), known.GetTimeSinceSeen(), Confirmed(known), "own", cls, now);
			Idle(u, veh, st, now, "no_los", d, cls);
		}
	}

	protected static bool Seek(SCR_AIUtilityComponent u, IEntity veh, SCR_AICombatMoveState ds, DCO_VehActState st, int cls, vector lkp, bool at, string trigger, float now)
	{
		st.m_fNextSeek_ms = now + 4000.0;
		vector c;
		float r;
		Bounds(u, veh, st, c, r);
		float boundR = r;
		float vc = vector.DistanceXZ(veh.GetOrigin(), c);
		if (vc > r)
		{
			r = vc;
			DCO_Perf.Count("veh:seek_outside_bound");
		}
		float d = vector.DistanceXZ(veh.GetOrigin(), lkp);
		float desired = Math.Clamp(d, EffMin(cls), EffMax(cls) * 0.7);
		if (at)
			desired = Math.Max(desired, ATStandoff(cls));
		vector pos;
		int checks;
		bool found = DCO_VehiclePositioning.FindFirePositionEx(veh, lkp, desired, DCO_VehiclePositioning.GetHullDownWeight(DCO_VehiclePositioning.GetPosture(u)), c, r, st.m_vLastFire, true, pos, checks, FriendPos(u));
		string action = "firepos";
		if (found)
		{
			ds.ApplyNewRequest(DCO_VehiclePositioning.CreateMoveRequest(veh.GetOrigin(), pos, lkp, SCR_EAICombatMoveReason.STANDARD));
			st.m_vLastFire = pos;
			st.m_fFireSince_ms = -1;
			st.m_fExpose_s = ExposeTime_s(u, cls);
		}
		else if (!at)
		{
			vector dir = lkp - veh.GetOrigin();
			dir[1] = 0;
			if (dir.Length() > 1.0)
			{
				pos = DCO_Util.Surface(veh.GetOrigin() + dir.Normalized() * Math.RandomFloat(30.0, 50.0));
				ds.ApplyNewRequest(DCO_VehiclePositioning.CreateMoveRequest(veh.GetOrigin(), pos, lkp, SCR_EAICombatMoveReason.STANDARD));
				action = "advance";
			}
			else
				action = "none";
		}
		else
		{
			st.m_fNextSeek_ms = now + 10000.0;
			action = "wait_at";
		}
		DCO_Perf.Count("veh:seek_" + action);
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, DCO_Log.IdVeh(veh), string.Format("veh_firepos_seek veh=%1 class=%2 trigger=%3 checks=%4 found=%5 action=%6 move=%7 tgt_d=%8 at=%9",
			DCO_Log.IdVeh(veh), ClassName(cls), trigger, checks, DCO_Util.B01(found), action, Math.Round(vector.DistanceXZ(veh.GetOrigin(), pos)), Math.Round(d), DCO_Util.B01(at)) + string.Format(" bound_r=%1 bound_d=%2", Math.Round(boundR), Math.Round(vc)));
		return found || action == "advance";
	}

	protected static bool Withdraw(SCR_AIUtilityComponent u, IEntity veh, SCR_AICombatMoveState ds, DCO_VehActState st, int cls, vector lkp, float now)
	{
		SCR_VehicleDamageManagerComponent dmg = SCR_VehicleDamageManagerComponent.Cast(veh.FindComponent(SCR_VehicleDamageManagerComponent));
		float hp = 1.0;
		if (dmg)
			hp = dmg.GetHealthScaled();
		for (int i = st.m_aRockets.Count() - 1; i >= 0; i--)
		{
			if (now - st.m_aRockets[i] > 60000.0)
				st.m_aRockets.Remove(i);
		}
		int crew = CrewAlive(veh);
		string why;
		if (hp < DCO_Stress.T(u, "va.withdraw_hp", 0.35))
			why = "hp";
		else if (st.m_aRockets.Count() >= 2)
			why = "rockets";
		else if (crew <= 1 && cls > 1)
			why = "crew";
		if (why.IsEmpty())
			return false;
		st.m_bWithdrawn = true;
		IEntity atShooter;
		vector threat = lkp;
		vector atPos;
		if (DCO_VehicleCombat.GetATThreat(veh, atShooter, atPos))
			threat = atPos;
		vector pos = DCO_Stress.AwayPoint(veh, threat, Math.RandomFloat(150.0, 300.0), 30.0);
		ds.ApplyNewRequest(DCO_VehiclePositioning.CreateMoveRequest(veh.GetOrigin(), pos, threat, SCR_EAICombatMoveReason.MOVE_FROM_TARGET));
		DCO_VehiclePositioning.MarkHiding(veh, 60.0);
		DCO_Perf.Count("veh:withdraw_" + why);
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, DCO_Log.IdVeh(veh), string.Format("veh_withdraw veh=%1 class=%2 reason=%3 hp=%4 crew=%5 rockets=%6 dist=%7",
			DCO_Log.IdVeh(veh), ClassName(cls), why, hp.ToString(-1, 2), crew, st.m_aRockets.Count(), Math.Round(vector.DistanceXZ(veh.GetOrigin(), pos))));
		return true;
	}

	protected static void Recon(SCR_AIUtilityComponent u, IEntity veh, DCO_VehActState st, vector lkp, float age, bool confirmed, string src, int cls, float now)
	{
		if (now < st.m_fNextRecon_ms || DCO_Stress.T(u, "va.recon_on", 1.0) < 0.5)
			return;
		if (cls == 3 && !confirmed)
			return;
		if (!DCO_GlobalAIComponent.BehaviorOnUtility(u, DCO_EBehavior.SUPPRESS_LAST_KNOWN))
			return;
		if (age < 5.0 || age > 60.0)
			return;
		if (vector.DistanceXZ(veh.GetOrigin(), lkp) > EffMax(cls))
			return;
		string skip;
		if (u.m_CombatComponent.GetCombatMode() == EAIGroupCombatMode.HOLD_FIRE)
			skip = "roe";
		else if (AmmoFrac(u) < DCO_Stress.T(u, "va.recon_ammo_floor", 0.3))
			skip = "ammo";
		else if (u.FindActionOfType(SCR_AISuppressBehavior))
			skip = "busy";
		for (int i = st.m_aRecon.Count() - 1; i >= 0; i--)
		{
			if (now - st.m_aRecon[i] > 60000.0)
				st.m_aRecon.Remove(i);
		}
		float perMin = DCO_Stress.T(u, "va.recon_soft_per_min", 8.0);
		if (cls == 2)
			perMin = DCO_Stress.T(u, "va.recon_apc_per_min", 5.0);
		else if (cls == 3)
			perMin = 2;
		if (skip.IsEmpty() && st.m_aRecon.Count() >= perMin)
			skip = "budget";
		vector pt = DCO_Util.Surface(lkp + Vector(Math.RandomFloat(-5.0, 5.0), 0, Math.RandomFloat(-5.0, 5.0)));
		if (skip.IsEmpty() && DCO_Util.HasFriendlyNear(u.m_OwnerEntity, pt, 30.0, "q:DCO_VehRecon"))
			skip = "lane";
		if (!skip.IsEmpty())
		{
			st.m_fNextRecon_ms = now + 3000.0;
			DCO_Perf.Count("veh:recon_skip_" + skip);
			return;
		}
		float dur = 2.5;
		float gapLo = 4.0;
		float gapHi = 8.0;
		if (cls == 2)
		{
			dur = 1.5;
			gapLo = 8.0;
			gapHi = 15.0;
		}
		else if (cls == 3)
		{
			dur = 1.0;
			gapLo = 20.0;
			gapHi = 30.0;
		}
		vector bbMin, bbMax;
		SCR_AISuppressionVolumeBase.CreateSuppressionBox(pt, 4.0, 3, bbMin, bbMax);
		SCR_AISuppressionObjectVolumeBox vol = new SCR_AISuppressionObjectVolumeBox(bbMin, bbMax);
		u.AddAction(new SCR_AISuppressBehavior(u, null, vol, dur, 1.5));
		st.m_aRecon.Insert(now);
		st.m_fNextRecon_ms = now + Math.RandomFloat(gapLo, gapHi) * 1000.0;
		DCO_Perf.Count("veh:recon_fire");
		DCO_Perf.Count("veh:recon_fire_" + src);
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, DCO_Log.IdVeh(veh), string.Format("veh_recon_fire veh=%1 class=%2 lkp_age_s=%3 dist=%4 bursts_min=%5 src=%6",
			DCO_Log.IdVeh(veh), ClassName(cls), Math.Round(age), Math.Round(vector.DistanceXZ(veh.GetOrigin(), pt)), st.m_aRecon.Count(), src));
	}

	static string CensusLine()
	{
		int armed;
		int firing;
		int hidden;
		int idle;
		int withdrawn;
		int scoots;
		float now = GetGame().GetWorld().GetWorldTime();
		foreach (IEntity v, DCO_VehActState st : GetState())
		{
			if (!v || !st)
				continue;
			armed++;
			scoots += st.m_iScoots;
			if (st.m_bWithdrawn)
				withdrawn++;
			else if (DCO_VehiclePositioning.IsHiding(v))
				hidden++;
			else if (st.m_fLastLos_ms >= 0 && now - st.m_fLastLos_ms < 3000.0)
				firing++;
			else if (st.m_fIdleSince_ms >= 0)
				idle++;
		}
		return string.Format("CENSUS veh armed=%1 firing=%2 hidden=%3 idle=%4 withdrawn=%5 scoots_total=%6", armed, firing, hidden, idle, withdrawn, scoots);
	}

	protected static void Idle(SCR_AIUtilityComponent u, IEntity veh, DCO_VehActState st, float now, string why, float d, int cls)
	{
		if (st.m_fIdleSince_ms < 0)
			st.m_fIdleSince_ms = now;
		st.m_sIdle = why;
		if (now - st.m_fIdleSince_ms < 20000.0 || now < st.m_fNextDiag_ms)
			return;
		st.m_fNextDiag_ms = now + 30000.0;
		DCO_Perf.Count("veh:idle_" + why);
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, DCO_Log.IdVeh(veh), string.Format("veh_idle_diag veh=%1 class=%2 idle_s=%3 reason=%4 tgt_d=%5 withdrawn=%6",
			DCO_Log.IdVeh(veh), ClassName(cls), Math.Round((now - st.m_fIdleSince_ms) / 1000.0), why, Math.Round(d), DCO_Util.B01(st.m_bWithdrawn)));
	}
}
