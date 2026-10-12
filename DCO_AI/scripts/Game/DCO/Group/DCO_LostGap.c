modded class SCR_AIGroupUtilityComponent
{
	protected float m_fDCOGapTick_ms;
	protected float m_fDCOGapIdleSince_ms = -1;
	protected bool m_bDCOGapLongLogged;

	void DCO_TickLostGap(float now_ms)
	{
		if (now_ms < m_fDCOGapTick_ms || !Replication.IsServer())
			return;
		m_fDCOGapTick_ms = now_ms + 1000.0;
		int t = DCO_Perf.Begin();
		DCO_LostGap.Tick(this, SCR_AIGroup.Cast(GetOwner()), now_ms);
		DCO_Perf.End("grp_lostgap", t);
	}

	void DCO_GapIdle(bool idle, string state, float now_ms)
	{
		if (idle)
		{
			if (m_fDCOGapIdleSince_ms < 0)
			{
				m_fDCOGapIdleSince_ms = now_ms;
				m_bDCOGapLongLogged = false;
			}
			float dur = (now_ms - m_fDCOGapIdleSince_ms) / 1000.0;
			if (dur > 10.0 && !m_bDCOGapLongLogged)
			{
				m_bDCOGapLongLogged = true;
				DCO_Perf.Count("lostgap:idle_long");
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Owner), string.Format("contact_idle grp=%1 idle_s=%2 state=%3 result=long blind_s=%4",
					DCO_Log.IdGroup(m_Owner), Math.Round(dur), state, Math.Round(DCO_LostGap.Blind(m_Owner))));
			}
			return;
		}
		if (m_fDCOGapIdleSince_ms < 0)
			return;
		float d = (now_ms - m_fDCOGapIdleSince_ms) / 1000.0;
		m_fDCOGapIdleSince_ms = -1;
		if (d < 3.0)
			return;
		if (d <= 10.0)
			DCO_Perf.Count("lostgap:idle_short");
		if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, DCO_Log.IdGroup(m_Owner), string.Format("contact_idle grp=%1 idle_s=%2 result=ended", DCO_Log.IdGroup(m_Owner), Math.Round(d)));
	}
}

class DCO_LostGap
{
	static float Blind(AIGroup g)
	{
		if (!g)
			return 0;
		array<AIAgent> agents = {};
		g.GetAgents(agents);
		float best = float.MAX;
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent || !ca.m_UtilityComponent.m_CombatComponent)
				continue;
			BaseTarget t = ca.m_UtilityComponent.m_CombatComponent.GetLastSeenEnemy();
			if (t)
				best = Math.Min(best, t.GetTimeSinceSeen());
		}
		if (best == float.MAX)
			return 0;
		return best;
	}

	static void Tick(SCR_AIGroupUtilityComponent u, SCR_AIGroup grp, float now_ms)
	{
		if (!grp || !u.m_Perception || DCO_Util.IsPlayerGroup(grp))
			return;
		string state;
		foreach (SCR_AIGroupTargetCluster c : u.m_Perception.m_aTargetClusters)
		{
			SCR_AITargetClusterState s = c.m_State;
			if (!s || !s.m_bDCOEngaged || s.m_iCountAlive <= 0)
				continue;
			if (s.m_eState == EAITargetClusterState.ATTACKING || s.m_eState == EAITargetClusterState.INVESTIGATING)
			{
				state = typename.EnumToString(EAITargetClusterState, s.m_eState);
				break;
			}
		}
		if (state.IsEmpty())
		{
			u.DCO_GapIdle(false, state, now_ms);
			return;
		}
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		int n;
		int idle;
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent)
				continue;
			n++;
			if (SCR_AIIdleBehavior.Cast(ca.m_UtilityComponent.GetCurrentBehavior()))
				idle++;
		}
		u.DCO_GapIdle(n > 0 && idle * 4 >= n * 3, state, now_ms);
	}
}
