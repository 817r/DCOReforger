[ComponentEditorProps(category: "AI/Cover", description: "Static-instance cover manager. Register vector positions, forget on release.")]
class SCR_CoverManagerComponentClass : ScriptComponentClass {}

class SCR_CoverManagerComponent : ScriptComponent
{
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

		IEntity above = DCO_TraceBuildingRoot(world, entity, origin, origin + Vector(0, traceDistance, 0));
		if (!above)
			return null;

		IEntity below = DCO_TraceBuildingRoot(world, entity, origin + Vector(0, 0.3, 0), origin - Vector(0, 0.5, 0));
		if (below != above)
			return null;

		return above;
	}

	protected static IEntity DCO_TraceBuildingRoot(World world, IEntity exclude, vector start, vector end)
	{
		TraceParam trace = new TraceParam();
		trace.Flags   = TraceFlags.ENTS;
		trace.Start   = start;
		trace.End     = end;
		trace.Exclude = exclude;

		DCO_Perf.Count("t:DCO_CoverManager");
		float hitFraction = world.TraceMove(trace, null);
		if (hitFraction >= 1.0)
			return null;

		if (!trace.TraceEnt)
			return null;

		IEntity root = trace.TraceEnt.GetRootParent();
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
		vector traceEnd = origin + Vector(0, traceDistance, 0);

		TraceParam trace  = new TraceParam();
		trace.Flags		  = TraceFlags.ENTS;
		trace.Start       = origin;
		trace.End         = traceEnd;
		trace.Exclude     = entity;

		DCO_Perf.Count("t:DCO_CoverManager");
		float hitFraction = world.TraceMove(trace, null);

		if (hitFraction >= 1.0)
		{
			return false;
		}

		if (!trace.TraceEnt)
		{
			return false;
		}

		IEntity root = trace.TraceEnt.GetRootParent();

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
