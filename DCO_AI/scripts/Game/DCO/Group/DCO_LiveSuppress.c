class DCO_LiveSuppressVolume : SCR_AISuppressionVolumeBox
{
	protected SCR_AIGroupUtilityComponent m_Gu;
	protected vector m_vAnchor;
	protected float m_fNextAt_ms;
	protected string m_sState = "init";
	protected float m_fStartMs;
	protected int m_iRetargets;
	bool m_bLifted;

	void ~DCO_LiveSuppressVolume()
	{
		if (DCO_Util.Teardown("sbf_live_end") || !GetGame() || !GetGame().GetWorld() || m_fStartMs <= 0)
			return;
		string grp = "-";
		if (m_Gu && m_Gu.m_Owner)
			grp = DCO_Log.IdGroup(m_Gu.m_Owner);
		DCO_Perf.Count("sbf:live_end");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, grp, string.Format("sbf_live_end grp=%1 dur_s=%2 retargets=%3 last_state=%4", grp, Math.Round((GetGame().GetWorld().GetWorldTime() - m_fStartMs) / 1000.0), m_iRetargets, m_sState));
	}

	void Init(SCR_AIGroupUtilityComponent gu, AIWaypoint wp, float height)
	{
		m_Gu = gu;
		m_vAnchor = wp.GetOrigin();
		float half = Math.Clamp(wp.GetCompletionRadius(), 8.0, 30.0);
		SetBox(m_vAnchor, half, Math.Max(height, DCO_Ctx.TuneForGU(gu, "sbf.height", 4.0)));
		m_fStartMs = GetGame().GetWorld().GetWorldTime();
		DCO_Perf.Count("sbf:live_start");
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(gu.m_Owner), string.Format("sbf_live_start grp=%1 anchor=%2 half=%3", DCO_Log.IdGroup(gu.m_Owner), DCO_Util.XZ(m_vAnchor), Math.Round(half)));
	}

	protected void SetBox(vector c, float half, float h)
	{
		float y = GetGame().GetWorld().GetSurfaceY(c[0], c[2]);
		m_vBBMin = Vector(c[0] - half, y, c[2] - half);
		m_vBBMax = Vector(c[0] + half, y + h, c[2] + half);
	}

	override vector GetCenterPosition()
	{
		Refresh();
		return 0.5 * (m_vBBMin + m_vBBMax);
	}

	protected AICommander_BaseComponent Commander()
	{
		if (!m_Gu || !m_Gu.DCO_GetUtil())
			return null;
		return m_Gu.DCO_GetUtil().GetMyCommander();
	}

	protected bool FriendNear(vector pos, float r, out vector at)
	{
		AICommander_BaseComponent cmd = Commander();
		if (!cmd)
			return false;
		DCO_GroupUtilityComponent me = m_Gu.DCO_GetUtil();
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g == me || g.IsInTransport())
				continue;
			DCO_EGroupTask t = g.GetTask();
			if (t != DCO_EGroupTask.ATTACK && t != DCO_EGroupTask.FLANK)
				continue;
			vector gp = g.GetOwner().GetOrigin();
			if (vector.DistanceXZ(gp, pos) <= r)
			{
				at = gp;
				return true;
			}
		}
		return false;
	}

	protected void Refresh()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fNextAt_ms || !m_Gu)
			return;
		m_fNextAt_ms = now + DCO_Ctx.TuneG("sbf.retarget_s", 10.0) * 1000.0;

		vector old = 0.5 * (m_vBBMin + m_vBBMax);
		float range = DCO_Ctx.TuneG("sbf.search_m", 200.0);
		vector best = old;
		float half = 0.5 * (m_vBBMax[0] - m_vBBMin[0]);
		string src = "keep";
		float bestScore = -1000;

		if (m_Gu.m_Perception)
		{
			foreach (SCR_AIGroupTargetCluster c : m_Gu.m_Perception.m_aTargetClusters)
			{
				if (!c || !c.m_State || c.m_State.m_iCountAlive <= 0)
					continue;
				SCR_AITargetClusterState s = c.m_State;
				float age = s.GetTimeSinceLastNewInformation();
				vector cc = s.GetCenterPosition();
				if (age > 90.0 || vector.DistanceXZ(cc, m_vAnchor) > range)
					continue;
				float sc = s.m_iCountAlive * 10.0 - age;
				if (sc > bestScore)
				{
					bestScore = sc;
					best = cc;
					half = Math.Clamp(0.5 * vector.DistanceXZ(s.m_vBBMin, s.m_vBBMax) + 5.0, 8.0, 30.0);
					src = "cluster";
				}
			}
		}

		if (src == "keep")
		{
			AICommander_BaseComponent cmd = Commander();
			CMD_ThreatResponseComponent tr;
			if (cmd)
				tr = cmd.GetThreatResponseComponent();
			if (tr)
			{
				float nowS = DCO_Util.NowS();
				foreach (CMD_ThreatEntry te : tr.GetThreats())
				{
					if (!te || vector.DistanceXZ(te.m_vBelievedPos, m_vAnchor) > range || nowS - te.m_fLastUpdateTime > 120.0)
						continue;
					float sc2 = 100.0 - te.m_fBelievedUncertainty;
					if (sc2 > bestScore)
					{
						bestScore = sc2;
						best = te.m_vBelievedPos;
						half = Math.Clamp(te.m_fBelievedUncertainty, 8.0, 30.0);
						src = "threat";
					}
				}
			}
		}

		string state = "aim";
		vector assault;
		float safe = DCO_Ctx.TuneG("sbf.lift_m", 40.0);
		if (FriendNear(best, half + safe, assault))
		{
			vector away = best - assault;
			away[1] = 0;
			state = "lift";
			if (away.LengthSq() > 1.0)
			{
				away.Normalize();
				vector shifted = best + away * (half + safe);
				vector other;
				if (!FriendNear(shifted, half + safe * 0.75, other))
				{
					best = shifted;
					state = "shift";
				}
			}
		}
		m_bLifted = state == "lift";
		SetBox(best, half, m_vBBMax[1] - m_vBBMin[1]);

		float moved = vector.DistanceXZ(old, best);
		if (moved < 10.0 && state == m_sState)
			return;
		DCO_Perf.Count("sbf:" + state + "_" + src);
		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Gu.m_Owner), string.Format("sbf_retarget grp=%1 state=%2 src=%3 moved_m=%4 half=%5 to=%6 anchor_d=%7", DCO_Log.IdGroup(m_Gu.m_Owner), state, src, Math.Round(moved), Math.Round(half), DCO_Util.XZ(best), Math.Round(vector.DistanceXZ(best, m_vAnchor))));
		m_sState = state;
		m_iRetargets++;
	}
}

modded class SCR_SuppressWaypointState
{
	override protected void AddSuppressActivity()
	{
		DCO_GroupUtilityComponent g;
		if (m_Utility)
			g = m_Utility.DCO_GetUtil();
		if (!g || !g.GetMyCommander() || DCO_Ctx.TuneG("sbf.live", 1.0) <= 0)
		{
			super.AddSuppressActivity();
			return;
		}

		float height = 1.0;
		SCR_SuppressWaypoint suppressWp = SCR_SuppressWaypoint.Cast(m_Waypoint);
		if (suppressWp)
			height = suppressWp.GetSuppressionHeight();

		DCO_LiveSuppressVolume volume = new DCO_LiveSuppressVolume(vector.Zero, vector.Zero);
		volume.Init(m_Utility, m_Waypoint, height);

		float priorityLevel = 0;
		SCR_AIWaypoint wp = SCR_AIWaypoint.Cast(m_Waypoint);
		if (wp)
			priorityLevel = wp.GetPriorityLevel();

		SCR_AISuppressActivity activity = new SCR_AISuppressActivity(m_Utility, m_Waypoint, volume, priorityLevel: priorityLevel);
		m_Utility.AddAction(activity);
		m_SuppressActivity = activity;
	}
}
