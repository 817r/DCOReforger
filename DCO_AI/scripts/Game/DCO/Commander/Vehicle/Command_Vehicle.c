class CMD_VehicleFinder
{
	static IEntity FindNearestVehicle(IEntity ent, vector fromPos, int requiredSeats, DCO_GroupUtilityComponent requestingGroup = null)
	{
		IEntity bestVehicle = null;
		float   bestDist    = 500;

		if (!ent)
			return null;

		SCR_BaseCompartmentManagerComponent compMgr = SCR_BaseCompartmentManagerComponent.Cast(ent.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!compMgr)
			return null;;

		int freeSeats = CountFreeSeats(compMgr);
		if (freeSeats < requiredSeats)
			return null;

		if (IsVehicleClaimed(ent))
			return null;;

		if (IsVehicleOwnedByOther(ent, requestingGroup))
			return null;

		float dist = vector.Distance(fromPos, ent.GetOrigin());
		if (dist < bestDist)
		{
			bestDist    = dist;
			bestVehicle = ent;
		}

		return bestVehicle;
	}

	static bool IsVehicleOwnedByOther(IEntity vehicle, DCO_GroupUtilityComponent requestingGroup)
	{
		DCO_TransportMissionComponent mission =
			DCO_TransportMissionComponent.Cast(vehicle.FindComponent(DCO_TransportMissionComponent));

		if (!mission)
			return false;

		if (!mission.HasOwner())
			return false;

		return !mission.IsOwnedBy(requestingGroup);
	}


	static float SeatFitPenalty(IEntity vehicle, int requiredSeats)
	{
		SCR_BaseCompartmentManagerComponent compMgr = SCR_BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
		return Math.Max(CountFreeSeats(compMgr) - requiredSeats, 0) * 10.0;
	}

	static int CountFreeSeats(SCR_BaseCompartmentManagerComponent compMgr)
	{
		if (!compMgr)
			return 0;

		int free = 0;
		array<BaseCompartmentSlot> slots = {};
		compMgr.GetCompartments(slots);

		foreach (BaseCompartmentSlot slot : slots)
		{
			if (!slot)
				continue;

			if (!slot.GetOccupant())
				free = free + 1;
		}

		return free;
	}

	static bool IsVehicleClaimed(IEntity vehicle)
	{
		DCO_TransportMissionComponent mission =
			DCO_TransportMissionComponent.Cast(
				vehicle.FindComponent(DCO_TransportMissionComponent));

		if (!mission)
			return true;

		return mission.IsActiveVehicle();
	}
}