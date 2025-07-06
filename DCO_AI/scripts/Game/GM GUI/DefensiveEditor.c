[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GroupDefendTacticsRadius: SCR_ValidTypeBaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item); 
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return null;
		if (editableEntity.GetEntityType() != EEditableEntityType.GROUP) return null;
		
		int tactics;
		float rad;
		SCR_DCO_AIGroupConfigComponent comp = SCR_DCO_AIGroupConfigComponent.GetDCOGroupAIConfigComponent(editableEntity.GetOwner());
		tactics = comp.GetTactics();
		
		if (tactics == DCO_GroupTactics.DEFENSIVE)
			rad = comp.GetDefendRadius();
		else
			return null;
		
		return SCR_BaseEditorAttributeVar.CreateFloat(rad);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var) return;
		
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER)) return;
		if (editableEntity.GetEntityType() != EEditableEntityType.GROUP) return;
		
		float tactics = var.GetFloat();

		SCR_DCO_AIGroupConfigComponent comp = SCR_DCO_AIGroupConfigComponent.GetDCOGroupAIConfigComponent(editableEntity.GetOwner());
		comp.SetDefendRadius(tactics);
		
		tactics = comp.GetDefendRadius();
	}
};