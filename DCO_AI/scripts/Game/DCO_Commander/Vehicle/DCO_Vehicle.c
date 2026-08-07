enum DCO_ETransportState
{
	IDLE         = 0,
	BOARDING     = 1,
	MOVING       = 2,
	DISEMBARKING = 3,
	DONE         = 4,
	CLAIMED		 = 5
}

[ComponentEditorProps(category: "GameScripted/Transport")]
class DCO_TransportMissionComponentClass : ScriptComponentClass {}

class DCO_TransportMissionComponent : ScriptComponent
{
	protected DCO_ETransportState       m_eState            = DCO_ETransportState.IDLE;
	protected DCO_GroupUtilityComponent m_PassengerGroup;
	protected vector                    m_vDestination      = vector.Zero;
	protected float                     m_fStateStartTime   = 0.0;
	protected EVehicleType				m_eVehType;

	protected AICommander_BaseComponent m_Commander;

	// === ADDED: Vehicle Ownership ===
	// Grup yang "memiliki" vehicle ini secara permanen. Beda sama m_PassengerGroup yang
	// cuma valid selama 1 mission aktif -- ownership ini nempel terus sampai grup ganti
	// vehicle atau vehicle ini hancur.
	protected DCO_GroupUtilityComponent m_OwnerGroup;

	bool HasOwner()
	{
		return m_OwnerGroup != null;
	}

	bool IsOwnedBy(DCO_GroupUtilityComponent grp)
	{
		return m_OwnerGroup == grp;
	}

	void ClaimOwnership(DCO_GroupUtilityComponent grp)
	{
		m_OwnerGroup = grp;
	}

	void ReleaseOwnership()
	{
		m_OwnerGroup = null;
	}

	DCO_GroupUtilityComponent GetOwnerGroup()
	{
		return m_OwnerGroup;
	}
	// === END ADDED ===

	static float BOARDING_TIMEOUT   = 40.0;
	static float MOVING_TIMEOUT     = 300.0;
	static float DISEMBARK_TIMEOUT  = 30.0;
	static float BOARDING_DIST      = 10.0;
	static float ARRIVAL_DIST       = 25.0;

	void StartMission(
		DCO_GroupUtilityComponent   passengerGroup,
		vector                      destination,
		FactionKey                  factionKey,
		float                       worldTime,
		AICommander_BaseComponent   commander)
	{
		m_PassengerGroup = passengerGroup;
		m_vDestination   = destination;
		m_Commander      = commander;

		SetState(DCO_ETransportState.BOARDING, worldTime);

		Print(string.Format("[DCO_Transport] Mission started | group: %1 | dest: %2",
			passengerGroup.GetOwner().GetName(), destination.ToString()));
	}
	
	bool AssignCommanderOwner(AICommander_BaseComponent cmd)
	{
		m_Commander = cmd;
		return true;
	}

	bool IsActiveVehicle()
	{
		return m_eState != DCO_ETransportState.IDLE
			&& m_eState != DCO_ETransportState.DONE;
	}

	void Tick(float worldTime)
	{
		switch (m_eState)
		{
			case DCO_ETransportState.BOARDING:
				TickBoarding(worldTime);
				break;
			case DCO_ETransportState.MOVING:
				TickMoving(worldTime);
				break;
			case DCO_ETransportState.DISEMBARKING:
				TickDisembarking(worldTime);
				break;
			default:
				break;
		}
	}

	protected void TickBoarding(float worldTime)
	{
		if (!m_PassengerGroup)
		{
			SetState(DCO_ETransportState.DONE, worldTime);
			return;
		}

		float dist = vector.Distance(
			m_PassengerGroup.GetOwner().GetOrigin(),
			GetOwner().GetOrigin());

		if (dist <= BOARDING_DIST)
		{
			BoardGroup();
			SetState(DCO_ETransportState.MOVING, worldTime);
			return;
		}

		if ((worldTime - m_fStateStartTime) > BOARDING_TIMEOUT)
		{
			Print("[DCO_Transport] BOARDING timeout — aborting");
			AbortMission(worldTime);
		}
	}

	protected void TickMoving(float worldTime)
	{
		float dist = vector.Distance(GetOwner().GetOrigin(), m_vDestination);

		if (dist <= ARRIVAL_DIST)
		{
			if (m_eVehType == EVehicleType.APC || (m_OwnerGroup && m_OwnerGroup == m_PassengerGroup))
				SetState(DCO_ETransportState.CLAIMED, worldTime);
			else
			{
				SetState(DCO_ETransportState.DISEMBARKING, worldTime);
				DisembarkGroup();			
			}
			Print("[DCO_Transport] Vehicle arrived — disembarking");
			return;
		}

		if ((worldTime - m_fStateStartTime) > MOVING_TIMEOUT)
		{
			Print("[DCO_Transport] MOVING timeout — force disembark");
			// === MODIFIED: sama kayak di atas -- owner tetep di dalem walau timeout ===
			if (m_eVehType == EVehicleType.APC || (m_OwnerGroup && m_OwnerGroup == m_PassengerGroup))
				SetState(DCO_ETransportState.CLAIMED, worldTime);
			else
			{
				SetState(DCO_ETransportState.DISEMBARKING, worldTime);
				DisembarkGroup();			
			}
			// === END MODIFIED ===
		}
	}

	protected void TickDisembarking(float worldTime)
	{
		if ((worldTime - m_fStateStartTime) > DISEMBARK_TIMEOUT)
		{
			if (m_PassengerGroup)
			{
				m_PassengerGroup.SetGroupRole(CMD_EGroupRole.NONE);
				m_PassengerGroup.SetGroupStatus(DCOG_EGroupStatus.IDLE);
			}

			SetState(DCO_ETransportState.DONE, worldTime);
			Print("[DCO_Transport] Mission complete");
		}
	}

	protected void BoardGroup()
	{
		if (!m_PassengerGroup || !m_Commander)
			return;

		SCR_AIGroup grp = SCR_AIGroup.Cast(m_PassengerGroup.GetOwner());
		if (!grp)
			return;
		
		grp.CompleteAllWaypoints();		

		SCR_AIWaypoint wpGetIn = m_Commander.SpawnGetInWP(GetOwner().GetOrigin());
		if (!wpGetIn)
			return;
		
		SCR_BoardingEntityWaypoint board = SCR_BoardingEntityWaypoint.Cast(wpGetIn);
		board.SetEntity(GetOwner());
		board.SetAllowance(true, true, true);
		board.SetPriorityLevel(SCR_AIBehaviorBase.PRIORITY_LEVEL_PLAYER);
		board.SetCompletionRadius(15);
		board.SetCompletionType(EAIWaypointCompletionType.Leader);
		
		grp.AddWaypoint(wpGetIn);

		float surfY  = GetGame().GetWorld().GetSurfaceY(m_vDestination[0], m_vDestination[2]);
		vector dest  = Vector(m_vDestination[0], surfY, m_vDestination[2]);

		SCR_AIWaypoint wpMove = m_Commander.SpawnMoveWP(dest);
		if (wpMove)
			grp.AddWaypoint(wpMove);

		m_PassengerGroup.SetGroupStatus(DCOG_EGroupStatus.TRANSITING);

		Print(string.Format("[DCO_Transport] Group boarding vehicle → moving to %1", dest.ToString()));
	}

	protected void DisembarkGroup()
	{
		if (!m_PassengerGroup || !m_Commander)
			return;

		SCR_AIGroup grp = SCR_AIGroup.Cast(m_PassengerGroup.GetOwner());
		if (!grp)
			return;
		
		grp.CompleteAllWaypoints();
		
		float surfY  = GetGame().GetWorld().GetSurfaceY(m_vDestination[0], m_vDestination[2]);
		vector dest  = Vector(m_vDestination[0], surfY, m_vDestination[2]);

		SCR_AIWaypoint wpGetOut = m_Commander.SpawnGetOutWP(dest);
		if (wpGetOut)
			grp.AddWaypoint(wpGetOut);
	}
	
	void AbortMission(float worldTime)
	{
		if (m_PassengerGroup)
		{
			m_PassengerGroup.SetGroupRole(CMD_EGroupRole.NONE);
			m_PassengerGroup.SetGroupStatus(DCOG_EGroupStatus.IDLE);
		}

		SetState(DCO_ETransportState.DONE, worldTime);
		Print("[DCO_Transport] Mission aborted");
	}

	protected void SetState(DCO_ETransportState newState, float worldTime)
	{
		m_eState          = newState;
		m_fStateStartTime = worldTime;
	}

	DCO_ETransportState GetTransportState() { return m_eState; }

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!m_Commander)
		{
			AICommander_ManagerComponent.GetInstance().RegisterVehicle(owner);
		}
			
		
		if (!Replication.IsServer())
			return;

		if (m_eState == DCO_ETransportState.IDLE || m_eState == DCO_ETransportState.DONE)
			return;

		float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
		Tick(worldTime);
	}

	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (!AICommander_ManagerComponent.GetInstance())
			return;
		
		Vehicle veh = Vehicle.Cast(owner);
		m_eVehType = veh.m_eVehicleType;
		SetEventMask(owner, EntityEvent.FRAME);
		
	}
}