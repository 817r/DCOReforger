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

class DCO_Skill : DCO_AIBase
{
	DCO_CUSTOMRANK cusRank;
	
	static EAISkill SetSkill(int indexAgent, int countAgent, IEntity entitiy, SCR_AICombatComponent combatComp)
	{
		EAISkill skill;
		
		EAISkill currSkill = combatComp.GetAISkill();
		
		SCR_ECharacterRank charRank = getRank(skill, indexAgent, countAgent);
		
		SCR_CharacterRankComponent charRankComp = SCR_CharacterRankComponent.Cast(entitiy.FindComponent(SCR_CharacterRankComponent));
		
		SCR_ECharacterRank currRank = charRankComp.GetCharacterRank(entitiy);
		
		#ifdef Workbench
		
		string DebugText = String.Format("%1 > %2", typename.EnumToString(DCO_CUSTOMRANK, cusRank), typename.EnumToString(SCR_ECharacterRank, charRank));
		
		#endif
		
		return skill;
	}
	
	static SCR_ECharacterRank getRank(out EAISkill skill, int indexAgent, int countAgent)
	{
		SCR_ECharacterRank charRank = SCR_ECharacterRank.PRIVATE;
		
		return  charRank;
	}
	
	void setRank(int cusRanks)
	{
		cusRank = cusRanks;
	}
}