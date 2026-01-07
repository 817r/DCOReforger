[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIIndividualPerceptionAttribute : SCR_ValidTypeBaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity) 
			return null;
		
		if (!IsValidEntityType(editableEntity.GetEntityType()))
			return null;
		
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER))
			return null;
		
		IEntity owner = editableEntity.GetOwner();
		if (!owner) 
			return null;
		
		ChimeraAIControlComponent aiComponents = ChimeraAIControlComponent.Cast(owner.FindComponent(ChimeraAIControlComponent));
		if (!aiComponents) 
			return null;
		
		DCO_AIConfigComponent aiConf = DCO_AIConfigComponent.Cast(aiComponents.GetAIAgent().FindComponent(DCO_AIConfigComponent));
		
		if (!aiConf)
			return null;		
		
		return SCR_BaseEditorAttributeVar.CreateFloat(aiConf.GetPerception());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity) 
			return;
		
		if (!IsValidEntityType(editableEntity.GetEntityType()))
			return;
		
		IEntity owner = editableEntity.GetOwner();
		if (!owner) 
			return;
		
		ChimeraAIControlComponent aiComponents = ChimeraAIControlComponent.Cast(owner.FindComponent(ChimeraAIControlComponent));
		if (!aiComponents) 
			return;
		
		DCO_AIConfigComponent aiConf = DCO_AIConfigComponent.Cast(aiComponents.GetAIAgent().FindComponent(DCO_AIConfigComponent));
		
		if (!aiConf)
			return;	
		
		aiConf.SetPerception(var.GetFloat());
	}
}