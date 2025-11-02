class DCO_AIIdentifierComponentClass: ScriptComponentClass
{
}

class DCO_AIIdentifierComponent: ScriptComponent
{
	[Attribute("0", UIWidgets.ComboBox, "AI identifies as", "", ParamEnumArray.FromEnum(DCO_EUnitType))]
	protected DCO_EUnitType unitType;
	
	[Attribute("0", UIWidgets.ComboBox, "AI skill level", "", ParamEnumArray.FromEnum(DCO_EAISkill))]
	protected DCO_EAISkill unitSkill;
	
	
	DCO_EUnitType GetUnitType()
	{
		return unitType;
	}
}