modded class AICommander_BaseComponent
{
	protected void ReleaseRearDefenders()
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;

		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (!g || g.IsPlayerGroup() || g.GetTask() != DCO_EGroupTask.DEFEND)
				continue;
			CMD_AICommanderObjectiveComponent obj = g.GetGroupObjective();
			if (!obj || !obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID) || IsObjectiveContested(obj))
				continue;

			float front = float.MAX;
			foreach (CMD_AICommanderObjectiveComponent other : mgr.m_aObjective)
			{
				if (other && !other.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
					front = Math.Min(front, vector.Distance(obj.GetOwner().GetOrigin(), other.GetOwner().GetOrigin()));
			}
			if (front < m_fLogiHubRearDist)
				continue;

			DetachFromObjective(g);
			g.CompleteAllWaypoints();
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("cmd=%1 All-in: rear defender %2 released from %3", m_sCommanderUID, g.GetOwner().GetName(), obj.GetLogName()));
		}
	}

	void StartObjectiveGarrison(DCO_GroupUtilityComponent grp, CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		if (!DCO_GroupJobs.Gate(this, grp, DCO_EGroupJob.GARRISON, "garrison"))
			return;
		if (TaskCountsSlot(grp.GetTask()) && grp.GetGroupObjective() == obj && obj.GetCurrentAssignedGroupCount(m_sFactionKey) > 0)
			obj.SetObjectiveGroup(m_sFactionKey, -1);
		DCO_GarrisonWaypoint wp = SpawnGarrisonWP(obj.GetOwner().GetOrigin(), ComputeThreatAngle(obj), obj.GetRadius());
		grp.CompleteAllWaypoints();
		grp.SetTask(DCO_EGroupTask.DEFEND, "AICommanderBase:1297");
		grp.SetGroupObjective(obj);
		if (wp)
			grp.MoveTo(wp, worldTime);
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
	        if (m_bDebugMode)
	            DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("cmd=%1 STALEMATE %2: no alternative objective -- holding position",
	                m_sCommanderUID, stalemateObj.GetLogName()));
	        return;
	    }

	    foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
	    {
	        if (!grp)
	            continue;

	        if (grp.GetGroupObjective() != stalemateObj)
	            continue;

	        if (grp.IsMoving())
	            continue;

	        if (grp.IsInContact())
	            continue;

	        if (!TaskCountsSlot(grp.GetTask()) || grp.IsInTransport())
	            continue;

	        grp.SetGroupObjective(altObj);
	        grp.SetTask(DCO_EGroupTask.ATTACK, "AICommanderBase:5233");

	        RandomGenerator rand = new RandomGenerator();
	        vector altPos = rand.GenerateRandomPointInRadius(m_fAltPosMinRadius, Math.Max(altObj.GetRadius(), m_fAltPosRadiusFloor), altObj.GetOwner().GetOrigin(), false);
	        altPos = DCO_Util.Surface(altPos);

	        grp.CompleteAllWaypoints();
	        if (!TryAssignTransport(grp, GetEntryPoint(altObj, grp.GetOwner().GetOrigin()), worldTime, DCO_EGroupTask.ATTACK))
	            SpawnMoveRoute(grp, grp.GetOwner().GetOrigin(), altPos, worldTime);

	        stalemateObj.SetObjectiveGroup(m_sFactionKey, -1);
	        altObj.SetObjectiveGroup(m_sFactionKey, 1);

	        break;
	    }
	}

	protected void ReleaseGroupsFromObjective(CMD_AICommanderObjectiveComponent obj)
	{
		if (!obj)
			return;

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || grp.IsPlayerGroup())
				continue;

			if (grp.GetGroupObjective() != obj)
				continue;

			if (!grp.CanCommanderOverrideRole() || grp.IsDedicatedTransport())
				continue;

			grp.CompleteAllWaypoints();
			grp.SetGroupObjective(null);
			grp.SetTask(DCO_EGroupTask.NONE, "AICommanderBase:5268");
		}

		ResetSyncState(obj);
	}

	protected void ReclaimStaleAssignments(float worldTime)
	{
		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || grp.IsPlayerGroup())
				continue;

			if (!grp.CanCommanderOverrideRole() || grp.IsDedicatedTransport())
				continue;

			DCO_EGroupTask task = grp.GetTask();

			if (task == DCO_EGroupTask.NONE
			 || task == DCO_EGroupTask.PATROL
			 || grp.GetSupportPlayerGroup() >= 0
			 || grp.IsInTransport()
			 || grp.HasState(DCO_EGroupState.RETREATING))
				continue;

			if (task == DCO_EGroupTask.DEFEND && !grp.GetGroupObjective())
				continue;

			if (grp.IsMoving())
				continue;

			if (grp.IsGroupHaveWaypoint())
				continue;

			CMD_AICommanderObjectiveComponent obj = grp.GetGroupObjective();

			bool stale;
			if (task == DCO_EGroupTask.DEFEND)
				stale = !obj || !obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID);
			else
				stale = !obj
					|| obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID)
					|| obj.CheckIsItLost(m_sFactionKey);

			if (!stale)
				continue;

			if (obj && TaskCountsSlot(task) && obj.GetCurrentAssignedGroupCount(m_sFactionKey) > 0)
				obj.SetObjectiveGroup(m_sFactionKey, -1);

			grp.SetGroupObjective(null);
			grp.SetTask(DCO_EGroupTask.NONE, "AICommanderBase:5319");
		}
	}

	protected float AngleDelta(float a, float b)
	{
		float twoPi = 2.0 * Math.PI;
		float d = a - b;

		while (d > Math.PI)
			d -= twoPi;

		while (d < -Math.PI)
			d += twoPi;

		return Math.AbsFloat(d);
	}

	protected float ComputeThreatAngle(CMD_AICommanderObjectiveComponent obj)
	{
		vector objPos = obj.GetOwner().GetOrigin();
		vector dir    = vector.Zero;

		if (threatComp)
		{
			array<ref CMD_ThreatEntry> threats = threatComp.GetThreats();
			if (threats)
			{
				foreach (CMD_ThreatEntry t : threats)
				{
					if (!t)
						continue;

					float dist = vector.Distance(t.m_vPosition, objPos);
					if (dist > m_fThreatDirectionRadius || dist < 1.0)
						continue;

					vector toThreat = t.m_vPosition - objPos;
					toThreat = Vector(toThreat[0], 0.0, toThreat[2]);
					toThreat = toThreat.Normalized();

					float weight = t.m_iEstimatedEnemyCount;
					if (weight < 1.0)
						weight = 1.0;

					dir = dir + toThreat * weight;
				}
			}
		}

		if (dir.LengthSq() < 0.001)
		{
			AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
			if (mgr)
			{
				float bestDistSq = float.MAX;
				foreach (CMD_AICommanderObjectiveComponent other : mgr.m_aObjective)
				{
					if (!other || other == obj)
						continue;

					if (other.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
						continue;

					vector otherPos = other.GetOwner().GetOrigin();
					float dsq = vector.DistanceSq(otherPos, objPos);
					if (dsq < bestDistSq)
					{
						bestDistSq = dsq;
						dir = otherPos - objPos;
					}
				}
			}
		}

		if (dir.LengthSq() < 0.001)
			dir = objPos - GetOwner().GetOrigin();

		dir = Vector(dir[0], 0.0, dir[2]);
		if (dir.LengthSq() < 0.001)
			return 0.0;

		return Math.Atan2(dir[2], dir[0]);
	}

	protected DCO_SectorGarrison PickNextSector(notnull array<ref DCO_SectorGarrison> sectors, int sectorCount, float sectorOffset, float threatAngle, CMD_AICommanderObjectiveComponent obj)
	{
		DCO_SectorGarrison best = null;
		float bestGap    = -1.0;
		float bestFacing = -2.0;
		bool learnExplore = DCO_LearnDefense.Explore(this, obj, "sector");
		float bestLearnBase = 0;
		float bestLearnMult = 1.0;
		float bestLearnN = 0;

		foreach (DCO_SectorGarrison cand : sectors)
		{
			if (!cand || cand.IsStaffed())
				continue;

			float candAngle = DCO_SectorMath.GetSectorMidAngle(cand.m_iSectorIndex, sectorCount, sectorOffset);

			float gap = Math.PI;
			foreach (DCO_SectorGarrison other : sectors)
			{
				if (!other || !other.IsStaffed())
					continue;

				float otherAngle = DCO_SectorMath.GetSectorMidAngle(other.m_iSectorIndex, sectorCount, sectorOffset);
				float d = AngleDelta(candAngle, otherAngle);
				if (d < gap)
					gap = d;
			}

			float learnN = 0;
			float learnMult = 1.0;
			if (!learnExplore)
				learnMult = DCO_LearnDefense.SectorMult(this, obj, candAngle, learnN);
			float facingBase = (Math.Cos(candAngle - threatAngle) + 1.0) * 0.5;
			float facing = facingBase * learnMult;

			bool better = false;
			if (gap > bestGap + 0.001)
				better = true;
			else if (Math.AbsFloat(gap - bestGap) <= 0.001 && facing > bestFacing)
				better = true;

			if (better)
			{
				best       = cand;
				bestGap    = gap;
				bestFacing = facing;
				bestLearnBase = facingBase;
				bestLearnMult = learnMult;
				bestLearnN = learnN;
			}
		}

		if (best && !learnExplore && obj)
		{
			float bestAngle = DCO_SectorMath.GetSectorMidAngle(best.m_iSectorIndex, sectorCount, sectorOffset);
			DCO_LearnDefense.LogUse(obj, "sector", DCO_LearnDefense.SectorName(DCO_LearnDefense.SectorOfDir(Vector(Math.Cos(bestAngle), 0, Math.Sin(bestAngle)))), bestLearnBase, bestLearnMult, bestLearnN);
		}
		return best;
	}

	protected void AssignGroupToSector(DCO_GroupUtilityComponent grp, CMD_AICommanderObjectiveComponent obj, DCO_SectorGarrison sec, float worldTime)
	{
		grp.CompleteAllWaypoints();
		grp.SetTask(DCO_EGroupTask.DEFEND, "AICommanderBase:5494");

		grp.SetGroupObjective(obj);

		grp.MoveTo(sec.m_Waypoint, worldTime);
		sec.m_Group = grp;
		int secCount = obj.GetSectorCount(m_sFactionKey);
		string secRange = "-";
		if (secCount > 0)
		{
			float step = 360.0 / secCount;
			float a0 = obj.GetSectorOffset(m_sFactionKey) * Math.RAD2DEG + sec.m_iSectorIndex * step;
			secRange = Math.Round(a0).ToString() + "-" + Math.Round(a0 + step).ToString();
		}
		vector secDir = sec.m_vPosition - obj.GetOwner().GetOrigin();
		DCO_Battle.Place(this, obj, AIGroup.Cast(grp.GetOwner()), "sector", sec.m_vPosition, Math.Atan2(secDir[0], secDir[2]) * Math.RAD2DEG, "-", "-", secRange, "-");
	}

	protected void UpdateObjectiveGarrison(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		array<DCO_GroupUtilityComponent> garrison;
		if (!m_mObjGarrison.Find(obj, garrison))
		{
			garrison = {};
			m_mObjGarrison.Insert(obj, garrison);
		}

		for (int i = garrison.Count() - 1; i >= 0; i--)
		{
			DCO_GroupUtilityComponent g = garrison[i];
			if (!g || g.GetGroupObjective() != obj || g.GetTask() != DCO_EGroupTask.DEFEND || !g.GetGarrisonActivity())
				garrison.Remove(i);
		}

		int friendlyNear, enemyNear;
		obj.CountNearbyUnitsCached(obj.GetRadius(), m_sFactionKey, friendlyNear, enemyNear);
		if (DCO_GlobalAIComponent.BehaviorOnCommander(this, DCO_EBehavior.COMBINED_ARMS))
		{
			vector sp = obj.GetOwner().GetOrigin();
			friendlyNear += DCO_Statics.CountManned(sp, obj.GetRadius() + m_fStaticsSearchMargin, m_sFactionKey) * m_iMannedStaticWeight;
			foreach (int gi, DCO_GroupUtilityComponent sg : garrison)
				sg.m_bDCOManStatics = gi == 0 && DCO_Statics.HasFree(sp, obj.GetRadius() + m_fStaticsSearchMargin, SCR_AIGroup.Cast(sg.GetOwner()));
		}
		if (enemyNear > 0 && enemyNear >= friendlyNear)
		{
			foreach (DCO_GroupUtilityComponent rg : garrison)
			{
				DCO_AIGarrisonActivity act = rg.GetGarrisonActivity();
				if (act && act.IsActive())
					act.ForceRelease();
			}
			return;
		}

		if (garrison.Count() >= DCO_Force.Max(this, "garrison"))
			return;

		DCO_GarrisonRegistry registry = DCO_GarrisonRegistry.GetInstance();
		if (!registry)
			return;

		vector objPos = obj.GetOwner().GetOrigin();
		DCO_GroupUtilityComponent grp = FindBestIdleGroupForTask(DCO_EGroupTask.DEFEND, objPos, false, null, -1, false, 0, DCO_EGroupJob.GARRISON);
		if (!grp || grp.IsPlayerGroup() || m_Frontline.IsReconGroup(grp))
			return;

		if (m_bGateDefendByManpower && !CanCommitGroup(grp))
			return;

		if (registry.CountFreeBuildings(objPos, obj.GetRadius(), AIGroup.Cast(grp.GetOwner())) <= 0)
			return;

		DCO_GarrisonWaypoint wp = SpawnGarrisonWP(objPos, ComputeThreatAngle(obj), obj.GetRadius());
		if (!wp)
			return;

		grp.CompleteAllWaypoints();
		grp.SetTask(DCO_EGroupTask.DEFEND, "AICommanderBase:5571");
		grp.SetGroupObjective(obj);
		grp.MoveTo(wp, worldTime);
		garrison.Insert(grp);
		DCO_Battle.Place(this, obj, AIGroup.Cast(grp.GetOwner()), "garrison_building", objPos, 90.0 - ComputeThreatAngle(obj) * Math.RAD2DEG, "-", "-", "-", "-");
	}

	protected void AssignDefendToObjective(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		if (!obj || !obj.GetOwner())
			return;

		float radius  = obj.GetRadius();
		if (radius <= 0.0)
			return;

		vector objPos = obj.GetOwner().GetOrigin();

		UpdateObjectiveGarrison(obj, worldTime);

		if (!obj.HasSectorGrid(m_sFactionKey))
		{
			float initialThreat = ComputeThreatAngle(obj);
			int   count  = DCO_SectorMath.ComputeSectorCount(radius, m_fArcPerSector, GetSectorPersonalityMod(), m_iMinSector, m_iMaxSector);
			float offset = DCO_SectorMath.ComputeSectorOffset(initialThreat, count);
			obj.InitSectorGrid(m_sFactionKey, count, offset);
		}

		array<ref DCO_SectorGarrison> sectors = obj.GetSectorGarrison(m_sFactionKey);
		if (!sectors || sectors.IsEmpty())
			return;

		int   sectorCount  = obj.GetSectorCount(m_sFactionKey);
		float sectorOffset = obj.GetSectorOffset(m_sFactionKey);
		float threatAngle  = DCO_LearnDefense.ThreatAngle(this, obj, ComputeThreatAngle(obj));

		float completionRadius = DCO_SectorMath.ComputeCompletionRadius(radius, m_fMaxCompletionRadius);
		float minSep           = completionRadius * 2.0;

		array<vector> taken = {};
		foreach (DCO_SectorGarrison s : sectors)
		{
			if (s && s.IsStaffed())
				taken.Insert(s.m_vPosition);
		}

		foreach (DCO_SectorGarrison sec : sectors)
		{
			if (!sec || !sec.NeedsReplenish())
				continue;

			DCO_GroupUtilityComponent grp = DCO_PickDefender(sec.m_vPosition);

			if (!grp)
				break;

			if (grp.IsPlayerGroup() || m_Frontline.IsReconGroup(grp))
				continue;

			AssignGroupToSector(grp, obj, sec, worldTime);
		}

		int garrisonCap = obj.GetDefendGroupCount();
		if (garrisonCap <= 0)
			garrisonCap = 1;

		int staffedNow = obj.GetStaffedSectorCount(m_sFactionKey);
		bool shortOn = DCO_DefShortfall.On(this);
		bool reserveUsed = false;
		string shortWhy = "";

		while (staffedNow < garrisonCap)
		{
			DCO_SectorGarrison target = PickNextSector(sectors, sectorCount, sectorOffset, threatAngle, obj);
			if (!target)
			{
				shortWhy = "no_sector";
				break;
			}

			DCO_GroupUtilityComponent grp = DCO_PickDefender(objPos);
			if (!grp || grp.IsPlayerGroup() || m_Frontline.IsReconGroup(grp))
			{
				shortWhy = "no_idle";
				break;
			}

			if (m_bGateDefendByManpower && !CanCommitGroup(grp))
			{
				if (!shortOn || reserveUsed)
				{
					shortWhy = "reserve_gate";
					break;
				}
				reserveUsed = true;
				DCO_Perf.Count("def:reinf_reserve");
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("def_reinf_reserve cmd=%1 obj=%2 grp=%3 staffed=%4 need=%5 -- reserve committed to a short defense before asking the spawner", m_sCommanderUID, obj.GetLogName(), grp.GetOwner().GetName(), staffedNow, garrisonCap));
			}

			vector pos;
			DCO_SectorMath.SamplePosition(objPos, radius, target.m_iSectorIndex, sectorCount, sectorOffset, minSep, taken, pos);

			SCR_AIWaypoint wp;
			if (DCO_FightPos.On() && DCO_Ctx.TuneForCmd(this, "def.sector_garrison", 1.0) > 0)
			{
				vector outDir = pos - objPos;
				wp = SpawnGarrisonWP(pos, Math.Atan2(outDir[2], outDir[0]), Math.Max(completionRadius, 30.0));
				DCO_Perf.Count("def:sector_garrison");
			}
			else
			{
				wp = SpawnDefendWP(pos);
				if (wp)
					wp.SetCompletionRadius(completionRadius);
			}
			if (!wp)
				break;

			target.m_vPosition = pos;
			target.m_Waypoint  = wp;
			taken.Insert(pos);

			AssignGroupToSector(grp, obj, target, worldTime);

			staffedNow = staffedNow + 1;
		}

		if (shortOn && staffedNow < garrisonCap && shortWhy != "no_sector")
			DCO_DefShortfallPlan(obj, garrisonCap, staffedNow, shortWhy, worldTime);
	}

	protected ref map<CMD_AICommanderObjectiveComponent, float> m_mDCOShortAt;
	protected bool m_bDCOShiftBusy;
	protected DCO_GroupUtilityComponent m_DCOShiftDonor;

	protected DCO_GroupUtilityComponent DCO_PickDefender(vector pos)
	{
		DCO_GroupUtilityComponent d = m_DCOShiftDonor;
		m_DCOShiftDonor = null;
		if (d && d.GetOwner() && d.GetUnitCount() > 0)
		{
			DCO_Perf.Count("def:reinf_shift_donor_used");
			return d;
		}
		return FindBestIdleGroupForTask(DCO_EGroupTask.DEFEND, pos);
	}

	protected void DCO_DefShortfallPlan(CMD_AICommanderObjectiveComponent obj, int cap, int staffed, string why, float worldTime)
	{
		if (m_bDCOShiftBusy)
			return;
		if (!m_mDCOShortAt)
			m_mDCOShortAt = new map<CMD_AICommanderObjectiveComponent, float>();
		if (m_mDCOShortAt.Count() > 64)
		{
			DCO_Perf.Count("def:short_map_prune");
			m_mDCOShortAt.Clear();
		}
		float next;
		if (m_mDCOShortAt.Find(obj, next) && worldTime < next)
			return;
		m_mDCOShortAt.Set(obj, worldTime + 30.0);
		vector objPos = obj.GetOwner().GetOrigin();
		float threatDist;
		float enemy;
		DCO_DefShortfall.ThreatNear(this, objPos, worldTime, threatDist, enemy);
		int need = cap;
		if (DCO_DefShortfall.FloorOn(this))
			need = Math.Max(need, DCO_DefShortfall.Floor(obj, enemy, false));
		int have = DCO_DefShortfall.Defenders(this, obj);
		if (have >= need)
			return;
		float eta = DCO_DefShortfall.SpawnerEtaS(this, obj, 0);
		DCO_GroupUtilityComponent donor;
		CMD_AICommanderObjectiveComponent donorObj;
		float donorTravel = 1e9;
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (!g || !g.GetOwner() || g.GetUnitCount() <= 0 || g.IsPlayerGroup() || g.IsDedicatedTransport() || g.IsMortar())
				continue;
			CMD_AICommanderObjectiveComponent go = g.GetGroupObjective();
			if (!go || go == obj || !go.GetOwner() || !go.IsCapturedBy(m_sFactionKey))
				continue;
			DCO_EGroupTask t = g.GetTask();
			if (t != DCO_EGroupTask.DEFEND && t != DCO_EGroupTask.GARRISON)
				continue;
			float gThreat;
			float gEnemy;
			DCO_DefShortfall.ThreatNear(this, go.GetOwner().GetOrigin(), worldTime, gThreat, gEnemy);
			if (gThreat < 500.0)
				continue;
			if (DCO_DefShortfall.Defenders(this, go) <= DCO_DefShortfall.Floor(go, gEnemy, false))
				continue;
			float travel = vector.DistanceXZ(g.GetOwner().GetOrigin(), objPos) / 2.0;
			if (travel < donorTravel)
			{
				donorTravel = travel;
				donor = g;
				donorObj = go;
			}
		}
		if (donor && (eta < 0 || donorTravel < eta))
		{
			DCO_Perf.Count("def:reinf_shift");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("def_reinf_shift cmd=%1 from=%2 to=%3 grp=%4 travel_s=%5 spawner_eta_s=%6 have=%7 need=%8 why=%9", m_sCommanderUID, donorObj.GetLogName(), obj.GetLogName(), donor.GetOwner().GetName(), Math.Round(donorTravel), Math.Round(eta), have, need, why));
			donor.CompleteAllWaypoints();
			donor.SetGroupObjective(null);
			donor.SetTask(DCO_EGroupTask.NONE, "DCO_DefShortfallShift");
			m_bDCOShiftBusy = true;
			m_DCOShiftDonor = donor;
			AssignDefendToObjective(obj, worldTime);
			bool used = m_DCOShiftDonor == null && donor.GetGroupObjective() == obj;
			m_DCOShiftDonor = null;
			m_bDCOShiftBusy = false;
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("def_reinf_shift_result cmd=%1 grp=%2 to=%3 assigned=%4", m_sCommanderUID, donor.GetOwner().GetName(), obj.GetLogName(), DCO_Util.B01(used)));
			if (!used)
				DCO_Perf.Count("def:reinf_shift_unassigned");
			return;
		}
		string skip = "no_donor";
		if (donor)
			skip = "donor_slower";
		DCO_Perf.Count("def:shortfall_plan");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("def_shortfall_plan cmd=%1 obj=%2 have=%3 need=%4 why=%5 shift=%6 donor_travel_s=%7 spawner_eta_s=%8", m_sCommanderUID, obj.GetLogName(), have, need, why, skip, Math.Round(Math.Min(donorTravel, 99999)), Math.Round(eta)));
		DCO_DefShortfall.Report(this, obj, need, have, "plan", 0, threatDist);
	}
}
