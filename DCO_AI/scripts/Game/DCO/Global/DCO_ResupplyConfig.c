class DCO_AIResupplyConfigComponentClass: ScriptComponentClass
{
}

class DCO_AIResupplyConfigComponent: ScriptComponent
{
	ref array<ref DCO_ResupplyConfig> MagPrefab = {};

	override void OnPostInit(IEntity owner)
	{
	}

	override void EOnInit(IEntity owner)
	{
	}

	void AddResupplyConfig(EWeaponType type, array<string> magPref)
	{
	}

	ResourceName GetRandomMagazinePrefab(EWeaponType type)
	{
	}
}

class DCO_ResupplyConfig : Managed
{
	int weaponType = 0;
	ref array<string> MagazinePrefab = {};

	void DCO_ResupplyConfig(int t, array<string> str)
	{
		weaponType = t;
		MagazinePrefab = str;
	}
}