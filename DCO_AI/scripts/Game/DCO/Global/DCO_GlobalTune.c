modded class DCO_GlobalAIComponent
{
	[Attribute("1", UIWidgets.Slider, "Overall AI aggression. 1 = DCO default (2.5x the old normal). Effective = value x 2.5. Lower = more careful, higher = pushes harder. Affects soldiers, squad posture and the commander.", params: "0.5 10 0.05", category: "Individual")]
	protected float m_fDCOAggression;
	protected ref map<string, float> m_mDCOTune;
	protected int m_iDCOTuneVer;
	protected static ref map<string, float> s_mDCOTuneBase;
	protected static int s_iDCOTuneVerAll;
	protected static ref map<string, vector> s_mDCOTuneRange;
	protected static ref map<string, string> s_mDCOTuneAlias;
	protected string m_sDCOPresetUploads = "gm";
	protected int m_iDCOPresetLimit = 200;
	protected string m_sDCOSaveConfig = "admin";
	protected bool m_bDCOStartupPrompt = true;
	protected bool m_bDCOGmAutoSave = true;

	bool DCO_GmAutoSave()				{ return m_bDCOGmAutoSave; }

	string DCO_PresetUploads()			{ return m_sDCOPresetUploads; }
	int DCO_PresetLimit()				{ return m_iDCOPresetLimit; }
	bool DCO_StartupPrompt()			{ return m_bDCOStartupPrompt; }

	static bool DCO_CanSaveConfig(int playerId, out string why)
	{
		DCO_GlobalAIComponent g = GetInstance();
		if (!g)
		{
			why = "no_global";
			return false;
		}
		why = g.m_sDCOSaveConfig;
		if (why == "off")
			return false;
		PlayerController pc = GetGame().GetPlayerController();
		if (pc && pc.GetPlayerId() == playerId)
		{
			why = "host";
			return true;
		}
		if (why == "gm")
			return true;
		PlayerManager pm = GetGame().GetPlayerManager();
		return pm && (pm.HasPlayerRole(playerId, EPlayerRole.ADMINISTRATOR) || pm.HasPlayerRole(playerId, EPlayerRole.SESSION_ADMINISTRATOR));
	}

	static int DCO_TuneVerAll()		{ return s_iDCOTuneVerAll; }
	int DCO_TuneVer()					{ return m_iDCOTuneVer; }
	float DCO_Aggression()				{ return DCO_AggressionSlider() * 2.5; }
	float DCO_AggressionSlider()		{ return Math.Clamp(m_fDCOAggression, 0.5, 10); }

	void DCO_SetAggression(float value)
	{
		m_fDCOAggression = Math.Clamp(value, 0.5, 10);
		m_iDCOTuneVer++;
		s_iDCOTuneVerAll++;
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("gm_set_aggression scope=%1 value=%2 eff=%3", DCO_DocScope(), m_fDCOAggression.ToString(-1, 2), DCO_Aggression().ToString(-1, 2)));
	}

	float DCO_TuneGet(string key, float def)
	{
		float v;
		if (m_mDCOTune && m_mDCOTune.Find(key, v))
			return v;
		if (s_mDCOTuneBase && s_mDCOTuneBase.Find(key, v))
			return v;
		return def;
	}

	void DCO_TuneSet(string key, float value)
	{
		if (!m_mDCOTune)
			m_mDCOTune = new map<string, float>();
		m_mDCOTune.Set(key, value);
		m_iDCOTuneVer++;
		s_iDCOTuneVerAll++;
	}

	float DCO_TuneF(string key, float cur)
	{
		float v;
		if (m_mDCOTune && m_mDCOTune.Find(key, v))
			return v;
		if (!s_mDCOTuneBase)
			s_mDCOTuneBase = new map<string, float>();
		if (!s_mDCOTuneBase.Contains(key))
			s_mDCOTuneBase.Set(key, cur);
		return cur;
	}

	int DCO_TuneI(string key, float cur)
	{
		return Math.Round(DCO_TuneF(key, cur));
	}

	bool DCO_TuneB(string key, float cur)
	{
		return DCO_TuneF(key, cur) >= 0.5;
	}

	static bool DCO_TuneRange(string key, out vector range)
	{
		if (!s_mDCOTuneRange)
		{
			DCO_GlobalAIComponent g = GetInstance();
			if (g && !s_mDCOTuneRange)
				g.DumpActiveConfig();
			if (!s_mDCOTuneRange)
				s_mDCOTuneRange = new map<string, vector>();
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("api_tune_registry keys=%1", s_mDCOTuneRange.Count()));
		}
		return s_mDCOTuneRange.Find(key, range);
	}

	static string DCO_TuneKeyOf(string name)
	{
		vector r;
		if (DCO_TuneRange(name, r))
			return name;
		string k;
		if (s_mDCOTuneAlias && s_mDCOTuneAlias.Find(name, k))
			return k;
		return string.Empty;
	}

	protected void DCO_TuneCfg(string key, string cfgKey, float def, float min, float max)
	{
		if (!s_mDCOTuneRange)
			s_mDCOTuneRange = new map<string, vector>();
		s_mDCOTuneRange.Set(key, Vector(min, max, def));
		if (!s_mDCOTuneAlias)
			s_mDCOTuneAlias = new map<string, string>();
		s_mDCOTuneAlias.Set(cfgKey, key);
		float cur = DCO_TuneGet(key, def);
		float got = CfgF(cfgKey, cur, min, max);
		if (m_iCfgMode != 2 && (got != cur || !m_mDCOTune || !m_mDCOTune.Contains(key)))
			DCO_TuneSet(key, got);
	}

	override protected void SyncConfig()
	{
		super.SyncConfig();
		CfgCat("Aggression");
		float aggr = CfgF("aiAggression", m_fDCOAggression, 0.5, 10.0);
		if (aggr != m_fDCOAggression)
		{
			m_fDCOAggression = aggr;
			m_iDCOTuneVer++;
			s_iDCOTuneVerAll++;
		}
		CfgCat("GroupPresets");
		string uploads = CfgS("groupPresetUploads", m_sDCOPresetUploads);
		uploads.ToLower();
		if (uploads == "gm" || uploads == "admin" || uploads == "off")
			m_sDCOPresetUploads = uploads;
		m_iDCOPresetLimit = CfgI("groupPresetLimit", m_iDCOPresetLimit, 0, 10000);
		CfgCat("GMSettings");
		string saveCfg = CfgS("gmSaveConfig", m_sDCOSaveConfig);
		saveCfg.ToLower();
		if (saveCfg == "gm" || saveCfg == "admin" || saveCfg == "off")
			m_sDCOSaveConfig = saveCfg;
		else if (m_iCfgMode == 0)
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", string.Format("cfg_bad_choice key=gmSaveConfig raw=%1 used=%2 allowed=gm/admin/off", saveCfg, m_sDCOSaveConfig));
		m_bDCOStartupPrompt = CfgB("startupPresetPrompt", m_bDCOStartupPrompt);
		m_bDCOGmAutoSave = CfgB("gmAutoSave", m_bDCOGmAutoSave);
		CfgCat("Tune");
		DCO_TuneSync0();
		DCO_TuneSyncFIAP();
		DCO_TuneSync1();
		DCO_TuneSync2();
		DCO_TuneSync3();
		DCO_TuneSyncBD();
		CfgCat("Vehicle AI");
		DCO_TuneSyncVA();
	}

	protected void DCO_TuneSyncVA()
	{
		DCO_TuneCfg("va.enabled", "tune_va_enabled", 1.0, 0.0, 1.0);
		DCO_TuneCfg("va.hide_soft_m", "tune_va_hide_soft_m", 120.0, 0.0, 400.0);
		DCO_TuneCfg("va.hide_apc_m", "tune_va_hide_apc_m", 80.0, 0.0, 400.0);
		DCO_TuneCfg("va.hide_tank_m", "tune_va_hide_tank_m", 50.0, 0.0, 400.0);
		DCO_TuneCfg("va.hide_min_s", "tune_va_hide_min_s", 20.0, 0.0, 180.0);
		DCO_TuneCfg("va.hide_max_s", "tune_va_hide_max_s", 40.0, 0.0, 180.0);
		DCO_TuneCfg("va.scoot_soft_m", "tune_va_scoot_soft_m", 300.0, 0.0, 800.0);
		DCO_TuneCfg("va.scoot_apc_m", "tune_va_scoot_apc_m", 250.0, 0.0, 800.0);
		DCO_TuneCfg("va.scoot_tank_m", "tune_va_scoot_tank_m", 200.0, 0.0, 800.0);
		DCO_TuneCfg("va.expose_soft_s", "tune_va_expose_soft_s", 15.0, 0.0, 120.0);
		DCO_TuneCfg("va.expose_apc_s", "tune_va_expose_apc_s", 12.0, 0.0, 120.0);
		DCO_TuneCfg("va.expose_tank_s", "tune_va_expose_tank_s", 15.0, 0.0, 120.0);
		DCO_TuneCfg("va.withdraw_hp", "tune_va_withdraw_hp", 0.35, 0.0, 1.0);
		DCO_TuneCfg("va.bound_extra_m", "tune_va_bound_extra_m", 250.0, 0.0, 600.0);
		DCO_TuneCfg("va.pair_radius_m", "tune_va_pair_radius_m", 80.0, 20.0, 300.0);
		DCO_TuneCfg("va.recon_on", "tune_va_recon_on", 1.0, 0.0, 1.0);
		DCO_TuneCfg("va.recon_soft_per_min", "tune_va_recon_soft_per_min", 8.0, 0.0, 30.0);
		DCO_TuneCfg("va.recon_apc_per_min", "tune_va_recon_apc_per_min", 5.0, 0.0, 30.0);
		DCO_TuneCfg("va.recon_ammo_floor", "tune_va_recon_ammo_floor", 0.3, 0.0, 1.0);
	}

	protected void DCO_TuneSyncFIAP()
	{
		DCO_TuneCfg("nav.debug", "tune_nav_debug", 0.0, 0.0, 3.0);
		DCO_TuneCfg("aim.head_mul", "tune_aim_head_mul", 1.0, 0.0, 3.0);
		DCO_TuneCfg("opp.exposed", "tune_opp_exposed", 1.0, 0.0, 1.0);
		DCO_TuneCfg("opp.distracted", "tune_opp_distracted", 1.0, 0.0, 1.0);
		DCO_TuneCfg("opp.suppressed_push", "tune_opp_suppressed_push", 1.0, 0.0, 1.0);
		DCO_TuneCfg("opp.cluster_scan", "tune_opp_cluster_scan", 1.0, 0.0, 1.0);
		DCO_TuneCfg("opp.patient", "tune_opp_patient", 1.0, 0.0, 1.0);
		DCO_TuneCfg("opp.loot", "tune_opp_loot", 1.0, 0.0, 1.0);
		DCO_TuneCfg("opp.reload", "tune_opp_reload", 1.0, 0.0, 1.0);
		DCO_TuneCfg("agg.open_fwd", "tune_agg_open_fwd", 0.0, 0.0, 1.0);
		DCO_TuneCfg("gren.ff_predict", "tune_gren_ff_predict", 1.0, 0.0, 1.0);
		DCO_TuneCfg("radio.squad_chatter", "tune_radio_squad_chatter", 0.0, 0.0, 1.0);
		DCO_TuneCfg("tac.stall_s", "tune_tac_stall_s", 60.0, 20.0, 300.0);
		DCO_TuneCfg("tac.stall_contact_s", "tune_tac_stall_contact_s", 120.0, 30.0, 600.0);
		DCO_TuneCfg("tac.pursuit_ratio", "tune_tac_pursuit_ratio", 1.5, 1.0, 4.0);
		DCO_TuneCfg("tac.lost_contact_s", "tune_tac_lost_contact_s", 60.0, 20.0, 600.0);
		DCO_TuneCfg("tac.pursuit_consol_s", "tune_tac_pursuit_consol_s", 180.0, 60.0, 900.0);
		DCO_TuneCfg("cmd.retreat_thr", "tune_cmd_retreat_thr", 1.0, 0.0, 1.0);
		DCO_TuneCfg("learn.ind_on", "tune_learn_ind_on", 1.0, 0.0, 1.0);
		DCO_TuneCfg("learn.ind_cap", "tune_learn_ind_cap", 0.25, 0.0, 0.5);
		DCO_TuneCfg("learn.ind_explore", "tune_learn_ind_explore", 0.12, 0.0, 0.5);
		DCO_TuneCfg("learn.ind_pick", "tune_learn_ind_pick", 0.0, 0.0, 1.0);
		DCO_TuneCfg("learn.ind_freeze", "tune_learn_ind_freeze", 0.0, 0.0, 1.0);
		DCO_TuneCfg("learn.ply_on", "tune_learn_ply_on", 0.0, 0.0, 1.0);
		DCO_TuneCfg("learn.ply_memory", "tune_learn_ply_memory", 1.0, 0.0, 1.0);
		DCO_TuneCfg("learn.ply_suppress", "tune_learn_ply_suppress", 1.0, 0.0, 1.0);
		DCO_TuneCfg("opp.callout", "tune_opp_callout", 1.0, 0.0, 1.0);
		DCO_TuneCfg("opp.bound_callout", "tune_opp_bound_callout", 1.0, 0.0, 1.0);
		DCO_TuneCfg("agg.profile", "tune_agg_profile", 1.0, 0.0, 1.0);
		DCO_TuneCfg("agg.bound_v2", "tune_agg_bound_v2", 1.0, 0.0, 1.0);
		DCO_TuneCfg("agg.retfire", "tune_agg_retfire", 1.0, 0.0, 1.0);
		DCO_TuneCfg("agg.usage_map", "tune_agg_usage_map", 1.0, 0.0, 1.0);
		DCO_TuneCfg("agg.postfrag", "tune_agg_postfrag", 1.0, 0.0, 1.0);
		DCO_TuneCfg("agg.postfrag_cd_s", "tune_agg_postfrag_cd_s", 20.0, 5.0, 120.0);
		DCO_TuneCfg("agg.pursue", "tune_agg_pursue", 1.0, 0.0, 1.0);
		DCO_TuneCfg("agg.pursue_cd_s", "tune_agg_pursue_cd_s", 30.0, 10.0, 180.0);
		DCO_TuneCfg("agg.pursue_grow_m", "tune_agg_pursue_grow_m", 8.0, 3.0, 30.0);
		DCO_TuneCfg("thr.low_cluster", "tune_thr_low_cluster", 0.0, 0.0, 1.0);
		DCO_TuneCfg("logi.low_rel", "tune_logi_low_rel", 1.0, 0.0, 1.0);
		DCO_TuneCfg("logi.extract_ineff", "tune_logi_extract_ineff", 1.0, 0.0, 1.0);
		DCO_TuneCfg("logi.cross_src", "tune_logi_cross_src", 1.0, 0.0, 1.0);
		DCO_TuneCfg("thr.flank_rel", "tune_thr_flank_rel", 1.0, 0.0, 1.0);
		DCO_TuneCfg("thr.flank_route", "tune_thr_flank_route", 0.0, 0.0, 1.0);
		DCO_TuneCfg("thr.flank_retarget", "tune_thr_flank_retarget", 1.0, 0.0, 1.0);
		DCO_TuneCfg("thr.flank_retarget_m", "tune_thr_flank_retarget_m", 30.0, 10.0, 150.0);
		DCO_TuneCfg("rearm.rel_init", "tune_rearm_rel_init", 1.0, 0.0, 1.0);
		DCO_TuneCfg("rearm.end_rel", "tune_rearm_end_rel", 1.0, 0.0, 1.0);
		DCO_TuneCfg("pc.night_on", "tune_pc_night_on", 1.0, 0.0, 1.0);
		DCO_TuneCfg("pc.night_min", "tune_pc_night_min", 0.25, 0.05, 1.0);
		DCO_TuneCfg("pc.night_lv_day", "tune_pc_night_lv_day", 8.0, -20.0, 20.0);
		DCO_TuneCfg("pc.night_lv_dark", "tune_pc_night_lv_dark", -6.0, -20.0, 20.0);
		DCO_TuneCfg("pc.night_flare_m", "tune_pc_night_flare_m", 0.0, 0.0, 1500.0);
		DCO_TuneCfg("hear.shot_err", "tune_hear_shot_err", 1.0, 0.0, 3.0);
		DCO_TuneCfg("rearm.reach_m", "tune_rearm_reach_m", 2.5, 1.0, 10.0);
		DCO_TuneCfg("pickup.debounce_s", "tune_pickup_debounce_s", 2.0, 0.0, 10.0);
		DCO_TuneCfg("pickup.fail_cooldown_s", "tune_pickup_fail_cooldown_s", 120.0, 10.0, 600.0);
		DCO_TuneCfg("spec.mg_pre_yield_rearm", "tune_spec_mg_pre_yield_rearm", 1.0, 0.0, 1.0);
		DCO_TuneCfg("spec.assist_commit_s", "tune_spec_assist_commit_s", 6.0, 1.0, 30.0);
		DCO_TuneCfg("ctrl.gun_exceptions", "tune_ctrl_gun_exceptions", 0.0, 0.0, 1.0);
		DCO_TuneCfg("cqb.lost_window_s", "tune_cqb_lost_window_s", 0.0, 0.0, 120.0);
		DCO_TuneCfg("opp.push_quiet_s", "tune_opp_push_quiet_s", 5.0, 2.0, 30.0);
		DCO_TuneCfg("opp.push_cd_s", "tune_opp_push_cd_s", 15.0, 5.0, 120.0);
		DCO_TuneCfg("opp.push_step_m", "tune_opp_push_step_m", 25.0, 10.0, 50.0);
		DCO_TuneCfg("opp.push_min_m", "tune_opp_push_min_m", 35.0, 10.0, 60.0);
		DCO_TuneCfg("opp.side_cd_s", "tune_opp_side_cd_s", 20.0, 5.0, 120.0);
		DCO_TuneCfg("opp.patient_max_s", "tune_opp_patient_max_s", 6.0, 1.0, 20.0);
		DCO_TuneCfg("fi.RadiusScale", "tune_fi_RadiusScale", 1.0, 0.25, 3.0);
		DCO_TuneCfg("fi.CandidatesPerTickScale", "tune_fi_CandidatesPerTickScale", 1.0, 0.25, 3.0);
		DCO_TuneCfg("fi.MaxCandidatesScale", "tune_fi_MaxCandidatesScale", 1.0, 0.25, 3.0);
		DCO_TuneCfg("fi.MaxTileWaitTicks", "tune_fi_MaxTileWaitTicks", 20.0, 1.0, 100.0);
		DCO_TuneCfg("fi.GoodEnoughScore", "tune_fi_GoodEnoughScore", 0.72, 0.0, 1.0);
		DCO_TuneCfg("fi.ResultTTL", "tune_fi_ResultTTL", 15.0, 1.0, 120.0);
		DCO_TuneCfg("fi.CellSize", "tune_fi_CellSize", 2.5, 0.5, 10.0);
		DCO_TuneCfg("fi.CellHeight", "tune_fi_CellHeight", 3.0, 1.0, 6.0);
		DCO_TuneCfg("fi.DedupeRadius", "tune_fi_DedupeRadius", 1.0, 0.0, 5.0);
		DCO_TuneCfg("fi.TopCandidates", "tune_fi_TopCandidates", 3.0, 1.0, 10.0);
		DCO_TuneCfg("fi.MinHeadroom", "tune_fi_MinHeadroom", 1.6, 1.0, 3.0);
		DCO_TuneCfg("fi.OccupancyRadius", "tune_fi_OccupancyRadius", 3.0, 0.5, 10.0);
		DCO_TuneCfg("fi.DoorBlockRadius", "tune_fi_DoorBlockRadius", 1.2, 0.0, 5.0);
		DCO_TuneCfg("fi.DoorAvoidRadius", "tune_fi_DoorAvoidRadius", 2.5, 0.0, 10.0);
		DCO_TuneCfg("fi.DoorAvoidPenalty", "tune_fi_DoorAvoidPenalty", 0.3, 0.0, 1.0);
		DCO_TuneCfg("fi.RejectChokepoints", "tune_fi_RejectChokepoints", 1.0, 0.0, 1.0);
		DCO_TuneCfg("fi.ChokepointWidth", "tune_fi_ChokepointWidth", 1.4, 0.5, 4.0);
		DCO_TuneCfg("fi.MinBookedDistance", "tune_fi_MinBookedDistance", 3.0, 0.0, 10.0);
		DCO_TuneCfg("fi.RequireFireLine", "tune_fi_RequireFireLine", 1.0, 0.0, 1.0);
		DCO_TuneCfg("fi.WeightCover", "tune_fi_WeightCover", 0.35, 0.0, 1.0);
		DCO_TuneCfg("fi.WeightFireLine", "tune_fi_WeightFireLine", 0.35, 0.0, 1.0);
		DCO_TuneCfg("fi.WeightSpread", "tune_fi_WeightSpread", 0.15, 0.0, 1.0);
		DCO_TuneCfg("fi.WeightProximity", "tune_fi_WeightProximity", 0.15, 0.0, 1.0);
		DCO_TuneCfg("fi.InteriorLineValue", "tune_fi_InteriorLineValue", 0.5, 0.0, 1.0);
		DCO_TuneCfg("fi.StickinessBonus", "tune_fi_StickinessBonus", 0.05, 0.0, 0.3);
		DCO_TuneCfg("ap.UpdateInterval_ms", "tune_ap_UpdateInterval_ms", 500.0, 100.0, 5000.0);
		DCO_TuneCfg("ap.AllowBuildingAssault", "tune_ap_AllowBuildingAssault", 1.0, 0.0, 1.0);
		DCO_TuneCfg("ap.BuildingAssaultDist", "tune_ap_BuildingAssaultDist", 40.0, 5.0, 200.0);
		DCO_TuneCfg("ap.FlankChance", "tune_ap_FlankChance", 0.35, 0.0, 1.0);
	}

	protected void DCO_TuneSync0()
	{
		DCO_TuneCfg("wf.UseRealisticReaction", "tune_wf_UseRealisticReaction", 1.0, 0.0, 1.0);
		DCO_TuneCfg("wf.SuppressedAudibleRange", "tune_wf_SuppressedAudibleRange", 180.0, 0.0, 600.0);
		DCO_TuneCfg("wf.InVehicleHearingFactor", "tune_wf_InVehicleHearingFactor", 0.5, 0.0, 1.0);
		DCO_TuneCfg("wf.AudibleGuaranteeThreshold", "tune_wf_AudibleGuaranteeThreshold", 0.3, 0.0, 1.0);
		DCO_TuneCfg("wf.DirectionErrorBaseDeg", "tune_wf_DirectionErrorBaseDeg", 5.0, 0.0, 45.0);
		DCO_TuneCfg("wf.DirectionErrorExtraDeg", "tune_wf_DirectionErrorExtraDeg", 20.0, 0.0, 90.0);
		DCO_TuneCfg("wf.LookDistFull", "tune_wf_LookDistFull", 60.0, 0.0, 400.0);
		DCO_TuneCfg("wf.LookDistMax", "tune_wf_LookDistMax", 400.0, 0.0, 2000.0);
		DCO_TuneCfg("wf.LookChanceAtMax", "tune_wf_LookChanceAtMax", 0.2, 0.0, 1.0);
		DCO_TuneCfg("wf.LookThreatBonus", "tune_wf_LookThreatBonus", 0.5, 0.0, 1.0);
		DCO_TuneCfg("wf.LookDelayMin", "tune_wf_LookDelayMin", 0.15, 0.0, 3.0);
		DCO_TuneCfg("wf.LookDelayMax", "tune_wf_LookDelayMax", 0.35, 0.0, 3.0);
		DCO_TuneCfg("wf.LookPreferClosest", "tune_wf_LookPreferClosest", 1.0, 0.0, 1.0);
		DCO_TuneCfg("wf.LookLockDuration", "tune_wf_LookLockDuration", 3.0, 0.0, 15.0);
		DCO_TuneCfg("wf.LookOverrideDistRatio", "tune_wf_LookOverrideDistRatio", 0.8, 0.1, 1.0);
		DCO_TuneCfg("wf.LookLockRefreshInterval", "tune_wf_LookLockRefreshInterval", 0.5, 0.1, 3.0);
		DCO_TuneCfg("wf.LookPriorityProximityBonus", "tune_wf_LookPriorityProximityBonus", 10.0, 0.0, 50.0);
		DCO_TuneCfg("wf.CoverThreatGate", "tune_wf_CoverThreatGate", 0.4, 0.0, 1.0);
		DCO_TuneCfg("wf.LowerStanceOnLowThreat", "tune_wf_LowerStanceOnLowThreat", 1.0, 0.0, 1.0);
		DCO_TuneCfg("wf.CoverDelayMin", "tune_wf_CoverDelayMin", 0.2, 0.0, 3.0);
		DCO_TuneCfg("wf.CoverDelayMax", "tune_wf_CoverDelayMax", 0.6, 0.0, 3.0);
		DCO_TuneCfg("wf.HighThreatProneThreshold", "tune_wf_HighThreatProneThreshold", 0.7, 0.0, 1.0);
		DCO_TuneCfg("wf.CoverSearchDistCap", "tune_wf_CoverSearchDistCap", 12.0, 2.0, 50.0);
		DCO_TuneCfg("wf.SharedMoveCooldown", "tune_wf_SharedMoveCooldown", 8.0, 0.0, 30.0);
		DCO_TuneCfg("wf.CoverPreferBuilding", "tune_wf_CoverPreferBuilding", 0.0, 0.0, 1.0);
		DCO_TuneCfg("wf.CoverVeryHighPriority", "tune_wf_CoverVeryHighPriority", 1.0, 0.0, 1.0);
		DCO_TuneCfg("wf.UseDodgeChanceOnCover", "tune_wf_UseDodgeChanceOnCover", 1.0, 0.0, 1.0);
		DCO_TuneCfg("wf.LookAfterCover", "tune_wf_LookAfterCover", 1.0, 0.0, 1.0);
		DCO_TuneCfg("wf.LookAfterCoverRepeats", "tune_wf_LookAfterCoverRepeats", 3.0, 0.0, 10.0);
		DCO_TuneCfg("wf.LookAfterCoverInterval", "tune_wf_LookAfterCoverInterval", 1.0, 0.1, 5.0);
		DCO_TuneCfg("wf.LookAfterCoverDelay", "tune_wf_LookAfterCoverDelay", 0.3, 0.0, 3.0);
	}

	protected void DCO_TuneSync1()
	{
		DCO_TuneCfg("wf.RandomizeCoverDirection", "tune_wf_RandomizeCoverDirection", 1.0, 0.0, 1.0);
		DCO_TuneCfg("wf.CoverDirWeightBackward", "tune_wf_CoverDirWeightBackward", 1.0, 0.0, 5.0);
		DCO_TuneCfg("wf.CoverDirWeightLeft", "tune_wf_CoverDirWeightLeft", 1.0, 0.0, 5.0);
		DCO_TuneCfg("wf.CoverDirWeightRight", "tune_wf_CoverDirWeightRight", 1.0, 0.0, 5.0);
		DCO_TuneCfg("wf.CoverDirWeightAnywhere", "tune_wf_CoverDirWeightAnywhere", 1.0, 0.0, 5.0);
		DCO_TuneCfg("wf.ScaleCoverDirByPersonality", "tune_wf_ScaleCoverDirByPersonality", 1.0, 0.0, 1.0);
		DCO_TuneCfg("wf.CoverDirForwardWeightScale", "tune_wf_CoverDirForwardWeightScale", 1.0, 0.0, 3.0);
		DCO_TuneCfg("wf.DodgeTowardWaypoint", "tune_wf_DodgeTowardWaypoint", 1.0, 0.0, 1.0);
		DCO_TuneCfg("ph.ReactToNearMiss", "tune_ph_ReactToNearMiss", 1.0, 0.0, 1.0);
		DCO_TuneCfg("ph.BulletsRepositionNoTarget", "tune_ph_BulletsRepositionNoTarget", 2.0, 1.0, 20.0);
		DCO_TuneCfg("ph.BulletsRepositionHeavy", "tune_ph_BulletsRepositionHeavy", 4.0, 1.0, 20.0);
		DCO_TuneCfg("ph.BulletsFleeNoCover", "tune_ph_BulletsFleeNoCover", 1.0, 1.0, 20.0);
		DCO_TuneCfg("ph.BulletsFleeEngaged", "tune_ph_BulletsFleeEngaged", 3.0, 1.0, 20.0);
		DCO_TuneCfg("ph.ScaleThresholdsByPersonality", "tune_ph_ScaleThresholdsByPersonality", 1.0, 0.0, 1.0);
		DCO_TuneCfg("ph.DeploySmokeOnFlee", "tune_ph_DeploySmokeOnFlee", 1.0, 0.0, 1.0);
		DCO_TuneCfg("ph.CoverPreferBuilding", "tune_ph_CoverPreferBuilding", 0.0, 0.0, 1.0);
		DCO_TuneCfg("ph.CoverVeryHighPriority", "tune_ph_CoverVeryHighPriority", 1.0, 0.0, 1.0);
		DCO_TuneCfg("ph.RespectOngoingMove", "tune_ph_RespectOngoingMove", 1.0, 0.0, 1.0);
		DCO_TuneCfg("ph.NewThreatAngleDeg", "tune_ph_NewThreatAngleDeg", 60.0, 10.0, 180.0);
		DCO_TuneCfg("ph.SkipWhileCoverActive", "tune_ph_SkipWhileCoverActive", 1.0, 0.0, 1.0);
		DCO_TuneCfg("ph.CoverMoveCooldown_s", "tune_ph_CoverMoveCooldown_s", 5.0, 0.0, 30.0);
		DCO_TuneCfg("ph.RepositionSearchDist", "tune_ph_RepositionSearchDist", 6.0, 1.0, 30.0);
		DCO_TuneCfg("ph.FleeSearchDist", "tune_ph_FleeSearchDist", 10.0, 1.0, 50.0);
		DCO_TuneCfg("ph.ForceStandWhileMoving", "tune_ph_ForceStandWhileMoving", 1.0, 0.0, 1.0);
		DCO_TuneCfg("ph.SuppressWhileMoving", "tune_ph_SuppressWhileMoving", 0.0, 0.0, 1.0);
		DCO_TuneCfg("ph.AimWhileMovingToCover", "tune_ph_AimWhileMovingToCover", 0.0, 0.0, 1.0);
		DCO_TuneCfg("ph.UseDodgeChance", "tune_ph_UseDodgeChance", 1.0, 0.0, 1.0);
		DCO_TuneCfg("ph.RandomizeCoverDirection", "tune_ph_RandomizeCoverDirection", 1.0, 0.0, 1.0);
		DCO_TuneCfg("ph.CoverDirWeightBackward", "tune_ph_CoverDirWeightBackward", 1.0, 0.0, 5.0);
		DCO_TuneCfg("ph.CoverDirWeightLeft", "tune_ph_CoverDirWeightLeft", 1.0, 0.0, 5.0);
		DCO_TuneCfg("ph.CoverDirWeightRight", "tune_ph_CoverDirWeightRight", 1.0, 0.0, 5.0);
		DCO_TuneCfg("ph.CoverDirWeightAnywhere", "tune_ph_CoverDirWeightAnywhere", 1.0, 0.0, 5.0);
		DCO_TuneCfg("ph.ScaleCoverDirByPersonality", "tune_ph_ScaleCoverDirByPersonality", 1.0, 0.0, 1.0);
		DCO_TuneCfg("ph.DodgeTowardWaypoint", "tune_ph_DodgeTowardWaypoint", 1.0, 0.0, 1.0);
		DCO_TuneCfg("cm.MaxBookingDuration", "tune_cm_MaxBookingDuration", 30.0, 5.0, 300.0);
	}

	protected void DCO_TuneSync2()
	{
		DCO_TuneCfg("ir.TowardThreatTolerance", "tune_ir_TowardThreatTolerance", 1.0, 0.0, 10.0);
		DCO_TuneCfg("ir.ThreatWeightDefend", "tune_ir_ThreatWeightDefend", 0.6, 0.0, 1.0);
		DCO_TuneCfg("ir.ThreatWeightFire", "tune_ir_ThreatWeightFire", 0.7, 0.0, 1.0);
		DCO_TuneCfg("ir.ThreatSampleSpread", "tune_ir_ThreatSampleSpread", 2.5, 0.0, 10.0);
		DCO_TuneCfg("cr.MinEnemyToReport", "tune_cr_MinEnemyToReport", 1.0, 1.0, 10.0);
		DCO_TuneCfg("cr.ContactMaxAge", "tune_cr_ContactMaxAge", 60.0, 5.0, 300.0);
		DCO_TuneCfg("cr.ContactClusterRadius", "tune_cr_ContactClusterRadius", 75.0, 10.0, 300.0);
		DCO_TuneCfg("cr.ReportMinShift", "tune_cr_ReportMinShift", 50.0, 5.0, 300.0);
		DCO_TuneCfg("cr.MinResendInterval", "tune_cr_MinResendInterval", 30.0, 0.0, 300.0);
		DCO_TuneCfg("cr.LowConfidenceThreshold", "tune_cr_LowConfidenceThreshold", 0.5, 0.0, 1.0);
		DCO_TuneCfg("cr.HighDangerEnemyCount", "tune_cr_HighDangerEnemyCount", 8.0, 1.0, 25.0);
		DCO_TuneCfg("cr.HighDangerDistance", "tune_cr_HighDangerDistance", 100.0, 10.0, 500.0);
		DCO_TuneCfg("cr.ReinforcementThreshold", "tune_cr_ReinforcementThreshold", 3.0, 1.0, 12.0);
		DCO_TuneCfg("cr.ReinfRequestTimeout", "tune_cr_ReinfRequestTimeout", 90.0, 10.0, 600.0);
		DCO_TuneCfg("cr.ArtilleryEnemyThreshold", "tune_cr_ArtilleryEnemyThreshold", 4.0, 1.0, 25.0);
		DCO_TuneCfg("cr.ArtilleryRequestCooldown", "tune_cr_ArtilleryRequestCooldown", 120.0, 0.0, 900.0);
		DCO_TuneCfg("cr.NonFavoredRoleArtyRejectChance", "tune_cr_NonFavoredRoleArtyRejectChance", 0.35, 0.0, 1.0);
		DCO_TuneCfg("cr.NoisePer100m", "tune_cr_NoisePer100m", 8.0, 0.0, 50.0);
		DCO_TuneCfg("cr.NightNoiseMul", "tune_cr_NightNoiseMul", 2.0, 1.0, 5.0);
		DCO_TuneCfg("cr.DetectedNoiseMul", "tune_cr_DetectedNoiseMul", 1.8, 1.0, 5.0);
		DCO_TuneCfg("cr.BucketSquadMin", "tune_cr_BucketSquadMin", 4.0, 2.0, 20.0);
		DCO_TuneCfg("cr.BucketPlatoonMin", "tune_cr_BucketPlatoonMin", 10.0, 4.0, 60.0);
		DCO_TuneCfg("cr.ArmorIdentifyDist", "tune_cr_ArmorIdentifyDist", 400.0, 50.0, 2000.0);
		DCO_TuneCfg("cr.RadioDelayMin", "tune_cr_RadioDelayMin", 5.0, 0.0, 60.0);
		DCO_TuneCfg("cr.RadioDelayMax", "tune_cr_RadioDelayMax", 20.0, 0.0, 120.0);
		DCO_TuneCfg("cr.RequireRadio", "tune_cr_RequireRadio", 0.0, 0.0, 1.0);
		DCO_TuneCfg("cr.SuppressedDelayMul", "tune_cr_SuppressedDelayMul", 1.5, 1.0, 5.0);
		DCO_TuneCfg("cr.SuppressedNoiseMul", "tune_cr_SuppressedNoiseMul", 1.5, 1.0, 5.0);
		DCO_TuneCfg("tt.BoardingDist", "tune_tt_BoardingDist", 25.0, 5.0, 100.0);
		DCO_TuneCfg("tt.BoardedRatio", "tune_tt_BoardedRatio", 0.7, 0.0, 1.0);
		DCO_TuneCfg("tt.ArrivalDist", "tune_tt_ArrivalDist", 25.0, 5.0, 150.0);
		DCO_TuneCfg("tt.BoardingTimeout", "tune_tt_BoardingTimeout", 60.0, 10.0, 600.0);
		DCO_TuneCfg("tt.DriveTimeout", "tune_tt_DriveTimeout", 300.0, 30.0, 1800.0);
	}

	protected void DCO_TuneSync3()
	{
		DCO_TuneCfg("tt.DisembarkTimeout", "tune_tt_DisembarkTimeout", 30.0, 5.0, 300.0);
		DCO_TuneCfg("tt.ReturnToRallyAfterDrop", "tune_tt_ReturnToRallyAfterDrop", 1.0, 0.0, 1.0);
		DCO_TuneCfg("tt.LoadTime", "tune_tt_LoadTime", 10.0, 0.0, 120.0);
		DCO_TuneCfg("tt.ResupplyTime", "tune_tt_ResupplyTime", 20.0, 0.0, 180.0);
		DCO_TuneCfg("tt.ResupplyMags", "tune_tt_ResupplyMags", 4.0, 0.0, 12.0);
		DCO_TuneCfg("tm.cqb_range_m", "tune_tm_cqb_range_m", 25.0, 0.0, 60.0);
		DCO_TuneCfg("tm.cqb_chance", "tune_tm_cqb_chance", 0.6, 0.0, 1.0);
		DCO_TuneCfg("tm.cqb_aggr_in", "tune_tm_cqb_aggr_in", 4.0, 1.0, 8.0);
		DCO_TuneCfg("tm.cqb_aggr_out", "tune_tm_cqb_aggr_out", 4.0, 1.0, 8.0);
		DCO_TuneCfg("tb.on", "tune_tb_on", 1.0, 0.0, 1.0);
		DCO_TuneCfg("tb.set_s", "tune_tb_set_s", 1.5, 0.5, 6.0);
		DCO_TuneCfg("tb.set_frac", "tune_tb_set_frac", 0.34, 0.0, 0.75);
		DCO_TuneCfg("tb.fire_s", "tune_tb_fire_s", 4.0, 1.0, 15.0);
		DCO_TuneCfg("tb.bound_max_s", "tune_tb_bound_max_s", 12.0, 3.0, 40.0);
		DCO_TuneCfg("tb.idle_s", "tune_tb_idle_s", 4.0, 1.0, 20.0);
		DCO_TuneCfg("tb.deadlock_s", "tune_tb_deadlock_s", 10.0, 3.0, 60.0);
		DCO_TuneCfg("tb.ow_sup", "tune_tb_ow_sup", 1.0, 0.0, 1.0);
		DCO_TuneCfg("tgt.claim", "tune_tgt_claim", 1.0, 0.0, 1.0);
		DCO_TuneCfg("tgt.claim_pen", "tune_tgt_claim_pen", 1.5, 0.0, 10.0);
		DCO_TuneCfg("tgt.claim_ttl", "tune_tgt_claim_ttl", 3.0, 0.5, 10.0);
		DCO_TuneCfg("mb.on", "tune_mb_on", 1.0, 0.0, 1.0);
		DCO_TuneCfg("mb.commit_s", "tune_mb_commit_s", 3.0, 0.5, 15.0);
		DCO_TuneCfg("mb.med_wait_s", "tune_mb_med_wait_s", 5.0, 1.0, 30.0);
		DCO_TuneCfg("mb.atk_fallback_s", "tune_mb_atk_fallback_s", 25.0, 5.0, 180.0);
		DCO_TuneCfg("mb.comp_hits", "tune_mb_comp_hits", 2.0, 1.0, 10.0);
		DCO_TuneCfg("mb.field", "tune_mb_field", 1.0, 0.0, 1.0);
		DCO_TuneCfg("mb.field_ring2_m", "tune_mb_field_ring2_m", 8.0, 0.0, 15.0);
		DCO_TuneCfg("mb.blast_m", "tune_mb_blast_m", 20.0, 0.0, 40.0);
		DCO_TuneCfg("mb.blast_s", "tune_mb_blast_s", 6.0, 0.0, 15.0);
		DCO_TuneCfg("tb.fm", "tune_tb_fm", 1.0, 0.0, 1.0);
		DCO_TuneCfg("gren.atk_close", "tune_gren_atk_close", 1.0, 0.0, 1.0);
		DCO_TuneCfg("gren.atk_close_m", "tune_gren_atk_close_m", 35.0, 10.0, 60.0);
		DCO_TuneCfg("gren.atk_threat_mul", "tune_gren_atk_threat_mul", 2.0, 1.0, 5.0);
		DCO_TuneCfg("tb.fm_min_m", "tune_tb_fm_min_m", 50.0, 10.0, 200.0);
		DCO_TuneCfg("tb.fm_max_m", "tune_tb_fm_max_m", 300.0, 100.0, 600.0);
		DCO_TuneCfg("tb.fm_assault_m", "tune_tb_fm_assault_m", 40.0, 10.0, 100.0);
		DCO_TuneCfg("tb.fm_lost_s", "tune_tb_fm_lost_s", 30.0, 5.0, 120.0);
		DCO_TuneCfg("mb.field_ring_m", "tune_mb_field_ring_m", 4.0, 1.0, 10.0);
		DCO_TuneCfg("mb.field_spacing_m", "tune_mb_field_spacing_m", 4.0, 1.0, 10.0);
		DCO_TuneCfg("tm.strafe_fire", "tune_tm_strafe_fire", 1.0, 0.0, 1.0);
		DCO_TuneCfg("tm.strafe_max_m", "tune_tm_strafe_max_m", 60.0, 0.0, 150.0);
		DCO_TuneCfg("fm.auto", "tune_fm_auto", 1.0, 0.0, 1.0);
		DCO_TuneCfg("spec.bt", "tune_spec_bt", 1.0, 0.0, 1.0);
		DCO_TuneCfg("is.est_share", "tune_is_est_share", 0.0, 0.0, 1.0);
		DCO_TuneCfg("is.designate", "tune_is_designate", 0.0, 0.0, 1.0);
		DCO_TuneCfg("is.quality", "tune_is_quality", 0.0, 0.0, 1.0);
		DCO_TuneCfg("hear.v2", "tune_hear_v2", 0.0, 0.0, 1.0);
		DCO_TuneCfg("hear.mask", "tune_hear_mask", 0.0, 0.0, 1.0);
		DCO_TuneCfg("hear.contact", "tune_hear_contact", 0.0, 0.0, 1.0);
		DCO_TuneCfg("hear.est", "tune_hear_est", 0.0, 0.0, 1.0);
		DCO_TuneCfg("rt.fast", "tune_rt_fast", 1.0, 0.0, 1.0);
		DCO_TuneCfg("rt.close_m", "tune_rt_close_m", 30.0, 0.0, 100.0);
		DCO_TuneCfg("rt.close_threat_s", "tune_rt_close_threat_s", 0.2, 0.0, 1.0);
		DCO_TuneCfg("rt.close_surprise_s", "tune_rt_close_surprise_s", 0.15, 0.0, 1.0);
		DCO_TuneCfg("rt.reacquire_s", "tune_rt_reacquire_s", 10.0, 0.0, 60.0);
		DCO_TuneCfg("rt.settle_close_s", "tune_rt_settle_close_s", 0.35, 0.1, 1.5);
		DCO_TuneCfg("rt.close_reset_mul", "tune_rt_close_reset_mul", 0.5, 0.0, 1.0);
		DCO_TuneCfg("fm.open_shape", "tune_fm_open_shape", 0.0, 0.0, 9.0);
		DCO_TuneCfg("fm.forest_shape", "tune_fm_forest_shape", 0.0, 0.0, 9.0);
		DCO_TuneCfg("fm.urban_shape", "tune_fm_urban_shape", 0.0, 0.0, 9.0);
		DCO_TuneCfg("fm.open_sp", "tune_fm_open_sp", 0.0, 0.0, 3.0);
		DCO_TuneCfg("fm.forest_sp", "tune_fm_forest_sp", 0.0, 0.0, 3.0);
		DCO_TuneCfg("fm.urban_sp", "tune_fm_urban_sp", 0.0, 0.0, 3.0);
		DCO_TuneCfg("fm.night_tight", "tune_fm_night_tight", 1.0, 0.0, 1.0);
		DCO_TuneCfg("fm.contact_line", "tune_fm_contact_line", 1.0, 0.0, 1.0);
		DCO_TuneCfg("fm.echelon", "tune_fm_echelon", 1.0, 0.0, 1.0);
		DCO_TuneCfg("reinf.green", "tune_reinf_green", 0.85, 0.0, 1.0);
		DCO_TuneCfg("reinf.amber", "tune_reinf_amber", 0.7, 0.0, 1.0);
		DCO_TuneCfg("reinf.red", "tune_reinf_red", 0.5, 0.0, 1.0);
		DCO_TuneCfg("reinf.ratio_attack", "tune_reinf_ratio_attack", 2.0, 0.0, 5.0);
		DCO_TuneCfg("reinf.ratio_hasty", "tune_reinf_ratio_hasty", 1.7, 0.0, 5.0);
		DCO_TuneCfg("reinf.ratio_defend", "tune_reinf_ratio_defend", 0.5, 0.0, 5.0);
		DCO_TuneCfg("reinf.conf_min", "tune_reinf_conf_min", 0.4, 0.0, 1.0);
		DCO_TuneCfg("reinf.hold_s", "tune_reinf_hold_s", 60.0, 0.0, 300.0);
	}

	protected void DCO_TuneSyncBD()
	{
		DCO_TuneCfg("mb.v2", "tune_mb_v2", 1.0, 0.0, 1.0);
		DCO_TuneCfg("mb.rush_s", "tune_mb_rush_s", 4.0, 1.0, 10.0);
		DCO_TuneCfg("mb.rush_stress_s", "tune_mb_rush_stress_s", 2.5, 1.0, 10.0);
		DCO_TuneCfg("mb.gain_min", "tune_mb_gain_min", 0.15, 0.0, 1.0);
		DCO_TuneCfg("ff.vert", "tune_ff_vert", 1.0, 0.0, 1.0);
		DCO_TuneCfg("cqb.kind_beh", "tune_cqb_kind_beh", 1.0, 0.0, 1.0);
		DCO_TuneCfg("cqb.in_bound_m", "tune_cqb_in_bound_m", 6.0, 2.0, 15.0);
		DCO_TuneCfg("cqb.out_cover_m", "tune_cqb_out_cover_m", 12.0, 4.0, 30.0);
		DCO_TuneCfg("cqb.out_lateral_m", "tune_cqb_out_lateral_m", 5.0, 2.0, 10.0);
		DCO_TuneCfg("tb.fm_gate", "tune_tb_fm_gate", 1.0, 0.0, 1.0);
		DCO_TuneCfg("tb.fm_close_m", "tune_tb_fm_close_m", 15.0, 5.0, 40.0);
		DCO_TuneCfg("tb.fm_gate_wait_s", "tune_tb_fm_gate_wait_s", 15.0, 3.0, 60.0);
		DCO_TuneCfg("tb.mg_ow", "tune_tb_mg_ow", 1.0, 0.0, 1.0);
		DCO_TuneCfg("tb.mg_leash_m", "tune_tb_mg_leash_m", 40.0, 10.0, 150.0);
		DCO_TuneCfg("mb.mg_exempt", "tune_mb_mg_exempt", 1.0, 0.0, 1.0);
		DCO_TuneCfg("cov.on", "tune_cov_on", 1.0, 0.0, 1.0);
		DCO_TuneCfg("sa.hazard_skip", "tune_sa_hazard_skip", 1.0, 0.0, 1.0);
		DCO_TuneCfg("sa.hazard_r", "tune_sa_hazard_r", 1.5, 0.5, 5.0);
		DCO_TuneCfg("cov.cands", "tune_cov_cands", 4.0, 1.0, 10.0);
		DCO_TuneCfg("cov.cands_min", "tune_cov_cands_min", 2.0, 1.0, 10.0);
		DCO_TuneCfg("cov.space_m", "tune_cov_space_m", 5.0, 1.0, 20.0);
		DCO_TuneCfg("cov.anchor_pad", "tune_cov_anchor_pad", 15.0, 0.0, 50.0);
		DCO_TuneCfg("cov.w_anchor", "tune_cov_w_anchor", 3.0, 0.0, 10.0);
		DCO_TuneCfg("cov.w_dist", "tune_cov_w_dist", 2.0, 0.0, 10.0);
		DCO_TuneCfg("cov.w_hard", "tune_cov_w_hard", 2.0, 0.0, 10.0);
		DCO_TuneCfg("cov.w_prot", "tune_cov_w_prot", 5.0, 0.0, 10.0);
		DCO_TuneCfg("cov.w_vanilla", "tune_cov_w_vanilla", 1.0, 0.0, 10.0);
		DCO_TuneCfg("osc.on", "tune_osc_on", 1.0, 0.0, 1.0);
		DCO_TuneCfg("osc.leg_m", "tune_osc_leg_m", 3.0, 0.5, 20.0);
		DCO_TuneCfg("osc.fix_dir", "tune_osc_fix_dir", 1.0, 0.0, 1.0);
		DCO_TuneCfg("osc.dir_keep_s", "tune_osc_dir_keep_s", 8.0, 1.0, 30.0);
		DCO_TuneCfg("osc.dir_noreverse_s", "tune_osc_dir_noreverse_s", 15.0, 1.0, 60.0);
		DCO_TuneCfg("osc.fix_commit", "tune_osc_fix_commit", 1.0, 0.0, 1.0);
		DCO_TuneCfg("osc.commit_s", "tune_osc_commit_s", 3.0, 0.5, 15.0);
		DCO_TuneCfg("osc.fix_cover", "tune_osc_fix_cover", 1.0, 0.0, 1.0);
		DCO_TuneCfg("osc.cover_half_deg", "tune_osc_cover_half_deg", 60.0, 10.0, 180.0);
		DCO_TuneCfg("osc.fix_short", "tune_osc_fix_short", 1.0, 0.0, 1.0);
		DCO_TuneCfg("osc.fix_bound", "tune_osc_fix_bound", 0.0, 0.0, 1.0);
		DCO_TuneCfg("osc.bound_min_s", "tune_osc_bound_min_s", 4.0, 1.0, 20.0);
		DCO_TuneCfg("cqb.door_avoid", "tune_cqb_door_avoid", 0.0, 0.0, 1.0);
		DCO_TuneCfg("cqb.door_push_m", "tune_cqb_door_push_m", 1.8, 0.5, 4.0);
		DCO_TuneCfg("cqb.in_min_m", "tune_cqb_in_min_m", 2.0, 0.5, 6.0);
		DCO_TuneCfg("cqb.out_min_m", "tune_cqb_out_min_m", 5.0, 1.0, 12.0);
		DCO_TuneCfg("cqb.out_rush_s", "tune_cqb_out_rush_s", 3.0, 1.0, 8.0);
		DCO_TuneCfg("cqb.out_need_cover", "tune_cqb_out_need_cover", 1.0, 0.0, 1.0);
	}

	override DCO_EAIPersonality RollWeightedPersonality()
	{
		float a = DCO_Aggression();
		float calm = Math.Max(2.0 - a, 0.0);
		if (a > 1.0)
			calm = 1.0 / a;
		float cautious = m_fPersonalityWeightCautious * calm * calm;
		float aggressive = m_fPersonalityWeightAggressive * a * a;
		float reckless = m_fPersonalityWeightReckless * a * a * a;
		float total = m_fPersonalityWeightStandard + cautious + aggressive + reckless;
		if (total <= 0.0)
			return DCO_EAIPersonality.STANDARD;
		float roll = Math.RandomFloat(0.0, total);
		if (roll < m_fPersonalityWeightStandard)
			return DCO_EAIPersonality.STANDARD;
		roll -= m_fPersonalityWeightStandard;
		if (roll < cautious)
			return DCO_EAIPersonality.CAUTIOUS;
		roll -= cautious;
		if (roll < aggressive)
			return DCO_EAIPersonality.AGGRESSIVE;
		return DCO_EAIPersonality.RECKLESS;
	}

	float DCO_GT_PersonalityWeightStandard()			{ return m_fPersonalityWeightStandard; }
	void DCO_ST_PersonalityWeightStandard(float v)		{ m_fPersonalityWeightStandard = v; }

	float DCO_GT_PersonalityWeightCautious()			{ return m_fPersonalityWeightCautious; }
	void DCO_ST_PersonalityWeightCautious(float v)		{ m_fPersonalityWeightCautious = v; }

	float DCO_GT_PersonalityWeightAggressive()			{ return m_fPersonalityWeightAggressive; }
	void DCO_ST_PersonalityWeightAggressive(float v)		{ m_fPersonalityWeightAggressive = v; }

	float DCO_GT_PersonalityWeightReckless()			{ return m_fPersonalityWeightReckless; }
	void DCO_ST_PersonalityWeightReckless(float v)		{ m_fPersonalityWeightReckless = v; }

	bool DCO_GT_StragglerEnabled()			{ return m_bStragglerEnabled; }
	void DCO_ST_StragglerEnabled(bool v)		{ m_bStragglerEnabled = v; }

	float DCO_GT_StragglerTime()			{ return m_fStragglerTime; }
	void DCO_ST_StragglerTime(float v)		{ m_fStragglerTime = v; }

	float DCO_GT_StragglerStuckTime()			{ return m_fStragglerStuckTime; }
	void DCO_ST_StragglerStuckTime(float v)		{ m_fStragglerStuckTime = v; }
}
