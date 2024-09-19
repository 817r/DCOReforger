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
	protected DCO_GroupIdentifer m_aIDF;
	protected IEntity m_Owner;
	SCR_AIGroup m_Group;
	bool isAutomated;
	
	// NUMBER MEMBER ROLES REFRENCES
	protected int rifleman, medic, LAT, MG, Sniper;

	static DCO_GroupIdentifer SetIdentification(IEntity unit, DCO_GroupIdentifer groups)
	{
		if (!unit)
			return DCO_GroupIdentifer.INFANTRY;
		
		DCO_GroupIdentifierComponent comp = GetGrouIdentifierComponent(unit);
		
		if (!comp)
			return DCO_GroupIdentifer.INFANTRY;
		
		return comp.SetGroupIdentification(groups);
	}
	
	static DCO_GroupIdentifer SetIdentificationAutomatic(IEntity unit, DCO_GroupIdentifer groups)
	{
		if (!unit)
			return DCO_GroupIdentifer.INFANTRY;
		
		DCO_GroupIdentifierComponent comp = GetGrouIdentifierComponent(unit);
		
		if (!comp)
			return DCO_GroupIdentifer.INFANTRY;
		
		return comp.SetGroupIdentificationAuto(groups);
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

	static DCO_GroupIdentifer SetAutomated(IEntity unit, bool tf)
	{
		if (!unit)
			return false;
		
		DCO_GroupIdentifierComponent comp = GetGrouIdentifierComponent(unit);
		
		if (!comp)
			return false;
		
		return comp.SetAutomated(tf);
	}
	
	

	static DCO_GroupIdentifer GetAutomated(IEntity unit)
	{		
		if (!unit)
			return false;
		
		DCO_GroupIdentifierComponent comp = GetGrouIdentifierComponent(unit);
		
		if (!comp)
			return false;
		
		return comp.GetAutomated();
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
	
	protected DCO_GroupIdentifer SetGroupIdentificationAuto(DCO_GroupIdentifer groups)
	{
		m_aIDF = groups;
		
		return groups;
	}
	
	protected DCO_GroupIdentifer GetGroupIdentification()
	{
		if (isAutomated) return m_aIDF;
		
		return m_Idf;
	}
	
	protected DCO_GroupIdentifer SetAutomated(bool tf)
	{
		isAutomated = tf;
		
		return tf;
	}
	
	protected DCO_GroupIdentifer GetAutomated()
	{
		return isAutomated;
	}
	
	bool isAutomatic()
	{
		return isAutomated;
	}
	
	void DCO_GroupIdentifierComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_Group = SCR_AIGroup.Cast(ent);
	}
}