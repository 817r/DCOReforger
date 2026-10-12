modded class SCR_AIInvestigateClusterActivity
{

	protected bool   m_bDCOStarted;
	protected bool   m_bDCOFinal;
	protected bool   m_bDCOCoverTeamMoving;
	protected vector m_vDCOBoundPos;
	protected float  m_fDCOBoundStart_ms;
	protected float  m_fDCONextUpdate_ms;
	protected float  m_fDCOBoundTimeout_ms = 20000.0;
	protected float  m_fDCOObserveEnd_ms = -1;

	override void OnActionSelected()
	{
		m_bOrdersSent = true;
		super.OnActionSelected();

		float now_ms = GetGame().GetWorld().GetWorldTime();
		if (!m_bDCOStarted)
		{
			m_bDCOStarted = true;
			float blind = DCO_LostGap.Blind(m_Utility.m_Owner);
			DCO_Perf.Count("lostgap:investigate_start");
			if (blind > 15.0)
				DCO_Perf.Count("lostgap:slow_start");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Utility.m_Owner), string.Format("investigate_start grp=%1 blind_s=%2 blind_switch=%3 identified=%4 lost=%5",
				DCO_Log.IdGroup(m_Utility.m_Owner), blind.ToString(1, 1), m_ClusterState.m_bDCOBlind, m_ClusterState.m_iCountIdentified, m_ClusterState.m_iCountLost));
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
		{
			if (m_fDCOObserveEnd_ms > 0 && now_ms >= m_fDCOObserveEnd_ms)
			{
				m_fDCOObserveEnd_ms = -1;
				DCO_Perf.Count("ctx:inv_observe_end");
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Utility.m_Owner), string.Format("ctxinvestigate grp=%1 phase=observe_end result=no_contact", DCO_Log.IdGroup(m_Utility.m_Owner)));
				Complete();
			}
			return;
		}

		bool arrived  =DCO_TeamArrived(DCO_GetMovingTeam(), m_vDCOBoundPos);
		bool timedOut = now_ms - m_fDCOBoundStart_ms > m_fDCOBoundTimeout_ms;
		if (!arrived && !timedOut)
			return;

		float boundS = (now_ms - m_fDCOBoundStart_ms) / 1000.0;
		string why = "arrived";
		if (timedOut)
			why = "timeout";
		else if (boundS < DCO_Ctx.TuneForGU(m_Utility, "osc.bound_min_s", 4.0))
		{
			why = "arrived_fast";
			if (DCO_Ctx.TuneForGU(m_Utility, "osc.fix_bound", 0.0) > 0)
			{
				DCO_Perf.Count("inv:bound_hold");
				return;
			}
		}
		DCO_Perf.Count("inv:bound_" + why);
		if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, DCO_Log.IdGroup(m_Utility.m_Owner), string.Format("inv_bound grp=%1 why=%2 bound_s=%3 cover_moving=%4 bound=%5", DCO_Log.IdGroup(m_Utility.m_Owner), why, boundS.ToString(-1, 1), DCO_Util.B01(m_bDCOCoverTeamMoving), DCO_Log.Pos(m_vDCOBoundPos)));

		m_bDCOCoverTeamMoving = !m_bDCOCoverTeamMoving;
		DCO_NextBound(now_ms);
		DCO_SendOrders();
	}

	protected bool DCO_CtxHunter()
	{
		if (m_Utility.DCO_GetPosture() == DCO_GroupTactics.AGGRESIVE)
			return true;
		return SCR_SearchAndDestroyWaypoint.Cast(m_Utility.m_Owner.GetCurrentWaypoint()) != null;
	}

	protected void DCO_Area(out vector pos, out float radius)
	{
		CalculateInvestigationArea(m_ClusterState, pos, radius);
		if (m_ClusterState.m_vDCOPursuit != vector.Zero)
		{
			pos = m_ClusterState.m_vDCOPursuit;
			radius = Math.Max(radius, 20.0);
		}
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
		DCO_Area(areaPos, areaRadius);

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

		float aggr = 1.0;
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetFor(m_Utility.m_Owner);
		if (g && dist - areaRadius <= 2.0 * g.DCO_TuneF("tm.cqb_range_m", 25.0))
			aggr = Math.Max(g.DCO_TuneF("tm.cqb_aggr_out", 4.0), 1.0);
		step *= aggr;
		m_fDCOBoundTimeout_ms = 20000.0 / aggr;
		if (aggr > 1.0)
		{
			DCO_Perf.Count("lostgap:investigate_cqb_aggr");
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Utility.m_Owner), string.Format("investigate_cqb grp=%1 dist=%2 area_r=%3 aggr=%4 step=%5 bound_s=%6",
				DCO_Log.IdGroup(m_Utility.m_Owner), Math.Round(dist), Math.Round(areaRadius), aggr.ToString(-1, 1), Math.Round(step), (m_fDCOBoundTimeout_ms * 0.001).ToString(-1, 1)));
		}

		if (g && g.DCO_TuneF("ctx.on", 1.0) > 0)
		{
			int band = DCO_Ctx.Band(dist - areaRadius);
			int env = DCO_Ctx.EnvAtPos(areaPos);
			string mode = "bound";
			if (band == DCO_ERangeBand.CONTACT || band == DCO_ERangeBand.CLOSE)
			{
				mode = "individual";
				m_bDCOFinal = true;
			}
			else if (band == DCO_ERangeBand.MEDIUM && env == DCO_EEnv.OPEN && !DCO_CtxHunter())
			{
				mode = "observe";
				m_bDCOFinal = true;
			}
			else if (band == DCO_ERangeBand.LONG || band == DCO_ERangeBand.EXTREME)
			{
				mode = "travel";
				step *= 1.6;
				m_fDCOBoundTimeout_ms *= 1.6;
			}
			DCO_Perf.Count("ctx:inv_" + mode);
			DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Utility.m_Owner), string.Format("ctxinvestigate grp=%1 phase=approach band=%2 lkp_env=%3 mode=%4 step=%5 dist=%6",
				DCO_Log.IdGroup(m_Utility.m_Owner), DCO_Ctx.BandName(band), DCO_Ctx.EnvName(env), mode, Math.Round(step), Math.Round(dist)));
			if (m_bDCOFinal)
				return;
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
		m_vDCOBoundPos = DCO_Util.Surface(m_vDCOBoundPos);
	}

	protected void DCO_SendOrders()
	{
		AICommunicationComponent comms = m_Utility.m_Owner.GetCommunicationComponent();
		if (!comms)
			return;

		vector areaPos;
		float areaRadius;
		DCO_Area(areaPos, areaRadius);

		if (m_bDCOFinal)
		{
			float dur = 10000;
			DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetFor(m_Utility.m_Owner);
			if (g && g.DCO_TuneF("ctx.on", 1.0) > 0)
			{
				int env = DCO_Ctx.EnvAtPos(areaPos);
				switch (env)
				{
					case DCO_EEnv.URBAN:
					{
						areaRadius = Math.Max(areaRadius, 20.0);
						dur = 75;
						break;
					}
					case DCO_EEnv.FOREST:
					{
						areaRadius = Math.Max(areaRadius, 35.0);
						dur = 105;
						break;
					}
					case DCO_EEnv.TRENCH:
					{
						areaRadius = Math.Max(areaRadius, 20.0);
						dur = 75;
						break;
					}
					case DCO_EEnv.OPEN:
					{
						dur = 38;
						break;
					}
				}
				if (dur < 10000)
				{
					switch (m_Utility.DCO_GetPosture())
					{
						case DCO_GroupTactics.AGGRESIVE: dur = 1.2 * dur; break;
						case DCO_GroupTactics.DEFENSIVE: dur = 0.8 * dur; break;
						case DCO_GroupTactics.EVASIVE: dur = 0.6 * dur; break;
					}
				}
				DCO_Perf.Count("ctx:inv_search_" + DCO_Ctx.EnvName(env));
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Utility.m_Owner), string.Format("ctxinvestigate grp=%1 phase=search lkp_env=%2 radius=%3 dur_s=%4 pos=%5",
					DCO_Log.IdGroup(m_Utility.m_Owner), DCO_Ctx.EnvName(env), Math.Round(areaRadius), Math.Round(dur), DCO_Log.Pos(areaPos)));
				float nowMs = GetGame().GetWorld().GetWorldTime();
				if (env == DCO_EEnv.INDOOR)
				{
					string bldMethod;
					IEntity bld = SCR_CoverManagerComponent.DCO_GetBuildingAtPos(areaPos, null, bldMethod);
					string fac = m_Utility.DCO_FactionKey();
					string gateWhy;
					if (bld && g.DCO_TuneF("ctx.cqb_gate", 1.0) > 0 && DCO_CQBPrecheck.Check(bld, m_Utility.m_Owner, gateWhy) == 0)
					{
						DCO_Perf.Count("ctx:inv_cqb_gate_skip");
						DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Utility.m_Owner), string.Format("ctxinvestigate grp=%1 phase=handoff_skip building=%2 why=%3", DCO_Log.IdGroup(m_Utility.m_Owner), DCO_Util.PrefabShort(bld), gateWhy));
						bld = null;
					}
					if (bld && !fac.IsEmpty())
					{
						DCO_BuildingClear.MarkContact(bld, fac, nowMs);
						DCO_Perf.Count("ctx:inv_cqb_handoff");
						DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Utility.m_Owner), string.Format("ctxinvestigate grp=%1 phase=handoff_cqb building=%2 method=%3 pos=%4",
							DCO_Log.IdGroup(m_Utility.m_Owner), DCO_Util.PrefabShort(bld), bldMethod, DCO_Log.Pos(bld.GetOrigin())));
					}
				}
				else if (env == DCO_EEnv.OPEN && !DCO_CtxHunter())
				{
					if (m_fDCOObserveEnd_ms < 0)
						m_fDCOObserveEnd_ms = nowMs + dur * 1000.0;
					DCO_SendCover(comms, m_aFireteamsInvestigate, areaPos);
					DCO_SendCover(comms, m_aFireteamsCover, areaPos);
					DCO_Perf.Count("ctx:inv_observe");
					DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, DCO_Log.IdGroup(m_Utility.m_Owner), string.Format("ctxinvestigate grp=%1 phase=observe dur_s=%2 pos=%3",
						DCO_Log.IdGroup(m_Utility.m_Owner), Math.Round(dur), DCO_Log.Pos(areaPos)));
					return;
				}
			}
			DCO_SendInvestigate(comms, m_aFireteamsInvestigate, areaPos, areaRadius, dur);
			DCO_SendInvestigate(comms, m_aFireteamsCover, areaPos, areaRadius, dur);
			return;
		}

		DCO_SendInvestigate(comms, DCO_GetMovingTeam(), m_vDCOBoundPos, 5.0, m_fDCOBoundTimeout_ms * 0.001);
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
