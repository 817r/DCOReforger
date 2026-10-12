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

class DCO_BuildingQueryCell
{
	ref array<IEntity> m_aBuildings = {};
	float m_fUntil_ms;
}

class DCO_GarrisonRegistry
{

	protected static ref DCO_GarrisonRegistry s_Instance;
	static bool s_bDebugDraw;

	protected BaseWorld m_World;
	protected ref map<ResourceName, ref DCO_GarrisonPrefabSlots> m_mPrefabCache = new map<ResourceName, ref DCO_GarrisonPrefabSlots>();
	protected ref map<IEntity, ref DCO_GarrisonBuilding> m_mBuildings = new map<IEntity, ref DCO_GarrisonBuilding>();
	protected ref array<ref DCO_GarrisonSlotGenerator> m_aQueue = {};
	protected ref array<int> m_aQueuePriority = {};
	protected ref array<IEntity> m_aQueryResult = {};

	protected int m_iCacheHits, m_iCacheMisses, m_iTracesTotal, m_iFramesBusy, m_iPeakQueue;
	protected float m_fLastReport_ms;
	protected float m_fNextSave_ms;

	int GetQueueCount()
	{
		return m_aQueue.Count();
	}

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
		m_fNextSave_ms = m_World.GetWorldTime() + 300000.0;
		DCO_BldCache.Preload();
		GetGame().GetCallqueue().CallLater(Tick, 0, true);
	}

	void ~DCO_GarrisonRegistry()
	{
		if (GetGame() && GetGame().GetCallqueue())
			GetGame().GetCallqueue().Remove(Tick);
	}

	protected ref map<int, ref DCO_BuildingQueryCell> m_mQueryCells = new map<int, ref DCO_BuildingQueryCell>();

	void FindBuildings(vector center, float radius, notnull array<IEntity> outBuildings)
	{
		outBuildings.Clear();
		if (radius > 112.0)
		{
			m_aQueryResult.Clear();
			DCO_Perf.Count("q:DCO_GarrisonRegistry_big");
			GetGame().GetWorld().QueryEntitiesBySphere(center, radius, OnBuildingQuery);
			outBuildings.Copy(m_aQueryResult);
			return;
		}
		float now = m_World.GetWorldTime();
		int cx = Math.Floor(center[0] / 16.0);
		int cz = Math.Floor(center[2] / 16.0);
		int rb = Math.Ceil(radius / 16.0);
		int key = (cx * 1024 + cz) * 8 + rb;
		DCO_BuildingQueryCell cell = m_mQueryCells.Get(key);
		if (!cell || now > cell.m_fUntil_ms)
		{
			if (m_mQueryCells.Count() > 4096)
				m_mQueryCells.Clear();
			m_aQueryResult.Clear();
			DCO_Perf.Count("q:DCO_GarrisonRegistry");
			vector cc = Vector((cx + 0.5) * 16.0, center[1], (cz + 0.5) * 16.0);
			GetGame().GetWorld().QueryEntitiesBySphere(cc, rb * 16.0 + 12.0, OnBuildingQuery);
			cell = new DCO_BuildingQueryCell();
			cell.m_aBuildings.Copy(m_aQueryResult);
			cell.m_fUntil_ms = now + 300000.0;
			m_mQueryCells.Set(key, cell);
		}
		else
			DCO_Perf.Count("q:DCO_GarrisonRegistry_hit");
		float r2 = radius * radius;
		foreach (IEntity b : cell.m_aBuildings)
		{
			if (!b)
				continue;
			vector mins, maxs;
			b.GetWorldBounds(mins, maxs);
			float dx = Math.Max(Math.Max(mins[0] - center[0], 0.0), center[0] - maxs[0]);
			float dz = Math.Max(Math.Max(mins[2] - center[2], 0.0), center[2] - maxs[2]);
			if (dx * dx + dz * dz <= r2)
				outBuildings.Insert(b);
		}
	}

	void FindBuildingsAt(vector pos, float radius, notnull array<IEntity> outBuildings)
	{
		outBuildings.Clear();
		m_aQueryResult.Clear();
		DCO_Perf.Count("q:DCO_GarrisonRegistry_at");
		GetGame().GetWorld().QueryEntitiesBySphere(pos, radius, OnBuildingQuery);
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
		if (0.5 * (maxs[0] - mins[0]) < 4.0 && 0.5 * (maxs[2] - mins[2]) < 4.0)
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
		if (!cached && !b.m_sPrefab.IsEmpty())
		{
			cached = DCO_BldCache.Load(b.m_sPrefab, building);
			if (cached)
				m_mPrefabCache.Set(b.m_sPrefab, cached);
		}
		if (cached && cached.m_bGrid && !b.m_sPrefab.IsEmpty())
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
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(group);
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
		if (!soft || soft.GetAgentsCount() == 0 || GetGame().GetWorld().GetWorldTime() - m_mSoftTime.Get(building) > 180000.0)
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
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(group);
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
		{
			b.m_aBookedBy[slotIdx] = agent;
			if (agent)
				DCO_BldLearn.Occupy(agent.GetControlledEntity(), b, b.m_aSlots[slotIdx], "defend");
		}
	}

	DCO_GarrisonBuilding Peek(IEntity building)
	{
		return m_mBuildings.Get(building);
	}

	bool IsComplete(DCO_GarrisonBuilding b)
	{
		if (!b || b.m_sPrefab.IsEmpty())
			return false;
		DCO_GarrisonPrefabSlots p = m_mPrefabCache.Get(b.m_sPrefab);
		return p && p.m_bGrid && p.m_bComplete;
	}

	bool AddSample(DCO_GarrisonBuilding b, vector worldPos, float cover, float fire, vector fireDir, float winDist)
	{
		if (!b || !b.m_Entity || b.m_sPrefab.IsEmpty())
			return false;

		DCO_GarrisonPrefabSlots p = m_mPrefabCache.Get(b.m_sPrefab);
		if (!p)
		{
			p = new DCO_GarrisonPrefabSlots();
			vector mins, maxs;
			b.m_Entity.GetBounds(mins, maxs);
			p.m_vMins = mins;
			p.m_vMaxs = maxs;
			m_mPrefabCache.Set(b.m_sPrefab, p);
		}
		if (p.m_bComplete)
			return false;

		vector local = b.m_Entity.CoordToLocal(worldPos);
		foreach (vector g : p.m_aGone)
		{
			if (vector.DistanceSq(g, local) < 2.25)
			{
				DCO_Perf.Count("bldcache:gone_block");
				return false;
			}
		}
		foreach (DCO_GarrisonSlot k : p.m_aSlots)
		{
			if (vector.DistanceSq(k.m_vLocalPos, local) < 2.25)
				return false;
		}

		DCO_GarrisonSlot slot = new DCO_GarrisonSlot();
		slot.m_vLocalPos = local;
		slot.m_eType = DCO_EGarrisonSlotType.COVER;
		slot.m_eStance = ECharacterStance.CROUCH;
		slot.m_fArc = 90;
		slot.m_fCover = cover;
		slot.m_fFire = fire;
		slot.m_fScore = 0.5 * cover + 0.5 * fire;
		slot.m_fWinDist = winDist;
		vector ld = b.m_Entity.VectorToLocal(fireDir);
		ld[1] = 0;
		if (ld.LengthSq() > 0.01)
			slot.m_fLocalYaw = ld.ToYaw();
		int floor = Math.Floor((local[1] - p.m_vMins[1]) / 2.8);
		slot.m_iFloor = Math.Max(floor, 0);
		p.m_aSlots.Insert(slot);
		p.m_bDirty = true;

		foreach (IEntity e, DCO_GarrisonBuilding ib : m_mBuildings)
		{
			if (ib && ib.m_bReady && ib.m_Entity && ib.m_sPrefab == b.m_sPrefab)
				AddInstanceSlot(ib, slot);
		}

		if (p.m_aSlots.Count() >= 120)
			PruneFull(b.m_sPrefab, p);
		return true;
	}

	void EndSample(DCO_GarrisonBuilding b, int added, string unit)
	{
		if (!b || b.m_sPrefab.IsEmpty())
			return;
		DCO_GarrisonPrefabSlots p = m_mPrefabCache.Get(b.m_sPrefab);
		if (!p || p.m_bComplete)
			return;

		if (added > 0)
			p.m_iMisses = 0;
		else
			p.m_iMisses++;
		p.m_bDirty = true;

		if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, unit, string.Format("bldcache_add prefab=%1 unit=%2 added=%3 total=%4 misses=%5 grid=%6", DCO_BldCache.Name(b.m_sPrefab), unit, added, p.m_aSlots.Count(), p.m_iMisses, DCO_Util.B01(p.m_bGrid)));

		if (p.m_bGrid && p.m_iMisses >= 3)
			Complete(b.m_sPrefab, p, "saturated");
	}

	protected void Complete(ResourceName prefab, DCO_GarrisonPrefabSlots p, string why)
	{
		p.m_bComplete = true;
		p.m_bDirty = true;
		DCO_Perf.Count("bldcache:complete");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("bldcache_complete prefab=%1 id=%2 slots=%3 reason=%4", DCO_BldCache.Name(prefab), DCO_BldCache.Id(prefab), p.m_aSlots.Count(), why));
	}

	protected void PruneFull(ResourceName prefab, DCO_GarrisonPrefabSlots p)
	{
		array<float> rates = {};
		array<int> order = {};
		foreach (int i, DCO_GarrisonSlot s : p.m_aSlots)
		{
			if (s.m_eType != DCO_EGarrisonSlotType.COVER)
				continue;
			float r = DCO_BldLearn.PrefabRate(prefab, s);
			if (r >= 0.35)
				continue;
			int at = 0;
			while (at < rates.Count() && rates[at] <= r)
				at++;
			rates.InsertAt(r, at);
			order.InsertAt(i, at);
		}

		if (order.IsEmpty())
		{
			Complete(prefab, p, "full");
			return;
		}

		int want = Math.Round(1.0 * p.m_aSlots.Count() * 0.2);
		int drop = Math.Min(order.Count(), Math.Max(1, want));
		array<vector> gone = {};
		for (int d = 0; d < drop; d++)
			gone.Insert(p.m_aSlots[order[d]].m_vLocalPos);
		foreach (vector lp : gone)
			RemovePrefabSlot(prefab, p, lp);

		DCO_Perf.Count("bldcache:prune_full");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("bldcache_prune prefab=%1 reason=full dropped=%2 below=%3 slots=%4", DCO_BldCache.Name(prefab), drop, order.Count(), p.m_aSlots.Count()));
	}

	bool PruneSlot(ResourceName prefab, vector localPos, float rate, float n)
	{
		DCO_GarrisonPrefabSlots p = m_mPrefabCache.Get(prefab);
		if (!p)
			return false;

		int count = p.m_aSlots.Count();
		float sum = 0;
		int below = 0;
		foreach (DCO_GarrisonSlot s : p.m_aSlots)
		{
			float r = DCO_BldLearn.PrefabRate(prefab, s);
			sum += r;
			if (r < rate)
				below++;
		}
		float avg = 0;
		if (count > 0)
			avg = sum / count;
		string skip;
		if (count <= 6)
			skip = "floor";
		else if (rate >= avg - 0.2)
			skip = "relative";
		else if (below * 2 >= count)
			skip = "top_half";
		if (!skip.IsEmpty())
		{
			DCO_Perf.Count("bldcache:prune_skip_" + skip);
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("bldcache_prune_skip prefab=%1 rate=%2 avg=%3 below=%4 slots=%5 n=%6 reason=%7", DCO_BldCache.Name(prefab), rate.ToString(-1, 2), avg.ToString(-1, 2), below, count, n.ToString(-1, 1), skip));
			return false;
		}

		if (!RemovePrefabSlot(prefab, p, localPos))
			return false;
		p.m_aGone.Insert(localPos);
		p.m_aGoneSes.Insert(DCO_BldLearn.Session());
		while (p.m_aGone.Count() > 30)
		{
			p.m_aGone.RemoveOrdered(0);
			p.m_aGoneSes.RemoveOrdered(0);
		}
		p.m_bComplete = false;
		p.m_iMisses = 0;
		DCO_Perf.Count("bldcache:prune_learned");
		float lx = localPos[0];
		float ly = localPos[1];
		float lz = localPos[2];
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("bldcache_prune prefab=%1 reason=learned slot=%2_%3_%4 rate=%5 n=%6 slots=%7 reopened=1 avg=%8 gone=%9", DCO_BldCache.Name(prefab), lx.ToString(-1, 1), ly.ToString(-1, 1), lz.ToString(-1, 1), rate.ToString(-1, 2), n.ToString(-1, 1), p.m_aSlots.Count(), avg.ToString(-1, 2), p.m_aGone.Count()));
		return true;
	}

	int ClearGone()
	{
		int n = 0;
		foreach (ResourceName prefab, DCO_GarrisonPrefabSlots p : m_mPrefabCache)
		{
			if (!p || p.m_aGone.IsEmpty())
				continue;
			n += p.m_aGone.Count();
			p.m_aGone.Clear();
			p.m_aGoneSes.Clear();
			p.m_bDirty = true;
		}
		return n;
	}

	protected bool RemovePrefabSlot(ResourceName prefab, DCO_GarrisonPrefabSlots p, vector localPos)
	{
		int idx = -1;
		foreach (int i, DCO_GarrisonSlot s : p.m_aSlots)
		{
			if (vector.DistanceSq(s.m_vLocalPos, localPos) < 0.01)
			{
				idx = i;
				break;
			}
		}
		if (idx < 0)
			return false;
		p.m_aSlots.RemoveOrdered(idx);
		p.m_bDirty = true;

		foreach (IEntity e, DCO_GarrisonBuilding b : m_mBuildings)
		{
			if (!b || b.m_sPrefab != prefab)
				continue;
			foreach (DCO_GarrisonSlot bs : b.m_aSlots)
			{
				if (bs && vector.DistanceSq(bs.m_vLocalPos, localPos) < 0.01)
					bs.m_bUnreachable = true;
			}
		}
		return true;
	}

	protected void SaveDirty(string why)
	{
		int t0 = System.GetTickCount();
		int files = 0;
		int slots = 0;
		int failed = 0;
		foreach (ResourceName prefab, DCO_GarrisonPrefabSlots p : m_mPrefabCache)
		{
			if (!p || !p.m_bDirty || prefab.IsEmpty())
				continue;
			if (DCO_BldCache.Save(prefab, p))
			{
				files++;
				slots += p.m_aSlots.Count();
			}
			else
				failed++;
		}
		if (files + failed == 0)
			return;
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("bldcache_save files=%1 slots=%2 failed=%3 ms=%4 why=%5", files, slots, failed, System.GetTickCount() - t0, why));
	}

	static void SaveAll()
	{
		if (s_Instance)
			s_Instance.SaveDirty("end");
	}

	void MarkUnreachable(DCO_GarrisonBuilding b, int slotIdx, bool persistent = true)
	{
		if (!b || !b.m_aSlots.IsIndexValid(slotIdx))
			return;

		b.m_aSlots[slotIdx].m_bUnreachable = true;
		b.m_aBookedBy[slotIdx] = null;

		DCO_GarrisonPrefabSlots prefab;
		if (persistent)
			prefab = m_mPrefabCache.Get(b.m_sPrefab);
		if (prefab)
		{
			vector lp = b.m_aSlots[slotIdx].m_vLocalPos;
			foreach (DCO_GarrisonSlot ps : prefab.m_aSlots)
			{
				if (vector.DistanceSq(ps.m_vLocalPos, lp) < 0.01)
				{
					ps.m_iFailCount++;
					prefab.m_bDirty = true;
					break;
				}
			}
		}

		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("garrison_unreachable slot=%1 type=%2 prefab=%3 persistent=%4", slotIdx, typename.EnumToString(DCO_EGarrisonSlotType, b.m_aSlots[slotIdx].m_eType), FilePath.StripPath(b.m_sPrefab), DCO_Util.B01(persistent)));
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

		if (m_World && m_World.GetWorldTime() >= m_fNextSave_ms)
		{
			m_fNextSave_ms = m_World.GetWorldTime() + 300000.0;
			SaveDirty("auto");
		}

		ReportMetrics();
	}

	protected void ProcessQueue()
	{
		m_iFramesBusy++;
		int budget = 60;
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
		int kept = 0;
		if (!gen.m_sPrefab.IsEmpty())
		{
			DCO_GarrisonPrefabSlots old = m_mPrefabCache.Get(gen.m_sPrefab);
			if (old)
			{
				foreach (DCO_GarrisonSlot os : old.m_aSlots)
				{
					bool near = false;
					foreach (DCO_GarrisonSlot gs : gen.m_Result.m_aSlots)
					{
						if (vector.DistanceSq(gs.m_vLocalPos, os.m_vLocalPos) < 2.25)
						{
							near = true;
							break;
						}
					}
					if (!near && gen.m_Result.m_aSlots.Count() < 120)
					{
						gen.m_Result.m_aSlots.Insert(os);
						kept++;
					}
				}
				gen.m_Result.m_iMisses = old.m_iMisses;
			}
			if (gen.m_Building)
			{
				vector gmins, gmaxs;
				gen.m_Building.GetBounds(gmins, gmaxs);
				gen.m_Result.m_vMins = gmins;
				gen.m_Result.m_vMaxs = gmaxs;
			}
			gen.m_Result.m_bGrid = true;
			gen.m_Result.m_bDirty = true;
			m_mPrefabCache.Set(gen.m_sPrefab, gen.m_Result);
		}

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

		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("garrison_prefab slots=%1 win=%2 roof=%3 door=%4 interior=%5 traces=%6 prefab=%7",
			gen.m_Result.m_aSlots.Count(), win, roof, door, interior, gen.m_Result.m_iTraces, FilePath.StripPath(gen.m_sPrefab)));
		DCO_Perf.Count("bldcache:new");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("bldcache_new prefab=%1 id=%2 slots=%3 grid_traces=%4 kept_samples=%5 source=grid", DCO_BldCache.Name(gen.m_sPrefab), DCO_BldCache.Id(gen.m_sPrefab), gen.m_Result.m_aSlots.Count(), gen.m_Result.m_iTraces, kept));
	}

	protected void Instantiate(DCO_GarrisonBuilding b, DCO_GarrisonPrefabSlots prefabSlots)
	{
		b.m_aSlots.Clear();
		b.m_aBookedBy.Clear();

		foreach (DCO_GarrisonSlot src : prefabSlots.m_aSlots)
			AddInstanceSlot(b, src);

		b.m_bReady = true;
	}

	protected void AddInstanceSlot(DCO_GarrisonBuilding b, DCO_GarrisonSlot src)
	{
		DCO_GarrisonSlot s = src.Duplicate();
		s.m_bUnreachable = src.m_iFailCount >= 2;
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

	protected bool IsBlockedByNeighbour(IEntity building, DCO_GarrisonSlot s)
	{
		vector start = s.m_vWorldPos + 1.2 * vector.Up;
		float frac = DCO_Util.TraceFrac(start, start + s.m_vWorldDir * 10, null, TraceFlags.ENTS | TraceFlags.WORLD, -1, "t:DCO_GarrisonRegistry");
		IEntity hitEnt = DCO_Util.LastTraceEnt();
		m_iTracesTotal++;

		if (frac >= 1 || !hitEnt)
			return false;

		if (hitEnt.GetRootParent() == building.GetRootParent())
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

		float hitRate = 1.0 * m_iCacheHits / Math.Max(1.0, m_iCacheHits + m_iCacheMisses);
		float tracesPerBusyFrame = 1.0 * m_iTracesTotal / Math.Max(1.0, m_iFramesBusy);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("garrison_perf traces=%1 per_busy_frame=%2 busy_frames=%3 queue=%4 peak_queue=%5 cache_hit=%6 prefabs=%7 buildings=%8",
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
					case DCO_EGarrisonSlotType.COVER: color = 0xFFCCCCCC; break;
				}
				float ln = 0;
				float lm = DCO_BldLearn.SlotMult(b, s, vector.Zero, false, "defend", ln);
				if (ln > 0 && lm >= 1.05)
					color = 0xFF33FF33;
				else if (ln > 0 && lm <= 0.95)
					color = 0xFFAA00AA;

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
