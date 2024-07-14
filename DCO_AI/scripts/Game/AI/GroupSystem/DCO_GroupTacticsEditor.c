[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_AIGroupTacticEditor : SCR_BaseFloatValueHolderEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		int GroupTac;
		
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		
		if (!editableEntity) return null;	
		
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return null;
		
		if (editableEntity.GetEntityType() == EEditableEntityType.GROUP)
		{
			SCR_AIGroup aiGroup = SCR_AIGroup.Cast(editableEntity.GetOwner()); 
			if (!aiGroup) return null;
			GroupTac = DCO_GroupTacticComponent.GetGroupTactic(aiGroup);
		} else return null;
		
		return SCR_BaseEditorAttributeVar.CreateInt(GroupTac);
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
			int GroupTac = var.GetInt();
		
			DCO_GroupTacticComponent.SetTactic(editableEntity.GetOwner(), GroupTac);
			
			GroupTac = DCO_GroupTacticComponent.GetGroupTactic(editableEntity.GetOwner());
		
		} else return;
	}
};
