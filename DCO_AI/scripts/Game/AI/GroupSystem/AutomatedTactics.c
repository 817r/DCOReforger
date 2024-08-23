[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_AIGetAutomated : SCR_BaseFloatValueHolderEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		int automated;
		
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		
		if (!editableEntity) return null;	
		
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return null;
		
		if (editableEntity.GetEntityType() == EEditableEntityType.GROUP)
		{
			SCR_AIGroup aiGroup = SCR_AIGroup.Cast(editableEntity.GetOwner()); 
			if (!aiGroup) return null;
			automated = DCO_GroupTacticComponent.getAutomated(aiGroup);
		} else return null;
		
		return SCR_BaseEditorAttributeVar.CreateInt(automated);
	}
	
	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return;
		
		if (editableEntity.GetEntityType() == EEditableEntityType.GROUP)
		{
			int automated = var.GetInt();
		
			DCO_GroupTacticComponent.setAutomated(editableEntity.GetOwner(), automated);
			
			automated = DCO_GroupTacticComponent.getAutomated(editableEntity.GetOwner());
		
		} else return;
	}
};
