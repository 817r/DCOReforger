enum GMC_EGroupType
{
	INFANTRY    = 0,
	PATROL      = 1,
	HEAVY       = 2,
	SNIPER      = 3,
	CUSTOM      = 4
}

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