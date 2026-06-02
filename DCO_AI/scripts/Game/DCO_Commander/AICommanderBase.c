[ComponentEditorProps(category: "GameScripted/Commander")]
class AICommander_BaseComponentClass : ScriptComponentClass
{
	// TO-DO GANTI SEMUA JADI YANG GA EDITABLE
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
	
	[Attribute("{2602CAB8AB74FBBF}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_GetOut.et", UIWidgets.ResourceNamePicker, desc: "Tasking For Player.", "et", category: "Commander Player Tasking Setting")]
	protected ResourceName m_sDefaultTaskPlayerMovePrefab;
	
	[Attribute("{6ED320498A60081C}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_ArtillerySupport.et", UIWidgets.ResourceNamePicker, desc: "Tasking For Player.", "et", category: "Commander Player Tasking Setting")]
	protected ResourceName m_sDefaultShootArtilleryPrefab;
	
	[Attribute("{6ED320498A60081C}PrefabsEditable/Auto/AI/Waypoints/E_AIWaypoint_ArtillerySupport.et", UIWidgets.ResourceNamePicker, desc: "Waypoint To Suppress Area.", "et", category: "Commander Player Tasking Setting")]
	protected ResourceName m_sDefaultSuppressPrefab;

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
	
	ResourceName GetDefaultMoveTaskPrefab()
	{
		return m_sDefaultTaskPlayerMovePrefab;
	}
	
	ResourceName GetShootArtilleryWaypointPrefab()
	{
		return m_sDefaultShootArtilleryPrefab;
	}
	
	ResourceName GetDefaultSuppressPrefab()
	{
		return m_sDefaultSuppressPrefab;
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
	
	[Attribute("30.0", UIWidgets.EditBox, "Interval think cycle dalam detik.", category: "Commander Setting")]
	protected float m_fThinkInterval;
	
	[Attribute("60.0", UIWidgets.EditBox, "Delay Before Start First Iteration", category: "Commander Setting")]
	protected float m_fDelayFirstIteration;
	
	[Attribute("120.0", UIWidgets.EditBox, "Cooldown (detik) sebelum stalemate response di-trigger lagi per objective", category: "Commander Setting")]
	protected float m_fStalemateResponseCooldown;
 
	[Attribute("2", UIWidgets.EditBox, "Unit count minimum group sebelum dipaksa retreat.", category: "Commander Setting")]
	protected int m_iRetreatThreshold;
	
	[Attribute("15.0", UIWidgets.EditBox, "Interval cek capture progress (detik)", category: "Commander Setting")]
	protected float m_fCaptureCheckInterval;
	
	[Attribute("400.0", UIWidgets.EditBox, "Jarak minimum sebelum cari transport", category: "Commander Setting")]
	protected float m_fTransportDistanceThreshold;
	
	[Attribute("50.0", UIWidgets.EditBox, "Radius pencarian kendaraan", category: "Commander Setting")]
	protected float m_fVehicleSearchRadius;
	
	[Attribute("50.0", UIWidgets.EditBox, "Radius Close To Commander", category: "Commander Setting")]
	protected float m_fBaseRadius;
	
	[Attribute("0.6", UIWidgets.Range, "Defend Chances Instead Patrol Around", params: "0 1 0.01", category: "Commander Setting")]
	protected float m_fDefendChance;
	
	[Attribute("2", UIWidgets.ComboBox, "Commander Mode", "", ParamEnumArray.FromEnum(CMD_ECommanderMode), category: "Commander Personality" )]
	protected CMD_ECommanderMode m_eCommanderModeExternal;
	
	[Attribute("0.5", UIWidgets.Range, "Agresivitas: seberapa cepat commit assault tanpa tunggu recon.\n0 = tunggu recon tiba dulu | 1 = langsung serang tanpa recon", params: "0 1 0.01", category: "Commander Personality")]
	protected float m_fAggression;
	
	[Attribute("0.5", UIWidgets.Range, "Adaptabilitas: kecepatan switching mode dan reaktivitas commander.\n0 = lambat bereaksi | 1 = sangat responsif", params: "0 1 0.01", category: "Commander Personality")]
	protected float m_fAdaptability;
	
    [Attribute("false", UIWidgets.CheckBox, desc: "Random Personality Every Playthough?", category: "Commander Personality")]
    bool m_bRandomPersonality;
 
	protected float m_fCaptureCheckTimer = 0.0;
	
	protected float m_fDefensiveTriggerCooldown = 0.0;
	static float DEFENSIVE_COOLDOWN = 180.0;
	
	protected ref array<CMD_AICommanderObjectiveComponent> m_aObjective = {};
	
	protected CMD_ECommanderState m_eCommanderState = CMD_ECommanderState.IDLE;
	protected CMD_ECommanderMode m_eCommanderMode = CMD_ECommanderMode.OFFENSIVE;
	
	protected CMD_ThreatResponseComponent threatComp;
	protected CMD_ArtillerySupport		  artySupport;
	
	protected ref array<DCO_GroupUtilityComponent> m_aOwnedGroup = {};
	protected ref array<IEntity> m_aVehicle = {};
	
	protected ref array<DCO_TransportTeamComponent> m_aTransportTeams = {};
	
	protected IEntity m_MyEnt;
	
	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mStalemateResponseTime = new map<CMD_AICommanderObjectiveComponent, float>();
	
	protected float m_fThinkTimer = 0 - m_fDelayFirstIteration;
	
	protected const float m_fFlankAngleMin = 15;
	protected const float m_fFlankAngleMax = 315;
	
	int GetOwnedGroupCount() { return m_aOwnedGroup.Count(); }
	int GetOwnedVehicle()	{return m_aVehicle.Count(); }
	
	bool send = true;
	
	bool RegisterGroup(DCO_GroupUtilityComponent grp)
	{
		
		if (grp.GetGroupRole() == CMD_EGroupRole.ARTILLERY)
		{
			artySupport.RegisterArtilleryGroup(grp);
			return true;
		}
		
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
		if (!AICommander_ManagerComponent.GetInstance()) return;
		AICommander_ManagerComponent.GetInstance().RegisterCommander(this);
		threatComp = CMD_ThreatResponseComponent.Cast(m_MyEnt.FindComponent(CMD_ThreatResponseComponent));
		if (m_bRandomPersonality)
		{
			m_fAggression = Math.RandomFloat01();
			m_fAdaptability = Math.RandomFloat01();
		}
		float adaptMod   = Math.Lerp(1.5, 0.5, m_fAdaptability);
		m_fThinkInterval = m_fThinkInterval * adaptMod;
		artySupport = CMD_ArtillerySupport.Cast(m_MyEnt.FindComponent(CMD_ArtillerySupport));
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
	
	void SwitchToDefensive(float worldTime)
	{
		m_eCommanderMode            = CMD_ECommanderMode.DEFENSIVE;
		m_fDefensiveTriggerCooldown = worldTime;
	 
		//Print(string.Format("[%1] SWITCHING TO DEFENSIVE MODE", m_sCommanderUID));
	}
	 
	void SwitchToOffensive()
	{
		m_eCommanderMode = CMD_ECommanderMode.OFFENSIVE;
	 
		//Print(string.Format("[%1] SWITCHING TO OFFENSIVE MODE", m_sCommanderUID));
	}
	 
	// Manual override dari luar (misalnya game mode bisa force defensive)
	void ForceDefensiveMode(float worldTime)   { SwitchToDefensive(worldTime); }
	void ForceOffensiveMode()                  { SwitchToOffensive(); }
	 
	CMD_ECommanderMode GetCommanderMode()      { return m_eCommanderMode; }
	
	vector ComputeFlankPosition(vector base, vector objective, float distance)
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
	    	if (m_fAggression >= Math.RandomFloat01())
	    	{
	    		TrySendToStaging(obj, worldTime);
	    		obj.MarkAssigned(m_sFactionKey);
	    		return;
	    	}
	        TrySendRecon(obj);
	        TrySendToStaging(obj, worldTime);
	        return;
	    }
	
	    if (objState == CMD_EObjectiveState.ASSIGNED)
	    {
	        if (m_fAggression < Math.RandomFloat01() && !obj.IsReconArrived(m_sFactionKey, worldTime))
	        {
	            //Print(string.Format("[%1] LOW AGGRESSION — waiting for recon at %2",
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
		DCO_GroupUtilityComponent reconGrp = FindBestIdleGroupForRole(CMD_EGroupRole.RECON, obj.GetOwner().GetOrigin());
		if (!reconGrp)
		{
			return;
		}
		
		if (reconGrp.IsPlayerGroup())
		{
			
			return;
		}
		
		reconGrp.CompleteAllWaypoints();
		
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
	
	        AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
	        if (!mgr)
	            return;
	
	        if (grp.GetGroupStatus() == DCOG_EGroupStatus.EXECUTING_COMMAND)
	            continue;
	
	        if (grp.GetGroupRole() != CMD_EGroupRole.NONE)
	            continue;
	
	        float worldTime = GetGame().GetWorld().GetWorldTime() / 1000.0;
	        RandomGenerator rand = new RandomGenerator();
	
	        array<vector> candidatePositions = new array<vector>();
	
	        array<CMD_AICommanderObjectiveComponent> allObjs = {};
	        mgr.GetTopObjectives(this, mgr.m_aObjective.Count(), allObjs);
	
	        foreach (CMD_AICommanderObjectiveComponent obj : allObjs)
	        {
	            if (!obj)
	                continue;
	
	            if (!obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
	                continue;
	
	            candidatePositions.Insert(obj.GetOwner().GetOrigin());
	        }
	
	        if (candidatePositions.IsEmpty() || candidatePositions.Count() < 2)
	            candidatePositions.Insert(GetOwner().GetOrigin());
	
	        for (int i = candidatePositions.Count() - 1; i > 0; i--)
	        {
	            int j = Math.RandomInt(0, i + 1);
	            vector tmp = candidatePositions[i];
	            candidatePositions[i] = candidatePositions[j];
	            candidatePositions[j] = tmp;
	        }
			
			grp.CompleteAllWaypoints();
	
	        foreach (vector basePos : candidatePositions)
	        {
	            vector patrolPos = rand.GenerateRandomPointInRadius(0, m_fBaseRadius, basePos, false);
	            patrolPos[1] = GetGame().GetWorld().GetSurfaceY(patrolPos[0], patrolPos[2]);
	
	            SCR_AIWaypoint wp = SpawnMoveWP(patrolPos);
	            if (wp)
	                grp.MoveTo(wp, worldTime);	            
	        }
	
	        grp.SetGroupRole(CMD_EGroupRole.RESERVE);
	    }
	}
	
	protected DCO_GroupUtilityComponent FindBestIdleGroupForRole(CMD_EGroupRole role, vector targetPos)
	{
		DCO_GroupUtilityComponent result = null;
	
		result = FindIdleGroupByCurrentRole(role, role, targetPos);
		if (result)
			return result;
		
		if (role == CMD_EGroupRole.ARMORED)
			return result;
	
		result = FindIdleGroupByCurrentRole(role, CMD_EGroupRole.NONE, targetPos);
		if (result)
			return result;
	
		result = FindIdleGroupByCurrentRole(role, CMD_EGroupRole.RESERVE, targetPos);
		if (result)
			return result;
		
		result = FindIdleGroupByCurrentRole(role, CMD_EGroupRole.RECON, targetPos);
		if (result)
			return result;
		
		result = FindIdleGroupByCurrentRole(role, CMD_EGroupRole.REINFORNCE, targetPos);
		if (result)
			return result;
	
		result = FindIdleGroupByCurrentRole(role, CMD_EGroupRole.DEFEND, targetPos);
		return result;
	}
	
	DCO_GroupUtilityComponent FindClosestIdleGroupForRole_Public(CMD_EGroupRole role, vector targetPos)
	{
	    DCO_GroupUtilityComponent result = null;
	
	    result = FindIdleGroupByCurrentRole(role, role, targetPos);
	    if (result)
	        return result;
	
	    if (role == CMD_EGroupRole.ARMORED)
	        return null;
	
	    result = FindIdleGroupByCurrentRole(role, CMD_EGroupRole.NONE, targetPos);
	    if (result)
	        return result;
	
	    result = FindIdleGroupByCurrentRole(role, CMD_EGroupRole.RESERVE, targetPos);
	    if (result)
	        return result;
	
	    result = FindIdleGroupByCurrentRole(role, CMD_EGroupRole.RECON, targetPos);
	    if (result)
	        return result;
	
	    result = FindIdleGroupByCurrentRole(role, CMD_EGroupRole.REINFORNCE, targetPos);
	    if (result)
	        return result;
	
	    result = FindIdleGroupByCurrentRole(role, CMD_EGroupRole.DEFEND, targetPos);
	    return result;
	}
	
	protected DCO_GroupUtilityComponent FindIdleGroupByCurrentRole(CMD_EGroupRole targetRole, CMD_EGroupRole currentRole, vector targetPos)
	{
		DCO_GroupUtilityComponent best = null;
		float bestScore = -1.0;
		float bestDistSq = -1.0;
	
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
			
			if (!grp.CanCommanderOverrideRole())
				continue;
			
			if (!grp.CanItHaveOrder())
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
			
	        float distSq = vector.DistanceSq(grp.GetOwner().GetOrigin(), targetPos);
	
			if (score > bestScore)
			{
		        if (bestDistSq < 0.0 || distSq < bestDistSq)
		        {
					bestScore = score;
		            bestDistSq = distSq;
					best = grp;
		        }
			}
		}
	
		return best;
	}
	
	protected void EvaluateCommanderMode(float worldTime)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;
		
		if (m_eCommanderModeExternal != CMD_ECommanderMode.BALANCED)
		{
			switch (m_eCommanderMode)
			{
				case CMD_ECommanderMode.DEFENSIVE:
				{
					SwitchToDefensive(worldTime);
					return;
				}
				case CMD_ECommanderMode.OFFENSIVE:
				{
					SwitchToOffensive();
					return;
				}
			}
		} else
		{
			m_eCommanderMode = m_eCommanderModeExternal;
			return;
		}
	 	/*
		bool anyLost = false;
		bool haveObjectiveToDefend = false;
	 
		foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
		{
			if (!obj)
				continue;
	 
			if (obj.IsCapturedBy(m_sFactionKey))
			{
				haveObjectiveToDefend = true;
				if (obj.GetObjectiveState(m_sFactionKey) == CMD_EObjectiveState.FAILED)
					anyLost = true;
				break;
			}
		}
	 
		if ((haveObjectiveToDefend || anyLost) && m_eCommanderMode == CMD_ECommanderMode.OFFENSIVE)
		{
			SwitchToDefensive(worldTime);
			return;
		}
	 
		if (m_eCommanderMode == CMD_ECommanderMode.DEFENSIVE && !anyLost)
		{
			float elapsed = worldTime - m_fDefensiveTriggerCooldown;
			if (elapsed >= DEFENSIVE_COOLDOWN)
				SwitchToOffensive();
		}*/
	}
	
	protected void Think(float worldTime)
	{
		if (!Replication.IsServer())
			return;
	 
		m_eCommanderState = CMD_ECommanderState.PLANNING;
	 
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;
	 
		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp)
				continue;
			
			if (!grp.CanItHaveOrder())
				continue;
	 
			if (grp.GetUnitCount() <= m_iRetreatThreshold
				&& grp.GetGroupStatus() != DCOG_EGroupStatus.IDLE)
			{
				//SCR_AIWaypoint rallyWP = SpawnMoveWP(GetOwner().GetOrigin());
				//if (rallyWP)
					//grp.ForceRetreat(rallyWP, worldTime);
			}
	 
			if (!grp.CheckOrderComplete(worldTime))
				continue;
		}
	 
		EvaluateCommanderMode(worldTime);
	 
		if (m_eCommanderMode == CMD_ECommanderMode.DEFENSIVE)
			ThinkDefensive(worldTime);
		else if (m_eCommanderMode == CMD_ECommanderMode.OFFENSIVE)
			ThinkOffensive(mgr, worldTime);
		else if (m_eCommanderMode == CMD_ECommanderMode.BALANCED)
		{
			ThinkDefensive(worldTime);
			ThinkOffensive(mgr, worldTime);
		}
	 
		m_eCommanderState = CMD_ECommanderState.COMMANDING;
	}
	
	protected void ThinkOffensive(AICommander_ManagerComponent mgr, float worldTime)
	{
		mgr.GetTopObjectivesOffensive(this, m_fObjectiveAtTheSameTime, m_aObjective);
	 
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
			
			if (obj.CheckAndMarkIfLost(m_sFactionKey))
			{
				obj.ResetLostStatus(m_sFactionKey);
				continue;
			}
	 
			AssignRolesToObjective(obj, worldTime);
		}
	}
	
	protected void ThinkDefensive(float worldTime)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;

		array<CMD_AICommanderObjectiveComponent> allObjs = {};
		mgr.GetTopObjectives(this, mgr.m_aObjective.Count(), allObjs);
	 
		bool hasAnyWork = false;
	 
		foreach (CMD_AICommanderObjectiveComponent obj : allObjs)
		{
			if (!obj)
				continue;
	 
			if (!obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
				continue;
			
			if (obj.CheckIsItLost(m_sFactionKey))
			{
				obj.ResetAssignedGroupCount(m_sFactionKey);
				obj.SetObjectiveState(m_sFactionKey, CMD_EObjectiveState.PENDING);
				continue;
			}
			
	 
			AssignDefendToObjective(obj, worldTime);
			hasAnyWork = true;
		}
		
		Print(hasAnyWork.ToString() + " < HAS DEFEND WORK FOR " + m_sCommanderUID + " " + m_sFactionKey);
	 
		if (!hasAnyWork)
		{
			SendIdleGroupsToReserve();
		}
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
	
	    DCO_GroupUtilityComponent assaultGrp = FindBestIdleGroupForRole(CMD_EGroupRole.ASSAULT, objPos);
	    if (assaultGrp)
	    {
			if (assaultGrp.IsPlayerGroup())
			{
				//CMD_TaskNotifier.Notify(assaultGrp.GetOwner(), "STAGING " + obj.GetOwner().GetName(), obj.GetOwner().GetOrigin(), CMD_ETaskType.MOVE);
				return;
			}
			assaultGrp.CompleteAllWaypoints();
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
	
	    DCO_GroupUtilityComponent flankGrp = FindBestIdleGroupForRole(CMD_EGroupRole.FLANK, objPos);
	    if (flankGrp)
	    {
			if (flankGrp.IsPlayerGroup())
			{
				//CMD_TaskNotifier.Notify(flankGrp.GetOwner(), "STAGING " + obj.GetOwner().GetName(), obj.GetOwner().GetOrigin(), CMD_ETaskType.MOVE);
				return;
			}
			flankGrp.CompleteAllWaypoints();
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
	
	protected void GenerateSearchWaypoints(vector center, float radius, array<SCR_AIWaypoint> outWaypoints, float wpSpacing = 50.0)
	{
	    if (!outWaypoints)
	        return;
	
	    outWaypoints.Clear();
	
	    int rings = Math.Max(1, (int)Math.Round(radius / wpSpacing));
	    
	    int baseSectorsPerRing = Math.Max(2, (int)Math.Round((radius * 2 * Math.PI) / (wpSpacing * 1.5))); 
	
	    RandomGenerator rand = new RandomGenerator();
	    float ringStep = radius / rings;
	
	    array<int> middleRings = new array<int>();
	    for (int r = 1; r < rings; r++)
	        middleRings.Insert(r);
	
	    for (int r = middleRings.Count() - 1; r > 0; r--)
	    {
	        int swapIdx = Math.RandomInt(0, r + 1);
	        int tmp = middleRings[r];
	        middleRings[r] = middleRings[swapIdx];
	        middleRings[swapIdx] = tmp;
	    }
	
	    array<int> ringOrder = new array<int>();
	    ringOrder.Insert(rings);
	    foreach (int r : middleRings)
	        ringOrder.Insert(r);
	    
	    if (rings > 1) 
	        ringOrder.Insert(rings);
	
	    foreach (int ring : ringOrder)
	    {
	        float radiusInner = ringStep * (ring - 1);
	        float radiusOuter = ringStep * ring;
	        
	        int currentSectors = Math.Max(2, (int)Math.Round(baseSectorsPerRing * ((float)ring / rings)));
	        float sectorAngle = 360.0 / currentSectors;
	
	        for (int sector = 0; sector < currentSectors; sector++)
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
	                outWaypoints.Insert(wp);
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
	
	    // --- Snap ke surface & validasi bukan air ---
	    BaseWorld world = GetGame().GetWorld();
	    if (!world)
	        return null;
		
		EWaterSurfaceType waterType = EWaterSurfaceType.WST_NONE;
	
	    float surfaceY = world.GetSurfaceY(pos[0], pos[2]);
		float lakeArea = 0;
	
	    float waterY = SCR_WorldTools.GetWaterSurfaceY(null, pos, waterType, lakeArea);
	    if (surfaceY < waterY)
	    {
			if (waterType == EWaterSurfaceType.WST_OCEAN)
	        	return null;
	    }
	
	    pos[1] = surfaceY;
	    EntitySpawnParams params = EntitySpawnParams();
	    params.TransformMode = ETransformMode.WORLD;
	    Math3D.MatrixIdentity4(params.Transform);
	    params.Transform[3] = pos;
	
	    return SCR_AIWaypoint.Cast(GetGame().SpawnEntityPrefab(res, null, params));
	}
	
	SCR_AIWaypoint SpawnArtilleryWP(vector pos)
	{
	    AICommander_BaseComponentClass data = AICommander_BaseComponentClass.Cast(GetComponentData(GetOwner()));
	    if (!data)
	        return null;
	
	    Resource res = Resource.Load(data.GetShootArtilleryWaypointPrefab());
	    if (!res || !res.IsValid())
	        return null;
	
	    BaseWorld world = GetGame().GetWorld();
	    if (!world)
	        return null;
		
		EWaterSurfaceType waterType = EWaterSurfaceType.WST_NONE;
	
	    float surfaceY = world.GetSurfaceY(pos[0], pos[2]);
		float lakeArea = 0;
	
	    float waterY = SCR_WorldTools.GetWaterSurfaceY(null, pos, waterType, lakeArea);
	    if (surfaceY < waterY)
	    {
			if (waterType == EWaterSurfaceType.WST_OCEAN)
	        	return null;
	    }
	
	    pos[1] = surfaceY;
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
	
	protected void HandleStalemateObjective(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
	    float lastResponse;
	    if (m_mStalemateResponseTime.Find(obj, lastResponse))
	    {
	        if ((worldTime - lastResponse) < m_fStalemateResponseCooldown)
	            return;
	    }
	
	    if (!m_mStalemateResponseTime.Contains(obj))
	        m_mStalemateResponseTime.Insert(obj, worldTime);
	    else
	        m_mStalemateResponseTime.Set(obj, worldTime);
	
	    obj.NotifyContested(worldTime);
	
	    bool slotFull = obj.IsGroupSlotFull(m_sFactionKey);
	
	    if (!slotFull)
	    {
	        TrySendAssaultWithSlots(obj, worldTime);
	        return;
	    }
	
	    ReallocateGroupFromStalemateObjective(obj, worldTime);
	}
	
	protected void ReallocateGroupFromStalemateObjective(CMD_AICommanderObjectiveComponent stalemateObj, float worldTime)
	{
	    AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
	    if (!mgr)
	        return;
	
	    array<CMD_AICommanderObjectiveComponent> candidates = {};
	    mgr.GetTopObjectivesOffensive(this, m_fObjectiveAtTheSameTime + 2, candidates);
	
	    CMD_AICommanderObjectiveComponent altObj = null;
	    foreach (CMD_AICommanderObjectiveComponent c : candidates)
	    {
	        if (!c || c == stalemateObj)
	            continue;
	        if (c.IsStalemate(m_sFactionKey, worldTime))
	            continue;
	        altObj = c;
	        break;
	    }
	
	    if (!altObj)
	    {
	        stalemateObj.ResetAssignedGroupCount(m_sFactionKey);
	        TrySendAssaultWithSlots(stalemateObj, worldTime);
	        return;
	    }
	
	    foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
	    {
	        if (!grp)
	            continue;
	
	        if (grp.GetGroupObjective() != stalemateObj)
	            continue;
	
	        if (grp.GetGroupStatus() == DCOG_EGroupStatus.EXECUTING_COMMAND)
	            continue;
	
	        grp.SetGroupObjective(altObj);
	        grp.SetGroupRole(CMD_EGroupRole.ASSAULT);
	
	        RandomGenerator rand = new RandomGenerator();
	        vector altPos = rand.GenerateRandomPointInRadius(5, altObj.GetRadius(), altObj.GetOwner().GetOrigin(), false);
	        altPos[1] = GetGame().GetWorld().GetSurfaceY(altPos[0], altPos[2]);
	
	        if (!TryAssignTransport(grp, altPos, worldTime))
	        {
	            SCR_AIWaypoint wp = SpawnMoveWP(altPos);
	            if (wp)
	                grp.MoveTo(wp, worldTime);
	        }
	
	        stalemateObj.SetObjectiveGroup(m_sFactionKey, -1);
	        altObj.SetObjectiveGroup(m_sFactionKey, 1);
	
	        break;
	    }
	}
	
	protected void AssignDefendToObjective(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
	    int needed = obj.GetDefendGroupCount();
	    int current = obj.GetCurrentAssignedGroupCount(m_sFactionKey);
	    int toSend = needed - current;
	
	    if (toSend <= 0)
	        return;
	
	    vector objPos = obj.GetOwner().GetOrigin();
	
	    array<int> roleList = new array<int>();
	
	    if (toSend > 2)
	    {
	        roleList.Insert(1);
	        for (int i = 1; i < toSend; i++)
	        {
				float qrfChance = 0.2 / i;
				float roll = Math.RandomFloat(0.0, 1.0);
				
				int role = 0;
				if (roll < qrfChance)
				    role = 1;
				
				roleList.Insert(role);
	        }
	    }
	    else
	    {
	        for (int i = 0; i < toSend; i++)
	            roleList.Insert(0);
	    }
	
	    for (int i = roleList.Count() - 1; i > 0; i--)
	    {
	        int j = Math.RandomInt(0, i + 1);
	        int tmp = roleList[i];
	        roleList[i] = roleList[j];
	        roleList[j] = tmp;
	    }
	
	    for (int i = 0; i < toSend; i++)
	    {
	        DCO_GroupUtilityComponent defGrp = FindBestIdleGroupForRole(CMD_EGroupRole.RESERVE, obj.GetOwner().GetOrigin());
	        if (!defGrp)
	            break;
			
			defGrp.CompleteAllWaypoints();
	
	        if (roleList[i] == 1)
	        {
	            vector qrfPos = ComputeFlankPosition(GetOwner().GetOrigin(), objPos, obj.GetRadius() * 0.8);
	            if (!TryAssignTransport(defGrp, qrfPos, worldTime))
	            {
	                SCR_AIWaypoint wp = SpawnMoveWP(qrfPos);
	                if (wp)
	                {
	                    defGrp.SetGroupRole(CMD_EGroupRole.RESERVE);
	                    defGrp.MoveTo(wp, worldTime);
	                    obj.SetObjectiveGroup(m_sFactionKey, 1);
	                }
	            }
	        }
	        else
	        {
	            float roll = Math.RandomFloat(0.0, 1.0);
	            bool bDoDefend = (roll < m_fDefendChance);
	
	            if (bDoDefend)
	            {
	                RandomGenerator rand = new RandomGenerator();
	                vector defendPos = rand.GenerateRandomPointInRadius(0, obj.GetRadius(), obj.GetOwner().GetOrigin(), false);
	                defendPos[1] = GetGame().GetWorld().GetSurfaceY(defendPos[0], defendPos[2]);
	
	                SCR_AIWaypoint wp = SpawnDefendWP(defendPos);
	                if (wp)
	                {
	                    defGrp.SetGroupRole(CMD_EGroupRole.DEFEND);
	                    defGrp.MoveTo(wp, worldTime);
	                    wp.SetCompletionRadius(obj.GetRadius());
	                    obj.SetObjectiveGroup(m_sFactionKey, 1);
	                }
	            }
	            else
	            {
	                AssignPatrolAroundObjective(defGrp, objPos, obj.GetRadius(), worldTime);
	                obj.SetObjectiveGroup(m_sFactionKey, 1);
	            }
	        }
	    }
	
	    Print("Assigning Number Of Squad to Defend : " + toSend.ToString());
	}
	
	protected void AssignPatrolAroundObjective(DCO_GroupUtilityComponent grp, vector center,float radius,float worldTime)
	{
		int patrolPoints = 4;
		float patrolRadius = radius * 1.6;
		grp.CompleteAllWaypoints();
		for (int p = 0; p < patrolPoints; p++)
		{
			float angleDeg = (360.0 / patrolPoints) * p;
			float angleRad = angleDeg * Math.DEG2RAD;
	
			float px = center[0] + Math.Cos(angleRad) * patrolRadius;
			float pz = center[2] + Math.Sin(angleRad) * patrolRadius;
			float py = GetGame().GetWorld().GetSurfaceY(px, pz);
	
			SCR_AIWaypoint wp = SpawnMoveWP(Vector(px, py, pz));
			if (wp)
				grp.MoveTo(wp, worldTime);
		}
	
		grp.SetGroupRole(CMD_EGroupRole.DEFEND);
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
			DCO_GroupUtilityComponent assaultGrp = FindBestIdleGroupForRole(CMD_EGroupRole.ASSAULT, objPos);
			if (assaultGrp)
			{
				if (assaultGrp.IsPlayerGroup())
				{
					//CMD_TaskNotifier.Notify(assaultGrp.GetOwner(), "ASSAULT " + obj.GetOwner().GetName(), obj.GetOwner().GetOrigin(), CMD_ETaskType.CAPTURE);
					return;
				}
				assaultGrp.CompleteAllWaypoints();
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
			DCO_GroupUtilityComponent flankGrp = FindBestIdleGroupForRole(CMD_EGroupRole.FLANK, objPos);
			if (flankGrp)
			{
				if (flankGrp.IsPlayerGroup())
				{
					//CMD_TaskNotifier.Notify(flankGrp.GetOwner(), "FLANK " + obj.GetOwner().GetName(), obj.GetOwner().GetOrigin(), CMD_ETaskType.DESTROY);
					return;
				}
				flankGrp.CompleteAllWaypoints();
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
	
	        if (obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
	            continue;
	
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
	
	        if (obj.IsStalemate(m_sFactionKey, worldTime))
	        {
	            HandleStalemateObjective(obj, worldTime);
	        }
	
	        if (obj.IsCaptureTimerComplete(m_sFactionKey, worldTime))
	        {
	            obj.SetCapturedBy(m_sFactionKey, true);
	            obj.ResetAssignedGroupCount(m_sFactionKey);
	            obj.ResetStalemateTracking(); // ← reset tracking setelah captured
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
	
	array<vector> GenerateArtilleryImpactPoints(vector center, float range, float dispersion, float accuracy, float numberOfShell = 3)
	{
	    array<vector> impactPoints = new array<vector>();
	    RandomGenerator rand = new RandomGenerator();
	
	    float effectiveDispersion = dispersion * (1.0 - Math.Clamp(accuracy, 0.0, 1.0));
	    float minRadius           = effectiveDispersion * 0.1;
	
	    for (int i = 0; i < numberOfShell; i++)
	    {
	        float r1     = rand.RandFloatXY(minRadius, effectiveDispersion);
	        float r2     = rand.RandFloatXY(minRadius, effectiveDispersion);
	        float radius = (r1 + r2) * 0.5;
	
	        float angleDeg = rand.RandFloatXY(0.0, 360.0);
	        float angleRad = angleDeg * Math.DEG2RAD;
	
	        float px = center[0] + Math.Cos(angleRad) * radius;
	        float pz = center[2] + Math.Sin(angleRad) * radius;
	        float py = GetGame().GetWorld().GetSurfaceY(px, pz);
	
	        impactPoints.Insert(Vector(px, py, pz));
	    }
	
	    return impactPoints;
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
			//Print(string.Format("[%1] Vehicle %2 tidak punya DCO_TransportMissionComponent — skip", m_sCommanderUID, vehicle.GetName()), LogLevel.WARNING);
			return;
		}
	 
		if (mission.IsActiveVehicle())
		{
			//Print(string.Format("[%1] Vehicle %2 sudah dipakai transport lain", m_sCommanderUID, vehicle.GetName()));
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
	 
		/*Print(string.Format("[%1] TRANSPORT assigned | group: %2 | vehicle: %3 | dest: %4",
			m_sCommanderUID,
			passengerGroup.GetOwner().GetName(),
			vehicle.GetName(),
			destination.ToString()));*/
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
	
	DCO_GroupUtilityComponent FindBestIdleGroupForRole_Public(CMD_EGroupRole role, vector pos)
	{
	    return FindBestIdleGroupForRole(role, pos);
	}
	
	void RegisterTransportTeam(DCO_TransportTeamComponent team)
	{
		if (!team || m_aTransportTeams.Contains(team))
			return;
	 
		team.SetRallyPoint(GetOwner().GetOrigin());
		m_aTransportTeams.Insert(team);
	 
		//Print(string.Format("[%1] Transport team registered: %2", m_sCommanderUID, team.GetOwner().GetName()));
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
			//Print(string.Format("[%1] Vehicle %2 tidak punya DCO_TransportMissionComponent — skip", m_sCommanderUID, vehicle.GetName()), LogLevel.WARNING);
			return null;
		}
	 
		if (mission.IsActiveVehicle())
		{
			//Print(string.Format("[%1] Vehicle %2 sudah dipakai transport lain", m_sCommanderUID, vehicle.GetName()));
			return null;
		}
		

		
		SCR_AIWaypoint wpToVehicle = SpawnMoveWP(vehicle.GetOrigin());
		if (wpToVehicle)
			t.GetDCOGroupUtility().MoveTo(wpToVehicle, wt);
		
		mission.StartMission(t.GetDCOGroupUtility(), TakeAt, m_sFactionKey, wt, this);
		
		return null;
	}
	
	void ComputeArtillerySpreadAndDispersion(vector center, float range, float dispersion, float accuracy, float numberOfShell = 3)
	{
	    RandomGenerator rand = new RandomGenerator();
	    
	    float effectiveDispersion = dispersion * (1.0 - Math.Clamp(accuracy, 0.0, 1.0));
	    float minRadius           = effectiveDispersion * 0.1;
	    
	    for (int i = 0; i < numberOfShell; i++)
	    {
	        float r1 = rand.RandFloatXY(minRadius, effectiveDispersion);
	        float r2 = rand.RandFloatXY(minRadius, effectiveDispersion);
	        float radius = (r1 + r2) * 0.5;
	        
	        float angleDeg = rand.RandFloatXY(0.0, 360.0);
	        float angleRad = angleDeg * Math.DEG2RAD;
	        
	        float px = center[0] + Math.Cos(angleRad) * radius;
	        float pz = center[2] + Math.Sin(angleRad) * radius;
	        float py = GetGame().GetWorld().GetSurfaceY(px, pz);
	        
	        vector shellImpact = Vector(px, py, pz);
	        
	        Print(string.Format("[Artillery] Shell %1 impact at %2 (radius: %3m from center)",
	            i + 1, shellImpact.ToString(), radius.ToString()));
	        
	        // TODO: spawn explosion / effect di shellImpact
	    }
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
		m_MyEnt = owner;
		InitializeCommander();
	}
}