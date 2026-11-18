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
class DCO_T_MED_FullAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "med.full_on"; }
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

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_DODGE_CoverRunAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "dodge.cover_run"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_MEDIC_KeepFightingAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "medic.keep_fighting"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_MEDIC_DangerGateAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "medic.danger_gate"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TAC_MinAttackMenAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "tac.min_attack_men"; }
	override protected float Def() { return 3.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_RF_ReconGapSAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "reinf.recon_gap_s"; }
	override protected float Def() { return 900.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_POSTURE_RatioRangeMAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "posture.ratio_range_m"; }
	override protected float Def() { return 400.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_POSTURE_RatioFloorAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "posture.ratio_floor"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_SMK_RuleAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "smk.rule"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_SMK_GroupCdSAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "smk.group_cd_s"; }
	override protected float Def() { return 60.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_SMK_BcWaitSAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "smk.bc_wait_s"; }
	override protected float Def() { return 5.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_SMK_PrioBoostAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "smk.prio_boost"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_SMK_PosV2Attribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "smk.pos_v2"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_INV_CoverDirAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "inv.cover_dir"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_INV_DeselGraceSAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "inv.desel_grace_s"; }
	override protected float Def() { return 2.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_ATK_ObstructedSAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "atk.obstructed_s"; }
	override protected float Def() { return 2.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_ROE_UnderFireSAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "roe.under_fire_s"; }
	override protected float Def() { return 30.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_SOP_NoneFightAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "sop.none_fight"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_SOP_ObserveLatchSAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "sop.observe_latch_s"; }
	override protected float Def() { return 120.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_IDLE_JoinFightMAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "idle.join_fight_m"; }
	override protected float Def() { return 250.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AT_GroupAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "at.group"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_GL_GroupAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "gl.group"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_GL_SafeMAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "gl.safe_m"; }
	override protected float Def() { return 25.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_COV_TerrainV2Attribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "cov.terrain_v2"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_COV_SectorRetryAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "cov.sector_retry"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_COV_DangerSafeAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "cov.danger_safe"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OSC_FixBoundAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "osc.fix_bound"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FM_CoverHoldSAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fm.cover_hold_s"; }
	override protected float Def() { return 20.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_STRAG_SkipFightingAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "strag.skip_fighting"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_STRAG_RepeatSAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "strag.repeat_s"; }
	override protected float Def() { return 45.0; }
}
