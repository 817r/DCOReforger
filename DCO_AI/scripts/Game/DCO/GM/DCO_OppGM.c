[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OPP_ExposedAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "opp.exposed"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OPP_DistractedAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "opp.distracted"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OPP_PushAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "opp.suppressed_push"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OPP_ClusterAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "opp.cluster_scan"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OPP_PatientAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "opp.patient"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OPP_LootAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "opp.loot"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OPP_ReloadAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "opp.reload"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AGG_ProfileAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "agg.profile"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AGG_BoundV2Attribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "agg.bound_v2"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AGG_RetfireAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "agg.retfire"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TM_RetfireChanceAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "tm.retfire_chance"; }
	override protected float Def() { return 0.3; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_LEARN_IndAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "learn.ind_on"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_LEARN_PlyAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "learn.ply_on"; }
	override protected bool Def() { return false; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_LEARN_PlyMemoryAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "learn.ply_memory"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_LEARN_PlySuppressAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "learn.ply_suppress"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AGG_OpenFwdAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "agg.open_fwd"; }
	override protected bool Def() { return false; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_RetreatThrAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "cmd.retreat_thr"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_RADIO_ChatterAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "radio.squad_chatter"; }
	override protected bool Def() { return false; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_GREN_FfPredictAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "gren.ff_predict"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OPP_StalemateAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "opp.stalemate"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OPP_StalemateSAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "opp.stalemate_s"; }
	override protected float Def() { return 20.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_KZ_HoldSAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "kz.hold_s"; }
	override protected float Def() { return 120.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_DODGE_HoldSAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "dodge.hold_s"; }
	override protected float Def() { return 20.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AMMO_SuppModeAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ammo.supp_mode"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AMMO_SuppNormalMagsAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "ammo.supp_normal_mags"; }
	override protected float Def() { return 4.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AMMO_SuppMinMagsAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "ammo.supp_min_mags"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AT_MaxMAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "at.max_m"; }
	override protected float Def() { return 350.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PERF_LiteAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "perf.lite_on"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PERF_LiteMAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "perf.lite_m"; }
	override protected float Def() { return 1000.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PERF_DormantAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "perf.dormant"; }
	override protected bool Def() { return false; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_LOD_SmartPlayerMAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "lod.smart_player_m"; }
	override protected float Def() { return 1000.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_LOD_SmartEnemyMAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "lod.smart_enemy_m"; }
	override protected float Def() { return 800.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_LOD_SmartHoldSAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "lod.smart_hold_s"; }
	override protected float Def() { return 60.0; }
}
