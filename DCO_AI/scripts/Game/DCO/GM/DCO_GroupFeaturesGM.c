class DCO_GroupFeatureAttrUtil
{
	static SCR_AIGroup GetGroup(Managed item)
	{
		SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(item);
		if (!editableGroup || editableGroup.HasEntityState(EEditableEntityState.PLAYER))
			return null;
		return editableGroup.GetAIGroupComponent();
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GroupDetailAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_AIGroup grp = DCO_GroupFeatureAttrUtil.GetGroup(item);
		if (!grp)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(DCO_GroupFeatures.DetailOf(grp.DCO_GetFeatureOff()));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		SCR_AIGroup grp = DCO_GroupFeatureAttrUtil.GetGroup(item);
		int detail = var.GetInt();
		if (!grp || detail == DCO_EGroupDetail.CUSTOM)
			return;
		grp.DCO_SetFeatureOff(DCO_GroupFeatures.PresetOff(detail), "gm_detail");
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GroupFeaturesAttribute : SCR_BaseMultiSelectPresetsEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_AIGroup grp = DCO_GroupFeatureAttrUtil.GetGroup(item);
		if (!grp)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(DCO_GroupFeatures.AllMask() & ~grp.DCO_GetFeatureOff());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		SCR_AIGroup grp = DCO_GroupFeatureAttrUtil.GetGroup(item);
		if (grp)
			grp.DCO_SetFeatureOff(DCO_GroupFeatures.AllMask() & ~var.GetInt(), "gm_features");
	}
}
