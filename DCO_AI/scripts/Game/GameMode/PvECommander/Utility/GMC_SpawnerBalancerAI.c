// FILE: addons/GMC_MyMod/Scripts/Game/Respawner/GMC_SpawnBalancerComponent.c
//
// Cara pakai:
//  1. Attach komponen ini ke entity yang SAMA dengan GMC_RespawnerComponent
//  2. Set m_iBaseUnitCount  → jumlah AI saat 0 player
//  3. Set m_fMultiplierPerPlayer → float positif = naik per player, negatif = turun
//  4. Set m_iMinUnits / m_iMaxUnits → batas bawah & atas hasil kalkulasi
//
// Formula:
//   balancedCount = clamp(base + floor(playerCount * multiplier), min, max)
//
// Contoh multiplier +2.0, base 4, 3 player → 4 + floor(3*2.0) = 10 AI
// Contoh multiplier -1.0, base 6, 3 player → 6 + floor(3*-1.0) = 3 AI

[ComponentEditorProps(category: "GMC_MyMod/Spawner", description: "Adjusts max AI spawn count based on connected player count")]
class GMC_SpawnBalancerComponentClass : ScriptComponentClass {}

class GMC_SpawnBalancerComponent : ScriptComponent
{
	//--------------------------------------------------------------------
	// EDITOR ATTRIBUTES

	[Attribute("4", UIWidgets.EditBox, "Jumlah AI dasar (saat 0 player online)")]
	protected int m_iBaseUnitCount;

	[Attribute("1.0", UIWidgets.EditBox,
		"Multiplier AI per player.\n+ = tambah AI tiap player join\n- = kurang AI tiap player join\nContoh: +2.0 → tiap player nambah 2 AI; -1.0 → tiap player kurang 1 AI")]
	protected float m_fMultiplierPerPlayer;

	[Attribute("1", UIWidgets.EditBox, "Jumlah AI minimum (batas bawah hasil kalkulasi)")]
	protected int m_iMinUnits;

	[Attribute("32", UIWidgets.EditBox, "Jumlah AI maksimum (batas atas hasil kalkulasi)")]
	protected int m_iMaxUnits;

	[Attribute("0", UIWidgets.CheckBox, "Aktifkan debug print?")]
	protected bool m_bDebugLog;

	//--------------------------------------------------------------------
	// RUNTIME STATE

	protected int m_iCurrentPlayerCount = 0;
	protected int m_iLastBalancedCount   = 0;

	// Event: dipanggil saat balanced count berubah → Invoke(int newCount, int playerCount)
	protected ref ScriptInvoker m_OnBalanceChanged;

	//--------------------------------------------------------------------
	// LIFECYCLE

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!Replication.IsServer()) return;

		// Hitung awal (0 player)
		m_iLastBalancedCount = ComputeCount(1);

		// Subscribe ke game mode player events
		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gm)
		{
			Print("[GMC_Balancer] WARNING: SCR_BaseGameMode tidak ditemukan, balancer tidak aktif.");
			return;
		}

		gm.GetOnPlayerRegistered().Insert(OnPlayerRegistered);
		gm.GetOnPlayerDisconnected().Insert(OnPlayerDisconnected);

		// Hitung ulang dengan jumlah player yang sudah ada saat init
		RefreshPlayerCount();
	}

	override void OnDelete(IEntity owner)
	{
		super.OnDelete(owner);

		// Unsubscribe agar tidak dangling pointer
		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gm)
		{
			gm.GetOnPlayerRegistered().Remove(OnPlayerRegistered);
			gm.GetOnPlayerDisconnected().Remove(OnPlayerDisconnected);
		}
	}

	//--------------------------------------------------------------------
	// PUBLIC API

	//! Kembalikan jumlah AI yang seharusnya di-spawn sekarang
	int GetBalancedCount()
	{
		return m_iLastBalancedCount;
	}

	//! Kembalikan jumlah player saat ini yang terlacak
	int GetPlayerCount()
	{
		return m_iCurrentPlayerCount;
	}

	//! Preview hasil kalkulasi untuk jumlah player tertentu (tanpa mengubah state)
	int PreviewCountForPlayers(int playerCount)
	{
		return ComputeCount(playerCount);
	}

	//! Override multiplier dari kode lain saat runtime
	void SetMultiplier(float multiplier)
	{
		if (!Replication.IsServer()) return;
		m_fMultiplierPerPlayer = multiplier;
		Recalculate();
		DebugLog(string.Format("Multiplier diubah ke %.2f → balanced count = %1", multiplier, m_iLastBalancedCount));
	}

	//! Override base unit count dari kode lain saat runtime
	void SetBaseUnitCount(int baseCount)
	{
		if (!Replication.IsServer()) return;
		m_iBaseUnitCount = baseCount;
		Recalculate();
	}

	//! Kembalikan string ringkasan status (untuk debug)
	string GetStatusString()
	{
		return string.Format(
			"players=%1 | base=%2 | multi=%.2f | result=%3 (min=%4, max=%5)",
			m_iCurrentPlayerCount,
			m_iBaseUnitCount,
			m_fMultiplierPerPlayer,
			m_iLastBalancedCount,
			m_iMinUnits,
			m_iMaxUnits
		);
	}

	//--------------------------------------------------------------------
	// SCRIPTINVOKER GETTER

	//! Subscribe untuk dengar perubahan balanced count
	//! Callback signature: void OnBalanceChanged(int newCount, int playerCount)
	ScriptInvoker GetOnBalanceChanged()
	{
		if (!m_OnBalanceChanged) m_OnBalanceChanged = new ScriptInvoker();
		return m_OnBalanceChanged;
	}

	//--------------------------------------------------------------------
	// STATIC HELPER

	static GMC_SpawnBalancerComponent GetFrom(IEntity entity)
	{
		if (!entity) return null;
		return GMC_SpawnBalancerComponent.Cast(entity.FindComponent(GMC_SpawnBalancerComponent));
	}

	//--------------------------------------------------------------------
	// INTERNAL

	//! Hitung ulang dari data player manager langsung
	protected void RefreshPlayerCount()
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm) return;

		// Hitung player yang benar-benar connected
		array<int> playerIDs = new array<int>();
		pm.GetAllPlayers(playerIDs);
		m_iCurrentPlayerCount = playerIDs.Count();

		Recalculate();
	}

	//! Hitung ulang balanced count dan fire event kalau berubah
	protected void Recalculate()
	{
		int newCount = ComputeCount(m_iCurrentPlayerCount);

		if (newCount == m_iLastBalancedCount) return; // tidak ada perubahan

		m_iLastBalancedCount = newCount;

		DebugLog(string.Format("Recalculate → %1 (players=%2, base=%3, multi=%.2f)",
			newCount, m_iCurrentPlayerCount, m_iBaseUnitCount, m_fMultiplierPerPlayer));

		if (m_OnBalanceChanged)
			m_OnBalanceChanged.Invoke(m_iLastBalancedCount, m_iCurrentPlayerCount);
	}

	//! Formula inti: base + floor(playerCount × multiplier), di-clamp ke [min, max]
	protected int ComputeCount(int playerCount)
	{
		float raw   = m_iBaseUnitCount + (playerCount * m_fMultiplierPerPlayer);
		int   floor = Math.Floor(raw);
		return Math.Clamp(floor, m_iMinUnits, m_iMaxUnits);
	}

	//! Dipanggil engine saat player baru terdaftar
	protected void OnPlayerRegistered(int playerID)
	{
		m_iCurrentPlayerCount++;
		DebugLog(string.Format("Player join (id=%1) → total=%2", playerID, m_iCurrentPlayerCount));
		Recalculate();
	}

	//! Dipanggil engine saat player disconnect
	protected void OnPlayerDisconnected(int playerID, KickCauseCode cause, int timeout)
	{
		m_iCurrentPlayerCount = Math.Max(0, m_iCurrentPlayerCount - 1);
		DebugLog(string.Format("Player leave (id=%1) → total=%2", playerID, m_iCurrentPlayerCount));
		Recalculate();
	}

	protected void DebugLog(string msg)
	{
		if (m_bDebugLog) PrintFormat("[GMC_Balancer] %1", msg);
	}
}