modded class SCR_AISettingsComponent : ScriptComponent
{
	[Attribute( defvalue: "1", uiwidget: UIWidgets.ComboBox, desc: "Rank System", params: "", enums: ParamEnumArray.FromEnum(DCO_CUSTOMRANK), category: "DCO GENERAL SETTING" )]
	DCO_CUSTOMRANK m_CusRank;
}