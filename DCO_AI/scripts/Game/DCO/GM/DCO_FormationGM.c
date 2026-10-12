[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GroupFormAttributeBase : SCR_BaseFloatValueHolderEditorAttribute
{
	protected int Get(DCO_GroupUtilityComponent g) { return 0; }
	protected void Set(DCO_GroupUtilityComponent g, int v) {}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GroupUtilityComponent g = GetGroupUtil(item);
		if (!g)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(Get(g));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_GroupUtilityComponent g = GetGroupUtil(item);
		if (g)
			Set(g, var.GetInt());
	}

	protected DCO_GroupUtilityComponent GetGroupUtil(Managed item)
	{
		SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(item);
		if (!editableGroup || editableGroup.HasEntityState(EEditableEntityState.PLAYER))
			return null;
		SCR_AIGroup group = editableGroup.GetAIGroupComponent();
		if (!group)
			return null;
		return DCO_GroupUtilityComponent.Cast(group.FindComponent(DCO_GroupUtilityComponent));
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GroupFormationAttribute : DCO_GroupFormAttributeBase
{
	override protected int Get(DCO_GroupUtilityComponent g) { return g.DCO_GetFormShape(); }
	override protected void Set(DCO_GroupUtilityComponent g, int v) { g.DCO_SetFormOverride(v, g.DCO_GetFormSpacing(), "gm"); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GroupFormationSpacingAttribute : DCO_GroupFormAttributeBase
{
	override protected int Get(DCO_GroupUtilityComponent g) { return g.DCO_GetFormSpacing(); }
	override protected void Set(DCO_GroupUtilityComponent g, int v) { g.DCO_SetFormOverride(g.DCO_GetFormShape(), v, "gm"); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_TuneDropdownAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	protected string Key() { return ""; }
	protected int Def() { return 0; }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalFloatAttribute.GetGlobal(item);
		if (!cfg)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(Math.Round(cfg.DCO_TuneGet(Key(), Def())));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalFloatAttribute.GetGlobal(item);
		if (!cfg || !var)
			return;
		cfg.DCO_TuneSet(Key(), var.GetInt());
		DCO_TuneAttr.Log(Key(), var.GetInt(), playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FM_OpenShapeAttribute : DCO_TuneDropdownAttribute
{
	override protected string Key() { return "fm.open_shape"; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FM_ForestShapeAttribute : DCO_TuneDropdownAttribute
{
	override protected string Key() { return "fm.forest_shape"; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FM_UrbanShapeAttribute : DCO_TuneDropdownAttribute
{
	override protected string Key() { return "fm.urban_shape"; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FM_OpenSpAttribute : DCO_TuneDropdownAttribute
{
	override protected string Key() { return "fm.open_sp"; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FM_ForestSpAttribute : DCO_TuneDropdownAttribute
{
	override protected string Key() { return "fm.forest_sp"; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FM_UrbanSpAttribute : DCO_TuneDropdownAttribute
{
	override protected string Key() { return "fm.urban_sp"; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FM_NightTightAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "fm.night_tight"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FM_ContactLineAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "fm.contact_line"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FM_EchelonAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "fm.echelon"; }
	override protected bool Def() { return true; }
}
