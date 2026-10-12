[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIIndividualPersonalityAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return null;

		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER))
			return null;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return null;

		SCR_AICombatComponent aiComponents = SCR_AICombatComponent.Cast(owner.FindComponent(SCR_AICombatComponent));
		if (!aiComponents)
			return null;

		SCR_AIUtilityComponent util = aiComponents.GetUtilityComponent();
		DCO_EAIPersonality p = DCO_AIConfigComponent.PersonalityOf(util);

		return SCR_BaseEditorAttributeVar.CreateInt(p);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return;

		SCR_AICombatComponent aiComponents = SCR_AICombatComponent.Cast(owner.FindComponent(SCR_AICombatComponent));
		if (!aiComponents)
			return;

		SCR_AIUtilityComponent util = aiComponents.GetUtilityComponent();
		DCO_AIConfigComponent conf = DCO_AIConfigComponent.Of(util);
		if (!conf)
			return;

		conf.SetPersonality(var.GetInt());
	}
}