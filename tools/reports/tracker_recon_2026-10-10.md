# Tracker reconciliation vs current code (2026-10-10)

Read-only audit. Tracker: D:/Git/dco-web/data/items.json (last update 2026-10-06). Code: D:/Git/DCOReforger/DCO_AI/scripts/Game/DCO (+ AI/*.bt, Configs). Backup: D:/Git/DCO_backup_2026-10-06_2343.

Method: every token in each note (DCO_*/CMD_*/SCR_* names, CamelCase functions, m_* fields, snake_case log events, dotted tune keys) was searched in the current tree and in the backup (scratchpad scan.js / scan2.js / chk.js). For items with empty notes (1-80), I searched title-based keys and the DCO_EBehavior flag checks. "LOST" = found only in the backup, so it was removed by the 2026-10-06 revert and not restored. Git context: HEAD 7b0a172 (2026-09-29 11:06). Everything later was uncommitted, so every done item from 09-29 onward was at risk.

Scope: 146 items (all done/tested, all hold, plus the named items).

Key facts behind many rows:
- The TR framework (DCO_TacticRole*, DCO_TR_FireManeuver, DCO_TR_FMFix/FMAssault.bt, AIWaypoint_TacticRole.et, DCO_AssaultBehaviors) and the tactic partials DCO_CommanderTactics_01.._15 exist only in the backup. They were stripped on purpose on 10-07 (revert-2026-10-07-cqb.md). The current DCO_CommanderTactics.c is HEAD plus the 10-07..10-10 edits (Rechoose, DCO_AssaultDrive, consolidate, push_keep_route).
- The 10-07 restores brought back Commander/**, the Spawner, the foundations, the Group drills, the optimisations (CoverManager, BldCache, Registry), TuneAttributes and ROE. These were NOT restored: DCO_AIMarkers.c, Modded_CheckLowAmmo.c, Modded_HealActivity.c, the BTFIX BT cleanup, world/Old + world/Test category folders.
- Two DCO_EBehavior flags exist but are never read: TACTIC_REEVAL (Global/DCO_GlobalAI.c:19) and CONTACT_SHARE_OPT (DCO_GlobalAI.c:18). Every other behavior flag has a real check outside GlobalAI/GlobalLOD.

## Table

| code | current status | verdict | evidence | proposed status | reason |
|---|---|---|---|---|---|
| 1 | done | PRESENT | DCO_MedicDispatcher (Medic/DCO_MedicDispatcher.c, lender logic; DCO_Battle.c:450 ref) | done | cross-group medic dispatcher in tree |
| 2 | done | PRESENT | DCO_EGroupCapability (DCO_BenchmarkLogger.c:319), DCO_EGroupTask/State (706/96 refs) | done | role refactor enums in use |
| 3 | done | PRESENT | DCO_PlayerTasking (AICommanderBase.c:797), player_eta DCO_PlayerTasking.c:806, DCO_PlayerRequests | done | |
| 4 | done | UNCLEAR | DCO_TerrainCache (AICommanderManager.c:634, 29 refs); not found: a distinct "morale tasking" key | done | terrain cache present; morale-tasking half has no identifiable name |
| 5 | done | PRESENT | Commander/Logistics/DCO_Logistics.c, DCO_LogisticsHub.c | done | |
| 6 | done | PRESENT | CMD_ThreatResponseComponent (34 files), Support/DCO_ThreatDecision.c | done | |
| 7 | done | PRESENT | DCO_CommanderOps (AICommanderBase.c:863), DCO_ArmorPair (Staging.c:250) | done | |
| 8 | done | PRESENT | def_ambush DCO_CommanderDefense.c:270, def_cb_fire :1175, feint (DoctrinePresets.conf:17) | done | |
| 9 | hold | n/a | world/CommanderFullScale.ent still exists | hold | user-parked |
| 10 | done | PRESENT | DCO_ContactSharing (Group/DCO_ContactSharing.c) | done | |
| 10b | done | PRESENT | DCO_GlobalConfig JSON; BD report 10-10: 454 settings read, unused=0 | done | |
| 11 | done | PRESENT | DCO_Strength (55 refs), overmatch; not found: "mortar_tier" key | done | strength/overmatch present; tier name not identifiable |
| 12 | done | PRESENT | DCO_Perf (1547 refs) | done | |
| 13 | done | PRESENT | rebuilt 10-07: Group/DCO_CQBActivity.c + Nav/DCO_INavCQBPlan (CQBActivity.c:28), Do CQB WP docqb_wp DCO_DoCQBWaypoint.c:56, cqb_start DCO_ClearBuilding.c | done | new CQB implementation replaces the old one; note should say so |
| 14 | done | PARTIAL | LOST: ReleasePlanGroups, main_staging, pursuit_skip, objective_cleared (backup Tactics_10/_15/_07). Re-done 10-07+: Rechoose DCO_CommanderTactics.c:1249, DCO_AssaultDrive :1202, tactic_ready AICommanderBase_13_Assignment.c:284, DCO_LiveSuppressVolume | tested | 10-05 note text describes deleted code; the 10-07..10-10 equivalents ran (memory commander-tactic-exec: 8 SBF tactics PASS >=1 run 10-08; PINCER 0/2 on 10-10). Rewrite the note |
| 15 | done | PRESENT | DCO_Night (45 refs), flare | done | |
| 16 | done | PRESENT | m_bAutoDetectDone DCO_GroupUtility.c:993, transport_team_demoted :1528 | done | |
| 17 | done | PRESENT | DCO_CommanderRadialCommand (Commands.conf:321), AssignRole AICommanderBase_13_Assignment.c:3 | done | |
| 18 | done | PRESENT | DCO_CommanderRadialCommand, CommandingMenu.conf arty/illum entries; fire_support token only in backup Tactics_11 | done | radial support entries present |
| 19 | done | PRESENT | DCO_RadioBank.conf, DCO_Radio (1663 refs) | done | |
| 20 | done | PRESENT | DCO_GroupContactReport (Group_Base.et:6) | done | |
| 21 | done | PRESENT | arty_warn CommanderArtillerySupport.c:359, WarnIncoming DCO_PlayerAwareness.c:226 | done | |
| 22 | done | PRESENT | DCO_ContactToast (DCO_BriefUI.c:351) | done | |
| 23 | done | PRESENT | DCO_CommanderSpawner (restored 10-07) | done | |
| 24 | done | PRESENT | budget in Spawner/DCO_CommanderSpawner.c, DCO_GroupPresets | done | |
| 25 | done | PRESENT | Commander/Group/DCO_DedicatedTransport.c (+Tune) | done | |
| 26 | done | PRESENT | illum handling (34 refs, CommandingMenu.conf:89) | done | |
| 27 | tested | PRESENT | static const count 57 now (70 in backup); no new static init pressure | tested | |
| 28 | done | PRESENT | RequestTransport (CommandingMenu.conf:110, 4 files) | done | |
| 29 | done | PRESENT | ShellCount (Commands.conf:560, 7 files) | done | |
| 30 | tested | PRESENT | m_eLODMode default "0" DCO_GlobalAI.c:140; DCO_GroupUtility.c:134 DCO_ApplyLOD; GM/DCO_GlobalLOD.c:2 | tested | |
| 31 | done | PRESENT | PerfSection DCO_Census.c:192 | done | |
| 32 | tested | PRESENT | Benchmark/DCO_Census.c | tested | |
| 33 | tested | PRESENT | same as 27 | tested | |
| 34 | tested | PRESENT | Global/DCO_Util.c (1040 refs) | tested | |
| 35 | tested | PRESENT | throwaway tool absent as intended | tested | |
| 36 | tested | PRESENT | Benchmark/DCO_LeanKeeper.c:1, SetWantedLeaning Modded_CombatComponent.c:485 | tested | |
| 37 | done | PRESENT | bool HasRadio DCO_GroupUtility.c:984 | done | |
| 38 | tested | PRESENT | lean test tool/log absent as intended | tested | |
| 39 | done | PRESENT | DCO_EBehavior.TARGET_PRIORITY checked in Modded_CombatComponent.c | done | |
| 40 | done | PRESENT | RELOAD_DISCIPLINE checked in Modded_CombatComponent.c | done | |
| 41 | done | PRESENT | PISTOL_CQB checked in Modded_CombatComponent.c | done | |
| 42 | done | PRESENT | SECURITY_360 checked in Modded_GroupUtilityComponent.c | done | |
| 43 | done | PRESENT | Group/DCO_Hearing.c, HEARING flag checked | done | |
| 44 | done | PRESENT | buddy_callout Modded_CombatComponent.c:1694 | done | |
| 45 | done | PRESENT | window_fire Modded_CombatComponent.c:1828, WINDOW_FIRE flag | done | |
| 46 | done | PRESENT | mg_bof DCO_Specialists.c:487, MG_GUNNER DCO_AttackPush.c:81, DCO_MGPosition.c | done | |
| 47 | done | PRESENT | AT_SPECIALIST checked in DCO_Specialists.c | done | |
| 48 | done | PRESENT | MARKSMAN checked in DCO_Specialists.c | done | |
| 49 | done | PRESENT | LEADER_RTO_GRENADIER checked in Modded_GroupUtilityComponent.c | done | |
| 50 | done | PARTIAL | flag CONTACT_SHARE_OPT DCO_GlobalAI.c:18 never read (also unread in backup); fire_est:share_hop_limit DCO_ContactSharing.c:338, share_queue_full | done | optimisation limits exist, the GM toggle is dead (pre-revert too) |
| 51 | done | MISSING | flag TACTIC_REEVAL DCO_GlobalAI.c:19 never read; tactic_reeval/hold_reinforce LOST (backup Tactics_13_Planning.c:113/126; flag read only in backup Tactics_14_Bounding.c). Only standoff escalate -> Rechoose remains (DCO_CommanderTactics.c:1380) | queued | mid-tactic re-eval was in the deleted partials; AUDITB3 F1 builds on it |
| 52 | done | PRESENT | Group/DCO_BreakContact.c, break_contact_start :94 | done | |
| 53 | done | PRESENT | Group/DCO_AmbushDrill.c | done | |
| 54 | done | PRESENT | Group/DCO_ContactSOP.c | done | |
| 55 | done | PRESENT | DCO_AIRearmBehavior DCO_Rearm.c:93, DCO_Rearm.bt | done | |
| 56 | done | PRESENT | Individual/DCO_Recovery.c | done | |
| 57 | done | PRESENT | Danger/DCO_Explosion.c, EXPLOSION_REACTIONS flag | done | |
| 58 | done | PRESENT | DCO_GrenadeUtility (35 refs) | done | |
| PT | done | UNCLEAR | LOD vanilla default present (row 30); not found: a turret-owner or get-in fix identifier | done | no identifiable names for the turret-owner/get-in parts |
| TIC | done | PRESENT | IN_CONTACT gate AICommanderBase_07_Transport.c:148, transport_skip :156 | done | |
| 59 | tested | PRESENT | voice_try/voice_heard DCO_Census.c:415, Group/DCO_VoiceReact.c | tested | |
| 60 | done | PRESENT | LEAN_PEEK flag in Modded_CombatComponent.c, lean:cqb DCO_Census.c:416 | done | |
| 61 | tested | PRESENT | sop:no_bypass_obj DCO_ContactSOP.c:199 | tested | |
| 62 | done | PRESENT | TryCloseFrag Modded_UpdateAttackData.c:213 | done | |
| 63 | done | PRESENT | reattack watchdog DCO_Census.c:411 | done | |
| 64 | done | PRESENT | Commander/DCO_CommanderPresets.c | done | |
| 65 | tested | PRESENT | m_DCOConfig guards (61 refs) | tested | |
| 66 | done | PRESENT | DCO_VoiceReact, PLAYER_VOICE flag in DCO_Hearing.c | done | |
| 67 | done | PRESENT | push_no_at DCO_Census.c:438, DCO_VehicleCombat | done | |
| 68 | done | PRESENT | duel DCO_Census.c:440 | done | |
| 69 | done | PRESENT | Individual/DCO_GunnerBehavior.c:11, Vehicle/Modded_CombatMoveVehicleGunner.c | done | |
| 70 | done | PRESENT | Group/DCO_AntiArmor.c:8 | done | |
| 71 | tested | PRESENT | Group/DCO_MountedOps.c | tested | |
| 72 | done | PRESENT | Group/DCO_ArmorPair.c | done | |
| 73 | done | PRESENT | SUPPRESS_LAST_KNOWN, ASSAULT_FIRE, REACTION_TIME flags checked (Modded_CombatComponent / _Attack / AimErrorOffset) | done | |
| 74 | done | PRESENT | DCO_CommanderEval (Commands.conf:190, Group/DCO_CommanderEval.c) | done | |
| 75 | done | PRESENT | DCO_AttackPush, Group/DCO_PostureOps.c | done | |
| 75E | done | PRESENT | Individual/DCO_MoraleSystem.c, DCO_MoraleCombatUtility.c | done | |
| 76 | done | UNCLEAR | HOLD_FIRE (Commands.conf:169, 12 files), burst; no item-specific log key | done | fire-tree fix not separately identifiable |
| 77 | done | PRESENT | back_ok/back_fail DCO_Census.c:467, ProneGuard Modded_CombatMoveState.c:295, PRONE_MOVE flag | done | |
| 78 | done | PRESENT | Group/DCO_PinnedGroup.c, behPinned DCO_GlobalAI.c:759, PINNED flag | done | |
| SER | done | PRESENT | DCO_ObjectiveSerializer.c:1, GetDisplayName/GetLogName, obj_name_fallback AICommanderObjective.c:1503 | done | |
| 79 | tested | PRESENT | threat_v DCO_BenchmarkLogger.c:103, SCR_SetCombatModeGroupCommand Commands.conf:163 | tested | |
| 80 | done | PRESENT | Group/DCO_Awareness.c, Group/DCO_Statics.c, AWARENESS (15 refs) + COMBINED_ARMS checks | done | |
| SUPBOX | done | PRESENT | SCR_AISuppressionVolumeClusterBox.SetClusterState Modded_SuppressionVolume.c:48-50 | done | |
| ASL | hold | MISSING | folded into TR1; TR stripped (see TR1) | drop | parent TR framework deliberately removed |
| TR1 | fix | MISSING | LOST: DCO_TacticRoleActivity, DCO_TR_FMFix.bt, DCO_TR_FMAssault.bt, AIWaypoint_TacticRole.et, DCO_WP_TacticRole.bt, tr_fix_pos/tr_ready/tr_commit (all backup only); test world DCO_T_TR1_FireManeuver not found | drop | TR stripped on purpose 10-07 (revert-2026-10-07-cqb); tactics to be redesigned |
| 81 | done | PRESENT | vanilla Prefabs/AI/Waypoints/* defaults AICommanderBase.c:16, SpawnMoveWP AICommanderBase_02_Waypoints.c:83 | done | |
| WPNE | done | PRESENT | same as 81 | done | duplicate of 81, could be merged |
| CMDATTR | done | PRESENT | m_sDCOGarrisonWpOverride AICommanderBase.c:1143, "Commander 00" category :4 | done | |
| GMCARD | done | PRESENT | DCO_GroupTaskTooltipDetail GM/DCO_GroupTooltip.c:157, Doing row DCO_StatsCard.c:44 | done | |
| GMPATH | done | PRESENT | OnFrameBase DCO_StatsCard.c:163, SCR_BaseEditableEntityFilter DCO_CmdViewWorld.c:465 | done | |
| BENCHWB | tested | PARTIAL | ApplyWorkbenchDefaults DCO_GlobalAI.c:397 forces perf+census only when DCO_TestCaseComponent.RunOn(), not in every WB run | done | behaviour changed (test runs only); update note |
| GMTOOLBAR | tested | PRESENT | Configs/Editor/ActionLists/Toolbar/EditToolbar.conf, UI/layouts/Editor/Toolbar/DCO_CommandMenu_*.layout (restored 10-07) | tested | |
| TESTLOC | done | PARTIAL | not found: DCO_FMTest world; Arleville tests now in world/Test/Commander/* on DefaultGMTest | done | rule still applies; the named world is gone |
| WORLDORG | done | MISSING | world/Old and world/Test/{Group,...} folders absent; DCO_TestLab, testing*, LostSoap, CommanderFullScale back at world/ root | drop | revert undid the move; world/Test/{Commander,Individual,Perf} is the new layout |
| SBFHOLD | done | PRESENT | ResolveMoveRequestMovePosAndDir Modded_CombatMoveLogic_Attack.c:762, MoveToNextPosCondition DCO_AttackPush.c:84 | done | |
| PRONEOSC | done | PRESENT | waitTime *= 2.5 Modded_CombatMoveLogic_Suppressive.c:62 | done | |
| STRAGGLER | done | PRESENT | Group/DCO_StragglerWatch.c:14, straggler:* counters :185 | done | |
| PLAYEROBJ | done | PRESENT | DCO_CheckStaleAssigned + HasActiveTask AICommanderBase_10_AttackControl.c:47-49 | done | |
| DOCTRINE | done | PRESENT | Commander/Doctrine/* (DCO_Doctrine, BattlePlan, Force, ForceCondition, Personality), DCO_DoctrinePresets.conf; LOST only RaidHvtBonus (backup Tactics_01) | done | |
| FFMG | tested | PRESENT | superseded: DCO_Util.FireLaneCheck DCO_Util.c:274 called from Modded_UpdateAttackData/UpdateSuppressionData/SuppressBehavior; ff_lane:* counters. HasSquadmateInFireLine (:249) has 0 callers, ff_line_block not found | tested | protection exists under new names; note should name FireLaneCheck |
| DEFBRIEF | done | PRESENT | DCO_DefenseTactics (10 refs), class DCO_Briefing DCO_Briefing.c:55, DCO_BriefUI, DCO_T_DEF_* worlds | done | no PASS ledger found; DEF runs exist (auditor 10-10) |
| BLDPUSH | done | PRESENT | DCO_ScanBuildingContacts/DCO_CQBTaskAllows Modded_GroupUtilityComponent.c:1382/1393; LOST tr_activity only | done | |
| POSTURE | done | PARTIAL | DCO_GetPosture/DCO_BasePosture/DCO_LadderIndex present; LOST TR rows (DCO_TacticRoleActivity, tr_assault_commit, CommitFireManeuver) | done | TR-phase part gone with TR; table itself works |
| PERFOPT | progress | PRESENT | O1 bld:hit DCO_CoverManager.c:141; O2 shot_merged DCO_DangerWeaponFired.c:603; O4 lean:cache_hit Modded_CombatComponent.c:530; O7 ag_callq Modded_UtilityComponent.c:37. not found: gp_enemy (grp_posture split), cmd_assign (O6 held) | done | memory ai-performance: stress test 24%->7.7%, spikes 223->4; minor gp_* split missing |
| ONOFF | hold | n/a | cancelled by user 2026-09-29 | drop | cancelled, not on hold |
| ITEST | queued | PARTIAL | class DCO_TestCaseComponent DCO_TestCase.c:39, Prefabs/Test/DCO_TestCase.et, ~100 worlds in world/Test/{Commander,Commander/Tactics,Individual,Perf}; not found: pilot DCO_T_TR1_FireManeuver, Group/ category folders | progress | framework built and in daily use; per-feature coverage incomplete |
| COHESION | done | PRESENT | OutsideCombatCohesion DCO_Util.c:161, CombatCohesionDist :142 | done | |
| WPNOISE | tested | PRESENT | SpawnMoveRoute AICommanderBase_02_Waypoints.c:90, TrySendToStaging _09_Staging.c:3 | tested | |
| IDLE | done | PRESENT | wp_idle Modded_GroupUtilityComponent.c:59 | done | |
| SMOKEBLD | tested | PRESENT | smoke_indoor_block Modded_ThrowGrenadeTo.c:28 | tested | |
| ITESTNOTE | tested | PRESENT | m_sIntent DCO_TestCase.c:52 | tested | |
| PRONECHANCE | tested | PRESENT | m_fProneChance DCO_GlobalAI.c:105, prone_blocked DCO_CQC.c:81 | tested | |
| OFFERUI | done | PRESENT | DCO_OfferCard, player_offer_sent DCO_PlayerTasking.c:1332, DCO_AcceptOffer input | done | |
| SHARENOTIFY | done | PRESENT | m_bShareToPlayers default "0" DCO_GlobalAI.c:312 | done | |
| FMIMPROVE | fix | MISSING | LOST: tr_prep, tr_sweep_pos, tr_assault_through, tr_stall, tr_refix, FindLosSpot (all backup DCO_TR_FireManeuver.c) | drop | built on TR; superseded by 10-07..10-10 tactic exec (AssaultDrive, consolidate, push) |
| BLDCACHE | progress | PRESENT | DCO_BldCache, DCO_BldLearn, bldcache_save DCO_GarrisonRegistry.c:533, slot_episode; 10-05 fixes: despawn DCO_BldLearn.c:419, tombstones in DCO_BldCache.c | done | code complete; first verification run (end=despawned, no mass prune) still pending |
| CMDLEARN | done | PRESENT | ApplyTactics/TacticEpisode called DCO_CommanderTactics.c:267/371; CmdLearn in Ops (9) + Defense (8); CmdLearn.Terrain callers 4 (6 in backup), PickAttackPos LOST | done | some staging/overwatch learning hooks lost with partials |
| GMMENU | done | PRESENT | DCO_Cat_* categories (40 confs, TempEdit.conf:58) | done | |
| MAPMARKCOMPAT | hold | n/a | DCO_MapOverlay, DCO_PlayerContactReports present | hold | waiting for the user's repro |
| LEARNDEFENSE | done | PRESENT | Commander/Learn/DCO_LearnDefense.c (28 refs), DCO_SectorGarrison | done | memory commander-audit 10-06: LEARNDEFENSE never runs for defensive commanders, so it may need fix after a run |
| DCOSETTINGS | done | PRESENT | Settings/* kept by revert; DCO_SettingsDialogUI (chimeraMenus.conf:6) | done | |
| DCOLOG | done | PRESENT | Log/DCO_Log.c, DCO_ELogChannel/Level, DCO_Report | done | |
| SETHELP | done | PRESENT | DCO_SettingsHelp/Glossary/Simple/Meters.conf | done | |
| SHOTFIX | done | PRESENT | DCO_ShotCounter, RegisterScriptHandler DCO_BenchmarkLogger.c:867, const_field :774 | done | |
| FMFIX | fix | PARTIAL | A (fire-line safety) present: FireLaneCheck DCO_Util.c:274, reach len+20, all groups of the same commander. B-D LOST: tr_attack_pos, offset_deg, tr_leg_check, tr_go, DCO_TRFireBehavior (backup Tactics_08/_09, DCO_TR_FireManeuver.c) | queued | A done; B (flank offset/attack position), C (GO gate/assault-through), D (composite abort) need re-spec on HEAD tactics |
| DCOSETPOLISH | done | PRESENT | DCO_SettingsMembers DCO_SettingsManager.c:631, DCO_RpcApplyMembers :848 | done | |
| DCOSETREAD | done | PRESENT | DCO_SettingsChooserUI (chimeraMenus.conf:11), DCO_ChooseVanilla/DCO inputs | done | |
| CONSOLEUI | done | PRESENT | DCO_SetUp/Down/Left/Right/Enter inputs chimeraInputCommon.conf:114-198 | done | |
| SETMETERS | done | PRESENT | DCO_SettingsMeters.conf, DCO_MeterState DCO_SettingsDialogUI.c:5037 | done | |
| AUDITB1 | done | PRESENT | rej_friendly, threat_pick, team_regroup, idf_stay, armor_fear_scaled, prone_crawl_uncapped, window_slot_pos, TryCloseFrag, FriendlyInBlast all found; LOST only m_mTeamOf (TR file) | done | |
| PRESETSPLIT | done | PRESENT | set_preset_note_group/unit DCO_RadioBank.conf:3218/3226, dco_preset_migrate DCO_SettingsPresets.c:114 | done | |
| AUDITB2 | queued | PARTIAL | F1: bc.no_retask DCO_GroupUtility.c:1641 (default 0), bc.dogleg PlanDogleg DCO_BreakContact.c:223, bc.face :211 (default 0, untested). not found: break_contact_hold_retask, at_backblast_block/indoor_rpg (F2), evasive_at50 (F3), displace_wait_partner (T1), mortar_adjust (T2), share delays (T4) | progress | only F1 started, behind switches; F2-F3 and T1-T4 not started |
| AUDITB3 | queued | MISSING | not found: shift_main_effort, reinforce_mass_only, pursuit_end, tic_exit_killzone, dead_ground; hold_reinforce/tactic_reeval LOST | queued | not started |
| AUDITB4 | queued | MISSING | ComputeSafeRadius unchanged (CommanderArtillerySupport.c:481, max(safe, spread+lethal), no RED); not found: danger_close_approved, def_ambush_site_reject, def_cb_azimuth, threat_decay, site_score (danger_close only in help .conf) | queued | not started |
| AUDITB5 | queued | MISSING | not found: enemy_mortar_heard, push_open, duel_flank_overwatch, veh_dismount_point, pickup_exposed_warning; no armor ITEST world | queued | not started |
| PRESETHELP | queued | MISSING | not found: a "Presets" help topic or a "preset" glossary term in DCO_SettingsHelp.conf / DCO_SettingsGlossary.conf | queued | not started |
| DOCTRINEX | queued | MISSING | not found: doctrine_initiative, recon_mode, enemy_condition_react, assault_mode, defence_ready; only label second_echelon_shift for ussr_depth DCO_OpPlan.c:990 (op branch, not X5) | queued | not started |
| AUDITLOG | done | PARTIAL | DCO_Log levels + DCO_TestCase log level present; LOST 50 TR proof lines (tr_wp_arrive, tr_fix_arrive, tr_fireline, tr_fix_los ...) | done | F&M proof lines went with TR; logger infra intact |
| AIMARKERS | done | MISSING | only field m_iAIMapMarkers DCO_GlobalAI.c:310 + getter (0 callers). LOST: DCO_AIMarkers.c (modded marking system, ai_marker logs), DCO_AIMapMarkersAttribute (backup Edit.conf:5607) | queued | setting has no effect; restore DCO_AIMarkers.c from backup (needs user OK) |
| AUDITLOG2 | done | PRESENT | DCO_Battle.c (battle.json), death reasons, DCO_Roles; LOST only DCO_TacticRole ref | done | |
| BTFIX | done | PARTIAL | H1 behaviour present (DCO_AIRearmBehavior, DCO_AIRearmTake DCO_Rearm.bt:30); H2 idle_cover DCO_IdleNodes.c:25; H3 reloc_start Modded_CombatMoveState.c:166; H4 DCO_IdleTakeTurrets.bt. Reverted: HandleAmmo.bt override + HandleCombatMoveRessuplying branch (HandleCombatMove.bt:256), old subtrees + H1-5 scripts back; ammo_low logger LOST (Modded_CheckLowAmmo.c) | fix | H1-1/3/4/5 cleanup and H1-L1 log need redoing |
| AIMFIX | done | PRESENT | DCO_AimUtility, DCO_IsBraced Modded_CombatComponent.c:2076, DCO_SetAimDebug Modded_AimErrorOffset.c:323 | done | |
| INDBT | progress | PARTIAL | DCO_BoundBehavior/aimed_yield, DCO_RelocateBehavior + DCO_Relocate.bt, DCO_CQBMemberBehavior, spec_* logs present. LOST: heal_defer/heal_resume (Modded_HealActivity.c); DCO_OverwatchBehavior/DCO_TRFireBehavior (TR, intended) | progress | restore heal deferral; rest done (TEST_RESULTS runs 7-9 PASS were pre-revert) |

## Verdict counts (146 items)

- PRESENT 118
- PARTIAL 11: 14, 50, BENCHWB, TESTLOC, POSTURE, ITEST, FMFIX, AUDITB2, AUDITLOG, BTFIX, INDBT
- MISSING 11: 51, ASL, TR1, WORLDORG, FMIMPROVE, AUDITB3, AUDITB4, AUDITB5, PRESETHELP, DOCTRINEX, AIMARKERS
- UNCLEAR 3: 4, PT, 76
- n/a 3 (hold/cancelled): 9, ONOFF, MAPMARKCOMPAT

Proposed status changes: 14 done->tested; 51 done->queued; AIMARKERS done->queued; BTFIX done->fix; BENCHWB tested->done; WORLDORG done->drop; TR1 fix->drop; FMIMPROVE fix->drop; FMFIX fix->queued; ASL hold->drop; ONOFF hold->drop; PERFOPT progress->done; BLDCACHE progress->done; ITEST queued->progress; AUDITB2 queued->progress. All other rows keep their status (some notes need text updates: 13, 14, FFMG, BENCHWB, TESTLOC).

## Most important mismatches (marked done/tested, but missing or broken in code)

1. AIMARKERS (done): the GM/JSON setting is stored in DCO_GlobalAI but nothing reads it. DCO_AIMarkers.c (modded SCR_AIEnemyMarkingSystem) and its GM attribute exist only in the backup. Turning markers off does nothing.
2. 51 Mid-tactic re-evaluation (done): the TACTIC_REEVAL flag is never read. tactic_reeval and hold_reinforce lived in the deleted tactic partials. Only the standoff escalate -> Rechoose path remains.
3. BTFIX (done): the BT cleanup was reverted. The old HandleAmmo.bt override, the HandleCombatMoveRessuplying branch and the old resupply subtrees and scripts are back, and the ammo_low logger (Modded_CheckLowAmmo.c) is gone. The new DCO_AIRearmBehavior now runs next to the old path.
4. WORLDORG (done): the world folder layout was reverted. world/Old does not exist, and DCO_TestLab, testing*, LostSoap and CommanderFullScale sit at the world/ root again.
5. 14 Tactic library (done): the note describes the 10-05 code (ReleasePlanGroups, main_staging, Infiltration main_assault, pursuit_skip, objective_cleared), and all of it is deleted. Equivalent behaviour was re-implemented 10-07..10-10 under other names, and the note needs rewriting.
6. INDBT (progress): the heal_defer / heal_resume group heal deferral (Modded_HealActivity.c) was lost. The TEST_RESULTS run 5-8 fixes that relied on it are no longer in the code.
7. BENCHWB (tested): perf + census are now forced on only for test-case runs (DCO_TestCaseComponent.RunOn), not for every Workbench run. The tracker text is outdated.
8. FFMG (tested): the function named in the note (HasSquadmateInFireLine) is orphaned, with 0 callers, and the counter ff_line_block no longer exists. The protection now lives in DCO_Util.FireLaneCheck (ff_lane:*), so the behaviour is present but the note is stale.
9. TR1 / FMIMPROVE / ASL (fix/fix/hold): the whole TR framework is gone by design (10-07). These should be dropped rather than left as "fix".
10. Items that should move forward: ITEST queued -> progress (the framework and about 100 worlds exist), PERFOPT progress -> done, BLDCACHE progress -> done (verification run pending), AUDITB2 queued -> progress (F1 behind bc.no_retask, bc.dogleg and bc.face, default 0, untested).
