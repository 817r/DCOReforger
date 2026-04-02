class SCR_AITakeItemFromArsenalMagWell : AITaskScripted
{
	// Inputs
	protected static const string PORT_PREFAB_RESOURCE_NAME = "PrefabResourceName";
	protected static const string PORT_ARSENAL_ENTITY = "ArsenalEntity";
	
	// Outputs
	protected static const string PORT_ITEM_ENTITY = "ItemEntity";
	protected static const string PORT_COUNT_ITEMS_TAKEN = "CountItemsTaken";
	
	[Attribute("1", UIWidgets.EditBox, desc: "Max amount of items we will try to take")]
	protected int m_iMaxItemsToTake;
	
	ref map<string, string> m_mTypenameToResourceName;
	
	override void OnInit(AIAgent owner)
	{
		m_mTypenameToResourceName = new map<string, string>;
		m_mTypenameToResourceName.Insert("MagazineWellAK545","{E5912E45754CD421}Prefabs/Weapons/Magazines/Magazine_545x39_AK_30rnd_Tracer.et");
		m_mTypenameToResourceName.Insert("MagazineWellStanag556","{A9A385FE1F7BF4BD}Prefabs/Weapons/Magazines/Magazine_556x45_STANAG_30rnd_Tracer.et");
		m_mTypenameToResourceName.Insert("MagazineWellPKM","{BEEA49E27174B224}Prefabs/Weapons/Magazines/Box_762x54_PK_100rnd_Tracer.et");
		m_mTypenameToResourceName.Insert("MagazineWellM60","{AD8AB93729348C3E}Prefabs/Weapons/Magazines/Box_762x51_M60_100rnd_Tracer.et");
		m_mTypenameToResourceName.Insert("MagazineWellM14","{6D18CC33708EE712}Prefabs/Weapons/Magazines/Magazine_762x51_M14_20rnd_Base.et");
		m_mTypenameToResourceName.Insert("MagazineWellSVD","{9CCB46C6EE632C1A}Prefabs/Weapons/Magazines/Magazine_762x54_SVD_10rnd_Sniper.et");
		m_mTypenameToResourceName.Insert("MagazineWellVZ58_762","{FAFA0D71E75CEBE2}Prefabs/Weapons/Magazines/Vz58/Magazine_762x39_Vz58_30rnd_Tracer.et");
		m_mTypenameToResourceName.Insert("MagazineWellM9Beretta","{9C05543A503DB80E}Prefabs/Weapons/Magazines/Magazine_9x19_M9_15rnd_Ball.et");
		m_mTypenameToResourceName.Insert("MagazineWellMakarovPM","{8B853CDD11BA916E}Prefabs/Weapons/Magazines/Magazine_9x18_PM_8rnd_Ball.et");
		m_mTypenameToResourceName.Insert("MagazineWellM249","{06D722FC2666EB83}Prefabs/Weapons/Magazines/Box_556x45_M249_200rnd_4Ball_1Tracer.et");
		m_mTypenameToResourceName.Insert("MagazineWellRPG7","{32E12D322E107F1C}Prefabs/Weapons/Ammo/Ammo_Rocket_PG7VM.et");
	}
	
	//------------------------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		// Read inputs
		typename prefabResourceName;
		IEntity arsenalEntity;
		
		GetVariableIn(PORT_PREFAB_RESOURCE_NAME, prefabResourceName);
		GetVariableIn(PORT_ARSENAL_ENTITY, arsenalEntity);
		
		if (!arsenalEntity)
			return ENodeResult.FAIL;
		
		IEntity myEntity = owner.GetControlledEntity();
		
		// Verify arsenal, verify that arsenal has item
		SCR_ArsenalInventoryStorageManagerComponent arsenalComp =
			SCR_ArsenalInventoryStorageManagerComponent.Cast(arsenalEntity.FindComponent(SCR_ArsenalInventoryStorageManagerComponent));
		
		SCR_InventoryStorageManagerComponent myInvComp = SCR_InventoryStorageManagerComponent.Cast(myEntity.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!myInvComp)
			return ENodeResult.FAIL;
		
		if (!arsenalComp)
			return ENodeResult.FAIL;

		int nItemsTaken = 0;
		
		ResourceName resourceName;
		
		if (m_mTypenameToResourceName.Find(prefabResourceName.ToString(),resourceName))
		{
			if(myInvComp.TrySpawnPrefabToStorage(resourceName))
				nItemsTaken++;			
	
			Print(" FOUND AMMO MAGAZINE WELL : " + prefabResourceName + " ITEM TAKEN : " + nItemsTaken.ToString());
				
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
	
	protected ref TStringArray s_aVarsOut = { PORT_ITEM_ENTITY, PORT_COUNT_ITEMS_TAKEN };
	override TStringArray GetVariablesOut() { return s_aVarsOut; }
	
	override static bool VisibleInPalette() { return true; }
	
	override static string GetOnHoverDescription() { return "Takes item from arsenal and transfers it to own inventory. Returns success if at least one item was taken"; }
}