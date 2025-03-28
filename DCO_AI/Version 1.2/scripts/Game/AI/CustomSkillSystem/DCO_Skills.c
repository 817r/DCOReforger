[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_AISkillsAttribute : SCR_BaseFloatValueHolderEditorAttribute
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
			cusRank = SCR_AICombatComponent.GetCharacterRank(editableEntity.GetOwner());
		}
		else if (editableEntity.GetEntityType() == EEditableEntityType.GROUP)
		{			
			SCR_AIGroup aiGroup = SCR_AIGroup.Cast(editableEntity.GetOwner()); 
			if (!aiGroup) return null;
			cusRank = SCR_AICombatComponent.GetCharacterRank(aiGroup.GetLeaderEntity());
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
		
		SCR_AICombatComponent.setSkill(editableEntity.GetOwner(), cusRank);
			
		cusRank = SCR_AICombatComponent.GetCharacterRank(editableEntity.GetOwner());	
	}
};
