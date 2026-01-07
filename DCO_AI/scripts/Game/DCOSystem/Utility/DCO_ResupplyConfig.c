class DCO_AIResupplyConfigComponentClass: ScriptComponentClass
{
}

class DCO_AIResupplyConfigComponent: ScriptComponent
{
	ref array<ref ResupplyConfig> MagPrefab = {};
	
	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
	}
	
	
	void AddResupplyConfig(EWeaponType type, array<string> magPref)
	{
		ResupplyConfig res = new ResupplyConfig(type, magPref);		
		MagPrefab.Insert(res);
	}
	
	ResourceName GetRandomMagazinePrefab(EWeaponType type)
	{
		Print("Mag Pref Res Config : " + MagPrefab.Count());
		array<string> send = {};
		foreach(ResupplyConfig a : MagPrefab)
		{
			if (a.weaponType == type)
				send.InsertAll(a.MagazinePrefab);
		}
		
		return send.GetRandomElement();
	}
}

class ResupplyConfig : Managed
{
	int weaponType = 0;
	ref array<string> MagazinePrefab = {};
	
	void ResupplyConfig(int t, array<string> str)
	{
		weaponType = t;
		MagazinePrefab = str;
	}
}