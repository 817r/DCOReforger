[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_PhaseOnAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "obj.phase_on"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_K2Attribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "agg.k2"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_GarArriveAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "garrison.arrive_start"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_DefPrioAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "def.obj_priority"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_ReleaseExecAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "posture.release_exec"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_FixLosAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "tac.fix_los"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_AggProfileAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "agg.cmd_profile"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_EstBlendAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "intel.est_blend"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_RoleOnAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "obj.role_on"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_GarLeftoverAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "garrison.leftover_field"; }
	override protected bool Def() { return true; }
}
