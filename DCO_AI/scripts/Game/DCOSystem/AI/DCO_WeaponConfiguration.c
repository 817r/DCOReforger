class DCO_AIWeaponConfigComponentClass: ScriptComponentClass
{
}

class DCO_AIWeaponConfigComponent: ScriptComponent
{
	[Attribute(UIWidgets.Auto, "Magazine Prefab")]
	protected ref array<string> MagPrefab = {};
	
	protected EWeaponType weapType = EWeaponType.WT_NONE;
	
	protected SCR_CharacterControllerComponent m_CharacterController;
	protected BaseWeaponComponent weap;
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		//super.OnPostInit(owner);
		//SetEventMask(owner, EntityEvent.INIT);
		
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		CharacterWeaponManagerComponent charWpn = CharacterWeaponManagerComponent.Cast(owner.FindComponent(CharacterWeaponManagerComponent));
		weap = SCR_AIWeaponHandling.GetCurrentWeaponComponent(charWpn);
		
		if (!weap.GetWeaponType() == EWeaponType.WT_RIFLE || !weap.GetWeaponType() == EWeaponType.WT_HANDGUN || !weap.GetWeaponType() == EWeaponType.WT_MACHINEGUN || !weap.GetWeaponType() == EWeaponType.WT_ROCKETLAUNCHER
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
		}
			
		
	}
	
	
}