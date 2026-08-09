class DCO_FindIndoorPosition: AITaskScripted
{
	static const string PORT_CENTER_OF_SEARCH		= "OriginIn";
	static const string PORT_RADIUS					= "RadiusIn";
	static const string PORT_VECTOR_BOOL			= "Is Position Found";
	static const string PORT_VECTOR_POS				= "Position Found";
	
	[Attribute("0", UIWidgets.EditBox)]
		protected float m_fRadius;
	
	protected IEntity m_Building;
	
	protected vector m_vLastSearchPos = vector.Zero;
	
	protected vector m_vLocalMins, m_vLocalMaxs;
	protected vector m_vCurrentQueryPos;
	
	protected ref array<IEntity> m_aQueryFoundBuilding = {};
	protected ref array<IEntity> m_aQueryFoundEntities = {};
	
	vector fPos;
	
	static const int maxAttempt = 1000;
	protected int attempt;

	protected float EYE_POS = 1.55;
	
	protected NavmeshWorldComponent m_pNavmesh;
	bool isFoundEnt = true;
	protected bool FoundPosition = false;
	
	#ifdef WORKBENCH
	// DEBUGER
	//protected ref array<ref Shape> m_aDebugShapes = {};
	
	#endif
	//------------------------------------------------------------------------------------------------
	static override bool VisibleInPalette() {return true;}
	
	//------------------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		vector searchPos;
		float searchRad;
		
		
		GetVariableIn(PORT_CENTER_OF_SEARCH, searchPos);
		if (!GetVariableIn(PORT_RADIUS, searchRad))
			searchRad = m_fRadius;
		
		if (searchPos == vector.Zero)
			return ENodeResult.FAIL;
		
		if (!m_Building || vector.DistanceSq(searchPos, m_vLastSearchPos) > 1.0)
		{
			m_vLastSearchPos = searchPos;
			FoundPosition    = false;
			attempt          = 0;
			
			m_aQueryFoundBuilding.Clear();
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
				m_Building = null;
				isFoundEnt = false;
				return ENodeResult.FAIL;
			}
			
			DCO_BuildingPositionComponent buildPosComp = DCO_BuildingPositionComponent.Cast(nearestEntity.FindComponent(DCO_BuildingPositionComponent));
			m_Building = buildPosComp.GetBuildingEntity();
			m_Building.GetBounds(m_vLocalMins, m_vLocalMaxs);
		}

		if (!FoundPosition)
		{
			if (attempt > maxAttempt)
			{
				isFoundEnt = false;
				return ENodeResult.FAIL;
			}
			
			RandomQueryStep();
			return ENodeResult.RUNNING;
		}
		
		if (FoundPosition)
		{
			SCR_CoverManagerComponent.GetInstance().RegisterPosition(owner.GetControlledEntity(), fPos);
			SetVariableOut(PORT_VECTOR_POS, fPos);
			SetVariableOut(PORT_VECTOR_BOOL, isFoundEnt);
			return ENodeResult.SUCCESS;			
		} 
		
		isFoundEnt = false;
		return ENodeResult.FAIL;	
		
	}
	
	bool QueryCallback(IEntity e)
	{
		DCO_BuildingPositionComponent comp = DCO_BuildingPositionComponent.Cast(e.FindComponent(DCO_BuildingPositionComponent));
		
		if (comp) // && comp.isInitialized == true
		{

			vector m_vLocalMinss, m_vLocalMaxss;
			comp.GetOwner().GetBounds(m_vLocalMinss, m_vLocalMaxss);
			float myR = 0.5*(m_vLocalMaxss[0] - m_vLocalMinss[0]);
			if (myR > 4) 
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
				if (!IsPositionOccupied(outPos))
				{
					#ifdef WORKBENCH
					//m_aDebugShapes.Insert(Shape.CreateSphere(COLOR_GREEN_A, ShapeFlags.NOZBUFFER | ShapeFlags.WIREFRAME, outPos, 0.2));
					#endif
					fPos = outPos;
					FoundPosition = true;
					isFoundEnt = true;
					return;
				} else
				{
					#ifdef WORKBENCH
					//m_aDebugShapes.Insert(Shape.CreateSphere(COLOR_BLUE_A, ShapeFlags.NOZBUFFER | ShapeFlags.TRANSP, outPos, 0.2));
					#endif
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
			params.End = outPos + 10 * vector.Up;
			
			if (params.TraceEnt.GetRootParent() != m_Building.GetRootParent())
				return DCO_BuildingPosCreation.FAIL;
			
			if (params.TraceEnt.GetRootParent() == m_Building.GetRootParent())
			{
				vector dir = params.End - params.Start;
				
				TraceParam parames = new TraceParam();
				parames.Flags = params.Flags;
				parames.Start = params.Start;
				parames.End = params.Start + (dir * -1);
				
				float traceResult2 = GetGame().GetWorld().TraceMove(parames, null);
				float traceResult1 = GetGame().GetWorld().TraceMove(params, null);
				
				vector hitPos1 = params.Start + (params.End - params.Start) * traceResult1;
				vector hitPos2 = parames.Start + (parames.End - parames.Start) * traceResult2;
				float corridorWidth = vector.Distance(hitPos1, hitPos2);
				
				if (corridorWidth < 1.5)
				    return DCO_BuildingPosCreation.FAIL;
			}
		}
		
		if (GetGame().GetWorld().GetSurfaceY(outPos[0], outPos[2]) < 0)
			return DCO_BuildingPosCreation.FAIL;
		
		return DCO_BuildingPosCreation.SUCCESS;
	}
	
	bool QueryCallbackC(IEntity e)
	{
		SCR_CharacterDamageManagerComponent comp = SCR_CharacterDamageManagerComponent.Cast(e.FindComponent(SCR_CharacterDamageManagerComponent));
		DoorComponent doorComp = DoorComponent.Cast(e.FindComponent(DoorComponent));
		
		if (comp && !comp.IsDestroyed())
			m_aQueryFoundEntities.Insert(e);
		
		if (doorComp)		
			m_aQueryFoundEntities.Insert(e);
		
		return true;
	}
	
	protected bool IsPositionOccupied(vector pos)
	{
		m_aQueryFoundEntities.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(pos, 3, QueryCallbackC);
		
		if (m_aQueryFoundEntities.Count() >= 1 || (SCR_CoverManagerComponent.GetInstance().GetNearestBookedDistanceXZ(pos) < 3 && SCR_CoverManagerComponent.GetInstance().GetNearestBookedDistanceXZ(pos) > 0))
			return true;
		
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