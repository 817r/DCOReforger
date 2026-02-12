class DCO_BuildingPositionComponentClass: ScriptComponentClass
{
}


// CREDITS TO GME TEAM TO ALLOW ME ABLE TO MODIFY THIS SCRIPT FROM GARRISON

class DCO_BuildingPositionComponent: ScriptComponent
{
	[Attribute("", UIWidgets.Auto, "FrontDoor", "")]
	protected vector m_vFrontDoor;
	
	[Attribute("1.51", UIWidgets.Slider, "Eye Level", "0.1 2 0.1")]
	protected float EYE_POS;
	
	[Attribute("30", UIWidgets.Slider, "Line of Sight Range To Calculate", "1 50 1")]
	protected float LOS_TRACER_LENGTH;
	
	[Attribute("15", UIWidgets.Slider, "How many Position to be Created", "1 50 1")]
	protected float m_iMaxPos;
	
	[Attribute("", UIWidgets.ResourceAssignArray, "Line of sight exclude what kind of prefab")]
	protected ref array<ResourceName> LOS_TRACER_EXCLUDED_PREFABS = {};
	
	protected IEntity m_Building;
	
	protected vector m_vLocalMins, m_vLocalMaxs;
	protected vector m_vCurrentQueryPos;
	
	//protected SCR_DestructibleBuildingEntity m_DestroyableBuildingEntity;
	protected NavmeshWorldComponent m_pNavmesh;
	
	bool isInitialized = false;
	
	protected ref array<vector> m_aIndoorPos = {};
	
	// DEBUGER
	protected ref array<ref Shape> m_aDebugShapes = {};
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		//m_DestroyableBuildingEntity = SCR_DestructibleBuildingEntity.Cast(owner.FindComponent(SCR_DestructibleBuildingEntity));
		SetEventMask(owner, EntityEvent.INIT);
		GetGame().GetCallqueue().CallLater(Initialize, 2, false, owner);
	}
	
	void Initialize(IEntity owner)
	{
		m_Building = owner;
		owner.GetBounds(m_vLocalMins, m_vLocalMaxs);
		float myR = 0.5*(m_vLocalMaxs[0] - m_vLocalMins[0]);
		if (myR < 5) return;
		m_pNavmesh = GetGame().GetAIWorld().GetNavmeshWorldComponent("Soldiers");
		GetGame().GetCallqueue().CallLater(RandomQueryStep, 5, true);
	}
	
	protected void RandomQueryStep()
	{
		vector outPos, outDir;
		m_vCurrentQueryPos = GetRandomPosInBounds();
		DCO_BuildingPosCreation result = QueryPos(m_vCurrentQueryPos, outPos);
		switch (result)
		{
			case DCO_BuildingPosCreation.SUCCESS:
			{
				m_aDebugShapes.Insert(Shape.CreateSphere(COLOR_BLUE_A, ShapeFlags.NOZBUFFER | ShapeFlags.TRANSP, outPos, 0.2));
				m_aIndoorPos.Insert(outPos);
				m_vCurrentQueryPos = GetRandomPosInBounds();
				break;
			}
			
			case DCO_BuildingPosCreation.FAIL:
			{
				m_vCurrentQueryPos = GetRandomPosInBounds();
				break;
			}
		}
		
		if (m_aIndoorPos.Count() >= m_iMaxPos)
		{
			GetGame().GetCallqueue().Remove(RandomQueryStep);
			isInitialized = true;
		}
	}
	
	protected vector GetRandomPosInBounds()
	{
		vector localPos;
		for (int i = 0; i < 3; i++)
		{
			localPos[i] = Math.RandomFloatInclusive(m_vLocalMins[i], m_vLocalMaxs[i]);
		};
		
		return m_Building.CoordToParent(localPos);
	}
	
	protected DCO_BuildingPosCreation QueryPos(vector queryPos, out vector outPos)
	{
		if (m_pNavmesh.IsTileRequested(queryPos))
			return DCO_BuildingPosCreation.RUNNING;
		
		if (!m_pNavmesh.IsTileLoaded(queryPos))
		{
			m_pNavmesh.LoadTileIn(queryPos);
			return DCO_BuildingPosCreation.RUNNING;
		};

		if (!m_pNavmesh.GetReachablePoint(queryPos, 2, outPos))
			return DCO_BuildingPosCreation.FAIL;
		
		TraceParam params = new TraceParam();
		params.Flags = TraceFlags.ENTS;
		params.Start = outPos + EYE_POS * vector.Up;
		params.End = outPos - 5 * vector.Up;
		
		if (GetGame().GetWorld().TraceMove(params, null) >= 0.9)
			return DCO_BuildingPosCreation.FAIL;
		
		if (params.TraceEnt.GetRootParent() != m_Building.GetRootParent())
			return DCO_BuildingPosCreation.FAIL;

		if (params.TraceNorm[1] < 0.999)
		{
			params.End = outPos + 15 * vector.Up;
			
			if (GetGame().GetWorld().TraceMove(params, null) >= 0.999)
				return DCO_BuildingPosCreation.FAIL;
			
			if (params.TraceEnt.GetRootParent() != m_Building.GetRootParent())
				return DCO_BuildingPosCreation.FAIL;
		}
		
		vector.FromYaw(ComputeBestLoSYaw(outPos));
		return DCO_BuildingPosCreation.SUCCESS;
	}
	
	protected float ComputeBestLoSYaw(vector pos)
	{
		vector eyePos = pos + EYE_POS * vector.Up;
		
		array<float> yaws = {};
		yaws.Reserve(36);
		
		for (int y = 0; y < 360; y += 10)
		{
			yaws.Insert(y);
		}
		
		SCR_ArrayHelperT<float>.Shuffle(yaws);
		
		float bestDistance = 0;
		float bestYaw = 0;
		TraceParam params = new TraceParam();
		params.Flags = TraceFlags.ENTS;
		params.Start = eyePos;
		
		foreach (float yaw : yaws)
		{
			float distance = LOS_TRACER_LENGTH;
			params.End = eyePos + LOS_TRACER_LENGTH * vector.FromYaw(yaw);
			float percentage = GetGame().GetWorld().TraceMove(params, LoSTracerEntityCallback);
			
			if (percentage < 1)
			{
				distance *= percentage;
			}
			
			if (distance > bestDistance)
			{
				bestDistance = distance;
				bestYaw = yaw;
			}
		}
		
		return bestYaw;
	}
	
	protected bool LoSTracerEntityCallback(IEntity entity)
	{
		EntityPrefabData data = entity.GetPrefabData();
		if (!data)
			return false;
		
		BaseContainer container = data.GetPrefab();
		
		foreach (ResourceName res : LOS_TRACER_EXCLUDED_PREFABS)
		{
			if (SCR_BaseContainerTools.IsKindOf(container, res))
				return false;
		}
		
		return true;
	}
	
	vector GetRandomPositionInside()
	{
		return m_aIndoorPos[Math.RandomIntInclusive(0, m_aIndoorPos.Count() - 1)];
	}
}
