modded class DCO_GlobalAIComponent
{
	override protected void SyncConfig()
	{
		super.SyncConfig();
		CfgCat("TuneTactics");
		DCO_TuneSyncTactics();
		CfgCat("TuneDefense");
		DCO_TuneSyncDefense();
		CfgFlush();
	}

	protected void DCO_TuneSyncTactics()
	{
		DCO_TuneCfg("tac.mett_on", "tune_tac_mett_on", 0.0, 0.0, 1.0);
		DCO_TuneCfg("tac.mett_band", "tune_tac_mett_band", 0.85, 0.3, 1.0);
		DCO_TuneCfg("tac.mett_intel_lo", "tune_tac_mett_intel_lo", 0.25, 0.0, 1.0);
		DCO_TuneCfg("tac.mett_intel_hi", "tune_tac_mett_intel_hi", 0.55, 0.0, 1.0);
		DCO_TuneCfg("tac.mett_recent_s", "tune_tac_mett_recent_s", 600.0, 0.0, 3600.0);
		DCO_TuneCfg("tac.mett_recent_mul", "tune_tac_mett_recent_mul", 0.7, 0.1, 1.0);
		DCO_TuneCfg("posture.release_exec", "tune_posture_release_exec", 1.0, 0.0, 1.0);
		DCO_TuneCfg("agg.cmd_profile", "tune_agg_cmd_profile", 1.0, 0.0, 1.0);
		DCO_TuneCfg("cmd.aggr_tempo", "tune_cmd_aggr_tempo", 0.0, 0.0, 1.0);
		DCO_TuneCfg("cmd.tempo_min", "tune_cmd_tempo_min", 0.75, 0.25, 1.0);
		DCO_TuneCfg("cmd.tempo_max", "tune_cmd_tempo_max", 2.0, 1.0, 4.0);
		DCO_TuneCfg("tac.push_keep_route", "tune_tac_push_keep_route", 0.0, 0.0, 1.0);
		DCO_TuneCfg("wp.attack_arrive", "tune_wp_attack_arrive", 0.0, 0.0, 1.0);
		DCO_TuneCfg("bc.no_retask", "tune_bc_no_retask", 0.0, 0.0, 1.0);
		DCO_TuneCfg("wp.attack_arrive_s", "tune_wp_attack_arrive_s", 8.0, 2.0, 60.0);
		DCO_TuneCfg("wp.arrive_any", "tune_wp_arrive_any", 0.0, 0.0, 1.0);
		DCO_TuneCfg("tac.pursuit_track", "tune_tac_pursuit_track", 0.0, 0.0, 1.0);
		DCO_TuneCfg("tac.pursuit_max_s", "tune_tac_pursuit_max_s", 300.0, 60.0, 900.0);
		DCO_TuneCfg("tac.pursuit_leash_m", "tune_tac_pursuit_leash_m", 600.0, 100.0, 1500.0);
		DCO_TuneCfg("tac.pursuit_retarget_s", "tune_tac_pursuit_retarget_s", 30.0, 10.0, 120.0);
		DCO_TuneCfg("fatigue.contact_acc", "tune_fatigue_contact_acc", 0.0, 0.0, 1.0);
		DCO_TuneCfg("fatigue.contact_m", "tune_fatigue_contact_m", 400.0, 50.0, 1000.0);
		DCO_TuneCfg("agg.k2", "tune_agg_k2", 1.0, 0.0, 1.0);
		DCO_TuneCfg("agg.k2_p0max", "tune_agg_k2_p0max", 0.9, 0.5, 1.0);
		DCO_TuneCfg("agg.k2_tempo_min", "tune_agg_k2_tempo_min", 0.7, 0.3, 1.0);
		DCO_TuneCfg("agg.k2_tempo_max", "tune_agg_k2_tempo_max", 2.0, 1.0, 3.0);
		DCO_TuneCfg("agg.k2_staging_floor_s", "tune_agg_k2_staging_floor_s", 30.0, 0.0, 300.0);
		DCO_TuneCfg("agg.k2_recon_floor_s", "tune_agg_k2_recon_floor_s", 90.0, 0.0, 600.0);
		DCO_TuneCfg("agg.k2_retreat_shift", "tune_agg_k2_retreat_shift", 0.05, 0.0, 0.2);
		DCO_TuneCfg("agg.k2_ratio_floor", "tune_agg_k2_ratio_floor", 1.3, 1.0, 3.0);
		DCO_TuneCfg("obj.focus_recon_s", "tune_obj_focus_recon_s", 120.0, 0.0, 600.0);
		DCO_TuneCfg("obj.focus_shaping", "tune_obj_focus_shaping", 0.0, 0.0, 1.0);
		DCO_TuneCfg("obj.role_on", "tune_obj_role_on", 1.0, 0.0, 1.0);
		DCO_TuneCfg("obj.phase_on", "tune_obj_phase_on", 1.0, 0.0, 1.0);
		DCO_TuneCfg("obj.role_handover_s", "tune_obj_role_handover_s", 300.0, 0.0, 3600.0);
		DCO_TuneCfg("obj.focus_giveup_s", "tune_obj_focus_giveup_s", 600.0, 60.0, 3600.0);
		DCO_TuneCfg("tac.fix_los", "tune_tac_fix_los", 1.0, 0.0, 1.0);
		DCO_TuneCfg("tac.standoff_fix", "tune_tac_standoff_fix", 0.0, 0.0, 1.0);
		DCO_TuneCfg("tac.standoff_min_s", "tune_tac_standoff_min_s", 240.0, 0.0, 900.0);
		DCO_TuneCfg("tac.standoff_conf", "tune_tac_standoff_conf", 0.5, 0.0, 1.0);
		DCO_TuneCfg("tac.standoff_max_s", "tune_tac_standoff_max_s", 600.0, 60.0, 1800.0);
		DCO_TuneCfg("intel.est_blend", "tune_intel_est_blend", 1.0, 0.0, 1.0);
		DCO_TuneCfg("op.occupy_empty", "tune_op_occupy_empty", 0.0, 0.0, 1.0);
		DCO_TuneCfg("veh.sbf", "tune_veh_sbf", 0.0, 0.0, 1.0);
		DCO_TuneCfg("veh.sbf_search_m", "tune_veh_sbf_search_m", 1500.0, 200.0, 5000.0);
		DCO_TuneCfg("veh.sbf_apc_min_m", "tune_veh_sbf_apc_min_m", 300.0, 100.0, 1500.0);
		DCO_TuneCfg("veh.sbf_apc_max_m", "tune_veh_sbf_apc_max_m", 550.0, 150.0, 2000.0);
		DCO_TuneCfg("veh.sbf_tank_min_m", "tune_veh_sbf_tank_min_m", 350.0, 100.0, 1500.0);
		DCO_TuneCfg("veh.sbf_tank_max_m", "tune_veh_sbf_tank_max_m", 650.0, 150.0, 2500.0);
		DCO_TuneCfg("veh.sbf_soft_min_m", "tune_veh_sbf_soft_min_m", 450.0, 100.0, 1500.0);
		DCO_TuneCfg("veh.sbf_soft_max_m", "tune_veh_sbf_soft_max_m", 700.0, 150.0, 2000.0);
		DCO_TuneCfg("veh.sbf_at_floor_m", "tune_veh_sbf_at_floor_m", 500.0, 100.0, 2000.0);
	}

	protected void DCO_TuneSyncDefense()
	{
		DCO_TuneCfg("garrison.arrive_start", "tune_garrison_arrive_start", 1.0, 0.0, 1.0);
		DCO_TuneCfg("garrison.arrive_s", "tune_garrison_arrive_s", 8.0, 2.0, 60.0);
		DCO_TuneCfg("garrison.prio_bump", "tune_garrison_prio_bump", 1.0, 0.0, 10.0);
		DCO_TuneCfg("garrison.leftover_field", "tune_garrison_leftover_field", 1.0, 0.0, 1.0);
		DCO_TuneCfg("def.fieldpos", "tune_def_fieldpos", 1.0, 0.0, 1.0);
		DCO_TuneCfg("def.sector_garrison", "tune_def_sector_garrison", 1.0, 0.0, 1.0);
		DCO_TuneCfg("def.fp_blind_fix", "tune_def_fp_blind_fix", 0.0, 0.0, 1.0);
		DCO_TuneCfg("def.fp_blind_m", "tune_def_fp_blind_m", 25.0, 0.0, 100.0);
		DCO_TuneCfg("def.fp_min_los", "tune_def_fp_min_los", 40.0, 0.0, 200.0);
		DCO_TuneCfg("def.fp_los_m", "tune_def_fp_los_m", 200.0, 50.0, 500.0);
		DCO_TuneCfg("def.fp_spacing", "tune_def_fp_spacing", 10.0, 3.0, 40.0);
		DCO_TuneCfg("def.crest_fan", "tune_def_crest_fan", 0.0, 0.0, 1.0);
		DCO_TuneCfg("def.trigger_extra", "tune_def_trigger_extra", 150.0, 0.0, 600.0);
		DCO_TuneCfg("def.place_r", "tune_def_place_r", 45.0, 15.0, 150.0);
		DCO_TuneCfg("def.engage_leash_on", "tune_def_engage_leash_on", 1.0, 0.0, 1.0);
		DCO_TuneCfg("def.engage_leash", "tune_def_engage_leash", 30.0, 0.0, 300.0);
		DCO_TuneCfg("def.perim_confirm_s", "tune_def_perim_confirm_s", 60.0, 0.0, 600.0);
		DCO_TuneCfg("def.perim_dot", "tune_def_perim_dot", -0.17, -1.0, 1.0);
		DCO_TuneCfg("cqb.prefab_negcache", "tune_cqb_prefab_negcache", 0.0, 0.0, 1.0);
		DCO_TuneCfg("cqb.prefab_negcache_fails", "tune_cqb_prefab_negcache_fails", 2.0, 1.0, 10.0);
		DCO_TuneCfg("def.obj_priority", "tune_def_obj_priority", 1.0, 0.0, 1.0);
		DCO_TuneCfg("def.obj_threat_w", "tune_def_obj_threat_w", 10.0, 0.0, 100.0);
		DCO_TuneCfg("def.obj_contested_w", "tune_def_obj_contested_w", 50.0, 0.0, 500.0);
	}
}
