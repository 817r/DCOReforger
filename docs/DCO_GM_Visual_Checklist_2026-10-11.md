# DCO GM visual checklist — 2026-10-11

For: the user, in-game, as Game Master. Written by DCO UI/UX.
Covers every new GM tab, attribute, card, page and overlay from 2026-10-10/11, in the order to test them. Each step says what to do and what you should see. Tick the box, or write what you saw instead.

All items are built (status 2026-10-11 09:10); the ones added today are compile-clean but untested in game.

---

## 0. Setup

- World: `world/Examples/DCO_Example_D_FullMission.ent`. Its contents:
  - **Viking** (US, attacker) and **Volga** (USSR, defender), Reinforcement Stock 60 each;
  - **Primary Objective** with linked secondaries **Secondary Objective** and **Checkpoint** (entity names OBJ_Primary, OBJ_Secondary, OBJ_CheckPoint; the GM sees the Name);
  - a Garrison squad, a Do CQB squad and a mortar team;
  - 2 spawners and 2 logistics hubs;
  - DCO Victory.
- Start Play as Game Master. Keep `dco.log` open: `$profile:DCO/Logs/<date>/<hours>/session_*/dco.log`. In Workbench, `$profile` is `Documents\My Games\ArmaReforgerWorkbench\profile`.
- UI text size at 100 %.

---

## 1. Start-up check

| # | Do | Expected | Log |
|---|---|---|---|
| 1.1 | Wait about 20 s after start. | **No** GM notice: Example D is clean. | `setup_check_done warn=0` |
| 1.2 | Stop. In Workbench, clear Volga's **Faction Key** (do not save the world). Play again. | After ~20 s, a GM notice titled "DCO setup check": "1 setup problem found. See setup_check lines in dco.log." It must say "problem", not "problems". | `setup_check cmd=Volga faction=EMPTY …`, then `setup_check_done warn=1` |
| 1.3 | Undo the change (Ctrl+Z or reload without saving). | — | — |

## 2. Objective tab (right-click Primary Objective › Attributes, tab "DCO Objective")

| # | Do | Expected | Log |
|---|---|---|---|
| 2.1 | Open the tab. | These rows are visible:<br>- **Type** (Capture / Destroy / Recon / Defend / Hold until)<br>- **Handled By** (All Commanders / Only Checked / All Except Checked)<br>- **Commanders** (checkbox list "Viking · US", "Volga · USSR", alphabetical)<br>- **Focus For** (same list)<br>- **Force Focus**<br>- **Linked Secondaries** (checkbox list in objective-list order; with one objective open each entry reads "<name> · <n> m" and the objective itself "· this objective") | — |
| 2.2 | Read the descriptions of all rows. | All English, one or two sentences. Commanders says it is "Ignored while Handled By is All Commanders". | — |
| 2.3 | Handled By = Only Checked, tick nothing, Apply. | GM notice "Nobody handles Primary Objective. It is ignored until a commander is checked." | `obj_assign_set`, `obj_assign_empty` |
| 2.4 | Tick Viking, Apply. Reopen. | Only Checked + Viking ticked are kept. | `obj_assign_set` |
| 2.5 | Focus For: tick Volga (not allowed while Only Checked = Viking). Apply. | GM notice "Volga cannot focus Primary Objective: it does not handle this objective." The tick is not kept. | `obj_focus_reject` |
| 2.6 | Focus For: tick Viking, Apply. Then open Secondary Objective and tick Viking in Focus For there, Apply. | GM notice "Viking's focus moves from Primary Objective to Secondary Objective." | `obj_focus_set` |
| 2.7 | Type: pick each entry once. | Defend and Hold until show their extra rows (time, faction, deadline, fail ratio). Capture / Destroy / Recon do not. | — |
| 2.8 | Linked Secondaries with Operation Primary on. | Secondary Objective and Checkpoint are ticked. Entries show distances ("Secondary Objective · <n> m"); Primary Objective reads "· this objective". Open two objectives together: plain names, no distances. | — |
| 2.9 | DCO Settings window (F2 on the objective): turn Operation Primary off. | Linked Secondaries row is greyed out (value kept). The vanilla dialog does **not** grey it (by design). | — |
| 2.10 | Put Handled By back to All Commanders, clear the focus. Apply. | — | — |

## 3. Commander tab (right-click Viking › Attributes, tab "DCO Commander: Style & Attack")

| # | Do | Expected | Log |
|---|---|---|---|
| 3.1 | Open the tab. | Rows **Focus Objective** (None + objectives), **Force Focus**, **Blocked Objectives** (checkbox list of objectives). | — |
| 3.2 | Focus Objective = Primary Objective, Force Focus on, Apply. Reopen Primary Objective's objective tab. | Focus For shows Viking ticked; Force Focus on. Both tabs agree. | `obj_focus_set` |
| 3.3 | Blocked Objectives: tick Checkpoint, Apply. Open Checkpoint's objective tab. | Handled By = All Except Checked with Viking ticked. | `obj_assign_set` |
| 3.4 | Untick it again, clear the focus (None), Apply. | Checkpoint back to All Commanders. Focus cleared, Force Focus off. | `obj_focus_done` or `obj_focus_set … none` |

## 4. Squad tab (right-click the US Do CQB squad › Attributes, tab "DCO Squad: Commander Permissions")

| # | Do | Expected | Log |
|---|---|---|---|
| 4.1 | Open the tab. | Rows:<br>- **Commander Can Task**<br>- **Task Preset** (Any Task / Attack Only / Recon Only / Defend Only / Transport Only / Reserve Only / Custom)<br>- **Allowed Tasks** (9 boxes: Attack, Flank, Support by Fire, Recon, Defend, Garrison, Patrol, Reserve, Transport)<br>- **First to Fight**<br>- **Can Ride Commander Transport** (renamed from Allow Transport) | — |
| 4.2 | **Task Preset = Recon Only → Apply → reopen.** | Dropdown shows **Recon Only**; only **Recon** is ticked. **Mask 8.** (This is AI Design's open test.) | `grpjob` set line with mask 8 / preset recon |
| 4.3 | Untick Recon and tick Attack + Flank by hand, Apply, reopen. | Task Preset shows **Custom**. | mask 3 |
| 4.3b | Task Preset = **Reserve Only**, Apply, reopen. | Only Reserve ticked. | mask 128 |
| 4.4 | Untick everything, Apply. | GM notice "<squad> gets no commander jobs." | — |
| 4.5 | First to Fight on, Apply. | Log shows your player id, not -1. | First to Fight line with `player=<your id>` |
| 4.6 | Pick Attack Only but do **not** Apply yet. | The boxes change at once to Attack, Flank, Support by Fire. | `grpjob_ui_sync src=preset` once |
| 4.7 | Then tick Recon. | Dropdown changes to Custom. Description: "Custom: 1 changed from Attack Only." In the DCO Settings window the row reads "Custom · 1 changed from Attack Only". | `grpjob_ui_sync src=tasks`, `grpjob_ui_custom` |
| 4.7b | Pick Attack Only again (vanilla dialog). | The description goes back to the normal Task Preset text (no "Custom: …" left over). | — |
| 4.8 | Untick everything (DCO Settings window, Group target). | Row reads "Custom · no tasks". | — |
| 4.9 | Commander Can Task off. | Task Preset, Allowed Tasks and First to Fight are greyed out in both the vanilla dialog and the DCO Settings window. Their values stay. Turn it on: same values. | `grpjob_ui_gate can_task=0` / `1` |
| 4.10 | Put the squad back to Any Task, Commander Can Task on. Apply. | — | — |

## 5. Assignment overlay (Ctrl+I in the GM world view)

| # | Do | Expected | Log |
|---|---|---|---|
| 5.1 | Viking › Attributes › **Commander Debug** on, **Commander Debug Layers**: tick **Assignment**. Ctrl+I. | Lines from Viking's HQ to objectives in Viking's colour:<br>- **thick** "Focus" (or "Focus · forced") to its focus;<br>- **dashed with ×** "Blocked" to blocked objectives;<br>- nothing to idle objectives.<br>Labels readable (22 px, dark halo). | `obj_assign_overlay on=1 … lines=<n>` |
| 5.2 | Nothing selected. | Only Focus and Blocked lines (declutter). | `obj_assign_overlay declutter=1 …` |
| 5.3 | Layer on, click the objective Viking is planning (e.g. Primary Objective). | Viking's thin "Planned" line to it appears, plus the line from every other commander to that objective. Viking's lines to other objectives do **not** appear (only a commander click shows all of a commander's lines). | `obj_assign_overlay auto=1 why=obj_sel` |
| 5.4 | Click **Viking** itself (commander select). | Its Planned line shows too (bug G2, fixed 08:55). | — |
| 5.5 | Layer **off** (untick Assignment), click Viking. | Its lines appear anyway: Focus, Blocked, Planned, plus idle objectives as thin dashed lines. Esc: lines gone. | `obj_assign_overlay auto=1 why=cmd_sel` |
| 5.6 | Layer off, click an objective. | One line from **every** commander to that objective, in each commander's colour. | `obj_assign_overlay auto=1 why=obj_sel` |
| 5.7 | Layer off, nothing selected. | No assignment lines. | `obj_assign_overlay auto=0` |
| 5.8 | World with more than 12 objectives (e.g. `world/CommanderFullScale.ent`): click a commander. | Focus / Blocked / Planned lines show; **no** idle dashed lines (left out above 12 objectives). | `obj_assign_overlay idle_lines=0 objectives=<n>` once |
| 5.9 | Dedicated server with you as a client GM: repeat 5.5 and 5.6. | Same lines as in Workbench. The `cmddbg:send` count does not rise while nothing changes. | `cmddbg_assign_only cmds=<n> shapes=<m>` only when the count changes |

Section 5 was built by Scripter on 2026-10-11 (compile clean 08:55), not yet tested in game. Hover does not show lines; only selection does.

## 6. Spawner in the world (GM)

| # | Do | Expected | Log |
|---|---|---|---|
| 6.1 | Look at Viking_Reinforcements' icon. | A status mark with both shape and colour:<br>- green dot = Ready;<br>- orange triangle = Waiting;<br>- blue ring = Cooldown;<br>- red diamond outline = Off;<br>- red square = Stopped. | — |
| 6.2 | Select (or hover) the spawner. | Filled spawn radius circle "Spawn radius 30 m". Dashed safe ring "Safe radius … · no spawn while anyone is inside". A line in the commander colour "→ Viking (US)". If it has a trigger objective, a dotted line "Trigger: <objective> lost". Deselect: circles and lines gone, mark stays. | — |
| 6.3 | Walk your player (or place a US soldier) inside the safe radius. Right-click the spawner › **Spawn now**. | Mark turns into an orange triangle. One GM message: "Viking_Reinforcements is waiting: Player <name> is <n> m away, inside the … m safe radius. It spawns as soon as the area is clear." With a **Spawn anyway** button. | `spawner_status state=waiting reason=player_near` |
| 6.4 | Press **Spawn anyway**. | Spawns once. Message "… → Viking: N groups ordered …". | `spawner_force`, `spawner_spawn` |
| 6.5 | Right-click › **Turn spawner on/off** (off). | Off mark. Status sentence "Turned off by the GM …". | `spawner_off reason=gm` |
| 6.6 | Turn it on again. | Back to Ready or Cooldown. | `spawner_on reason=gm` |

## 7. Spawner window (select the spawner › F2, or DCO Settings › Force › Spawner)

| # | Do | Expected | Log |
|---|---|---|---|
| 7.1 | Open it. | Window bar: spawner name, crumb, **Preset** button ("Counterattack wave", or "… · N rows changed" after edits). | — |
| 7.2 | Status panel (left). | Status pill plus one sentence with the reason. Rows: Feeds, Spawned, Last, Next allowed, Reinforcement (request colour by priority), On the way, Pool bar, Stock (the counter it does not take from is dimmed). Buttons Spawn now / Show on map / Turn off. No "Spawner Spawner" prefix, no "1 groups". | — |
| 7.3 | Sheet order (C1, held for your visual check). | Expected sections in this order, with headers "1 of 5" … "5 of 5":<br>1. **Who gets the groups**<br>2. **Which groups · force pool**<br>3. **When it spawns**<br>4. **When it stops**<br>5. **Limits**<br>Faction AI cap appears under Limits, dimmed, with the tag "Global". **Write the order you see** if it differs. | — |
| 7.4 | Commander chooses On. | Weight column is **not** dimmed (accepted on Auditor's request). The note line explains that weight only breaks ties. | — |
| 7.5 | Pool: click a row. | One edit panel under the table: Count − / +, Role, Experience, Morale, Shape, Arrival, Cost ("auto: …" or "set by GM"), Done / Remove group. Total line updates. | `spawner_pool op=edit` |
| 7.6 | **+ Add group** › "<faction> catalog" and "My presets". | Presets tagged "· server" / "· my PC" / "· built-in"; first entry "+ New preset". | `spawner_pool op=add` |
| 7.7 | When it spawns / When it stops cards. | Each card is On/Off with one line of text. Header shows "N of 5 on" / "N of 4 on". Countdown updates every 5 s, not every frame. | — |
| 7.8 | Workbench parity: in Workbench, set Cooldown to an out-of-range value. | Clamped, with a log line. | `cfg_clamped` / spawner clamp line |

## 8. Group presets

| # | Do | Expected | Log |
|---|---|---|---|
| 8.1 | DCO Settings › Global › **Group presets**. | Count "N on this server · M on my PC · K built-in" (single player: no server part). Filters Faction / Where. Table with Where tags. Built-in rows: Edit / Rename / Delete disabled, Duplicate works. Read-only row "Who can copy presets to this server · set by server". | `group_preset_open from=global` |
| 8.2 | **New preset**: add 9 men, vehicle with 4 seats. | 12-slot bar. Men without a seat shaded in the warm colour. Seat line "9 men, 4 seats: 5 walk … (shaded in the bar above)". Checks panel; "Ready to save." when valid. | — |
| 8.3 | Back with changes. | "Discard this preset? The changes are lost." Discard / Keep editing. | `group_preset_discard` |
| 8.4 | Save. | Returns to the list with the new preset selected. | `group_preset op=save` |
| 8.5 | World: right-click an AI group › **Save as group preset**. | GM message: Saved "<faction> <group> (saved)" on this server (<n> men). Rename or edit it in Global › Group presets. … | `group_preset op=save from=world` |
| 8.6 | From the spawner: + Add group › My presets › + New preset › **Save and add to <spawner>**. | Back in the force pool with the new row selected. | `spawner_pool op=add` |

## 9. Reinforcement requests (DCO Settings › Global › Commander › "Reinforcement requests")

| # | Do | Expected | Log |
|---|---|---|---|
| 9.1 | Open the page. | Rows: **Routine Below**, **Priority Below**, **Urgent Below**, **Attack Ratio**, **Hasty Attack Ratio**, **Defense Ratio**, **Min Intel Confidence**, **Hold Time**. No "Reinforcements: " prefix on the names. | — |
| 9.2 | Look at the values. | Shares show 2 decimals (0.85, not 0.9). Hold time shows "s". | — |
| 9.3 | Read the help of each row. | Mentions "FIA: 0.1 higher" on the strength rows. The Attack Ratio help points to the Hasty Attack Ratio row. Hold time help says the check runs every 30 s. | — |
| 9.4 | Commander page (Viking). | Block "Reinforcements · Viking": Stock men and Stock points bars, one line per spawner with its status pill, and the request line. | — |
| 9.5 | Strength bar with ticks (C4). | Optional, not built. Expect it to be missing. | — |

## 10. Simple mode (DCO Settings › Global › Simple)

| # | Do | Expected | Log |
|---|---|---|---|
| 10.1 | Fresh mission, open Simple. | A level is highlighted on every row. Note any row with no highlight (S9: Cohesion, Suppression). | — |
| 10.2 | AI Aggression description. | Says Normal is the DCO default and already bolder than vanilla, and that Cautious is the calmest DCO level. | — |
| 10.3 | AI Skill description. | Ends "Applies to newly spawned soldiers. Groups from a Commander Spawner use the spawner's Experience instead." (4 rows carry "Applies to newly spawned soldiers".) | — |
| 10.4 | Artillery row. | First level reads **"Off for AI"**. | — |
| 10.5 | Click each level of Squad teamwork, Grenades & GL, Suppression. | The highlight moves to the level clicked. If two levels highlight together, or a level never highlights, note it (S3/S4). | `dco_settings` set lines |

## 11. Text sizes (held for your visual check: G8, D1, TEXTSIZE)

Target: body text, buttons and card text **18 px** at 100 % UI size; headers, badges and tags at least 14–16 px. Write "OK" or "small" for each.

| # | Where | Size in the layout today |
|---|---|---|
| 11.1 | Contact toast (top of screen on contact) | 17 px |
| 11.2 | Map labels (map open) | 14 px |
| 11.3 | Briefing world markers | 14 px |
| 11.4 | Offer card (player tasking) | 15 / 17 px |
| 11.5 | Radio banner with several lines (children can shrink to 0.6×) | down to ~10 px |
| 11.6 | Spawner status panel side lines, state chip, buttons | 16 / 17 px |
| 11.7 | Spawner pool cells, edit labels, total line, picker rows | 16 px |
| 11.8 | Spawner GM card body (long "waiting" text) | 16 px; check the text is **not cut off** |
| 11.9 | Group preset library columns, builder lines | 16 px; some 14 px |
| 11.10 | GM UI text size option (TEXTSIZE): set it larger | All DCO GM text above grows with it. |

## 12. Pad only (controller, no mouse) in the DCO Settings window

Today only setting rows, Simple rows, the top bar, the footer and dialogs are reachable by pad (audit 2026-10-10). Expected today: the items below are **not** reachable. They are in the CONSOLEUI backlog (item 146), not a regression.

| # | Try with the pad | Expected today |
|---|---|---|
| 12.1 | Setting rows, Simple rows, LB/RB sections | Work. After LB/RB no row is highlighted until you move. |
| 12.2 | Spawner pool rows, edit panel, + Add group | Not reachable |
| 12.3 | Spawner status panel buttons (Spawn now / Spawn anyway / Turn off) | Not reachable |
| 12.4 | Group preset builder: Save | Not reachable (B, B discards) |
| 12.5 | Group preset library buttons, rename box | Not reachable |
| 12.6 | Faction / Members scope spinner, favourite star | Not reachable |
| 12.7 | Right stick on a long sheet | Does not scroll the sheet |

---

## Report back

For each section, send the step numbers that did not match, plus a screenshot or the log line. The owners are:
- Scripter: sections 2, 3, 5, 6, 7;
- Scripter 2: sections 1, 4 (UX part), 8, 9;
- UI/UX: sections 10, 11, 12 and all text.
