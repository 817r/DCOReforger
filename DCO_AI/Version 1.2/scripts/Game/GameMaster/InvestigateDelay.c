[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_AIInvestigateDelayEditorAttribute : SCR_BaseValueListEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		float cusRank;
		
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		
		if (!editableEntity) return null;		
		
		
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return null;
		if (editableEntity.GetEntityType() == EEditableEntityType.SYSTEM) return null;
		
		if (editableEntity.GetEntityType() == EEditableEntityType.CHARACTER)
		{
			cusRank = SCRDCO_AIConfigComponent.GetInvestigateDelay(editableEntity.GetOwner());
		}
		else if (editableEntity.GetEntityType() == EEditableEntityType.GROUP)
		{			
			SCR_AIGroup aiGroup = SCR_AIGroup.Cast(editableEntity.GetOwner()); 
			if (!aiGroup) return null;
			cusRank = SCRDCO_AIConfigComponent.GetInvestigateDelay(aiGroup.GetLeaderEntity());
		} else return null;
		
		return SCR_BaseEditorAttributeVar.CreateFloat(cusRank);
	}
	
	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return;
		
		float cusRank = var.GetFloat();
		//int rankApplication;
		
		SCRDCO_AIConfigComponent.SetInvestigationDelay(editableEntity.GetOwner(), cusRank);
			
		cusRank = SCRDCO_AIConfigComponent.GetInvestigateDelay(editableEntity.GetOwner());
	}
};
