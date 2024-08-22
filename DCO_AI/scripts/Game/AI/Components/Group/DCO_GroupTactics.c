enum DCO_GroupTactic{
	EVASIVE,
	DEFENSIVE,
	OFFENSIVE,
	AGGRESIVE
};

class DCO_GroupTacticComponentClass : ScriptComponentClass
{
	
}


class DCO_GroupTacticComponent : ScriptComponent
{
	[Attribute(defvalue: "2", uiwidget: UIWidgets.ComboBox, desc: "DCO Group Identifer", enums: ParamEnumArray.FromEnum(DCO_GroupTactic))]
	DCO_GroupTactic m_tac;
	protected SCR_AIGroup m_Group;
	
	static DCO_GroupTactic SetTactic(IEntity unit, DCO_GroupTactic groups)
	{
		if (!unit)
			return DCO_GroupTactic.OFFENSIVE;
		
		DCO_GroupTacticComponent comp = GetGroupTacticComponent(unit);
		
		if (!comp)
			return DCO_GroupTactic.OFFENSIVE;
		
		return comp.SetGroupTactic(groups);
	}

	static DCO_GroupTactic GetGroupTactic(IEntity unit)
	{		
		if (!unit)
			return DCO_GroupTactic.OFFENSIVE;
		
		DCO_GroupTacticComponent comp = GetGroupTacticComponent(unit);
		
		if (!comp)
			return DCO_GroupTactic.OFFENSIVE;
		
		return comp.GetGroupTactic();
	}
	
	static DCO_GroupTacticComponent GetGroupTacticComponent(IEntity unit)
	{
		return DCO_GroupTacticComponent.Cast(unit.FindComponent(DCO_GroupTacticComponent));
	}
	
	void setIdentifier(DCO_GroupTactic groups)
	{
		m_tac = groups;
	}
	
	protected DCO_GroupTactic SetGroupTactic(DCO_GroupTactic groups)
	{
		m_tac = groups;
		
		return groups;
	}
	
	protected DCO_GroupTactic GetGroupTactic()
	{
		return m_tac;
	}
	
	void DCO_GroupTacticComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_Group = SCR_AIGroup.Cast(ent);
	}
}