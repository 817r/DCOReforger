enum DCO_CUSTOMRANK{
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


class DCO_Skill : DCO_AIBase
{
	static EAISkill SetSkill(int indexAgent, int countAgent, IEntity entitiy, SCR_AICombatComponent combatComp)
	{
		EAISkill skill;
		
		EAISkill currSkill = combatComp.GetAISkill();
		
		SCR_ECharacterRank charRank = getRank(skill, indexAgent, countAgent);
		
		SCR_CharacterRankComponent charRankComp = SCR_CharacterRankComponent.Cast(entitiy.FindComponent(SCR_CharacterRankComponent));
		
		SCR_ECharacterRank currRank = charRankComp.GetCharacterRank(entitiy);
		
		
		
		return skill;
	}
	
	static SCR_ECharacterRank getRank(out EAISkill skill, int indexAgent, int countAgent)
	{
		SCR_ECharacterRank charRank = SCR_ECharacterRank.PRIVATE;
		
		return  charRank;
	}
}