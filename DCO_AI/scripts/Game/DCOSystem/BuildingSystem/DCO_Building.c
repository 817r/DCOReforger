class DCO_BuildingPositionComponentClass: ScriptComponentClass
{
}


// CREDITS TO GME TEAM TO ALLOW ME ABLE TO MODIFY THIS SCRIPT FROM GARRISON

// DO NOT USE THIS SCRIPT AS IT OFTEN BREAKS THE GAME ENTIRELY

// NEED HELP OPTIMIZING THIS



class DCO_BuildingPositionComponent: ScriptComponent
{	
	[Attribute("1.51", UIWidgets.Slider, "Eye Level", "0.1 2 0.1")]
	protected float EYE_POS;
	
	[Attribute("10", UIWidgets.Slider, "Line of Sight Range To Calculate", "1 50 1")]
	protected float LOS_TRACER_LENGTH;
	
	[Attribute("10", UIWidgets.Slider, "How many Position to be Created", "1 50 1")]
	protected float m_iMaxPos;
	
	[Attribute("", UIWidgets.ResourceAssignArray, "Line of sight exclude what kind of prefab")]
	protected ref array<ResourceName> LOS_TRACER_EXCLUDED_PREFABS = {};
	
	protected IEntity m_Building;
	
	protected vector m_vLocalMins, m_vLocalMaxs;
	protected vector m_vCurrentQueryPos;
	
	//protected SCR_DestructibleBuildingEntity m_DestroyableBuildingEntity;
	protected NavmeshWorldComponent m_pNavmesh;
	
	bool isInitialized = false;
	
	protected ref array<vector> m_aIndoorPos = new array<vector>;
	protected ref array<IEntity> m_aQueryFoundEntities = new array<IEntity>;
	
	static const int maxAttempt = 30;
	protected int attempt;
	
	#ifdef WORKBENCH
	// DEBUGER
	protected ref array<ref Shape> m_aDebugShapes = {};
	
	#endif
	override void OnPostInit(IEntity owner)
	{
		//super.OnPostInit(owner);
		//m_DestroyableBuildingEntity = SCR_DestructibleBuildingEntity.Cast(owner.FindComponent(SCR_DestructibleBuildingEntity));
		//SetEventMask(owner, EntityEvent.INIT);
		m_Building = owner;
	}
	
	IEntity GetBuildingEntity()
	{
		return m_Building;
	}
	
	override void EOnInit(IEntity owner)
	{
		/*
		m_Building = owner;
		owner.GetBounds(m_vLocalMins, m_vLocalMaxs);
		float myR = 0.5*(m_vLocalMaxs[0] - m_vLocalMins[0]);
		if (myR < 5) return;	
		GetGame().GetCallqueue().CallLater(RandomQueryStep, 200, false); 
		/*s_aGenerationQueue.Insert(this);
		if (!s_bIsProcessingQueue)
        {
			PrintString("Initalizing Building");
            s_bIsProcessingQueue = true;
            GetGame().GetCallqueue().CallLater(ProcessGlobalQueue, 100, false); 
        }*/
	}
	
	bool RandomQueryStep()
	{
		
		if (!m_pNavmesh)
			m_pNavmesh = GetGame().GetAIWorld().GetNavmeshWorldComponent("Soldiers");
		vector outPos, outDir;
		m_vCurrentQueryPos = GetRandomPosInBounds();
		DCO_BuildingPosCreation result = QueryPos(m_vCurrentQueryPos, outPos);
		switch (result)
		{
			case DCO_BuildingPosCreation.SUCCESS:
			{
				#ifdef WORKBENCH
				m_aDebugShapes.Insert(Shape.CreateSphere(COLOR_BLUE_A, ShapeFlags.NOZBUFFER | ShapeFlags.TRANSP, outPos, 0.2));
				#endif
				if (!IsPositionCloseToOther(outPos))
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
		
		attempt++;
		
		if (m_aIndoorPos.Count() >= m_iMaxPos || attempt > maxAttempt)
		{
			GetGame().GetCallqueue().Remove(RandomQueryStep);
			
			return true;
		} else
		{
			GetGame().GetCallqueue().CallLater(RandomQueryStep, 200, false); 
		}
		
		return false;
	}
	
	protected bool IsPositionCloseToOther(vector pos)
	{
		for (int i = m_aIndoorPos.Count() - 1; i >= 0; i--)
		{
			if (vector.Distance(pos, m_aIndoorPos[i]) <= 2.5)
				return true;
		}
		return false;
	}
	
	protected vector GetRandomPosInBounds()
	{
		vector localPos;
		for (int i = 0; i < 3; i++)
		{
			localPos[i] = Math.RandomFloatInclusive(m_vLocalMins[i], m_vLocalMaxs[i]);
		};
		
		float groundHeight = GetGame().GetWorld().GetSurfaceY(localPos[0], localPos[2]);
		if (localPos[1] < groundHeight)
			localPos[1] = groundHeight;
		
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
		
		if (GetGame().GetWorld().GetSurfaceY(outPos[0], outPos[2]) < 0)
			return DCO_BuildingPosCreation.FAIL;
		
		return DCO_BuildingPosCreation.SUCCESS;
	}
	
	bool QueryCallback(IEntity e)
	{
		SCR_CharacterDamageManagerComponent comp = SCR_CharacterDamageManagerComponent.Cast(e.FindComponent(SCR_CharacterDamageManagerComponent));
		
		if (comp && !comp.IsDestroyed())
			m_aQueryFoundEntities.Insert(e);
		
		return true;
	}
	
	protected bool IsPositionOccupied(vector pos)
	{
		GetGame().GetWorld().QueryEntitiesBySphere(pos, 1, QueryCallback);
		
		foreach (IEntity e : m_aQueryFoundEntities)
		{
			return true;
		}
		
		m_aQueryFoundEntities.Clear();
		return false;
	}
	
	vector GetRandomPositionInside()
	{
		vector pos = m_aIndoorPos[Math.RandomIntInclusive(0, m_aIndoorPos.Count() - 1)];
		while (IsPositionOccupied(pos))
		{
			pos = m_aIndoorPos[Math.RandomIntInclusive(0, m_aIndoorPos.Count() - 1)];
		}
		return pos;
	}
	
	vector GetPosClose(vector pos)
	{
		
	}
	
	// CENTRALIZED QUEUE
	
	// THIS IS NOT WORKING THAT WELL
	
	static ref array<DCO_BuildingPositionComponent> s_aGenerationQueue = {};
    static bool s_bIsProcessingQueue = false;
	
	static void ProcessGlobalQueue()
    {
		PrintString("s_aGenerationQueue : " + s_aGenerationQueue.Count().ToString());
        if (s_aGenerationQueue.IsEmpty())
        {
            s_bIsProcessingQueue = false;
            return;
        }

        DCO_BuildingPositionComponent currentBuilding = s_aGenerationQueue[0];

        if (!currentBuilding)
        {
            s_aGenerationQueue.Remove(0);
            GetGame().GetCallqueue().CallLater(ProcessGlobalQueue, 5, false);
            return;
        }

        bool isFinished = currentBuilding.RandomQueryStep();

        if (isFinished)
        {
            s_aGenerationQueue.RemoveOrdered(0);
        }
        
        GetGame().GetCallqueue().CallLater(ProcessGlobalQueue, 20, false);
    }	
}
