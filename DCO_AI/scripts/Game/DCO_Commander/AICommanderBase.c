[ComponentEditorProps(category: "GameScripted/Commander")]
class AICommander_BaseComponentClass : ScriptComponentClass
{
	[Attribute("{35BD6541CBB8AC08}Prefabs/AI/Waypoints/AIWaypoint_Cycle.et", UIWidgets.ResourceNamePicker, desc: "Cycle waypoint to be used for waypoints in hierarchy.", "et", category: "Commander Waypoint Setting")]
	protected ResourceName m_sCycleWaypointPrefab;

	[Attribute("{FFF9518F73279473}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_Move.et", UIWidgets.ResourceNamePicker, desc: "Waypoint to be used Move.", "et", category: "Commander Waypoint Setting")]
	protected ResourceName m_sDefaultMoveWaypointPrefab;
	
	[Attribute("{D9C14ECEC9772CC6}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_Defend.et", UIWidgets.ResourceNamePicker, desc: "Waypoint to be used Defend.", "et", category: "Commander Waypoint Setting")]
	protected ResourceName m_sDefaultDefendWaypointPrefab;
	
	[Attribute("{2E6D3ABB8094159A}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_GetIn.et", UIWidgets.ResourceNamePicker, desc: "Waypoint to be used Get In.", "et", category: "Commander Waypoint Setting")]
	protected ResourceName m_sDefaultGetInWaypointPrefab;
	
	[Attribute("{2602CAB8AB74FBBF}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_GetOut.et", UIWidgets.ResourceNamePicker, desc: "Waypoint to be used Get Out.", "et", category: "Commander Waypoint Setting")]
	protected ResourceName m_sDefaultGetOutWaypointPrefab;
	

	ResourceName GetCycleWaypointPrefab()
	{
		return m_sCycleWaypointPrefab;
	}

	ResourceName GetDefaultMoveWaypointPrefab()
	{
		return m_sDefaultMoveWaypointPrefab;
	}
	
	ResourceName GetDefaultDefendWaypointPrefab()
	{
		return m_sDefaultDefendWaypointPrefab;
	}
	
	ResourceName GetDefaultGetInWaypointPrefab()
	{
		return m_sDefaultGetInWaypointPrefab;
	}
	
	ResourceName GetDefaultGetOutWaypointPrefab()
	{
		return m_sDefaultGetOutWaypointPrefab;
	}
}

class AICommander_BaseComponent : ScriptComponent
{
	[Attribute("", UIWidgets.Font, desc: "UID of the Commander.", category: "Commander General Setting")]
	protected string m_sCommanderUID;
	
	[Attribute("", UIWidgets.Auto, desc: "Faction Key of the Commander.", category: "Commander General Setting")]
	protected FactionKey m_sFactionKey;	
	
	[Attribute("3.0", UIWidgets.Auto, "Number of the Objective Can be processed at the same time", category: "Commander Objective Setting")]
	protected int m_fObjectiveAtTheSameTime;
	
	[Attribute("30.0", UIWidgets.EditBox, "Interval think cycle dalam detik.", category: "Commander")]
	protected float m_fThinkInterval;
	
	[Attribute("60.0", UIWidgets.EditBox, "Delay Before Start First Iteration", category: "Commander")]
	protected float m_fDelayFirstIteration;
 
	[Attribute("2", UIWidgets.EditBox, "Unit count minimum group sebelum dipaksa retreat.", category: "Commander")]
	protected int m_iRetreatThreshold;
	
	[Attribute("15.0", UIWidgets.EditBox, "Interval cek capture progress (detik)", category: "Commander")]
	protected float m_fCaptureCheckInterval;
	
	[Attribute("400.0", UIWidgets.EditBox, "Jarak minimum sebelum cari transport", category: "Commander")]
	protected float m_fTransportDistanceThreshold;
	
	[Attribute("50.0", UIWidgets.EditBox, "Radius pencarian kendaraan", category: "Commander")]
	protected float m_fVehicleSearchRadius;
	
	[Attribute("50.0", UIWidgets.EditBox, "Radius pencarian kendaraan", category: "Commander")]
	protected float m_fBaseRadius;
 
	protected float m_fCaptureCheckTimer = 0.0;
	
	protected ref array<CMD_AICommanderObjectiveComponent> m_aObjective = {};
	
	protected CMD_ECommanderState m_eCommanderState = CMD_ECommanderState.IDLE;
	
	protected CMD_ThreatResponseComponent threatComp;
	
	protected ref array<DCO_GroupUtilityComponent> m_aOwnedGroup = {};
	protected ref array<IEntity> m_aVehicle = {};
	
	protected ref array<DCO_TransportTeamComponent> m_aTransportTeams = {};
	
	protected float m_fThinkTimer = 0 - m_fDelayFirstIteration;
	
	protected const float m_fFlankAngleMin = 15;
	protected const float m_fFlankAngleMax = 315;
	
	int GetOwnedGroupCount() { return m_aOwnedGroup.Count(); }
	int GetOwnedVehicle()	{return m_aVehicle.Count(); }
	
	bool send = true;
	
	bool RegisterGroup(DCO_GroupUtilityComponent grp)
	{
		if (!m_aOwnedGroup.Contains(grp))
			m_aOwnedGroup.Insert(grp);
		
		return true;
	}
	
	bool RegisterVehicle(IEntity grp)
	{
		if (!m_aVehicle.Contains(grp))
			m_aVehicle.Insert(grp);
		
		return true;
	}
	
	bool IsGroupHere(DCO_GroupUtilityComponent grp)
	{
		return m_aOwnedGroup.Contains(grp);
	}
	
	bool UnregisterGroup(DCO_GroupUtilityComponent grp)
	{
		if (m_aOwnedGroup.Contains(grp))
			m_aOwnedGroup.RemoveItem(grp);
		
		return true;
	}
	
	protected void InitializeCommander()
	{
		AICommander_ManagerComponent.GetInstance().RegisterCommander(this);
		threatComp = CMD_ThreatResponseComponent.Cast(GetOwner().FindComponent(CMD_ThreatResponseComponent));
	}
	
	CMD_ThreatResponseComponent GetThreatResponseComponent()
	{
		return threatComp;
	}
	
	FactionKey GetCommanderFactionKey()
	{
		return m_sFactionKey;
	}
	
	string GetCommanderUID()
	{
		return m_sCommanderUID;
	}
	
	protected vector ComputeFlankPosition(vector base, vector objective, float distance)
	{
		vector axis = objective - base;
		axis = Vector(axis[0], 0.0, axis[2]);
		axis = axis.Normalized();
	 
		float angleDeg = Math.RandomFloat(m_fFlankAngleMin, m_fFlankAngleMax);
		float angleRad = angleDeg * Math.DEG2RAD;
	 
		float cosA = Math.Cos(angleRad);
		float sinA = Math.Sin(angleRad);

		vector dirLeft  = Vector(
			axis[0] * cosA - axis[2] * sinA,
			0.0,
			axis[0] * sinA + axis[2] * cosA);
	 
		vector dirRight = Vector(
			axis[0] * cosA + axis[2] * sinA,
			0.0,
			axis[0] * (-sinA) + axis[2] * cosA);
	 
		float axisRatio = Math.RandomFloat(0.45, 0.70);
		float totalDist = vector.Distance(base, objective);
		vector midpoint = base + axis * (totalDist * axisRatio);
	 
		vector candidateLeft  = midpoint + dirLeft  * distance;
		vector candidateRight = midpoint + dirRight * distance;
	 
		candidateLeft[1]  = GetGame().GetWorld().GetSurfaceY(candidateLeft[0],  candidateLeft[2]);
		candidateRight[1] = GetGame().GetWorld().GetSurfaceY(candidateRight[0], candidateRight[2]);
	 
		float scoreLeft  = EvaluateFlankCandidate(candidateLeft,  objective);
		float scoreRight = EvaluateFlankCandidate(candidateRight, objective);
	 
		vector chosen;
	 
		if (scoreLeft >= scoreRight)
			chosen = candidateLeft;
		else
			chosen = candidateRight;
	 
		chosen[1] = GetGame().GetWorld().GetSurfaceY(chosen[0], chosen[2]);
	 
		return chosen;
	}
	
	protected float EvaluateFlankCandidate(vector candidate, vector objective)
	{
		vector toObjective = objective - candidate;
		toObjective = Vector(toObjective[0], 0.0, toObjective[2]);
		toObjective = toObjective.Normalized();
	 
		vector toBase = GetOwner().GetOrigin() - candidate;
		toBase = Vector(toBase[0], 0.0, toBase[2]);
		toBase = toBase.Normalized();
	 
		float dot         = vector.Dot(toObjective, toBase);
		float angleScore  = 1.0 - Math.AbsFloat(dot);
	 
		return angleScore;
	}
 
	protected void AssignRolesToObjective(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
	    CMD_EObjectiveState objState = obj.GetObjectiveState(m_sFactionKey);
	
	    if (objState == CMD_EObjectiveState.COMPLETED || objState == CMD_EObjectiveState.FAILED)
	        return;
	
	    if (objState == CMD_EObjectiveState.PENDING)
	    {
	        TrySendRecon(obj);
	        TrySendToStaging(obj, worldTime);
	        return;
	    }
	
	    if (objState == CMD_EObjectiveState.ASSIGNED)
	    {
	        if (!obj.IsReconArrived(m_sFactionKey, worldTime))
	        {
	            //Print(string.Format("[%1] Objective %2 — menunggu recon tiba",
	                //m_sCommanderUID, obj.GetOwner().GetName()));
	            return;
	        }
	
	        //Print(string.Format("[%1] Recon tiba — kirim assault ke %2",
	            //m_sCommanderUID, obj.GetOwner().GetName()));
	        TrySendAssaultWithSlots(obj, worldTime);
	    }
	}
 
	protected void TrySendRecon(CMD_AICommanderObjectiveComponent obj)
	{
		DCO_GroupUtilityComponent reconGrp = FindBestIdleGroupForRole(CMD_EGroupRole.RECON);
		if (!reconGrp)
		{
			return;
		}
		
		if (reconGrp.IsPlayerGroup())
		{
			//CMD_TaskNotifier.Notify(reconGrp.GetOwner(), "Recon " + obj.GetOwner().GetName(), obj.GetOwner().GetOrigin(), CMD_ETaskType.RECON);
			return;
		}
		
		float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
		
		RandomGenerator rand = new RandomGenerator();
		vector objPos    = rand.GenerateRandomPointInRadius(obj.GetRadius() / 5, obj.GetRadius(), obj.GetOwner().GetOrigin(), false);
		objPos[1]		 = GetGame().GetWorld().GetSurfaceY(objPos[0], objPos[2]);
				
	    vector reconPos = CMD_ReconSpotFinder.FindBestReconSpot(reconGrp.GetOwner().GetOrigin(), objPos, 300.0, 80.0, 16);
		if (reconPos == vector.Zero)
			return;
		
		if (TryAssignTransport(reconGrp, reconPos, worldTime))
    		return;
 
		SCR_AIWaypoint wp = SpawnMoveWP(reconPos);
		if (!wp)
			return;
 
		
		reconGrp.SetGroupRole(CMD_EGroupRole.RECON);
		reconGrp.MoveTo(wp, worldTime);
 		obj.SetReconGroup(m_sFactionKey, reconGrp);
		obj.MarkAssigned(m_sFactionKey);
 
		//Print(string.Format("[%1] RECON: %2 → %3",
			//m_sCommanderUID, reconGrp.GetOwner().GetName(), obj.GetOwner().GetName()));
	}
 
	protected void SendIdleGroupsToReserve()
	{
		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp)
				continue;
 
			if (grp.GetGroupStatus() == DCOG_EGroupStatus.EXECUTING_COMMAND)
				continue;
 
			if (grp.GetGroupRole() == CMD_EGroupRole.NONE)
			{
				SCR_AIWaypoint wp = SpawnMoveWP(GetOwner().GetOrigin());
				if (wp)
				{
					float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
					grp.SetGroupRole(CMD_EGroupRole.RESERVE);
					grp.MoveTo(wp, worldTime);
				}
			}
		}
	}
	
	protected DCO_GroupUtilityComponent FindBestIdleGroupForRole(CMD_EGroupRole role)
	{
		DCO_GroupUtilityComponent result = null;
	
		result = FindIdleGroupByCurrentRole(role, role);
		if (result)
			return result;
	
		result = FindIdleGroupByCurrentRole(role, CMD_EGroupRole.NONE);
		if (result)
			return result;
	
		result = FindIdleGroupByCurrentRole(role, CMD_EGroupRole.RESERVE);
		if (result)
			return result;
		
		result = FindIdleGroupByCurrentRole(role, CMD_EGroupRole.RECON);
		if (result)
			return result;
	
		result = FindIdleGroupByCurrentRole(role, CMD_EGroupRole.REINFORNCE);
		return result;
	}
	
	protected DCO_GroupUtilityComponent FindIdleGroupByCurrentRole(CMD_EGroupRole targetRole, CMD_EGroupRole currentRole)
	{
		DCO_GroupUtilityComponent best = null;
		float bestScore = -1.0;
	
		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp)
				continue;
	
			if (grp.GetGroupStatus() == DCOG_EGroupStatus.EXECUTING_COMMAND)
				continue;
	
			if (grp.GetGroupRole() != currentRole)
				continue;
			
			if (grp.IsDedicatedTransport())
				continue;
	
			int unitCount = grp.GetUnitCount();
	
			/*
			switch (targetRole)
			{
				case CMD_EGroupRole.RECON:
					if (unitCount < 1 || unitCount > 5)
						continue;
					break;
				case CMD_EGroupRole.ASSAULT:
					if (unitCount < 6)
						continue;
					break;
				case CMD_EGroupRole.FLANK:
					if (unitCount < 5)
						continue;
					break;
				default:
					break;
			}*/
	
			float score = 0.0;
			float strengthPct = Math.Clamp(unitCount / 12.0 * 100.0, 0.0, 100.0);
	
			switch (targetRole)
			{
				case CMD_EGroupRole.RECON:
					score = 100.0 - strengthPct;
					break;
				case CMD_EGroupRole.ASSAULT:
					score = strengthPct;
					break;
				case CMD_EGroupRole.FLANK:
					score = 100.0 - Math.AbsFloat(strengthPct - 50.0);
					break;
				default:
					score = strengthPct;
					break;
			}
	
			if (score > bestScore)
			{
				bestScore = score;
				best = grp;
			}
		}
	
		return best;
	}
	
	protected void Think(float worldTime)
	{
		if (!Replication.IsServer())
			return;
	
		m_eCommanderState = CMD_ECommanderState.COMMANDING;
	
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;
	
		mgr.GetTopObjectives(this, m_fObjectiveAtTheSameTime, m_aObjective);
	
		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp)
				continue;
	
			if (grp.GetUnitCount() <= m_iRetreatThreshold
				&& grp.GetGroupStatus() != DCOG_EGroupStatus.IDLE)
			{
				
				RandomGenerator rand = new RandomGenerator();
 
				vector centerGround    	 = rand.GenerateRandomPointInRadius(0, m_fBaseRadius, GetOwner().GetOrigin(), false);
				centerGround[1]		 = GetGame().GetWorld().GetSurfaceY(centerGround[0], centerGround[2]);
				
				SCR_AIWaypoint wp = SpawnMoveWP(centerGround);
				
				grp.MoveTo(wp, worldTime);
			}
	
			if (!grp.CheckOrderComplete(worldTime))
				continue;
		}
	
		if (m_aObjective.IsEmpty())
		{
			SendIdleGroupsToReserve();
			m_eCommanderState = CMD_ECommanderState.IDLE;
			return;
		}
	
		for (int i = 0; i < m_aObjective.Count(); i++)
		{
			CMD_AICommanderObjectiveComponent obj = m_aObjective[i];
			if (!obj)
				continue;
	
			CMD_EObjectiveState state = obj.GetObjectiveState(m_sFactionKey);
			//Print(typename.EnumToString(CMD_EObjectiveState, state) + " < STATUS | FK > " + m_sFactionKey);
	
			AssignRolesToObjective(obj, worldTime);
		}
	
		m_eCommanderState = CMD_ECommanderState.PLANNING;
	}
	
	protected void TrySendToStaging(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
	    vector objPos  = obj.GetOwner().GetOrigin();
	    vector base    = GetOwner().GetOrigin();
	    vector axis    = objPos - base;
	    axis           = Vector(axis[0], 0.0, axis[2]);
	    axis           = axis.Normalized();
	    float dist     = vector.Distance(base, objPos);
	
	    vector stagingPos   = base + axis * (dist * Math.RandomFloatInclusive(0.15, 0.4));
	    stagingPos[1]       = GetGame().GetWorld().GetSurfaceY(stagingPos[0], stagingPos[2]);
	
	    DCO_GroupUtilityComponent assaultGrp = FindBestIdleGroupForRole(CMD_EGroupRole.ASSAULT);
	    if (assaultGrp)
	    {
			if (assaultGrp.IsPlayerGroup())
			{
				//CMD_TaskNotifier.Notify(assaultGrp.GetOwner(), "STAGING " + obj.GetOwner().GetName(), obj.GetOwner().GetOrigin(), CMD_ETaskType.MOVE);
				return;
			}
			if (TryAssignTransport(assaultGrp, stagingPos, worldTime))
    			return;
	        SCR_AIWaypoint wp = SpawnMoveWP(stagingPos);
	        if (wp)
	        {
	            assaultGrp.SetGroupRole(CMD_EGroupRole.ASSAULT);
	            assaultGrp.MoveTo(wp, worldTime);
	            //Print(string.Format("[%1] ASSAULT → STAGING: %2", m_sCommanderUID, assaultGrp.GetOwner().GetName()));
	        }
	    }
	
	    DCO_GroupUtilityComponent flankGrp = FindBestIdleGroupForRole(CMD_EGroupRole.FLANK);
	    if (flankGrp)
	    {
			if (flankGrp.IsPlayerGroup())
			{
				//CMD_TaskNotifier.Notify(flankGrp.GetOwner(), "STAGING " + obj.GetOwner().GetName(), obj.GetOwner().GetOrigin(), CMD_ETaskType.MOVE);
				return;
			}
			if (TryAssignTransport(flankGrp, stagingPos, worldTime))
    			return;
	        vector flankStaging = ComputeFlankPosition(base, stagingPos, 150);
	        SCR_AIWaypoint wp   = SpawnMoveWP(flankStaging);
	        if (wp)
	        {
	            flankGrp.SetGroupRole(CMD_EGroupRole.FLANK);
	            flankGrp.MoveTo(wp, worldTime);
	            //Print(string.Format("[%1] FLANK → STAGING: %2", m_sCommanderUID, flankGrp.GetOwner().GetName()));
	        }
	    }
	}
	
	protected void GenerateSearchWaypoints(vector center, float radius, array<SCR_AIWaypoint> outWaypoints,	int rings = 3, int sectorsPerRing = 5)
	{
		if (!outWaypoints)
			return;
	 
		outWaypoints.Clear();
	 
		vector centerGround = Vector(center[0], GetGame().GetWorld().GetSurfaceY(center[0], center[2]), center[2]);
		RandomGenerator rand = new RandomGenerator();
 
		centerGround    	 = rand.GenerateRandomPointInRadius(0, radius, center, false);
		centerGround[1]		 = GetGame().GetWorld().GetSurfaceY(centerGround[0], centerGround[2]);
		SCR_AIWaypoint wpCenter = SpawnMoveWP(centerGround);
		if (wpCenter)
			outWaypoints.Insert(wpCenter);
	 
		for (int ring = 1; ring <= rings; ring++)
		{
			// Setiap ring punya band dari radiusInner ke radiusOuter
			float ringStep    = radius / rings;
			float radiusInner = ringStep * (ring - 1);
			float radiusOuter = ringStep * ring;
	 
			float sectorAngle = 360.0 / sectorsPerRing;
	 
			for (int sector = 0; sector < sectorsPerRing; sector++)
			{
				float angleMin = sectorAngle * sector;
				float angleMax = sectorAngle * (sector + 1);
				float angleDeg = Math.RandomFloat(angleMin, angleMax);
				float angleRad = angleDeg * Math.DEG2RAD;
	 
				float dist = Math.RandomFloat(radiusInner + 1.0, radiusOuter);
	 
				float px = center[0] + Math.Cos(angleRad) * dist;
				float pz = center[2] + Math.Sin(angleRad) * dist;
				float py = GetGame().GetWorld().GetSurfaceY(px, pz);
	 
				SCR_AIWaypoint wp = SpawnMoveWP(Vector(px, py, pz));
				if (wp)
				{
					outWaypoints.Insert(wp);
				}
			}
		}
	}
	
	SCR_AIWaypoint SpawnMoveWP(vector pos)
	{
		AICommander_BaseComponentClass data = AICommander_BaseComponentClass.Cast(GetComponentData(GetOwner()));
		if (!data)
			return null;
 
		Resource res = Resource.Load(data.GetDefaultMoveWaypointPrefab());
		if (!res || !res.IsValid())
			return null;
 
		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = pos;
 
		return SCR_AIWaypoint.Cast(GetGame().SpawnEntityPrefab(res, null, params));
	}
 
	SCR_AIWaypoint SpawnDefendWP(vector pos)
	{
		AICommander_BaseComponentClass data = AICommander_BaseComponentClass.Cast(GetComponentData(GetOwner()));
		if (!data)
			return null;
 
		Resource res = Resource.Load(data.GetDefaultDefendWaypointPrefab());
		if (!res || !res.IsValid())
			return null;
 
		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = pos;
 
		return SCR_AIWaypoint.Cast(GetGame().SpawnEntityPrefab(res, null, params));
	}
	
	protected void TrySendAssaultWithSlots(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		if (obj.IsGroupSlotFull(m_sFactionKey))
		{
			//Print(string.Format("[%1] [%5] Objective %2 slot penuh (%3 required) Terisi : %4",
				//m_sCommanderUID, obj.GetOwner().GetName(), obj.GetRequiredGroupCount(), obj.GetCurrentAssignedGroupCount(m_sFactionKey), m_sFactionKey));
			return;
		}
		
		RandomGenerator rand = new RandomGenerator();
 
		vector objPos    = rand.GenerateRandomPointInRadius(5, obj.GetRadius(), obj.GetOwner().GetOrigin(), false);
		objPos[1]		 = GetGame().GetWorld().GetSurfaceY(objPos[0], objPos[2]);
		
		float objRad	 = obj.GetRadius();
		int required     = obj.GetRequiredGroupCount();
		int slotsLeft    = required - obj.GetCurrentAssignedGroupCount(m_sFactionKey);
 
		if (slotsLeft > 0)
		{
			DCO_GroupUtilityComponent assaultGrp = FindBestIdleGroupForRole(CMD_EGroupRole.ASSAULT);
			if (assaultGrp)
			{
				if (assaultGrp.IsPlayerGroup())
				{
					//CMD_TaskNotifier.Notify(assaultGrp.GetOwner(), "ASSAULT " + obj.GetOwner().GetName(), obj.GetOwner().GetOrigin(), CMD_ETaskType.CAPTURE);
					return;
				}
				if (TryAssignTransport(assaultGrp, objPos, worldTime))
    				return;
				array<SCR_AIWaypoint> searchWPs = {};
				GenerateSearchWaypoints(obj.GetOwner().GetOrigin(), obj.GetRadius(), searchWPs);
				if (searchWPs.Count() > 0)
				{
					foreach (SCR_AIWaypoint wp : searchWPs)
    					assaultGrp.MoveTo(wp, worldTime);
					
					assaultGrp.SetGroupRole(CMD_EGroupRole.ASSAULT);
					if (assaultGrp.GetGroupObjective() != obj)
					{
						assaultGrp.SetGroupObjective(obj);
						obj.SetObjectiveGroup(m_sFactionKey, 1);
						slotsLeft = slotsLeft - 1;
					}
					
					objPos    		 = rand.GenerateRandomPointInRadius(5, obj.GetRadius(), obj.GetOwner().GetOrigin(), false);
					objPos[1]		 = GetGame().GetWorld().GetSurfaceY(objPos[0], objPos[2]);

					/*Print(string.Format("[%1] ASSAULT (slotted require %3): %2 → %4 Slots Left %6",
						m_sCommanderUID,
						obj.GetCurrentAssignedGroupCount(m_sFactionKey),
						required,
						assaultGrp.GetOwner().GetName(),
						obj.GetOwner().GetName(),
						slotsLeft));*/
				}
			}
		}
 
		if (slotsLeft > 0)
		{
			DCO_GroupUtilityComponent flankGrp = FindBestIdleGroupForRole(CMD_EGroupRole.FLANK);
			if (flankGrp)
			{
				if (flankGrp.IsPlayerGroup())
				{
					//CMD_TaskNotifier.Notify(flankGrp.GetOwner(), "FLANK " + obj.GetOwner().GetName(), obj.GetOwner().GetOrigin(), CMD_ETaskType.DESTROY);
					return;
				}
				if (TryAssignTransport(flankGrp, objPos, worldTime))
    				return;
				
				for(int wpNum = 1; wpNum < 4; wpNum++)
				{
				    float axisRatio = 0.40 + (wpNum - 1) * 0.25;
				
				    float sideOffset = 80.0 - (wpNum - 1) * 25.0;
				
				    vector axis    = objPos - GetOwner().GetOrigin();
				    axis           = Vector(axis[0], 0.0, axis[2]);
				    axis           = axis.Normalized();
				    float totalDist = vector.Distance(GetOwner().GetOrigin(), objPos);
				
				    vector alongPoint = GetOwner().GetOrigin() + axis * (totalDist * axisRatio);
				    vector flankPos   = ComputeFlankPosition(alongPoint, objPos, sideOffset);
				
				    SCR_AIWaypoint wp = SpawnMoveWP(flankPos);
				    if (wp)
				    {
				        flankGrp.SetGroupRole(CMD_EGroupRole.FLANK);
				        flankGrp.MoveTo(wp, worldTime);
				
				        /*Print(string.Format("[%1] FLANK WP %2/3 (ratio: %3 offset: %4m): %5",
				            m_sCommanderUID,
				            wpNum,
				            axisRatio.ToString(),
				            sideOffset.ToString(),
				            flankGrp.GetOwner().GetName()));*/
				    }
				}
				
				SCR_AIWaypoint Objwp = SpawnMoveWP(objPos);
				flankGrp.MoveTo(Objwp, worldTime);
				
				if (flankGrp.GetGroupObjective() != obj)
				{
					flankGrp.SetGroupObjective(obj);
					obj.SetObjectiveGroup(m_sFactionKey, 1);
					slotsLeft = slotsLeft - 1;
				}
			}
		}
	}

	protected void TrySendDefend(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		int defendNeeded = obj.GetDefendGroupCount();
		int current      = obj.GetCurrentAssignedGroupCount(m_sFactionKey);
		int toSend       = defendNeeded - current;
 
		if (toSend <= 0)
			return;
		
		RandomGenerator rand = new RandomGenerator();
 
		vector objPos 	 = rand.GenerateRandomPointInRadius(5, obj.GetRadius(), obj.GetOwner().GetOrigin(), true);
 		objPos[1]		 = GetGame().GetWorld().GetSurfaceY(objPos[0], objPos[2]);
		
		for (int i = 0; i < toSend; i++)
		{
			DCO_GroupUtilityComponent defGrp = FindBestIdleGroupForRole(CMD_EGroupRole.RESERVE);
			if (!defGrp)
				break;
			
			if (defGrp.IsPlayerGroup())
			{
				//CMD_TaskNotifier.Notify(defGrp.GetOwner(), "DEFEND " + obj.GetOwner().GetName(), obj.GetOwner().GetOrigin(), CMD_ETaskType.DEFEND);
				return;
			}
 
			SCR_AIWaypoint wp = SpawnDefendWP(objPos);
			if (!wp)
				break;
 
			defGrp.SetGroupRole(CMD_EGroupRole.RESERVE);
			defGrp.MoveTo(wp, worldTime);
			
			if (defGrp.GetGroupObjective() != obj)
			{
				defGrp.SetGroupObjective(obj);
				obj.SetObjectiveGroup(m_sFactionKey, 1);
			}
				
 
			/*Print(string.Format("[%1] DEFEND (post-capture %2/%3): %4 → %5",
				m_sCommanderUID,
				obj.GetCurrentAssignedGroupCount(m_sFactionKey),
				defendNeeded,
				defGrp.GetOwner().GetName(),
				obj.GetOwner().GetName()));*/
		}
	}
	
	protected void ThinkCaptureProgress(float worldTime)
	{
		foreach (CMD_AICommanderObjectiveComponent obj : m_aObjective)
		{
			if (!obj)
				continue;
 
			CMD_EObjectiveState state = obj.GetObjectiveState(m_sFactionKey);
			
			if (obj.CountNearbyUnits(obj.GetRadius(), m_sFactionKey, true) < obj.CountNearbyUnits(obj.GetRadius(), m_sFactionKey, false))
				obj.SetObjectiveState(m_sFactionKey, CMD_EObjectiveState.ASSIGNED);
 
			if (state == CMD_EObjectiveState.COMPLETED || state == CMD_EObjectiveState.FAILED)
				continue;
 
			if (obj.IsCapturedBy(m_sFactionKey))
			{
				TrySendDefend(obj, worldTime);
				continue;
			}
 
			if (state != CMD_EObjectiveState.ASSIGNED)
				continue;
 
			if (!obj.IsCaptureTimerRunning(m_sFactionKey))
			{
				if (obj.GetCurrentAssignedGroupCount(m_sFactionKey) > 0)
				{
					if (obj.CountNearbyUnits(obj.GetRadius(), m_sFactionKey, true) > 0)
						obj.StartCaptureTimer(m_sFactionKey, worldTime);
				}
					
				continue;
			}
 
			float progress = obj.GetCaptureProgress(m_sFactionKey, worldTime);
			/*Print(string.Format("[%1] Capture progress %2: %3%",
				m_sCommanderUID,
				obj.GetOwner().GetName(),
				(progress).ToString()));*/
 
			if (obj.IsCaptureTimerComplete(m_sFactionKey, worldTime))
			{
				obj.SetCapturedBy(m_sFactionKey, true);
				obj.ResetAssignedGroupCount(m_sFactionKey);
 
				/*Print(string.Format("[%1] CAPTURED: %2 — transitioning to DEFEND (%3 groups)",
					m_sCommanderUID,
					obj.GetOwner().GetName(),
					obj.GetDefendGroupCount()));*/
 
				TrySendDefend(obj, worldTime);
			}
		}
	}
	
	protected IEntity TryAssignTransports(DCO_GroupUtilityComponent passengerGroup)
	{
		if (!passengerGroup)
			return null;
	 
		int unitCount = passengerGroup.GetUnitCount();
		if (unitCount <= 0)
			return null;
	 
		vector groupPos = passengerGroup.GetOwner().GetOrigin();
		IEntity vehicle = null;
		
		for(int i = 0; i < m_aVehicle.Count(); i++)
		{
			vehicle = CMD_VehicleFinder.FindNearestVehicle(m_aVehicle[i], groupPos, unitCount);
			if (vehicle) break;
		}
	 
		if (!vehicle)
		{
			return null;
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
			Print(string.Format("[%1] Vehicle %2 tidak punya DCO_TransportMissionComponent — skip",
				m_sCommanderUID, vehicle.GetName()), LogLevel.WARNING);
			return;
		}
	 
		if (mission.IsActiveVehicle())
		{
			Print(string.Format("[%1] Vehicle %2 sudah dipakai transport lain",
				m_sCommanderUID, vehicle.GetName()));
			return;
		}
	 
		passengerGroup.SetGroupRole(CMD_EGroupRole.TRANSPORT);
		
		SCR_AIGroup grp = SCR_AIGroup.Cast(passengerGroup.GetOwner());
		if (!grp)
			return;
		
		grp.CompleteAllWaypoints();		
	 
		// Kirim group jalan kaki ke vehicle dulu
		SCR_AIWaypoint wpToVehicle = SpawnMoveWP(vehicle.GetOrigin());
		if (wpToVehicle)
			passengerGroup.MoveTo(wpToVehicle, worldTime);
	 
		// Pass `this` supaya mission bisa spawn GetIn/GetOut WP
		mission.StartMission(passengerGroup, destination, m_sFactionKey, worldTime, this);
	 
		Print(string.Format("[%1] TRANSPORT assigned | group: %2 | vehicle: %3 | dest: %4",
			m_sCommanderUID,
			passengerGroup.GetOwner().GetName(),
			vehicle.GetName(),
			destination.ToString()));
	}
	
	SCR_AIWaypoint SpawnGetInWP(vector pos)
	{
		AICommander_BaseComponentClass data = AICommander_BaseComponentClass.Cast(GetComponentData(GetOwner()));
		if (!data)
			return null;
	 
		Resource res = Resource.Load(data.GetDefaultGetInWaypointPrefab());
		if (!res || !res.IsValid())
			return null;
	 
		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = pos;
	 
		return SCR_AIWaypoint.Cast(GetGame().SpawnEntityPrefab(res, null, params));
	}
	 
	SCR_AIWaypoint SpawnGetOutWP(vector pos)
	{
		AICommander_BaseComponentClass data = AICommander_BaseComponentClass.Cast(GetComponentData(GetOwner()));
		if (!data)
			return null;
	 
		Resource res = Resource.Load(data.GetDefaultGetOutWaypointPrefab());
		if (!res || !res.IsValid())
			return null;
	 
		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = pos;
	 
		return SCR_AIWaypoint.Cast(GetGame().SpawnEntityPrefab(res, null, params));
	}
	
	DCO_GroupUtilityComponent FindBestIdleGroupForRole_Public(CMD_EGroupRole role)
	{
	    return FindBestIdleGroupForRole(role);
	}
	
	void RegisterTransportTeam(DCO_TransportTeamComponent team)
	{
		if (!team || m_aTransportTeams.Contains(team))
			return;
	 
		team.SetRallyPoint(GetOwner().GetOrigin());
		m_aTransportTeams.Insert(team);
	 
		Print(string.Format("[%1] Transport team registered: %2",
			m_sCommanderUID, team.GetOwner().GetName()));
	}
	
	bool TryAssignTransport(DCO_GroupUtilityComponent passengerGroup, vector destination, float worldTime)
	{
		if (!passengerGroup)
			return false;
	 
		float dist = vector.Distance(passengerGroup.GetOwner().GetOrigin(), destination);
		if (dist < m_fTransportDistanceThreshold)
			return false;
	 
		int unitCount = passengerGroup.GetUnitCount();
		if (unitCount <= 0)
			return false;
	 
		// Dedicated transport team dulu
		/*DCO_TransportTeamComponent team = GetAvailableTransportTeam(passengerGroup.GetOwner().GetOrigin(), worldTime);
		if (team)
		{
			team.AssignJob(passengerGroup, destination, this, worldTime);
			Print(string.Format("[%1] TRANSPORT via dedicated team: %2 carrying %3",
				m_sCommanderUID,
				team.GetOwner().GetName(),
				passengerGroup.GetOwner().GetName()));
			return true;
		}*/
	 
		// Fallback: cari vehicle biasa
		vector groupPos = passengerGroup.GetOwner().GetOrigin();
		IEntity vehicle = TryAssignTransports(passengerGroup);
	 
		if (!vehicle)
        	return false;
		
		BeginTransportMission(passengerGroup, vehicle, destination, worldTime);
		return true;
	}
	 
	DCO_TransportTeamComponent GetAvailableTransportTeam(vector TakeAt, float wt)
	{
		IEntity vehicle = null;
		DCO_TransportTeamComponent t = null;
		foreach (DCO_TransportTeamComponent team : m_aTransportTeams)
		{
			if (team && team.IsAvailable())
			{
				vehicle = TryAssignTransports(team.GetDCOGroupUtility());
				t = team;
				break;
			}
		}
		DCO_TransportMissionComponent mission = DCO_TransportMissionComponent.Cast(vehicle.FindComponent(DCO_TransportMissionComponent));
	 
		if (!mission)
		{
			Print(string.Format("[%1] Vehicle %2 tidak punya DCO_TransportMissionComponent — skip",
				m_sCommanderUID, vehicle.GetName()), LogLevel.WARNING);
			return null;
		}
	 
		if (mission.IsActiveVehicle())
		{
			Print(string.Format("[%1] Vehicle %2 sudah dipakai transport lain",
				m_sCommanderUID, vehicle.GetName()));
			return null;
		}
		

		
		SCR_AIWaypoint wpToVehicle = SpawnMoveWP(vehicle.GetOrigin());
		if (wpToVehicle)
			t.GetDCOGroupUtility().MoveTo(wpToVehicle, wt);
		
		mission.StartMission(t.GetDCOGroupUtility(), TakeAt, m_sFactionKey, wt, this);
		
		return null;
	}
	
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if (!Replication.IsServer())
			return;
 
		m_fThinkTimer += timeSlice;
		 m_fCaptureCheckTimer += timeSlice;

		if (m_fThinkTimer >= m_fThinkInterval)
		{
			m_fThinkTimer = 0.0;
			float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
			Think(worldTime);
		}
		
		if (m_fCaptureCheckTimer >= m_fCaptureCheckInterval)
		{
		    m_fCaptureCheckTimer = 0.0;
		    ThinkCaptureProgress(GetGame().GetWorld().GetWorldTime() / 1000.0);
		}
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
		InitializeCommander();
	}
}