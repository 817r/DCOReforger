LAPORAN DCO REALISM — shift malam 2026-10-10 (03:50–09:40)

A. YANG SELESAI
1. Perbaikan bug CQB retry-loop
   - Gejala (log 10-09, TAC_RIF session_175120, juga PURSUIT dan MTC): satu grup memulai CQB pada gedung yang sama sampai 10 kali, kira-kira tiap 60 detik. Setiap kali langsung batal dengan alasan plan_no_outside_door atau plan_empty (gedung tidak punya pintu luar di graph navigasi). Grup terjebak siklus mulai-batal dan tidak ikut bertempur.
   - Akar masalah:
     a) DCO_CQBActivity.Abort selalu memberi jeda ulang 60 detik, termasuk saat gagal karena struktur gedung, yang tidak akan berubah.
     b) DCO_BuildingClear.FindContactBuilding tetap mengembalikan gedung yang masih dalam masa jeda, jadi grup tidak pernah memilih gedung lain.
   - Perbaikan:
     a) Abort dengan alasan plan_* sekarang memberi jeda cqb.plan_fail_retry_s (default 600 detik).
     b) FindContactBuilding melewati gedung yang masih dalam masa jeda.
   - Log: cqb_abort ... retry_s=; counter cqb:plan_fail_long_retry dan cqb_skip:contact_retry.
   - File: Group/DCO_CQBActivity.c, Group/DCO_ClearBuilding.c. Compile bersih 04:06.
2. Setting realisme bisa diatur dari config server
   - Sebelumnya 22 setting realisme (panik, morale, suppression, persepsi, lama di cover, CQB retry) tidak ada di file config server, jadi admin tidak bisa mengubahnya.
   - Sekarang terdaftar (total 30 key setelah close-in, jarak berhenti, dan tembakan balasan ditambahkan) di kategori "Realism" di $profile:DCO/DCO_GlobalConfig.json.
   - File baru: Global/DCO_GlobalTuneRealism.c.
   - Log saat start: realism_cfg panic_th=.. panic_max_s=.. cover_hold=.. identify=.. supp_drop=.. alert_heard_s=.. area_fire=.. cqb_retry_s=.. event_half_s=..
   - DIUJI:
     a) Server dengan profile kosong otomatis membuat file config, dan 22 key barunya muncul.
     b) Nilai di file diubah (morale_panic_threshold 3.1, cover_hold_mul 3.0), server dijalankan lagi, dan log membaca panic_th=3.1 cover_hold=3. Nilai config benar-benar terbaca dan dipakai.
3. Audit "setting tanpa efek"
   - 272 key tune di config server: semua dibaca kode. 13 key opp.* dan fm.* dibaca lewat nama yang disusun saat runtime.
   - 316 atribut tune di menu GM: semua efektif. Sebagian lewat setter langsung ke komandan, objective, artileri, threat response, dan cohesion.
   - Hasil: tidak ada setting yang tidak berefek.
4. Saklar agresi penyerang "close-in" (attacker_close_in / tm.atk_close_in, default 0 = tidak mengubah perilaku)
   - Masalah (keluhan "AI kurang agresif"):
     a) DCO_Engage.AdvanceAllowed menahan gerakan maju begitu musuh lebih dekat dari MinDist: aggressive 30 m, balanced 50 m, defensive 80 m. Ini berlaku juga untuk grup yang sedang menyerang.
     b) Di PERF_STRESS 10-09, gerbang too_close menahan maju 339 kali per run.
     c) Arm AGG milik BD juga tidak bisa melewati gerbang ini, karena penyerang di cover dalam MinDist tetap tidak maju.
   - Perbaikan:
     a) Untuk grup penyerang (task ATTACK/FLANK, atau grup tanpa komandan dengan waypoint Search & Destroy), MinDist dikali tm.atk_min_dist_mul (0.4). Posture balanced jadi 20 m, sehingga mendesak sampai jarak serbu, dan CQB push mengambil alih di bawah 25 m.
     b) Gerbang "under_fire" tidak diubah: prajurit yang sedang ditekan tembakan tetap tidak maju, jadi tidak bunuh diri.
   - Jarak dasar 30/50/80 m sekarang juga bisa diatur dari config server: advance_stop_dist_aggressive_m / advance_stop_dist_balanced_m / advance_stop_dist_defensive_m. Pengali untuk penyerang: attacker_stop_dist_mul.
   - Log: advance_gate why=close_in; counter tm:adv_close_in. Saat start, realism_cfg mencatat close_in= stop_mul= stop_agg= stop_bal= stop_def=.
   - File: Individual/DCO_Engage.c (Attacker, MinDist, BaseMinDist, cabang close_in di AdvanceAllowed). Compile bersih 04:19. World tes baru: DCO_T_PERF_STRESS_ABL_CLOSEIN.
   - Tes: A/B 3× PERF_STRESS_ABL_CLOSEIN vs 3× BASE di blok BD (hasil di B2).
5. Audit lanjutan: 95 setting non-tune di config server (aimAccuracy, dodgeChance, cqbEnabled, share*, medic*, dan lainnya) semuanya dipakai kode lewat getter.

B. YANG DITES DAN HASILNYA
1. Tes config server (profile pribadi, server start pendek):
   - File config otomatis memuat 22 key Realism.
   - Setelah morale_panic_threshold=3.1 dan cover_hold_mul=3.0 ditulis, log menampilkan realism_cfg panic_th=3.1 cover_hold=3.
   - LULUS: nilai config benar-benar dibaca dan dipakai.
   - Tes ulang 09:31 untuk 8 key yang ditambahkan belakangan (session_093144, profile pribadi). Di JSON ditulis attacker_close_in=1, attacker_stop_dist_mul=0.5, advance_stop_dist_aggressive_m=25, advance_stop_dist_balanced_m=40, advance_stop_dist_defensive_m=70, morale_panic_max_s=45. Log: realism_cfg ... panic_max_s=45 close_in=1 stop_mul=0.5 stop_agg=25 stop_bal=40 stop_def=70. LULUS.
   - Profile game bersama (07:53) sudah otomatis memuat semua 30 key Realism, termasuk return_fire_unseen_*.
   - Bukti bahwa nilai itu mengubah perilaku: di blok PERF, tm.atk_close_in=1 menurunkan penahanan too_close 509 → 40 dan close_in lolos 140× (lihat B2). Nilai JSON dan GM masuk ke penyimpanan tune yang sama (DCO_TuneGet).
   - Compile tree terbaru 09:31 bersih (3.8 detik).
2. Blok PERF bersama (dijalankan antrean BD, Logs/2026-10-10/06-12, build sama untuk semua arm). Run: BASE ×3 (063741_2, 065839, 074034_2), PANIC30 ×2 (064809_2, 073006), CLOSEIN ×3 (063741, 071940_2, 074034).
   Rata-rata BASE / PANIC30 / CLOSEIN:
   - korban 68.7 / 59.5 / 67.3
   - mati di tempat terbuka 60.7 / 51 / 60.7
   - tiarap diam saat mati 25.7 / 23 / 25.7
   - mati saat panik 12.7 / 8.5 / 13
   - lama panik (median) 64 / 30 / 135 detik
   - fratricide 3.7 / 6.5 / 3.7
   - median jarak bunuh 13.7 / 16 / 15.7 m; di bawah 25 m 71 / 64.5 / 67%
   - waktu setengah korban jatuh 281 / 261 / 254 detik
   - peluru per prajurit 46.3 / 46.7 / 43.7
   - CLOSEIN: penahanan maju too_close turun 509 → 40; close_in lolos 140×.
   KEPUTUSAN (aturan "tidak boleh memburuk"):
   - PANIC30: hampir semua metrik membaik (korban −13%, mati di tempat terbuka −16%, mati saat panik −33%, panik 30 detik vs 64 detik). Tapi fratricide naik 3.7 → 6.5, dan baru 2 run. Default TETAP 0 (mati). Admin bisa menyalakan lewat morale_panic_max_s=30. Perlu 3+ run lagi untuk diputuskan jadi default.
   - CLOSEIN: bekerja sesuai desain dan tempo lebih cepat (setengah korban jatuh 27 detik lebih awal). Selain itu netral; peluru per prajurit −6%. Default TETAP 0. Admin bisa menyalakan lewat attacker_close_in=1. PERF_STRESS adalah meeting engagement (kedua pihak menyerang); efek sebenarnya perlu diuji di skenario serbuan (TAC_FM / DEF_*).
   - Analisis fratricide PANIC30 (setelah blok): 8 dari 10 FF di run 1 terjadi dalam satu jendela 65 detik (t161–226), sebagian besar granat/ledakan. Run 2 hanya 3 FF. Kenaikan FF tidak berhubungan dengan pelepasan panik; kemungkinan besar variasi kebetulan dari satu insiden granat.
   - Analisis fratricide semua arm di blok (death_ctx, jenis damage): FF per run BASE 3.7 (45% ledakan), AGG 7.5 (87% ledakan), OPP 8.5 (88% ledakan), AGG2 2.0 (25%), PANIC30 6.5 (62%), CLOSEIN 3.7 (64%). Arm agresif (AGG/OPP) menaikkan FF hampir seluruhnya lewat granat/UGL/roket. Pengecekan teman saat melempar sudah ada (radius 15 m + jalur), jadi penyebabnya kemungkinan teman yang masuk area ledakan selama sumbu menyala, atau granat memantul. Data dikirim ke BD untuk keputusan default AGG/OPP. BD menyatakan data ini cocok dengan perbaikan yang sudah ia siapkan (mb.blast_m/blast_s: memblokir gerakan masuk ke titik ledakan teman yang masih baru selama 6 detik, radius 20 m; belum dites, dilaporkan BD sendiri). Tidak ada perubahan kode (aturan user: FF sedikit itu wajar).
   - Perbaikan CQB loop: di blok ini tidak terlihat lagi pola mulai-batal berulang (cqb:plan_fail_long_retry 0 di BASE).

C. YANG BELUM
1. A/B CTX (ctx.on) di blok PERF: 0 run (antrean penuh). Tetap OFF.
2. PANIC30 baru 2 run, sedangkan aturan butuh 3+. BD sudah mengantrekan 1× PANIC30 + 1× BASE (q_realism.ps1). Antrean hanya jalan kalau tidak ada game selama 2 menit di jendela 09:05–09:25; kalau lane masih sibuk, otomatis dilewati. Status akhir: DILEWATI. Tidak ada lane kosong di 09:05–09:25 (antrean Z/ctx masih jalan; auto-skip 09:25:04). Masuk jendela tes berikutnya. Kalau dilewati: perlu 2–3 run lagi untuk menentukan default.
3. CLOSEIN belum diuji di skenario serbuan (TAC_FM / DEF_*), padahal efek sebenarnya di sana (PERF_STRESS = kedua pihak menyerang).
4. A/B perception_identify_mul 1.25 (usulan F1) belum dijalankan; belum ada lane kosong.
5. A/B return_fire_unseen_chance (usulan F4) belum dijalankan.
6. Pemilih gedung CQB yang melewati gedung tanpa pintu luar sejak awal (F8): belum dikerjakan. Loop-nya sudah tertangani oleh perbaikan A1 (jeda 600 detik).
7. Pembersihan log navlink_stall (F7): belum. File NavGuard milik penulis lain, perlu TAHAN/SIAP.

F. PEMIKIRAN ULANG: APA LAGI SUPAYA DCO JADI AI "WOW" (urut dampak, berdasar data log 10-05 s/d 10-10)
1. Jarak tembak terlalu dekat. Di PERF_STRESS, median jarak kill 12–15 m dan target pertama didapat di 20–60 m.
   - Data Features Auditor (death_ctx 10-09/10-10, 967 kematian di world non-PERF): jarak bunuh p25 15 m, p50 36 m, p75 110 m, 38% di bawah 25 m. Per world p50: DEF_PERIMETER 152 m, DEF_AREA 66 m, DEF_DEPTH 47 m, DEF_REVERSE 38 m, DEF_DELAY 25 m.
   - Pola mati diam di tempat terbuka (static_open): 88% sedang dalam AttackBehavior, dengan pembunuh di median 11 m. Artinya baku tembak jarak sangat dekat, bukan ditembak dari jauh.
   - Usul metrik tetap untuk setiap tes: p50 jarak bunuh dan persentase di bawah 25 m. AI "WoW" seharusnya kontak di 100–300 m: saling tekan tembakan dulu, baru manuver.
   - Tuas yang sudah ada:
     a) persepsi siaga minimal saat misi atau setelah mendengar tembakan (aktif);
     b) tembakan area grup SBF (aktif);
     c) perception_identify_mul (bisa diatur).
   - Usulan: A/B identify_mul 1.25 (arm B 10-06: ABF PASS, fase ABF dapat 3 kill vs 0) di PERF_STRESS dan TAC_FM, 3+3 run.
2. Panik yang terlalu lama. Panik berlangsung 60–430 detik, dan 322 kematian terjadi dalam keadaan panik (kebanyakan tiarap diam di tempat terbuka). Sudah di-A/B (B2): morale_panic_max_s 30 menurunkan korban dan kematian saat panik, n=2; default masih 0, perlu 3+ run.
3. Penyerang berhenti mendesak di 30–80 m. Saklar attacker_close_in sudah dibuat dan di-A/B di PERF_STRESS (netral, B2); berikutnya uji di skenario serbuan.
4. Mati oleh penembak tak terlihat (40–50% kematian defender di tes DEF). Fitur tembakan balasan ke arah penembak tak terlihat sudah ada (return_fire_unseen_chance, default 0). Perlu di-A/B ulang; aturan user sekarang "fratricide sedikit itu wajar".
5. Bertahan di cover: cover_hold_mul ×2 aktif (korban turun, tembakan naik). Fitur "lari ke cover terdekat saat tertekan di tempat terbuka" (tm.open_rule_v2) belum terbukti di PERF_STRESS. Perlu world tes terarah: 1 regu ditembaki dari posisi tetap di lapangan terbuka.
6. Regu memutus kontak sebelum habis (bc.*): hasil A/B 10-06 campuran (korban turun, fratricide dan panik naik). Perlu dikaji ulang bersama aturan CQB yang baru.
7. Kebersihan log: navlink_stall tercatat 100–250 kali per run, sebagian besar false positive (menunggu medic, kru mortir statis, menunggu naik kendaraan). Saran: kecualikan behavior yang memang diam, supaya log tidak berisik dan hemat performa.
8. Pemilih gedung CQB: banyak gedung tanpa pintu luar di graph navigasi (plan_no_outside_door). Loop-nya sudah diperbaiki; berikutnya pemilih gedung sebaiknya melewati gedung seperti ini sejak awal.
9. Fratricide granat saat AI dibuat lebih agresif: 87–88% FF di arm AGG/OPP berasal dari ledakan. Kalau nanti FF dianggap terlalu banyak, tuas yang paling tepat adalah pengecekan ulang tepat sebelum granat dilepas (bukan saat memutuskan melempar) atau sumbu yang dimasak lebih lama. Bukan tambahan pengecekan jalur tembak senapan.

D. FILE YANG DIUBAH
- scripts/Game/DCO/Group/DCO_CQBActivity.c (Abort)
- scripts/Game/DCO/Group/DCO_ClearBuilding.c (FindContactBuilding)
- scripts/Game/DCO/Global/DCO_GlobalTuneRealism.c (BARU)
- scripts/Game/DCO/Individual/DCO_Engage.c (Attacker, MinDist/BaseMinDist, close_in)
- world/Test/Perf/DCO_T_PERF_STRESS_ABL_CLOSEIN.ent (+ .meta, _Layers) (BARU, world tes)

E. FITUR/SETTING BARU (semua di $profile:DCO/DCO_GlobalConfig.json, kategori Realism)
- cqb_plan_fail_retry_s (key tune cqb.plan_fail_retry_s)
  Apa: jeda sebelum grup boleh mencoba CQB lagi pada gedung yang rencana CQB-nya gagal karena struktur, misalnya tidak ada pintu luar di graph navigasi.
  Default 600, rentang 30–3600 detik.
  Efek: kecil = grup lebih sering mencoba lagi (risiko siklus mulai-batal); besar = gedung itu dilewati lebih lama.
- morale_panic_threshold (tm.morale_panic_th)
  Apa: ambang morale untuk masuk PANIC.
  Default 2.4, rentang 1.5–4.0.
  Efek: makin tinggi makin jarang panik (AI lebih tegar dan agresif); 2.0 = perilaku lama (lebih sering panik).
- morale_panic_max_s (tm.panic_max_s)
  Apa: batas lama PANIC. Setelah X detik prajurit turun ke SHAKEN dan tekanan morale dikurangi ke 0.9 × ambang panik.
  Default 0 (mati), rentang 0–300.
  Efek: 30 = panik maksimal 30 detik, lalu prajurit kembali bertempur dalam keadaan shaken.
- morale_panic_refract_s (tm.panic_refract_s)
  Apa: masa jeda setelah panik dilepas; selama itu prajurit tidak bisa panik lagi (paling buruk shaken).
  Default 20, rentang 0–120.
- morale_suppressed_per_s (tm.morale_supp_s1)
  Apa: kenaikan tekanan morale per detik saat ditekan tembakan.
  Default 0.04, rentang 0–0.3.
- morale_pinned_per_s (tm.morale_supp_s2)
  Apa: kenaikan per detik saat pinned.
  Default 0.07, rentang 0–0.5.
- morale_alerted_per_s / morale_threatened_per_s (tm.morale_alerted_s / tm.morale_threatened_s)
  Apa: kenaikan morale per detik selama status threat ALERTED / THREATENED.
  Default 0.015 / 0.03.
  Efek: makin besar, pertempuran panjang makin cepat menggoyahkan prajurit.
- morale_aimed_at_stress (tm.morale_aimed)
  Apa: stres saat dibidik atau ditembak musuh yang terlihat, maksimal sekali per 5 detik.
  Default 0.04, rentang 0–0.3.
- morale_recover_per_s / morale_recover_calm_s / morale_recover_per_friend_s
  Apa: laju pulih morale per detik setelah tenang selama N detik, ditambah bonus per teman satu regu.
  Default 0.01 / 10 / 0.001.
  Efek: makin besar, prajurit makin cepat pulih.
- morale_event_half_life_s (tm.morale_event_half_s)
  Apa: berapa cepat efek kejadian buruk (teman tewas, ledakan) memudar.
  Default 90, rentang 10–600 detik.
- suppression_decay_per_s (tm.supp_drop_s)
  Apa: laju turunnya tekanan tembakan per detik.
  Default 0.06, rentang 0.02–0.2.
  Efek: kecil = prajurit tertekan lebih lama; besar = cepat bangkit lagi.
- alert_after_gunfire_s / alert_gunfire_radius_m (tm.heard_floor_s / tm.heard_floor_m)
  Apa: prajurit yang mendengar tembakan dalam radius X m menjadi minimal VIGILANT selama Y detik (persepsi lebih tajam).
  Default 45 detik / 300 m.
- cover_hold_mul (tm.cover_hold_mul)
  Apa: pengali lama prajurit bertahan di cover sebelum bergerak lagi.
  Default 2.0, rentang 0.5–5.0.
  Efek (A/B 10-06): ×2 → korban turun, prajurit lebih banyak menembak, tempo pertempuran sama.
- sbf_area_fire (tm.area_fire)
  Apa: grup support-by-fire atau tembakan suppress yang diperintahkan boleh menembaki area musuh walau target tidak terlihat, asal 40 m pertama di depan bersih.
  Default 1 (aktif), 0 = mati.
- perception_identify_mul (pc.identify_mul)
  Apa: kecepatan mengenali musuh.
  Default 1.0, rentang 0.3–3.0.
  Efek: lebih tinggi = melihat musuh dari jarak lebih jauh.
- perception_threatened / perception_shaken_mul / perception_panic_mul
  Apa: persepsi saat ditembaki berat, dan pengali tunnel vision saat SHAKEN / PANIC.
  Default 1.0 / 0.8 / 0.6.
- advance_stop_dist_aggressive_m / advance_stop_dist_balanced_m / advance_stop_dist_defensive_m (tm.min_dist_agg_m / tm.min_dist_bal_m / tm.min_dist_def_m)
  Apa: jarak ke musuh di mana prajurit berhenti maju, menurut posture regu.
  Default 30 / 50 / 80 m, rentang 0–150 / 0–200 / 0–300.
  Efek: makin kecil makin agresif mendekat.
- attacker_close_in (tm.atk_close_in)
  Apa: kalau 1, khusus grup penyerang (task ATTACK/FLANK, atau tanpa komandan dengan waypoint Search & Destroy) jarak berhenti di atas dikali attacker_stop_dist_mul.
  Default 0 (mati), nilai 0 atau 1.
- attacker_stop_dist_mul (tm.atk_min_dist_mul)
  Apa: pengali jarak berhenti untuk penyerang saat attacker_close_in=1.
  Default 0.4, rentang 0.1–1.0.
  Contoh: balanced 50 m × 0.4 = penyerang tetap maju sampai 20 m.
- return_fire_unseen_chance / return_fire_unseen_cooldown_s / return_fire_unseen_max_m (tm.retfire_*)
  Apa: prajurit yang ditembaki penembak tak terlihat membalas ke perkiraan posisi penembak (meleset beberapa derajat, menembak 3–5 detik).
  Default 0 (mati) / 12 detik / 400 m.
  Contoh: 0.5 = 50% peluang membalas tiap kali ditembaki, paling sering sekali per 12 detik per prajurit.
