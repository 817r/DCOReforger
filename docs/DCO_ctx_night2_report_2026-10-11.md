LAPORAN SESI "Jenis CQB dan long range" (combat context / ctx), malam 2026-10-10/11

SELESAI:
1. CTX-ENVCAL, analisis 46 sel env dari 3 run CTX_URBAN di Arleville (log envcell, pusat kota 2786,1687):
   - Hasil dengan ambang sekarang (bangunan >= 3 dalam 30 m = URBAN, pohon >= 12 dalam 20 m = FOREST): URBAN 12, FOREST 28, OPEN 6.
   - Pusat kota (<= 60 m dari pusat) dengan 3-7 bangunan: URBAN, sudah benar.
   - Tepi kota dengan 1-2 bangunan dan halaman berpohon masih terbaca FOREST: 34 m (1 bld, 17 pohon), 48 m (2 bld, 27 pohon), 71 m (2 bld, 16 pohon), 92 m (2 bld, 16 pohon); sel 51 m (2 bld, 5 pohon) terbaca OPEN.
   - Utara kota tanpa bangunan dengan 16-79 pohon: FOREST, sudah benar (kebun dan hutan). Ladang dengan 0-9 pohon: OPEN, sudah benar.
   - Ambang hutan 12 sudah tepat: pemisahnya jelas, OPEN 0-9 pohon dan FOREST 16+ pohon.
   - Rekomendasi: ctx_urban_bld_min 3 -> 2, supaya tepi desa (rumah + kebun) dihitung kota (sudut ke sudut, hard cover), bukan hutan. Kompleks peternakan 2 bangunan juga akan jadi URBAN, dan itu realistis.
   - Ditunda: perubahan default ini menunggu perbaikan "config JSON membekukan default" dari Scripter 2. Config bersama masih berisi ctx_urban_bld_min 3, dan A/B malam ini sengaja tetap memakai 3 supaya datanya sebanding dengan run kemarin.

2. CTX-SPEC-TRACKER: spec ctx lengkap ditulis ke D:/Git/DCOReforger/docs/DCO_Combat_Context_Spec.md (band, env + kalibrasi, matriks, profil, hook, config, log, tes, item belum).
3. EVENTAPI #166 (Queue baru) SELESAI, compile bersih 21:28, static guard OK:
   - File baru Commander/DCO_Events.c: singleton tanpa static initializer; diakses lewat DCO_Events.Get() atau AICommander_ManagerComponent.GetInstance().GetEvents().
   - Event lewat polling tiap 1 detik dari EOnFrame manager: objective owner berubah, objective contested on/off, mode commander berubah (OFFENSIVE/DEFENSIVE/BALANCED), stok commander habis (soldiers/points).
   - Event lewat hook 1 baris (TAHAN/SIAP dengan Scripter): operasi (semua op_* lewat DCO_OpPlan.Ev: op_plan=start, op_branch, op_replan, op_phase, op_end=end), spawner (spawner_spawn, spawner_off, spawner_stop), reinforcement request, fire mission (arty_request).
   - Setiap event ditulis ke log CMD (event name=... atau log asli untuk op_*) + counter evt:<nama>.
   - Dokumen untuk mission maker: D:/Git/DCOReforger/docs/DCO_Event_API.md.
   - Realism, Scripter 2, dan Scripter sudah dikabari API siap (untuk OBJTYPES #167 dan OBJPHASE #168).
4. VICTORY #169 (Queue baru) kode selesai, compile bersih 21:31:54, static guard OK:
   - File baru GameMode/DCO_Victory.c, komponen DCO_VictoryComponent (pasang di entity game mode atau entity mana pun).
   - Kondisi: pegang >= N objective; pegang semua objective Primary; pegang objective X selama T menit berturut-turut; semua commander lawan kehabisan stok prajurit.
   - Terpenuhi -> EndGameMode vanilla (pemenang = faksi itu).
   - Log victory_init, victory_check, victory_hold, victory_end; event GetOnVictory.
   - Kondisi tipe DEFEND/DESTROY menunggu OBJTYPES #167 (Realism).
   - Insiden: sekitar 40 detik (21:31) compile rusak karena DCO_Victory.c tersimpan sebelum DCO_Events.Victory ada. Sudah diperbaiki dan BD sudah dikabari; ke depan dependensi selalu ditulis duluan.
5. STATICGUARD #173: DCO_Ctx.c punya 4 static initializer (s_aRoots, s_aFortKeys 8 string, s_fBudgetAt, s_aTrees). Semua jadi lazy init dengan nilai sama, bobot -12; DCO_Events.c dan DCO_Victory.c 0 static. Guard: DCO_AI weight=345 (baseline 345, +0) OK.
6. Dunia tes baru: world/Test/Commander/DCO_T_VICTORY (salinan FM_Commander, victory hold_count 3, grace 30 s, 120 s) dan world/Test/Individual/DCO_T_CTX_OBSERVE (defender kehilangan fireteam di area terbuka, ctx.on 1, 360 s). Keduanya diantrekan di lane 2 BD.
7. CTX-SUP (belum di-ACC user; di belakang switch ctx_sup_on default 0), compile bersih 21:37, guard OK:
   - Brief desain ke DCO Research lebih dulu. Masukan Research (MCRP 3-01A, ATP 3-21.8) dipakai: lama suppress mengikuti rekan yang sedang bound (sampai rekan berhenti + 3 s), minimal 8 s, maksimal 30 s; berhenti suppress kalau sisa magazen <= 2 (kecuali MG); HARD_STANDOFF hanya saat ada rekan bergerak; cek FF lane tetap jalan.
   - Berlaku saat relasi ASSAULT (cover musuh lebih baik) di band <= MEDIUM dan unit tidak sedang bergerak. Laju tembak suppress x(0.5 + SUP) = x1.2-1.3.
   - Log sup_window dan ctxsup; counter ctx:sup_window, ctx:sup_fire, ctx:sup_ammo_cap.
   - Fungsi baru DCO_Ctx.MateMoveAge_s (cek rekan bergerak, di-cache per grup tiap 0.5 s) dan DCO_Ctx.SpareMags.
   - File: Modded_UpdateAttackData.c (TAHAN/SIAP dengan AI Design), DCO_Ctx.c, DCO_GlobalTuneCtx.c.
8. CTX-HIDE (switch ctx_hide_on default 0), compile bersih 21:38:
   - Reaksi HideFromThreat infantri memakai batas band ctx (CLOSE 50 m, LONG 400 m) menggantikan konstanta vanilla 40/250 m bila ctx_on dan ctx_hide_on = 1. Bagian kendaraan tidak diubah.
   - Counter ctx:hide_band.
   - File: Modded_HideFromThreat.c (4 baris), DCO_Ctx.c (HideOn, HideCloseM, HideFarM).
9. CTX-CQBGATE (switch ctx_cqb_gate default 0), compile bersih 21:39:
   - Handoff room clear dari investigate hanya dilakukan kalau DCO_CQBPrecheck.Check (fungsi dari Realism) tidak mengembalikan 0 (bangunan tidak bisa direncanakan: plan_empty / no_outside_door / no_rooms). Hasil tidak diketahui (-1) tetap boleh.
   - Log ctxinvestigate phase=handoff_skip building= why=; counter ctx:inv_cqb_gate_skip.
   - File: Modded_InvestigateClusterActivity.c.
   - Rencana: dijadikan default 1 setelah blok A/B, karena hanya mencegah CQB ke jembatan/gudang.
10. Config server: key baru ctx_sup_on, ctx_sup_floor_s, ctx_sup_cap_s, ctx_sup_set_s, ctx_sup_min_mags, ctx_hide_on, ctx_cqb_gate, plus log ctx_cfg2 saat load.
11. VICTORY + OBJTYPES: kondisi baru "Typed Objectives" (default off). Setelah semua objective bertipe misi (DEFEND/HOLD_UNTIL/DESTROY dari OBJTYPES #167 milik Realism) selesai, faksi dengan kemenangan terbanyak menang. Mapping sesuai review Realism:
   - DEFEND sukses = defender; DEFEND gagal = faksi yang merebut (captured_by_X), selain itu musuh terkuat di area.
   - HOLD_UNTIL sukses = pemegang; HOLD_UNTIL gagal = tidak ada pemenang.
   - DESTROY sukses = Type Faction objective, selain itu musuh non-defender terkuat; DESTROY gagal = defender.
   - Seri = tidak ada pemenang (log victory_typed_no_winner).
   - Compile bersih 21:45:51.
12. CTX-CONF: isi tabel profil ctx bisa ditimpa admin lewat config server.
   - Key baru ctx_row_<NAMA>, kosong = pakai tabel bawaan. Ada 29 baris: CONTACT, CLOSE, MEDIUM, LONG, EXTREME, 14 jenis kontak, ENVMUL_<ENV> x5, ENVSET_<ENV> x5.
   - Isinya 18 angka dipisah spasi dengan urutan FIRE AIM ACC SUP FRAG MOVE BMIN BMAX CMAX PACE UNIT CRAWL PEEK C2C CONC SMOKE THREAT REACT.
   - Baris dengan jumlah angka salah ditolak (log WARN ctx_profile_bad) dan baris bawaan tetap dipakai.
   - Log ctx_profile_override per baris dan ctx_profile_ready rows=29 overrides=N.
   - Alasan memakai JSON server, bukan .conf: file .conf ada di dalam pak mod dan tidak bisa diubah admin server.
13. MEMBANTU DCO Scripter: overlay GM "Assignment" untuk OBJASSIGN/OBJFOCUS (tampilan saja, layer default OFF), compile bersih 21:54, guard OK.
   - Layer debug commander baru (DCO_ECDLayer.ASSIGN). Data dikirim lewat RPC debug yang sudah ada, jadi tidak ada replikasi baru.
   - Garis dari HQ commander ke tepi objective:
     - Focus: ungu tebal, label "Focus" atau "Focus · forced";
     - Blocked (objective tidak boleh diproses commander itu): merah putus-putus + x, label "Blocked";
     - Planned (main effort): putih tipis.
     - Catatan: warna diganti jadi warna commander jam 01:09 (lihat poin 17).
   - Allowed-idle tidak digambar: server tidak tahu seleksi GM, jadi aturan declutter dipakai.
   - Tombol "Assignment" ke-7 di atribut GM Commander Debug Layers. Nilai GUI 64 dipetakan ke layer 128, sehingga aman di kedua cara multi-select dan bit CARD tetap terjaga.
   - Log obj_assign_overlay on=1 player= cmd= lines=N.
   - File: Commander/Player/DCO_CmdDbg.c, Configs/Editor/AttributeLists/Edit.conf, Configs/DCO/DCO_SettingsHelp.conf (TAHAN/SIAP dengan Scripter 2).
14. HEAL-UNSEEN (temuan Research dari tes urban, doktrin TCCC care under fire; switch heal_unseen_gate default 0, belum dites), compile bersih 22:18, guard OK:
   - Self-heal ditunda kalau dalam heal_unseen_s (6 s) terakhir ada penembak tak terlihat dalam heal_unseen_m (25 m) dan waktu sampai pingsan karena pendarahan > 8 s. Soldier mencari cover dulu (DCO_RequestCover); heal kritis tetap jalan.
   - Log heal_defer (MEDIC DEBUG), counter heal:unseen_gate.
   - Penanda waktu "penembak tak terlihat dekat" ditambahkan di Modded_ThreatSystem (DCO_UnseenCloseAt).
   - Key config baru (kategori HealUnderFire): heal_unseen_gate, heal_unseen_m, heal_unseen_s.
   - Keputusan pemilik desain medic (AI Design): medic yang menolong orang lain ikut di-gate dengan switch dan jendela yang sama. Selama korban baru saja ditembaki penembak tak terlihat dekat, medic tidak berlari ke korban dan perilaku tempurnya yang jalan; pengecualian korban <= 8 s ke pingsan. Diterapkan di Modded_MedicHealBehavior.c, compile bersih 22:20; log heal_defer who=medic, counter heal:unseen_gate_medic.
   - HealWait tidak di-gate (keputusan AI Design: menunggu itu pasif dan sudah di cover).
   - A/B di dunia urban lewat antrean BD nanti.
15. MEMBANTU DCO Behavior designer: bug GenerateRandomPointInRadius min=max=0 (dari scan Auditor di PANIC30; 2 AI dapat tujuan di pojok peta). Compile bersih 22:51, guard OK.
   - Semua pemanggil DCO ternyata sudah dijaga. Dua node vanilla yang bisa menghasilkan radius 0:
     - SCR_AIGetRandomLookPosition (titik target = posisi sendiri);
     - SCR_AIGetRandomPointWithExclude (radius cari 0).
   - Guard dipasang di node itu (file baru BT/Tasks/Modded_RandomRadiusGuard.c): radius < 0.5 -> pakai titik asal / jepit ke minimum.
   - Log rng_zero_radius_guard caller=..., counter rng_zero_radius_guard:*.
   - Kalau error masih muncul dengan counter 0, pemanggilnya di C++ atau script lain dan butuh stack trace (-scrDebug).
16. MEMBANTU DCO UI/UX: contoh misi D untuk Mission Maker Guide, world/Examples/DCO_Example_D_FullMission.ent (GUID {561C95C58FDF1978}).
   - Layer: 01_Commanders (Viking US OFFENSIVE / Volga USSR DEFENSIVE, Reinforcement Stock 60), 02_Objectives (Primary + 2 secondary terhubung), 03_Groups (squad USSR di Garrison WP mode DEFEND, squad US di Do CQB WP, mortir US + kru 4 orang role MORTAR, sisanya Auto Assign), 04_Spawner_Logistics (spawner Viking stok MEN preset Counterattack wave, spawner Volga stok MEN preset Steady top-up, 1 LogisticsHub per sisi), 05_Victory (Hold All Primary + Enemy Stock Empty).
17. MEMBANTU DCO UI/UX: overlay Assignment poin 2-4 dari spec OBJASSIGN (compile bersih 01:09, guard OK, belum dilihat in-game).
   - Semua shape Assignment memakai warna commander; gaya garis tetap jadi pembeda (Focus tebal, Blocked putus-putus + x, Planned tipis).
   - Planned hanya digambar untuk commander yang sedang dipilih di commander view; tanpa pilihan hanya Focus + Blocked.
   - Label sudah 22 px dengan halo (poin 4 sudah terpenuhi tanpa perubahan).
   - Log obj_assign_overlay declutter=0/1 planned_hidden=N sel_ci=N color=commander.

DITES:
1. VICTORY1 (21:48, BD lane 2, session_214803): LULUS.
   - Log: victory_init hold_count=3 grace_s=30 -> event_api_ready objectives=3 commanders=2 -> victory_end winner=USSR reason=hold_count held=3 need=3 pada T+34.5.
   - Console: SCR_BaseGameMode state POSTGAME dan layar End Screen muncul.
   - Ini membuktikan polling EVENTAPI dan jalur VICTORY sampai EndGameMode.
2. CTX-AB urban (Arleville, run 21:26-22:15):
   - OFF 2 run: deaths 12/17, mati di area terbuka 83/88%, panic 1/1, cover 12/5%.
   - ON 3 run: deaths 11/11/17 (rata-rata 13 vs 14,5), mati di area terbuka 82/64/76% (74% vs 85,5%), panic 3/2/3, cover 8/10/11%.
   - TK0 (ctx on, threat_k 0) 1 run: deaths 13, mati di area terbuka 92%, panic 1, cover 5%. Jadi threat weight 0,5 membantu mengurangi kematian di area terbuka dan menambah cover; tetap 0,5.
   - Panic per faksi: hampir semua tambahan panic di ON ada di pihak bertahan USSR (7 dari 8: ledakan granat 3, crossfire 2), penyerang US hanya 1. Artinya serbuan dengan ctx menekan pertahanan; ini efek yang diinginkan, bukan kelemahan penyerang.
   - URB3 (OFF ketiga, session_223643): deaths 13 (US 6 / USSR 7).
   - VERDICT FINAL Research, 3 vs 3: URBAN PASS, tidak ada regresi.
     - Deaths 13,0 vs 14,0; mati di area terbuka 9,7 vs 12,0; FF 0 vs 0,3.
     - Cek ulang 23:42 (Realism): death_ctx adalah hitungan kematian yang benar; selisih +1 dari unit_summary adalah unit pingsan di akhir run, bukan mati. Angka di atas tetap berlaku.
     - Hits 128,7 vs 117,3; peluru per unit 29,4 vs 36,3 (lebih efisien).
     - Korban penyerang 5,7 vs 7,0; korban bertahan 7,3 vs 7,0.
     - Jarak bunuh lebih pendek (10,7 vs 16,3 m), efek CQB urban yang diharapkan.
     - Kasus prone-still sudah dijelaskan (bukan membeku di area terbuka).
     - ctx.threat_k tetap 0,5.
   - Default ctx_on baru boleh ON kalau PERF_STRESS (CTX1-3 vs B1-3) juga tidak regresi.
3. CTX-AB PERF_STRESS:
   - CTX1 vs B1: deaths 57 vs 76, fratricide 0 vs 10, panic 16 vs 18, cover 11 vs 9%.
   - CTX2 vs B2: deaths 76 vs 74, fratricide 9 vs 3, panic 21 vs 33.
   - CTX3 vs B3: deaths 69 vs 72, fratricide 6 vs 7, panic 11 vs 18.
   - Rata-rata 3 vs 3 (ON vs OFF): deaths 67,3 vs 74,0; fratricide 5,0 vs 6,7; mati di area terbuka 59,7 vs 68,7; panic 16,0 vs 23,0; cover 11,3 vs 9,7%. Semua metrik sama atau lebih baik. 
   - Verdict Research: TIDAK ADA REGRESI pada survival, FF, efektivitas. Korban turun di kedua sisi (US 27,3 -> 24,0; USSR 46,7 -> 43,3). Granat 18 -> 11 (-39%, kandidat tuning gate FRAG).
   - Verdict Realism: default ON BELUM. Peluru per unit 51,0 -> 40,1 (-21%, di luar noise) walau hits sama (790 vs 805); durasi panic median 146 -> 57 s lebih baik.
   - Pecahan per peran (battle.json, OFF -> ON):
     - Rifle: peluru 41,3 -> 28,5, hits 5,50 -> 6,53, akurasi 13% -> 23%. Ini disiplin tembak (lebih sedikit buang peluru, lebih banyak kena).
     - MG/AR: peluru 104,3 -> 77,8 (-25%), hits 3,48 -> 2,68, unit tanpa tembakan 4% -> 10%. Ini kehilangan suppression yang nyata, jadi default diblok.
     - GL/AT: hampir sama.
   - Dugaan penyebab MG: (a) ThreatW menurunkan threat dari penembak >200 m, padahal laju tembak supresif = maxFireRate x threat; (b) gate MOVE/bound membuat MG lebih sering pindah.
   - KEPUTUSAN: ctx_on tetap 0 malam ini (aturan no-regression). A/B berikutnya: PERF_STRESS_CTX + threat_k 0 vs CTX vs BASE, 3 run masing-masing, plus tuning gate FRAG.
   - Arahan Realism untuk fix: MG/AR adalah base of fire, volumenya mengikuti tugas, bukan threat pribadi. Kalau threat_k 0 mengembalikan MG ke ~100 peluru, fix-nya floor 1,0 (atau pengecualian ThreatW) untuk laju supresif MG/AR. Kalau penyebabnya gate MOVE, batasi waktu pindah MG/AR (bound pendek di antara burst).
   - Target A/B dari Research (doktrin ATP 3-21.8 / MCRP 3-01A): MG sustained ~100 peluru/menit, AR ~85, rifle ~12-15; MG/AR seharusnya 6-8x rifle, tapi di OFF pun baru 2,5x (ON 2,7x). Jadi volume MG sudah rendah sebelum ctx.
     - Lulus kalau: peluru MG/AR per unit >= OFF (104), idealnya >= 4x rifle; MG/AR tanpa tembakan <= 4%; akurasi rifle tetap naik; korban/FF per sisi tidak lebih buruk.
     - Rencana fix (belum dikoding malam ini karena tidak ada slot tes dan file bersama dikompilasi ulang tiap run BD): untuk MG (WT_MACHINEGUN) laju supresif = maxFireRate x max(threat, floor) lewat switch baru (default 0) di dua titik fireRate Modded_UpdateAttackData.c (baris ~167 dan ~366); ThreatW tetap untuk rifle.
4. Catatan Auditor soal run VICTORY: ada VME "Failed move" (waypoint 2 m di pusat Arleville, berasal dari logika commander FM_Commander yang ikut tersalin) dan ITEST end=aborted t=61 dengan trace grup yang hilang. Keduanya bukan bug VICTORY: misi memang berakhir di T+34 dan world disalin dari FM_Commander.
5. CTX_OBSERVE OBS1: tidak ada kontak (penempatan grup salah, 3 soldier USSR mati di spawn). World diperbaiki, OBS2 diminta dengan prioritas rendah.
7. Example D load-check (Scripter, 02:29, 95 s): LULUS. Commander Viking + Volga init, 3 objective, victory_init enabled=1 all_primary=1, kedua spawner READY, garrison_start x2, preset_workbench 1/2, setup_check_done warn=0 info=1, 0 error script/world.
6. Compile/guard: semua perubahan malam ini compile bersih (cc.sh, terakhir 01:09). Static guard 02:31: DCO_AI weight=110 (baseline 110, +0), pool cost 10 unit (baseline 10); TestAI weight=1, cost 0; hasil OK. Cek fallback tune (dco-tune-fallback-check.js): keys 468, uses 484, mismatch 0.

BELUM:
1. Default ctx_on / ctx_cqb_gate = 1: DIBLOK oleh Realism (volume MG/AR -25%). Tetap 0. Bisa dinyalakan per server lewat ctx_on=1 di DCO_GlobalConfig.json (sudah dites: config server bekerja). Script flip siap untuk setelah A/B threat_k 0 lulus.
2. CTX-MODLAYER: baru dikerjakan setelah default ON (sesuai urutan).
3. ctx_urban_bld_min 3 -> 2: ditunda, butuh A/B sendiri.
4. CTX_OBSERVE OBS2: prioritas rendah di backlog BD; perilaku observe di OPEN belum terbukti di game.
6. OBJASSIGN overlay poin 1 (garis Assignment tampil sendiri saat GM memilih/hover objektif atau commander walau layer mati): butuh server selalu kirim shape ASSIGN + filter di client; belum dikerjakan.
7. CTX-SUP, CTX-HIDE, heal unseen gate: sudah terpasang di belakang switch 0, belum ada A/B in-game.

FILE:
- Baru:
  - Scripts/Game/DCO/Global/DCO_GlobalTuneCtx.c (setting CombatContext + HealUnderFire)
  - Scripts/Game/DCO/Commander/DCO_Events.c (EVENTAPI)
  - Scripts/Game/DCO/GameMode/DCO_Victory.c (VICTORY)
  - Scripts/Game/DCO/BT/Tasks/Modded_RandomRadiusGuard.c (bantu BD)
  - world/Test/Perf/DCO_T_PERF_STRESS_CTX, world/Test/Individual/DCO_T_CTX_URBAN, _ON, _ON_TK0, DCO_T_CTX_OBSERVE, world/Test/Commander/DCO_T_VICTORY
  - world/Examples/DCO_Example_D_FullMission.ent (+ .meta, _Layers: default, 01_Commanders, 02_Objectives, 03_Groups, 04_Spawner_Logistics, 05_Victory)
  - docs/DCO_Combat_Context_Spec.md (CTX-SPEC-TRACKER), docs/DCO_Event_API.md
- Diubah:
  - Scripts/Game/DCO/Global/DCO_Ctx.c
  - Individual/Modded/Modded_CombatMoveLogic_Attack.c, Modded_UpdateAttackData.c, Modded_AimErrorOffset.c, Modded_ThreatSystem.c, Modded_HideFromThreat.c, Modded_HealBehavior.c, Modded_MedicHealBehavior.c
  - Group/Modded_InvestigateClusterActivity.c, BT/Tasks/DCO_AttackPush.c
  - Hook 1 baris: Commander/Operations/DCO_OpPlan.c, Commander/Spawner/DCO_CommanderSpawner.c, Commander/Spawner/DCO_ReinfRequest.c, Commander/Support/CommanderArtillerySupport.c
  - Commander/Player/DCO_CmdDbg.c, Configs/Editor/AttributeLists/Edit.conf, Configs/DCO/DCO_SettingsHelp.conf (overlay Assignment, bantu Scripter)
  - Commander/Player/DCO_CmdViewWorld.c (DrawDebug: warna commander + declutter Planned, bantu UI/UX, compile bersih 01:09)

FITUR/SETTING BARU:
1. Server config kategori CombatContext (DCO_GlobalConfig.json):
   - ctx_on (0): seluruh sistem konteks jarak x lingkungan.
   - ctx_threat_k (0,5), ctx_react_scale (0).
   - ctx_band_contact_m / close_m / medium_m / long_m (10/50/200/400).
   - ctx_urban_bld_min (3), ctx_forest_tree_min (12).
   - ctx_sup_on (0), ctx_sup_floor_s (8), ctx_sup_cap_s (30), ctx_sup_set_s (3), ctx_sup_min_mags (2): tembakan menekan selama rekan bergerak.
   - ctx_hide_on (0): jarak sembunyi infanteri mengikuti profil konteks.
   - ctx_cqb_gate (0): investigasi di dalam gedung dicek dulu oleh CQB precheck.
   - ctx_row_<NAMA>: override per baris tabel profil (19 baris x 18 parameter) tanpa compile.
2. Server config kategori HealUnderFire: heal_unseen_gate (0), heal_unseen_m (25), heal_unseen_s (6). Prajurit dan medic menunda heal dan cari cover kalau penembak tak terlihat ada dekat.
3. Komponen DCO_VictoryComponent (di entity mana pun): Enabled, Hold Count, Hold All Primary, Hold Objective + Hold Minutes, Enemy Stock Empty, Typed Objectives, Grace Seconds (60), Check Seconds (5). Akhir misi lewat EndGameMode (end screen).
4. Event API DCO_Events.Get(): OnObjectiveOwnerChanged, OnObjectiveContested, OnCommanderModeChanged, OnCommanderStockEmpty, OnOperation, OnSpawner, OnReinforcementRequest, OnFireMission, OnVictory, OnAnyEvent. Dokumen: docs/DCO_Event_API.md.
5. GM overlay Commander Debug layer "Assignment": semua garis berwarna commander; Focus tebal + label, Blocked putus-putus + tanda silang + label "Blocked", Planned tipis dan hanya tampil untuk commander yang sedang dipilih (declutter). Label 22 px dengan halo. Log obj_assign_overlay on/declutter.
6. Guard radius nol untuk node BT vanilla GetRandomLookPosition / GetRandomPointWithExclude (selalu aktif, hanya mencegah error).
7. Contoh misi D lengkap untuk Mission Maker Guide.

SIAPA MEMBANTU SIAPA:
- ctx membantu DCO Scripter: overlay Assignment (OBJASSIGN/OBJFOCUS), hook EVENTAPI di file Scripter (TAHAN/SIAP).
- ctx membantu DCO Behavior designer: guard radius nol GenerateRandomPointInRadius.
- ctx membantu DCO UI/UX: contoh misi D + overlay Assignment poin 2-4.
- DCO Research membantu ctx: brief CTX-SUP (doktrin suppress), verdict A/B urban dan PERF, target volume MG/AR.
- DCO Realism membantu ctx: DCO_CQBPrecheck untuk CTX-CQBGATE, mapping Typed Objectives, verdict PERF (blok default ON), perbaikan dco-abmetrics.
- DCO Scripter 2 membantu ctx: perbaikan "config JSON membekukan default" (DCO_TuneCfg selalu simpan nilai load) + dco-tune-fallback-check.js.
- DCO Behavior designer membantu ctx: semua run A/B di lane (URB, URBON, URBTK0, B1-3, CTX1-3, VICTORY1, OBS1).
- DCO AI Design membantu ctx: keputusan gate medic (heal unseen) dan TAHAN/SIAP Modded_UpdateAttackData.c.

SPEC DOC (CTX-SPEC-TRACKER): D:/Git/DCOReforger/docs/DCO_Combat_Context_Spec.md
