enum DCO_EPatrolPattern
{
	RING    = 0,
	LANE    = 1,
	ARC     = 2,
	ADVANCE = 3
}

class DCO_CommanderPatrol : Managed
{
	protected ref map<DCO_GroupUtilityComponent, ref array<vector>> m_mMemory = new map<DCO_GroupUtilityComponent, ref array<vector>>();
	protected int m_iSmartRemaining;

	protected float m_fRadiusScale = 1.6;
	protected float m_fFullSquadSize = 12.0;
	protected float m_fVehicleCruiseSpeed = 25.0;
	protected float m_fRoadSnapMaxDist = 120.0;

	protected int m_iMemoryRetries = 5;
	protected float m_fMemoryJumpMin = 1.1;
	protected float m_fMemoryJumpMax = 1.8;

	protected float m_fCalmRingChance = 0.5;
	protected float m_fAdvancePerAggression = 0.5;
	protected float m_fArcBaseChance = 0.5;
	protected float m_fArcPerAggression = 0.2;
	protected float m_fLaneChanceUntil = 0.8;

	protected int m_iPointsMin = 4;
	protected int m_iPointsMaxExcl = 7;
	protected float m_fRingJitterMin = 0.85;
	protected float m_fRingJitterMax = 1.15;
	protected float m_fLaneLength = 1.8;
	protected float m_fLaneLateral = 0.35;
	protected float m_fArcSpanMin = 120.0;
	protected float m_fArcSpanMax = 160.0;
	protected float m_fArcJitterMin = 0.8;
	protected float m_fArcJitterMax = 1.2;
	protected float m_fAdvanceLength = 2.2;
	protected float m_fAdvanceLateralMin = 0.25;
	protected float m_fAdvanceLateralMax = 0.6;

	protected int m_iRefineTries = 5;
	protected float m_fRefineRadius = 35.0;
	protected float m_fRefineMinShare = 0.3;
	protected float m_fFullElevationGain = 15.0;
	protected float m_fEyeHeight = 1.2;
	protected float m_fTargetHeight = 1.0;
	protected float m_fPartialLosFraction = 0.85;
	protected float m_fPartialLosScore = 0.5;
	protected float m_fLosWeight = 0.6;
	protected float m_fElevationWeight = 0.4;

	void ResetSmartBudget(int budget)
	{
		m_iSmartRemaining = budget;
	}

	void ForgetGroup(DCO_GroupUtilityComponent grp)
	{
		m_mMemory.Remove(grp);
	}

	void GenerateRoute(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent grp, vector center, float radius, float worldTime, IEntity mountedVehicle = null)
	{
		if (!grp)
			return;

		float patrolRadius = radius * m_fRadiusScale * RadiusMultiplier(cmd, grp);
		if (mountedVehicle)
			patrolRadius = patrolRadius * cmd.GetVehiclePatrolRadiusMul();

		vector patrolCenter = ApplyFrontlineDrift(cmd, center);

		patrolCenter = AvoidRecentCenters(cmd, grp, patrolCenter, patrolRadius);
		RememberCenter(cmd, grp, patrolCenter);

		vector lookAt;
		bool   hasThreat = cmd.GetFrontline().TryGetHottestPoint(lookAt);

		int pattern = PickPattern(cmd, hasThreat);

		array<vector> points = {};

		switch (pattern)
		{
			case DCO_EPatrolPattern.LANE:    BuildLane(patrolCenter, patrolRadius, hasThreat, lookAt, points); break;
			case DCO_EPatrolPattern.ARC:     BuildArc(patrolCenter, patrolRadius, lookAt, points);             break;
			case DCO_EPatrolPattern.ADVANCE: BuildAdvance(patrolCenter, patrolRadius, lookAt, points);         break;
			default:                         BuildRing(patrolCenter, patrolRadius, points);                    break;
		}

		grp.CompleteAllWaypoints();

		EMovementType moveType = EMovementType.WALK;
		RoadNetworkManager roads;
		if (mountedVehicle)
		{
			moveType = EMovementType.RUN;
			if (!grp.IsMountedIn(mountedVehicle))
			{
				SCR_BoardingEntityWaypoint board = SCR_BoardingEntityWaypoint.Cast(cmd.SpawnGetInWP(mountedVehicle.GetOrigin()));
				if (board)
				{
					board.SetEntity(mountedVehicle);
					board.SetAllowance(true, true, true);
					board.SetCompletionType(EAIWaypointCompletionType.All);
					grp.MoveTo(board, worldTime);
				}
			}

			SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
			if (aiWorld)
				roads = aiWorld.GetRoadNetworkManager();
		}

		foreach (vector raw : points)
		{
			vector p = raw;
			p = DCO_Util.Surface(p);

			if (hasThreat && !mountedVehicle)
				p = RefinePoint(cmd, p, lookAt);

			if (roads)
				p = SnapToRoad(roads, p);

			EWaterSurfaceType waterType = EWaterSurfaceType.WST_NONE;
			float lakeArea = 0;
			float waterY   = SCR_WorldTools.GetWaterSurfaceY(null, p, waterType, lakeArea);

			if (p[1] < waterY && waterType != EWaterSurfaceType.WST_NONE)
				continue;

			if (!mountedVehicle && !DCO_PatrolPointOnNavmesh(grp, p))
				continue;

			SCR_AIWaypoint wp = cmd.SpawnMoveWP(p, moveType);
			if (wp)
				grp.MoveTo(wp, worldTime);
		}

		if (mountedVehicle)
			grp.SetVehicleCruiseSpeed(m_fVehicleCruiseSpeed);
	}

	protected bool DCO_PatrolPointOnNavmesh(DCO_GroupUtilityComponent grp, inout vector p)
	{
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return true;
		NavmeshWorldComponent nav = aiWorld.GetNavmeshWorldComponent("Soldiers");
		if (!nav || !nav.IsTileLoaded(p))
			return true;
		vector snapped;
		if (nav.GetReachablePoint(p, 8.0, snapped))
		{
			p = DCO_Util.Surface(snapped);
			return true;
		}
		DCO_Perf.Count("patrol:pt_off_navmesh");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("patrol_pt_skip grp=%1 pos=%2 reason=off_navmesh", DCO_Log.IdGroup(SCR_AIGroup.Cast(grp.GetOwner())), DCO_Util.XZ(p)));
		return false;
	}

	protected vector SnapToRoad(RoadNetworkManager roads, vector p)
	{
		BaseRoad road;
		float dist;
		roads.GetClosestRoad(p, road, dist, true);
		if (!road || dist > m_fRoadSnapMaxDist)
			return p;

		array<vector> roadPts = {};
		road.GetPoints(roadPts);
		vector best = p;
		float bestSq = float.MAX;
		foreach (vector rp : roadPts)
		{
			float dSq = vector.DistanceSqXZ(rp, p);
			if (dSq < bestSq)
			{
				bestSq = dSq;
				best = rp;
			}
		}
		best = DCO_Util.Surface(best);
		return best;
	}

	protected float RadiusMultiplier(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent grp)
	{
		float units    = grp.GetUnitCount();
		float strength = Math.Clamp(units / m_fFullSquadSize, 0.0, 1.0);

		return Math.Lerp(cmd.DCO_GT_PatrolRadiusMulMin(), cmd.DCO_GT_PatrolRadiusMulMax(), strength);
	}

	protected vector ApplyFrontlineDrift(AICommander_BaseComponent cmd, vector center)
	{
		float drift = cmd.DCO_GT_PatrolFrontlineDrift();
		if (drift <= 0.0)
			return center;

		vector frontline, facing;
		if (!cmd.GetFrontline().GetNearestPoint(center, frontline, facing))
			return center;

		if (facing.LengthSq() < 0.001)
			return center;

		vector drifted = center + (facing * drift);
		drifted[1]     = GetGame().GetWorld().GetSurfaceY(drifted[0], drifted[2]);

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return drifted;

		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj || !obj.GetOwner())
				continue;

			if (obj.IsCapturedBy(cmd.GetCommanderFactionKey(), cmd.GetCommanderUID()))
				continue;

			float standoff = cmd.DCO_GT_PatrolFrontlineStandoff() + obj.GetRadius();

			if (vector.DistanceSq(drifted, obj.GetOwner().GetOrigin()) < standoff * standoff)
				return center;
		}

		return drifted;
	}

	protected vector AvoidRecentCenters(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent grp, vector candidate, float patrolRadius)
	{
		array<vector> memory;
		if (!m_mMemory.Find(grp, memory) || memory.IsEmpty())
			return candidate;

		float memoryRadius = cmd.DCO_GT_PatrolMemoryRadius();
		for (int attempt = 0; attempt < m_iMemoryRetries; attempt++)
		{
			bool tooClose = false;

			foreach (vector old : memory)
			{
				if (vector.DistanceSq(candidate, old) < memoryRadius * memoryRadius)
				{
					tooClose = true;
					break;
				}
			}

			if (!tooClose)
				return candidate;

			float angle = Math.RandomFloat(0.0, 360.0) * Math.DEG2RAD;
			float dist  = memoryRadius * Math.RandomFloatInclusive(m_fMemoryJumpMin, m_fMemoryJumpMax);

			candidate = candidate + Vector(Math.Cos(angle) * dist, 0.0, Math.Sin(angle) * dist);
			candidate = DCO_Util.Surface(candidate);
		}

		return candidate;
	}

	protected void RememberCenter(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent grp, vector center)
	{
		array<vector> memory;
		if (!m_mMemory.Find(grp, memory))
		{
			memory = new array<vector>();
			m_mMemory.Insert(grp, memory);
		}

		memory.Insert(center);

		while (memory.Count() > cmd.DCO_GT_PatrolMemorySize())
			memory.Remove(0);
	}

	protected int PickPattern(AICommander_BaseComponent cmd, bool hasThreat)
	{
		if (!hasThreat)
		{
			if (Math.RandomFloat01() < m_fCalmRingChance)
				return DCO_EPatrolPattern.RING;

			return DCO_EPatrolPattern.LANE;
		}

		float roll = Math.RandomFloat01();
		float aggression = cmd.DCO_EffAggression();

		if (roll < aggression * m_fAdvancePerAggression)
			return DCO_EPatrolPattern.ADVANCE;

		if (roll < m_fArcBaseChance + (aggression * m_fArcPerAggression))
			return DCO_EPatrolPattern.ARC;

		if (roll < m_fLaneChanceUntil)
			return DCO_EPatrolPattern.LANE;

		return DCO_EPatrolPattern.RING;
	}

	protected void BuildRing(vector center, float radius, out array<vector> outPoints)
	{
		int   count      = Math.RandomInt(m_iPointsMin, m_iPointsMaxExcl);
		float startAngle = Math.RandomFloat(0.0, 360.0);

		for (int i = 0; i < count; i++)
		{
			float angleRad = (startAngle + (360.0 / count) * i) * Math.DEG2RAD;
			float dist     = radius * Math.RandomFloatInclusive(m_fRingJitterMin, m_fRingJitterMax);

			outPoints.Insert(center + Vector(Math.Cos(angleRad) * dist, 0.0, Math.Sin(angleRad) * dist));
		}
	}

	protected void BuildLane(vector center, float radius, bool hasThreat, vector lookAt, out array<vector> outPoints)
	{
		vector axis;

		if (hasThreat)
		{
			vector toThreat = lookAt - center;
			toThreat        = Vector(toThreat[0], 0.0, toThreat[2]);

			if (toThreat.LengthSq() < 1.0)
				toThreat = Vector(1.0, 0.0, 0.0);
			else
				toThreat = toThreat.Normalized();

			axis = Vector(-toThreat[2], 0.0, toThreat[0]);
		}
		else
		{
			float a = Math.RandomFloat(0.0, 360.0) * Math.DEG2RAD;
			axis    = Vector(Math.Cos(a), 0.0, Math.Sin(a));
		}

		vector side = Vector(-axis[2], 0.0, axis[0]);

		int   legs   = Math.RandomInt(m_iPointsMin, m_iPointsMaxExcl);
		float length = radius * m_fLaneLength;

		for (int i = 0; i < legs; i++)
		{
			float t = (i / (float)Math.Max(legs - 1, 1)) * 2.0 - 1.0;

			float lateral = radius * m_fLaneLateral;
			if (i % 2 == 1)
				lateral = -lateral;

			outPoints.Insert(center + (axis * (t * length)) + (side * lateral));
		}
	}

	protected void BuildArc(vector center, float radius, vector lookAt, out array<vector> outPoints)
	{
		vector toThreat = lookAt - center;
		toThreat        = Vector(toThreat[0], 0.0, toThreat[2]);

		float baseAngle;
		if (toThreat.LengthSq() < 1.0)
			baseAngle = Math.RandomFloat(0.0, 360.0);
		else
			baseAngle = Math.Atan2(toThreat[2], toThreat[0]) * Math.RAD2DEG;

		float span  = Math.RandomFloatInclusive(m_fArcSpanMin, m_fArcSpanMax);
		int   count = Math.RandomInt(m_iPointsMin, m_iPointsMaxExcl);

		for (int i = 0; i < count; i++)
		{
			float t        = i / (float)Math.Max(count - 1, 1);
			float angleDeg = baseAngle - (span * 0.5) + (span * t);
			float angleRad = angleDeg * Math.DEG2RAD;

			float dist = radius * Math.RandomFloatInclusive(m_fArcJitterMin, m_fArcJitterMax);

			outPoints.Insert(center + Vector(Math.Cos(angleRad) * dist, 0.0, Math.Sin(angleRad) * dist));
		}
	}

	protected void BuildAdvance(vector center, float radius, vector lookAt, out array<vector> outPoints)
	{
		vector dir = lookAt - center;
		dir        = Vector(dir[0], 0.0, dir[2]);

		if (dir.LengthSq() < 1.0)
			dir = Vector(1.0, 0.0, 0.0);
		else
			dir = dir.Normalized();

		vector side = Vector(-dir[2], 0.0, dir[0]);

		int   count  = Math.RandomInt(m_iPointsMin, m_iPointsMaxExcl);
		float length = radius * m_fAdvanceLength;

		for (int i = 0; i < count; i++)
		{
			float t = (i + 1) / (float)count;

			float lateral = radius * Math.RandomFloatInclusive(m_fAdvanceLateralMin, m_fAdvanceLateralMax);
			if (i % 2 == 1)
				lateral = -lateral;

			outPoints.Insert(center + (dir * (t * length)) + (side * lateral));
		}
	}

	protected vector RefinePoint(AICommander_BaseComponent cmd, vector candidate, vector lookAt)
	{
		if (!cmd.DCO_GT_PatrolTerrainScoring() || m_iSmartRemaining <= 0)
			return candidate;

		m_iSmartRemaining = m_iSmartRemaining - 1;

		vector best      = candidate;
		float  bestScore = ScorePoint(candidate, lookAt);

		for (int i = 0; i < m_iRefineTries; i++)
		{
			float angle = Math.RandomFloat(0.0, 360.0) * Math.DEG2RAD;
			float dist  = Math.RandomFloatInclusive(m_fRefineRadius * m_fRefineMinShare, m_fRefineRadius);

			vector test = candidate + Vector(Math.Cos(angle) * dist, 0.0, Math.Sin(angle) * dist);
			test[1]     = GetGame().GetWorld().GetSurfaceY(test[0], test[2]);

			float score = ScorePoint(test, lookAt);

			if (score > bestScore)
			{
				bestScore = score;
				best      = test;
			}
		}

		return best;
	}

	protected float ScorePoint(vector pos, vector lookAt)
	{
		float heightDiff = pos[1] - lookAt[1];
		float elevScore  = Math.Clamp(heightDiff / m_fFullElevationGain, 0.0, 1.0);

		vector eye     = Vector(pos[0], pos[1] + m_fEyeHeight, pos[2]);
		vector target  = Vector(lookAt[0], lookAt[1] + m_fTargetHeight, lookAt[2]);

		float hitFraction = DCO_Util.TraceFrac(eye, target, null, TraceFlags.ANY_CONTACT, -1, "t:AICommanderBase");

		float losScore;
		if (hitFraction >= 1.0)
			losScore = 1.0;
		else if (hitFraction >= m_fPartialLosFraction)
			losScore = m_fPartialLosScore;
		else
			losScore = 0.0;

		return (losScore * m_fLosWeight) + (elevScore * m_fElevationWeight);
	}
}
