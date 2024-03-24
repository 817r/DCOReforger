[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_AIRankEditorAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AIAgent agent;
		
		SCR_ECharacterRank rank;
		
		DCO_CUSTOMRANK cusRank;
		
		SCR_EditableEntityComponent editableEntityComponent = SCR_EditableEntityComponent.Cast(item);

		if (agent == null)
			return null;
		
		IEntity controlledEntity = agent.GetControlledEntity();
		
		if (controlledEntity)
			rank = SCR_CharacterRankComponent.GetCharacterRank(controlledEntity);
		
		return SCR_BaseEditorAttributeVar.CreateInt(cusRank);
	}
	
	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
		{
			AIAgent agent;
			DCO_CUSTOMRANK cusRank = var.GetInt();
			
			DCO_SkillComponent dcoSkillComponent;
			
			IEntity controlledEntity;
			
			SCR_ECharacterRank rank = var.GetInt();
			
			SCR_CharacterRankComponent characterRankComponent;
			
			SCR_EditableEntityComponent editableEntityComponent = SCR_EditableEntityComponent.Cast(item);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void SetSkill(AIAgent agent, IEntity controlledEntity, DCO_CUSTOMRANK cusRank)
	{
		EAISkill skill = EAISkill.REGULAR;
		
		switch (cusRank)
		{
			case DCO_CUSTOMRANK.TRAITOR:	
			{
				skill = EAISkill.REGULAR; 		
				cusRank = DCO_CUSTOMRANK.TRAITOR;
				break;
			}
			case DCO_CUSTOMRANK.RECRUIT:
			{
				skill = EAISkill.REGULAR; 		
				cusRank = DCO_CUSTOMRANK.RECRUIT;
				break;
			}
			case DCO_CUSTOMRANK.PRIVATE:
			{
				skill = EAISkill.REGULAR; 		
				cusRank = DCO_CUSTOMRANK.PRIVATE;
				break;
			}
			case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
			{
				skill = EAISkill.REGULAR; 		
				cusRank = DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS;
				break;
			}
		}

		
		DCO_AIInfoComponent aiInfoComponent = DCO_AIInfoComponent.Cast(agent.FindComponent(DCO_AIInfoComponent));
		
		SCR_AICombatComponent aiCombatComponent = SCR_AICombatComponent.Cast(controlledEntity.FindComponent(SCR_AICombatComponent));
		
		if (aiInfoComponent)
		{
			aiCombatComponent.SetAISkill(skill);
		}
	}
	
	void DCO_SetRanking(DCO_CUSTOMRANK customRank)
	{
		
		EAISkill skill = EAISkill.REGULAR;
		
		switch (customRank)
		{
			case DCO_CUSTOMRANK.TRAITOR:	
			{
				skill = EAISkill.REGULAR; 		
				customRank = DCO_CUSTOMRANK.TRAITOR;
				break;
			}
			case DCO_CUSTOMRANK.RECRUIT:
			{
				skill = EAISkill.REGULAR; 		
				customRank = DCO_CUSTOMRANK.RECRUIT;
				break;
			}
			case DCO_CUSTOMRANK.PRIVATE:
			{
				skill = EAISkill.REGULAR; 		
				customRank = DCO_CUSTOMRANK.PRIVATE;
				break;
			}
			case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
			{
				skill = EAISkill.REGULAR; 		
				customRank = DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS;
				break;
			}
		}
	}
};
