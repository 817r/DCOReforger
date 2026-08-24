class SCR_AIFindPositionWithMagazineWell : AITaskScripted
{
	// Inputs
	protected static const string PORT_POS = "Pos";
	protected static const string PORT_PREFAB_RESOURCE_NAME = "MagazineWell";
	
	// Outputs
	protected static const string PORT_ARSENAL_ENTITY = "ArsenalEntity";
	
	[Attribute("50", UIWidgets.EditBox)]
	protected float m_fSearchRadius;
	
	// Used for query
	protected ref array<IEntity> m_aQueryFoundEntities = {};
	protected typename prefabResourceName;
	
	//------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		vector searchPos;
		
		GetVariableIn(PORT_POS, searchPos);
		GetVariableIn(PORT_PREFAB_RESOURCE_NAME, prefabResourceName);
		
		if (searchPos == vector.Zero)
			return ENodeResult.FAIL;
		
		m_aQueryFoundEntities.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(searchPos, m_fSearchRadius, QueryCallback);
		
		// Select closest one
		IEntity nearestEntity = null;
		float smallestDistSq = float.MAX;
		
		//Print(m_aQueryFoundEntities.Count().ToString() + " < FOUND ENT WITH MAGAZINE WELL > " + prefabResourceName.ToString());
		
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
			//Print("NO NEAREST ENT TO RESUPPLY " + prefabResourceName.ToString());
			return ENodeResult.FAIL;
		}
		
		//Print(nearestEntity.Type().ToString() + " < FOUND NEAREST ENT TO RESUPPLY > " + prefabResourceName.ToString());
			
		
		SetVariableOut(PORT_ARSENAL_ENTITY, nearestEntity);
		return ENodeResult.SUCCESS;
	}
	
	bool QueryCallback(IEntity e)
	{
		array<IEntity> outItems = {};
		array<typename> components = {};
        components.Insert(MagazineComponent);
		SCR_InventoryStorageManagerComponent comp = SCR_InventoryStorageManagerComponent.Cast(e.FindComponent(SCR_InventoryStorageManagerComponent));
		SCR_CharacterPerceivableComponent perc = SCR_CharacterPerceivableComponent.Cast(e.FindComponent(SCR_CharacterPerceivableComponent));
		if (comp && perc.isDead)
		{
			comp.FindItemsWithComponents(outItems, components);
		}
		
		foreach (IEntity ent : outItems)
		{
			MagazineComponent magComp = MagazineComponent.Cast(ent.FindComponent(MagazineComponent));
			if (magComp)
			{
				if (!magComp.GetMagazineWell())
					continue;
				
				typename currMw = magComp.GetMagazineWell().Type();
				//Print(currMw.ToString() + " < FOUND THIS IN INVENTORY");
				if (prefabResourceName == currMw)
				{
					m_aQueryFoundEntities.Insert(e);
				}
			}
		}
		
		return true;
	}
	
	//------------------------------------------------------------
	override static bool VisibleInPalette() { return true; }
	
	override static string GetOnHoverDescription() { return "Finds nearest Entity which has a given prefab in it."; }
	
	protected static ref TStringArray s_aVarsIn = { PORT_POS, PORT_PREFAB_RESOURCE_NAME };
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
	
	protected static ref TStringArray s_aVarsOut = { PORT_ARSENAL_ENTITY };
	override TStringArray GetVariablesOut() { return s_aVarsOut; }
}