[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_EnableAimImprovement: SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity || (editableEntity.GetEntityType() != EEditableEntityType.CHARACTER && editableEntity.GetEntityType() != EEditableEntityType.GROUP)) return null;
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return null;
		
		bool enable = true;
		
		//If character
		if (editableEntity.GetEntityType() == EEditableEntityType.CHARACTER)
		{
			enable = SCR_DCO_AIConfigComponent.GetEnableAimImprovement(editableEntity.GetOwner());
		}
		else
		{
			SCR_AIGroup aiGroup = SCR_AIGroup.Cast(editableEntity.GetOwner()); 
			if (!aiGroup) return null;
			enable = SCR_DCO_AIConfigComponent.GetEnableAimImprovement(aiGroup.GetLeaderEntity());
		}
		
		return SCR_BaseEditorAttributeVar.CreateBool(enable);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var) return;
		
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity.GetEntityType() == EEditableEntityType.CHARACTER && !editableEntity.GetEntityType() == EEditableEntityType.GROUP) return;
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return;
		
		bool enable = var.GetBool();
		
		if (editableEntity.GetEntityType() == EEditableEntityType.CHARACTER)
		{
			SCR_DCO_AIConfigComponent rankComponent = SCR_DCO_AIConfigComponent.GetDCOAIConfigComponent(editableEntity.GetOwner());
			if (!rankComponent) return;
			
			rankComponent.SetAimImprovement(enable);
			enable = rankComponent.EnableAimImprovement();
		}
	}
};