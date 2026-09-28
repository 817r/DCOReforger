class DCO_AIResupplyConfigComponentClass: ScriptComponentClass
{
}

class DCO_AIResupplyConfigComponent: ScriptComponent
{
	ref array<ref ResupplyConfig> MagPrefab = {};

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