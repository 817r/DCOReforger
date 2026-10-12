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
	[Attribute("0", UIWidgets.ComboBox, "Type of this group.", "", ParamEnumArray.FromEnum(GMC_EGroupType))]
	GMC_EGroupType m_eGroupType;

	[Attribute("", UIWidgets.ResourceNamePicker, "Group prefab (required).", "et")]
	ResourceName m_sGroupPrefab;

	[Attribute("3.0", UIWidgets.EditBox, "Spacing between units when spawning (m).")]
	float m_fSpreadRadius;
}