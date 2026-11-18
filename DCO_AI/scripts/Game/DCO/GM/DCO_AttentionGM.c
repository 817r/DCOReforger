[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_ATT_OnAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "att.on"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_ATT_ScanSAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "att.scan_s"; }
	override protected float Def() { return 15.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_ATT_NearMAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "att.near_m"; }
	override protected float Def() { return 15.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_ATT_PredSAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "att.pred_s"; }
	override protected float Def() { return 8.0; }
}
