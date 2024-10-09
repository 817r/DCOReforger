[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_AIRankEditorAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		int cusRank;
		
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		
		if (!editableEntity) return null;		
		
		
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return null;
		if (editableEntity.GetEntityType() == EEditableEntityType.SYSTEM) return null;
		
		if (editableEntity.GetEntityType() == EEditableEntityType.CHARACTER)
		{
			cusRank = DCO_SkillComponent.GetCharacterRank(editableEntity.GetOwner());
		}
		else if (editableEntity.GetEntityType() == EEditableEntityType.GROUP)
		{			
			SCR_AIGroup aiGroup = SCR_AIGroup.Cast(editableEntity.GetOwner()); 
			if (!aiGroup) return null;
			cusRank = DCO_SkillComponent.GetCharacterRank(aiGroup.GetLeaderEntity());
		} else return null;
		
		return SCR_BaseEditorAttributeVar.CreateInt(cusRank);
	}
	
	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return;
		
		int cusRank = var.GetInt();
		//int rankApplication;
		
		DCO_SkillComponent.setSkill(editableEntity.GetOwner(), cusRank);
			
		cusRank = DCO_SkillComponent.GetCharacterRank(editableEntity.GetOwner());
		
		/*if (editableEntity.GetEntityType() == EEditableEntityType.CHARACTER)
		{
			DCO_SkillComponent.setSkill(editableEntity.GetOwner(), cusRank);
			
			cusRank = DCO_SkillComponent.GetCharacterRank(editableEntity.GetOwner());
		} else if (editableEntity.GetEntityType() == EEditableEntityType.GROUP)
		{
			SCR_AIGroup aiGroup = SCR_AIGroup.Cast(editableEntity.GetOwner());
			
			array<AIAgent> agents;
			aiGroup.GetAgents(agents);
			int groupNumber = agents.Count();
			AIAgent leaderAgent = aiGroup.GetLeaderAgent();
			
			for (int i = 0; i < groupNumber; i++)
			{
				if (agents.Get(i) == leaderAgent)
				{
					rankApplication = cusRank + 1;
				} else 
				{					
					rankApplication = cusRank;
				}
			}
			
			DCO_SkillComponent.setSkill(editableEntity.GetOwner(), rankApplication);
			
			cusRank = DCO_SkillComponent.GetCharacterRank(editableEntity.GetOwner());
		}*/
		

	}
};
