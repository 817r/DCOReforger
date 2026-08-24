class CMD_ReconSpotFinder
{
	static float EYE_HEIGHT               = 1.2;
	static float WEIGHT_LOS                = 0.55;
	static float WEIGHT_ELEVATION          = 0.30;
	static float WEIGHT_DIST               = 0.15;
	static float OPTIMAL_DIST              = 80.0;
	static float OPTIMAL_HEIGHT_ADVANTAGE  = 15.0;

	static vector FindBestReconSpot(
		vector observerBase,
		vector targetPos,
		float  searchRadius    = 300.0,
		float  minDistToTarget = 200.0,
		int    candidateCount  = 16)
	{
		if (!Replication.IsServer())
			return vector.Zero;

		vector searchCenter = (observerBase + targetPos) * 0.5;

		vector bestPos   = vector.Zero;
		float  bestScore = -1.0;

		int perRing = candidateCount / 2;

		for (int ring = 0; ring < 2; ring++)
		{
			float ringRadius;
			if (ring == 0)
				ringRadius = searchRadius * 0.6;
			else
				ringRadius = searchRadius * 1.0;

			for (int i = 0; i < perRing; i++)
			{
				float baseAngle = i * 360.0 / perRing;
				float jitter    = Math.RandomFloat(-15.0, 15.0);
				float angleDeg  = baseAngle + jitter;
				float angleRad  = angleDeg * Math.DEG2RAD;

				float cx = searchCenter[0] + Math.Cos(angleRad) * ringRadius;
				float cz = searchCenter[2] + Math.Sin(angleRad) * ringRadius;
				float cy = GetGame().GetWorld().GetSurfaceY(cx, cz);

				vector candidate = Vector(cx, cy, cz);

				float distToTarget = vector.Distance(candidate, targetPos);
				if (distToTarget < minDistToTarget)
					continue;

				float distToBase = vector.Distance(candidate, observerBase);
				if (distToBase > searchRadius * 2.0)
					continue;

				float score = EvaluateCandidate(candidate, targetPos);

				if (score > bestScore)
				{
					bestScore = score;
					bestPos   = candidate;
				}
			}
		}

		if (bestScore < 0.1)
		{
			//Print("[CMD_ReconSpot] Tidak ada posisi valid (best score: " + bestScore + ")",
			//	LogLevel.WARNING);
			return vector.Zero;
		}

		//Print(string.Format("[CMD_ReconSpot] Best spot: %1 | score: %2",
			//bestPos.ToString(), bestScore.ToString()));
		
		float SurfaceY = GetGame().GetWorld().GetSurfaceY(bestPos[0], bestPos[2]);
		bestPos[1] = SurfaceY;
		
		return bestPos;
	}

	protected static float EvaluateCandidate(vector candidatePos, vector targetPos)
	{
		vector eyePos  = Vector(candidatePos[0], candidatePos[1] + EYE_HEIGHT, candidatePos[2]);
		vector targEye = Vector(targetPos[0], targetPos[1] + 1.0, targetPos[2]);

		float losScore = ScoreLOS(eyePos, targEye);

		if (losScore <= 0.0)
			return 0.0;

		float distScore = ScoreDistance(candidatePos, targetPos);
		
		float elevScore = ScoreElevation(candidatePos, targetPos);

		return (losScore * WEIGHT_LOS) + (distScore * WEIGHT_DIST) + (elevScore * WEIGHT_ELEVATION);
	}

	protected static float ScoreElevation(vector candidatePos, vector targetPos)
	{
		float heightDiff = candidatePos[1] - targetPos[1];
		
		if (heightDiff <= 0.0)
			return 0.0;
		
		return Math.Clamp(heightDiff / OPTIMAL_HEIGHT_ADVANTAGE, 0.0, 1.0);
	}

	protected static float ScoreLOS(vector fromPos, vector toPos)
	{
		TraceParam trace = new TraceParam();
		trace.Start = fromPos;
		trace.End   = toPos;
		trace.Flags = TraceFlags.ANY_CONTACT;

		float hitFraction = GetGame().GetWorld().TraceMove(trace, null);

		if (hitFraction >= 1.0)
			return 1.0;

		if (hitFraction >= 0.85)
			return 0.5;

		return 0.0;
	}

	protected static float ScoreDistance(vector candidatePos, vector targetPos)
	{
		float dist      = vector.Distance(candidatePos, targetPos);
		float deviation = Math.AbsFloat(dist - OPTIMAL_DIST);
		float tolerance = 40.0;

		return Math.Max(0.0, 1.0 - (deviation / tolerance));
	}
}