modded class DCO_GlobalAIComponent
{
	protected ref map<string, string> m_mDCOCtxRows;
	protected ref map<string, int> m_mDCODodgeAgg;
	protected float m_fDCODodgeAggAt;

	void DCO_DodgeAgg(string key)
	{
		if (!m_mDCODodgeAgg)
			m_mDCODodgeAgg = new map<string, int>();
		m_mDCODodgeAgg.Set(key, m_mDCODodgeAgg.Get(key) + 1);
		float now = GetGame().GetWorld().GetWorldTime();
		if (m_fDCODodgeAggAt <= 0)
			m_fDCODodgeAggAt = now + 30000.0;
		if (now < m_fDCODodgeAggAt)
			return;
		m_fDCODodgeAggAt = now + 30000.0;
		string line = "dodge_summary scope=all";
		foreach (string k, int v : m_mDCODodgeAgg)
		{
			line += string.Format(" %1=%2", k, v);
		}
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-", line);
	}

	string DCO_CtxRow(string name)
	{
		string v;
		if (m_mDCOCtxRows && m_mDCOCtxRows.Find(name, v))
			return v;
		return string.Empty;
	}

	protected void DCO_SyncCtxRows()
	{
		if (!m_mDCOCtxRows)
			m_mDCOCtxRows = new map<string, string>();
		array<string> names = DCO_Ctx.RowNames();
		foreach (string n : names)
		{
			string cur = DCO_CtxRow(n);
			string got = CfgS("ctx_row_" + n, cur);
			if (m_iCfgMode == 0 && got != cur)
				m_mDCOCtxRows.Set(n, got);
		}
	}

	override protected void SyncConfig()
	{
		super.SyncConfig();
		CfgCat("CombatContext");
		DCO_TuneCfg("ctx.on", "ctx_on", 1.0, 0.0, 1.0);
		DCO_TuneCfg("ctx.threat_k", "ctx_threat_k", 0.5, 0.0, 1.5);
		DCO_TuneCfg("ctx.react_scale", "ctx_react_scale", 0.0, 0.0, 2.0);
		DCO_TuneCfg("ctx.band_contact_m", "ctx_band_contact_m", 10.0, 2.0, 30.0);
		DCO_TuneCfg("ctx.band_close_m", "ctx_band_close_m", 50.0, 20.0, 120.0);
		DCO_TuneCfg("ctx.band_medium_m", "ctx_band_medium_m", 200.0, 80.0, 400.0);
		DCO_TuneCfg("ctx.band_long_m", "ctx_band_long_m", 400.0, 200.0, 1000.0);
		DCO_TuneCfg("ctx.urban_bld_min", "ctx_urban_bld_min", 3.0, 1.0, 20.0);
		DCO_TuneCfg("ctx.forest_tree_min", "ctx_forest_tree_min", 12.0, 3.0, 60.0);
		DCO_TuneCfg("ctx.sup_on", "ctx_sup_on", 1.0, 0.0, 1.0);
		DCO_TuneCfg("ctx.sup_floor_s", "ctx_sup_floor_s", 8.0, 0.0, 30.0);
		DCO_TuneCfg("ctx.sup_cap_s", "ctx_sup_cap_s", 30.0, 5.0, 90.0);
		DCO_TuneCfg("ctx.sup_set_s", "ctx_sup_set_s", 3.0, 0.0, 15.0);
		DCO_TuneCfg("ctx.sup_min_mags", "ctx_sup_min_mags", 2.0, 0.0, 10.0);
		DCO_TuneCfg("ctx.hide_on", "ctx_hide_on", 1.0, 0.0, 1.0);
		DCO_TuneCfg("ctx.cqb_gate", "ctx_cqb_gate", 1.0, 0.0, 1.0);
		DCO_TuneCfg("ctx.mg_floor", "ctx_mg_floor", 1.0, 0.0, 1.0);
		DCO_SyncCtxRows();
		CfgCat("HealUnderFire");
		DCO_TuneCfg("heal.unseen_gate", "heal_unseen_gate", 1.0, 0.0, 1.0);
		DCO_TuneCfg("heal.unseen_m", "heal_unseen_m", 25.0, 5.0, 100.0);
		DCO_TuneCfg("heal.unseen_s", "heal_unseen_s", 6.0, 1.0, 30.0);
		CfgCat("DodgeAssist");
		DCO_TuneCfg("dodge.cover_vs_shooter", "dodge_cover_vs_shooter", 1.0, 0.0, 1.0);
		DCO_TuneCfg("dodge.nearmiss_gate", "dodge_nearmiss_gate", 1.0, 0.0, 1.0);
		DCO_TuneCfg("dodge.dir_assist", "dodge_dir_assist", 1.0, 0.0, 1.0);
		DCO_TuneCfg("dodge.arc_narrow_deg", "dodge_arc_narrow_deg", 45.0, 10.0, 90.0);
		DCO_TuneCfg("dodge.arc_wide_deg", "dodge_arc_wide_deg", 65.0, 10.0, 120.0);
		DCO_TuneCfg("dodge.nearmiss_m", "dodge_nearmiss_m", 1.5, 0.5, 5.0);
		DCO_TuneCfg("dodge.nearmiss2_m", "dodge_nearmiss2_m", 5.0, 1.0, 15.0);
		DCO_TuneCfg("dodge.nearmiss2_s", "dodge_nearmiss2_s", 3.0, 0.5, 10.0);
		DCO_TuneCfg("dodge.hit_s", "dodge_hit_s", 2.0, 0.5, 10.0);
		DCO_TuneCfg("dodge.rush_min_m", "dodge_rush_min_m", 5.0, 1.0, 15.0);
		DCO_TuneCfg("dodge.rush_max_m", "dodge_rush_max_m", 15.0, 3.0, 30.0);
		DCO_TuneCfg("dodge.rush_heavy_m", "dodge_rush_heavy_m", 8.0, 2.0, 20.0);
		DCO_TuneCfg("dodge.heavy_threat", "dodge_heavy_threat", 0.7, 0.1, 2.0);
		DCO_TuneCfg("dodge.crawl_max_m", "dodge_crawl_max_m", 5.0, 1.0, 10.0);
		DCO_TuneCfg("dodge.spacing_m", "dodge_spacing_m", 5.0, 0.0, 15.0);
		DCO_TuneCfg("dodge.mg_micro_m", "dodge_mg_micro_m", 3.0, 1.0, 10.0);
		DCO_TuneCfg("dodge.max_shooters", "dodge_max_shooters", 3.0, 1.0, 6.0);
		DCO_TuneCfg("dodge.max_eval", "dodge_max_eval", 10.0, 3.0, 21.0);
		DCO_TuneCfg("dodge.ray_margin_m", "dodge_ray_margin_m", 0.3, 0.0, 2.0);
		DCO_TuneCfg("dodge.nearmiss_sep_s", "dodge_nearmiss_sep_s", 0.5, 0.0, 3.0);
		CfgFlush();
		if (m_iCfgMode == 0)
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("ctx_cfg on=%1 threat_k=%2 react_scale=%3 bands=%4/%5/%6/%7 urban_bld_min=%8 forest_tree_min=%9",
				DCO_TuneGet("ctx.on", 1.0), DCO_TuneGet("ctx.threat_k", 0.5), DCO_TuneGet("ctx.react_scale", 0.0), DCO_TuneGet("ctx.band_contact_m", 10.0), DCO_TuneGet("ctx.band_close_m", 50.0),
				DCO_TuneGet("ctx.band_medium_m", 200.0), DCO_TuneGet("ctx.band_long_m", 400.0), DCO_TuneGet("ctx.urban_bld_min", 3.0), DCO_TuneGet("ctx.forest_tree_min", 12.0)));
		if (m_iCfgMode == 0)
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("ctx_cfg2 sup_on=%1 sup_floor_s=%2 sup_cap_s=%3 sup_set_s=%4 sup_min_mags=%5 hide_on=%6 cqb_gate=%7 mg_floor=%8",
				DCO_TuneGet("ctx.sup_on", 1.0), DCO_TuneGet("ctx.sup_floor_s", 8.0), DCO_TuneGet("ctx.sup_cap_s", 30.0), DCO_TuneGet("ctx.sup_set_s", 3.0), DCO_TuneGet("ctx.sup_min_mags", 2.0),
				DCO_TuneGet("ctx.hide_on", 1.0), DCO_TuneGet("ctx.cqb_gate", 1.0), DCO_TuneGet("ctx.mg_floor", 1.0)));
		if (m_iCfgMode == 0)
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("dodge_cfg cover_vs_shooter=%1 nearmiss_gate=%2 dir_assist=%3 nearmiss_m=%4 rush_max_m=%5 crawl_max_m=%6 spacing_m=%7 mg_micro_m=%8",
				DCO_TuneGet("dodge.cover_vs_shooter", 1.0), DCO_TuneGet("dodge.nearmiss_gate", 1.0), DCO_TuneGet("dodge.dir_assist", 1.0), DCO_TuneGet("dodge.nearmiss_m", 1.5), DCO_TuneGet("dodge.rush_max_m", 15.0), DCO_TuneGet("dodge.crawl_max_m", 5.0), DCO_TuneGet("dodge.spacing_m", 5.0), DCO_TuneGet("dodge.mg_micro_m", 3.0)));
	}
}
