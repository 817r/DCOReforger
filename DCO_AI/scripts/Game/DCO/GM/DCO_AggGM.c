[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AGG_PostFragAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "agg.postfrag"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AGG_UsageMapAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "agg.usage_map"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AGG_PursueAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "agg.pursue"; }
	override protected bool Def() { return true; }
}
