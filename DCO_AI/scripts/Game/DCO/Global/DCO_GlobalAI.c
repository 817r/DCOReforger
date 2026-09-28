class DCO_GlobalAIComponentClass: ScriptComponentClass
{
}

enum DCO_EAILODMode
{
	VANILLA,
	PREVENT_MAX_LOD,
	FULL_DETAIL
}

class DCO_GlobalAIComponent: ScriptComponent
{
	[Attribute("1.8", UIWidgets.Slider, "Global AI unit skill level", "0.1 10 0.1")]
	protected float unitAimSkillAccuracy;

	[Attribute( defvalue: "15", uiwidget: UIWidgets.Slider, desc: "Unit skill", params: "1 60 0.01" )]
	protected float m_fTimeToMaxAccuracy;

	[Attribute( defvalue: "1.5", uiwidget: UIWidgets.Slider, desc: "Unit Perception", params: "0.5 5 0.01" )]
	protected float m_fAiPerception;

	[Attribute( defvalue: "0", uiwidget: UIWidgets.Auto, desc: "Magical Ammo")]
	protected bool m_bIsMagicallyResupplied;

	[Attribute( defvalue: "700", uiwidget: UIWidgets.Slider, desc: "Unit Perception", params: "0 2000 0.01" )]
	protected float m_fVehicleDismountDanger;

	[Attribute("2", UIWidgets.ComboBox, "AI Custom skill in combat", "", ParamEnumArray.FromEnum(DCO_AISKILL) )]
	protected DCO_AISKILL m_eAISkillDefault;

	[Attribute( defvalue: "1", uiwidget: UIWidgets.Slider, desc: "How Suppression Affecting this AI", params: "0 2 0.01" )]
	protected float m_fSuppressionEffect;

	[Attribute("0.9", UIWidgets.Range, "Base chance AI mau aktif nyari cover pas ke-detect di tempat terbuka (0-1). Di-scale lebih lanjut sama personality -- RECKLESS turun paling banyak (ceroboh), AGGRESSIVE turun sedang (combat-oriented tapi disiplin), CAUTIOUS naik.", params: "0 1 0.01" )]
	protected float m_fTakeCoverChance;

	[Attribute("80", UIWidgets.Slider, "Bobot personality MEASURED (relatif ke 3 lainnya, gak harus total 100)", params: "0 100 1")]
	protected float m_fPersonalityWeightStandard;

	[Attribute("7", UIWidgets.Slider, "Bobot personality CAUTIOUS", params: "0 100 1")]
	protected float m_fPersonalityWeightCautious;

	[Attribute("12", UIWidgets.Slider, "Bobot personality AGGRESSIVE", params: "0 100 1")]
	protected float m_fPersonalityWeightAggressive;

	[Attribute("3", UIWidgets.Slider, "Bobot personality RECKLESS", params: "0 100 1")]
	protected float m_fPersonalityWeightReckless;

	[Attribute("0.6", UIWidgets.Slider, "Peluang AI dodge (lari cari perlindungan) tiap kali denger tembakan. Di-scale personality kalau dodgeScaleByPersonality nyala.", params: "0 1 0.05")]
	protected float m_fDodgeChance;

	[Attribute("8.0", UIWidgets.Slider, "Cooldown (detik) sebelum AI yang sama boleh dodge lagi. Ini rem utamanya -- tanpa cooldown, full auto bikin dodgeChance gak ada artinya.", params: "0 120 0.5")]
	protected float m_fDodgeCooldown;

	[Attribute("250.0", UIWidgets.Slider, "Jarak maksimum (m) tembakan yang masih bisa memicu dodge.", params: "0 1000 5")]
	protected float m_fDodgeMaxDist;

	[Attribute("30.0", UIWidgets.Slider, "Jarak pencarian bangunan/cover saat dodge.", params: "5 100 1")]
	protected float m_fDodgeSearchDist;

	[Attribute("1", UIWidgets.CheckBox, "Skala peluang dodge pakai personality AI (CAUTIOUS naik, RECKLESS turun).")]
	protected bool m_bDodgeScaleByPersonality;

	[Attribute("1", UIWidgets.Slider, "Jumlah tembakan musuh (yang cukup ngancem) sebelum AI mau dodge. 1 = langsung di tembakan pertama (perilaku lama).", params: "1 20 1")]
	protected int m_iDodgeShotThreshold;

	[Attribute("5.0", UIWidgets.Slider, "Jendela waktu (detik) ngitung tembakan. Kalau gak ada tembakan baru selama ini, hitungan balik ke 0.", params: "1 30 0.5")]
	protected float m_fDodgeShotWindow;

	[Attribute("0.5", UIWidgets.Slider, "Seberapa sering AI lempar frag grenade. 0 = gak pernah, 0.5 = default, 1 = sering banget (chance dikali 2).", params: "0 1 0.01", category: "Weapon Usage")]
	protected float m_fGrenadeUsage;

	[Attribute("0.7", UIWidgets.Slider, "Seberapa sering AI pakai grenade launcher (UGL). 0 = gak pernah, 0.5 = default, 1 = sering banget (chance dikali 2).", params: "0 1 0.01", category: "Weapon Usage")]
	protected float m_fGLUsage;

	[Attribute("1", UIWidgets.Slider, "Akurasi grenade launcher (UGL). Lebih tinggi = sebaran ledakan lebih rapat. 1 = default.", params: "0.1 5 0.1", category: "Weapon Usage")]
	protected float m_fGLAccuracy;

	[Attribute("0.5", UIWidgets.Slider, "Seberapa sering AI lempar smoke buat nutup gerak. 0 = gak pernah, 0.5 = default, 1 = sering banget (chance dikali 2).", params: "0 1 0.01", category: "Weapon Usage")]
	protected float m_fSmokeUsage;

	[Attribute("1", UIWidgets.ComboBox, "LOD simulasi AI. VANILLA = engine yang ngatur (AI jauh dari pemain bisa diem), PREVENT_MAX_LOD = AI tetap jalan jauh dari pemain (perlu buat AI Commander), FULL_DETAIL = paling berat.", "", ParamEnumArray.FromEnum(DCO_EAILODMode), category: "Performance")]
	protected DCO_EAILODMode m_eLODMode;

	[Attribute("0", UIWidgets.CheckBox, "Performance profiling: catat waktu per bagian DCO, query/raycast per detik, FPS dan spike frame tiap 30 detik ke log benchmark + $profile:DCO_Bench/perf_*.log. OFF = hampir nol biaya.", category: "Performance")]
	protected bool m_bPerfProfiling;

	[Attribute("30", UIWidgets.Slider, "Detik grup tanpa waypoint sebelum masuk IDLE (anggota cari cover lalu diam).", params: "5 300 1", category: "Idle")]
	protected float m_fIdleEnterTime;

	[Attribute("100", UIWidgets.Slider, "Jarak (m) cari cover / posisi dalam bangunan waktu IDLE, dari posisi leader.", params: "10 200 5", category: "Idle")]
	protected float m_fIdleCoverSearchDist;

	[Attribute("15", UIWidgets.Slider, "IDLE: musuh sedekat ini (m) = overrun, AI boleh keluar cover.", params: "0 100 1", category: "Idle")]
	protected float m_fIdleOverrunDist;

	[Attribute("1", UIWidgets.CheckBox, "IDLE: boleh keluar cover buat bantu teman (nyerang musuh yang gak lagi nembakin grup ini).", category: "Idle")]
	protected bool m_bIdleLeaveToAssist;

	[Attribute("0", UIWidgets.CheckBox, "IDLE: boleh keluar cover buat investigasi ancaman.", category: "Idle")]
	protected bool m_bIdleLeaveToInvestigate;

	[Attribute("1", UIWidgets.CheckBox, "IDLE: grup tetap nerima order commander. Off = commander gak ngambil grup yang lagi IDLE.", category: "Idle")]
	protected bool m_bIdleFollowCommander;

	[Attribute("1", UIWidgets.CheckBox, "Tarik balik anggota yang ketinggalan jauh dari leader.", category: "Straggler")]
	protected bool m_bStragglerEnabled;

	[Attribute("15", UIWidgets.Slider, "Detik anggota di luar jarak cohesion sebelum ditarik balik.", params: "5 120 1", category: "Straggler")]
	protected float m_fStragglerTime;

	[Attribute("30", UIWidgets.Slider, "Detik jarak ke leader gak berkurang = macet. Dicoba titik lain sekali, habis itu ditandai macet (gak dihitung di spread grup).", params: "10 300 1", category: "Straggler")]
	protected float m_fStragglerStuckTime;

	[Attribute("0", UIWidgets.CheckBox, "Log tiap straggler: jarak, behavior aktif, combat mode, cover, leash. Buat diagnosa.", category: "Straggler")]
	protected bool m_bStragglerLog;

	[Attribute("2", UIWidgets.Slider, "Squad maju (postur serang + bounding) kalau anggota hidup >= rasio ini x musuh yang diketahui dan gak ditekan berat. Juga boleh keluar dari tahan IDLE / leash Defend.", params: "1 5 0.1", category: "Combat")]
	protected float m_fSuperiorRatio;

	[Attribute("1", UIWidgets.CheckBox, "Target di luar jarak efektif senjata dan gak terancam langsung: diam di tempat (tiarap kalau LOS tetap ada, kalau gak jongkok), tembakan tunggal pelan.", category: "Combat")]
	protected bool m_bLongRangeHold;

	[Attribute("350", UIWidgets.Slider, "Jarak efektif senapan (m). Di luar ini laju tembak dibatasi (tunggal pelan), threat gak boleh naikin.", params: "100 800 10", category: "Combat")]
	protected float m_fRifleEffectiveRange;

	[Attribute("700", UIWidgets.Slider, "Jarak efektif MG (m). MG tetap boleh tembakan beruntun (penekan) di jarak jauh sampai sini.", params: "200 1500 10", category: "Combat")]
	protected float m_fMGEffectiveRange;

	[Attribute("1", UIWidgets.CheckBox, "Overmatch assault: squad yang unggul jelas (kekuatan berbobot >= rasio unggul x personality) dan kontaknya pasti maju bounding walau musuh masih kelihatan.", category: "Combat")]
	protected bool m_bOvermatchAssault;

	[Attribute("250", UIWidgets.Slider, "Jarak maksimum (m) musuh buat mulai overmatch assault.", params: "100 500 10", category: "Combat")]
	protected float m_fOvermatchMaxDist;

	[Attribute("1", UIWidgets.CheckBox, "Clear building: squad yang lagi ATTACK / FLANK / overmatch dan lihat musuh di dalam gedung < 100 m membersihkan gedung itu (isolasi, stack di pintu, breach, clear per ruangan). Juga sapu gedung setelah objective direbut.", category: "CQB")]
	protected bool m_bCQBEnabled;

	[Attribute("6", UIWidgets.Slider, "Minimal anggota hidup buat pisah tim support (isolasi, MG diutamakan). Di bawahnya semua masuk.", params: "3 12 1", category: "CQB")]
	protected int m_iCQBIsolateMin;

	[Attribute("10", UIWidgets.Slider, "Detik maksimum nunggu tim ngumpul di pintu sebelum masuk (leader RECKLESS gak nunggu).", params: "0 30 1", category: "CQB")]
	protected float m_fCQBStackWait;

	[Attribute("2", UIWidgets.Slider, "Tim masuk kehilangan segini (mati/pingsan) = batal, mundur ke posisi support, coba lagi setelah 2 menit.", params: "1 6 1", category: "CQB")]
	protected int m_iCQBAbortLosses;

	[Attribute("1", UIWidgets.CheckBox, "Capture objective nunggu sampai gak ada gedung berstatus CONTACT di dalam radius objective.", category: "CQB")]
	protected bool m_bCQBCaptureWaits;

	[Attribute("180", UIWidgets.Slider, "Detik tanpa kontak baru sebelum gedung CONTACT otomatis dianggap bersih (biar capture gak macet).", params: "30 600 10", category: "CQB")]
	protected float m_fCQBContactTimeout;

	[Attribute("1", UIWidgets.CheckBox, "Perilaku malam: flare squad, disiplin lampu, ROE bertahan & jarak tembak jauh dipendekkan, illumination sebelum assault.", category: "Night")]
	protected bool m_bNightEnabled;

	[Attribute("75", UIWidgets.Slider, "Cooldown flare per squad (detik).", params: "30 300 5", category: "Night")]
	protected float m_fNightFlareCooldown;

	[Attribute("0.6", UIWidgets.Slider, "Pengali jarak tembak grup bertahan (Defend/Garrison) dan jarak efektif long-range saat malam.", params: "0.2 1 0.05", category: "Night")]
	protected float m_fNightEngageMul;

	[Attribute("1", UIWidgets.CheckBox, "Disiplin lampu: senter AI mati di luar ruangan saat malam, nyala cuma buat tim masuk CQB di dalam gedung. Lampu depan kendaraan dimatikan kalau musuh < 800 m.", category: "Night")]
	protected bool m_bNightLightDiscipline;

	[Attribute("0", UIWidgets.Slider, "Maksimal laporan kontak per scan per grup (override global). 0 = pakai nilai prefab grup (default 3).", params: "0 10 1", category: "Contact Report")]
	protected int m_iContactReportMaxPerScan;

	[Attribute("0", UIWidgets.Slider, "Interval scan laporan kontak (detik, override global). 0 = pakai nilai prefab grup (default 15).", params: "0 60 1", category: "Contact Report")]
	protected float m_fContactReportScanInterval;

	[Attribute("-1", UIWidgets.EditBox, "Stok bala bantuan awal semua commander (prajurit). -1 = pakai nilai tiap commander.", category: "Commander Spawner")]
	protected int m_iCommanderStockDefault;

	[Attribute("0", UIWidgets.EditBox, "Batas total AI per faction buat Commander Spawner. Kalau tercapai, spawn ditunda. 0 = tanpa batas.", category: "Commander Spawner")]
	protected int m_iSpawnerFactionAICap;

	[Attribute("0", UIWidgets.CheckBox, "Boleh beberapa grup berbagi satu gedung (posisi indoor defend / idle / garrison). OFF = satu gedung satu grup.", category: "Combat")]
	protected bool m_bShareBuildings;

	[Attribute("0", UIWidgets.ComboBox, "Preferensi posisi defend / idle: Seimbang (indoor & outdoor dinilai bareng, bonus benteng), Indoor, Outdoor.", enums: { ParamEnum("Balanced", "0"), ParamEnum("Indoor", "1"), ParamEnum("Outdoor", "2") }, category: "Combat")]
	protected int m_iCoverPreference;

	[Attribute("1", UIWidgets.CheckBox, "Squad AI saling berbagi kontak musuh (suara / radio pendek) tanpa lewat commander.", category: "Contact Sharing")]
	protected bool m_bShareEnabled;

	[Attribute("100", UIWidgets.Slider, "Jangkauan suara (m): teriak ke squad kawan, gak perlu radio.", params: "20 300 5", category: "Contact Sharing")]
	protected float m_fShareVoiceRange;

	[Attribute("400", UIWidgets.Slider, "Jangkauan radio pendek (m).", params: "100 1500 25", category: "Contact Sharing")]
	protected float m_fShareRadioRange;

	[Attribute("8", UIWidgets.Slider, "Noise posisi dasar (m) per 100 m jarak pengamat ke kontak.", params: "0 50 1", category: "Contact Sharing")]
	protected float m_fShareNoisePer100m;

	[Attribute("1.5", UIWidgets.Slider, "Pengali noise jalur suara (lebih kasar).", params: "0.5 5 0.1", category: "Contact Sharing")]
	protected float m_fShareVoiceNoise;

	[Attribute("1", UIWidgets.Slider, "Pengali noise jalur radio pendek.", params: "0.5 5 0.1", category: "Contact Sharing")]
	protected float m_fShareRadioNoise;

	[Attribute("0", UIWidgets.CheckBox, "Wajib radio: jalur radio pendek cuma jalan kalau pengirim dan penerima bawa radio.", category: "Contact Sharing")]
	protected bool m_bShareRequireRadio;

	[Attribute("2", UIWidgets.Slider, "Jumlah lompatan (1 = cuma langsung, 2 = penerima boleh nerusin sekali).", params: "1 2 1", category: "Contact Sharing")]
	protected int m_iShareHops;

	[Attribute("0", UIWidgets.CheckBox, "Kontak titipan ditandai di peta pemain kawan (marker musuh AI vanilla).", category: "Contact Sharing")]
	protected bool m_bShareMapMarkers;

	[Attribute("1", UIWidgets.CheckBox, "Pemain kawan dalam jangkauan dapat notifikasi kecil waktu squad AI berbagi kontak.", category: "Contact Sharing")]
	protected bool m_bShareToPlayers;

	[Attribute("0", UIWidgets.CheckBox, "Log tiap kiriman kontak (pengirim -> penerima, lompatan, jalur, noise).", category: "Contact Sharing")]
	protected bool m_bShareDebug;

	[Attribute("1", UIWidgets.CheckBox, "Medic AI boleh merawat korban kawan di luar grupnya kalau grup korban gak punya medic yang bisa datang.", category: "Cross-Group Medic")]
	protected bool m_bMedicCrossGroup;

	[Attribute("150", UIWidgets.Slider, "Radius (m) cari medic grup lain buat korban berdarah.", params: "25 500 5", category: "Cross-Group Medic")]
	protected float m_fMedicRadiusBleeding;

	[Attribute("300", UIWidgets.Slider, "Radius (m) cari medic grup lain buat korban pingsan.", params: "25 800 5", category: "Cross-Group Medic")]
	protected float m_fMedicRadiusUnconscious;

	[Attribute("1", UIWidgets.CheckBox, "Pemain kawan ikut dirawat medic AI.", category: "Cross-Group Medic")]
	protected bool m_bMedicIncludePlayers;

	[Attribute("1", UIWidgets.CheckBox, "Korban PINGSAN boleh dijemput di bawah tembakan (medic gak lagi ditembaki langsung, pakai asap). Korban berdarah selalu nunggu aman.", category: "Cross-Group Medic")]
	protected bool m_bMedicUnderFire;

	[Attribute("1", UIWidgets.Slider, "Maksimal medic yang dipinjamkan satu grup dalam satu waktu.", params: "1 4 1", category: "Cross-Group Medic")]
	protected int m_iMedicMaxLentPerGroup;

	[Attribute("90", UIWidgets.Slider, "Detik sebelum pinjaman medic dianggap gagal dan dicoba medic lain.", params: "20 300 5", category: "Cross-Group Medic")]
	protected float m_fMedicTimeout;

	[Attribute("1", UIWidgets.Slider, "Overlay peta grup AI kawan buat pemain. 0 = mati, 1 = hanya squad yang terikat support ke grup pemain atau punya objective yang sama, 2 = semua.", params: "0 2 1", category: "Player Awareness")]
	protected int m_iOverlayMode;

	[Attribute("1500", UIWidgets.Slider, "Radius (m) dasar objective di overlay mode relevan (objective dalam 2x radius ini ditampilkan).", params: "200 5000 50", category: "Player Awareness")]
	protected float m_fOverlayRadius;

	[Attribute("800", UIWidgets.Slider, "Radius (m) siaran radio operasi ke pemain di sekitar objective.", params: "100 3000 50", category: "Player Awareness")]
	protected float m_fRadioRadius;

	[Attribute("400", UIWidgets.Slider, "Radius (m) peringatan artileri ke pemain kawan di sekitar titik tembak.", params: "100 1500 25", category: "Player Awareness")]
	protected float m_fArtyWarnRadius;

	[Attribute("500", UIWidgets.Slider, "Jarak maksimum (m) musuh yang masih mau diinvestigasi grup.", params: "0 1500 10", category: "Investigate")]
	protected float m_fInvestigateMaxDist;

	[Attribute("1", UIWidgets.Slider, "Peluang grup investigasi kontak yang belum teridentifikasi. Di-roll sekali per cluster.", params: "0 1 0.05", category: "Investigate")]
	protected float m_fInvestigateChance;

	[Attribute("1", UIWidgets.CheckBox, "Baca override config dari file JSON di folder profile server. Kalau OFF, nilai di atas dipake apa adanya.", category: "Server Config")]
	protected bool m_bUseServerConfigFile;

	[Attribute("$profile:DCO/DCO_GlobalConfig.json", UIWidgets.EditBox, "Path file JSON config. Prefix $profile: nunjuk ke folder -profile server.", category: "Server Config")]
	protected string m_sServerConfigPath;

	[Attribute("1", UIWidgets.CheckBox, "Kalau file JSON belum ada, otomatis bikin file berisi nilai Workbench sekarang. Kalau sudah ada tapi ada key yang belum tercantum, file lama disalin ke .bak lalu key yang kurang ditambah (nilai admin tetap).", category: "Server Config")]
	protected bool m_bAutoGenerateConfigFile;

	protected static bool s_bConfigLoaded = false;

	static DCO_GlobalAIComponent m_sInstance;

	static DCO_GlobalAIComponent GetInstance()
	{
		if (m_sInstance && !s_bConfigLoaded)
		{
			s_bConfigLoaded = true;
			m_sInstance.LoadServerConfig();
		}

		return m_sInstance;
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		ApplyPerfProfiling();
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		DCO_Perf.Tick(timeSlice);
	}

	override void OnDelete(IEntity owner)
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(PushShareToPlayers);
		DCO_Perf.WriteSummary("end");
		super.OnDelete(owner);
	}

	bool GetPerfProfiling()						{ return m_bPerfProfiling; }

	void SetPerfProfiling(bool b)
	{
		m_bPerfProfiling = b;
		ApplyPerfProfiling();
	}

	protected void ApplyPerfProfiling()
	{
		bool on = m_bPerfProfiling && Replication.IsServer();
		DCO_Perf.SetEnabled(on);
		if (on)
			SetEventMask(GetOwner(), EntityEvent.FRAME);
		else
			ClearEventMask(GetOwner(), EntityEvent.FRAME);
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		m_sInstance = this;
		s_bConfigLoaded = false;

		SetEventMask(owner, EntityEvent.INIT);
		if (Replication.IsServer())
			GetGame().GetCallqueue().CallLater(PushShareToPlayers, SHARE_PUSH_MS, true);
	}

	protected static const int SHARE_PUSH_MS = 5000;

	protected void PushShareToPlayers()
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm)
			return;
		array<int> players = {};
		pm.GetPlayers(players);
		foreach (int pid : players)
		{
			SCR_PlayerControllerGroupComponent comp = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(pid);
			if (comp)
				comp.DCO_SetServerShare(m_bShareToPlayers);
		}
	}

	protected static const int CFG_LOAD = 0;
	protected static const int CFG_WRITE = 1;
	protected static const int CFG_DUMP = 2;
	protected static const int CFG_DUMP_PER_LINE = 6;

	protected int m_iCfgMode;
	protected ref SCR_JsonLoadContext m_CfgLoad;
	protected ref SCR_JsonSaveContext m_CfgSave;
	protected ref array<string> m_aCfgMissing = {};
	protected int m_iCfgApplied;
	protected string m_sCfgCat;
	protected string m_sCfgLine;
	protected int m_iCfgLineKeys;

	protected void SyncConfig()
	{
		CfgCat("Unit");
		unitAimSkillAccuracy = CfgF("unitAimSkillAccuracy", unitAimSkillAccuracy, 0.1, 10.0);
		m_fTimeToMaxAccuracy = CfgF("timeToMaxAccuracy", m_fTimeToMaxAccuracy, 1.0, 60.0);
		m_fAiPerception = CfgF("aiPerception", m_fAiPerception, 0.5, 5.0);
		m_fVehicleDismountDanger = CfgF("vehicleDismountDanger", m_fVehicleDismountDanger, 0.0, 2000.0);
		m_fSuppressionEffect = CfgF("suppressionEffect", m_fSuppressionEffect, 0.0, 2.0);
		m_fTakeCoverChance = CfgF("takeCoverChance", m_fTakeCoverChance, 0.0, 1.0);
		m_bIsMagicallyResupplied = CfgB("magicallyResupplied", m_bIsMagicallyResupplied);
		m_eAISkillDefault = CfgI("aiSkillDefault", m_eAISkillDefault, DCO_AISKILL.NOOB, DCO_AISKILL.TERMINATOR);

		CfgCat("Personality");
		m_fPersonalityWeightStandard = CfgF("personalityWeightMeasured", m_fPersonalityWeightStandard, 0.0, 100.0);
		m_fPersonalityWeightCautious = CfgF("personalityWeightCautious", m_fPersonalityWeightCautious, 0.0, 100.0);
		m_fPersonalityWeightAggressive = CfgF("personalityWeightAggressive", m_fPersonalityWeightAggressive, 0.0, 100.0);
		m_fPersonalityWeightReckless = CfgF("personalityWeightReckless", m_fPersonalityWeightReckless, 0.0, 100.0);

		CfgCat("Dodge");
		m_fDodgeChance = CfgF("dodgeChance", m_fDodgeChance, 0.0, 1.0);
		m_fDodgeCooldown = CfgF("dodgeCooldown", m_fDodgeCooldown, 0.0, 120.0);
		m_fDodgeMaxDist = CfgF("dodgeMaxDist", m_fDodgeMaxDist, 0.0, 1000.0);
		m_fDodgeSearchDist = CfgF("dodgeSearchDist", m_fDodgeSearchDist, 5.0, 100.0);
		m_bDodgeScaleByPersonality = CfgB("dodgeScaleByPersonality", m_bDodgeScaleByPersonality);
		m_iDodgeShotThreshold = CfgI("dodgeShotThreshold", m_iDodgeShotThreshold, 1, 20);
		m_fDodgeShotWindow = CfgF("dodgeShotWindow", m_fDodgeShotWindow, 1.0, 30.0);

		CfgCat("Weapon Usage");
		m_fGrenadeUsage = CfgF("grenadeUsage", m_fGrenadeUsage, 0.0, 1.0);
		m_fGLUsage = CfgF("glUsage", m_fGLUsage, 0.0, 1.0);
		m_fGLAccuracy = CfgF("glAccuracy", m_fGLAccuracy, 0.1, 5.0);
		m_fSmokeUsage = CfgF("smokeUsage", m_fSmokeUsage, 0.0, 1.0);

		CfgCat("Performance");
		m_eLODMode = CfgI("lodMode", m_eLODMode, DCO_EAILODMode.VANILLA, DCO_EAILODMode.FULL_DETAIL);
		m_bPerfProfiling = CfgB("perfProfiling", m_bPerfProfiling);

		CfgCat("Idle");
		m_fIdleEnterTime = CfgF("idleEnterTime", m_fIdleEnterTime, 5.0, 300.0);
		m_fIdleCoverSearchDist = CfgF("idleCoverSearchDist", m_fIdleCoverSearchDist, 10.0, 200.0);
		m_fIdleOverrunDist = CfgF("idleOverrunDist", m_fIdleOverrunDist, 0.0, 100.0);
		m_bIdleLeaveToAssist = CfgB("idleLeaveToAssist", m_bIdleLeaveToAssist);
		m_bIdleLeaveToInvestigate = CfgB("idleLeaveToInvestigate", m_bIdleLeaveToInvestigate);
		m_bIdleFollowCommander = CfgB("idleFollowCommander", m_bIdleFollowCommander);

		CfgCat("Straggler");
		m_bStragglerEnabled = CfgB("stragglerEnabled", m_bStragglerEnabled);
		m_fStragglerTime = CfgF("stragglerTime", m_fStragglerTime, 5.0, 120.0);
		m_fStragglerStuckTime = CfgF("stragglerStuckTime", m_fStragglerStuckTime, 10.0, 300.0);
		m_bStragglerLog = CfgB("stragglerLog", m_bStragglerLog);

		CfgCat("Combat");
		m_fSuperiorRatio = CfgF("superiorRatio", m_fSuperiorRatio, 1.0, 5.0);
		m_bLongRangeHold = CfgB("longRangeHold", m_bLongRangeHold);
		m_fRifleEffectiveRange = CfgF("rifleEffectiveRange", m_fRifleEffectiveRange, 100.0, 800.0);
		m_fMGEffectiveRange = CfgF("mgEffectiveRange", m_fMGEffectiveRange, 200.0, 1500.0);
		m_bShareBuildings = CfgB("shareBuildings", m_bShareBuildings);
		m_iCoverPreference = CfgI("coverPreference", m_iCoverPreference, 0, 2);
		m_bOvermatchAssault = CfgB("overmatchAssault", m_bOvermatchAssault);
		m_fOvermatchMaxDist = CfgF("overmatchMaxDist", m_fOvermatchMaxDist, 100.0, 500.0);

		CfgCat("CQB");
		m_bCQBEnabled = CfgB("cqbEnabled", m_bCQBEnabled);
		m_iCQBIsolateMin = CfgI("cqbIsolateMin", m_iCQBIsolateMin, 3, 12);
		m_fCQBStackWait = CfgF("cqbStackWait", m_fCQBStackWait, 0.0, 30.0);
		m_iCQBAbortLosses = CfgI("cqbAbortLosses", m_iCQBAbortLosses, 1, 6);
		m_bCQBCaptureWaits = CfgB("cqbCaptureWaits", m_bCQBCaptureWaits);
		m_fCQBContactTimeout = CfgF("cqbContactTimeout", m_fCQBContactTimeout, 30.0, 600.0);

		CfgCat("Night");
		m_bNightEnabled = CfgB("nightEnabled", m_bNightEnabled);
		m_fNightFlareCooldown = CfgF("nightFlareCooldown", m_fNightFlareCooldown, 30.0, 300.0);
		m_fNightEngageMul = CfgF("nightEngageMul", m_fNightEngageMul, 0.2, 1.0);
		m_bNightLightDiscipline = CfgB("nightLightDiscipline", m_bNightLightDiscipline);

		CfgCat("Contact Report");
		m_iContactReportMaxPerScan = CfgI("contactReportMaxPerScan", m_iContactReportMaxPerScan, 0, 10);
		m_fContactReportScanInterval = CfgF("contactReportScanInterval", m_fContactReportScanInterval, 0.0, 60.0);

		CfgCat("Commander Spawner");
		m_iCommanderStockDefault = CfgI("commanderStock", m_iCommanderStockDefault, -1, 100000);
		m_iSpawnerFactionAICap = CfgI("spawnerFactionAICap", m_iSpawnerFactionAICap, 0, 2000);

		CfgCat("Contact Sharing");
		m_bShareEnabled = CfgB("shareEnabled", m_bShareEnabled);
		m_fShareVoiceRange = CfgF("shareVoiceRange", m_fShareVoiceRange, 20.0, 300.0);
		m_fShareRadioRange = CfgF("shareRadioRange", m_fShareRadioRange, 100.0, 1500.0);
		m_fShareNoisePer100m = CfgF("shareNoisePer100m", m_fShareNoisePer100m, 0.0, 50.0);
		m_fShareVoiceNoise = CfgF("shareVoiceNoise", m_fShareVoiceNoise, 0.5, 5.0);
		m_fShareRadioNoise = CfgF("shareRadioNoise", m_fShareRadioNoise, 0.5, 5.0);
		m_bShareRequireRadio = CfgB("shareRequireRadio", m_bShareRequireRadio);
		m_iShareHops = CfgI("shareHops", m_iShareHops, 1, 2);
		m_bShareMapMarkers = CfgB("shareMapMarkers", m_bShareMapMarkers);
		m_bShareToPlayers = CfgB("shareToPlayers", m_bShareToPlayers);
		m_bShareDebug = CfgB("shareDebug", m_bShareDebug);

		CfgCat("Cross-Group Medic");
		m_bMedicCrossGroup = CfgB("medicCrossGroup", m_bMedicCrossGroup);
		m_fMedicRadiusBleeding = CfgF("medicRadiusBleeding", m_fMedicRadiusBleeding, 25.0, 500.0);
		m_fMedicRadiusUnconscious = CfgF("medicRadiusUnconscious", m_fMedicRadiusUnconscious, 25.0, 800.0);
		m_bMedicIncludePlayers = CfgB("medicIncludePlayers", m_bMedicIncludePlayers);
		m_bMedicUnderFire = CfgB("medicUnderFire", m_bMedicUnderFire);
		m_iMedicMaxLentPerGroup = CfgI("medicMaxLentPerGroup", m_iMedicMaxLentPerGroup, 1, 4);
		m_fMedicTimeout = CfgF("medicTimeout", m_fMedicTimeout, 20.0, 300.0);

		CfgCat("Player Awareness");
		m_iOverlayMode = CfgI("overlayMode", m_iOverlayMode, 0, 2);
		m_fOverlayRadius = CfgF("overlayRadius", m_fOverlayRadius, 200.0, 5000.0);
		m_fRadioRadius = CfgF("radioRadius", m_fRadioRadius, 100.0, 3000.0);
		m_fArtyWarnRadius = CfgF("artyWarnRadius", m_fArtyWarnRadius, 100.0, 1500.0);

		CfgCat("Investigate");
		m_fInvestigateMaxDist = CfgF("investigateMaxDist", m_fInvestigateMaxDist, 0.0, 1500.0);
		m_fInvestigateChance = CfgF("investigateChance", m_fInvestigateChance, 0.0, 1.0);

		CfgFlush();
	}

	protected float CfgF(string key, float cur, float min, float max)
	{
		if (m_iCfgMode == CFG_WRITE)
		{
			m_CfgSave.WriteValue(key, cur);
			return cur;
		}
		if (m_iCfgMode == CFG_DUMP)
		{
			CfgDumpAdd(key, cur.ToString());
			return cur;
		}

		float v;
		if (!m_CfgLoad.ReadValue(key, v))
		{
			m_aCfgMissing.Insert(key);
			return cur;
		}
		m_iCfgApplied++;
		return Math.Clamp(v, min, max);
	}

	protected int CfgI(string key, int cur, int min, int max)
	{
		if (m_iCfgMode == CFG_WRITE)
		{
			m_CfgSave.WriteValue(key, cur);
			return cur;
		}
		if (m_iCfgMode == CFG_DUMP)
		{
			CfgDumpAdd(key, cur.ToString());
			return cur;
		}

		int v;
		if (!m_CfgLoad.ReadValue(key, v))
		{
			m_aCfgMissing.Insert(key);
			return cur;
		}
		m_iCfgApplied++;
		return Math.ClampInt(v, min, max);
	}

	protected bool CfgB(string key, bool cur)
	{
		if (m_iCfgMode == CFG_WRITE)
		{
			m_CfgSave.WriteValue(key, cur);
			return cur;
		}
		if (m_iCfgMode == CFG_DUMP)
		{
			CfgDumpAdd(key, cur.ToString());
			return cur;
		}

		bool v;
		if (!m_CfgLoad.ReadValue(key, v))
		{
			m_aCfgMissing.Insert(key);
			return cur;
		}
		m_iCfgApplied++;
		return v;
	}

	protected void CfgCat(string name)
	{
		if (m_iCfgMode != CFG_DUMP)
			return;
		CfgFlush();
		m_sCfgCat = name;
	}

	protected void CfgDumpAdd(string key, string val)
	{
		if (m_iCfgLineKeys >= CFG_DUMP_PER_LINE)
			CfgFlush();
		m_sCfgLine += " " + key + "=" + val;
		m_iCfgLineKeys++;
	}

	protected void CfgFlush()
	{
		if (m_iCfgMode == CFG_DUMP && m_iCfgLineKeys > 0)
			Print("[DCO][Config] " + m_sCfgCat + ":" + m_sCfgLine);
		m_sCfgLine = string.Empty;
		m_iCfgLineKeys = 0;
	}

	protected void LoadServerConfig()
	{
		if (!m_bUseServerConfigFile)
			return;

		if (!Replication.IsServer())
			return;

		if (m_sServerConfigPath.IsEmpty())
		{
			Print("[DCO][Config] Path config kosong, skip.", LogLevel.WARNING);
			return;
		}

		if (!FileIO.FileExists(m_sServerConfigPath))
		{
			PrintFormat("[DCO][Config] File %1 gak ketemu.", m_sServerConfigPath, level: LogLevel.NORMAL);

			if (m_bAutoGenerateConfigFile)
				WriteDefaultConfig();

			DumpActiveConfig();
			return;
		}

		SCR_JsonLoadContext ctx = new SCR_JsonLoadContext();
		if (!ctx.LoadFromFile(m_sServerConfigPath))
		{
			PrintFormat("[DCO][Config] GAGAL parse %1 -- kemungkinan JSON invalid (koma nyangkut / kurung kurang). Pake nilai Workbench.",
				m_sServerConfigPath, level: LogLevel.ERROR);
			DumpActiveConfig();
			return;
		}

		m_iCfgMode = CFG_LOAD;
		m_CfgLoad = ctx;
		m_aCfgMissing.Clear();
		m_iCfgApplied = 0;
		SyncConfig();

		bool migrated = false;
		float legacy;
		if (m_aCfgMissing.Contains("personalityWeightMeasured") && ctx.ReadValue("personalityWeightStandard", legacy))
		{
			m_fPersonalityWeightStandard = Math.Clamp(legacy, 0.0, 100.0);
			m_aCfgMissing.RemoveItem("personalityWeightMeasured");
			m_iCfgApplied++;
			migrated = true;
		}
		m_CfgLoad = null;

		PrintFormat("[DCO][Config] %1 setting di-override dari %2", m_iCfgApplied, m_sServerConfigPath);

		if (m_bAutoGenerateConfigFile && (migrated || !m_aCfgMissing.IsEmpty()))
			MergeMissingKeys(migrated);

		ApplyPerfProfiling();
		DumpActiveConfig();
	}

	protected void MergeMissingKeys(bool migrated)
	{
		string bak = m_sServerConfigPath + ".bak";
		if (FileIO.FileExists(bak))
			FileIO.DeleteFile(bak);
		if (!SCR_FileIOHelper.CopyFile(m_sServerConfigPath, bak))
		{
			PrintFormat("[DCO][Config] GAGAL backup ke %1 -- file config gak diubah.", bak, level: LogLevel.ERROR);
			return;
		}

		if (!WriteDefaultConfig())
			return;

		string added;
		foreach (string key : m_aCfgMissing)
		{
			if (!added.IsEmpty())
				added += ", ";
			added += key;
		}
		PrintFormat("[DCO][Config] %1 key baru ditambah ke %2 (backup: %3): %4", m_aCfgMissing.Count(), m_sServerConfigPath, bak, added);
		if (migrated)
			Print("[DCO][Config] personalityWeightStandard diganti jadi personalityWeightMeasured");
	}

	protected bool WriteDefaultConfig()
	{
		string dir = GetDirectoryFromPath(m_sServerConfigPath);
		if (!dir.IsEmpty())
			FileIO.MakeDirectory(dir);

		m_CfgSave = new SCR_JsonSaveContext();
		m_CfgSave.WriteValue("_comment", "DCO Global AI config. Nilai di file ini dipakai server. Key yang belum ada otomatis ditambah pakai nilai Workbench waktu server start (file lama disalin ke .bak). Mau key ikut nilai Workbench? Matikan Auto Generate lalu hapus barisnya.");

		m_iCfgMode = CFG_WRITE;
		SyncConfig();
		m_iCfgMode = CFG_LOAD;

		bool ok = m_CfgSave.SaveToFile(m_sServerConfigPath);
		m_CfgSave = null;

		if (ok)
			PrintFormat("[DCO][Config] Config ditulis ke %1", m_sServerConfigPath);
		else
			PrintFormat("[DCO][Config] GAGAL nulis %1 -- cek permission folder profile.", m_sServerConfigPath, level: LogLevel.ERROR);
		return ok;
	}

	protected string GetDirectoryFromPath(string path)
	{
		int lastSlash = -1;
		int len = path.Length();

		for (int i = 0; i < len; i++)
		{
			string ch = path.Get(i);
			if (ch == "/" || ch == "\\")
				lastSlash = i;
		}

		if (lastSlash <= 0)
			return string.Empty;

		return path.Substring(0, lastSlash);
	}

	void DumpActiveConfig()
	{
		m_iCfgMode = CFG_DUMP;
		SyncConfig();
		m_iCfgMode = CFG_LOAD;
	}

	float GetDodgeChance()
	{
		return m_fDodgeChance;
	}

	float SetDodgeChance(float f)
	{
		m_fDodgeChance = f;
		return m_fDodgeChance;
	}

	float GetDodgeCooldown()
	{
		return m_fDodgeCooldown;
	}

	float SetDodgeCooldown(float f)
	{
		m_fDodgeCooldown = f;
		return m_fDodgeCooldown;
	}

	float GetDodgeMaxDist()
	{
		return m_fDodgeMaxDist;
	}

	float SetDodgeMaxDist(float f)
	{
		m_fDodgeMaxDist = f;
		return m_fDodgeMaxDist;
	}

	float GetDodgeSearchDist()
	{
		return m_fDodgeSearchDist;
	}

	float SetDodgeSearchDist(float f)
	{
		m_fDodgeSearchDist = f;
		return m_fDodgeSearchDist;
	}

	bool GetDodgeScaleByPersonality()
	{
		return m_bDodgeScaleByPersonality;
	}

	bool SetDodgeScaleByPersonality(bool b)
	{
		m_bDodgeScaleByPersonality = b;
		return m_bDodgeScaleByPersonality;
	}

	int GetDodgeShotThreshold()
	{
		return m_iDodgeShotThreshold;
	}

	int SetDodgeShotThreshold(int i)
	{
		m_iDodgeShotThreshold = Math.ClampInt(i, 1, 20);
		return m_iDodgeShotThreshold;
	}

	float GetDodgeShotWindow()
	{
		return m_fDodgeShotWindow;
	}

	float SetDodgeShotWindow(float f)
	{
		m_fDodgeShotWindow = Math.Clamp(f, 1.0, 30.0);
		return m_fDodgeShotWindow;
	}

	void ReloadServerConfig()
	{
		if (!Replication.IsServer())
			return;

		s_bConfigLoaded = true;
		LoadServerConfig();
	}

	DCO_AISKILL SetAISkill(DCO_AISKILL ski)
	{
		m_eAISkillDefault = ski;
		return m_eAISkillDefault;
	}

	DCO_AISKILL GetAISkill()
	{
		return m_eAISkillDefault;
	}

	float GetDismountDistance()
	{
		return m_fVehicleDismountDanger;
	}

	float SetDismountDistance(float acc)
	{
		m_fVehicleDismountDanger = acc;
		return m_fVehicleDismountDanger;
	}

	float GetAccuracyTime()
	{
		return m_fTimeToMaxAccuracy;
	}

	float SetAccuracyTime(float acc)
	{
		m_fTimeToMaxAccuracy = acc;
		return m_fTimeToMaxAccuracy;
	}

	float GetUnitSkill()
	{
		return unitAimSkillAccuracy;
	}

	float SetUnitSkill(float sk)
	{
		unitAimSkillAccuracy = sk;
		return sk;
	}

	float GetUnitPerception()
	{
		return m_fAiPerception;
	}

	float SetUnitPerception(float sk)
	{
		m_fAiPerception = sk;
		return sk;
	}

	bool GetUnitMagicMagazine()
	{
		return m_bIsMagicallyResupplied;
	}

	bool SetUnitMagicMagazine(bool s)
	{
		m_bIsMagicallyResupplied = s;
		return m_bIsMagicallyResupplied;
	}

	float GetSuppressionEffect()
	{
		return m_fSuppressionEffect;
	}

	float SetSuppressionEffect(float f)
	{
		m_fSuppressionEffect = f;
		return m_fSuppressionEffect;
	}

	float GetTakeCoverChance()
	{
		return m_fTakeCoverChance;
	}

	float SetTakeCoverChance(float f)
	{
		m_fTakeCoverChance = f;
		return m_fTakeCoverChance;
	}

	float GetGrenadeUsage()
	{
		return m_fGrenadeUsage;
	}

	float SetGrenadeUsage(float f)
	{
		m_fGrenadeUsage = Math.Clamp(f, 0.0, 1.0);
		return m_fGrenadeUsage;
	}

	float GetGLUsage()
	{
		return m_fGLUsage;
	}

	float SetGLUsage(float f)
	{
		m_fGLUsage = Math.Clamp(f, 0.0, 1.0);
		return m_fGLUsage;
	}

	float GetGLAccuracy()
	{
		return m_fGLAccuracy;
	}

	float SetGLAccuracy(float f)
	{
		m_fGLAccuracy = Math.Clamp(f, 0.1, 5.0);
		return m_fGLAccuracy;
	}

	float GetSmokeUsage()
	{
		return m_fSmokeUsage;
	}

	float SetSmokeUsage(float f)
	{
		m_fSmokeUsage = Math.Clamp(f, 0.0, 1.0);
		return m_fSmokeUsage;
	}

	float GetIdleEnterTime()					{ return m_fIdleEnterTime; }
	void SetIdleEnterTime(float f)				{ m_fIdleEnterTime = Math.Clamp(f, 5.0, 300.0); }
	float GetIdleCoverSearchDist()				{ return m_fIdleCoverSearchDist; }
	void SetIdleCoverSearchDist(float f)		{ m_fIdleCoverSearchDist = Math.Clamp(f, 10.0, 200.0); }
	float GetIdleOverrunDist()					{ return m_fIdleOverrunDist; }
	void SetIdleOverrunDist(float f)			{ m_fIdleOverrunDist = Math.Clamp(f, 0.0, 100.0); }
	bool GetIdleLeaveToAssist()					{ return m_bIdleLeaveToAssist; }
	void SetIdleLeaveToAssist(bool b)			{ m_bIdleLeaveToAssist = b; }
	bool GetIdleLeaveToInvestigate()			{ return m_bIdleLeaveToInvestigate; }
	void SetIdleLeaveToInvestigate(bool b)		{ m_bIdleLeaveToInvestigate = b; }
	bool GetIdleFollowCommander()				{ return m_bIdleFollowCommander; }
	void SetIdleFollowCommander(bool b)			{ m_bIdleFollowCommander = b; }
	bool GetStragglerEnabled()					{ return m_bStragglerEnabled; }
	float GetStragglerTime()					{ return m_fStragglerTime; }
	float GetStragglerStuckTime()				{ return m_fStragglerStuckTime; }
	bool GetStragglerLog()						{ return m_bStragglerLog; }

	int GetOverlayMode()						{ return m_iOverlayMode; }
	void SetOverlayMode(float f)				{ m_iOverlayMode = Math.Clamp(Math.Round(f), 0, 2); }
	float GetOverlayRadius()					{ return m_fOverlayRadius; }
	void SetOverlayRadius(float f)				{ m_fOverlayRadius = Math.Clamp(f, 200.0, 5000.0); }
	float GetRadioRadius()						{ return m_fRadioRadius; }
	void SetRadioRadius(float f)				{ m_fRadioRadius = Math.Clamp(f, 100.0, 3000.0); }
	float GetArtyWarnRadius()					{ return m_fArtyWarnRadius; }
	void SetArtyWarnRadius(float f)				{ m_fArtyWarnRadius = Math.Clamp(f, 100.0, 1500.0); }

	float GetSuperiorRatio()					{ return m_fSuperiorRatio; }
	void SetSuperiorRatio(float f)				{ m_fSuperiorRatio = Math.Clamp(f, 1.0, 5.0); }
	bool GetLongRangeHold()						{ return m_bLongRangeHold; }
	void SetLongRangeHold(bool b)				{ m_bLongRangeHold = b; }
	float GetRifleEffectiveRange()				{ return m_fRifleEffectiveRange; }
	void SetRifleEffectiveRange(float f)		{ m_fRifleEffectiveRange = Math.Clamp(f, 100.0, 800.0); }
	float GetMGEffectiveRange()					{ return m_fMGEffectiveRange; }
	void SetMGEffectiveRange(float f)			{ m_fMGEffectiveRange = Math.Clamp(f, 200.0, 1500.0); }
	bool GetShareBuildings()					{ return m_bShareBuildings; }
	void SetShareBuildings(bool b)				{ m_bShareBuildings = b; }
	int GetCoverPreference()					{ return m_iCoverPreference; }
	void SetCoverPreference(float f)			{ m_iCoverPreference = Math.Clamp(Math.Round(f), 0, 2); }
	bool GetOvermatchAssault()					{ return m_bOvermatchAssault; }
	void SetOvermatchAssault(bool b)			{ m_bOvermatchAssault = b; }
	float GetOvermatchMaxDist()					{ return m_fOvermatchMaxDist; }
	void SetOvermatchMaxDist(float f)			{ m_fOvermatchMaxDist = Math.Clamp(f, 100.0, 500.0); }
	bool GetNightEnabled()						{ return m_bNightEnabled; }
	void SetNightEnabled(bool b)				{ m_bNightEnabled = b; }
	int GetCommanderStockDefault()				{ return m_iCommanderStockDefault; }
	void SetCommanderStockDefault(int i)		{ m_iCommanderStockDefault = Math.Clamp(i, -1, 100000); }
	int GetSpawnerFactionAICap()				{ return m_iSpawnerFactionAICap; }
	void SetSpawnerFactionAICap(int i)			{ m_iSpawnerFactionAICap = Math.Clamp(i, 0, 2000); }
	int GetContactReportMaxPerScan()			{ return m_iContactReportMaxPerScan; }
	void SetContactReportMaxPerScan(int i)		{ m_iContactReportMaxPerScan = Math.Clamp(i, 0, 10); }
	float GetContactReportScanInterval()		{ return m_fContactReportScanInterval; }
	void SetContactReportScanInterval(float f)	{ m_fContactReportScanInterval = Math.Clamp(f, 0.0, 60.0); }
	float GetNightFlareCooldown()				{ return m_fNightFlareCooldown; }
	void SetNightFlareCooldown(float f)			{ m_fNightFlareCooldown = Math.Clamp(f, 30.0, 300.0); }
	float GetNightEngageMul()					{ return m_fNightEngageMul; }
	void SetNightEngageMul(float f)				{ m_fNightEngageMul = Math.Clamp(f, 0.2, 1.0); }
	bool GetNightLightDiscipline()				{ return m_bNightLightDiscipline; }
	void SetNightLightDiscipline(bool b)		{ m_bNightLightDiscipline = b; }
	bool GetCQBEnabled()						{ return m_bCQBEnabled; }
	void SetCQBEnabled(bool b)					{ m_bCQBEnabled = b; }
	int GetCQBIsolateMin()						{ return m_iCQBIsolateMin; }
	void SetCQBIsolateMin(float f)				{ m_iCQBIsolateMin = Math.Clamp(Math.Round(f), 3, 12); }
	float GetCQBStackWait()						{ return m_fCQBStackWait; }
	void SetCQBStackWait(float f)				{ m_fCQBStackWait = Math.Clamp(f, 0.0, 30.0); }
	int GetCQBAbortLosses()						{ return m_iCQBAbortLosses; }
	void SetCQBAbortLosses(float f)				{ m_iCQBAbortLosses = Math.Clamp(Math.Round(f), 1, 6); }
	bool GetCQBCaptureWaits()					{ return m_bCQBCaptureWaits; }
	void SetCQBCaptureWaits(bool b)				{ m_bCQBCaptureWaits = b; }
	float GetCQBContactTimeout()				{ return m_fCQBContactTimeout; }
	void SetCQBContactTimeout(float f)			{ m_fCQBContactTimeout = Math.Clamp(f, 30.0, 600.0); }

	bool GetShareEnabled()						{ return m_bShareEnabled; }
	void SetShareEnabled(bool b)				{ m_bShareEnabled = b; }
	float GetShareVoiceRange()					{ return m_fShareVoiceRange; }
	void SetShareVoiceRange(float f)			{ m_fShareVoiceRange = Math.Clamp(f, 20.0, 300.0); }
	float GetShareRadioRange()					{ return m_fShareRadioRange; }
	void SetShareRadioRange(float f)			{ m_fShareRadioRange = Math.Clamp(f, 100.0, 1500.0); }
	float GetShareNoisePer100m()				{ return m_fShareNoisePer100m; }
	void SetShareNoisePer100m(float f)			{ m_fShareNoisePer100m = Math.Clamp(f, 0.0, 50.0); }
	float GetShareVoiceNoise()					{ return m_fShareVoiceNoise; }
	void SetShareVoiceNoise(float f)			{ m_fShareVoiceNoise = Math.Clamp(f, 0.5, 5.0); }
	float GetShareRadioNoise()					{ return m_fShareRadioNoise; }
	void SetShareRadioNoise(float f)			{ m_fShareRadioNoise = Math.Clamp(f, 0.5, 5.0); }
	bool GetShareRequireRadio()					{ return m_bShareRequireRadio; }
	void SetShareRequireRadio(bool b)			{ m_bShareRequireRadio = b; }
	int GetShareHops()							{ return m_iShareHops; }
	void SetShareHops(float f)					{ m_iShareHops = Math.Clamp(Math.Round(f), 1, 2); }
	bool GetShareMapMarkers()					{ return m_bShareMapMarkers; }
	void SetShareMapMarkers(bool b)				{ m_bShareMapMarkers = b; }
	bool GetShareToPlayers()					{ return m_bShareToPlayers; }
	void SetShareToPlayers(bool b)				{ m_bShareToPlayers = b; PushShareToPlayers(); }
	bool GetShareDebug()						{ return m_bShareDebug; }
	void SetShareDebug(bool b)					{ m_bShareDebug = b; }

	bool GetMedicCrossGroup()					{ return m_bMedicCrossGroup; }
	void SetMedicCrossGroup(bool b)				{ m_bMedicCrossGroup = b; }
	float GetMedicRadiusBleeding()				{ return m_fMedicRadiusBleeding; }
	void SetMedicRadiusBleeding(float f)		{ m_fMedicRadiusBleeding = Math.Clamp(f, 25.0, 500.0); }
	float GetMedicRadiusUnconscious()			{ return m_fMedicRadiusUnconscious; }
	void SetMedicRadiusUnconscious(float f)		{ m_fMedicRadiusUnconscious = Math.Clamp(f, 25.0, 800.0); }
	bool GetMedicIncludePlayers()				{ return m_bMedicIncludePlayers; }
	void SetMedicIncludePlayers(bool b)			{ m_bMedicIncludePlayers = b; }
	bool GetMedicUnderFire()					{ return m_bMedicUnderFire; }
	void SetMedicUnderFire(bool b)				{ m_bMedicUnderFire = b; }
	int GetMedicMaxLentPerGroup()				{ return m_iMedicMaxLentPerGroup; }
	void SetMedicMaxLentPerGroup(float f)		{ m_iMedicMaxLentPerGroup = Math.Clamp(Math.Round(f), 1, 4); }
	float GetMedicTimeout()						{ return m_fMedicTimeout; }
	void SetMedicTimeout(float f)				{ m_fMedicTimeout = Math.Clamp(f, 20.0, 300.0); }

	float GetInvestigateMaxDist()				{ return m_fInvestigateMaxDist; }
	void SetInvestigateMaxDist(float f)			{ m_fInvestigateMaxDist = Math.Clamp(f, 0.0, 1500.0); }
	float GetInvestigateChance()				{ return m_fInvestigateChance; }
	void SetInvestigateChance(float f)			{ m_fInvestigateChance = Math.Clamp(f, 0.0, 1.0); }

	DCO_EAILODMode GetLODMode()
	{
		return m_eLODMode;
	}

	void SetLODMode(DCO_EAILODMode mode)
	{
		mode = Math.ClampInt(mode, DCO_EAILODMode.VANILLA, DCO_EAILODMode.FULL_DETAIL);
		if (mode == m_eLODMode || !Replication.IsServer())
			return;

		m_eLODMode = mode;

		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return;

		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (SCR_ChimeraAIAgent.Cast(agent))
				ApplyLOD(agent, true);
		}
	}

	static void ApplyLOD(notnull AIAgent agent, bool reset = false)
	{
		DCO_EAILODMode mode = DCO_EAILODMode.PREVENT_MAX_LOD;
		DCO_GlobalAIComponent global = GetInstance();
		if (global)
			mode = global.m_eLODMode;

		if (reset)
		{
			agent.SetPermanentLOD(-1);
			agent.PreventMaxLOD(0);
		}

		switch (mode)
		{
			case DCO_EAILODMode.PREVENT_MAX_LOD:
				agent.PreventMaxLOD();
				break;
			case DCO_EAILODMode.FULL_DETAIL:
				agent.SetPermanentLOD(0);
				break;
		}
	}

	DCO_EAIPersonality RollWeightedPersonality()
	{
		float total = m_fPersonalityWeightStandard + m_fPersonalityWeightCautious
			+ m_fPersonalityWeightAggressive + m_fPersonalityWeightReckless;

		if (total <= 0.0)
			return DCO_EAIPersonality.STANDARD;

		float roll = Math.RandomFloat(0.0, total);

		if (roll < m_fPersonalityWeightStandard)
			return DCO_EAIPersonality.STANDARD;
		roll -= m_fPersonalityWeightStandard;

		if (roll < m_fPersonalityWeightCautious)
			return DCO_EAIPersonality.CAUTIOUS;
		roll -= m_fPersonalityWeightCautious;

		if (roll < m_fPersonalityWeightAggressive)
			return DCO_EAIPersonality.AGGRESSIVE;

		return DCO_EAIPersonality.RECKLESS;
	}
}