class DCO_AIWeaponConfigComponentClass: ScriptComponentClass
{
}

class DCO_AIWeaponConfigComponent: ScriptComponent
{
	[Attribute(UIWidgets.Auto, "Magazine Prefab")]
	protected ref array<string> MagPrefab = {};
	
	EWeaponType weapType = EWeaponType.WT_NONE;
	
	SCR_CharacterControllerComponent m_CharacterController;
	DCO_AIResupplyConfigComponent resConf;
	BaseWeaponComponent weap;
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
		
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		weap = BaseWeaponComponent.Cast(owner.FindComponent(BaseWeaponComponent));
		
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
			resConf = DCO_AIResupplyConfigComponent.Cast(weap.GetOwner().GetParent().FindComponent(DCO_AIResupplyConfigComponent));
			resConf.AddResupplyConfig(weapType, MagPrefab);
			m_CharacterController = SCR_CharacterControllerComponent.Cast(owner.FindComponent(SCR_CharacterControllerComponent));
		
			if (m_CharacterController)
				SetEventMask(owner, EntityEvent.FRAME);
		}
	}
	
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		if (m_CharacterController.IsReloading() && weap.GetCurrentMagazine())
		{
			weap.GetCurrentMagazine().GetOwner().GetPrefabData().GetPrefab().GetResourceName();
			resConf.AddResupplyConfig(weapType, MagPrefab);
		}
			
		
	}
	
	
}