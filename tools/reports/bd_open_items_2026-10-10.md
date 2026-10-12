DAFTAR PEKERJAAN BD (DCO Behavior Designer) YANG BELUM SELESAI — 2026-10-10
Catatan umum:
- Semua tes PERF_STRESS = 10 menit per run + ±2 menit kompilasi/muat. Aturan lane maksimal 2 game bersamaan, jadi A/B 3+3 ≈ 40–45 menit plus analisis ±15 menit.
- Aturan tanpa regresi berlaku untuk semua item: saklar default 0 sampai A/B 3 run vs OFF tidak menunjukkan regresi.
- Tes hanya dijalankan setelah aba-aba user.

BD-1 — AGGFM: A/B paket agresif versi perbaikan
- Apa yang belum: paket AGG versi perbaikan belum pernah dites. Isinya:
  - tb.on, tgt.claim, mb.on, mb.field;
  - tb.fm (Squad F&M);
  - tb.set_frac 0,34;
  - mb.field_ring2_m 8;
  - hindari radius ledakan kawan (mb.blast_m 20 / mb.blast_s 6);
  - callout "Moving!/Covering!".
  Versi pertama (AGG) sudah dites 2 run: semua fitur aktif, tapi tembak kawan naik 4 → 8 per run (87% ledakan granat/roket kawan), jadi tetap mati. Perbaikannya dibuat setelah itu.
- Status: kode selesai belum dites (lolos kompilasi 07:53).
- Ketergantungan: lane tes kosong (koordinasi dengan antrean Scripter / Realism / ctx / AI Design). Baseline bisa dipakai bersama dengan arm lain di blok yang sama.
- Pemblokir: tidak ada selain lane.
- Perkiraan: 0 jam kode; tes 3× world/Test/Perf/DCO_T_PERF_STRESS_AGGFM vs 3× DCO_T_PERF_STRESS ≈ 45 menit + analisis 15 menit (skrip scratchpad ab_block.js).
- File: scripts/Game/DCO/Group/DCO_TeamBound.c, Individual/DCO_MoveBrain.c, Individual/Utility/DCO_TargetClaim.c, Individual/Modded/Modded_CombatMoveLogic_Attack.c, Modded_CombatComponent.c, Modded_UpdateAttackData.c, Individual/DCO_Engage.c, Individual/Utility/DCO_GrenadeUtility.c, Global/DCO_GlobalTune.c, world/Test/Perf/DCO_T_PERF_STRESS_AGGFM.

BD-2 — Close Assault Grenades (gren.atk_close): tes terpisah
- Apa yang belum: fitur granat jarak dekat untuk penyerang (≤35 m, ancaman sampai 2× batas vanilla, tidak saat panik) belum dites sama sekali, dan sengaja TIDAK dimasukkan ke world AGGFM. Alasannya, granat adalah sumber utama tembak kawan, jadi efeknya harus dipisah.
- Status: kode selesai belum dites.
- Ketergantungan: sebaiknya setelah BD-1 (pakai AGGFM sebagai dasar kalau BD-1 lolos), atau langsung di atas baseline. World tes perlu dibuat (copy PERF_STRESS + gren.atk_close 1).
- Pemblokir: tidak ada.
- Perkiraan: 10 menit membuat world; tes 3+3 ≈ 45 menit + analisis 15 menit.
- File: Individual/Modded/Modded_CombatComponent.c (DCO_GrenCloseAttack), Global/DCO_GlobalTune.c.

BD-3 — Keputusan default ON untuk paket AGG
- Apa yang belum: kalau BD-1 (dan BD-2) lolos tanpa regresi, default tb.on / tgt.claim / mb.on / mb.field / tb.fm perlu dinaikkan ke 1.
  - Catatan penting: config JSON server membekukan nilai default lama. Server yang file config-nya sudah ada tetap memakai 0 walau default kode diubah. Jadi pengumuman ke admin server, atau mekanisme upgrade default, perlu diputuskan.
- Status: menunggu A/B, lalu menunggu keputusan user.
- Ketergantungan: BD-1, BD-2.
- Pemblokir: hasil A/B.
- Perkiraan: 15 menit kode (ubah default + teks bantuan GM).
- File: Global/DCO_GlobalTune.c, GM/DCO_TuneAttributesBD.c, Configs/Editor/AttributeLists/Edit.conf.

BD-4 — Cek visual 6 atribut GM di Global Combat
- Apa yang belum: tampilan 6 saklar baru di panel GM belum pernah dilihat: Squad Bounding Handshake, Squad Fire and Maneuver, Fire Distribution, Trigger-Based Movement, Field Cover Spots, Close Assault Grenades. Yang sudah dipastikan hanya secara statis: class ada, Edit.conf terisi, teks bantuan ditambahkan Scripter 2.
- Status: belum mulai (izin kontrol layar ditolak malam ini).
- Ketergantungan: user membuka panel GM sendiri, atau memberi izin kontrol layar.
- Pemblokir: izin.
- Perkiraan: 10 menit.
- File: Configs/Editor/AttributeLists/Edit.conf, scripts/Game/DCO/GM/DCO_TuneAttributesBD.c.

BD-5 — Move Brain tahap 2 (desain penuh yang belum dibangun)
- Apa yang belum: yang dibangun malam ini adalah versi pemicu (kapan pindah/maju). Bagian desain yang belum dibuat:
  - Pemilihan posisi dengan skor dari jawaban Research: perlindungan 0,35 / keterbukaan 0,20 / kemajuan 0,20 / garis tembak 0,15 / jarak kawan 0,10; hanya pindah kalau skor naik ≥ 0,15.
  - Biaya perjalanan, yaitu lama terbuka × jumlah musuh yang melihat.
  - Aksi GESER KECIL (1–3 m / ganti sikap), MERANGKAK, dan MUNDUR terarah.
  - Panjang rush 3–5 detik (2–3 detik saat ditekan).
- Status: setengah jalan.
- Ketergantungan: hasil BD-1, supaya tahap 2 dibangun di atas versi yang terbukti tidak regresi.
- Pemblokir: tidak ada.
- Perkiraan: kode 3–4 jam + kompilasi; tes A/B 3+3 ≈ 45 menit.
- File: Individual/DCO_MoveBrain.c, Individual/Modded/Modded_CombatMoveLogic_Attack.c, Individual/DCO_CoverId.c (dipakai).

BD-6 — Audit kedalaman no. 1: A/B semua lapisan yang sudah jadi tapi mati
- Apa yang belum: dari audit 10-09 (prioritas no. 1), user memilih "mulai dari 2-3 dulu", jadi no. 1 belum dikerjakan. Daftar lapisan:
  - cov.on (pemilih cover hybrid);
  - ctx.on (konteks tempur, milik sesi ctx);
  - osc.fix_dir / fix_cover / fix_short / fix_commit / fix_bound (perbaikan osilasi);
  - fm.auto (formasi otomatis, milik Scripter);
  - paket OPP (milik AI Design; OPP1/OPP2 sudah dinilai tetap mati).
  Masing-masing butuh 3 run vs OFF.
- Status: belum mulai (sudah disetujui sebagai daftar prioritas, urutan setelah 2-3).
- Ketergantungan: pemilik lapisan (ctx untuk ctx.on, Scripter untuk fm.auto, AI Design untuk OPP). Lane tes.
- Pemblokir: lane; sebagian world A/B sudah ada (PERF_STRESS_COV, DEF_DELAY_OSC, PERF_STRESS_CTX).
- Perkiraan: ±45 menit lane per lapisan → total ±4–5 jam lane; analisis ±15 menit per lapisan.
- File: world/Test/Perf/DCO_T_PERF_STRESS_COV, world/Test/Commander/Tactics/DCO_T_DEF_DELAY_OSC, world/Test/Perf/DCO_T_PERF_STRESS_CTX, Individual/DCO_CoverEval.c, Benchmark/DCO_Osc.c.

BD-7 — Audit no. 5a: pencarian kontak gagal ±85–90%
- Apa yang belum: sebabnya sudah ketemu. Bounding investigate grup bertukar peran setiap ±1 detik (DCO_TeamArrived langsung true), sehingga perintah investigate dibatalkan berulang. Perbaikannya sudah ada: osc.fix_bound, minimal 4 detik per bound. Masih mati dan belum pernah dites sendirian; di world DEF_DELAY_OSC ia tercampur dengan F1–F4.
- Status: kode selesai belum dites (menunggu A/B).
- Ketergantungan: world A/B baru yang hanya menyalakan osc.fix_bound.
- Pemblokir: tidak ada.
- Perkiraan: 10 menit membuat world; tes 3+3 ≈ 45 menit; metrik search:end_failed vs search:arrived.
- File: Group/Modded_InvestigateClusterActivity.c, Benchmark/DCO_Osc.c.

BD-8 — Audit no. 5b: granat hanya 8% berhasil
- Apa yang belum:
  - Penolakan karena ancaman tinggi (vanilla FRAG_GRENADE_MAX_THREAT, ±12 kali per menit) ditangani sebagian oleh BD-2.
  - Penyebab penolakan lain (jarak, jalur lempar, kawan dekat) belum dirinci.
- Status: belum mulai (analisis).
- Ketergantungan: BD-2.
- Pemblokir: tidak ada.
- Perkiraan: analisis log 30 menit; kalau perlu perbaikan, 1–2 jam kode + A/B 45 menit.
- File: Individual/Utility/DCO_GrenadeUtility.c, Individual/Modded/Modded_CombatComponent.c.

BD-9 — Audit no. 5c: penghentian tembakan karena cek jalur kawan (106–108 kali per menit)
- Apa yang belum: rinciannya 48 karena grup lain dan 38 karena anggota sendiri. Lebar jalur tan 0,035 berarti ±10 m di jarak 300 m, kemungkinan terlalu lebar di jarak jauh. Aturan user "beberapa tembak kawan itu wajar" mendukung pelonggaran.
- Status: belum mulai; desain perlu ACC user.
- Ketergantungan: tidak ada.
- Pemblokir: ACC desain.
- Perkiraan: desain 1 jam; kode 1 jam; A/B 45 menit.
- File: Global/DCO_Util.c (FireLaneCheck / HasSquadmateInFireLine), Individual/Modded/Modded_UpdateAttackData.c.

BD-10 — CQB Indoor vs Outdoor: perilaku yang berbeda
- Apa yang belum: pembagian sudah ada di kode (DCO_Engage.CqbKind; tm.cqb_aggr_in dan tm.cqb_aggr_out, keduanya default 4). Tapi:
  - (1) Perbedaan PERILAKU indoor vs outdoor yang diminta user ("mereka punya behavior beda") belum didesain; sekarang yang beda hanya angkanya.
  - (2) A/B Realism di lapangan terbuka (cqb_aggr 4 vs 1, 3 run) menunjukkan tewas 69,3 vs 64,3, sedikit lebih buruk. Nilai outdoor perlu A/B sendiri (4 vs 2 vs 1).
  - (3) Indoor 4× sudah 4/4 PASS di CQBBT.
- Status: desain belum mulai; A/B outdoor menunggu.
- Ketergantungan: tidak ada.
- Pemblokir: keputusan desain user.
- Perkiraan: desain 1 jam; kode 2 jam; A/B outdoor 3 arm × 3 run ≈ 1 jam lane.
- File: Individual/DCO_Engage.c, Individual/Modded/Modded_CombatMoveLogic_Attack.c, Group/Modded_InvestigateClusterActivity.c.

BD-11 — Redesign toolbar waypoint GM (Vanilla WP / DCO WP)
- Apa yang belum:
  - Sudah dikirim ke user (10-07): evaluasi UI/UX (4 masalah mayor: status halaman ambigu, kontras 2–2,5:1, teks 11–13 px, bentuk tidak konsisten), 6 gaya visual (A tab … F), hitungan overflow, dan mockup "komando berbasis niat" (rombak total).
  - User belum memilih gaya. Spesifikasi belum ditulis atau dikirim.
- Status: menunggu keputusan user.
- Ketergantungan: DCO UI/UX (gaya), Scripter (implementasi).
- Pemblokir: pilihan user.
- Perkiraan: spesifikasi 1 jam. Implementasi tergantung pilihan: gaya tab ±2–3 jam; rombak total berbasis niat ±1–2 hari.
- File: Configs/Editor/ActionLists/Toolbar/EditToolbar.conf, UI/layouts/Editor/Toolbar/DCO_CommandMenu_*.layout, Configs/Editor/PlaceableEntities/DCO/DCO_Waypoints.conf.

BD-12 — Run untuk Realism: PANIC30 + BASE
- Apa yang belum: Realism minta 1× PERF_STRESS_ABL_PANIC30 + 1× PERF_STRESS paralel. Diantrekan dengan syarat lane kosong 09:05–09:25, dilewati otomatis karena lane penuh. Realism sudah memutuskan PANIC30 dan CLOSEIN tetap default 0, jadi ini hanya data tambahan.
- Status: belum mulai (dilewati).
- Ketergantungan: lane.
- Pemblokir: lane.
- Perkiraan: 15 menit (2 run paralel).
- File: world/Test/Perf/DCO_T_PERF_STRESS_ABL_PANIC30, scratchpad q_realism.ps1.

BD-13 — 6 arm uji config Features Auditor
- Apa yang belum: 6 arm uji config milik Features Auditor saya antrekan untuk jendela berikutnya, karena lane malam ini penuh. Untuk pembanding mereka bisa memakai baseline blok.
- Status: belum mulai (diantrekan).
- Ketergantungan: Features Auditor (skrip mereka harus memakai path X/profile/DCO dan argumen -dcoTest -dcoTestExit; tanpa itu tidak ada log DCO).
- Pemblokir: lane.
- Perkiraan: 6 run ≈ 40 menit di 2 lane.
- File: skrip milik Features Auditor.

BD-14 — Class DCO_TestArea hilang di world tes dasar
- Apa yang belum: 4 entitas penanda (posisi penyerang dan objektif) di world dasar semua tes memakai class DCO_TestArea yang tidak ada di addon mana pun. Akibatnya 1 baris error per sesi dan penanda tidak tampil, tapi tidak fatal. Belum diubah karena world dasar dipakai semua tes.
- Status: ditahan (perlu keputusan).
- Ketergantungan: pemilik world dasar (Scripter / Features Auditor).
- Pemblokir: keputusan: buat class stub kosong, atau hapus penanda.
- Perkiraan: 15 menit + 1 run tes apa saja untuk memastikan.
- File: world tes dasar (SubScene DefaultGMTest.ent), scripts (class baru kalau dipilih stub).

BD-15 — Perbaikan SIEGE dari bedah taktik (10-06) hilang karena revert
- Apa yang belum:
  - Perbaikan saya (FIX dikirim lebih awal saat SIEGE naik ke serangan; SIEGE 4/4 PASS) terhapus oleh revert 2026-10-06 23:43; FMSendFixEarly sudah tidak ada di kode.
  - Temuan tambahan yang belum dikerjakan: cek garis pandang FIX ke objektif hanya dianggap ada kalau lintasan bebas sampai 10 m dari pusat objektif (radius objektif 60 m), sehingga semua kandidat posisi FIX gagal.
  - Sejak revert, taktik komandan dipegang Scripter, dengan engine yang berbeda (SIEGE 2/2 PASS 10-08).
  - Bedah taktik lain ("satu-satu sampai lolos konsisten") berhenti setelah SIEGE karena user beralih ke topik "AI pasif".
- Status: ditahan / digantikan oleh pekerjaan Scripter.
- Ketergantungan: Scripter (pemilik kode komandan).
- Pemblokir: perlu dicek apakah cek LOS 10 m yang sama masih ada di kode Scripter.
- Perkiraan: cek kode 20 menit; kalau masih ada, perbaikan ±1 jam + 2 run TAC_SIEGE ≈ 50 menit.
- File: Commander/Operations/DCO_CommanderTactics_*.c (milik Scripter).

BD-16 — Tes lama yang FAIL, dari survei status saya 10-06 (pemilik belum jelas)
- Apa yang belum: daftar FAIL dari survei 10-06, sebelum revert. Semuanya perlu run ulang di build sekarang untuk tahu status aslinya:
  - IND_COVER 0/8 (cover:shift_outdoor dan cover:peek tidak muncul);
  - IND_WINDOW 2/14 (granat lewat jendela dan cook-off tidak terjadi);
  - IND_GLSMOKE semua FAIL (smoke:tracked_ai_gl tidak tercatat);
  - IND_ATHUNT 0/2 di survei itu (sebelumnya pernah PASS 10-03);
  - BTFIX_Combat 0/2 (reloc:arrived tidak pernah);
  - VEH_UAZGUN 0/4;
  - TR1_FireManeuver 0/1.
  - CQBWP 0/8 sudah usang, karena waypoint Assault Building dihapus 10-07 dan diganti Do CQB.
- Status: belum mulai.
- Ketergantungan: pemilik masing-masing fitur (Individual = Scripter 2 / BD, kendaraan = Scripter).
- Pemblokir: lane.
- Perkiraan: 1 run per world ≈ 7 world × ±12 menit ≈ 45 menit di 2 lane; perbaikan tergantung hasil.
- File: world/Test/Individual/DCO_T_IND_COVER, _WINDOW, _GLSMOKE, _ATHUNT, world tes BTFIX / VEH_UAZGUN / TR1.

Urutan yang saya sarankan:
1. BD-1 (AGGFM).
2. BD-7 (osc.fix_bound sendirian).
3. BD-2 (granat dekat).
4. BD-4 (cek GM, 10 menit).
5. BD-3 (keputusan default).
6. BD-10 (desain CQB indoor/outdoor).
7. BD-5 (Move Brain tahap 2).
8. BD-9 (lebar jalur tembak).
9. BD-6 (A/B lapisan lain).
10. BD-16 (run ulang tes lama).
11. Sisanya (BD-11 menunggu user, BD-12/13 kapan saja ada lane kosong, BD-14/15 keputusan pemilik).
