[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_MaxAimImprovementValue: SCR_ValidTypeBaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity || (editableEntity.GetEntityType() != EEditableEntityType.CHARACTER && editableEntity.GetEntityType() != EEditableEntityType.GROUP)) return null;
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return null;
		
		float enable;
		
		//If character
		if (editableEntity.GetEntityType() == EEditableEntityType.CHARACTER)
		{
			SCR_DCO_AIConfigComponent rankComponent = SCR_DCO_AIConfigComponent.GetCharacterRankComponent(editableEntity.GetOwner());
			enable = rankComponent.GetMaxAimImprovement();
		}
		else 
		{
			SCR_AIGroup aiGroup = SCR_AIGroup.Cast(editableEntity.GetOwner()); 
			if (!aiGroup) return null;
			SCR_DCO_AIConfigComponent rankComponent = SCR_DCO_AIConfigComponent.GetCharacterRankComponent(aiGroup.GetLeaderEntity());
			enable = rankComponent.GetMaxAimImprovement();
		}
		
		return SCR_BaseEditorAttributeVar.CreateFloat(enable);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var) return;
		
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity.GetEntityType() == EEditableEntityType.CHARACTER && !editableEntity.GetEntityType() == EEditableEntityType.GROUP) return;
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return;
		
		float enable = var.GetFloat();
		
		if (editableEntity.GetEntityType() == EEditableEntityType.CHARACTER)
		{
			SCR_DCO_AIConfigComponent rankComponent = SCR_DCO_AIConfigComponent.GetCharacterRankComponent(editableEntity.GetOwner());
			if (!rankComponent) return;
			
			rankComponent.SetMaxAimImprovement(enable);
			
			enable = SCR_DCO_AIConfigComponent.GetEnableAimImprovement(editableEntity.GetOwner());
		}
	}
};