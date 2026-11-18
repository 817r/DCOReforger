class DCO_VehMove
{
	protected static string s_sSrc;
	protected static ref map<IEntity, ref array<float>> s_mTimes;
	protected static map<IEntity, ref array<float>> GetTimes()
	{
		if (!s_mTimes)
			s_mTimes = new map<IEntity, ref array<float>>();
		return s_mTimes;
	}
	protected static ref map<IEntity, ref array<float>> s_mApplied;
	protected static map<IEntity, ref array<float>> GetApplied()
	{
		if (!s_mApplied)
			s_mApplied = new map<IEntity, ref array<float>>();
		return s_mApplied;
	}

	static void Src(string src)
	{
		s_sSrc = src;
	}

	static void SrcDefault(string src)
	{
		if (s_sSrc.IsEmpty())
			s_sSrc = src;
	}

	static bool IsDriver(SCR_AIUtilityComponent u)
	{
		return u && u.m_AIInfo && u.m_AIInfo.HasUnitState(EUnitState.PILOT);
	}

	static bool On(SCR_AIUtilityComponent u)
	{
		return DCO_Stress.T(u, "va.brain", 0.0) > 0.5;
	}

	static string TakeSrc()
	{
		string src = s_sSrc;
		s_sSrc = "";
		return src;
	}

	static bool OnRequest(SCR_AIUtilityComponent u, SCR_AICombatMoveRequestBase request, string src)
	{
		if (src.IsEmpty())
			src = "other";
		bool on = On(u);
		SCR_AICombatMoveRequest_Move rq = SCR_AICombatMoveRequest_Move.Cast(request);
		if (rq && on)
		{
			rq.m_eUnitType = SCR_EAICombatMoveUnitType.GROUND_VEHICLE;
			if (rq.m_eDirection == SCR_EAICombatMoveDirection.BACKWARD)
			{
				float maxDur = DCO_Stress.T(u, "va.reverse_max_m", 14.0) / SCR_AICombatMoveUtils.GROUND_VEHICLE_GENERIC_SPEED;
				if (rq.m_fMoveDuration_s > maxDur)
				{
					rq.m_fMoveDuration_s = maxDur;
					DCO_Perf.Count("vmove:reverse_capped");
				}
			}
		}
		DCO_Perf.Count("vmove:" + src);
		IEntity veh = DCO_VehicleCombat.GetVehicle(u.m_OwnerEntity);
		Stamp(GetTimes(), veh);
		if (rq && veh && DCO_VehDbg.IsOn())
		{
			vector to = DCO_VehBrain.PredictDest(veh, rq);
			DCO_VehDbg.NoteMove(veh, src, to, vector.DistanceXZ(veh.GetOrigin(), to));
		}
		if (rq && veh && DCO_Log.On(DCO_ELogChannel.VEH, DCO_ELogLevel.DEBUG))
		{
			float moveD = -1;
			if (rq.m_eDirection == SCR_EAICombatMoveDirection.CUSTOM_POS)
				moveD = vector.DistanceXZ(veh.GetOrigin(), rq.m_vMovePos);
			DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.DEBUG, DCO_Log.IdVeh(veh), string.Format("veh_move veh=%1 src=%2 reason=%3 dir=%4 dur_s=%5 move_d=%6 brain=%7",
				DCO_Log.IdVeh(veh), src, typename.EnumToString(SCR_EAICombatMoveReason, rq.m_eReason), typename.EnumToString(SCR_EAICombatMoveDirection, rq.m_eDirection),
				rq.m_fMoveDuration_s.ToString(1, 1), Math.Round(moveD), DCO_Util.B01(on)));
		}
		return on;
	}

	static void NoteApplied(SCR_AIUtilityComponent u)
	{
		if (!u)
			return;
		DCO_Perf.Count("vmove:applied");
		Stamp(GetApplied(), DCO_VehicleCombat.GetVehicle(u.m_OwnerEntity));
	}

	protected static void Stamp(map<IEntity, ref array<float>> m, IEntity veh)
	{
		if (!veh)
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		array<float> times = m.Get(veh);
		if (!times)
		{
			if (m.Count() > 128)
				Prune(m, now);
			times = {};
			m.Set(veh, times);
		}
		for (int i = times.Count() - 1; i >= 0; i--)
		{
			if (now - times[i] > 60000.0)
				times.Remove(i);
		}
		times.Insert(now);
	}

	protected static void Prune(map<IEntity, ref array<float>> m, float now)
	{
		array<IEntity> stale = {};
		foreach (IEntity v, array<float> times : m)
		{
			if (!v || Recent(times, now) == 0)
				stale.Insert(v);
		}
		foreach (IEntity sv : stale)
			m.Remove(sv);
	}

	protected static int Recent(array<float> times, float now)
	{
		if (!times)
			return 0;
		int n = 0;
		foreach (float t : times)
		{
			if (now - t <= 60000.0)
				n++;
		}
		return n;
	}

	static int Rate(IEntity veh)
	{
		return Recent(GetTimes().Get(veh), GetGame().GetWorld().GetWorldTime());
	}

	static int AppliedRate(IEntity veh)
	{
		return Recent(GetApplied().Get(veh), GetGame().GetWorld().GetWorldTime());
	}

	static string CensusLine()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		int vehicles = 0;
		int offered = 0;
		int peak = 0;
		foreach (IEntity v, array<float> times : GetTimes())
		{
			int n = Recent(times, now);
			if (!v || n == 0)
				continue;
			vehicles++;
			offered += n;
			if (n > peak)
				peak = n;
		}
		int applied = 0;
		foreach (IEntity av, array<float> atimes : GetApplied())
		{
			if (av)
				applied += Recent(atimes, now);
		}
		return string.Format("CENSUS vmove vehicles=%1 offered_last_min=%2 applied_last_min=%3 peak_offered_per_veh=%4", vehicles, offered, applied, peak);
	}
}

modded class SCR_AICombatMoveLogicVehicleGunnerBase
{
	override protected void ApplyNewRequest(notnull SCR_AICombatMoveRequestBase rq)
	{
		if (SCR_AICombatMoveLogicVehicleGunner_Suppressive.Cast(this))
			DCO_VehMove.SrcDefault("gunner_supp");
		else
			DCO_VehMove.SrcDefault("gunner_attack");
		super.ApplyNewRequest(rq);
	}
}
