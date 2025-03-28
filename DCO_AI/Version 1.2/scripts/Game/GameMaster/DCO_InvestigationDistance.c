[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_AIInvestigationDistanceEditor : SCR_BaseValueListEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		float InfoShare;
		
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		
		
		if (!editableEntity) return null;	
		
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return null;
		
		if (editableEntity.GetEntityType() == EEditableEntityType.CHARACTER)
		{
			InfoShare = SCRDCO_AIConfigComponent.GetInvestigationDist(editableEntity.GetOwner());
		}
		else if (editableEntity.GetEntityType() == EEditableEntityType.GROUP)
		{
			SCR_AIGroup aiGroup = SCR_AIGroup.Cast(editableEntity.GetOwner()); 
			if (!aiGroup) return null;
			InfoShare = SCRDCO_AIConfigComponent.GetInvestigationDist(aiGroup.GetLeaderEntity());
		} else return null;
		
		return SCR_BaseEditorAttributeVar.CreateFloat(InfoShare);
	}
	
	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);

		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return;
		
		float InfoShare = var.GetFloat();
		
		SCRDCO_AIConfigComponent.SetInvestigationDist(editableEntity.GetOwner(), InfoShare);
			
		InfoShare = SCRDCO_AIConfigComponent.GetInvestigationDist(editableEntity.GetOwner());
	}
};
