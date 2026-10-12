modded class AICommander_BaseComponent
{
	protected void TrySendToStaging(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
	    vector objPos  = obj.GetOwner().GetOrigin();
	    vector base    = GetOwner().GetOrigin();

	    vector stagingPos = GetOrCreateStagingPos(obj);

	    int attackSends = 1;
	    DCO_TacticPlan plan = m_Tactics.GetPlan(obj);
	    bool fireManeuver = plan && plan.m_eTactic == DCO_ETactic.FIRE_AND_MANEUVER;
	    int requiredGroups = GetRequiredAttackGroups(obj);
	    if (m_OpPlanner && m_OpPlanner.Main() == obj)
	        requiredGroups = Math.Max(requiredGroups, m_OpPlanner.NeedOf(obj));
	    int forceMax = DCO_Force.AttackMax(this);
	    requiredGroups = Math.Min(requiredGroups, forceMax);
	    int onObj = 0;
	    foreach (DCO_GroupUtilityComponent oo : m_aOwnedGroup)
	    {
	        if (oo && oo.GetGroupObjective() == obj && (oo.GetTask() == DCO_EGroupTask.ATTACK || oo.GetTask() == DCO_EGroupTask.SUPPORT_BY_FIRE))
	            onObj++;
	    }
	    int planNeed = Math.Ceil(1.0 * requiredGroups * m_fPlanNeedMargin);
	    if (planNeed > forceMax)
	        planNeed = forceMax;
	    if (fireManeuver)
	    {
	        attackSends = Math.Max(0, planNeed - onObj);
	        if (onObj == 0)
	            attackSends = Math.Max(2, attackSends);
	        DCO_Perf.Count("plan_size");
	        DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("plan_size cmd=%1 obj=%2 need=%3 sent=%4 groups=%5 required=%6", m_sCommanderUID, obj.GetLogName(), planNeed, attackSends, onObj, requiredGroups));
	    }
	    DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("staging_sizing cmd=%1 obj=%2 required=%3 sends=%4 assigned=%5 est_def=%6 target_ratio=%7 reserve_floor=%8 all_in=%9", m_sCommanderUID, obj.GetLogName(), requiredGroups, attackSends, obj.GetCurrentAssignedGroupCount(m_sFactionKey), m_Ops.EstimateDefenders(obj).ToString(-1, 1), m_Ops.GetTargetRatio(this, obj).ToString(-1, 2), Math.Round(GetReserveFloor()), m_bAllIn) + " tactic=" + DCO_TacticNameOf(plan));

	    for (int sendIdx = 0; sendIdx < attackSends; sendIdx++)
	    {
	        DCO_GroupUtilityComponent assaultGrp = FindBestIdleGroupForTask(DCO_EGroupTask.ATTACK, objPos);
	        if (!assaultGrp)
	            break;
	        if (assaultGrp.IsPlayerGroup())
	            return;
	        bool tacticMin = sendIdx < 2 && DCO_TacticTest.Forces(this, obj);
	        if (!tacticMin && !CanCommitGroup(assaultGrp))
	            break;
	        if (tacticMin)
	            DCO_Perf.Count("staging_sizing:tactic_min");

	        int alreadyStaged = 0;
	        foreach (DCO_GroupUtilityComponent staged : m_aOwnedGroup)
	        {
	            if (staged && staged != assaultGrp && staged.GetGroupObjective() == obj && staged.GetTask() == DCO_EGroupTask.ATTACK)
	                alreadyStaged++;
	        }
	        vector toObj = objPos - stagingPos;
	        toObj[1] = 0;
	        toObj.Normalize();
	        vector slotSide = Vector(-toObj[2], 0.0, toObj[0]);
	        float slotOffset = m_fStagingSlotSpacing * ((alreadyStaged + 1) / 2);
	        if (alreadyStaged % 2 == 1)
	            slotOffset = -slotOffset;
	        vector groupStaging = DCO_Util.Surface(stagingPos + slotSide * slotOffset);

	        assaultGrp.CompleteAllWaypoints();
	        bool viaTransport = TryAssignTransport(assaultGrp, groupStaging, worldTime, DCO_EGroupTask.ATTACK);
	        if (viaTransport || SpawnMoveRoute(assaultGrp, assaultGrp.GetOwner().GetOrigin(), groupStaging, worldTime))
	        {
	            if (!viaTransport)
	                assaultGrp.SetTask(DCO_EGroupTask.ATTACK, "AICommanderBase:4745");
	            if (assaultGrp.GetGroupObjective() != obj)
	            {
	                assaultGrp.SetGroupObjective(obj);
	                obj.SetObjectiveGroup(m_sFactionKey, 1);
	            }
	            TryAttachArmorToAssault(obj, stagingPos, worldTime);
	        }
	        else
	            break;
	    }

	    if (fireManeuver)
	        return;

	    if (DCO_GroupsOnObj(obj) >= forceMax)
	    {
	        DCO_Perf.Count("force:attack_full_staging");
	        DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("force_cap cmd=%1 mission=attack wanted=%2 cap=%3 path=staging_extra obj=%4", m_sCommanderUID, DCO_GroupsOnObj(obj) + 1, forceMax, obj.GetLogName()));
	        return;
	    }

	    if (obj.GetRequiredGroupCount() >= 2 && !ObjectiveHasSuppressGroup(obj))
	    {
	        DCO_GroupUtilityComponent sbf = FindBestIdleGroupForTask(DCO_EGroupTask.SUPPORT_BY_FIRE, objPos, false, obj);
	        if (sbf && !sbf.IsPlayerGroup() && CanCommitGroup(sbf))
	        {
	            vector sbfPos = m_Ops.FindOverwatch(obj, sbf.GetOwner().GetOrigin(), -1, -1, -1, GetCommanderFactionKey());
	            if (sbfPos != vector.Zero)
	            {
	                sbf.CompleteAllWaypoints();
	                sbf.SetGroupObjective(obj);
	                if (!TryAssignTransport(sbf, sbfPos, worldTime, DCO_EGroupTask.SUPPORT_BY_FIRE))
	                {
	                    sbf.SetTask(DCO_EGroupTask.SUPPORT_BY_FIRE, "AICommanderBase:4772");
	                    SpawnMoveRoute(sbf, sbf.GetOwner().GetOrigin(), sbfPos, worldTime);
	                }
	                SCR_AIWaypoint sup = SpawnSuppressWP(objPos);
	                if (sup)
	                    sbf.MoveTo(sup, worldTime);
	            }
	        }
	    }

	    int flankSends = 1;
	    if (plan && plan.m_eTactic == DCO_ETactic.PINCER)
	        flankSends = 2;
	    for (int flankIdx = 0; flankIdx < flankSends; flankIdx++)
	    {
	    DCO_GroupUtilityComponent flankGrp;
	    if (DCO_GroupsOnObj(obj) < forceMax)
	        flankGrp = FindBestIdleGroupForTask(DCO_EGroupTask.FLANK, objPos);
	    if (!flankGrp)
	        break;
	    if (flankGrp)
	    {
			if (flankGrp.IsPlayerGroup())
			{
				return;
			}
			if (!CanCommitGroup(flankGrp) && !DCO_TacticTest.Forces(this, obj))
			{
				return;
			}
			flankGrp.CompleteAllWaypoints();

			float  stagingDist  = vector.Distance(stagingPos, objPos);
			bool   swing        = m_Tactics.AllowsFlankSwing(obj);
			int    pincerSide   = m_Tactics.NextFlankSide(obj);
	        vector flankStaging;
			if (!swing)
				flankStaging = FlankPoint(stagingPos, objPos, stagingDist, m_fFlankStagingAngle, 1);
			else if (pincerSide != 0)
				flankStaging = FlankPoint(stagingPos, objPos, stagingDist, 90.0, pincerSide);
			else
				flankStaging = ComputeFlankPosition(stagingPos, objPos, stagingDist);

			DCO_EGroupTask flankTask = DCO_EGroupTask.FLANK;
			if (!swing)
				flankTask = DCO_EGroupTask.ATTACK;

			bool flankViaTransport = TryAssignTransport(flankGrp, flankStaging, worldTime, flankTask);
			if (flankViaTransport || SpawnMoveRoute(flankGrp, flankGrp.GetOwner().GetOrigin(), flankStaging, worldTime))
	        {
	            if (!flankViaTransport)
	                flankGrp.SetTask(flankTask, "AICommanderBase:4814");
	            if (flankGrp.GetGroupObjective() != obj)
	            {
	                flankGrp.SetGroupObjective(obj);
	                obj.SetObjectiveGroup(m_sFactionKey, 1);
	            }
	            DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("tactic_flank_staging cmd=%1 obj=%2 grp=%3 tactic=%4 side=%5 swing=%6", m_sCommanderUID, obj.GetLogName(), flankGrp.GetOwner().GetName(), DCO_TacticNameOf(plan), pincerSide, swing));
	        }
	    }
	    }
	}

	protected bool ObjectiveHasSuppressGroup(CMD_AICommanderObjectiveComponent obj)
	{
		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp)
				continue;

			if (grp.GetTask() == DCO_EGroupTask.SUPPORT_BY_FIRE && grp.GetGroupObjective() == obj)
				return true;
		}
		return false;
	}

	protected void TryGatherForSynchronizedAssault(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		vector objPos = obj.GetOwner().GetOrigin();

		vector stagingPos = GetOrCreateStagingPos(obj);

		foreach (DCO_GroupUtilityComponent staged : m_aOwnedGroup)
		{
			if (!staged || staged.GetGroupObjective() != obj || staged.GetTask() != DCO_EGroupTask.ATTACK)
				continue;

			if (staged.IsMoving() || staged.IsGroupHaveWaypoint())
				continue;

			if (vector.Distance(staged.GetOwner().GetOrigin(), stagingPos) <= m_fStagingArriveRadius)
				continue;

			if (!TryAssignTransport(staged, stagingPos, worldTime, DCO_EGroupTask.ATTACK))
				SpawnMoveRoute(staged, staged.GetOwner().GetOrigin(), stagingPos, worldTime);
		}

		int required = GetRequiredAttackGroups(obj);

		while (obj.GetCurrentAssignedGroupCount(m_sFactionKey) < required)
		{
			DCO_GroupUtilityComponent assaultGrp = FindBestIdleGroupForTask(DCO_EGroupTask.ATTACK, objPos);
			if (!assaultGrp)
				break;

			if (assaultGrp.IsPlayerGroup())
				break;

			if (!CanCommitGroup(assaultGrp))
				break;

			assaultGrp.CompleteAllWaypoints();

			if (TryAssignTransport(assaultGrp, stagingPos, worldTime, DCO_EGroupTask.ATTACK))
			{
				if (assaultGrp.GetGroupObjective() != obj)
				{
					assaultGrp.SetGroupObjective(obj);
					obj.SetObjectiveGroup(m_sFactionKey, 1);
				}
				continue;
			}

			if (!SpawnMoveRoute(assaultGrp, assaultGrp.GetOwner().GetOrigin(), stagingPos, worldTime))
				break;

			assaultGrp.SetTask(DCO_EGroupTask.ATTACK, "AICommanderBase:6229");
			if (assaultGrp.GetGroupObjective() != obj)
			{
				assaultGrp.SetGroupObjective(obj);
				obj.SetObjectiveGroup(m_sFactionKey, 1);
			}
		}

		TryAttachArmorToAssault(obj, stagingPos, worldTime);
	}

	protected DCO_GroupUtilityComponent DCO_IdleArmedSoft(CMD_AICommanderObjectiveComponent obj)
	{
		vector objPos = obj.GetOwner().GetOrigin();
		DCO_GroupUtilityComponent best;
		float bestD = m_fIdleArmedSearch;
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (!g || g.IsPlayerGroup() || g.IsArmor())
				continue;
			if (g.GetGroupObjective() == obj && DCO_ArmorPair.ArmedCrew(g))
				return null;
		}
		foreach (DCO_GroupUtilityComponent g2 : m_aOwnedGroup)
		{
			if (!g2 || g2.IsPlayerGroup() || g2.IsArmor() || g2.IsDedicatedTransport() || g2.GetGroupObjective())
				continue;
			if (g2.GetTask() != DCO_EGroupTask.NONE && g2.GetTask() != DCO_EGroupTask.PATROL)
				continue;
			if (!DCO_ArmorPair.ArmedCrew(g2))
				continue;
			float d = vector.DistanceXZ(g2.GetOwner().GetOrigin(), objPos);
			if (d < bestD)
			{
				bestD = d;
				best = g2;
			}
		}
		if (best)
		{
			DCO_Perf.Count("veh_sbf:soft_armed");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("veh_sbf_soft grp=%1 obj=%2 dist=%3", best.GetOwner().GetName(), obj.GetLogName(), Math.Round(bestD)));
		}
		return best;
	}

	protected void TryAttachArmorToAssault(CMD_AICommanderObjectiveComponent obj, vector stagingPos, float worldTime)
	{
		int attached = 0;
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (g && g.IsArmor() && g.GetGroupObjective() == obj)
				attached++;
		}

		if (attached >= 2)
			return;

		DCO_GroupUtilityComponent armor = FindBestIdleGroupForTask(DCO_EGroupTask.ATTACK, obj.GetOwner().GetOrigin(), false, null, -1, true);
		if (!armor)
			armor = DCO_IdleArmedSoft(obj);
		if (!armor || armor.IsPlayerGroup())
			return;

		vector objPos = obj.GetOwner().GetOrigin();
		vector centroid = DCO_AttackerCentroid();
		float sideDeg;
		bool pathClear;
		int losHits;
		int losTests;
		string losAim = "none";
		string action = "sbf";
		vector overwatch = vector.Zero;
		if (overwatch != vector.Zero)
			StagingSideOk(objPos, obj.GetRadius(), centroid, overwatch, sideDeg, pathClear);
		else
		{
			overwatch = DCO_VehicleStagingOverwatch(objPos, stagingPos);
			action = "overwatch_staging";
			if (overwatch == vector.Zero)
			{
				DCO_Perf.Count("veh_sbf:no_site_reserve");
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("veh_sbf grp=%1 obj=%2 state=no_site action=reserve plan_owned=%3", armor.GetOwner().GetName(), obj.GetLogName(), DCO_Util.B01(m_Tactics != null && m_Tactics.ControlsObjective(obj))));
				return;
			}
			DCO_Perf.Count("veh_sbf:no_site_staging");
			if (!m_aDCOVehOw.Contains(armor))
			{
				m_aDCOVehOw.Insert(armor);
				m_aDCOVehOwObj.Insert(obj);
			}
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("veh_sbf grp=%1 obj=%2 state=no_site action=overwatch_staging pos=%3 dist_staging=%4 dist_obj=%5", armor.GetOwner().GetName(), obj.GetLogName(), DCO_Util.XZ(overwatch), Math.Round(vector.DistanceXZ(overwatch, stagingPos)), Math.Round(vector.DistanceXZ(overwatch, objPos))));
		}

		armor.CompleteAllWaypoints();
		if (!SpawnMoveRoute(armor, armor.GetOwner().GetOrigin(), overwatch, worldTime))
		{
			DCO_Perf.Count("veh_sbf_fail:no_route");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), "veh_sbf_fail grp=" + armor.GetOwner().GetName() + " obj=" + obj.GetLogName() + " reason=no_route");
			return;
		}
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("veh_sbf grp=%1 obj=%2 pos=%3 dist=%4 los=%5/%6 aim=%7 side_deg=%8 state=sent", armor.GetOwner().GetName(), obj.GetLogName(), DCO_Util.XZ(overwatch), Math.Round(vector.DistanceXZ(overwatch, objPos)), losHits, losTests, losAim, Math.Round(sideDeg)) + " action=" + action);

		armor.SetTask(DCO_EGroupTask.SUPPORT_BY_FIRE, "AICommanderBase:6298");
		armor.SetGroupObjective(obj);
		obj.SetObjectiveGroup(m_sFactionKey, 1);
		m_Ops.RegisterArmor(this, armor, obj, overwatch);
	}

	protected void DCO_VehOwTick(float worldTime)
	{
		for (int i = m_aDCOVehOw.Count() - 1; i >= 0; i--)
		{
			DCO_GroupUtilityComponent v = m_aDCOVehOw[i];
			CMD_AICommanderObjectiveComponent obj = m_aDCOVehOwObj[i];
			if (!v || !v.GetOwner() || v.GetUnitCount() <= 0 || !obj || obj.IsCapturedBy(m_sFactionKey) || v.GetGroupObjective() != obj)
			{
				m_aDCOVehOw.Remove(i);
				m_aDCOVehOwObj.Remove(i);
				continue;
			}
			float at;
			if (m_mDCOVehOwAt.Find(v, at) && worldTime - at < m_fVehOwRecheck_s)
				continue;
			m_mDCOVehOwAt.Set(v, worldTime);
			vector pos = v.GetOwner().GetOrigin();
			vector objPos = obj.GetOwner().GetOrigin();
			float edge = obj.GetRadius();
			bool tgt = false;
			SCR_AIGroupUtilityComponent gu = v.GetGroupUtilityComponent();
			if (gu && gu.m_Perception)
			{
				foreach (SCR_AIGroupTargetCluster c : gu.m_Perception.m_aTargetClusters)
				{
					if (c && c.m_State && c.m_State.m_iCountAlive > 0)
						tgt = true;
				}
			}
			int losHits;
			int losTests;
			string losAim = "none";
			vector site = vector.Zero;
			bool losHere = site != vector.Zero && vector.DistanceXZ(site, pos) <= m_fVehOwSiteTolerance;
			string action = "hold";
			if (tgt)
				action = "hold";
			else if (site != vector.Zero && !losHere)
			{
				action = "search";
				v.CompleteAllWaypoints();
				SpawnMoveRoute(v, pos, site, worldTime);
				m_Ops.MoveArmorOverwatch(v, site);
			}
			else if (site == vector.Zero && IsAssaultReleased(obj))
			{
				vector assault = DCO_AttackerCentroid();
				vector dir = DCO_Specialist.Flat(assault - objPos);
				float keep = Math.Max(edge + m_fVehOwEdgeKeep, vector.DistanceXZ(assault, objPos) + m_fVehOwBehindAssault);
				vector follow = DCO_Util.Surface(objPos + dir * keep);
				if (vector.DistanceXZ(follow, pos) > 60.0)
				{
					action = "move";
					v.CompleteAllWaypoints();
					SpawnMoveRoute(v, pos, follow, worldTime);
					m_Ops.MoveArmorOverwatch(v, follow);
				}
			}
			DCO_Perf.Count("veh_ow_tick:" + action);
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("veh_ow_tick grp=%1 obj=%2 los=%3/%4 tgt=%5 action=%6 site=%7 dist_edge=%8", v.GetOwner().GetName(), obj.GetLogName(), losHits, losTests, DCO_Util.B01(tgt), action, DCO_Util.XZ(site), Math.Round(vector.DistanceXZ(pos, objPos) - edge)));
		}
	}

	protected void DCO_ScopeFixRole(DCO_GroupUtilityComponent g, CMD_AICommanderObjectiveComponent obj, vector site, float worldTime)
	{
		string result = "no_route";
		g.CompleteAllWaypoints();
		if (SpawnMoveRoute(g, g.GetOwner().GetOrigin(), site, worldTime))
		{
			m_mDCOScopeGoAt.Remove(g);
			g.SetTask(DCO_EGroupTask.SUPPORT_BY_FIRE, "AICommanderBase:scope_fix");
			g.SetGroupObjective(obj);
			result = "move_sbf";
		}
		DCO_Perf.Count("op_fix_role:" + result);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("op_fix_role cmd=%1 grp=%2 obj=%3 site=%4 result=%5", m_sCommanderUID, g.GetOwner().GetName(), obj.GetLogName(), DCO_Util.XZ(site), result));
	}

	protected void DCO_ScopeFixOpen(CMD_AICommanderObjectiveComponent obj, DCO_GroupUtilityComponent g, float worldTime)
	{
		float goAt;
		if (!m_mDCOScopeGoAt.Find(g, goAt))
		{
			if (!g.m_bDCOTRReady)
				return;
			m_mDCOScopeGoAt.Set(g, worldTime);
			g.DCO_TRSignal(DCO_ETRSignal.GO);
			DCO_Perf.Count("op_fix_open");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("op_fix_open cmd=%1 grp=%2 obj=%3 signal=GO", m_sCommanderUID, g.GetOwner().GetName(), obj.GetLogName()));
			return;
		}
		if (goAt > 0 && worldTime - goAt >= m_fScopeFixOpenDelay_s)
		{
			m_mDCOScopeGoAt.Set(g, -1);
			g.DCO_TRSignal(DCO_ETRSignal.SUSTAIN);
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("op_fix_open cmd=%1 grp=%2 obj=%3 signal=SUSTAIN", m_sCommanderUID, g.GetOwner().GetName(), obj.GetLogName()));
		}
	}

	bool DCO_IsScopeFix(DCO_GroupUtilityComponent g)
	{
		foreach (CMD_AICommanderObjectiveComponent o, array<DCO_GroupUtilityComponent> l : m_mDCOScopeFix)
		{
			if (l && l.Contains(g))
				return true;
		}
		return false;
	}

	protected void DCO_ScopeFixTend(CMD_AICommanderObjectiveComponent obj, DCO_GroupUtilityComponent g, float worldTime)
	{
		SCR_AIGroupUtilityComponent gu = g.GetGroupUtilityComponent();
		if (gu && gu.DCO_GetPosture() != DCO_GroupTactics.BALANCE)
			gu.DCO_ForcePosture(DCO_GroupTactics.BALANCE, GetGame().GetWorld().GetWorldTime(), m_fScopeFixPosture_ms, false, "scope_fix_sbf");
		vector site;
		if (!m_mDCOScopeSite.Find(g, site) || site == vector.Zero)
			return;
		float d = vector.DistanceXZ(g.GetOwner().GetOrigin(), site);
		if (d <= m_fScopeFixArrive)
		{
			m_mDCOScopeBest.Remove(g);
			DCO_ScopeFixOpen(obj, g, worldTime);
			return;
		}
		float best;
		if (!m_mDCOScopeBest.Find(g, best) || d < best - m_fScopeFixProgress)
		{
			m_mDCOScopeBest.Set(g, d);
			m_mDCOScopeBestAt.Set(g, worldTime);
			return;
		}
		float bestAt = m_mDCOScopeBestAt.Get(g);
		if (worldTime - bestAt < m_fScopeFixStall_s || g.IsInContact())
			return;
		m_mDCOScopeBestAt.Set(g, worldTime);
		m_mDCOScopeBest.Set(g, d);
		string act = "-";
		if (gu && gu.GetCurrentAction())
			act = gu.GetCurrentAction().ClassName();
		g.CompleteAllWaypoints();
		SpawnMoveRoute(g, g.GetOwner().GetOrigin(), site, worldTime);
		g.SetTask(DCO_EGroupTask.SUPPORT_BY_FIRE, "AICommanderBase:scope_fix_stall");
		DCO_ScopeFixRole(g, obj, site, worldTime);
		DCO_Perf.Count("op_fix_stall_reissue");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("op_fix_stall_reissue cmd=%1 grp=%2 obj=%3 dist_site=%4 stalled_s=%5 cause=%6", m_sCommanderUID, g.GetOwner().GetName(), obj.GetLogName(), Math.Round(d), Math.Round(worldTime - bestAt), act));
	}

	string DCO_ScopeFix(CMD_AICommanderObjectiveComponent obj, int want)
	{
		array<DCO_GroupUtilityComponent> list = m_mDCOScopeFix.Get(obj);
		if (!list)
		{
			list = {};
			m_mDCOScopeFix.Set(obj, list);
		}
		for (int i = list.Count() - 1; i >= 0; i--)
		{
			DCO_GroupUtilityComponent g = list[i];
			if (!g || !g.GetOwner() || g.GetUnitCount() <= 0 || g.GetGroupObjective() != obj || g.GetTask() != DCO_EGroupTask.SUPPORT_BY_FIRE)
				list.Remove(i);
		}
		foreach (DCO_GroupUtilityComponent tg : list)
			DCO_ScopeFixTend(obj, tg, DCO_Util.NowS());
		float worldTime = DCO_Util.NowS();
		vector objPos = obj.GetOwner().GetOrigin();
		float edge = obj.GetRadius();
		int guard = 0;
		while (list.Count() < want && guard < m_iScopeFixTries)
		{
			guard++;
			DCO_GroupUtilityComponent f = FindBestIdleGroupForTask(DCO_EGroupTask.NONE, objPos, false, obj);
			if (!f && DCO_Ctx.TuneForCmd(this, "veh.sbf", 0) > 0)
				f = FindBestIdleGroupForTask(DCO_EGroupTask.NONE, objPos, false, obj, -1, true);
			if (!f || f.IsPlayerGroup())
				break;
			float owMin = m_fScopeFixOwMin;
			float owMax = m_fScopeFixOwMax;
			float owIdeal = m_fScopeFixOwIdeal;
			vector vehPos = vector.Zero;
			if (f.IsArmor() || f.GetGroupVehicle())
			{
				int vcls = DCO_VehicleActive.VehClass(f.GetGroupVehicle());
				bool vehOk = DCO_Ctx.TuneForCmd(this, "veh.sbf", 0) > 0 && vcls > 0 && DCO_ArmorPair.ArmedCrew(f) && DCO_CommanderTactics.HasGunner(f.GetGroupVehicle());
				if (vehOk && m_Ops.GetIntel(obj).m_bArmor && vcls < 3)
					vehOk = false;
				if (!vehOk)
					break;
				owMin = DCO_Ctx.TuneForCmd(this, "veh.sbf_apc_min_m", 300.0);
				owMax = DCO_Ctx.TuneForCmd(this, "veh.sbf_apc_max_m", 550.0);
				if (vcls == 3)
				{
					owMin = DCO_Ctx.TuneForCmd(this, "veh.sbf_tank_min_m", 350.0);
					owMax = DCO_Ctx.TuneForCmd(this, "veh.sbf_tank_max_m", 650.0);
				}
				else if (vcls == 1)
				{
					owMin = DCO_Ctx.TuneForCmd(this, "veh.sbf_soft_min_m", 450.0);
					owMax = DCO_Ctx.TuneForCmd(this, "veh.sbf_soft_max_m", 700.0);
				}
				owIdeal = (owMin + owMax) * 0.5;
				if (!DCO_RoleOnce(obj.GetLogName() + "|vsbf|" + f.GetOwner().GetName(), 60.0))
					break;
				string vWhy;
				string vBand;
				string vAt;
				vehPos = m_Tactics.VehSbfSite(this, obj, f, vcls, vWhy, vBand, vAt);
				if (vehPos == vector.Zero)
				{
					DCO_Perf.Count("veh_sbf:skip_" + vWhy);
					DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("veh_sbf_skip path=scope_fix obj=%1 veh=%2 class=%3 reason=%4 at=%5 band=%6", obj.GetLogName(), f.GetOwner().GetName(), DCO_VehicleActive.ClassName(vcls), vWhy, vAt, vBand));
					break;
				}
				DCO_Perf.Count("veh_sbf:scope_fix_" + DCO_VehicleActive.ClassName(vcls));
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("veh_sbf_pick path=scope_fix obj=%1 veh=%2 class=%3 band=%4 at=%5 los_ok=1 pos=%6", obj.GetLogName(), f.GetOwner().GetName(), DCO_VehicleActive.ClassName(vcls), vBand, vAt, DCO_Util.XZ(vehPos)));
			}
			vector pos = vehPos;
			if (pos == vector.Zero)
				pos = m_Ops.FindOverwatch(obj, f.GetOwner().GetOrigin(), edge + owMin, edge + owMax, edge + owIdeal, GetCommanderFactionKey());
			if (pos == vector.Zero)
			{
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("op_fix_send cmd=%1 obj=%2 grp=%3 result=no_overwatch", m_sCommanderUID, obj.GetLogName(), f.GetOwner().GetName()));
				break;
			}
			if (!TacticCommit(f, obj, DCO_EGroupTask.SUPPORT_BY_FIRE, pos, worldTime))
				break;
			list.Insert(f);
			m_mDCOScopeSite.Set(f, pos);
			DCO_ScopeFixRole(f, obj, pos, worldTime);
			DCO_Perf.Count("op_fix_send");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("op_fix_send cmd=%1 obj=%2 grp=%3 pos=%4 dist_edge=%5 mg=%6", m_sCommanderUID, obj.GetLogName(), f.GetOwner().GetName(), DCO_Util.XZ(pos), Math.Round(vector.DistanceXZ(pos, objPos) - edge), DCO_Util.B01(f.HasMG())));
		}
		DCO_ScopeFixFires(obj, list, worldTime);
		string names = "";
		foreach (DCO_GroupUtilityComponent n : list)
		{
			if (!names.IsEmpty())
				names += "+";
			names += n.GetOwner().GetName();
		}
		if (names.IsEmpty())
			names = "none";
		return names;
	}

	protected void DCO_ScopeFixFires(CMD_AICommanderObjectiveComponent obj, array<DCO_GroupUtilityComponent> fixes, float worldTime)
	{
		if (fixes.IsEmpty() || !artySupport || !artySupport.HasRegisteredUnits())
			return;
		float at;
		if (m_mDCOScopeFixFireAt.Find(obj, at) && worldTime - at < 60.0)
			return;
		vector objPos = obj.GetOwner().GetOrigin();
		vector target = vector.Zero;
		float bestUnc = obj.GetRadius();
		if (threatComp)
		{
			foreach (CMD_ThreatEntry t : threatComp.GetThreats())
			{
				if (!t || vector.DistanceXZ(t.m_vBelievedPos, objPos) > obj.GetRadius() + m_fScopeFixIntelMargin || t.m_fBelievedUncertainty >= bestUnc)
					continue;
				bestUnc = t.m_fBelievedUncertainty;
				target = t.m_vBelievedPos;
			}
		}
		if (target == vector.Zero)
			return;
		m_mDCOScopeFixFireAt.Set(obj, worldTime);
		int tubes = Math.Max(1, artySupport.DCO_UnitCount());
		CMD_FireMissionRequest req = new CMD_FireMissionRequest(target, SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE, worldTime, tubes);
		req.m_sSource = "scope_fix";
		if (artySupport.RequestShellImpact(req, worldTime, tubes))
		{
			DCO_Perf.Count("fm_fires:fix_suppress");
			DCO_Log.Write(DCO_ELogChannel.TACTIC, DCO_ELogLevel.INFO, "-", string.Format("fm_fires obj=%1 phase=fix_suppress tube=%2 rounds=%3 target=%4", obj.GetLogName(), tubes, tubes, DCO_Util.XZ(target)));
		}
	}

	DCO_GroupUtilityComponent DCO_TakeScopeFix(CMD_AICommanderObjectiveComponent obj)
	{
		array<DCO_GroupUtilityComponent> list = m_mDCOScopeFix.Get(obj);
		if (!list)
			return null;
		foreach (DCO_GroupUtilityComponent g : list)
		{
			if (g && g.GetOwner() && g.GetUnitCount() > 0 && g.GetGroupObjective() == obj)
			{
				list.RemoveItem(g);
				return g;
			}
		}
		return null;
	}

	protected vector DCO_VehicleStagingOverwatch(vector objPos, vector stagingPos)
	{
		if (stagingPos == vector.Zero)
			return vector.Zero;
		vector back = DCO_Specialist.Flat(stagingPos - objPos);
		array<float> offs = {150.0, 100.0, 250.0, 300.0};
		foreach (float o : offs)
		{
			vector c = DCO_Util.Surface(stagingPos + back * o);
			if (!IsRoutePointUsable(c))
				continue;
			float frac = DCO_Util.TraceFrac(c + Vector(0, m_fVehicleEyeHeight, 0), stagingPos + Vector(0, m_fStagingAimHeight, 0), null, TraceFlags.WORLD | TraceFlags.ENTS, -1, "t:veh_staging_ow");
			if (vector.Distance(c, stagingPos) * (1.0 - frac) <= m_fVehOwBlockTolerance)
				return c;
		}
		return vector.Zero;
	}

	protected void CountStagingGroups(CMD_AICommanderObjectiveComponent obj, out int here, out int total, out float maxDist)
	{
		here = 0;
		total = 0;
		maxDist = 0;
		m_iDCOStageContactHere = 0;
		m_iDCOStageContactFwd = 0;
		vector stagingPos = GetOrCreateStagingPos(obj);

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || grp.GetGroupObjective() != obj)
				continue;

			if (grp.GetTask() != DCO_EGroupTask.ATTACK)
				continue;

			total++;
			float d = vector.Distance(grp.GetOwner().GetOrigin(), stagingPos);
			maxDist = Math.Max(maxDist, d);
			if (grp.IsInTransport())
				continue;
			if (d <= m_fStagingArriveRadius)
			{
				here++;
				m_mDCOStageArrived.Set(grp, obj);
			}
			else if (StageContactHere(grp, obj, d))
			{
				here++;
				m_iDCOStageContactHere++;
			}
			else if (StageContactForward(grp, obj, stagingPos))
			{
				here++;
				m_iDCOStageContactFwd++;
			}
		}
	}

	protected bool StageContactForward(DCO_GroupUtilityComponent grp, CMD_AICommanderObjectiveComponent obj, vector stagingPos)
	{
		if (!grp.HasState(DCO_EGroupState.IN_CONTACT))
			return false;
		vector objPos = obj.GetOwner().GetOrigin();
		float dObj = vector.DistanceXZ(grp.GetOwner().GetOrigin(), objPos);
		return dObj <= m_fStageContactForward || dObj < vector.DistanceXZ(stagingPos, objPos);
	}

	protected bool StageContactHere(DCO_GroupUtilityComponent grp, CMD_AICommanderObjectiveComponent obj, float dist)
	{
		return dist <= m_fStageContactRadius && m_mDCOStageArrived.Get(grp) == obj && grp.HasState(DCO_EGroupState.IN_CONTACT);
	}

	protected void ClearStageArrived(CMD_AICommanderObjectiveComponent obj)
	{
		array<DCO_GroupUtilityComponent> gone = {};
		foreach (DCO_GroupUtilityComponent g, CMD_AICommanderObjectiveComponent o : m_mDCOStageArrived)
		{
			if (!g || o == obj)
				gone.Insert(g);
		}
		foreach (DCO_GroupUtilityComponent rg : gone)
			m_mDCOStageArrived.Remove(rg);
	}

	protected void ResetSyncState(CMD_AICommanderObjectiveComponent obj)
	{
		ClearStagingPos(obj);
		m_mAssaultReleased.Remove(obj);
		m_mDCOReleaseStrength.Remove(obj);
		m_mStagingStartTime.Remove(obj);
		m_mFirstArrivalTime.Remove(obj);
		m_mSyncDeadline.Remove(obj);
		m_mAssignedTime.Remove(obj);
		ClearStageArrived(obj);
	}

	protected bool IsObjectiveCommitted(CMD_AICommanderObjectiveComponent obj)
	{
		if (obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID) || obj.IsCommanderBlackListed(m_sCommanderUID))
			return false;

		if (obj.GetObjectiveState(m_sFactionKey) != CMD_EObjectiveState.ASSIGNED)
			return false;

		return m_mStagingStartTime.Contains(obj) || m_mAssaultReleased.Contains(obj) || HasCommittedGroups(obj);
	}

	protected bool HasCommittedGroups(CMD_AICommanderObjectiveComponent obj)
	{
		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (grp && grp.GetGroupObjective() == obj && TaskCountsSlot(grp.GetTask()))
				return true;
		}
		return false;
	}

	protected bool AreStagedGroupsArrived(CMD_AICommanderObjectiveComponent obj)
	{
		int found = 0;
		vector stagingPos = GetOrCreateStagingPos(obj);

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || grp.GetGroupObjective() != obj)
				continue;

			if (grp.GetTask() != DCO_EGroupTask.ATTACK)
				continue;

			found++;

			float sd = vector.Distance(grp.GetOwner().GetOrigin(), stagingPos);
			if (StageContactHere(grp, obj, sd) || StageContactForward(grp, obj, stagingPos))
				continue;

			if (grp.IsMoving())
				return false;

			if (sd > m_fStagingArriveRadius)
				return false;
		}

		foreach (DCO_GroupUtilityComponent riding : m_aOwnedGroup)
		{
			if (riding && riding.GetGroupObjective() == obj && riding.IsInTransport())
				return false;
		}

		if (m_bPlayerTasking && m_PlayerTasking.IsBlockingSync(this, obj))
			return false;

		if (threatComp && threatComp.IsHoldingRelease(obj, DCO_Util.NowS()))
			return false;

		return found > 0;
	}

	protected void RequestAssaultIllumination(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		if (!DCO_Night.IsActive() || !artySupport || !artySupport.HasRegisteredUnits())
			return;

		CMD_FireMissionRequest req = new CMD_FireMissionRequest(obj.GetOwner().GetOrigin(), SCR_EAIArtilleryAmmoType.ILLUMINATION, worldTime, 1);
		req.m_sSource = "assault_illum";
		req.m_sTier = "illum";
		artySupport.RequestShellImpact(req, worldTime, 1);
	}

	protected void ReleaseSynchronizedAssault(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		m_mDCOReleaseStrength.Set(obj, GetAttackStrength(obj));
		vector relSt;
		float relStart;
		if (m_mStagingPos.Find(obj, relSt) && m_mStagingStartTime.Find(obj, relStart))
			DCO_CmdLearn.StagingEpisode(this, obj, relSt, relStart, "go");
		if (m_bPlayerTasking)
			GetGame().GetCallqueue().CallLater(NotifyPlayersReleased, m_iReleaseNotifyDelay_ms, false, obj);

		vector stagingPos = GetOrCreateStagingPos(obj);
		m_Ops.OnAssaultReleased(this, obj, stagingPos, worldTime);
		ClearStagingPos(obj);
		RequestAssaultIllumination(obj, worldTime);

		int releasedCount = 0;

		array<DCO_GroupUtilityComponent> toRelease = {};
		array<DCO_GroupUtilityComponent> toFlank   = {};
		float stagingToObj = vector.DistanceXZ(stagingPos, obj.GetOwner().GetOrigin());

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || grp.GetGroupObjective() != obj || grp.IsInTransport())
				continue;

			vector gp = grp.GetOwner().GetOrigin();
			if (grp.GetTask() == DCO_EGroupTask.ATTACK && (m_bDCOReleaseAll || vector.Distance(gp, stagingPos) <= m_fStagingArriveRadius || vector.DistanceXZ(gp, obj.GetOwner().GetOrigin()) < stagingToObj))
				toRelease.Insert(grp);
			else if (grp.GetTask() == DCO_EGroupTask.FLANK)
				toFlank.Insert(grp);
		}

		int totalEntering = toRelease.Count() + toFlank.Count();
		if (totalEntering <= 0)
			return;

		vector objPos    = obj.GetOwner().GetOrigin();

		vector approachFrom = vector.Zero;
		foreach (DCO_GroupUtilityComponent ag : toRelease)
			approachFrom = approachFrom + ag.GetOwner().GetOrigin();
		if (toRelease.IsEmpty())
			approachFrom = GetOwner().GetOrigin();
		else
			approachFrom = approachFrom * (1.0 / toRelease.Count());

		float approachDeg = ApproachAngle(approachFrom, objPos) * Math.RAD2DEG;
		float sectorSize  = 180.0 / Math.Max(1, toRelease.Count());

		for (int gi = 0; gi < toRelease.Count(); gi++)
		{
			DCO_GroupUtilityComponent grp = toRelease[gi];
			grp.CompleteAllWaypoints();

			array<SCR_AIWaypoint> searchWPs = {};
			GenerateSearchWaypoints(objPos, obj.GetRadius(), searchWPs, m_fSearchWpSpacing, approachDeg - 90.0 + sectorSize * gi, sectorSize);

			if (searchWPs.IsEmpty())
				continue;

			grp.MoveToRoute(searchWPs, worldTime);

			releasedCount++;
		}

		for (int fi = 0; fi < toFlank.Count(); fi++)
		{
			DCO_GroupUtilityComponent fgrp = toFlank[fi];
			fgrp.CompleteAllWaypoints();

			float flankOwnDeg = ApproachAngle(fgrp.GetOwner().GetOrigin(), objPos) * Math.RAD2DEG;
			float sideSign    = 1.0;
			float rel = flankOwnDeg - approachDeg;
			while (rel > 180.0)
				rel -= 360.0;
			while (rel < -180.0)
				rel += 360.0;
			if (rel < 0.0)
				sideSign = -1.0;

			float flankSpan = m_fFlankSearchArc;
			float arcCenter = approachDeg + sideSign * m_fFlankArcOffset;
			float arcStart  = arcCenter - flankSpan * 0.5;

			float approachAngle = arcCenter * Math.DEG2RAD;
			float approachDist  = obj.GetRadius() * m_fApproachRadiusMul;

			vector approach = objPos + Vector(
				Math.Cos(approachAngle) * approachDist,
				0.0,
				Math.Sin(approachAngle) * approachDist);

			approach = DCO_Util.Surface(approach);

			SCR_AIWaypoint approachWp = SpawnMoveWP(approach);
			if (approachWp)
				fgrp.MoveTo(approachWp, worldTime);

			array<SCR_AIWaypoint> flankWPs = {};
			GenerateSearchWaypoints(objPos, obj.GetRadius(), flankWPs, m_fSearchWpSpacing, arcStart, flankSpan);

			if (flankWPs.IsEmpty())
				continue;

			fgrp.MoveToRoute(flankWPs, worldTime);

			releasedCount++;
		}
	}

	protected void PushStalledAssaultGroups(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		if (IsGroupGatheringForAssault(obj))
			return;

		vector objPos = obj.GetOwner().GetOrigin();
		float  radius = obj.GetRadius();

		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || grp.IsPlayerGroup() || grp.GetGroupObjective() != obj)
				continue;

			if (!TaskCountsSlot(grp.GetTask()))
				continue;

			if (grp.IsMoving() || grp.IsGroupHaveWaypoint())
				continue;

			vector pos = grp.GetOwner().GetOrigin();
			if (vector.Distance(Vector(pos[0], 0, pos[2]), Vector(objPos[0], 0, objPos[2])) <= radius)
				continue;

			float ownDeg = ApproachAngle(pos, objPos) * Math.RAD2DEG;

			array<SCR_AIWaypoint> wps = {};
			GenerateSearchWaypoints(objPos, radius, wps, m_fSearchWpSpacing, ownDeg - m_fAssaultSearchArc * 0.5, m_fAssaultSearchArc);
			if (!wps.IsEmpty())
				grp.MoveToRoute(wps, worldTime);
		}
	}

	protected void TrySendAssaultWithSlots(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		PushStalledAssaultGroups(obj, worldTime);

		int    required  = GetRequiredAttackGroups(obj);
		int    onObjAll  = DCO_GroupsOnObj(obj);
		if (obj.GetCurrentAssignedGroupCount(m_sFactionKey) >= required || onObjAll >= DCO_Force.AttackMax(this))
			return;

		vector objCenter = obj.GetOwner().GetOrigin();
		float  objRad    = obj.GetRadius();
		int    slotsLeft = required - obj.GetCurrentAssignedGroupCount(m_sFactionKey);

		if (required >= 2 && !ObjectiveHasSuppressGroup(obj) && onObjAll + 1 < DCO_Force.AttackMax(this))
		{
			DCO_GroupUtilityComponent suppressGrp = FindBestIdleGroupForTask(DCO_EGroupTask.SUPPORT_BY_FIRE, objCenter, false, obj);
			if (suppressGrp && !suppressGrp.IsPlayerGroup() && CanCommitGroup(suppressGrp))
			{
				vector suppressPos = m_Ops.FindOverwatch(obj, suppressGrp.GetOwner().GetOrigin(), -1, -1, -1, GetCommanderFactionKey());
				if (suppressPos != vector.Zero)
				{
					suppressGrp.CompleteAllWaypoints();
					suppressGrp.SetGroupObjective(obj);
					if (!TryAssignTransport(suppressGrp, suppressPos, worldTime, DCO_EGroupTask.SUPPORT_BY_FIRE))
					{
						suppressGrp.SetTask(DCO_EGroupTask.SUPPORT_BY_FIRE, "AICommanderBase:6821");
						SpawnMoveRoute(suppressGrp, suppressGrp.GetOwner().GetOrigin(), suppressPos, worldTime);
					}
				}
			}
		}

		if (slotsLeft > 0)
		{
			DCO_GroupUtilityComponent assaultGrp = FindBestIdleGroupForTask(DCO_EGroupTask.ATTACK, objCenter, false, obj);
			if (assaultGrp && !assaultGrp.IsPlayerGroup() && CanCommitGroup(assaultGrp))
			{
				assaultGrp.CompleteAllWaypoints();

				vector from  = assaultGrp.GetOwner().GetOrigin();
				vector entry = GetEntryPoint(obj, from);

				int   arcCount = Math.Max(1, required);
				int   arcIndex = obj.GetCurrentAssignedGroupCount(m_sFactionKey) % arcCount;
				float arcSpan  = 180.0 / arcCount;
				float startDeg = ApproachAngle(from, objCenter) * Math.RAD2DEG - 90.0 + arcSpan * arcIndex;

				bool sent = TryAssignTransport(assaultGrp, entry, worldTime, DCO_EGroupTask.ATTACK);
				if (!sent)
				{
					array<SCR_AIWaypoint> searchWPs = {};
					GenerateSearchWaypoints(objCenter, objRad, searchWPs, m_fSearchWpSpacing, startDeg, arcSpan);
					if (!searchWPs.IsEmpty())
					{
						SpawnMoveRoute(assaultGrp, from, entry, worldTime);
						assaultGrp.MoveToRoute(searchWPs, worldTime);
						assaultGrp.SetTask(DCO_EGroupTask.ATTACK, "AICommanderBase:6852");
						sent = true;
					}
				}

				if (sent && assaultGrp.GetGroupObjective() != obj)
				{
					assaultGrp.SetGroupObjective(obj);
					obj.SetObjectiveGroup(m_sFactionKey, 1);
					slotsLeft = slotsLeft - 1;
				}
			}
		}

		if (slotsLeft > 0 && m_Tactics.AllowsFlankSwing(obj))
		{
			DCO_GroupUtilityComponent flankGrp = FindBestIdleGroupForTask(DCO_EGroupTask.FLANK, objCenter, false, obj);
			if (flankGrp && !flankGrp.IsPlayerGroup() && CanCommitGroup(flankGrp))
			{
				flankGrp.CompleteAllWaypoints();

				vector ffrom   = flankGrp.GetOwner().GetOrigin();
				int    pSide   = m_Tactics.NextFlankSide(obj);
				vector swingPt;
				if (pSide != 0)
					swingPt = FlankPoint(ffrom, objCenter, objRad + m_fFlankSwingOut, m_fFlankSwingAngle, pSide);
				else
					swingPt = ComputeFlankPosition(ffrom, objCenter, objRad + m_fFlankSwingOut);

				float approachDeg = ApproachAngle(ffrom, objCenter) * Math.RAD2DEG;
				float rel = ApproachAngle(swingPt, objCenter) * Math.RAD2DEG - approachDeg;
				while (rel > 180.0)
					rel -= 360.0;
				while (rel < -180.0)
					rel += 360.0;
				int side = 1;
				if (rel < 0.0)
					side = -1;

				bool fsent = TryAssignTransport(flankGrp, swingPt, worldTime, DCO_EGroupTask.FLANK);
				if (!fsent && SpawnMoveRoute(flankGrp, ffrom, swingPt, worldTime))
				{
					SCR_AIWaypoint edgeWp = SpawnMoveWP(FlankPoint(ffrom, objCenter, objRad + m_fFlankEdgeOut, m_fFlankEdgeAngle, side));
					if (edgeWp)
						flankGrp.MoveTo(edgeWp, worldTime);

					array<SCR_AIWaypoint> flankWPs = {};
					GenerateSearchWaypoints(objCenter, objRad, flankWPs, m_fSearchWpSpacing, approachDeg + side * m_fFlankSwingAngle - m_fFlankSearchArc * 0.5, m_fFlankSearchArc);
					if (!flankWPs.IsEmpty())
						flankGrp.MoveToRoute(flankWPs, worldTime);

					flankGrp.SetTask(DCO_EGroupTask.FLANK, "AICommanderBase:6903");
					fsent = true;
				}

				if (fsent && flankGrp.GetGroupObjective() != obj)
				{
					flankGrp.SetGroupObjective(obj);
					obj.SetObjectiveGroup(m_sFactionKey, 1);
				}
			}
		}
	}
}
