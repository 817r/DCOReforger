enum DCO_ELogiJob
{
	MEDEVAC,
	EXTRACT,
	REINFORCE,
	DEPLOY,
	REDEPLOY,
	RESUPPLY
}

class DCO_LogiJob : Managed
{
	int m_iId;
	DCO_ELogiJob m_eKind;
	ref array<DCO_GroupUtilityComponent> m_aGroups = {};
	ref array<SCR_AIGroup> m_aPlayerGroups = {};
	ref array<IEntity> m_aCasualties = {};
	ref array<IEntity> m_aLoaded = {};
	vector m_vPickup;
	vector m_vDest;
	vector m_vLZ;
	vector m_vAltLZ;
	bool m_bAltUsed;
	bool m_bFromPlayer;
	bool m_bRolePriority;
	int m_iRequester = -1;
	float m_fCreated;
	float m_fAssigned;
	float m_fEta;
	DCO_TransportTeamComponent m_Team;
	DCO_GroupUtilityComponent m_Escort;
	int m_iBoarded;
	int m_iLeft;
	float m_fPickupShift;
	bool m_bMarked;

	bool IsTransportKind()
	{
		return m_eKind == DCO_ELogiJob.DEPLOY || m_eKind == DCO_ELogiJob.REINFORCE || m_eKind == DCO_ELogiJob.REDEPLOY || m_eKind == DCO_ELogiJob.EXTRACT;
	}

	int SeatsNeeded()
	{
		int n = m_aCasualties.Count() + m_aLoaded.Count();
		if (m_eKind == DCO_ELogiJob.RESUPPLY)
			return 0;
		foreach (DCO_GroupUtilityComponent g : m_aGroups)
		{
			if (g)
				n += g.GetUnitCount();
		}
		foreach (SCR_AIGroup pg : m_aPlayerGroups)
		{
			if (pg)
				n += pg.GetPlayerIDs().Count();
		}
		return n;
	}

	bool IsEmpty()
	{
		if (m_eKind == DCO_ELogiJob.MEDEVAC)
			return m_aCasualties.IsEmpty() && m_aLoaded.IsEmpty();
		for (int i = m_aGroups.Count() - 1; i >= 0; i--)
		{
			if (!m_aGroups[i])
				m_aGroups.Remove(i);
		}
		for (int i = m_aPlayerGroups.Count() - 1; i >= 0; i--)
		{
			if (!m_aPlayerGroups[i])
				m_aPlayerGroups.Remove(i);
		}
		return m_aGroups.IsEmpty() && m_aPlayerGroups.IsEmpty();
	}
}

class DCO_HubCasualty
{
	IEntity m_Ent;
	float m_fArrive;
}

class DCO_Logistics
{
	protected static const float TICK_S = 2;
	protected static const float SCAN_S = 10;
	protected static const float WALK_MPS = 1.4;

	protected ref array<ref DCO_LogiJob> m_aJobs = {};
	protected ref array<ref DCO_HubCasualty> m_aHubCasualties = {};
	protected ref map<IEntity, float> m_mDownSince = new map<IEntity, float>();
	protected vector m_vHub;
	protected string m_sHubSource;
	protected float m_fTimer;
	protected float m_fScanTimer;
	protected static int s_iNextId;

	static float Now()
	{
		return GetGame().GetWorld().GetWorldTime() / 1000.0;
	}

	vector GetHub()				{ return m_vHub; }
	void ForceScan()			{ m_fScanTimer = SCAN_S; }
	array<ref DCO_LogiJob> GetJobs()	{ return m_aJobs; }

	void Update(AICommander_BaseComponent cmd, float timeSlice)
	{
		m_fTimer += timeSlice;
		if (m_fTimer < TICK_S)
			return;
		m_fTimer = 0;

		float now = Now();
		m_vHub = cmd.GetHubFor(cmd.GetOwner().GetOrigin(), m_sHubSource);
		foreach (DCO_TransportTeamComponent team : cmd.GetTransportTeams())
		{
			if (team)
				team.SetRallyPoint(m_vHub);
		}

		m_fScanTimer += TICK_S;
		if (m_fScanTimer >= SCAN_S)
		{
			m_fScanTimer = 0;
			ScanNeeds(cmd, now);
		}

		Prune(cmd, now);
		Dispatch(cmd, now);
		UpdateHubCasualties(cmd, now);
	}

	void Clear()
	{
		m_aJobs.Clear();
		m_mDownSince.Clear();
	}

	int GetPriority(AICommander_BaseComponent cmd, DCO_LogiJob job)
	{
		int p = cmd.GetLogiPriority(job.m_eKind);
		if (job.m_bFromPlayer)
			p = Math.Max(p, cmd.GetLogiPlayerPriority());
		if (job.m_bRolePriority)
			p++;
		return p;
	}

	bool HasJobFor(DCO_GroupUtilityComponent grp)
	{
		foreach (DCO_LogiJob j : m_aJobs)
		{
			if (j.m_aGroups.Contains(grp))
				return true;
		}
		return false;
	}

	protected bool HasCasualty(IEntity ent)
	{
		foreach (DCO_LogiJob j : m_aJobs)
		{
			if (j.m_aCasualties.Contains(ent) || j.m_aLoaded.Contains(ent))
				return true;
		}
		foreach (DCO_HubCasualty h : m_aHubCasualties)
		{
			if (h.m_Ent == ent)
				return true;
		}
		return false;
	}

	protected DCO_LogiJob NewJob(DCO_ELogiJob kind, vector pickup, vector dest, float now)
	{
		DCO_LogiJob job = new DCO_LogiJob();
		job.m_iId = ++s_iNextId;
		job.m_eKind = kind;
		job.m_vPickup = pickup;
		job.m_vDest = dest;
		job.m_vLZ = dest;
		job.m_vAltLZ = dest;
		job.m_fCreated = now;
		m_aJobs.Insert(job);
		return job;
	}

	static DCO_ELogiJob KindForTask(DCO_EGroupTask task)
	{
		switch (task)
		{
			case DCO_EGroupTask.REINFORCE:
				return DCO_ELogiJob.REINFORCE;
			case DCO_EGroupTask.NONE:
			case DCO_EGroupTask.PATROL:
			case DCO_EGroupTask.DEFEND:
			case DCO_EGroupTask.GARRISON:
				return DCO_ELogiJob.REDEPLOY;
		}
		return DCO_ELogiJob.DEPLOY;
	}

	bool RequestTransport(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent grp, vector dest, DCO_EGroupTask taskAfter, float now)
	{
		DCO_ELogiJob kind = KindForTask(taskAfter);
		vector from = grp.GetOwner().GetOrigin();
		int units = grp.GetUnitCount();

		float batchR = cmd.GetLogiBatchRadius();
		foreach (DCO_LogiJob j : m_aJobs)
		{
			if (!j.m_Team || !j.IsTransportKind() || j.m_eKind == DCO_ELogiJob.EXTRACT || !j.m_aPlayerGroups.IsEmpty())
				continue;
			if (!j.m_Team.IsPickingUp() || now - j.m_fAssigned > cmd.GetLogiBatchWindow())
				continue;
			if (vector.DistanceXZ(from, j.m_vPickup) > batchR || vector.DistanceXZ(dest, j.m_vDest) > batchR)
				continue;
			if (j.m_Team.GetFreeCargoSeats() < j.SeatsNeeded() + units)
				continue;

			grp.SetTask(taskAfter);
			j.m_aGroups.Insert(grp);
			j.m_Team.AddPassenger(grp, now);
			Log(j, "batch", string.Format("group=%1 total_groups=%2", grp.GetOwner().GetName(), j.m_aGroups.Count()));
			return true;
		}

		int prio = cmd.GetLogiPriority(kind);
		int higher;
		foreach (DCO_LogiJob q : m_aJobs)
		{
			if (q.m_Team || q.IsEmpty() || GetPriority(cmd, q) <= prio)
				continue;
			float qEta;
			if (BestTeamFor(cmd, q, qEta))
				higher++;
		}
		if (CountAvailableTeams(cmd) <= higher)
			return false;

		DCO_LogiJob probe = new DCO_LogiJob();
		probe.m_eKind = kind;
		probe.m_vPickup = from;
		probe.m_vDest = dest;
		probe.m_aGroups.Insert(grp);
		float eta;
		DCO_TransportTeamComponent team = BestTeamFor(cmd, probe, eta);
		if (!team)
			return false;

		float walk = vector.Distance(from, dest) / WALK_MPS;
		if (eta > walk * (1 - cmd.GetLogiEtaMargin()))
			return false;

		DCO_LogiJob job = NewJob(kind, from, dest, now);
		job.m_aGroups.Insert(grp);
		grp.SetTask(taskAfter);
		Assign(cmd, job, team, eta, now);
		return true;
	}

	protected int CountAvailableTeams(AICommander_BaseComponent cmd)
	{
		int n;
		foreach (DCO_TransportTeamComponent t : cmd.GetTransportTeams())
		{
			if (t && t.IsAvailable() && t.ResolveVehicle())
				n++;
		}
		return n;
	}

	protected DCO_TransportTeamComponent BestTeamFor(AICommander_BaseComponent cmd, DCO_LogiJob job, out float outEta)
	{
		DCO_TransportTeamComponent best;
		outEta = float.MAX;
		int seats = job.SeatsNeeded();
		foreach (DCO_TransportTeamComponent t : cmd.GetTransportTeams())
		{
			if (!t || !t.IsAvailable() || !t.AllowsJob(job.m_eKind))
				continue;

			IEntity veh = t.ResolveVehicle();
			if (!veh)
				continue;

			if (job.m_eKind == DCO_ELogiJob.RESUPPLY)
			{
				if (!HasResupplyStation(veh))
					continue;
			}
			else if (DCO_TransportTeamComponent.CountFreeCargo(veh) <= 0 || (job.m_eKind == DCO_ELogiJob.MEDEVAC && seats <= 0))
			{
				continue;
			}

			float eta = (vector.Distance(t.GetTeamPos(), job.m_vPickup) + vector.Distance(job.m_vPickup, job.m_vDest)) / 7.0 + 45.0;
			if (DCO_TransportTeamComponent.CountFreeCargo(veh) < seats)
				eta += 120;

			if (eta < outEta)
			{
				outEta = eta;
				best = t;
			}
		}
		return best;
	}

	protected void Assign(AICommander_BaseComponent cmd, DCO_LogiJob job, DCO_TransportTeamComponent team, float eta, float now)
	{
		PlanLZ(cmd, job);
		job.m_fAssigned = now;
		job.m_fEta = eta;
		team.AssignLogiJob(job, cmd, now);
		Log(job, "assign", string.Format("team=%1 wait=%2s eta=%3s seats=%4", team.GetOwner().GetName(),
			Math.Round(now - job.m_fCreated), Math.Round(eta), job.SeatsNeeded()));

		if (!job.m_aPlayerGroups.IsEmpty() && job.m_eKind != DCO_ELogiJob.MEDEVAC)
		{
			AnnounceLZ(job, eta);
			SendMarks(job, false);
		}
		else if (job.m_iRequester > 0)
		{
			DCO_Radio.Player(job.m_iRequester, "LOGISTICS", "logi_enroute", DCO_ERadioKind.INFO, DCO_Radio.P("kind", KindKey(job.m_eKind), "eta", DCO_Radio.N(Math.Max(1, eta / 60))));
		}
	}

	protected void AnnounceLZ(DCO_LogiJob job, float eta)
	{
		float off = vector.DistanceXZ(job.m_vDest, job.m_vLZ);
		string etaMin = DCO_Radio.N(Math.Max(1, eta / 60));
		foreach (SCR_AIGroup pg : job.m_aPlayerGroups)
		{
			if (!pg)
				continue;
			if (off < 25)
				DCO_Radio.Group(pg.GetGroupID(), "LOGISTICS", "logi_inbound_mark", DCO_ERadioKind.INFO, DCO_Radio.P("grid", DCO_PlayerComms.Grid(job.m_vLZ), "eta", etaMin));
			else
				DCO_Radio.Group(pg.GetGroupID(), "LOGISTICS", "logi_inbound", DCO_ERadioKind.INFO, DCO_Radio.P("grid", DCO_PlayerComms.Grid(job.m_vLZ), "dist", DCO_Radio.N(off), "dir", DCO_Radio.Dir(job.m_vDest, job.m_vLZ), "eta", etaMin));
		}
	}

	void SendMarks(DCO_LogiJob job, bool clear)
	{
		if (clear && !job.m_bMarked)
			return;

		array<float> marks = {};
		if (!clear)
		{
			job.m_bMarked = true;
			marks.Insert(job.m_vPickup[0]);
			marks.Insert(job.m_vPickup[2]);
			marks.Insert(job.m_vLZ[0]);
			marks.Insert(job.m_vLZ[2]);
			if (!job.m_bAltUsed && vector.DistanceXZ(job.m_vAltLZ, job.m_vLZ) > 1)
			{
				marks.Insert(job.m_vAltLZ[0]);
				marks.Insert(job.m_vAltLZ[2]);
			}
		}

		foreach (SCR_AIGroup pg : job.m_aPlayerGroups)
		{
			if (!pg)
				continue;
			foreach (int pid : pg.GetPlayerIDs())
			{
				SCR_PlayerControllerGroupComponent comp = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(pid);
				if (comp)
					comp.DCO_SendTransportMarks(marks);
			}
		}
	}

	void OnPickupMoved(DCO_LogiJob job, vector teamPos)
	{
		SendMarks(job, false);
		if (job.m_fPickupShift < 300)
			return;

		job.m_fPickupShift = 0;
		string etaMin = DCO_Radio.N(Math.Max(1, vector.Distance(teamPos, job.m_vPickup) / 7.0 / 60));
		foreach (SCR_AIGroup pg : job.m_aPlayerGroups)
		{
			if (pg)
				DCO_Radio.Group(pg.GetGroupID(), "LOGISTICS", "logi_pickup_moved", DCO_ERadioKind.INFO, DCO_Radio.P("grid", DCO_PlayerComms.Grid(job.m_vPickup), "eta", etaMin));
		}
	}

	void OnLZDiverted(DCO_LogiJob job)
	{
		if (job.m_aPlayerGroups.IsEmpty())
			return;

		SendMarks(job, false);
		foreach (SCR_AIGroup pg : job.m_aPlayerGroups)
		{
			if (pg)
				DCO_Radio.Group(pg.GetGroupID(), "LOGISTICS", "logi_lz_divert", DCO_ERadioKind.WARNING, DCO_Radio.P("grid", DCO_PlayerComms.Grid(job.m_vLZ)));
		}
	}

	static vector SnapPickup(vector p)
	{
		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		RoadNetworkManager roads;
		if (aiWorld)
			roads = aiWorld.GetRoadNetworkManager();
		if (roads)
		{
			BaseRoad road;
			float dist;
			roads.GetClosestRoad(p, road, dist, true);
			if (road && dist <= 100)
			{
				array<vector> pts = {};
				road.GetPoints(pts);
				vector best = p;
				float bestSq = float.MAX;
				foreach (vector rp : pts)
				{
					float dSq = vector.DistanceSqXZ(rp, p);
					if (dSq < bestSq)
					{
						bestSq = dSq;
						best = rp;
					}
				}
				if (bestSq <= 10000)
					return Surface(best);
			}
		}

		vector open;
		if (SCR_WorldTools.FindEmptyTerrainPosition(open, p, 100, 4, 3))
			return Surface(open);
		return Surface(p);
	}

	protected static bool IsValidDest(vector from, vector p)
	{
		if (p == vector.Zero || vector.DistanceXZ(from, p) > 8000)
			return false;
		return p[1] - GetGame().GetWorld().GetSurfaceY(p[0], p[2]) < 50;
	}

	protected void Dispatch(AICommander_BaseComponent cmd, float now)
	{
		array<DCO_LogiJob> unserved = {};
		while (true)
		{
			DCO_LogiJob pick;
			int pickPrio = -1;
			foreach (DCO_LogiJob j : m_aJobs)
			{
				if (j.m_Team || j.IsEmpty() || unserved.Contains(j))
					continue;
				int p = GetPriority(cmd, j);
				if (!pick || p > pickPrio || (p == pickPrio && j.m_fCreated < pick.m_fCreated))
				{
					pick = j;
					pickPrio = p;
				}
			}
			if (!pick)
				return;

			float eta;
			DCO_TransportTeamComponent team = BestTeamFor(cmd, pick, eta);
			if (!team)
			{
				unserved.Insert(pick);
				continue;
			}
			Assign(cmd, pick, team, eta, now);
		}
	}

	protected void Prune(AICommander_BaseComponent cmd, float now)
	{
		for (int i = m_aJobs.Count() - 1; i >= 0; i--)
		{
			DCO_LogiJob j = m_aJobs[i];
			if (j.m_eKind == DCO_ELogiJob.MEDEVAC)
			{
				for (int c = j.m_aCasualties.Count() - 1; c >= 0; c--)
				{
					if (!IsDown(j.m_aCasualties[c]))
						j.m_aCasualties.Remove(c);
				}
			}

			if (j.m_Team)
				continue;

			if (j.IsEmpty() || now - j.m_fCreated > 300.0)
			{
				foreach (DCO_GroupUtilityComponent g : j.m_aGroups)
				{
					if (g && g.IsInTransport())
						g.EndTransport();
				}
				if (j.m_iRequester > 0 && !j.IsEmpty())
					DCO_Radio.Player(j.m_iRequester, "LOGISTICS", "logi_none_cancelled", DCO_ERadioKind.WARNING);
				Log(j, "done", "result=0 why=queue_timeout");
				m_aJobs.Remove(i);
			}
		}
	}

	void OnJobFinished(AICommander_BaseComponent cmd, DCO_LogiJob job, bool success, string why)
	{
		float now = Now();
		ReleaseEscort(job);
		SendMarks(job, true);
		Log(job, "done", string.Format("result=%1 why=%2 eta=%3s actual=%4s boarded=%5 left=%6", success, why,
			Math.Round(job.m_fEta), Math.Round(now - job.m_fAssigned), job.m_iBoarded, job.m_iLeft));

		if (job.m_iRequester > 0)
		{
			if (success)
				DCO_Radio.Player(job.m_iRequester, "LOGISTICS", "logi_complete", DCO_ERadioKind.REPORT, DCO_Radio.P("kind", KindKey(job.m_eKind)));
			else
				DCO_Radio.Player(job.m_iRequester, "LOGISTICS", "logi_aborted", DCO_ERadioKind.WARNING, DCO_Radio.P("kind", KindKey(job.m_eKind)));
		}

		m_aJobs.RemoveItem(job);
	}

	void QueueFollowUp(DCO_GroupUtilityComponent grp, DCO_LogiJob parent, float now)
	{
		if (!grp || !parent || !parent.IsTransportKind())
			return;

		grp.BeginTransport(DCO_EGroupTask.NONE);
		DCO_LogiJob job = NewJob(parent.m_eKind, grp.GetOwner().GetOrigin(), parent.m_vDest, now);
		job.m_bRolePriority = parent.m_bRolePriority;
		job.m_aGroups.Insert(grp);
		Log(job, "follow_up", string.Format("parent=%1 group=%2 units=%3", parent.m_iId, grp.GetOwner().GetName(), grp.GetUnitCount()));
	}

	void OnGroupReleased(DCO_GroupUtilityComponent grp)
	{
		foreach (DCO_LogiJob j : m_aJobs)
		{
			if (!j.m_Team)
				j.m_aGroups.RemoveItem(grp);
		}
	}

	protected void ScanNeeds(AICommander_BaseComponent cmd, float now)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		bool resupply = cmd.IsLogiResupply() && !(cfg && cfg.GetUnitMagicMagazine());

		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g.IsPlayerGroup() || g.IsDedicatedTransport() || g.IsMortar() || g.IsInTransport() || HasJobFor(g))
				continue;

			SCR_AIGroup grp = SCR_AIGroup.Cast(g.GetOwner());
			if (!grp)
				continue;

			if (cmd.IsLogiMedevac())
				ScanCasualties(cmd, grp, now);

			vector pos = g.GetOwner().GetOrigin();
			if (cmd.IsLogiExtract() && g.CanBeTransported() && (g.HasState(DCO_EGroupState.COMBAT_INEFFECTIVE) || g.HasState(DCO_EGroupState.RETREATING)))
			{
				vector hub = cmd.GetHubFor(pos, m_sHubSource);
				if (vector.DistanceXZ(pos, hub) > cmd.GetLogiExtractMinDist())
				{
					QueueExtract(cmd, g, hub, now);
					continue;
				}
			}

			if (resupply && IsLowAmmo(grp, cmd.GetLogiLowAmmoMags()))
			{
				DCO_LogiJob job = NewJob(DCO_ELogiJob.RESUPPLY, pos, pos, now);
				job.m_aGroups.Insert(g);
				Log(job, "queue", "group=" + grp.GetName());
			}
		}

		array<IEntity> stale = {};
		foreach (IEntity ent, float t : m_mDownSince)
		{
			if (!IsDown(ent))
				stale.Insert(ent);
		}
		foreach (IEntity ent : stale)
			m_mDownSince.Remove(ent);
	}

	protected void QueueExtract(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent g, vector hub, float now)
	{
		cmd.DetachFromObjective(g);
		g.BeginTransport(DCO_EGroupTask.NONE);

		SCR_AIGroup grp = SCR_AIGroup.Cast(g.GetOwner());
		grp.CompleteAllWaypoints();
		SCR_AIWaypoint wp = cmd.SpawnMoveWP(hub);
		if (wp)
			g.MoveTo(wp, now);

		DCO_LogiJob job = NewJob(DCO_ELogiJob.EXTRACT, g.GetOwner().GetOrigin(), hub, now);
		job.m_aGroups.Insert(g);
		Log(job, "queue", string.Format("group=%1 units=%2 state=%3", grp.GetName(), g.GetUnitCount(), g.GetState()));
	}

	protected void ScanCasualties(AICommander_BaseComponent cmd, SCR_AIGroup grp, float now)
	{
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			IEntity ent = a.GetControlledEntity();
			if (!IsDown(ent) || DCO_VehicleCombat.GetVehicle(ent) || HasCasualty(ent))
				continue;

			float since;
			if (!m_mDownSince.Find(ent, since))
			{
				m_mDownSince.Set(ent, now);
				continue;
			}

			if (now - since < 45.0 || DCO_MedicDispatcher.IsBooked(ent))
				continue;

			AddCasualty(cmd, ent, now, -1);
		}
	}

	protected DCO_LogiJob AddCasualty(AICommander_BaseComponent cmd, IEntity ent, float now, int requester)
	{
		vector pos = ent.GetOrigin();
		foreach (DCO_LogiJob j : m_aJobs)
		{
			if (j.m_eKind != DCO_ELogiJob.MEDEVAC || j.m_Team || vector.DistanceXZ(j.m_vPickup, pos) > cmd.GetLogiBatchRadius())
				continue;
			j.m_aCasualties.Insert(ent);
			return j;
		}

		DCO_LogiJob job = NewJob(DCO_ELogiJob.MEDEVAC, pos, cmd.GetHubFor(pos, m_sHubSource), now);
		job.m_aCasualties.Insert(ent);
		if (requester > 0)
		{
			job.m_bFromPlayer = true;
			job.m_iRequester = requester;
		}
		Log(job, "queue", "casualty=" + ent.ToString());
		return job;
	}

	static void NotifyStabilized(IEntity casualty)
	{
		AICommander_BaseComponent cmd = FindCommanderOf(casualty);
		if (cmd && cmd.GetLogistics() && IsDown(casualty))
			cmd.GetLogistics().m_mDownSince.Set(casualty, -1000);
	}

	protected static AICommander_BaseComponent FindCommanderOf(IEntity ent)
	{
		AIControlComponent ctrl = AIControlComponent.Cast(ent.FindComponent(AIControlComponent));
		if (!ctrl || !ctrl.GetAIAgent())
			return null;
		SCR_AIGroup grp = SCR_AIGroup.Cast(ctrl.GetAIAgent().GetParentGroup());
		if (!grp)
			return null;
		DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));
		if (!g)
			return null;
		return g.GetMyCommander();
	}

	static bool IsDown(IEntity ent)
	{
		ChimeraCharacter c = ChimeraCharacter.Cast(ent);
		if (!c || !c.GetCharacterController())
			return false;
		return c.GetCharacterController().GetLifeState() == ECharacterLifeState.INCAPACITATED;
	}

	static bool IsLowAmmo(SCR_AIGroup grp, int minMags)
	{
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		int low, total;
		foreach (AIAgent a : agents)
		{
			IEntity ent = a.GetControlledEntity();
			if (!ent)
				continue;

			SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(ent.FindComponent(SCR_AICombatComponent));
			BaseWeaponManagerComponent wm = BaseWeaponManagerComponent.Cast(ent.FindComponent(BaseWeaponManagerComponent));
			if (!combat || !wm || !wm.GetCurrent())
				continue;

			array<BaseMuzzleComponent> muzzles = {};
			wm.GetCurrent().GetMuzzlesList(muzzles);
			if (muzzles.IsEmpty() || !muzzles[0] || !muzzles[0].GetMagazineWell())
				continue;

			total++;
			if (combat.GetMagazineCount(muzzles[0].GetMagazineWell().Type(), false) < minMags)
				low++;
		}
		return total > 0 && low * 2 >= total;
	}

	static bool HasResupplyStation(IEntity ent)
	{
		if (!ent)
			return false;
		if (ent.FindComponent(SCR_ResupplySupportStationComponent))
			return true;
		IEntity child = ent.GetChildren();
		while (child)
		{
			if (HasResupplyStation(child))
				return true;
			child = child.GetSibling();
		}
		return false;
	}

	void AddHubCasualty(IEntity ent, float now)
	{
		DCO_HubCasualty h = new DCO_HubCasualty();
		h.m_Ent = ent;
		h.m_fArrive = now;
		m_aHubCasualties.Insert(h);
	}

	protected void UpdateHubCasualties(AICommander_BaseComponent cmd, float now)
	{
		array<AIAgent> recovered = {};
		SCR_AIGroup template;
		for (int i = m_aHubCasualties.Count() - 1; i >= 0; i--)
		{
			DCO_HubCasualty h = m_aHubCasualties[i];
			ChimeraCharacter c = ChimeraCharacter.Cast(h.m_Ent);
			if (!c || !c.GetCharacterController() || c.GetCharacterController().GetLifeState() == ECharacterLifeState.DEAD)
			{
				m_aHubCasualties.Remove(i);
				continue;
			}

			if (now - h.m_fArrive < cmd.GetLogiTreatTime())
				continue;

			m_aHubCasualties.Remove(i);
			SCR_CharacterDamageManagerComponent dmg = SCR_CharacterDamageManagerComponent.Cast(c.GetDamageManager());
			if (dmg)
				dmg.FullHeal();

			if (SCR_CharacterHelper.IsPlayer(c))
				continue;

			AIControlComponent ctrl = AIControlComponent.Cast(c.FindComponent(AIControlComponent));
			if (!ctrl || !ctrl.GetAIAgent())
				continue;

			AIAgent agent = ctrl.GetAIAgent();
			if (!template)
				template = SCR_AIGroup.Cast(agent.GetParentGroup());
			recovered.Insert(agent);
		}

		if (recovered.IsEmpty() || !template)
			return;

		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (recovered.IsEmpty())
				break;
			if (!g || g.IsPlayerGroup() || g.IsArmor() || g.IsMortar() || g.IsDedicatedTransport() || !g.IsAvailableReserve())
				continue;
			SCR_AIGroup grp = SCR_AIGroup.Cast(g.GetOwner());
			if (!grp || vector.DistanceXZ(grp.GetOrigin(), m_vHub) > 400)
				continue;
			int missing = g.DCO_GetPeakStrength() - g.GetUnitCount();
			while (missing > 0 && !recovered.IsEmpty())
			{
				AIAgent a = recovered[0];
				recovered.Remove(0);
				AIGroup old = a.GetParentGroup();
				if (old)
					old.RemoveAgent(a);
				grp.AddAgent(a);
				missing--;
			}
			Print(string.Format("[DCO_Logistics] recovered soldiers refill %1 (now %2)", grp.GetName(), g.GetUnitCount()));
		}
		if (recovered.IsEmpty())
			return;

		SCR_AIGroup newGrp = SplitIntoNewGroup(template, recovered, cmd);
		string line = string.Format("logi_recovered count=%1 group=%2 manpower=%3", recovered.Count(), newGrp, cmd.GetTotalManpower());
		Print("[DCO_Logistics] " + line);
		DCO_BenchmarkLoggerComponent.Event(line);
	}

	static SCR_AIGroup SplitIntoNewGroup(SCR_AIGroup template, array<AIAgent> agents, AICommander_BaseComponent cmd)
	{
		if (!template || agents.IsEmpty() || !agents[0].GetControlledEntity())
			return null;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = agents[0].GetControlledEntity().GetOrigin();

		SCR_AIGroup.IgnoreSpawning(true);
		Resource res = Resource.Load(template.GetPrefabData().GetPrefabName());
		SCR_AIGroup newGrp;
		if (res.IsValid())
			newGrp = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(res, GetGame().GetWorld(), params));
		SCR_AIGroup.IgnoreSpawning(false);

		if (!newGrp)
			return null;

		newGrp.SetFaction(template.GetFaction());
		foreach (AIAgent a : agents)
		{
			AIGroup old = a.GetParentGroup();
			if (old)
				old.RemoveAgent(a);
			newGrp.AddAgent(a);
		}

		DCO_GroupUtilityComponent util = DCO_GroupUtilityComponent.Cast(newGrp.FindComponent(DCO_GroupUtilityComponent));
		if (util && cmd)
			cmd.AssignGroup(util);
		return newGrp;
	}

	protected void PlanLZ(AICommander_BaseComponent cmd, DCO_LogiJob job)
	{
		if (job.m_eKind == DCO_ELogiJob.MEDEVAC || job.m_eKind == DCO_ELogiJob.EXTRACT)
		{
			job.m_vDest = cmd.GetHubFor(job.m_vPickup, m_sHubSource);
			job.m_vLZ = job.m_vDest;
			job.m_vAltLZ = job.m_vDest;
			return;
		}
		if (job.m_eKind == DCO_ELogiJob.RESUPPLY)
		{
			job.m_vLZ = job.m_vDest;
			job.m_vAltLZ = job.m_vDest;
			return;
		}

		vector back = job.m_vPickup - job.m_vDest;
		back[1] = 0;
		if (back.LengthSq() < 1)
			back = "0 0 1";
		back.Normalize();

		job.m_vLZ = ShiftSafe(cmd, job.m_vDest, back);
		job.m_vAltLZ = job.m_vLZ;
		array<float> angles = {50, -50, 100, -100};
		foreach (float deg : angles)
		{
			vector alt = ShiftSafe(cmd, job.m_vDest, Rotate(back, deg));
			if (vector.DistanceXZ(alt, job.m_vLZ) > 60)
			{
				job.m_vAltLZ = alt;
				break;
			}
		}
	}

	protected static vector Rotate(vector dir, float deg)
	{
		float r = deg * Math.DEG2RAD;
		float c = Math.Cos(r);
		float s = Math.Sin(r);
		return Vector(dir[0] * c - dir[2] * s, 0, dir[0] * s + dir[2] * c);
	}

	protected vector ShiftSafe(AICommander_BaseComponent cmd, vector dest, vector dir)
	{
		float safe = cmd.GetLogiLZSafeDist();
		for (int i = 0; i <= 8; i++)
		{
			vector p = dest + dir * (i * 50.0);
			if (!IsNearThreat(cmd, p, safe))
				return Surface(p);
		}
		return Surface(dest + dir * (8 * 50.0));
	}

	protected static vector Surface(vector p)
	{
		p[1] = GetGame().GetWorld().GetSurfaceY(p[0], p[2]);
		return p;
	}

	bool IsNearThreat(AICommander_BaseComponent cmd, vector p, float safe)
	{
		array<CMD_ThreatEntry> dangers = {};
		CollectDangers(cmd, dangers);
		foreach (CMD_ThreatEntry t : dangers)
		{
			if (t && vector.DistanceXZ(t.m_vBelievedPos, p) < safe + t.m_fBelievedUncertainty)
				return true;
		}
		return false;
	}

	protected void CollectDangers(AICommander_BaseComponent cmd, notnull array<CMD_ThreatEntry> outList)
	{
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (tr)
		{
			foreach (CMD_ThreatEntry t : tr.GetThreats())
				outList.Insert(t);
		}
		if (cmd.GetDefense())
			cmd.GetDefense().GetDangers(outList);
	}

	bool IsLZHot(AICommander_BaseComponent cmd, DCO_LogiJob job, DCO_GroupUtilityComponent teamGrp)
	{
		if (teamGrp && teamGrp.HasState(DCO_EGroupState.IN_CONTACT))
			return true;
		return IsNearThreat(cmd, job.m_vLZ, cmd.GetLogiLZSafeDist());
	}

	void BuildRoute(AICommander_BaseComponent cmd, vector from, vector to, notnull array<vector> outPts)
	{
		outPts.Clear();
		array<CMD_ThreatEntry> dangers = {};
		CollectDangers(cmd, dangers);
		if (!dangers.IsEmpty())
		{
			float safe = cmd.GetLogiLZSafeDist();
			vector seg = to - from;
			seg[1] = 0;
			float len = seg.Length();
			CMD_ThreatEntry pick;
			float pickT = float.MAX;
			vector pickClosest;
			if (len > 1)
			{
				vector dir = seg * (1 / len);
				foreach (CMD_ThreatEntry t : dangers)
				{
					if (!t)
						continue;
					float r = safe + t.m_fBelievedUncertainty;
					if (vector.DistanceXZ(from, t.m_vBelievedPos) < r || vector.DistanceXZ(to, t.m_vBelievedPos) < r)
						continue;

					vector rel = t.m_vBelievedPos - from;
					float along = rel[0] * dir[0] + rel[2] * dir[2];
					if (along <= 0 || along >= len)
						continue;

					vector closest = from + dir * along;
					if (vector.DistanceXZ(closest, t.m_vBelievedPos) < r && along < pickT)
					{
						pick = t;
						pickT = along;
						pickClosest = closest;
					}
				}
			}

			if (pick)
			{
				vector push = pickClosest - pick.m_vBelievedPos;
				push[1] = 0;
				if (push.LengthSq() < 1)
					push = Vector(-seg[2], 0, seg[0]);
				push.Normalize();
				outPts.Insert(Surface(pick.m_vBelievedPos + push * (safe + pick.m_fBelievedUncertainty + 75.0)));
			}
		}
		outPts.Insert(to);
	}

	void RequestEscort(AICommander_BaseComponent cmd, DCO_LogiJob job, vector pos, float now, bool risky = false)
	{
		if (!cmd.IsLogiEscort() || job.m_Escort)
			return;
		if (!risky && job.m_eKind != DCO_ELogiJob.MEDEVAC && job.m_eKind != DCO_ELogiJob.EXTRACT)
			return;

		DCO_GroupUtilityComponent esc = cmd.FindBestIdleGroupForTask_Public(DCO_EGroupTask.SUPPORT_BY_FIRE, pos, -1, true, 1);
		if (!esc)
			return;

		SCR_AIGroup grp = SCR_AIGroup.Cast(esc.GetOwner());
		SCR_AIWaypoint wp = cmd.SpawnMoveWP(pos);
		if (!grp || !wp)
			return;

		esc.SetTask(DCO_EGroupTask.SUPPORT_BY_FIRE);
		grp.CompleteAllWaypoints();
		esc.MoveTo(wp, now);
		job.m_Escort = esc;
		Log(job, "escort", "group=" + grp.GetName());
	}

	protected void ReleaseEscort(DCO_LogiJob job)
	{
		if (!job.m_Escort)
			return;
		if (job.m_Escort.GetTask() == DCO_EGroupTask.SUPPORT_BY_FIRE)
			job.m_Escort.SetTask(DCO_EGroupTask.NONE);
		job.m_Escort = null;
	}

	float EstimatePlayerTransport(AICommander_BaseComponent cmd, SCR_AIGroup pgrp, vector from, vector dest, out float pickup)
	{
		pickup = -1;
		DCO_LogiJob probe = new DCO_LogiJob();
		probe.m_eKind = DCO_ELogiJob.DEPLOY;
		probe.m_vPickup = from;
		probe.m_vDest = dest;
		probe.m_aPlayerGroups.Insert(pgrp);
		float eta;
		DCO_TransportTeamComponent team = BestTeamFor(cmd, probe, eta);
		if (!team)
			return -1;
		pickup = vector.Distance(team.GetTeamPos(), from) / 7.0;
		return eta;
	}

	bool CancelPlayerJob(SCR_AIGroup pgrp)
	{
		foreach (DCO_LogiJob j : m_aJobs)
		{
			if (!j.m_aPlayerGroups.Contains(pgrp))
				continue;

			Log(j, "cancel", "player_group=" + pgrp.GetGroupID());
			j.m_iRequester = -1;
			if (j.m_Team)
				j.m_Team.AbortJob("player_cancel", Now());
			else
				m_aJobs.RemoveItem(j);
			return true;
		}
		return false;
	}

	bool RequestRoleTransport(AICommander_BaseComponent cmd, int pid, SCR_AIGroup pgrp, vector from, vector dest)
	{
		if (!pgrp)
			return false;
		foreach (DCO_LogiJob j : m_aJobs)
		{
			if (j.m_aPlayerGroups.Contains(pgrp))
				return true;
		}

		DCO_LogiJob job = NewJob(DCO_ELogiJob.DEPLOY, from, dest, Now());
		job.m_aPlayerGroups.Insert(pgrp);
		job.m_bFromPlayer = true;
		job.m_bRolePriority = true;
		job.m_iRequester = pid;
		Log(job, "queue", "player_group=" + pgrp.GetGroupID() + " role=1");
		return true;
	}

	static void HandlePlayer(int pid, DCO_ELogiJob kind, vector target)
	{
		if (!Replication.IsServer())
			return;

		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		AICommander_BaseComponent cmd = DCO_PlayerContactReports.FindCommanderForPlayer(pid);
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(pid);
		if (!groups || !cmd || !player || !cmd.GetLogistics())
		{
			DCO_Radio.Player(pid, "LOGISTICS", "no_net");
			return;
		}

		cmd.GetLogistics().DoHandlePlayer(cmd, pid, groups.GetPlayerGroup(pid), player, kind, target);
	}

	protected void DoHandlePlayer(AICommander_BaseComponent cmd, int pid, SCR_AIGroup pgrp, IEntity player, DCO_ELogiJob kind, vector target)
	{
		float now = Now();
		vector ppos = player.GetOrigin();

		if (kind == DCO_ELogiJob.MEDEVAC)
		{
			array<IEntity> found = {};
			CollectDownNear(cmd, ppos, found);
			if (found.IsEmpty())
			{
				DCO_Radio.Player(pid, "LOGISTICS", "logi_no_casualties");
				return;
			}
			DCO_LogiJob job;
			foreach (IEntity c : found)
				job = AddCasualty(cmd, c, now, pid);
			job.m_bFromPlayer = true;
			job.m_iRequester = pid;
			DCO_Radio.Player(pid, "LOGISTICS", "logi_medevac_received", DCO_ERadioKind.INFO, DCO_Radio.P("count", found.Count().ToString()));
			return;
		}

		if (!pgrp)
			return;

		foreach (DCO_LogiJob j : m_aJobs)
		{
			if (j.m_aPlayerGroups.Contains(pgrp))
			{
				DCO_Radio.Player(pid, "LOGISTICS", "logi_already");
				return;
			}
		}

		vector dest = target;
		if (kind == DCO_ELogiJob.EXTRACT)
		{
			dest = cmd.GetHubFor(ppos, m_sHubSource);
		}
		else if (!IsValidDest(ppos, target))
		{
			DCO_Radio.Player(pid, "LOGISTICS", "logi_no_dest", DCO_ERadioKind.WARNING);
			DCO_BenchmarkLoggerComponent.Event(string.Format("logi_reject player=%1 reason=no_dest target=%2", pid, target));
			return;
		}

		DCO_LogiJob job = NewJob(kind, ppos, dest, now);
		job.m_aPlayerGroups.Insert(pgrp);
		job.m_bFromPlayer = true;
		job.m_iRequester = pid;
		Log(job, "queue", "player_group=" + pgrp.GetGroupID());
		DCO_Radio.Player(pid, "LOGISTICS", "logi_received", DCO_ERadioKind.INFO, DCO_Radio.P("kind", KindKey(kind), "grid", DCO_PlayerComms.Grid(dest)));
	}

	protected void CollectDownNear(AICommander_BaseComponent cmd, vector pos, notnull array<IEntity> outList)
	{
		FactionManager fm = GetGame().GetFactionManager();
		Faction mine;
		if (fm)
			mine = fm.GetFactionByKey(cmd.GetCommanderFactionKey());

		m_aQuery.Clear();
		DCO_Perf.Count("q:DCO_Logistics");
		GetGame().GetWorld().QueryEntitiesBySphere(pos, 100.0, QueryCallback, null, EQueryEntitiesFlags.DYNAMIC);
		foreach (IEntity e : m_aQuery)
		{
			SCR_ChimeraCharacter c = SCR_ChimeraCharacter.Cast(e);
			if (!c || !IsDown(c) || DCO_VehicleCombat.GetVehicle(c) || HasCasualty(c))
				continue;
			if (mine && c.GetFaction() && !mine.IsFactionFriendly(c.GetFaction()))
				continue;
			outList.Insert(c);
		}
		m_aQuery.Clear();
	}

	protected ref array<IEntity> m_aQuery = {};
	protected bool QueryCallback(IEntity e)
	{
		if (ChimeraCharacter.Cast(e))
			m_aQuery.Insert(e);
		return true;
	}

	static string KindKey(DCO_ELogiJob kind)
	{
		switch (kind)
		{
			case DCO_ELogiJob.MEDEVAC:		return "@kind_medevac";
			case DCO_ELogiJob.EXTRACT:		return "@kind_extract";
			case DCO_ELogiJob.REINFORCE:	return "@kind_reinforce";
			case DCO_ELogiJob.DEPLOY:		return "@kind_transport";
			case DCO_ELogiJob.REDEPLOY:		return "@kind_redeploy";
			case DCO_ELogiJob.RESUPPLY:		return "@kind_resupply";
		}
		return "@kind_logistics";
	}

	static string KindLabel(DCO_ELogiJob kind)
	{
		switch (kind)
		{
			case DCO_ELogiJob.MEDEVAC:		return "MEDEVAC";
			case DCO_ELogiJob.EXTRACT:		return "Extraction";
			case DCO_ELogiJob.REINFORCE:	return "Reinforcement lift";
			case DCO_ELogiJob.DEPLOY:		return "Transport";
			case DCO_ELogiJob.REDEPLOY:		return "Redeploy lift";
			case DCO_ELogiJob.RESUPPLY:		return "Resupply";
		}
		return "Logistics";
	}

	static void Log(DCO_LogiJob job, string what, string extra)
	{
		string line = string.Format("logi_%1 id=%2 kind=%3 %4", what, job.m_iId, typename.EnumToString(DCO_ELogiJob, job.m_eKind), extra);
		Print("[DCO_Logistics] " + line);
		DCO_BenchmarkLoggerComponent.Event(line);
	}

	void DrawDebug(notnull array<ref Shape> shapes, notnull array<ref DebugTextWorldSpace> texts, int flags)
	{
		shapes.Insert(Shape.CreateSphere(0xFF33AAFF, flags, m_vHub + "0 6 0", 6));
		texts.Insert(DCO_DebugDraw.SpawnText(m_vHub + "0 16 0", string.Format("HUB (%1)\nat hub: %2 casualties", m_sHubSource, m_aHubCasualties.Count()), 16, 0xFF33AAFF));

		foreach (DCO_LogiJob j : m_aJobs)
		{
			string label = string.Format("%1 #%2 seats %3", KindLabel(j.m_eKind), j.m_iId, j.SeatsNeeded());
			if (!j.m_Team)
			{
				texts.Insert(DCO_DebugDraw.SpawnText(j.m_vPickup + "0 10 0", label + "\nQUEUED", 14, 0xFFAAAAAA));
				continue;
			}

			vector tp = j.m_Team.GetTeamPos() + "0 3 0";
			shapes.Insert(Shape.CreateArrow(tp, j.m_vPickup + "0 3 0", 2, 0xFFFFCC00, flags));
			shapes.Insert(Shape.CreateArrow(j.m_vPickup + "0 3 0", j.m_vLZ + "0 3 0", 2, 0xFF33CC66, flags));
			shapes.Insert(Shape.CreateSphere(0xFF33CC66, flags, j.m_vLZ + "0 3 0", 4));
			if (vector.DistanceXZ(j.m_vAltLZ, j.m_vLZ) > 1)
				shapes.Insert(Shape.CreateSphere(0xFFFFCC00, flags, j.m_vAltLZ + "0 3 0", 3));
			texts.Insert(DCO_DebugDraw.SpawnText(tp + "0 8 0", string.Format("%1\n%2", label,
				typename.EnumToString(DCO_ETransportTeamState, j.m_Team.GetTeamState())), 14, 0xFFFFCC00));
		}
	}
}
