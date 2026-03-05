modded class SCR_AIWeaponHandling
{
	override static void StartMagazineSwitchCharacter(CharacterControllerComponent controller, BaseMagazineComponent newMagazineComp)
	{
		DCO_AIConfigComponent conf;
		SCR_InventoryStorageManagerComponent inv;
		SCR_AICombatComponent comb = SCR_AICombatComponent.Cast(controller.GetOwner().FindComponent(SCR_AICombatComponent));
		if (comb)
		{
			SCR_AIUtilityComponent utils = comb.GetUtilityComponent();
			if (utils)
			{
				conf = utils.m_DCOConfig;
				inv = utils.m_AIInfo.GetInventoryStorageManager();
			}
		}
		
		if (!conf || !conf.GetMagicMag())
		{
			controller.ReloadWeaponWith(newMagazineComp.GetOwner());
			return;
		}
			
		
		ResourceName resName = newMagazineComp.GetOwner().GetPrefabData().GetPrefab().GetResourceName();
		
		inv.TrySpawnPrefabToStorage(resName);
		controller.ReloadWeaponWith(newMagazineComp.GetOwner());
	}
}