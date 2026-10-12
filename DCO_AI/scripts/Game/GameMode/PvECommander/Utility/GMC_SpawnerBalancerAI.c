[ComponentEditorProps(category: "GMC_MyMod/Spawner", description: "Adjusts max AI spawn count based on connected player count")]
class GMC_SpawnBalancerComponentClass : ScriptComponentClass {}

class GMC_SpawnBalancerComponent : ScriptComponent
{
	[Attribute("4", UIWidgets.EditBox, "Base AI count (with 0 players online).")]
	protected int m_iBaseUnitCount;

	[Attribute("1.0", UIWidgets.EditBox,
		"AI multiplier per player.\n+ = add AI for each player who joins\n- = remove AI for each player who joins\nExample: +2.0 -> each player adds 2 AI; -1.0 -> each player removes 1 AI")]
	protected float m_fMultiplierPerPlayer;

	[Attribute("1", UIWidgets.EditBox, "Minimum AI count (lower limit of the result).")]
	protected int m_iMinUnits;

	[Attribute("32", UIWidgets.EditBox, "Maximum AI count (upper limit of the result).")]
	protected int m_iMaxUnits;

	[Attribute("0", UIWidgets.CheckBox, "Enable debug print?")]
	protected bool m_bDebugLog;

	protected int m_iCurrentPlayerCount = 0;
	protected int m_iLastBalancedCount   = 0;

	protected ref ScriptInvoker m_OnBalanceChanged;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!Replication.IsServer()) return;

		m_iLastBalancedCount = ComputeCount(1);

		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gm)
		{
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", "SCR_BaseGameMode not found, balancer is off.");
			return;
		}

		gm.GetOnPlayerRegistered().Insert(OnPlayerRegistered);
		gm.GetOnPlayerDisconnected().Insert(OnPlayerDisconnected);

		RefreshPlayerCount();
	}

	override void OnDelete(IEntity owner)
	{
		super.OnDelete(owner);

		SCR_BaseGameMode gm = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (gm)
		{
			gm.GetOnPlayerRegistered().Remove(OnPlayerRegistered);
			gm.GetOnPlayerDisconnected().Remove(OnPlayerDisconnected);
		}
	}

	int GetBalancedCount()
	{
		return m_iLastBalancedCount;
	}

	int GetPlayerCount()
	{
		return m_iCurrentPlayerCount;
	}

	int PreviewCountForPlayers(int playerCount)
	{
		return ComputeCount(playerCount);
	}

	void SetMultiplier(float multiplier)
	{
		if (!Replication.IsServer()) return;
		m_fMultiplierPerPlayer = multiplier;
		Recalculate();
		DebugLog(string.Format("Multiplier changed to %.2f -> balanced count = %1", multiplier, m_iLastBalancedCount));
	}

	void SetBaseUnitCount(int baseCount)
	{
		if (!Replication.IsServer()) return;
		m_iBaseUnitCount = baseCount;
		Recalculate();
	}

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

	ScriptInvoker GetOnBalanceChanged()
	{
		if (!m_OnBalanceChanged) m_OnBalanceChanged = new ScriptInvoker();
		return m_OnBalanceChanged;
	}

	static GMC_SpawnBalancerComponent GetFrom(IEntity entity)
	{
		if (!entity) return null;
		return GMC_SpawnBalancerComponent.Cast(entity.FindComponent(GMC_SpawnBalancerComponent));
	}

	protected void RefreshPlayerCount()
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		if (!pm) return;

		array<int> playerIDs = new array<int>();
		pm.GetAllPlayers(playerIDs);
		m_iCurrentPlayerCount = playerIDs.Count();

		Recalculate();
	}

	protected void Recalculate()
	{
		int newCount = ComputeCount(m_iCurrentPlayerCount);

		if (newCount == m_iLastBalancedCount) return;

		m_iLastBalancedCount = newCount;

		DebugLog(string.Format("Recalculate → %1 (players=%2, base=%3, multi=%.2f)",
			newCount, m_iCurrentPlayerCount, m_iBaseUnitCount, m_fMultiplierPerPlayer));

		if (m_OnBalanceChanged)
			m_OnBalanceChanged.Invoke(m_iLastBalancedCount, m_iCurrentPlayerCount);
	}

	protected int ComputeCount(int playerCount)
	{
		float raw   = m_iBaseUnitCount + (playerCount * m_fMultiplierPerPlayer);
		int   floor = Math.Floor(raw);
		return Math.Clamp(floor, m_iMinUnits, m_iMaxUnits);
	}

	protected void OnPlayerRegistered(int playerID)
	{
		m_iCurrentPlayerCount++;
		DebugLog(string.Format("Player join (id=%1) → total=%2", playerID, m_iCurrentPlayerCount));
		Recalculate();
	}

	protected void OnPlayerDisconnected(int playerID, KickCauseCode cause, int timeout)
	{
		m_iCurrentPlayerCount = Math.Max(0, m_iCurrentPlayerCount - 1);
		DebugLog(string.Format("Player leave (id=%1) → total=%2", playerID, m_iCurrentPlayerCount));
		Recalculate();
	}

	protected void DebugLog(string msg)
	{
		if (m_bDebugLog) DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", msg);
	}
}