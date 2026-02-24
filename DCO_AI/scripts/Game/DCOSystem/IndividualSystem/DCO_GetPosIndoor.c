class DCO_FindIndoorPosition: AITaskScripted
{
	static const string PORT_CENTER_OF_SEARCH		= "OriginIn";
	static const string PORT_RADIUS					= "RadiusIn";
	static const string PORT_VECTOR_BOOL			= "Is Position Found";
	static const string PORT_VECTOR_POS				= "Position Found";
	
	[Attribute("0", UIWidgets.EditBox)]
	protected float m_fRadius;
	
	protected IEntity m_Building;
	
	protected vector m_vLocalMins, m_vLocalMaxs;
	protected vector m_vCurrentQueryPos;
	
	protected ref array<IEntity> m_aQueryFoundBuilding = {};
	protected ref array<IEntity> m_aQueryFoundEntities = {};
	
	vector fPos;
	
	static const int maxAttempt = 30;
	protected int attempt;

	protected float EYE_POS = 1.55;
	
	protected float LOS_TRACER_LENGTH = 15;
	protected NavmeshWorldComponent m_pNavmesh;
	
	protected bool FoundPosition = false;
	
	protected ref array<ResourceName> LOS_TRACER_EXCLUDED_PREFABS = {
		"{F1793FE006FDF888}Prefabs/Structures/BuildingParts/Doors/Door_Base.et",
		"{2B188379767C8461}Prefabs/Structures/Core/DestructibleWindow_Base.et",
		"{86834A0D5920F32F}Prefabs/Structures/Core/DestructibleGlass_Base.et",
	};
	
	#ifdef WORKBENCH
	// DEBUGER
	protected ref array<ref Shape> m_aDebugShapes = {};
	
	#endif
	//------------------------------------------------------------------------------------------------
	static override bool VisibleInPalette() {return true;}
	
	//------------------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		vector searchPos;
		float searchRad;
		bool isFoundEnt = true;
		
		GetVariableIn(PORT_CENTER_OF_SEARCH, searchPos);
		if (!GetVariableIn(PORT_RADIUS, searchRad))
			searchRad = m_fRadius;
		
		if (searchPos == vector.Zero)
			return ENodeResult.FAIL;
		
		
		GetGame().GetWorld().QueryEntitiesBySphere(searchPos, searchRad, QueryCallback);
		
		IEntity nearestEntity = null;
		float smallestDistSq = float.MAX;
		
		foreach (IEntity e : m_aQueryFoundBuilding)
		{
			float distSq = vector.DistanceSq(e.GetOrigin(), searchPos);
			if (distSq < smallestDistSq)
			{
				nearestEntity = e;
				smallestDistSq = distSq;
			}
		}
		
		if (!nearestEntity)
		{
			isFoundEnt = false;
			return ENodeResult.FAIL;
		}
		
		DCO_BuildingPositionComponent buildPosComp = DCO_BuildingPositionComponent.Cast(nearestEntity.FindComponent(DCO_BuildingPositionComponent));
		m_Building = buildPosComp.GetBuildingEntity();
		m_Building.GetBounds(m_vLocalMins, m_vLocalMaxs);
		
		if (!FoundPosition)
		{
			RandomQueryStep();
			return ENodeResult.RUNNING;		
		} else
		{
			SetVariableOut(PORT_VECTOR_POS, fPos);
			SetVariableOut(PORT_VECTOR_BOOL, isFoundEnt);
			return ENodeResult.SUCCESS;			
		}
	}
	
	bool QueryCallback(IEntity e)
	{
		DCO_BuildingPositionComponent comp = DCO_BuildingPositionComponent.Cast(e.FindComponent(DCO_BuildingPositionComponent));
		
		if (comp) // && comp.isInitialized == true
		{
			protected vector m_vLocalMinss, m_vLocalMaxss;
			comp.GetOwner().GetBounds(m_vLocalMinss, m_vLocalMaxss);
			float myR = 0.5*(m_vLocalMaxss[0] - m_vLocalMinss[0]);
			if (myR > 5) 
				m_aQueryFoundBuilding.Insert(e);
		}
			
		
		return true;
	}
	
	void RandomQueryStep()
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
				if (!IsPositionOccupied(outPos))
				{
					fPos = outPos;
					FoundPosition = true;
					return;
				}
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
		
		if (attempt > maxAttempt)
		{
			return;
		}
		return;
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
		#ifdef WORKBENCH
		m_aDebugShapes.Insert(Shape.CreateSphere(COLOR_YELLOW_A, ShapeFlags.NOZBUFFER | ShapeFlags.TRANSP, localPos, 0.2));
		#endif
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
	
	bool QueryCallbackC(IEntity e)
	{
		m_aQueryFoundEntities.Clear();
		SCR_CharacterDamageManagerComponent comp = SCR_CharacterDamageManagerComponent.Cast(e.FindComponent(SCR_CharacterDamageManagerComponent));
		DoorComponent doorComp = DoorComponent.Cast(e.FindComponent(DoorComponent));
		
		if (comp && !comp.IsDestroyed())
			m_aQueryFoundEntities.Insert(e);
		else if (doorComp)		
			m_aQueryFoundEntities.Insert(e);
		
		return true;
	}
	
	protected bool IsPositionOccupied(vector pos)
	{
		GetGame().GetWorld().QueryEntitiesBySphere(pos, 2.5, QueryCallbackC);
		
		foreach (IEntity e : m_aQueryFoundEntities)
		{

			return true;
		}
		
		return false;
	}
		
	//------------------------------------------------------------------------------------------------
	protected static ref TStringArray s_aVarsOut = {
		PORT_VECTOR_POS,
		PORT_VECTOR_BOOL
	};
	override TStringArray GetVariablesOut()
	{
		return s_aVarsOut;
	}
	
	//------------------------------------------------------------------------------------------------
	protected static ref TStringArray s_aVarsIn = {
		PORT_CENTER_OF_SEARCH,
		PORT_RADIUS
	};
	override TStringArray GetVariablesIn()
	{
		return s_aVarsIn;
	}
	
	//------------------------------------------------------------------------------------------------
	static override string GetOnHoverDescription()
	{
		return "Find Indoor Position.";
	}
};