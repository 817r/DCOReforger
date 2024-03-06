modded class SCR_AISettingsComponent : ScriptComponent
{
	[Attribute( defvalue: "1", uiwidget: UIWidgets.ComboBox, desc: "Rank System", params: "", enums: ParamEnumArray.FromEnum(DCO_CUSTOMRANK), category: "DCO GENERAL SETTING" )]
	DCO_CUSTOMRANK m_CusRank;
	
	[Attribute( defvalue: "1", uiwidget: UIWidgets.ComboBox, desc: "Formation Offset", params: "", enums: ParamEnumArray.FromEnum(DCO_EFormationType), category: "DCO GENERAL SETTING" )]
	DCO_EFormationType m_EFormation;
}