class DCO_MountedState
{
	IEntity m_Vehicle;
	SCR_AIGroup m_CrewGroup;
	int m_iMode;
	bool m_bMech;
	bool m_bSplitDone;
	float m_fNextFollow_ms;
	float m_fDismountAt_ms;
}

class DCO_MountedOps
{
	static DCO_EGroupCapability Classify(SCR_AIGroup grp, IEntity veh)
	{
		if (!veh || !DCO_VehicleCombat.IsArmored(veh))
			return DCO_EGroupCapability.MOTORIZED;
		PerceivableComponent perc = PerceivableComponent.Cast(veh.FindComponent(PerceivableComponent));
		if (perc && perc.GetUnitType() == EAIUnitType.UnitType_VehicleHeavy)
			return DCO_EGroupCapability.ARMOR;
		if (!grp)
			return DCO_EGroupCapability.MECHANIZED;

		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		int crew, alive;
		foreach (AIAgent a : agents)
		{
			IEntity e = a.GetControlledEntity();
			if (!e || !DCO_Util.IsAlive(e))
				continue;
			alive++;
			if (IsCrewSeat(e, veh))
				crew++;
		}
		if (crew == 0)
			crew = 2;
		if (alive - crew >= 2)
			return DCO_EGroupCapability.MECHANIZED;
		return DCO_EGroupCapability.ARMOR;
	}

	static bool IsCrewSeat(IEntity ent, IEntity veh)
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(ent);
		if (!ch || !ch.IsInVehicle() || DCO_VehicleCombat.GetVehicle(ch) != veh)
			return false;
		CompartmentAccessComponent access = ch.GetCompartmentAccessComponent();
		if (!access || !access.GetCompartment())
			return false;
		BaseCompartmentSlot slot = access.GetCompartment();
		if (PilotCompartmentSlot.Cast(slot))
			return true;
		return TurretCompartmentSlot.Cast(slot) != null;
	}

	static bool HasOwnVehicle(DCO_GroupUtilityComponent g)
	{
		return g && g.m_DCOMounted && g.m_DCOMounted.m_Vehicle && VehicleUsable(g.m_DCOMounted.m_Vehicle);
	}

	protected static bool VehicleUsable(IEntity veh)
	{
		DamageManagerComponent dmg = DamageManagerComponent.Cast(veh.FindComponent(DamageManagerComponent));
		return !dmg || dmg.GetState() != EDamageState.DESTROYED;
	}

	static void Tick(DCO_GroupUtilityComponent g, SCR_AIGroup grp, SCR_AIGroupUtilityComponent u)
	{
		AICommander_BaseComponent cmd = g.GetMyCommander();
		if (!cmd || g.IsPlayerGroup() || g.IsDedicatedTransport() || g.IsInTransport() || g.m_DCOSupportOf)
			return;

		DCO_MountedState st = g.m_DCOMounted;
		if (!st)
		{
			DCO_EGroupCapability cap = g.GetCapability();
			if (cap != DCO_EGroupCapability.MOTORIZED && cap != DCO_EGroupCapability.MECHANIZED)
				return;
			IEntity veh = g.GetGroupVehicle();
			if (!veh || !DCO_VehicleCombat.GetVehicle(grp.GetLeaderEntity()))
				return;
			st = new DCO_MountedState();
			st.m_Vehicle = veh;
			st.m_bMech = cap == DCO_EGroupCapability.MECHANIZED;
			g.m_DCOMounted = st;
			DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("veh_group grp=%1 type=%2 veh=%3", grp, typename.EnumToString(DCO_EGroupCapability, cap), veh));
		}

		if (!st.m_bSplitDone)
		{
			st.m_bSplitDone = true;
			SplitExtra(g, grp, st, cmd);
		}

		if (!st.m_Vehicle || !VehicleUsable(st.m_Vehicle))
		{
			VehicleLost(g, grp, st, cmd, "destroyed");
			return;
		}

		float now = GetGame().GetWorld().GetWorldTime();
		IEntity leader = grp.GetLeaderEntity();
		if (!leader)
			return;
		vector lp = leader.GetOrigin();
		bool contact = u.m_Perception && !u.m_Perception.m_aTargetClusters.IsEmpty();

		vector target;
		bool hasTarget;
		if (g.GetGroupObjective() && g.GetGroupObjective().GetOwner())
		{
			target = g.GetGroupObjective().GetOwner().GetOrigin();
			hasTarget = true;
		}
		else if (grp.GetCurrentWaypoint())
		{
			target = grp.GetCurrentWaypoint().GetOrigin();
			hasTarget = true;
		}

		if (st.m_iMode == 0)
		{
			if (!DCO_VehicleCombat.GetVehicle(leader))
				return;
			float dismountDist = 400.0;
			if (st.m_bMech)
				dismountDist = 175.0;
			bool mountedAssault = MountedAssault(g, grp, cmd, st, target, hasTarget);
			if (mountedAssault)
				dismountDist = 50.0;
			float d = float.MAX;
			if (hasTarget)
				d = vector.DistanceXZ(lp, target);
			bool atStop = !grp.GetCurrentWaypoint() && g.GetPhase() != DCO_ETaskPhase.MOVING;
			if ((contact && !mountedAssault) || d < dismountDist || atStop)
			{
				string why = "distance";
				if (contact && !mountedAssault)
					why = "contact";
				else if (atStop)
					why = "arrived";
				Dismount(g, grp, st, cmd, target, hasTarget, why);
			}
			return;
		}

		if (st.m_iMode == 5)
			return;

		SCR_AIGroup crewGrp = st.m_CrewGroup;
		int crewAlive = CountAlive(crewGrp);
		if (crewAlive == 0)
		{
			if (!contact && vector.DistanceXZ(st.m_Vehicle.GetOrigin(), lp) < 200.0 && st.m_iMode != 4)
			{
				Board(grp, st, cmd, false);
				st.m_iMode = 4;
				DCO_Perf.Count("veh:crew_replace");
				DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("veh_crew_replace grp=%1", grp));
			}
			if (st.m_iMode == 4 && DCO_VehicleCombat.GetVehicle(leader) == st.m_Vehicle)
			{
				st.m_iMode = 0;
				st.m_CrewGroup = null;
				g.SetOwnedVehicle(st.m_Vehicle);
			}
			return;
		}

		AIWaypoint wp = grp.GetCurrentWaypoint();
		bool farMove = wp && vector.DistanceXZ(wp.GetOrigin(), lp) > 600.0 && !contact;

		if (st.m_iMode == 3 || farMove)
		{
			if (st.m_iMode != 3)
			{
				st.m_iMode = 3;
				DCO_Perf.Count("veh:pickup");
				DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("veh_pickup grp=%1 dist=%2", grp, Math.Round(vector.DistanceXZ(st.m_Vehicle.GetOrigin(), lp))));
				MoveGroup(crewGrp, cmd, lp);
			}
			if (vector.DistanceXZ(st.m_Vehicle.GetOrigin(), lp) < 40.0)
				Remount(g, grp, st, cmd);
			else if (now >= st.m_fNextFollow_ms)
			{
				st.m_fNextFollow_ms = now + 10000.0;
				MoveGroup(crewGrp, cmd, lp);
			}
			return;
		}

		if (st.m_bMech && now >= st.m_fNextFollow_ms)
		{
			st.m_fNextFollow_ms = now + 5000.0;
			if (DCO_VehicleCombat.IsATKnown(st.m_Vehicle, null))
				return;
			vector back = lp - target;
			back[1] = 0;
			if (!hasTarget || back.Length() < 1)
				back = st.m_Vehicle.GetOrigin() - lp;
			back[1] = 0;
			if (back.Length() < 1)
				return;
			back.Normalize();
			vector pos = DCO_Util.Surface(lp + back * 100.0);
			float vd = vector.DistanceXZ(st.m_Vehicle.GetOrigin(), pos);
			if (vd > 50.0 && vector.DistanceXZ(st.m_Vehicle.GetOrigin(), lp) > 150.0 || vd > 120.0)
			{
				MoveGroup(crewGrp, cmd, pos);
				DCO_Perf.Count("veh:mech_follow");
			}
		}
	}

	protected static int CountAlive(SCR_AIGroup grp)
	{
		if (!grp)
			return 0;
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		int n;
		foreach (AIAgent a : agents)
		{
			if (a && DCO_Util.IsAlive(a.GetControlledEntity()))
				n++;
		}
		return n;
	}

	protected static void MoveGroup(SCR_AIGroup grp, AICommander_BaseComponent cmd, vector pos)
	{
		if (!grp)
			return;
		SCR_AIWaypoint wp = cmd.SpawnMoveWP(pos);
		if (!wp)
			return;
		grp.CompleteAllWaypoints();
		grp.AddWaypoint(wp);
	}

	protected static bool MountedAssault(DCO_GroupUtilityComponent g, SCR_AIGroup grp, AICommander_BaseComponent cmd, DCO_MountedState st, vector target, bool hasTarget)
	{
		if (!hasTarget)
			return false;
		DCO_EAIPersonality pers = DCO_EAIPersonality.STANDARD;
		SCR_ChimeraAIAgent la = SCR_ChimeraAIAgent.Cast(grp.GetLeaderAgent());
		if (la)
			pers = DCO_AIConfigComponent.PersonalityOf(la.m_UtilityComponent);
		if (pers == DCO_EAIPersonality.CAUTIOUS)
			return false;

		bool at = DCO_VehicleCombat.IsATKnown(st.m_Vehicle, null);
		bool armor;
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (tr)
		{
			float now = DCO_Util.NowS();
			foreach (CMD_ThreatEntry t : tr.GetThreats())
			{
				if (!t || vector.DistanceXZ(t.m_vBelievedPos, target) > 400.0)
					continue;
				armor = armor || t.m_bArmorSeen;
				if (t.m_bATSeen && now - t.m_fLastUpdateTime <= 60.0)
					at = true;
			}
		}
		if (armor)
			return false;
		if (pers == DCO_EAIPersonality.RECKLESS)
			return true;
		if (at)
			return false;
		if (pers == DCO_EAIPersonality.AGGRESSIVE)
			return true;

		int raw;
		float enemy = cmd.GetOps().Estimate(cmd, target, 300.0, DCO_Util.NowS(), false, raw);
		return raw > 0 && g.GetUnitCount() >= enemy * 2.0;
	}

	protected static void Dismount(DCO_GroupUtilityComponent g, SCR_AIGroup grp, DCO_MountedState st, AICommander_BaseComponent cmd, vector target, bool hasTarget, string why)
	{
		IEntity veh = st.m_Vehicle;
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		array<AIAgent> crew = {};
		array<AIAgent> dismounts = {};
		bool hasTurret;
		foreach (AIAgent a : agents)
		{
			IEntity e = a.GetControlledEntity();
			if (!e || !DCO_Util.IsAlive(e))
				continue;
			if (IsCrewSeat(e, veh))
			{
				crew.Insert(a);
				ChimeraCharacter ch = ChimeraCharacter.Cast(e);
				if (ch && TurretCompartmentSlot.Cast(ch.GetCompartmentAccessComponent().GetCompartment()))
					hasTurret = true;
			}
			else
				dismounts.Insert(a);
		}
		if (crew.IsEmpty() || dismounts.IsEmpty())
		{
			st.m_iMode = 5;
			return;
		}
		if (!hasTurret && dismounts.Count() > 2)
		{
			crew.Insert(dismounts[dismounts.Count() - 1]);
			dismounts.Remove(dismounts.Count() - 1);
		}

		SCR_AIGroup crewGrp = DCO_Logistics.SplitIntoNewGroup(grp, crew, null);
		if (!crewGrp)
			return;
		DCO_GroupUtilityComponent cu = DCO_GroupUtilityComponent.Cast(crewGrp.FindComponent(DCO_GroupUtilityComponent));
		if (cu)
		{
			cu.m_DCOSupportOf = g;
			cu.SetOwnedVehicle(veh);
		}
		st.m_CrewGroup = crewGrp;
		g.SetOwnedVehicle(null);

		IEntity leader = grp.GetLeaderEntity();
		if (leader)
		{
			SCR_AIWaypoint getOut = cmd.SpawnGetOutWP(leader.GetOrigin());
			if (getOut)
				grp.AddWaypointAt(getOut, 0);
		}

		st.m_fDismountAt_ms = GetGame().GetWorld().GetWorldTime();
		if (st.m_bMech)
		{
			st.m_iMode = 2;
			DCO_Perf.Count("veh:dismount_mech");
		}
		else
		{
			st.m_iMode = 1;
			vector away = veh.GetOrigin() - target;
			if (!hasTarget)
				away = -veh.GetWorldTransformAxis(2);
			away[1] = 0;
			if (away.Length() > 1)
				away.Normalize();
			vector laager = veh.GetOrigin() + away * 250.0;
			vector cover = DCO_TerrainCache.QueryBest(DCO_ETerrainFlag.FOREST_EDGE, laager, 150.0, string.Empty, laager);
			if (cover != vector.Zero)
				laager = cover;
			MoveGroup(crewGrp, cmd, DCO_Util.Surface(laager));
			DCO_Perf.Count("veh:dismount_motor");
		}
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("veh_dismount grp=%1 mech=%2 why=%3 crew=%4 dismounts=%5", grp, st.m_bMech, why, crew.Count(), dismounts.Count()));
	}

	protected static void Board(SCR_AIGroup grp, DCO_MountedState st, AICommander_BaseComponent cmd, bool front)
	{
		SCR_AIWaypoint wp = cmd.SpawnGetInWP(st.m_Vehicle.GetOrigin());
		SCR_BoardingEntityWaypoint board = SCR_BoardingEntityWaypoint.Cast(wp);
		if (!board)
			return;
		board.SetEntity(st.m_Vehicle);
		board.SetAllowance(true, true, true);
		board.SetCompletionType(EAIWaypointCompletionType.All);
		grp.AddWaypointAt(board, 0);
	}

	protected static void Remount(DCO_GroupUtilityComponent g, SCR_AIGroup grp, DCO_MountedState st, AICommander_BaseComponent cmd)
	{
		SCR_AIGroup crewGrp = st.m_CrewGroup;
		if (crewGrp)
		{
			array<AIAgent> agents = {};
			crewGrp.GetAgents(agents);
			foreach (AIAgent a : agents)
			{
				crewGrp.RemoveAgent(a);
				grp.AddAgent(a);
			}
		}
		st.m_CrewGroup = null;
		st.m_iMode = 0;
		g.SetOwnedVehicle(st.m_Vehicle);
		Board(grp, st, cmd, true);
		DCO_Perf.Count("veh:remount");
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("veh_remount grp=%1", grp));
	}

	protected static void VehicleLost(DCO_GroupUtilityComponent g, SCR_AIGroup grp, DCO_MountedState st, AICommander_BaseComponent cmd, string why)
	{
		SCR_AIGroup crewGrp = st.m_CrewGroup;
		if (crewGrp)
		{
			array<AIAgent> agents = {};
			crewGrp.GetAgents(agents);
			foreach (AIAgent a : agents)
			{
				crewGrp.RemoveAgent(a);
				grp.AddAgent(a);
			}
		}
		g.m_DCOMounted = null;
		g.SetOwnedVehicle(null);
		IEntity leader = grp.GetLeaderEntity();
		if (leader && DCO_VehicleCombat.GetVehicle(leader))
		{
			SCR_AIWaypoint getOut = cmd.SpawnGetOutWP(leader.GetOrigin());
			if (getOut)
				grp.AddWaypointAt(getOut, 0);
		}
		DCO_Perf.Count("veh:vehicle_lost");
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("veh_lost grp=%1 why=%2", grp, why));
	}

	protected static void SplitExtra(DCO_GroupUtilityComponent g, SCR_AIGroup grp, DCO_MountedState st, AICommander_BaseComponent cmd)
	{
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		map<IEntity, ref array<AIAgent>> byVeh = new map<IEntity, ref array<AIAgent>>();
		array<AIAgent> onFoot = {};
		int inMain;
		foreach (AIAgent a : agents)
		{
			IEntity e = a.GetControlledEntity();
			if (!e || !DCO_Util.IsAlive(e))
				continue;
			IEntity v = DCO_VehicleCombat.GetVehicle(e);
			if (!v)
			{
				onFoot.Insert(a);
				continue;
			}
			if (v == st.m_Vehicle)
			{
				inMain++;
				continue;
			}
			array<AIAgent> list = byVeh.Get(v);
			if (!list)
			{
				list = {};
				byVeh.Set(v, list);
			}
			list.Insert(a);
		}

		foreach (IEntity v, array<AIAgent> list : byVeh)
		{
			SCR_AIGroup ng = DCO_Logistics.SplitIntoNewGroup(grp, list, cmd);
			DCO_GroupUtilityComponent nu;
			if (ng)
				nu = DCO_GroupUtilityComponent.Cast(ng.FindComponent(DCO_GroupUtilityComponent));
			if (nu)
				nu.SetOwnedVehicle(v);
			DCO_Perf.Count("veh:split_vehicle");
			DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("veh_split grp=%1 new=%2 veh=%3 units=%4", grp, ng, v, list.Count()));
		}

		if (onFoot.IsEmpty() || inMain == 0)
			return;
		SCR_BaseCompartmentManagerComponent mgr = SCR_BaseCompartmentManagerComponent.Cast(st.m_Vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (CMD_VehicleFinder.CountFreeSeats(mgr) >= onFoot.Count())
			return;
		SCR_AIGroup og = DCO_Logistics.SplitIntoNewGroup(grp, onFoot, cmd);
		DCO_Perf.Count("veh:split_overflow");
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("veh_split_overflow grp=%1 new=%2 units=%3", grp, og, onFoot.Count()));
	}
}
