# DCO Features Auditor - catatan kerja malam 2026-10-10

## 1. Aturan "test selalu Full Detail" untuk grup tanpa commander (SELESAI, DITES)
- Masalah: DCO_TestCase.Begin memanggil SetLODMode(FULL_DETAIL), tetapi ReapplyCommanderLOD hanya mengubah grup milik commander. Di test tanpa commander (semua test Individual/Squad, PERF_STRESS) AI tetap LOD vanilla. Bukti: PERF_STRESS ai_dco_locked=0, ai_lodmid 78-88.
- Perbaikan (scripts/Game/DCO/Benchmark/DCO_TestCase.c): DCO_SweepTestLOD memaksa FULL_DETAIL ke semua AI saat test mulai dan tiap 5 s (AI yang spawn belakangan); grup dengan LOD override sendiri dilewati. Log baru: itest_lod ... all_ai=N, itest_lod_sweep newly_full=N.
- Atribut baru m_bVanillaLOD ("Keep the normal AI LOD..."), dipasang 1 di 16 world world/Test/Perf/*: world perf tetap mencerminkan server (log itest_lod rule=perf_vanilla).
- Tes: compile bersih; DCO_T_CTX_URBAN: itest_lod_sweep newly_full=27 lalu 1, PERF ai_lod0=28/28 ai_dco_locked=28 (sebelumnya 0).

## 2. Audit performa (SELESAI)
- Baseline valid: 10-05 session_014249, 10-07 135745 dan 231021 (PERF_STRESS, ~110 AI, tanpa commander, game exe): porsi DCO rata-rata 4,4-7,0% frame, puncak ~8,7%; spike >50 ms hampir semua engine murni.
- Run PERF 10-09 TIDAK VALID: PERF_STRESS dan PERF_STRESS_COV dijalankan berpasangan bersamaan (2 game), semua biaya per panggilan naik 1,5-1,8x.
- Regresi 10-07 sore/malam: ag_actions 20->67 us/evaluasi, ag_combat 79->118, spec_mg baru 4,6 ms/s, grp_overmatch 28->63, trace baru FireLaneNear 75/s, IndoorNav 54/s.
- Probe baru m_bActionProbe (DCO_TestCase.c, log itest_actions tiap 30 s, aktif di PERF_STRESS): antrean action rata-rata 3-4, maks 9, obsolete <=10 -> tidak ada penumpukan action; biaya datang dari evaluasi per behavior.
- Run 10-10 04:00 di bawah 3 game bersamaan: dinormalisasi dengan ag_morale (x2,05) setara 10-07 malam -> tidak ada regresi baru.
- Belum: pembanding vanilla (tanpa DCO) belum pernah diukur.

## 3. Audit config (DIKERJAKAN SESI LAIN, diverifikasi)
- Temuan: key mati cqbEngine, cqbStyle, cqbEntrySize, cqbFloorTimeout, cqbAbortPct, tacticStallPct, aiMapMarkers, behContactShareOpt, behTacticReeval; default tune tac.mett_band 0.7 vs kode 0.85. Saat dicek ulang 04:05 sesi lain sudah menghapus/menyamakan; audit ulang: mismatch 0, key mati tinggal false positive.
- Alat: scratchpad tune_audit2.js, cfg_audit.js.

## 4. Bukti efek config aiAggression lewat file JSON (SELESAI, DITES)
- Metode: 3 run PERF_STRESS bersamaan (04:11-04:22), masing-masing dengan profil privat (-profile terpisah) berisi DCO_GlobalConfig.json hanya {"aiAggression": X}. Log: "1 setting di-override dari $profile:DCO/DCO_GlobalConfig.json" dan "Aggression: aiAggression=X" di ketiga run -> jalur file config server ke game terbukti.
- Hasil (aggr 0.5 / 1 / 3, satu run per nilai, CPU penuh oleh 6 game -> indikatif):
  - kill pertama 189 / 176 / 127 s (makin agresif makin cepat kontak);
  - panic_flight 53 / 18 / 2; postur EVASIVE 32 / 18 / 18; mati saat pinned 37 / 14 / 14; pin_crawl_cover 96 / 47 / 32; stance_drop 711 / 335 / 266; perubahan state morale 295 / 220 / 149;
  - move_profile 245 / 305 / 565; advance_gate 836 / 837 / 1293; cqb_engage 231 / 226 / 414; target_engage_block 307 / 251 / 454;
  - total mati 69 / 57 / 72; fratricide 8 / 3 / 3; median jarak bunuh 12 / 12 / 14 m;
  - postur AGGRESIVE 18 / 30 / 28 (tidak naik di atas 1).
- Kesimpulan: aiAggression terbaca dari JSON dan efeknya besar dan searah: rendah = banyak panik/tiarap/pinned, tinggi = kontak lebih cepat, lebih banyak maju dan CQB, hampir tanpa panik.

## 5. Referensi config server untuk admin (SELESAI)
- File baru: D:/Git/DCOReforger/docs/DCO_Server_Config.md (di luar folder addon, tidak ikut terpaket). Dibangkitkan dari kode oleh scratchpad gen_cfgdoc.js: 415 key JSON dalam 20 kategori, kolom key, tipe, nilai shipped (dari file JSON yang dibuat otomatis oleh game), rentang, nama di GM, deskripsi (dari Edit.conf / atribut).
- Bagian "Interaction with aiAggression": takeCover/dodge/suppressionEffect dikali 1/agresivitas efektif (shipped x0,4); grenadeUsage/glUsage dikali agresivitas lalu dipotong ke 1.
- Bagian "Verified in game" berisi hasil A/B aiAggression.
- 113 key belum punya deskripsi (kebanyakan saklar tune_* untuk A/B).

## 6. Temuan setting yang tidak berefek di sebagian besar rentangnya (DILAPORKAN, belum diperbaiki)
- grenadeUsage dan glUsage: efektif = min(nilai x 2,5, 1) pada agresivitas default -> semua nilai 0,4-1,0 sama (maksimum). Shipped 0,5 dan 0,7 sudah mentok. File Individual/DCO_AIConfig.c sedang diedit sesi lain (04:18), jadi dilaporkan ke DCO Behavior designer (koordinator test + blok A/B agresivitas) dengan usulan rumus tanpa saturasi eff = 1 - (1 - nilai)^agresivitas; perubahan apa pun mengubah frekuensi granat default sehingga butuh A/B.
- Update 04:27 (dari AI Design lewat BD): grenadeUsage/glUsage sudah diberi rumus tanpa saturasi di balik saklar tune_agg_usage_map (default 0, perlu A/B sendiri). takeCover/dodge/suppressionEffect x Calm dinyatakan by design; perbaikan teks bantuan dipegang Scripter 2. Dokumen config dibangkitkan ulang 04:31 (421 key).

## 7. Aturan "int-left math" di memori proyek ternyata SALAH (DIBUKTIKAN)
- Probe engine sementara di addon TestAI (Scripts/Game/TAI/TAI_ZZIntMathProbe.c, dijalankan 2x lalu dihapus), game 1.8.0.13: (3*45.0)*DEG2RAD = 2,356; 3*0,5 = 1,5; 3*f(0,4) = 1,2; 3*50*0,01 = 1,5; fungsiInt()*f = 1,2; 3/2 ke float = 1,5; 5 < 9*0,7 benar. Campuran int/float selalu dihitung float.
- Dampak: pemindai pola itu (79 kandidat) tidak menemukan bug nyata; bug 10-05 yang dulu disalahkan ke pola ini (COMBAT_INEFFECTIVE, CostOf, DangerZTune) punya penyebab lain. Memori dikoreksi (enfusion-int-left-math + enfusion-int-math-probe), BD dan Scripter 2 diberi tahu.

## 8. Sensus error runtime dari 88 run sejak 10-09 04:00 (itest_runs/*/console.log) + 2 perbaikan
- VM Exception x2 (DEF_DEPTH 03:40): vanilla "Failed move" grup ke waypoint patroli commander di pantai (3400,y2,2182 dan 3244,y10,2196); dari dco.log: cmd_order ... why=PATROL/HOLDING. Penyebab: titik rute patroli (Commander/DCO_CommanderPatrol.c GenerateRoute) hanya dicek "di bawah air", tidak dicek navmesh.
  - PERBAIKAN: DCO_PatrolPointOnNavmesh - titik patroli infanteri di-snap ke navmesh "Soldiers" (GetReachablePoint radius 8 m) bila tile navmesh termuat; titik tanpa navmesh dilewati (counter patrol:pt_off_navmesh, log INFO patrol_pt_skip grp= pos= reason=off_navmesh). Patroli kendaraan tidak berubah. Compile bersih 04:34. Belum dites (tidak ada slot game); bukti akan datang dari run commander sesi lain (patrol_pt_skip muncul, "Failed move" hilang).
- DCO_GunnerBehavior.OnActionSelected/OnActionDeselected mengakses m_Utility.m_PerceptionComponent tanpa cek null dan tanpa guard teardown (pola vanilla SCR_AIAttackBehavior selalu cek null).
  - PERBAIKAN: cek m_Utility null di keduanya + DCO_Util.Teardown("gunner_deselect") di Deselected. Tanpa perubahan perilaku. Compile bersih 04:31.
- RandomGenerator::GenerateRandomPointInRadius minRadius 0 = maxRadius 0 (9x): engine mengembalikan titik pusat, hanya log; 5 pemanggil dengan radius bisa 0 (DCO_GetVectorFromCombatMove, Modded_AIGetRandomPoint, Modded_CalculateNextCombatMovePos, Modded_CombatMoveLogic_Attack x2). Prioritas rendah, belum diubah.
- "Trying to allocate same compartment twice!" 16x di 10 run (kursi kendaraan dijatah dua kali), belum ditelusuri.
- GUI (E) "ImageWidget Bg: Position/Size works only when min and max anchor is the same" 1932x (~22/run): sumber tidak jelas (tidak ada skrip DCO yang mengatur posisi/ukuran widget Bg); kemungkinan layout vanilla GM.
- 255x per jenis warning SCR_JsonLoadContext/SaveContext obsolete di 13 file DCO (hanya peringatan deprecation).
- UPDATE GenerateRandomPointInRadius: probe engine (TestAI, dihapus) membuktikan radius 0 mengembalikan <0,0,0> (pojok peta), BUKAN titik pusat -> setiap kejadian = tujuan gerak ke pojok peta. Pemanggil DCO: Modded_AIGetRandomPoint (sudah dijepit), Modded_CalculateNextCombatMovePos (sudah ber-guard), DCO_GetVectorFromCombatMove.RandomizeDestinationPos (kode mati, tidak dipanggil), Modded_CombatMoveLogic_Attack cohesion/regroup (radius = DCO_Util.CombatCohesionDist x 0,5; minimum praktis 0,025 m, jadi tidak tepat 0). Sumber 9 error belum terbukti (tanpa stack trace, bisa vanilla). Usulan perbaikan akar: Math.Max(d, 2.0) di DCO_Util.CombatCohesionDist (file sedang diedit sesi lain malam ini, tidak saya sentuh).
- Review perubahan sesi lain sejak snapshot 04:26 (DCO_GlobalAI.c cfg_applied dibatch 20 per baris; DCO_GlobalTune.c agg.usage_map; DCO_AIConfig.c UsageMap di balik saklar; GM/DCO_AggGM.c, GM/DCO_TuneAttributesBD.c baru): tidak ada regresi (fitur baru default OFF). Catatan kecil: buffer cfg_applied dari impor layer faksi tidak pernah di-flush.

## 9. Bug di fitur baru cfg_unused (DILAPORKAN ke BD untuk pemilik DCO_GlobalAI.c)
- cfg_unused membaca DCO_GlobalConfig.json baris per baris dan hanya mengambil key pertama per baris. File yang ditulis game sendiri hanya 1 baris -> hanya "_comment" yang dicek (dan dilaporkan sebagai unused, false positive); semua key lain tidak pernah dicek, jadi salah ketik nama key oleh admin tidak akan terdeteksi.
- Usulan: baca seluruh file, anggap token berkutip sebagai key bila karakter non-spasi sebelumnya '{' atau ',' dan sesudahnya ':'; lewati _comment.

## 10. Data "kenapa AI kurang agresif" (dikirim ke BD sebagai bukti, tanpa perubahan kode)
- Event advance_gate (DCO_Engage.ApproachAllowed) dari 3 run aiAggression; dicatat hanya saat alasan berubah per unit (jumlah transisi):
  - default 1: ok 272, under_fire 268, too_close 121, cqb_range 110, zone_urban 42, zone_cqb 15, overmatch 9; jarak minimum (min_d) 80 m x446, 50 m x239, 30 m x152;
  - 0,5: ok 280, too_close 200, under_fire 164; min_d 80 m x668;
  - 3: ok 439, under_fire 411, cqb_range 204, too_close 140; min_d 30 m x555.
- Pembacaan: (1) under_fire = stress level >= 1 menghentikan SEMUA gerak maju kecuali sedang assault/dorongan CQB/zona urban; satu tingkat stres cukup untuk membekukan maju, dan ini pemblokir utama bahkan di agresivitas 3. Pengecualian "bound terlindungi" (boleh maju di S1 bila kawan menembak beberapa detik terakhir) akan menyasar tepat masalah ini. (2) Pada nilai shipped kebanyakan prajurit berhenti mendekat di 80 m; di agresivitas 3 turun ke 30 m.

## 11. Review perubahan sesi lain sejak 04:26 (lanjutan)
- Modded_CombatMoveLogic_Attack.c: guard radius minimal 2 m di dua pemanggilan cohesion (menutup kasus <0,0,0>) - sudah dikerjakan sesi lain.
- DCO_GlobalTune.c: 21 key Vehicle AI (va.*) dibuka ke config dengan default sama dengan kode -> tanpa perubahan perilaku.
- DCO_Opp.c: patient-skip saat tim sedang overwatch (Team Bound) - kecil, aman.
- DCO_PostFrag.c (sprint maju setelah granat meledak, saklar agg.postfrag default 0): guard jarak, cooldown per grup, gerbang anggota, log lengkap - sehat.
- Pemindai aturan proyek: 0 komentar di .c, static const tetap 89 (tidak ada yang baru).

## 12. Audit runtime CQB baru (dibangun ulang 10-07) - efektivitas rendah (DILAPORKAN ke sesi "Jenis CQB dan long range")
- Semua dco.log 10-09 + 10-10: cqb_start 460, cqb_abort 397 (86%), cqb_cleared 32 (7%).
- Alasan abort: plan_empty 204, plan_no_outside_door 109, losses 67, plan_no_rooms 9, stack_timeout 8 -> sekitar 80% CQB batal di tahap rencana dalam 1 s.
- Per bangunan: Castle_01_Bridge 78 + Castle_01_Bridge_Pillar 54 (jembatan bukan bangunan berruangan), nama bangunan kosong 99, Barn_E_02 37, Shed_03 23, FarmHouse_E_1L01 14, ChurchSmall 9 (no_rooms) dst. Grup yang sama mencoba struktur yang sama sampai 6x per sesi.
- Berhasil: ChurchSmall_E_01_weathered 15, House_Village_E_1I05s 5, FarmHouse_E_1L01 3.
- Usulan: hanya mulai CQB pada bangunan yang graph Indoor Nav-nya punya ruangan yang terjangkau dari pintu luar; cache negatif per prefab; telusuri kenapa entitas bangunan kosong pada 99 abort. File DCO_CQBActivity.c sedang diedit sesi lain malam ini, jadi tidak saya ubah.
- Pemilik CQB baru = DCO Scripter (bukan sesi "Jenis CQB"); bukti sudah diteruskan ke DCO Scripter.

## 13. Tabel kesehatan fitur (event *_start vs hasil, semua dco.log 10-09 + 10-10) dan 1 perbaikan
- Sehat: search (14994 start / 14535 end), navlink (5430 ok / 72 no_move, median 1,4 s), garrison (slot tak terjangkau ditandai persistent ke cache), overmatch (293 start / 158 done / 105 abort), relocate (selisih start/end hanya karena relocate_start level DEBUG).
- Perlu perhatian: break_contact 1126 start / 279 done / 573 cancel; sbf_live 378 start tanpa event akhir (aturan "setiap fitur log" tidak terpenuhi untuk akhir SBF); rearm no_source 4381 (cari sumber amunisi terus tanpa hasil).
- BUG pickup senjata: 194 dari 243 pickup berakhir result=armed_already, median 1,4 s setelah mulai (p10 0,6 s, p90 2,2 s) -> pemicu membaca keadaan sesaat (ganti senjata / reload / isi ulang magazen ajaib) dan prajurit sempat berlari ke senjata mayat lalu batal.
  - PERBAIKAN (Individual/DCO_WeaponPickup.c): kebutuhan senjata harus bertahan >= pickup.debounce_s (default 2 s, dibaca lewat DCO_Stress.T) sebelum pickup_start; field baru DCO_PickupState.m_fNeedSince_ms; counter pickup:debounce_wait. Compile bersih 04:47. Belum dites run; indikator keberhasilan: porsi armed_already turun.
- DCO Scripter diberi tahu tentang 3 edit di file yang kemungkinan miliknya (GunnerBehavior, CommanderPatrol, WeaponPickup).
- Update dari Scripter 2: bug cfg_unused (bagian 9) sudah diperbaiki 04:45 dan diverifikasi (file 1 baris 446 key terbaca, unused=0, typo terdeteksi).
- break contact: 1126 start -> 279 done, 573 cancel, SEMUA cancel why=retasked; waktu dari mulai sampai dibatalkan p10 6 s, median 28 s, p90 89 s. Mekanisme: DCO_GroupUtility.SetTask mematikan m_bRetreating untuk tugas apa pun selain NONE, lalu DCO_BreakContact.Update membatalkan. Ini fix AUDITB2 F1 yang sudah diputuskan user 09-30 tetapi belum dikerjakan. File sedang diedit sesi lain -> diserahkan ke BD/pemilik commander dengan usulan: lewati grup dengan m_DCOBreak aktif di pemilihan grup commander.
- sbf_live: volume suppression tanpa hook akhir sehingga tidak ada log akhir (pelanggaran kecil aturan log).

## 14. Audit AUDITLOG2 (logger alasan mati) + 1 perbaikan
- Kesesuaian: 2097 killed infanteri = 2097 death_ctx (satu-satu), semua field ada, cause terisi.
- Distribusi penyebab (10-09/10-10, sebelum perbaikan): unseen_shooter 672, static_open 475, moving_open 397, grenade 186, vehicle_fire 100, fratricide 98, in_cover_outgunned 88, cqb 39, in_cover 21, exposed_window 21. Hanya 6% mati dalam cover.
- BUG: bleed=1 tidak pernah muncul; 352 kematian dmg=TRUE src=- wound_to_death_s=0, 306 di antaranya salah masuk unseen_shooter. Penyebab: pukulan terakhir engine saat mati kehabisan darah / saat pingsan adalah damage TRUE tanpa sumber, dan OnDamage menimpa data luka fatal asli.
  - PERBAIKAN (Benchmark/DCO_BenchmarkLogger.c): damage TRUE tanpa sumber setelah luka nyata = finisher (counter death:finisher_true): data luka fatal asli dipertahankan, bleed=1 bila korban sedang berdarah atau pingsan. Compile bersih 04:51. BD diberi tahu bahwa angka penyebab sebelum build ini tidak sebanding (unseen_shooter riil ~366 = 17%, "mati karena luka" ~17% sebelumnya tidak terlihat).

## 15. Battle Map Viewer (alat baru, permintaan lama user "lihat tempat commander ngeletakin pasukan + order")
- File: D:/Git/DCOReforger/tools/battle_map.html (HTML mandiri, tanpa server, tanpa library).
- Cara pakai: buka di browser, lalu pilih atau tarik file battle.json dari folder sesi log (profile/DCO/Logs/<tanggal>/<blok>/session_*/battle.json).
- Yang digambar, di grid koordinat X/Z peta:
  - lingkaran objektif, dengan pemilik awal → akhir;
  - setiap cmd_order sebagai simbol (▲ attack/flank, ■ defend/garrison, ◆ suppress/recon/transport, ✕ withdraw/rally, ● move), warna per faksi commander;
  - def_place: kotak kosong dengan garis arah hadap;
  - cmd_intel: lingkaran tipis, ukurannya mengikuti size;
  - death_ctx: tanda ✕ berwarna per penyebab mati, plus garis opsional ke posisi pembunuh (kpos).
- Kontrol:
  - slider waktu dan tombol Putar (replay, layer muncul sesuai t);
  - checkbox per layer;
  - zoom dengan roda mouse, pan dengan drag;
  - tooltip berisi semua field record saat hover.
- Panel samping: commander (mode, agresi), ringkasan commander, hasil taktik, kematian per faksi, penyebab mati.
- Diuji: skrip dijalankan di node dengan DOM tiruan memakai data DEF_DELAY session_040503 (423 order, 45 def_place, 139 intel, 16 kematian).
  - SVG terbentuk tanpa NaN/undefined;
  - ringkasan benar (US hilang 8 bunuh 7, USSR hilang 8 bunuh 6, 5 hasil DEF_DELAY);
  - filter waktu berfungsi.
  - Bug flip sumbu Z saat pan/zoom ditemukan dan sudah diperbaiki.
  - BELUM dilihat secara visual di browser, karena browser pane tidak bisa menjalankan file lokal.

## 16. Penyebab mati baru "vehicle_collision" (perbaikan logger)
- Temuan:
  - di 533 battle.json (9183 kematian) ada sekitar 25 kematian dmg=COLLISION;
  - mayoritas tertabrak kendaraan kawan (kveh=1), dan tercatat sebagai fratricide atau unseen_shooter;
  - padahal itu masalah sopir, bukan tembakan.
- Perbaikan: DCO_Battle.c Cause(): dmg COLLISION sekarang langsung dicatat "vehicle_collision", dicek sebelum fratricide. Compile bersih.
- Viewer sudah punya warna untuk penyebab ini.
- Belum diverifikasi di run baru; muncul otomatis begitu ada kematian karena tabrakan.

## 17. Loop pickup "equip_refused" (perbaikan)
- Bukti (session_044535, 04:55):
  - U15 tiga kali berturut-turut pickup_end result=equip_refused untuk Rifle_AK74_GP25 yang sama, dalam 15 detik (ms=21758, 1452, 6603);
  - ditambah 3 kali jalan ke sana.
- Penyebab: DCO_LogEnd melepas booking (Unbook) setiap kali gagal, jadi Find langsung memilih senjata yang sama lagi.
- Perbaikan (DCO_WeaponPickup.c DCO_LogEnd):
  - hasil equip_refused atau unreachable sekarang MEM-BOOK senjata itu selama cooldown, bukan melepasnya;
  - hasil gagal lainnya tetap Unbook seperti sebelumnya.
  - Compile bersih.
- FITUR/SETTING BARU: tune key pickup.fail_cooldown_s.
  - Default 120 detik. Dibaca lewat DCO_Stress.T, bisa diubah lewat tune TestCase.
  - Efek: senjata yang ditolak engine atau tak terjangkau tidak dicoba siapa pun selama 120 s.
  - Belum terdaftar di DCO_GlobalConfig.
- Belum diverifikasi di run baru. Ciri berhasil: tidak ada lagi pickup_end equip_refused berulang untuk who+weapon yang sama dalam 120 s.

## 18. Penembak MG gagal rearm karena diserobot reposisi MG (perbaikan)
- Data (dco.log 10-09 + 10-10, 55 file yang punya timeout rearm):
  - rearm_end: 55 taken, 95 timeout (90 s);
  - 83 dari 95 timeout terjadi bersamaan dengan spec_mg_position, 54 di antaranya relocate why=mg_pos_precontact;
  - 44 timeout juga diiringi navlink_stall.
- Contoh: U15 (MG) di session_032240.
  - rearm_start src=station dist=32 threat=SAFE pada T+4.8;
  - lalu spec_mg_position precontact memindahkan dia setiap ±45 s ke titik yang tidak pernah tercapai (relocate_end short/timeout, moved 0-17 m);
  - rearm_end timeout pada T+95, sumber diblokir 300 s;
  - setelah itu rearm_no_source berulang tiap ~16 s.
  - Artinya senjata paling boros amunisi justru paling sering gagal isi ulang.
- Perbaikan (Individual/DCO_Specialists.c TickMGPre):
  - evaluasi posisi MG precontact (belum kontak) tidak dimulai selama DCO_AIRearmBehavior atau DCO_AIPickupWeaponBehavior aktif;
  - counter spec:mg_precontact_yield_rearm;
  - reposisi MG saat kontak (displace, clear_lane, bof_unseen, contact_group_target) tidak diubah.
  - Compile bersih 05:00.
- FITUR/SETTING BARU: tune key spec.mg_pre_yield_rearm.
  - Default 1 (aktif), 0 = perilaku lama. Dibaca lewat DCO_Stress.T.
  - Efek: MG mengisi amunisi dulu sebelum mengambil posisi precontact.
- Belum diverifikasi di run baru. Ciri berhasil: porsi rearm_end timeout pada unit MG turun, dan counter spec:mg_precontact_yield_rearm > 0.
- Temuan sampingan (belum diperbaiki, untuk pemilik MG specialist):
  - pemilih posisi precontact berulang memilih titik yang tidak tercapai (navlink_stall, moved 0-1 m, result=timeout) dengan cover=0;
  - titik yang sama (2870,1748) dipilih lagi setelah gagal, tanpa blacklist titik gagal.

## 19. Asisten MG diperintah ulang terus dan tidak pernah bergerak (perbaikan)
- Data relocate why=mg_assistant (dco.log 10-09 + 10-10):
  - 1493 berakhir superseded_by_mg_assistant (rata-rata hanya bergerak 2,2 m, sisa 31 m ke tujuan, umur 3,1 s);
  - dibanding 1232 arrived, 578 timeout, 407 short.
  - Contoh U90 03:30:44-50: tiga order ulang dalam 5 s, moved=0.
- Penyebab:
  - UpdateAssistant (DCO_Specialists.c) mengirim relocate baru setiap tick bila IsExecutingRequest() false;
  - tujuannya = posisi MG + 3 m ke belakang, jadi bergeser setiap kali MG bergerak;
  - toleransi DCO_SameJob hanya 3 m.
  - Ini sama dengan temuan osc A/B 10-09 (169 → 7 reversal), tapi fix-nya terikat ke switch gabungan osc.fix_commit yang masih OFF, jadi di default bug-nya tetap ada.
- Perbaikan (DCO_Specialists.c UpdateAssistant):
  - bila asisten sedang menjalankan relocate mg_assistant yang belum selesai dan umurnya < spec.assist_commit_s, order baru tidak dikirim;
  - counter spec:assist_commit_hold.
  - Tidak menyentuh DCO_RelocateBehavior.c (sedang dipegang sesi lain).
  - Compile bersih 05:01.
- FITUR/SETTING BARU: tune key spec.assist_commit_s.
  - Default 6 detik (sama dengan durasi relocate asisten), 0 = perilaku lama. Dibaca lewat DCO_Stress.T.
  - Efek: asisten berkomitmen 6 s ke satu tujuan sebelum boleh diarahkan ulang.
- Belum diverifikasi di run baru. Ciri berhasil: superseded_by_mg_assistant turun drastis, porsi arrived naik.
- Juga terlihat (belum diubah): mg_displace 458 arrived vs 390 timeout; mg_pos_precontact 169 arrived dari ±500.
- Dicek dan TIDAK diperbaiki karena datanya kecil:
  - MG precontact memilih ulang titik yang baru gagal hanya 7 kali dari 173 kegagalan (514 move).
  - mg_displace "arrived" rata-rata hanya bergerak 4,5 m dengan sisa 15,7 m ke tujuan. Ini masalah "durasi dianggap sampai" yang sudah ditangani osc.fix_short (A/B pending).

## 20. Review kode sesi lain sejak snapshot 04:26 (dilakukan 05:10, diff 1279 baris, 24 file)
- DCO_Pursue.c (baru, agg.pursue default 0, OFF). DUA MASALAH LOGIKA, dikirim ke DCO AI Design, file tidak saya ubah:
  1. Deteksi "musuh menjauh" memakai jarak centroid regu SEKARANG dikurangi jarak 3 s lalu.
     - Regu yang mundur 5 m (fallback, break contact, bounding mundur) menghasilkan grew ≥ 5 walau musuh diam, lalu 2 orang dikirim maju.
     - Sebaliknya, musuh mundur sambil regu maju tidak terdeteksi.
     - Usul: simpan posisi musuh dan hitung grew = d − jarak(centroid sekarang, posisi musuh lama).
  2. DCO_Opp.Gate tidak menolak grup yang IsRetreating() (break contact). Pursue dan opp_ move bisa memerintah maju di tengah break contact.
- DCO_GroupUtility.c:
  - bc.no_retask (CanItHaveOrder menolak grup yang sedang break contact): aman. m_DCOBreak di-null-kan saat Update() selesai, ada batas keras 240 s, m_Task selalu non-null.
  - wp.attack_arrive (default 0): grup ATTACK/FLANK menyelesaikan waypoint bila separuh anggota di radius+10 m selama 8 s. Logika OK.
- DCO_CQBActivity.c cqb.prefab_negcache (default 0):
  - prefab yang 2x gagal plan (plan_empty/no_outside_door/no_rooms) dilewati.
  - Catatan: kuncinya per PREFAB, jadi satu instance rusak memblokir semua instance prefab yang sama di peta. Untuk alasan plan, itu wajar karena plan memang bergantung prefab.
- DCO_Engage.c + DCO_MoveBrain.c (mb.on default 0):
  - di stres level 1, prajurit tim penyerang boleh tetap maju bila rekan menembak dalam 3 s (covered advance);
  - ini menjawab temuan advance_gate under_fire saya. OK.
- DCO_Opp.c + Modded_CombatComponent.c + Modded_CombatMoveLogic_Attack.c: callout suara.
  - opp.bound_callout default 1, AKTIF: callout under fire saat pinned (30 s per orang), callout bound per grup (20 s);
  - laporan kontak yang sama di-dedup 8 s per regu.
  - Rate limit ada, peta dibersihkan saat > 500/1000 entri. OK.
- DCO_GlobalAI.c:
  - cfg_applied sekarang dibatch 20 key per baris;
  - parser key JSON baru (CfgJsonKeys) menangani file satu baris dan escape;
  - cfg_unused dibatasi 40 baris log. OK.
- DCO_AIConfig.c agg.usage_map (default 0): rumus 1 − (1 − raw)^aggr, konsisten di jalur reapply. OK.
- DCO_GlobalTune.c:
  - kategori baru "Vehicle AI": 19 key va.* (va.enabled default 1), semuanya benar dibaca DCO_VehicleActive.c;
  - range cr.HighDangerEnemyCount / ArtilleryEnemyThreshold diturunkan 30 → 25.
  - Key baru agg.pursue*, agg.usage_map, opp.bound_callout, wp.attack_arrive*, bc.no_retask, cqb.prefab_negcache* terdaftar.
- DCO_TuneAttributesBD.c, DCO_AggGM.c: atribut GM tune baru (tb.on, tb.fm, tgt.claim, mb.on, mb.field, gren.atk_close, agg.*), semuanya default OFF. OK.

## 21. Re-reasoning: apa yang membuat DCO terasa "WoW", dan apa yang dikerjakan malam ini
Ukuran "WoW" untuk AI milsim, dari sudut pemain:
(a) musuh terasa hidup: bergerak dengan tujuan, tidak diam di tempat terbuka, tidak maju-mundur tanpa alasan;
(b) musuh terasa pintar sebagai tim: ada base of fire, ada yang bergerak (bounding), MG jadi ancaman nyata;
(c) musuh menekan: mengejar, melempar granat, tidak gampang panik di setting agresif;
(d) musuh terdengar: callout kontak, under fire, moving;
(e) commander terlihat punya rencana.

Data malam ini terhadap ukuran itu:
- (a) LEMAH:
  - hanya 6% kematian terjadi di dalam cover; 23% mati diam di tempat terbuka, 19% mati bergerak di tempat terbuka;
  - relocate sering "selesai" karena durasi habis, bukan karena sampai (mg_displace: rata-rata bergerak 4,5 m dari 20 m);
  - asisten MG diperintah ulang 1493 kali tanpa bergerak.
  - DIKERJAKAN: komitmen asisten MG (bagian 19).
  - Sisanya menunggu A/B osc.fix_* dan defender cover selection.
- (b) SEDANG:
  - MG punya base of fire dan pemilihan posisi, tapi MG gagal isi amunisi pada 83 dari 95 timeout rearm.
  - DIKERJAKAN: MG precontact mengalah ke rearm (bagian 18).
- (c) MEMBAIK:
  - aiAggression terbukti monoton (first kill 189 → 127 s, panic flight 53 → 2);
  - granat sudah tidak jenuh (agg.usage_map);
  - advance tidak lagi diblok total saat ditembaki (mb covered advance);
  - pursue baru, tapi ada 2 bug logika yang sudah dilaporkan.
  - Semua masih OFF menunggu A/B.
- (d) BARU: callout under fire, bound, dan dedup kontak (opp.bound_callout default ON).
- (e) BISA DILIHAT: alat battle_map.html memvisualisasikan order commander, penempatan bertahan, intel, dan kematian per waktu (bagian 15).
- Fondasi data:
  - penyebab mati sekarang jujur: finisher/bleed (bagian 14) dan vehicle_collision (bagian 16);
  - pickup tidak lagi loop (bagian 17);
  - pickup debounce dan patrol off-navmesh (bagian 13 dan 8).
Prioritas berikut yang saya sarankan (bukan dikerjakan malam ini, butuh A/B oleh BD):
1. A/B osc.fix_short + osc.fix_commit, karena paling langsung memperbaiki (a).
2. Nyalakan mb.on dan agg.pursue setelah 2 bug pursue diperbaiki. A/B di DEF_DELAY dengan kunci lokasi objektif.
3. CQB abort 86%. Pemiliknya sudah menerima data; cqb.prefab_negcache siap di-A/B.

## 22. KOREKSI bagian 18: rearm MG tetap timeout walau yield aktif
- Run SEL_DAY 05:04 dan HAMMER 05:06 memuat semua perbaikan:
  - counter spec:mg_precontact_yield_rearm muncul di keduanya;
  - tetapi U15 (SEL_DAY) dan U17 (HAMMER) tetap rearm_end result=timeout pada T+95, tanpa satu pun spec_mg_position di jendela itu.
- Jejak U15:
  - rearm_start src=station dist=32 pada T+5;
  - bergerak sekitar 10 s, lalu stance PRONE oleh BT pada T+15;
  - diam sampai timeout;
  - tidak ada rearm_remove, artinya move rearm tidak pernah selesai: ada behavior lain berprioritas > 62 yang menahannya.
  - Non-MG (U36, U42) di run yang sama berhasil taken.
- Kesimpulan:
  - korelasi 83/95 dengan spec_mg_position di bagian 18 BUKAN penyebab utama;
  - yield tetap benar secara logika (MG tidak boleh pindah posisi precontact selagi isi amunisi), tapi tidak menyelesaikan timeout.
- LOG TAMBAHAN (DCO_Rearm.c DCO_LogEnd, compile bersih 05:10): rearm_end sekarang ditambah cur=<behavior yang sedang jalan> d=<jarak ke sumber> removes=<n>.
  - Run berikutnya akan menunjukkan behavior mana yang menahan MG.
  - Belum diketahui penyebabnya. Status: TERBUKA.
- Petunjuk tambahan dari unit_summary:
  - U15 di SEL_DAY/HAMMER adalah roles=mg|leader grup USSR_MG_1 (pihak bertahan), yaitu MG sekaligus LEADER tim;
  - U36 dan U42 yang berhasil bukan leader.
  - Dugaan kuat: behavior leader/defend grup (prioritas > 62) menahan leader di posisinya.
  - Diverifikasi lewat field cur= di run berikutnya.

## 23. Jarak tembak saat membunuh + koreksi bias statistik penyebab mati
- PERINGATAN DATA: statistik penyebab mati di bagian 14 dan 21 bercampur dengan world PERF_STRESS dan PERF_STRESS_COV.
  - Kedua world ini menyumbang 1010 dari 1977 kematian (10-09/10-10) dan menaruh pasukan sangat dekat.
  - Dipisah:

| penyebab | PERF (n=1010) | non-PERF (n=967) |
|---|---|---|
| unseen_shooter | 28% | 38% (masih termasuk salah klasifikasi finisher sebelum perbaikan 04:51) |
| moving_open | 20% | 18% |
| static_open | 29% | 15% |
| grenade | 9% | 9% |
| vehicle_fire | 0% | 9% |
| cqb | 0% | 4% |
| in_cover_outgunned | 5% | 3% |
| exposed_window | 0% | 2% |
| fratricide | 7% | 2% |
| in_cover | 1% | 1% |

- static_open (442 kematian total): 88% korban sedang SCR_AIAttackBehavior, task NONE/HOLDING 71%. Jarak pembunuh median 11 m (p75 23 m).
  - Artinya ini bukan "diam di tempat terbuka ditembak dari jauh", tapi baku tembak jarak sangat dekat. Penentunya kecepatan reaksi dekat (rt.*, belum dites), bukan pemilihan cover.
- Jarak bunuh infanteri:
  - non-PERF: p25 15 m, p50 36 m, p75 110 m; 38% di bawah 25 m;
  - PERF: 63-65% di bawah 25 m.
  - Per world: DEF_PERIMETER p50 152 m, DEF_AREA 66 m, DEF_DEPTH 47 m, DEF_REVERSE 38 m, DEF_DELAY 25 m.
  - Pertempuran infanteri nyata di medan campuran biasanya pecah jauh lebih jauh. Ini indikator realisme baru yang layak dipantau per run (dikirim ke DCO Realism).
- Saran ke BD: keluarkan PERF_STRESS dari statistik perilaku dan pakai hanya untuk performa.

## 24. Dokumen config server lengkap (448 key, semuanya berdeskripsi)
- D:/Git/DCOReforger/docs/DCO_Server_Config.md diregenerasi 05:2x:
  - 448 key dalam 20 kategori (sebelumnya 421);
  - termasuk key baru sesi lain malam ini: Vehicle AI va.*, agg.pursue*, agg.usage_map, opp.bound_callout, wp.attack_arrive*, bc.no_retask, cqb.prefab_negcache*.
- 120 key yang belum punya deskripsi GM sekarang punya deskripsi faktual.
  - Ditulis subagent yang menelusuri kode setiap key (file scratchpad cfg_desc_extra.json, dipakai generator gen_cfgdoc.js).
  - Satu-satu dicek acak: aimModel, ctx_on, tune_bc_no_retask, tune_agg_pursue_cd_s, morale_panic_threshold. Hasilnya cocok dengan kode.
  - Key yang hanya bekerja bila switch lain ON menyebut switch-nya di deskripsi.
- TEMUAN config:
  - aimModel dibaca dengan CfgI("aimModel", m_iAimModel, 2, 2), jadi rentangnya 2..2: nilai apa pun di-clamp ke 2 dan key ini tidak berpengaruh;
  - field-nya hanya ditulis saat migrasi config lama;
  - key ini masih dikirim di file config server. Kandidat untuk dihapus oleh pemilik GlobalAI.
- 128 key masih tanpa padanan di jendela GM. Itu normal untuk key tune server-only.

## 25. Verifikasi pasif di run SEL_DAY 05:04 dan HAMMER 05:06 (build dengan perbaikan 04:34-05:01)
- TERBUKTI, logger finisher/bleed (bagian 14): death_ctx SEL_DAY memuat bleed_no_medic dengan bleed=1 (4 kematian) dan unseen_shooter dengan bleed=1 (1 kematian).
  - Sebelum perbaikan, bleed=1 muncul 0 kali dari 2097 kematian.
- TERBUKTI, vehicle_collision (bagian 16): 1 kematian tertabrak kendaraan di SEL_DAY sekarang tercatat vehicle_collision.
- AKTIF tapi BELUM KONKLUSIF, komitmen asisten MG (bagian 19):
  - counter spec:assist_commit_hold naik di kedua run;
  - superseded:arrived = 7:4 (SEL_DAY) dan 3:9 (HAMMER);
  - baseline world yang sama sebelum perbaikan sangat bervariasi: HAMMER 17:1 dan 4:3, SEL_DAY 1:5.
  - Sampel terlalu kecil. Butuh run PERF_STRESS, yang punya banyak tim MG.
- TIDAK MENYELESAIKAN masalah, MG yield rearm (bagian 18/22): U15 dan U17 (MG + leader) tetap timeout 2x.
  - Diagnostik cur= belum ada di run ini karena di-compile 05:10, setelah run dimulai.
- Tidak teruji (kejadiannya tidak muncul):
  - pickup debounce/cooldown: 0 pickup di kedua run;
  - patrol off-navmesh: 0 patrol_pt_skip, juga 0 "Failed move".
- Tidak ada VM exception baru di HAMMER/PINCER/SEL_DAY sejak semua perubahan.

## 26. Audit statis setting GM setelah GM cleanup malam ini
- Semua 728 class atribut yang dirujuk Configs/Editor/AttributeLists/Edit.conf ada di script. Tidak ada rujukan ke class yang sudah dihapus.
- Console SEL_DAY: tidak ada "Unknown class" untuk atribut GM.
- TEMUAN (tidak diubah, base world dipakai semua test):
  - world/Test/DefaultGMTest_Layers/default.layer berisi grup $grp DCO_TestArea dengan 4 marker (TA_FM_US2, TA_FM_USSR2, dan seterusnya, label "Attacker Position", panah m_vArrowTo);
  - class DCO_TestArea tidak ada di addon mana pun (kemungkinan hilang saat revert 10-06);
  - akibatnya setiap test world memuat error "WORLD (E): Unknown class 'DCO_TestArea'" dan marker visual FM tidak pernah tampil.
  - Pilihan: hapus 4 entity itu dari base layer, atau buat ulang class marker di addon TestAI. Keputusan ada di pemilik test world / BD.

## 27. AKAR MASALAH rearm ditemukan: pemicu memakai angka mutlak, bukan relatif ke loadout (perbaikan)
- Bukti diagnostik baru (run SEL_DAY 05:26, field cur/d dari bagian 22):
  - rearm_end who=U15 (MG + leader) result=no_gain src=station d=0 ms=17899 cur=DCO_AIRearmBehavior;
  - MG SAMPAI di stasiun, tapi stasiun tidak menambah apa pun.
- Penyebab:
  - DCO_Rearm.Tick memicu rearm bila magazen cadangan ≤ 1, untuk senjata apa pun;
  - loadout MG bawaan hanya punya 1 sabuk cadangan, jadi pada T+5 (belum kontak) MG sudah dianggap kehabisan;
  - di stasiun, ResupplyMagazines hanya mengisi sampai jumlah loadout default, jadi hasilnya no_gain atau timeout;
  - stasiun lalu diblok 300 s dan rearm_no_source berulang.
- Skala (dco.log 10-09 + 10-10):
  - 124 dari 243 rearm_start (51%) terjadi di 30 detik pertama misi, sebelum ada tembakan;
  - hasil semua rearm: taken 70, timeout 99, no_gain 29, unreachable 8, move_failed 4, source_gone 3, enough 1, cancel 1.
- Perbaikan (Individual/DCO_Rearm.c):
  - DCO_RearmState mencatat jumlah cadangan awal per magazine well (m_iInitSpare/m_InitWell; direset bila ganti senjata);
  - rearm dengan cadangan 1 hanya dipicu bila 1 < jumlah awal;
  - cadangan 0 tetap selalu memicu, jadi senjata hasil pickup tetap bisa rearm;
  - counter rearm:skip_loadout_full.
  - Compile bersih 06:17.
- FITUR/SETTING BARU: tune key rearm.rel_init.
  - Default 1 (aktif), 0 = perilaku lama. Dibaca lewat DCO_Stress.T.
  - Efek: MG, AT, dan senjata lain yang loadout-nya memang sedikit magazen tidak lagi lari ke stasiun di awal misi. Rifleman (loadout ±6 magazen) tetap rearm saat tinggal ≤ 1.
- Hubungan dengan bagian 18 dan 22: yield MG precontact tetap dipertahankan, tapi akar masalah timeout MG adalah ini.
- Belum diverifikasi di run. Ciri berhasil:
  - rearm_start di T < 30 s hampir nol;
  - counter rearm:skip_loadout_full > 0;
  - porsi timeout dan no_gain turun.
- Callout baru (opp.bound_callout default ON) dicek di SEL_DAY 05:26: under_fire rata-rata 0,05/s untuk semua unit (sekitar 1 per 20 s), bound/moving sekitar 0. Tidak spam.
- Verifikasi parsial 06:38 (PERF_STRESS_ABL_CLOSEIN, build 06:17): counter rearm:skip_loadout_full 0,3/s, artinya jalur baru aktif untuk unit yang cadangannya sama dengan loadout; 0 rearm_start di 73 s pertama. Baseline PERF_STRESS sebelum perbaikan juga 0 rearm awal (tidak ada stasiun dekat), jadi pembuktian penuh butuh world TAC (SEL_DAY/HAMMER) yang punya stasiun.

## 28. Verifikasi komitmen asisten MG di run PERF_STRESS (06:37-07:30)
- Data relocate why=mg_assistant, world DCO_T_PERF_STRESS biasa:
  - sebelum perbaikan: 8 run, 1110 relocate;
  - sesudah: 2 run, 108 relocate.
- Per relocate:
  - jarak tempuh rata-rata 5,2 → 6,0 m (+15%);
  - sisa jarak ke tujuan saat selesai 32,1 → 24,4 m (−24%);
  - durasi 6,2 → 7,8 s.
- Order ulang (superseded_by_mg_assistant): 37% → 16% dari semua akhir relocate. Jumlah relocate per run sekitar 139 → 54 (churn turun).
- Semua varian PERF (AGG/OPP/ABL), 12 run sesudah vs 15 run sebelum: superseded 39% → 16%.
- Catatan jujur:
  - label "arrived" turun (29% → 10%) dan "short" naik;
  - tetapi DCO_RelocateBehavior.c diubah sesi lain 04:21 (hasil "short" baru untuk akhir yang > 4 m dari tujuan), jadi label sebelum/sesudah tidak sebanding;
  - durasi run juga berbeda (10-09 sebagian lebih panjang).
- Kesimpulan:
  - churn turun jelas, dan efisiensi per relocate membaik sedikit;
  - belum ada A/B 3-run pada build yang sama, jadi switch spec.assist_commit_s tetap tersedia (0 = lama) bila BD ingin A/B.

## 29. Review kode sesi lain 05:10-07:37 (diff 326 baris, 7 file)
- DCO_Pursue.c: KEDUA temuan saya (bagian 20) sudah diperbaiki AI Design.
  - (1) grew sekarang = d − jarak(centroid sekarang, posisi musuh SEBELUMNYA). Ditambah syarat arah gerak musuh menjauh (dot ≥ 0,7), ambang agg.pursue_grow_m (default 8 m, baru), dan 2 sampel berturut-turut sebelum mengejar.
  - (2) DCO_Opp.Gate menolak grup IsRetreating() / IsBreakingContact() (DCO_Opp.c:284). Ini berlaku juga untuk opp_ move.
- DCO_PostFrag.c: ledakan granat dicocokkan ke lemparan yang tertunda berdasarkan jarak (≤ 10 m dari target), bukan berdasarkan entity sumber. Yang basi (> 9 s) dibersihkan setiap ledakan. OK.
- DCO_TeamBound.c tb.set_frac (default 0): tim dianggap bergerak bila jumlah penggerak > alive × frac. Default 0 identik dengan perilaku lama. OK.
- DCO_GroupUtility.c wp.arrive_any (default 0):
  - penyelesaian waypoint saat sampai diperluas ke task lain, kecuali DEFEND/GARRISON/TRANSPORT/FIRE_MISSION;
  - cek tipe sekarang exact SCR_AIWaypoint, jadi subclass waypoint (termasuk waypoint DCO turunan) dikecualikan. OK.
- Key baru terdaftar: agg.pursue_grow_m, tb.set_frac, wp.arrive_any.
- Tidak ada temuan baru.

## 30. TERVERIFIKASI: perbaikan pemicu rearm (bagian 27)
- Sebelum perbaikan (10-09 + 10-10, 21 world DEF/TAC, 64 run): setiap run punya tepat 2 rearm_start dalam 190 s pertama (DEF_DELAY 9 run = 18, DEF_AREA 7 run = 14, DEF_DEPTH 7 = 14, dan seterusnya). Pola deterministik: unit MG/GL dengan cadangan 1 sesuai loadout.
- Sesudah perbaikan (run DCO_T_DEF_AREA_GAR 07:52, build 06:17, antrean Scripter):
  - 0 rearm_start dalam 190 s pertama;
  - counter rearm:skip_loadout_full aktif (0,1/s).
  - Run PERF_STRESS 06:37: jalur skip aktif 0,3/s.
- Efek samping yang diketahui: di SEL_DAY sebelum perbaikan, satu unit (U36, leader tim GL, cadangan awal 1) sempat mendapat +1 magazen dari stasiun di awal misi. Sekarang unit seperti itu baru rearm saat cadangannya 0. Ini tidak merugikan, karena rearm tetap terpicu begitu amunisi benar-benar terpakai.
- Belum terlihat: rearm yang terpicu setelah amunisi benar-benar terpakai dalam pertempuran. Butuh run lebih panjang.

## 31. Rearm di pertempuran nyata (run antrean Z 07:50-09:05) + 1 perbaikan
- 13 run (DEF_AREA_GAR x2, HAMMER_PUSH, PINCER_PUSH, SEL_DAY_OFF, SEL_NIGHT_OFF, 7 IND_*): rearm awal (< 190 s) = 0 di semua run, padahal sebelumnya selalu 2 per run DEF/TAC.
- Rearm di tengah misi tetap terpicu (1-3 per run, spare 0/1, src corpse/station). Hasil: enough 1, source_gone 1, unreachable 1, timeout 5.
- Diagnostik timeout:
  - 3x cur=SCR_AIAttackBehavior, d 7-77 m: diserobot pertempuran, wajar;
  - 1x cur=SCR_AIMoveIndividuallyBehavior, d 54 → 94 m: grup bergerak menjauh;
  - 1x BUG: U15 di stasiun d=1 m, cur=DCO_RearmMoveBehavior, removes=2, timeout 90 s.
    - Penyebab: behavior gerak rearm (prioritas induk +0,1, radius 1 m) tidak pernah selesai karena titik pusat stasiun tidak terjangkau dalam 1 m;
    - akibatnya behavior ambil amunisi tidak pernah jalan, walau prajurit sudah dalam jarak ambil 3 m.
- PERBAIKAN (DCO_Rearm.c DCO_RearmMoveBehavior):
  - gerak dianggap selesai bila prajurit dalam rearm.reach_m (default 2,5 m) dari target;
  - counter rearm:move_reach_done;
  - berlaku juga untuk pickup senjata, yang memakai behavior gerak yang sama.
  - Compile bersih 09:06. Belum diverifikasi di run.
- FITUR/SETTING BARU: tune key rearm.reach_m, default 2,5 m.

## 32. Review kode sesi lain 07:40-09:07 (diff 297 baris)
- agg.profile default diubah 0 → 1 (GM Def true) oleh AI Design.
  - Menurut catatan memori mereka, identik pada slider agresi 1, sehingga sesuai aturan no-regression di setting default.
  - Ditambah: peluang cover saat bound = raw × 0,4 × personality × slider^0,7.
- TEMUAN SISTEM CONFIG (penting untuk "config harus benar-benar bekerja"):
  - DCO_GlobalConfig.json yang dibuat otomatis menyimpan SEMUA key dengan nilai default saat file dibuat (tune_agg_profile=0);
  - nilai di file selalu menang atas default di kode;
  - jadi setiap perubahan default di kode TIDAK sampai ke server yang file JSON-nya sudah ada; agg.profile tetap OFF di server lama.
  - Usul: tulis key tune_* yang nilainya sama dengan default sebagai "tidak di-set" (jangan dibekukan), atau beri versi default di file dan migrasikan nilai yang masih sama dengan default lama.
- DCO_MoveBrain.c (mb.on): ring kedua posisi field (mb.field_ring2_m, default 0), tunggu ledakan granat regu (mb.blast_m 20 / mb.blast_s 6). Logika OK, aktif hanya bila mb.on.
- DCO_GrenadeUtility.c granat cluster: cooldown regu 30 s, tidak melempar bila kawan dalam 25 m dari titik ledak, jendela umur kontak 10-35 → 10-25 s. OK.
- DCO_TeamBound.c: callout bound oleh tim yang bergerak. OK.
- Dokumen config diregenerasi: 454 key, semuanya berdeskripsi.
