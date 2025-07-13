[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GroupTacticsEditor: SCR_BasePresetsEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item); 
		if (!editableEntity) return null;
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return null;
		if (editableEntity.GetEntityType() != EEditableEntityType.GROUP) return null;
		
		int tactics;
		SCR_DCO_AIGroupConfigComponent comp = SCR_DCO_AIGroupConfigComponent.GetDCOGroupAIConfigComponent(editableEntity.GetOwner());
		tactics = comp.GetTactics();
		
		return SCR_BaseEditorAttributeVar.CreateInt(tactics);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var) return;
		
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return;
		if (editableEntity.GetEntityType() != EEditableEntityType.GROUP) return;
		
		int tactics = var.GetInt();

		SCR_DCO_AIGroupConfigComponent comp = SCR_DCO_AIGroupConfigComponent.GetDCOGroupAIConfigComponent(editableEntity.GetOwner());
		comp.SetTactics(tactics);
		
		tactics = comp.GetTactics();
	}
};