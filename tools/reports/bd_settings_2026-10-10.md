FITUR/SETTING BARU (BD, 2026-10-10 malam)

Semua key ada di file config server `$profile:DCO/DCO_GlobalConfig.json`.
- Key otomatis ditambahkan ke file kalau belum ada (file lama disalin ke .bak).
- Log server mencatat "N setting di-override dari <path>".
- Nilai di luar rentang otomatis dipotong (clamp) ke rentang yang sah.

1. Team Bound — maju bergantian dengan jabat tangan (handshake)
   - Apa: fireteam dalam satu regu maju bergantian. Tim baru boleh maju kalau tim lain sudah berhenti di posisi DAN (bila musuh diketahui) sudah menembak. Sebelumnya giliran ditentukan jam setiap 6–12 detik.
   - Cara kerja: status tiap tim dicek setiap 0,5 detik dari posisi anggota (bergerak > 0,6 m) dan waktu tembakan terakhir. Giliran diberikan ke tim yang paling lama tidak maju. Giliran selesai saat tim sudah maju lalu berhenti (set), tidak bergerak 4 detik (idle), atau setelah 12 detik (timeout). Kalau menunggu lebih dari 10 detik, giliran tetap diberikan (deadlock).
   - Tembakan perlindungan: selama tim lain bergerak, tim pelindung terus menembak posisi terakhir musuh sampai 30 detik setelah musuh terakhir terlihat (biasanya hanya 5–12 detik).
   - Key:
     - tune_tb_on — 0 (mati) / 1 (nyala), default 0.
     - tune_tb_set_s — default 1,5, rentang 0,5–6. Detik diam sebelum tim dianggap "set".
     - tune_tb_fire_s — default 4, rentang 1–15. Tembakan dalam N detik terakhir dihitung sebagai "melindungi".
     - tune_tb_bound_max_s — default 12, rentang 3–40. Batas lama satu bound.
     - tune_tb_idle_s — default 4, rentang 1–20. Giliran dilepas kalau tim tidak bergerak.
     - tune_tb_deadlock_s — default 10, rentang 3–60. Batas tunggu sebelum maju tanpa perlindungan.
     - tune_tb_ow_sup — default 1, rentang 0–1. Tembakan perlindungan saat tim lain bergerak.
   - Contoh efek: tune_tb_deadlock_s 5 membuat regu lebih cepat maju meski tidak dilindungi. tune_tb_fire_s 2 membuat syarat perlindungan lebih ketat.
   - Log: tb_turn (INFO) dan tb_end (DEBUG). Counter: tb:turn_set_covered, tb:turn_set_no_enemy, tb:turn_deadlock, tb:wait_set, tb:wait_cover, tb:end_set/idle/timeout, tb:ow_suppress.

2. Target Claim — pembagian target dalam regu
   - Apa: anggota regu tidak semuanya menembak musuh yang sama. Target yang sudah diambil kawan diberi penalti skor, sehingga penembak memilih musuh lain.
   - Cara kerja: setiap regu mencatat target tiap anggota (kedaluwarsa 3 detik). Penalti 2,5 poin per kawan, maksimal 2 kawan. Pengecualian:
     - musuh yang sedang menembak saya tetap prioritas;
     - MG tidak terkena penalti (boleh menumpuk pada sasaran berbahaya).
   - Key:
     - tune_tgt_claim — 0/1, default 0.
     - tune_tgt_claim_pen — default 2,5, rentang 0–10.
     - tune_tgt_claim_ttl — default 3, rentang 0,5–10 detik.
   - Contoh efek: tune_tgt_claim_pen 6 membuat tembakan menyebar sangat kuat; 1 hanya berpengaruh saat skor dua target seimbang.
   - Log: tgt_claim_active (INFO, sekali per sesi) dan tgt_claim (DEBUG). Counter: tgt:claim_spread, tgt:claim_same.

3. Move Brain — keputusan gerak berbasis pemicu (menggantikan timer acak)
   - Apa: kapan prajurit pindah posisi ditentukan pemicu, bukan timer acak. Sebelumnya prajurit di cover menunggu 30–90 detik × 2 (sampai sekitar 3 menit).
   - Cara kerja (dievaluasi setiap 0,5 detik):
     - Pindah segera kalau cover tertembus: tidak menutupi dari arah musuh, cover lunak sedang ditembaki, atau ≥2 tembakan dekat dalam 3 detik. Ini juga berlaku untuk grup bertahan.
     - Penyerang (postur agresif, waypoint S&D, atau tugas ATTACK/FLANK/REINFORCE) maju setelah minimal 3 detik di posisi, dengan syarat:
       - dilindungi (giliran Team Bound, atau kawan menembak dalam 3 detik terakhir), ATAU musuh diam lebih dari 6 detik;
       - saat tekanan sedang, butuh dilindungi DAN sudah 5 detik di posisi;
       - saat pinned, tidak maju;
       - kalau tidak ada pemicu, tetap maju setelah 25 detik.
     - Grup bertahan tetap memakai perilaku lama, kecuali cover-nya tertembus.
   - Key:
     - tune_mb_on — 0/1, default 0.
     - tune_mb_commit_s — default 3, rentang 0,5–15. Minimal detik di posisi sebelum maju; mencegah maju-mundur.
     - tune_mb_med_wait_s — default 5, rentang 1–30.
     - tune_mb_atk_fallback_s — default 25, rentang 5–180. Semakin kecil, semakin agresif.
     - tune_mb_comp_hits — default 2, rentang 1–10. Jumlah tembakan dekat yang dianggap posisi tertembus.
   - Contoh efek: tune_mb_atk_fallback_s 10 membuat penyerang sangat agresif; 60 lebih hati-hati.
   - Log: mb_active (INFO, sekali per sesi) dan mb_decide (DEBUG). Counter: mb:move_cover_no_block, mb:move_spot_hit, mb:move_covered, mb:move_lull, mb:move_fallback, mb:stay_commit/pinned/not_turn/wait.

4. Field Spot — titik terlindung di lapangan saat cover vanilla kosong
   - Apa: saat bergerak, prajurit menuju titik lapangan yang benar-benar melindungi dari arah musuh (cekungan, batu, pohon, pagar), bukan berhenti di tempat terbuka. Di tes, 87% pencarian cover vanilla di lapangan kosong.
   - Cara kerja: ada 9 kandidat, yaitu titik tujuan plus 8 titik di sekelilingnya dengan radius 4 m. Setiap kandidat dicek dengan jejak tembak ke arah musuh setinggi tiarap, jongkok, dan berdiri. Kandidat dicoret kalau kurang dari 4 m dari kawan. Dipilih kandidat dengan perlindungan tertinggi, lalu sikap akhirnya diatur: tiarap kalau hanya terlindung rendah, jongkok kalau setinggi jongkok. Cover vanilla di dekat titik itu tetap dipakai kalau ada.
   - Key:
     - tune_mb_field — 0/1, default 0.
     - tune_mb_field_ring_m — default 4, rentang 1–10.
     - tune_mb_field_spacing_m — default 4, rentang 1–10.
   - Log: mb_field (DEBUG). Counter: mb:field_hit_prone/crouch/stand, mb:field_miss. Section perf: mb_field.

5. CQB Indoor / Outdoor (sebelumnya)
   - tune_tm_cqb_aggr_in — default 4, rentang 1–8.
   - tune_tm_cqb_aggr_out — default 4, rentang 1–8.
   - Fungsinya: membagi jeda gerak saat musuh berada dalam tune_tm_cqb_range_m (25 m), dengan nilai berbeda untuk di dalam gedung dan di luar.

6. Squad F&M — fire & maneuver di dalam regu (tb.fm, dibangun di atas Team Bound)
   - Apa: regu penyerang yang kontak di jarak 50–300 m membagi peran:
     - tim yang memegang MG/AR menjadi base of fire: diam dan terus melindungi;
     - tim lain bermanuver lewat samping ke titik flank (tegak lurus garis base→musuh, jarak 30–80 m dari musuh), bound per bound, hanya saat base sedang menembak.
     - Saat tim manuver tinggal ≤40 m dari musuh: semua tim boleh maju (serbu).
     - "Angkat tembakan" terjadi otomatis lewat cek jalur tembak kawan yang sudah ada.
   - Key:
     - tune_tb_fm — 0/1, default 0. Butuh tune_tb_on 1.
     - tune_tb_fm_min_m — default 50, rentang 10–200.
     - tune_tb_fm_max_m — default 300, rentang 100–600.
     - tune_tb_fm_assault_m — default 40, rentang 10–100.
     - tune_tb_fm_lost_s — default 30, rentang 5–120. F&M dibatalkan kalau musuh tidak terlihat selama N detik.
   - Log: sfm_start (INFO: base, man, side, flank, enemy), sfm_assault, sfm_end (why=off|lost|team_lost) dan sfm_step (DEBUG). Counter: sfm:start, sfm:assault, sfm:flank_step, sfm:end_*.
   - STATUS: kompilasi bersih, BELUM dites (tidak ada lane tersisa malam ini).

PEMIKIRAN ULANG (re-reasoning): apa yang membuat DCO jadi "WoW"
Dasarnya: audit kode 3 lapis + data 157 sesi tes (~2.160 menit).
- Otak tembak sudah dalam: rata-rata L3 (persepsi, prioritas target, model bidikan, morale).
- Otak gerak masih dangkal: L1–L2 (timer acak, cover vanilla kosong 87%).
- Kerja sama tim hampir kosong: L0–L1 (bounding berbasis jam, tidak ada F&M dalam regu, tidak ada pembagian target).

Yang membuat pemain berkata "WoW" adalah perilaku tim yang TERLIHAT:
- satu tim menembak sementara tim lain maju;
- ada yang memutar ke samping;
- setiap prajurit punya sasaran sendiri;
- prajurit selalu berhenti di balik sesuatu, bukan di tanah lapang.
Itulah fokus malam ini:
1. Team Bound (handshake + tembakan perlindungan) → saling melindungi yang terlihat.
2. Target Claim → tembakan menyebar seperti regu sungguhan (pola frontal/silang doktrin).
3. Move Brain → maju saat dilindungi (agresif tapi tidak bunuh diri); pindah segera kalau cover tertembus.
4. Field Spot → berhenti di titik yang benar-benar melindungi dari arah musuh.
5. Squad F&M (tb.fm) → base of fire + manuver memutar + serbu. Siap dites.

Temuan yang perlu ditindaklanjuti:
- Pencarian kontak tercatat "gagal" sekitar 90%, tapi hampir semuanya terjadi saat mendekati titik. Penyebabnya bounding investigate grup bertukar peran setiap sekitar 1 detik sehingga perintah investigate dibatalkan berulang. Perbaikannya sudah ada (osc.fix_bound, minimal 4 detik per bound), tapi masih mati dan perlu A/B.
- Granat hanya 8% berhasil. Penolakan karena ancaman tinggi (aturan vanilla FRAG_GRENADE_MAX_THREAT) terjadi sekitar 12 kali per menit, justru saat jarak dekat, ketika granat paling berguna. Usulan: saklar terpisah untuk melonggarkan ambang ini bagi penyerang dalam 35 m.
- Penghentian tembakan karena cek jalur kawan terjadi 108 kali per menit (48 karena grup lain, 38 karena anggota sendiri). Lebar jalur perlu dikaji: tan 0,035 berarti sekitar 10 m di jarak 300 m.
- Banyak lapisan yang sudah dibangun masih mati: ctx.on, cov.on, fm.auto, osc.fix_*, opp.*. Tiap lapisan perlu A/B 3 run, lalu dinyalakan kalau tidak ada regresi.
- Ide berikutnya (belum dibuat): suara callout tim ("Bergerak!", "Lindungi!", "Siap!") yang dipicu tb_turn / sfm_start, supaya kerja sama tim juga TERDENGAR oleh pemain. Ini bisa jadi efek "WoW" paling murah.

7. Tambahan untuk Move Brain (mb.on), 04:42
   - Pengecualian "maju saat dilindungi" di gerbang AdvanceAllowed. Sebelumnya, begitu stres level 1 ("under_fire"), semua gerak maju ditolak. Ini penghambat maju nomor 1, bahkan di agresi 3 (data Features Auditor).
   - Sekarang: penyerang di level stres 1 boleh maju kalau ada kawan yang menembak dalam 3 detik terakhir. Saat pinned (level 2) tetap tidak boleh. Batas jarak minimal (too_close) tetap berlaku.
   - Counter: tm:adv_mb_covered.
   - Arah: keputusan maju dari Move Brain tidak lagi diubah jadi mundur (counter mb:dir_fix_forward).

8. Close Assault Grenades (gren.atk_close), 04:26
   - Key:
     - tune_gren_atk_close — 0/1, default 0.
     - tune_gren_atk_close_m — default 35, rentang 10–60.
     - tune_gren_atk_threat_mul — default 2, rentang 1–5.
   - Apa: penyerang yang musuhnya ≤35 m tetap boleh melempar frag walau ancaman melebihi batas vanilla (sampai 2× batas itu), tapi tidak saat panik.
   - Counter: gren:atk_close_allow.
   - STATUS: kompilasi bersih, belum dites.

9. Atribut GM (Global Combat): Squad Bounding Handshake, Squad Fire and Maneuver, Fire Distribution, Trigger-Based Movement, Field Cover Spots, Close Assault Grenades.
   - Teks bantuan ditambahkan oleh Scripter 2.
   - Tampilan di UI GM belum dicek.

10. Perbaikan bug: titik acak kohesi dengan radius 0 menghasilkan (0,0,0), sehingga prajurit lari ke pojok peta. Sekarang radius minimal 2 m (Modded_CombatMoveLogic_Attack.c).

11. tune_tb_set_frac — default 0 (perilaku lama), rentang 0–0,75. Ditambahkan 07:09.
   - Apa: porsi anggota tim yang boleh masih bergeser sementara tim tetap dianggap "set". Nilai 0,34 berarti 1 dari 3–4 anggota boleh bergeser.
   - Alasan: di run AGG #1, handshake sering menunggu "set" (tb:wait_set 1272 per run) dan giliran akhirnya diberikan lewat deadlock (105 per run). Satu anggota yang bergeser kecil membatalkan status "set" seluruh timnya.
   - Rekomendasi nilai tes berikutnya: 0,34 (sudah dimasukkan ke world PERF_STRESS_AGGFM).

12. Callout suara untuk Team Bound dan F&M (07:51)
   - Saat giliran bound diberikan dengan perlindungan, dan saat F&M masuk fase serbu, anggota pertama tim berteriak "Moving!" dan satu kawan yang sedang menembak membalas "Covering!".
   - Memakai DCO_Opp.BoundCallout buatan AI Design (jeda 20 detik per regu, tune opp.bound_callout).
   - Counter: tb:callout. Ikut mati bersama tb.on.

HASIL TES — blok A/B bersama PERF_STRESS (build 06:3x, 10 menit per run, sekitar 119 AI)
| Arm | n | Tewas | Terbuka | Tembak kawan | Panik | Peluru/unit | Hits | Mati pertama (detik) | Cover % |
| Baseline | 3 | 69 | 61 | 4 | 13 | 46,3 | 806 | 100 | 10,2 |
| AGG (tb+claim+mb+field) | 2 | 71 | 66 | 8 | 15 | 41,4 | 820 | 90 | 8,3 |
| CLOSEIN (Realism) | 3 | 67 | 61 | 4 | 13 | 43,7 | 724 | 94 | 10,1 |
| PANIC30 (Realism) | 2 | 60 | 51 | 7 | 9 | 46,7 | 609 | 111 | 11,9 |
| AGG2 (AI Design) | 2 | 72 | 63 | 2 | 15 | 51,2 | 781 | 106 | 12,4 |
| OPP (AI Design) | 2 | 75 | 67 | 9 | 18 | 41,7 | 997 | 109 | 10,9 |
- AGG: semua fitur terbukti aktif. Counter per run: mb:move_covered 113, mb:move_cover_no_block 47, tb:ow_suppress 927, tgt:claim_same 968, field hit 57 / miss 83, tb:turn_deadlock 104, tb:wait_set 1157.
- Keputusan: AGG tetap MATI (default 0), sesuai aturan tanpa regresi. Tidak ada perbaikan jumlah tewas. Tembak kawan sendiri 8 vs 4, dan 14 dari 16 kasus adalah ledakan granat/roket kawan dalam 8–32 m: prajurit maju ke radius ledakan kawan sendiri.
- Langkah berikutnya untuk AGG:
  1. tb.set_frac 0,34 (sudah ada) supaya handshake tidak jatuh ke deadlock.
  2. Move Brain menghindari titik jatuh granat/roket kawan selama sekitar 5 detik.
  3. Perluas kandidat Field Spot (ring kedua 8 m, plus titik di samping).
  4. Uji ulang 3+3 lawan baseline. World PERF_STRESS_AGGFM sudah siap (termasuk tb.fm dan set_frac 0,34).
- Sesi: lihat D:/Git/DCOReforger/tools/reports/bd_block_result_2026-10-10.txt

BUKTI CONFIG SERVER (end-to-end) — LULUS 07:52
- Metode: world PERF_STRESS biasa TANPA m_aTune, profil server privat (-profile cfgprof_bd, -dcoTest). Semua fitur dinyalakan HANYA lewat profile/DCO/DCO_GlobalConfig.json: tune_tb_on 1, tune_tgt_claim 1, tune_mb_on 1, tune_mb_field 1.
- Hasil:
  1. Run tanpa file → file config dibuat otomatis, dan semua key baru tertulis dengan nilai default (05:26).
  2. Run berikutnya membaca 453–454 setting dari file (cfg_summary unused=0).
  3. Key yang ditambahkan belakangan (tune_tb_set_frac) otomatis digabung ke file lama (backup .bak).
  4. Dengan nilai 1 di file: tgt_claim_active muncul di T+48 detik, mb_active di T+54 detik, dan baris tb_turn muncul. Jadi nilai config benar-benar terbaca dan mengubah perilaku.
- Session: scratchpad cfgprof_bd/profile/DCO/Logs/2026-10-10/06-12/session_075114.
- Atribut GM (in-game) untuk keenam saklar ada di Edit.conf (kategori Global Combat). Tampilan di UI belum dicek; Scripter 2 tidak dapat izin kontrol layar.

13. Perbaikan AGG setelah hasil blok (07:53, kompilasi bersih, BELUM dites)
   - Hindari radius ledakan kawan (bagian dari mb.on):
     - Setiap granat dilempar (NotifyGrenadeThrown), titik sasarannya dicatat per regu. Titik sasaran = target lemparan, atau posisi terakhir musuh yang sedang dibidik.
     - Penyerang tidak maju selama mb.blast_s (6 detik) kalau ia berada dalam mb.blast_m (20 m) dari titik itu (counter mb:stay_blast_wait).
     - Field Spot juga menolak kandidat dalam 15 m dari titik itu.
     - Key: tune_mb_blast_m (default 20, rentang 0–40; 0 = mati), tune_mb_blast_s (default 6, rentang 0–15).
   - Ring kedua Field Spot: 8 kandidat tambahan di radius mb.field_ring2_m. Hanya dicek kalau ring pertama tidak menemukan perlindungan setinggi jongkok.
     - Key: tune_mb_field_ring2_m (default 0 = mati, rentang 0–15; rekomendasi 8).
   - World uji berikutnya, world/Test/Perf/DCO_T_PERF_STRESS_AGGFM: tb.on, tgt.claim, mb.on, mb.field, tb.fm, tb.set_frac 0,34, mb.field_ring2_m 8. Jalankan 3× vs 3× baseline.
