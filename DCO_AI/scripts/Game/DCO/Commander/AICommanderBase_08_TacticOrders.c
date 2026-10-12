modded class AICommander_BaseComponent
{
	vector TacticStagingPos(CMD_AICommanderObjectiveComponent obj)
	{
		return GetOrCreateStagingPos(obj);
	}

	bool TacticCommit(DCO_GroupUtilityComponent g, CMD_AICommanderObjectiveComponent obj, DCO_EGroupTask task, vector pos, float worldTime)
	{
		if (!g || !obj)
			return false;
		if (!DCO_OrderAllowed(g, "commit"))
			return false;

		g.CompleteAllWaypoints();
		bool viaTransport = TryAssignTransport(g, pos, worldTime, task);
		if (!viaTransport && !SpawnMoveRoute(g, g.GetOwner().GetOrigin(), pos, worldTime))
			return false;

		if (!viaTransport)
			g.SetTask(task, "AICommanderBase:834");
		if (g.GetGroupObjective() != obj)
		{
			g.SetGroupObjective(obj);
			if (TaskCountsSlot(task))
				obj.SetObjectiveGroup(m_sFactionKey, 1);
		}
		return true;
	}

	void TacticAppendMove(DCO_GroupUtilityComponent g, vector pos, float worldTime)
	{
		SCR_AIWaypoint wp = SpawnMoveWP(pos);
		if (wp)
			g.MoveTo(wp, worldTime);
	}

	bool DCO_OrderAllowed(DCO_GroupUtilityComponent g, string what)
	{
		if (!g || !g.GetOwner())
			return false;
		AIGroup ag = AIGroup.Cast(g.GetOwner());
		if (!ag || DCO_CtrlLock.AskGroup(ag, DCO_ECtrlReq.CMD_ORDER) != DCO_ECtrlAns.QUEUE)
			return true;
		DCO_Perf.Count("ctrl_order_queued:" + what);
		float now = DCO_Util.NowS();
		float at;
		if (!m_mDCOCtrlLogAt.Find(g, at) || now - at >= m_fCtrlLogEvery_s)
		{
			m_mDCOCtrlLogAt.Set(g, now);
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("ctrl_order_queued cmd=%1 grp=%2 order=%3 reason=ctrl_lock_queue", m_sCommanderUID, g.GetOwner().GetName(), what));
		}
		return false;
	}

	void DCO_WithdrawLock(DCO_GroupUtilityComponent g, string why)
	{
		if (!g || !g.GetOwner())
			return;
		AIGroup ag = AIGroup.Cast(g.GetOwner());
		if (ag)
			DCO_CtrlLock.Withdraw(ag, why);
	}

	void TacticMove(DCO_GroupUtilityComponent g, vector pos, float worldTime)
	{
		if (!g || g.IsInTransport())
			return;
		if (!DCO_OrderAllowed(g, "move"))
			return;

		g.CompleteAllWaypoints();
		if (!SpawnMoveRoute(g, g.GetOwner().GetOrigin(), pos, worldTime))
			TacticAppendMove(g, pos, worldTime);
	}

	void TacticAssault(DCO_GroupUtilityComponent g, CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		if (!g || !obj || g.IsInTransport())
			return;
		if (!DCO_OrderAllowed(g, "assault"))
			return;

		g.CompleteAllWaypoints();
		vector center = obj.GetOwner().GetOrigin();
		vector from = g.GetOwner().GetOrigin();
		vector entry = GetEntryPoint(obj, from);

		array<SCR_AIWaypoint> wps = {};
		GenerateSearchWaypoints(center, obj.GetRadius(), wps, m_fSearchWpSpacing, ApproachAngle(from, center) * Math.RAD2DEG - m_fAssaultSearchArc * 0.5, m_fAssaultSearchArc);
		SpawnMoveRoute(g, from, entry, worldTime);
		if (!wps.IsEmpty())
			g.MoveToRoute(wps, worldTime);
		g.SetTask(DCO_EGroupTask.ATTACK, "AICommanderBase:909");

		if (g.GetGroupObjective() != obj)
		{
			g.SetGroupObjective(obj);
			obj.SetObjectiveGroup(m_sFactionKey, 1);
		}
	}

	void TacticRelease(DCO_GroupUtilityComponent g, CMD_AICommanderObjectiveComponent obj)
	{
		if (!g)
			return;

		if (g.GetGroupObjective() == obj && TaskCountsSlot(g.GetTask()) && obj.GetCurrentAssignedGroupCount(m_sFactionKey) > 0)
			obj.SetObjectiveGroup(m_sFactionKey, -1);
		if (!g.IsInTransport())
			g.CompleteAllWaypoints();
		g.SetTask(DCO_EGroupTask.NONE, "AICommanderBase:927");
		g.SetGroupObjective(null);
	}
}
