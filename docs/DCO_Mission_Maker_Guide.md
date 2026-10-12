# DCO AI — Mission Maker Guide

How to build a mission with DCO AI in the Workbench World Editor and in Game Master: AI commanders, objectives, groups, waypoints, spawners, logistics, artillery, players, settings, debugging and the mistakes that cost the most time.

Written 2026-10-10 from the DCO_AI source (every fact below was checked in the code). Sections marked **Coming** describe features that are being built right now; they will be filled in when those features land.

- Server config reference (every JSON key): [DCO_Server_Config.md](DCO_Server_Config.md)
- UI text in DCO is English only; this guide uses the names you see in the editor and in Game Master.

---

## Contents

1. [How DCO thinks (the 2-minute version)](#1-how-dco-thinks-the-2-minute-version)
2. [What every scene needs](#2-what-every-scene-needs)
3. [AI Commander](#3-ai-commander)
4. [Objectives](#4-objectives)
5. [Getting groups under a commander](#5-getting-groups-under-a-commander)
6. [DCO waypoints: Garrison and Do CQB](#6-dco-waypoints-garrison-and-do-cqb)
7. [Commander Spawner (reinforcements)](#7-commander-spawner-reinforcements)
8. [Logistics hub and transport](#8-logistics-hub-and-transport)
9. [Artillery and mortars](#9-artillery-and-mortars)
10. [Vehicles: motorized and mechanized groups](#10-vehicles-motorized-and-mechanized-groups)
11. [Players in a DCO mission](#11-players-in-a-dco-mission)
12. [Settings, presets and the server config](#12-settings-presets-and-the-server-config)
13. [Game Master workflow](#13-game-master-workflow)
14. [Debugging: overlays, logs and tools](#14-debugging-overlays-logs-and-tools)
15. [Performance and mission size](#15-performance-and-mission-size)
16. [Common mistakes](#16-common-mistakes)
17. [Server load failure: "Too many instructions per function"](#17-server-load-failure-too-many-instructions-per-function)
18. [Example missions](#18-example-missions)
19. [Victory conditions and script events](#19-victory-conditions-and-script-events)
20. [Being tested (off by default)](#20-being-tested-off-by-default)
21. [Checklist before you publish](#21-checklist-before-you-publish)

---

## 1. How DCO thinks (the 2-minute version)

- An **AI Commander** owns a faction's AI groups. Every think cycle it looks at the **objectives** on the map, estimates the enemy, compares strength (a force ratio), and decides what to attack, what to defend and which groups to send.
- **Objectives** are areas with an owner. Groups capture them by standing in them; the owner changes when one side clearly controls the area long enough.
- **Groups** do the fighting with DCO's squad and soldier behaviour (bounding, suppression, cover, CQB, medics, specialists). They only take commander orders once they are **assigned to a commander**.
- **Spawners** bring reinforcements to a commander. **Logistics hubs** are where transport, MEDEVAC and resupply happen. **Mortar teams** give the commander artillery.
- **Players** are never ordered around like AI groups. A player group can opt in to receive task offers from its commander.
- Almost everything can be changed in **Game Master** (DCO Settings window) and the important global values also in the **server config file**.

---

## 2. What every scene needs

| Need | What to place / check | Why |
|---|---|---|
| Game mode | Any game mode built on `Prefabs/MP/Modes/GameMode_Base.et` (DCO overrides it and adds `DCO_GlobalAIComponent`). The vanilla `GameMode_Editor_Full.et` works. | Global DCO settings live on this component. |
| AI World + navmesh | `Prefabs/AI/SCR_AIWorld.et` with the navmesh set up as in vanilla (DCO overrides this prefab and adds formations + the cover manager). | No navmesh = no movement, no indoor CQB plans. |
| Faction manager | `SCR_FactionManager` with the factions you use. | Without it the commander faction list is empty (log: `FactionManager not present yet -- faction list is empty`). |
| Commander manager | `Prefabs/Commander/CommanderManager.et` (holds `AICommander_ManagerComponent`). | Commanders and objectives register with it. The GM versions of commander/objective spawn it automatically, the base prefabs do **not** — place it yourself in Workbench missions. |
| Perception manager, map entity | As in any Reforger mission. | Spotting and the map overlay. |

The per-faction settings entity (`Prefabs/DCO/DCO_FactionSettings.et`) is spawned by code; you do not place it.

---

## 3. AI Commander

### Prefabs
- Workbench: `Prefabs/Commander/US_Commander.et` (based on `Prefabs/Commander/Commander.et`).
- Game Master: `PrefabsEditable/DCO/Commander/E_AICommander.et` ("AI Commander").
- Components: `AICommander_AlgorithmComponent` (the commander), `CMD_ThreatResponseComponent` (reacting to threats), `CMD_ArtillerySupport` (artillery), plus replication/hierarchy.

### Set these on every commander
| Attribute | What to set | Default / note |
|---|---|---|
| **Faction Key** | The faction this commander leads, e.g. `US`. | Empty by default. Leave it empty and groups of other factions are rejected; the start-up check logs `setup_check cmd=<UID> faction=EMPTY`. |
| **Commander UID** | A short unique name, e.g. `Viking`. | Empty, duplicate, or containing `;` or `\|` → renamed to `<FACTION>-N` with a warning. Other entities (spawners, groups, objectives) refer to commanders by this UID. |
| **Commander Mode** | Offensive, Defensive or Balanced. | Balanced. The mode does not change by itself during the mission (only through GM or a preset). |
| Personality preset | Measured, Careful, Bold or Scout. | Measured. |
| Doctrine | Empty = the faction doctrine (US 1989, USSR 1989, FIA), or Default (Measured), Defensive, Aggressive, Recon-heavy, or a JSON preset in `$profile:DCO/Doctrine/`. | Faction doctrine. |
| Max Groups per Attack | How many groups go at one objective. | 6 (help recommends 3–6). Server JSON `maxAttackGroups` ≥ 1 overrides it. |
| Objectives at the same time | How many objectives it works on at once. | 3 in script, 4 in the prefab. |
| Allowed Tactics / Allowed Defense Tactics | Untick tactics you do not want. | All allowed. |
| Reserve % / All-in | Share of groups kept back. | 20 % / off. |
| Start Preset | Name of a commander preset to load at start (see §12). | Empty. |

**Think interval:** `US_Commander.et` sets 150 s between decisions (the script default is 30 s). Lower it for small, fast missions; keep it high for big ones. The first think happens after "Delay First Iteration" (60 s).

### How it decides
- **Offensive:** only attacks. **Defensive:** only defends. **Balanced:** first assigns defenders (idle groups × defend share, at least one unless All-in), then attacks with the rest.
- Attack targets are ranked by proximity, base value and threat. Objectives the commander already owns, objectives on cooldown and objectives that block this commander are skipped.
- An attack needs a force ratio of about **2 : 1** (aggressive commander) up to **3 : 1** (cautious commander). An aborted attack puts that objective on a 300 s cooldown.

---

## 4. Objectives

### Prefabs
- Workbench: `Prefabs/Commander/Commander_Objective.et` (prefab: base value 70, radius 145).
- Game Master: `PrefabsEditable/DCO/Commander/E_AICommander_Objective.et`.

### Attributes
| Attribute | Meaning | Default |
|---|---|---|
| Name | Shown in logs, overlays and briefings. Give every objective a unique, readable name. | — |
| Radius | Capture area (m). | 30 (prefab 145) |
| Base Value | How much a commander wants it (0–100). | 50 (prefab 70) |
| Type | Capture, Destroy, Recon, Defend or Hold until (GM: "DCO Objective" tab). See *Objective types* below. | Capture |
| Strategic Value | High (×1.67), Moderate, Low (×0.5), No. | Moderate |
| Initial Owner Commander UID (GM: Controlled By) | Who holds it at start (100 % control). Empty = neutral. | Empty |
| Retain | The owner never gives it up for doctrine reasons. | Off |
| Operation Primary + Secondary Radius / Linked Secondaries | Makes this the main objective of an operation; secondaries are the objectives within the radius, or the ones you tick in **Linked Secondaries** (a checkbox list of the objectives in objective-list order, each with its distance when one objective is open; used only when Operation Primary is on). | Off / 600 m |
| Defend Groups / Max Attack Groups | Groups a commander puts on it. | 2 / 4 |
| Allow All Factions / Allowed Factions | Who may capture it. | All |
| Category | Town, Military installation, Supply depot, Terrain. **Not used by the AI yet.** | — |

> **Old field:** "Set Captured by Commander" is only read when Initial Owner Commander UID is empty (its first entry becomes the owner; log `obj_owner_legacy`). Use **Initial Owner Commander UID** for new missions.

### Which commanders handle it, and focus
In the GM objective dialog, tab **"DCO Objective"** (Workbench: category "Objective"):

| Row | Meaning | Default |
|---|---|---|
| **Handled By** | All Commanders / Only Checked / All Except Checked. Every commander path (attack, defense, patrol, recon, reinforcement, operation secondaries) checks this one rule. Commander Spawners do not use it. | All Commanders |
| **Commanders** | Checkbox list "UID · FACTION". Only Checked: these plan it. All Except Checked: these ignore it. | — |
| **Focus For** | These commanders attack this objective first at their next decision. A commander has one focus at a time; ticking it here moves its focus from its old objective. | — |
| **Force Focus** | Attack even when the force ratio says the enemy is too strong. | Off |
| Attack Only / Defend Only Commanders | Limit a commander to attacking or defending this objective (needs the tune switch `obj.role_on`). | — |

- From the commander side (tab **"DCO Commander: Style & Attack"**): **Focus Objective** (None or one objective), **Force Focus**, **Blocked Objectives**. Ticking a blocked objective rewrites that objective's Handled By / Commanders, so both sides always agree.
- A focus clears itself when the objective is captured, removed, no longer allowed or role-blocked (log `obj_focus_done`).
- Lists show at most **24** commanders / objectives (alphabetical); entries beyond that are kept but not shown.
- Old missions: a filled "Blacklisted Co" list is read as All Except Checked with those commanders.
- Workbench fields: objective `m_iDCOAssignMode` (0/1/2), `m_aDCOAssignList`, `m_aDCOAttackOnly`, `m_aDCODefendOnly`; commander `m_sDCOFocusObjective`, `m_bDCOFocusForce` (category "Commander 04. Attack").
- GMs get a notice when an objective is left without a commander ("Nobody handles <objective>. It is ignored until a commander is checked."), when a focus moves ("<commander>'s focus moves from <A> to <B>.") and when a focus is refused ("<commander> cannot focus <objective>: it does not handle this objective.").
- A deleted commander is removed from every objective list (log `obj_assign_drop`); in Only Checked mode an objective whose listed commanders are all gone counts as handled by nobody.
- Logs: `obj_assign_set`, `obj_assign_empty` (nobody handles it), `obj_assign_drop`, `obj_focus_set`, `obj_focus_reject`, `obj_focus_done`.

### Objective types
| Type | Done when | Rows that matter |
|---|---|---|
| Capture | A side takes and holds the area (default). | — |
| Recon | The commander watches it and reports what is there. | — |
| Defend | The defending side still owns it **Type Time** seconds after mission start. Fails early when the defenders stay below **Defend Fail Ratio** of the attackers for 60 s. **Defend Retain** keeps it on Retain while the defense runs. | Type Time, Type Faction (Auto = owner at start), Defend Fail Ratio, Defend Retain |
| Hold until | A side owns it for **Type Time** seconds without a break. Fails when **Type Deadline** passes first. | Type Time, Type Faction (Auto = any side), Type Deadline |
| Destroy | Every entity listed in **Destroy Targets** is destroyed or deleted. Fails when **Type Deadline** passes first. With no targets listed it works like a Destroy without tracking. | Destroy Targets (**Workbench only**: entity names, field `m_aDCODestroyTargets`), Type Faction (Auto = any side that did not own it at start), Type Deadline |

- Logs: `obj_type_done obj=<name> type=<type> why=…` and `obj_type_fail …`. These also feed the **Typed Objectives** victory rule (§19).

### Objective phases (needs `tune_obj_phase_on = 1`)
Phases make an operation unfold in order ("take the bridge, then the town"). They only work when the server setting `tune_obj_phase_on` is 1; it is 0 by default while it is being tested (Beta).

| Row (GM "DCO Objective" tab) | Meaning | Default |
|---|---|---|
| **Phase** | 0 = always active. Phase N is attacked by a side only after that side holds every objective of phases 1 to N-1. Objectives a side owns are always defended. | 0 |
| **Active After** | Checkbox list of objectives that must meet the Active Condition first. Recon may still watch this objective before then. | — |
| **Active Condition** | When Active After counts as done: **Captured by the attacking side** / **Captured by anyone** / **Lost by its first owner**. | Captured by the attacking side |
| **Active Delay** | Reorganisation pause (s) after the conditions are met, before the attack may start. | 120 |

- Logs: `obj_phase_active obj=<name> faction=<f> phase=<n> waited_s=<s>` when an objective opens for a side; `obj_phase_missing` / `obj_phase_dep_gone` when an Active After entry names an objective that does not exist or was deleted.

### How capture works
- Control rises at `100 / Capture Hold Duration` % per second (60 s), scaled by how many units are inside (full speed at 8) and by how clearly one side outnumbers the other.
- A side becomes owner at 100 % control and loses it below 50 %. An empty area drifts back toward the last owner at 1 % per second.
- An objective only advances while some commander is working on it (within the last 60 s). An objective nobody plans for does not change hands.

---

## 5. Getting groups under a commander

Every group built on `Prefabs/AI/Groups/Group_Base.et` already carries the DCO group components (DCO overrides the vanilla prefab). What it does **not** do is join a commander by itself.

Ways to put a group under a commander:
1. **Workbench:** on the group, tick **Auto Assign** and optionally set **Dedicated Co** (the commander UID; category "Commander"). Without a dedicated UID the group joins a random commander of its faction. It retries 12 × 5 s; then the log says `Auto-assign … failed: no commander … for faction …`.
2. **Game Master:** group attribute **Commander** (dropdown of commanders of that faction).
3. **Commander Spawner:** spawned groups are assigned at once (§7).

Which jobs a squad may get (tab **"DCO Squad: Commander Permissions"**; Workbench `m_iDCOAllowedJobs`, `m_bDCOFirstToFight` on the group utility component):

| Row | Meaning | Default |
|---|---|---|
| **Task Preset** | Any Task / Attack Only / Recon Only / Defend Only / Transport Only / Reserve Only / Custom. Picking a preset ticks its boxes at once; ticking boxes by hand shows "Custom · N changed from <preset>". | Any Task |
| **Allowed Tasks** | Attack, Flank, Support by Fire, Recon, Defend, Garrison, Patrol, Reserve, Transport. Unchecked jobs are never assigned. | All |
| **First to Fight** | The commander picks this squad first when it builds an attack. | Off |

An empty Allowed Tasks list warns the GM ("… gets no commander jobs."). Transport Only on a squad without a vehicle is treated as Any Task, with a warning.

Other useful group attributes (same tab): **Commander Can Task** (off = the commander leaves this squad alone; Task Preset, Allowed Tasks and First to Fight are then greyed out), Allow Reinforce, Allow Call Reinforcement, Allow Call Artillery, Can Ride Commander Transport, Send Contact Reports, Allow Player Support, Dedicated Transport, Transport: Allowed Jobs.

**Player groups are never tasked as AI.** See §11.

---

## 6. DCO waypoints: Garrison and Do CQB

Both are in the GM Command mode list and open their settings dialog when placed (the waypoint waits up to 60 s while you set it up).

### Garrison (DCO)
Prefab `Prefabs/AI/Waypoints/AIWaypoint_Garrison.et` (GM: "Garrison (DCO)"). The group occupies buildings near the waypoint and fights from windows; with no building it takes field fighting positions, and as a last resort it defends like the vanilla Defend waypoint.

| Attribute | Meaning | Default |
|---|---|---|
| Garrison Mode | Hold (never leave), Defend (a small flank team may leave), Release (garrison until a trigger, then free). | Defend |
| Fill Ratio | Share of windows/positions to man. | 1 |
| Max Buildings | 0 = automatic from the group size. | 0 |
| Fill Order | Building first or windows first. | Building first |
| Rotate When Idle / Suppressed | Swap positions every 30 s, or after 8 s under fire. | On |
| Allow Roof | Use roofs. | On |
| Field Positions | Dig in outside when there is no building. | On |
| Release Distance / Casualties / Suppressed Time | Release triggers (Release mode). | 30 m / 50 % / 20 s |

Buildings are searched within the completion radius (min 30 m; prefab 40 m).

### Do CQB (DCO)
Prefab `Prefabs/AI/Waypoints/AIWaypoint_DoCQB.et` (GM: "Do CQB (DCO)"). The group clears the building nearest to the waypoint, room by room.

| Attribute | Meaning | Default |
|---|---|---|
| CQB Target Radius | How far from the waypoint to look for the building. | 25 m |
| CQB Attempts | Tries before giving up (15 s between tries). | 2 |
| CQB Hold After Clear | Stay inside afterwards (the waypoint then never completes). | Off |

- You do **not** place an indoor navigation graph; DCO builds it from the soldier navmesh at runtime.
- The plan fails when the building has no rooms on the navmesh, no outside door, or the group has fewer than 2 men; a building already being cleared by another friendly group is skipped.

---

## 7. Commander Spawner (reinforcements)

Prefab `Prefabs/Commander/CommanderSpawner.et`; GM: "AI Commander Spawner". It creates groups near itself and hands them to a commander.

### Who gets the groups
- **Target Commander:** Weakest of faction (default), Nearest of faction, or a specific Commander UID. **Faction:** empty = all.

### Which groups (force pool)
- Each entry: group prefab (or a **group preset by name**), optional vehicle, weight, budget (−1 = unlimited), role (Auto, Assault, Fire team, AT, Recon, Motorized), Experience, Morale, Shape, Arrival time, Cost.
- **Empty pool** = random groups from the commander's faction catalog.
- **Commander Chooses** (on): the commander picks AT against armour, assault for objectives, recon for unclear contacts, motorized for far objectives. Off = weighted random.
- **Vehicles:** Crewed (vehicle comes with its own crew) or Group mounts (the group drives).
- Groups Per Spawn 1, Spawn Radius 30 m.

### When it spawns
- Spawn At Start (10 s after start), Trigger Objective + event (lost / captured), Manpower Below N, **Commander Request** (the commander asks when worn down, too weak for its mission, or missing AT/recon). Cooldown 300 s.
- **Presets:** One-shot (everything once at start), Counterattack wave (objective lost, 2 groups, 10 min arrival), Steady top-up (manpower below 60 % of the peak count of soldiers in the commander's own squads; nothing happens before the first losses), Commander on call (commander asks), Custom.
- **Which preset for which side:** Counterattack wave is for a **defender**: it fires when an objective its side holds is lost, so it needs a trigger objective owned from the start. An attacker that owns nothing never triggers it; give attackers **Commander on call** (or Steady top-up).

### When it stops
- Overrun (more enemies than friends — at least 2 — inside the safe radius for the hold time), home objective lost (only when an enemy owns it; neutral does not count), no commander left, late in the mission (optional), out of men. "When the reason is gone" switches it back on.

### Limits
- **Safe Radius** 300 m: nothing spawns while a player or an enemy is inside (no pop-in in front of players).
- **Take From Stock:** none, men, or points from the commander's **Reinforcement Stock** (−1 = unlimited; server JSON `commanderStock`).
- **Spawner Faction AI Cap** (global, 0 = no cap; JSON `spawnerFactionAICap`): spawners wait while a faction has this many AI.

### GM actions
Context menu: **Spawn now**, **Spawn anyway** (only while waiting; skips the safe radius once, never the AI cap), **Turn spawner on/off**. Every state change is reported to GMs.

### Group presets
- Built-in presets ship with the mod (US Veteran rifle squad, AT hunter team, Recon team; USSR Motor rifle squad, Recon team; FIA Militia squad).
- Your own presets live in `$profile:DCO/GroupPresets/` on the server or your PC; build them in DCO Settings › Global › Group presets, or right-click an AI group › **Save as group preset**.

---

## 8. Logistics hub and transport

- Prefab `Prefabs/Commander/LogisticsHub.et`; GM: "AI Commander Logistics Hub". One attribute: **Hub Faction** (Auto = nearest commander).
- Transport teams rally there, casualties are brought there and return to the commander's manpower, extracted squads regroup there.
- **No hub placed:** the commander uses its own objective that is at least 500 m from any objective it does not own.
- **Transport teams** need nothing extra: a group of at most 3 men, all in one unarmed vehicle with a driver and 4+ free cargo seats, becomes a transport team automatically. Or set **Dedicated Transport** on the group and choose **Transport: Allowed Jobs** (Transport, Extract, MEDEVAC, Resupply).
- Commander tab "Logistics": Auto MEDEVAC, Auto Extract, Auto Resupply (needs a resupply station on the vehicle; off while Magic Magazine is on), Armed Escort — all on.
- Vehicle attribute **Transport Commander**: this commander may use the vehicle for transport.
- New Global › Commander settings (all on by default except the last): **Low Ammo: Compare To Own Loadout**, **Extraction: Only Combat-Ineffective Squads**, **Crosslevel: Needs Ammo Source At Hub**, **Threat: Big Groups Are Never Low Priority** (off, under test).

> **Checklist — a hub that actually works**
> 1. A **Logistics Hub** with its faction set. Squads withdraw to it and logistics jobs start there.
> 2. At least one **transport team** with its own vehicle: either the automatic kind above, or a group with **Dedicated Transport** ticked, its **Transport: Allowed Jobs** set, and a **Get In Nearest** waypoint on its vehicle so the crew boards. Without a team, every Resupply / MEDEVAC / Extract request waits 5 minutes and is cancelled (log `why=queue_timeout wait=no_team`).
> 3. For **Resupply**, the team's vehicle must carry a resupply station: an ammo truck (M923A1 ammo, Ural-4320 ammo). A normal transport truck cannot resupply.
> 4. An **ammo source at the hub** (arsenal box or ammo truck). A low-ammo squad within 200 m of the hub restocks there by itself (crosslevel). Without one, the commander sends a Resupply job instead (log `crosslevel_skip … reason=no_ammo_source`).
> Example D (§18) has all four for both sides.

---

## 9. Artillery and mortars

- The commander already has `CMD_ArtillerySupport` (range 80–3500 m, danger close 100 m).
- Place a **mortar** (or a mortar composition) and a **group of up to 4 soldiers** whose leader is within **40 m** of it, assigned to the commander. Within about a minute after assignment, while the group is untasked, it becomes a mortar team automatically. You can also force the group role to **Mortar**.
- Squads with **Allow Call Artillery** may request fire. Players request HE / smoke / illumination from the radial menu (§11).
- Without artillery, the commander still attacks; it just skips smoke screens and illumination before assaults.

---

## 10. Vehicles: motorized and mechanized groups

- A group counts as **motorized** (unarmoured vehicle), **mechanized** (armoured vehicle with at least 2 non-crew men) or **armor**, automatically, when its leader is in the group vehicle — or set the squad preset.
- Motorized: the crew drives to a laager about 250 m from the target (at a forest edge when possible) after dropping the squad. Mechanized: the crew stays in the vehicle and supports.
- Spawner entries can carry a vehicle (§7).

---

## 11. Players in a DCO mission

- **AI commanders never take over player groups.** A player group gets *offers* (task cards) from its faction's commander only if the group leader turns on **Commander Orders** in the Groups menu (default off). The radio then confirms with an "orders on net" message.
- Commander tab "Player Tasking": Task Player Groups (on), Count Player Groups in Plans (on, max 2 per objective), Time To Answer 120 s (no answer = AI takes the task, no penalty), Max Travel Time 8 min (beyond it the commander offers transport).
- **Briefings** (global "DCO Global: Players"): orders at operation start, FRAGO when the plan changes, phases, withdraw and abort. Player Briefing Max Level: 0 Hardcore (radio text only), 1 Standard (orders tab), 2 Assisted (orders card). Briefing World Markers: 0 = follow each player's level.
- **Map overlay** for players: friendly AI squads relevant to them (radius 1500 m). Radio messages to player groups within 800 m of an operation; artillery warnings within 400 m.
- **Player radial requests:** contact reports, suppress, support, flank left/right, armour, transport, extract, MEDEVAC, artillery (HE/smoke/illumination), roles (assault, fix, block, recon), cancel, show orders.

---

## 12. Settings, presets and the server config

### DCO Settings window
- Open with **F2**, the toolbar button **DCO Settings** (global), the context menu **DCO Settings** (selected entities), or **Open properties** (a chooser appears when an entity has both vanilla and DCO settings).
- Modes: **Simple** (a few rows with levels — what most players need), **Advanced**, **Expert** (everything, including debug switches).
- Targets: Global, Faction, Commander, Group, Unit. Faction values override Global for that faction; the window shows inherited values dimmed with a tag.
- A blue **Beta** tag next to a name marks a setting that is still being tested and may change; its help ends with "Beta: still being tested and may change." Type `beta` in the search box to list them. A folder header carries the tag only when every setting inside is Beta. The full list: [DCO_Experimental_Features.md](DCO_Experimental_Features.md).

### Presets
| Kind | Where | How |
|---|---|---|
| Settings presets | `$profile:DCO/Presets/<scope>/<name>.json` (server) | Presets panel in the window; Export/Import as text (`DCO1|…`) via the clipboard. |
| Commander presets | `$profile:DCO/CommanderPresets/<name>.json` | GM "Preset: Load / Save As"; Workbench "Start Preset" loads one before the first think (missing file = warning, prefab values). |
| Doctrine | Built-in in the mod; custom in `$profile:DCO/Doctrine/` | Faction doctrine in DCO Settings (Global target, faction spinner); per commander via Doctrine. |
| Group presets | Server and PC `$profile:DCO/GroupPresets/`, built-ins in the mod | §7. |

### Server config file
- `$profile:DCO/DCO_GlobalConfig.json`, read once at server start. Every key, default and range: [DCO_Server_Config.md](DCO_Server_Config.md).
- If the file is missing, DCO writes it with all keys. Missing keys are added later (the old file is kept as `.bak`).
- **Spell keys exactly.** A typo is not an error message; the key is simply unknown.
- **Per-faction values:** a `"factions"` block gives one faction its own values, e.g. `"factions": { "USSR": { "aiAggression": 1.5 } }`. Layers, last wins: file global → GM global → file faction block → GM faction. Server-wide keys (doctrine, group presets, gmSaveConfig, startupPresetPrompt, perfProfiling) are ignored inside a faction (`cfg_faction_global_only`). Details: [DCO_Server_Config.md › Per-faction values](DCO_Server_Config.md#per-faction-values).
- **Defaults follow DCO updates.** A key whose value still equals the old default takes the new default at the next start (log `cfg_default_migrated`, `.bak` kept). Values you changed are never touched. A file from before this rule is only checked once: up to 40 keys that differ from today's default are listed as `cfg_differs_from_default`. Fix those by hand, or delete the key to get the new default.
- **Feature switches** (`tune_…` keys with 0 / 1): new behaviours start as 0 and are switched on by default once they pass their test. The [config guide](https://claude.ai/artifact/FnWobXJ4MiHCAQZ4GhLNDC) lists every key with its current default; the full table is in [DCO_Server_Config.md](DCO_Server_Config.md).
- **GM changes are live but not saved** to the file. A GM with permission can use **Save to server config** (JSON key `gmSaveConfig`: gm / admin / off, default admin). Faction overrides are not saved.
- The main feel knob is `aiAggression` (default 1 = the DCO default, already bolder than vanilla Arma).

---

## 13. Game Master workflow

- Placeable DCO entities (Edit mode content browser): **AI Commander**, **AI Commander Objective**, **AI Commander Logistics Hub**, **AI Commander Spawner**. Waypoints (Command mode): **Garrison (DCO)**, **Do CQB (DCO)**.
- Quick GM mission:
  1. Place an **AI Commander** for each side; set Faction Key and a UID (F2 › Commander).
  2. Place **objectives**; set Name, Radius and **Controlled By** (the starting owner).
  3. Place groups; set each group's **Commander** (or use spawners).
  4. Optional: a **spawner** per side with a preset (e.g. Counterattack wave), a **logistics hub**, a mortar team.
  5. Open **DCO Settings › Global** for the feel (Simple mode: AI Aggression, Skill, Suppression…).
  6. Turn on the **intent overlay** to watch the plan (§14).

---

## 14. Debugging: overlays, logs and tools

### Commander intent overlay
- Global setting **Commander Intent Overlay** (DCO Global: System): Off / GM only. Always available in Workbench, never to normal players.
- **Ctrl+I** toggles it: on the map (map open) or in the GM world view. Click to select; Esc closes.
- Map layers: plan, groups, enemy, decisions, truth (truth shows real enemy positions — for testing only).
- Per commander: **Commander Debug** and **Commander Debug Layers** (Objectives, Focus, Plan geometry, AP candidates, Threats, Group lines, Assignment). Shapes are only sent while the overlay is open.
  - Objectives: one ring per objective (red = current main objective, blue = held, grey = other) with owner, defenders seen and confidence.
  - Focus: purple ring + line from the commander to its focus, with the reason.
  - Threats: red dots at believed enemy positions with the estimated count.
  - Assignment: commander → objective lines in the commander's colour — thick "Focus" / "Focus · forced", dashed with a cross "Blocked", thin "Planned". With the layer on and nothing selected only Focus and Blocked are drawn. Selecting a commander or an objective in the commander view shows its lines **even with the layer off**: a commander shows all its lines, and an objective shows the line from every commander to it. Planned and idle lines (thin dashed; only in worlds with 12 objectives or fewer) show only while something is selected. Logs: `obj_assign_overlay auto=1 why=cmd_sel|obj_sel`.
  - Group lines: each group to its waypoint, coloured by role (yellow fix, red assault, white transport, cyan recon, grey reserve).
  - Side card: what the commander is thinking, its focus, plan, what it waits on and its last 6 decisions.
- Older 3D debug: "Debug Mode" checkboxes on commander, manager, group, objective and threat response (Workbench), drawn while the GM is open.

### Logs
- Server only, one folder per session: `$profile:DCO/Logs/<date>/<hours>/session_<time>/` with `session.txt` (build, world, mods, config), `dco.log`, `perf.log`, `census.log`. In Workbench `$profile` is `Documents\My Games\ArmaReforgerWorkbench\profile`.
- Only errors go to the game console (prefix `[DCO]`). GM setting changes go to `$profile:DCO/Logs/settings_changes.log`.
- Line format: `time T+sec level CHANNEL subject text`. Channels: SYS, CMD, TACTIC, SQUAD, UNIT, COMBAT, CQB, ARTY, LOGI, MEDIC, VEH, PLAYER, GM, PERF, CENSUS, TEST.
- The log level is fixed at INFO for mission makers.
- Lines worth searching for:

| What | Search for |
|---|---|
| Commander decisions | `cmd_think`, `doctrine_decision`, `op_plan`, `tactic_chosen`, `tactic_none`, `tactic_abort`, `tactic_result` |
| Orders to groups | `cmd_order`, `task_change`, `task_no_wp` |
| Capture | `capture_blocked` (and `objective_owner` with a Benchmark Logger placed) |
| Spawner | `spawner_status`, `spawner_defer`, `spawner_spawn`, `spawner_assign`, `spawner_arrive`, `reinf_request`, `spawner_idle` (every 120 s: why a ready spawner is not spawning), `spawner_trigger_none` (an objective trigger that can never fire) |
| Config / presets | `config` in session.txt, `cfg_clamped`, `cfg_bad_type`, `cfg_parse_fail`, `loaded preset`, `start preset … not found` |
| Setup problems | `setup_check` (start-up checks, see below), `Auto-assign … failed`, `Commander UID … invalid/duplicate`, `doctrine_missing`, `Manager Prefab`, `obj_over_cap`, `obj_owner_legacy` |

### Start-up checks
About 20 s after the mission starts, DCO checks the setup and writes one block of lines to `dco.log`. If something is wrong, every GM online at that moment also gets one notice: "N setup problems found. See setup_check lines in dco.log." (a GM who joins later only finds the log lines).

| Line | Meaning |
|---|---|
| `setup_check global=missing` | No DCO_GlobalAIComponent: the game mode is not built on GameMode_Base. |
| `setup_check cmd=<UID> faction=EMPTY` | A commander has no Faction Key. |
| `setup_check cmd=<UID> uid=EMPTY` | A commander has no UID. |
| `setup_check navmesh=missing_world` | No navmesh in the world. |
| `setup_check obj=<name> navmesh=none` | No navmesh within 30 m of the objective (a tile that is not loaded yet is checked again 30 s later). |
| `setup_check obj=<name> handled_by=nobody` | No commander handles this objective. |
| `setup_check_done warn=<n> info=<n> commanders=<n> objectives=<n>` | Summary. `warn=0` means the setup is fine. |

### Tools
- **Battle map:** at the end of a mission DCO writes `battle.json` into the session log folder. Open `tools/battle_map.html` and drop the file on it: a replay with a time slider.
- **Census** (DCO Global: System): Census Logging + Census Interval (10–300 s) write population, morale and group lines. Off again after a restart.
- **Performance Profiling** (JSON `perfProfiling`): a PERF line every 30 s.
- **Benchmark Logger:** add `DCO_BenchmarkLoggerComponent` to an entity for periodic commander/objective/group lines and a summary.
- In Workbench play mode, profiling and census turn on automatically.

---

## 15. Performance and mission size

- **AI LOD Mode** (global): Vanilla (default, recommended), Prevent Max LOD, Full Detail. Per squad: "AI LOD" (Global default / Vanilla / Keep awake / Always full) and "AI detail" (Full / Balanced / Light).
- **Spawner Faction AI Cap:** set it to what your server can run (the help suggests 80–150 per faction).
- **Max Groups per Attack** 3–6, and Objectives at the same time, keep commanders from committing everything at once.
- Simple mode **Performance** row: Quality / Balanced / Performance.
- A raised think interval (150 s default in the commander prefab) costs less on big maps.

---

## 16. Common mistakes

| Symptom | Cause | Fix |
|---|---|---|
| An attacker's spawner never spawns | Preset Counterattack wave on the attacker: it waits for an objective its side holds to be lost, and the attacker holds none. | Use Commander on call or Steady top-up for attackers (Example D). GMs get the notice "<spawner> can never use its objective trigger …" and the log has `spawner_trigger_none`. |
| Commander does nothing at all | Base `Commander.et` / `Commander_Objective.et` placed without `CommanderManager.et` (log: `init tanpa manager dan Manager Prefab kosong`). | Place `Prefabs/Commander/CommanderManager.et`, or use the editable prefabs. |
| Commander does nothing for the first minutes | Think interval 150 s + first think after 60 s. | Lower Think Interval / Delay First Iteration for small missions. |
| Groups never join the commander | Group_Base has **Auto Assign off**; or the commander's **Faction Key is empty or different** (rejected; empty shows as `setup_check … faction=EMPTY`). | Tick Auto Assign (and set Dedicated Co), or set the GM Commander attribute; set Faction Key. |
| A side does not start with its objectives | Initial Owner Commander UID is empty (the old "Set Captured by Commander" is only a fallback). | Set **Initial Owner Commander UID** (GM: Controlled By). |
| An objective never changes hands | No commander is planning it (objectives only advance while worked on), or nobody may capture it (Allowed Factions). | Check the commander can reach it and wants it (Base Value), check Allowed Factions. |
| AI does not move at all | No navmesh, or the area is outside it (`setup_check navmesh=missing_world` or `obj=… navmesh=none`). | Generate the soldier navmesh for your area. |
| Do CQB ends immediately | No building in 25 m, building has no outside door/rooms on the navmesh, fewer than 2 men. | Move the waypoint, raise CQB Target Radius, use a bigger group. |
| Spawner never spawns | Player or enemy inside the 300 m safe radius, faction AI cap reached, stock empty, cooldown. | Watch the spawner status (GM card / `spawner_defer`), use Spawn anyway, lower the safe radius. |
| Commander takes a player's group | It does not; players get offers only after opting in. | — |
| A config value has no effect | Typo in the key, or the old default is still in the file. | Compare with [DCO_Server_Config.md](DCO_Server_Config.md); delete the key to get the new default. |
| Commander Debug Layers "AP candidates" shows nothing | That layer has no data yet. | Use the other layers. |

---

## 17. Server load failure: "Too many instructions per function"

**What you see:** the dedicated server stops at start with
`Can't compile "Game" script module! … Too many instructions per function`
pointing at some file (often a vanilla one, e.g. `SCR_AIProcessOrder.c(69)`). The file named is **not** the cause.

**Cause:** the Enfusion engine initialises all `static` values of every loaded script (vanilla + DCO + every other mod) in **one** generated function, and that function has an instruction limit (Bohemia tracker T200535). When the total of all mods together goes over the limit, the server cannot load the Game module. It can happen when you add a mod, when a mod updates, or after a DCO update.

**How to find the culprit:**
1. Start the server with only Reforger + DCO. If it loads, DCO alone is within budget.
2. Add your other mods back in halves (binary search) until the error returns. The last half you added contains a mod with many statics.
3. Report it to that mod's author (and to us with the full list of mods from `session.txt`).

**What DCO does:** DCO keeps its own static budget small and checks it before every release (tracker item STATICGUARD: a guard counts DCO's static initialisers and blocks any growth). DCO settings, labels and option lists live in config files and are built at runtime instead of static tables.

---

## 18. Example missions

Four example missions to open in Workbench and study, in `world/Examples/`. Status 2026-10-11: all four pass the start-up check (`setup_check_done warn=0`, one game each); only D has had a full run so far. The parts marked *Next* are still missing. Names below are the entity name with the objective **Name** (what GMs, logs and overlays show) in brackets.

### (a) Commander vs commander, 3 objectives
World: `world/Examples/DCO_Example_A_CommanderVsCommander.ent`
- US commander **Viking** (Offensive, Aggressive doctrine, Bold personality) against USSR commander **Volga** (Defensive).
- Objectives: OBJ_Primary (**Village**, held by Volga), OBJ_Secondary (**Farm**, neutral), OBJ_CheckPoint (**Crossroads**, held by Viking).
- Load-check 2026-10-11: `warn=0`; the 3 objective tiles were not loaded at T+20 s and passed the recheck 30 s later (`navmesh=ok`). No script or world errors in Scripter's 2026-10-10 run.
- US and USSR groups join their commander by faction (Auto Assign).
- Shows: commander setup, objectives, capture, the intent overlay (Ctrl+I).

### (b) Operation with a win condition
World: `world/Examples/DCO_Example_B_Operation.ent`
- OBJ_Primary (**Primary Objective**) with Operation Primary on and two Linked Secondaries: OBJ_Secondary (**Secondary Objective**) and OBJ_CheckPoint (**Checkpoint**).
- Load-check 2026-10-11: `warn=0` (Secondary Objective and Checkpoint passed the navmesh recheck).
- **DCO_VictoryComponent** on the game mode with the rule *Hold All Primary* (see §19).
- Shows: operations with linked secondaries, a win condition.
- *Next:* objective phases (available now behind `tune_obj_phase_on`, §4) and a Counterattack wave spawner for the defender (USSR holds the primary at start, so the wave can trigger).

### (c) Garrisoned defender vs attacker with reinforcements
World: `world/Examples/DCO_Example_C_GarrisonAndReinforcements.ent`
- **Volga** defends with its defense library (garrisons and field fighting positions); **Viking** attacks.
- One objective, **Primary Objective**.
- Spawner **Viking_Reinforcements**: 2 group types, cooldown 60 s, Commander asks on, safe radius 150 m.
- Load-check 2026-10-11: `warn=0` (Primary Objective passed the navmesh recheck).
- Shows: garrison defense, a spawner fed by commander requests, the safe radius.
- *Next:* a mortar team for Viking and a reinforcement stock of 60 men.

### (d) Complete mission template
World: `world/Examples/DCO_Example_D_FullMission.ent` — layers numbered in the order of this guide. **Load-checked 2026-10-11, `setup_check_done warn=0`** (Checkpoint passed the navmesh recheck), `victory_init enabled=1`, both spawners READY; one full run so far (see below).
- `01_Commanders`: **Viking** (US, Offensive) and **Volga** (USSR, Defensive), Reinforcement Stock 60 each (§7).
- `02_Objectives`: OBJ_Primary (**Primary Objective**, Operation Primary, linked secondaries OBJ_Secondary (**Secondary Objective**) and OBJ_CheckPoint (**Checkpoint**)), all held by Volga at start.
- `03_Groups`: a USSR squad on a **Garrison** waypoint (Defend mode) and a US squad on a **Do CQB** waypoint in Arleville (§6); a US **mortar team** — mortar + 4 men about 15 m away, role Mortar (§9); **5 US fire teams** US_FT_1 … US_FT_5 for the attack; every group on Auto Assign.
- `04_Spawner_Logistics`: **Viking_Reinforcements** (attacker: Take From Stock = Men, preset **Commander on call**, catalog Morale High), **Volga_Reinforcements** (defender: Men, preset **Steady top-up**), a **logistics hub** per side (§8).
- `05_Victory`: **DCO_VictoryComponent** with Hold All Primary + Enemy Stock Empty (§19) — the attacker wins by taking the primary, the defender by bleeding the attacker's stock.
- Expected start-up log: `setup_check_done warn=0` (§14).
- `default`: the commander manager has **Prevent Use LOD** ticked. This field is deprecated and has no effect: AI LOD is set by the Global AI **LOD Mode** (default Vanilla). Leave it as it is; it does nothing either way.
- First full run (2026-10-11, after the victory fix of §19): **US won** at T+967 s, `victory_end winner=US reason=all_primary held=1 captured=3`.
- *Known issue in that run, both causes fixed (to be confirmed in the next Example D run):* neither spawner spawned.
  - Viking: a Counterattack wave preset on the attacker never fires (no objective held at start). It now uses Commander on call; this case now logs WARN `spawner_trigger_none`.
  - Volga: Steady top-up worked as designed. Its line counts soldiers of the commander's own squads (peak 10, line 6), and manpower stayed at 10 until the first loss at T+515 s. It could not top up after that because a bug had switched it off at T+451 s (home objective counted as lost). Fixed: the home objective is lost only when an enemy owns it.
- Logistics in `04_Spawner_Logistics`, the same for both sides (§8 checklist):
  - near each hub (US_LogisticsHub, USSR_LogisticsHub), an **ammo truck** (M923A1 / Ural-4320) with a light fire team as crew: Dedicated Transport, Allowed Jobs = Resupply only;
  - a **covered transport truck** with its crew: Allowed Jobs = Transport + Extract + MEDEVAC;
  - each crew has a **Get In Nearest** waypoint on its truck;
  - an **arsenal box** at the hub (US_Hub_Arsenal / USSR arsenal) for crosslevel.
- Viking_Reinforcements' pool also has a **US Recon Team** (budget 1, role Recon), so the commander's requests for recon can be filled.

### Existing worlds you can open now
| World | What it shows |
|---|---|
| `world/DCO_Showcase.ent` (Arland) | 3 commanders, 5 objectives, US/USSR/FIA squads, vehicles, Benchmark Logger. |
| `world/DCO_TestLab.ent` (Arland) | US with squads, recon, vehicles and a mortar attacks a USSR hamlet. |
| `world/CommanderFullScale.ent` (Everon) | Full-scale US vs USSR war, 15 towns. |
| `world/ShowcaseRichz.ent` (Cain) | Two commanders with objective layers (starting owners come from the old "Set Captured by Commander" field). |
| `world/Test/DefaultGMTest.ent` | Bare GM test base (game mode, AI world, map) used by all `DCO_T_*` test worlds. |

---

## 19. Victory conditions and script events

### DCO Victory (ends the mission from DCO objectives)
Add **DCO_VictoryComponent** to the **game mode entity**. Rules can be combined; the first faction that meets any active rule wins.

| Attribute | Meaning | Default |
|---|---|---|
| Enabled | Turn the DCO victory check on. | On |
| Hold Count | A faction wins when it holds at least this many DCO objectives **and at least one of them was captured** (not owned by it at mission start). 0 = off. | 0 |
| Hold All Primary | A faction wins when it holds every objective marked Operation Primary (needs at least one) **and captured at least one of those primaries** (did not own it at mission start). Capturing only a secondary is not enough. | Off |
| Hold Objective + Hold Minutes | Entity name of one objective; a faction wins when it holds it this many minutes in a row. | Empty / 5 |
| Enemy Stock Empty | A faction wins when every enemy commander has used up its reinforcement stock (men). Unlimited stock (-1) never counts. | Off |
| Typed Objectives | Win from objective mission types (Defend, Hold until, Destroy): when all are finished, the faction that won most of them wins. | Off |
| Grace Seconds / Check Seconds | Time before the first check / between checks. | 60 / 5 |

**Captured, not kept (rule since 2026-10-11).** Hold Count and Hold All Primary only count for a faction that has taken something:
- **Hold Count:** at least one of the held objectives (any kind) was captured.
- **Hold All Primary:** at least one of the **primary** objectives was captured (since 15:45: in a test the defender took a neutral secondary and won at once).
- DCO records each objective's owner at mission start (log `victory_start_owner obj=<name> owner=<faction|none>`). An objective that gets its first owner during the grace time counts as that faction's starting objective (`late=1`).
- An objective that was neutral at start counts as captured once a faction takes it.
- A faction that meets Hold Count or Hold All Primary without the captured objective it needs does not win; the log says `victory_skip reason=start_owner faction=<f> …`.
- So a **defender that only keeps what it started with never wins through these two rules**. Give the defender a way to win: Enemy Stock Empty, Hold Objective (with Hold Minutes) or typed objectives (Defend / Hold until).
- Why: before this rule, a defender that started on all primaries won the moment the grace time ended (Example D ended at T+67 s with a USSR win).
- The end of the mission is logged as `victory_end winner=<faction> reason=<rule> …`, e.g. `victory_end winner=US reason=all_primary held=1 captured=3 captured_primary=1` (`captured_primary` since 15:45).

Examples: `world/Examples/DCO_Example_B_Operation.ent` uses Hold All Primary; `world/Examples/DCO_Example_D_FullMission.ent` uses Hold All Primary for the attacker and Enemy Stock Empty for the defender.

### Script events (for your own mission logic)
DCO raises server-side events for objective owner changes, contested objectives, commander mode changes, empty reinforcement stock, operations, spawners, reinforcement requests and fire missions. Subscribe from a server-side component (e.g. your game mode). Full reference with code: [DCO_Event_API.md](DCO_Event_API.md).

```c
DCO_Events ev = DCO_Events.Get();
ev.GetOnObjectiveOwnerChanged().Insert(OnOwnerChanged);
```

### Script control of focus and assignment (server side)
- Commander: `SetFocusObjective(objective, force)` / `ClearFocus()` on `AICommander_BaseComponent` (or `DCO_ObjAssign.SetFocusObjective(cmd, obj, force)` / `DCO_ObjAssign.ClearFocus(cmd)`).
- Objective: `DCO_SetAssign(mode, uids, by, playerId)`, `DCO_CanProcess(uid)`, `DCO_SetObjectiveType(type, by, playerId)`, `DCO_SetLinkedSecondaries(names, by, playerId)`.
- Group: `SetAllowedJobs(mask)` (Attack 1, Flank 2, Support by Fire 4, Recon 8, Defend 16, Garrison 32, Patrol 64, Reserve 128, Transport 256), `SetFirstToFight(bool)`.

---

## 20. Being tested (off by default)

These work but are switched off until they pass their tests. Turn them on in the server config (§12) or in DCO Settings (Expert); everything marked **Beta** in DCO Settings is in this state or newer.

- **Objective phases** — `tune_obj_phase_on = 1` (§4).
- **Attack Only / Defend Only commanders per objective** — `tune_obj_role_on = 1` (§4).
- The list of every switch and its test status is kept by the team in `tools/reports/default0_switches_2026-10-11.md`; switches that pass become default ON in an update (§12).

Earlier "coming soon" items that have landed: Destroy with targets (§4 *Objective types*), objective phases (§4), per-faction blocks in the server config (§12).

---

## 21. Checklist before you publish

- [ ] Game mode on GameMode_Base, AI World with navmesh, Faction Manager, Commander Manager.
- [ ] Every commander: Faction Key and a unique UID set; mode and think interval fit the mission size.
- [ ] Every objective: unique Name, Radius, Initial Owner Commander UID where needed.
- [ ] Every AI group: Auto Assign (+ Dedicated Co) or a GM Commander, or comes from a spawner.
- [ ] Spawners: faction AI cap and safe radius set for your server; stock if reinforcements should run out.
- [ ] Played once in Workbench with the intent overlay; `setup_check_done warn=0`; no `Auto-assign … failed`, `Manager Prefab`, `UID … invalid` or `doctrine_missing` lines in `dco.log`.
- [ ] Server config reviewed against [DCO_Server_Config.md](DCO_Server_Config.md); server starts with your full mod list (§17).
