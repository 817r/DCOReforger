[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_AIForceStance : SCR_BaseFloatValueHolderEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		int stance;
		
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		
		if (!editableEntity) return null;		
		
		
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return null;
		if (editableEntity.GetEntityType() == EEditableEntityType.SYSTEM) return null;
		
		if (editableEntity.GetEntityType() == EEditableEntityType.CHARACTER)
		{
			stance = SCRDCO_AIConfigComponent.GetStances(editableEntity.GetOwner());
		}
		else if (editableEntity.GetEntityType() == EEditableEntityType.GROUP)
		{			
			SCR_AIGroup aiGroup = SCR_AIGroup.Cast(editableEntity.GetOwner()); 
			if (!aiGroup) return null;
			stance = SCRDCO_AIConfigComponent.GetStances(aiGroup.GetLeaderEntity());
		} else return null;
		
		return SCR_BaseEditorAttributeVar.CreateInt(stance);
	}
	
	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return;
		
		int stance = var.GetInt();
		//int rankApplication;
		
		SCRDCO_AIConfigComponent.SetStances(editableEntity.GetOwner(), stance);
			
		stance = SCRDCO_AIConfigComponent.GetStances(editableEntity.GetOwner());
	}
};
