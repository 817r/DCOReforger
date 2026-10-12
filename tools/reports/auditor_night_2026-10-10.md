LAPORAN MALAM DCO Features Auditor, 2026-10-10 (03:51-09:40)
Aturan dipatuhi:
- tanpa git commit/push;
- tanpa komentar di .c DCO;
- tanpa static const baru;
- file yang sedang diedit sesi lain tidak disentuh (temuan dikirim ke pemiliknya);
- run game hanya membaca log dari antrean BD/Scripter.
Semua edit .c compile bersih (cc.sh).

A. SELESAI + DITES (ada bukti dari run)

1. Test selalu Full Detail untuk grup tanpa commander.
   - File: Benchmark/DCO_TestCase.c.
   - Masalah: SetLODMode(FULL_DETAIL) hanya mengenai grup milik commander, sehingga semua test Individual/Squad/PERF berjalan di LOD vanilla.
   - Perbaikan: DCO_SweepTestLOD memaksa FULL_DETAIL ke semua AI saat test mulai dan setiap 5 s (untuk AI yang spawn belakangan).
   - Hasil tes: DCO_T_CTX_URBAN ai_lod0 = 28/28 (sebelumnya 0 terkunci).
2. Bukti config aiAggression bekerja lewat file JSON server.
   - 3 run PERF_STRESS dengan profil privat; nilai 0,5 / 1 / 3 terbaca dari JSON.
   - Kill pertama 189 / 176 / 127 s; panic_flight 53 / 18 / 2; mati saat pinned 37 / 14 / 14; advance_gate 836 / 837 / 1293; cqb_engage 231 / 226 / 414.
   - Efeknya besar dan searah. Masuk bagian "Verified in game" di dokumen config.
3. Logger penyebab mati: finisher/bleed.
   - File: Benchmark/DCO_BenchmarkLogger.c.
   - Masalah: bleed=1 muncul 0 kali dari 2097 kematian; 306 kematian salah masuk unseen_shooter.
   - Perbaikan: damage TRUE tanpa sumber setelah luka nyata diperlakukan sebagai finisher; data luka asli dipertahankan.
   - Terbukti di SEL_DAY 05:04: bleed_no_medic+bleed 4, unseen_shooter+bleed 1.
4. Penyebab mati baru vehicle_collision.
   - File: Benchmark/DCO_Battle.c.
   - Masalah: sekitar 25 kematian tertabrak kendaraan sebelumnya tercatat fratricide/unseen_shooter.
   - Terbukti di SEL_DAY 05:04 dan 05:26 (1 kematian per run).
5. Pemicu rearm relatif ke loadout (AKAR MASALAH rearm).
   - File: Individual/DCO_Rearm.c.
   - Masalah: rearm dipicu saat cadangan ≤ 1 untuk senjata apa pun. MG (loadout 1 sabuk) lari ke stasiun pada T+5, lalu no_gain (stasiun hanya mengisi sampai jumlah loadout) atau timeout 90 s.
   - Skala: 51% rearm_start terjadi di 30 s pertama; hasil 10-09/10: taken 70, timeout 99, no_gain 29.
   - Perbaikan: cadangan awal dicatat per magazine well; cadangan 1 hanya memicu rearm bila di bawah jumlah awal; cadangan 0 selalu memicu.
   - Terbukti:
     - sebelumnya 64 run dari 21 world DEF/TAC selalu punya tepat 2 rearm di 190 s pertama;
     - sesudah perbaikan, 13 run antrean Z (07:50-09:05: DEF_AREA_GAR x2, HAMMER_PUSH, PINCER_PUSH, SEL_DAY_OFF, SEL_NIGHT_OFF, 7 IND_*) punya 0;
     - rearm di tengah pertempuran tetap terpicu (1-3 per run);
     - counter rearm:skip_loadout_full aktif.
   - Ditemukan lewat LOG TAMBAHAN di rearm_end (cur=, d=, removes=), yang menunjukkan MG sampai di stasiun (d=0) tapi no_gain.
6. Review kode sesi lain (04:26-05:10, 05:10-07:37, 07:40-09:07; 1902 baris diff).
   - 2 bug logika di DCO_Pursue.c ditemukan dan dilaporkan ke AI Design, keduanya SUDAH diperbaiki:
     - regu yang mundur terbaca sebagai "musuh menjauh";
     - tidak ada cek break contact / retreat.
   - Fitur lain dicek dan aman: bc.no_retask (ada batas 240 s), wp.attack_arrive/arrive_any, cqb.prefab_negcache, mb covered advance, callout suara, agg.usage_map, parser cfg_unused baru, Vehicle AI va.*.
   - Callout suara (opp.bound_callout, default ON) dicek di run: under_fire sekitar 1 per 20 s untuk semua unit, tidak spam.
7. Tidak ada VM exception baru di HAMMER, PINCER, SEL_DAY sejak semua perubahan malam ini.

B. SELESAI, DITES SEBAGIAN / BELUM KONKLUSIF

8. Asisten MG tidak lagi diperintah ulang terus.
   - File: Individual/DCO_Specialists.c UpdateAssistant.
   - Masalah: 1493 relocate mg_assistant di-supersede oleh dirinya sendiri (rata-rata bergerak 2,2 m).
   - Perbaikan: komitmen 6 s ke satu tujuan.
   - Run PERF_STRESS sesudahnya:
     - order ulang 37% → 16% dari semua akhir relocate;
     - relocate per run sekitar 139 → 54;
     - jarak tempuh per relocate 5,2 → 6,0 m;
     - sisa jarak ke tujuan 32 → 24 m.
   - Belum konklusif: label hasil relocate diubah sesi lain 04:21, dan panjang run berbeda. Belum ada A/B 3-run di build yang sama.
9. MG precontact menunggu selama rearm/pickup.
   - File: DCO_Specialists.c TickMGPre.
   - Aktif (counter muncul), tapi BUKAN akar masalah rearm (lihat no. 5). Dipertahankan karena logikanya benar.
10. Battle Map Viewer.
    - File baru: D:/Git/DCOReforger/tools/battle_map.html. Memenuhi permintaan "lihat tempat commander menaruh pasukan, memberi order, dan lainnya".
    - Buka di browser, lalu tarik battle.json dari profile/DCO/Logs/<tanggal>/<blok>/session_*/battle.json.
    - Isi peta: order commander per faksi (simbol per jenis), penempatan bertahan + arah hadap, intel, kematian (warna per penyebab + garis ke pembunuh).
    - Kontrol: slider waktu dan replay, zoom/pan, tooltip semua field. Panel samping: commander, hasil taktik, kematian per faksi/penyebab.
    - Dites dengan node + DOM tiruan memakai data DEF_DELAY (423 order, 45 penempatan, 139 intel, 16 kematian). Bug flip sumbu Z saat pan/zoom sudah diperbaiki.
    - BELUM dilihat di browser sungguhan, karena browser pane tidak menjalankan file lokal.
11. Dokumen config server D:/Git/DCOReforger/docs/DCO_Server_Config.md.
    - 454 key, 20 kategori, SEMUA berdeskripsi. 120 deskripsi ditelusuri dari kode (5 dicek acak, cocok).
    - Kolom: nilai shipped, rentang, nama di GM, deskripsi. Ada bagian interaksi aiAggression dan hasil verifikasi in-game.

C. SELESAI, BELUM DITES (kejadiannya belum muncul di run)

12. Patroli: titik di luar navmesh.
    - File: Commander/DCO_CommanderPatrol.c.
    - Masalah: 2 VM exception "Failed move" ke titik patroli di pantai.
    - Perbaikan: titik di-snap ke navmesh, atau dilewati (log patrol_pt_skip).
    - 0 kejadian di run sesudahnya dan 0 "Failed move".
13. Pickup debounce.
    - File: Individual/DCO_WeaponPickup.c.
    - Masalah: 194 dari 243 pickup berakhir armed_already setelah median 1,4 s.
    - Perbaikan: kebutuhan senjata harus bertahan 2 s sebelum pickup dimulai.
14. Pickup cooldown setelah gagal.
    - File: DCO_WeaponPickup.c.
    - Masalah: satu prajurit gagal equip senjata yang sama 3 kali dalam 15 s.
    - Perbaikan: senjata yang gagal di-book 120 s.
15a. Gerak rearm/pickup selesai bila sudah dalam jangkauan.
    - File: Individual/DCO_Rearm.c DCO_RearmMoveBehavior.
    - Masalah (ditemukan lewat log cur=): U15 di stasiun d=1 m, cur=DCO_RearmMoveBehavior, timeout 90 s.
      - Gerak dengan radius 1 m tidak pernah selesai karena pusat stasiun tak terjangkau, jadi behavior ambil amunisi tidak pernah jalan.
    - Perbaikan: gerak dianggap selesai dalam rearm.reach_m (2,5 m).
    - Compile bersih 09:06, belum ada run.
15. GunnerBehavior null guard + guard teardown.
    - File: Individual/DCO_GunnerBehavior.c. Tanpa perubahan perilaku.

D. TEMUAN / DATA (dilaporkan, tidak diubah)

16. Audit performa:
    - porsi DCO 4,4-7% frame, puncak ~8,7%; spike engine murni;
    - run PERF 10-09 tidak valid (2 game bersamaan);
    - regresi 10-07 di ag_actions (20 → 67 us) dan spec_mg;
    - probe action queue menunjukkan tidak ada penumpukan;
    - pembanding vanilla belum pernah diukur.
17. Aturan memori "int-left math" SALAH: probe engine membuktikan campuran int/float selalu dihitung float. Memori dikoreksi.
18. GenerateRandomPointInRadius(0, 0, c) mengembalikan <0,0,0> (pojok peta), bukan titik pusat. Pemanggil cohesion sudah diberi radius minimal 2 m oleh sesi lain.
19. CQB baru:
    - 460 start, 86% abort (sekitar 80% di tahap rencana), hanya 7% clear;
    - jembatan dan bangunan tanpa nama mendominasi;
    - data dikirim ke DCO Scripter, yang lalu membuat cqb.prefab_negcache (default 0, perlu A/B).
20. Break contact: 573 dari 573 cancel karena retask commander → dikerjakan Scripter (bc.no_retask, default 0).
21. Advance gate: stres level 1 menghentikan semua gerak maju; jarak berhenti 80 m di nilai default → dikerjakan AI Design (mb covered advance, default 0).
22. Statistik penyebab mati harus memisahkan world PERF_STRESS (separuh dari semua kematian, pasukan sangat dekat).
    - Non-PERF: median jarak bunuh 36 m, 38% di bawah 25 m.
    - static_open ternyata baku tembak jarak sangat dekat (median 11 m, 88% sedang attack), bukan ditembak dari jauh.
    - Dikirim ke DCO Realism sebagai metrik realisme baru.
23. aimModel di config server tidak berpengaruh (rentang 2..2). Kandidat dihapus.
24. Base world test merujuk class DCO_TestArea yang sudah tidak ada: error di setiap test world, marker "Attacker Position" tidak tampil. Dilaporkan ke BD.
24b. SISTEM CONFIG: perubahan default di kode tidak sampai ke server lama.
    - DCO_GlobalConfig.json yang dibuat otomatis membekukan nilai default saat file dibuat, dan nilai di file selalu menang atas default di kode.
    - Contoh malam ini: agg.profile 0 → 1 oleh AI Design. Server yang sudah punya file tetap memakai 0.
    - Usul: key tune_* yang nilainya sama dengan default jangan dibekukan, atau beri versi default di file dan migrasikan.
25. sbf_live tidak punya log akhir (pelanggaran kecil aturan "setiap fitur log"). File sedang dipegang sesi lain.
26. Hal kecil, belum ditelusuri:
    - "Trying to allocate same compartment twice" 16x;
    - GUI "ImageWidget Bg" ~22 per run;
    - warning SCR_JsonLoadContext obsolete di 13 file.

E. TIDAK SELESAI

27. 6 arm A/B config (superiorRatio 1/4, overmatchAssault false, takeCoverChance 0,2, suppressionEffect 2/0,2) belum dijalankan, karena lane BD penuh sampai 09:30. Profil privat sudah siap di scratchpad.
28. Unit di world FM Commander (DCO_T_FM_Commander) masih perlu dirapikan tingginya di Workbench, dan test FM menunggu fix + logger.
29. Pembanding performa vanilla belum diukur.

F. FILE YANG DIUBAH MALAM INI (oleh saya)
- DCO_AI/scripts/Game/DCO/Benchmark/DCO_TestCase.c (m_bVanillaLOD, m_bActionProbe, DCO_SweepTestLOD, DCO_ProbeActions)
- DCO_AI/world/Test/Perf/*/default.layer, 16 world (m_bVanillaLOD 1; PERF_STRESS juga m_bActionProbe 1)
- DCO_AI/scripts/Game/DCO/Individual/DCO_GunnerBehavior.c
- DCO_AI/scripts/Game/DCO/Commander/DCO_CommanderPatrol.c
- DCO_AI/scripts/Game/DCO/Individual/DCO_WeaponPickup.c
- DCO_AI/scripts/Game/DCO/Benchmark/DCO_BenchmarkLogger.c
- DCO_AI/scripts/Game/DCO/Benchmark/DCO_Battle.c
- DCO_AI/scripts/Game/DCO/Individual/DCO_Specialists.c
- DCO_AI/scripts/Game/DCO/Individual/DCO_Rearm.c
- D:/Git/DCOReforger/tools/battle_map.html (baru)
- D:/Git/DCOReforger/docs/DCO_Server_Config.md (dibangkitkan)
- TestAI addon: probe sementara TAI_ZZIntMathProbe.c, dibuat lalu DIHAPUS.

G. FITUR/SETTING BARU
Semua tune key di bawah dibaca lewat DCO_Stress.T, jadi bisa diatur per test lewat m_aTune TestCase atau tune store GM. Belum terdaftar di DCO_GlobalConfig.json.
1. rearm.rel_init
   - Default 1, range 0/1.
   - Cara kerja: cadangan magazen awal dicatat per jenis magazen; cadangan 1 hanya memicu rearm bila di bawah jumlah awal, cadangan 0 selalu memicu.
   - Contoh efek: MG dengan 1 sabuk cadangan tidak lagi lari ke stasiun di awal misi; rifleman dengan 6 magazen tetap rearm saat tinggal 1.
   - 0 = perilaku lama.
2. pickup.debounce_s
   - Default 2,0 s.
   - Cara kerja: prajurit harus terus "tanpa senjata layak" selama 2 s sebelum mulai mengambil senjata.
   - Contoh efek: tidak lagi berlari ke senjata mayat hanya karena sesaat ganti senjata atau reload.
3. pickup.fail_cooldown_s
   - Default 120 s.
   - Cara kerja: senjata yang ditolak engine (equip_refused) atau tak terjangkau (unreachable) tidak dicoba siapa pun selama 120 s.
4. spec.mg_pre_yield_rearm
   - Default 1, range 0/1.
   - Cara kerja: evaluasi posisi MG sebelum kontak menunggu selama prajurit sedang rearm atau mengambil senjata.
5. spec.assist_commit_s
   - Default 6 s.
   - Cara kerja: asisten MG tidak diberi tujuan baru selama relocate-nya yang sekarang belum berumur 6 s.
   - Contoh efek: asisten benar-benar bergerak mengikuti MG, tidak diam karena order baru setiap tick.
   - 0 = perilaku lama.
6. Atribut TestCase m_bVanillaLOD (default 0): test perf memakai LOD normal seperti server; dipasang 1 di 16 world perf.
7. Atribut TestCase m_bActionProbe (default 0): setiap 30 s log antrean action semua AI (itest_actions).
7b. rearm.reach_m
   - Default 2,5 m.
   - Cara kerja: gerak menuju stasiun/mayat/senjata dianggap selesai bila prajurit sudah dalam 2,5 m dari target, lalu langsung mengambil.
   - Contoh efek: prajurit tidak lagi berdiri 90 s di samping kotak amunisi tanpa mengambil.
8. Field log baru:
   - rearm_end cur=, d=, removes= (behavior yang sedang jalan, jarak ke sumber, jumlah move ulang);
   - death_ctx cause=vehicle_collision;
   - counter rearm:move_reach_done, death:finisher_true, rearm:skip_loadout_full, spec:assist_commit_hold, spec:mg_precontact_yield_rearm, pickup:debounce_wait, patrol:pt_off_navmesh.
9. Alat: tools/battle_map.html dan docs/DCO_Server_Config.md (lihat no. 10-11).

H. RE-REASONING: apa yang membuat DCO "WoW", dan arah berikutnya
Ukuran WoW dari sisi pemain:
(a) musuh terasa hidup;
(b) musuh pintar sebagai tim (base of fire + gerak; MG jadi ancaman nyata);
(c) musuh menekan (mengejar, granat, tidak gampang panik saat agresif);
(d) musuh terdengar (callout);
(e) commander terlihat punya rencana.
Posisi sekarang:
- (a) masih LEMAH: hanya ~4% kematian di dalam cover; relocate sering "selesai" karena durasi habis (mg_displace rata-rata bergerak 4,5 m dari 20 m).
- (b) MEMBAIK malam ini: MG tidak lagi buang 90 s di stasiun di awal misi, dan asisten MG mengikuti.
- (c) aiAggression terbukti monoton; paket agresi (usage_map, mb covered advance, pursue, postfrag, team bound) siap, tapi masih OFF menunggu A/B.
- (d) callout baru default ON, tidak spam.
- (e) battle_map.html membuat rencana commander bisa dilihat.
Saran urutan berikutnya (butuh A/B oleh BD):
1. osc.fix_short + osc.fix_commit, untuk (a).
2. Nyalakan mb.on dan agg.pursue sesudah A/B, untuk (c).
3. A/B cqb.prefab_negcache, karena 86% CQB batal.
4. Jalankan 6 arm A/B config yang sudah disiapkan.
5. Pakai jarak bunuh (p50, %<25 m) di world non-PERF sebagai metrik realisme per run.
