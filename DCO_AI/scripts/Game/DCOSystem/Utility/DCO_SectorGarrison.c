class DCO_SectorGarrison
{
	int                       m_iSectorIndex;
	vector                    m_vPosition;
	SCR_AIWaypoint            m_Waypoint;
	DCO_GroupUtilityComponent m_Group;

	void DCO_SectorGarrison(int sectorIndex)
	{
		m_iSectorIndex = sectorIndex;
		m_vPosition    = vector.Zero;
	}

	bool IsStaffed()
	{
		return m_Waypoint != null;
	}

	bool NeedsReplenish()
	{
		return m_Waypoint != null && m_Group == null;
	}
}

class DCO_SectorMath
{
	static const float BAND_INNER        = 0.55;  // batas dalam band penempatan (x radius)
	static const float BAND_OUTER        = 0.90;  // batas luar band penempatan (x radius)
	static const float ANGULAR_INSET     = 0.20;  // margin tiap sisi wedge (fraksi lebar sektor)
	static const float COMPLETION_FACTOR = 0.25;  // completion radius = radius x ini (di-cap)
	static const int   SAMPLE_ATTEMPTS   = 12;

	//------------------------------------------------------------------------------------------------
	static int ComputeSectorCount(float radius, float arcPerSector, float personalityMod, int minSector, int maxSector)
	{
		if (arcPerSector <= 0.0)
			arcPerSector = 60.0;

		if (minSector < 1)
			minSector = 1;

		if (maxSector < minSector)
			maxSector = minSector;

		float circumference = 2.0 * Math.PI * radius;
		int raw = Math.Round((circumference / arcPerSector) * personalityMod);

		return Math.ClampInt(raw, minSector, maxSector);
	}

	//------------------------------------------------------------------------------------------------
	static float ComputeSectorOffset(float threatAngleRad, int sectorCount)
	{
		if (sectorCount <= 0)
			return 0.0;

		return threatAngleRad - (Math.PI / sectorCount);
	}

	//------------------------------------------------------------------------------------------------
	static float ComputeCompletionRadius(float radius, float maxCompletion)
	{
		float value = radius * COMPLETION_FACTOR;
		if (value > maxCompletion)
			value = maxCompletion;

		return value;
	}

	//------------------------------------------------------------------------------------------------
	static float GetSectorMidAngle(int sectorIndex, int sectorCount, float sectorOffset)
	{
		if (sectorCount <= 0)
			return sectorOffset;

		float step = (2.0 * Math.PI) / sectorCount;
		return sectorOffset + (sectorIndex + 0.5) * step;
	}

	static bool SamplePosition(vector center, float radius, int sectorIndex, int sectorCount, float sectorOffset, float minSep, notnull array<vector> taken, out vector result)
	{
		result = center;

		if (sectorCount <= 0 || radius <= 0.0)
			return false;

		float step     = (2.0 * Math.PI) / sectorCount;
		float angStart = sectorOffset + sectorIndex * step + ANGULAR_INSET * step;
		float angEnd   = sectorOffset + (sectorIndex + 1) * step - ANGULAR_INSET * step;
		float minSepSq = minSep * minSep;

		for (int attempt = 0; attempt < SAMPLE_ATTEMPTS; attempt++)
		{
			float ang = Math.RandomFloat(angStart, angEnd);
			float rad = Math.RandomFloat(radius * BAND_INNER, radius * BAND_OUTER);

			vector candidate = Vector(
				center[0] + rad * Math.Cos(ang),
				0.0,
				center[2] + rad * Math.Sin(ang));

			candidate[1] = GetGame().GetWorld().GetSurfaceY(candidate[0], candidate[2]);

			if (IsInWater(candidate))
				continue;

			if (IsTooClose(candidate, taken, minSepSq))
				continue;

			result = candidate;
			return true;
		}

		float midAng = sectorOffset + (sectorIndex + 0.5) * step;
		result = Vector(
			center[0] + radius * BAND_OUTER * Math.Cos(midAng),
			0.0,
			center[2] + radius * BAND_OUTER * Math.Sin(midAng));
		result[1] = GetGame().GetWorld().GetSurfaceY(result[0], result[2]);

		return false;
	}

	static bool IsInWater(vector pos)
	{
		EWaterSurfaceType waterType = EWaterSurfaceType.WST_NONE;
		float lakeArea = 0;
		float waterY = SCR_WorldTools.GetWaterSurfaceY(null, pos, waterType, lakeArea);

		return (pos[1] < waterY && waterType != EWaterSurfaceType.WST_NONE);
	}

	//------------------------------------------------------------------------------------------------
	static bool IsTooClose(vector candidate, notnull array<vector> taken, float minSepSq)
	{
		foreach (vector p : taken)
		{
			if (vector.DistanceSq(p, candidate) < minSepSq)
				return true;
		}

		return false;
	}
}