class SCR_AITakeItemFromInventory_InventoryCallback : ScriptedInventoryOperationCallback
{
	protected IEntity m_ItemEntity;
	
	IEntity GetEntity() 
	{ 
		return m_ItemEntity;
	}

	override protected void OnComplete()
	{
		RplId itemId = GetItem();
		RplComponent rplComp = RplComponent.Cast(Replication.FindItem(itemId));
		if (rplComp)
			m_ItemEntity = rplComp.GetEntity();
	}
}

class SCR_AIRemoveItemItemFromInventory_InventoryCallback : ScriptedInventoryOperationCallback
{
	protected IEntity m_ItemEntity;
	
	IEntity GetEntity() 
	{ 
		return m_ItemEntity;
	}

	override protected void OnComplete()
	{
		RplId itemId = GetItem();
		RplComponent rplComp = RplComponent.Cast(Replication.FindItem(itemId));
		if (rplComp)
			m_ItemEntity = rplComp.GetEntity();
	}
}

class SCR_AITakeItemFromInventory : AITaskScripted
{
	// Inputs
	protected static const string PORT_PREFAB_RESOURCE_NAME = "PrefabResourceName";
	protected static const string PORT_ARSENAL_ENTITY = "ArsenalEntity";
	
	// Outputs
	protected static const string PORT_ITEM_ENTITY = "ItemEntity";
	protected static const string PORT_COUNT_ITEMS_TAKEN = "CountItemsTaken";
	
	[Attribute("1", UIWidgets.EditBox, desc: "Max amount of items we will try to take")]
	protected int m_iMaxItemsToTake;
	
	SCR_InventoryStorageManagerComponent arsenalComp;
	ResourceName prefabResourceName;
	
	//------------------------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		// Read inputs
		IEntity arsenalEntity;
		
		GetVariableIn(PORT_PREFAB_RESOURCE_NAME, prefabResourceName);
		GetVariableIn(PORT_ARSENAL_ENTITY, arsenalEntity);
		
		if (prefabResourceName.IsEmpty() || !arsenalEntity)
			return ENodeResult.FAIL;
		
		// Verify arsenal, verify that arsenal has item
		arsenalComp =
			SCR_InventoryStorageManagerComponent.Cast(arsenalEntity.FindComponent(SCR_InventoryStorageManagerComponent));
		
		if (!arsenalComp)
			return ENodeResult.FAIL;
		
		if (!arsenalComp.IsItemInStorage(arsenalComp, prefabResourceName, null))
			return ENodeResult.FAIL;
		
		// Verify own inventory
		IEntity myEntity = owner.GetControlledEntity();
		if (!myEntity)
			return ENodeResult.FAIL;
		InventoryStorageManagerComponent myInvComp = InventoryStorageManagerComponent.Cast(myEntity.FindComponent(InventoryStorageManagerComponent));
		if (!myInvComp)
			return ENodeResult.FAIL;
		
		int nItemsTaken = 0;
		IEntity lastTakenItem = null;
		SCR_AITakeItemFromInventory_InventoryCallback invCallback = new SCR_AITakeItemFromInventory_InventoryCallback();
		
		while (true)
		{
			bool success = myInvComp.TrySpawnPrefabToStorage(prefabResourceName, purpose: EStoragePurpose.PURPOSE_DEPOSIT, cb: invCallback);
			IEntity itemEntity = invCallback.GetEntity();
			bool removesuccess = IsItemDeleted();
			bool contin = success && removesuccess;
			
			if (!contin || !itemEntity)
				break;
			
			nItemsTaken++;
			Print("Magazine Taken : " + nItemsTaken.ToString());
			lastTakenItem = itemEntity;

			if (m_iMaxItemsToTake != -1 && nItemsTaken == m_iMaxItemsToTake)
				break;
		}

		if (nItemsTaken == 0)
			return ENodeResult.FAIL;
		
		SetVariableOut(PORT_ITEM_ENTITY, lastTakenItem);
		SetVariableOut(PORT_COUNT_ITEMS_TAKEN, nItemsTaken);
		
		return ENodeResult.SUCCESS;
	}
	
	bool IsItemDeleted()
	{
		array<typename> components = {};
		array<IEntity> foundItems = {};
        components.Insert(MagazineComponent);
       	
		for (int i = arsenalComp.FindItemsWithComponents(foundItems,components, EStoragePurpose.PURPOSE_DEPOSIT)-1; i> -1; i--)
		{
			MagazineComponent magComp = MagazineComponent.Cast(foundItems[i].FindComponent(MagazineComponent));
			if (magComp &&  magComp.GetOwner().GetPrefabData().GetPrefab().GetResourceName() == prefabResourceName)
			{
				foundItems.Remove(i);
			}
		}
		
		SCR_AIRemoveItemItemFromInventory_InventoryCallback invCB = new SCR_AIRemoveItemItemFromInventory_InventoryCallback();

		InventoryItemComponent pInvComp = InventoryItemComponent.Cast(foundItems[0].FindComponent(InventoryItemComponent));
		InventoryStorageSlot parentSlot = pInvComp.GetParentSlot();
		bool removed = arsenalComp.TryRemoveItemFromStorage(foundItems[0],parentSlot.GetStorage(), invCB);
		IEntity droped = invCB.GetEntity();
		SCR_EntityHelper.DeleteEntityAndChildren(droped);
		return true;
	}
	
	//------------------------------------------------------------------------------------------------------
	protected ref TStringArray s_aVarsIn = { PORT_ARSENAL_ENTITY, PORT_PREFAB_RESOURCE_NAME };
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
	
	protected ref TStringArray s_aVarsOut = { PORT_ITEM_ENTITY, PORT_COUNT_ITEMS_TAKEN };
	override TStringArray GetVariablesOut() { return s_aVarsOut; }
	
	override static bool VisibleInPalette() { return true; }
	
	override static string GetOnHoverDescription() { return "Takes item from Dead Entities and transfers it to own inventory. Returns success if at least one item was taken"; }
}