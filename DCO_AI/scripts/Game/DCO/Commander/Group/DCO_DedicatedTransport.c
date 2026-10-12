enum DCO_ETransportTeamState
{
	AVAILABLE             = 0,
	MOVING_TO_PASSENGER   = 1,
	WAITING_FOR_BOARD     = 2,
	MOVING_TO_DESTINATION = 3,
	WAITING_FOR_DISEMBARK = 4,
	RETURNING             = 5
}

enum DCO_ELogiJobFlag
{
	TRANSPORT	= 1,
	EXTRACT		= 2,
	MEDEVAC		= 4,
	RESUPPLY	= 8
}

[ComponentEditorProps(category: "GameScripted/Transport", description: "Marks this group as a dedicated transport team")]
class DCO_TransportTeamComponentClass : ScriptComponentClass {}

class DCO_TransportTeamComponent : ScriptComponent
{
	[Attribute("15", UIWidgets.Flags, "Logistics jobs this team may take. Transport = deploy, reinforce, redeploy. Resupply also needs a resupply station on the vehicle.", enums: ParamEnumArray.FromEnum(DCO_ELogiJobFlag), category: "Transport Team")]
	protected int m_iAllowedJobs;

	int GetAllowedJobs()				{ return m_iAllowedJobs; }
	void SetAllowedJobs(int mask)		{ m_iAllowedJobs = mask & 15; }

	bool AllowsJob(DCO_ELogiJob kind)
	{
		int flag = DCO_ELogiJobFlag.TRANSPORT;
		if (kind == DCO_ELogiJob.EXTRACT)
			flag = DCO_ELogiJobFlag.EXTRACT;
		else if (kind == DCO_ELogiJob.MEDEVAC)
			flag = DCO_ELogiJobFlag.MEDEVAC;
		else if (kind == DCO_ELogiJob.RESUPPLY)
			flag = DCO_ELogiJobFlag.RESUPPLY;
		return (m_iAllowedJobs & flag) != 0;
	}

	[Attribute("25.0", UIWidgets.EditBox, "Distance (m) between the team vehicle and passengers that counts as close enough to start boarding.", category: "Transport Team")]
	protected float m_fBoardingDist;

	[Attribute("0.7", UIWidgets.Slider, "Only used on boarding timeout: share of active members (leader required) that must be inside for the group to come along. The rest are split into a reserve group at the pickup point. Normally waits for EVERYONE to board.", params: "0 1 0.05", category: "Transport Team")]
	protected float m_fBoardedRatio;

	[Attribute("25.0", UIWidgets.EditBox, "Arrival distance to the destination (m).", category: "Transport Team")]
	protected float m_fArrivalDist;

	[Attribute("60.0", UIWidgets.EditBox, "Boarding timeout (s).", category: "Transport Team")]
	protected float m_fBoardingTimeout;

	[Attribute("300.0", UIWidgets.EditBox, "Driving timeout (s).", category: "Transport Team")]
	protected float m_fDriveTimeout;

	[Attribute("30.0", UIWidgets.EditBox, "Disembark timeout (s).", category: "Transport Team")]
	protected float m_fDisembarkTimeout;

	[Attribute("1", UIWidgets.CheckBox, "After dropping off passengers, return to the rally point (hub)? If false, stand by at the last LZ and take new jobs from there.", category: "Transport Team")]
	protected bool m_bReturnToRallyAfterDrop;

	[Attribute("10", UIWidgets.EditBox, "MEDEVAC: loading seconds per casualty.", category: "Transport Team")]
	protected float m_fLoadTime;

	[Attribute("20", UIWidgets.EditBox, "RESUPPLY: seconds to hand over supplies to a group.", category: "Transport Team")]
	protected float m_fResupplyTime;

	[Attribute("4", UIWidgets.EditBox, "RESUPPLY: maximum magazines per soldier.", category: "Transport Team")]
	protected int m_iResupplyMags;

	protected DCO_ETransportTeamState   m_eTeamState       = DCO_ETransportTeamState.AVAILABLE;
	protected DCO_LogiJob               m_Job;
	protected vector                    m_vRallyPoint      = vector.Zero;
	protected float                     m_fStateStartTime  = 0.0;
	protected DCO_GroupUtilityComponent m_SelfGroupUtil;
	protected AICommander_BaseComponent m_Commander;
	protected int                       m_iJobsCompleted   = 0;
	protected IEntity					m_Vehicle;
	protected float                     m_fStateTimeout;
	protected float                     m_fNextAction;
	protected float                     m_fLastRetarget;
	protected float                     m_fVehicleCheck;
	protected bool                      m_bHadVehicle;
	protected bool                      m_bCrewInfantry;
	protected vector                    m_vProgressPos;
	protected float                     m_fProgressTime;
	protected bool                      m_bRepathed;
	protected IEntity                   m_BadVehicle;
	bool						m_bRegistered      = false;


	void SetCommander(AICommander_BaseComponent cmd)
	{
		m_Commander = cmd;
		if (cmd)
			SetEventMask(GetOwner(), EntityEvent.FRAME);
		else
			ClearEventMask(GetOwner(), EntityEvent.FRAME);
	}

	void SetRallyPoint(vector rally)					{ m_vRallyPoint = rally; }
	bool IsAvailable()
	{
		if (m_bCrewInfantry)
			return false;
		return m_eTeamState == DCO_ETransportTeamState.AVAILABLE || (m_eTeamState == DCO_ETransportTeamState.RETURNING && !m_Job);
	}
	bool IsPickingUp()									{ return m_eTeamState == DCO_ETransportTeamState.MOVING_TO_PASSENGER || m_eTeamState == DCO_ETransportTeamState.WAITING_FOR_BOARD; }
	DCO_ETransportTeamState GetTeamState()				{ return m_eTeamState; }
	int GetJobsCompleted()								{ return m_iJobsCompleted; }
	DCO_LogiJob GetJob()								{ return m_Job; }
	bool HasPassenger(DCO_GroupUtilityComponent grp)	{ return m_Job && m_Job.m_aGroups.Contains(grp); }

	DCO_GroupUtilityComponent GetDCOGroupUtility()
	{
		return DCO_GroupUtilityComponent.Cast(GetOwner().FindComponent(DCO_GroupUtilityComponent));
	}

	void SetVehicle(IEntity veh)	{ m_Vehicle = veh; }
	IEntity GetVehicle()			{ return m_Vehicle; }

	IEntity ResolveVehicle()
	{
		if (m_Vehicle && m_Vehicle == m_BadVehicle)
			m_Vehicle = null;
		if (m_Vehicle)
		{
			DamageManagerComponent dmg = DamageManagerComponent.Cast(m_Vehicle.FindComponent(DamageManagerComponent));
			if (dmg && dmg.GetState() == EDamageState.DESTROYED)
				m_Vehicle = null;
			else
				return m_Vehicle;
		}

		SCR_AIGroup grp = SCR_AIGroup.Cast(GetOwner());
		if (!grp)
			return null;

		IEntity veh = DCO_VehicleCombat.GetVehicle(grp.GetLeaderEntity());
		if (!veh)
		{
			array<AIAgent> agents = {};
			grp.GetAgents(agents);
			foreach (AIAgent a : agents)
			{
				veh = DCO_VehicleCombat.GetVehicle(a.GetControlledEntity());
				if (veh && veh != m_BadVehicle)
					break;
			}
		}

		if (veh == m_BadVehicle)
			veh = null;
		m_Vehicle = veh;
		return veh;
	}

	vector GetTeamPos()
	{
		IEntity veh = ResolveVehicle();
		if (veh)
			return veh.GetOrigin();
		return GetOwner().GetOrigin();
	}

	static int CountFreeCargo(IEntity veh)
	{
		if (!veh)
			return 0;

		SCR_BaseCompartmentManagerComponent mgr = SCR_BaseCompartmentManagerComponent.Cast(veh.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!mgr)
			return 0;

		array<BaseCompartmentSlot> slots = {};
		mgr.GetCompartments(slots);
		int free = 0;
		foreach (BaseCompartmentSlot slot : slots)
		{
			if (CargoCompartmentSlot.Cast(slot) && !slot.GetOccupant())
				free++;
		}
		return free;
	}

	int GetFreeCargoSeats() { return CountFreeCargo(ResolveVehicle()); }

	protected static void GetPlayerEntities(SCR_AIGroup pg, notnull array<IEntity> outList)
	{
		outList.Clear();
		PlayerManager pm = GetGame().GetPlayerManager();
		array<int> pids = pg.GetPlayerIDs();
		foreach (int pid : pids)
		{
			IEntity ent = pm.GetPlayerControlledEntity(pid);
			if (ent)
				outList.Insert(ent);
		}
	}

	protected static bool CountBoarded(SCR_AIGroup grp, IEntity veh, out int outInside, out int outActive)
	{
		outInside = 0;
		outActive = 0;
		if (!grp || !veh)
			return false;

		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			IEntity ent = a.GetControlledEntity();
			if (!DCO_Util.IsAlive(ent))
				continue;

			outActive++;
			if (DCO_VehicleCombat.GetVehicle(ent) == veh)
				outInside++;
		}
		return DCO_VehicleCombat.GetVehicle(grp.GetLeaderEntity()) == veh;
	}

	protected static void CountPlayersBoarded(SCR_AIGroup pg, IEntity veh, out int outInside, out int outActive)
	{
		outInside = 0;
		outActive = 0;
		array<IEntity> ents = {};
		GetPlayerEntities(pg, ents);
		foreach (IEntity ent : ents)
		{
			if (!DCO_Util.IsAlive(ent))
				continue;
			outActive++;
			if (DCO_VehicleCombat.GetVehicle(ent) == veh)
				outInside++;
		}
	}

	void AssignLogiJob(DCO_LogiJob job, AICommander_BaseComponent commander, float worldTime)
	{
		m_Job = job;
		m_Commander = commander;
		job.m_Team = this;

		if (job.IsTransportKind())
		{
			foreach (DCO_GroupUtilityComponent g : job.m_aGroups)
			{
				if (g && !g.IsInTransport())
					g.BeginTransport(DCO_EGroupTask.NONE);
			}
		}

		GoPickup(worldTime);

		int seats = GetFreeCargoSeats();
		int need = job.SeatsNeeded();
		DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), string.Format("%1 job #%2 %3 | seats need %4 have %5 | pickup %6 | LZ %7 (alt %8)",
			GetOwner().GetName(), job.m_iId, typename.EnumToString(DCO_ELogiJob, job.m_eKind), need, seats,
			job.m_vPickup, job.m_vLZ, job.m_vAltLZ));
		if (job.m_eKind != DCO_ELogiJob.RESUPPLY && seats < need)
			DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.WARN, DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), string.Format("%1 short of %2 seats -- the rest split off / left behind", GetOwner().GetName(), need - seats));
	}

	void AddPassenger(DCO_GroupUtilityComponent grp, float worldTime)
	{
		if (!grp.IsInTransport())
			grp.BeginTransport(DCO_EGroupTask.NONE);
		if (m_eTeamState == DCO_ETransportTeamState.WAITING_FOR_BOARD)
			BoardGroup(grp);
	}

	protected void GoPickup(float worldTime)
	{
		UpdatePickup();
		SetTeamState(DCO_ETransportTeamState.MOVING_TO_PASSENGER, worldTime);
		m_fStateTimeout = m_fBoardingTimeout + vector.Distance(GetTeamPos(), m_Job.m_vPickup) / 3.0;
		m_fLastRetarget = worldTime;
		TeamMoveTo(m_Job.m_vPickup, worldTime);

		foreach (DCO_GroupUtilityComponent g : m_Job.m_aGroups)
		{
			if (g && g.HasState(DCO_EGroupState.IN_CONTACT) && m_Commander && m_Commander.GetLogistics())
			{
				m_Commander.GetLogistics().RequestEscort(m_Commander, m_Job, m_Job.m_vPickup, worldTime);
				break;
			}
		}
	}

	protected bool m_bDCOHoldContact;

	protected bool PassengersInContact()
	{
		if (!m_Job || !m_Job.IsTransportKind())
			return false;
		foreach (DCO_GroupUtilityComponent g : m_Job.m_aGroups)
		{
			if (g && (g.IsInContact() || g.IsBreakingContact()))
				return true;
		}
		return false;
	}

	protected void UpdatePickup()
	{
		vector teamPos = GetTeamPos();
		if (m_Job.m_eKind == DCO_ELogiJob.MEDEVAC)
		{
			float best = float.MAX;
			foreach (IEntity c : m_Job.m_aCasualties)
			{
				if (!c)
					continue;
				float d = vector.DistanceSq(teamPos, c.GetOrigin());
				if (d < best)
				{
					best = d;
					m_Job.m_vPickup = c.GetOrigin();
				}
			}
			return;
		}

		foreach (DCO_GroupUtilityComponent g : m_Job.m_aGroups)
		{
			if (!g)
				continue;
			SCR_AIGroup grp = SCR_AIGroup.Cast(g.GetOwner());
			if (grp && grp.GetLeaderEntity())
				m_Job.m_vPickup = grp.GetLeaderEntity().GetOrigin();
			else
				m_Job.m_vPickup = g.GetOwner().GetOrigin();
			return;
		}

		foreach (SCR_AIGroup pg : m_Job.m_aPlayerGroups)
		{
			array<IEntity> ents = {};
			GetPlayerEntities(pg, ents);
			if (!ents.IsEmpty())
			{
				m_Job.m_vPickup = ents[0].GetOrigin();
				return;
			}
		}
	}

	protected void FollowPlayerLeader(float worldTime)
	{
		if (vector.DistanceXZ(GetTeamPos(), m_Job.m_vPickup) <= 150.0)
			return;

		IEntity leader;
		foreach (SCR_AIGroup pg : m_Job.m_aPlayerGroups)
		{
			if (!pg)
				continue;
			leader = GetGame().GetPlayerManager().GetPlayerControlledEntity(pg.GetLeaderID());
			if (!DCO_Util.IsAlive(leader))
			{
				leader = null;
				array<IEntity> ents = {};
				GetPlayerEntities(pg, ents);
				foreach (IEntity e : ents)
				{
					if (DCO_Util.IsAlive(e))
					{
						leader = e;
						break;
					}
				}
			}
			if (leader)
				break;
		}
		if (!leader)
			return;

		vector lp = leader.GetOrigin();
		if (vector.DistanceXZ(lp, m_Job.m_vPickup) <= 50.0)
			return;

		vector np = DCO_Logistics.SnapPickup(lp);
		m_Job.m_fPickupShift += vector.DistanceXZ(np, m_Job.m_vPickup);
		DCO_Logistics.Log(m_Job, "pickup_follow", string.Format("from=%1 to=%2 shift=%3", m_Job.m_vPickup, np, Math.Round(m_Job.m_fPickupShift)));
		m_Job.m_vPickup = np;
		TeamMoveTo(np, worldTime);
		if (m_Commander && m_Commander.GetLogistics())
			m_Commander.GetLogistics().OnPickupMoved(m_Job, GetTeamPos());
	}

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
		}
	}

	protected void TickMovingToPassenger(float worldTime)
	{
		if (!m_Job || m_Job.IsEmpty())
		{
			Finish(false, "no_passenger", worldTime);
			return;
		}

		if (PassengersInContact())
		{
			m_fStateStartTime = worldTime;
			if (!m_bDCOHoldContact)
			{
				m_bDCOHoldContact = true;
				float dist = vector.DistanceXZ(GetTeamPos(), m_Job.m_vPickup);
				DCO_Logistics.Log(m_Job, "hold_contact", string.Format("dist=%1", Math.Round(dist)));
				if (dist < 300.0)
				{
					vector away = GetTeamPos() - m_Job.m_vPickup;
					away[1] = 0;
					if (away.LengthSq() < 1.0)
						away = Vector(1, 0, 0);
					away.Normalize();
					TeamMoveTo(DCO_Logistics.SnapPickup(m_Job.m_vPickup + away * 300.0), worldTime);
				}
				else
					TeamMoveTo(GetTeamPos(), worldTime);
			}
			return;
		}
		if (m_bDCOHoldContact)
		{
			m_bDCOHoldContact = false;
			UpdatePickup();
			DCO_Logistics.Log(m_Job, "hold_resume", string.Format("pickup=%1", m_Job.m_vPickup));
			m_fStateTimeout = m_fBoardingTimeout + vector.Distance(GetTeamPos(), m_Job.m_vPickup) / 3.0;
			TeamMoveTo(m_Job.m_vPickup, worldTime);
			m_fLastRetarget = worldTime;
		}

		if (worldTime - m_fLastRetarget > 10.0)
		{
			m_fLastRetarget = worldTime;
			if (m_Job.m_aGroups.IsEmpty() && !m_Job.m_aPlayerGroups.IsEmpty() && m_Job.m_eKind != DCO_ELogiJob.MEDEVAC)
			{
				FollowPlayerLeader(worldTime);
			}
			else
			{
				vector old = m_Job.m_vPickup;
				UpdatePickup();
				if (vector.DistanceXZ(old, m_Job.m_vPickup) > 40.0)
					TeamMoveTo(m_Job.m_vPickup, worldTime);
			}
		}

		if (vector.DistanceXZ(GetTeamPos(), m_Job.m_vPickup) <= m_fBoardingDist)
		{
			OnAtPickup(worldTime);
			return;
		}

		if (CheckStuck(worldTime, m_Job.m_vPickup))
			return;

		if ((worldTime - m_fStateStartTime) > m_fStateTimeout)
		{
			DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), string.Format("%1 timeout moving to pickup — aborting", GetOwner().GetName()));
			Finish(false, "pickup_timeout", worldTime);
		}
	}

	protected void OnAtPickup(float worldTime)
	{
		if (!ResolveVehicle())
		{
			DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), string.Format("%1 has no vehicle - aborting", GetOwner().GetName()));
			Finish(false, "no_vehicle", worldTime);
			return;
		}

		SetTeamState(DCO_ETransportTeamState.WAITING_FOR_BOARD, worldTime);
		m_fStateTimeout = m_fBoardingTimeout;

		switch (m_Job.m_eKind)
		{
			case DCO_ELogiJob.MEDEVAC:
				m_fNextAction = worldTime + m_fLoadTime;
				return;

			case DCO_ELogiJob.RESUPPLY:
				m_fNextAction = worldTime + m_fResupplyTime;
				return;
		}

		foreach (DCO_GroupUtilityComponent g : m_Job.m_aGroups)
		{
			if (g)
				BoardGroup(g);
		}
		foreach (SCR_AIGroup pg : m_Job.m_aPlayerGroups)
		{
			if (pg)
				DCO_Radio.Group(pg.GetGroupID(), "LOGISTICS", "logi_mount_up");
		}
	}

	protected void TickWaitingForBoard(float worldTime)
	{
		if (!m_Job)
		{
			Finish(false, "job_lost", worldTime);
			return;
		}

		IEntity veh = ResolveVehicle();
		if (!veh)
		{
			Finish(false, "no_vehicle", worldTime);
			return;
		}

		if (m_Job.m_eKind == DCO_ELogiJob.MEDEVAC)
		{
			TickLoading(worldTime, veh);
			return;
		}

		if (m_Job.m_eKind == DCO_ELogiJob.RESUPPLY)
		{
			if (worldTime >= m_fNextAction)
			{
				int n = Deliver(veh);
				m_Job.m_iBoarded = n;
				Finish(n > 0, "delivered", worldTime);
			}
			return;
		}

		if (m_Job.IsEmpty())
		{
			Finish(false, "no_passenger", worldTime);
			return;
		}

		if (m_Job.m_iBoarded == 0 && PassengersInContact())
		{
			foreach (DCO_GroupUtilityComponent hg : m_Job.m_aGroups)
			{
				if (!hg || hg.IsMountedIn(veh))
					continue;
				SCR_AIGroup hgrp = SCR_AIGroup.Cast(hg.GetOwner());
				if (hgrp)
					hgrp.CompleteAllWaypoints();
			}
			SetTeamState(DCO_ETransportTeamState.MOVING_TO_PASSENGER, worldTime);
			return;
		}

		bool allIn = true;
		bool anyIn = false;
		foreach (DCO_GroupUtilityComponent g : m_Job.m_aGroups)
		{
			int inside, active;
			bool leaderIn = CountBoarded(SCR_AIGroup.Cast(g.GetOwner()), veh, inside, active);
			if (!leaderIn || inside < active)
				allIn = false;
			if (leaderIn)
				anyIn = true;
		}
		foreach (SCR_AIGroup pg : m_Job.m_aPlayerGroups)
		{
			int pin, pact;
			CountPlayersBoarded(pg, veh, pin, pact);
			if (pin < pact || pact == 0)
				allIn = false;
			if (pin > 0)
				anyIn = true;
		}

		if (allIn || (anyIn && CountFreeCargo(veh) == 0))
		{
			Depart(worldTime, veh);
			return;
		}

		if ((worldTime - m_fStateStartTime) <= m_fBoardingTimeout)
			return;

		for (int i = m_Job.m_aGroups.Count() - 1; i >= 0; i--)
		{
			DCO_GroupUtilityComponent g = m_Job.m_aGroups[i];
			int inside, active;
			bool leaderIn = CountBoarded(SCR_AIGroup.Cast(g.GetOwner()), veh, inside, active);
			if (leaderIn && inside >= Math.Ceil(active * m_fBoardedRatio))
				continue;

			DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), string.Format("%1 board timeout %2 (boarded %3/%4, leader %5) - left behind",
				GetOwner().GetName(), g.GetOwner().GetName(), inside, active, leaderIn));
			g.EndTransport();
			m_Job.m_aGroups.Remove(i);
			m_Job.m_iLeft += active;
		}
		for (int i = m_Job.m_aPlayerGroups.Count() - 1; i >= 0; i--)
		{
			int pin, pact;
			CountPlayersBoarded(m_Job.m_aPlayerGroups[i], veh, pin, pact);
			if (pin == 0)
				m_Job.m_aPlayerGroups.Remove(i);
		}

		if (m_Job.IsEmpty())
		{
			Finish(false, "board_timeout", worldTime);
			return;
		}
		Depart(worldTime, veh);
	}

	protected void TickLoading(float worldTime, IEntity veh)
	{
		if (worldTime < m_fNextAction)
			return;

		for (int i = m_Job.m_aCasualties.Count() - 1; i >= 0; i--)
		{
			IEntity c = m_Job.m_aCasualties[i];
			if (!DCO_Logistics.IsDown(c))
			{
				m_Job.m_aCasualties.Remove(i);
				continue;
			}

			if (vector.DistanceXZ(c.GetOrigin(), veh.GetOrigin()) > 30.0)
				continue;

			if (!LoadCasualty(c, veh))
			{
				DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), string.Format("%1 no seats left for casualties", GetOwner().GetName()));
				m_Job.m_aCasualties.Clear();
				break;
			}

			m_Job.m_aCasualties.Remove(i);
			m_Job.m_aLoaded.Insert(c);
			m_fNextAction = worldTime + m_fLoadTime;
			DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), string.Format("%1 casualty loaded (%2 in vehicle, %3 left)", GetOwner().GetName(), m_Job.m_aLoaded.Count(), m_Job.m_aCasualties.Count()));
			return;
		}

		if (!m_Job.m_aCasualties.IsEmpty() && CountFreeCargo(veh) > 0)
		{
			GoPickup(worldTime);
			return;
		}

		if (m_Job.m_aLoaded.IsEmpty())
		{
			Finish(false, "no_casualty_loaded", worldTime);
			return;
		}
		Depart(worldTime, veh);
	}

	protected static bool LoadCasualty(IEntity casualty, IEntity veh)
	{
		SCR_BaseCompartmentManagerComponent mgr = SCR_BaseCompartmentManagerComponent.Cast(veh.FindComponent(SCR_BaseCompartmentManagerComponent));
		ChimeraCharacter c = ChimeraCharacter.Cast(casualty);
		if (!mgr || !c)
			return false;

		BaseCompartmentSlot slot = mgr.GetFirstFreeCompartmentOfType(SCR_PatientCompartmentSlot);
		if (!slot)
			slot = mgr.GetFirstFreeCompartmentOfType(ECompartmentType.CARGO);
		if (!slot)
			return false;

		SCR_CompartmentAccessComponent acc = SCR_CompartmentAccessComponent.Cast(c.GetCompartmentAccessComponent());
		return acc && acc.MoveInVehicle(veh, ECompartmentType.CARGO, false, slot);
	}

	protected int Deliver(IEntity veh)
	{
		int n;
		foreach (DCO_GroupUtilityComponent g : m_Job.m_aGroups)
		{
			SCR_AIGroup grp;
			if (g)
				grp = SCR_AIGroup.Cast(g.GetOwner());
			if (!grp)
				continue;

			array<AIAgent> agents = {};
			grp.GetAgents(agents);
			foreach (AIAgent a : agents)
			{
				IEntity ent = a.GetControlledEntity();
				if (!DCO_Util.IsAlive(ent) || vector.DistanceXZ(ent.GetOrigin(), veh.GetOrigin()) > 50.0)
					continue;

				SCR_InventoryStorageManagerComponent inv = SCR_InventoryStorageManagerComponent.Cast(ent.FindComponent(SCR_InventoryStorageManagerComponent));
				if (!inv)
					continue;
				inv.ResupplyMagazines(m_iResupplyMags);
				n++;
			}
		}
		return n;
	}

	protected int CountAboard(SCR_AIGroup grp, IEntity veh)
	{
		int n;
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			IEntity c = a.GetControlledEntity();
			if (c && DCO_VehicleCombat.GetVehicle(c) == veh)
				n++;
		}
		return n;
	}

	protected void ReleaseToDest(DCO_GroupUtilityComponent g, vector dest, string why)
	{
		if (!g || !g.GetOwner() || !m_Commander)
			return;
		SCR_AIGroup grp = SCR_AIGroup.Cast(g.GetOwner());
		if (!grp)
			return;
		grp.CompleteAllWaypoints();
		float d = vector.DistanceXZ(g.GetOwner().GetOrigin(), dest);
		bool moved = false;
		if (d > 20.0)
		{
			SCR_AIWaypoint wp = m_Commander.SpawnMoveWP(DCO_Util.Surface(dest));
			if (wp)
			{
				grp.AddWaypoint(wp);
				moved = true;
			}
		}
		DCO_Perf.Count("transport_release:" + why);
		DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("transport_release team=%1 grp=%2 reason=%3 dist=%4 move=%5", GetOwner().GetName(), grp.GetName(), why, Math.Round(d), moved));
	}

	protected bool HoldsBoardingWaypoint(DCO_GroupUtilityComponent g)
	{
		AIGroup ag;
		if (g)
			ag = AIGroup.Cast(g.GetOwner());
		return ag && SCR_BoardingWaypoint.Cast(ag.GetCurrentWaypoint()) != null;
	}

	protected int SplitLeftBehind(SCR_AIGroup passengerGrp, IEntity veh)
	{
		array<AIAgent> agents = {};
		array<AIAgent> left = {};
		passengerGrp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			ChimeraCharacter c = ChimeraCharacter.Cast(a.GetControlledEntity());
			if (!c || DCO_VehicleCombat.GetVehicle(c) == veh)
				continue;

			if (!DCO_Util.IsNotDead(c))
				continue;

			left.Insert(a);
		}

		if (left.IsEmpty())
			return 0;

		SCR_AIGroup newGrp = DCO_Logistics.SplitIntoNewGroup(passengerGrp, left, m_Commander);
		if (!newGrp && m_Job)
			DCO_Logistics.MoveLeftBehind(passengerGrp, left, m_Job.m_vDest, "transport_split_failed");
		DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), string.Format("%1 %2 members left behind -> new group %3", GetOwner().GetName(), left.Count(), newGrp));
		if (newGrp && m_Job && left.Count() >= 2 && CountFreeCargo(veh) == 0 && m_Commander && m_Commander.GetLogistics())
		{
			DCO_GroupUtilityComponent rest = DCO_GroupUtilityComponent.Cast(newGrp.FindComponent(DCO_GroupUtilityComponent));
			m_Commander.GetLogistics().QueueFollowUp(rest, m_Job, DCO_Util.NowS());
		}
		return left.Count();
	}

	protected void Depart(float worldTime, IEntity veh)
	{
		if (m_Job.m_eKind == DCO_ELogiJob.MEDEVAC)
		{
			m_Job.m_iBoarded = m_Job.m_aLoaded.Count();
		}
		else
		{
			array<DCO_GroupUtilityComponent> walkers = {};
			foreach (DCO_GroupUtilityComponent wg : m_Job.m_aGroups)
			{
				SCR_AIGroup wgrp;
				if (wg)
					wgrp = SCR_AIGroup.Cast(wg.GetOwner());
				if (wgrp && CountAboard(wgrp, veh) == 0)
					walkers.Insert(wg);
			}
			foreach (DCO_GroupUtilityComponent walker : walkers)
			{
				m_Job.m_aGroups.RemoveItem(walker);
				m_Job.m_iLeft += walker.GetUnitCount();
				if (walker.IsInTransport())
					walker.EndTransport();
				ReleaseToDest(walker, m_Job.m_vDest, "not_boarded");
			}
			foreach (DCO_GroupUtilityComponent g : m_Job.m_aGroups)
			{
				SCR_AIGroup grp = SCR_AIGroup.Cast(g.GetOwner());
				if (!grp)
					continue;
				m_Job.m_iLeft += SplitLeftBehind(grp, veh);
				m_Job.m_iBoarded += g.GetUnitCount();
				g.SetPhase(DCO_ETaskPhase.MOVING);
			}
			foreach (SCR_AIGroup pg : m_Job.m_aPlayerGroups)
			{
				int pin, pact;
				CountPlayersBoarded(pg, veh, pin, pact);
				m_Job.m_iBoarded += pin;
				m_Job.m_iLeft += pact - pin;
			}
		}

		SetTeamState(DCO_ETransportTeamState.MOVING_TO_DESTINATION, worldTime);
		m_fStateTimeout = Math.Max(m_fDriveTimeout, 60.0 + vector.Distance(GetTeamPos(), m_Job.m_vLZ) / 3.0);
		TeamMoveTo(m_Job.m_vLZ, worldTime);

		DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), string.Format("%1 departing job #%2 | boarded %3 | left behind %4",
			GetOwner().GetName(), m_Job.m_iId, m_Job.m_iBoarded, m_Job.m_iLeft));
	}

	protected void TickMovingToDestination(float worldTime)
	{
		if (!m_Job)
		{
			Finish(false, "job_lost", worldTime);
			return;
		}

		float dist = vector.DistanceXZ(GetTeamPos(), m_Job.m_vLZ);

		DCO_Logistics logi;
		if (m_Commander)
			logi = m_Commander.GetLogistics();
		if (logi && !m_Job.m_bAltUsed && dist < 300.0 && logi.IsLZHot(m_Commander, m_Job, m_SelfGroupUtil))
		{
			m_Job.m_bAltUsed = true;
			logi.RequestEscort(m_Commander, m_Job, m_Job.m_vLZ, worldTime);
			if (vector.DistanceXZ(m_Job.m_vAltLZ, m_Job.m_vLZ) > 1)
			{
				DCO_Logistics.Log(m_Job, "lz_hot", string.Format("from=%1 to=%2", m_Job.m_vLZ, m_Job.m_vAltLZ));
				m_Job.m_vLZ = m_Job.m_vAltLZ;
				logi.OnLZDiverted(m_Job);
				TeamMoveTo(m_Job.m_vLZ, worldTime);
				return;
			}
		}

		if (dist <= m_fArrivalDist)
		{
			OnArrive(worldTime);
			return;
		}

		if (CheckStuck(worldTime, m_Job.m_vLZ))
			return;

		if ((worldTime - m_fStateStartTime) > m_fStateTimeout)
		{
			DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), string.Format("%1 drive timeout — force disembark", GetOwner().GetName()));
			OnArrive(worldTime);
		}
	}

	protected void OnArrive(float worldTime)
	{
		if (m_Job.m_eKind == DCO_ELogiJob.MEDEVAC)
		{
			foreach (IEntity c : m_Job.m_aLoaded)
			{
				ChimeraCharacter ch = ChimeraCharacter.Cast(c);
				if (!ch)
					continue;
				CompartmentAccessComponent acc = ch.GetCompartmentAccessComponent();
				if (acc && acc.IsInCompartment())
					acc.GetOutVehicle(EGetOutType.TELEPORT, -1, ECloseDoorAfterActions.INVALID, false);
				if (m_Commander && m_Commander.GetLogistics())
					m_Commander.GetLogistics().AddHubCasualty(c, worldTime);
			}
		}
		else
		{
			DisembarkPassengers();
		}

		SetTeamState(DCO_ETransportTeamState.WAITING_FOR_DISEMBARK, worldTime);
		DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), string.Format("%1 arrived job #%2 — disembarking", GetOwner().GetName(), m_Job.m_iId));
	}

	protected void TickWaitingForDisembark(float worldTime)
	{
		if ((worldTime - m_fStateStartTime) < m_fDisembarkTimeout)
			return;

		Finish(true, "delivered", worldTime);
	}

	protected void TickReturning(float worldTime)
	{
		if (m_vRallyPoint == vector.Zero)
		{
			SetTeamState(DCO_ETransportTeamState.AVAILABLE, worldTime);
			return;
		}

		if (vector.DistanceXZ(GetTeamPos(), m_vRallyPoint) <= 30.0)
		{
			SetTeamState(DCO_ETransportTeamState.AVAILABLE, worldTime);
			if (m_SelfGroupUtil)
				m_SelfGroupUtil.SetPhase(DCO_ETaskPhase.HOLDING);

			DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), string.Format("%1 back at rally — available", GetOwner().GetName()));
			return;
		}

		CheckStuck(worldTime, m_vRallyPoint);
	}

	protected void Finish(bool success, string why, float worldTime)
	{
		DCO_LogiJob job = m_Job;
		m_Job = null;

		if (job)
		{
			IEntity finVeh = ResolveVehicle();
			foreach (DCO_GroupUtilityComponent g : job.m_aGroups)
			{
				if (g && g.IsInTransport())
					g.EndTransport();
				if (!success || !job.IsTransportKind() || !HoldsBoardingWaypoint(g))
					continue;
				SCR_AIGroup fg = SCR_AIGroup.Cast(g.GetOwner());
				if (fg && finVeh && CountAboard(fg, finVeh) > 0)
					continue;
				ReleaseToDest(g, job.m_vDest, "getout_held");
			}
			job.m_Team = null;
			if (m_Commander && m_Commander.GetLogistics())
				m_Commander.GetLogistics().OnJobFinished(m_Commander, job, success, why);
		}

		if (success)
			m_iJobsCompleted++;

		if (m_bCrewInfantry)
		{
			SetTeamState(DCO_ETransportTeamState.AVAILABLE, worldTime);
			return;
		}

		if (m_bReturnToRallyAfterDrop || !success)
			ReturnToRally(worldTime);
		else
			SetTeamState(DCO_ETransportTeamState.AVAILABLE, worldTime);
	}

	protected bool BoardGroup(DCO_GroupUtilityComponent g)
	{
		if (!g || !m_Commander)
			return false;

		SCR_AIGroup passengerGrp = SCR_AIGroup.Cast(g.GetOwner());
		IEntity veh = ResolveVehicle();
		if (!passengerGrp || !veh)
			return false;

		SCR_AIWaypoint wpGetIn = m_Commander.SpawnGetInWP(veh.GetOrigin());
		SCR_BoardingEntityWaypoint board = SCR_BoardingEntityWaypoint.Cast(wpGetIn);
		if (!board)
		{
			if (wpGetIn)
				SCR_EntityHelper.DeleteEntityAndChildren(wpGetIn);
			return false;
		}

		passengerGrp.CompleteAllWaypoints();

		board.SetEntity(veh);
		board.SetAllowance(false, false, true);
		board.SetPriorityLevel(SCR_AIBehaviorBase.PRIORITY_LEVEL_PLAYER);
		board.SetCompletionType(EAIWaypointCompletionType.All);

		passengerGrp.AddWaypoint(wpGetIn);
		return true;
	}

	protected void DisembarkPassengers()
	{
		if (!m_Commander)
			return;

		float surfY = GetGame().GetWorld().GetSurfaceY(m_Job.m_vLZ[0], m_Job.m_vLZ[2]);
		vector dest = Vector(m_Job.m_vLZ[0], surfY, m_Job.m_vLZ[2]);
		foreach (DCO_GroupUtilityComponent g : m_Job.m_aGroups)
		{
			SCR_AIGroup grp = SCR_AIGroup.Cast(g.GetOwner());
			if (!grp)
				continue;

			grp.CompleteAllWaypoints();
			SCR_AIWaypoint wpGetOut = m_Commander.SpawnGetOutWP(dest);
			if (wpGetOut)
				grp.AddWaypoint(wpGetOut);
			if (vector.DistanceXZ(dest, m_Job.m_vDest) > 20.0)
			{
				SCR_AIWaypoint wpOn = m_Commander.SpawnMoveWP(DCO_Util.Surface(m_Job.m_vDest));
				if (wpOn)
					grp.AddWaypoint(wpOn);
			}
		}
		foreach (SCR_AIGroup pg : m_Job.m_aPlayerGroups)
		{
			if (pg)
				DCO_Radio.Group(pg.GetGroupID(), "LOGISTICS", "logi_dismount", DCO_ERadioKind.GO);
		}
	}

	protected void ReturnToRally(float worldTime)
	{
		SetTeamState(DCO_ETransportTeamState.RETURNING, worldTime);

		if (m_vRallyPoint != vector.Zero)
			TeamMoveTo(m_vRallyPoint, worldTime);
	}

	protected void TeamMoveTo(vector pos, float worldTime)
	{
		if (!m_Commander || !m_SelfGroupUtil)
			return;

		SCR_AIGroup team = SCR_AIGroup.Cast(GetOwner());
		IEntity veh = ResolveVehicle();
		if (!team)
			return;

		team.CompleteAllWaypoints();

		int onFoot;
		array<AIAgent> agents = {};
		team.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			IEntity ent = a.GetControlledEntity();
			if (DCO_Util.IsAlive(ent) && DCO_VehicleCombat.GetVehicle(ent) != veh)
				onFoot++;
		}

		SCR_BaseCompartmentManagerComponent mgr;
		if (veh)
			mgr = SCR_BaseCompartmentManagerComponent.Cast(veh.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (onFoot > 0 && onFoot > CMD_VehicleFinder.CountFreeSeats(mgr))
		{
			DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.WARN, DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), string.Format("%1 %2 team members got no seat", GetOwner().GetName(), onFoot));
			onFoot = 0;
		}

		if (veh && onFoot > 0)
		{
			SCR_BoardingEntityWaypoint board = SCR_BoardingEntityWaypoint.Cast(m_Commander.SpawnGetInWP(veh.GetOrigin()));
			if (board)
			{
				board.SetEntity(veh);
				board.SetAllowance(true, true, true);
				board.SetCompletionType(EAIWaypointCompletionType.All);
				team.AddWaypoint(board);
			}
		}

		array<vector> pts = {};
		if (m_Commander.GetLogistics())
			m_Commander.GetLogistics().BuildRoute(m_Commander, GetTeamPos(), pos, pts);
		else
			pts.Insert(pos);

		if (m_Job && m_Commander.GetLogistics() && m_Commander.GetDefense() && m_Commander.GetDefense().CrossesDanger(GetTeamPos(), pos))
			m_Commander.GetLogistics().RequestEscort(m_Commander, m_Job, pos, worldTime, true);

		array<SCR_AIWaypoint> wps = {};
		foreach (vector p : pts)
		{
			SCR_AIWaypoint wp = m_Commander.SpawnMoveWP(p);
			if (wp)
				wps.Insert(wp);
		}
		if (!wps.IsEmpty())
			m_SelfGroupUtil.MoveToRoute(wps, worldTime);

		DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), string.Format("%1 moving to %2 | state %3 | vehicle %4 | on foot %5 | detour %6",
			GetOwner().GetName(), pos, typename.EnumToString(DCO_ETransportTeamState, m_eTeamState), veh != null, onFoot, pts.Count() > 1));
	}

	protected void SetTeamState(DCO_ETransportTeamState newState, float worldTime)
	{
		m_eTeamState      = newState;
		m_fStateStartTime = worldTime;
		m_vProgressPos    = GetTeamPos();
		m_fProgressTime   = worldTime;
		m_bRepathed       = false;
	}

	protected bool CheckStuck(float worldTime, vector target)
	{
		vector pos = GetTeamPos();
		if (vector.DistanceXZ(pos, m_vProgressPos) > 10.0)
		{
			m_vProgressPos = pos;
			m_fProgressTime = worldTime;
			return false;
		}
		if (worldTime - m_fProgressTime < 20.0)
			return false;

		m_fProgressTime = worldTime;
		float dist = vector.DistanceXZ(pos, target);
		if (dist <= 75.0)
		{
			LogEvent(string.Format("logi_near_arrive team=%1 state=%2 dist=%3", GetOwner().GetName(), typename.EnumToString(DCO_ETransportTeamState, m_eTeamState), Math.Round(dist)));
			if (m_eTeamState == DCO_ETransportTeamState.MOVING_TO_PASSENGER)
				OnAtPickup(worldTime);
			else if (m_eTeamState == DCO_ETransportTeamState.MOVING_TO_DESTINATION)
				OnArrive(worldTime);
			else
				SetTeamState(DCO_ETransportTeamState.AVAILABLE, worldTime);
			return true;
		}

		if (!m_bRepathed)
		{
			m_bRepathed = true;
			LogEvent(string.Format("logi_stuck_repath team=%1 state=%2 dist=%3", GetOwner().GetName(), typename.EnumToString(DCO_ETransportTeamState, m_eTeamState), Math.Round(dist)));
			TeamMoveTo(target, worldTime);
			return false;
		}

		AbandonVehicle(worldTime);
		return true;
	}

	protected void AbandonVehicle(float worldTime)
	{
		IEntity veh = ResolveVehicle();
		if (!veh)
			return;

		LogEvent(string.Format("logi_vehicle_stuck team=%1 state=%2 job=%3", GetOwner().GetName(), typename.EnumToString(DCO_ETransportTeamState, m_eTeamState), m_Job != null));

		array<DCO_GroupUtilityComponent> riders = {};
		array<SCR_AIGroup> playerRiders = {};
		vector lz;
		if (m_Job && m_eTeamState == DCO_ETransportTeamState.MOVING_TO_DESTINATION)
		{
			lz = m_Job.m_vLZ;
			if (m_Job.m_eKind == DCO_ELogiJob.MEDEVAC)
			{
				foreach (IEntity c : m_Job.m_aLoaded)
				{
					ChimeraCharacter ch = ChimeraCharacter.Cast(c);
					if (!ch)
						continue;
					CompartmentAccessComponent cacc = ch.GetCompartmentAccessComponent();
					if (cacc && cacc.IsInCompartment())
						cacc.GetOutVehicle(EGetOutType.TELEPORT, -1, ECloseDoorAfterActions.INVALID, false);
				}
			}
			else
			{
				riders.Copy(m_Job.m_aGroups);
				playerRiders.Copy(m_Job.m_aPlayerGroups);
			}
		}

		m_BadVehicle = veh;
		m_Vehicle = null;
		m_bHadVehicle = true;
		CheckVehicle(worldTime);

		vector here = veh.GetOrigin();
		if (m_Commander)
		{
			SCR_AIGroup team = SCR_AIGroup.Cast(GetOwner());
			SCR_AIWaypoint crewOut = m_Commander.SpawnGetOutWP(here);
			if (team && crewOut)
				team.AddWaypoint(crewOut);

			foreach (DCO_GroupUtilityComponent g : riders)
			{
				SCR_AIGroup grp;
				if (g)
					grp = SCR_AIGroup.Cast(g.GetOwner());
				if (!grp)
					continue;
				grp.CompleteAllWaypoints();
				SCR_AIWaypoint wpOut = m_Commander.SpawnGetOutWP(here);
				if (wpOut)
					grp.AddWaypoint(wpOut);
				SCR_AIWaypoint wpWalk = m_Commander.SpawnMoveWP(lz);
				if (wpWalk)
					g.MoveTo(wpWalk, worldTime);
			}
		}
		foreach (SCR_AIGroup pg : playerRiders)
		{
			if (pg)
				DCO_Radio.Group(pg.GetGroupID(), "LOGISTICS", "logi_dismount", DCO_ERadioKind.GO);
		}
	}

	protected void LogEvent(string line)
	{
		DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), line);
	}

	protected void CheckVehicle(float worldTime)
	{
		if (!m_Commander || !m_SelfGroupUtil)
			return;

		if (ResolveVehicle())
		{
			m_bHadVehicle = true;
			if (m_bCrewInfantry)
			{
				m_bCrewInfantry = false;
				m_SelfGroupUtil.DCO_SetDedicatedFlag(true);
				m_Commander.RegisterTransportTeam(this);
				DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), string.Format("%1 crew got a vehicle again - transport team again", GetOwner().GetName()));
			}
			return;
		}

		if (!m_bHadVehicle || m_bCrewInfantry)
			return;

		m_bCrewInfantry = true;
		if (m_Job)
			Finish(false, "vehicle_lost", worldTime);
		SetTeamState(DCO_ETransportTeamState.AVAILABLE, worldTime);

		SCR_AIGroup team = SCR_AIGroup.Cast(GetOwner());
		if (team)
			team.CompleteAllWaypoints();
		m_SelfGroupUtil.DCO_SetDedicatedFlag(false);
		m_SelfGroupUtil.SetTask(DCO_EGroupTask.NONE);
		m_Commander.UnregisterTransportTeam(this);

		string line = string.Format("logi_vehicle_lost team=%1 crew=%2 -> reserve infantry", GetOwner().GetName(), m_SelfGroupUtil.GetUnitCount());
		DCO_Log.Write(DCO_ELogChannel.LOGI, DCO_ELogLevel.INFO, DCO_Log.IdGroup(AIGroup.Cast(GetOwner())), line);
		if (m_SelfGroupUtil.IsAutoTransport())
		{
			m_SelfGroupUtil.DemoteAutoTransport("vehicle_lost");
			SetCommander(null);
			m_bCrewInfantry = false;
			m_bHadVehicle = false;
			m_Vehicle = null;
		}
	}

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer())
			return;

		float worldTime = DCO_Util.NowS();

		m_fVehicleCheck += timeSlice;
		if (m_fVehicleCheck >= 5)
		{
			m_fVehicleCheck = 0;
			CheckVehicle(worldTime);
		}

		if (m_eTeamState != DCO_ETransportTeamState.AVAILABLE)
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
		m_SelfGroupUtil = DCO_GroupUtilityComponent.Cast(owner.FindComponent(DCO_GroupUtilityComponent));
	}

	void OnThreatAhead(float worldTime)
	{
		if (!m_Commander || !m_Commander.GetLogistics())
			return;

		DCO_Logistics logi = m_Commander.GetLogistics();
		if (m_Job && m_eTeamState == DCO_ETransportTeamState.MOVING_TO_DESTINATION && !m_Job.m_bAltUsed
			&& vector.DistanceXZ(m_Job.m_vAltLZ, m_Job.m_vLZ) > 1 && logi.IsNearThreat(m_Commander, m_Job.m_vLZ, m_Commander.GetLogiLZSafeDist()))
		{
			m_Job.m_bAltUsed = true;
			DCO_Logistics.Log(m_Job, "lz_threat", string.Format("from=%1 to=%2", m_Job.m_vLZ, m_Job.m_vAltLZ));
			m_Job.m_vLZ = m_Job.m_vAltLZ;
			logi.OnLZDiverted(m_Job);
		}

		if (m_Job && m_eTeamState == DCO_ETransportTeamState.MOVING_TO_PASSENGER)
			TeamMoveTo(m_Job.m_vPickup, worldTime);
		else if (m_Job && m_eTeamState == DCO_ETransportTeamState.MOVING_TO_DESTINATION)
			TeamMoveTo(m_Job.m_vLZ, worldTime);
		else if (m_eTeamState == DCO_ETransportTeamState.RETURNING)
			TeamMoveTo(m_vRallyPoint, worldTime);
	}

	void AbortJob(string why, float worldTime)
	{
		if (m_Job)
			Finish(false, why, worldTime);
	}

	void OnPassengerReleased(DCO_GroupUtilityComponent grp, float worldTime)
	{
		if (!m_Job)
			return;

		m_Job.m_aGroups.RemoveItem(grp);
		if (m_Job.IsEmpty() && m_eTeamState != DCO_ETransportTeamState.RETURNING)
			Finish(false, "passenger_released", worldTime);
	}

	void ReleaseFromCommander(float worldTime)
	{
		if (m_Job)
		{
			m_bCrewInfantry = true;
			Finish(false, "team_released", worldTime);
			m_bCrewInfantry = false;
		}

		SetCommander(null);
		m_bRegistered = false;
		m_bCrewInfantry = false;
		m_bHadVehicle = false;
		SetTeamState(DCO_ETransportTeamState.AVAILABLE, worldTime);
	}
}
