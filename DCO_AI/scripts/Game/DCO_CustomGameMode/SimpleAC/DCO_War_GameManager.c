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
	
	//! If enabled custom weather Id will be used on session start. Authority only.
	[Attribute(defvalue: "0", desc: "If enabled, custom weather Id will be used. Authority only.", category: "Wargame: Environment")]
	protected bool m_bUseCustomWeather;

	//! Weather IDs are the same as used in the TimeAndWeatherManager. Weather set on game start. Authority only.
	[Attribute(defvalue: "", desc: "Weather IDs are the same as used in the TimeAndWeatherManager. Weather set on game start. Authority only.", category: "Wargame: Environment")]
	protected string m_sCustomWeatherId;

	//! If enabled custom time of the day will be used on session start. Authority only.
	[Attribute(defvalue: "0", desc: "If enabled, custom time of the day will be used. Authority only.", category: "Wargame: Environment")]
	protected bool m_bUseCustomTime;

	//! Time of the day set on game start. Authority only.
	[Attribute(defvalue: "7", desc: "Time of the day set on game start. Authority only.", category: "Wargame: Environment", params: "0 24 0.01")]
	protected float m_fCustomTimeOfTheDay;
	
	//! If enabled then capture status is persistent
	[Attribute(defvalue: "0", desc: "Should actions be persistent (sticky) in the sense that they are not cleared when players leave the area?")]
	protected bool m_bPersistentAreaFactions;
	
	protected ref array<DCO_BaseAICommander> m_lcommander = {};

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
		Register a capture area to this manager. Area must be unique.
	*/
	void RegisterCommander(DCO_BaseAICommander commander)
	{
		m_lcommander.Insert(commander);
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Unregisters a capture area from this manager.
	*/
	void unregisterCommander(DCO_BaseAICommander commander)
	{
		int indexOf = m_lcommander.Find(commander);
		if (indexOf != -1)
			m_lcommander.Remove(indexOf);
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
		Returns an area at given index.
	*/
	DCO_ObjectiveArea GetArea(int index)
	{
		return m_aAreas[index];
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
		Returns true when factions should be persistent, ie. left unchanged when last player(s) leave
		the capture area.
	*/
	bool GetIsAreaFactionPersistent()
	{
		return m_bPersistentAreaFactions;
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
	
	void getAllObjectiveArea(out array<DCO_ObjectiveArea> area)
	{
		area = m_aAreas;
	}
	
	protected override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		if (m_bUseCustomTime)
			SetTimeOfTheDay(m_fCustomTimeOfTheDay);

		if (m_bUseCustomWeather)
			SetWeather(m_sCustomWeatherId);
	}
}
