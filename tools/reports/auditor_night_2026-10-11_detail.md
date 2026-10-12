# DCO Features Auditor - kerja malam 2026-10-10 21:20 - 03:00

## R1. Review perbaikan config JSON (Scripter 2, DCO_GlobalAI.c 13:05 + Settings 13:xx)
- Error handling:
  - JSON invalid → cfg_parse_fail, file tidak diubah;
  - tipe salah → cfg_bad_type, merge dilewati (cfg_merge_skipped);
  - key hilang → merge seperti biasa.
  - Logika benar.
- Dicek tambahan: literal bulat untuk key float ("aiAggression": 2) terbaca benar sebagai 2 (from_json=2, bad_type=0; uji cfgerr_missing_125754 milik Scripter 2). Admin boleh menulis 2 atau 2.0.
- Simpan config dari GM (DCO_SaveActiveConfig + tombol Presets Global):
  - diverifikasi di server lewat VerifiedGM + DCO_CanSaveConfig, .bak dibuat, semua langkah di-log (cfg_save_gm result=...);
  - CATATAN: menulis SEMUA nilai aktif, termasuk default, ke file. Ini memperkuat masalah "default beku" (AUD-CFGDEFAULT): setelah GM menekan simpan, perubahan default di kode berikutnya juga tidak sampai.
- Prompt preset saat startup: s_bDCO_StartupDone/s_bDCO_StartupAsked statis dan tidak di-reset. Di Workbench, sesi play kedua dalam proses yang sama tidak menampilkan prompt (LOW, WB saja).
- Migrasi default beku (usulan AUD-CFGDEFAULT) BELUM dikerjakan. Pemilik: Scripter 2 ("perbaikan config JSON default PERTAMA").
- OBJASSIGN/EVENTAPI: belum ada kode per 21:24. Akan direview saat muncul.

## R2. Review learning (CmdLearn/LearnStore always ON; IndLearn learn.ind_on 0; PlyLearn learn.ply_on 0) → AI Design. SEMUA SUDAH DIPERBAIKI 21:33
- HIGH CmdLearn.Get tidak reset saat ganti world: data peta A dipakai di peta B, ResetAll dari test bisa menghapus data ASLI, kill hook tidak terpasang, kill hantu di DeathsNear. Sudah diperbaiki (m_World + learn_world_switch).
- MED:
  - overwatch credit 500 m semua kill → sekarang hanya kill grup sendiri (KillsByGroup);
  - IndLearn 1 kematian menolak sel untuk semua faksi → key per faksi, butuh bobot > 1;
  - LearnStore tumbuh tanpa batas → PruneSmall + save hanya saat berubah;
  - mati kehabisan darah ditolak → jendela 120 s.
- LOW:
  - file korup ditimpa → .recovered.json;
  - exposure dt tetap → dt nyata;
  - null PlyLearn;
  - s_mNext di-prune;
  - spot PlyLearn tidak pernah dimuat → ambang 0,65 + umur 20 menit;
  - entri commander terhapus;
  - VsAt di-cache 10 s.
- Dibiarkan oleh pemilik: log cover_skip saat fallback.

## R3. Review THREAT-DOSSIER + ThreatResponse (always ON) → AI Design 21:36, menunggu perbaikan
- HIGH: counter-flank ditolak setiap kali titik flank lebih rendah dari ancaman (CommanderThreatResponse.c:1727 memakai skor < 0; skor negatif = tanah lebih rendah). Regresi tanpa switch, ditambah log tiap Think.
- HIGH/MED: penandaan decoy menarik grup dari pertempuran nyata (severity dossier lain tidak dicek), lalu eskalasi palsu + radio.
- MED:
  - evaluasi window 300 s menilai hold/PREPARE_DEFENSE "ineffective", sehingga means bolak-balik hold↔flank tiap 5 menit;
  - CapOk total/3: commander kecil tidak pernah merespons ancaman non-objektif;
  - satu dossier dipakai dua misi (radius 250 vs 225 m);
  - dossier yatim tidak dibersihkan;
  - CmdLearn.Tick sekarang dipanggil dan skor taktik dipengaruhi learning, tanpa switch (konfirmasi niat).
- LOW: spam log threat_cap/threat_flank_skip; log penolakan NAI/TAI hilang; m_mSector/m_bHPT tidak pernah reset.

## C3 (AUDITB2 F1 bagian 2): dogleg + menghadap kontak setelah rally - KODE SELESAI, menunggu A/B
- File: Group/DCO_BreakContact.c (TAHAN/SIAP dengan Scripter). Compile bersih 21:27, static guard OK.
- bc.dogleg (default 0):
  - saat kontak putus (break_contact_done), grup lebih dulu ke titik belok 45-90° dari sumbu ke rally (bc.dogleg_min_deg 45, bc.dogleg_max_deg 90, jarak bc.dogleg_m 60), baru ke rally;
  - titik dicek ke navmesh, dibalik bila mengarah ke musuh, ditolak bila lebih dekat ke musuh;
  - grup ber-commander: 2 rute berurutan; tanpa commander: elemen ke titik belok lalu rally (tiba ≤ 20 m atau 90 s).
  - Log break_contact_turn grp deg dist to rally / break_contact_turn_skip reason / break_contact_turn_done why leg_s; counter bc:turn, bc:turn_skip_*, bc:turn_leg_*.
- bc.face (default 0):
  - saat tiba di rally (break_contact_rally), semua anggota melihat ke arah kontak terakhir dalam kipas bc.face_fan_deg 60° selama bc.face_s 10 s (pengamanan sektor);
  - log break_contact_face grp units bearing fan dur_s threat_d; counter bc:face.
- A/B: world baru DCO_T_PERF_STRESS_BC {E134A6A726E9458B} (m_aTune bc.dogleg 1 + bc.face 1) ×3 diantrekan ke BD vs baseline PERF_STRESS ×3.

## STATICGUARD #173 (URGENT): tools/dco-static-guard.js + tools/static-guard-baseline.json - SELESAI
- Cara kerja:
  - memindai semua .c di DCO_AI/scripts dan TestAI/Scripts, setelah komentar dan isi string dibuang;
  - menghitung setiap deklarasi static/const yang punya initializer (bukan fungsi); bobot = 1 + jumlah elemen array/map literal;
  - baseline per tanda tangan (file + teks deklarasi);
  - exit 1 bila total naik di atas baseline atau di atas budget;
  - mencetak file:baris setiap initializer baru/bertambah.
- Opsi: --top (25 file terberat), --budget=N, --update-baseline (sesudah ada pengurangan).
- Angka baseline 21:30: DCO_AI weight 345, 268 deklarasi (static 25, static ref 182, static const 100, const non-static 38), sesuai hitungan grep 230 + 38. TestAI 1.
- 21:40: 340 (−5, GrenadeUtility AI Design).

## STATICGUARD - margin nyata dari probe compile (game 1.8.0.13)
- Metode:
  - addon probe sementara di scratchpad (BUKAN TestAI dan BUKAN DCO_AI), compile game exe ±15 s per percobaan, pencarian biner;
  - dua konfigurasi: vanilla saja, dan vanilla + DCO_AI;
  - probe dihapus setelahnya.
  - Catatan: probe pertama sempat ditaruh di TestAI 21:32-21:33. BD memberi tahu bahwa lane memuat TestAI; file langsung dihapus, dan 3 run lane yang mulai saat itu dicek bersih (tidak ada COMPILE_ERROR).
- Biaya per bentuk (8000 deklarasi tiap bentuk):
  - static int/string = literal, static const int/string = literal, const int = literal: 8000 LOLOS, biaya ≈ 0 (literal dilipat compiler, tidak masuk pool);
  - static float = 0.25 * Math.PI (ekspresi): gagal di sekitar 3600;
  - static ref array<int> = {1}: gagal;
  - static ref map = new map(): sisa ruang dengan DCO 492-500, vanilla saja 538-546;
  - static ref array<string> = {"a","b","c","d"}: 176-180 (≈ 2,8 unit "new" per deklarasi).
- Kesimpulan:
  - pool diisi oleh EKSPRESI, ARRAY LITERAL (terutama string) dan NEW, bukan literal;
  - inline 679 const literal pada 09-29 hampir tidak menambah ruang; yang mahal adalah array string port BT (s_aVarsIn/s_aVarsOut) dan new map/array statis.
- Beban DCO sekarang ≈ 46 unit "new" (≈ 8,5% dari ruang ±540 yang ditinggalkan vanilla). Sisa untuk mod lain ≈ 495 unit.
- Server pemain yang gagal (log 09.10.2026 00:19) kemungkinan memuat mod lain yang ikut memenuhi pool. Pesan error menunjuk file vanilla (SCR_AIProcessOrder.c:69 s_aVarsOut) karena pool diisi berurutan menurut path file; baris tempat batas terlampaui yang dilaporkan, bukan penyebabnya. Mengurangi DCO tetap menambah margin untuk server dengan banyak mod.
- Pesan error berpindah-pindah file (terbukti di probe: N=500 gagal di SCR_WeaponInfo_MultiWeaponTurret.c / SCR_AIDecideActivity.c), jadi bisect per addon wajib untuk mencari penyebab di server pemain.

## AUD-CONSOLE
- (a) "Trying to allocate same compartment twice!" (1-7 per run):
  - akar: DCO_Statics.ManStatics dan DCO_FindTurrets memanggil SCR_AIGroup.AllocateCompartment lagi untuk slot yang sudah dialokasikan grup yang sama, ketika pending habis dan prajurit belum naik (FreeSlot hanya mengecek IsReserved slot, bukan daftar alokasi grup);
  - PERBAIKAN: cek GetAllocatedCompartments dulu, hanya alokasi bila belum ada; counter static_man:already_allocated / turret:already_allocated;
  - file: Group/DCO_Statics.c, BT/Tasks/DCO_FindTurrets.c. Compile bersih 21:39, guard OK.
- (b) GUI "ImageWidget Bg: Position/Size works only when min and max anchor is the same" (6-30 per run):
  - akar: DCO_BriefUI.c (kartu offer) men-skala SetPos/SetSize SEMUA anak widget "Card", termasuk Bg yang anchor-nya stretch (0 0 1 1);
  - PERBAIKAN: hanya widget dengan anchor min == max yang di-skala;
  - file: Commander/Player/DCO_BriefUI.c. Compile bersih 21:40.
  - Verifikasi di run berikutnya: hitungan "ImageWidget Bg" harus turun ke 0. Kalau masih ada, sumber lain = layout Settings (DCO_SetCell/SetEdit Bg).

## AUD-TRACKER: rekonsiliasi tracker vs kode (146 item done/tested/hold + item bernama) - SELESAI
- Tabel lengkap per item (status sekarang, verdict, bukti file:baris, usulan status, alasan): D:/Git/DCOReforger/tools/reports/tracker_recon_2026-10-10.md. items.json TIDAK diedit.
- Verdict: PRESENT 118, PARTIAL 11, MISSING 11, UNCLEAR 3, n/a 3.
- USULAN PERUBAHAN STATUS (untuk Daily Routine Tracker):
  - 14 done → tested, catatan ditulis ulang: kode 10-05 hilang, tapi dibangun ulang 10-07..10-10 dengan nama lain dan sudah ada run.
  - 51 done → queued: flag TACTIC_REEVAL tidak pernah dibaca; tactic_reeval/hold_reinforce hilang saat revert.
  - AIMARKERS done → queued: SETTING GM/JSON TIDAK BERFUNGSI. m_iAIMapMarkers disimpan tapi getter-nya tanpa pemanggil; DCO_AIMarkers.c hanya ada di backup 10-06. Perlu restore dengan OK user.
  - BTFIX done → fix: pembersihan BT ter-revert (override HandleAmmo.bt lama, cabang HandleCombatMoveRessuplying, subtree resupply lama kembali; logger ammo_low hilang). DCO_AIRearmBehavior baru sekarang jalan berdampingan dengan jalur lama.
  - BENCHWB tested → done: perf + census sekarang hanya dipaksa saat run test-case; teks tracker usang.
  - WORLDORG done → drop: susunan folder world ter-revert.
  - TR1 fix → drop, FMIMPROVE fix → drop, ASL hold → drop: framework TR sengaja dihapus 10-07. Konsekuensi: AUD-FMTEST perlu desain ulang, karena dasar uji FM-nya hilang. Keputusan user.
  - FMFIX fix → queued: bagian A (FireLaneCheck) ada; B-D (flank offset, GO gate, composite abort) hilang.
  - ONOFF hold → drop.
  - PERFOPT progress → done; BLDCACHE progress → done (run verifikasi pending).
  - ITEST queued → progress (framework + ±100 world ada).
  - AUDITB2 queued → progress (F1: bc.no_retask, bc.dogleg, bc.face; default 0, belum dites).
- Catatan basi yang perlu ditulis ulang: 13, 14, FFMG (perlindungan sekarang DCO_Util.FireLaneCheck), BENCHWB, TESTLOC.
- INDBT: penundaan heal grup (heal_defer/heal_resume) hilang saat revert.

## AUD-VERIFY-NIGHT (f): baseline baru penyebab mati - build ≥ 04:51, world non-PERF (26 run, 298 kematian, 10-10 05:04-21:49)
- moving_open 21%, static_open 20%, unseen_shooter 17% (+5% unseen_shooter+bleed), grenade 13%, bleed_no_medic 7%, vehicle_fire 7%, exposed_window 3%, in_cover 2%, in_cover_outgunned 2%, vehicle_collision 1%, cqb 1%, fratricide < 1%.
- bleed=1 sekarang tercatat pada 39 kematian (13%). Sebelum perbaikan logger: 0 dari 2097.
- unseen_shooter turun dari 38% (data lama yang tercampur finisher) ke 22%.
- Mati di tempat terbuka (moving_open + static_open) tetap 41%; di dalam cover hanya 4%. Ini masalah realisme terbesar yang masih ada.
- Jarak bunuh infanteri: p25 13 m, p50 29 m, p75 81 m, 43% di bawah 25 m. Campuran world termasuk 6 run CTX_URBAN (tempur kota jarak dekat).
- Untuk A/B berikutnya pakai alat bersama tools/dco-abmetrics.js (Realism), yang sudah memuat peringatan untuk sesi sebelum 04:51.

## AUD-VERIFY-NIGHT (a-d): BELUM terverifikasi
- Paket B (DEF_DEPTH untuk patrol, SEL_DAY untuk rearm/pickup) tidak terjadwal malam ini (konfirmasi BD).
- Run lane sejak 09:10 hanya CTX/PERF/VICTORY: 2 pickup_end (timeout), 0 rearm_end, 0 patrol_pt_skip.

## STATICGUARD - kalibrasi akhir dan hasil sesudah pengurangan (21:59)
- Satuan = satu `static ref map<string,int> = new map<string,int>()`. Kapasitas terukur (vanilla saja, pencarian biner):

| bentuk | kapasitas | biaya |
|---|---|---|
| new | 542 | 1,0 |
| array {1} | 361 | 1,5 |
| array 4 int | 181 | 3,0 |
| array 4 string | 180 | 3,0 |
| ekspresi 0.25*Math.PI | 1075 | 0,5 |
| literal | 8000+ | 0 |

  - Model: array = 1,0 + 0,5 per elemen; new = 1; ekspresi ≈ 0,5; literal 0.
  - Pool bersama satu modul, bukan per class: 500 new dibagi ke 20 class lolos, 2000 gagal.
- Sisa ruang setelah vanilla + DCO, diukur langsung:
  - sebelum pengurangan (21:37-21:41): 492-500 unit → beban DCO ≈ 46 unit (8,5%);
  - sesudah pengurangan Scripter 2 (21:59): 537-540 unit → beban DCO ≈ 2-6 unit (±1%).
  - Ruang untuk mod lain di server pemain naik ≈ 40 unit.
- Guard sekarang juga melaporkan "pool cost" per root (model di atas) dan gagal bila biaya naik. DCO sekarang 10 unit menurut model (terukur ≤ 6).
- Probe dijalankan di addon scratchpad terisolasi dan sudah dihapus. Satu insiden: probe pertama sempat ada di TestAI 21:32-21:33; tidak ada run lane yang terdampak (dicek).

## R4. Review K2 + OBJASSIGN + OBJFOCUS (Scripter, kode 22:00) → dikirim ke Scripter ±22:20
- Default (K2 off, mode assign 0, tanpa fokus, role off) hampir identik, dengan dua pengecualian:
  - DEFEND/HOLD_UNTIL ditambahkan ke typeBonus GetRequiredGroupCount TANPA switch → objektif bertahan butuh +1 grup (perubahan default);
  - spam WARN obj_assign_violation tiap think di world lama yang memakai blacklist lama.
- HIGH:
  - OBJ-1 DCO_OpPlan melewati filter assign/role → objektif dilepas lalu diambil lagi tiap think (tactic restart terus);
  - OBJ-2 (terverifikasi) cabang release untuk perubahan assign tidak pernah tercapai, karena IsObjectiveCommitted sudah false lewat IsCommanderBlackListed → grup yang dikeluarkan GM tetap ASSIGNED.
- MED:
  - OBJ-3 fokus paksa tanpa batas menyerah + log tiap think;
  - OBJ-4 handover attack_only bisa menunggu selamanya (cap staffing sektor per faksi);
  - OBJ-5 fokus tidak dicek terhadap role;
  - OBJ-7 indeks roster GM bisa menunjuk objektif/komandan lain bila roster berubah saat dialog terbuka;
  - OBJ-8 komandan di atas slot 24 / belum di roster terhapus dari list saat GM mengubah checkbox;
  - K2-1 (terverifikasi) K2 menyalakan inversi agresi ambush walau slider 1;
  - K2-2 K2 masih memakai AggrA lama di skor taktik dan consolidation hold.
- LOW: K2-3 retreat loss bisa negatif di slider rendah, K2-4 clamp 0,01, log dedupe, sisa OBJ-9.
- Bersih: pembagian nol, floor K2, bit mask (≤ 2^23), migrasi blacklist lama, null guard, satuan, Format, static guard, komentar.
- Tindak lanjut Scripter 22:12: semua HIGH/MED diperbaiki, kecuali 4 keputusan sadar.
  - Diperbaiki:
    - OBJ-1 DCO_MayAttack di OpPlan;
    - OBJ-2 cek sebelum IsObjectiveCommitted;
    - OBJ-3 throttle + give-up obj.focus_giveup_s 600;
    - OBJ-4 handover timeout obj.role_handover_s 300 untuk relief non-OFFENSIVE;
    - OBJ-5 fokus dibersihkan bila role memblok atau entity dihapus;
    - OBJ-6 throttle;
    - OBJ-7 RosterKey stale-check;
    - OBJ-8 MergeChecked;
    - K2-3, K2-4, K2-5, format.
  - Dibiarkan:
    - typeBonus DEFEND/HOLD_UNTIL. DIVERIFIKASI auditor: kedua nilai enum itu baru (snapshot 07:40 hanya CAPTURE/DESTROY/RECON), jadi objektif lama tidak terpengaruh dan default identik.
    - K2-1 (inversi ambush dalam paket K2, disengaja);
    - K2-2;
    - fokus melewati cap;
    - WARN obj_assign_empty sesaat.

## BANTU Scripter 2: spawner A12/A13/A17 - SELESAI (compile bersih 22:17, guard OK, belum dites in-game: butuh GM)
- File (TAHAN dari s2, SIAP dikirim 22:18): Commander/Spawner/DCO_GroupPresets.c, Settings/DCO_SettingsPresetsUI.c, Configs/DCO/GroupPresets/DCO_BuiltinGroupPresets.conf.
- A12 tabrakan file key:
  - FileKey untuk nama ASCII tidak berubah (file lama tetap cocok). Nama dengan byte non-ASCII mendapat akhiran hash 5 karakter, jadi nama Cyrillic yang panjangnya sama tidak bertabrakan;
  - path key lama tetap dibaca/dihapus hanya bila nama di dalam file sama, dan dipindah ke key baru saat disimpan (log group_preset_key_migrated);
  - PBNameTaken membandingkan file key (dialog Replace muncul untuk nama berbeda ber-key sama);
  - log group_preset_key_collision.
- A13 built-in:
  - "Recon team" ganda → "US Recon team" / "USSR Recon team";
  - FindBuiltin/Find(name, faction) dengan alias lama "Recon team" → "<faksi> Recon team" (log group_preset_builtin_alias);
  - nama user = nama built-in ditolak: "\"<name>\" is a built-in name. Pick another." (UI PBChecks + server save/copy/rename, log group_preset_refused reason=builtin_name).
  - Saran ke s2: DCO_CommanderSpawner.c:518 meneruskan faksi ke Find.
- A17 batas:
  - nama maks 40 karakter (ditolak di UI dan server, log reason=name_too_long; simpan-dari-grup dipotong + log group_preset_renamed);
  - daftar RplProp dibatasi ke limit (200 bila 0), dengan WARN group_preset_list_capped.

## BANTU Scripter 2: review A1/D3/A6 GlobalAI - BERSIH, 3 catatan dikirim 22:22
- Default yang direkam = atribut world; rotasi skenario antar world yang atributnya berbeda memigrasi key tidak-diedit setiap pergantian.
- Tulis JSON tidak atomik.
- 6 desimal (aman untuk default sekarang).

## BANTU BD (2): scan error run lane 21:24-22:16 - SELESAI 22:30
- 15 console.log + 17 dco.log. Alat: scratchpad lanescan.js; hasil lanescan.out.
- Baru:
  - VME "Failed move" di VICTORY 21:48:48 (waypoint 2 m tak terjangkau), dikirim ke ctx (pemilik test VICTORY);
  - VICTORY FAIL aborted t=61 + trace group hilang (US_GL_3/4, US_FT_4);
  - COMPILE_ERROR PANIC301 21:57 sementara (ReleaseGroupsFromObjective saat edit OBJ Scripter; retry 22:00 OK);
  - GenerateRandomPointInRadius(0,0) 2× di PANIC30: semua pemanggil DCO terjaga, kemungkinan vanilla, belum ada pemilik.
- Noise vanilla diketahui: SCR_MenuLayoutEditorComponent ×47, ResupplySupportStation catalog ×24.
- s2 22:25: catatan 1 diperbaiki (record default per world, DCO_GlobalConfig.defaults.<world>.txt, boot test 4/4). Saran A13 diterapkan (spawner Find(name, faksi)). Dokumen config diperbarui.

## BANTU BD (1): review kode BD (MoveBrain V2, CqbKindMove, ff.vert, TuneSyncBD, TestArea) - SELESAI 22:35
- Tanpa regresi di switch 0 (terverifikasi per jalur). TestArea dipulihkan dari backup, identik dan API cocok, sehingga "Unknown class DCO_TestArea" di base world hilang (temuan AUD-TESTAREA kemarin selesai).
- Temuan saat switch ON (dikirim ke BD):
  - MED: crawl memakai spot cache jauh + spot yang ditolak ikut di-cache;
  - MED: TakeSpot menimpa arah lateral/mundur;
  - MED: biaya FieldSpotV2 ≤ 46-70 trace per panggilan, dan gagal tidak di-cache;
  - LOW: cover_first vs CqbKindMove, counter turret_clear, kandidat posisi sendiri, blast_wait tanpa opt-in.

## TEMUAN HIGH (sampingan review): agg.profile efektif OFF di semua server
- DCO_GlobalTune.c:189 mendaftarkan default 1.0, tapi DCO_TuneCfg hanya menyimpan nilai config yang BERBEDA dari default, sementara ketiga pembaca (DCO_Stress.c:14/22, Modded_GroupUtilityComponent.c:1093) memakai fallback 0.0. Hasilnya OFF walau GM menampilkan ON.
- Dikirim ke AI Design. A/B AGG yang mengandalkan default ON perlu dicek ulang.
- ALAT BARU tools/dco-tune-check.js: membandingkan default registrasi DCO_TuneCfg dengan semua fallback pembaca (DCO_TuneF/TuneG/TuneFor*/DCO_Stress.T); exit 1 bila ada yang berbeda. 462 key → hanya agg.profile.
- AI Design 22:27: ketiga pembaca agg.profile sekarang fallback 1.0; dco-tune-check 0 mismatch (diverifikasi auditor). Hasil lama: A/B AGG2 10-10 memakai m_aTune (benar ON); uji config slider 2.5 lewat JSON berjalan dengan agg.profile OFF; lane malam ini memakai profil utama yang menyimpan 0.0 eksplisit (OFF di kedua arm, konsisten). Registrasi legacy-default tune_agg_profile di tabel s2 menyusul.
- BD 22:28: temuan 1-6 diperbaiki (crawl tanpa TakeSpot, TakeSpot hanya FORWARD + gain ≥ gain_min, gagal di-cache 2 s, LOS hanya top-3 lewat Los(), CUSTOM_POS terpilih tidak diubah CqbKindMove, vert_clear untuk infanteri, kandidat < 1,5 m dilewati). Temuan 7 disengaja (bagian perbaikan mb.on, diuji di arm AGGFM).

## A/B spec.assist_commit_s (BD lane, 1 dari 3 pasang selesai 22:44)
- B1 PERF_STRESS (commit ON = default 6 s), session_212507: 79 relocate mg_assistant, superseded_by_mg_assistant 8 (10%), rata-rata moved 6,3 m, left 32,7 m.
- ASSIST01 (commit 0), session_223320, itest_tune spec.assist_commit_s 0 terpasang: 145 relocate, superseded 41 (28%), moved 6,0 m, left 26,6 m.
- Sementara searah dengan dugaan: commit memangkas churn order ulang ±70% dan jumlah relocate hampir separuh. Sisa jarak lebih besar di ON. Belum konklusif; menunggu B2/B3 + ASSIST02/03.

## CFGFACTION #170 (Scripter 2): dokumen config diperbarui (bagian "Per-faction values"), review sedang berjalan

## R5. Review ronde 2 untuk Scripter (dikirim 22:5x)
- OBJPHASE (obj.phase_on, OFF identik):
  - HIGH: jalur exploit/axis (DCO_CommanderOps NextOnAxis/PlanAxis) melewati gate fase dan delay;
  - HIGH (terverifikasi): Anchor DCO_OpPlan.c:378 mempromosikan primary terkunci → op end/new tiap think;
  - HIGH (terverifikasi): "lost by first owner" mengunci pemilik pertama "" (netral) → gate tertutup permanen, atau pemilik pertama salah tercatat;
  - MED: recapture tanpa cek fase;
  - LOW: dependency dihapus = met tanpa log, volume obj_gate_wait, atribut GM clamp 10 vs 20.
- V5/D7 (tac.fix_los, tac.standoff_fix, veh.sbf; OFF identik):
  - HIGH (terverifikasi): hold standoff low_conf return sebelum timeout 900 s → SIEGE/ABF bisa tertahan selamanya;
  - MED: scope-fix tidak pernah melihat ARMOR (filter armor=false), ArmedCrew tidak cek kursi turret terisi;
  - LOW: situs FixLos tanpa cek air/navmesh/ancaman, kendaraan hilang saat SBF tanpa re-plan, alasan log no_vehicle.

## R6. Review CFGFACTION #170 untuk Scripter 2 (dikirim 22:5x)
- MED:
  - key global/doktrin di blok faksi di-log applied tapi tanpa efek;
  - build awal gagal tanpa retry dan memblok rebuild;
  - (terverifikasi) simpan GM setelah parse gagal bisa menghapus file asli admin lewat rotasi .bak.
- LOW: factions bukan objek dibuang diam-diam, entri faksi buruk membangun layer, simpan GM menimpa edit disk.
- Bersih: parser, trap 8 KB, write-back, urutan layer, tanpa blok = identik.
- Spawner UI + CheckAssignEmpty (bagian 3, tanpa HIGH):
  - MED: catatan "pool only" bisa menempel di pesan yang salah;
  - MED (terverifikasi): teks "Weight is ignored" salah, karena ChoiceScore masih +0,25 × weight;
  - LOW: halaman komandan hanya membaca spawner pertama, sideH 560 terlalu kecil (±604), CallLater CheckAssignEmpty tidak di-Remove di OnDelete, 4 nit teks.
- s2 23:00: F-1..F-5 diperbaiki (global_only, retry build 10×1 s tanpa holder yatim, simpan GM ditolak saat parse_fail / factions_not_object, validasi entri faksi), F-6 di README; boot 3/3. Blok di akhir file 18 KB terbaca (bytes=17938). Dokumen config diperbarui.
- 23:1x: dokumen config 595 key, semuanya berdeskripsi (80 deskripsi baru ditelusuri dari kode). Termasuk key C3 milikku yang sudah didaftarkan ke config (break_contact_dogleg_*, break_contact_face_*).
- Scripter 23:1x: ronde 2 diperbaiki (shadow compile bersih, guard OK, tune-check 0).
  - OBJPHASE P-1..P-7 semua;
  - V5/D7 V-1 (tac.standoff_max_s 600 baru), V-2, V-3 (HasGunner), V-4, V-6;
  - spawner S-1..S-5 + nit.
  - Belum: V-5 (re-plan saat kendaraan hilang di SBF), nit preview pool vs Label().

## R7. Bantu AI Design (1) review kode + (2) verifikasi log - SELESAI 23:2x
- (2) CMDLEARN + THREAT-DOSSIER di 2 sesi komandan sah (DEF_AREA_GAR 22:16, DEF_AREA 22:43): tidak ada spam approach destroyed, world_switch benar 0, assess/ineffective tanpa churn, cap 1, decoy 0. tactic_select_learn 0 karena world test memakai ForceSelect (bukan bug). LOW: Tactics.json ditulis tiap 300 s walau entries=0.
- (1) GRPTASKFLAGS: mask default 511 tidak pernah memblok.
  - MED (terverifikasi): PickGroup fallback AnyCombat (31) membuat squad Defend Only / Recon Only bisa dipakai untuk serbuan RAID/RIF/Refill;
  - MED: gate garrison void → objektif tanpa garrison dengan mask non-default;
  - 4 LOW.
- CMDLEARN staging (always ON):
  - MED: sampel negatif dobel pada release normal;
  - MED (terverifikasi): episode "clear" tidak pernah tercatat karena urutan ResetSyncState;
  - LOW: reward condong ke spot yang sama, penggunaan tanpa log.
- SquadChatter bersih.
- AGGFIX bersih di default; LOW: floor wait P7 2 s bisa memperpanjang, retreat_thr absolut, hysteresis postur memakai agresi global.
- AI Design 23:10: G-1 (RoleJob eksplisit), G-2 (Allows(48) di selector), L-1 (dedupe), L-2 (urutan), L-3 (explore 12%), L-4 (LogUse sekali), A-1, A-2, preempt gate dipindah. Dibiarkan: A-3 (hysteresis global, terbatas), redundansi 13:558, biaya TeamOk/EffMask. Cek auditor: tune-check 0, guard OK.

## AUDITLOG2: selisih unit mati vs death_ctx (laporan Realism) - DIPERBAIKI 23:30
- Bukan death_ctx yang hilang: unit_summary alive=0 juga mencakup unit TIDAK SADAR saat run berakhir (life_s = durasi run, cause "-"). Contoh session_221134: 8 unit.
- Benchmark/DCO_Battle.c: field baru unit_summary state=alive|uncon|dead|gone|dead_unlogged (alive tetap). Compile bersih, guard OK. Realism disarankan menghitung deaths dari state=dead.
- FITUR/SETTING BARU: field log state= di unit_summary/battle.json.

## Pemantau flip default (aturan BD: flip 0→1 hanya setelah 02:25)
- Snapshot 998 default (DCO_TuneCfg + atribut m_b/m_f) pukul 23:30 (scratchpad defsnap2.js + defsnap_2330.json), dicek tiap 5 menit sampai 02:25.
- Sejak 07:40 hanya agg.profile yang berubah (0 → 1). Baru efektif setelah perbaikan fallback 22:27, tapi lane tidak terkontaminasi karena profil utama menyimpan 0.0 eksplisit.

## A/B spec.assist_commit_s - SELESAI (3 vs 3, lane BD, PERF_STRESS, build sama, profil utama)
- ON (default 6 s): B1 session_212507, B2 224442, B3 235841. OFF (ASSIST0, m_aTune 0): 223320, 234803, 2026-10-11/010210.
- Metrik asisten:
  - order ulang (superseded_by_mg_assistant): ON 10/25/16% (rata-rata 17%), OFF 28/28/37% (rata-rata 31%) → churn turun ±45% relatif;
  - relocate per run ±106 vs ±104; arrived total 31 vs 20.
- Metrik standar no-regression (tools/dco-abmetrics.js), ON vs OFF:
  - mati 74 vs 78,7;
  - mati di terbuka 68,7 vs 71,7;
  - FF 6,7 vs 10,3;
  - mati diam tiarap 25 vs 31,3;
  - musuh terlihat sebelum mati 73% vs 66%;
  - kills 67,3 vs 68,3;
  - mati saat panik 17,3 vs 14,3;
  - kematian pertama 110 s vs 85 s;
  - median jarak bunuh 15,3 vs 15 m.
- VERDICT: tanpa regresi; ON lebih baik atau setara di hampir semua metrik. Default 6 s DIPERTAHANKAN (sudah default). Satu catatan: mati saat panik sedikit lebih tinggi pada ON (17 vs 14), masih dalam variasi run.
- Research (analis A/B bersama) memberi verdict yang sama: OFF = regresi (FF +54% hampir semua ledakan, kematian US +34%), jadi default 6 dipertahankan. Data auditor: FF tambahan di OFF hampir semua granat tangan (FRAGMENTATION) dari role at/leader dalam 3-18 m; mekanisme (asisten MG bolak-balik di zona granat rekan) belum terbukti. Catatan alat: dmg≠KINETIC termasuk COLLISION.
- FF granat: saran Research diteruskan ke AI Design → switch baru gren.ff_predict (default 0, GM 'Grenade: Predict Friendly Movement'): radius + sebaran (10% jarak) + 8 m, posisi kawan 3,5 s ke depan, log gren_ff_block DEBUG. OFF identik. Perlu A/B blok berikutnya. Jalur lempar vanilla belum ditelusuri.
