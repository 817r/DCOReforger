# DCO Modding API (events, settings, AI state)

Use this API from your own addon instead of `modded class` on DCO internals. DCO internals change between updates; the functions on this page stay stable.

- Code: `scripts/Game/DCO/API/DCO_API.c` (settings + state) and `scripts/Game/DCO/Commander/DCO_Events.c` (events)
- Version: `DCO_API.Version()` returns `100` (= 1.0, major x 100 + minor).
- Example addon: `DCO_API_Example/` (a listener that uses every part of this page).

## Rules

- Add DCO AI (GUID `5EDAAE0D3CC60D6B`) to the Dependencies of your `addon.gproj`.
- Everything runs on the server. Calls that change something are refused on clients.
- Every change needs your mod name as the first argument. It is written to the DCO log (`api_set mod=... scope=... key=... old=... new=...`) and to `report.txt` under "Changed by other mods through the DCO API", so server owners can see which mod changed what.
- Refused calls return `false` and write a warning (`api_reject mod=... key=... why=...`). Reasons: `unknown_key`, `not_server`, `mod_name_empty`, `no_faction_layer`, `no_dco_squad`, `no_dco_soldier`.
- Values outside the allowed range are clamped, not refused.
- Within one major version, nothing on this page is removed or renamed. A function that will go away is marked deprecated here for at least one release first.

## Settings

| Function | What it does |
|---|---|
| `DCO_API.HasSetting(key)` | True if the key exists. |
| `DCO_API.GetSetting(key, faction = "")` | Current value. Empty faction = global. |
| `DCO_API.SetSetting(mod, key, value, faction = "")` | Changes a global setting, or the setting of one faction. A faction only has its own settings when the server config has a `factions` block for it; otherwise the call is refused. |
| `DCO_API.GetSquadSetting(group, key)` / `SetSquadSetting(mod, group, key, value)` | Per squad. Key: `vehicleExit` (-1 = use the global value). |
| `DCO_API.GetSoldierSetting(soldier, key)` / `SetSoldierSetting(mod, soldier, key, value)` | Per soldier. Keys: `aiSkill`, `aimAccuracy`, `aimSpeed`, `aiPerception`, `suppressionEffect`, `takeCoverChance`, `personality` (0 Cautious, 1 Standard, 2 Aggressive, 3 Reckless). |

Global and faction keys:
- the main settings, with the same names as in `DCO_GlobalConfig.json`: `aiAggression`, `vehicleExit`, `aiSkillDefault`, `aimAccuracy`, `aimSpeed`, `aiPerception`, `suppressionEffect`, `takeCoverChance`;
- every tuning key (544). Use the name from `DCO_GlobalConfig.json` (for example `tune_hear_shot_err`); the short internal name (`hear.shot_err`) works too. The full list with ranges is in `DCO_GlobalConfig_README.txt` next to the server config.

Global skill, accuracy, aim, suppression and cover values are copied to soldiers when they spawn, so a change reaches new soldiers. Aggression, perception and tuning keys take effect at once. Use the soldier functions to change soldiers that already exist.

```c
DCO_API.SetSetting("MyMod", "aiAggression", 1.5);
DCO_API.SetSetting("MyMod", "tune_hear_shot_err", 0.5);
DCO_API.SetSetting("MyMod", "aiPerception", 0.8, "USSR");
DCO_API.SetSquadSetting("MyMod", group, "vehicleExit", 16);
DCO_API.SetSoldierSetting("MyMod", soldier, "aiPerception", 1.5);
```

## AI state (read only)

| Function | Returns |
|---|---|
| `SoldierMorale(soldier)` | `moraleState`: 0 Calm, 1 Tense, 2 Shaken, 3 Panic, 4 Break. -1 if not a DCO soldier. |
| `SoldierMoraleValue(soldier)` | Morale pressure as a number (higher = worse). |
| `SoldierStress(soldier)` | 0 calm, 1 suppressed, 2 pinned. |
| `SoldierSuppression(soldier)` | Suppression 0..1. |
| `SoldierInCover(soldier)` | True when in cover or inside a building. |
| `SoldierAmmo(soldier)` | Rounds left in the current magazine, 0..1. |
| `SquadTask(group)` | `DCO_EGroupTask`: NONE, ATTACK, FLANK, SUPPORT_BY_FIRE, DEFEND, GARRISON, RECON, PATROL, REINFORCE, TRANSPORT, FIRE_MISSION. |
| `SquadPhase(group)` | `DCO_ETaskPhase`: MOVING, STAGING, EXECUTING, HOLDING. |
| `SquadState(group)` | `DCO_EGroupState` flags: EN_ROUTE 1, IN_CONTACT 2, SUPPRESSED 4, COMBAT_INEFFECTIVE 8, RETREATING 16, MOUNTED 32. |
| `SquadMorale(group)` | Average morale pressure of the squad. |
| `SquadAlive(group)` / `SquadPeak(group)` | Soldiers alive now / most soldiers the squad had. |
| `SquadObjective(group)` | The objective the commander gave the squad, or null. |
| `SquadCommander(group)` | The commander of the squad, or null. |
| `Commanders(outList)` / `Objectives(outList)` | All DCO commanders / objectives. Returns the count. |

Commander details come from the commander itself: `GetCommanderMode()`, `GetReinforcementStock()`, `GetPointsStock()`, `GetCommanderFactionKey()`. Objective details: `GetOwnerFaction()`, `GetLogName()`.

## Events

```c
DCO_Events ev = DCO_API.Events();
// or
DCO_Events ev = DCO_Events.Get();
// or
DCO_Events ev = AICommander_ManagerComponent.GetInstance().GetEvents();
```

The events only fire on the server (`Replication.IsServer()`). Subscribe from a server-side component, for example your game mode or a mission entity.

### Soldier and squad events

| Getter | Signature | When it fires |
|---|---|---|
| `GetOnContact()` | `(SCR_AIGroup group, IEntity enemy, vector enemyPos)` | A squad that knew no enemies now knows one (seen or heard). It fires again only after the squad has forgotten every enemy (about 150 s without new information). |
| `GetOnStressChanged()` | `(IEntity soldier, int fromLevel, int toLevel)` | Stress changes between 0 calm, 1 suppressed and 2 pinned. |
| `GetOnMoraleChanged()` | `(IEntity soldier, int fromState, int toState)` | Morale state changes (`moraleState` values above). |
| `GetOnSquadTaskChanged()` | `(AIGroup group, DCO_EGroupTask fromTask, DCO_EGroupTask toTask, string by)` | A squad gets a new task. `by` says who gave it. |
| `GetOnSoldierKilled()` | `(IEntity victim, IEntity killer, float distance)` | An AI soldier dies. `killer` is null when unknown; `distance` is -1 then. |

Stress and morale events can fire often in big battles. Keep the handlers short.

Logging: contact and soldier_killed write `event name=...` lines; morale and task changes are already in the log as `morale_state` and `task_change`. Every event also counts `evt:<name>` in the performance counters.

### Commander and mission events

| Getter | Signature | When it fires |
|---|---|---|
| `GetOnObjectiveOwnerChanged()` | `(CMD_AICommanderObjectiveComponent objective, FactionKey fromFaction, FactionKey toFaction)` | Objective owner changes. An empty key means no owner. Checked once per second. |
| `GetOnObjectiveContested()` | `(CMD_AICommanderObjectiveComponent objective, bool contested)` | Two or more factions now have presence in the objective (true), or only one or none is left (false). |
| `GetOnCommanderModeChanged()` | `(AICommander_BaseComponent commander, CMD_ECommanderMode fromMode, CMD_ECommanderMode toMode)` | Commander switches OFFENSIVE / DEFENSIVE / BALANCED. |
| `GetOnCommanderStockEmpty()` | `(AICommander_BaseComponent commander, string stockKind)` | Reinforcement stock drops from >0 to 0. `stockKind` is `"soldiers"` or `"points"`. Unlimited stock (-1) never fires. |
| `GetOnOperation()` | `(string eventName, string detail)` | Commander operation events: `op_plan` (start), `op_phase`, `op_phase_end`, `op_branch`, `op_replan`, `op_reserve`, `op_end` (end), and more. `detail` is the full `key=value` log line, e.g. `op_end cmd=US_1 primary=OBJ_Town reason=captured ...`. |
| `GetOnSpawner()` | `(string eventName, string detail)` | `spawner_spawn` (group spawned), `spawner_off` (switched off, e.g. by GM or enemy nearby), `spawner_stop` (stopped for good: stock or points used up). |
| `GetOnReinforcementRequest()` | `(string eventName, string detail)` | A commander asked for reinforcements (`reinf_request cmd= id= kind= why= ...`). |
| `GetOnFireMission()` | `(string eventName, string detail)` | An artillery or mortar fire request was queued (`arty_request src= tier= shells= type= pos=x,z`). |
| `GetOnAnyEvent()` | `(string eventName, string detail)` | Every event above, in text form. Easy for logging or for forwarding to Scenario Framework. |

## Example

```c
class MyMissionLogic : ScriptComponent
{
	override void OnPostInit(IEntity owner)
	{
		if (!Replication.IsServer())
			return;
		DCO_Events.Get().GetOnObjectiveOwnerChanged().Insert(OnOwner);
		DCO_Events.Get().GetOnCommanderStockEmpty().Insert(OnStockEmpty);
		DCO_Events.Get().GetOnOperation().Insert(OnOp);
	}

	void OnOwner(CMD_AICommanderObjectiveComponent obj, FactionKey from, FactionKey to)
	{
		Print("Objective " + obj.GetLogName() + " taken by " + to);
	}

	void OnStockEmpty(AICommander_BaseComponent cmd, string kind)
	{
		Print(cmd.GetCommanderUID() + " has no more " + kind);
	}

	void OnOp(string name, string detail)
	{
		if (name == "op_end")
			Print("Operation finished: " + detail);
	}
}
```

Always remove your listeners when your component is deleted, e.g. `GetOnOperation().Remove(OnOp)`.

`GetOnAnyEvent()` covers the commander and mission events only, not the soldier and squad events.

## Logging

Every event writes one line to the DCO log, channel CMD:

```
event name=<eventName> <detail>
```

Operation events are already logged by the planner, so they only add a perf counter `evt:<eventName>`. At start the log shows `event_api_ready objectives=N commanders=M`.

## Notes

- Objective "completed/failed" at operation level: `op_end ... reason=` tells you why the operation ended.
- Objective level: owner change = captured or lost; contested = fighting inside.
- The polled events (owner, contested, mode, stock) are checked once per second, so a callback may come up to 1 s after the change.

## DCO Victory component (VICTORY #169)

`DCO_VictoryComponent` (`scripts/Game/DCO/GameMode/DCO_Victory.c`) ends the mission when a faction meets one of the conditions below.

Setup: add the component to your game mode entity, or to any entity in the world. Only the server checks the conditions. The first faction to meet a condition wins, and the mission ends through the vanilla `EndGameMode` (end reason SCORELIMIT, winner = that faction).

| Attribute | Default | Meaning |
|---|---|---|
| Enabled | on | Master switch. |
| Hold Count | 0 (off) | A faction wins when it holds at least this many DCO objectives. |
| Hold All Primary | off | A faction wins when it holds every objective marked Primary. Needs at least one primary objective. |
| Hold Objective | empty (off) | Entity name of one objective. A faction wins after holding it for Hold Minutes in a row. The timer resets when the owner changes. |
| Hold Minutes | 5 | Minutes for Hold Objective. |
| Enemy Stock Empty | off | A faction wins when every enemy commander has used up its soldier stock. Unlimited stock (-1) never counts. |
| Typed Objectives | off | Uses objective mission types (OBJTYPES #167). Once every typed objective has finished, the faction that won the most of them wins. Who wins each one: DEFEND done = defender; DEFEND failed = the faction that captured it (why=captured_by_X), otherwise the strongest enemy faction in the area; HOLD_UNTIL done = holder, HOLD_UNTIL failed = nobody; DESTROY done = the objective Type Faction, otherwise the strongest non-defender faction; DESTROY failed = defender (nobody if the defender was neutral). A tie means no winner (log victory_typed_no_winner). |
| Grace Seconds | 60 | No check before this many seconds after start. |
| Check Seconds | 5 | Time between checks. |


Logs (channel CMD):
- `victory_init` with the settings, once at start;
- `victory_check` with each faction's held/total and primary counts, written whenever they change;
- `victory_hold obj= owner= need_min=` when the owner of the hold objective changes;
- `victory_end winner= reason=` when the mission ends.

Script event: `DCO_Events.Get().GetOnVictory()` with `("victory_end", "victory_end winner=US reason=hold_count held=3 need=3")`.
