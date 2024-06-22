[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_AIRankEditorAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		int cusRank;
		
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return null;

		cusRank = DCO_SkillComponent.GetCharacterRank(editableEntity.GetOwner());
		
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

		//DCO_SkillComponent rankComponent = DCO_SkillComponent.Cast(editableEntity.FindComponent(DCO_SkillComponent));
			//if (!rankComponent) return;
			
		DCO_SkillComponent.setSkill(editableEntity.GetOwner(), cusRank);
			
		cusRank = DCO_SkillComponent.GetCharacterRank(editableEntity.GetOwner());
		
	}
};
