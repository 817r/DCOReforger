class DCO_FindIndoorPosition: AITaskScripted
{
	static const string PORT_CENTER_OF_SEARCH		= "OriginIn";
	static const string PORT_RADIUS					= "RadiusIn";
	static const string PORT_VECTOR_BOOL			= "Is Position Found";
	static const string PORT_VECTOR_POS				= "Position Found";
	
	[Attribute("0", UIWidgets.EditBox)]
	protected float m_fRadius;
	
	protected ref array<IEntity> m_aQueryFoundEntities = {};
	
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
		
		foreach (IEntity e : m_aQueryFoundEntities)
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
		vector fPos = buildPosComp.GetRandomPositionInside();
		
		SetVariableOut(PORT_VECTOR_POS, fPos);
		SetVariableOut(PORT_VECTOR_BOOL, isFoundEnt);
		return ENodeResult.SUCCESS;		
	}
	
	bool QueryCallback(IEntity e)
	{
		DCO_BuildingPositionComponent comp = DCO_BuildingPositionComponent.Cast(e.FindComponent(DCO_BuildingPositionComponent));
		
		if (comp && comp.isInitialized == true)
			m_aQueryFoundEntities.Insert(e);
		
		return true;
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