enum DCO_GroupTactic{
	EVASIVE,
	DEFENSIVE,
	AGGRESIVE
};

class DCO_GroupTacticComponentClass : ScriptComponentClass
{
	
}


class DCO_GroupTacticComponent : ScriptComponent
{
	[Attribute(defvalue: "1", uiwidget: UIWidgets.ComboBox, desc: "DCO Group Identifer", enums: ParamEnumArray.FromEnum(DCO_GroupTactic))]
	DCO_GroupTactic m_tac;
	DCO_GroupIdentifer m_idf;
	
	DCO_GroupIdentifierComponent m_IdentifierComponent;
	protected IEntity m_Owner;
	protected SCR_AIGroup m_Group;
	array<AIAgent> agents;
	
	void automaticIdentification()
	{
		m_Group = SCR_AIGroup.Cast(m_Group.FindComponent(SCR_AIGroup));		
		m_IdentifierComponent = DCO_GroupIdentifierComponent.Cast(m_Group.FindComponent(DCO_GroupIdentifierComponent));
		m_idf = m_IdentifierComponent.GetGroupIndentification(m_Owner);
		
		switch(m_idf)
		{
			case DCO_GroupIdentifer.PATROL:
			{
				SetGroupTactic(DCO_GroupTactic.EVASIVE);
				break;
			}
			case DCO_GroupIdentifer.INFANTRY:
			{
				SetGroupTactic(DCO_GroupTactic.AGGRESIVE); break;
			}
			case DCO_GroupIdentifer.MACHINEGUN_TEAM:
			{
				SetGroupTactic(DCO_GroupTactic.DEFENSIVE); break;
			}
			case DCO_GroupIdentifer.SNIPER_TEAM:
			{
				SetGroupTactic(DCO_GroupTactic.EVASIVE); break;
			}
			case DCO_GroupIdentifer.SAPPER:
			{
				SetGroupTactic(DCO_GroupTactic.EVASIVE); break;
			}
			case DCO_GroupIdentifer.INFANTRY_AT:
			{
				SetGroupTactic(DCO_GroupTactic.DEFENSIVE); break;
			}
		}
	}
	
	static DCO_GroupTactic SetTactic(IEntity unit, DCO_GroupTactic groups)
	{
		if (!unit)
			return DCO_GroupTactic.DEFENSIVE;
		
		DCO_GroupTacticComponent comp = GetGroupTacticComponent(unit);
		
		if (!comp)
			return DCO_GroupTactic.DEFENSIVE;
		
		return comp.SetGroupTactic(groups);
	}

	static DCO_GroupTactic GetGroupTactic(IEntity unit)
	{		
		if (!unit)
			return DCO_GroupTactic.DEFENSIVE;
		
		DCO_GroupTacticComponent comp = GetGroupTacticComponent(unit);
		
		if (!comp)
			return DCO_GroupTactic.DEFENSIVE;
		
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