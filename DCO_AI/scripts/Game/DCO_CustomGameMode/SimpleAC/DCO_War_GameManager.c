[ComponentEditorProps(category: "GameScripted/GameMode/CaptureAndHold", description: "Manager component allowing access and API over CaptureAndHold areas.")]
class DCO_War_GameModeClass : SCR_BaseGameModeComponentClass
{
}

//------------------------------------------------------------------------------------------------
/*!
	Capture & Hold manager that allows registration and management of areas.
	This component must be attached to a SCR_BaseGameMode entity!
	There should only be a single manager at any given time.
*/
class DCO_War_GameMode : SCR_BaseGameModeComponent
{
	//! Manager singleton instance, assigned on first get call
	private static DCO_War_GameMode s_pInstance;

	//! Array of all areas registered within this manager.
	protected ref array<DCO_ObjectiveArea> m_aAreas = {};

	//! Array of all spawn areas registered within this manager.
	protected ref array<DCO_AreaObjectiveSpawn> m_aSpawnAreas = {};

	//! If enabled custom weather Id will be used on session start. Authority only.
	[Attribute(defvalue: "0", desc: "If enabled, custom weather Id will be used. Authority only.", category: "CaptureAndHold: Environment")]
	protected bool m_bUseCustomWeather;

	//! Weather IDs are the same as used in the TimeAndWeatherManager. Weather set on game start. Authority only.
	[Attribute(defvalue: "", desc: "Weather IDs are the same as used in the TimeAndWeatherManager. Weather set on game start. Authority only.", category: "CaptureAndHold: Environment")]
	protected string m_sCustomWeatherId;

	//! If enabled custom time of the day will be used on session start. Authority only.
	[Attribute(defvalue: "0", desc: "If enabled, custom time of the day will be used. Authority only.", category: "CaptureAndHold: Environment")]
	protected bool m_bUseCustomTime;

	//! Time of the day set on game start. Authority only.
	[Attribute(defvalue: "7", desc: "Time of the day set on game start. Authority only.", category: "CaptureAndHold: Environment", params: "0 24 0.01")]
	protected float m_fCustomTimeOfTheDay;
	
	//! If enabled then capture status is persistent
	[Attribute(defvalue: "0", desc: "Should actions be persistent (sticky) in the sense that they are not cleared when players leave the area?")]
	protected bool m_bPersistentAreaFactions;

	//------------------------------------------------------------------------------------------------
	/*!
		Register a capture area to this manager. Area must be unique.
	*/
	void RegisterArea(DCO_ObjectiveArea area)
	{
		m_aAreas.Insert(area);
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Unregisters a capture area from this manager.
	*/
	void UnregisterArea(DCO_ObjectiveArea area)
	{
		int indexOf = m_aAreas.Find(area);
		if (indexOf != -1)
			m_aAreas.Remove(indexOf);
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Register a spawn area to this manager. Area must be unique.
	*/
	void RegisterSpawnArea(DCO_AreaObjectiveSpawn spawnArea)
	{
		m_aSpawnAreas.Insert(spawnArea);
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Unregisters a capture area from this manager.
	*/
	void UnregisterSpawnArea(DCO_AreaObjectiveSpawn spawnArea)
	{
		int indexOf = m_aSpawnAreas.Find(spawnArea);
		if (indexOf != -1)
			m_aSpawnAreas.Remove(indexOf);
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Returns the number of registered capture areas.
	*/
	int GetAreaCount()
	{
		return m_aAreas.Count();
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Returns the number of registered spawn areas.
	*/
	int GetSpawnAreaCount()
	{
		return m_aSpawnAreas.Count();
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Returns an area at given index.
	*/
	DCO_ObjectiveArea GetArea(int index)
	{
		return m_aAreas[index];
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Returns a spawn area at given index.
	*/
	DCO_AreaObjectiveSpawn GetSpawnArea(int index)
	{
		return m_aSpawnAreas[index];
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Fills the provided array with all registered zones and returns the count.
	*/
	int GetAreas(notnull array<DCO_ObjectiveArea> outAreas)
	{
		int count = 0;
		foreach (DCO_ObjectiveArea area : m_aAreas)
		{
			outAreas.Insert(area);
			++count;
		}
		return count;
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Fills the provided array with all registered spawn areas and returns the count.
	*/
	int GetAreas(notnull array<DCO_AreaObjectiveSpawn> outAreas)
	{
		int count = 0;
		foreach (DCO_AreaObjectiveSpawn area : m_aSpawnAreas)
		{
			outAreas.Insert(area);
			++count;
		}
		return count;
	}
	
	//------------------------------------------------------------------------------------------------
	/*!
		Returns true when factions should be persistent, ie. left unchanged when last player(s) leave
		the capture area.
	*/
	bool GetIsAreaFactionPersistent()
	{
		return m_bPersistentAreaFactions;
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Initializes this manager component and hooks up events.
	*/
	protected override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);

		// This is not the best way of solving this problem,
		// but for a small game mode like this it's completely fine.
		SCR_MapEntity.GetOnMapOpen().Insert(OnMapOpen);
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Unhooks events.
	*/
	protected override void OnDelete(IEntity owner)
	{
		super.OnDelete(owner);

		SCR_MapEntity.GetOnMapOpen().Remove(OnMapOpen);
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Pans the map to provided world coordinates.
	*/
	protected void DoPanZoomMap(float x, float z, float zoom)
	{
		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity)
			return;

		// Zoom and pan to objectives almost immediately
		mapEntity.ZoomPanSmooth(zoom, x, z, 0.001);
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Finds the average center of all objectives and pans and zooms the map to it.
	*/
	protected void OnMapOpen(MapConfiguration config)
	{
		// Get average of all positions
		float x;
		float z;
		int count;
		foreach (DCO_ObjectiveArea area : m_aAreas)
		{
			vector worldPos = area.GetWorldObjectiveCenter();
			x += worldPos[0];
			z += worldPos[2];
			++count;
		}

		// No can do!
		if (count == 0)
			return;

		x /= (float)count;
		z /= (float)count;

		SCR_MapEntity mapEntity = SCR_MapEntity.GetMapInstance();
		if (!mapEntity)
			return;

		CanvasWidget mapWidget = mapEntity.GetMapWidget();
		vector usize = mapWidget.GetSizeInUnits();
		float zoomVal = usize[0] / (usize[0] * mapWidget.PixelPerUnit());

		// Unfortunately we need to "override" the default respawn menu focus,
		// currently not aware of a nicer way - perhaps it will begone in some future update :)
		GetGame().GetCallqueue().CallLater(DoPanZoomMap, 100, false, x, z, zoomVal);
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Called when provided player dies.
	*/
	protected override void OnPlayerKilled(int playerId, IEntity playerEntity, IEntity killerEntity, notnull Instigator killer)
	{
		super.OnPlayerKilled(playerId, playerEntity, killer);

		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(playerEntity);
		if (!character)
			return;

		Faction faction = character.GetFaction();
		if (!faction)
			return;

		foreach (SCR_SpawnArea area : m_aSpawnAreas)
		{
			Faction areaFaction = area.GetAffiliatedFaction();
			if (!areaFaction)
				continue;

			if (!areaFaction.IsFactionFriendly(faction))
				continue;

			// We have to query ourselves, the character is dead and filtered out
			if (!area.QueryEntityInside(character))
				continue;

			// Character died in a friendly spawn zone, therefore
			// let's clear their respawn timer to reduce frustration
			// of spawn / team kills
			SCR_BaseGameMode gameMode = GetGameMode();
			if (!gameMode)
				return;

			// Unless SCR_RespawnTimerComponent handles individual times on top of faction times,
			// this will have no impact unfortunately
			SCR_RespawnTimerComponent respawnTimer = SCR_RespawnTimerComponent.Cast(gameMode.FindComponent(SCR_RespawnTimerComponent));
			if (!respawnTimer)
				return;

			// Reset to 0
			respawnTimer.SetRespawnTime(playerId, 0.0);
		}
	}

	//------------------------------------------------------------------------------------------------
	/*
		Finds area manager on current game mode and returns it or null if none.
	*/
	static DCO_War_GameMode GetAreaManager()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return null;

		if (!s_pInstance)
			s_pInstance = DCO_War_GameMode.Cast(gameMode.FindComponent(DCO_War_GameMode));

		return s_pInstance;
	}


	//------------------------------------------------------------------------------------------------
	/*!
		Forcefully sets weather to provided weatherId. Authority only.
	*/
	protected void SetWeather(string weatherId)
	{
		if (!m_pGameMode.IsMaster())
			return;

		if (weatherId.IsEmpty())
			return;

		ChimeraWorld world = GetOwner().GetWorld();
		if (!world)
			return;
		
		TimeAndWeatherManagerEntity weatherManager = world.GetTimeAndWeatherManager();
		if (!weatherManager)
		{
			Print("Cannot initialize weather: TimeAndWeatherManagerEntity not found!", LogLevel.WARNING);
			return;
		}

		weatherManager.ForceWeatherTo(true, weatherId, 0.0);
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Forcefully sets time of the day to provided value. Authority only.
	*/
	protected void SetTimeOfTheDay(float timeOfTheDay)
	{
		if (!m_pGameMode.IsMaster())
			return;

		ChimeraWorld world = GetOwner().GetWorld();
		if (!world)
			return;
		
		TimeAndWeatherManagerEntity weatherManager = world.GetTimeAndWeatherManager();
		if (!weatherManager)
		{
			Print("Cannot initialize TimeOfTheDay: TimeAndWeatherManagerEntity not found!", LogLevel.WARNING);
			return;
		}

		weatherManager.SetTimeOfTheDay(timeOfTheDay, true);
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Initialize the manager.
	*/
	protected override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		if (m_bUseCustomTime)
			SetTimeOfTheDay(m_fCustomTimeOfTheDay);

		if (m_bUseCustomWeather)
			SetWeather(m_sCustomWeatherId);
	}
}
