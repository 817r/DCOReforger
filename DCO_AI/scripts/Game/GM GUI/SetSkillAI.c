[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SkillLevel: SCR_BasePresetsEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity || (editableEntity.GetEntityType() != EEditableEntityType.CHARACTER && editableEntity.GetEntityType() != EEditableEntityType.GROUP)) return null;
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return null;
		
		int enable;
		
		//If character
		if (editableEntity.GetEntityType() == EEditableEntityType.CHARACTER)
		{
			SCR_DCO_AIConfigComponent rankComponent = SCR_DCO_AIConfigComponent.GetDCOAIConfigComponent(editableEntity.GetOwner());
			enable = rankComponent.GetSkillLevel();
		}
		else 
		{
			SCR_AIGroup aiGroup = SCR_AIGroup.Cast(editableEntity.GetOwner()); 
			if (!aiGroup) return null;
			SCR_DCO_AIConfigComponent rankComponent = SCR_DCO_AIConfigComponent.GetDCOAIConfigComponent(aiGroup.GetLeaderEntity());
			enable = rankComponent.GetSkillLevel();
		}
		
		return SCR_BaseEditorAttributeVar.CreateInt(enable);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var) return;
		
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity.GetEntityType() == EEditableEntityType.CHARACTER && !editableEntity.GetEntityType() == EEditableEntityType.GROUP) return;
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return;
		
		float enable = var.GetInt();
		
		if (editableEntity.GetEntityType() == EEditableEntityType.CHARACTER)
		{
			SCR_DCO_AIConfigComponent rankComponent = SCR_DCO_AIConfigComponent.GetDCOAIConfigComponent(editableEntity.GetOwner());
			if (!rankComponent) return;
			
			rankComponent.SetSkillLevel(enable);
			
			enable = SCR_DCO_AIConfigComponent.GetEnableAimImprovement(editableEntity.GetOwner());
		}
	}
};