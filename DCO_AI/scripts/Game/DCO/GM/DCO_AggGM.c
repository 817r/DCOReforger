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

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AGG_ReachAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "agg.reach"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_SOP_EngageMAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "sop.engage_m"; }
	override protected float Def() { return 150.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_RT_PointBlankMAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "rt.pointblank_m"; }
	override protected float Def() { return 15.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_RT_MoveFireMAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "rt.move_fire_m"; }
	override protected float Def() { return 150.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_ROE_SelfDefenceMAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "roe.self_defence_m"; }
	override protected float Def() { return 150.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TM_PanicCloseMAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "tm.panic_close_m"; }
	override protected float Def() { return 15.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FRONT_FreeMAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "front.free_m"; }
	override protected float Def() { return 60.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FRONT_CloseMAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "front.close_m"; }
	override protected float Def() { return 50.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FF_NearAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ff.near"; }
	override protected bool Def() { return true; }
}
