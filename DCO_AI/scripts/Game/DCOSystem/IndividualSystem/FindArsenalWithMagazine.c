class SCR_AIFindArsenalWithMagazineWell : AITaskScripted
{
	// Inputs
	protected static const string PORT_POS = "Pos";
	protected static const string PORT_PREFAB_RESOURCE_NAME = "MagazineWellTypename";
	
	// Outputs
	protected static const string PORT_ARSENAL_ENTITY = "ArsenalEntity";
	
	[Attribute("0", UIWidgets.EditBox)]
	protected float m_fSearchRadius;
	
	// Used for query
	protected ref array<IEntity> m_aQueryFoundEntities = {};
	protected typename m_sQueryResourceName;
	
	//------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		vector searchPos;
		typename prefabResourceName;
		
		GetVariableIn(PORT_POS, searchPos);
		GetVariableIn(PORT_PREFAB_RESOURCE_NAME, prefabResourceName);
		
		if (searchPos == vector.Zero)
			return ENodeResult.FAIL;
		
		m_aQueryFoundEntities.Clear();
		m_sQueryResourceName = prefabResourceName;
		GetGame().GetWorld().QueryEntitiesBySphere(searchPos, m_fSearchRadius, QueryCallback);
		
		Print(m_aQueryFoundEntities.Count().ToString() + " < FOUND ARSENAL WITH MAGAZINE WELL > " + prefabResourceName.ToString());
		
		// Select closest one
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
		
		if (nearestEntity)
			Print(nearestEntity.Type().ToString() + " < FOUND NEAREST ARSENAL TO RESUPPLY > " + prefabResourceName.ToString());
		
		if (!nearestEntity)
			return ENodeResult.FAIL;
		
		SetVariableOut(PORT_ARSENAL_ENTITY, nearestEntity);
		return ENodeResult.SUCCESS;
	}
	
	bool QueryCallback(IEntity e)
	{
		SCR_ServicePointComponent comp = SCR_ServicePointComponent.Cast(e.FindComponent(SCR_ServicePointComponent));
		if (comp)
		{
			Print(e.Type().ToString() + " < FOUND SERVICE POINT TYPE > " + comp.GetType());
			if (comp.GetType() == SCR_EServicePointType.ARMORY)
				m_aQueryFoundEntities.Insert(e);
		}
		
		return true;
	}
	
	//------------------------------------------------------------
	override static bool VisibleInPalette() { return true; }
	
	override static string GetOnHoverDescription() { return "Finds nearest arsenal which has a given prefab in it."; }
	
	protected static ref TStringArray s_aVarsIn = { PORT_POS, PORT_PREFAB_RESOURCE_NAME };
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
	
	protected static ref TStringArray s_aVarsOut = { PORT_ARSENAL_ENTITY };
	override TStringArray GetVariablesOut() { return s_aVarsOut; }
}