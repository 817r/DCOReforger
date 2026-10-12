modded class AICommander_BaseComponent
{
	protected void ThinkCaptureProgress(float worldTime)
	{
	    if (!Replication.IsServer())
	        return;

	    AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
	    if (!mgr)
	        return;

	    foreach (CMD_AICommanderObjectiveComponent obj : mgr.m_aObjective)
	    {
	        if (!obj)
	            continue;

	        CMD_EObjectiveState state = obj.GetObjectiveState(m_sFactionKey);

	        if (state == CMD_EObjectiveState.COMPLETED || state == CMD_EObjectiveState.FAILED)
	            continue;

	        if (obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID))
	        {
	            if (m_Tactics)
	                m_Tactics.OnOwnerFlip(this, obj, worldTime);
	            continue;
	        }

	        int friendlyNear;
	        int enemyNear;
	        obj.CountNearbyUnitsCached(obj.GetRadius(), m_sFactionKey, friendlyNear, enemyNear);

	        if (friendlyNear < enemyNear && obj.GetCurrentAssignedGroupCount(m_sFactionKey) > 0)
	        {
	            obj.SetObjectiveState(m_sFactionKey, CMD_EObjectiveState.ASSIGNED);
	            state = CMD_EObjectiveState.ASSIGNED;
	        }

	        if (state != CMD_EObjectiveState.ASSIGNED)
	            continue;

	        if (!obj.IsCaptureTimerRunning(m_sFactionKey))
	        {
	            if (friendlyNear > 0 && friendlyNear >= enemyNear)
	                obj.StartCaptureTimer(m_sFactionKey, worldTime);

	            continue;
	        }

	        obj.AssessObjective(m_sFactionKey, worldTime);

	        if (obj.IsStalemate(m_sFactionKey, worldTime))
	            HandleStalemateObjective(obj, worldTime);

	        if (obj.IsCaptureTimerComplete(m_sFactionKey, worldTime))
	        {
	            if (CQBBlocksCapture(obj, worldTime))
	                continue;

	            obj.SetCapturedBy(m_sFactionKey, true);
	            obj.ResetAssignedGroupCount(m_sFactionKey);
	            obj.ResetStalemateTracking();
	            SweepObjective(obj);
	            m_Tactics.OnCaptured(this, obj, worldTime);
	        }
	    }
	}

	protected bool FreeForClear(DCO_GroupUtilityComponent g, CMD_AICommanderObjectiveComponent obj)
	{
		if (!g || !g.GetOwner() || g.IsPlayerGroup() || g.IsMortar() || g.IsDedicatedTransport() || !g.IsInfantry() || g.GetGroupObjective() == obj)
			return false;
		return g.IsAvailableReserve() && g.CanItHaveOrder() && CanCommitGroup(g) && DCO_GroupJobs.Gate(this, g, 1, "capture_reserve");
	}

	protected int CountFreeGroups(CMD_AICommanderObjectiveComponent obj)
	{
		int n;
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (FreeForClear(g, obj))
				n++;
		}
		return n;
	}

	protected DCO_GroupUtilityComponent NearestFreeGroup(CMD_AICommanderObjectiveComponent obj, vector pos)
	{
		DCO_GroupUtilityComponent best;
		float bestD = m_fSweepGroupSearch;
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (!FreeForClear(g, obj))
				continue;
			float d = vector.DistanceXZ(g.GetOwner().GetOrigin(), pos);
			if (d < bestD)
			{
				bestD = d;
				best = g;
			}
		}
		return best;
	}

	protected bool CQBBlocksCapture(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetForFaction(m_sFactionKey);
		if (!cfg || !cfg.GetCQBEnabled() || !cfg.GetCQBCaptureWaits())
			return false;

		float now_ms = GetGame().GetWorld().GetWorldTime();
		vector objPos = obj.GetOwner().GetOrigin();
		int freeGroups = CountFreeGroups(obj);
		string action = "clear";
		float timeout_s = cfg.GetCQBContactTimeout();
		if (freeGroups < 1)
		{
			action = "timeout90";
			timeout_s = Math.Min(90.0, timeout_s);
		}
		array<IEntity> contact = {};
		DCO_BuildingClear.FindContactBuildings(objPos, obj.GetRadius(), m_sFactionKey, timeout_s * 1000, now_ms, contact);
		if (contact.IsEmpty())
		{
			m_mCapturePlan.Remove(obj);
			return false;
		}

		string plan = action + ":" + contact.Count().ToString();
		if (m_mCapturePlan.Get(obj) != plan)
		{
			m_mCapturePlan.Set(obj, plan);
			DCO_Perf.Count("capture_contact_plan:" + action);
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("capture_contact_plan obj=%1 buildings=%2 free_groups=%3 action=%4", obj.GetLogName(), contact.Count(), freeGroups, action));
		}

		foreach (IEntity building : contact)
		{
			if (SweepBuilding(obj, building, now_ms) || action != "clear")
				continue;
			DCO_GroupUtilityComponent helper = m_mClearHelper.Get(building);
			if (helper && helper.GetOwner() && helper.GetUnitCount() > 0 && helper.GetGroupObjective() == obj)
				continue;
			helper = NearestFreeGroup(obj, building.GetOrigin());
			if (!helper)
				continue;
			helper.CompleteAllWaypoints();
			helper.SetTask(DCO_EGroupTask.ATTACK, "AICommanderBase:7311");
			helper.SetGroupObjective(obj);
			obj.SetObjectiveGroup(m_sFactionKey, 1);
			SpawnMoveRoute(helper, helper.GetOwner().GetOrigin(), building.GetOrigin(), worldTime);
			m_mClearHelper.Set(building, helper);
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("capture_contact_clear obj=%1 building=%2 grp=%3", obj.GetLogName(), building, DCO_Log.IdGroup(AIGroup.Cast(helper.GetOwner()))));
		}

		if (worldTime - m_mCaptureBlockLog.Get(obj) > m_fCaptureBlockLogEvery_s)
		{
			m_mCaptureBlockLog.Set(obj, worldTime);
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("capture_blocked obj=%1 building=%2 faction=%3 buildings=%4 timeout_s=%5", obj.GetLogName(), contact[0], m_sFactionKey, contact.Count(), Math.Round(timeout_s)));
		}
		return true;
	}

	protected void SweepObjective(CMD_AICommanderObjectiveComponent obj)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetForFaction(m_sFactionKey);
		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		if (!cfg || !cfg.GetCQBEnabled() || !reg)
			return;

		float now_ms = GetGame().GetWorld().GetWorldTime();
		array<IEntity> buildings = {};
		reg.FindBuildings(obj.GetOwner().GetOrigin(), obj.GetRadius(), buildings);
		foreach (IEntity b : buildings)
		{
			if (DCO_BuildingClear.State(b, m_sFactionKey) == 3)
				continue;
			if (!SweepBuilding(obj, b, now_ms))
				return;
		}
	}

	protected bool SweepBuilding(CMD_AICommanderObjectiveComponent obj, IEntity building, float now_ms)
	{
		vector p = obj.GetOwner().GetOrigin();
		float r = obj.GetRadius() + m_fSweepMargin;
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (!g || g.GetGroupObjective() != obj || g.IsPlayerGroup() || vector.DistanceXZ(g.GetOwner().GetOrigin(), p) > r)
				continue;

			SCR_AIGroup grp = SCR_AIGroup.Cast(g.GetOwner());
			if (!grp || !grp.GetGroupUtilityComponent())
				continue;
			if (grp.GetGroupUtilityComponent().DCO_StartSweep(building, now_ms))
				return true;
		}
		return false;
	}
}
