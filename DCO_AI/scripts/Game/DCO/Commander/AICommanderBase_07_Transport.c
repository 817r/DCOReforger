modded class AICommander_BaseComponent
{
	protected void DCO_GetInBlock(DCO_GroupUtilityComponent g, IEntity veh, string key, string why, float nowS)
	{
		m_mDCOGetInBlock.Set(key, nowS + m_fGetInBlock_s);
		DCO_Perf.Count("getin_block:" + why);
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), "getin_block grp=" + g.GetOwner().GetName() + " veh=" + string.Format("%1", veh.GetName()) + " reason=" + why + " block_s=" + Math.Round(m_fGetInBlock_s).ToString());
	}

	protected void DCO_NoteGetInTry(DCO_GroupUtilityComponent g, IEntity veh)
	{
		float nowS = DCO_Util.NowS();
		string key = string.Format("%1|%2", g.GetOwner().GetName(), veh);
		float first;
		if (!m_mDCOGetInFirst.Find(key, first) || nowS - first > m_fGetInRetryWindow_s)
		{
			m_mDCOGetInFirst.Set(key, nowS);
			m_mDCOGetInTries.Set(key, 1);
			return;
		}
		int tries = m_mDCOGetInTries.Get(key) + 1;
		m_mDCOGetInTries.Set(key, tries);
		if (tries >= m_iGetInMaxTries)
		{
			m_mDCOGetInFirst.Remove(key);
			m_mDCOGetInTries.Remove(key);
			DCO_GetInBlock(g, veh, key, "failed_x2", nowS);
		}
	}

	protected IEntity TryAssignTransports(DCO_GroupUtilityComponent passengerGroup)
	{
		if (!passengerGroup)
			return null;

		int unitCount = passengerGroup.GetUnitCount();
		if (unitCount <= 0)
			return null;

		IEntity ownedVeh = passengerGroup.GetOwnedVehicle();
		if (ownedVeh)
		{
			DamageManagerComponent dmg = DamageManagerComponent.Cast(ownedVeh.FindComponent(DamageManagerComponent));
			bool vehicleDestroyed = dmg && dmg.GetState() == EDamageState.DESTROYED;

			DCO_TransportMissionComponent ownedMission = DCO_TransportMissionComponent.Cast(ownedVeh.FindComponent(DCO_TransportMissionComponent));

			float ownedDist = vector.Distance(passengerGroup.GetOwner().GetOrigin(), ownedVeh.GetOrigin());
			if (!vehicleDestroyed && ownedMission && !ownedMission.IsActiveVehicle() && ownedDist <= m_fVehicleSearchRadius * m_fOwnedVehicleReachMul)
				return ownedVeh;

			if (vehicleDestroyed || !ownedMission)
			{
				if (ownedMission)
					ownedMission.ReleaseOwnership();
				passengerGroup.SetOwnedVehicle(null);
			}
		}

		vector groupPos = passengerGroup.GetOwner().GetOrigin();
		IEntity vehicle  = null;
		float   bestScore = float.MAX;

		float nowS = DCO_Util.NowS();
		foreach (IEntity cand : m_aVehicle)
		{
			if (!cand)
				continue;
			float d = vector.Distance(groupPos, cand.GetOrigin());
			if (d > m_fVehicleSearchRadius)
				continue;

			if (!CMD_VehicleFinder.FindNearestVehicle(cand, groupPos, unitCount, passengerGroup))
				continue;

			string blockKey = string.Format("%1|%2", passengerGroup.GetOwner().GetName(), cand);
			if (m_mDCOGetInBlock.Get(blockKey) > nowS)
				continue;
			string why = CMD_VehicleFinder.BlockReason(cand, passengerGroup);
			if (!why.IsEmpty())
			{
				DCO_GetInBlock(passengerGroup, cand, blockKey, why, nowS);
				continue;
			}

			float score = d + CMD_VehicleFinder.SeatFitPenalty(cand, unitCount);
			if (score < bestScore)
			{
				bestScore = score;
				vehicle   = cand;
			}
		}

		return vehicle;
	}

	protected void BeginTransportMission(
		DCO_GroupUtilityComponent passengerGroup,
		IEntity                   vehicle,
		vector                    destination,
		float                     worldTime)
	{
		DCO_TransportMissionComponent mission =
			DCO_TransportMissionComponent.Cast(vehicle.FindComponent(DCO_TransportMissionComponent));

		if (!mission)
		{
			return;
		}

		if (mission.IsActiveVehicle())
		{
			return;
		}

		mission.ClaimOwnership(passengerGroup);
		passengerGroup.SetOwnedVehicle(vehicle);

		passengerGroup.BeginTransport(DCO_EGroupTask.NONE);

		SCR_AIGroup grp = SCR_AIGroup.Cast(passengerGroup.GetOwner());
		if (!grp)
			return;

		grp.CompleteAllWaypoints();

		SCR_AIWaypoint wpToVehicle = SpawnMoveWP(vehicle.GetOrigin());
		if (wpToVehicle)
			passengerGroup.MoveTo(wpToVehicle, worldTime);

		mission.StartMission(passengerGroup, destination, m_sFactionKey, worldTime, this);
	}

	bool TryAssignTransport(DCO_GroupUtilityComponent passengerGroup, vector destination, float worldTime, DCO_EGroupTask taskAfter = DCO_EGroupTask.NONE)
	{
		if (!passengerGroup || !passengerGroup.CanBeTransported() || DCO_MountedOps.HasOwnVehicle(passengerGroup))
			return false;

		float dist = vector.Distance(passengerGroup.GetOwner().GetOrigin(), destination);
		if (dist < m_fTransportDistanceThreshold)
			return false;

		int unitCount = passengerGroup.GetUnitCount();
		if (unitCount <= 0)
			return false;

		string skip;
		if (passengerGroup.HasState(DCO_EGroupState.IN_CONTACT))
			skip = "in_contact";
		else if (DCO_NearEnemyObjective(passengerGroup.GetOwner().GetOrigin(), m_fTransportEnemyObjClearance))
			skip = "near_enemy_obj";
		else if (DCO_PlanGuard(passengerGroup, "transport"))
			skip = "plan_owned";
		if (!skip.IsEmpty())
		{
			DCO_Perf.Count("transport_skip:" + skip);
			if (DCO_Log.On(DCO_ELogChannel.VEH, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.DEBUG, DCO_Log.IdCmd(this), "transport_skip grp=" + passengerGroup.GetOwner().GetName() + " reason=" + skip + " dist=" + Math.Round(dist).ToString());
			return false;
		}

		if (m_Logistics.RequestTransport(this, passengerGroup, destination, taskAfter, worldTime))
			return true;

		vector groupPos = passengerGroup.GetOwner().GetOrigin();
		IEntity vehicle = TryAssignTransports(passengerGroup);

		if (!vehicle)
        	return false;
		DCO_NoteGetInTry(passengerGroup, vehicle);

		passengerGroup.SetTask(taskAfter, "AICommanderBase:7606");
		BeginTransportMission(passengerGroup, vehicle, destination, worldTime);
		return true;
	}
}
