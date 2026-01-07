class DCO_AIWeaponConfigComponentClass: ScriptComponentClass
{
}

class DCO_AIWeaponConfigComponent: ScriptComponent
{
	[Attribute(UIWidgets.Auto, "Magazine Prefab")]
	protected ref array<string> MagPrefab = {};
	
	EWeaponType weapType = EWeaponType.WT_NONE;
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		BaseWeaponComponent weap = BaseWeaponComponent.Cast(owner.FindComponent(BaseWeaponComponent));
		
		if (!weap.GetWeaponType() == EWeaponType.WT_RIFLE || !weap.GetWeaponType() == EWeaponType.WT_HANDGUN || !weap.GetWeaponType() == EWeaponType.WT_MACHINEGUN
				|| !weap.GetWeaponType() == EWeaponType.WT_SNIPERRIFLE)
		return;

		weapType = weap.GetWeaponType();
		array<BaseMuzzleComponent> outMuzzles = {};
		weap.GetMuzzlesList(outMuzzles);
		
		foreach(BaseMuzzleComponent a : outMuzzles)
		{
			if (!a.IsDisposable())
			{
				MagPrefab.Insert(a.GetMagazine().GetOwner().GetPrefabData().GetPrefab().GetResourceName());
			}
		}
		
		if (weap.GetOwner().GetParent())
		{
			DCO_AIResupplyConfigComponent resConf = DCO_AIResupplyConfigComponent.Cast(weap.GetOwner().GetParent().FindComponent(DCO_AIResupplyConfigComponent));
			resConf.AddResupplyConfig(weapType, MagPrefab);
		}
		
	}
	
	
}