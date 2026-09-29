modded class SCR_AIInvestigateClusterActivity
{

	protected bool   m_bDCOStarted;
	protected bool   m_bDCOFinal;
	protected bool   m_bDCOCoverTeamMoving;
	protected vector m_vDCOBoundPos;
	protected float  m_fDCOBoundStart_ms;
	protected float  m_fDCONextUpdate_ms;

	override void OnActionSelected()
	{
		m_bOrdersSent = true;
		super.OnActionSelected();

		float now_ms = GetGame().GetWorld().GetWorldTime();
		if (!m_bDCOStarted)
		{
			m_bDCOStarted = true;
			DCO_NextBound(now_ms);
		}
		else
		{
			m_fDCOBoundStart_ms = now_ms;
		}

		DCO_SendOrders();
	}

	void DCO_Update(float now_ms)
	{
		if (!m_bDCOStarted || GetActionState() != EAIActionState.RUNNING)
			return;

		if (now_ms < m_fDCONextUpdate_ms)
			return;
		m_fDCONextUpdate_ms = now_ms + 1000.0;

		if (m_Utility.DCO_GetPosture() == DCO_GroupTactics.EVASIVE)
		{
			Fail();
			return;
		}

		if (m_bDCOFinal)
			return;

		bool arrived  = DCO_TeamArrived(DCO_GetMovingTeam(), m_vDCOBoundPos);
		bool timedOut = now_ms - m_fDCOBoundStart_ms > 20000.0;
		if (!arrived && !timedOut)
			return;

		m_bDCOCoverTeamMoving = !m_bDCOCoverTeamMoving;
		DCO_NextBound(now_ms);
		DCO_SendOrders();
	}

	protected void DCO_NextBound(float now_ms)
	{
		m_fDCOBoundStart_ms = now_ms;

		if (m_aFireteamsCover.IsEmpty())
		{
			m_bDCOCoverTeamMoving = false;
			m_bDCOFinal = true;
			return;
		}

		vector movingPos, coverPos;
		if (!DCO_GetTeamCenter(DCO_GetMovingTeam(), movingPos) || !DCO_GetTeamCenter(DCO_GetCoverTeam(), coverPos))
		{
			m_bDCOFinal = true;
			return;
		}

		vector areaPos;
		float areaRadius;
		CalculateInvestigationArea(m_ClusterState, areaPos, areaRadius);

		vector toArea = areaPos - movingPos;
		toArea[1] = 0;
		float dist = toArea.Length();
		if (dist <= areaRadius)
		{
			m_bDCOFinal = true;
			return;
		}

		vector dir = toArea / dist;

		float step = 25.0;
		switch (m_Utility.DCO_GetPosture())
		{
			case DCO_GroupTactics.DEFENSIVE: step = 15.0; break;
			case DCO_GroupTactics.AGGRESIVE: step = 40.0; break;
		}

		vector toCover = coverPos - movingPos;
		toCover[1] = 0;
		float advance = Math.Max(step, vector.Dot(toCover, dir) + 10.0);

		if (advance >= dist - areaRadius)
		{
			m_bDCOFinal = true;
			return;
		}

		m_vDCOBoundPos = movingPos + dir * advance;
		m_vDCOBoundPos[1] = GetGame().GetWorld().GetSurfaceY(m_vDCOBoundPos[0], m_vDCOBoundPos[2]);
	}

	protected void DCO_SendOrders()
	{
		AICommunicationComponent comms = m_Utility.m_Owner.GetCommunicationComponent();
		if (!comms)
			return;

		vector areaPos;
		float areaRadius;
		CalculateInvestigationArea(m_ClusterState, areaPos, areaRadius);

		if (m_bDCOFinal)
		{
			DCO_SendInvestigate(comms, m_aFireteamsInvestigate, areaPos, areaRadius, 10000);
			DCO_SendInvestigate(comms, m_aFireteamsCover, areaPos, areaRadius, 10000);
			return;
		}

		DCO_SendInvestigate(comms, DCO_GetMovingTeam(), m_vDCOBoundPos, 5.0, 20000.0 * 0.001);
		DCO_SendCover(comms, DCO_GetCoverTeam(), areaPos);
	}

	protected void DCO_SendInvestigate(AICommunicationComponent comms, TFireteamLockRefArray fireteams, vector pos, float radius, float duration)
	{
		array<AIAgent> agents = {};
		foreach (SCR_AIGroupFireteamLock ft : fireteams)
		{
			if (!ft || !ft.GetFireteam())
				continue;

			agents.Clear();
			ft.GetFireteam().GetMembers(agents);
			foreach (AIAgent agent : agents)
			{
				if (!agent || SCR_AICompartmentHandling.IsInCompartment(agent))
					continue;

				SCR_AIMessage_Investigate msg = SCR_AIMessage_Investigate.Create(this, pos, radius, true, duration: duration);
				msg.SetReceiver(agent);
				comms.RequestBroadcast(msg, agent);
			}
		}
	}

	protected void DCO_SendCover(AICommunicationComponent comms, TFireteamLockRefArray fireteams, vector watchPos)
	{
		array<AIAgent> agents = {};
		foreach (SCR_AIGroupFireteamLock ft : fireteams)
		{
			if (!ft || !ft.GetFireteam())
				continue;

			agents.Clear();
			ft.GetFireteam().GetMembers(agents);
			foreach (AIAgent agent : agents)
			{
				if (!agent || SCR_AICompartmentHandling.IsInCompartment(agent))
					continue;

				SCR_AIMessage_Cancel cancel = SCR_AIMessage_Cancel.Create(this);
				cancel.SetReceiver(agent);
				comms.RequestBroadcast(cancel, agent);

				SCR_AIMessage_CoverCluster cover = new SCR_AIMessage_CoverCluster();
				cover.m_RelatedGroupActivity = this;
				cover.SetReceiver(agent);
				comms.RequestBroadcast(cover, agent);

				SCR_ChimeraAIAgent chimeraAgent = SCR_ChimeraAIAgent.Cast(agent);
				if (chimeraAgent && chimeraAgent.m_UtilityComponent)
					chimeraAgent.m_UtilityComponent.LookAt(watchPos, 20000.0 * 0.001);
			}
		}
	}

	protected TFireteamLockRefArray DCO_GetMovingTeam()
	{
		if (m_bDCOCoverTeamMoving)
			return m_aFireteamsCover;

		return m_aFireteamsInvestigate;
	}

	protected TFireteamLockRefArray DCO_GetCoverTeam()
	{
		if (m_bDCOCoverTeamMoving)
			return m_aFireteamsInvestigate;

		return m_aFireteamsCover;
	}

	protected bool DCO_GetTeamCenter(TFireteamLockRefArray fireteams, out vector center)
	{
		center = vector.Zero;
		int count = 0;

		array<AIAgent> agents = {};
		foreach (SCR_AIGroupFireteamLock ft : fireteams)
		{
			if (!ft || !ft.GetFireteam())
				continue;

			agents.Clear();
			ft.GetFireteam().GetMembers(agents);
			foreach (AIAgent agent : agents)
			{
				if (!agent || !agent.GetControlledEntity())
					continue;

				center += agent.GetControlledEntity().GetOrigin();
				count++;
			}
		}

		if (count == 0)
			return false;

		center = center * (1.0 / count);
		return true;
	}

	protected bool DCO_TeamArrived(TFireteamLockRefArray fireteams, vector pos)
	{
		int total = 0;
		int near = 0;

		array<AIAgent> agents = {};
		foreach (SCR_AIGroupFireteamLock ft : fireteams)
		{
			if (!ft || !ft.GetFireteam())
				continue;

			agents.Clear();
			ft.GetFireteam().GetMembers(agents);
			foreach (AIAgent agent : agents)
			{
				if (!agent || !agent.GetControlledEntity())
					continue;

				total++;
				if (vector.DistanceXZ(agent.GetControlledEntity().GetOrigin(), pos) < 10.0)
					near++;
			}
		}

		return total == 0 || near * 2 >= total;
	}

	override string GetDebugPanelText()
	{
		return super.GetDebugPanelText() + string.Format(" | DCO %1 final=%2 coverMoving=%3",
			typename.EnumToString(DCO_GroupTactics, m_Utility.DCO_GetPosture()), m_bDCOFinal, m_bDCOCoverTeamMoving);
	}
}
