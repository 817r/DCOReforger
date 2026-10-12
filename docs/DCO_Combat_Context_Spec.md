# DCO Combat Context (ctx) — Spec dan status implementasi

Versi: 2026-10-10 malam. Pemilik: sesi "Jenis CQB dan long range".
Disepakati user di chat pada 2026-10-06 (istilah, 5 band, 4 env, matriks kontak). Implementasi ada di belakang saklar `ctx_on` (default 0).

## 1. Tujuan

- Istilah jarak dan lingkungan diseragamkan untuk seluruh DCO.
- Perilaku tembak, gerak, dan investigate dibedakan per jarak dan per lingkungan lewat satu tabel profil. Behavior tidak lagi mengecek jarak/env sendiri.
- Semua bisa diatur admin server lewat file config.

## 2. Range band (selector 1)

| Band | Jarak | Tune key | Config server |
|---|---|---|---|
| CONTACT | < 10 m | ctx.band_contact_m | ctx_band_contact_m (2-30) |
| CLOSE | 10 - < 50 m | ctx.band_close_m | ctx_band_close_m (20-120) |
| MEDIUM | 50 - < 200 m | ctx.band_medium_m | ctx_band_medium_m (80-400) |
| LONG | 200 - < 400 m | ctx.band_long_m | ctx_band_long_m (200-1000) |
| EXTREME | >= 400 m | - | - |

- Jarak diukur ke target saat ini; kalau target tidak terlihat, ke LKP.
- Hysteresis 2 m di setiap ambang.
- Kode: `DCO_Ctx.Band`, `DCO_Ctx.BandHyst`.

## 3. Environment (selector 2)

- Enum `DCO_EEnv`: INDOOR, TRENCH, URBAN, FOREST, OPEN. TRENCH dan FOREST adalah sub-tipe CLOSE_TERRAIN.
- Urutan cek: INDOOR (IsEntityInsideBuilding atau DCO_GetBuildingAtPos), lalu TRENCH (prefab benteng dalam 3 m), lalu URBAN (bangunan >= ctx_urban_bld_min dalam 30 m), lalu FOREST (Tree >= ctx_forest_tree_min dalam 20 m), lalu OPEN.
- Cache sel 25 m (`ctx.cell_m`). Satu query sphere STATIC per sel. Maks 4 sel baru per frame; sel yang tertunda sementara dianggap OPEN.
- Hysteresis env soldier: berganti hanya setelah 2 pembacaan berturut-turut (jeda +-1 s) sama.
- Env siapa yang dipakai:
  - env saya untuk gerak, cover, dan pace;
  - env target untuk tembak, frag, dan akurasi;
  - env LKP untuk investigate.
- Kalibrasi 2026-10-10 (Arleville, 46 sel):
  - ambang hutan 12 sudah tepat (OPEN 0-9 pohon, FOREST 16+ pohon);
  - ambang kota direkomendasikan 2 karena tepi desa dengan 1-2 rumah dan kebun masih terbaca FOREST. Ditunda sampai perbaikan pembekuan default config.

## 4. Relasi dan jenis kontak

- Peringkat cover per env: INDOOR 3, TRENCH 3, URBAN 2, FOREST 1, OPEN 0 (tune ctx.rank_*).
- Relasi (semua band):
  - HOLD: peringkat saya lebih tinggi dari musuh;
  - ASSAULT: peringkat musuh lebih tinggi dari saya;
  - EVEN: peringkat sama.
- Jenis kontak hanya di CONTACT/CLOSE. Ke-25 sel matriks terisi; total 14 jenis:
  - HOLD_INDOOR, HOLD_TRENCH, HOLD_URBAN, HOLD_FOREST, menurut env saya;
  - ASSAULT_INDOOR, ASSAULT_TRENCH, ASSAULT_URBAN, ASSAULT_FOREST, menurut env musuh;
  - CQB_INDOOR (bangunan yang sama);
  - HARD_STANDOFF (bangunan beda, INDOOR vs TRENCH, atau TRENCH vs TRENCH di CLOSE);
  - TRENCH_CLEAR (TRENCH vs TRENCH di CONTACT);
  - CQB_URBAN, FOREST_CLOSE, OPEN_MEETING.
- Di MEDIUM/LONG relasi hanya berupa modifier:
  - HOLD: geser posisi tembak, suppress x0.8;
  - ASSAULT: smoke, bound per fireteam, suppress x1.3.
- Di EXTREME relasi tidak dipakai.

Matriks (baris = env saya, kolom = env musuh; urutan kolom INDOOR, TRENCH, URBAN, FOREST, OPEN):

| saya \ musuh | INDOOR | TRENCH | URBAN | FOREST | OPEN |
|---|---|---|---|---|---|
| INDOOR | CQB_INDOOR / HARD_STANDOFF | HARD_STANDOFF | HOLD_INDOOR | HOLD_INDOOR | HOLD_INDOOR |
| TRENCH | HARD_STANDOFF | TRENCH_CLEAR / HARD_STANDOFF | HOLD_TRENCH | HOLD_TRENCH | HOLD_TRENCH |
| URBAN | ASSAULT_INDOOR | ASSAULT_TRENCH | CQB_URBAN | HOLD_URBAN | HOLD_URBAN |
| FOREST | ASSAULT_INDOOR | ASSAULT_TRENCH | ASSAULT_URBAN | FOREST_CLOSE | HOLD_FOREST |
| OPEN | ASSAULT_INDOOR | ASSAULT_TRENCH | ASSAULT_URBAN | ASSAULT_FOREST | OPEN_MEETING |

## 5. Peran layer

- Selector: band dan env.
- Modifier (tetap di jalur kode lama, supaya tidak terhitung dua kali): stress, morale, posture, personality, fatigue, malam/smoke.
- Gate (menang mutlak, tetap di sistem lama): pinned, ROE, task, role, amunisi, luka, target armor, target hanya LKP.
- Urutan evaluasi: gate, lalu selector (atau baris override), lalu modifier.

## 6. Tabel profil (`DCO_Ctx.Tables`)

Parameter: FIRE (0 single, 1 burst, 2 auto, 3 tidak menembak), AIM, ACC, SUP, FRAG, MOVE, BMIN, BMAX, CMAX, PACE (0 walk, 1 run, 2 sprint), UNIT (0 individu, 1 buddy, 2 fireteam, 3 squad), CRAWL, PEEK, C2C, CONC, SMOKE, THREAT, REACT.

MOVE: 0 tidak, 1 hanya ke cover, 2 reposisi/flank, 3 maju, 4 ruang per ruang, 5 lateral, 7 sepanjang parit, 8 geser posisi tembak, 9 ke dead angle.

Baris band:

| Band | Ringkasan isi |
|---|---|
| CONTACT | auto, frag tidak, hanya ke cover 0-3 m, sprint, threat 2.0 |
| CLOSE | burst, frag ya, bound 5-10 m buddy, run, threat 1.5 |
| MEDIUM | burst, bound 15-25 m fireteam, threat 1.0 |
| LONG | single, reposisi, bound 15-30 m, threat 0.6 |
| EXTREME | rifleman tidak menembak, tidak bergerak menyerang, threat 0.3 |

Catatan: di CONTACT/CLOSE yang dipakai selalu baris override jenis kontak. Baris band CONTACT/CLOSE hanya dipakai ThreatW.

Modifier env (hanya untuk baris band, bukan override):

| Env | Efek |
|---|---|
| INDOOR | bound dan cover x0.5, walk, peek; target INDOOR: akurasi x0.9, suppress x1.3 |
| URBAN | cover x0.7, sudut ke sudut, smoke saat menyeberang, peek |
| FOREST | bound x0.7, concealment; target FOREST: akurasi x0.9, frag tidak |
| TRENCH | bound x0.6, walk; target TRENCH: frag ya |
| OPEN | cover x1.5, crawl x1.5, bound x1.2, smoke; target OPEN: akurasi x1.1 |

## 7. Hook yang terpasang (semua `ctx_on`)

1. Tembak (Modded_UpdateAttackData):
   - fire mode dari profil;
   - frag diblok kalau FRAG 0;
   - delay tembakan pertama = (REACT + AIM) x ctx_react_scale, default 0.
2. Aim (Modded_AimErrorOffset): sigma dibagi ACC (clamp 0.5-2.0).
3. CombatMove (Modded_CombatMoveLogic_Attack):
   - gate MOVE (0/1 diam kalau sudah di cover; 2/5/8 tidak maju lurus);
   - penyerang (AGGRESIVE atau waypoint S&D) tetap maju: 0/5/8 dan 2-di-band-LONG diubah jadi maju;
   - pace dibatasi;
   - bound BMIN/BMAX;
   - giliran buddy/fireteam saat di cover;
   - titik tujuan: dead angle (ASSAULT_INDOOR), lalu sudut bangunan (C2C), lalu balik pohon (CONC);
   - mencoba smoke.
4. Threat (Modded_ThreatSystem): tekanan peluru x (1 + (THREAT band penembak - 1) x ctx_threat_k).
5. Investigate grup (Modded_InvestigateClusterActivity):
   - cara mendekat menurut band ke LKP (travel, bound, individu);
   - LKP OPEN di MEDIUM untuk grup non-pemburu: observe dari jauh, lalu Complete;
   - radius dan durasi pencarian menurut env LKP, dikali posture (AGG x1.2, DEF x0.8, EVA x0.6);
   - LKP INDOOR: handoff ke room clear lewat DCO_BuildingClear.MarkContact.
6. AttackPush (DCO_AttackPush): isCQB dari jenis kontak (CQB_INDOOR, CQB_URBAN, TRENCH_CLEAR).

## 8. Config server (`$profile:DCO/DCO_GlobalConfig.json`, kategori CombatContext)

ctx_on (0), ctx_threat_k (0.5), ctx_react_scale (0), ctx_band_contact_m (10), ctx_band_close_m (50), ctx_band_medium_m (200), ctx_band_long_m (400), ctx_urban_bld_min (3), ctx_forest_tree_min (12).

- Kode: Global/DCO_GlobalTuneCtx.c.
- Log saat load: `ctx_cfg on=... threat_k=...`.
- Tes 2026-10-10: mengubah ctx_on=1 di file terbukti menyalakan perilaku.

## 9. Logging dan counter

- Log:
  - ctx (perubahan band/env/kind), envcell, fire, ctxmove, ctxspot (corner, concealment, dead_angle), ctxreact, ctxpush;
  - ctxinvestigate (approach, search, handoff_cqb, observe, observe_end), ctx_init, ctx_cfg.
- Counter: ctx:* (update, kind_change, fire_*, frag_block, move_hold_cover, no_advance, atk_keep_advance, bound_wait_*, bound_turn_*, spot_*, c2c_miss, conc_miss, dead_angle_miss, smoke_try, threat_w, inv_*).

## 10. Tes

- Dunia tes:
  - world/Test/Perf/DCO_T_PERF_STRESS_CTX (ctx_on 1) vs DCO_T_PERF_STRESS;
  - world/Test/Individual/DCO_T_CTX_URBAN (0) vs DCO_T_CTX_URBAN_ON (1), Arleville, 360 s.
- Analyzer: scratchpad sesi e88bdb3e, ab.js (deaths, ff, open%, panic, cover%, counter ctx).
- Hasil terakhir: lihat laporan malam (Daily Routine Tracker).

## 11. Belum dikerjakan

- CTX-DEFAULT (ctx_on 1 setelah A/B dan perbaikan pembekuan config).
- CTX-HIDE (HideFromThreat memakai band).
- CTX-CQBGATE (handoff hanya ke bangunan yang punya ruangan Indoor Nav).
- CTX-CONF (tabel ke .conf).
- CTX-SUP (suppress jendela/treeline saat ASSAULT).
- CTX-MODLAYER (modifier ke lapis ctx dengan clamp 0.5-2.0).

## Status 2026-10-11 02:35

- Urban A/B 3v3 (CTX_URBAN): PASS, no regression.
- PERF_STRESS A/B 3v3: deaths 67.3 vs 74.0, died in open 59.7 vs 68.7, FF 5.0 vs 6.7, panic median 57 vs 146 s. Realism BLOCKED default ON: MG/AR rounds per unit 104.3 -> 77.8 (-25%), hits 3.48 -> 2.68, zero-round 4% -> 10%. Rifle rounds -31% with hit% 13 -> 23 (fire discipline, kept).
- ctx_on and ctx_cqb_gate stay default 0. Servers can enable with ctx_on=1 in DCO_GlobalConfig.json.
- Next A/B: PERF_STRESS_CTX with ctx.threat_k 0 vs CTX vs BASE, 3 runs each. Pass: MG/AR rounds/unit >= 104 (ideally >= 4x rifle), MG/AR zero-round <= 4%, rifle hit% kept, per-side deaths/FF not worse.
- Planned fix if threat_k 0 restores the MG: MG suppressive fireRate = maxFireRate * max(threat, floor) behind a new switch (default 0); ThreatW stays for rifles. If not, cap MG/AR move time under the MOVE gate.
- Grenades -39% with ctx on: tune FRAG for bands MEDIUM/LONG in the same A/B.

## Changes 2026-10-11 morning

- ctx.mg_floor (default 0, 0-1, config ctx_mg_floor): for WT_MACHINEGUN, threat used for suppressive rate and unseen window = max(threat, floor). Log mg_floor, counter ctx:mg_floor. A/B world DCO_T_PERF_STRESS_CTX_MGF (floor 1).
- heal.unseen_gate trigger: unseen-shooter bullet impact near the unit counts at any distance (TCCC effective fire); flyby still needs shooter < heal.unseen_m.
- CTX-SUP deliberate deviation: in relation ASSAULT the 8 s floor window and the x(0.5+SUP) rate apply even with no mate moving (base of fire stays up). HARD_STANDOFF still needs a mate moving.
- Urban A/B worlds: DCO_T_CTX_URBAN_ON_BLD2 / _SUP / _HIDE / _CQBG / _HEAL vs DCO_T_CTX_URBAN_ON.
- ctx.mg_floor is deliberately not gated on ctx.on: MG/AR volume is already low with ctx off (2.5x rifle vs doctrine 6-8x), so the floor must be testable on BASE too. Counters: ctx:mg_floor_oor / _suppress / _window (only where the floor raised the threat that was used).
