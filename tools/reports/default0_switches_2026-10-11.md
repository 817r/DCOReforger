# Default-0 feature switches: master list (Auditor)

Generated 2026-10-11 17:16 (UTC+7) from DCO_TuneCfg registrations (default 0, range 0-1) plus 6 unregistered switches and 1 value switch. Policy (user, 2026-10-11): a switch that passes its ≤ 300 s functional run (WORKS) becomes default ON; "not reached" keeps 0.

| key | owner | functional run | verdict | flipped | migration | note |
|---|---|---|---|---|---|---|
| cr.RequireRadio | ? (old) | NO WORLD | no owner | n | n | stays 0 until an owner is found |
| ctrl.gun_exceptions | ? (old) | IND_CTRL_GUN | pending | n | n |  |
| obj.focus_shaping | ? (old) | NO WORLD | no owner | n | n | stays 0 until an owner is found |
| ph.AimWhileMovingToCover | ? (old) | NO WORLD | no owner | n | n | stays 0 until an owner is found |
| ph.CoverPreferBuilding | ? (old) | NO WORLD | no owner | n | n | stays 0 until an owner is found |
| ph.SuppressWhileMoving | ? (old) | NO WORLD | no owner | n | n | stays 0 until an owner is found |
| wf.CoverPreferBuilding | ? (old) | NO WORLD | no owner | n | n | stays 0 until an owner is found |
| cqb.lost_window_s | ? (value switch 0=off, range 0-120) | CQBLOST | pending | n | n | value: flip = pick a value |
| agg.bound_v2 | AI Design | PERF_STRESS_BOUND session_154406 (300 s) | WORKS: agg:bound_bid in 18 PERF samples (counted only in BoundChanceOf = v2 path), agg:bound_mg_half 4, DCO err 0 | y | y | migration added (s2) |
| agg.open_fwd | AI Design | OPENFWD1 (queued, new AI Design world) | pending | n | n |  |
| agg.postfrag | AI Design | PERF_STRESS_POSTFRAG session_130745 (300 s; 5 runs today = limit) | WORKS: agg_postfrag lines 22-53 per run, DCO err 0 | y | y |  |
| agg.retfire | AI Design | PERF_STRESS_RETFIRE session_130213 (300 s) | WORKS: agg:retfire_roll_ok 2, return_fire_unseen event U157 (rechecked exact), DCO err 0 | y | y |  |
| agg.usage_map | AI Design | PERF_STRESS_USAGE session_131841 (300 s) | WORKS: agg:usage_map counter 4.0/s at init (per-unit mapping), DCO err 0 | y | y | decision line aggr_unit is DEBUG only |
| cmd.retreat_thr | AI Design | CMD_RETREAT_THR session_151324 (300 s, run 3 of 3) | WORKS: remnant_thr grp=US_FT_1 units=2 peak=4 thr=2 + remnant_pull, itest PASS all_met, DCO err 0 | y | y |  |
| gren.ff_predict | AI Design | PERF_STRESS_GRENFF session_141502 (300 s) | WORKS: gren_ff_block 21 lines, gren:ff_block_pred 4 samples, DCO err 0 | y | y |  |
| learn.ind_freeze | AI Design | PERF_STRESS_ILEARNF | pending | n | n |  |
| learn.ind_on | AI Design | PERF_STRESS_ILEARN session_142604 = ILEARN1 (300 s) | WORKS: ilearn_event 29, ilearn_use 32, ilearn_summary start (test store), DCO err 0 | y | y |  |
| learn.ply_on | AI Design | NO WORLD | pending | n | n |  |
| opp.cluster_scan | AI Design | IND_OPP_CLUSTER session_143132 (300 s) | WORKS: opp_cluster_frag unit=U16 dist=25 centroid=3012,1923 (T+41.9), itest PASS, DCO err 0 | y | y |  |
| opp.distracted | AI Design | PERF_STRESS_O_DISTRACT session_134036 (300 s) | WORKS: opp:distracted_shot in 14 PERF samples, DCO err 0 | y | y |  |
| opp.exposed | AI Design | PERF_STRESS_O_EXPOSED session_132937 (300 s) | WORKS: opp:exposed_pick + exposed_fast_settle in 18 PERF samples, DCO err 0 | y | y |  |
| opp.loot | AI Design | IND_OPP_LOOT session_140811 (300 s) | WORKS: 6 opp_loot events (e.g. unit=U11 spare=2 corpse_d=6), itest PASS all_met, DCO err 0 | y | y, boot-tested (s2) | migration added (s2) |
| opp.patient | AI Design | PERF_STRESS_O_PATIENT session_135132 (300 s) | WORKS: opp:patient_move 12, opp:patient_hold 4 samples (rechecked exact), DCO err 0 | y | y |  |
| opp.reload | AI Design | PERF_STRESS_O_RELOAD session_140241 (300 s) | WORKS: opp_reload unit=U113 tgt=U49 dist=22 (T+134.6), opp:reload_rush 2, DCO err 0 | y | y, boot-tested (s2) | migration added (s2) |
| opp.suppressed_push | AI Design | IND_OPP_PUSH session_121350 (300 s) | WORKS: opp_push 13 lines (2 by MG assistant), itest PASS, DCO err 0 | y | y |  |
| radio.squad_chatter | AI Design | NO WORLD | pending | n | n |  |
| rearm.end_rel | AI Design | IND_REARM session_161451 = REARM1 (300 s) | WORKS: 6 rearm_start, 6 rearm_end result=taken (station, n=4), ASSERT rearm:enough_loadout PASS, itest PASS all_met, DCO err 0 | y | y | GM row added (AI Design) |
| thr.low_cluster | AI Design | THRLOW_ON session_121520 (3 runs) | KEPT 0: not triggered (my earlier hit was the TEST INTENT/ASSERT text) | n | n | condition never formed in 3 lane runs |
| cqb.door_avoid | BD | CQBKINDU1 (urban, running ~13:58) | pending: 0 in CQBKIND 134731 | n | n | signal cqb:door_shift / cqb_kind_move reason=door_shift |
| cqb.kind_beh | BD | CQBKIND_URBAN session_135834 (300 s) | WORKS: cqb_kind_move 6 lines, DCO err 0 | y | y, boot-tested (s2) |  |
| ff.vert | BD | PERF_STRESS_FFVERT session_134200 (300 s) | WORKS: ff_lane:vert_clear in 13 PERF samples, DCO err 0 | y | y, boot-tested (s2) | ready to flip (BD batch ~14:20) |
| gren.atk_close | BD | PERF_STRESS_GREN session_160209 = GREN2 (300 s) | WORKS: gren:atk_close_allow in 8 PERF samples, DCO err 0 | y | y | pre-check: fires (gren:atk_close_allow 26) but only in a 600 s run; GREN2 300 s needed |
| mb.mg_exempt | BD | PERF_STRESS_AGGMG session_105406 (300 s) | WORKS: mb:mg_exempt 0.7-2.9/s, DCO err 0 | y | y, boot-tested (s2) | GM row added (BD, float Def 1.0) |
| mb.v2 | BD | PERF_STRESS_AGGV2 session_133628 (300 s) | WORKS: mb:v2_eval in 16 PERF samples, mb:stay_no_gain 12, DCO err 0 | y | y, boot-tested (s2) | ready to flip (BD batch ~14:20); mb_pick to INFO in that batch |
| sa.hazard_skip | BD | SA_HAZARD session_153714 (300 s) | WORKS: sa_hazard_skip 3x (e.g. U11 sa_pos=2807,1724 hazard_d=2.9), 0 barbed-wire wounds, DCO err 0 | y | y | NEW 15:3x: skip vanilla smart-action points within sa.hazard_r (1.5 m) of barbed tape (SCR_SpecialCollisionHandlerComponent); root cause of OBS U19/U20 deaths; hazard name fixed 15:38 (ClassName:model) |
| tb.fm_gate | BD | PERF_STRESS_AGGGATE session_104833 (300 s) | WORKS: sfm_gate_wait 8, sfm_gate_timeout 1, DCO err 0 | y | y, boot-tested (s2) | GM row added (BD, float Def 1.0) |
| tb.mg_ow | BD | PERF_STRESS_AGGMG session_105406 (300 s) | WORKS: tb:mg_hold 1.1-3.7/s, DCO err 0 | y | y, boot-tested (s2) | GM row added (BD, float Def 1.0) |
| tgt.claim_pen | BD | - | value change (BD) | value | y, boot-tested (s2) | VALUE change 2.5 to 1.5 (not a switch) |
| cov.on | BD (orphan dco-ai-95) | PERF_STRESS_COV session_135304 (300 s) | WORKS: cover_pick 568 lines, cov:eval 12 samples (COV 135304), DCO err 0 | y | y, boot-tested (s2) | UNREGISTERED; ready to flip after BD registers it (BD batch ~14:20) |
| osc.fix_bound | BD (orphan dco-ai-95) | OSC42 (OSC4 + osc.fix_bound 1, 300 s) and OSC41 | KEPT 0: not reached (inv:bound_hold 0; every bound investigate ended by timeout, none arrived under osc.bound_min_s 4 s) | n | n | UNREGISTERED; needs a dedicated world |
| osc.fix_commit | BD (orphan dco-ai-95) | PERF_STRESS_OSC4 session_140411 (300 s) | WORKS: move_arbiter:commit_* 12 samples (decision only exists with the switch), DCO err 0 | y | y, boot-tested (s2) | UNREGISTERED; BD registers (TuneCfg default 0 + GM + help) before COV1/OSC41 |
| osc.fix_cover | BD (orphan dco-ai-95) | PERF_STRESS_OSC4 session_140411 (300 s) | WORKS: reloc_bhv:cover_* 33 samples, DCO err 0 | y | y, boot-tested (s2) | UNREGISTERED; BD registers (TuneCfg default 0 + GM + help) before COV1/OSC41 |
| osc.fix_dir | BD (orphan dco-ai-95) | PERF_STRESS_OSC4 session_140411 (300 s) | WORKS: osc:dir_new in 10 samples, DCO err 0 | y | y, boot-tested (s2) | UNREGISTERED; BD registers (TuneCfg default 0 + GM + help) before COV1/OSC41 |
| osc.fix_short | BD (orphan dco-ai-95) | PERF_STRESS_OSC4 session_140411 (300 s) | WORKS: reloc_bhv:dur_extend 18 samples, DCO err 0 | y | y, boot-tested (s2) | UNREGISTERED; BD registers (TuneCfg default 0 + GM + help) before COV1/OSC41 |
| ctx.cqb_gate | ctx | CTX_URBAN_ON_CQBG session_144712 (300 s) | WORKS: ctx:inv_cqb_gate_skip in 12 PERF samples, DCO err 0 | y | y | flipped 16:50, approved by user (per user to Tracker 16:55); OPEN GAP: no GM row (separate GM work) |
| ctx.hide_on | ctx | CTX_URBAN_ON_HIDE session_143818 (300 s) | WORKS: ctx:hide_band in 15 PERF samples, DCO err 0 | y | y | flipped 16:50, approved by user (per user to Tracker 16:55); OPEN GAP: no GM row (separate GM work) |
| ctx.mg_floor | ctx | PERF_STRESS_CTX_MGF session_111056 (300 s) | WORKS: ctx:mg_floor_suppress in 10 PERF samples, DCO err 0 | y | y | flipped 16:50, approved by user (per user to Tracker 16:55); OPEN GAP: no GM row (separate GM work) |
| ctx.on | ctx | CTX_OBSERVE,CTX_URBAN_ON_BLD2,CTX_URBAN_ON_CQBG,CTX_URBAN_ON_HEAL,CTX_URBAN_ON_HIDE,CTX_URBAN_ON_SUP,PERF_STRESS_CTX_MGF | WORKS: ctx:inv_ 28-38 per 300 s run (CTX_URBAN_ON_BLD2/SUP, CTX_MGF), DCO err 0 | y | y | flipped 16:50, approved by user (per user to Tracker 16:55); OPEN GAP: no GM row (separate GM work) |
| ctx.sup_on | ctx | CTX_URBAN_ON_SUP session_131433 (300 s) | WORKS: ctx:sup_window 18 samples, DCO err 0 | y | y | flipped 16:50, approved by user (per user to Tracker 16:55); OPEN GAP: no GM row (separate GM work) |
| dodge.cover_vs_shooter | ctx | DODGEA1 session_113317 + DODGE2 session_124012 (300 s) | WORKS: dodge:cover_compromised_ray/arc/hit; DODGE2 summary cover_compromised_ray 315, DCO err 0 | y | y | flipped 16:50, approved by user (per user to Tracker 16:55); OPEN GAP: no GM row (separate GM work) |
| dodge.dir_assist | ctx | DODGEC1 session_114415 + DODGE2 session_124012 (300 s) | WORKS: dodge:dir_los_break/dir_crawl/dir_crossfire; DODGE2 dir_los_break 247, DCO err 0 | y | y | flipped 16:50, approved by user (per user to Tracker 16:55); OPEN GAP: no GM row (separate GM work) |
| dodge.nearmiss_gate | ctx | DODGEB1 session_113846 + DODGE2 session_124012 (300 s) | WORKS: dodge:gate_nearmiss/gate_hit; DODGE2 gate_nearmiss2 318, DCO err 0 | y | y | flipped 16:50, approved by user (per user to Tracker 16:55); OPEN GAP: no GM row (separate GM work) |
| heal.unseen_gate | ctx | HEAL_UNSEEN session_144349 = HEALU1 (300 s cap, ended 203 s all_met) | WORKS: ASSERT PASS heal:unseen_gate actual=2 (met T+173), 8 heal_defer lines, itest PASS, DCO err 0 | y | y | flipped 16:50, approved by user (per user to Tracker 16:55); OPEN GAP: no GM row (separate GM work) |
| bc.ladder_slider | Realism | PERF_STRESS_ABL_C session_161838 = ABLC1 (300 s) | WORKS: bc:ladder_slider in 20 PERF samples, DCO err 0 | y | y (added by s2 16:52 after user approval via Tracker) | user approved Realism migration batch (Tracker 16:5x) |
| bc.shaken | Realism | PERF_STRESS_ABL_PANIC30_BCSH session_153836 = BCSH2 (300 s) | WORKS: break_contact_start grp=G18 why=self_DEFAULT_shaken (T+145.3), DCO err 0 | y | y (added by s2 16:52 after user approval via Tracker) | user approved Realism migration batch (Tracker 16:5x) |
| cm.dir_decode | Realism | PERF_STRESS_DIRDEC session_144555 (300 s) | WORKS: cm:req_dir_decoded in 18 PERF samples, DCO err 0 | y | y (added by s2 16:52 after user approval via Tracker) | user approved Realism migration batch (Tracker 16:5x) |
| cqb.precheck | Realism | PERF_STRESS_PRECHECK session_110526 (300 s) | WORKS: cqb_precheck 13 lines, DCO err 0 | y | y, boot-tested (s2) | GM row added 12:17 (Realism) |
| tm.atk_close_in | Realism | PERF_STRESS_ABL_CLOSEIN session_153114 (300 s) | WORKS: advance_gate why=close_in 29 lines (e.g. U33 enemy_d=28 min_d=12), DCO err 0 | y | y (added by s2 16:52 after user approval via Tracker) | user approved Realism migration batch (Tracker 16:5x) |
| tm.cover_first | Realism | IND_OPENCOVER_ON session_133059 | KEPT 0: tm:cover_first_hit/miss 0, not reached in 300 s | n | n | pre-check: no feature line found in its latest run (owner to confirm the log name or the run) |
| tm.open_rule_v2 | Realism | IND_OPENCOVER_ON session_133059 (300 s) | WORKS: open_rule action=prone_crawl 9 lines (v2 zone-0 path), DCO err 0 | y | y (user chose to keep it, via Tracker 16:5x) | entry originally added after my bypass; user decided to keep |
| tm.retfire_chance | Realism | NO WORLD | not a switch (chance value, Realism) | n | n | excluded from flips |
| bc.dogleg | Realism (C3 by Auditor) | PERF_STRESS_BC (603 s, before the 300 s rule) | NOT REACHED in 300 s (works at 603 s: turn 10, face 2) | n | n | stays 0 until a ≤300 s world forces early break contact |
| bc.face | Realism (C3 by Auditor) | PERF_STRESS_BC (603 s, before the 300 s rule) | NOT REACHED in 300 s (face 2 at T+500+) | n | n | stays 0 |
| agg.cmd_profile | Scripter | TAC_HAMMER_AGGP session_154731 = HAMAGGP1 (300 s, agg.k2=0) | WORKS (isolated): ASSERT PASS agg:p7_cmd actual=126 (met T+129.6), K2 off, DCO err 0 | y | y |  |
| agg.k2 | Scripter | OBJ_FORCE session_125806 (300 s) | WORKS: ASSERT cmd_aggr/obj_focus_pick/tactic_chosen PASS, itest PASS, DCO err 0 | y | y | migration added |
| air.on | Scripter | NO WORLD | pending | n | n |  |
| bc.no_retask | Scripter | NO WORLD | pending | n | n |  |
| cmd.aggr_tempo | Scripter | TAC_HAMMER_PUSH | pending | n | n |  |
| cqb.prefab_negcache | Scripter | NO WORLD | pending | n | n |  |
| def.crest_fan | Scripter | DEF_REVERSE | pending | n | n |  |
| def.fp_blind_fix | Scripter | VICTORY_TYPED,DEF_AREA_GAR,DEF_AREA_OBJTYPE,DEF_REVERSE | pending | n | n | owner per Scripter: BD/AI Design (defense fighting position) |
| def.obj_priority | Scripter | FM_Commander_CAPPRIO session_143454 (300 s) | WORKS: def_obj_order cmd=USSR_CMD first=Primary_Objective scores Primary 100 / Secondary 60 / Check_Point 40 (T+18.7), DCO err 0 | y | y | migration added |
| fatigue.contact_acc | Scripter | NO WORLD | pending | n | n |  |
| fm.auto | Scripter | PERF_STRESS_FMAUTO session_141054 (300 s) | WORKS: formation_set src=auto (e.g. G3 to=Wedge_W env=OPEN), 89 formation_set lines, DCO err 0 | y | y, boot-tested (s2) |  |
| garrison.arrive_start | Scripter | DEF_AREA_GAR session_132001 (300 s) | WORKS: garrison_arrive_start grp=G6 inside=2/4 waited_s=9 (T+110.9), DCO err 0 | y | y | GM row added 14:06 (DCO_T_CMD_GarArriveAttribute); migration existed |
| garrison.leftover_field | Scripter | IND_GAR_LEFTOVER session_164237 (300 s, run 3 of 3) | WORKS: garrison_leftover_field grp=G1 leftover=4 slots=4 assigned=4 (T+5), itest PASS | y | y | migration requested by Scripter from s2 ~16:49 (not refused) |
| hear.contact | Scripter | INDSPEC1 (queued) | pending | n | n |  |
| hear.est | Scripter | INDSPEC1 (queued) | pending | n | n |  |
| hear.mask | Scripter | INDSPEC1 (queued) | pending | n | n |  |
| hear.v2 | Scripter | INDSPEC1 (queued) | pending | n | n |  |
| intel perceived dedup | Scripter | - | pending | n | n | ALWAYS ON bug fix (user finding 5): a perceived contact counts only for the nearest objective; hold_no_eyes logged on change |
| intel.est_blend | Scripter | EXDE1 Example_D_FullMission_E session_153241 (300 s) | WORKS: est_update how=decay x4 (Primary 16.0->15.9, floor 8.0), DCO err 0 (blend_down/hold_no_eyes not reached: no contact by T+107) | y | y | migration requested by Scripter from s2 |
| is.designate | Scripter | INDSPEC1 (queued) | pending | n | n |  |
| is.est_share | Scripter | INDSPEC1 (queued) | pending | n | n |  |
| is.quality | Scripter | INDSPEC1 (queued) | pending | n | n |  |
| obj.phase_on | Scripter | OBJ_PHASE session_125226 (300 s) | WORKS: obj_phase/obj_phase_active, ASSERT obj_gate_wait PASS, DCO err 0 (tactic_chosen not reached in 300 s, same in OBJASSIGN1 without phases) | y | y, boot-tested (s2) | GM row + migration added 13:00, boot-tested |
| obj.role_on | Scripter | OBJ_ROLE session_153552 = OBJROLE1 (300 s) | WORKS: obj_role_block 10 lines (US_CMD_B, Primary_Objective role=defend_only, paths committed/attack), DCO err 0 | y | y | pre-check: no feature line found in its latest run (owner to confirm the log name or the run) |
| op.occupy_empty | Scripter | EXDE2 queued (world fixed) | pending: not triggered in EXDE1 | n | n |  |
| posture.release_exec | Scripter | TAC_HAMMER_F session_145820 = HAMF1 (300 s) | WORKS: posture:release_exec in 6 PERF samples, DCO err 0 | y | y |  |
| spec.bt | Scripter | PERF_STRESS_SPECBT session_141627 (300 s) | WORKS: spec_bt:start_* 30 samples, DCO err 0 | y | y, boot-tested (s2) |  |
| tac.fix_los | Scripter | TAC_SIEGE_V5 session_152015 (300 s) | WORKS (path only): fix_site_los 3x, tried=27 each, result=none_keep_old every time; DCO err 0 | y | y | a successful re-site never shown |
| tac.lost_contact_s floor | Scripter | - | pending | n | n | VALUE change: AggWait floor 90 to 30 s (only when the aggression tempo is not 1; before, the floor had no effect) |
| tac.mett_on | Scripter | SELDAY1 (queued) | pending | n | n |  |
| tac.pursuit_track | Scripter | PTRACK1 (queued) | pending | n | n |  |
| tac.push_keep_route | Scripter | TAC_HAMMER_PUSH | pending | n | n |  |
| tac.standoff_fix | Scripter | TAC_SIEGE_V5 | pending | n | n |  |
| veh.sbf | Scripter | TAC_SIEGE_VSBF | pending | n | n |  |
| wp.arrive_any | Scripter | TAC_HAMMER_PUSH | pending | n | n |  |
| wp.attack_arrive | Scripter | TAC_HAMMER_PUSH | pending | n | n |  |
| agg.pursue | Scripter 2 | PERF_STRESS_PURSUE session_120659 (300 s) | WORKS: pursue lines 164, agg_pursue relocate 15, DCO err 0 | y | y |  |
| grp.bc_not_free | Scripter 2 | THRBC_ON session_122048 (300 s) | WORKS: ASSERT PASS reserve:skip_breaking_contact actual=135 (met T+144.5), itest PASS, DCO err 0 | y | y |  |
| reinf.cap_substitute | Scripter 2 | NO WORLD | KEPT 0: not reachable in 300 s (earliest RECON capability request T+362) | n | n | post-release per policy |
| reinf.def_shortfall | Scripter 2 | DEF_SHORT session_115122 = DEFSHORT2 (300 s) | WORKS: def_shortfall 12, reinf_request 15, itest PASS, DCO err 0 | y | y | flipped before the list was built |
| thr.flank_rel | Scripter 2 | THRFLANK_ON session_114550 (300 s) | WORKS: threat_flank how=friend_axis, DCO err 0, itest PASS | y | y |  |
| thr.flank_retarget | Scripter 2 | THRFLANK_ON session_114550 (300 s) | WORKS: threat_flank_retarget moved_m=31, DCO err 0, itest PASS | y | y |  |
| thr.flank_route | Scripter 2 | THRFLANK_ON | KEPT 0: detour not triggered (owner AI Design reported) | n | n | pre-check: no feature line found in its latest run (owner to confirm the log name or the run) |

Already ON as bug fixes (WORKS EXD1/EXD2, not in this list): logi.low_rel, logi.extract_ineff, logi.cross_src.
