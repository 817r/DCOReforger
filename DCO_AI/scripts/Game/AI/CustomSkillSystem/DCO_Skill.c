enum DCO_CUSTOMRANK{
	RECRUIT,
	PRIVATE,
	PRIVATE_FIRST_CLASS,
	SPECIALIST
};

class DCO_SkillComponentClass : ScriptComponentClass
{
}


class DCO_SkillComponent : ScriptComponent
{
	[Attribute(defvalue: "1", uiwidget: UIWidgets.ComboBox, desc: "DCO Custom Ranks", enums: ParamEnumArray.FromEnum(DCO_CUSTOMRANK))]
	DCO_CUSTOMRANK m_ERank;
	protected IEntity m_Owner;
	
	static DCO_CUSTOMRANK setSkill(IEntity unit, DCO_CUSTOMRANK rank)
	{
		if (!unit)
			return DCO_CUSTOMRANK.RECRUIT;
		
		DCO_SkillComponent comp = GetCharacterSkillRankComponent(unit);
		
		if (!comp)
			return DCO_CUSTOMRANK.RECRUIT;
		
		return comp.SetCharacterRank(rank);
	}

	static DCO_CUSTOMRANK GetCharacterRank(IEntity unit)
	{
		if (!unit)
			return DCO_CUSTOMRANK.RECRUIT;
		
		DCO_SkillComponent comp = GetCharacterSkillRankComponent(unit);
		
		if (!comp)
			return DCO_CUSTOMRANK.RECRUIT;
		
		return comp.GetCharacterRank();
	}
	
	static DCO_SkillComponent GetCharacterSkillRankComponent(IEntity unit)
	{
		return DCO_SkillComponent.Cast(unit.FindComponent(DCO_SkillComponent));
	}
	
	void setAISkills(DCO_CUSTOMRANK rank)
	{
		m_ERank = rank;
	}
	
	protected DCO_CUSTOMRANK SetCharacterRank(DCO_CUSTOMRANK rank)
	{
		m_ERank = rank;
		
		return rank;
	}
	
	protected DCO_CUSTOMRANK GetCharacterRank()
	{
		return m_ERank;
	}
	
	void DCO_SkillComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_Owner = ent;
	}
}