class DCO_AIIdentifierComponentClass: ScriptComponentClass
{
}

class DCO_AIIdentifierComponent: ScriptComponent
{
	[Attribute("0", UIWidgets.Flags, "", enums: ParamEnumArray.FromEnum(DCO_EAIIndividualRoles))]
	protected DCO_EAIIndividualRoles m_eUnitCapabilities;
	
	DCO_EAIIndividualRoles GetUnitType()
	{
		return m_eUnitCapabilities;
	}
}