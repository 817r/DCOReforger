class DCO_GarrisonBuilding
{
	IEntity m_Entity;
	ResourceName m_sPrefab;
	ref array<ref DCO_GarrisonSlot> m_aSlots = {};
	ref array<AIAgent> m_aBookedBy = {};
	SCR_AIGroup m_Owner;
	bool m_bReady;

	int CountFreeSlots()
	{
		int n;
		foreach (AIAgent a : m_aBookedBy)
		{
			if (!a)
				n++;
		}
		return n;
	}
}

class DCO_GarrisonRegistry
{
	static const int TRACE_BUDGET_PER_FRAME = 60;
	static const float MIN_BUILDING_HALF_WIDTH = 4.0;
	static const int UNREACHABLE_FAILS = 2;

	protected static ref DCO_GarrisonRegistry s_Instance;
	static bool s_bDebugDraw;

	protected BaseWorld m_World;
	protected ref map<ResourceName, ref DCO_GarrisonPrefabSlots> m_mPrefabCache = new map<ResourceName, ref DCO_GarrisonPrefabSlots>();
	protected ref map<IEntity, ref DCO_GarrisonBuilding> m_mBuildings = new map<IEntity, ref DCO_GarrisonBuilding>();
	protected ref array<ref DCO_GarrisonSlotGenerator> m_aQueue = {};
	protected ref array<int> m_aQueuePriority = {};
	protected ref array<IEntity> m_aQueryResult = {};
	protected ref TraceParam m_Trace = new TraceParam();

	protected int m_iCacheHits, m_iCacheMisses, m_iTracesTotal, m_iFramesBusy, m_iPeakQueue;
	protected float m_fLastReport_ms;

	static DCO_GarrisonRegistry GetInstance()
	{
		if (!Replication.IsServer())
			return null;

		if (!s_Instance || s_Instance.m_World != GetGame().GetWorld())
			s_Instance = new DCO_GarrisonRegistry();

		return s_Instance;
	}

	void DCO_GarrisonRegistry()
	{
		m_World = GetGame().GetWorld();
		GetGame().GetCallqueue().CallLater(Tick, 0, true);
	}

	void ~DCO_GarrisonRegistry()
	{
		if (GetGame() && GetGame().GetCallqueue())
			GetGame().GetCallqueue().Remove(Tick);
	}

	void FindBuildings(vector center, float radius, notnull array<IEntity> outBuildings)
	{
		m_aQueryResult.Clear();
		DCO_Perf.Count("q:DCO_GarrisonRegistry");
		GetGame().GetWorld().QueryEntitiesBySphere(center, radius, OnBuildingQuery);
		outBuildings.Copy(m_aQueryResult);
	}

	protected bool OnBuildingQuery(IEntity e)
	{
		if (!e)
			return true;

		DCO_BuildingPositionComponent comp = DCO_BuildingPositionComponent.Cast(e.FindComponent(DCO_BuildingPositionComponent));
		if (!comp || !comp.GetBuildingEntity())
			return true;

		vector mins, maxs;
		e.GetBounds(mins, maxs);
		if (0.5 * (maxs[0] - mins[0]) < MIN_BUILDING_HALF_WIDTH && 0.5 * (maxs[2] - mins[2]) < MIN_BUILDING_HALF_WIDTH)
			return true;

		if (!m_aQueryResult.Contains(e))
			m_aQueryResult.Insert(e);
		return true;
	}

	DCO_GarrisonBuilding Request(IEntity building, int priority = 0)
	{
		if (!building)
			return null;

		DCO_GarrisonBuilding b = m_mBuildings.Get(building);
		if (b)
			return b;

		b = new DCO_GarrisonBuilding();
		b.m_Entity = building;
		b.m_sPrefab = SCR_ResourceNameUtils.GetPrefabName(building);
		m_mBuildings.Insert(building, b);

		DCO_GarrisonPrefabSlots cached = m_mPrefabCache.Get(b.m_sPrefab);
		if (cached && !b.m_sPrefab.IsEmpty())
		{
			m_iCacheHits++;
			Instantiate(b, cached);
			return b;
		}

		m_iCacheMisses++;
		if (!IsQueued(b.m_sPrefab, building))
			Enqueue(new DCO_GarrisonSlotGenerator(building, b.m_sPrefab), priority);

		return b;
	}

	int CountFreeBuildings(vector center, float radius, AIGroup group)
	{
		array<IEntity> found = {};
		FindBuildings(center, radius, found);

		int free = 0;
		foreach (IEntity b : found)
		{
			if (!IsBuildingUsedByOther(b, group))
				free++;
		}
		return free;
	}

	static bool IsBuildingGarrisonedByOther(IEntity building, AIGroup group)
	{
		if (!s_Instance || !building)
			return false;

		DCO_GarrisonBuilding b = s_Instance.m_mBuildings.Get(building);
		return b && b.m_Owner && b.m_Owner != group;
	}

	protected static const float SOFT_OWNER_TTL_MS = 180000;
	protected ref map<IEntity, AIGroup> m_mSoftOwner = new map<IEntity, AIGroup>();
	protected ref map<IEntity, float> m_mSoftTime = new map<IEntity, float>();

	static void MarkIndoorUse(IEntity building, AIGroup group)
	{
		DCO_GarrisonRegistry reg = GetInstance();
		if (!reg || !building || !group)
			return;
		reg.m_mSoftOwner.Set(building, group);
		reg.m_mSoftTime.Set(building, GetGame().GetWorld().GetWorldTime());
	}

	static bool IsBuildingUsedByOther(IEntity building, AIGroup group)
	{
		if (!building)
			return false;
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (cfg && cfg.GetShareBuildings())
			return IsBuildingGarrisonedByOther(building, group);

		if (IsBuildingGarrisonedByOther(building, group))
			return true;
		return s_Instance && s_Instance.IsSoftOwnedByOther(building, group);
	}

	protected bool IsSoftOwnedByOther(IEntity building, AIGroup group)
	{
		AIGroup soft;
		if (!m_mSoftOwner.Find(building, soft))
			return false;
		if (!soft || soft.GetAgentsCount() == 0 || GetGame().GetWorld().GetWorldTime() - m_mSoftTime.Get(building) > SOFT_OWNER_TTL_MS)
		{
			m_mSoftOwner.Remove(building);
			m_mSoftTime.Remove(building);
			return false;
		}
		return soft != group;
	}

	bool IsOwnedByOther(DCO_GarrisonBuilding b, SCR_AIGroup group)
	{
		if (b.m_Owner && b.m_Owner != group)
			return true;
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (cfg && cfg.GetShareBuildings())
			return false;
		return IsSoftOwnedByOther(b.m_Entity, group);
	}

	bool Claim(DCO_GarrisonBuilding b, SCR_AIGroup group)
	{
		if (!b || IsOwnedByOther(b, group))
			return false;

		b.m_Owner = group;
		return true;
	}

	void Release(DCO_GarrisonBuilding b, SCR_AIGroup group)
	{
		if (!b || b.m_Owner != group)
			return;

		b.m_Owner = null;
		for (int i = 0; i < b.m_aBookedBy.Count(); i++)
			b.m_aBookedBy[i] = null;
	}

	void Book(DCO_GarrisonBuilding b, int slotIdx, AIAgent agent)
	{
		if (b && b.m_aBookedBy.IsIndexValid(slotIdx))
			b.m_aBookedBy[slotIdx] = agent;
	}

	void MarkUnreachable(DCO_GarrisonBuilding b, int slotIdx)
	{
		if (!b || !b.m_aSlots.IsIndexValid(slotIdx))
			return;

		b.m_aSlots[slotIdx].m_bUnreachable = true;
		b.m_aBookedBy[slotIdx] = null;

		DCO_GarrisonPrefabSlots prefab = m_mPrefabCache.Get(b.m_sPrefab);
		if (prefab && prefab.m_aSlots.IsIndexValid(slotIdx))
			prefab.m_aSlots[slotIdx].m_iFailCount++;

		DCO_BenchmarkLoggerComponent.Event(string.Format("garrison_unreachable slot=%1 type=%2 prefab=%3", slotIdx, typename.EnumToString(DCO_EGarrisonSlotType, b.m_aSlots[slotIdx].m_eType), FilePath.StripPath(b.m_sPrefab)));
	}

	void Invalidate(IEntity building)
	{
		m_mBuildings.Remove(building);
	}

	protected bool IsQueued(ResourceName prefab, IEntity building)
	{
		foreach (DCO_GarrisonSlotGenerator g : m_aQueue)
		{
			if (g.m_Building == building || (!prefab.IsEmpty() && g.m_sPrefab == prefab))
				return true;
		}
		return false;
	}

	protected void Enqueue(DCO_GarrisonSlotGenerator gen, int priority)
	{
		int at = m_aQueue.Count();
		for (int i = 0; i < m_aQueuePriority.Count(); i++)
		{
			if (priority > m_aQueuePriority[i])
			{
				at = i;
				break;
			}
		}
		m_aQueue.InsertAt(gen, at);
		m_aQueuePriority.InsertAt(priority, at);
		m_iPeakQueue = Math.Max(m_iPeakQueue, m_aQueue.Count());
	}

	protected void Tick()
	{
		if (!m_aQueue.IsEmpty())
			ProcessQueue();

		if (s_bDebugDraw)
			DebugDraw();

		ReportMetrics();
	}

	protected void ProcessQueue()
	{
		m_iFramesBusy++;
		int budget = TRACE_BUDGET_PER_FRAME;
		while (budget > 0 && !m_aQueue.IsEmpty())
		{
			DCO_GarrisonSlotGenerator gen = m_aQueue[0];
			int used = gen.Step(budget);
			budget -= used;
			m_iTracesTotal += used;

			if (!gen.m_bDone)
				break;

			m_aQueue.RemoveOrdered(0);
			m_aQueuePriority.RemoveOrdered(0);
			OnGenerated(gen);
		}
	}

	protected void OnGenerated(DCO_GarrisonSlotGenerator gen)
	{
		if (!gen.m_sPrefab.IsEmpty())
			m_mPrefabCache.Set(gen.m_sPrefab, gen.m_Result);

		foreach (IEntity e, DCO_GarrisonBuilding b : m_mBuildings)
		{
			if (b.m_bReady || !b.m_Entity)
				continue;

			if (b.m_Entity == gen.m_Building || (!gen.m_sPrefab.IsEmpty() && b.m_sPrefab == gen.m_sPrefab))
				Instantiate(b, gen.m_Result);
		}

		int win, roof, door, interior;
		foreach (DCO_GarrisonSlot s : gen.m_Result.m_aSlots)
		{
			switch (s.m_eType)
			{
				case DCO_EGarrisonSlotType.WINDOW: win++; break;
				case DCO_EGarrisonSlotType.ROOF: roof++; break;
				case DCO_EGarrisonSlotType.DOOR_GUARD: door++; break;
				default: interior++; break;
			}
		}

		DCO_BenchmarkLoggerComponent.Event(string.Format("garrison_prefab slots=%1 win=%2 roof=%3 door=%4 interior=%5 traces=%6 prefab=%7",
			gen.m_Result.m_aSlots.Count(), win, roof, door, interior, gen.m_Result.m_iTraces, FilePath.StripPath(gen.m_sPrefab)));
	}

	protected void Instantiate(DCO_GarrisonBuilding b, DCO_GarrisonPrefabSlots prefabSlots)
	{
		b.m_aSlots.Clear();
		b.m_aBookedBy.Clear();

		foreach (DCO_GarrisonSlot src : prefabSlots.m_aSlots)
		{
			DCO_GarrisonSlot s = src.Duplicate();
			s.m_bUnreachable = src.m_iFailCount >= UNREACHABLE_FAILS;
			s.m_vWorldPos = b.m_Entity.CoordToParent(s.m_vLocalPos);
			vector dir = b.m_Entity.VectorToParent(DCO_GarrisonSlot.YawToDir(s.m_fLocalYaw));
			dir[1] = 0;
			s.m_vWorldDir = dir.Normalized();

			if (s.m_eType == DCO_EGarrisonSlotType.WINDOW || s.m_eType == DCO_EGarrisonSlotType.ROOF)
			{
				if (IsBlockedByNeighbour(b.m_Entity, s))
				{
					s.m_eType = DCO_EGarrisonSlotType.INTERIOR;
					s.m_eStance = ECharacterStance.CROUCH;
				}
			}

			b.m_aSlots.Insert(s);
			b.m_aBookedBy.Insert(null);
		}

		b.m_bReady = true;
	}

	protected bool IsBlockedByNeighbour(IEntity building, DCO_GarrisonSlot s)
	{
		vector start = s.m_vWorldPos + 1.2 * vector.Up;
		m_Trace.Flags = TraceFlags.ENTS | TraceFlags.WORLD;
		m_Trace.Start = start;
		m_Trace.End = start + s.m_vWorldDir * 10;
		m_Trace.TraceEnt = null;
		DCO_Perf.Count("t:DCO_GarrisonRegistry");
		float frac = GetGame().GetWorld().TraceMove(m_Trace, null);
		m_iTracesTotal++;

		if (frac >= 1 || !m_Trace.TraceEnt)
			return false;

		if (m_Trace.TraceEnt.GetRootParent() == building.GetRootParent())
			return false;

		return frac * 10 < 6;
	}

	protected void ReportMetrics()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fLastReport_ms < 60000)
			return;

		m_fLastReport_ms = now;
		if (m_iCacheHits + m_iCacheMisses == 0)
			return;

		float hitRate = m_iCacheHits / Math.Max(1.0, m_iCacheHits + m_iCacheMisses);
		float tracesPerBusyFrame = m_iTracesTotal / Math.Max(1.0, m_iFramesBusy);
		DCO_BenchmarkLoggerComponent.Event(string.Format("garrison_perf traces=%1 per_busy_frame=%2 busy_frames=%3 queue=%4 peak_queue=%5 cache_hit=%6 prefabs=%7 buildings=%8",
			m_iTracesTotal, tracesPerBusyFrame, m_iFramesBusy, m_aQueue.Count(), m_iPeakQueue, hitRate, m_mPrefabCache.Count(), m_mBuildings.Count()));
	}

	protected void DebugDraw()
	{
		foreach (IEntity e, DCO_GarrisonBuilding b : m_mBuildings)
		{
			if (!b.m_bReady || !b.m_Owner)
				continue;

			foreach (int i, DCO_GarrisonSlot s : b.m_aSlots)
			{
				int color = 0xFF888888;
				switch (s.m_eType)
				{
					case DCO_EGarrisonSlotType.WINDOW: color = 0xFFFF3333; break;
					case DCO_EGarrisonSlotType.ROOF: color = 0xFFFF9900; break;
					case DCO_EGarrisonSlotType.DOOR_GUARD: color = 0xFF3399FF; break;
				}

				vector p = s.m_vWorldPos + 0.3 * vector.Up;
				float size = 0.2;
				if (b.m_aBookedBy[i])
					size = 0.35;

				Shape.CreateSphere(color, ShapeFlags.ONCE | ShapeFlags.NOZBUFFER, p, size);
				vector line[2] = {p, p + s.m_vWorldDir * 1.5};
				Shape.CreateLines(color, ShapeFlags.ONCE | ShapeFlags.NOZBUFFER, line, 2);
			}
		}
	}
}
