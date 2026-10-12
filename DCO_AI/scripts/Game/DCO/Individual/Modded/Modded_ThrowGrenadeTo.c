modded class SCR_AIThrowGrenadeToBehavior : SCR_AIBehaviorBase
{
	protected bool m_bDCOSmokeChecked;
	protected bool m_bDCOSelected;
	protected IEntity m_DCOHeldGren;

	override float CustomEvaluate()
	{
		if (m_bDCOSelected && m_Utility && m_Utility.m_OwnerEntity && e_WeaponType.m_Value == EWeaponType.WT_FRAGGRENADE)
		{
			BaseWeaponManagerComponent wm = BaseWeaponManagerComponent.Cast(m_Utility.m_OwnerEntity.FindComponent(BaseWeaponManagerComponent));
			if (wm && wm.GetCurrentGrenade())
				m_DCOHeldGren = wm.GetCurrentGrenade().GetOwner();
		}
		float age = GetGame().GetWorld().GetWorldTime() - m_fStartTime;
		if (age > 3000.0 && (!m_bDCOSelected || age > 10000.0))
		{
			DCO_Perf.Count("throw_timeout");
			Fail();
			return 0;
		}

		if (!m_bDCOSmokeChecked)
		{
			m_bDCOSmokeChecked = true;
			if (e_WeaponType.m_Value == EWeaponType.WT_SMOKEGRENADE && DCO_SmokeIntoBuilding())
			{
				DCO_Perf.Count("smoke_indoor_block");
				Fail();
				return 0;
			}
			if (e_WeaponType.m_Value == EWeaponType.WT_SMOKEGRENADE && DCO_AssaultLock.SmokeBlocked(m_Utility, "throw_behavior"))
			{
				Fail();
				return 0;
			}
		}

		return super.CustomEvaluate();
	}

	override void OnActionSelected()
	{
		super.OnActionSelected();
		m_bDCOSelected = true;
	}

	override void OnActionCompleted()
	{
		super.OnActionCompleted();
		if (DCO_Util.Teardown("throw_completed"))
			return;
		if (e_WeaponType.m_Value == EWeaponType.WT_SMOKEGRENADE)
			DCO_SmokeField.Add(null, m_vTargetPosition.m_Value, "ai_throw", 3.0, 55.0);
		if (e_WeaponType.m_Value == EWeaponType.WT_FRAGGRENADE && m_Utility && m_Utility.m_OwnerEntity)
		{
			float d = vector.DistanceXZ(m_Utility.m_OwnerEntity.GetOrigin(), m_vTargetPosition.m_Value);
			DCO_Perf.Count("gren:thrown_frag");
			DCO_GrenadeWatch.OnThrown(m_Utility.m_OwnerEntity);
			DCO_PostFrag.OnThrow(m_Utility, m_vTargetPosition.m_Value);
			if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("gren_thrown unit=%1 dist=%2 cook=%3 target=%4",
					DCO_Log.IdUnit(m_Utility.m_OwnerEntity), Math.Round(d), d <= 20.0, DCO_Log.Pos(m_vTargetPosition.m_Value)));
			GetGame().GetCallqueue().CallLater(DCO_Cook, 100, false, m_Utility.m_OwnerEntity, d, 0, m_vTargetPosition.m_Value, m_DCOHeldGren);
		}
		DCO_RestoreWeapon();
	}

	override void OnActionFailed()
	{
		super.OnActionFailed();
		if (m_Utility && e_WeaponType.m_Value == EWeaponType.WT_FRAGGRENADE)
			DCO_GrenadeWatch.OnFailed(m_Utility.m_OwnerEntity);
		if (!DCO_Util.Teardown("throw_failed"))
			DCO_RestoreWeapon();
	}

	protected static ref array<IEntity> s_aDCOCook;
	protected static ref array<IEntity> s_aDCOCooked;

	protected static bool DCO_CookCollect(IEntity e)
	{
		if (!e.GetParent() && e.FindComponent(TimerTriggerComponent))
			s_aDCOCook.Insert(e);
		return true;
	}

	static void DCO_Cook(IEntity me, float dist, int tries, vector aim, IEntity held)
	{
		if (!me)
			return;
		if (!s_aDCOCook)
			s_aDCOCook = {};
		s_aDCOCook.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(me.GetOrigin() + Vector(0, 1.0, 0), 8.0, DCO_CookCollect, null, EQueryEntitiesFlags.ALL);
		if (!s_aDCOCooked)
			s_aDCOCooked = {};
		for (int i = s_aDCOCooked.Count() - 1; i >= 0; i--)
		{
			if (!s_aDCOCooked[i])
				s_aDCOCooked.Remove(i);
		}
		IEntity gren;
		if (held && !held.GetParent() && !s_aDCOCooked.Contains(held) && held.FindComponent(TimerTriggerComponent))
			gren = held;
		int nCand = s_aDCOCook.Count();
		int nSlow;
		foreach (IEntity e : s_aDCOCook)
		{
			if (gren)
				break;
			if (s_aDCOCooked.Contains(e))
				continue;
			Physics ph = e.GetPhysics();
			if (ph && ph.IsDynamic() && ph.GetVelocity().Length() < 1.5)
			{
				nSlow++;
				continue;
			}
			gren = e;
			break;
		}
		s_aDCOCook.Clear();
		if (!gren)
		{
			if (tries < 4)
			{
				GetGame().GetCallqueue().CallLater(DCO_Cook, 250, false, me, dist, tries + 1, aim, held);
				return;
			}
			DCO_Perf.Count("gren:cook_nofind");
			if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("gren_cook_nofind unit=%1 dist=%2 candidates=%3 slow=%4 held=%5 held_parent=%6", DCO_Log.IdUnit(me), Math.Round(dist), nCand, nSlow, held != null, held && held.GetParent() != null));
			return;
		}
		s_aDCOCooked.Insert(gren);
		GetGame().GetCallqueue().CallLater(DCO_TrackLand, 100, false, gren, me, aim, gren.GetOrigin(), 0, dist);
		if (dist > 20.0)
			return;
		if (DCO_GrenadeWatch.Breach(me))
		{
			DCO_Perf.Count("gren:cook_skip_breach");
			if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("gren_cook_skip unit=%1 dist=%2 reason=breach_full_fuse", DCO_Log.IdUnit(me), Math.Round(dist)));
			return;
		}
		GetGame().GetCallqueue().CallLater(DCO_CookApply, 150, false, gren, me, dist, aim, gren.GetOrigin(), tries, 0.0);
	}

	static void DCO_CookApply(IEntity gren, IEntity me, float dist, vector aim, vector p0, int tries, float elapsed)
	{
		if (!gren)
			return;
		TimerTriggerComponent trig = TimerTriggerComponent.Cast(gren.FindComponent(TimerTriggerComponent));
		if (!trig)
			return;
		vector p1 = gren.GetOrigin();
		if (elapsed <= 0.0)
		{
			GetGame().GetCallqueue().CallLater(DCO_CookApply, 300, false, gren, me, dist, aim, p1, tries, 0.3);
			return;
		}
		float vh = vector.DistanceXZ(p0, p1) / elapsed;
		if (vh < 1.0 && elapsed < 1.0)
		{
			GetGame().GetCallqueue().CallLater(DCO_CookApply, 150, false, gren, me, dist, aim, p0, tries, elapsed + 0.15);
			return;
		}
		float rem = vector.DistanceXZ(p1, aim);
		float flight = 3.0;
		if (vh > 1.0)
			flight = rem / vh;
		float before = trig.GetTimer();
		float cook = Math.RandomFloat(1.0, 2.0);
		float after = Math.Max(before - cook, Math.Max(flight + 0.8, 1.2));
		if (after >= before - 0.2)
		{
			DCO_Perf.Count("gren:cook_skip_flight");
			if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("gren_cook_skip unit=%1 dist=%2 timer=%3 flight_est=%4 speed=%5 measured_s=%6",
					DCO_Log.IdUnit(me), Math.Round(dist), before.ToString(1, 1), flight.ToString(1, 1), vh.ToString(1, 1), elapsed.ToString(1, 2)));
			return;
		}
		trig.SetTimer(after);
		DCO_Perf.Count("gren:cook");
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("gren_cook unit=%1 dist=%2 timer_before=%3 timer_after=%4 flight_est=%5 speed=%6 measured_s=%7",
				DCO_Log.IdUnit(me), Math.Round(dist), before.ToString(1, 1), after.ToString(1, 1), flight.ToString(1, 1), vh.ToString(1, 1), elapsed.ToString(1, 2)));
	}

	static void DCO_TrackLand(IEntity g, IEntity me, vector aim, vector last, int ticks, float dist)
	{
		if (g && ticks < 80)
		{
			GetGame().GetCallqueue().CallLater(DCO_TrackLand, 100, false, g, me, aim, g.GetOrigin(), ticks + 1, dist);
			return;
		}
		bool indoor = DCO_Util.TraceFrac(last + Vector(0, 0.3, 0), last + Vector(0, 12.0, 0), null, TraceFlags.WORLD | TraceFlags.ENTS, -1, "t:gren_land") < 1.0;
		bool aimIndoor = DCO_Util.TraceFrac(aim + Vector(0, 0.6, 0), aim + Vector(0, 12.0, 0), null, TraceFlags.WORLD | TraceFlags.ENTS, -1, "t:gren_land") < 1.0;
		float err = vector.Distance(last, aim);
		if (!g)
			DCO_GrenadeWatch.OnExploded(me);
		DCO_Perf.Count("gren:land");
		if (err <= 3.0)
			DCO_Perf.Count("gren:land_on_aim");
		if (aimIndoor && indoor)
			DCO_Perf.Count("gren:land_indoor_ok");
		else if (aimIndoor)
			DCO_Perf.Count("gren:land_indoor_miss");
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("gren_land unit=%1 dist=%2 aim_err=%3 aim_indoor=%4 land_indoor=%5 flight_s=%6 exploded=%7 pos=%8",
				DCO_Log.IdUnit(me), Math.Round(dist), Math.Round(err * 10) / 10, aimIndoor, indoor, (1.0 * ticks * 0.1).ToString(1, 1), g == null, DCO_Log.Pos(last)));
	}

	protected void DCO_RestoreWeapon()
	{
		if (!m_Utility || !m_Utility.m_CombatComponent)
			return;
		SCR_AICombatComponent cc = m_Utility.m_CombatComponent;
		EWeaponType cur = cc.GetCurrentWeaponType();
		if (cur != EWeaponType.WT_SMOKEGRENADE && cur != EWeaponType.WT_FRAGGRENADE)
			return;
		BaseWeaponComponent w;
		int muzzle;
		BaseMagazineComponent mag;
		cc.EvaluateExpectedWeapon(w, muzzle, mag);
		if (!w || w.GetWeaponType() == EWeaponType.WT_SMOKEGRENADE || w.GetWeaponType() == EWeaponType.WT_FRAGGRENADE)
			return;
		CharacterControllerComponent ctrl = cc.GetCharacterController();
		if (ctrl && ctrl.SelectWeapon(w))
			DCO_Perf.Count("throw_restore_weapon");
	}

	protected bool DCO_SmokeIntoBuilding()
	{
		vector target = m_vTargetPosition.m_Value;
		IEntity self;
		if (m_Utility)
			self = m_Utility.m_OwnerEntity;
		if (DCO_Util.TraceFrac(target + Vector(0, 0.5, 0), target + Vector(0, 12.0, 0), self, TraceFlags.WORLD | TraceFlags.ENTS, -1, "t:smoke_indoor") >= 1.0)
			return false;
		IEntity roof = DCO_Util.LastTraceEnt();
		return roof && !TreeEntity.Cast(roof) && !TreeEntity.Cast(roof.GetRootParent()) && !ChimeraCharacter.Cast(roof) && !Vehicle.Cast(roof.GetRootParent());
	}
};

class DCO_GrenWatchState
{
	float m_fRequested_ms;
	float m_fThrown_ms;
	float m_fExploded_ms;
	bool m_bFailed;
}

class DCO_GrenadeWatch
{
	protected static ref map<IEntity, ref DCO_GrenWatchState> s_mState;

	protected static DCO_GrenWatchState Get(IEntity thrower, bool create)
	{
		if (!thrower)
			return null;
		if (!s_mState)
			s_mState = new map<IEntity, ref DCO_GrenWatchState>();
		DCO_GrenWatchState st = s_mState.Get(thrower);
		if (!st && create)
		{
			st = new DCO_GrenWatchState();
			s_mState.Set(thrower, st);
		}
		return st;
	}

	static void OnRequested(IEntity thrower)
	{
		DCO_GrenWatchState st = Get(thrower, true);
		if (!st)
			return;
		st.m_fRequested_ms = GetGame().GetWorld().GetWorldTime();
		st.m_fThrown_ms = 0;
		st.m_fExploded_ms = 0;
		st.m_bFailed = false;
	}

	static void OnThrown(IEntity thrower)
	{
		DCO_GrenWatchState st = Get(thrower, false);
		if (st && st.m_fThrown_ms <= 0)
			st.m_fThrown_ms = GetGame().GetWorld().GetWorldTime();
	}

	static void OnFailed(IEntity thrower)
	{
		DCO_GrenWatchState st = Get(thrower, false);
		if (st && st.m_fThrown_ms <= 0)
			st.m_bFailed = true;
	}

	static void OnExploded(IEntity thrower)
	{
		DCO_GrenWatchState st = Get(thrower, false);
		if (st && st.m_fThrown_ms > 0 && st.m_fExploded_ms <= 0)
			st.m_fExploded_ms = GetGame().GetWorld().GetWorldTime();
	}

	static int State(IEntity thrower, float now, out float since_ms)
	{
		since_ms = 0;
		DCO_GrenWatchState st = Get(thrower, false);
		if (!st || st.m_bFailed)
			return -1;
		if (st.m_fExploded_ms > 0)
		{
			since_ms = now - st.m_fExploded_ms;
			return 2;
		}
		if (st.m_fThrown_ms > 0)
		{
			since_ms = now - st.m_fThrown_ms;
			if (since_ms > 4500.0)
			{
				since_ms = since_ms - 4500.0;
				return 2;
			}
			return 1;
		}
		since_ms = now - st.m_fRequested_ms;
		return 0;
	}

	static bool Breach(IEntity thrower)
	{
		DCO_GrenWatchState st = Get(thrower, false);
		return st && st.m_fExploded_ms <= 0 && !st.m_bFailed && GetGame().GetWorld().GetWorldTime() - st.m_fRequested_ms < 15000.0;
	}

	static float ThrowDelay_ms(IEntity thrower)
	{
		DCO_GrenWatchState st = Get(thrower, false);
		if (!st || st.m_fThrown_ms <= 0)
			return -1;
		return st.m_fThrown_ms - st.m_fRequested_ms;
	}

	static float FlightTime_ms(IEntity thrower)
	{
		DCO_GrenWatchState st = Get(thrower, false);
		if (!st || st.m_fThrown_ms <= 0 || st.m_fExploded_ms <= 0)
			return -1;
		return st.m_fExploded_ms - st.m_fThrown_ms;
	}
}
