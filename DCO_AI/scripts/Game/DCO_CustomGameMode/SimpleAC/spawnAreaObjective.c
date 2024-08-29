[EntityEditorProps(category: "GameScripted/GameMode/CaptureAndHold", description: "This area trigger detects enemies not belonging to assigned faction, and raises callbacks which allow penalizing abusive behaviour.")]
class DCO_AreaObjectiveSpawnClass : SCR_SpawnAreaClass
{
}

//------------------------------------------------------------------------------------------------
/*!
	This area trigger detects enemies not belonging to assigned faction,
	and raises callbacks which allow penalizing abusive behaviour.
*/
class DCO_AreaObjectiveSpawn : SCR_SpawnArea
{
	//! Last entity stored as the "local offender" or null if none.
	protected IEntity m_pLastLocalEntity;

	//! Authority map for time spent in the trigger for individual entities (offending ones)
	protected ref map<IEntity, float> m_mTimeStamps;

	//! RplComponent attached to this are or null if none
	protected RplComponent m_pRplComponent;

	//------------------------------------------------------------------------------------------------
	/*!
		Initializes and registers this area into parent manager.
	*/
	protected override void OnInit(IEntity owner)
	{
		super.OnInit(owner);

		// Supress messages out of playmode, order of things is not quite guaranteed here
		if (!GetGame().InPlayMode())
			return;

		m_pRplComponent = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (!m_pRplComponent)
			Print("SCR_CaptureAndHoldSpawnArea cannot find RplComponent! Functionality will be limited!", LogLevel.WARNING);

		// Register self in manager
		DCO_War_GameMode parentManager = DCO_War_GameMode.GetAreaManager();
		if (!parentManager)
		{
			Print("DCO_AreaObjectiveSpawn cannot find DCO_War_GameMode! Functionality might be limited!", LogLevel.WARNING);
			return;
		}

		parentManager.RegisterSpawnArea(this);

		// Authority only
		if (!m_pRplComponent || !m_pRplComponent.IsMaster())
			return;

		m_mTimeStamps = new map<IEntity, float>();
		SetEventMask(EntityEvent.FRAME);
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Called when a character enters this area.
	*/
	protected override event void OnCharacterEnter(IEntity character, bool isFriendly)
	{
		super.OnCharacterEnter(character, isFriendly);

		// Raise callback for local player
		if (character == SCR_PlayerController.GetLocalControlledEntity())
			OnLocalPlayerEnter(character, isFriendly);

		// Authority only
		if (!m_pRplComponent || !m_pRplComponent.IsMaster())
			return;

		// Enemy occupants only
		if (isFriendly)
			return;

		if (!m_mTimeStamps.Contains(character))
			m_mTimeStamps.Insert(character, GetWorld().GetWorldTime());
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Called when the local character enters this area.
	*/
	protected event void OnLocalPlayerEnter(IEntity character, bool isFriendly)
	{
		// Store the local player entity,
		// death occuring and similar things come unexpectedly,
		// this way we can always make sure we clear the previous state
		m_pLastLocalEntity = character;

		if (isFriendly)
			return;
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Called when a character leaves this area.
	*/
	protected override event void OnCharacterExit(IEntity character, bool isFriendly)
	{
		super.OnCharacterExit(character, isFriendly);

		// Raise callback for local player
		if (character == m_pLastLocalEntity || character == SCR_PlayerController.GetLocalControlledEntity())
		{
			m_pLastLocalEntity = null;
			OnLocalPlayerExit(character, isFriendly);
		}

		// Authority only
		if (!m_pRplComponent || !m_pRplComponent.IsMaster())
			return;

		// Enemy occupants only
		if (isFriendly)
			return;

		if (m_mTimeStamps.Contains(character))
			m_mTimeStamps.Remove(character);
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Called when the local character leaves this area.
	*/
	protected event void OnLocalPlayerExit(IEntity character, bool isFriendly)
	{
		if (isFriendly)
			return;

		SCR_PopUpNotification popupNotifications = SCR_PopUpNotification.GetInstance();
		if (!popupNotifications)
			return;

		if (popupNotifications.GetCurrentMsg())
			popupNotifications.HideCurrentMsg();
	}

	//------------------------------------------------------------------------------------------------
	/*!
		Unregister self from parent manager.
	*/
	protected void ~DCO_AreaObjectiveSpawn()
	{
		// Far from ideal, OnDelete would be better

		// Register self in manager
		DCO_War_GameMode parentManager = DCO_War_GameMode.GetAreaManager();
		if (!parentManager)
			return;

		parentManager.UnregisterSpawnArea(this);
	}
}
