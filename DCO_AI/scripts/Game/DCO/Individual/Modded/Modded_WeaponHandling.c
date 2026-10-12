modded class SCR_AIWeaponHandling
{
	override static void StartMagazineSwitchCharacter(CharacterControllerComponent controller, BaseMagazineComponent newMagazineComp)
	{
		super.StartMagazineSwitchCharacter(controller, newMagazineComp);

		if (!Replication.IsServer())
			return;

		if (!controller || !newMagazineComp)
			return;

		IEntity ownerEntity = controller.GetOwner();
		if (!ownerEntity)
			return;

		bool magic;
		SCR_InventoryStorageManagerComponent inv;

		SCR_AICombatComponent comb = SCR_AICombatComponent.Cast(ownerEntity.FindComponent(SCR_AICombatComponent));
		if (comb)
		{
			SCR_AIUtilityComponent utils = comb.GetUtilityComponent();
			if (utils)
			{
				magic = DCO_AIConfigComponent.MagicMagOf(utils);

				if (utils.m_AIInfo)
					inv = utils.m_AIInfo.GetInventoryStorageManager();
			}
		}

		if (!magic || !inv)
			return;

		IEntity magEntity = newMagazineComp.GetOwner();
		if (!magEntity)
			return;

		EntityPrefabData magPrefabData = magEntity.GetPrefabData();
		if (!magPrefabData)
			return;

		ResourceName resName = magPrefabData.GetPrefab().GetResourceName();
		if (resName.IsEmpty())
			return;

		inv.TrySpawnPrefabToStorage(resName);
	}
}