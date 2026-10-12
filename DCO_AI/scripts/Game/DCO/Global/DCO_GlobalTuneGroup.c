modded class DCO_GlobalAIComponent
{
	override protected void SyncConfig()
	{
		super.SyncConfig();
		CfgCat("GroupTasks");
		DCO_TuneCfg("grp.bc_not_free", "tune_grp_bc_not_free", 1.0, 0.0, 1.0);
		DCO_TuneCfg("reinf.cap_substitute", "tune_reinf_cap_substitute", 0.0, 0.0, 1.0);
		DCO_TuneCfg("reinf.def_shortfall", "tune_reinf_def_shortfall", 1.0, 0.0, 1.0);
		DCO_TuneCfg("def.garrison_floor", "tune_def_garrison_floor", 1.0, 0.0, 1.0);
		DCO_TuneCfg("reinf.def_cooldown_s", "tune_reinf_def_cooldown_s", 240.0, 0.0, 600.0);
		DCO_TuneCfg("reinf.def_stock_reserve", "tune_reinf_def_stock_reserve", 0.35, 0.0, 1.0);
	}
}
