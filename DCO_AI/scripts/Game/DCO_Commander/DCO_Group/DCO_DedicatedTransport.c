enum DCO_ETransportTeamState
{
	AVAILABLE             = 0,
	MOVING_TO_PASSENGER   = 1,
	WAITING_FOR_BOARD     = 2,
	MOVING_TO_DESTINATION = 3,
	WAITING_FOR_DISEMBARK = 4,
	RETURNING             = 5
}

[ComponentEditorProps(category: "GameScripted/Transport", description: "Marks this group as a dedicated transport team")]
class DCO_TransportTeamComponentClass : ScriptComponentClass {}

class DCO_TransportTeamComponent : ScriptComponent
{
	[Attribute("8.0", UIWidgets.EditBox, "Jarak boarding dianggap cukup dekat (meter)", category: "Transport Team")]
	protected float m_fBoardingDist;

	[Attribute("25.0", UIWidgets.EditBox, "Jarak arrival ke destination (meter)", category: "Transport Team")]
	protected float m_fArrivalDist;

	[Attribute("60.0", UIWidgets.EditBox, "Boarding timeout (detik)", category: "Transport Team")]
	protected float m_fBoardingTimeout;

	[Attribute("300.0", UIWidgets.EditBox, "Driving timeout (detik)", category: "Transport Team")]
	protected float m_fDriveTimeout;

	[Attribute("30.0", UIWidgets.EditBox, "Disembark timeout (detik)", category: "Transport Team")]
	protected float m_fDisembarkTimeout;
	
	// === ADDED: RTB vs Stay at LZ ===
	[Attribute("1", UIWidgets.CheckBox, "Setelah drop off passenger, RTB balik ke rally point? Kalau false, standby/parkir di LZ terakhir dan siap terima job baru dari situ.", category: "Transport Team")]
	protected bool m_bReturnToRallyAfterDrop;
	// === END ADDED ===

	protected DCO_ETransportTeamState   m_eTeamState       = DCO_ETransportTeamState.AVAILABLE;
	protected DCO_GroupUtilityComponent m_PassengerGroup;
	protected vector                    m_vDestination     = vector.Zero;
	protected vector                    m_vRallyPoint      = vector.Zero;
	protected float                     m_fStateStartTime  = 0.0;
	protected DCO_GroupUtilityComponent m_SelfGroupUtil;
	protected AICommander_BaseComponent m_Commander;
	protected int                       m_iJobsCompleted   = 0;
	protected IEntity					m_Vehicle;
	bool						m_bRegistered      = false;

	void AssignJob(
		DCO_GroupUtilityComponent   passengerGroup,
		vector                      destination,
		AICommander_BaseComponent   commander,
		float                       worldTime)
	{
		m_PassengerGroup = passengerGroup;
		m_vDestination   = destination;
		m_Commander      = commander;

		passengerGroup.SetGroupRole(CMD_EGroupRole.TRANSPORT);

		SetTeamState(DCO_ETransportTeamState.MOVING_TO_PASSENGER, worldTime);

		if (m_Commander && m_SelfGroupUtil)
		{
			SCR_AIWaypoint wp = m_Commander.SpawnMoveWP(passengerGroup.GetOwner().GetOrigin());
			if (wp)
				m_SelfGroupUtil.MoveTo(wp, worldTime);
		}

		Print(string.Format("[DCO_TransportTeam] %1 assigned | passenger: %2 | dest: %3",
			GetOwner().GetName(),
			passengerGroup.GetOwner().GetName(),
			destination.ToString()));
	}
	
	DCO_GroupUtilityComponent GetDCOGroupUtility()
	{
		return DCO_GroupUtilityComponent.Cast(GetOwner().FindComponent(DCO_GroupUtilityComponent));
	}
	
	void SetVehicle(IEntity veh)
	{
		m_Vehicle = veh;
	}
	
	IEntity GetVehicle()
	{
		return m_Vehicle;
	}

	void SetRallyPoint(vector rally)  { m_vRallyPoint = rally; }
	bool IsAvailable()                { return m_eTeamState == DCO_ETransportTeamState.AVAILABLE; }
	DCO_ETransportTeamState GetTeamState() { return m_eTeamState; }
	int GetJobsCompleted()            { return m_iJobsCompleted; }

	protected void Tick(float worldTime)
	{
		switch (m_eTeamState)
		{
			case DCO_ETransportTeamState.MOVING_TO_PASSENGER:
				TickMovingToPassenger(worldTime);
				break;

			case DCO_ETransportTeamState.WAITING_FOR_BOARD:
				TickWaitingForBoard(worldTime);
				break;

			case DCO_ETransportTeamState.MOVING_TO_DESTINATION:
				TickMovingToDestination(worldTime);
				break;

			case DCO_ETransportTeamState.WAITING_FOR_DISEMBARK:
				TickWaitingForDisembark(worldTime);
				break;

			case DCO_ETransportTeamState.RETURNING:
				TickReturning(worldTime);
				break;

			default:
				break;
		}
	}

	protected void TickMovingToPassenger(float worldTime)
	{
		if (!m_PassengerGroup)
		{
			ReturnToRally(worldTime);
			return;
		}

		float dist = vector.Distance(
			GetOwner().GetOrigin(),
			m_PassengerGroup.GetOwner().GetOrigin());

		if (dist <= m_fBoardingDist)
		{
			BoardPassenger(worldTime);
			SetTeamState(DCO_ETransportTeamState.WAITING_FOR_BOARD, worldTime);
			return;
		}

		if ((worldTime - m_fStateStartTime) > m_fBoardingTimeout)
		{
			Print(string.Format("[DCO_TransportTeam] %1 timeout moving to passenger — aborting",
				GetOwner().GetName()));
			AbortJob(worldTime);
		}
	}

	protected void TickWaitingForBoard(float worldTime)
	{
		if (!m_PassengerGroup)
		{
			ReturnToRally(worldTime);
			return;
		}

		if (m_PassengerGroup.GetGroupStatus() == DCOG_EGroupStatus.TRANSITING)
		{
			SetTeamState(DCO_ETransportTeamState.MOVING_TO_DESTINATION, worldTime);

			if (m_Commander && m_SelfGroupUtil)
			{
				float surfY = GetGame().GetWorld().GetSurfaceY(m_vDestination[0], m_vDestination[2]);
				vector dest = Vector(m_vDestination[0], surfY, m_vDestination[2]);

				SCR_AIWaypoint wp = m_Commander.SpawnMoveWP(dest);
				if (wp)
					m_SelfGroupUtil.MoveTo(wp, worldTime);
			}

			Print(string.Format("[DCO_TransportTeam] %1 passenger boarded — heading to destination",
				GetOwner().GetName()));
			return;
		}

		if ((worldTime - m_fStateStartTime) > m_fBoardingTimeout)
		{
			Print(string.Format("[DCO_TransportTeam] %1 board timeout — aborting",
				GetOwner().GetName()));
			AbortJob(worldTime);
		}
	}

	protected void TickMovingToDestination(float worldTime)
	{
		float dist = vector.Distance(GetOwner().GetOrigin(), m_vDestination);

		if (dist <= m_fArrivalDist)
		{
			DisembarkPassenger(worldTime);
			SetTeamState(DCO_ETransportTeamState.WAITING_FOR_DISEMBARK, worldTime);

			Print(string.Format("[DCO_TransportTeam] %1 arrived at destination — disembarking",
				GetOwner().GetName()));
			return;
		}

		if ((worldTime - m_fStateStartTime) > m_fDriveTimeout)
		{
			Print(string.Format("[DCO_TransportTeam] %1 drive timeout — force disembark",
				GetOwner().GetName()));
			DisembarkPassenger(worldTime);
			SetTeamState(DCO_ETransportTeamState.WAITING_FOR_DISEMBARK, worldTime);
		}
	}

	protected void TickWaitingForDisembark(float worldTime)
	{
		if ((worldTime - m_fStateStartTime) < m_fDisembarkTimeout)
			return;

		if (m_PassengerGroup)
		{
			m_PassengerGroup.SetGroupRole(CMD_EGroupRole.NONE);
			m_PassengerGroup.SetGroupStatus(DCOG_EGroupStatus.IDLE);
			m_PassengerGroup = null;
		}

		m_iJobsCompleted = m_iJobsCompleted + 1;

		// === ADDED: RTB vs Stay at LZ ===
		if (m_bReturnToRallyAfterDrop)
		{
			Print(string.Format("[DCO_TransportTeam] %1 job #%2 complete — returning to rally",
				GetOwner().GetName(), m_iJobsCompleted));

			ReturnToRally(worldTime);
		}
		else
		{
			Print(string.Format("[DCO_TransportTeam] %1 job #%2 complete — standby di LZ",
				GetOwner().GetName(), m_iJobsCompleted));

			SetTeamState(DCO_ETransportTeamState.AVAILABLE, worldTime);
		}
		// === END ADDED ===
	}

	protected void TickReturning(float worldTime)
	{
		if (m_vRallyPoint == vector.Zero)
		{
			SetTeamState(DCO_ETransportTeamState.AVAILABLE, worldTime);
			return;
		}

		float dist = vector.Distance(GetOwner().GetOrigin(), m_vRallyPoint);
		if (dist <= 20.0)
		{
			SetTeamState(DCO_ETransportTeamState.AVAILABLE, worldTime);
			if (m_SelfGroupUtil)
				m_SelfGroupUtil.SetGroupStatus(DCOG_EGroupStatus.IDLE);

			Print(string.Format("[DCO_TransportTeam] %1 back at rally — available",
				GetOwner().GetName()));
		}
	}

	protected void BoardPassenger(float worldTime)
	{
		if (!m_PassengerGroup || !m_Commander)
			return;

		SCR_AIGroup passengerGrp = SCR_AIGroup.Cast(m_PassengerGroup.GetOwner());
		if (!passengerGrp)
			return;

		passengerGrp.CompleteAllWaypoints();

		SCR_AIWaypoint wpGetIn = m_Commander.SpawnGetInWP(GetOwner().GetOrigin());
		if (!wpGetIn)
			return;

		SCR_BoardingEntityWaypoint board = SCR_BoardingEntityWaypoint.Cast(wpGetIn);
		board.SetEntity(GetOwner());
		board.SetAllowance(false, false, true);
		board.SetPriorityLevel(SCR_AIBehaviorBase.PRIORITY_LEVEL_PLAYER);
		board.SetCompletionRadius(15);
		board.SetCompletionType(EAIWaypointCompletionType.Leader);

		passengerGrp.AddWaypoint(wpGetIn);
		m_PassengerGroup.SetGroupStatus(DCOG_EGroupStatus.TRANSITING);
	}

	protected void DisembarkPassenger(float worldTime)
	{
		if (!m_PassengerGroup || !m_Commander)
			return;

		SCR_AIGroup passengerGrp = SCR_AIGroup.Cast(m_PassengerGroup.GetOwner());
		if (!passengerGrp)
			return;

		passengerGrp.CompleteAllWaypoints();

		float surfY  = GetGame().GetWorld().GetSurfaceY(m_vDestination[0], m_vDestination[2]);
		vector dest  = Vector(m_vDestination[0], surfY, m_vDestination[2]);

		SCR_AIWaypoint wpGetOut = m_Commander.SpawnGetOutWP(dest);
		if (wpGetOut)
			passengerGrp.AddWaypoint(wpGetOut);
	}

	protected void AbortJob(float worldTime)
	{
		if (m_PassengerGroup)
		{
			m_PassengerGroup.SetGroupRole(CMD_EGroupRole.NONE);
			m_PassengerGroup.SetGroupStatus(DCOG_EGroupStatus.IDLE);
			m_PassengerGroup = null;
		}

		ReturnToRally(worldTime);
	}
	
	protected void ReturnToRally(float worldTime)
	{
		SetTeamState(DCO_ETransportTeamState.RETURNING, worldTime);

		if (m_vRallyPoint != vector.Zero && m_Commander && m_SelfGroupUtil)
		{
			SCR_AIWaypoint wp = m_Commander.SpawnMoveWP(m_vRallyPoint);
			if (wp)
				m_SelfGroupUtil.MoveTo(wp, worldTime);
		}
	}

	protected void SetTeamState(DCO_ETransportTeamState newState, float worldTime)
	{
		m_eTeamState      = newState;
		m_fStateStartTime = worldTime;
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer())
			return;

		if (m_eTeamState == DCO_ETransportTeamState.AVAILABLE)
			return;

		float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
		Tick(worldTime);
	}

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
		SetEventMask(owner, EntityEvent.FRAME);
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
 
		m_SelfGroupUtil = DCO_GroupUtilityComponent.Cast(owner.FindComponent(DCO_GroupUtilityComponent));
 
		if (!m_SelfGroupUtil)
			return;
		
		Registering();
	}
	
	void Registering()
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;
 
		FactionKey fk = m_SelfGroupUtil.GetFactionKey();
		if (fk == string.Empty)
			return;
 
		foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
		{
			if (!cmd)
				continue;
 
			if (cmd.GetCommanderFactionKey() != fk)
				continue;
 
			cmd.RegisterTransportTeam(this);
			m_bRegistered = true;
			break;
		}
	}
}