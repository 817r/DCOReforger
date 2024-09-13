enum DCO_GroupIdentifer{
	RECON,
	SNIPER_TEAM,
	INFANTRY,
	INFANTRY_AT,
	PATROL,
	MACHINEGUN_TEAM,
	SAPPER
};

class DCO_GroupIdentifierComponentClass : ScriptComponentClass
{
}


class DCO_GroupIdentifierComponent : ScriptComponent
{
	[Attribute(defvalue: "2", uiwidget: UIWidgets.ComboBox, desc: "DCO Group Identifer", enums: ParamEnumArray.FromEnum(DCO_GroupIdentifer))]
	DCO_GroupIdentifer m_Idf;
	protected IEntity m_Owner;
	SCR_AIGroup m_Group;
	
	static DCO_GroupIdentifer SetIdentification(IEntity unit, DCO_GroupIdentifer groups)
	{
		if (!unit)
			return DCO_GroupIdentifer.INFANTRY;
		
		DCO_GroupIdentifierComponent comp = GetGrouIdentifierComponent(unit);
		
		if (!comp)
			return DCO_GroupIdentifer.INFANTRY;
		
		return comp.SetGroupIdentification(groups);
	}

	static DCO_GroupIdentifer GetGroupIndentification(IEntity unit)
	{		
		if (!unit)
			return DCO_GroupIdentifer.INFANTRY;
		
		DCO_GroupIdentifierComponent comp = GetGrouIdentifierComponent(unit);
		
		if (!comp)
			return DCO_GroupIdentifer.INFANTRY;
		
		return comp.GetGroupIdentification();
	}
	
	static DCO_GroupIdentifierComponent GetGrouIdentifierComponent(IEntity unit)
	{
		return DCO_GroupIdentifierComponent.Cast(unit.FindComponent(DCO_GroupIdentifierComponent));
	}
	
	void setIdentifier(DCO_GroupIdentifer groups)
	{
		m_Idf = groups;
	}
	
	protected DCO_GroupIdentifer SetGroupIdentification(DCO_GroupIdentifer groups)
	{
		m_Idf = groups;
		
		return groups;
	}
	
	protected DCO_GroupIdentifer GetGroupIdentification()
	{
		return m_Idf;
	}
	
	void DCO_GroupIdentifierComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_Group = SCR_AIGroup.Cast(ent);
	}
}