[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GroupTacticsAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_AIGroupUtilityComponent groupUtil = GetGroupUtility(item);
		if (!groupUtil)
			return null;

		return SCR_BaseEditorAttributeVar.CreateInt(groupUtil.DCO_GetPostureOverride());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		SCR_AIGroupUtilityComponent groupUtil = GetGroupUtility(item);
		if (groupUtil)
			groupUtil.DCO_SetPostureOverride(var.GetInt());
	}

	protected SCR_AIGroupUtilityComponent GetGroupUtility(Managed item)
	{
		SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(item);
		if (!editableGroup || editableGroup.HasEntityState(EEditableEntityState.PLAYER))
			return null;

		SCR_AIGroup group = editableGroup.GetAIGroupComponent();
		if (!group)
			return null;

		return SCR_AIGroupUtilityComponent.Cast(group.FindComponent(SCR_AIGroupUtilityComponent));
	}
}
