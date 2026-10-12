[ComponentEditorProps(category: "AI/Cover", description: "Static-instance cover manager. Register vector positions, forget on release.")]
class SCR_CoverManagerComponentClass : ScriptComponentClass {}

class DCO_BldEntCache
{
	vector m_vPos;
	float m_fAt_ms = -1;
	bool m_bInside;
	float m_fBldAt_ms = -1;
	IEntity m_Building;
}

class DCO_BldPosCache
{
	float m_fY;
	float m_fAt_ms;
	IEntity m_Building;
	string m_sMethod;
}

class SCR_CoverManagerComponent : ScriptComponent
{
	protected static ref map<IEntity, ref DCO_BldEntCache> s_mDCOEnt;
	protected static map<IEntity, ref DCO_BldEntCache> GetDCOEnt()
	{
		if (!s_mDCOEnt)
			s_mDCOEnt = new map<IEntity, ref DCO_BldEntCache>();
		return s_mDCOEnt;
	}
	protected static ref map<int, ref DCO_BldPosCache> s_mDCOPos;
	protected static map<int, ref DCO_BldPosCache> GetDCOPos()
	{
		if (!s_mDCOPos)
			s_mDCOPos = new map<int, ref DCO_BldPosCache>();
		return s_mDCOPos;
	}

	protected static DCO_BldEntCache DCO_EntEntry(IEntity entity, float now)
	{
		DCO_BldEntCache c = GetDCOEnt().Get(entity);
		if (c)
			return c;
		if (GetDCOEnt().Count() > 1024)
		{
			for (int i = GetDCOEnt().Count() - 1; i >= 0; i--)
			{
				DCO_BldEntCache o = GetDCOEnt().GetElement(i);
				if (!GetDCOEnt().GetKey(i) || !o || now - Math.Max(o.m_fAt_ms, o.m_fBldAt_ms) > 10000.0)
					GetDCOEnt().RemoveElement(i);
			}
		}
		c = new DCO_BldEntCache();
		GetDCOEnt().Set(entity, c);
		return c;
	}

	protected static SCR_CoverManagerComponent s_Instance;

	static SCR_CoverManagerComponent GetInstance()
	{
		return s_Instance;
	}

	[Attribute("30", UIWidgets.EditBox, "Max booking duration before auto-release (s)")]
	protected float m_fMaxBookingDuration;

	[Attribute("1", UIWidgets.CheckBox, "Show debug visualization")]
	protected bool m_bDebugDraw;

	protected ref map<IEntity, vector> m_mBookings;
	protected ref map<IEntity, float>  m_mBookTimes;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		m_mBookings  = new map<IEntity, vector>();
		m_mBookTimes = new map<IEntity, float>();

		s_Instance = this;

		SetEventMask(owner, EntityEvent.FRAME);
	}

	override void OnDelete(IEntity owner)
	{
		if (s_Instance == this)
			s_Instance = null;

		super.OnDelete(owner);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		array<IEntity> toRelease = new array<IEntity>();

		foreach (IEntity agent, float bookedAt : m_mBookTimes)
		{
			float elapsed = (System.GetTickCount() - bookedAt) * 0.001;

			if (agent != null && elapsed >= m_fMaxBookingDuration)
				toRelease.Insert(agent);
		}

		foreach (IEntity agent : toRelease)
		{
			ForgetBooking(agent);
		}
	}

	bool RegisterPosition(IEntity agent, vector position)
	{
		if (!agent)
		{
			return false;
		}

		m_mBookings.Set(agent, position);
		m_mBookTimes.Set(agent, System.GetTickCount());

		return true;
	}

	static IEntity DCO_GetBuildingAt(IEntity entity, float traceDistance = 10.0)
	{
		if (!entity)
			return null;

		World world = GetGame().GetWorld();
		if (!world)
			return null;

		vector origin = entity.GetOrigin();
		float now = world.GetWorldTime();
		DCO_BldEntCache c;
		if (traceDistance == 10.0)
		{
			c = DCO_EntEntry(entity, now);
			if (c.m_fBldAt_ms >= 0 && now - c.m_fBldAt_ms < 1000.0 && vector.DistanceSq(c.m_vPos, origin) < 1.0)
			{
				DCO_Perf.Count("bld:hit");
				return c.m_Building;
			}
			DCO_Perf.Count("bld:miss");
		}

		IEntity result;
		IEntity above = DCO_TraceBuildingRoot(world, entity, origin, origin + Vector(0, traceDistance, 0));
		if (above)
		{
			IEntity below = DCO_TraceBuildingRoot(world, entity, origin + Vector(0, 0.3, 0), origin - Vector(0, 0.5, 0));
			if (below == above)
				result = above;
		}
		if (c)
		{
			if (vector.DistanceSq(c.m_vPos, origin) >= 1.0)
				c.m_fAt_ms = -1;
			c.m_vPos = origin;
			c.m_fBldAt_ms = now;
			c.m_Building = result;
		}
		return result;
	}

	static IEntity DCO_GetBuildingAtPos(vector pos, IEntity exclude, out string method)
	{
		method = "none";
		World world = GetGame().GetWorld();
		if (!world)
			return null;

		float now = world.GetWorldTime();
		int key = Math.Floor(pos[0] * 0.5) * 8192 + Math.Floor(pos[2] * 0.5);
		DCO_BldPosCache pc = GetDCOPos().Get(key);
		if (pc && now - pc.m_fAt_ms < 15000.0 && Math.AbsFloat(pc.m_fY - pos[1]) < 1.5 && (pc.m_sMethod == "none" || pc.m_Building))
		{
			DCO_Perf.Count("bldpos:hit");
			method = pc.m_sMethod;
			return pc.m_Building;
		}
		DCO_Perf.Count("bldpos:miss");
		if (GetDCOPos().Count() > 4096)
			GetDCOPos().Clear();
		pc = new DCO_BldPosCache();
		pc.m_fY = pos[1];
		pc.m_fAt_ms = now;
		GetDCOPos().Set(key, pc);
		pc.m_Building = DCO_FindBuildingAtPos(world, pos, exclude, method);
		pc.m_sMethod = method;
		return pc.m_Building;
	}

	protected static IEntity DCO_FindBuildingAtPos(World world, vector pos, IEntity exclude, out string method)
	{
		method = "none";
		IEntity above = DCO_TraceBuildingRoot(world, exclude, pos + Vector(0, 0.3, 0), pos + Vector(0, 10.0, 0));
		if (above)
		{
			method = "trace";
			return above;
		}

		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		if (!reg)
			return null;
		array<IEntity> buildings = {};
		reg.FindBuildingsAt(pos, 2.5, buildings);
		foreach (IEntity b : buildings)
		{
			if (!b)
				continue;
			vector mins, maxs;
			b.GetWorldBounds(mins, maxs);
			if (pos[0] < mins[0] - 1.0 || pos[0] > maxs[0] + 1.0 || pos[2] < mins[2] - 1.0 || pos[2] > maxs[2] + 1.0)
				continue;
			if (pos[1] < mins[1] - 2.0 || pos[1] > maxs[1] + 2.0)
				continue;
			method = "registry";
			return b;
		}
		return null;
	}

	protected static IEntity DCO_TraceBuildingRoot(World world, IEntity exclude, vector start, vector end)
	{
		float hitFraction = DCO_Util.TraceFrac(start, end, exclude, TraceFlags.ENTS, -1, "t:DCO_CoverManager");
		IEntity hitEnt = DCO_Util.LastTraceEnt();
		if (hitFraction >= 1.0)
			return null;

		if (!hitEnt)
			return null;

		IEntity root = hitEnt.GetRootParent();
		if (!root)
			return null;

		DCO_BuildingPositionComponent buildingComp = DCO_BuildingPositionComponent.Cast(root.FindComponent(DCO_BuildingPositionComponent));
		if (!buildingComp)
			return null;

		IEntity building = buildingComp.GetBuildingEntity();
		if (!building)
			return root;

		return building;
	}

	bool ReleasePosition(IEntity agent)
	{
		if (!agent)
		{
			return false;
		}

		if (!m_mBookings.Contains(agent))
		{
			return false;
		}

		ForgetBooking(agent);

		return true;
	}

	static bool IsEntityInsideBuilding(IEntity entity, float traceDistance = 10.0)
	{
		if (!entity)
		{
			return false;
		}

		World world = GetGame().GetWorld();

		if (!world)
		{
			return false;
		}

		vector origin   = entity.GetOrigin();
		float now = world.GetWorldTime();
		DCO_BldEntCache c;
		if (traceDistance == 10.0)
		{
			c = DCO_EntEntry(entity, now);
			if (c.m_fAt_ms >= 0 && now - c.m_fAt_ms < 1000.0 && vector.DistanceSq(c.m_vPos, origin) < 1.0)
			{
				DCO_Perf.Count("bld:hit");
				return c.m_bInside;
			}
			DCO_Perf.Count("bld:miss");
			if (vector.DistanceSq(c.m_vPos, origin) >= 1.0)
				c.m_fBldAt_ms = -1;
			c.m_vPos = origin;
			c.m_fAt_ms = now;
			c.m_bInside = DCO_InsideTrace(entity, origin, traceDistance);
			return c.m_bInside;
		}
		return DCO_InsideTrace(entity, origin, traceDistance);
	}

	protected static bool DCO_InsideTrace(IEntity entity, vector origin, float traceDistance)
	{
		vector traceEnd = origin + Vector(0, traceDistance, 0);

		float hitFraction = DCO_Util.TraceFrac(origin, traceEnd, entity, TraceFlags.ENTS, -1, "t:DCO_CoverManager");
		IEntity hitEnt = DCO_Util.LastTraceEnt();

		if (hitFraction >= 1.0)
		{
			return false;
		}

		if (!hitEnt)
		{
			return false;
		}

		IEntity root = hitEnt.GetRootParent();

		DCO_BuildingPositionComponent buildingComp =
			DCO_BuildingPositionComponent.Cast(root.FindComponent(DCO_BuildingPositionComponent));

		if (!buildingComp)
		{
			return false;
		}

		return true;
	}

	vector BookNearestFreePosition(IEntity agent, array<vector> candidatePositions)
	{
		if (!agent)
			return vector.Zero;

		if (!candidatePositions || candidatePositions.IsEmpty())
			return vector.Zero;

		vector agentOrigin = agent.GetOrigin();
		float  bestDist    = float.MAX;
		vector bestPos     = vector.Zero;
		bool   found       = false;

		foreach (vector pos : candidatePositions)
		{
			if (IsPositionBooked(pos))
				continue;

			float dist = vector.Distance(agentOrigin, pos);

			if (dist < bestDist)
			{
				bestDist = dist;
				bestPos  = pos;
				found    = true;
			}
		}

		if (!found)
		{
			return vector.Zero;
		}

		RegisterPosition(agent, bestPos);
		return bestPos;
	}

	bool IsPositionBooked(vector position)
	{
		foreach (IEntity agent, vector bookedPos : m_mBookings)
		{
			if (bookedPos == position)
				return true;
		}
		return false;
	}

	bool HasActiveBooking(IEntity agent)
	{
		if (!agent)
			return false;

		return m_mBookings.Contains(agent);
	}

	vector GetBookedPosition(IEntity agent)
	{
		if (!agent)
			return vector.Zero;

		if (!m_mBookings.Contains(agent))
			return vector.Zero;

		return m_mBookings.Get(agent);
	}

	int GetActiveBookingCount()
	{
		return m_mBookings.Count();
	}

	void ReleaseAll()
	{
		m_mBookings.Clear();
		m_mBookTimes.Clear();
	}

	float GetNearestBookedDistanceXZ(vector positionA)
	{
		if (m_mBookings.IsEmpty())
			return -1;

		float nearestDist = float.MAX;

		foreach (IEntity agent, vector bookedPos : m_mBookings)
		{
			float dx   = positionA[0] - bookedPos[0];
			float dz   = positionA[2] - bookedPos[2];
			float dist = Math.Sqrt(dx * dx + dz * dz);

			if (dist < nearestDist)
				nearestDist = dist;
		}

		return nearestDist;
	}

	protected void ForgetBooking(IEntity agent)
	{
		m_mBookings.Remove(agent);
		m_mBookTimes.Remove(agent);
	}
}
