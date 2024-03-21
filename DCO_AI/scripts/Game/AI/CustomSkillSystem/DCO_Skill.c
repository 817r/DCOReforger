enum DCO_CUSTOMRANK{
	TRAITOR,
	RECRUIT,
	PRIVATE,
	PRIVATE_FIRST_CLASS,
	SPECIALIST,
	SERGEANT,
	STAFF_SERGEANT,
	SERGEANT_FIRST_CLASS,
	MASTER_SERGEANT,
	FIRST_SERGEANT,
	SERGEANT_MAJOR,
	COMMAND_SERGEANT_MAJOR,
	SECOND_LIEUTENANT,
	FIRST_LIEUTENANT,
	CAPTAIN,
	MAJOR,
	LIEUTENANT_COLONEL,
	COLONEL,
	GENERAL
};

class DCO_SkillComponentClass : ScriptComponentClass
{
	
}


class DCO_SkillComponent : ScriptComponent
{
	
	[Attribute(defvalue: "1", uiwidget: UIWidgets.ComboBox, desc: "DCO Custom Ranks", enums: ParamEnumArray.FromEnum(DCO_CUSTOMRANK))]
	protected DCO_CUSTOMRANK m_ERank;
	protected IEntity m_Owner;
	
	static DCO_SkillComponent GetCharacterSkillRankComponent(IEntity unit)
	{
		return DCO_SkillComponent.Cast(unit.FindComponent(DCO_SkillComponent));
	}
	
}