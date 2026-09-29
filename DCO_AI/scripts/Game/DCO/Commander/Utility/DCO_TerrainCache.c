enum DCO_ETerrainFlag
{
	HILLTOP		= 1,
	JUNCTION	= 2,
	BRIDGE		= 4,
	FOREST_EDGE	= 8,
	OVERWATCH	= 16,
	FLAT_OPEN	= 32,
	ROAD		= 64
}

class DCO_TerrainPoint
{
	vector m_vPos;
	int m_iFlags;
	float m_fRelElev;
	ref array<string> m_aLos = {};
}

class DCO_TerrainCache
{
	protected static const float START_DELAY_S = 15;
	protected static const string DIR = "$profile:DCO/TerrainCache";

	protected static ref DCO_TerrainCache s_Instance;

	protected bool m_bReady;
	protected bool m_bStarted;
	protected float m_fElapsed;
	protected float m_fStartTick_ms;
	protected string m_sFile;
	protected string m_sKey;
	protected RoadNetworkManager m_Roads;

	protected ref array<ref DCO_TerrainPoint> m_aPoints = {};
	protected ref array<vector> m_aPending = {};
	protected int m_iPendingIdx;
	protected ref array<vector> m_aObjPos = {};
	protected ref array<string> m_aObjNames = {};

	protected int m_iTreeCount;

	static DCO_TerrainCache Get()
	{
		if (!s_Instance)
			s_Instance = new DCO_TerrainCache();
		return s_Instance;
	}

	static bool IsReady()
	{
		return s_Instance && s_Instance.m_bReady;
	}

	static void Tick(float timeSlice)
	{
		DCO_TerrainCache c = Get();
		if (c.m_bReady)
			return;

		c.m_fElapsed += timeSlice;
		if (!c.m_bStarted)
		{
			if (c.m_fElapsed >= START_DELAY_S)
				c.Start();
			return;
		}

		c.Step();
	}

	static int Query(int mask, vector center, float radius, notnull array<DCO_TerrainPoint> outPts, string losObj = "")
	{
		if (!IsReady())
			return 0;

		float r2 = radius * radius;
		foreach (DCO_TerrainPoint p : s_Instance.m_aPoints)
		{
			if (!(p.m_iFlags & mask) || vector.DistanceSqXZ(p.m_vPos, center) > r2)
				continue;
			if (!losObj.IsEmpty() && !p.m_aLos.Contains(losObj))
				continue;
			outPts.Insert(p);
		}
		return outPts.Count();
	}

	static vector QueryBest(int mask, vector center, float radius, string losObj = "", vector prefer = "0 0 0")
	{
		array<DCO_TerrainPoint> pts = {};
		if (Query(mask, center, radius, pts, losObj) == 0)
			return vector.Zero;

		vector anchor = center;
		if (prefer != vector.Zero)
			anchor = prefer;

		float best = -float.MAX;
		vector bestPos;
		foreach (DCO_TerrainPoint p : pts)
		{
			float s = p.m_fRelElev - vector.DistanceXZ(p.m_vPos, anchor) / 50;
			if (s > best)
			{
				best = s;
				bestPos = p.m_vPos;
			}
		}
		return bestPos;
	}

	protected void Start()
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr || mgr.m_aObjective.IsEmpty())
			return;

		m_bStarted = true;
		m_fStartTick_ms = System.GetTickCount();

		vector mins = Vector(float.MAX, 0, float.MAX);
		vector maxs = Vector(-float.MAX, 0, -float.MAX);
		string hash;
		foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
		{
			if (!o || !o.GetOwner())
				continue;
			vector p = o.GetOwner().GetOrigin();
			m_aObjPos.Insert(p);
			m_aObjNames.Insert(o.GetOwner().GetName());
			hash += string.Format("%1:%2,%3;", o.GetOwner().GetName(), Math.Round(p[0] / 10), Math.Round(p[2] / 10));
			mins[0] = Math.Min(mins[0], p[0]);
			mins[2] = Math.Min(mins[2], p[2]);
			maxs[0] = Math.Max(maxs[0], p[0]);
			maxs[2] = Math.Max(maxs[2], p[2]);
		}

		string world = GetGame().GetWorldFile();
		m_sKey = string.Format("v%1|%2|%3", 1, world, hash.Hash());
		string safe = world;
		array<string> bad = {"/", "{", "}", ".", ":", " "};
		foreach (string ch : bad)
			safe.Replace(ch, "_");
		m_sFile = DIR + "/" + safe + ".json";

		if (Load())
		{
			m_bReady = true;
			Print(string.Format("[DCO_Terrain] cache hit %1 (%2 titik)", m_sFile, m_aPoints.Count()));
			return;
		}

		ChimeraAIWorld aiWorld = ChimeraAIWorld.Cast(GetGame().GetAIWorld());
		if (aiWorld)
			m_Roads = aiWorld.GetRoadNetworkManager();

		mins = mins - Vector(600.0, 0, 600.0);
		maxs = maxs + Vector(600.0, 0, 600.0);
		AnalyzeRoads(mins, maxs);

		array<float> dists = {};
		for (float x = mins[0]; x <= maxs[0]; x += 50.0)
		{
			for (float z = mins[2]; z <= maxs[2]; z += 50.0)
			{
				vector c = Vector(x, 0, z);
				m_aPending.Insert(c);
				dists.Insert(NearestObjDist(c));
			}
		}
		SortByDistance(dists);
		Print(string.Format("[DCO_Terrain] mulai hitung %1 sel (%2 objective), budget %3 sel/frame", m_aPending.Count(), m_aObjPos.Count(), 6));
	}

	protected void Step()
	{
		int end = Math.Min(m_iPendingIdx + 6, m_aPending.Count());
		for (; m_iPendingIdx < end; m_iPendingIdx++)
			EvaluateCell(m_aPending[m_iPendingIdx]);

		if (m_iPendingIdx < m_aPending.Count())
			return;

		m_bReady = true;
		m_aPending.Clear();
		Save();

		int counts[7];
		foreach (DCO_TerrainPoint p : m_aPoints)
		{
			for (int b = 0; b < 7; b++)
			{
				if (p.m_iFlags & (1 << b))
					counts[b] = counts[b] + 1;
			}
		}
		string line = string.Format("terrain_cache built points=%1 time=%2s hill=%3 junction=%4 bridge=%5 forest=%6 overwatch=%7 lz=%8",
			m_aPoints.Count(), ((System.GetTickCount() - m_fStartTick_ms) / 1000).ToString(-1, 1), counts[0], counts[1], counts[2], counts[3], counts[4], counts[5]);
		Print("[DCO_Terrain] " + line);
		DCO_BenchmarkLoggerComponent.Event(line);
	}

	protected void EvaluateCell(vector c)
	{
		BaseWorld world = GetGame().GetWorld();
		c[1] = world.GetSurfaceY(c[0], c[2]);

		EWaterSurfaceType waterType;
		float lakeArea;
		if (SCR_WorldTools.GetWaterSurfaceY(world, c, waterType, lakeArea) > c[1])
			return;

		float sum, minH = float.MAX, maxH = -float.MAX;
		for (int i = 0; i < 8; i++)
		{
			float a = i * Math.PI2 / 8;
			float h = world.GetSurfaceY(c[0] + Math.Cos(a) * 120.0, c[2] + Math.Sin(a) * 120.0);
			sum += h;
		}
		float rel = c[1] - sum / 8;

		for (int i = 0; i < 4; i++)
		{
			float a = i * Math.PI2 / 4;
			float h = world.GetSurfaceY(c[0] + Math.Cos(a) * 20, c[2] + Math.Sin(a) * 20);
			minH = Math.Min(minH, h);
			maxH = Math.Max(maxH, h);
		}

		int flags;
		if (rel > 10.0)
			flags |= DCO_ETerrainFlag.HILLTOP;

		float roadDist = float.MAX;
		if (m_Roads)
		{
			BaseRoad road;
			m_Roads.GetClosestRoad(c, road, roadDist);
			if (road && roadDist <= 25.0)
				flags |= DCO_ETerrainFlag.ROAD;
		}

		float objDist = NearestObjDist(c);
		m_iTreeCount = 0;
		if (roadDist <= 80.0 || objDist <= 900.0)
		{
			DCO_Perf.Count("q:DCO_TerrainCache");
			world.QueryEntitiesBySphere(c, 20, CountTree, null, EQueryEntitiesFlags.STATIC);
		}

		if (roadDist <= 80.0 && m_iTreeCount >= 3 && m_iTreeCount <= 14)
			flags |= DCO_ETerrainFlag.FOREST_EDGE;

		if (objDist <= 900.0 && m_iTreeCount == 0 && maxH - minH <= 2.5)
			flags |= DCO_ETerrainFlag.FLAT_OPEN;

		DCO_TerrainPoint p = new DCO_TerrainPoint();
		foreach (int i, vector o : m_aObjPos)
		{
			float d = vector.DistanceXZ(c, o);
			if (d < 150.0 || d > 500.0)
				continue;
			if (HasLOS(c, o))
				p.m_aLos.Insert(m_aObjNames[i]);
		}
		if (!p.m_aLos.IsEmpty())
			flags |= DCO_ETerrainFlag.OVERWATCH;

		if (flags == 0)
			return;

		p.m_vPos = c;
		p.m_iFlags = flags;
		p.m_fRelElev = rel;
		m_aPoints.Insert(p);
	}

	protected bool CountTree(IEntity e)
	{
		if (Tree.Cast(e))
			m_iTreeCount++;
		return true;
	}

	protected static bool HasLOS(vector from, vector to)
	{
		TraceParam trace = new TraceParam();
		trace.Start = Vector(from[0], from[1] + 1.8, from[2]);
		trace.End = Vector(to[0], GetGame().GetWorld().GetSurfaceY(to[0], to[2]) + 1.5, to[2]);
		trace.Flags = TraceFlags.ANY_CONTACT | TraceFlags.WORLD | TraceFlags.ENTS;
		DCO_Perf.Count("t:DCO_TerrainCache");
		return GetGame().GetWorld().TraceMove(trace, null) >= 0.97;
	}

	protected void AnalyzeRoads(vector mins, vector maxs)
	{
		if (!m_Roads)
			return;

		array<BaseRoad> roads = {};
		m_Roads.GetRoadsInAABB(Vector(mins[0], -100, mins[2]), Vector(maxs[0], 1000, maxs[2]), roads);

		BaseWorld world = GetGame().GetWorld();
		array<vector> ends = {};
		array<vector> pts = {};
		foreach (BaseRoad r : roads)
		{
			pts.Clear();
			if (!r || r.GetPoints(pts) < 2)
				continue;

			ends.Insert(pts[0]);
			ends.Insert(pts[pts.Count() - 1]);

			foreach (vector rp : pts)
			{
				EWaterSurfaceType wt;
				float la;
				float ground = world.GetSurfaceY(rp[0], rp[2]);
				if (rp[1] - ground > 3 || SCR_WorldTools.GetWaterSurfaceY(world, rp, wt, la) > ground + 0.5)
				{
					AddRoadPoint(rp, DCO_ETerrainFlag.BRIDGE);
					break;
				}
			}
		}

		for (int i = 0; i < ends.Count(); i++)
		{
			int n = 1;
			for (int j = 0; j < ends.Count(); j++)
			{
				if (i != j && vector.DistanceSqXZ(ends[i], ends[j]) < 100)
					n++;
			}
			if (n >= 3)
				AddRoadPoint(ends[i], DCO_ETerrainFlag.JUNCTION);
		}
	}

	protected void AddRoadPoint(vector pos, int flag)
	{
		foreach (DCO_TerrainPoint p : m_aPoints)
		{
			if ((p.m_iFlags & flag) && vector.DistanceSqXZ(p.m_vPos, pos) < 400)
				return;
		}

		DCO_TerrainPoint p = new DCO_TerrainPoint();
		p.m_vPos = pos;
		p.m_iFlags = flag | DCO_ETerrainFlag.ROAD;
		m_aPoints.Insert(p);
	}

	protected float NearestObjDist(vector c)
	{
		float best = float.MAX;
		foreach (vector o : m_aObjPos)
			best = Math.Min(best, vector.DistanceXZ(c, o));
		return best;
	}

	protected void SortByDistance(array<float> dists)
	{
		array<int> idx = {};
		for (int i = 0; i < dists.Count(); i++)
			idx.Insert(i);

		map<int, ref array<vector>> buckets = new map<int, ref array<vector>>();
		int maxB;
		foreach (int i : idx)
		{
			int b = dists[i] / 100;
			maxB = Math.Max(maxB, b);
			array<vector> arr = buckets.Get(b);
			if (!arr)
			{
				arr = {};
				buckets.Insert(b, arr);
			}
			arr.Insert(m_aPending[i]);
		}

		m_aPending.Clear();
		for (int b = 0; b <= maxB; b++)
		{
			array<vector> arr = buckets.Get(b);
			if (arr)
				m_aPending.InsertAll(arr);
		}
	}

	protected void Save()
	{
		FileIO.MakeDirectory("$profile:DCO");
		FileIO.MakeDirectory(DIR);

		array<float> xs = {}, ys = {}, zs = {}, els = {};
		array<int> fl = {};
		array<string> los = {};
		foreach (DCO_TerrainPoint p : m_aPoints)
		{
			xs.Insert(p.m_vPos[0]);
			ys.Insert(p.m_vPos[1]);
			zs.Insert(p.m_vPos[2]);
			els.Insert(p.m_fRelElev);
			fl.Insert(p.m_iFlags);
			string l;
			foreach (int i, string n : p.m_aLos)
			{
				if (i > 0)
					l += "|";
				l += n;
			}
			los.Insert(l);
		}

		SCR_JsonSaveContext ctx = new SCR_JsonSaveContext();
		ctx.WriteValue("key", m_sKey);
		ctx.WriteValue("x", xs);
		ctx.WriteValue("y", ys);
		ctx.WriteValue("z", zs);
		ctx.WriteValue("e", els);
		ctx.WriteValue("f", fl);
		ctx.WriteValue("los", los);
		if (!ctx.SaveToFile(m_sFile))
			Print("[DCO_Terrain] gagal nyimpen " + m_sFile, LogLevel.WARNING);
	}

	protected bool Load()
	{
		if (!FileIO.FileExists(m_sFile))
			return false;

		SCR_JsonLoadContext ctx = new SCR_JsonLoadContext();
		if (!ctx.LoadFromFile(m_sFile))
			return false;

		string key;
		if (!ctx.ReadValue("key", key) || key != m_sKey)
			return false;

		array<float> xs = {}, ys = {}, zs = {}, els = {};
		array<int> fl = {};
		array<string> los = {};
		if (!ctx.ReadValue("x", xs) || !ctx.ReadValue("y", ys) || !ctx.ReadValue("z", zs) || !ctx.ReadValue("e", els) || !ctx.ReadValue("f", fl) || !ctx.ReadValue("los", los))
			return false;

		m_aPoints.Clear();
		for (int i = 0; i < xs.Count(); i++)
		{
			DCO_TerrainPoint p = new DCO_TerrainPoint();
			p.m_vPos = Vector(xs[i], ys[i], zs[i]);
			p.m_iFlags = fl[i];
			p.m_fRelElev = els[i];
			if (!los[i].IsEmpty())
				los[i].Split("|", p.m_aLos, true);
			m_aPoints.Insert(p);
		}
		return true;
	}
}
