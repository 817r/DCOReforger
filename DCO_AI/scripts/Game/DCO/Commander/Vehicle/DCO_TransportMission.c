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
	[Attribute("0", UIWidgets.CheckBox, "Auto-assign vehicle ini ke commander waktu scenario mulai (tanpa GM). Dedicated Co kosong = commander random yang faction-nya sama.", category: "Commander")]
	protected bool m_bAutoAssign;

	[Attribute("", UIWidgets.Auto, "UID commander tujuan auto-assign. Kosong = random se-faction.", category: "Commander")]
	protected string m_sDedicatedCo;

	protected int m_iAutoAssignTries;
	protected const int AUTO_ASSIGN_MAX_TRIES = 12;
	protected const int AUTO_ASSIGN_RETRY_MS = 5000;

	protected DCO_ETransportState       m_eState            = DCO_ETransportState.IDLE;
	protected DCO_GroupUtilityComponent m_PassengerGroup;
	protected vector                    m_vDestination      = vector.Zero;
	protected float                     m_fStateStartTime   = 0.0;
	protected EVehicleType				m_eVehType;

	protected AICommander_BaseComponent m_Commander;

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

	static float BOARDING_TIMEOUT   = 40.0;
	static float MOVING_TIMEOUT     = 300.0;
	static float DISEMBARK_TIMEOUT  = 30.0;
	static float BOARDING_DIST      = 25.0;

	protected float m_fBoardingTimeout = 40.0;

	protected float m_fMovingTimeout = 300.0;
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
		m_fBoardingTimeout = BOARDING_TIMEOUT + vector.Distance(passengerGroup.GetOwner().GetOrigin(), GetOwner().GetOrigin()) / 1.2;

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
			&& m_eState != DCO_ETransportState.DONE
			&& m_eState != DCO_ETransportState.CLAIMED;
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
			m_fMovingTimeout = Math.Max(MOVING_TIMEOUT, 60.0 + vector.Distance(GetOwner().GetOrigin(), m_vDestination) / 2.5);
			return;
		}

		if ((worldTime - m_fStateStartTime) > m_fBoardingTimeout)
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
			Print("[DCO_Transport] Vehicle arrived");
			Arrive(worldTime);
			return;
		}

		if ((worldTime - m_fStateStartTime) > m_fMovingTimeout)
		{
			Print("[DCO_Transport] MOVING timeout — force arrive");
			Arrive(worldTime);
		}
	}

	protected void Arrive(float worldTime)
	{
		bool staysMounted = m_eVehType == EVehicleType.APC || (m_PassengerGroup && m_PassengerGroup.IsArmor());
		if (!staysMounted)
		{
			SetState(DCO_ETransportState.DISEMBARKING, worldTime);
			DisembarkGroup();
			return;
		}

		if (m_PassengerGroup)
		{
			m_PassengerGroup.EndTransport();
		}
		SetState(DCO_ETransportState.CLAIMED, worldTime);
	}

	protected void TickDisembarking(float worldTime)
	{
		if ((worldTime - m_fStateStartTime) > DISEMBARK_TIMEOUT)
		{
			if (m_PassengerGroup)
			{
				m_PassengerGroup.EndTransport();
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
		board.SetCompletionType(EAIWaypointCompletionType.All);

		grp.AddWaypoint(wpGetIn);
		Print(string.Format("[DCO_Transport] GetIn %1 -> %2 | wp sekarang %3", grp.GetName(), GetOwner().GetName(), grp.GetCurrentWaypoint()));

		float surfY  = GetGame().GetWorld().GetSurfaceY(m_vDestination[0], m_vDestination[2]);
		vector dest  = Vector(m_vDestination[0], surfY, m_vDestination[2]);

		SCR_AIWaypoint wpMove = m_Commander.SpawnMoveWP(dest);
		if (wpMove)
			grp.AddWaypoint(wpMove);

		m_PassengerGroup.SetPhase(DCO_ETaskPhase.MOVING);

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
			m_PassengerGroup.EndTransport();
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
			return;

		if (!Replication.IsServer())
			return;

		if (!IsActiveVehicle())
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

		if (m_bAutoAssign && Replication.IsServer())
			GetGame().GetCallqueue().CallLater(TryAutoAssign, AUTO_ASSIGN_RETRY_MS, false);

		if (!AICommander_ManagerComponent.GetInstance())
			return;

		Vehicle veh = Vehicle.Cast(owner);
		m_eVehType = veh.m_eVehicleType;
	}

	protected void TryAutoAssign()
	{
		if (m_Commander)
			return;

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		SCR_VehicleFactionAffiliationComponent fac = SCR_VehicleFactionAffiliationComponent.Cast(GetOwner().FindComponent(SCR_VehicleFactionAffiliationComponent));
		if (mgr && fac)
		{
			FactionKey fk;
			if (fac.GetAffiliatedFaction())
				fk = fac.GetAffiliatedFactionKey();
			else
				fk = fac.GetDefaultFactionKey();

			AICommander_BaseComponent cmd = mgr.PickCommanderForFaction(fk, m_sDedicatedCo);
			if (cmd && cmd.AssignVehicle(GetOwner()))
				return;
		}

		m_iAutoAssignTries++;
		if (m_iAutoAssignTries < AUTO_ASSIGN_MAX_TRIES)
			GetGame().GetCallqueue().CallLater(TryAutoAssign, AUTO_ASSIGN_RETRY_MS, false);
		else
			Print(string.Format("[DCO_Transport] Auto-assign %1 gagal: gak ada commander '%2'", GetOwner().GetName(), m_sDedicatedCo), LogLevel.WARNING);
	}

	DCO_GroupUtilityComponent GetPassengerGroup()
	{
		return m_PassengerGroup;
	}

	AICommander_BaseComponent GetCommanderOwner()
	{
		return m_Commander;
	}

	void ActivateForCommander(AICommander_BaseComponent cmd)
	{
		if (!cmd)
			return;

		m_Commander = cmd;

		Vehicle veh = Vehicle.Cast(GetOwner());
		if (veh)
			m_eVehType = veh.m_eVehicleType;

		SetEventMask(GetOwner(), EntityEvent.FRAME);
	}

	void ReleaseFromCommander(float worldTime)
	{
		if (IsActiveVehicle())
			AbortMission(worldTime);

		if (m_OwnerGroup)
		{
			if (m_OwnerGroup.GetOwnedVehicle() == GetOwner())
				m_OwnerGroup.SetOwnedVehicle(null);
			m_OwnerGroup = null;
		}

		m_PassengerGroup = null;
		m_Commander = null;
		ClearEventMask(GetOwner(), EntityEvent.FRAME);
	}
}