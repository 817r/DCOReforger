# DCO Settings window: pad-only access (CONSOLEUI, backlog item 146). DRAFT

Status: **draft for the user's decision. Not sent to any Scripter.** Written by DCO UI/UX on 2026-10-11 from the pad audit of 2026-10-10 and today's code.

Goal: a GM with only a controller (Xbox / PlayStation, or a PC GM without a mouse) can reach and use **every** control in the DCO Settings window. The mouse and keyboard keep working exactly as today.

---

## 1. Where it fails today

Pad focus only knows two kinds of lines. `FocusKeys` (DCO_SettingsDialogUI.c ~1337) collects:
- setting rows (line type 1, part 0);
- Simple rows (line type 3).

`MoveFocus`, `OnKeySelect` (~984) and `KeyChange` (~1410) only act on those two kinds. Everything else on the page has click targets (`Act(...)`, ~2882) but the pad never visits them:

| Area | What the pad cannot do |
|---|---|
| Spawner: force pool | select a pool row, open the edit panel (~16 buttons), **+ Add group**, pick from the catalog or **My presets**, **+ New preset** |
| Spawner: status panel (left) | **Spawn now**, **Spawn anyway**, **Turn off / Turn on**, **Show on map** |
| Group preset builder | every control, including **Save** (B, B only discards) |
| Group preset library | filters, rows, **New / Edit / Duplicate / Rename / Delete**, **Copy to server / Copy to my PC** |
| Top of the sheet | Faction / Members scope spinner, favourite star, section **?** help, section revert, **Clear search** |
| Description line | the "use faction / group value" action |
| Text boxes | builder name box and library rename box (search and TEXT rows already work) |
| Keyboard-only actions | Log (L), Discard (D), Save (S), Export (X), Import (I) |
| Feedback | after LB/RB (section switch) no row is highlighted until the stick moves; the right stick does not scroll the sheet |

Already fine: setting rows, Simple rows, the MULTI cell cursor, top bar (View click/hold, Y presets), footer, discard-on-close dialog, review panel, the open-properties chooser.

---

## 2. Model: one focus list for everything

**One rule:** every rendered line that has at least one click target is a stop for the pad. Inside a line, left/right moves between its click targets ("cells"). A runs the focused cell.

1. **Record the targets per line.** While a line renders, store the first and last `Act` id it creates (`m_iActFrom`, `m_iActTo` on DCO_SetLine). No new tables; this is two ints per line.
2. **FocusKeys** adds every line whose act range is not empty, in screen order. Setting rows and Simple rows keep their keys, so nothing changes for them.
3. **Inside a line:** `m_iFocusAct` = the focused act id.
   - Left/right steps through the line's acts.
   - On setting rows, left/right still changes the value as today. To reach the row's other cells (star, revert), hold LT + left/right.
4. **A** runs `OnAct(m_iFocusAct)`, exactly like a mouse click on that cell. Text-box acts focus the box and open the virtual keyboard.
5. **Highlight:** draw a 2 px outline around the focused cell, plus the existing row gradient on the focused line. Use the colour already used for the pad outline (`Outline(...)`).
6. **After LB/RB** (section switch), and after any panel opens or closes, focus the first focusable line. A highlight is then always visible.
7. **Scrolling:** moving the focus off screen scrolls the sheet so the focused line stays in view, with one line of margin. The right stick scrolls the sheet when no panel scrolls.

---

## 3. Per-area behaviour

### Spawner (DCO Settings › Force › Spawner)
- **Status panel** becomes one focus stop at the top: "Spawner actions". Left/right moves through **Spawn now / Spawn anyway (only while waiting) / Turn off / Show on map**; A runs it.
- **Force pool rows:** A opens the edit panel under the table. Inside the panel:
  - up/down moves between its rows (Count, Role, Experience, Morale, Shape, Arrival, Cost);
  - left/right changes the value;
  - **Done** and **Remove group** are the last stop.
  - B closes the panel.
- **Pool shortcuts**, as in the mockup and shown in the footer while a pool row is focused: **X** remove group, **Y** + Add group.
- **Add group picker:** LB/RB switches the "<faction> catalog" / "My presets" tabs, up/down picks, A adds, B closes.

### Group preset builder
- **Three columns:** "Add soldiers", "Group", "Preset". LB/RB moves between the columns; up/down moves inside one.
- **Add soldiers column:** A adds the soldier type. The minus on a card is the card's second cell (left/right).
- **Group column:** A selects a member. On a member, the cells are **↑ ↓ ★ ✕** (left/right moves between them, A runs one). **Y** is a shortcut for ★ make leader, **X** for ✕ remove.
- **Save:** **Menu (Start)** = Save, shown in the footer while the builder is open. The Save button is also the last stop of the right column.
- **Prompts:** A confirms and B cancels in the Discard / Replace prompts. The prompt takes the focus while it is open.
- **Name box:** a normal stop. A opens the virtual keyboard with the title "Preset name".

### Group preset library
- **Top row stops:** filters (Faction, Where) and the buttons **New / Edit / Duplicate / Rename / Delete / Copy to server / Copy to my PC**.
- **Table rows:** up/down; A = Edit.
- **Shortcuts** (footer): **Y** New, **RB** Duplicate, **X** Delete, **B** Back.
- **Rename:** opens the virtual keyboard with the title "New name" and focuses it at once.

### Top of the sheet
- **Scope spinner** (Faction / Members): **LT + LB/RB** changes it.
- **Favourite star:** **LT + A** on any setting row.
- **Section ? help and section revert:** become cells on the section header line.
- **Clear search:** a cell on the search line.

### Description line action ("use faction / group value")
- **LT + X** on a setting row that has the action. Shown in the footer only when it applies.

### Keyboard-only actions
- **View / Back button (hold)** opens a small "More" menu: Log, Export, Import, Save preset, Discard. Up/down picks, A runs, B closes.
- The keyboard letters stay as they are.

---

## 4. Footer

The footer always shows the pad prompts that work **on the focused line**. At most 6 prompts, most useful first. Examples:
- Setting row: `A Edit · ←→ Change · LT+A Star · Y Presets · LB/RB Section · B Close`
- Pool row: `A Edit · X Remove · Y Add group · LB/RB Section · B Close`
- Builder member: `A Select · Y Leader · X Remove · LB/RB Column · Start Save · B Back`

Prompt text follows the style guide: short verbs, Title Case for button names.

---

## 5. Logging (GM channel, only on change)

- `dco_settings_pad_focus line=<type> act=<kind> area=<setting|simple|spawner|pool|builder|library|top>`: only when the focused area changes, not on every step.
- `dco_settings_pad_act kind=<act kind> area=<area>`: when A runs a cell that was **not** reachable before (types 6-13, 20-28). This shows which new paths are used.
- `dco_settings_pad_more item=<log|export|import|save|discard>`: from the More menu.

---

## 6. Tests (pad only, mouse unplugged)

1. Open DCO Settings with the pad, go to Force › Spawner:
   - press **Spawn now** with the pad;
   - open a pool row, change Count and Experience, press Done;
   - add a group from My presets;
   - remove it with X.
2. Builder from the spawner:
   - add 6 soldiers, make the 3rd the leader with Y, rename with the virtual keyboard;
   - save with Start; the window returns to the pool with the new row selected.
3. Library:
   - filter Faction = US, Duplicate a built-in preset, Rename it, Copy to my PC, Delete it;
   - every confirm prompt works with A and B.
4. Scope spinner to a faction with LT+LB/RB; star a setting with LT+A; open the section help with the pad.
5. After every LB/RB a row is highlighted. A long sheet scrolls with the right stick, and moving the focus off screen scrolls it too.
6. Mouse and keyboard still work exactly as before (regression).
7. Performance: rendering does not get slower. Act ranges are recorded during the existing render pass, so no extra pass is needed.

---

## 7. Order of work (if approved)

1. Generic act cursor + highlight + focus after section switch (§2). This alone makes the spawner panel, pool and library reachable.
2. Builder columns + Start = Save + prompts (§3).
3. LT combos (star, scope, description action) + the More menu.
4. Footer prompts per line + right-stick scroll.

Rules for whoever builds it: no // comments in DCO .c files. STATICGUARD: no static tables; prompt texts go through the RadioBank T keys like the existing `set_*` strings. Every new path logs (§5).
