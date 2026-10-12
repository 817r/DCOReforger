class DCO_GlobalAIComponentClass: ScriptComponentClass
{
}

enum DCO_EBehavior
{
	TARGET_PRIORITY = 1,
	RELOAD_DISCIPLINE = 2,
	PISTOL_CQB = 4,
	SECURITY_360 = 8,
	HEARING = 16,
	BUDDY_CALLOUT = 32,
	WINDOW_FIRE = 64,
	MG_GUNNER = 128,
	AT_SPECIALIST = 256,
	MARKSMAN = 512,
	LEADER_RTO_GRENADIER = 1024,
	CONTACT_SHARE_OPT = 2048,
	TACTIC_REEVAL = 4096,
	BREAK_CONTACT = 8192,
	AMBUSH_DRILL = 16384,
	CONTACT_SOP = 32768,
	REARM = 65536,
	SPECIALIST_RECOVERY = 131072,
	REARM_ENEMY_SOURCES = 262144,
	ENEMY_WEAPONS = 524288,
	EXPLOSION_REACTIONS = 1048576,
	PLAYER_VOICE = 2097152,
	LEAN_PEEK = 4194304,
	SUPPRESS_LAST_KNOWN = 8388608,
	ASSAULT_FIRE = 16777216,
	REACTION_TIME = 33554432,
	PRONE_MOVE = 67108864,
	PINNED = 134217728,
	AWARENESS = 268435456,
	COMBINED_ARMS = 536870912,
	ASSAULT_ACTIVITY = 1073741824
}

enum DCO_EDisembarkRule
{
	AUTO,
	DISABLED_ONLY,
	ORDERS_ONLY,
	NEVER
}

enum DCO_EVehicleExit
{
	ORDERED = 1,
	DAMAGED = 2,
	PASSENGERS = 4,
	UNARMED_DRIVERS = 8,
	COMBAT_CREW = 16
}

enum DCO_EAILODMode
{
	VANILLA,
	PREVENT_MAX_LOD,
	FULL_DETAIL
}

class DCO_GlobalAIComponent: ScriptComponent
{
	[Attribute("1.0", UIWidgets.Slider, "Aim Accuracy: divides aim error at all ranges. 1.0 = DCO baseline. Effective divisor = value / 4.6 (4.6 = old realistic, 2.0 = 2026-10-05 baseline)", "0.1 10 0.1")]
	protected float unitAimSkillAccuracy;

	[Attribute( defvalue: "1", uiwidget: UIWidgets.Slider, desc: "Aim Speed: divides aim settle time. 1.0 = realistic", params: "0.25 4 0.01" )]
	protected float m_fAimSpeed;

	[Attribute( defvalue: "1", uiwidget: UIWidgets.Slider, desc: "Aim Shake: reduces posture and fatigue sway. 1.0 = realistic", params: "0.25 4 0.01" )]
	protected float m_fAimShake;


	[Attribute( defvalue: "1.5", uiwidget: UIWidgets.Slider, desc: "Unit Perception", params: "0.5 5 0.01" )]
	protected float m_fAiPerception;

	[Attribute( defvalue: "0", uiwidget: UIWidgets.Auto, desc: "Magical Ammo")]
	protected bool m_bIsMagicallyResupplied;

	[Attribute( defvalue: "700", uiwidget: UIWidgets.Slider, desc: "Unit Perception", params: "0 2000 0.01" )]
	protected float m_fVehicleDismountDanger;

	[Attribute("0", UIWidgets.ComboBox, "When AI may leave a vehicle. Auto = as usual. Disabled only = only when the vehicle is badly damaged or ordered. Orders only = only through a get out waypoint or order. Never = never get out. A destroyed or burning vehicle always allows getting out.", enums: ParamEnumArray.FromEnum(DCO_EDisembarkRule))]
	protected DCO_EDisembarkRule m_eDisembarkRule;

	[Attribute("15", UIWidgets.Flags, "When AI may leave a vehicle. Ordered = Get Out waypoint, GM order or commander task. Damaged = vehicle below 50 % health. Passengers = infantry in cargo seats. Unarmed drivers = drivers of vehicles without a weapon. Combat crew = driver, gunner and commander of armed vehicles. A destroyed or burning vehicle, or one below 25 %, can always be left. Static weapons and players are not affected.", enums: ParamEnumArray.FromEnum(DCO_EVehicleExit))]
	protected int m_iVehicleExit;

	[Attribute("2", UIWidgets.ComboBox, "AI Custom skill in combat", "", ParamEnumArray.FromEnum(DCO_AISKILL) )]
	protected DCO_AISKILL m_eAISkillDefault;

	[Attribute( defvalue: "1", uiwidget: UIWidgets.Slider, desc: "How Suppression Affecting this AI", params: "0 2 0.01" )]
	protected float m_fSuppressionEffect;

	[Attribute("0.9", UIWidgets.Range, "Base chance (0-1) that AI actively seek cover when detected in the open. Further scaled by personality: RECKLESS drops most (careless), AGGRESSIVE drops a little (combat minded but disciplined), CAUTIOUS rises.", params: "0 1 0.01" )]
	protected float m_fTakeCoverChance;

	[Attribute("80", UIWidgets.Slider, "MEASURED personality weight (relative to the other 3, does not need to total 100).", params: "0 100 1")]
	protected float m_fPersonalityWeightStandard;

	[Attribute("7", UIWidgets.Slider, "CAUTIOUS personality weight.", params: "0 100 1")]
	protected float m_fPersonalityWeightCautious;

	[Attribute("12", UIWidgets.Slider, "AGGRESSIVE personality weight.", params: "0 100 1")]
	protected float m_fPersonalityWeightAggressive;

	[Attribute("3", UIWidgets.Slider, "RECKLESS personality weight.", params: "0 100 1")]
	protected float m_fPersonalityWeightReckless;

	[Attribute("0.6", UIWidgets.Slider, "Chance AI dodge (run for cover) each time they hear gunfire. Scaled by personality when dodgeScaleByPersonality is on.", params: "0 1 0.05")]
	protected float m_fDodgeChance;

	[Attribute("1.0", UIWidgets.Slider, "Chance AI may go prone in any situation. 0 = never prone, 1 = always allowed. The roll is kept for 10 s per AI; AI already prone stay prone.", params: "0 1 0.05")]
	protected float m_fProneChance;

	[Attribute("0.0", UIWidgets.Slider, "Chance AI may go prone in CQB (inside a building or enemy closer than close combat range). 0 = never prone in CQB, 1 = always allowed. The roll is kept for 10 s per AI.", params: "0 1 0.05")]
	protected float m_fCQBProneChance;

	[Attribute("8.0", UIWidgets.Slider, "Cooldown (s) before the same AI may dodge again. This is the main brake; without it, full auto fire makes dodgeChance meaningless.", params: "0 120 0.5")]
	protected float m_fDodgeCooldown;

	[Attribute("250.0", UIWidgets.Slider, "Maximum distance (m) of gunfire that can still trigger a dodge.", params: "0 1000 5")]
	protected float m_fDodgeMaxDist;

	[Attribute("30.0", UIWidgets.Slider, "Building or cover search distance when dodging.", params: "5 100 1")]
	protected float m_fDodgeSearchDist;

	[Attribute("1", UIWidgets.CheckBox, "Scale dodge chance by AI personality (CAUTIOUS higher, RECKLESS lower).")]
	protected bool m_bDodgeScaleByPersonality;

	[Attribute("1", UIWidgets.Slider, "Number of threatening enemy shots before AI dodge. 1 = on the first shot.", params: "1 20 1")]
	protected int m_iDodgeShotThreshold;

	[Attribute("5.0", UIWidgets.Slider, "Shot counting window (s). If no new shot comes within this time, the count resets to 0.", params: "1 30 0.5")]
	protected float m_fDodgeShotWindow;

	[Attribute("0.5", UIWidgets.Slider, "How often AI throw frag grenades. 0 = never, 0.5 = default, 1 = very often (chance x2).", params: "0 1 0.01", category: "Weapon Usage")]
	protected float m_fGrenadeUsage;

	[Attribute("0.7", UIWidgets.Slider, "How often AI use grenade launchers (UGL). 0 = never, 0.5 = default, 1 = very often (chance x2).", params: "0 1 0.01", category: "Weapon Usage")]
	protected float m_fGLUsage;

	[Attribute("1", UIWidgets.Slider, "Grenade launcher (UGL) accuracy. Higher = tighter impact spread. 1 = default.", params: "0.1 5 0.1", category: "Weapon Usage")]
	protected float m_fGLAccuracy;

	[Attribute("0.5", UIWidgets.Slider, "How often AI throw smoke to cover movement. 0 = never, 0.5 = default, 1 = very often (chance x2).", params: "0 1 0.01", category: "Weapon Usage")]
	protected float m_fSmokeUsage;

	[Attribute("0", UIWidgets.ComboBox, "AI simulation LOD for groups owned by an AI Commander. Groups without a commander are always vanilla (unless overridden per group). VANILLA = engine decides, PREVENT_MAX_LOD = commander groups keep running far from players, FULL_DETAIL = commander groups at full detail (heaviest).", "", ParamEnumArray.FromEnum(DCO_EAILODMode), category: "Performance")]
	protected DCO_EAILODMode m_eLODMode;

	[Attribute("0", UIWidgets.CheckBox, "Performance profiling: logs time per DCO part, queries and raycasts per second, FPS and frame spikes every 30 s to dco.log (PERF channel) + perf.log in the session folder $profile:DCO/Logs. OFF = almost no cost.", category: "Performance")]
	protected bool m_bPerfProfiling;

	[Attribute("1", UIWidgets.ComboBox, "DCO log files in $profile:DCO/Logs. Off = no files (errors still go to the game console). Report only = just report.txt, a short readable summary of settings, problems and performance. Full = all technical logs (dco.log, battle.json) for bug reports. Test worlds and Workbench always log in full.", enums: ParamEnumArray.FromEnum(DCO_ELogMode), category: "Performance")]
	protected int m_iLogMode;

	[Attribute("0", UIWidgets.CheckBox, "Commander intent map overlay (Dev/GM): plans, decision reasons, group roles, the commander's picture of the enemy. Off = none; on = Game Master only. Always allowed in Workbench.", category: "Performance")]
	protected bool m_bIntentOverlay;

	[Attribute("2147483647", UIWidgets.Flags, "Active DCO individual and squad behaviors.", "", ParamEnumArray.FromEnum(DCO_EBehavior), category: "Behavior")]
	protected int m_iBehaviorFlags;

	[Attribute("15", UIWidgets.Slider, "An attack counts as stalled if the leading group advanced less than this percent since the check 2 minutes ago.", params: "5 50 1", category: "Behavior")]
	protected float m_fTacticStallPct;

	[Attribute("30", UIWidgets.Slider, "Seconds a group goes without a waypoint before it becomes IDLE (members take cover and hold still).", params: "5 300 1", category: "Idle")]
	protected float m_fIdleEnterTime;

	[Attribute("100", UIWidgets.Slider, "Distance (m) from the leader to search for cover or building positions while IDLE.", params: "10 200 5", category: "Idle")]
	protected float m_fIdleCoverSearchDist;

	[Attribute("15", UIWidgets.Slider, "IDLE: an enemy this close (m) = overrun, AI may leave cover.", params: "0 100 1", category: "Idle")]
	protected float m_fIdleOverrunDist;

	[Attribute("1", UIWidgets.CheckBox, "IDLE: may leave cover to help friends (attack enemies that are not firing at this group).", category: "Idle")]
	protected bool m_bIdleLeaveToAssist;

	[Attribute("0", UIWidgets.CheckBox, "IDLE: may leave cover to investigate threats.", category: "Idle")]
	protected bool m_bIdleLeaveToInvestigate;

	[Attribute("1", UIWidgets.CheckBox, "IDLE: the group still accepts commander orders. Off = the commander does not take IDLE groups.", category: "Idle")]
	protected bool m_bIdleFollowCommander;

	[Attribute("1", UIWidgets.CheckBox, "Pull back members that fall far behind the leader.", category: "Straggler")]
	protected bool m_bStragglerEnabled;

	[Attribute("15", UIWidgets.Slider, "Seconds a member stays beyond cohesion distance before being pulled back.", params: "5 120 1", category: "Straggler")]
	protected float m_fStragglerTime;

	[Attribute("30", UIWidgets.Slider, "Seconds without the distance to the leader shrinking = stuck. One other point is tried, then the member is marked stuck (not counted in group spread).", params: "10 300 1", category: "Straggler")]
	protected float m_fStragglerStuckTime;

	[Attribute("0", UIWidgets.CheckBox, "Log each straggler: distance, active behavior, combat mode, cover, leash. For diagnosis.", category: "Straggler")]
	protected bool m_bStragglerLog;

	[Attribute("40", UIWidgets.Slider, "Distance (m) from a member to the leader in open-ground combat before being pulled back near the leader, for groups without fireteams. Multiplied by the group cohesion multiplier.", params: "10 100 1", category: "Straggler")]
	protected float m_fCombatCohesionOpen;

	[Attribute("15", UIWidgets.Slider, "Distance (m) from a member to the leader in combat when the member or leader is inside a building. Multiplied by the group cohesion multiplier.", params: "5 50 1", category: "Straggler")]
	protected float m_fCombatCohesionUrban;

	[Attribute("-1", UIWidgets.Slider, "Armor fear morale load multiplier. -1 = automatic per faction (US and USSR 0.5, others 1.0).", params: "-1 2 0.05", category: "Combat")]
	protected float m_fArmorFearScale;

	[Attribute("2", UIWidgets.Slider, "The squad advances (assault posture + bounding) if living members >= this ratio x known enemies and it is not heavily suppressed. Also allowed to leave IDLE hold or the Defend leash.", params: "1 5 0.1", category: "Combat")]
	protected float m_fSuperiorRatio;

	[Attribute("1", UIWidgets.CheckBox, "Targets beyond the weapon's effective range that are not a direct threat: hold position (prone if line of sight remains, otherwise crouch), slow single shots.", category: "Combat")]
	protected bool m_bLongRangeHold;

	[Attribute("350", UIWidgets.Slider, "Rifle effective range (m). Beyond it the fire rate is limited (slow single shots) and threat may not raise it.", params: "100 800 10", category: "Combat")]
	protected float m_fRifleEffectiveRange;

	[Attribute("700", UIWidgets.Slider, "MG effective range (m). MGs may still fire bursts (suppression) at long range up to this.", params: "200 1500 10", category: "Combat")]
	protected float m_fMGEffectiveRange;

	[Attribute("1", UIWidgets.CheckBox, "Overmatch assault: a squad that is clearly stronger (weighted strength >= superiority ratio x personality) with a confirmed contact advances by bounding even while the enemy is still visible.", category: "Combat")]
	protected bool m_bOvermatchAssault;

	[Attribute("250", UIWidgets.Slider, "Maximum enemy distance (m) to start an overmatch assault.", params: "100 500 10", category: "Combat")]
	protected float m_fOvermatchMaxDist;

	[Attribute("1", UIWidgets.CheckBox, "Clear building: a squad on ATTACK, FLANK or overmatch that sees an enemy inside a building < 100 m clears that building (isolate, stack at the door, breach, clear room by room). Also sweeps buildings after an objective is taken.", category: "CQB")]
	protected bool m_bCQBEnabled;

	[Attribute("6", UIWidgets.Slider, "Minimum living members to split off a support team (isolation, MG preferred). Below this everyone goes in.", params: "3 12 1", category: "CQB")]
	protected int m_iCQBIsolateMin;

	[Attribute("6", UIWidgets.Slider, "Maximum seconds to wait for the team to gather at the door before entering (RECKLESS leaders do not wait).", params: "0 30 1", category: "CQB")]
	protected float m_fCQBStackWait;

	[Attribute("2", UIWidgets.Slider, "If the entry team loses this many (killed or unconscious), cancel, fall back to the support position and try again after 2 minutes.", params: "1 6 1", category: "CQB")]
	protected int m_iCQBAbortLosses;

	[Attribute("1", UIWidgets.CheckBox, "Objective capture waits until no building inside the objective radius has CONTACT status.", category: "CQB")]
	protected bool m_bCQBCaptureWaits;

	[Attribute("180", UIWidgets.Slider, "Seconds without new contact before a CONTACT building automatically counts as clear (so capture does not stall).", params: "30 600 10", category: "CQB")]
	protected float m_fCQBContactTimeout;

	[Attribute("0", UIWidgets.Slider, "CQB engine: 0 = group activity + BT (new), 1 = old state machine (Legacy) for comparison.", params: "0 1 1", category: "CQB")]
	protected int m_iCQBEngine;

	[Attribute("0", UIWidgets.Slider, "Room entry style: 0 = 1989 Pairs (No.1 near corner, No.2 far side), 1 = Modern 4-stack, 2 = by faction (US pairs, USSR pairs + always grenade, FIA isolate from outside).", params: "0 2 1", category: "CQB")]
	protected int m_iCQBStyle;

	[Attribute("0", UIWidgets.Slider, "Breach grenade: 0 = Auto (known contact or defended objective = frag, sweep without contact = no grenade), 1 = Always, 2 = Never, 3 = Stun only (no stun = no grenade).", params: "0 3 1", category: "CQB")]
	protected int m_iCQBBreachMode;

	[Attribute("4", UIWidgets.Slider, "Entry team size (2-4). The rest become the support/isolation team, at least 1 soldier outside if the group has 4 or more.", params: "2 4 1", category: "CQB")]
	protected int m_iCQBEntrySize;

	[Attribute("60", UIWidgets.Slider, "Time limit per room (s) before a CQB pair skips that room. Total building limit = 120 s + rooms x this value.", params: "20 120 5", category: "CQB")]
	protected float m_fCQBRoomTimeout;

	[Attribute("60", UIWidgets.Slider, "Extra time limit per floor (s). Total = rooms x room limit + floors x this, min 120 s, max 600 s.", params: "30 180 10", category: "CQB")]
	protected float m_fCQBFloorTimeout;

	[Attribute("50", UIWidgets.Slider, "Cancel only if entry team casualties >= this percent of the starting team AND no replacement is available from the support team.", params: "25 100 5", category: "CQB")]
	protected float m_fCQBAbortPct;

	[Attribute("1", UIWidgets.CheckBox, "Night behavior: squad flares, light discipline, shorter defensive ROE and long range fire distance, illumination before an assault.", category: "Night")]
	protected bool m_bNightEnabled;

	[Attribute("75", UIWidgets.Slider, "Flare cooldown per squad (s).", params: "30 300 5", category: "Night")]
	protected float m_fNightFlareCooldown;

	[Attribute("0.6", UIWidgets.Slider, "Multiplier for the fire range of defending groups (Defend/Garrison) and the long range effective distance at night.", params: "0.2 1 0.05", category: "Night")]
	protected float m_fNightEngageMul;

	[Attribute("1", UIWidgets.CheckBox, "Light discipline: AI flashlights off outdoors at night, on only for CQB entry teams inside buildings. Vehicle headlights off when an enemy is < 800 m.", category: "Night")]
	protected bool m_bNightLightDiscipline;

	[Attribute("0", UIWidgets.Slider, "Maximum contact reports per scan per group (global override). 0 = use the group prefab value (default 3).", params: "0 10 1", category: "Contact Report")]
	protected int m_iContactReportMaxPerScan;

	[Attribute("0", UIWidgets.Slider, "Contact report scan interval (s, global override). 0 = use the group prefab value (default 15).", params: "0 60 1", category: "Contact Report")]
	protected float m_fContactReportScanInterval;

	[Attribute("-1", UIWidgets.EditBox, "Starting reinforcement stock for all commanders (soldiers). -1 = use each commander's own value.", category: "Commander Spawner")]
	protected int m_iCommanderStockDefault;
	protected int m_iMaxAttackGroupsDefault = -1;

	[Attribute("0", UIWidgets.EditBox, "Total AI cap per faction for Commander Spawners. When reached, spawning is delayed. 0 = unlimited.", category: "Commander Spawner")]
	protected int m_iSpawnerFactionAICap;

	[Attribute("0", UIWidgets.CheckBox, "Allow several groups to share one building (indoor defend, idle or garrison positions). OFF = one group per building.", category: "Combat")]
	protected bool m_bShareBuildings;

	[Attribute("0", UIWidgets.ComboBox, "Defend and idle position preference: Balanced (indoor and outdoor scored together, fortification bonus), Indoor, Outdoor.", enums: { ParamEnum("Balanced", "0"), ParamEnum("Indoor", "1"), ParamEnum("Outdoor", "2") }, category: "Combat")]
	protected int m_iCoverPreference;

	[Attribute("1", UIWidgets.CheckBox, "AI squads share enemy contacts with each other (voice or short range radio) without going through the commander.", category: "Contact Sharing")]
	protected bool m_bShareEnabled;

	[Attribute("100", UIWidgets.Slider, "Voice range (m): shouting to friendly squads, no radio needed.", params: "20 300 5", category: "Contact Sharing")]
	protected float m_fShareVoiceRange;

	[Attribute("400", UIWidgets.Slider, "Short range radio reach (m).", params: "100 1500 25", category: "Contact Sharing")]
	protected float m_fShareRadioRange;

	[Attribute("8", UIWidgets.Slider, "Base position noise (m) per 100 m between observer and contact.", params: "0 50 1", category: "Contact Sharing")]
	protected float m_fShareNoisePer100m;

	[Attribute("1.5", UIWidgets.Slider, "Voice path noise multiplier (coarser).", params: "0.5 5 0.1", category: "Contact Sharing")]
	protected float m_fShareVoiceNoise;

	[Attribute("1", UIWidgets.Slider, "Short range radio path noise multiplier.", params: "0.5 5 0.1", category: "Contact Sharing")]
	protected float m_fShareRadioNoise;

	[Attribute("0", UIWidgets.CheckBox, "Radio required: the short range radio path only works if both sender and receiver carry a radio.", category: "Contact Sharing")]
	protected bool m_bShareRequireRadio;

	[Attribute("2", UIWidgets.Slider, "Number of hops (1 = direct only, 2 = the receiver may pass it on once).", params: "1 2 1", category: "Contact Sharing")]
	protected int m_iShareHops;

	[Attribute("0", UIWidgets.CheckBox, "Shared contacts are marked on friendly players' maps (vanilla AI enemy markers).", category: "Contact Sharing")]
	protected bool m_bShareMapMarkers;

	[Attribute("0", UIWidgets.ComboBox, "Enemy markers placed on the map by AI groups (vanilla and DCO Share Map Markers): follow the vanilla faction attribute, off for everyone, or on for everyone.", enums: { ParamEnum("Follow faction", "0"), ParamEnum("Off", "1"), ParamEnum("On", "2") }, category: "Contact Sharing")]
	protected int m_iAIMapMarkers;

	[Attribute("0", UIWidgets.CheckBox, "Friendly players in range get a small notification when AI squads share a contact. Off by default.", category: "Contact Sharing")]
	protected bool m_bShareToPlayers;

	[Attribute("0", UIWidgets.CheckBox, "Log each shared contact (sender -> receiver, hops, path, noise).", category: "Contact Sharing")]
	protected bool m_bShareDebug;

	[Attribute("1", UIWidgets.CheckBox, "AI medics may treat friendly casualties outside their own group if the casualty's group has no medic who can come.", category: "Cross-Group Medic")]
	protected bool m_bMedicCrossGroup;

	[Attribute("150", UIWidgets.Slider, "Radius (m) to look for a medic from another group for a bleeding casualty.", params: "25 500 5", category: "Cross-Group Medic")]
	protected float m_fMedicRadiusBleeding;

	[Attribute("300", UIWidgets.Slider, "Radius (m) to look for a medic from another group for an unconscious casualty.", params: "25 800 5", category: "Cross-Group Medic")]
	protected float m_fMedicRadiusUnconscious;

	[Attribute("1", UIWidgets.CheckBox, "Friendly players are also treated by AI medics.", category: "Cross-Group Medic")]
	protected bool m_bMedicIncludePlayers;

	[Attribute("1", UIWidgets.CheckBox, "UNCONSCIOUS casualties may be recovered under fire (the medic is not under direct fire, uses smoke). Bleeding casualties always wait until it is safe.", category: "Cross-Group Medic")]
	protected bool m_bMedicUnderFire;

	[Attribute("1", UIWidgets.Slider, "Maximum medics one group lends out at a time.", params: "1 4 1", category: "Cross-Group Medic")]
	protected int m_iMedicMaxLentPerGroup;

	[Attribute("90", UIWidgets.Slider, "Seconds before a medic loan counts as failed and another medic is tried.", params: "20 300 5", category: "Cross-Group Medic")]
	protected float m_fMedicTimeout;

	[Attribute("1", UIWidgets.Slider, "Map overlay of friendly AI groups for players. 0 = off, 1 = only squads tied to support a player group or sharing its objective, 2 = all.", params: "0 2 1", category: "Player Awareness")]
	protected int m_iOverlayMode;

	[Attribute("1500", UIWidgets.Slider, "Base objective radius (m) in relevant overlay mode (objectives within 2x this radius are shown).", params: "200 5000 50", category: "Player Awareness")]
	protected float m_fOverlayRadius;

	[Attribute("800", UIWidgets.Slider, "Radius (m) of operation radio broadcasts to players around an objective.", params: "100 3000 50", category: "Player Awareness")]
	protected float m_fRadioRadius;

	[Attribute("400", UIWidgets.Slider, "Radius (m) of artillery warnings to friendly players around the impact point.", params: "100 1500 25", category: "Player Awareness")]
	protected float m_fArtyWarnRadius;

	[Attribute("1", UIWidgets.CheckBox, "Commander briefings to player groups (OPORD, FRAGO, phases, withdraw/cancel). Off = nothing is sent and players see Disabled by server.", category: "Player Awareness")]
	protected bool m_bPlayerBriefings;

	[Attribute("2", UIWidgets.Slider, "Highest briefing HUD level players may choose. 0 = Hardcore, 1 = Standard, 2 = Assisted.", params: "0 2 1", category: "Player Awareness")]
	protected int m_iPlayerBriefingCap;

	[Attribute("0", UIWidgets.Slider, "Briefing world markers. 0 = follow the player's level, 1 = off for everyone, 2 = on for everyone.", params: "0 2 1", category: "Player Awareness")]
	protected int m_iBriefingWorldMarkers;

	[Attribute("500", UIWidgets.Slider, "Maximum enemy distance (m) a group will still investigate.", params: "0 1500 10", category: "Investigate")]
	protected float m_fInvestigateMaxDist;

	[Attribute("1", UIWidgets.Slider, "Chance a group investigates an unidentified contact. Rolled once per cluster.", params: "0 1 0.05", category: "Investigate")]
	protected float m_fInvestigateChance;

	[Attribute("1", UIWidgets.CheckBox, "Read config overrides from a JSON file in the server profile folder. If OFF, the values above are used as they are.", category: "Server Config")]
	protected bool m_bUseServerConfigFile;

	[Attribute("$profile:DCO/DCO_GlobalConfig.json", UIWidgets.EditBox, "Path of the JSON config file. The $profile: prefix points to the server -profile folder.", category: "Server Config")]
	protected string m_sServerConfigPath;

	[Attribute("1", UIWidgets.CheckBox, "If the JSON file does not exist, create it with the current Workbench values. If it exists but is missing keys, the old file is copied to .bak and the missing keys are added (admin values are kept).", category: "Server Config")]
	protected bool m_bAutoGenerateConfigFile;

	protected static bool s_bConfigLoaded;

	static DCO_GlobalAIComponent m_sInstance;

	static DCO_GlobalAIComponent GetInstance()
	{
		if (m_sInstance && !s_bConfigLoaded)
		{
			s_bConfigLoaded = true;
			m_sInstance.LoadServerConfig();
		}

		return m_sInstance;
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		ApplyWorkbenchDefaults();
		ApplyPerfProfiling();
	}

	protected void ApplyWorkbenchDefaults()
	{
		if (!GetGame().InPlayMode() || !Replication.IsServer() || !DCO_TestCaseComponent.RunOn())
			return;
		m_bPerfProfiling = true;
		DCO_Census.SetEnabled(true);
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		DCO_Perf.Tick(timeSlice);
	}

	override void OnDelete(IEntity owner)
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(PushShareToPlayers);
		DCO_Perf.WriteSummary("end");
		DCO_Census.OnShutdown();
		DCO_Perf.SetCensus(false);
		super.OnDelete(owner);
	}

	bool GetBehaviorFlag(int bit)				{ return (m_iBehaviorFlags & bit) != 0; }
	int GetBehaviorMask()						{ return m_iBehaviorFlags; }
	float GetTacticStallPct()					{ return m_fTacticStallPct; }
	void SetTacticStallPct(float f)				{ m_fTacticStallPct = Math.Clamp(f, 5.0, 50.0); }

	void SetBehaviorFlag(int bit, bool on)
	{
		if (on)
			m_iBehaviorFlags = m_iBehaviorFlags | bit;
		else
			m_iBehaviorFlags = m_iBehaviorFlags & ~bit;
	}

	static bool BehaviorOn(DCO_EBehavior bit)
	{
		DCO_GlobalAIComponent g = GetInstance();
		return !g || (g.m_iBehaviorFlags & bit) != 0;
	}

	bool GetPerfProfiling()						{ return m_bPerfProfiling; }

	int GetLogMode()							{ return m_iLogMode; }

	void SetLogMode(int mode)
	{
		m_iLogMode = Math.ClampInt(mode, DCO_ELogMode.OFF, DCO_ELogMode.FULL);
		Print("[DCO] DCO Logging set to " + typename.EnumToString(DCO_ELogMode, m_iLogMode), LogLevel.NORMAL);
	}

	void SetPerfProfiling(bool b)
	{
		m_bPerfProfiling = b;
		ApplyPerfProfiling();
	}

	bool GetIntentOverlay()						{ return m_bIntentOverlay && DCO_TestCaseComponent.RunOn(); }

	void SetIntentOverlay(bool b)
	{
		m_bIntentOverlay = b;
		DCO_CmdViewStore.Refresh();
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "gm_set_intent_overlay on=" + DCO_Util.B01(b).ToString());
	}

	bool GetCensus()							{ return DCO_Census.IsOn(); }
	float GetCensusInterval()					{ return DCO_Census.GetInterval(); }
	void SetCensusInterval(float s)				{ DCO_Census.SetInterval(s); }

	void SetCensus(bool b)
	{
		DCO_Census.SetEnabled(b && Replication.IsServer());
		ApplyPerfProfiling();
	}

	protected void ApplyPerfProfiling()
	{
		bool on = m_bPerfProfiling && Replication.IsServer();
		DCO_Perf.SetEnabled(on);
		if (on || DCO_Census.IsOn())
			SetEventMask(GetOwner(), EntityEvent.FRAME);
		else
			ClearEventMask(GetOwner(), EntityEvent.FRAME);
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_sInstance = this;
		s_bConfigLoaded = false;

		SetEventMask(owner, EntityEvent.INIT);
		if (Replication.IsServer())
		{
			GetGame().GetCallqueue().CallLater(PushShareToPlayers, 5000, true);
			GetGame().GetCallqueue().CallLater(ReapplyCommanderLOD, 30000, false, "startup");
		}
	}


	protected void PushShareToPlayers()
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;
		array<int> players = {};
		pm.GetPlayers(players);
		foreach (int pid : players)
		{
			SCR_PlayerControllerGroupComponent comp = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(pid);
			if (!comp)
				continue;
			comp.DCO_SetServerShare(m_bShareToPlayers);
			int cap = -1;
			if (m_bPlayerBriefings)
				cap = m_iPlayerBriefingCap;
			comp.DCO_SetBriefing(cap, m_iBriefingWorldMarkers);
		}
	}


	protected ref DCO_Doctrine m_DCO_Doc;
	protected static int s_iDCO_DocVersion;

	static int DCO_DocVersion()
	{
		return s_iDCO_DocVersion;
	}

	static void DCO_BumpDoc()
	{
		s_iDCO_DocVersion++;
	}

	DCO_Doctrine DCO_GetDoctrine()
	{
		if (!m_DCO_Doc)
		{
			bool found;
			m_DCO_Doc = DCO_DoctrineStore.Resolve(DCO_DoctrineStore.DefaultName(), found);
		}
		return m_DCO_Doc;
	}

	protected string DCO_DocScope()
	{
		return "global";
	}

	protected void DCO_DocChanged(string what)
	{
		DCO_Doctrine d = DCO_GetDoctrine();
		DCO_DoctrinePreset preset = DCO_DoctrineStore.Find(d.m_sPreset);
		d.m_bModified = false;
		if (preset)
		{
			DCO_Doctrine clean = new DCO_Doctrine();
			clean.FromPreset(preset, d.m_sSource);
			d.m_bModified = !d.SameTables(clean);
		}
		s_iDCO_DocVersion++;
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "doctrine_set scope=" + DCO_DocScope() + " what=" + what + " preset=" + DCO_SettingsBaseline.LogText(d.m_sPreset) + " modified=" + d.m_bModified.ToString() + " masks=" + d.MaskText() + " numbers=" + d.NumText() + " force=" + d.m_sForce);
	}

	void DCO_SetDocPreset(string name)
	{
		bool found;
		DCO_Doctrine d = DCO_DoctrineStore.Resolve(name, found);
		if (!found)
		{
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.WARN, "-", "doctrine_missing scope=" + DCO_DocScope() + " preset=" + DCO_SettingsBaseline.LogText(name));
			return;
		}
		m_DCO_Doc = d;
		DCO_DocChanged("preset");
	}

	void DCO_SetDocMask(int index, int mask)
	{
		DCO_Doctrine d = DCO_GetDoctrine();
		if (d.Mask(index) == mask)
			return;
		d.SetMask(index, mask);
		DCO_DocChanged("mask" + index.ToString());
	}

	void DCO_SetDocNumber(int which, float value)
	{
		DCO_Doctrine d = DCO_GetDoctrine();
		float before = d.Number(which);
		d.SetNumber(which, value);
		if (Math.AbsFloat(d.Number(which) - before) < 0.001)
			return;
		DCO_DocChanged("number" + which.ToString());
	}

	void DCO_SetDocForce(string key, int field, float value)
	{
		DCO_Doctrine d = DCO_GetDoctrine();
		float before = d.ForceValue(key, field);
		d.SetForce(key, field, value);
		if (Math.AbsFloat(d.ForceValue(key, field) - before) < 0.001)
			return;
		DCO_DocChanged("force_" + key + "_" + field.ToString());
	}

	protected void DCO_SyncDoctrine()
	{
		DCO_Doctrine d = DCO_GetDoctrine();
		string preset = CfgS("doctrinePreset", d.m_sPreset);
		string masks = CfgS("doctrineMasks", d.MaskText());
		string numbers = CfgS("doctrineNumbers", d.NumText());
		string force = CfgS("doctrineForce", d.m_sForce);
		if (m_iCfgMode != 0)
			return;
		if (preset != d.m_sPreset)
		{
			bool found;
			DCO_Doctrine loaded = DCO_DoctrineStore.Resolve(preset, found);
			if (found)
				m_DCO_Doc = loaded;
			d = DCO_GetDoctrine();
		}
		array<string> parts = {};
		masks.Split("|", parts, false);
		if (parts.Count() >= 3)
		{
			DCO_Doctrine.ParseMasks(parts[0], d.m_aBattle);
			DCO_Doctrine.ParseMasks(parts[1], d.m_aDefense);
			DCO_Doctrine.ParseMasks(parts[2], d.m_aSupport);
			if (parts.Count() == 3)
				DCO_Doctrine.StagedLegacy(d.m_aBattle, "config:" + DCO_DocScope());
		}
		d.ParseNumbers(numbers);
		if (!force.IsEmpty())
			d.m_sForce = DCO_Force.Canon(force);
		DCO_DocChanged("config");
	}

	protected int m_iCfgMode;
	protected ref SCR_JsonLoadContext m_CfgLoad;
	protected ref SCR_JsonSaveContext m_CfgSave;
	protected ref array<string> m_aCfgMissing = {};
	protected int m_iCfgApplied;
	protected bool m_bCfgTrace;
	protected bool m_bCfgFileLoad;
	protected string m_sCfgFactions;
	protected string m_sCfgFaction;
	protected int m_iCfgFactionApplied;
	protected int m_iCfgFactionGlobalOnly;
	protected ref array<string> m_aCfgFactionOk;
	protected bool m_bCfgParseFailed;
	protected bool m_bCfgFactionsBad;
	protected ref set<string> m_CfgSeen;
	protected int m_iCfgFromJson;
	protected int m_iCfgFromDefault;
	protected int m_iCfgUnused;
	protected int m_iCfgBadType;
	protected ref array<string> m_aCfgFileKeys;
	protected ref set<string> m_CfgFileKeySet;
	protected ref array<string> m_aCfgReadme;
	protected string m_sCfgCat;
	protected string m_sCfgLine;
	protected string m_sCfgTraceBuf;
	protected int m_iCfgTraceN;
	protected int m_iCfgLineKeys;
	protected ref map<string, string> m_mCfgOldDef;
	protected ref map<string, string> m_mCfgNewDef;
	protected int m_iCfgDefMigrated;
	protected int m_iCfgLegacyDiff;
	protected bool m_bCfgDefFromWrite;

	protected void SyncConfig()
	{
		CfgCat("Unit");
		unitAimSkillAccuracy = CfgF("aimAccuracy", unitAimSkillAccuracy, 0.1, 10.0);
		m_fAimSpeed = CfgF("aimSpeed", m_fAimSpeed, 0.25, 4.0);
		m_fAimShake = CfgF("aimShake", m_fAimShake, 0.25, 4.0);
		m_fAiPerception = CfgF("aiPerception", m_fAiPerception, 0.5, 5.0);
		m_fVehicleDismountDanger = CfgF("vehicleDismountDanger", m_fVehicleDismountDanger, 0.0, 2000.0);
		m_eDisembarkRule = CfgI("disembarkRule", m_eDisembarkRule, DCO_EDisembarkRule.AUTO, DCO_EDisembarkRule.NEVER);
		m_iVehicleExit = CfgI("vehicleExit", m_iVehicleExit, 0, 31);
		if (m_iCfgMode == 0 && m_sCfgFaction.IsEmpty() && m_CfgFileKeySet && !m_CfgFileKeySet.Contains("vehicleExit") && m_eDisembarkRule != DCO_EDisembarkRule.AUTO)
		{
			m_iVehicleExit = VehicleExitFromRule(m_eDisembarkRule);
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cfg_vehicle_exit_from_rule rule=%1 flags=%2", typename.EnumToString(DCO_EDisembarkRule, m_eDisembarkRule), m_iVehicleExit));
		}
		m_fSuppressionEffect = CfgF("suppressionEffect", m_fSuppressionEffect, 0.0, 2.0);
		m_fTakeCoverChance = CfgF("takeCoverChance", m_fTakeCoverChance, 0.0, 1.0);
		m_bIsMagicallyResupplied = CfgB("magicallyResupplied", m_bIsMagicallyResupplied);
		m_eAISkillDefault = CfgI("aiSkillDefault", m_eAISkillDefault, DCO_AISKILL.NOOB, DCO_AISKILL.TERMINATOR);

		CfgCat("Personality");
		m_fPersonalityWeightStandard = CfgF("personalityWeightMeasured", m_fPersonalityWeightStandard, 0.0, 100.0);
		m_fPersonalityWeightCautious = CfgF("personalityWeightCautious", m_fPersonalityWeightCautious, 0.0, 100.0);
		m_fPersonalityWeightAggressive = CfgF("personalityWeightAggressive", m_fPersonalityWeightAggressive, 0.0, 100.0);
		m_fPersonalityWeightReckless = CfgF("personalityWeightReckless", m_fPersonalityWeightReckless, 0.0, 100.0);

		CfgCat("Dodge");
		m_fDodgeChance = CfgF("dodgeChance", m_fDodgeChance, 0.0, 1.0);
		m_fDodgeCooldown = CfgF("dodgeCooldown", m_fDodgeCooldown, 0.0, 120.0);
		m_fDodgeMaxDist = CfgF("dodgeMaxDist", m_fDodgeMaxDist, 0.0, 1000.0);
		m_fDodgeSearchDist = CfgF("dodgeSearchDist", m_fDodgeSearchDist, 5.0, 100.0);
		m_bDodgeScaleByPersonality = CfgB("dodgeScaleByPersonality", m_bDodgeScaleByPersonality);
		m_fProneChance = CfgF("proneChance", m_fProneChance, 0.0, 1.0);
		m_fCQBProneChance = CfgF("cqbProneChance", m_fCQBProneChance, 0.0, 1.0);
		m_iDodgeShotThreshold = CfgI("dodgeShotThreshold", m_iDodgeShotThreshold, 1, 20);
		m_fDodgeShotWindow = CfgF("dodgeShotWindow", m_fDodgeShotWindow, 1.0, 30.0);

		CfgCat("Weapon Usage");
		m_fGrenadeUsage = CfgF("grenadeUsage", m_fGrenadeUsage, 0.0, 1.0);
		m_fGLUsage = CfgF("glUsage", m_fGLUsage, 0.0, 1.0);
		m_fGLAccuracy = CfgF("glAccuracy", m_fGLAccuracy, 0.1, 5.0);
		m_fSmokeUsage = CfgF("smokeUsage", m_fSmokeUsage, 0.0, 1.0);

		CfgCat("Performance");
		m_eLODMode = CfgI("lodMode", m_eLODMode, DCO_EAILODMode.VANILLA, DCO_EAILODMode.FULL_DETAIL);
		m_bPerfProfiling = CfgB("perfProfiling", m_bPerfProfiling);
		m_iLogMode = CfgI("logMode", m_iLogMode, DCO_ELogMode.OFF, DCO_ELogMode.FULL);
		m_bIntentOverlay = CfgB("intentOverlay", m_bIntentOverlay);

		CfgCat("Idle");
		m_fIdleEnterTime = CfgF("idleEnterTime", m_fIdleEnterTime, 5.0, 300.0);
		m_fIdleCoverSearchDist = CfgF("idleCoverSearchDist", m_fIdleCoverSearchDist, 10.0, 200.0);
		m_fIdleOverrunDist = CfgF("idleOverrunDist", m_fIdleOverrunDist, 0.0, 100.0);
		m_bIdleLeaveToAssist = CfgB("idleLeaveToAssist", m_bIdleLeaveToAssist);
		m_bIdleLeaveToInvestigate = CfgB("idleLeaveToInvestigate", m_bIdleLeaveToInvestigate);
		m_bIdleFollowCommander = CfgB("idleFollowCommander", m_bIdleFollowCommander);

		CfgCat("Straggler");
		m_bStragglerEnabled = CfgB("stragglerEnabled", m_bStragglerEnabled);
		m_fStragglerTime = CfgF("stragglerTime", m_fStragglerTime, 5.0, 120.0);
		m_fStragglerStuckTime = CfgF("stragglerStuckTime", m_fStragglerStuckTime, 10.0, 300.0);
		m_bStragglerLog = CfgB("stragglerLog", m_bStragglerLog);
		m_fCombatCohesionOpen = CfgF("combatCohesionOpen", m_fCombatCohesionOpen, 10.0, 100.0);
		m_fCombatCohesionUrban = CfgF("combatCohesionUrban", m_fCombatCohesionUrban, 5.0, 50.0);
		m_fArmorFearScale = CfgF("armorFearScale", m_fArmorFearScale, -1.0, 2.0);

		CfgCat("Combat");
		m_fSuperiorRatio = CfgF("superiorRatio", m_fSuperiorRatio, 1.0, 5.0);
		m_bLongRangeHold = CfgB("longRangeHold", m_bLongRangeHold);
		m_fRifleEffectiveRange = CfgF("rifleEffectiveRange", m_fRifleEffectiveRange, 100.0, 800.0);
		m_fMGEffectiveRange = CfgF("mgEffectiveRange", m_fMGEffectiveRange, 200.0, 1500.0);
		m_bShareBuildings = CfgB("shareBuildings", m_bShareBuildings);
		m_iCoverPreference = CfgI("coverPreference", m_iCoverPreference, 0, 2);
		m_bOvermatchAssault = CfgB("overmatchAssault", m_bOvermatchAssault);
		m_fOvermatchMaxDist = CfgF("overmatchMaxDist", m_fOvermatchMaxDist, 100.0, 500.0);

		CfgCat("CQB");
		m_bCQBEnabled = CfgB("cqbEnabled", m_bCQBEnabled);
		m_iCQBIsolateMin = CfgI("cqbIsolateMin", m_iCQBIsolateMin, 3, 12);
		m_fCQBStackWait = CfgF("cqbStackWait", m_fCQBStackWait, 0.0, 30.0);
		m_iCQBAbortLosses = CfgI("cqbAbortLosses", m_iCQBAbortLosses, 1, 6);
		m_bCQBCaptureWaits = CfgB("cqbCaptureWaits", m_bCQBCaptureWaits);
		m_fCQBContactTimeout = CfgF("cqbContactTimeout", m_fCQBContactTimeout, 30.0, 600.0);
		m_iCQBBreachMode = CfgI("cqbBreachMode", m_iCQBBreachMode, 0, 3);
		m_fCQBRoomTimeout = CfgF("cqbRoomTimeout", m_fCQBRoomTimeout, 20.0, 120.0);

		CfgCat("Behavior");
		SetBehaviorFlag(DCO_EBehavior.TARGET_PRIORITY, CfgB("behTargetPriority", GetBehaviorFlag(DCO_EBehavior.TARGET_PRIORITY)));
		SetBehaviorFlag(DCO_EBehavior.RELOAD_DISCIPLINE, CfgB("behReloadDiscipline", GetBehaviorFlag(DCO_EBehavior.RELOAD_DISCIPLINE)));
		SetBehaviorFlag(DCO_EBehavior.PISTOL_CQB, CfgB("behPistolCQB", GetBehaviorFlag(DCO_EBehavior.PISTOL_CQB)));
		SetBehaviorFlag(DCO_EBehavior.SECURITY_360, CfgB("behSecurity360", GetBehaviorFlag(DCO_EBehavior.SECURITY_360)));
		SetBehaviorFlag(DCO_EBehavior.HEARING, CfgB("behHearing", GetBehaviorFlag(DCO_EBehavior.HEARING)));
		SetBehaviorFlag(DCO_EBehavior.BUDDY_CALLOUT, CfgB("behBuddyCallout", GetBehaviorFlag(DCO_EBehavior.BUDDY_CALLOUT)));
		SetBehaviorFlag(DCO_EBehavior.WINDOW_FIRE, CfgB("behWindowFire", GetBehaviorFlag(DCO_EBehavior.WINDOW_FIRE)));
		SetBehaviorFlag(DCO_EBehavior.MG_GUNNER, CfgB("behMGGunner", GetBehaviorFlag(DCO_EBehavior.MG_GUNNER)));
		SetBehaviorFlag(DCO_EBehavior.AT_SPECIALIST, CfgB("behATSpecialist", GetBehaviorFlag(DCO_EBehavior.AT_SPECIALIST)));
		SetBehaviorFlag(DCO_EBehavior.MARKSMAN, CfgB("behMarksman", GetBehaviorFlag(DCO_EBehavior.MARKSMAN)));
		SetBehaviorFlag(DCO_EBehavior.LEADER_RTO_GRENADIER, CfgB("behLeaderRTOGrenadier", GetBehaviorFlag(DCO_EBehavior.LEADER_RTO_GRENADIER)));
		SetBehaviorFlag(DCO_EBehavior.BREAK_CONTACT, CfgB("behBreakContact", GetBehaviorFlag(DCO_EBehavior.BREAK_CONTACT)));
		SetBehaviorFlag(DCO_EBehavior.AMBUSH_DRILL, CfgB("behAmbushDrill", GetBehaviorFlag(DCO_EBehavior.AMBUSH_DRILL)));
		SetBehaviorFlag(DCO_EBehavior.CONTACT_SOP, CfgB("behContactSOP", GetBehaviorFlag(DCO_EBehavior.CONTACT_SOP)));
		SetBehaviorFlag(DCO_EBehavior.REARM, CfgB("behRearm", GetBehaviorFlag(DCO_EBehavior.REARM)));
		SetBehaviorFlag(DCO_EBehavior.SPECIALIST_RECOVERY, CfgB("behSpecialistRecovery", GetBehaviorFlag(DCO_EBehavior.SPECIALIST_RECOVERY)));
		SetBehaviorFlag(DCO_EBehavior.REARM_ENEMY_SOURCES, CfgB("behRearmEnemySources", GetBehaviorFlag(DCO_EBehavior.REARM_ENEMY_SOURCES)));
		SetBehaviorFlag(DCO_EBehavior.ENEMY_WEAPONS, CfgB("behEnemyWeapons", GetBehaviorFlag(DCO_EBehavior.ENEMY_WEAPONS)));
		SetBehaviorFlag(DCO_EBehavior.EXPLOSION_REACTIONS, CfgB("behExplosionReactions", GetBehaviorFlag(DCO_EBehavior.EXPLOSION_REACTIONS)));
		SetBehaviorFlag(DCO_EBehavior.PLAYER_VOICE, CfgB("behPlayerVoice", GetBehaviorFlag(DCO_EBehavior.PLAYER_VOICE)));
		SetBehaviorFlag(DCO_EBehavior.LEAN_PEEK, CfgB("behLeanPeek", GetBehaviorFlag(DCO_EBehavior.LEAN_PEEK)));
		SetBehaviorFlag(DCO_EBehavior.SUPPRESS_LAST_KNOWN, CfgB("behSuppressLastKnown", GetBehaviorFlag(DCO_EBehavior.SUPPRESS_LAST_KNOWN)));
		SetBehaviorFlag(DCO_EBehavior.ASSAULT_FIRE, CfgB("behAssaultFire", GetBehaviorFlag(DCO_EBehavior.ASSAULT_FIRE)));
		SetBehaviorFlag(DCO_EBehavior.REACTION_TIME, CfgB("behReactionTime", GetBehaviorFlag(DCO_EBehavior.REACTION_TIME)));
		SetBehaviorFlag(DCO_EBehavior.PRONE_MOVE, CfgB("behProneMove", GetBehaviorFlag(DCO_EBehavior.PRONE_MOVE)));
		SetBehaviorFlag(DCO_EBehavior.PINNED, CfgB("behPinned", GetBehaviorFlag(DCO_EBehavior.PINNED)));
		SetBehaviorFlag(DCO_EBehavior.AWARENESS, CfgB("behAwareness", GetBehaviorFlag(DCO_EBehavior.AWARENESS)));
		SetBehaviorFlag(DCO_EBehavior.COMBINED_ARMS, CfgB("behCombinedArms", GetBehaviorFlag(DCO_EBehavior.COMBINED_ARMS)));

		CfgCat("Night");
		m_bNightEnabled = CfgB("nightEnabled", m_bNightEnabled);
		m_fNightFlareCooldown = CfgF("nightFlareCooldown", m_fNightFlareCooldown, 30.0, 300.0);
		m_fNightEngageMul = CfgF("nightEngageMul", m_fNightEngageMul, 0.2, 1.0);
		m_bNightLightDiscipline = CfgB("nightLightDiscipline", m_bNightLightDiscipline);

		CfgCat("Contact Report");
		m_iContactReportMaxPerScan = CfgI("contactReportMaxPerScan", m_iContactReportMaxPerScan, 0, 10);
		m_fContactReportScanInterval = CfgF("contactReportScanInterval", m_fContactReportScanInterval, 0.0, 60.0);

		CfgCat("Commander Spawner");
		m_iCommanderStockDefault = CfgI("commanderStock", m_iCommanderStockDefault, -1, 100000);
		m_iMaxAttackGroupsDefault = CfgI("maxAttackGroups", m_iMaxAttackGroupsDefault, -1, 20);
		m_iSpawnerFactionAICap = CfgI("spawnerFactionAICap", m_iSpawnerFactionAICap, 0, 2000);

		CfgCat("Contact Sharing");
		m_bShareEnabled = CfgB("shareEnabled", m_bShareEnabled);
		m_fShareVoiceRange = CfgF("shareVoiceRange", m_fShareVoiceRange, 20.0, 300.0);
		m_fShareRadioRange = CfgF("shareRadioRange", m_fShareRadioRange, 100.0, 1500.0);
		m_fShareNoisePer100m = CfgF("shareNoisePer100m", m_fShareNoisePer100m, 0.0, 50.0);
		m_fShareVoiceNoise = CfgF("shareVoiceNoise", m_fShareVoiceNoise, 0.5, 5.0);
		m_fShareRadioNoise = CfgF("shareRadioNoise", m_fShareRadioNoise, 0.5, 5.0);
		m_bShareRequireRadio = CfgB("shareRequireRadio", m_bShareRequireRadio);
		m_iShareHops = CfgI("shareHops", m_iShareHops, 1, 2);
		m_bShareMapMarkers = CfgB("shareMapMarkers", m_bShareMapMarkers);
		m_bShareToPlayers = CfgB("shareToPlayers", m_bShareToPlayers);
		m_bShareDebug = CfgB("shareDebug", m_bShareDebug);

		CfgCat("Cross-Group Medic");
		m_bMedicCrossGroup = CfgB("medicCrossGroup", m_bMedicCrossGroup);
		m_fMedicRadiusBleeding = CfgF("medicRadiusBleeding", m_fMedicRadiusBleeding, 25.0, 500.0);
		m_fMedicRadiusUnconscious = CfgF("medicRadiusUnconscious", m_fMedicRadiusUnconscious, 25.0, 800.0);
		m_bMedicIncludePlayers = CfgB("medicIncludePlayers", m_bMedicIncludePlayers);
		m_bMedicUnderFire = CfgB("medicUnderFire", m_bMedicUnderFire);
		m_iMedicMaxLentPerGroup = CfgI("medicMaxLentPerGroup", m_iMedicMaxLentPerGroup, 1, 4);
		m_fMedicTimeout = CfgF("medicTimeout", m_fMedicTimeout, 20.0, 300.0);

		CfgCat("Player Awareness");
		m_iOverlayMode = CfgI("overlayMode", m_iOverlayMode, 0, 2);
		m_fOverlayRadius = CfgF("overlayRadius", m_fOverlayRadius, 200.0, 5000.0);
		m_fRadioRadius = CfgF("radioRadius", m_fRadioRadius, 100.0, 3000.0);
		m_fArtyWarnRadius = CfgF("artyWarnRadius", m_fArtyWarnRadius, 100.0, 1500.0);
		m_bPlayerBriefings = CfgB("playerBriefings", m_bPlayerBriefings);
		m_iPlayerBriefingCap = CfgI("playerBriefingCap", m_iPlayerBriefingCap, 0, 2);
		m_iBriefingWorldMarkers = CfgI("briefingWorldMarkers", m_iBriefingWorldMarkers, 0, 2);

		CfgCat("Investigate");
		m_fInvestigateMaxDist = CfgF("investigateMaxDist", m_fInvestigateMaxDist, 0.0, 1500.0);
		m_fInvestigateChance = CfgF("investigateChance", m_fInvestigateChance, 0.0, 1.0);

		CfgCat("Doctrine");
		DCO_SyncDoctrine();

		CfgFlush();
	}

	protected float CfgF(string key, float cur, float min, float max)
	{
		if (m_iCfgMode == 1)
		{
			CfgDefNote(key, cur.ToString());
			CfgReadme(key, cur.ToString(), min.ToString() + " .. " + max.ToString());
			m_CfgSave.WriteValue(key, cur);
			return cur;
		}
		if (m_iCfgMode == 2)
		{
			CfgDumpAdd(key, cur.ToString());
			return cur;
		}

		float v;
		if (!m_CfgLoad.ReadValue(key, v))
		{
			CfgNotRead(key, "number", cur.ToString());
			return cur;
		}
		m_iCfgApplied++;
		float used = Math.Clamp(v, min, max);
		if (used != v)
			CfgClamped(key, v.ToString(), used.ToString());
		if (CfgDefMigrate(key, v.ToString(), cur.ToString(), true))
			used = cur;
		CfgTrace(key, used.ToString(), true);
		CfgNoteDiff(key, used.ToString(), cur.ToString(), true);
		return used;
	}

	protected int CfgI(string key, int cur, int min, int max)
	{
		if (m_iCfgMode == 1)
		{
			CfgDefNote(key, cur.ToString());
			CfgReadme(key, cur.ToString(), min.ToString() + " .. " + max.ToString() + " (whole number)");
			m_CfgSave.WriteValue(key, cur);
			return cur;
		}
		if (m_iCfgMode == 2)
		{
			CfgDumpAdd(key, cur.ToString());
			return cur;
		}

		int v;
		if (!m_CfgLoad.ReadValue(key, v))
		{
			CfgNotRead(key, "whole_number", cur.ToString());
			return cur;
		}
		m_iCfgApplied++;
		int used = Math.ClampInt(v, min, max);
		if (used != v)
			CfgClamped(key, v.ToString(), used.ToString());
		if (CfgDefMigrate(key, v.ToString(), cur.ToString(), true))
			used = cur;
		CfgTrace(key, used.ToString(), true);
		CfgNoteDiff(key, used.ToString(), cur.ToString(), true);
		return used;
	}

	protected string CfgS(string key, string cur)
	{
		if (m_iCfgMode == 1)
		{
			CfgDefNote(key, cur);
			CfgReadme(key, "\"" + cur + "\"", "text");
			m_CfgSave.WriteValue(key, cur);
			return cur;
		}
		if (m_iCfgMode == 2)
		{
			CfgDumpAdd(key, cur);
			return cur;
		}

		string v;
		if (!m_CfgLoad.ReadValue(key, v))
		{
			CfgNotRead(key, "text", cur);
			return cur;
		}
		m_iCfgApplied++;
		if (CfgDefMigrate(key, v, cur, false))
			v = cur;
		CfgTrace(key, v, true);
		CfgNoteDiff(key, v, cur, false);
		return v;
	}

	protected bool CfgB(string key, bool cur)
	{
		if (m_iCfgMode == 1)
		{
			CfgDefNote(key, cur.ToString());
			CfgReadme(key, cur.ToString(), "true / false");
			m_CfgSave.WriteValue(key, cur);
			return cur;
		}
		if (m_iCfgMode == 2)
		{
			CfgDumpAdd(key, cur.ToString());
			return cur;
		}

		bool v;
		if (!m_CfgLoad.ReadValue(key, v))
		{
			CfgNotRead(key, "true_false", cur.ToString());
			return cur;
		}
		m_iCfgApplied++;
		if (CfgDefMigrate(key, v.ToString(), cur.ToString(), false))
			v = cur;
		CfgTrace(key, v.ToString(), true);
		CfgNoteDiff(key, v.ToString(), cur.ToString(), false);
		return v;
	}

	protected void CfgNotRead(string key, string expected, string cur)
	{
		if (!m_sCfgFaction.IsEmpty())
			return;
		if (m_mCfgNewDef)
			m_mCfgNewDef.Set(key, cur);
		CfgTrace(key, cur, false);
		if (!m_CfgFileKeySet || !m_CfgFileKeySet.Contains(key))
		{
			m_aCfgMissing.Insert(key);
			return;
		}
		m_iCfgBadType++;
		DCO_Perf.Count("cfg:bad_type");
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", string.Format("cfg_bad_type key=%1 expected=%2 used=%3 file=%4 (value in file kept, fix it and restart)", key, expected, cur, m_sServerConfigPath));
	}

	protected void CfgDefNote(string key, string val)
	{
		if (!m_bCfgDefFromWrite)
			return;
		if (!m_mCfgNewDef)
			m_mCfgNewDef = new map<string, string>();
		m_mCfgNewDef.Set(key, val);
	}

	protected bool CfgDefMigrate(string key, string fileVal, string newDef, bool num)
	{
		if (!m_bCfgFileLoad)
			return false;
		if (!m_mCfgNewDef)
			m_mCfgNewDef = new map<string, string>();
		m_mCfgNewDef.Set(key, newDef);
		if (!m_mCfgOldDef)
		{
			string legacyOld = CfgLegacyDefault(key);
			if (!legacyOld.IsEmpty() && !CfgSame(legacyOld, newDef, num) && CfgSame(fileVal, legacyOld, num))
			{
				m_iCfgDefMigrated++;
				DCO_Perf.Count("cfg:default_migrated_legacy");
				DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", string.Format("cfg_default_migrated key=%1 file=%2 old_default=%3 new_default=%4 source=legacy_table (file from before the defaults record; this key is known to have changed its default)", key, fileVal, legacyOld, newDef));
				return true;
			}
			if (!CfgSame(fileVal, newDef, num))
			{
				m_iCfgLegacyDiff++;
				if (m_iCfgLegacyDiff <= 40)
					DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cfg_differs_from_default key=%1 file=%2 default=%3 (no defaults record yet: edited by an admin or an old frozen default, check it)", key, fileVal, newDef));
			}
			return false;
		}
		string old;
		if (!m_mCfgOldDef.Find(key, old) || CfgSame(old, newDef, num) || !CfgSame(fileVal, old, num))
			return false;
		m_iCfgDefMigrated++;
		DCO_Perf.Count("cfg:default_migrated");
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", string.Format("cfg_default_migrated key=%1 file=%2 old_default=%3 new_default=%4 (value was never edited, so it follows the new default)", key, fileVal, old, newDef));
		return true;
	}

	protected static string CfgLegacyDefault(string key)
	{
		if (key == "morale_panic_max_s" || key == "ctx_on" || key == "ctx_cqb_gate" || key == "tune_garrison_arrive_start" || key == "tune_agg_profile")
			return "0";
		if (key == "tune_tb_on" || key == "tune_tgt_claim" || key == "tune_mb_on" || key == "tune_mb_field" || key == "tune_tb_fm")
			return "0";
		if (key == "tune_tb_set_frac" || key == "tune_mb_field_ring2_m" || key == "cqb_precheck" || key == "tune_tb_fm_gate" || key == "tune_tb_mg_ow")
			return "0";
		if (key == "tune_mb_mg_exempt" || key == "tune_thr_flank_rel" || key == "tune_thr_flank_retarget" || key == "tune_reinf_def_shortfall" || key == "tune_agg_pursue")
			return "0";
		if (key == "tune_grp_bc_not_free" || key == "tune_def_garrison_floor" || key == "tune_obj_phase_on" || key == "tune_agg_k2" || key == "tune_agg_postfrag")
			return "0";
		if (key == "tune_agg_usage_map" || key == "tune_opp_exposed" || key == "tune_opp_suppressed_push" || key == "tune_agg_retfire" || key == "tune_opp_distracted")
			return "0";
		if (key == "tune_opp_patient" || key == "tune_opp_reload" || key == "tune_opp_loot" || key == "open_ground_rule_v2" || key == "tune_fm_auto")
			return "0";
		if (key == "tune_gren_ff_predict" || key == "tune_mb_v2" || key == "tune_ff_vert" || key == "tune_cov_on" || key == "tune_cqb_kind_beh")
			return "0";
		if (key == "tune_osc_fix_dir" || key == "tune_osc_fix_commit" || key == "tune_osc_fix_cover" || key == "tune_osc_fix_short" || key == "tune_spec_bt")
			return "0";
		if (key == "tune_learn_ind_on" || key == "tune_opp_cluster_scan" || key == "tune_def_obj_priority" || key == "tune_posture_release_exec" || key == "tune_cmd_retreat_thr")
			return "0";
		if (key == "tune_tac_fix_los" || key == "tune_agg_cmd_profile" || key == "tune_sa_hazard_skip" || key == "tune_intel_est_blend" || key == "tune_obj_role_on")
			return "0";
		if (key == "tune_agg_bound_v2" || key == "tune_gren_atk_close" || key == "tune_rearm_end_rel" || key == "tune_garrison_leftover_field")
			return "0";
		if (key == "attacker_close_in" || key == "combat_move_dir_decode" || key == "break_contact_when_shaken" || key == "break_contact_ladder_slider")
			return "0";
		if (key == "ctx_mg_floor" || key == "ctx_sup_on" || key == "ctx_hide_on" || key == "heal_unseen_gate")
			return "0";
		if (key == "dodge_cover_vs_shooter" || key == "dodge_nearmiss_gate" || key == "dodge_dir_assist")
			return "0";
		if (key == "tune_learn_ind_pick")
			return "1";
		if (key == "tune_pc_night_flare_m")
			return "400";
		if (key == "tune_tgt_claim_pen")
			return "2.5";
		return string.Empty;
	}

	protected void CfgNoteDiff(string key, string v, string def, bool num)
	{
		if (!CfgSame(v, def, num))
			DCO_Report.Setting(m_sCfgFaction, key, v, def);
	}

	protected static bool CfgSame(string a, string b, bool num)
	{
		if (!num)
			return a == b;
		float fa = a.ToFloat();
		float fb = b.ToFloat();
		return Math.AbsFloat(fa - fb) <= 0.0001 * Math.Max(1.0, Math.AbsFloat(fb));
	}

	protected string CfgDefaultsPath()
	{
		string world = GetGame().GetWorldFile();
		int slash = world.LastIndexOf("/");
		if (slash >= 0)
			world = world.Substring(slash + 1, world.Length() - slash - 1);
		world.Replace(".ent", "");
		array<string> bad = {"{", "}", ".", ":", " "};
		foreach (string ch : bad)
			world.Replace(ch, "_");
		if (world.IsEmpty())
			world = "world";
		string name = "DCO_GlobalConfig.defaults." + world + ".txt";
		string dir = GetDirectoryFromPath(m_sServerConfigPath);
		if (dir.IsEmpty())
			return name;
		return dir + "/" + name;
	}

	protected void CfgReadDefaults()
	{
		m_mCfgOldDef = null;
		string path = CfgDefaultsPath();
		if (!FileIO.FileExists(path))
		{
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", "cfg_defaults_missing path=" + path + " (first start of this world with this version: a defaults record for this world is created now, later default changes migrate)");
			return;
		}
		FileHandle f = FileIO.OpenFile(path, FileMode.READ);
		if (!f)
			return;
		m_mCfgOldDef = new map<string, string>();
		string line;
		int guard;
		while (guard < 20000 && f.ReadLine(line) >= 0)
		{
			guard++;
			if (line.IsEmpty() || line.StartsWith("#"))
				continue;
			int tab = line.IndexOf("=");
			if (tab <= 0)
				continue;
			m_mCfgOldDef.Set(line.Substring(0, tab), line.Substring(tab + 1, line.Length() - tab - 1));
		}
		f.Close();
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cfg_defaults_loaded path=%1 keys=%2", path, m_mCfgOldDef.Count()));
	}

	protected void CfgWriteDefaults(string why)
	{
		if (!m_mCfgNewDef || m_mCfgNewDef.IsEmpty())
			return;
		string path = CfgDefaultsPath();
		FileHandle f = FileIO.OpenFile(path, FileMode.WRITE);
		if (!f)
		{
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", "cfg_defaults_write_fail path=" + path);
			return;
		}
		f.WriteLine("# DCO config defaults of world " + GetGame().GetWorldFile() + " at the last write of " + m_sServerConfigPath + ". One record per world, so scenario rotation does not migrate. Machine file, do not edit: a JSON value equal to its old default here follows the new code default.");
		foreach (string k, string v : m_mCfgNewDef)
			f.WriteLine(k + "=" + v);
		f.Close();
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cfg_defaults_written path=%1 keys=%2 why=%3", path, m_mCfgNewDef.Count(), why));
	}

	protected void CfgReadFileKeys()
	{
		m_aCfgFileKeys = {};
		m_CfgFileKeySet = new set<string>();
		FileHandle f = FileIO.OpenFile(m_sServerConfigPath, FileMode.READ);
		if (!f)
			return;
		string text;
		string line;
		int guard;
		while (guard < 20000 && f.ReadLine(line) >= 0)
		{
			guard++;
			text += line + " ";
		}
		f.Close();
		m_sCfgFactions = string.Empty;
		m_aCfgFactionOk = null;
		m_bCfgFactionsBad = false;
		array<string> top = {};
		int s;
		int e;
		if (CfgObjAt(text, "factions", top, s, e))
		{
			for (int bi = s; bi <= e; bi++)
				m_sCfgFactions += text.Get(bi);
			array<string> names = {};
			int ns;
			int ne;
			CfgObjAt(m_sCfgFactions, string.Empty, names, ns, ne);
			string list;
			foreach (string fn : names)
			{
				if (!list.IsEmpty())
					list += ",";
				list += fn;
			}
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cfg_factions_found n=%1 factions=%2 file=%3", names.Count(), list, m_sServerConfigPath));
		}
		else if (top.Contains("factions"))
		{
			m_bCfgFactionsBad = true;
			DCO_Perf.Count("cfg:factions_not_object");
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", "cfg_factions_not_object file=" + m_sServerConfigPath + " (\"factions\" must be an object like {\"USSR\": {...}}; ignored, and the file is not rewritten or saved from GM so it is not lost)");
		}
		CfgJsonKeys(text, m_aCfgFileKeys, s, e);
		foreach (string key : m_aCfgFileKeys)
			m_CfgFileKeySet.Insert(key);
	}

	static bool CfgObjAt(string text, string want, notnull array<string> keys, out int objStart, out int objEnd)
	{
		objStart = -1;
		objEnd = -1;
		bool found;
		int n = text.Length();
		int depth;
		int i;
		while (i < n)
		{
			string c = text.Get(i);
			if (c == "\"")
			{
				int j = i + 1;
				while (j < n)
				{
					string d = text.Get(j);
					if (d == "\\")
					{
						j += 2;
						continue;
					}
					if (d == "\"")
						break;
					j++;
				}
				if (depth == 1)
				{
					int k = j + 1;
					while (k < n && text.Get(k).Trim() == "")
						k++;
					if (k < n && text.Get(k) == ":")
					{
						string key = text.Substring(i + 1, j - i - 1);
						keys.Insert(key);
						if (!found && !want.IsEmpty() && key == want)
						{
							int v = k + 1;
							while (v < n && text.Get(v).Trim() == "")
								v++;
							if (v < n && text.Get(v) == "{")
							{
								int end = CfgMatch(text, v);
								if (end > v)
								{
									objStart = v;
									objEnd = end;
									found = true;
								}
							}
						}
					}
				}
				i = j + 1;
				continue;
			}
			if (c == "{" || c == "[")
				depth++;
			else if (c == "}" || c == "]")
				depth--;
			i++;
		}
		return found;
	}

	static int CfgMatch(string text, int open)
	{
		int n = text.Length();
		int depth;
		int i = open;
		while (i < n)
		{
			string c = text.Get(i);
			if (c == "\"")
			{
				i++;
				while (i < n && text.Get(i) != "\"")
				{
					if (text.Get(i) == "\\")
						i++;
					i++;
				}
				i++;
				continue;
			}
			if (c == "{" || c == "[")
				depth++;
			else if (c == "}" || c == "]")
			{
				depth--;
				if (depth == 0)
					return i;
			}
			i++;
		}
		return -1;
	}

	static void DCO_CfgFactionKeys(notnull array<string> names)
	{
		DCO_GlobalAIComponent g = GetInstance();
		if (!g || !g.m_aCfgFactionOk)
			return;
		foreach (string n : g.m_aCfgFactionOk)
			names.Insert(n);
	}

	static bool DCO_CfgAnyFaction()
	{
		DCO_GlobalAIComponent g = GetInstance();
		return g && !g.m_sCfgFactions.IsEmpty();
	}

	static void DCO_CfgValidateFactions()
	{
		DCO_GlobalAIComponent g = GetInstance();
		if (!g || g.m_sCfgFactions.IsEmpty())
			return;
		g.m_aCfgFactionOk = {};
		array<string> names = {};
		int s;
		int e;
		CfgObjAt(g.m_sCfgFactions, string.Empty, names, s, e);
		FactionManager fm = GetGame().GetFactionManager();
		foreach (string n : names)
		{
			array<string> dummy = {};
			string why;
			bool body;
			if (CfgObjAt(g.m_sCfgFactions, n, dummy, s, e))
			{
				for (int ci = s + 1; ci < e; ci++)
				{
					if (g.m_sCfgFactions.Get(ci).Trim() != "")
					{
						body = true;
						break;
					}
				}
				if (!body)
					why = "empty";
				else if (fm && !fm.GetFactionByKey(n))
					why = "unknown_faction";
			}
			else
				why = "not_object";
			if (!why.IsEmpty())
			{
				DCO_Perf.Count("cfg:faction_skip");
				DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", string.Format("cfg_faction_skip faction=%1 reason=%2 file=%3 (no faction layer built; the entry stays in the file)", n, why, g.m_sServerConfigPath));
				continue;
			}
			if (!g.m_aCfgFactionOk.Contains(n))
				g.m_aCfgFactionOk.Insert(n);
		}
	}

	protected static bool CfgGlobalOnly(string key)
	{
		return key.StartsWith("doctrine") || key.StartsWith("groupPreset") || key == "gmSaveConfig" || key == "gmAutoSave" || key == "startupPresetPrompt" || key == "perfProfiling";
	}

	static bool DCO_CfgHasFaction(string factionKey)
	{
		array<string> names = {};
		DCO_CfgFactionKeys(names);
		return names.Contains(factionKey);
	}

	bool DCO_CfgApplyFaction(string factionKey)
	{
		DCO_GlobalAIComponent g = GetInstance();
		if (!g || g.m_sCfgFactions.IsEmpty() || factionKey.IsEmpty())
			return false;
		array<string> names = {};
		int s;
		int e;
		if (!CfgObjAt(g.m_sCfgFactions, factionKey, names, s, e))
			return false;
		string sub;
		for (int bi = s; bi <= e; bi++)
			sub += g.m_sCfgFactions.Get(bi);
		SCR_JsonLoadContext ctx = new SCR_JsonLoadContext();
		if (!ctx.ImportFromString(sub))
		{
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.ERROR, "-", string.Format("cfg_faction_parse_fail faction=%1 file=%2 (this faction block is skipped)", factionKey, g.m_sServerConfigPath));
			return false;
		}
		array<string> keys = {};
		CfgObjAt(sub, string.Empty, keys, s, e);
		m_sCfgFaction = factionKey;
		m_iCfgFactionApplied = 0;
		m_iCfgFactionGlobalOnly = 0;
		m_CfgSeen = new set<string>();
		m_iCfgMode = 0;
		m_CfgLoad = ctx;
		SyncConfig();
		m_CfgLoad = null;
		m_sCfgFaction = string.Empty;
		int unknown;
		foreach (string k : keys)
		{
			if (m_CfgSeen.Contains(k))
				continue;
			unknown++;
			DCO_Perf.Count("cfg:faction_unknown_key");
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", string.Format("cfg_faction_unknown_key faction=%1 key=%2 (unknown key or wrong value type, ignored) file=%3", factionKey, k, g.m_sServerConfigPath));
		}
		m_CfgSeen = null;
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cfg_faction_summary faction=%1 keys=%2 applied=%3 unknown=%4 global_only=%5", factionKey, keys.Count(), m_iCfgFactionApplied, unknown, m_iCfgFactionGlobalOnly));
		return true;
	}

	protected void DCO_OnCfgLoaded()
	{
	}

	protected void CfgTrace(string key, string value, bool fromJson)
	{
		if (!m_sCfgFaction.IsEmpty())
		{
			if (!fromJson)
				return;
			m_CfgSeen.Insert(key);
			if (CfgGlobalOnly(key))
			{
				m_iCfgFactionGlobalOnly++;
				DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", string.Format("cfg_faction_global_only faction=%1 key=%2 (this setting is global only; set it at the top level, ignored here)", m_sCfgFaction, key));
				return;
			}
			m_iCfgFactionApplied++;
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cfg_faction_applied faction=%1 key=%2 value=%3", m_sCfgFaction, key, value));
			return;
		}
		if (!m_bCfgTrace)
			return;
		if (!m_CfgSeen)
			m_CfgSeen = new set<string>();
		m_CfgSeen.Insert(key);
		string source = "default";
		if (fromJson)
		{
			source = "json";
			m_iCfgFromJson++;
		}
		else
			m_iCfgFromDefault++;
		m_sCfgTraceBuf += " " + key + "=" + value + "/" + source;
		m_iCfgTraceN++;
		if (m_iCfgTraceN >= 20)
			CfgTraceFlush();
	}

	protected void CfgTraceFlush()
	{
		if (m_iCfgTraceN > 0)
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", "cfg_applied n=" + m_iCfgTraceN + m_sCfgTraceBuf);
		m_sCfgTraceBuf = string.Empty;
		m_iCfgTraceN = 0;
	}

	protected void CfgClamped(string key, string raw, string used)
	{
		DCO_Perf.Count("cfg:clamped");
		string scope = "global";
		if (!m_sCfgFaction.IsEmpty())
			scope = "faction:" + m_sCfgFaction;
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", string.Format("cfg_clamped key=%1 raw=%2 used=%3 scope=%4 file=%5", key, raw, used, scope, m_sServerConfigPath));
	}

	protected void CfgReportUnused()
	{
		if (!m_CfgSeen)
			m_CfgSeen = new set<string>();
		m_CfgSeen.Insert("personalityWeightStandard");
		m_CfgSeen.Insert("timeToMaxAccuracy");
		m_CfgSeen.Insert("aimModel");
		m_CfgSeen.Insert("_comment");
		m_CfgSeen.Insert("factions");
		array<string> keys = m_aCfgFileKeys;
		if (!keys)
			keys = {};
		int unused;
		foreach (string key : keys)
		{
			if (m_CfgSeen.Contains(key))
				continue;
			unused++;
			DCO_Perf.Count("cfg:unused");
			if (unused <= 40)
				DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", string.Format("cfg_unused key=%1 file=%2", key, m_sServerConfigPath));
		}
		m_iCfgUnused = unused;
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cfg_summary file=%1 keys_in_file=%2 from_json=%3 from_default=%4 unused=%5 bad_type=%6", m_sServerConfigPath, keys.Count(), m_iCfgFromJson, m_iCfgFromDefault, unused, m_iCfgBadType));
	}

	static void CfgJsonKeys(string text, notnull array<string> keys, int skipFrom = -1, int skipTo = -1)
	{
		int n = text.Length();
		string prev = "";
		int i = 0;
		while (i < n)
		{
			if (i == skipFrom && skipTo > skipFrom)
			{
				i = skipTo + 1;
				prev = "}";
				continue;
			}
			string c = text.Get(i);
			if (c != "\"")
			{
				if (c != " " && c != "\t" && c != "\r" && c != "\n")
					prev = c;
				i++;
				continue;
			}
			int start = i + 1;
			int j = start;
			while (j < n)
			{
				string d = text.Get(j);
				if (d == "\\")
				{
					j += 2;
					continue;
				}
				if (d == "\"")
					break;
				j++;
			}
			int k = j + 1;
			while (k < n && (text.Get(k) == " " || text.Get(k) == "\t" || text.Get(k) == "\r" || text.Get(k) == "\n"))
				k++;
			if ((prev == "{" || prev == ",") && k < n && text.Get(k) == ":" && j > start)
				keys.Insert(text.Substring(start, j - start));
			prev = "\"";
			i = j + 1;
		}
	}

	protected void CfgCat(string name)
	{
		if (m_iCfgMode == 1)
			m_sCfgCat = name;
		if (m_iCfgMode != 2)
			return;
		CfgFlush();
		m_sCfgCat = name;
	}

	protected void CfgReadme(string key, string def, string range)
	{
		if (!m_aCfgReadme)
			return;
		string real;
		if (!m_bCfgDefFromWrite && m_mCfgNewDef && m_mCfgNewDef.Find(key, real))
			def = real;
		m_aCfgReadme.Insert(string.Format("%1 | %2 | default %3 | %4", key, m_sCfgCat, def, range));
	}

	protected void WriteConfigReadme()
	{
		if (!m_aCfgReadme)
			return;
		string dir = GetDirectoryFromPath(m_sServerConfigPath);
		string path = "DCO_GlobalConfig_README.txt";
		if (!dir.IsEmpty())
			path = dir + "/" + path;
		FileHandle f = FileIO.OpenFile(path, FileMode.WRITE);
		if (!f)
		{
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", "cfg_readme_fail path=" + path);
			return;
		}
		f.WriteLine("DCO Global AI server config: " + m_sServerConfigPath);
		f.WriteLine("Edit the JSON while the server is stopped. Values outside the range are clamped and logged as cfg_clamped in dco.log.");
		f.WriteLine("Every key read at start is logged in cfg_applied lines (20 keys per line, each as key=value/json or key=value/default). Keys no code reads are logged as cfg_unused.");
		f.WriteLine("Per-faction values: add a block \"factions\": {\"USSR\": {\"aiAggression\": 1.5, \"tune_agg_profile\": 1}} with any keys from this list. They apply to that faction only, on top of the global values; GM faction settings apply on top of them. Logged as cfg_faction_applied / cfg_faction_unknown_key. The block is kept when the file is rewritten. Global-only keys (doctrine*, groupPreset*, gmSaveConfig, startupPresetPrompt, perfProfiling) are ignored inside a faction block (cfg_faction_global_only). Entries whose faction key is unknown, empty or not an object are skipped (cfg_faction_skip). GM Save to server config writes the factions block as it was at server start, so edits made on disk while the server runs are overwritten.");
		f.WriteLine("Format: key | category | default | allowed range");
		f.WriteLine("");
		foreach (string line : m_aCfgReadme)
			f.WriteLine(line);
		f.Close();
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cfg_readme_written path=%1 keys=%2", path, m_aCfgReadme.Count()));
	}

	protected void CfgDumpAdd(string key, string val)
	{
		if (m_iCfgLineKeys >= 6)
			CfgFlush();
		m_sCfgLine += " " + key + "=" + val;
		m_iCfgLineKeys++;
	}

	protected void CfgFlush()
	{
		if (m_iCfgMode == 2 && m_iCfgLineKeys > 0)
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", m_sCfgCat + ":" + m_sCfgLine);
		m_sCfgLine = string.Empty;
		m_iCfgLineKeys = 0;
	}

	protected void LoadServerConfig()
	{
		if (!m_bUseServerConfigFile)
		{
			DCO_Report.Config("not used (this world has Use Server Config File off, so settings come from the world itself)");
			return;
		}

		if (!Replication.IsServer())
			return;

		if (m_sServerConfigPath.IsEmpty())
		{
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", "Config path is empty, skipped.");
			DCO_Report.Config("not used (the config path is empty)");
			return;
		}

		if (!FileIO.FileExists(m_sServerConfigPath))
		{
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("File %1 not found.", m_sServerConfigPath));
			if (m_bAutoGenerateConfigFile)
				DCO_Report.Config("not found, so a new file with Recommended values was written: " + m_sServerConfigPath);
			else
				DCO_Report.Config("not found (" + m_sServerConfigPath + "), using the world values");

			if (m_bAutoGenerateConfigFile)
			{
				m_mCfgNewDef = new map<string, string>();
				m_bCfgDefFromWrite = true;
				bool wrote = WriteDefaultConfig();
				m_bCfgDefFromWrite = false;
				if (wrote)
					CfgWriteDefaults("new_file");
			}

			DumpActiveConfig();
			return;
		}

		SCR_JsonLoadContext ctx = new SCR_JsonLoadContext();
		if (!ctx.LoadFromFile(m_sServerConfigPath))
		{
			m_bCfgParseFailed = true;
			DCO_Perf.Count("cfg:parse_fail");
			DCO_Report.Config("INVALID JSON, the file was ignored. Fix it and restart: " + m_sServerConfigPath);
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.ERROR, "-", string.Format("cfg_parse_fail file=%1 -- invalid JSON (stray comma / missing bracket / unclosed quote). File NOT changed. Settings keep their previous values (Workbench values on a fresh start). Check the format in DCO_GlobalConfig_README.txt, fix it, then restart.", m_sServerConfigPath));
			DumpActiveConfig();
			return;
		}

		m_bCfgParseFailed = false;
		DCO_Report.Config("read from " + m_sServerConfigPath);
		CfgReadFileKeys();
		CfgReadDefaults();
		m_mCfgNewDef = new map<string, string>();
		m_iCfgDefMigrated = 0;
		m_iCfgLegacyDiff = 0;
		m_iCfgBadType = 0;
		m_iCfgMode = 0;
		m_CfgLoad = ctx;
		m_aCfgMissing.Clear();
		m_iCfgApplied = 0;
		m_iCfgFromJson = 0;
		m_iCfgFromDefault = 0;
		m_CfgSeen = new set<string>();
		m_sCfgTraceBuf = string.Empty;
		m_iCfgTraceN = 0;
		m_bCfgTrace = true;
		m_bCfgFileLoad = true;
		SyncConfig();
		m_bCfgFileLoad = false;
		m_bCfgTrace = false;
		CfgTraceFlush();
		CfgReportUnused();

		bool migrated = false;
		bool aimMigrated = false;
		float legacy;
		if (m_aCfgMissing.Contains("personalityWeightMeasured") && ctx.ReadValue("personalityWeightStandard", legacy))
		{
			m_fPersonalityWeightStandard = Math.Clamp(legacy, 0.0, 100.0);
			m_aCfgMissing.RemoveItem("personalityWeightMeasured");
			m_iCfgApplied++;
			migrated = true;
		}
		if (m_CfgFileKeySet && m_CfgFileKeySet.Contains("timeToMaxAccuracy"))
		{
			float oldTime = -1;
			ctx.ReadValue("timeToMaxAccuracy", oldTime);
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.WARN, "-", string.Format("aim_cfg_migrated file=%1 old_accuracy=%2 old_time=%3", m_sServerConfigPath, unitAimSkillAccuracy, oldTime));
			unitAimSkillAccuracy = 1.0;
			aimMigrated = true;
		}
		if (m_CfgFileKeySet && m_CfgFileKeySet.Contains("aimModel"))
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", "cfg_key_retired key=aimModel (had no effect, dropped at the next rewrite of " + m_sServerConfigPath + ")");
		m_CfgLoad = null;

		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("%1 settings overridden from %2", m_iCfgApplied, m_sServerConfigPath));

		bool rewrote;
		if (m_bAutoGenerateConfigFile && (migrated || aimMigrated || !m_aCfgMissing.IsEmpty() || m_iCfgDefMigrated > 0))
		{
			string skipWhy = "bad_type";
			if (m_iCfgBadType == 0)
				skipWhy = "factions_not_object";
			if (m_iCfgBadType > 0 || m_bCfgFactionsBad)
				DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", string.Format("cfg_merge_skipped reason=%4 bad=%1 missing=%2 file=%3 (file not rewritten so admin values stay)", m_iCfgBadType, m_aCfgMissing.Count(), m_sServerConfigPath, skipWhy));
			else
				rewrote = MergeMissingKeys(migrated);
		}
		if (m_iCfgDefMigrated == 0 || rewrote)
			CfgWriteDefaults("load");
		else
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", string.Format("cfg_defaults_kept migrated=%1 file=%2 (file not rewritten, so the old defaults record stays and the migration repeats at next start)", m_iCfgDefMigrated, m_sServerConfigPath));
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cfg_defaults_summary migrated=%1 legacy_differs=%2 record=%3 rewrote=%4", m_iCfgDefMigrated, m_iCfgLegacyDiff, m_mCfgOldDef != null, rewrote));
		m_aCfgFileKeys = null;
		m_CfgFileKeySet = null;

		ApplyWorkbenchDefaults();
		ApplyPerfProfiling();
		DumpActiveConfig();
		LogAimCfg();
		if (!m_sCfgFactions.IsEmpty())
			DCO_OnCfgLoaded();
	}

	protected bool MergeMissingKeys(bool migrated)
	{
		if (m_iCfgUnused > 0)
		{
			string unknownBak = m_sServerConfigPath + ".unknown.bak";
			if (FileIO.FileExists(unknownBak))
				FileIO.DeleteFile(unknownBak);
			if (SCR_FileIOHelper.CopyFile(m_sServerConfigPath, unknownBak))
				DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", string.Format("cfg_unknown_backup keys=%1 copy=%2 (the merge below drops unknown keys from %3)", m_iCfgUnused, unknownBak, m_sServerConfigPath));
		}
		string bak = m_sServerConfigPath + ".bak";
		if (FileIO.FileExists(bak))
			FileIO.DeleteFile(bak);
		if (!SCR_FileIOHelper.CopyFile(m_sServerConfigPath, bak))
		{
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.ERROR, "-", string.Format("FAILED to back up to %1 -- config file not changed.", bak));
			return false;
		}

		if (!WriteDefaultConfig())
			return false;

		string added;
		foreach (string key : m_aCfgMissing)
		{
			if (!added.IsEmpty())
				added += ", ";
			added += key;
		}
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("%1 new keys added to %2 (backup: %3): %4", m_aCfgMissing.Count(), m_sServerConfigPath, bak, added));
		if (migrated)
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", "personalityWeightStandard renamed to personalityWeightMeasured");
		if (m_iCfgDefMigrated > 0)
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cfg_default_migrated_written n=%1 file=%2", m_iCfgDefMigrated, m_sServerConfigPath));
		return true;
	}

	bool DCO_SaveActiveConfig(int playerId, out string path)
	{
		path = m_sServerConfigPath;
		if (!Replication.IsServer() || !m_bUseServerConfigFile || m_sServerConfigPath.IsEmpty())
		{
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", string.Format("cfg_save_gm result=skip player=%1 reason=config_file_off path=%2", playerId, m_sServerConfigPath));
			return false;
		}
		if (m_bCfgParseFailed || m_bCfgFactionsBad)
		{
			string why = "parse_fail";
			if (!m_bCfgParseFailed)
				why = "factions_not_object";
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.WARN, "-", string.Format("cfg_save_gm result=skip player=%1 reason=%2 path=%3 (fix the file on disk first, then restart; saving now would overwrite the admin file)", playerId, why, m_sServerConfigPath));
			return false;
		}
		string bak = m_sServerConfigPath + ".bak";
		if (FileIO.FileExists(m_sServerConfigPath))
		{
			if (FileIO.FileExists(bak))
				FileIO.DeleteFile(bak);
			if (!SCR_FileIOHelper.CopyFile(m_sServerConfigPath, bak))
			{
				DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.ERROR, "-", string.Format("cfg_save_gm result=fail player=%1 reason=backup_fail bak=%2", playerId, bak));
				return false;
			}
		}
		bool ok = WriteDefaultConfig();
		string result = "fail";
		if (ok)
			result = "ok";
		DCO_Perf.Count("cfg:save_gm_" + result);
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cfg_save_gm result=%1 player=%2 path=%3 bak=%4", result, playerId, m_sServerConfigPath, bak));
		return ok;
	}

	protected bool WriteDefaultConfig()
	{
		string dir = GetDirectoryFromPath(m_sServerConfigPath);
		if (!dir.IsEmpty())
			FileIO.MakeDirectory(dir);

		m_CfgSave = new SCR_JsonSaveContext();
		m_CfgSave.SetMaxDecimalPlaces(6);
		m_CfgSave.WriteValue("_comment", "DCO Global AI server config. Every key, its default and allowed range are listed in DCO_GlobalConfig_README.txt next to this file. Missing keys are added with the current values at server start (old file copied to .bak).");

		m_aCfgReadme = {};
		m_sCfgCat = "General";
		m_iCfgMode = 1;
		SyncConfig();
		m_iCfgMode = 0;
		WriteConfigReadme();
		m_aCfgReadme = null;

		string raw = m_CfgSave.ExportToString();
		m_CfgSave = null;
		int last = raw.Length() - 1;
		while (last > 0 && raw.Get(last) != "}")
			last--;
		string extra;
		if (!m_sCfgFactions.IsEmpty() && last > 0)
		{
			extra = ",\"factions\":" + m_sCfgFactions;
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", "cfg_factions_kept file=" + m_sServerConfigPath + " (the factions block is written back unchanged)");
		}
		bool ok = CfgWritePretty(m_sServerConfigPath, raw, extra, last);

		if (ok)
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("Config written to %1", m_sServerConfigPath));
		else
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.ERROR, "-", string.Format("FAILED to write %1 -- check the profile folder permissions.", m_sServerConfigPath));
		return ok;
	}

	protected bool CfgWritePretty(string path, string raw, string extra = "", int insertAt = -1)
	{
		array<string> lines = {};
		string cur;
		string pad;
		bool inStr;
		bool escape;
		int m = extra.Length();
		if (insertAt < 0)
			m = 0;
		int n = raw.Length() + m;
		for (int i = 0; i < n; i++)
		{
			string c;
			if (m == 0 || i < insertAt)
				c = raw.Get(i);
			else if (i < insertAt + m)
				c = extra.Get(i - insertAt);
			else
				c = raw.Get(i - m);
			if (inStr)
			{
				cur += c;
				if (escape)
					escape = false;
				else if (c == "\\")
					escape = true;
				else if (c == "\"")
					inStr = false;
				continue;
			}
			if (c == "\"")
			{
				cur += c;
				inStr = true;
			}
			else if (c == "{" || c == "[")
			{
				lines.Insert(cur + c);
				pad += "  ";
				cur = pad;
			}
			else if (c == "}" || c == "]")
			{
				if (cur.Trim() != "")
					lines.Insert(cur);
				if (pad.Length() >= 2)
					pad = pad.Substring(0, pad.Length() - 2);
				cur = pad + c;
			}
			else if (c == ",")
			{
				lines.Insert(cur + c);
				cur = pad;
			}
			else if (c == ":")
				cur += ": ";
			else if (c.Trim() != "")
				cur += c;
		}
		if (cur.Trim() != "")
			lines.Insert(cur);
		FileHandle f = FileIO.OpenFile(path, FileMode.WRITE);
		if (!f)
			return false;
		foreach (string line : lines)
			f.WriteLine(line);
		f.Close();
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cfg_written_pretty path=%1 lines=%2 bytes=%3", path, lines.Count(), n));
		return true;
	}

	protected string GetDirectoryFromPath(string path)
	{
		int lastSlash = -1;
		int len = path.Length();

		for (int i = 0; i < len; i++)
		{
			string ch = path.Get(i);
			if (ch == "/" || ch == "\\")
				lastSlash = i;
		}

		if (lastSlash <= 0)
			return string.Empty;

		return path.Substring(0, lastSlash);
	}

	void DumpActiveConfig()
	{
		m_iCfgMode = 2;
		SyncConfig();
		m_iCfgMode = 0;
	}

	float GetProneChance()				{ return m_fProneChance; }
	void SetProneChance(float f)		{ m_fProneChance = Math.Clamp(f, 0.0, 1.0); }
	float GetCQBProneChance()			{ return m_fCQBProneChance; }
	void SetCQBProneChance(float f)		{ m_fCQBProneChance = Math.Clamp(f, 0.0, 1.0); }

	float GetDodgeChance()
	{
		return m_fDodgeChance;
	}

	float SetDodgeChance(float f)
	{
		m_fDodgeChance = f;
		return m_fDodgeChance;
	}

	float GetDodgeCooldown()
	{
		return m_fDodgeCooldown;
	}

	float SetDodgeCooldown(float f)
	{
		m_fDodgeCooldown = f;
		return m_fDodgeCooldown;
	}

	float GetDodgeMaxDist()
	{
		return m_fDodgeMaxDist;
	}

	float SetDodgeMaxDist(float f)
	{
		m_fDodgeMaxDist = f;
		return m_fDodgeMaxDist;
	}

	float GetDodgeSearchDist()
	{
		return m_fDodgeSearchDist;
	}

	float SetDodgeSearchDist(float f)
	{
		m_fDodgeSearchDist = f;
		return m_fDodgeSearchDist;
	}

	bool GetDodgeScaleByPersonality()
	{
		return m_bDodgeScaleByPersonality;
	}

	bool SetDodgeScaleByPersonality(bool b)
	{
		m_bDodgeScaleByPersonality = b;
		return m_bDodgeScaleByPersonality;
	}

	int GetDodgeShotThreshold()
	{
		return m_iDodgeShotThreshold;
	}

	int SetDodgeShotThreshold(int i)
	{
		m_iDodgeShotThreshold = Math.ClampInt(i, 1, 20);
		return m_iDodgeShotThreshold;
	}

	float GetDodgeShotWindow()
	{
		return m_fDodgeShotWindow;
	}

	float SetDodgeShotWindow(float f)
	{
		m_fDodgeShotWindow = Math.Clamp(f, 1.0, 30.0);
		return m_fDodgeShotWindow;
	}

	void ReloadServerConfig()
	{
		if (!Replication.IsServer())
			return;

		s_bConfigLoaded = true;
		LoadServerConfig();
		ReapplyCommanderLOD("config_reload");
	}

	DCO_AISKILL SetAISkill(DCO_AISKILL ski)
	{
		m_eAISkillDefault = ski;
		return m_eAISkillDefault;
	}

	DCO_AISKILL GetAISkill()
	{
		return m_eAISkillDefault;
	}

	DCO_EDisembarkRule GetDisembarkRule()
	{
		return m_eDisembarkRule;
	}

	void SetDisembarkRule(int rule)
	{
		m_eDisembarkRule = Math.ClampInt(rule, DCO_EDisembarkRule.AUTO, DCO_EDisembarkRule.NEVER);
	}

	int GetVehicleExit()
	{
		return m_iVehicleExit & 31;
	}

	void SetVehicleExit(int flags)
	{
		m_iVehicleExit = flags & 31;
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("vehicle_exit_set scope=global flags=%1", m_iVehicleExit));
	}

	static int VehicleExitFromRule(int rule)
	{
		if (rule == DCO_EDisembarkRule.DISABLED_ONLY)
			return DCO_EVehicleExit.ORDERED | DCO_EVehicleExit.DAMAGED;
		if (rule == DCO_EDisembarkRule.ORDERS_ONLY)
			return DCO_EVehicleExit.ORDERED;
		if (rule == DCO_EDisembarkRule.NEVER)
			return 0;
		return 31;
	}

	float GetDismountDistance()
	{
		return m_fVehicleDismountDanger;
	}

	float SetDismountDistance(float acc)
	{
		m_fVehicleDismountDanger = acc;
		return m_fVehicleDismountDanger;
	}

	float GetAimSpeed()
	{
		return m_fAimSpeed;
	}

	void SetAimSpeed(float v)
	{
		m_fAimSpeed = Math.Clamp(v, 0.25, 4.0);
		LogAimCfg();
	}

	float GetAimShake()
	{
		return m_fAimShake;
	}

	void SetAimShake(float v)
	{
		m_fAimShake = Math.Clamp(v, 0.25, 4.0);
		LogAimCfg();
	}

	float GetUnitSkill()
	{
		return unitAimSkillAccuracy;
	}

	float SetUnitSkill(float sk)
	{
		unitAimSkillAccuracy = sk;
		LogAimCfg();
		return sk;
	}

	void LogAimCfg()
	{
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-", string.Format("aim_cfg scope=global accuracy=%1 eff=%4 shake=%2 speed=%3 model=angle floor_mrad=0.2", unitAimSkillAccuracy, m_fAimShake, m_fAimSpeed, unitAimSkillAccuracy / 4.6));
	}

	float GetUnitPerception()
	{
		return m_fAiPerception;
	}

	float SetUnitPerception(float sk)
	{
		m_fAiPerception = sk;
		return sk;
	}

	bool GetUnitMagicMagazine()
	{
		return m_bIsMagicallyResupplied;
	}

	bool SetUnitMagicMagazine(bool s)
	{
		m_bIsMagicallyResupplied = s;
		return m_bIsMagicallyResupplied;
	}

	float GetSuppressionEffect()
	{
		return m_fSuppressionEffect;
	}

	float SetSuppressionEffect(float f)
	{
		m_fSuppressionEffect = f;
		return m_fSuppressionEffect;
	}

	float GetTakeCoverChance()
	{
		return m_fTakeCoverChance;
	}

	float SetTakeCoverChance(float f)
	{
		m_fTakeCoverChance = f;
		return m_fTakeCoverChance;
	}

	float GetGrenadeUsage()
	{
		return m_fGrenadeUsage;
	}

	float SetGrenadeUsage(float f)
	{
		m_fGrenadeUsage = Math.Clamp(f, 0.0, 1.0);
		return m_fGrenadeUsage;
	}

	float GetGLUsage()
	{
		return m_fGLUsage;
	}

	float SetGLUsage(float f)
	{
		m_fGLUsage = Math.Clamp(f, 0.0, 1.0);
		return m_fGLUsage;
	}

	float GetGLAccuracy()
	{
		return m_fGLAccuracy;
	}

	float SetGLAccuracy(float f)
	{
		m_fGLAccuracy = Math.Clamp(f, 0.1, 5.0);
		return m_fGLAccuracy;
	}

	float GetSmokeUsage()
	{
		return m_fSmokeUsage;
	}

	float SetSmokeUsage(float f)
	{
		m_fSmokeUsage = Math.Clamp(f, 0.0, 1.0);
		return m_fSmokeUsage;
	}

	float GetIdleEnterTime()					{ return m_fIdleEnterTime; }
	void SetIdleEnterTime(float f)				{ m_fIdleEnterTime = Math.Clamp(f, 5.0, 300.0); }
	float GetIdleCoverSearchDist()				{ return m_fIdleCoverSearchDist; }
	void SetIdleCoverSearchDist(float f)		{ m_fIdleCoverSearchDist = Math.Clamp(f, 10.0, 200.0); }
	float GetIdleOverrunDist()					{ return m_fIdleOverrunDist; }
	void SetIdleOverrunDist(float f)			{ m_fIdleOverrunDist = Math.Clamp(f, 0.0, 100.0); }
	bool GetIdleLeaveToAssist()					{ return m_bIdleLeaveToAssist; }
	void SetIdleLeaveToAssist(bool b)			{ m_bIdleLeaveToAssist = b; }
	bool GetIdleLeaveToInvestigate()			{ return m_bIdleLeaveToInvestigate; }
	void SetIdleLeaveToInvestigate(bool b)		{ m_bIdleLeaveToInvestigate = b; }
	bool GetIdleFollowCommander()				{ return m_bIdleFollowCommander; }
	void SetIdleFollowCommander(bool b)			{ m_bIdleFollowCommander = b; }
	bool GetStragglerEnabled()					{ return m_bStragglerEnabled; }
	float GetStragglerTime()					{ return m_fStragglerTime; }
	float GetStragglerStuckTime()				{ return m_fStragglerStuckTime; }
	bool GetStragglerLog()						{ return m_bStragglerLog; }
	float GetCombatCohesionOpen()				{ return m_fCombatCohesionOpen; }
	void SetCombatCohesionOpen(float f)			{ m_fCombatCohesionOpen = Math.Clamp(f, 10.0, 100.0); }
	float GetCombatCohesionUrban()				{ return m_fCombatCohesionUrban; }
	void SetCombatCohesionUrban(float f)		{ m_fCombatCohesionUrban = Math.Clamp(f, 5.0, 50.0); }
	float GetArmorFearScale()					{ return m_fArmorFearScale; }
	void SetArmorFearScale(float f)			{ m_fArmorFearScale = Math.Clamp(f, -1.0, 2.0); }

	int GetOverlayMode()						{ return m_iOverlayMode; }
	void SetOverlayMode(float f)				{ m_iOverlayMode = Math.Clamp(Math.Round(f), 0, 2); }
	float GetOverlayRadius()					{ return m_fOverlayRadius; }
	void SetOverlayRadius(float f)				{ m_fOverlayRadius = Math.Clamp(f, 200.0, 5000.0); }
	float GetRadioRadius()						{ return m_fRadioRadius; }
	void SetRadioRadius(float f)				{ m_fRadioRadius = Math.Clamp(f, 100.0, 3000.0); }
	float GetArtyWarnRadius()					{ return m_fArtyWarnRadius; }
	void SetArtyWarnRadius(float f)				{ m_fArtyWarnRadius = Math.Clamp(f, 100.0, 1500.0); }
	bool GetPlayerBriefings()					{ return m_bPlayerBriefings; }
	void SetPlayerBriefings(bool b)				{ m_bPlayerBriefings = b; PushShareToPlayers(); }
	int GetPlayerBriefingCap()					{ return m_iPlayerBriefingCap; }
	void SetPlayerBriefingCap(float f)			{ m_iPlayerBriefingCap = Math.Clamp(Math.Round(f), 0, 2); PushShareToPlayers(); }
	int GetBriefingWorldMarkers()				{ return m_iBriefingWorldMarkers; }
	void SetBriefingWorldMarkers(float f)		{ m_iBriefingWorldMarkers = Math.Clamp(Math.Round(f), 0, 2); PushShareToPlayers(); }

	float GetSuperiorRatio()					{ return m_fSuperiorRatio; }
	void SetSuperiorRatio(float f)				{ m_fSuperiorRatio = Math.Clamp(f, 1.0, 5.0); }
	bool GetLongRangeHold()						{ return m_bLongRangeHold; }
	void SetLongRangeHold(bool b)				{ m_bLongRangeHold = b; }
	float GetRifleEffectiveRange()				{ return m_fRifleEffectiveRange; }
	void SetRifleEffectiveRange(float f)		{ m_fRifleEffectiveRange = Math.Clamp(f, 100.0, 800.0); }
	float GetMGEffectiveRange()					{ return m_fMGEffectiveRange; }
	void SetMGEffectiveRange(float f)			{ m_fMGEffectiveRange = Math.Clamp(f, 200.0, 1500.0); }
	bool GetShareBuildings()					{ return m_bShareBuildings; }
	void SetShareBuildings(bool b)				{ m_bShareBuildings = b; }
	int GetCoverPreference()					{ return m_iCoverPreference; }
	void SetCoverPreference(float f)			{ m_iCoverPreference = Math.Clamp(Math.Round(f), 0, 2); }
	bool GetOvermatchAssault()					{ return m_bOvermatchAssault; }
	void SetOvermatchAssault(bool b)			{ m_bOvermatchAssault = b; }
	float GetOvermatchMaxDist()					{ return m_fOvermatchMaxDist; }
	void SetOvermatchMaxDist(float f)			{ m_fOvermatchMaxDist = Math.Clamp(f, 100.0, 500.0); }
	bool GetNightEnabled()						{ return m_bNightEnabled; }
	void SetNightEnabled(bool b)				{ m_bNightEnabled = b; }
	int GetCommanderStockDefault()				{ return m_iCommanderStockDefault; }
	int GetMaxAttackGroupsDefault()			{ return m_iMaxAttackGroupsDefault; }
	void SetCommanderStockDefault(int i)		{ m_iCommanderStockDefault = Math.Clamp(i, -1, 100000); }
	int GetSpawnerFactionAICap()				{ return m_iSpawnerFactionAICap; }
	void SetSpawnerFactionAICap(int i)			{ m_iSpawnerFactionAICap = Math.Clamp(i, 0, 2000); }
	int GetContactReportMaxPerScan()			{ return m_iContactReportMaxPerScan; }
	void SetContactReportMaxPerScan(int i)		{ m_iContactReportMaxPerScan = Math.Clamp(i, 0, 10); }
	float GetContactReportScanInterval()		{ return m_fContactReportScanInterval; }
	void SetContactReportScanInterval(float f)	{ m_fContactReportScanInterval = Math.Clamp(f, 0.0, 60.0); }
	float GetNightFlareCooldown()				{ return m_fNightFlareCooldown; }
	void SetNightFlareCooldown(float f)			{ m_fNightFlareCooldown = Math.Clamp(f, 30.0, 300.0); }
	float GetNightEngageMul()					{ return m_fNightEngageMul; }
	void SetNightEngageMul(float f)				{ m_fNightEngageMul = Math.Clamp(f, 0.2, 1.0); }
	bool GetNightLightDiscipline()				{ return m_bNightLightDiscipline; }
	void SetNightLightDiscipline(bool b)		{ m_bNightLightDiscipline = b; }
	bool GetCQBEnabled()						{ return m_bCQBEnabled; }
	void SetCQBEnabled(bool b)					{ m_bCQBEnabled = b; }
	int GetCQBIsolateMin()						{ return m_iCQBIsolateMin; }
	void SetCQBIsolateMin(float f)				{ m_iCQBIsolateMin = Math.Clamp(Math.Round(f), 3, 12); }
	float GetCQBStackWait()						{ return m_fCQBStackWait; }
	void SetCQBStackWait(float f)				{ m_fCQBStackWait = Math.Clamp(f, 0.0, 30.0); }
	int GetCQBAbortLosses()						{ return m_iCQBAbortLosses; }
	void SetCQBAbortLosses(float f)				{ m_iCQBAbortLosses = Math.Clamp(Math.Round(f), 1, 6); }
	bool GetCQBCaptureWaits()					{ return m_bCQBCaptureWaits; }
	void SetCQBCaptureWaits(bool b)				{ m_bCQBCaptureWaits = b; }
	float GetCQBContactTimeout()				{ return m_fCQBContactTimeout; }
	void SetCQBContactTimeout(float f)			{ m_fCQBContactTimeout = Math.Clamp(f, 30.0, 600.0); }
	int GetCQBEngine()							{ return m_iCQBEngine; }
	void SetCQBEngine(float f)					{ m_iCQBEngine = Math.Clamp(Math.Round(f), 0, 1); }
	int GetCQBStyle()							{ return m_iCQBStyle; }
	void SetCQBStyle(float f)					{ m_iCQBStyle = Math.Clamp(Math.Round(f), 0, 2); }
	int GetCQBBreachMode()						{ return m_iCQBBreachMode; }
	void SetCQBBreachMode(float f)				{ m_iCQBBreachMode = Math.Clamp(Math.Round(f), 0, 3); }
	int GetCQBEntrySize()						{ return m_iCQBEntrySize; }
	void SetCQBEntrySize(float f)				{ m_iCQBEntrySize = Math.Clamp(Math.Round(f), 2, 4); }
	float GetCQBRoomTimeout()					{ return m_fCQBRoomTimeout; }
	void SetCQBRoomTimeout(float f)				{ m_fCQBRoomTimeout = Math.Clamp(f, 20.0, 120.0); }
	float GetCQBFloorTimeout()					{ return m_fCQBFloorTimeout; }
	void SetCQBFloorTimeout(float f)			{ m_fCQBFloorTimeout = Math.Clamp(f, 30.0, 180.0); }
	float GetCQBAbortPct()						{ return m_fCQBAbortPct; }
	void SetCQBAbortPct(float f)				{ m_fCQBAbortPct = Math.Clamp(f, 25.0, 100.0); }

	bool GetShareEnabled()						{ return m_bShareEnabled; }
	void SetShareEnabled(bool b)				{ m_bShareEnabled = b; }
	float GetShareVoiceRange()					{ return m_fShareVoiceRange; }
	void SetShareVoiceRange(float f)			{ m_fShareVoiceRange = Math.Clamp(f, 20.0, 300.0); }
	float GetShareRadioRange()					{ return m_fShareRadioRange; }
	void SetShareRadioRange(float f)			{ m_fShareRadioRange = Math.Clamp(f, 100.0, 1500.0); }
	float GetShareNoisePer100m()				{ return m_fShareNoisePer100m; }
	void SetShareNoisePer100m(float f)			{ m_fShareNoisePer100m = Math.Clamp(f, 0.0, 50.0); }
	float GetShareVoiceNoise()					{ return m_fShareVoiceNoise; }
	void SetShareVoiceNoise(float f)			{ m_fShareVoiceNoise = Math.Clamp(f, 0.5, 5.0); }
	float GetShareRadioNoise()					{ return m_fShareRadioNoise; }
	void SetShareRadioNoise(float f)			{ m_fShareRadioNoise = Math.Clamp(f, 0.5, 5.0); }
	bool GetShareRequireRadio()					{ return m_bShareRequireRadio; }
	void SetShareRequireRadio(bool b)			{ m_bShareRequireRadio = b; }
	int GetShareHops()							{ return m_iShareHops; }
	void SetShareHops(float f)					{ m_iShareHops = Math.Clamp(Math.Round(f), 1, 2); }
	bool GetShareMapMarkers()					{ return m_bShareMapMarkers; }
	void SetShareMapMarkers(bool b)				{ m_bShareMapMarkers = b; }
	int GetAIMapMarkers()						{ return m_iAIMapMarkers; }
	void SetAIMapMarkers(int mode)				{ m_iAIMapMarkers = Math.ClampInt(mode, 0, 2); }
	bool GetShareToPlayers()					{ return m_bShareToPlayers; }
	void SetShareToPlayers(bool b)				{ m_bShareToPlayers = b; PushShareToPlayers(); }
	bool GetShareDebug()						{ return m_bShareDebug && DCO_TestCaseComponent.RunOn(); }
	void SetShareDebug(bool b)					{ m_bShareDebug = b; }

	bool GetMedicCrossGroup()					{ return m_bMedicCrossGroup; }
	void SetMedicCrossGroup(bool b)				{ m_bMedicCrossGroup = b; }
	float GetMedicRadiusBleeding()				{ return m_fMedicRadiusBleeding; }
	void SetMedicRadiusBleeding(float f)		{ m_fMedicRadiusBleeding = Math.Clamp(f, 25.0, 500.0); }
	float GetMedicRadiusUnconscious()			{ return m_fMedicRadiusUnconscious; }
	void SetMedicRadiusUnconscious(float f)		{ m_fMedicRadiusUnconscious = Math.Clamp(f, 25.0, 800.0); }
	bool GetMedicIncludePlayers()				{ return m_bMedicIncludePlayers; }
	void SetMedicIncludePlayers(bool b)			{ m_bMedicIncludePlayers = b; }
	bool GetMedicUnderFire()					{ return m_bMedicUnderFire; }
	void SetMedicUnderFire(bool b)				{ m_bMedicUnderFire = b; }
	int GetMedicMaxLentPerGroup()				{ return m_iMedicMaxLentPerGroup; }
	void SetMedicMaxLentPerGroup(float f)		{ m_iMedicMaxLentPerGroup = Math.Clamp(Math.Round(f), 1, 4); }
	float GetMedicTimeout()						{ return m_fMedicTimeout; }
	void SetMedicTimeout(float f)				{ m_fMedicTimeout = Math.Clamp(f, 20.0, 300.0); }

	float GetInvestigateMaxDist()				{ return m_fInvestigateMaxDist; }
	void SetInvestigateMaxDist(float f)			{ m_fInvestigateMaxDist = Math.Clamp(f, 0.0, 1500.0); }
	float GetInvestigateChance()				{ return m_fInvestigateChance; }
	void SetInvestigateChance(float f)			{ m_fInvestigateChance = Math.Clamp(f, 0.0, 1.0); }

	DCO_EAILODMode GetLODMode()
	{
		return m_eLODMode;
	}

	void SetLODMode(DCO_EAILODMode mode)
	{
		mode = Math.ClampInt(mode, DCO_EAILODMode.VANILLA, DCO_EAILODMode.FULL_DETAIL);
		if (mode == m_eLODMode || !Replication.IsServer())
			return;

		m_eLODMode = mode;
		ReapplyCommanderLOD("mode_change");
	}

	static DCO_EAILODMode GlobalLODMode()
	{
		DCO_GlobalAIComponent global = GetInstance();
		if (!global)
			return DCO_EAILODMode.VANILLA;
		return global.m_eLODMode;
	}

	static void ReapplyCommanderLOD(string reason)
	{
		if (!Replication.IsServer())
			return;

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
		{
			foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
			{
				if (!cmd)
					continue;
				foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
				{
					if (g)
						g.DCO_ApplyLOD();
				}
			}
		}
		LogLODSummary(reason);
	}

	static void ApplyLOD(notnull AIAgent agent, DCO_EAILODMode mode)
	{
		SCR_AIUtilityComponent util = SCR_AIUtilityComponent.Cast(agent.FindComponent(SCR_AIUtilityComponent));
		int prev = 0;
		if (util)
			prev = util.m_iDCO_LOD;
		if (util && prev == mode)
			return;

		if (prev != DCO_EAILODMode.VANILLA)
		{
			agent.SetPermanentLOD(-1);
			agent.PreventMaxLOD(0);
		}

		if (mode == DCO_EAILODMode.PREVENT_MAX_LOD)
			agent.PreventMaxLOD();
		else if (mode == DCO_EAILODMode.FULL_DETAIL)
			agent.SetPermanentLOD(0);

		if (util)
			util.m_iDCO_LOD = mode;
	}

	static void LogLODSummary(string reason)
	{
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return;

		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);
		int countVanilla = 0;
		int countPrevent = 0;
		int countFull = 0;
		foreach (AIAgent agent : agents)
		{
			SCR_AIUtilityComponent util = SCR_AIUtilityComponent.Cast(agent.FindComponent(SCR_AIUtilityComponent));
			if (!util)
				continue;
			if (util.m_iDCO_LOD == DCO_EAILODMode.PREVENT_MAX_LOD)
				countPrevent++;
			else if (util.m_iDCO_LOD == DCO_EAILODMode.FULL_DETAIL)
				countFull++;
			else
				countVanilla++;
		}

		string line = string.Format("lod_summary reason=%1 global=%2 vanilla=%3 prevent=%4 full=%5", reason, typename.EnumToString(DCO_EAILODMode, GlobalLODMode()), countVanilla, countPrevent, countFull);
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", line);
	}

	DCO_EAIPersonality RollWeightedPersonality()
	{
		float total = m_fPersonalityWeightStandard + m_fPersonalityWeightCautious
			+ m_fPersonalityWeightAggressive + m_fPersonalityWeightReckless;

		if (total <= 0.0)
			return DCO_EAIPersonality.STANDARD;

		float roll = Math.RandomFloat(0.0, total);

		if (roll < m_fPersonalityWeightStandard)
			return DCO_EAIPersonality.STANDARD;
		roll -= m_fPersonalityWeightStandard;

		if (roll < m_fPersonalityWeightCautious)
			return DCO_EAIPersonality.CAUTIOUS;
		roll -= m_fPersonalityWeightCautious;

		if (roll < m_fPersonalityWeightAggressive)
			return DCO_EAIPersonality.AGGRESSIVE;

		return DCO_EAIPersonality.RECKLESS;
	}
}