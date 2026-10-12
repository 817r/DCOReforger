LAPORAN DCO REALISM — shift malam 2 (2026-10-10 21:20 → 2026-10-11 02:40)

SELESAI
1. B3/REAL-07 CQB pre-check Indoor Nav (C4, bersama Scripter + ctx)
   - Masalah: grup memilih gedung untuk CQB, lalu baru saat mulai ketahuan gedung itu tidak bisa direncanakan (graph Indoor Nav tanpa pintu luar / kosong / tanpa ruangan). Hasilnya satu percobaan gagal per gedung per faksi, lalu jeda 600 detik (perbaikan malam 1).
   - Solusi: fungsi pre-check di file baru Group/DCO_CQBPrecheck.c:
     a) Check(building, owner, out why) memakai ulang DCO_INavCQBPlan.Make tanpa mengubah file Nav. Hasil: 1 = bisa, 0 = tidak bisa (why = empty / no_outside_door / no_rooms), -1 = belum diketahui (budget / no_graph).
     b) Plannable(building, owner): false hanya kalau hasil 0; yang belum diketahui tetap diizinkan.
     c) Hasil di-cache per gedung selama cqb.precheck_ttl_s (600 detik).
     d) Maksimal 1 pembangunan graph per frame, untuk mencegah lonjakan beban.
   - Hook (di belakang switch cqb.precheck, default 0):
     a) DCO_CQBActivity.Create: guard setelah negcache Scripter, sehingga berlaku untuk semua pemanggil.
     b) DCO_BuildingClear.FindContactBuilding: gedung yang tidak bisa dilewati, sehingga pemilih memilih gedung lain.
   - Log:
     a) cqb_precheck building= pos= result= why= built_now= ttl_s= (sekali per gedung per TTL);
     b) cqb_skip ... reason=precheck_<why> (maksimal sekali per 60 detik per gedung);
     c) counter cqb_precheck:ok/bad/unknown/cached/budget, cqb_skip:precheck, cqb_skip:contact_precheck.
   - Fungsi diserahkan ke ctx untuk CTX-CQBGATE; ctx akan memasangnya setelah blok A/B ctx.
   - TAHAN/SIAP dengan Scripter selesai 21:28. Shadow compile bersih. static-guard DCO_AI +0.
2. STATICGUARD #173: file saya diperiksa.
   - Satu static initializer baru dari saya (DCO_CQBPrecheck.c s_fBuildAt_ms = -1, dibuat 21:27) dihapus 21:30, sebelum blok BD mulai.
   - File lain (DCO_GlobalTuneRealism.c, DCO_Engage.c, hook CQB) +0.
   - Dilaporkan ke Scripter 2.
3. REAL-08 navlink_stall (TAHAN/SIAP dengan BD, pemilik DCO_NavGuard.c)
   - Data 2 run BASE (363 baris):
     a) 362 baris punya blocked=0 dan ladder=0, artinya bukan macet di navlink;
     b) 35% terjadi di behavior yang memang diam (attack/suppress/observe/medic/idle);
     c) sisanya jeda saat investigate atau bounding (85% VIGILANT).
   - Perbaikan (hanya log): navlink_stall ditulis INFO hanya kalau blocked atau ladder; selain itu DEBUG. Counter tidak berubah.
   - Shadow compile bersih 21:32.
4. REAL-09 alat metrik A/B bersama: D:/Git/DCOReforger/tools/dco-abmetrics.js
   - Metrik standar per arm: tewas, mati di terbuka, FF + % ledakan, tiarap diam, mati saat panik, jumlah dan durasi panik, peluru/unit, hits, mati pertama, setengah korban, cover%, jarak bunuh p50, % di bawah 25 m, korban per pihak.
   - Diumumkan ke Auditor dan Research (analis A/B).
5. World tes baru (siap; menunggu lane):
   - DCO_T_DEF_AREA_CLOSEIN, DCO_T_DEF_AREA_RETFIRE (REAL-02/04)
   - DCO_T_IND_OPENCOVER, DCO_T_IND_OPENCOVER_ON (REAL-05)
   - DCO_T_PERF_STRESS_PRECHECK (tes B3)
6. REAL-04: agg.retfire dan tm.retfire ternyata sudah satu fungsi (DCO_ReturnFireUnseen; chance = max dari keduanya). Tidak ada yang perlu disatukan. Sudah dikabarkan ke AI Design.
7. OBJTYPES #167 (Queue baru, setelah EVENTAPI siap 21:28)
   - Tipe objective baru di CMD_EObjectiveType: DEFEND (3) dan HOLD_UNTIL (4), ditambahkan setelah RECON. Nilai CAPTURE/DESTROY/RECON tetap, jadi world lama tidak berubah.
   - DESTROY sekarang bisa punya daftar entity target.
   - File baru Commander/Objective/DCO_ObjTypes.c:
     a) modded CMD_AICommanderObjectiveComponent dengan atribut kategori "Objective Type";
     b) tracker 1 detik dari tick manager, hanya di server;
     c) invoker bertipe DCO_ObjTypes.Get().GetOnObjectiveResult() (objective, done);
     d) event obj_type_done / obj_type_fail ke OnAnyEvent EVENTAPI.
   - DEFEND:
     a) defender = faksi di atribut, atau pemilik di awal misi;
     b) selesai kalau objective dipegang sampai Time (detik dari awal);
     c) gagal kalau direbut faksi musuh (why=captured_by_X);
     d) opsional gagal lebih awal kalau defender di area kurang dari Defend Ratio × penyerang selama 60 detik;
     e) selama bertahan objective di-set Retain (tidak mundur karena doctrine), lalu dikembalikan setelah selesai.
   - HOLD_UNTIL:
     a) faksi pemegang (atau siapa saja) harus memiliki objective tanpa putus selama Time detik;
     b) putus kepemilikan me-reset timer (log hold_break);
     c) opsional deadline.
   - DESTROY dengan Destroy Targets (nama entity):
     a) selesai kalau semua target hancur (DamageManager DESTROYED) atau terhapus;
     b) nama yang tidak ditemukan diberi WARN obj_type_target_missing;
     c) opsional deadline.
   - Realisme: DEFEND/HOLD_UNTIL dianggap seperti CAPTURE oleh commander: bobot taktik METT sama, bonus grup sama (case di GetRequiredGroupCount ditambahkan Scripter bersama edit OBJASSIGN-nya), dan cek RECON tidak berubah.
   - Log: obj_type_start / obj_type_defender / obj_type_hold_start / obj_type_hold_break / obj_type_progress (tiap 60 detik) / obj_type_target / obj_type_target_missing / obj_type_done / obj_type_fail; counter objtype:*.
   - TAHAN/SIAP: CMD_Utility.c, DCO_MettTable.c, CommanderDebug.c dengan Scripter. AICommanderObjective.c dipegang Scripter (OBJASSIGN); case-nya ia tambahkan sendiri.
   - Compile bersih 21:40; static-guard OK.
   - Diserahkan ke ctx (VICTORY #169) dan Scripter 2 (GMOBJFIELDS #165: setter Time/Faction/Deadline siap).
   - World tes fungsional DCO_T_DEF_AREA_OBJTYPE (700 detik, 3 objective: DEFEND real, HOLD_UNTIL 120 detik, DESTROY target palsu + deadline 180 detik; assert start>=3, done>=1, fail>=1) menunggu lane BD.
8. dco-abmetrics.js: 3 tambahan dari Auditor:
   a) vehicle_collision dikecualikan dari jarak bunuh;
   b) kolom jarak bunuh p50 per faksi korban;
   c) peringatan untuk run sebelum build 10-10 04:51.
9. Membantu Scripter (OBJASSIGN #163, aturan saling bantu)
   - Memetakan semua tempat commander memilih objective sebagai target:
     a) patrol (DCO_CommanderPatrol.c:219) hanya cek jarak aman, bukan memilih target. Tidak difilter, karena kalau difilter patroli malah mendekati objective terlarang.
     b) recon_front: filter dipasang sendiri di DCO_CommanderFrontline.c PickReconSegment (baris 399). Geometri frontline tetap utuh.
     c) Untuk file Scripter, daftar baris dikirim dan ia pasang sendiri: guard di TrySendRecon (_13, path recon) dan DCO_ReinforceAssault (_10, path reinforce), satu titik untuk semua pemanggil (OpPlan ×2, _13, TacticTest).
     d) QRF bertahan di DCO_DefenseTactics tidak difilter (bela diri lokal, sesuai Research).
     e) ThreatResponse dan Spawner (file Scripter 2): daftar baris diteruskan Scripter ke Scripter 2.
   - Compile bersih 21:48; static-guard OK.
10. Membantu ctx (VICTORY #169): review pemetaan pemenang dari objective bertipe.
   - DEFEND gagal: pemenang = perebut.
   - HOLD_UNTIL lewat deadline: tidak ada pemenang.
   - DESTROY: pemenang = Type Faction atau faksi non-defender terkuat; gagal = defender.
   - Saya menambah defender untuk DESTROY (pemilik di awal) di DCO_ObjTypes.c. ctx sudah menerapkan semuanya (compile 21:45).
11. Audit config arah sebaliknya (kode → config server): 62 key tune dibaca kode tetapi TIDAK ada di DCO_GlobalConfig.json, sehingga admin tidak bisa mengaturnya.
   - Rinciannya:
     a) tm.* 41: aturan gerak di tempat terbuka, rush ke cover, panik, suppression;
     b) bc.* 11: break contact;
     c) air.* 3;
     d) tactic.consol_* 2;
     e) cm.dir_decode 1;
     f) pc.safe/vigilant/alerted 3;
     g) itest.free_select 1.
   - Dipastikan default setiap key sama di semua pemanggil (DCO_Stress.TA hanya mengalikan nilai dasar dengan agresi).
   - DCO_TuneCfg hanya mengubah nilai kalau isi JSON berbeda dari default, jadi mendaftarkan key dengan default yang sama TIDAK mengubah perilaku apa pun.
   - 58 key didaftarkan di DCO_GlobalTuneRealism.c, kategori "Realism Movement", "Realism Suppression", "Break Contact", "Air And Consolidation".
   - Yang sengaja tidak didaftarkan:
     a) pc.safe/vigilant/alerted: keputusan user 10-05, hanya identify_mul yang dibuka;
     b) itest.free_select: khusus tes.
   - Audit ulang: tinggal 4 key, semuanya sengaja.
   - Compile bersih 22:15; static-guard OK.
   - Belum ada tes boot JSON khusus untuk 58 key ini (lane penuh). Mekanismenya sama dengan 30 key Realism yang sudah 2× lulus tes baca JSON.
12. REAL-01 PANIC30: run ke-3 (malam ini, session_220058) dibanding BASE malam ini (session_212507):
   - korban 63 vs 76
   - FF 2 vs 10
   - mati saat panik 8 vs 15
   - lama panik 30 vs 214 detik
   - peluru/unit 56 vs 49
   - Kumulatif n=3 vs BASE n=4: korban 60.7 vs 70.5, FF 5.0 vs 5.3 (tidak lagi lebih buruk), mati saat panik 8.3 vs 13.3.
   - Catatan: jumlah episode panik naik (39 vs 18) karena panik pendek bisa berulang, tetapi total waktu panik turun sekitar 3×.
   - (Data lanjutan dan keputusan final: lihat poin 13 dan DITES 2.)
   - Migrasi config JSON siap dari Scripter 2: morale_panic_max_s sudah ada di tabel legacy (default lama 0), jadi server lama ikut pindah ke 30 secara otomatis.
13. REAL-01 PANIC30: KEPUTUSAN = jadi default 30 detik.
   - Data kumulatif PANIC30 n=4 vs BASE n=5 (PERF_STRESS 10-10, pagi + malam):
     a) korban 65 vs 71.2 (−9%)
     b) mati di tempat terbuka 56.8 vs 63.8
     c) mati saat panik 8.8 vs 15.2 (−42%)
     d) lama panik 30 vs 98 detik
     e) FF 5.0 vs 4.8 (sama)
     f) peluru/unit 49.7 vs 48.1
     g) jarak bunuh p50 16.3 vs 14.4 m
   - Tidak ada metrik utama yang memburuk. Hits −7% sejalan dengan korban yang lebih sedikit.
   - Default BELUM diubah: perubahan default perilaku diblok penjaga izin sesi (lihat BELUM 1). Ini jadi keputusan user.
   - CARA MENGUBAH DEFAULT (wajib 3 tempat):
     a) DCO_GlobalTuneRealism.c baris 8: default 0.0 → 30.0;
     b) baris 108: fallback log realism_cfg 0.0 → 30.0;
     c) DCO_MoraleSystem.c baris 285: DCO_Stress.T(..., "tm.panic_max_s", 0.0) → 30.0.
   - Temuan penting: DCO_TuneCfg hanya menyimpan nilai kalau isi JSON berbeda dari default. Kalau sama, kode membaca fallback di pemanggil. Jadi mengubah default di DCO_TuneCfg saja TIDAK berefek.
   - Sudah dikabarkan ke Scripter 2 (migrasi JSON) dan dicatat di memory sebagai aturan untuk setiap pengubahan default.
   - Migrasi JSON server lama: morale_panic_max_s=0 di tabel legacy (Scripter 2), sehingga server lama ikut pindah ke 30.
14. Audit konsistensi default: untuk 331 key terdaftar, default di DCO_TuneCfg dibandingkan dengan semua fallback di pemanggil. Hasilnya 0 yang tidak cocok, artinya default yang tertulis di config memang yang berlaku.
15. Membantu Scripter 2 (GMOBJFIELDS #165): field OBJTYPES bisa diatur dari GM.
   - Setter dengan log di DCO_ObjTypes.c:
     a) Time (0–7200 detik), Deadline, Defend Ratio (0–1), Faction, Retain;
     b) log obj_fields_set ... applies=now|next_start_defender_locked;
     c) tidak ada perubahan kalau nilainya sama.
   - File baru DCO_ObjTypesGM.c berisi 5 atribut GM: 3 slider, 1 checkbox, 1 dropdown faksi dari FactionManager.
   - Edit.conf (5 blok, kategori Commander Objective) dan DCO_SettingsHelp.conf (5 penjelasan).
   - Destroy Targets tetap hanya di Workbench, karena GM tidak punya input teks bebas.
   - Compile bersih 00:56; static-guard OK; dco-gm-audit noHelp 0 / deadAttr 0; jumlah kurung seimbang.
   - BELUM dicek visual di GM (perlu GM).

DITES + HASIL
1. Compile: setiap perubahan .c dites dengan shadow compile (salinan gproj + scripts di scratchpad, tree tidak tersentuh sebelum compile bersih), lalu dicompile ulang pada tree. Waktu compile bersih: 21:28, 21:30, 21:32, 21:40, 21:45, 21:48, 22:15, 23:06. static-guard DCO_AI OK setelah setiap edit.
2. REAL-01 PANIC30 (antrean BD): run malam ini 220058 (63 tewas), 231624 (78), 003029 (52).
   - FINAL n=5 vs BASE n=6:
     a) korban 62.4 vs 71.3 (−12.5%)
     b) mati di tempat terbuka 55 vs 64.7
     c) mati saat panik 9.0 vs 15.0 (−40%)
     d) lama panik 30 vs 105 detik
     e) FF 4.2 vs 5.2
     f) peluru/unit 47.7 vs 48.7
     g) TIDAK ada metrik yang lebih buruk.
   - MG/AR per peran dalam noise.
3. Review realisme A/B sesi lain (REAL-10), PERF_STRESS malam ini, BASE n=3 dari build yang sama (212507/224442/235841):
   a) CTX (ctx.on), n=3:
      - LEBIH BAIK: mati di tempat terbuka 68.7→59.7, tiarap diam 25→23.7, lama panik 146→57 detik.
      - SAMA: korban 74→67.3, FF, hits.
      - LEBIH BURUK: peluru/unit 51→40.1 (−21%).
      - Rekomendasi: belum default ON sampai volume tembakan dijelaskan. Dikirim ke ctx dan Research.
   b) BOUND (agg.bound_v2, milik AI Design), n=3 (221134/232654/004103) vs BASE n=3:
      - LEBIH BURUK: total hits 790→649 (−18%, di luar noise). Hit% total sekitar 13%→10% pada volume tembakan yang sama.
      - Arah membaik tetapi masih dalam noise: mati di tempat terbuka 68.7→58.3 (−15%), tiarap diam 25→20.7, korban 74→70.
      - MG/AR: cover 6→9.4%, hits/unit −17%. Artinya MG ikut bounding ke cover seperti rifleman, padahal sebagai base of fire MG seharusnya jarang berpindah.
      - Rifle: hits +11%, kills +12%.
      - Verdict: BELUM default. Syarat ON: perbaikan MG/AR di bound_v2 lalu re-A/B 3+3.
   g) Kebisingan per peran: hits MG/AR per unit di BASE saja berkisar 2.33–3.89 antar run (sd sekitar 0.55). Penurunan hits MG pada AGGFM (−23% vs BASE n=3, −13% vs BASE n=6) masih dalam noise. Data ini diberikan ke BD. Rencana BD menyalakan AGGFM sebagai default setelah 02:25 TIDAK terlaksana, karena penjaga izin sesi BD memblok perubahan default. Keputusan diserahkan ke user (laporan BD memuat data, verdict Realism YA, verdict AI Design TIDAK, dan skrip agg_flip.js).
   e) AGGFM (paket tb.* milik BD), n=3 untuk keputusan default BD-3:
      - TIDAK ada metrik yang lebih buruk.
      - LEBIH BAIK: FF 6.7→1.7, tiarap diam 25→18.7.
      - SAMA dengan arah membaik: korban 74→64, mati di tempat terbuka 68.7→59.7, mati saat panik 17.3→13.
      - MG/AR: peluru +6%, hits/unit −23%, kills/unit sama. Rifle: peluru −19%.
      - Jawaban realisme untuk pertanyaan BD: hits MG −23% DAPAT DITERIMA, karena tugas MG adalah base of fire dan hasil yang penting (korban pihak sendiri, FF) membaik.
      - Rekomendasi: default ON. Dua hal untuk diawasi berikutnya: MG yang tidak menembak sama sekali naik 4→9%, dan peluru rifle −19%.
   f) Untuk CTX, ctx memecah per peran: rifle −31% peluru tetapi +19% hits (disiplin tembak, realistis); MG/AR −25% peluru (supresi hilang). Saran realisme ke ctx: volume tembakan MG/AR mengikuti tugas base of fire, bukan tingkat ancaman pribadi, jadi pakai lantai 1.0 atau kecualikan MG/AR dari ThreatW.
   c) Data awal (n=1-2), bukan verdict:
      - AGGFM: korban 66.5, FF 2.
      - OSCB: lama panik turun (27 detik).
      - ASSIST0: FF 10, korban 76.
   d) Pagi, n=2:
      - AGG: korban sedang heal 13% (BASE 4%), FF ×2 karena granat.
      - AGG2: pembunuh terlihat 78% (BASE 69%), FF rendah.
      - OPP: mati saat menghindar granat 15% (BASE 6%).
4. Alat dco-abmetrics.js dicoba pada PERF_STRESS dan DEF_AREA.
   - Tanggal 10-11 juga diuji. Hitungan korban sudah dikoreksi: death_ctx benar; selisih dengan unit_summary alive=0 adalah unit yang pingsan saat run berakhir (temuan Auditor; field state ditambahkan Auditor 23:30).
5. TIDAK dites (tidak ada lane):
   - OBJTYPES (world DEF_AREA_OBJTYPE siap, di backlog BD);
   - CQB pre-check (world PERF_STRESS_PRECHECK);
   - OPENCOVER;
   - DEF_AREA_CLOSEIN/RETFIRE;
   - ID125;
   - PANIC30_BCSH;
   - boot test 58 key config baru.

BELUM
1. KEPUTUSAN UNTUK USER: default PANIC30 → 30 detik.
   - Alasan belum dilakukan: Realism sudah memutuskan YA berdasarkan data (n=5 vs BASE n=6, tidak ada metrik yang memburuk). Tetapi pengubahan default perilaku diblok penjaga izin sesi. Pukul 23:30 skrip untuk menyiapkan perubahan ini ditolak ("Modify Shared Resources"). Penjaga yang sama juga memblok BD untuk AGGFM. Saya tidak mengakali blokir itu.
   - Silakan user mengubahnya sendiri, atau mengizinkan, di 3 tempat:
   - DCO_GlobalTuneRealism.c baris 8 (default config 0.0 → 30.0);
   - baris 108 (fallback log);
   - DCO_MoraleSystem.c baris 285 (fallback di pemanggil).
   Setelah itu jalankan tools/dco-tune-fallback-check.js (alat Scripter 2) dan static-guard.
   - Sambil menunggu, admin server sudah bisa menyalakannya sekarang lewat config: morale_panic_max_s=30 di DCO_GlobalConfig.json.
2. OBJTYPES #167: kode selesai, BELUM dites. World DCO_T_DEF_AREA_OBJTYPE (700 detik) ada di urutan pertama backlog BD.
3. CQB pre-check (B3/REAL-07): kode selesai, default OFF, BELUM dites. World DCO_T_PERF_STRESS_PRECHECK.
4. REAL-02 CLOSEIN di world serbuan: world DCO_T_DEF_AREA_CLOSEIN siap, belum ada lane.
5. REAL-04 RETFIRE: world DCO_T_DEF_AREA_RETFIRE siap, belum ada lane. agg.retfire dan tm.retfire sudah satu mekanisme.
6. REAL-03 ID125: world sudah ada, belum ada lane. Default tetap 1 sampai user memutuskan.
7. REAL-05 OPENCOVER: world IND_OPENCOVER / _ON (300 detik) siap, belum ada lane.
8. REAL-06 BREAKCONTACT: world PERF_STRESS_ABL_PANIC30_BCSH (PANIC30 + bc.shaken) siap, belum ada lane. bc.* sekarang bisa diatur dari config server.
9. Tes boot JSON untuk 58 key config baru: belum (lane penuh).
10. A/B di world DEF harus diselang-seling (A,B,A,B) atau memakai store learning yang tetap. BuildingLearn/CommanderLearn di profile utama bertambah antar run. Patch store _test dari AI Design ditunda sampai blok selesai.
11. REAL-10/11 (taktik FAIL) dan REAL-12 (arah desain T1): tetap DITAHAN sesuai tracker.

FILE
Kode (.c, tanpa komentar, static-guard OK):
- BARU scripts/Game/DCO/Group/DCO_CQBPrecheck.c
- BARU scripts/Game/DCO/Commander/Objective/DCO_ObjTypes.c
- scripts/Game/DCO/Group/DCO_CQBActivity.c (guard precheck di Create, TAHAN/SIAP Scripter)
- scripts/Game/DCO/Group/DCO_ClearBuilding.c (skip precheck di FindContactBuilding)
- scripts/Game/DCO/Global/DCO_GlobalTuneRealism.c (cqb.precheck*, + 58 key config baru)
- scripts/Game/DCO/Individual/DCO_NavGuard.c (level log navlink_stall; TAHAN/SIAP BD)
- scripts/Game/DCO/Commander/Utility/CMD_Utility.c (enum DEFEND, HOLD_UNTIL; SIAP Scripter)
- scripts/Game/DCO/Commander/Operations/DCO_MettTable.c (DEFEND/HOLD_UNTIL = bobot CAPTURE; SIAP Scripter)
- scripts/Game/DCO/Commander/Utility/CommanderDebug.c (nama tipe; SIAP Scripter)
- scripts/Game/DCO/Commander/DCO_CommanderFrontline.c (filter OBJASSIGN recon_front; bantuan untuk Scripter)
- BARU scripts/Game/DCO/Commander/Objective/DCO_ObjTypesGM.c (atribut GM OBJTYPES; bantuan untuk Scripter 2)
- Configs/Editor/AttributeLists/Edit.conf (5 blok atribut OBJTYPES)
- Configs/DCO/DCO_SettingsHelp.conf (5 penjelasan)
World tes (BARU, .ent + .meta + _Layers):
- world/Test/Commander/Tactics/DCO_T_DEF_AREA_CLOSEIN, DCO_T_DEF_AREA_RETFIRE, DCO_T_DEF_AREA_OBJTYPE
- world/Test/Individual/DCO_T_IND_OPENCOVER, DCO_T_IND_OPENCOVER_ON
- world/Test/Perf/DCO_T_PERF_STRESS_PRECHECK, DCO_T_PERF_STRESS_ABL_PANIC30_BCSH
Alat:
- BARU D:/Git/DCOReforger/tools/dco-abmetrics.js (metrik A/B bersama)
Tanpa git commit/push/branch.

SALING BANTU (siapa membantu siapa)
- Realism → Scripter: filter OBJASSIGN (pemetaan semua jalur pemilihan objective; recon_front dipasang sendiri; daftar baris untuk _10/_13 dan ThreatResponse/Spawner).
- Realism → Scripter 2: field GM OBJTYPES (GMOBJFIELDS #165), laporan static (STATICGUARD), temuan fallback default (Scripter 2 memperbaiki DCO_TuneCfg dari akarnya).
- Realism → ctx: fungsi CQB pre-check untuk CTX-CQBGATE, review pemetaan VICTORY, verdict CTX + saran realisme MG.
- Realism → BD: data dan perbaikan log navlink_stall, data FF granat, verdict AGGFM + analisis noise MG.
- Realism → AI Design: verdict agg.bound_v2 (n=3, bersyarat).
- Realism → Auditor/Research: alat dco-abmetrics.js (+ kolom yang mereka minta).
- Auditor → Realism: koreksi hitungan korban (unit pingsan di akhir run) + field state di unit_summary.
- Scripter → Realism: SIAP file CQB/commander, menambah case DEFEND/HOLD_UNTIL di GetRequiredGroupCount.
- Scripter 2 → Realism: migrasi default JSON (tabel legacy morale_panic_max_s).
- BD → Realism: menjalankan PANIC30 ×3 di lane 1.

FITUR/SETTING BARU
- 58 setting AI baru di config server (DCO_GlobalConfig.json). Semuanya memakai default yang sama dengan kode, jadi perilaku tidak berubah sampai admin mengubahnya.
  a) Realism Movement: cover_first, open_ground_rule_v2, open_ground_still_s 2, open_ground_kneel_s 3, open_ground_wait_mul 2, open_ground_cover_max_m 15, rush_cover_max_m 15, rush_crawl_m 6, rush_enemy_min_m 30, rush_calm_min_s 3 / max 5, rush_shaken_min_s 2 / max 3, pinned_crawl_max_m 15, panic_close_m 15, panic_mid_m 50, panic_flight_max_m 30, panic_freeze_min_s 1 / max 3, panic_nofire_min_s 10 / max 30, panic_mid_nofire_min_s 5 / max 10, individual_break_min_m 100 / max 300, morale_in_cover_mul 0.85, morale_down_hold_s 40, sbf_area_fire_clear_m 40, combat_move_dir_decode 0.
  b) Realism Suppression: suppressed_threshold 0.6, pinned_threshold_open 2, pinned_threshold_cover 3, suppressed_hold_s 3, suppression_in_cover_mul 0.65, suppression_in_building_mul 0.6, suppression_gain_tense/shaken/panic/spirited_mul 1.1/1.25/1.5/0.9, suppression_recover_shaken/panic/spirited_mul 0.7/0.5/1.15.
  c) Break Contact: break_contact_keep_frac 0.4 (regu memutus kontak di 40% kekuatan), break_contact_when_shaken 0 (1 = juga kalau mayoritas shaken), break_contact_ineffective_s 150, break_contact_ladder_slider 0, break_contact_dogleg 0 (+ _m 60, _min_deg 45, _max_deg 90), break_contact_face_threat 0 (+ _fan_deg 60, _s 10).
  d) Air And Consolidation: air_threat_on 0, air_threat_contact_m 400, air_threat_lane_skip_deg 10, tactic_consolidate_ring_m 150, tactic_consolidate_enemy_max 1.
  e) Rentang nilai dibatasi (min–max) untuk setiap key.
- dco-abmetrics.js: kolom baru untuk tim (deaths, unconEnd, moving, seen%, kills, ffVeh, kP50vUS/vRU).
- Objective Type (atribut objective, Workbench; GM lewat GMOBJFIELDS Scripter 2):
  a) Objective Type: DEFEND / HOLD_UNTIL (baru), DESTROY + Destroy Targets.
  b) m_fDCOTypeTime_s: default 1200 detik.
  c) m_sDCOTypeFaction: kosong = pemilik awal / siapa saja.
  d) m_fDCOTypeDeadline_s: 0 = tidak ada.
  e) m_fDCOTypeDefendRatio: 0 = mati; contoh 0.25 = gagal kalau kalah jumlah 4:1 selama 60 detik.
  f) m_bDCOTypeRetain: default 1.
  g) m_aDCODestroyTargets: nama entity.
  h) Di GM (kategori Commander Objective): Type Time, Type Faction (Auto/faksi), Type Deadline, Defend Fail Ratio, Defend Retain. Destroy Targets hanya di Workbench.
- cqb_precheck (tune cqb.precheck): default 0, nilai 0/1. Kalau 1, gedung yang graph Indoor Nav-nya tidak bisa direncanakan untuk CQB dilewati sebelum CQB dimulai, dan pemilih gedung kontak memilih gedung lain.
- cqb_precheck_ttl_s (cqb.precheck_ttl_s): default 600, rentang 60–3600 detik. Berapa lama hasil pre-check per gedung disimpan.
