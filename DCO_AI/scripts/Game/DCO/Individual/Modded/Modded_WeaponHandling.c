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
				if (!magic)
				{
					float chance = DCO_Stress.T(utils, "ammo.magic_chance", 0.0);
					if (chance > 0 && Math.RandomFloat(0.0, 100.0) < chance)
					{
						magic = true;
						DCO_Perf.Count("ammo:magic_chance_hit");
						DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(ownerEntity), string.Format("magic_mag unit=%1 chance=%2", DCO_Log.IdUnit(ownerEntity), Math.Round(chance)));
					}
					else if (chance > 0)
						DCO_Perf.Count("ammo:magic_chance_miss");
				}

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