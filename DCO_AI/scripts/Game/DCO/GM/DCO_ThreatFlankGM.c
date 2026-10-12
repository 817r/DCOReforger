[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_THR_FlankFriendAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "thr.flank_rel"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_THR_FlankRouteAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "thr.flank_route"; }
	override protected bool Def() { return false; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_THR_FlankRetargetAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "thr.flank_retarget"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_THR_LowClusterAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "thr.low_cluster"; }
	override protected bool Def() { return false; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_LOGI_LowRelAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "logi.low_rel"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_LOGI_ExtractIneffAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "logi.extract_ineff"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_LOGI_CrossSrcAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "logi.cross_src"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_REARM_EndRelAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "rearm.end_rel"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PC_NightOnAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "pc.night_on"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PC_NightMinAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "pc.night_min"; }
	override protected float Def() { return 0.25; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_HEAR_ShotErrAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "hear.shot_err"; }
	override protected float Def() { return 1.0; }
}
