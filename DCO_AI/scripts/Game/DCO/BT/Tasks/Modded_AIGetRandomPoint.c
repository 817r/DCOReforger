modded class SCR_AIGetRandomPointWithExclude
{
	protected static bool s_bDCOReportedZeroRadius;

	override protected bool FindPosition2D(out vector randomPos, vector randomSphereOrigin, float randomSphereRadius, vector excludeSphereOrigin = vector.Zero, float excludeRadius = 0, int iterationCount = 50)
	{
		float minRadius = excludeRadius + 0.5;
		if (randomSphereRadius < minRadius)
		{
			if (!s_bDCOReportedZeroRadius)
			{
				s_bDCOReportedZeroRadius = true;
				Print(string.Format("[DCO] SCR_AIGetRandomPointWithExclude radius %1 dijepit ke %2 di %3. Laporan sekali per sesi.", randomSphereRadius, minRadius, randomSphereOrigin), LogLevel.WARNING);
			}
			randomSphereRadius = minRadius;
		}

		return super.FindPosition2D(randomPos, randomSphereOrigin, randomSphereRadius, excludeSphereOrigin, excludeRadius, iterationCount);
	}
}

modded class SCR_AIGetRandomPoint
{
	protected static bool s_bDCOReportedZeroRadius;

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		vector origin = vector.Zero;
		IEntity waypointEnt;

		if (GetVariableIn(WAYPOINT_PORT, waypointEnt))
		{
			AIWaypoint waypoint = AIWaypoint.Cast(waypointEnt);
			if (!waypoint)
			{
				NodeError(this, owner, WAYPOINT_PORT + " is null! or wrong type");
				return ENodeResult.FAIL;
			}
			m_Radius = waypoint.GetCompletionRadius();
			origin = waypoint.GetOrigin();
		}
		else
		{
			if (GetVariableType(true, RADIUS_PORT) == int)
			{
				int radius;
				GetVariableIn(RADIUS_PORT, radius);
				m_Radius = radius;
			}
			else if (GetVariableType(true, RADIUS_PORT) == float)
			{
				GetVariableIn(RADIUS_PORT, m_Radius);
			}

			if (GetVariableType(true, ORIGIN_PORT) == vector)
				GetVariableIn(ORIGIN_PORT, origin);
		}

		float minRadius = m_ExclusionRadius + 0.5;
		if (m_Radius < minRadius)
		{
			if (!s_bDCOReportedZeroRadius)
			{
				s_bDCOReportedZeroRadius = true;
				string who = "?";
				if (owner && owner.GetParentGroup())
					who = owner.GetParentGroup().GetName();
				Print(string.Format("[DCO] SCR_AIGetRandomPoint radius %1 dijepit ke %2 (group %3, waypoint %4). Laporan sekali per sesi.", m_Radius, minRadius, who, waypointEnt), LogLevel.WARNING);
			}
			m_Radius = minRadius;
		}

		vector result = s_AIRandomGenerator.GenerateRandomPointInRadius(m_ExclusionRadius, m_Radius, origin, true);
		result[1] = origin[1];
		SetVariableOut(PORT_RESULT_VECTOR, result);

		return ENodeResult.SUCCESS;
	}
}
