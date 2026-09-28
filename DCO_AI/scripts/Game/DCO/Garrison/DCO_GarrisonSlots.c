enum DCO_EGarrisonSlotType
{
	WINDOW,
	ROOF,
	DOOR_GUARD,
	INTERIOR
}

class DCO_GarrisonSlot
{
	vector m_vLocalPos;
	float m_fLocalYaw;
	float m_fArc;
	ECharacterStance m_eStance;
	DCO_EGarrisonSlotType m_eType;
	int m_iFloor;
	float m_fScore;
	int m_iFailCount;
	bool m_bUnreachable;

	vector m_vWorldPos;
	vector m_vWorldDir;

	DCO_GarrisonSlot Duplicate()
	{
		DCO_GarrisonSlot s = new DCO_GarrisonSlot();
		s.m_vLocalPos = m_vLocalPos;
		s.m_fLocalYaw = m_fLocalYaw;
		s.m_fArc = m_fArc;
		s.m_eStance = m_eStance;
		s.m_eType = m_eType;
		s.m_iFloor = m_iFloor;
		s.m_fScore = m_fScore;
		return s;
	}

	static vector YawToDir(float yawDeg)
	{
		float r = yawDeg * Math.DEG2RAD;
		return Vector(Math.Sin(r), 0, Math.Cos(r));
	}
}

class DCO_GarrisonDoor
{
	vector m_vPos;
	vector m_vNormal;
	vector m_vSide;
	float m_fHalfWidth;
}

class DCO_GarrisonPrefabSlots
{
	ref array<ref DCO_GarrisonSlot> m_aSlots = {};
	int m_iTraces;
}

class DCO_GarrisonSlotGenerator
{
	protected static const float CELL_XZ = 2.0;
	protected static const float CELL_Y = 2.8;
	protected static const float DEDUPE = 0.8;
	protected static const float MIN_HEADROOM = 1.6;
	protected static const float HEADROOM_PROBE = 6.0;
	protected static const float RAY_LEN = 15.0;
	protected static const float EXIT_MARGIN = 0.5;
	protected static const float WALL_NEAR = 1.2;
	protected static const float H_CROUCH = 1.0;
	protected static const float H_STAND = 1.45;
	protected static const float H_DOOR = 0.3;
	protected static const float DOOR_GUARD_MIN = 3.0;
	protected static const float DOOR_GUARD_MAX = 8.0;
	protected static const float DOOR_CLEAR = 1.5;
	protected static const float DOOR_DEPTH = 1.0;
	protected static const float DOOR_SIDE_PAD = 0.3;
	protected static const float SAME_FLOOR_DY = 1.5;
	protected static const int MAX_INTERIOR = 8;
	protected static const int DIRS = 8;
	protected static const float ROOF_MIN_NORMAL_Y = 0.96;

	IEntity m_Building;
	ResourceName m_sPrefab;
	ref DCO_GarrisonPrefabSlots m_Result = new DCO_GarrisonPrefabSlots();
	bool m_bDone;

	protected vector m_vMins, m_vMaxs;
	protected int m_iNX, m_iNY, m_iNZ, m_iCursor;
	protected ref array<vector> m_aEvaluated = {};
	protected ref array<ref DCO_GarrisonDoor> m_aDoors = {};
	protected ref array<ref DCO_GarrisonSlot> m_aRaw = {};
	protected ref TraceParam m_Trace = new TraceParam();
	protected int m_iDefaultMask;
	protected NavmeshWorldComponent m_Navmesh;
	protected int m_iTileWait;

	void DCO_GarrisonSlotGenerator(IEntity building, ResourceName prefab)
	{
		m_Building = building;
		m_sPrefab = prefab;
		m_iDefaultMask = m_Trace.LayerMask;
		building.GetBounds(m_vMins, m_vMaxs);

		vector size = m_vMaxs - m_vMins;
		m_iNX = Math.Max(1, Math.Ceil(size[0] / CELL_XZ));
		m_iNY = Math.Max(1, Math.Ceil(size[1] / CELL_Y));
		m_iNZ = Math.Max(1, Math.Ceil(size[2] / CELL_XZ));

		CollectDoors();
	}

	int Step(int budget)
	{
		if (m_bDone)
			return 0;

		if (!m_Building)
		{
			m_bDone = true;
			return 0;
		}

		if (!m_Navmesh)
			m_Navmesh = GetGame().GetAIWorld().GetNavmeshWorldComponent("Soldiers");

		int start = m_Result.m_iTraces;
		int total = m_iNX * m_iNY * m_iNZ;

		int iterations = 0;
		while (m_iCursor < total && (m_Result.m_iTraces - start) < budget && iterations < 200)
		{
			iterations++;
			int r = EvaluateCell(m_iCursor);
			if (r < 0)
			{
				m_iTileWait++;
				if (m_iTileWait < 60)
					break;
			}

			m_iTileWait = 0;
			m_iCursor++;
		}

		if (m_iCursor >= total)
			Finish();

		return m_Result.m_iTraces - start;
	}

	protected void CollectDoors()
	{
		vector center = m_Building.CoordToParent((m_vMins + m_vMaxs) * 0.5);
		float radius = 0.5 * vector.Distance(m_vMins, m_vMaxs) + 1;
		DCO_Perf.Count("q:DCO_GarrisonSlots");
		GetGame().GetWorld().QueryEntitiesBySphere(center, radius, OnDoorFound);
	}

	protected bool OnDoorFound(IEntity e)
	{
		if (!e || !e.FindComponent(BaseDoorComponent))
			return true;

		vector mins, maxs;
		e.GetBounds(mins, maxs);
		vector world = e.CoordToParent((mins + maxs) * 0.5);
		world[1] = e.GetOrigin()[1];

		vector size = maxs - mins;
		vector axisX = m_Building.VectorToLocal(e.VectorToParent(vector.Right));
		vector axisZ = m_Building.VectorToLocal(e.VectorToParent(vector.Forward));
		axisX[1] = 0;
		axisZ[1] = 0;
		axisX.Normalize();
		axisZ.Normalize();

		DCO_GarrisonDoor door = new DCO_GarrisonDoor();
		door.m_vPos = m_Building.CoordToLocal(world);
		if (size[0] < size[2])
		{
			door.m_vNormal = axisX;
			door.m_vSide = axisZ;
			door.m_fHalfWidth = 0.5 * size[2];
		}
		else
		{
			door.m_vNormal = axisZ;
			door.m_vSide = axisX;
			door.m_fHalfWidth = 0.5 * size[0];
		}
		m_aDoors.Insert(door);
		return true;
	}

	protected int EvaluateCell(int idx)
	{
		int ix = idx % m_iNX;
		int rest = idx / m_iNX;
		int iz = rest % m_iNZ;
		int iy = rest / m_iNZ;

		vector local;
		local[0] = m_vMins[0] + (ix + 0.5) * CELL_XZ;
		local[1] = m_vMins[1] + iy * CELL_Y + 0.3;
		local[2] = m_vMins[2] + (iz + 0.5) * CELL_XZ;
		vector world = m_Building.CoordToParent(local);

		if (!m_Navmesh)
			return 0;

		if (m_Navmesh.IsTileRequested(world))
			return -1;

		if (!m_Navmesh.IsTileLoaded(world))
		{
			m_Navmesh.LoadTileIn(world);
			return -1;
		}

		vector snapped;
		if (!m_Navmesh.GetReachablePoint(world, 1.2, snapped))
			return 0;

		vector snappedLocal = m_Building.CoordToLocal(snapped);
		if (!InsideBounds(snappedLocal, 0.3) || Math.AbsFloat(snappedLocal[1] - local[1]) > SAME_FLOOR_DY)
			return 0;

		foreach (vector e : m_aEvaluated)
		{
			if (vector.DistanceSq(e, snappedLocal) < DEDUPE * DEDUPE)
				return 0;
		}
		m_aEvaluated.Insert(snappedLocal);

		if (!ClearDoorway(snappedLocal))
			return 0;
		snapped = m_Building.CoordToParent(snappedLocal);

		float headroom = Trace(snapped + 0.1 * vector.Up, snapped + HEADROOM_PROBE * vector.Up);
		bool openSky = headroom >= 0.999;
		if (openSky)
		{
			if (snapped[1] - GetGame().GetWorld().GetSurfaceY(snapped[0], snapped[2]) < 2.0)
				return 0;

			Trace(snapped + 0.5 * vector.Up, snapped - 1.0 * vector.Up);
			if (!IsBuilding(m_Trace.TraceEnt))
				return 0;

			if (m_Trace.TraceNorm[1] < ROOF_MIN_NORMAL_Y)
				return 0;
		}
		else
		{
			if (headroom * HEADROOM_PROBE < MIN_HEADROOM)
				return 0;

			if (!IsBuilding(m_Trace.TraceEnt))
				return 0;

			vector side = m_Building.VectorToParent(vector.Right);
			vector fwd = m_Building.VectorToParent(vector.Forward);
			if (!HasHeadroom(snapped + side) || !HasHeadroom(snapped - side) || !HasHeadroom(snapped + fwd) || !HasHeadroom(snapped - fwd))
				return 0;
		}

		ClassifyAndStore(snapped, snappedLocal, openSky, iy);
		return 1;
	}

	protected void ClassifyAndStore(vector pos, vector posLocal, bool openSky, int floor)
	{
		array<float> windowYaw = {};
		bool anyCrouch = false;
		int wallNear = 0;
		float longestInterior = 0;
		float longestYaw = 0;

		for (int i = 0; i < DIRS; i++)
		{
			float yawLocal = i * 360.0 / DIRS;
			vector dir = m_Building.VectorToParent(DCO_GarrisonSlot.YawToDir(yawLocal));
			dir[1] = 0;
			dir.Normalize();

			vector o = pos + H_CROUCH * vector.Up;
			float hit = Trace(o, o + dir * RAY_LEN, true) * RAY_LEN;
			float exitDist = BoundsExitDistance(o, dir);
			bool exits = hit >= exitDist + EXIT_MARGIN;

			if (hit < WALL_NEAR)
				wallNear++;

			if (exits)
			{
				vector oLow = pos + H_DOOR * vector.Up;
				float hitLow = Trace(oLow, oLow + dir * RAY_LEN, true) * RAY_LEN;
				float exitLow = BoundsExitDistance(oLow, dir);
				if (hitLow >= exitLow + EXIT_MARGIN)
				{
					if (exitLow < DOOR_CLEAR)
						return;
					continue;
				}

				windowYaw.Insert(yawLocal);
				anyCrouch = true;
				continue;
			}

			if (hit < WALL_NEAR)
			{
				vector oHigh = pos + H_STAND * vector.Up;
				float hitHigh = Trace(oHigh, oHigh + dir * RAY_LEN, true) * RAY_LEN;
				if (hitHigh >= BoundsExitDistance(oHigh, dir) + EXIT_MARGIN)
				{
					windowYaw.Insert(yawLocal);
					continue;
				}
			}

			if (hit > longestInterior)
			{
				longestInterior = hit;
				longestYaw = yawLocal;
			}
		}

		DCO_GarrisonSlot slot = new DCO_GarrisonSlot();
		slot.m_vLocalPos = posLocal;
		slot.m_iFloor = floor;
		float cover = wallNear / 8.0;

		if (!windowYaw.IsEmpty())
		{
			slot.m_eType = DCO_EGarrisonSlotType.WINDOW;
			if (openSky)
				slot.m_eType = DCO_EGarrisonSlotType.ROOF;

			slot.m_fLocalYaw = CircularMean(windowYaw);
			slot.m_fArc = Math.Clamp(windowYaw.Count() * 45.0, 45.0, 180.0);
			slot.m_eStance = ECharacterStance.STAND;
			if (anyCrouch)
				slot.m_eStance = ECharacterStance.CROUCH;
			slot.m_fScore = 0.5 + 0.05 * windowYaw.Count() + 0.3 * cover;
			m_aRaw.Insert(slot);
			return;
		}

		if (openSky)
			return;

		foreach (DCO_GarrisonDoor gd : m_aDoors)
		{
			vector door = gd.m_vPos;
			if (Math.AbsFloat(door[1] - posLocal[1]) > SAME_FLOOR_DY)
				continue;

			float d = vector.DistanceXZ(door, posLocal);
			if (d < DOOR_GUARD_MIN || d > DOOR_GUARD_MAX)
				continue;

			vector eye = pos + H_CROUCH * vector.Up;
			vector doorWorld = m_Building.CoordToParent(door) + H_CROUCH * vector.Up;
			if (Trace(eye, doorWorld, true) < 0.9)
				continue;

			vector toDoor = door - posLocal;
			slot.m_eType = DCO_EGarrisonSlotType.DOOR_GUARD;
			slot.m_fLocalYaw = toDoor.ToYaw();
			slot.m_fArc = 60;
			slot.m_eStance = ECharacterStance.CROUCH;
			slot.m_fScore = 0.4 + 0.3 * cover;
			m_aRaw.Insert(slot);
			return;
		}

		if (wallNear < 2)
			return;

		slot.m_eType = DCO_EGarrisonSlotType.INTERIOR;
		slot.m_fLocalYaw = longestYaw;
		slot.m_fArc = 90;
		slot.m_eStance = ECharacterStance.CROUCH;
		slot.m_fScore = cover;
		m_aRaw.Insert(slot);
	}

	protected void Finish()
	{
		m_bDone = true;

		for (int i = 1; i < m_aRaw.Count(); i++)
		{
			for (int j = i; j > 0 && m_aRaw[j].m_fScore > m_aRaw[j - 1].m_fScore; j--)
				m_aRaw.SwapItems(j, j - 1);
		}

		int interiorCount = 0;
		foreach (DCO_GarrisonSlot s : m_aRaw)
		{
			if (s.m_eType == DCO_EGarrisonSlotType.INTERIOR)
			{
				if (interiorCount >= MAX_INTERIOR)
					continue;
			}

			if (IsMergedInto(s))
				continue;

			if (s.m_eType == DCO_EGarrisonSlotType.INTERIOR)
				interiorCount++;

			m_Result.m_aSlots.Insert(s);
		}

		m_aRaw.Clear();
		m_aEvaluated.Clear();
	}

	protected bool IsMergedInto(DCO_GarrisonSlot s)
	{
		float mergeDist = 2.5;
		if (s.m_eType == DCO_EGarrisonSlotType.WINDOW)
			mergeDist = 1.8;
		else if (s.m_eType == DCO_EGarrisonSlotType.ROOF)
			mergeDist = 3.0;

		foreach (DCO_GarrisonSlot k : m_Result.m_aSlots)
		{
			if (k.m_eType != s.m_eType)
				continue;

			if (vector.DistanceSq(k.m_vLocalPos, s.m_vLocalPos) > mergeDist * mergeDist)
				continue;

			if (s.m_eType == DCO_EGarrisonSlotType.WINDOW && AngleDiff(k.m_fLocalYaw, s.m_fLocalYaw) > 60)
				continue;

			return true;
		}
		return false;
	}

	protected float Trace(vector start, vector end, bool sightOnly = false)
	{
		m_Trace.Flags = TraceFlags.ENTS | TraceFlags.WORLD;
		m_Trace.LayerMask = m_iDefaultMask;
		if (sightOnly)
			m_Trace.LayerMask = EPhysicsLayerDefs.ViewGeometry | EPhysicsLayerDefs.Terrain;
		m_Trace.Start = start;
		m_Trace.End = end;
		m_Trace.TraceEnt = null;
		m_Result.m_iTraces++;
		DCO_Perf.Count("t:DCO_GarrisonSlots");
		return GetGame().GetWorld().TraceMove(m_Trace, IgnoreDynamic);
	}

	protected bool IgnoreDynamic(notnull IEntity e)
	{
		return !ChimeraCharacter.Cast(e) && !Vehicle.Cast(e);
	}

	protected DCO_GarrisonDoor DoorwayAt(vector posLocal)
	{
		foreach (DCO_GarrisonDoor d : m_aDoors)
		{
			vector rel = posLocal - d.m_vPos;
			if (Math.AbsFloat(rel[1]) > SAME_FLOOR_DY)
				continue;

			if (Math.AbsFloat(vector.Dot(rel, d.m_vNormal)) < DOOR_DEPTH && Math.AbsFloat(vector.Dot(rel, d.m_vSide)) < d.m_fHalfWidth + DOOR_SIDE_PAD)
				return d;
		}
		return null;
	}

	protected bool ClearDoorway(inout vector posLocal)
	{
		DCO_GarrisonDoor d = DoorwayAt(posLocal);
		if (!d)
			return true;

		vector rel = posLocal - d.m_vPos;
		float along = vector.Dot(rel, d.m_vNormal);
		float lat = vector.Dot(rel, d.m_vSide);
		float sideOut = d.m_fHalfWidth + DOOR_SIDE_PAD + 0.2;
		float deepOut = DOOR_DEPTH + 0.2;
		float alongSign = 1;
		if (along < 0)
			alongSign = -1;

		array<vector> shifts = {};
		shifts.Insert(d.m_vSide * (sideOut - lat));
		shifts.Insert(d.m_vSide * (-sideOut - lat));
		shifts.Insert(d.m_vNormal * (alongSign * deepOut - along));

		for (int i = 1; i < shifts.Count(); i++)
		{
			for (int j = i; j > 0 && shifts[j].LengthSq() < shifts[j - 1].LengthSq(); j--)
				shifts.SwapItems(j, j - 1);
		}

		foreach (vector shift : shifts)
		{
			vector reached;
			if (!m_Navmesh.GetReachablePoint(m_Building.CoordToParent(posLocal + shift), 0.3, reached))
				continue;

			vector reachedLocal = m_Building.CoordToLocal(reached);
			if (!InsideBounds(reachedLocal, 0.3) || Math.AbsFloat(reachedLocal[1] - posLocal[1]) > SAME_FLOOR_DY)
				continue;

			if (DoorwayAt(reachedLocal))
				continue;

			posLocal = reachedLocal;
			return true;
		}
		return false;
	}

	protected bool HasHeadroom(vector pos)
	{
		return Trace(pos + 0.1 * vector.Up, pos + MIN_HEADROOM * vector.Up) >= 0.999;
	}

	protected bool IsBuilding(IEntity e)
	{
		return e && e.GetRootParent() == m_Building.GetRootParent();
	}

	protected bool InsideBounds(vector local, float margin)
	{
		for (int a = 0; a < 3; a++)
		{
			if (local[a] < m_vMins[a] - margin || local[a] > m_vMaxs[a] + margin)
				return false;
		}
		return true;
	}

	protected float BoundsExitDistance(vector worldOrigin, vector worldDir)
	{
		vector o = m_Building.CoordToLocal(worldOrigin);
		vector d = m_Building.VectorToLocal(worldDir);
		float tExit = float.MAX;
		for (int a = 0; a < 3; a++)
		{
			if (Math.AbsFloat(d[a]) < 0.0001)
				continue;

			float t1 = (m_vMins[a] - o[a]) / d[a];
			float t2 = (m_vMaxs[a] - o[a]) / d[a];
			tExit = Math.Min(tExit, Math.Max(t1, t2));
		}
		return Math.Max(tExit, 0);
	}

	static float CircularMean(array<float> yawsDeg)
	{
		float sx, sz;
		foreach (float y : yawsDeg)
		{
			sx += Math.Sin(y * Math.DEG2RAD);
			sz += Math.Cos(y * Math.DEG2RAD);
		}
		return Vector(sx, 0, sz).ToYaw();
	}

	static float AngleDiff(float a, float b)
	{
		float d = Math.AbsFloat(Math.Repeat(a - b + 180, 360) - 180);
		return d;
	}
}
