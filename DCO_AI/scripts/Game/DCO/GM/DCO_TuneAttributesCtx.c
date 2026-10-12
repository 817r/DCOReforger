[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CTX_OnAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ctx.on"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CTX_MgFloorAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ctx.mg_floor"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CTX_SupOnAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ctx.sup_on"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CTX_HideOnAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ctx.hide_on"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CTX_CqbGateAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ctx.cqb_gate"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_HEAL_UnseenGateAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "heal.unseen_gate"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_DODGE_CoverVsShooterAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "dodge.cover_vs_shooter"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_DODGE_NearmissGateAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "dodge.nearmiss_gate"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_DODGE_DirAssistAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "dodge.dir_assist"; }
	override protected bool Def() { return true; }
}
