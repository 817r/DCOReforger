[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_AIMovement : SCR_BaseFloatValueHolderEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		bool movement;
		
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		
		if (!editableEntity) return null;	
		
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return null;
		
		if (editableEntity.GetEntityType() == EEditableEntityType.CHARACTER)
		{
			movement = SCRDCO_AIConfigComponent.GetEnableMovement(editableEntity.GetOwner());
		}
		else if (editableEntity.GetEntityType() == EEditableEntityType.GROUP)
		{
			return null;
		} else return null;
		
		return SCR_BaseEditorAttributeVar.CreateBool(movement);
	}
	
	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return;
		bool movement = var.GetBool();
		
		if (editableEntity.GetEntityType() == EEditableEntityType.CHARACTER)
		{
			SCRDCO_AIConfigComponent.SetEnableMovement(editableEntity.GetOwner(), movement);
			movement = SCRDCO_AIConfigComponent.GetEnableMovement(editableEntity.GetOwner());
		} else if (editableEntity.GetEntityType() == EEditableEntityType.GROUP)
		{
			return;
		
		} else return;
	}
};
