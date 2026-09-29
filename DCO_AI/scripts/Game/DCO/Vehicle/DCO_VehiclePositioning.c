class DCO_VehiclePositioning
{
	protected static const float BAD_POS_RADIUS_SQ  = 15 * 15;

	protected static ref array<float> s_aRings = {25, 50, 85};

	protected static ref map<IEntity, float>  s_mHideUntil_ms = new map<IEntity, float>();
	protected static ref map<IEntity, vector> s_mBadPos       = new map<IEntity, vector>();
	protected static ref map<IEntity, float>  s_mBadPosTime   = new map<IEntity, float>();

	protected static NavmeshWorldComponent s_Navmesh;
	protected static bool s_bNavmeshResolved;
	protected static ref TraceParam s_Trace = new TraceParam();

	static DCO_GroupTactics GetPosture(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.GetOwner())
			return DCO_GroupTactics.BALANCE;

		SCR_AIGroup group = SCR_AIGroup.Cast(utility.GetOwner().GetParentGroup());
		if (!group || !group.GetGroupUtilityComponent())
			return DCO_GroupTactics.BALANCE;

		return group.GetGroupUtilityComponent().DCO_GetPosture();
	}

	static float GetStandoffScale(DCO_GroupTactics posture)
	{
		switch (posture)
		{
			case DCO_GroupTactics.AGGRESIVE: return 0.7;
			case DCO_GroupTactics.DEFENSIVE: return 1.3;
			case DCO_GroupTactics.EVASIVE:   return 1.6;
		}
		return 1.0;
	}

	static float GetHullDownWeight(DCO_GroupTactics posture)
	{
		switch (posture)
		{
			case DCO_GroupTactics.AGGRESIVE: return 0.1;
			case DCO_GroupTactics.DEFENSIVE: return 0.45;
			case DCO_GroupTactics.EVASIVE:   return 0.45;
		}
		return 0.25;
	}

	static bool PrefersHiding(DCO_GroupTactics posture, IEntity vehicle)
	{
		if (posture == DCO_GroupTactics.EVASIVE || posture == DCO_GroupTactics.DEFENSIVE)
			return true;

		SCR_VehicleDamageManagerComponent dmg = SCR_VehicleDamageManagerComponent.Cast(vehicle.FindComponent(SCR_VehicleDamageManagerComponent));
		return dmg && dmg.GetHealthScaled() < 0.5;
	}

	static bool IsHiding(IEntity vehicle)
	{
		float until;
		return s_mHideUntil_ms.Find(vehicle, until) && GetGame().GetWorld().GetWorldTime() < until;
	}

	static void MarkHiding(IEntity vehicle)
	{
		s_mHideUntil_ms.Set(vehicle, GetGame().GetWorld().GetWorldTime() + Math.RandomFloat(8.0, 14.0) * 1000);
	}

	static bool FindFirePosition(IEntity vehicle, vector targetPos, float desiredDist, float hullDownWeight,
		vector wpPos, float wpRadius, out vector outPos)
	{
		int pt = DCO_Perf.Begin();
		bool found = DoFindFirePosition(vehicle, targetPos, desiredDist, hullDownWeight, wpPos, wpRadius, outPos);
		DCO_Perf.End("veh_fire_position", pt);
		return found;
	}

	protected static bool DoFindFirePosition(IEntity vehicle, vector targetPos, float desiredDist, float hullDownWeight,
		vector wpPos, float wpRadius, out vector outPos)
	{
		float hull, turret;
		GetSightHeights(vehicle, hull, turret);

		vector origin = vehicle.GetOrigin();
		vector targetEye = targetPos + Vector(0, 1.0, 0);
		float maxRing = s_aRings[s_aRings.Count() - 1];

		IEntity atShooter;
		vector atPos;
		bool hasAT = DCO_VehicleCombat.GetATThreat(vehicle, atShooter, atPos);
		vector atEye = atPos + Vector(0, 1.6, 0);

		float bestScore = -float.MAX;
		bool found;
		float angle0 = Math.RandomFloat(0, Math.PI2);

		foreach (float ring : s_aRings)
		{
			for (int i = 0; i < 8; i++)
			{
				vector cand;
				if (!MakeCandidate(vehicle, origin, ring, angle0 + i * Math.PI2 / 8, wpPos, wpRadius, cand))
					continue;

				float dTgt = vector.DistanceXZ(cand, targetPos);
				if (dTgt < 40.0)
					continue;

				if (!IsClear(cand + Vector(0, turret, 0), targetEye, vehicle))
					continue;

				float score = 0.4 * (1 - Math.Clamp(Math.AbsFloat(dTgt - desiredDist) / desiredDist, 0, 1))
					+ 0.15 * (1 - ring / maxRing)
					+ 0.1 * Math.Clamp((cand[1] - targetPos[1]) / 10, 0, 1);

				if (!IsClear(cand + Vector(0, hull, 0), targetEye, vehicle))
					score += hullDownWeight;

				if (hasAT)
				{
					if (vector.DistanceXZ(cand, atPos) < 250.0)
						score -= 0.3;
					if (IsClear(atEye, cand + Vector(0, turret, 0), vehicle))
						score -= 0.3;
				}

				if (score <= bestScore || !SnapToNavmesh(cand))
					continue;

				bestScore = score;
				outPos = cand;
				found = true;
			}
		}

		return found;
	}

	static bool FindHidePosition(IEntity vehicle, vector threatPos, vector wpPos, float wpRadius, out vector outPos)
	{
		float hull, turret;
		GetSightHeights(vehicle, hull, turret);

		vector origin = vehicle.GetOrigin();
		vector threatEye = threatPos + Vector(0, 1.6, 0);
		float myThreatDist = vector.DistanceXZ(origin, threatPos);
		float maxRing = s_aRings[s_aRings.Count() - 1];

		float bestScore = -float.MAX;
		bool found;
		float angle0 = Math.RandomFloat(0, Math.PI2);

		foreach (float ring : s_aRings)
		{
			for (int i = 0; i < 8; i++)
			{
				vector cand;
				if (!MakeCandidate(vehicle, origin, ring, angle0 + i * Math.PI2 / 8, wpPos, wpRadius, cand))
					continue;

				float dThreat = vector.DistanceXZ(cand, threatPos);
				if (dThreat < 40.0)
					continue;

				if (IsClear(threatEye, cand + Vector(0, turret, 0), vehicle))
					continue;

				float score = 0.5 * Math.Clamp((dThreat - myThreatDist) / 60 + 0.5, 0, 1)
					+ 0.3 * (1 - ring / maxRing);

				if (score <= bestScore || !SnapToNavmesh(cand))
					continue;

				bestScore = score;
				outPos = cand;
				found = true;
			}
		}

		return found;
	}

	static SCR_AICombatMoveRequest_Move CreateATEvadeRequest(IEntity vehicle, vector atPos)
	{
		vector hidePos;
		if (FindHidePosition(vehicle, atPos, vector.Zero, 0, hidePos))
		{
			MarkHiding(vehicle);
			DCO_BenchmarkLoggerComponent.Event(string.Format("veh_hide_at veh=%1 move=%2", vehicle, Math.Round(vector.Distance(vehicle.GetOrigin(), hidePos))));
			return CreateMoveRequest(vehicle.GetOrigin(), hidePos, atPos, SCR_EAICombatMoveReason.MOVE_FROM_TARGET);
		}

		return DCO_VehicleCombat.CreateEvadeRequest(vehicle, atPos);
	}

	static SCR_AICombatMoveRequest_Move CreateMoveRequest(vector vehiclePos, vector movePos, vector targetPos, SCR_EAICombatMoveReason reason)
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		rq.m_eReason = reason;
		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		rq.m_eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
		rq.m_vMovePos = movePos;
		rq.m_vTargetPos = targetPos;
		rq.m_eMovementType = EMovementType.RUN;
		rq.m_fMoveDuration_s = vector.Distance(vehiclePos, movePos) / SCR_AICombatMoveUtils.GROUND_VEHICLE_GENERIC_SPEED * 2 + 5;
		rq.m_bTryFindCover = false;
		rq.m_bFailIfNoCover = false;
		rq.m_bAimAtTarget = false;
		rq.m_bAimAtTargetEnd = false;
		rq.GetOnFailed().Insert(OnMoveFailed);
		return rq;
	}

	protected static void OnMoveFailed(SCR_AIUtilityComponent utility, SCR_AICombatMoveRequestBase request, SCR_EAICombatMoveRequestFailReason failReason)
	{
		SCR_AICombatMoveRequest_Move rq = SCR_AICombatMoveRequest_Move.Cast(request);
		if (!utility || !rq)
			return;

		IEntity vehicle = DCO_VehicleCombat.GetVehicle(utility.m_OwnerEntity);
		if (!vehicle)
			return;

		s_mBadPos.Set(vehicle, rq.m_vMovePos);
		s_mBadPosTime.Set(vehicle, GetGame().GetWorld().GetWorldTime());
	}

	protected static bool MakeCandidate(IEntity vehicle, vector origin, float ring, float angle, vector wpPos, float wpRadius, out vector cand)
	{
		BaseWorld world = GetGame().GetWorld();
		cand = origin + Vector(Math.Sin(angle) * ring, 0, Math.Cos(angle) * ring);

		if (wpRadius > 0 && vector.DistanceXZ(cand, wpPos) > wpRadius)
			return false;

		float y = world.GetSurfaceY(cand[0], cand[2]);
		if (world.IsOcean() && y < world.GetOceanBaseHeight() + 0.5)
			return false;

		const float d = 4;
		float dy = Math.Max(Math.AbsFloat(world.GetSurfaceY(cand[0] + d, cand[2]) - world.GetSurfaceY(cand[0] - d, cand[2])),
			Math.AbsFloat(world.GetSurfaceY(cand[0], cand[2] + d) - world.GetSurfaceY(cand[0], cand[2] - d)));
		if (dy / (2 * d) > 0.35)
			return false;

		cand[1] = y;

		vector bad;
		float badTime;
		if (s_mBadPos.Find(vehicle, bad) && s_mBadPosTime.Find(vehicle, badTime)
			&& world.GetWorldTime() - badTime < 60000.0 && vector.DistanceSqXZ(bad, cand) < BAD_POS_RADIUS_SQ)
			return false;

		return true;
	}

	protected static void GetSightHeights(IEntity vehicle, out float hull, out float turret)
	{
		vector mins, maxs;
		vehicle.GetBounds(mins, maxs);
		float height = Math.Max(maxs[1] - mins[1], 1.5);
		hull = height * 0.45;
		turret = height * 0.9;
	}

	protected static bool IsClear(vector from, vector to, IEntity exclude)
	{
		vector dir = to - from;
		float len = dir.Length();
		if (len < 3)
			return true;

		dir = dir * (1 / len);
		s_Trace.Start = from + dir;
		s_Trace.End = to - dir * 2;
		s_Trace.Exclude = exclude;
		s_Trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
		s_Trace.LayerMask = EPhysicsLayerDefs.Projectile;
		DCO_Perf.Count("t:DCO_VehiclePositioning");
		return GetGame().GetWorld().TraceMove(s_Trace, null) >= 1;
	}

	protected static bool SnapToNavmesh(inout vector pos)
	{
		if (!s_bNavmeshResolved)
		{
			s_bNavmeshResolved = true;
			AIWorld aiWorld = GetGame().GetAIWorld();
			if (aiWorld)
				s_Navmesh = aiWorld.GetNavmeshWorldComponent("BTRlike");
		}

		if (!s_Navmesh)
			return true;

		vector snapped;
		if (!s_Navmesh.GetReachablePoint(pos, 5.0, snapped))
			return false;

		if (vector.DistanceXZ(snapped, pos) > 5.0)
			return false;

		pos = snapped;
		return true;
	}
}
