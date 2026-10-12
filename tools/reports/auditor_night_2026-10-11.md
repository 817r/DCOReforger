LAPORAN MALAM DCO Features Auditor, 2026-10-10 21:20 → 2026-10-11 02:40
Aturan dipatuhi:
- tanpa git;
- tanpa komentar di .c;
- tanpa static initializer baru (static guard OK di setiap edit);
- TAHAN/SIAP untuk file bersama;
- run game hanya di slot lane BD (1 game);
- semua review baca-saja, temuan dikirim ke pemiliknya.
Salinan lengkap: D:/Git/DCOReforger/tools/reports/auditor_night_2026-10-11.md (catatan rinci: auditor_night_2026-10-11_detail.md).

ANGKA STATIC GUARD (wajib)
- Sebelum (21:30): DCO_AI weight 345 (268 deklarasi: static 25, static ref 182, static const 100, const 38).
  - Beban pool terukur ≈ 46 unit (vanilla menyisakan ±542 unit; DCO ≈ 8,5%).
- Sesudah (pengurangan Scripter 2, terkunci sejak 21:50): weight 110 (102 deklarasi: static 7, static const 65, const 38).
  - Beban pool model 10 unit, terukur ≤ 6 unit (±1%). Sisa untuk mod lain naik ±40 unit.
- Akhir malam (02:00): weight 110, pool cost 10, baseline 110, budget 150, OK. TestAI weight 1, cost 0. tune-check: 468 key, 0 mismatch.

A. SELESAI + DITES / TERVERIFIKASI
1. STATICGUARD #173 (URGENT): alat tools/dco-static-guard.js + baseline tools/static-guard-baseline.json.
   - Memindai DCO_AI + TestAI dan menghitung setiap static/const yang punya initializer.
   - Melaporkan weight (1 + elemen array) dan "pool cost" terkalibrasi.
   - Exit 1 bila naik di atas baseline atau budget, dan mencetak file:baris yang baru. Opsi --top, --update-baseline, --budget=, --scan=<dir>.
   - Diumumkan ke Scripter, Scripter 2, AI Design, ctx, Realism, BD.
2. STATICGUARD: margin nyata lewat probe compile (game 1.8.0.13, addon probe terisolasi, dihapus sesudahnya).
   - LITERAL tidak memakan pool: 8000× static int/string, static const, dan const = literal semua lolos.
   - Biaya (satuan = 1 "static ref X = new X()"): new 1,0; array literal 1,0 + 0,5 per elemen (int dan string sama); ekspresi seperti 0.25*Math.PI ±0,5.
   - Pool BERSAMA satu modul (dibagi ke 20 class tetap sama).
   - Kapasitas vanilla saja 538-546; vanilla + DCO sebelum pengurangan 492-500; sesudah pengurangan 537-540.
   - Pesan error selalu menunjuk file vanilla tempat batas terlampaui (pool diisi berurutan menurut path), bukan penyebabnya. Server pemain yang gagal kemungkinan juga memuat mod lain; bila masih gagal dengan build baru, perlu bisect per addon di server itu.
   - Insiden: probe pertama sempat ada di TestAI 21:32-21:33. BD memberi tahu lane memuat TestAI; dihapus seketika, dan 3 run yang mulai saat itu dicek bersih.
3. A/B spec.assist_commit_s (perbaikan asisten MG semalam), 3 vs 3 di lane BD.
   - Churn order ulang asisten ON 17% vs OFF 31%.
   - Standar (dco-abmetrics), ON vs OFF:
     - mati 74 vs 78,7;
     - mati di terbuka 68,7 vs 71,7;
     - FF 6,7 vs 10,3;
     - mati diam tiarap 25 vs 31,3;
     - seen 73% vs 66%;
     - kills 67,3 vs 68,3;
     - kematian pertama 110 s vs 85 s;
     - kP50 15,3 vs 15 m.
   - VERDICT: tanpa regresi; default 6 s dipertahankan. Research setuju (FF OFF +54%, kebanyakan granat tangan kawan dari role at/leader pada 3-18 m).
   - Tindak lanjut: saran granat FF dikirim ke AI Design → switch baru gren.ff_predict (default 0; cek posisi kawan yang diprediksi + sebaran lemparan). Belum di-A/B (GREN1 ada di backlog BD).
4. Rearm/pickup/patrol semalam: perbaikan pemicu rearm terbukti 10-10 siang (13 run). a-d lain belum (lihat C).
5. AUD-VERIFY f (baseline baru penyebab mati, build ≥ 04:51, 26 run non-PERF, 298 kematian):
   - moving_open 21%, static_open 20%, unseen_shooter 17% (+5% dengan bleed), grenade 13%, bleed_no_medic 7%, vehicle_fire 7%;
   - bleed=1 tercatat 13% (sebelum perbaikan 0%);
   - mati di dalam cover hanya 4%; jarak bunuh p50 29 m.
6. AUD-KILLDIST: menjadi alat bersama tools/dco-abmetrics.js (Realism). Tiga tambahanku masuk: vehicle_collision dikecualikan, kP50 per faksi korban, peringatan sesi sebelum 04:51.
7. AUD-TESTAREA: selesai lewat BD (DCO_TestArea.c dipulihkan dari backup; review: identik, API cocok, atribut cocok).
8. Alat baru tools/dco-tune-check.js: membandingkan default registrasi setiap DCO_TuneCfg dengan semua fallback pembaca; exit 1 bila beda.
   - Menemukan bug HIGH: agg.profile efektif OFF di semua server (registrasi 1, pembaca fallback 0, dan DCO_TuneCfg tidak menyimpan nilai yang sama dengan default).
   - Diperbaiki AI Design 22:27; sekarang 0 mismatch.
9. Pemantau flip default (aturan BD sebelum 02:25): snapshot 998 default pukul 23:30, dicek tiap 5 menit.
   - Satu-satunya flip sejak 07:40: agg.profile (efektif sejak 22:27). Lane tidak terkontaminasi karena profil utama menyimpan 0.0 eksplisit.
   - Hasil: 1002 default dibandingkan, 0 berubah sampai cek terakhir sebelum laporan ini (02:00; pemantau jalan terus sampai 02:25).
   - Default bukan-nol baru yang muncul (key baru, bukan flip): tb.fm_close_m 15 dan tb.fm_gate_wait_s 15. Switch-nya tb.fm_gate tetap 0, jadi tidak mengubah perilaku.

B. SELESAI, BELUM DITES (kode, compile bersih + guard OK)
10. C3 / AUDITB2 F1 bagian 2: grup berbelok (dogleg) lalu menghadap kontak setelah rally.
    - File: Group/DCO_BreakContact.c (TAHAN/SIAP dengan Scripter).
    - bc.dogleg: titik belok 45-90° dari sumbu ke rally, 60 m, dicek navmesh, tidak boleh mendekati musuh.
    - bc.face: anggota melihat ke arah kontak dalam kipas 60° selama 10 s saat tiba di rally.
    - Default 0. Key sudah didaftarkan ke config server (break_contact_dogleg*, break_contact_face*).
    - A/B: world DCO_T_PERF_STRESS_BC dibuat, tapi lane penuh → belum ada run.
11. AUD-CONSOLE:
    - (a) "Trying to allocate same compartment twice!": AllocateCompartment hanya bila slot belum ada di GetAllocatedCompartments grup (Group/DCO_Statics.c, BT/Tasks/DCO_FindTurrets.c; counter static_man:already_allocated / turret:already_allocated).
    - (b) "ImageWidget Bg": skala kartu offer tidak lagi menyentuh anak dengan anchor stretch (Commander/Player/DCO_BriefUI.c). TAPI warning masih muncul 6× per world load di run sesudahnya, jadi sumber utama belum terbukti (kemungkinan layout vanilla atau DCO lain saat HUD dibuat).
    - (c) SCR_JsonLoad/SaveContext obsolete: 27 pemakaian di 11 file, hanya warning; tidak diubah (pemilik banyak).
12. AUD-SBFLOG: log sbf_live_end grp dur_s retargets last_state di destruktor volume (Group/DCO_LiveSuppress.c; TAHAN/SIAP Scripter).
13. AUDITLOG2: selisih "mati vs death_ctx" yang dilaporkan Realism ternyata unit PINGSAN saat run berakhir.
    - unit_summary sekarang punya field state=alive|uncon|dead|gone|dead_unlogged (Benchmark/DCO_Battle.c); alive tidak diubah.
14. BANTU Scripter 2: spawner A12/A13/A17 (Commander/Spawner/DCO_GroupPresets.c, Settings/DCO_SettingsPresetsUI.c, Configs/DCO/GroupPresets/DCO_BuiltinGroupPresets.conf).
    - File key non-ASCII + hash, migrasi key lama, cek tabrakan berbasis key.
    - "Recon team" ganda → US/USSR Recon team, plus alias data lama.
    - Nama built-in ditolak; nama maks 40 karakter; daftar RplProp maks 200.
    - Butuh uji GM.

C. BELUM SELESAI
15. AUD-VERIFY a-d (patrol off-navmesh, pickup debounce/cooldown, rearm.reach_m): tidak ada run DEF_DEPTH/SEL_DAY malam ini (konfirmasi BD), dan run lane hanya CTX/PERF/VICTORY.
16. AUD-CFGAB6: 3 dari 6 arm DITES di SLOT AUD 01:23-01:55 (PERF_STRESS ±170 AI, profil privat = salinan profil utama dengan 1 key diubah; nilai dikonfirmasi lewat cfg_summary dari JSON; 1 run per arm vs rata-rata 3 run BASE; indikatif).
    | arm | mati | terbuka | panik | mati panik | kills | mati-1 | kP50 |
    | BASE (default) | 74 | 68,7 | 22,7 | 17,3 | 67,3 | 110 s | 15,3 m |
    | superiorRatio 1 | 75 | 67 | 24 | 21 | 71 | 101 s | 15 m |
    | takeCoverChance 0,2 | 76 | 71 | 27 | 16 | 75 | 83 s | 18 m |
    | suppressionEffect 0,2 | 65 | 54 | 1 | 0 | 63 | 132 s | 14 m |
    - suppressionEffect 0,2: efek KUAT, panik hampir hilang (1 vs 23), mati di terbuka -21%, kematian pertama 22 s lebih lambat. Setting ini benar-benar bekerja dan sangat berpengaruh.
    - takeCoverChance 0,2: kematian pertama lebih cepat (83 vs 110 s), jarak bunuh sedikit lebih jauh; efek kecil.
    - superiorRatio 1: dalam variasi antar-run (world ini tanpa komandan; key ini terutama dipakai keputusan serang komandan/grup).
    - Hasil sudah ditulis di docs/DCO_Server_Config.md bagian "Verified in game".
    - BELUM: arm superiorRatio 4, overmatchAssault false, suppressionEffect 2 (slot habis).
17. A/B C3 (PERF_STRESS_BC ×3): belum ada lane.
18. AUD-PERFVANILLA: tidak ada lane yang benar-benar kosong.
19. AUD-FMTEST: tetap ditahan (TR1/FMFIX/FMIMPROVE; rekonsiliasi tracker mengusulkan TR1/FMIMPROVE drop, keputusan user).
20. Warning "ImageWidget Bg" sisa 6× per world load (lihat 11b).
21. GenerateRandomPointInRadius(0,0) 2× di world PANIC30: semua pemanggil DCO terjaga; kemungkinan vanilla; belum ada stack.
21b. Catatan LOW (review 02:00, DCO_TeamBound.c, gate tb.fm_gate): saat assault menunggu base of fire dan saat timeout hanya ada counter (sfm:assault_wait_bof / sfm:assault_gate_timeout), tanpa baris log; sfm_close sudah log. Usul: satu log sfm_gate_timeout grp wait_s. Switch default 0, jadi tidak mendesak.

D. REVIEW KODE (semua sesi; temuan dikirim, sebagian besar sudah diperbaiki pemilik malam ini)
22. Perbaikan config JSON (Scripter 2): error handling bersih; literal bulat untuk float terbaca benar. Catatan: simpan GM membekukan semua default.
23. Learning CmdLearn/IndLearn/PlyLearn (AI Design): HIGH CmdLearn tidak reset saat ganti world + 4 MED + 7 LOW. SEMUA diperbaiki 21:33.
24. THREAT-DOSSIER (AI Design, always ON): HIGH counter-flank ditolak di tanah rendah, decoy menarik grup dari pertempuran nyata + 5 MED. Diperbaiki 21:37.
25. K2 + OBJASSIGN + OBJFOCUS (Scripter): HIGH OpPlan melewati filter, cabang release tak tercapai + 7 MED. Diperbaiki 22:12 (4 keputusan sadar dibiarkan, alasannya diverifikasi).
26. Ronde 2 Scripter: OBJPHASE (3 HIGH), V5/D7 (1 HIGH standoff tanpa akhir), spawner UI (2 MED). Diperbaiki 23:1x (kecuali V-5 dan satu nit).
27. CFGFACTION #170 (Scripter 2): 3 MED (termasuk simpan GM setelah parse gagal yang bisa menghapus file admin) + 3 LOW. Diperbaiki 23:00.
28. Review A1/D3/A6 GlobalAI (Scripter 2): bersih + 3 catatan; record default per world diterapkan (rotasi skenario tidak lagi memicu migrasi).
29. Kode BD (MoveBrain V2, CqbKindMove, ff.vert, TuneSyncBD, TestArea): tanpa regresi di switch 0, 3 MED + 4 LOW. Diperbaiki 22:28.
30. GRPTASKFLAGS + staging learn + SquadChatter + AGGFIX (AI Design): mask default 511 tidak pernah memblok. MED: squad Defend/Recon Only bisa dipakai serbuan, staging learn sampel dobel dan "clear" tak pernah tercatat. Diperbaiki 23:10.
31. Verifikasi log CMDLEARN + THREAT-DOSSIER (AI Design): 2 sesi komandan sah; tidak ada anomali. tactic_select_learn 0 karena world test ForceSelect.
32. Scan error run lane (BD): 15+17 log. Baru: VME failed move di VICTORY (ke ctx), compile error sementara 21:57, random-point radius 0. Sisanya noise vanilla.

E. REKONSILIASI TRACKER (usulan untuk Daily Routine Tracker; items.json TIDAK diedit)
33. Tabel lengkap: D:/Git/DCOReforger/tools/reports/tracker_recon_2026-10-10.md. 146 item: PRESENT 118, PARTIAL 11, MISSING 11, UNCLEAR 3.
34. Usulan status:
    - 14 done → tested (catatan ditulis ulang);
    - 51 done → queued;
    - AIMARKERS done → queued (SETTING GM TIDAK BERFUNGSI; kode hanya di backup 10-06);
    - BTFIX done → fix;
    - BENCHWB tested → done;
    - WORLDORG done → drop;
    - TR1/FMIMPROVE fix → drop dan ASL hold → drop (framework TR sengaja dihapus; keputusan user karena AUD-FMTEST bergantung);
    - FMFIX fix → queued;
    - ONOFF hold → drop;
    - PERFOPT/BLDCACHE progress → done;
    - ITEST queued → progress;
    - AUDITB2 queued → progress.

F. FILE YANG DIUBAH (oleh saya malam ini)
- DCO_AI/scripts/Game/DCO/Group/DCO_BreakContact.c (C3)
- DCO_AI/scripts/Game/DCO/Group/DCO_Statics.c, BT/Tasks/DCO_FindTurrets.c (compartment)
- DCO_AI/scripts/Game/DCO/Commander/Player/DCO_BriefUI.c (Bg)
- DCO_AI/scripts/Game/DCO/Group/DCO_LiveSuppress.c (sbf_live_end)
- DCO_AI/scripts/Game/DCO/Benchmark/DCO_Battle.c (state=)
- DCO_AI/scripts/Game/DCO/Commander/Spawner/DCO_GroupPresets.c, Settings/DCO_SettingsPresetsUI.c, Configs/DCO/GroupPresets/DCO_BuiltinGroupPresets.conf (A12/A13/A17, untuk Scripter 2)
- DCO_AI/world/Test/Perf/DCO_T_PERF_STRESS_ASSIST0.ent/.meta/_Layers, DCO_T_PERF_STRESS_BC.ent/.meta/_Layers (baru)
- tools/dco-static-guard.js, tools/static-guard-baseline.json, tools/dco-tune-check.js (baru)
- tools/reports/tracker_recon_2026-10-10.md (baru)
- docs/DCO_Server_Config.md (diregenerasi 02:00: 600 key, semua berdeskripsi termasuk 4 key baru tb.fm_gate/tb.fm_close_m/tb.fm_gate_wait_s/tac.standoff_max_s; bagian per-faction, migrasi default per world, penanganan error, hasil arm config)
- Sementara, sudah dihapus: probe TestAI TAI_ZZStaticProbe.c dan addon probe di scratchpad.

G. FITUR/SETTING BARU
1. bc.dogleg (JSON break_contact_dogleg)
   - Default 0, range 0/1.
   - Cara kerja: saat kontak putus, grup lebih dulu ke titik belok 45-90° dari sumbu mundur, baru ke rally.
   - Contoh efek: tembakan musuh ke sumbu mundur lama meleset; mortir yang membidik rute mundur tidak mengena.
2. bc.dogleg_m — default 60 m (20-200): jarak titik belok.
3. bc.dogleg_min_deg / bc.dogleg_max_deg — default 45 / 90 derajat: rentang sudut belok.
4. bc.face (JSON break_contact_face)
   - Default 0.
   - Cara kerja: setiba di rally, anggota grup melihat ke arah kontak terakhir (kipas pengamanan).
   - Contoh efek: grup tidak lagi berdiri membelakangi musuh setelah mundur.
5. bc.face_fan_deg — default 60°: lebar kipas pandang.
6. bc.face_s — default 10 s: durasi.
7. Field log baru:
   - unit_summary state=alive|uncon|dead|gone|dead_unlogged;
   - sbf_live_end;
   - break_contact_turn / turn_skip / turn_done / face;
   - group_preset_refused / key_collision / key_migrated / builtin_alias / renamed / list_capped;
   - counter static_man:already_allocated, turret:already_allocated, bc:*, sbf:live_end.
8. Alat:
   - tools/dco-static-guard.js (wajib sebelum SIAP);
   - tools/dco-tune-check.js;
   - world A/B DCO_T_PERF_STRESS_ASSIST0 dan DCO_T_PERF_STRESS_BC.

H. SIAPA MEMBANTU SIAPA
- Auditor → Scripter:
  - review K2/OBJASSIGN/OBJFOCUS dan ronde 2 (OBJPHASE, V5/D7, spawner UI);
  - perbaikan compartment/Bg/sbf di file miliknya (dengan SIAP).
- Auditor → Scripter 2:
  - spawner A12/A13/A17 (kode);
  - review A1/D3/A6 dan CFGFACTION;
  - dokumen config;
  - pengukuran pool static sesudah pengurangannya.
- Auditor → AI Design:
  - review learning, THREAT-DOSSIER, GRPTASKFLAGS/staging learn/SquadChatter/AGGFIX;
  - verifikasi log CMDLEARN/DOSSIER;
  - temuan agg.profile.
- Auditor → BD:
  - review MoveBrain V2/CqbKindMove/ff.vert/TestArea;
  - scan error run lane;
  - analisis A/B ASSIST0;
  - pemantau flip default.
- Auditor → Realism: akar selisih death_ctx + field state; masukan dco-abmetrics.
- Dibantu: BD (lane + slot AUD), Research (riset batas static init T200535), Realism (alat dco-abmetrics), subagent (deskripsi 124 key config, review paralel).
