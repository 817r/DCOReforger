modded class AICommander_BaseComponent
{
	bool DCO_FriendlySide(vector objPos, float objRadius, vector pos)
	{
		float sideDeg;
		bool pathClear;
		return StagingSideOk(objPos, objRadius, DCO_AttackerCentroid(), pos, sideDeg, pathClear);
	}

	vector ComputeFlankPosition(vector from, vector objective, float distance)
	{
		float angle = Math.RandomFloat(m_fFlankAngleMin, m_fFlankAngleMax);

		vector left  = FlankPoint(from, objective, distance, angle, 1);
		vector right = FlankPoint(from, objective, distance, angle, -1);

		if (EvaluateFlankCandidate(left, objective) >= EvaluateFlankCandidate(right, objective))
			return left;

		return right;
	}

	vector FlankPoint(vector from, vector objective, float distance, float angleDeg, int side)
	{
		float baseAng = ApproachAngle(from, objective);
		float a = baseAng + 1.0 * side * angleDeg * Math.DEG2RAD;

		vector p = objective + Vector(Math.Cos(a) * distance, 0.0, Math.Sin(a) * distance);
		p = DCO_Util.Surface(p);
		return p;
	}

	protected float ApproachAngle(vector from, vector objective)
	{
		vector d = from - objective;
		if (d[0] * d[0] + d[2] * d[2] < 1.0)
			d = GetOwner().GetOrigin() - objective;

		return Math.Atan2(d[2], d[0]);
	}

	protected vector GetEntryPoint(CMD_AICommanderObjectiveComponent obj, vector from)
	{
		return FlankPoint(from, obj.GetOwner().GetOrigin(), obj.GetRadius() + 60.0, 0.0, 1);
	}

	float EvaluateFlankCandidate(vector candidate, vector objective)
	{
		if (!IsRoutePointUsable(candidate))
			return -1.0;

		return Math.Clamp((candidate[1] - objective[1]) / m_fFlankHeightScale, -1.0, 1.0) + Math.RandomFloat(0.0, m_fFlankScoreJitter);
	}

	protected vector GetOrCreateStagingPos(CMD_AICommanderObjectiveComponent obj)
	{
		vector cached;
		if (m_mStagingPos.Find(obj, cached))
			return cached;

		vector objPos = obj.GetOwner().GetOrigin();
		vector base   = DCO_AttackerCentroid();

		vector axis = objPos - base;
		axis        = Vector(axis[0], 0.0, axis[2]);
		axis        = axis.Normalized();
		vector rawAxis = axis;
		axis        = m_Ops.AdjustApproachAxis(obj, axis);

		float dist = vector.Distance(base, objPos);

		float minDist = Math.Max(m_fStagingMinDistance, obj.GetRadius() + m_fStagingMargin);
		float maxDist = Math.Max(m_fStagingMaxDistance, minDist);

		float standoff = Math.Clamp(dist * m_fStagingLegFraction, minDist, maxDist);

		if (standoff >= dist)
			standoff = dist * 0.5;

		vector pos = PickCoveredStagingPoint(objPos, axis, standoff, dist);

		pos = PullStagingClearOfHostileObjectives(obj, base, axis, dist, standoff, pos);
		pos = DCO_Util.Surface(pos);

		bool rotated = vector.Dot(axis, rawAxis) < m_fStagingAxisSameDot;
		float sideDeg;
		bool pathClear;
		if (!StagingSideOk(objPos, obj.GetRadius(), base, pos, sideDeg, pathClear))
		{
			DCO_Perf.Count("staging_side:fallback");
			pos = PickCoveredStagingPoint(objPos, rawAxis, standoff, dist);
			pos = PullStagingClearOfHostileObjectives(obj, base, rawAxis, dist, standoff, pos);
			pos = DCO_Util.Surface(pos);
			rotated = false;
			StagingSideOk(objPos, obj.GetRadius(), base, pos, sideDeg, pathClear);
		}

		m_mStagingPos.Insert(obj, pos);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("staging_pos cmd=%1 obj=%2 pos=%3 obj_dist=%4 side_deg=%5 path_clear=%6 rotated=%7 from=%8", m_sCommanderUID, obj.GetLogName(), DCO_Util.XZ(pos), Math.Round(vector.DistanceXZ(pos, objPos)), Math.Round(sideDeg), pathClear, rotated, DCO_Util.XZ(base)));

		return pos;
	}

	protected vector PickCoveredStagingPoint(vector objPos, vector axis, float standoff, float legDist)
	{
		BaseWorld world = GetGame().GetWorld();
		float baseAng = Math.Atan2(-axis[2], -axis[0]);

		vector best = objPos - axis * standoff;
		float bestScore = -1000.0;
		bool stExplore = DCO_CmdLearn.Explore();
		float bestStM = 1.0;
		float bestStN = 0;

		for (int di = 0; di < m_iStagingDepthSteps; di++)
		{
			float d = standoff + di * m_fStagingDepthStep;
			if (d >= legDist)
				break;

			for (int ai = -m_iStagingAngleSteps; ai <= m_iStagingAngleSteps; ai++)
			{
				float a = baseAng + ai * m_fStagingAngleStep * Math.DEG2RAD;
				vector cand = objPos + Vector(Math.Cos(a) * d, 0, Math.Sin(a) * d);
				cand = DCO_Util.Surface(cand);

				if (!IsRoutePointUsable(cand))
					continue;

				float hit = DCO_Util.TraceFrac(Vector(cand[0], cand[1] + m_fCoverEyeHeight, cand[2]), Vector(objPos[0], objPos[1] + m_fCoverTargetHeight, objPos[2]), null, TraceFlags.WORLD | TraceFlags.ENTS, -1, "t:AICommanderBase");

				float score = 0.0;
				if (hit < m_fCoverBlockedFrac)
					score = score + m_fCoverBonus;
				score = score - Math.AbsFloat(ai) * m_fCoverAnglePenalty - 1.0 * di * m_fCoverDepthPenalty;
				float stN;
				float stM = 1.0;
				if (!stExplore)
					stM = DCO_CmdLearn.TerrainMult("staging", cand, 50.0, stN);
				if (stN > 0)
					score = score + (stM - 1.0) * 4.0 * m_fCoverBonus;

				if (score > bestScore)
				{
					bestScore = score;
					best = cand;
					bestStM = stM;
					bestStN = stN;
				}
			}
		}

		if (stExplore)
			DCO_CmdLearn.LogExplore("staging", DCO_LearnStore.PosKey(best, 50.0));
		else if (bestStN > 0)
			DCO_CmdLearn.LogUse("staging", DCO_LearnStore.PosKey(best, 50.0), bestScore, bestStM, bestStN);
		return best;
	}

	protected void DCO_TaskNoWpWatch(float worldTime)
	{
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (!g || !g.GetOwner() || g.IsPlayerGroup() || g.IsInTransport())
				continue;
			DCO_EGroupTask t = g.GetTask();
			AIGroup ag = AIGroup.Cast(g.GetOwner());
			bool orphan = (t == DCO_EGroupTask.SUPPORT_BY_FIRE || t == DCO_EGroupTask.ATTACK || t == DCO_EGroupTask.FLANK) && !g.GetGroupObjective();
			if (!orphan || !ag || ag.GetCurrentWaypoint() || g.IsMoving() || g.IsBreakingContact())
			{
				m_mDCONoWpSince.Remove(g);
				continue;
			}
			float since;
			if (!m_mDCONoWpSince.Find(g, since))
			{
				m_mDCONoWpSince.Set(g, worldTime);
				continue;
			}
			if (worldTime - since < 60.0)
				continue;
			m_mDCONoWpSince.Remove(g);
			g.SetTask(DCO_EGroupTask.NONE, "AICommanderBase:2778");
			DCO_Perf.Count("task_no_wp:release");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), "task_no_wp grp=" + g.GetOwner().GetName() + " task=" + typename.EnumToString(DCO_EGroupTask, t) + " action=release reason=no_objective");
		}
	}

	vector DCO_AttackerCentroid()
	{
		vector sum = vector.Zero;
		int n = 0;
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (!g || !g.GetOwner() || g.IsPlayerGroup() || g.IsMortar() || g.IsDedicatedTransport() || g.IsMedicTeam())
				continue;
			sum = sum + g.GetOwner().GetOrigin();
			n++;
		}
		if (n == 0)
			return GetOwner().GetOrigin();
		return sum * (1.0 / n);
	}

	protected bool StagingSideOk(vector objPos, float objRadius, vector from, vector pos, out float sideDeg, out bool pathClear)
	{
		vector toStage = DCO_Specialist.Flat(pos - objPos);
		vector toFrom = DCO_Specialist.Flat(from - objPos);
		sideDeg = Math.Acos(Math.Clamp(vector.Dot(toStage, toFrom), -1.0, 1.0)) * Math.RAD2DEG;
		vector ab = pos - from;
		ab[1] = 0;
		vector ap = objPos - from;
		ap[1] = 0;
		float lenSq = ab.LengthSq();
		float t = 0;
		if (lenSq > 1.0)
			t = Math.Clamp((ap[0] * ab[0] + ap[2] * ab[2]) / lenSq, 0.0, 1.0);
		pathClear = vector.DistanceXZ(objPos, from + ab * t) > objRadius + m_fStagingPathClearance;
		return sideDeg <= 90.0 && pathClear;
	}

	protected void ClearStagingPos(CMD_AICommanderObjectiveComponent obj)
	{
		vector stp;
		float sst;
		if (m_mStagingPos.Find(obj, stp) && !m_mAssaultReleased.Contains(obj) && m_mStagingStartTime.Find(obj, sst))
			DCO_CmdLearn.StagingEpisode(this, obj, stp, sst, "clear");
		if (m_mStagingPos.Contains(obj))
			m_mStagingPos.Remove(obj);
	}

	protected vector PullStagingClearOfHostileObjectives(
		CMD_AICommanderObjectiveComponent target,
		vector base, vector axis, float legDist, float standoff, vector candidate)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return candidate;


		vector targetPos = target.GetOwner().GetOrigin();

		for (int attempt = 0; attempt < m_iStagingClearTries; attempt++)
		{
			bool clashes = false;

			foreach (CMD_AICommanderObjectiveComponent other : mgr.m_aObjective)
			{
				if (!other || !other.GetOwner() || other == target)
					continue;

				if (other.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
					continue;

				float keepOut = other.GetRadius() + m_fHostileObjKeepOut;

				if (vector.DistanceSq(candidate, other.GetOwner().GetOrigin()) < keepOut * keepOut)
				{
					clashes = true;
					break;
				}
			}

			if (!clashes)
				return candidate;

			standoff = standoff + m_fStagingClearStep;

			if (standoff >= legDist)
				return candidate;

			candidate = targetPos - axis * standoff;
		}

		return candidate;
	}
}
