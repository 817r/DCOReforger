class CMD_VehicleQueryCollector
{
	ref array<IEntity> m_aEntities = {};

	bool OnEntity(IEntity ent)
	{
		if (ent.FindComponent(BaseCompartmentManagerComponent) && (Turret.Cast(ent) || SCR_AIVehicleUsability.VehicleCanMove(ent)) && !SCR_AIVehicleUsability.VehicleIsOnFire(ent))
			return true;
		return false;
	}
}

class CMD_VehicleFinder
{
	static IEntity FindNearestVehicle(IEntity ent, vector fromPos, int requiredSeats)
	{
		IEntity bestVehicle = null;
		float   bestDist    = 500;
		
		if (!ent)
			return null;

		SCR_BaseCompartmentManagerComponent compMgr = SCR_BaseCompartmentManagerComponent.Cast(ent.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!compMgr)
			return null;;

		int freeSeats = CountFreeSeats(compMgr);
		if (freeSeats < requiredSeats || freeSeats > requiredSeats * 2)
			return null;

		if (IsVehicleClaimed(ent))
			return null;;

		float dist = vector.Distance(fromPos, ent.GetOrigin());
		if (dist < bestDist)
		{
			bestDist    = dist;
			bestVehicle = ent;
		}

		return bestVehicle;
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