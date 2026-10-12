[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CTRL_GunExceptionsAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ctrl.gun_exceptions"; }
	override protected bool Def() { return false; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CQB_LostWindowAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cqb.lost_window_s"; }
	override protected float Def() { return 0; }
}
