static bool IsInOpenArea(IEntity aiEntity, float checkRadius = 15.0)
{
	if (!aiEntity) return false;

	vector center = aiEntity.GetOrigin();
	center[1] = center[1] + 1.0;

	BaseWorld world = GetGame().GetWorld();
	int openDirections = 0;
	const int totalDirections = 8;

	for (int i = 0; i < totalDirections; i++)
	{
		float angle = (i * (360.0 / totalDirections)) * Math.DEG2RAD;

		vector endPos = center;
		endPos[0] = center[0] + (Math.Sin(angle) * checkRadius);
		endPos[2] = center[2] + (Math.Cos(angle) * checkRadius);

		TraceParam param = new TraceParam();
		param.Start = center;
		param.End = endPos;
		param.Exclude = aiEntity;
		param.LayerMask = EPhysicsLayerDefs.Projectile;

		float hitFraction = world.TraceMove(param, null);

		if (hitFraction == 1.0)
		{
			openDirections++;
		}
	}

	bool horizontallyEnclosed = (openDirections < 6);

	const float ROOF_CHECK_HEIGHT = 8.0;

	if (horizontallyEnclosed)
	{
		TraceParam roofParam = new TraceParam();
		roofParam.Start = center;
		roofParam.End   = center + (vector.Up * ROOF_CHECK_HEIGHT);
		roofParam.Exclude = aiEntity;
		roofParam.LayerMask = EPhysicsLayerDefs.Projectile;

		float roofHitFraction = world.TraceMove(roofParam, null);
		bool hasRoof = (roofHitFraction < 1.0);

		if (!hasRoof)
			return true;
	}

	if (openDirections >= 6)
	{
		return true;
	}

	return false;
}