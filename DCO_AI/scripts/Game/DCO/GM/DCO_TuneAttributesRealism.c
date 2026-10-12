[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CQB_PrecheckAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "cqb.precheck"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TM_OpenRuleV2Attribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "tm.open_rule_v2"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CM_DirDecodeAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "cm.dir_decode"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TM_AtkCloseInAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "tm.atk_close_in"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_BC_ShakenAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "bc.shaken"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_BC_LadderSliderAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "bc.ladder_slider"; }
	override protected bool Def() { return true; }
}
