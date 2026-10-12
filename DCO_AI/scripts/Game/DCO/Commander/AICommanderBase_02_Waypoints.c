modded class AICommander_BaseComponent
{
	protected void GenerateSearchWaypoints(vector center, float radius, array<SCR_AIWaypoint> outWaypoints, float wpSpacing = 50.0, float angleOffset = 0.0, float angleSpan = 360.0)
	{
	    if (!outWaypoints)
	        return;

	    outWaypoints.Clear();

	    if (angleSpan <= 0.0)
	        angleSpan = 360.0;

	    int budget = Math.Max(2, m_iMaxSearchWaypoints);

	    int rings = Math.Max(1, (int)Math.Round(radius / wpSpacing));

	    int maxRings = Math.Max(1, budget / m_iMinWpPerRing);
	    if (rings > maxRings)
	        rings = maxRings;

	    int baseSectorsPerRing = Math.Max(2, (int)Math.Round((radius * 2 * Math.PI) / (wpSpacing * m_fRingSectorSpacing)));

	    float ringStep  = radius / rings;
	    float spanRatio = angleSpan / 360.0;

	    array<int> sectorsPerRing = {};
	    int wanted = 0;

	    for (int ringCalc = rings; ringCalc >= 1; ringCalc--)
	    {
	        float ringRatioCalc = (float)ringCalc / rings;
	        int   cs = Math.Max(2, (int)Math.Round(1.0 * baseSectorsPerRing * ringRatioCalc * spanRatio));

	        sectorsPerRing.Insert(cs);
	        wanted = wanted + cs;
	    }

	    if (wanted > budget)
	    {
	        float scale = budget / (float)wanted;

	        for (int si = 0; si < sectorsPerRing.Count(); si++)
	            sectorsPerRing.Set(si, Math.Max(1, (int)Math.Round(sectorsPerRing.Get(si) * scale)));
	    }

	    int ringIndex = 0;

	    for (int ring = rings; ring >= 1; ring--)
	    {
	        float radiusInner = ringStep * (ring - 1);
	        float radiusOuter = ringStep * ring;

	        int currentSectors = sectorsPerRing.Get(ringIndex);
	        ringIndex          = ringIndex + 1;

	        float sectorAngle = angleSpan / currentSectors;

	        for (int sector = 0; sector < currentSectors; sector++)
	        {
	            if (outWaypoints.Count() >= budget)
	                return;

	            float angleMin = angleOffset + sectorAngle * sector;
	            float angleMax = angleOffset + sectorAngle * (sector + 1);
	            float angleDeg = Math.RandomFloat(angleMin, angleMax);
	            float angleRad = angleDeg * Math.DEG2RAD;

	            float dist = Math.RandomFloat(radiusInner + 1.0, radiusOuter);

	            float px = center[0] + Math.Cos(angleRad) * dist;
	            float pz = center[2] + Math.Sin(angleRad) * dist;
	            float py = GetGame().GetWorld().GetSurfaceY(px, pz);

	            vector p = Vector(px, py, pz);

	            EWaterSurfaceType waterType = EWaterSurfaceType.WST_NONE;
	            float lakeArea = 0;
	            float waterY   = SCR_WorldTools.GetWaterSurfaceY(null, p, waterType, lakeArea);

	            if (py < waterY && waterType != EWaterSurfaceType.WST_NONE)
	                continue;

	            SCR_AIWaypoint wp = SpawnMoveWP(p);
	            if (wp)
	                outWaypoints.Insert(wp);
	        }
	    }
	}

	bool SpawnMoveRoute(DCO_GroupUtilityComponent grp, vector from, vector to, float worldTime)
	{
		if (!grp)
			return false;

		float total = vector.Distance(from, to);
		array<SCR_AIWaypoint> addedLegs = {};

		if (m_fWaypointLegDistance > 0.0 && total > m_fWaypointLegDistance)
		{
			int legs = Math.Floor(total / m_fWaypointLegDistance);

			for (int i = 1; i < legs; i++)
			{
				float t = i / (float)legs;

				vector leg = from + ((to - from) * t);

				vector safeLeg;
				if (!FindSafeRoutePoint(leg, to, safeLeg))
					continue;

				SCR_AIWaypoint legWp = SpawnMoveWP(DCO_Util.Surface(safeLeg + Vector(Math.RandomFloat(-m_fRouteLegJitter, m_fRouteLegJitter), 0.0, Math.RandomFloat(-m_fRouteLegJitter, m_fRouteLegJitter))));
				if (!legWp)
					legWp = SpawnMoveWP(safeLeg);
				if (legWp)
				{
					grp.MoveTo(legWp, worldTime);
					addedLegs.Insert(legWp);
				}
			}
		}

		float jitterAng = Math.RandomFloat(0.0, Math.PI2);
		float jitterDist = Math.RandomFloat(m_fDestJitterMin, m_fDestJitterMax);
		SCR_AIWaypoint destWp = SpawnMoveWP(DCO_Util.Surface(to + Vector(Math.Cos(jitterAng) * jitterDist, 0.0, Math.Sin(jitterAng) * jitterDist)));
		if (!destWp)
			destWp = SpawnMoveWP(to);
		if (!destWp)
		{
			AIGroup aiGrp = AIGroup.Cast(grp.GetOwner());
			foreach (SCR_AIWaypoint leg : addedLegs)
			{
				if (!leg)
					continue;
				if (aiGrp)
					aiGrp.RemoveWaypoint(leg);
				SCR_EntityHelper.DeleteEntityAndChildren(leg);
			}
			if (!addedLegs.IsEmpty() && !grp.IsGroupHaveWaypoint())
				grp.SetPhase(DCO_ETaskPhase.HOLDING);
			return false;
		}

		grp.MoveTo(destWp, worldTime);
		return true;
	}

	protected bool FindSafeRoutePoint(vector candidate, vector heading, out vector result)
	{
		vector dir = heading - candidate;
		dir        = Vector(dir[0], 0.0, dir[2]);

		if (dir.LengthSq() < 1.0)
			dir = Vector(1.0, 0.0, 0.0);
		else
			dir = dir.Normalized();

		vector side = Vector(-dir[2], 0.0, dir[0]);


		for (int step = 0; step <= m_iNudgeSteps; step++)
		{
			float offset = (step / (float)m_iNudgeSteps) * m_fWaypointNudgeRadius;

			for (int s = 0; s < 2; s++)
			{
				float signedOffset = offset;
				if (s == 1)
					signedOffset = -offset;

				vector test = candidate + (side * signedOffset);
				test[1]     = GetGame().GetWorld().GetSurfaceY(test[0], test[2]);

				if (IsRoutePointUsable(test))
				{
					result = test;
					return true;
				}
			}

			if (offset <= 0.0)
				continue;
		}

		return false;
	}

	protected bool IsRoutePointUsable(vector pos)
	{
		EWaterSurfaceType waterType = EWaterSurfaceType.WST_NONE;
		float lakeArea = 0;
		float waterY   = SCR_WorldTools.GetWaterSurfaceY(null, pos, waterType, lakeArea);

		if (pos[1] < waterY && waterType != EWaterSurfaceType.WST_NONE)
			return false;

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return true;

		foreach (CMD_AICommanderObjectiveComponent other : mgr.m_aObjective)
		{
			if (!other || !other.GetOwner())
				continue;

			if (other.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
				continue;

			float keepOut = other.GetRadius();

			if (vector.DistanceSq(pos, other.GetOwner().GetOrigin()) < keepOut * keepOut)
				return false;
		}

		return true;
	}

	SCR_AIWaypoint SpawnMoveWP(vector pos, EMovementType moveType = EMovementType.RUN)
	{
	    AICommander_BaseComponentClass data = AICommander_BaseComponentClass.Cast(GetComponentData(GetOwner()));
	    if (!data)
	        return null;

	    Resource res = Resource.Load(data.GetDefaultMoveWaypointPrefab());
	    if (!res || !res.IsValid())
	        return null;

	    BaseWorld world = GetGame().GetWorld();
	    if (!world)
	        return null;

		EWaterSurfaceType waterType = EWaterSurfaceType.WST_NONE;

	    float surfaceY = world.GetSurfaceY(pos[0], pos[2]);
		float lakeArea = 0;

	    float waterY = SCR_WorldTools.GetWaterSurfaceY(null, pos, waterType, lakeArea);
	    if (surfaceY < waterY)
	    {
			if (waterType == EWaterSurfaceType.WST_OCEAN || waterType == EWaterSurfaceType.WST_RIVER || waterType == EWaterSurfaceType.WST_POND)
	        	return null;
	    }

	    pos[1] = surfaceY;
	    EntitySpawnParams params = EntitySpawnParams();
	    params.TransformMode = ETransformMode.WORLD;
	    Math3D.MatrixIdentity4(params.Transform);
	    params.Transform[3] = pos;

		SCR_AIWaypoint wp = SCR_AIWaypoint.Cast(GetGame().SpawnEntityPrefab(res, null, params));
		if (!wp)
			return null;

		if (SCR_WorldTools.IsObjectUnderwater(wp))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(wp);
			return null;
		}

		wp.SetCompletionRadius(m_fMoveWpRadius);
		wp.SetCompletionType(EAIWaypointCompletionType.All);
		SCR_AIGroupCharactersMovementSpeedSetting mspeed = SCR_AIGroupCharactersMovementSpeedSetting.Create(SCR_EAISettingOrigin.BEHAVIOR, moveType);
		wp.AddSetting(mspeed);

	    return wp;
	}

	SCR_AIWaypoint SpawnArtilleryWP(vector pos)
	{
	    AICommander_BaseComponentClass data = AICommander_BaseComponentClass.Cast(GetComponentData(GetOwner()));
	    if (!data)
	        return null;

	    Resource res = Resource.Load(data.GetShootArtilleryWaypointPrefab());
	    if (!res || !res.IsValid())
	        return null;

	    BaseWorld world = GetGame().GetWorld();
	    if (!world)
	        return null;

		EWaterSurfaceType waterType = EWaterSurfaceType.WST_NONE;

	    float surfaceY = world.GetSurfaceY(pos[0], pos[2]);
		float lakeArea = 0;

	    float waterY = SCR_WorldTools.GetWaterSurfaceY(null, pos, waterType, lakeArea);
	    if (surfaceY < waterY)
	    {
			if (waterType == EWaterSurfaceType.WST_OCEAN)
	        	return null;
	    }

	    pos[1] = surfaceY;
	    EntitySpawnParams params = EntitySpawnParams();
	    params.TransformMode = ETransformMode.WORLD;
	    Math3D.MatrixIdentity4(params.Transform);
	    params.Transform[3] = pos;

	    return SCR_AIWaypoint.Cast(GetGame().SpawnEntityPrefab(res, null, params));
	}

	vector DCO_SuppressAimPos(vector pos)
	{
		CMD_ThreatResponseComponent tr = GetThreatResponseComponent();
		if (!tr)
			return pos;
		CMD_ThreatEntry best;
		float bestU = float.MAX;
		foreach (CMD_ThreatEntry t : tr.GetThreats())
		{
			if (!t || vector.DistanceXZ(t.m_vBelievedPos, pos) > m_fSuppressIntelMatch)
				continue;
			if (t.m_fBelievedUncertainty < bestU)
			{
				bestU = t.m_fBelievedUncertainty;
				best = t;
			}
		}
		if (!best)
		{
			DCO_Perf.Count("supp_wp:center");
			return pos;
		}
		DCO_Perf.Count("supp_wp:intel");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.DEBUG, DCO_Log.IdCmd(this), string.Format("supp_wp_aim cmd=%1 from=%2 to=%3 shift_m=%4 unc=%5", m_sCommanderUID, DCO_Util.XZ(pos), DCO_Util.XZ(best.m_vBelievedPos), Math.Round(vector.DistanceXZ(pos, best.m_vBelievedPos)), Math.Round(bestU)));
		return best.m_vBelievedPos;
	}

	SCR_AIWaypoint SpawnSuppressWP(vector pos)
	{
		pos = DCO_SuppressAimPos(pos);
	    AICommander_BaseComponentClass data = AICommander_BaseComponentClass.Cast(GetComponentData(GetOwner()));
	    if (!data)
	        return null;

	    Resource res = Resource.Load(data.GetDefaultSuppressPrefab());
	    if (!res || !res.IsValid())
	        return null;

	    EntitySpawnParams params = EntitySpawnParams();
	    params.TransformMode = ETransformMode.WORLD;
	    Math3D.MatrixIdentity4(params.Transform);
	    params.Transform[3] = pos;

	    return SCR_AIWaypoint.Cast(GetGame().SpawnEntityPrefab(res, null, params));
	}

	SCR_AIWaypoint SpawnDefendWP(vector pos)
	{
		AICommander_BaseComponentClass data = AICommander_BaseComponentClass.Cast(GetComponentData(GetOwner()));
		if (!data)
			return null;

		Resource res = Resource.Load(data.GetDefaultDefendWaypointPrefab());
		if (!res || !res.IsValid())
			return null;

		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = pos;

		return SCR_AIWaypoint.Cast(GetGame().SpawnEntityPrefab(res, null, params));
	}

	ResourceName GetGarrisonWaypointPrefab()
	{
		if (!m_sDCOGarrisonWpOverride.IsEmpty())
			return m_sDCOGarrisonWpOverride;
		AICommander_BaseComponentClass data = AICommander_BaseComponentClass.Cast(GetComponentData(GetOwner()));
		if (data)
			return data.GetGarrisonWaypointPrefab();
		return string.Empty;
	}

	ResourceName GetTacticRoleWaypointPrefab()
	{
		AICommander_BaseComponentClass data = AICommander_BaseComponentClass.Cast(GetComponentData(GetOwner()));
		if (data)
			return data.GetTacticRoleWaypointPrefab();
		return string.Empty;
	}

	DCO_GarrisonWaypoint SpawnGarrisonWP(vector pos, float threatAngle, float radius, bool holdMode = false)
	{
		Resource res = Resource.Load(GetGarrisonWaypointPrefab());
		if (!res || !res.IsValid())
			return null;

		vector dir = Vector(Math.Cos(threatAngle), 0, Math.Sin(threatAngle));
		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.AnglesToMatrix(Vector(dir.ToYaw(), 0, 0), params.Transform);
		pos = DCO_Util.Surface(pos);
		params.Transform[3] = pos;

		DCO_GarrisonWaypoint wp = DCO_GarrisonWaypoint.Cast(GetGame().SpawnEntityPrefab(res, null, params));
		if (!wp)
			return null;

		wp.SetCompletionRadius(Math.Max(radius, m_fGarrisonWpMinRadius));
		if (holdMode)
			wp.SetMode(DCO_EGarrisonMode.HOLD);
		else
			wp.SetMode(m_eGarrisonMode);
		return wp;
	}

	SCR_AIWaypoint SpawnGetInWP(vector pos)
	{
		AICommander_BaseComponentClass data = AICommander_BaseComponentClass.Cast(GetComponentData(GetOwner()));
		if (!data)
			return null;

		Resource res = Resource.Load(data.GetDefaultGetInWaypointPrefab());
		if (!res || !res.IsValid())
			return null;

		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = pos;

		SCR_AIWaypoint wp = SCR_AIWaypoint.Cast(GetGame().SpawnEntityPrefab(res, null, params));
		if (wp)
			wp.SetCompletionRadius(m_fGetInWpRadius);
		return wp;
	}

	SCR_AIWaypoint SpawnGetOutWP(vector pos)
	{
		AICommander_BaseComponentClass data = AICommander_BaseComponentClass.Cast(GetComponentData(GetOwner()));
		if (!data)
			return null;

		Resource res = Resource.Load(data.GetDefaultGetOutWaypointPrefab());
		if (!res || !res.IsValid())
			return null;

		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = pos;

		return SCR_AIWaypoint.Cast(GetGame().SpawnEntityPrefab(res, null, params));
	}
}
