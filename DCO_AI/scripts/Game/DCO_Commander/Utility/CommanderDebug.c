//------------------------------------------------------------------------------------------------
//! Helper bersama buat overlay debug 3D, Game Master only.
//!
//! File ini SENGAJA gak punya state dan gak gambar apa-apa sendiri. Isinya cuma
//! primitif yang dipakai bareng: cek GM, spawn teks, palet warna, dan penamaan enum.
//!
//! Kepemilikan gambarnya ada di komponen masing-masing:
//!   objective -> CMD_AICommanderObjectiveComponent  (kontrol, presence, slot)
//!   manager   -> AICommander_ManagerComponent       (faction, tally global)
//!   commander -> AICommander_BaseComponent          (mode, personality, tuning)
//!   group     -> DCO_GroupUtilityComponent          (role, tugas, status order)
//!
//! Tiap komponen punya checkbox m_bDebugMode dan timer sendiri, jadi bisa dinyalain
//! terpisah dan gak ada satu komponen yang harus tau isi komponen lain.
//!
//! Semua teks yang muncul di layar ditulis dalam Bahasa Inggris.
//------------------------------------------------------------------------------------------------
class DCO_DebugDraw
{
	static const int COLOR_OWNED     = 0xFF33CC66;
	static const int COLOR_TARGET    = 0xFFFF5522;
	static const int COLOR_QUEUED    = 0xFFAAAAAA;
	static const int COLOR_STAGING   = 0xFFFFCC00;
	static const int COLOR_COMMANDER = 0xFF33AAFF;
	static const int COLOR_MANAGER   = 0xFFFFFFFF;
	static const int COLOR_FRONTLINE = 0xFF6688FF;
	static const int COLOR_NEUTRAL   = 0xFF666666;
	static const int COLOR_TEXT_BG   = 0x80000000;

	static const int COLOR_ROLE_ASSAULT = 0xFFFF6644;
	static const int COLOR_ROLE_DEFEND  = 0xFF44DD88;
	static const int COLOR_ROLE_RECON   = 0xFFCC66FF;
	static const int COLOR_ROLE_IDLE    = 0xFFBBBBBB;

	//! Bola cuma sebagai PENANDA POSISI, bukan area. Area ditulis sebagai angka.
	static const float MARKER_BIG   = 2.0;
	static const float MARKER_SMALL = 0.8;

	//------------------------------------------------------------------------------------------------
	//! INI SATU-SATUNYA TITIK yang bergantung ke API editor. Kalau gagal kompilasi di
	//! versi Reforger kamu, ganti isinya jadi `return true;` -- checkbox m_bDebugMode
	//! di tiap komponen udah cukup jadi saklarnya, dan sisa file gak perlu disentuh.
	static bool IsLocalPlayerInGM()
	{
		SCR_EditorManagerEntity editor = SCR_EditorManagerEntity.GetInstance();
		if (!editor)
			return false;

		return editor.IsOpened();
	}

	//------------------------------------------------------------------------------------------------
	static DebugTextWorldSpace SpawnText(vector pos, string text, float size, int color)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		return DebugTextWorldSpace.Create(
			world,
			text,
			DebugTextFlags.CENTER | DebugTextFlags.FACE_CAMERA,
			pos[0], pos[1], pos[2],
			size,
			color,
			COLOR_TEXT_BG);
	}

	static int Flags()
	{
		return ShapeFlags.NOZBUFFER | ShapeFlags.TRANSP;
	}

	//------------------------------------------------------------------------------------------------
	static string F1(float v) { return (Math.Round(v * 10.0) / 10.0).ToString(); }
	static string M(float v)  { return Math.Round(v).ToString() + "m"; }

	static string YesNo(bool v)
	{
		if (v)
			return "yes";

		return "no";
	}

	//------------------------------------------------------------------------------------------------
	//! Palet ditulis sebagai switch, bukan static array. Array statis ber-`ref` di
	//! level kelas gampang bermasalah waktu inisialisasi di EnforceScript.
	static int PaletteAt(int index)
	{
		switch (index % 6)
		{
			case 0: return 0xFF4488FF; // biru
			case 1: return 0xFFFF5533; // merah
			case 2: return 0xFF44DD88; // hijau
			case 3: return 0xFFFFCC22; // kuning
			case 4: return 0xFFCC66FF; // ungu
		}
		return 0xFF33CCCC;             // toska
	}

	//! Warna dipilih dari INDEKS faction di daftar manager, bukan dari urutan iterasi
	//! map -- urutan map gak dijamin stabil, dan bar yang warnanya ganti-ganti tiap
	//! refresh gak ada gunanya buat dibaca.
	static int FactionColor(FactionKey fk)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return COLOR_NEUTRAL;

		for (int i = 0; i < mgr.m_aAvailableFactions.Count(); i++)
		{
			if (mgr.m_aAvailableFactions.Get(i) == fk)
				return PaletteAt(i);
		}

		return COLOR_NEUTRAL;
	}

	//------------------------------------------------------------------------------------------------
	static string ModeName(int mode)
	{
		switch (mode)
		{
			case CMD_ECommanderMode.OFFENSIVE: return "OFFENSIVE";
			case CMD_ECommanderMode.DEFENSIVE: return "DEFENSIVE";
			case CMD_ECommanderMode.BALANCED:  return "BALANCED";
		}
		return "UNKNOWN";
	}

	static string CommanderStateName(int state)
	{
		switch (state)
		{
			case CMD_ECommanderState.IDLE:       return "idle";
			case CMD_ECommanderState.PLANNING:   return "planning";
			case CMD_ECommanderState.COMMANDING: return "commanding";
			case CMD_ECommanderState.DEAD:       return "dead";
			case CMD_ECommanderState.REPLACING:  return "replacing";
		}
		return "unknown";
	}

	static string ObjectiveStateName(int state)
	{
		switch (state)
		{
			case CMD_EObjectiveState.PENDING:   return "pending";
			case CMD_EObjectiveState.ASSIGNED:  return "assigned";
			case CMD_EObjectiveState.COMPLETED: return "completed";
			case CMD_EObjectiveState.FAILED:    return "failed";
		}
		return "unknown";
	}

	static string ObjectiveTypeName(int type)
	{
		switch (type)
		{
			case CMD_EObjectiveType.CAPTURE: return "CAPTURE";
			case CMD_EObjectiveType.DESTROY: return "DESTROY";
			case CMD_EObjectiveType.RECON:   return "RECON";
		}
		return "UNKNOWN";
	}

	static string CaptureStatusName(int status)
	{
		switch (status)
		{
			case DCO_ECaptureStatus.CAPTURING: return "CAPTURING";
			case DCO_ECaptureStatus.CONTESTED: return "CONTESTED";
		}
		return "idle";
	}

	static string RoleName(int role)
	{
		switch (role)
		{
			case CMD_EGroupRole.NONE:       return "UNASSIGNED";
			case CMD_EGroupRole.RECON:      return "RECON";
			case CMD_EGroupRole.ASSAULT:    return "ASSAULT";
			case CMD_EGroupRole.FLANK:      return "FLANK";
			case CMD_EGroupRole.TRANSPORT:  return "TRANSPORT";
			case CMD_EGroupRole.RESERVE:    return "RESERVE";
			case CMD_EGroupRole.RETREAT:    return "RETREAT";
			case CMD_EGroupRole.REINFORNCE: return "REINFORCE";
			case CMD_EGroupRole.DEFEND:     return "DEFEND";
			case CMD_EGroupRole.ARMORED:    return "ARMORED";
			case CMD_EGroupRole.ARTILLERY:  return "ARTILLERY";
			case CMD_EGroupRole.SUPPRESS:   return "SUPPRESS";
		}
		return "ROLE " + role.ToString();
	}

	static string GroupStatusName(int status)
	{
		switch (status)
		{
			case DCOG_EGroupStatus.IDLE:              return "idle";
			case DCOG_EGroupStatus.EXECUTING_COMMAND: return "following order";
			case DCOG_EGroupStatus.INITIATIVE:        return "own initiative";
			case DCOG_EGroupStatus.TRANSITING:        return "transiting";
		}
		return "unknown";
	}

	static string ThreatLevelName(int level)
	{
		switch (level)
		{
			case CMD_EThreatLevel.NEGLIGIBLE: return "NEGLIGIBLE";
			case CMD_EThreatLevel.LOW:        return "LOW";
			case CMD_EThreatLevel.MEDIUM:     return "MEDIUM";
			case CMD_EThreatLevel.HIGH:       return "HIGH";
			case CMD_EThreatLevel.CRITICAL:   return "CRITICAL";
		}
		return "UNKNOWN";
	}

	//! Skala tunggal dari abu ke merah. Warna di sini membawa arti tingkat bahaya,
	//! jadi sengaja tidak memakai palet faction -- dua hal berbeda tidak boleh dibaca
	//! dengan kunci warna yang sama.
	static int ThreatLevelColor(int level)
	{
		switch (level)
		{
			case CMD_EThreatLevel.NEGLIGIBLE: return 0xFF888888; // abu
			case CMD_EThreatLevel.LOW:        return 0xFF66CC66; // hijau
			case CMD_EThreatLevel.MEDIUM:     return 0xFFFFCC22; // kuning
			case CMD_EThreatLevel.HIGH:       return 0xFFFF8822; // oranye
			case CMD_EThreatLevel.CRITICAL:   return 0xFFFF3322; // merah
		}
		return COLOR_NEUTRAL;
	}

	static int RoleColor(int role)
	{
		switch (role)
		{
			case CMD_EGroupRole.ASSAULT:
			case CMD_EGroupRole.FLANK:
			case CMD_EGroupRole.ARMORED:
			case CMD_EGroupRole.SUPPRESS:
				return COLOR_ROLE_ASSAULT;

			case CMD_EGroupRole.DEFEND:
				return COLOR_ROLE_DEFEND;

			case CMD_EGroupRole.RECON:
				return COLOR_ROLE_RECON;
		}
		return COLOR_ROLE_IDLE;
	}
}