class DCO_OpenAreaCache
{
	vector m_vPos;
	float m_fRadius;
	float m_fAt_ms;
	bool m_bOpen;

	protected static ref map<IEntity, ref DCO_OpenAreaCache> s_mCache;
	protected static map<IEntity, ref DCO_OpenAreaCache> GetCache()
	{
		if (!s_mCache)
			s_mCache = new map<IEntity, ref DCO_OpenAreaCache>();
		return s_mCache;
	}

	static DCO_OpenAreaCache Get(IEntity e, float now)
	{
		DCO_OpenAreaCache c = GetCache().Get(e);
		if (c)
			return c;
		if (GetCache().Count() > 1024)
		{
			for (int i = GetCache().Count() - 1; i >= 0; i--)
			{
				DCO_OpenAreaCache o = GetCache().GetElement(i);
				if (!GetCache().GetKey(i) || !o || now - o.m_fAt_ms > 10000.0)
					GetCache().RemoveElement(i);
			}
		}
		c = new DCO_OpenAreaCache();
		c.m_fAt_ms = -100000.0;
		GetCache().Set(e, c);
		return c;
	}
}

static bool IsInOpenArea(IEntity aiEntity, float checkRadius = 15.0)
{
	if (!aiEntity) return false;

	float now = aiEntity.GetWorld().GetWorldTime();
	DCO_OpenAreaCache cache = DCO_OpenAreaCache.Get(aiEntity, now);
	if (now - cache.m_fAt_ms < 2000.0 && cache.m_fRadius == checkRadius && vector.DistanceSq(cache.m_vPos, aiEntity.GetOrigin()) < 2.25)
	{
		DCO_Perf.Count("open:hit");
		return cache.m_bOpen;
	}
	DCO_Perf.Count("open:miss");
	cache.m_vPos = aiEntity.GetOrigin();
	cache.m_fRadius = checkRadius;
	cache.m_fAt_ms = now;
	cache.m_bOpen = DCO_TraceOpenArea(aiEntity, checkRadius);
	return cache.m_bOpen;
}

static bool DCO_TraceOpenArea(IEntity aiEntity, float checkRadius)
{
	vector center = aiEntity.GetOrigin();
	center[1] = center[1] + 1.0;

	int openDirections = 0;

	for (int i = 0; i < 8; i++)
	{
		float angle = (i * (360.0 / 8)) * Math.DEG2RAD;

		vector endPos = center;
		endPos[0] = center[0] + (Math.Sin(angle) * checkRadius);
		endPos[2] = center[2] + (Math.Cos(angle) * checkRadius);

		float hitFraction = DCO_Util.TraceFrac(center, endPos, aiEntity, -1, EPhysicsLayerDefs.Projectile, "t:DCO_IsAIInOpenArea");

		if (hitFraction == 1.0)
		{
			openDirections++;
		}
		else if (i - openDirections >= 2)
			break;
	}

	bool horizontallyEnclosed = (openDirections < 6);


	if (horizontallyEnclosed)
	{
		float roofHitFraction = DCO_Util.TraceFrac(center, center + (vector.Up * 8.0), aiEntity, -1, EPhysicsLayerDefs.Projectile, "t:DCO_IsAIInOpenArea");
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
