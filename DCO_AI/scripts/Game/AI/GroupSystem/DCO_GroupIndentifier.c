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
	protected SCR_AIGroup m_Group;
	array<AIAgent> agents;
	
	int sniper;
	int rifleman;
	int mg;
	int gl;
	int medic;
	int at;
	
	void automaticIdentification()
	{
		m_Group = SCR_AIGroup.Cast(m_Group.FindComponent(SCR_AIGroup));		
		m_Group.GetAgents(agents);
		int groupMem;
		groupMem = m_Group.GetAgentsCount();
		
		for(int i = 0; i < groupMem; i++)
		{
			AIAgent nowAgent = agents.Get(i);
			
			SCR_AIUtilityComponent util = SCR_AIUtilityComponent.Cast(nowAgent.FindComponent(SCR_AIUtilityComponent));
			
			if(!util) return;
						
			if (util.m_AIInfo.HasRole(EUnitRole.SNIPER)) sniper++;
			if (util.m_AIInfo.HasRole(EUnitRole.RIFLEMAN)) rifleman++;
			if (util.m_AIInfo.HasRole(EUnitRole.MACHINEGUNNER)) mg++;
			if (util.m_AIInfo.HasRole(EUnitRole.GRENADIER)) gl++;
			if (util.m_AIInfo.HasRole(EUnitRole.MEDIC)) medic++;
			if (util.m_AIInfo.HasRole(EUnitRole.AT_SPECIALIST)) at++;
		}
		
		if(sniper > 0 && groupMem < 3)
		{
			setIdentifier(DCO_GroupIdentifer.SNIPER_TEAM);
		} else if (mg > 0 && groupMem > 2)
		{
			setIdentifier(DCO_GroupIdentifer.MACHINEGUN_TEAM);
		} else if (at > 0 && groupMem > 2)
		{
			setIdentifier(DCO_GroupIdentifer.INFANTRY_AT);
		} else if (groupMem > 1 && groupMem < 5)
		{
			setIdentifier(DCO_GroupIdentifer.PATROL);
		} else
		{
			setIdentifier(DCO_GroupIdentifer.INFANTRY);
		}
	}
	
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