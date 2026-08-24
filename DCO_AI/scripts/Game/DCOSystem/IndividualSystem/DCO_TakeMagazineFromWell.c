class SCR_AITakeItemFromInventoryWell : AITaskScripted
{
	// Inputs
	protected static const string PORT_PREFAB_RESOURCE_NAME = "MagazineWell";
	protected static const string PORT_ARSENAL_ENTITY = "ArsenalEntity";
	
	// Outputs
	protected static const string PORT_COUNT_ITEMS_TAKEN = "CountItemsTaken";
	
	[Attribute("1", UIWidgets.EditBox, desc: "Max amount of items we will try to take")]
	protected int m_iMaxItemsToTake;
	
	SCR_InventoryStorageManagerComponent arsenalComp;
	protected typename prefabResourceName;
	
	//------------------------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		// Read inputs
		IEntity arsenalEntity;
		
		GetVariableIn(PORT_PREFAB_RESOURCE_NAME, prefabResourceName);
		GetVariableIn(PORT_ARSENAL_ENTITY, arsenalEntity);
		
		if (!arsenalEntity)
			return ENodeResult.FAIL;
		
		// Verify arsenal, verify that arsenal has item
		arsenalComp =
			SCR_InventoryStorageManagerComponent.Cast(arsenalEntity.FindComponent(SCR_InventoryStorageManagerComponent));
		
		if (!arsenalComp)
			return ENodeResult.FAIL;
		
		array<IEntity> arsEnt = {};
		array<typename> components = {};
        components.Insert(MagazineComponent);
		
		arsenalComp.FindItemsWithComponents(arsEnt, components);
		
		//Print("FOUND ITEM : " + arsEnt.Count().ToString());
		
		if (arsEnt.Count() < 1)
			return ENodeResult.FAIL;
		
		// Verify own inventory
		IEntity myEntity = owner.GetControlledEntity();
		if (!myEntity)
			return ENodeResult.FAIL;
		SCR_InventoryStorageManagerComponent myInvComp = SCR_InventoryStorageManagerComponent.Cast(myEntity.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!myInvComp)
			return ENodeResult.FAIL;
		
		int nItemsTaken = 0;

		foreach(IEntity en : arsEnt)
		{
			MagazineComponent magComp = MagazineComponent.Cast(en.FindComponent(MagazineComponent));
			if (magComp)
			{					
				if (!magComp.GetMagazineWell())
					continue;
					
				if (prefabResourceName == magComp.GetMagazineWell().Type())
				{
					myInvComp.TrySpawnPrefabToStorage(en.GetPrefabData().GetPrefab().GetResourceName());
					arsenalComp.TryDeleteItem(en);
					nItemsTaken++;						
				}
			}			
			
			//Print(" FOUND AMMO MAGAZINE WELL : " + prefabResourceName + " ITEM TAKEN : " + nItemsTaken.ToString());
			if (m_iMaxItemsToTake != -1 && nItemsTaken == m_iMaxItemsToTake)
				return ENodeResult.SUCCESS;
		}

		if (nItemsTaken == 0)
			return ENodeResult.FAIL;
		
		SetVariableOut(PORT_COUNT_ITEMS_TAKEN, nItemsTaken);
		
		return ENodeResult.SUCCESS;
	}
	
	//------------------------------------------------------------------------------------------------------
	protected ref TStringArray s_aVarsIn = { PORT_ARSENAL_ENTITY, PORT_PREFAB_RESOURCE_NAME };
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
	
	protected ref TStringArray s_aVarsOut = { PORT_COUNT_ITEMS_TAKEN };
	override TStringArray GetVariablesOut() { return s_aVarsOut; }
	
	override static bool VisibleInPalette() { return true; }
	
	override static string GetOnHoverDescription() { return "Takes item from Dead Entities and transfers it to own inventory. Returns success if at least one item was taken"; }
}