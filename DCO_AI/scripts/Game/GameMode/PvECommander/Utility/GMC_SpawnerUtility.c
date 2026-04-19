// FILE: addons/GMC_MyMod/Scripts/Game/Respawner/GMC_RespawnGroupConfig.c

//! Jenis group yang tersedia untuk di-spawn
enum GMC_EGroupType
{
	INFANTRY    = 0,  //! Regu infanteri standar
	PATROL      = 1,  //! Patroli ringan
	HEAVY       = 2,  //! Pasukan berat / kendaraan
	SNIPER      = 3,  //! Tim sniper
	CUSTOM      = 4   //! Custom — isi prefab sendiri
}

//! Konfigurasi satu tipe group: berisi jenis dan daftar prefab unit-nya
[BaseContainerProps()]
class GMC_RespawnGroupConfig
{
	[Attribute("0", UIWidgets.ComboBox, "Tipe group ini", "", ParamEnumArray.FromEnum(GMC_EGroupType))]
	GMC_EGroupType m_eGroupType;

	[Attribute("", UIWidgets.ResourceNamePicker, "Prefab Group (wajib)", "et")]
	ResourceName m_sGroupPrefab;

	[Attribute("3.0", UIWidgets.EditBox, "Jarak antar unit saat spawn (meter)")]
	float m_fSpreadRadius;
}