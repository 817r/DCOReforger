class DCO_DoCQBWaypointClass : SCR_AIWaypointClass
{
}

class DCO_DoCQBWaypoint : SCR_AIWaypoint
{
	[Attribute("25", UIWidgets.Slider, "Search radius (m) for the target building around the waypoint. The building nearest the waypoint is chosen.", category: "CQB", params: "5 60 1")]
	protected float m_fTargetRadius;

	[Attribute("2", UIWidgets.Slider, "How many times the group tries to clear the building before giving up and moving to the next waypoint.", category: "CQB", params: "1 5 1")]
	protected int m_iMaxAttempts;

	[Attribute("0", UIWidgets.CheckBox, "After the building is clear the group stays in it; the waypoint does not complete.", category: "CQB")]
	protected bool m_bHoldAfterClear;

	protected IEntity m_Building;
	protected int m_iAttempts;
	protected bool m_bInAttempt;
	protected bool m_bDone;
	protected float m_fNextTryS;
	protected float m_fStartS = -1;
	protected string m_sLastWhy;
	protected DCO_CQBActivity m_Act;

	int GetMaxAttempts() { return m_iMaxAttempts; }
	void SetMaxAttempts(int v) { m_iMaxAttempts = Math.ClampInt(v, 1, 5); }
	bool GetHoldAfterClear() { return m_bHoldAfterClear; }
	void SetHoldAfterClear(bool v) { m_bHoldAfterClear = v; }
	float GetTargetRadius() { return m_fTargetRadius; }
	void SetTargetRadius(float v) { m_fTargetRadius = Math.Clamp(v, 5, 60); m_Building = null; }

	override bool DCO_WantsSetupDialog() { return true; }

	override string DCO_SetupInfo()
	{
		return string.Format("type=do_cqb attempts=%1 hold=%2 radius=%3", m_iMaxAttempts, DCO_Util.B01(m_bHoldAfterClear), m_fTargetRadius);
	}

	protected string Who(SCR_AIGroup group)
	{
		return DCO_Log.IdGroup(group) + "/" + group.GetName();
	}

	ENodeResult DCO_Tick(SCR_AIGroup group, SCR_AIGroupUtilityComponent util)
	{
		if (m_bDone)
			return ENodeResult.RUNNING;
		float now = DCO_Util.NowS();
		if (m_fStartS < 0)
			m_fStartS = now;
		if (!m_Building)
		{
			m_Building = util.DCO_NearestBuilding(GetOrigin(), m_fTargetRadius, GetOrigin());
			if (!m_Building)
				return End(group, false, "no_building");
			DCO_Perf.Count("docqb_wp:target");
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("docqb_wp_target grp=%1 prefab=%2 dist_wp=%3 radius=%4 attempts=%5 hold=%6", Who(group), DCO_Util.PrefabShort(m_Building), Math.Round(vector.DistanceXZ(GetOrigin(), m_Building.GetOrigin())), m_fTargetRadius, m_iMaxAttempts, DCO_Util.B01(m_bHoldAfterClear)));
		}
		if (m_bInAttempt)
		{
			if (m_Act && m_Act.IsRunning())
				return ENodeResult.RUNNING;
			m_bInAttempt = false;
			string res = "unknown";
			if (m_Act)
				res = m_Act.EndWhy();
			m_Act = null;
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("docqb_wp_attempt_end grp=%1 attempt=%2/%3 result=%4 alive=%5", Who(group), m_iAttempts, m_iMaxAttempts, res, group.GetAgentsCount()));
			if (res == "cleared")
				return End(group, true, "cleared");
			m_fNextTryS = now + 15.0;
		}
		if (group.GetAgentsCount() < 2)
			return End(group, false, "no_men");
		if (now < m_fNextTryS)
			return ENodeResult.RUNNING;
		if (m_iAttempts >= m_iMaxAttempts)
			return End(group, false, "attempts_exhausted");
		string why;
		if (!util.DCO_StartWaypointCQB(m_Building, this, why))
		{
			m_fNextTryS = now + 5.0;
			DCO_Perf.Count("docqb_wp:start_fail_" + why);
			if (why != m_sLastWhy)
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("docqb_wp_wait grp=%1 why=%2 attempt=%3/%4", Who(group), why, m_iAttempts, m_iMaxAttempts));
			m_sLastWhy = why;
			if (why == "no_entry")
				m_iAttempts++;
			return ENodeResult.RUNNING;
		}
		m_sLastWhy = "";
		m_bInAttempt = true;
		m_Act = util.DCO_GetCQBActivity();
		m_iAttempts++;
		DCO_Perf.Count("docqb_wp:attempt");
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("docqb_wp_attempt grp=%1 attempt=%2/%3", Who(group), m_iAttempts, m_iMaxAttempts));
		return ENodeResult.RUNNING;
	}

	protected ENodeResult End(SCR_AIGroup group, bool cleared, string why)
	{
		DCO_Perf.Count("docqb_wp:end_" + why);
		bool hold = cleared && m_bHoldAfterClear;
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, DCO_Log.IdGroup(group), string.Format("docqb_wp_end grp=%1 result=%2 attempts=%3 secs=%4 hold=%5 alive=%6", Who(group), why, m_iAttempts, Math.Round(DCO_Util.NowS() - m_fStartS), DCO_Util.B01(hold), group.GetAgentsCount()));
		if (hold)
		{
			m_bDone = true;
			return ENodeResult.RUNNING;
		}
		group.CompleteWaypoint(this);
		return ENodeResult.SUCCESS;
	}
}

class DCO_AIDoCQB : AITaskScripted
{
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		SCR_AIGroup group = SCR_AIGroup.Cast(owner);
		if (!group)
			return ENodeResult.FAIL;
		DCO_DoCQBWaypoint wp = DCO_DoCQBWaypoint.Cast(group.GetCurrentWaypoint());
		if (!wp)
			return ENodeResult.FAIL;
		if (wp.DCO_SetupPending())
		{
			DCO_Perf.Count("docqb_wp:wait_setup");
			return ENodeResult.RUNNING;
		}
		SCR_AIGroupUtilityComponent util = SCR_AIGroupUtilityComponent.Cast(group.FindComponent(SCR_AIGroupUtilityComponent));
		if (!util)
			return ENodeResult.FAIL;
		return wp.DCO_Tick(group, util);
	}

	static override bool VisibleInPalette() { return true; }

	static override string GetOnHoverDescription() { return "DCO: Do CQB waypoint. Clears the target building room by room in pairs with a random room order."; }
}
