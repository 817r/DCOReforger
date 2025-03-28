[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_AIAutomatedGroupIdentificationEditor : SCR_BaseFloatValueHolderEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		bool groupIdf;
		
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		
		if (!editableEntity) return null;	
		
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return null;
		
		if (editableEntity.GetEntityType() == EEditableEntityType.GROUP)
		{
			SCR_AIGroup aiGroup = SCR_AIGroup.Cast(editableEntity.GetOwner()); 
			if (!aiGroup) return null;
			groupIdf = DCO_GroupIdentifierComponent.GetAutomated(aiGroup);
		} else return null;
		
		return SCR_BaseEditorAttributeVar.CreateInt(groupIdf);
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
			bool groupIdf = var.GetInt();
		
			DCO_GroupIdentifierComponent.SetAutomated(editableEntity.GetOwner(), groupIdf);
			
			groupIdf = DCO_GroupIdentifierComponent.GetAutomated(editableEntity.GetOwner());
		
		} else return;
	}
};
