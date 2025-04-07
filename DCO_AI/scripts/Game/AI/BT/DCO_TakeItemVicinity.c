class DCO_TakeEntityFromVicinity : AITaskScripted
{
	static const string PORT_ENTITY = "Entity To Pick";

	private IEntity m_OwnerEntity;
	private SCR_InventoryStorageManagerComponent m_Inventory;
	
	protected SCR_AICombatComponent m_CombatComponent;
	protected SCR_AIUtilityComponent m_UtilityComponent;
	protected SCR_InventoryStorageManagerComponent targetInventory;
	protected SCR_CharacterControllerComponent myCharController;
	private IEntity toPick;
	
	protected static ref TStringArray s_aVarsIn = {
		PORT_ENTITY
	};
	override TStringArray GetVariablesIn()
    {
        return s_aVarsIn;
    }	
	
	override void OnInit(AIAgent owner)
	{
		m_OwnerEntity = owner.GetControlledEntity();
		IEntity controlledEnt = owner.GetControlledEntity();
		m_CombatComponent = SCR_AICombatComponent.Cast(controlledEnt.FindComponent(SCR_AICombatComponent));
		m_UtilityComponent = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
		if (!m_OwnerEntity)
			Debug.Error("Owner must be a character!");
		m_Inventory = SCR_InventoryStorageManagerComponent.Cast(m_OwnerEntity.FindComponent(SCR_InventoryStorageManagerComponent));	
		myCharController = m_UtilityComponent.GetCharacterController();		
	}
	
	bool isEnough()
	{
		array<BaseInventoryStorageComponent> storages = {};
		targetInventory.GetStorages(storages);
		return true;
	}
	
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!m_Inventory)
			ENodeResult.FAIL;
		
		if (!GetVariableIn(PORT_ENTITY,toPick))
			ENodeResult.FAIL;
		
		targetInventory = SCR_InventoryStorageManagerComponent.Cast(toPick.FindComponent(SCR_InventoryStorageManagerComponent));
		array<BaseWeaponComponent> myWpCompt = {};
		array<typename> mymgCompt = {};
		array<IEntity> itemsss = {};
		m_CombatComponent.GetCurrentWeaponManager().GetWeaponsList(itemsss);
		
		foreach (IEntity m : itemsss)
		{
			BaseWeaponComponent mw = BaseWeaponComponent.Cast(m.FindComponent(BaseWeaponComponent));
			if (mw)
			{
				myWpCompt.Insert(mw);
				Print("MY WEAPON " + mw.GetUIInfo().GetName());
				BaseMagazineComponent mg = mw.GetCurrentMagazine();
				if (mg)
				{
					mymgCompt.Insert(mg.GetMagazineWell().Type());
					Print("MY MAGAZINE WELL " + mg.GetMagazineWell().ToString());
				}
			}
		}
		
		if (targetInventory)
		{
			array<IEntity> items = {};
			targetInventory.GetItems(items);
			
			foreach (IEntity item : items)
			{
				BaseWeaponComponent Weapcomp = BaseWeaponComponent.Cast(item.FindComponent(BaseWeaponComponent));
				BaseMagazineComponent Magcomp = BaseMagazineComponent.Cast(item.FindComponent(BaseMagazineComponent));

				if (Weapcomp)
				{					
					// Weapcomp.GetWeaponType() == EWeaponType.WT_MACHINEGUN
					if (Weapcomp.GetWeaponType() == EWeaponType.WT_RIFLE)
					{
						int take = 0;
						for (int i = 0; i < mymgCompt.Count(); i++)
						{
							if (mymgCompt[i] == Weapcomp.GetCurrentMagazine().GetMagazineWell().Type())
							{
								Print("SAME MAGAZINE WELL");
							} else
							{
								take++;
								Print("Take Iteration : " + take.ToString());
								Print("Take Fullfill : " + mymgCompt.Count().ToString());
							} 
						}
						
						bool takeWeapon = take == mymgCompt.Count();
						if (takeWeapon)
						{							
							InventoryItemComponent pInvComp = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
							InventoryStorageSlot parentSlot = pInvComp.GetParentSlot();
							bool removed = targetInventory.TryRemoveItemFromStorage(item,parentSlot.GetStorage());
								
							//bool take = targetInventory.TryRemoveItemFromInventory(item);
							//SCR_AIDebugVisualization.VisualizeMessage(m_UtilityComponent.m_OwnerEntity, Weapcomp.GetUIInfo().GetName(), EAIDebugCategory.COMBAT, 1.4, Color.White);
							if (removed)
							{
								myCharController.TryPlayItemGesture(EItemGesture.EItemGesturePickUp);
								m_Inventory.TryInsertItem(item);
								m_UtilityComponent.takeWeaponAlready = true;
								Print("Take Weapon");							
							} else
								Print("Take Weapon Fail");
						}
					}
					//Print(Weapcomp.GetUIInfo().GetName());
				} 
				
				if (Magcomp)
				{
					for (int i = 0; i < mymgCompt.Count(); i++)
					{
						if (Magcomp.GetMagazineWell().Type() == mymgCompt[i])
						{
							Print("TAKING MAGAZINE " + mymgCompt[i]);
							InventoryItemComponent pInvComp = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
							InventoryStorageSlot parentSlot = pInvComp.GetParentSlot();
							bool removed = targetInventory.TryRemoveItemFromStorage(item,parentSlot.GetStorage());
							bool muat = true;
							//SCR_AIDebugVisualization.VisualizeMessage(m_UtilityComponent.m_OwnerEntity, Magcomp.GetUIInfo().GetName(), EAIDebugCategory.COMBAT, 1.4, Color.White);
							if (removed)
							{
								myCharController.TryPlayItemGesture(EItemGesture.EItemGesturePickUp);
								muat = m_Inventory.TryInsertItem(item);	
								Print("Take Magazine");							
							} else
								Print("Take Magazine Fail");	
							
							if (!muat)
							{
								m_Inventory.ResupplyMagazines(2);
							}
						}
					}
					//Print(Magcomp.GetUIInfo().GetName());
				}
			}
			//SCR_AIDebugVisualization.VisualizeMessage(m_UtilityComponent.m_OwnerEntity, "No Suitable Item " + items.Count().ToString() ,EAIDebugCategory.INFO, 1.4, Color.White);
		}
		
		CharacterVicinityComponent vicinity = CharacterVicinityComponent.Cast(m_OwnerEntity.FindComponent(CharacterVicinityComponent));
		if (vicinity)
		{
			array<IEntity> items = {};
			vicinity.GetAvailableItems(items);

			foreach (IEntity item : items)
			{
				BaseWeaponComponent Weapcomp = BaseWeaponComponent.Cast(item.FindComponent(BaseWeaponComponent));
				BaseMagazineComponent Magcomp = BaseMagazineComponent.Cast(item.FindComponent(BaseMagazineComponent));
				
				if (Weapcomp)
				{					
					// Weapcomp.GetWeaponType() == EWeaponType.WT_MACHINEGUN
					if (Weapcomp.GetWeaponType() == EWeaponType.WT_RIFLE)
					{
						int take = 0;
						for (int i = 0; i < mymgCompt.Count(); i++)
						{
							if (mymgCompt[i] == Weapcomp.GetCurrentMagazine().GetMagazineWell().Type())
							{
								Print("SAME MAGAZINE WELL");
							} else
							{
								take++;
								Print("Take Iteration : " + take.ToString());
								Print("Take Fullfill : " + mymgCompt.Count().ToString());
							} 
						}
						
						bool takeWeapon = take == mymgCompt.Count();
						if (takeWeapon)
						{															
							//bool take = targetInventory.TryRemoveItemFromInventory(item);
							//SCR_AIDebugVisualization.VisualizeMessage(m_UtilityComponent.m_OwnerEntity, Weapcomp.GetUIInfo().GetName(), EAIDebugCategory.COMBAT, 1.4, Color.White);
							myCharController.TryPlayItemGesture(EItemGesture.EItemGesturePickUp);
							m_Inventory.TryInsertItem(item);
							m_UtilityComponent.takeWeaponAlready = true;
							Print("Take Weapon");							
						}
					}
					//Print(Weapcomp.GetUIInfo().GetName());
				} 
				
				if (Magcomp)
				{
					for (int i = 0; i < mymgCompt.Count(); i++)
					{
						if (Magcomp.GetMagazineWell().Type() == mymgCompt[i])
						{
							Print("TAKING MAGAZINE " + mymgCompt[i]);
							bool muat = true;
							//SCR_AIDebugVisualization.VisualizeMessage(m_UtilityComponent.m_OwnerEntity, Magcomp.GetUIInfo().GetName(), EAIDebugCategory.COMBAT, 1.4, Color.White);
							myCharController.TryPlayItemGesture(EItemGesture.EItemGesturePickUp);
							muat = m_Inventory.TryInsertItem(item);	
							Print("Take Magazine");							
							
							if (!muat)
							{
								m_Inventory.ResupplyMagazines(2);
							}
						}
					}
					//Print(Magcomp.GetUIInfo().GetName());
				}
			}
			//SCR_AIDebugVisualization.VisualizeMessage(m_UtilityComponent.m_OwnerEntity, "No Suitable Item " + items.Count().ToString() ,EAIDebugCategory.INFO, 1.4, Color.White);
		}

		return ENodeResult.SUCCESS;
	}
	
	protected static override bool VisibleInPalette()
	{
		return true;
	}
	
	protected static override string GetOnHoverDescription()
	{
		return "AI task that picks up all magazines of provided MagazineWell type in the vicinity of its inventory.";
	}
};