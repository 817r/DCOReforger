# DCO UI text style guide — DRAFT for decision

Status: **draft, waiting for 4 decisions from the user.** Nothing has been rewritten yet. Written by DCO UI/UX on 2026-10-10 from a count of today's text (Edit.conf names and descriptions, DCO_SettingsHelp.conf, DCO_RadioBank.conf, DCO_SettingsSimple.conf).

Scope: every text a player, Game Master or mission maker reads — GM attribute names and descriptions, DCO Settings help, Simple rows, radio and banner text, GM cards, Workbench attribute descriptions, logs meant for admins. Not in scope: log keys (`spawner_status …`), JSON keys, class names.

---

## Decision 1 — Capital letters in setting names

Today: of 736 GM setting names, about **600 are Title Case** ("Max Groups per Attack", "Spawner Faction AI Cap"), **23 sentence case** ("Enemy identification speed", "Retreat when"), the rest one word. Vanilla Reforger GM attributes use Title Case for names.

| Option | Example | For | Against |
|---|---|---|---|
| **A. Names Title Case, everything else sentence case** *(recommended)* | Name "Max Groups per Attack"; description "Most groups this commander sends against one objective." | Matches vanilla GM and 80 % of today's names; only ~23 names change. Xbox/Microsoft guidance allows title case for short labels of 1–3 words. | Long names in Title Case read heavier. |
| B. Everything sentence case | "Max groups per attack" | Easier to read, matches the help texts. | Differs from vanilla GM; ~600 names change. |

Rule either way: descriptions, help, cards, radio and banners are **sentence case**; option labels in dropdowns follow the names rule.

## Decision 2 — Spelling: US or UK English

Today it is mixed: armor 56 vs armour 3, behavior 25 vs behaviour 15, defense 39 vs defence 58, meters/maneuver US, centre/grey UK.

| Option | For | Against |
|---|---|---|
| **A. US English** *(recommended)* | Reforger's own UI uses US spelling; most DCO text already does (armor, meters, maneuver, behavior). | ~100 words change (defence, behaviour, centre, grey, recognise). |
| B. UK English | Matches part of the doctrine texts. | Differs from vanilla; more words change. |

Military terms keep their established form either way: "Support by fire", "MEDEVAC", "LZ", "FRAGO", "OPORD".

## Decision 3 — "Default X." at the end of descriptions

Today 32 descriptions end with "… Default 50." The DCO Settings window already shows the default value (dimmed value + "Default" tag and revert), and the default can change in code while the text stays — then the text lies.

| Option | For | Against |
|---|---|---|
| **A. Remove "Default X." from descriptions** *(recommended)* | No stale numbers; the window shows the real default; shorter text. | Workbench tooltips lose the number (Workbench shows the default in the field itself). |
| B. Keep it, check it by script | The number is visible in vanilla GM too. | Needs a check that every number still matches the code. |

## Decision 4 — Order of the rewrite

| Option | Order | Why |
|---|---|---|
| **A. What players see first** *(recommended)* | 1 Simple mode rows + their help → 2 radio, banners, GM cards → 3 Advanced settings names + descriptions → 4 Expert settings → 5 Workbench descriptions (many still Indonesian) | Most players only use Simple mode; mission makers read Workbench texts last. |
| B. File by file | Edit.conf, then Help, then RadioBank, then scripts | Easier to track, but player-facing fixes come late. |

---

## Rules that do not need a decision (proposed, from the UI/UX playbook)

1. **English only** in anything a player, GM or mission maker reads. Workbench attribute descriptions included (several are still Indonesian, e.g. the Commander Spawner component and DCO_GlobalAI Workbench descriptions).
2. **Say what it does, not how it is built.** "Soldiers bound forward more often" — not "scales m_fTakeCoverChance by calm".
3. **One idea per description, ≤ 2 sentences, ≤ 160 characters.** Longer explanations go into the help panel (What / Low / High / Recommended).
4. **Units in the name or the value, not in prose.** "Safe Radius" + value "300 m"; time in s or min, distance in m or km, shares in %.
5. **Numbers:** digits, a space before units ("300 m", "60 s", "85 %" — choose "85%" or "85 %" once; proposal: no space before %), ranges with an en dash ("80–150").
6. **Plurals:** never "1 groups". Use "1 group / 2 groups".
7. **Start sentences with a capital,** end with a full stop; option labels and buttons have no full stop.
8. **Name things the same everywhere.** One term per concept: "commander" (not "CO"), "squad" for AI groups in player-facing text, "objective", "spawner", "reinforcement stock".
9. **Honest labels.** If a setting only applies to newly spawned soldiers, say "Applies to newly spawned soldiers." If a level is not what the name suggests, say what it is.
10. **No gendered pronouns for soldiers or players** ("they", or rephrase).
11. **Buttons say the action:** "Spawn now", "Save preset", "Copy to server" — not "OK".
12. **Warnings say what happened and what to do:** "Viking's reinforcement stock is empty. Add stock on Viking or set Take from stock to No."
13. **Experimental settings** (added 2026-10-11, user request): never mark them in the name. The DCO Settings window shows the tag "Beta" next to the name, and the help ends with "Beta: still being tested and may change. Report anything that feels wrong." Both are generated from `m_iBeta 1` in DCO_SettingsHelp.conf, so deleting that line removes both. Headers get the tag only when every setting inside is Beta.

## After the decisions

1. Turn this draft into the final style guide (docs/DCO_UI_Text_Style_Guide.md).
2. Re-run the text inventory (the GM settings were cleaned up on 2026-10-10: 39 removed, 53 names shortened, help for all 707), then rewrite in the order of Decision 4, in batches the Scripters can review.
