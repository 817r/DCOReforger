class DCO_ConcealmentUtility
{
	static bool IsPositionExposed(vector pos, IEntity excludeEntity, float checkRadius = 15.0)
	{
		int perfStart = DCO_Perf.Begin();
		vector center = pos;
		center[1] = center[1] + 1.0;

		int openDirections = 0;

		for (int i = 0; i < 8; i++)
		{
			float angle = (i * 45.0) * Math.DEG2RAD;

			vector endPos = center;
			endPos[0] = center[0] + (Math.Sin(angle) * checkRadius);
			endPos[2] = center[2] + (Math.Cos(angle) * checkRadius);

			float hitFraction = DCO_Util.TraceFrac(center, endPos, excludeEntity, -1, EPhysicsLayerDefs.Projectile, "t:DCO_ConcealmentUtility");

			if (hitFraction == 1.0)
				openDirections++;
		}

		DCO_Perf.End("expose", perfStart);
		return openDirections >= 6;
	}

	static bool IsSkylined(vector pos, vector threat, float eye = 1.7)
	{
		BaseWorld w = GetGame().GetWorld();
		vector head = Vector(pos[0], w.GetSurfaceY(pos[0], pos[2]) + eye, pos[2]);
		vector e = Vector(threat[0], w.GetSurfaceY(threat[0], threat[2]) + 1.7, threat[2]);
		vector dir = head - e;
		float len = dir.Length();
		if (len < 30.0)
			return false;
		dir = dir * (1.0 / len);
		DCO_Perf.Count("skyline_check");
		for (float d = 25.0; d < 1500.0; d = d * 2.5)
		{
			vector q = head + dir * d;
			if (q[1] <= w.GetSurfaceY(q[0], q[2]))
				return false;
		}
		return true;
	}
}
