class CMD_ReconSpotFinder
{
	static float EYE_HEIGHT               = 1.6;
	static float WEIGHT_LOS               = 0.55;
	static float WEIGHT_ELEVATION         = 0.30;
	static float WEIGHT_NEAR_SIDE         = 0.15;
	static float OPTIMAL_HEIGHT_ADVANTAGE = 15.0;
	static float ARC_HALF_DEG             = 75.0;

	static vector FindBestReconSpot(
		vector observerBase,
		vector targetPos,
		float  minDistToTarget,
		float  maxDistToTarget,
		int    candidateCount  = 16)
	{
		if (!Replication.IsServer())
			return vector.Zero;

		if (maxDistToTarget < minDistToTarget)
			maxDistToTarget = minDistToTarget;

		BaseWorld world = GetGame().GetWorld();

		vector toObs = observerBase - targetPos;
		toObs[1] = 0;
		float baseAngle;
		if (toObs.LengthSq() < 1.0)
			baseAngle = Math.RandomFloat(0, 2.0 * Math.PI);
		else
			baseAngle = Math.Atan2(toObs[2], toObs[0]);

		vector bestPos   = vector.Zero;
		float  bestScore = -1.0;
		float  arcHalf   = ARC_HALF_DEG * Math.DEG2RAD;

		for (int i = 0; i < candidateCount; i++)
		{
			float t = (i + 0.5) / candidateCount;
			float ang = baseAngle - arcHalf + (2.0 * arcHalf * t) + Math.RandomFloat(-0.08, 0.08);
			float dist;
			if (i % 2 == 0)
				dist = Math.Lerp(minDistToTarget, maxDistToTarget, 0.25);
			else
				dist = Math.Lerp(minDistToTarget, maxDistToTarget, 0.8);

			float cx = targetPos[0] + Math.Cos(ang) * dist;
			float cz = targetPos[2] + Math.Sin(ang) * dist;
			vector candidate = Vector(cx, world.GetSurfaceY(cx, cz), cz);

			if (DCO_SectorMath.IsInWater(candidate))
				continue;

			float nearSide = 1.0 - Math.AbsFloat(ang - baseAngle) / arcHalf;

			float score = (ScoreLOS(candidate, targetPos) * WEIGHT_LOS)
				+ (ScoreElevation(candidate, targetPos) * WEIGHT_ELEVATION)
				+ (Math.Clamp(nearSide, 0, 1) * WEIGHT_NEAR_SIDE);

			if (score > bestScore)
			{
				bestScore = score;
				bestPos   = candidate;
			}
		}

		return bestPos;
	}

	protected static float ScoreElevation(vector candidatePos, vector targetPos)
	{
		float heightDiff = candidatePos[1] - targetPos[1];
		if (heightDiff <= 0.0)
			return 0.0;

		return Math.Clamp(heightDiff / OPTIMAL_HEIGHT_ADVANTAGE, 0.0, 1.0);
	}

	protected static float ScoreLOS(vector candidatePos, vector targetPos)
	{
		TraceParam trace = new TraceParam();
		trace.Start = Vector(candidatePos[0], candidatePos[1] + EYE_HEIGHT, candidatePos[2]);
		trace.End   = Vector(targetPos[0], targetPos[1] + 1.5, targetPos[2]);
		trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;

		DCO_Perf.Count("t:ReconFindSpot");
		float hitFraction = GetGame().GetWorld().TraceMove(trace, null);

		return Math.Clamp((hitFraction - 0.5) / 0.4, 0.0, 1.0);
	}
}
