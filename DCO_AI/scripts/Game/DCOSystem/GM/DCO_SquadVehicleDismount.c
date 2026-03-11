[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AISquadVehicleDismountAttribute : SCR_ValidTypeBaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_EditableGroupComponent editableGroupComponent = SCR_EditableGroupComponent.Cast(item);
		if (!editableGroupComponent)
			return null;
		
		if (editableGroupComponent.HasEntityState(EEditableEntityState.PLAYER))
			return null;
		
		if (!IsValidEntityType(editableGroupComponent.GetEntityType()))
			return null;
		
		SCR_AIGroup owner = editableGroupComponent.GetAIGroupComponent();
		if (!owner) 
			return null;
		
		array<AIAgent> agents = {};
		owner.GetAgents(agents);
		
		DCO_AIConfigComponent aiConf = DCO_AIConfigComponent.Cast(owner.GetLeaderAgent().FindComponent(DCO_AIConfigComponent));
		
		if (!aiConf)
			return null;		
		
		return SCR_BaseEditorAttributeVar.CreateFloat(aiConf.GetDismountDistance());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		SCR_EditableGroupComponent editableGroupComponent = SCR_EditableGroupComponent.Cast(item);
		if (!editableGroupComponent)
			return;
		
		if (editableGroupComponent.HasEntityState(EEditableEntityState.PLAYER))
			return;
		
		if (!IsValidEntityType(editableGroupComponent.GetEntityType()))
			return;
		
		SCR_AIGroup owner = editableGroupComponent.GetAIGroupComponent();
		if (!owner) 
			return;
		
		array<AIAgent> agents = {};
		owner.GetAgents(agents);
		
		foreach(AIAgent a : agents)
		{
			DCO_AIConfigComponent aiConf = DCO_AIConfigComponent.Cast(a.FindComponent(DCO_AIConfigComponent));
			
			if (!aiConf)
				return;	
			
			aiConf.SetDismountDistance(var.GetFloat());			
		}
	}
}