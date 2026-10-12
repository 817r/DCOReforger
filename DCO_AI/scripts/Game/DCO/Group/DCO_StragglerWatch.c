class DCO_StragglerEntry
{
	float m_fFarSince_ms;
	float m_fBestDist;
	float m_fBestTime_ms;
	int m_iRetries;
	bool m_bStuck;
	bool m_bStuckLogged;
	vector m_vAnchor;
	float m_fStillSince_ms;
	SCR_AIMoveIndividuallyBehavior m_Regroup;
}

class DCO_StragglerWatch
{
	protected static float REGROUP_PRIORITY() { return SCR_AIActionBase.PRIORITY_BEHAVIOR_ATTACK_SELECTED + 1; }

	protected ref map<AIAgent, ref DCO_StragglerEntry> m_mEntries = new map<AIAgent, ref DCO_StragglerEntry>();
	protected float m_fNext_ms;

	bool IsStuck(AIAgent agent)
	{
		DCO_StragglerEntry e = m_mEntries.Get(agent);
		return e && e.m_bStuck;
	}

	void Update(float now_ms, AIGroup group, bool garrison)
	{
		if (now_ms < m_fNext_ms)
			return;
		m_fNext_ms = now_ms + 3000.0;

		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(group);
		if (garrison || (cfg && !cfg.GetStragglerEnabled()) || !CanWatch(group))
		{
			m_mEntries.Clear();
			return;
		}

		float far_ms = 15000;
		float stuck_ms = 30000;
		bool log;
		if (cfg)
		{
			far_ms = cfg.GetStragglerTime() * 1000;
			stuck_ms = cfg.GetStragglerStuckTime() * 1000;
			log = cfg.GetStragglerLog();
		}

		float cohesion = 50.0;
		DCO_GroupConfigComponent gcfg = DCO_GroupConfigComponent.Cast(group.FindComponent(DCO_GroupConfigComponent));
		if (gcfg)
			cohesion = gcfg.GetCohesionDistance();

		IEntity leader = group.GetLeaderEntity();
		AIAgent leaderAgent = group.GetLeaderAgent();
		vector leaderPos = leader.GetOrigin();
		float moveCohesion = Math.Max(cohesion, 80.0);
		DCO_GroupUtilityComponent dgu = DCO_GroupUtilityComponent.Cast(group.FindComponent(DCO_GroupUtilityComponent));
		bool grpContact = dgu && dgu.HasState(DCO_EGroupState.IN_CONTACT);
		SCR_AIGroupUtilityComponent sgu = SCR_AIGroupUtilityComponent.Cast(group.FindComponent(SCR_AIGroupUtilityComponent));
		bool grpBusy = false;
		if (sgu)
		{
			AIActionBase gact = sgu.GetCurrentAction();
			grpBusy = gact && gact.ClassName().Contains("Cluster");
		}

		array<AIAgent> agents = {};
		group.GetAgents(agents);
		for (int i = m_mEntries.Count() - 1; i >= 0; i--)
		{
			if (!agents.Contains(m_mEntries.GetKey(i)))
				m_mEntries.RemoveElement(i);
		}

		foreach (AIAgent agent : agents)
		{
			if (agent == leaderAgent)
				continue;

			ChimeraCharacter c = ChimeraCharacter.Cast(agent.GetControlledEntity());
			SCR_AIUtilityComponent util = SCR_AIUtilityComponent.Cast(agent.FindComponent(SCR_AIUtilityComponent));
			if (!c || !util || !IsFree(c, util))
			{
				m_mEntries.Remove(agent);
				continue;
			}

			vector myPos = c.GetOrigin();
			float dist = vector.Distance(myPos, leaderPos);
			float limit = moveCohesion;
			if (grpContact)
			{
				bool urban;
				limit = DCO_Util.CombatCohesionDist(group, c, leader, urban);
			}
			if (dist <= limit)
			{
				m_mEntries.Remove(agent);
				continue;
			}

			DCO_StragglerEntry e = m_mEntries.Get(agent);
			if (!e)
			{
				e = new DCO_StragglerEntry();
				e.m_fFarSince_ms = now_ms;
				e.m_fBestDist = dist;
				e.m_fBestTime_ms = now_ms;
				e.m_vAnchor = myPos;
				e.m_fStillSince_ms = now_ms;
				m_mEntries.Insert(agent, e);
				continue;
			}

			if (vector.Distance(myPos, e.m_vAnchor) >= 3.0)
			{
				e.m_vAnchor = myPos;
				e.m_fStillSince_ms = now_ms;
				if (e.m_bStuck)
				{
					e.m_bStuck = false;
					e.m_bStuckLogged = false;
					e.m_iRetries = 0;
					e.m_fBestDist = dist;
					e.m_fBestTime_ms = now_ms;
					DCO_Perf.Count("straggler:unstuck");
				}
			}

			if (dist < e.m_fBestDist - 3.0)
			{
				e.m_fBestDist = dist;
				e.m_fBestTime_ms = now_ms;
			}

			if (e.m_bStuck)
			{
				TrySplit(group, agent, c, util, e, dist, now_ms, grpContact, grpBusy);
				continue;
			}

			if (now_ms - e.m_fFarSince_ms < far_ms)
				continue;

			if (util.m_ThreatSystem && util.m_ThreatSystem.GetState() == EAIThreatState.THREATENED)
				continue;

			bool regrouping = e.m_Regroup && e.m_Regroup.GetActionState() != EAIActionState.COMPLETED
				&& e.m_Regroup.GetActionState() != EAIActionState.FAILED;

			if (now_ms - e.m_fBestTime_ms > stuck_ms)
			{
				if (regrouping)
					e.m_Regroup.Fail();
				if (e.m_iRetries >= 1)
				{
					e.m_bStuck = true;
					TrySplit(group, agent, c, util, e, dist, now_ms, grpContact, grpBusy);
					continue;
				}

				e.m_iRetries++;
				e.m_fBestTime_ms = now_ms;
				ActionLog(group, c, dist, "regroup_alt", now_ms - e.m_fFarSince_ms, DCO_ELogLevel.DEBUG, "");
				PushRegroup(util, e, AltPoint(leaderPos, c.GetOrigin()), null, limit);
				continue;
			}

			if (regrouping)
				continue;

			if (log)
				LogStraggler(c, util, dist, group);

			ActionLog(group, c, dist, "regroup", now_ms - e.m_fFarSince_ms, DCO_ELogLevel.DEBUG, "");
			FailStale(util);
			PushRegroup(util, e, leaderPos, leader, limit);
		}
	}

	protected void ActionLog(AIGroup group, ChimeraCharacter c, float dist, string action, float far_ms, DCO_ELogLevel level, string extra)
	{
		DCO_Perf.Count("straggler:" + action);
		if (!DCO_Log.On(DCO_ELogChannel.SQUAD, level))
			return;
		DCO_Log.Write(DCO_ELogChannel.SQUAD, level, DCO_Log.IdGroup(group), string.Format("straggler grp=%1 unit=%2 dist=%3 action=%4 far_s=%5 wp=%6",
			DCO_Log.IdGroup(group), DCO_Log.IdUnit(c), Math.Round(dist), action, Math.Round(far_ms / 1000.0), group.GetCurrentWaypoint() != null) + extra);
	}

	protected void TrySplit(AIGroup group, AIAgent agent, ChimeraCharacter c, SCR_AIUtilityComponent util, DCO_StragglerEntry e, float dist, float now_ms, bool grpContact, bool grpBusy)
	{
		float still_ms = now_ms - e.m_fStillSince_ms;
		bool memberContact = util.m_ThreatSystem && util.m_ThreatSystem.GetState() == EAIThreatState.THREATENED;
		if (!memberContact && util.m_CombatComponent && util.m_CombatComponent.GetCurrentTarget())
			memberContact = true;
		string block;
		SCR_AIGroup sg = SCR_AIGroup.Cast(group);
		if (!sg || !group.GetCurrentWaypoint())
			block = "no_wp";
		else if (dist < 150.0)
			block = "near";
		else if (grpContact || memberContact)
			block = "contact";
		else if (grpBusy)
			block = "busy";
		else if (still_ms < 60000.0)
			block = "still_short";
		string extra = string.Format(" still_s=%1", Math.Round(still_ms / 1000.0));
		if (!block.IsEmpty())
		{
			if (!e.m_bStuckLogged)
			{
				e.m_bStuckLogged = true;
				ActionLog(group, c, dist, "stuck_hold", now_ms - e.m_fFarSince_ms, DCO_ELogLevel.INFO, extra + " block=" + block);
			}
			return;
		}
		AICommander_BaseComponent cmd;
		DCO_GroupUtilityComponent gu = DCO_GroupUtilityComponent.Cast(group.FindComponent(DCO_GroupUtilityComponent));
		if (gu)
			cmd = gu.GetMyCommander();
		array<AIAgent> one = {agent};
		SCR_AIGroup ng = DCO_Logistics.SplitIntoNewGroup(sg, one, cmd);
		if (!ng)
		{
			e.m_fStillSince_ms = now_ms;
			ActionLog(group, c, dist, "split_failed", now_ms - e.m_fFarSince_ms, DCO_ELogLevel.INFO, extra);
			return;
		}
		m_mEntries.Remove(agent);
		ActionLog(group, c, dist, "split", now_ms - e.m_fFarSince_ms, DCO_ELogLevel.INFO, extra);
	}

	protected bool CanWatch(AIGroup group)
	{
		ChimeraCharacter leader = ChimeraCharacter.Cast(group.GetLeaderEntity());
		if (!leader || leader.IsInVehicle() || SCR_CharacterHelper.IsPlayer(leader))
			return false;

		if (!DCO_Util.IsAlive(leader))
			return false;

		return !SCR_DefendWaypoint.Cast(group.GetCurrentWaypoint());
	}

	protected bool IsFree(ChimeraCharacter c, SCR_AIUtilityComponent util)
	{
		if (c.IsInVehicle())
			return false;

		if (!DCO_Util.IsAlive(c))
			return false;

		if (util.m_CombatComponent)
		{
			DCO_SpecState spec = util.m_CombatComponent.DCO_GetSpec();
			if (spec && spec.m_iSortie != 0)
			{
				DCO_Perf.Count("straggler:skip_sortie");
				return false;
			}
		}

		return !util.m_DCOConfig || util.m_DCOConfig.GetLeashType() == DCO_ELeashType.NONE;
	}

	protected void FailStale(SCR_AIUtilityComponent util)
	{
		SCR_AIBehaviorBase b = util.GetCurrentBehavior();
		if (!b)
			return;

		if (SCR_AIAttackBehavior.Cast(b) || SCR_AIMoveFromDangerBehavior.Cast(b) || SCR_AIAvoidCharacterBehavior.Cast(b)
			|| SCR_AIRetreatFromTargetBehavior.Cast(b) || SCR_AIRetreatWhileLookAtBehavior.Cast(b)
			|| SCR_AIHealBehavior.Cast(b) || SCR_AIHealWaitBehavior.Cast(b) || SCR_AIMedicHealBehavior.Cast(b)
			|| SCR_AIVehicleBehavior.Cast(b) || SCR_AIThrowGrenadeToBehavior.Cast(b) || SCR_AISuppressBehavior.Cast(b)
			|| SCR_AIMoveInFormationBehavior.Cast(b) || SCR_AIIdleBehavior.Cast(b) || SCR_AIMoveIndividuallyBehavior.Cast(b)
			|| SCR_AIDefendBehavior.Cast(b) || SCR_AIStaticArtilleryBehavior.Cast(b))
			return;

		b.Fail();
	}

	protected void PushRegroup(SCR_AIUtilityComponent util, DCO_StragglerEntry e, vector pos, IEntity leader, float cohesion)
	{
		if (util.m_CombatMoveState)
			util.m_CombatMoveState.m_bInCover = false;

		float prio = REGROUP_PRIORITY();
		if (util.m_CombatComponent && util.m_CombatComponent.GetCurrentTarget())
		{
			prio = SCR_AIActionBase.PRIORITY_BEHAVIOR_ATTACK_SELECTED - 1;
			DCO_Perf.Count("straggler:regroup_below_attack");
			if (DCO_Log.On(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(util.m_OwnerEntity), string.Format("straggler_regroup_prio unit=%1 prio=%2 reason=has_target", DCO_Log.IdUnit(util.m_OwnerEntity), prio));
		}
		SCR_AIMoveIndividuallyBehavior regroup = new SCR_AIMoveIndividuallyBehavior(util, null, pos, prio, ent: leader, radius: cohesion * 0.5);
		util.AddAction(regroup);
		e.m_Regroup = regroup;

		AICharacterMovementComponent move = AICharacterMovementComponent.Cast(util.GetAIAgent().GetMovementComponent());
		if (move)
			move.SetMovementTypeWanted(EMovementType.RUN);
	}

	protected vector AltPoint(vector leaderPos, vector myPos)
	{
		vector dir = leaderPos - myPos;
		dir[1] = 0;
		dir.Normalize();
		vector side = Vector(-dir[2], 0, dir[0]);
		if (Math.RandomInt(0, 2) == 0)
			side = -side;

		vector p = leaderPos + side * 15.0;
		p = DCO_Util.Surface(p);
		return p;
	}

	protected void LogStraggler(ChimeraCharacter c, SCR_AIUtilityComponent util, float dist, AIGroup group)
	{
		string behavior = "none";
		if (util.GetCurrentBehavior())
			behavior = util.GetCurrentBehavior().GetActionDebugInfo();

		string threat = "?";
		if (util.m_ThreatSystem)
			threat = typename.EnumToString(EAIThreatState, util.m_ThreatSystem.GetState());

		string combatMode = "?";
		SCR_AIGroup scrGroup = SCR_AIGroup.Cast(group);
		if (scrGroup && scrGroup.GetGroupUtilityComponent())
			combatMode = typename.EnumToString(EAIGroupCombatMode, scrGroup.GetGroupUtilityComponent().GetCombatModeActual());

		bool inCover = util.m_CombatMoveState && util.m_CombatMoveState.m_bInCover;

		DCO_Log.Write(DCO_ELogChannel.SQUAD, DCO_ELogLevel.INFO, "-", string.Format("%1 grp=%2 dist=%3 behavior=%4 threat=%5 combat=%6 inCover=%7 wp=%8",
			c, group, Math.Round(dist), behavior, threat, combatMode, inCover, group.GetCurrentWaypoint()));
	}
}
