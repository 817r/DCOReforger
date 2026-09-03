class DCO_AimUtility
{
	protected static const float EYE_HEIGHT_M       = 1.6;
	protected static const float LOS_TRACE_MIN      = 0.5;
	protected static const float STALE_TARGET_POS_S = 12.0;

	static vector ResolveOrientationPos(IEntity myEntity, vector threatPos, float traceFraction, float timeSinceSeen)
	{
		if (!myEntity)
			return threatPos;

		if (traceFraction > LOS_TRACE_MIN && timeSinceSeen < STALE_TARGET_POS_S)
			return threatPos;

		vector flat = threatPos;
		flat[1] = myEntity.GetOrigin()[1] + EYE_HEIGHT_M;
		return flat;
	}
}