class DCO_INavSample
{
	vector m_vL;
	int m_iX;
	int m_iZ;
	int m_iFloor = -1;
	int m_iRoom = -1;
	int m_iDist = -1;
	int m_iDirAll;
	int m_iDirRoom;
}

class DCO_INavDoor
{
	vector m_vL;
	vector m_vN;
	vector m_vS;
	float m_fHW;
	bool m_bReal;
	int m_iRoomA = -2;
	int m_iRoomB = -2;
	IEntity m_Ent;
}

class DCO_INavRoom
{
	int m_iId;
	int m_iKind;
	int m_iFloor;
	int m_iCenter = -1;
	int m_iAccess;
	int m_iWindows;
	vector m_vMin;
	vector m_vMax;
	ref array<int> m_aSamples = {};
}

class DCO_INavPoint
{
	vector m_vL;
	int m_iType;
	int m_iRoom = -1;
	int m_iOrder;
	int m_iLevel;
	int m_iDoor = -1;
}

class DCO_INavGraph
{
	IEntity m_Building;
	string m_sPrefab;
	vector m_vMins;
	vector m_vMaxs;
	float m_fStep = 1.0;
	int m_iTraces;
	float m_fMs;
	ref array<float> m_aFloors = {};
	ref array<ref DCO_INavSample> m_aSamples = {};
	ref map<int, ref array<int>> m_mCell = new map<int, ref array<int>>();
	ref array<int> m_aLinkA = {};
	ref array<int> m_aLinkB = {};
	ref array<int> m_aLinkKind = {};
	ref array<int> m_aLinkDoor = {};
	ref array<ref DCO_INavDoor> m_aDoors = {};
	ref array<ref DCO_INavRoom> m_aRooms = {};
	ref array<ref DCO_INavPoint> m_aPoints = {};
	ref array<vector> m_aRejP = {};
	ref array<int> m_aRejWhy = {};
	ref array<int> m_aRejN = {0, 0, 0, 0, 0, 0, 0, 0};

	void Rej(int why, vector local, bool keep)
	{
		m_aRejN[why] = m_aRejN[why] + 1;
		if (!keep || m_aRejP.Count() >= 6000)
			return;
		m_aRejP.Insert(local);
		m_aRejWhy.Insert(why);
	}

	vector W(vector local)
	{
		return m_Building.CoordToParent(local);
	}

	int CountPoints(int type)
	{
		int n;
		foreach (DCO_INavPoint p : m_aPoints)
		{
			if (p.m_iType == type)
				n++;
		}
		return n;
	}
}

class DCO_IndoorNav
{
	protected static ref map<IEntity, ref DCO_INavGraph> s_mGraphs;
	protected static ref TraceParam s_Trace;

	static DCO_INavGraph Get(IEntity building, bool build)
	{
		if (!building)
			return null;
		if (!s_mGraphs)
			s_mGraphs = new map<IEntity, ref DCO_INavGraph>();
		DCO_INavGraph g;
		if (s_mGraphs.Find(building, g))
			return g;
		if (!build)
			return null;
		g = Build(building);
		if (g)
			s_mGraphs.Set(building, g);
		return g;
	}

	static void Forget(IEntity building)
	{
		if (s_mGraphs && building)
			s_mGraphs.Remove(building);
	}

	static IEntity NearestBuilding(vector pos, float radius)
	{
		s_aFound = {};
		GetGame().GetWorld().QueryEntitiesBySphere(pos, radius, OnBuildingFound);
		IEntity best;
		float bestD = float.MAX;
		foreach (IEntity e : s_aFound)
		{
			vector mins, maxs;
			e.GetBounds(mins, maxs);
			vector local = e.CoordToLocal(pos);
			float dx = Math.Max(0, Math.Max(mins[0] - local[0], local[0] - maxs[0]));
			float dz = Math.Max(0, Math.Max(mins[2] - local[2], local[2] - maxs[2]));
			float d = dx * dx + dz * dz;
			if (d < bestD)
			{
				bestD = d;
				best = e;
			}
		}
		s_aFound = null;
		return best;
	}

	protected static ref array<IEntity> s_aFound;

	protected static bool OnBuildingFound(IEntity e)
	{
		if (e && e.FindComponent(DCO_BuildingPositionComponent))
			s_aFound.Insert(e);
		return true;
	}

	protected static bool TraceFilter(notnull IEntity e)
	{
		return !ChimeraCharacter.Cast(e) && !Vehicle.Cast(e) && !e.FindComponent(BaseDoorComponent);
	}

	protected static float Trace(DCO_INavGraph g, vector a, vector b)
	{
		if (!s_Trace)
			s_Trace = new TraceParam();
		s_Trace.Flags = TraceFlags.ENTS | TraceFlags.WORLD;
		s_Trace.LayerMask = EPhysicsLayerDefs.Projectile;
		s_Trace.Start = a;
		s_Trace.End = b;
		s_Trace.TraceEnt = null;
		g.m_iTraces++;
		DCO_Perf.Count("t:DCO_IndoorNav");
		return GetGame().GetWorld().TraceMove(s_Trace, TraceFilter);
	}

	protected static bool HitIsBuilding(IEntity building)
	{
		IEntity e = s_Trace.TraceEnt;
		while (e)
		{
			if (e == building)
				return true;
			e = e.GetParent();
		}
		return false;
	}

	protected static bool Clear(DCO_INavGraph g, vector localA, vector localB, float height)
	{
		vector a = g.W(localA + height * vector.Up);
		vector b = g.W(localB + height * vector.Up);
		return Trace(g, a, b) >= 0.999;
	}

	protected static int CellKey(int ix, int iz)
	{
		return ix * 4096 + iz;
	}

	static DCO_INavGraph Build(IEntity building)
	{
		float t0 = System.GetTickCount();
		NavmeshWorldComponent nav = GetGame().GetAIWorld().GetNavmeshWorldComponent("Soldiers");
		if (!nav)
		{
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.WARN, "-", "inav_build result=fail reason=no_navmesh");
			return null;
		}
		if (!nav.IsTileLoaded(building.GetOrigin()))
		{
			nav.LoadTileIn(building.GetOrigin());
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", "inav_build result=wait reason=tile_loading");
			return null;
		}

		DCO_INavGraph g = new DCO_INavGraph();
		g.m_Building = building;
		g.m_sPrefab = SCR_ResourceNameUtils.GetPrefabName(building);
		building.GetBounds(g.m_vMins, g.m_vMaxs);
		vector size = g.m_vMaxs - g.m_vMins;
		if (Math.Min(size[0], size[2]) < 8.0)
			g.m_fStep = 0.75;

		CollectDoors(g);
		Sample(g, nav);
		Floors(g);
		Links(g);
		Rooms(g);
		Centers(g);
		DoorPoints(g, nav);
		ClearPoints(g);
		Threats(g);

		g.m_fMs = System.GetTickCount() - t0;
		int extreme, high, medium;
		foreach (DCO_INavPoint p : g.m_aPoints)
		{
			if (p.m_iType != 4)
				continue;
			if (p.m_iLevel == 3)
				extreme++;
			else if (p.m_iLevel == 2)
				high++;
			else
				medium++;
		}
		int roomsN, halls, stairs;
		foreach (DCO_INavRoom r : g.m_aRooms)
		{
			if (r.m_iKind == 0)
				roomsN++;
			else if (r.m_iKind == 1)
				halls++;
			else if (r.m_iKind == 2)
				stairs++;
		}
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("inav_build result=ok prefab=%1 pos=%2 samples=%3 links=%4 floors=%5 rooms=%6 halls=%7 stairs=%8 doors=%9",
			g.m_sPrefab, building.GetOrigin().ToString(false), g.m_aSamples.Count(), g.m_aLinkA.Count(), g.m_aFloors.Count(), roomsN, halls, stairs, g.m_aDoors.Count()));
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("inav_points prefab=%1 door_nodes=%2 stack=%3 clear=%4 threat_extreme=%5 threat_high=%6 threat_medium=%7 traces=%8 ms=%9",
			g.m_sPrefab, g.CountPoints(1), g.CountPoints(2), g.CountPoints(3), extreme, high, medium, g.m_iTraces, Math.Round(g.m_fMs)));
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("inav_reject prefab=%1 no_nav=%2 snap_off=%3 edge=%4 dup=%5 no_floor=%6 floor_not_bld=%7 no_ceiling=%8 low_ceiling=%9",
			g.m_sPrefab, g.m_aRejN[0], g.m_aRejN[1], g.m_aRejN[2], g.m_aRejN[3], g.m_aRejN[4], g.m_aRejN[5], g.m_aRejN[6], g.m_aRejN[7]));
		return g;
	}

	protected static void CollectDoors(DCO_INavGraph g)
	{
		s_aFound = {};
		vector center = g.W((g.m_vMins + g.m_vMaxs) * 0.5);
		float radius = 0.5 * vector.Distance(g.m_vMins, g.m_vMaxs) + 1;
		GetGame().GetWorld().QueryEntitiesBySphere(center, radius, OnDoorFound);
		foreach (IEntity e : s_aFound)
		{
			vector mins, maxs;
			e.GetBounds(mins, maxs);
			vector world = e.CoordToParent((mins + maxs) * 0.5);
			world[1] = e.GetOrigin()[1];
			vector local = g.m_Building.CoordToLocal(world);
			if (local[0] < g.m_vMins[0] - 1 || local[0] > g.m_vMaxs[0] + 1 || local[2] < g.m_vMins[2] - 1 || local[2] > g.m_vMaxs[2] + 1)
				continue;
			vector dsize = maxs - mins;
			vector axisX = g.m_Building.VectorToLocal(e.VectorToParent(vector.Right));
			vector axisZ = g.m_Building.VectorToLocal(e.VectorToParent(vector.Forward));
			axisX[1] = 0;
			axisZ[1] = 0;
			axisX.Normalize();
			axisZ.Normalize();
			DCO_INavDoor d = new DCO_INavDoor();
			d.m_vL = local;
			d.m_bReal = true;
			d.m_Ent = e;
			if (dsize[0] < dsize[2])
			{
				d.m_vN = axisX;
				d.m_vS = axisZ;
				d.m_fHW = 0.5 * dsize[2];
			}
			else
			{
				d.m_vN = axisZ;
				d.m_vS = axisX;
				d.m_fHW = 0.5 * dsize[0];
			}
			g.m_aDoors.Insert(d);
		}
		s_aFound = null;
	}

	protected static bool OnDoorFound(IEntity e)
	{
		if (e && e.FindComponent(BaseDoorComponent))
			s_aFound.Insert(e);
		return true;
	}

	protected static void Sample(DCO_INavGraph g, NavmeshWorldComponent nav)
	{
		float step = g.m_fStep;
		int nx = Math.Ceil((g.m_vMaxs[0] - g.m_vMins[0]) / step);
		int nz = Math.Ceil((g.m_vMaxs[2] - g.m_vMins[2]) / step);
		int ny = Math.Floor((g.m_vMaxs[1] - g.m_vMins[1] - 1.0) / 0.8) + 1;
		ny = Math.Max(ny, 1);
		for (int ix = 0; ix < nx; ix++)
		{
			for (int iz = 0; iz < nz; iz++)
			{
				for (int iy = 0; iy < ny; iy++)
				{
					vector p;
					p[0] = g.m_vMins[0] + (ix + 0.5) * step;
					p[1] = g.m_vMins[1] + 0.3 + 0.8 * iy;
					p[2] = g.m_vMins[2] + (iz + 0.5) * step;
					vector snapped;
					if (!nav.GetReachablePoint(g.W(p), 1.0, snapped))
					{
						g.Rej(0, p, false);
						continue;
					}
					vector sl = g.m_Building.CoordToLocal(snapped);
					if (Math.AbsFloat(sl[0] - p[0]) > 0.75 * step || Math.AbsFloat(sl[2] - p[2]) > 0.75 * step || Math.AbsFloat(sl[1] - p[1]) > 0.6)
					{
						g.Rej(1, sl, true);
						continue;
					}
					if (sl[0] < g.m_vMins[0] + 0.2 || sl[0] > g.m_vMaxs[0] - 0.2 || sl[2] < g.m_vMins[2] + 0.2 || sl[2] > g.m_vMaxs[2] - 0.2)
					{
						g.Rej(2, sl, true);
						continue;
					}
					int key = CellKey(ix, iz);
					array<int> cell;
					if (g.m_mCell.Find(key, cell))
					{
						bool dup = false;
						foreach (int ci : cell)
						{
							if (Math.AbsFloat(g.m_aSamples[ci].m_vL[1] - sl[1]) < 0.5)
							{
								dup = true;
								break;
							}
						}
						if (dup)
						{
							g.Rej(3, sl, false);
							continue;
						}
					}
					float down = Trace(g, g.W(sl + 0.3 * vector.Up), g.W(sl - 0.5 * vector.Up));
					if (down >= 0.999)
					{
						g.Rej(4, sl, true);
						continue;
					}
					if (!HitIsBuilding(g.m_Building))
					{
						g.Rej(5, sl, true);
						continue;
					}
					float up = Trace(g, g.W(sl + 0.1 * vector.Up), g.W(sl + 20.1 * vector.Up));
					if (up >= 0.999)
					{
						g.Rej(6, sl, true);
						continue;
					}
					if (up * 20.0 < 1.6)
					{
						g.Rej(7, sl, true);
						continue;
					}
					DCO_INavSample s = new DCO_INavSample();
					s.m_vL = sl;
					s.m_iX = ix;
					s.m_iZ = iz;
					if (!cell)
					{
						cell = {};
						g.m_mCell.Set(key, cell);
					}
					cell.Insert(g.m_aSamples.Count());
					g.m_aSamples.Insert(s);
				}
			}
		}
	}

	protected static void Floors(DCO_INavGraph g)
	{
		array<float> ys = {};
		foreach (DCO_INavSample s : g.m_aSamples)
		{
			ys.Insert(s.m_vL[1]);
		}
		ys.Sort();
		int minCount = Math.Max(4, 0.05 * ys.Count());
		int start = 0;
		for (int i = 1; i <= ys.Count(); i++)
		{
			if (i < ys.Count() && ys[i] - ys[i - 1] <= 0.25)
				continue;
			int count = i - start;
			if (count >= minCount)
			{
				float sum = 0;
				for (int k = start; k < i; k++)
				{
					sum += ys[k];
				}
				g.m_aFloors.Insert(sum / count);
			}
			start = i;
		}
		foreach (DCO_INavSample s2 : g.m_aSamples)
		{
			foreach (int fi, float fy : g.m_aFloors)
			{
				if (Math.AbsFloat(s2.m_vL[1] - fy) <= 0.3)
				{
					s2.m_iFloor = fi;
					break;
				}
			}
		}
	}

	protected static int DirBit(int dx, int dz)
	{
		if (dx > 0)
			return 1;
		if (dx < 0)
			return 2;
		if (dz > 0)
			return 4;
		return 8;
	}

	protected static void Links(DCO_INavGraph g)
	{
		foreach (int i, DCO_INavSample a : g.m_aSamples)
		{
			for (int n = 0; n < 2; n++)
			{
				int dx = 1 - n;
				int dz = n;
				array<int> cell;
				if (!g.m_mCell.Find(CellKey(a.m_iX + dx, a.m_iZ + dz), cell))
					continue;
				foreach (int j : cell)
				{
					DCO_INavSample b = g.m_aSamples[j];
					float dy = Math.AbsFloat(b.m_vL[1] - a.m_vL[1]);
					if (dy > 0.75)
						continue;
					if (a.m_iFloor >= 0 && b.m_iFloor >= 0 && a.m_iFloor != b.m_iFloor)
						continue;
					if (!Clear(g, a.m_vL, b.m_vL, 0.5) || !Clear(g, a.m_vL, b.m_vL, 1.3))
						continue;
					int kind = 0;
					int door = -1;
					if (a.m_iFloor < 0 || b.m_iFloor < 0)
						kind = 2;
					else
					{
						door = DoorCrossed(g, a.m_vL, b.m_vL);
						if (door >= 0)
							kind = 1;
						else if (IsOpening(g, a.m_vL, b.m_vL))
							kind = 3;
					}
					g.m_aLinkA.Insert(i);
					g.m_aLinkB.Insert(j);
					g.m_aLinkKind.Insert(kind);
					g.m_aLinkDoor.Insert(door);
					a.m_iDirAll |= DirBit(dx, dz);
					b.m_iDirAll |= DirBit(-dx, -dz);
				}
			}
		}
	}

	protected static int DoorCrossed(DCO_INavGraph g, vector a, vector b)
	{
		vector mid = (a + b) * 0.5;
		foreach (int di, DCO_INavDoor d : g.m_aDoors)
		{
			float sa = vector.Dot(a - d.m_vL, d.m_vN);
			float sb = vector.Dot(b - d.m_vL, d.m_vN);
			if (sa * sb > 0)
				continue;
			if (Math.AbsFloat(vector.Dot(mid - d.m_vL, d.m_vS)) > d.m_fHW + 0.3)
				continue;
			if (Math.AbsFloat(mid[1] - d.m_vL[1]) > 1.5)
				continue;
			return di;
		}
		return -1;
	}

	protected static float WidthAt(DCO_INavGraph g, vector p, vector side)
	{
		vector c = g.W(p + 1.0 * vector.Up);
		vector sw = g.m_Building.VectorToParent(side);
		float l = Trace(g, c, c + sw * 1.5);
		float r = Trace(g, c, c - sw * 1.5);
		return 1.5 * l + 1.5 * r;
	}

	protected static bool IsOpening(DCO_INavGraph g, vector a, vector b)
	{
		vector dir = b - a;
		dir[1] = 0;
		dir.Normalize();
		vector side = Vector(-dir[2], 0, dir[0]);
		vector mid = (a + b) * 0.5;
		float w = WidthAt(g, mid, side);
		if (w >= 1.5)
			return false;
		float before = WidthAt(g, mid - dir * 1.0, side);
		float after = WidthAt(g, mid + dir * 1.0, side);
		return before >= w + 0.8 && after >= w + 0.8;
	}

	protected static void Rooms(DCO_INavGraph g)
	{
		int count = g.m_aSamples.Count();
		array<ref array<int>> adj = {};
		for (int i = 0; i < count; i++)
		{
			adj.Insert(new array<int>());
		}
		foreach (int li, int a : g.m_aLinkA)
		{
			int b = g.m_aLinkB[li];
			int kind = g.m_aLinkKind[li];
			if (kind != 0)
			{
				if (kind == 2 && g.m_aSamples[a].m_iFloor < 0 && g.m_aSamples[b].m_iFloor < 0)
				{
					adj[a].Insert(b);
					adj[b].Insert(a);
				}
				continue;
			}
			adj[a].Insert(b);
			adj[b].Insert(a);
		}
		for (int s = 0; s < count; s++)
		{
			if (g.m_aSamples[s].m_iRoom >= 0)
				continue;
			DCO_INavRoom room = new DCO_INavRoom();
			room.m_iId = g.m_aRooms.Count();
			array<int> stack = {s};
			g.m_aSamples[s].m_iRoom = room.m_iId;
			while (!stack.IsEmpty())
			{
				int u = stack[stack.Count() - 1];
				stack.Remove(stack.Count() - 1);
				room.m_aSamples.Insert(u);
				foreach (int v : adj[u])
				{
					if (g.m_aSamples[v].m_iRoom >= 0)
						continue;
					g.m_aSamples[v].m_iRoom = room.m_iId;
					stack.Insert(v);
				}
			}
			g.m_aRooms.Insert(room);
		}
		foreach (int li2, int a2 : g.m_aLinkA)
		{
			int b2 = g.m_aLinkB[li2];
			DCO_INavSample sa = g.m_aSamples[a2];
			DCO_INavSample sb = g.m_aSamples[b2];
			if (sa.m_iRoom != sb.m_iRoom)
				continue;
			vector d = sb.m_vL - sa.m_vL;
			int dx = 0;
			int dz = 0;
			if (Math.AbsFloat(d[0]) > Math.AbsFloat(d[2]))
			{
				dx = 1;
				if (d[0] < 0)
					dx = -1;
			}
			else
			{
				dz = 1;
				if (d[2] < 0)
					dz = -1;
			}
			sa.m_iDirRoom |= DirBit(dx, dz);
			sb.m_iDirRoom |= DirBit(-dx, -dz);
		}
		foreach (DCO_INavRoom r : g.m_aRooms)
		{
			int stairN = 0;
			int floorVotes = -1;
			vector lo = Vector(float.MAX, float.MAX, float.MAX);
			vector hi = Vector(-float.MAX, -float.MAX, -float.MAX);
			foreach (int si : r.m_aSamples)
			{
				DCO_INavSample smp = g.m_aSamples[si];
				if (smp.m_iFloor < 0)
					stairN++;
				else if (floorVotes < 0)
					floorVotes = smp.m_iFloor;
				for (int c = 0; c < 3; c++)
				{
					lo[c] = Math.Min(lo[c], smp.m_vL[c]);
					hi[c] = Math.Max(hi[c], smp.m_vL[c]);
				}
			}
			r.m_vMin = lo;
			r.m_vMax = hi;
			r.m_iFloor = floorVotes;
			float w = r.m_vMax[0] - r.m_vMin[0] + g.m_fStep;
			float h = r.m_vMax[2] - r.m_vMin[2] + g.m_fStep;
			if (2 * stairN >= r.m_aSamples.Count())
				r.m_iKind = 2;
			else if (r.m_aSamples.Count() < 3)
				r.m_iKind = 3;
			else if (r.m_aSamples.Count() >= 6 && Math.Max(w / h, h / w) > 3.0)
				r.m_iKind = 1;
			else
				r.m_iKind = 0;
		}
	}

	protected static void Centers(DCO_INavGraph g)
	{
		foreach (DCO_INavRoom r : g.m_aRooms)
		{
			array<int> queue = {};
			foreach (int si : r.m_aSamples)
			{
				DCO_INavSample s = g.m_aSamples[si];
				if (s.m_iDirRoom != 15)
				{
					s.m_iDist = 0;
					queue.Insert(si);
				}
			}
			int head = 0;
			while (head < queue.Count())
			{
				DCO_INavSample u = g.m_aSamples[queue[head]];
				head++;
				for (int n = 0; n < 4; n++)
				{
					int dx = 0;
					int dz = 0;
					if (n == 0)
						dx = 1;
					else if (n == 1)
						dx = -1;
					else if (n == 2)
						dz = 1;
					else
						dz = -1;
					array<int> cell;
					if (!g.m_mCell.Find(CellKey(u.m_iX + dx, u.m_iZ + dz), cell))
						continue;
					foreach (int vi : cell)
					{
						DCO_INavSample v = g.m_aSamples[vi];
						if (v.m_iRoom != r.m_iId || v.m_iDist >= 0)
							continue;
						if (Math.AbsFloat(v.m_vL[1] - u.m_vL[1]) > 0.75)
							continue;
						v.m_iDist = u.m_iDist + 1;
						queue.Insert(vi);
					}
				}
			}
			vector mean = (r.m_vMin + r.m_vMax) * 0.5;
			int best = -1;
			float bestScore = -float.MAX;
			foreach (int si2 : r.m_aSamples)
			{
				DCO_INavSample s2 = g.m_aSamples[si2];
				float score = 10.0 * s2.m_iDist - vector.DistanceXZ(s2.m_vL, mean);
				if (score > bestScore)
				{
					bestScore = score;
					best = si2;
				}
			}
			r.m_iCenter = best;
			if (best >= 0)
			{
				DCO_INavPoint p = new DCO_INavPoint();
				p.m_vL = g.m_aSamples[best].m_vL;
				p.m_iType = 0;
				p.m_iRoom = r.m_iId;
				g.m_aPoints.Insert(p);
			}
		}
	}

	protected static bool Snap(DCO_INavGraph g, NavmeshWorldComponent nav, vector local, float maxXZ, out vector outLocal)
	{
		vector snapped;
		if (!nav.GetReachablePoint(g.W(local), 0.5, snapped))
			return false;
		outLocal = g.m_Building.CoordToLocal(snapped);
		return vector.DistanceXZ(outLocal, local) <= maxXZ && Math.AbsFloat(outLocal[1] - local[1]) <= 1.0;
	}

	protected static int RoomAt(DCO_INavGraph g, vector local)
	{
		int best = -1;
		float bestD = 1.6 * 1.6;
		foreach (DCO_INavSample s : g.m_aSamples)
		{
			if (Math.AbsFloat(s.m_vL[1] - local[1]) > 1.0)
				continue;
			float d = vector.DistanceSqXZ(s.m_vL, local);
			if (d < bestD)
			{
				bestD = d;
				best = s.m_iRoom;
			}
		}
		return best;
	}

	protected static int RoomAtSide(DCO_INavGraph g, vector local, DCO_INavDoor door, int side)
	{
		int best = -1;
		float bestD = 2.0 * 2.0;
		foreach (DCO_INavSample s : g.m_aSamples)
		{
			if (Math.AbsFloat(s.m_vL[1] - local[1]) > 1.0)
				continue;
			if (side * vector.Dot(s.m_vL - door.m_vL, door.m_vN) < 0.2)
				continue;
			float d = vector.DistanceSqXZ(s.m_vL, local);
			if (d < bestD)
			{
				bestD = d;
				best = s.m_iRoom;
			}
		}
		return best;
	}

	protected static void DoorPoints(DCO_INavGraph g, NavmeshWorldComponent nav)
	{
		array<int> openingSeen = {};
		foreach (int li, int kind : g.m_aLinkKind)
		{
			if (kind != 3)
				continue;
			vector a = g.m_aSamples[g.m_aLinkA[li]].m_vL;
			vector b = g.m_aSamples[g.m_aLinkB[li]].m_vL;
			vector mid = (a + b) * 0.5;
			bool near = false;
			foreach (DCO_INavDoor od : g.m_aDoors)
			{
				if (vector.Distance(od.m_vL, mid) < 1.2)
				{
					near = true;
					break;
				}
			}
			if (near)
				continue;
			vector dir = b - a;
			dir[1] = 0;
			dir.Normalize();
			DCO_INavDoor d = new DCO_INavDoor();
			d.m_vL = mid;
			d.m_vN = dir;
			d.m_vS = Vector(-dir[2], 0, dir[0]);
			d.m_fHW = 0.5 * WidthAt(g, mid, d.m_vS);
			d.m_bReal = false;
			g.m_aDoors.Insert(d);
		}
		foreach (int di, DCO_INavDoor door : g.m_aDoors)
		{
			for (int side = -1; side <= 1; side += 2)
			{
				vector node;
				vector want = door.m_vL + door.m_vN * (0.8 * side);
				int room = -1;
				if (Snap(g, nav, want, 0.6, node))
				{
					room = RoomAtSide(g, node, door, side);
					DCO_INavPoint p = new DCO_INavPoint();
					p.m_vL = node;
					p.m_iType = 1;
					p.m_iDoor = di;
					p.m_iRoom = room;
					g.m_aPoints.Insert(p);
				}
				if (side < 0)
					door.m_iRoomA = room;
				else
					door.m_iRoomB = room;
				for (int lat = -1; lat <= 1; lat += 2)
				{
					for (int k = 0; k < 2; k++)
					{
						vector sp = door.m_vL + door.m_vN * (0.5 * side) + door.m_vS * (lat * (door.m_fHW + 0.6 + 0.9 * k));
						vector spn;
						if (!Snap(g, nav, sp, 0.5, spn))
							continue;
						vector look = door.m_vL + door.m_vN * (0.3 * side);
						if (!Clear(g, spn, look, 1.2))
							continue;
						DCO_INavPoint st = new DCO_INavPoint();
						st.m_vL = spn;
						st.m_iType = 2;
						st.m_iDoor = di;
						st.m_iOrder = k;
						st.m_iRoom = RoomAt(g, spn);
						g.m_aPoints.Insert(st);
					}
				}
			}
			if (door.m_iRoomA >= 0)
				g.m_aRooms[door.m_iRoomA].m_iAccess++;
			if (door.m_iRoomB >= 0 && door.m_iRoomB != door.m_iRoomA)
				g.m_aRooms[door.m_iRoomB].m_iAccess++;
		}
	}

	protected static bool Visible(DCO_INavGraph g, vector a, vector b)
	{
		return Clear(g, a, b, 1.2);
	}

	protected static void ClearPoints(DCO_INavGraph g)
	{
		foreach (DCO_INavRoom r : g.m_aRooms)
		{
			if (r.m_iKind == 2 || r.m_iKind == 3)
				continue;
			array<vector> doorNodes = {};
			foreach (DCO_INavPoint dp : g.m_aPoints)
			{
				if (dp.m_iType == 1 && dp.m_iRoom == r.m_iId)
					doorNodes.Insert(dp.m_vL);
			}
			array<vector> picks = {};
			array<int> hidden = {};
			array<float> doorDist = {};
			foreach (int si : r.m_aSamples)
			{
				DCO_INavSample s = g.m_aSamples[si];
				int m = s.m_iDirAll;
				bool corner = ((m & 3) != 3) && ((m & 12) != 12);
				if (!corner)
					continue;
				bool dup = false;
				foreach (vector q : picks)
				{
					if (vector.Distance(q, s.m_vL) < 1.5)
					{
						dup = true;
						break;
					}
				}
				if (dup)
					continue;
				int seen = 0;
				float dd = float.MAX;
				foreach (vector dn : doorNodes)
				{
					dd = Math.Min(dd, vector.Distance(dn, s.m_vL));
					if (Visible(g, dn, s.m_vL))
						seen = 1;
				}
				picks.Insert(s.m_vL);
				hidden.Insert(seen);
				doorDist.Insert(dd);
			}
			array<vector> clearSet = {};
			for (int pass = 0; pass < 2; pass++)
			{
				while (true)
				{
					int best = -1;
					float bestD = float.MAX;
					foreach (int pi, vector pv : picks)
					{
						if (hidden[pi] != pass || doorDist[pi] < 0)
							continue;
						if (doorDist[pi] < bestD)
						{
							bestD = doorDist[pi];
							best = pi;
						}
					}
					if (best < 0 || clearSet.Count() >= 6)
						break;
					clearSet.Insert(picks[best]);
					doorDist[best] = -1;
				}
			}
			int pockets = 0;
			foreach (int si2 : r.m_aSamples)
			{
				if (pockets >= 2 || doorNodes.IsEmpty())
					break;
				vector sv = g.m_aSamples[si2].m_vL;
				bool close = false;
				foreach (vector cv : clearSet)
				{
					if (vector.Distance(cv, sv) < 1.5)
					{
						close = true;
						break;
					}
				}
				if (close)
					continue;
				bool seenAny = false;
				foreach (vector dn2 : doorNodes)
				{
					if (Visible(g, dn2, sv))
					{
						seenAny = true;
						break;
					}
				}
				if (!seenAny)
				{
					foreach (vector cv2 : clearSet)
					{
						if (Visible(g, cv2, sv))
						{
							seenAny = true;
							break;
						}
					}
				}
				if (seenAny)
					continue;
				clearSet.Insert(sv);
				pockets++;
			}
			foreach (int order, vector cp : clearSet)
			{
				DCO_INavPoint p = new DCO_INavPoint();
				p.m_vL = cp;
				p.m_iType = 3;
				p.m_iRoom = r.m_iId;
				p.m_iOrder = order + 1;
				g.m_aPoints.Insert(p);
			}
		}
	}

	protected static void AddThreat(DCO_INavGraph g, vector local, int level, int room)
	{
		foreach (DCO_INavPoint p : g.m_aPoints)
		{
			if (p.m_iType == 4 && vector.Distance(p.m_vL, local) < 1.5)
			{
				p.m_iLevel = Math.Max(p.m_iLevel, level);
				return;
			}
		}
		DCO_INavPoint t = new DCO_INavPoint();
		t.m_vL = local;
		t.m_iType = 4;
		t.m_iLevel = level;
		t.m_iRoom = room;
		g.m_aPoints.Insert(t);
	}

	protected static void Threats(DCO_INavGraph g)
	{
		foreach (DCO_INavDoor d : g.m_aDoors)
		{
			AddThreat(g, d.m_vL + 1.2 * vector.Up, 3, d.m_iRoomA);
		}
		foreach (int li, int kind : g.m_aLinkKind)
		{
			if (kind != 2)
				continue;
			DCO_INavSample a = g.m_aSamples[g.m_aLinkA[li]];
			DCO_INavSample b = g.m_aSamples[g.m_aLinkB[li]];
			if (a.m_iFloor >= 0 && b.m_iFloor < 0)
				AddThreat(g, a.m_vL + 1.2 * vector.Up, 2, a.m_iRoom);
			else if (b.m_iFloor >= 0 && a.m_iFloor < 0)
				AddThreat(g, b.m_vL + 1.2 * vector.Up, 2, b.m_iRoom);
		}
		foreach (DCO_INavRoom r : g.m_aRooms)
		{
			if (r.m_iKind == 1)
			{
				int ea = -1;
				int eb = -1;
				float farD = -1;
				foreach (int i : r.m_aSamples)
				{
					foreach (int j : r.m_aSamples)
					{
						float d2 = vector.DistanceSqXZ(g.m_aSamples[i].m_vL, g.m_aSamples[j].m_vL);
						if (d2 > farD)
						{
							farD = d2;
							ea = i;
							eb = j;
						}
					}
				}
				if (ea >= 0)
				{
					AddThreat(g, g.m_aSamples[ea].m_vL + 1.2 * vector.Up, 2, r.m_iId);
					AddThreat(g, g.m_aSamples[eb].m_vL + 1.2 * vector.Up, 2, r.m_iId);
				}
			}
			if (r.m_iKind == 2 || r.m_iKind == 3)
				continue;
			foreach (int si : r.m_aSamples)
			{
				DCO_INavSample s = g.m_aSamples[si];
				for (int n = 0; n < 4; n++)
				{
					int bit = 1 << n;
					if ((s.m_iDirAll & bit) != 0)
						continue;
					vector dir;
					if (n == 0)
						dir = Vector(1, 0, 0);
					else if (n == 1)
						dir = Vector(-1, 0, 0);
					else if (n == 2)
						dir = Vector(0, 0, 1);
					else
						dir = Vector(0, 0, -1);
					vector eye = g.W(s.m_vL + 1.3 * vector.Up);
					vector dw = g.m_Building.VectorToParent(dir);
					float high = Trace(g, eye, eye + dw * 3.0);
					if (high < 0.999)
						continue;
					vector knee = g.W(s.m_vL + 0.4 * vector.Up);
					float low = Trace(g, knee, knee + dw * 3.0);
					if (low >= 0.999)
						continue;
					AddThreat(g, s.m_vL + dir * (3.0 * low) + 1.3 * vector.Up, 1, r.m_iId);
					r.m_iWindows++;
				}
			}
		}
	}

	static string Json(DCO_INavGraph g)
	{
		string s = "{\"prefab\":\"" + g.m_sPrefab + "\",\"pos\":\"" + g.m_Building.GetOrigin().ToString(false) + "\"";
		s += string.Format(",\"step\":%1,\"ms\":%2,\"traces\":%3", g.m_fStep, Math.Round(g.m_fMs), g.m_iTraces);
		s += ",\"mins\":" + V(g.m_vMins) + ",\"maxs\":" + V(g.m_vMaxs);
		s += ",\"floors\":[";
		foreach (int fi, float fy : g.m_aFloors)
		{
			if (fi > 0)
				s += ",";
			s += fy.ToString(1, 2);
		}
		s += "],\"samples\":[";
		foreach (int i, DCO_INavSample smp : g.m_aSamples)
		{
			if (i > 0)
				s += ",";
			s += string.Format("[%1,%2,%3,%4,%5]", smp.m_vL[0].ToString(1, 2), smp.m_vL[1].ToString(1, 2), smp.m_vL[2].ToString(1, 2), smp.m_iRoom, smp.m_iFloor);
		}
		s += "],\"links\":[";
		foreach (int li, int a : g.m_aLinkA)
		{
			if (li > 0)
				s += ",";
			s += string.Format("[%1,%2,%3]", a, g.m_aLinkB[li], g.m_aLinkKind[li]);
		}
		s += "],\"rooms\":[";
		foreach (int ri, DCO_INavRoom r : g.m_aRooms)
		{
			if (ri > 0)
				s += ",";
			s += string.Format("{\"id\":%1,\"kind\":%2,\"floor\":%3,\"n\":%4,\"access\":%5,\"windows\":%6,\"center\":%7}", r.m_iId, r.m_iKind, r.m_iFloor, r.m_aSamples.Count(), r.m_iAccess, r.m_iWindows, r.m_iCenter);
		}
		s += "],\"doors\":[";
		foreach (int di, DCO_INavDoor d : g.m_aDoors)
		{
			if (di > 0)
				s += ",";
			s += string.Format("{\"p\":%1,\"n\":%2,\"hw\":%3,\"real\":%4,\"a\":%5,\"b\":%6}", V(d.m_vL), V(d.m_vN), d.m_fHW.ToString(1, 2), d.m_bReal, d.m_iRoomA, d.m_iRoomB);
		}
		s += "],\"points\":[";
		foreach (int pi, DCO_INavPoint p : g.m_aPoints)
		{
			if (pi > 0)
				s += ",";
			s += string.Format("{\"t\":%1,\"p\":%2,\"room\":%3,\"order\":%4,\"level\":%5,\"door\":%6}", p.m_iType, V(p.m_vL), p.m_iRoom, p.m_iOrder, p.m_iLevel, p.m_iDoor);
		}
		s += "],\"rejects\":[";
		foreach (int ji, vector jp : g.m_aRejP)
		{
			if (ji > 0)
				s += ",";
			s += string.Format("[%1,%2,%3,%4]", jp[0].ToString(1, 2), jp[1].ToString(1, 2), jp[2].ToString(1, 2), g.m_aRejWhy[ji]);
		}
		s += "]}";
		return s;
	}

	protected static string V(vector p)
	{
		return string.Format("[%1,%2,%3]", p[0].ToString(1, 2), p[1].ToString(1, 2), p[2].ToString(1, 2));
	}

	static string Dump(DCO_INavGraph g)
	{
		FileIO.MakeDirectory("$profile:DCO");
		FileIO.MakeDirectory("$profile:DCO/IndoorNav");
		string name = g.m_sPrefab;
		int slash = name.LastIndexOf("/");
		if (slash >= 0)
			name = name.Substring(slash + 1, name.Length() - slash - 1);
		name.Replace(".et", "");
		vector o = g.m_Building.GetOrigin();
		string path = string.Format("$profile:DCO/IndoorNav/%1_%2_%3.json", name, Math.Round(o[0]), Math.Round(o[2]));
		FileHandle f = FileIO.OpenFile(path, FileMode.WRITE);
		if (!f)
			return string.Empty;
		f.Write(Json(g));
		f.Close();
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", "inav_dump path=" + path);
		return path;
	}
}
