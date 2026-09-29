class DCO_StragglerEntry
{
	float m_fFarSince_ms;
	float m_fBestDist;
	float m_fBestTime_ms;
	int m_iRetries;
	bool m_bStuck;
	SCR_AIMoveIndividuallyBehavior m_Regroup;
}

class DCO_StragglerWatch
{
	protected static const int MAX_RETRIES = 1;
	protected static const float REGROUP_PRIORITY = SCR_AIActionBase.PRIORITY_BEHAVIOR_ATTACK_SELECTED + 1;

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
		m_fNext_ms = now_ms + 5000.0;

		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
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

			float dist = vector.Distance(c.GetOrigin(), leaderPos);
			if (dist <= cohesion)
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
				m_mEntries.Insert(agent, e);
				continue;
			}

			if (dist < e.m_fBestDist - 3.0)
			{
				e.m_fBestDist = dist;
				e.m_fBestTime_ms = now_ms;
			}

			if (e.m_bStuck || now_ms - e.m_fFarSince_ms < far_ms)
				continue;

			if (util.m_ThreatSystem && util.m_ThreatSystem.GetState() == EAIThreatState.THREATENED)
				continue;

			bool regrouping = e.m_Regroup && e.m_Regroup.GetActionState() != EAIActionState.COMPLETED
				&& e.m_Regroup.GetActionState() != EAIActionState.FAILED;

			if (now_ms - e.m_fBestTime_ms > stuck_ms)
			{
				if (regrouping)
					e.m_Regroup.Fail();
				if (e.m_iRetries >= MAX_RETRIES)
				{
					e.m_bStuck = true;
					Print(string.Format("[DCO_Straggler] %1 MACET %2 m dari leader %3 -- gak dihitung di spread",
						c, Math.Round(dist), leader), LogLevel.WARNING);
					continue;
				}

				e.m_iRetries++;
				e.m_fBestTime_ms = now_ms;
				PushRegroup(util, e, AltPoint(leaderPos, c.GetOrigin()), null, cohesion);
				continue;
			}

			if (regrouping)
				continue;

			if (log)
				LogStraggler(c, util, dist, group);

			FailStale(util);
			PushRegroup(util, e, leaderPos, leader, cohesion);
		}
	}

	protected bool CanWatch(AIGroup group)
	{
		ChimeraCharacter leader = ChimeraCharacter.Cast(group.GetLeaderEntity());
		if (!leader || leader.IsInVehicle() || SCR_CharacterHelper.IsPlayer(leader))
			return false;

		CharacterControllerComponent ctrl = leader.GetCharacterController();
		if (!ctrl || ctrl.GetLifeState() != ECharacterLifeState.ALIVE)
			return false;

		return !SCR_DefendWaypoint.Cast(group.GetCurrentWaypoint());
	}

	protected bool IsFree(ChimeraCharacter c, SCR_AIUtilityComponent util)
	{
		if (c.IsInVehicle())
			return false;

		CharacterControllerComponent ctrl = c.GetCharacterController();
		if (!ctrl || ctrl.GetLifeState() != ECharacterLifeState.ALIVE)
			return false;

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

		SCR_AIMoveIndividuallyBehavior regroup = new SCR_AIMoveIndividuallyBehavior(util, null, pos, REGROUP_PRIORITY, ent: leader, radius: cohesion * 0.5);
		util.AddAction(regroup);
		e.m_Regroup = regroup;
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
		p[1] = GetGame().GetWorld().GetSurfaceY(p[0], p[2]);
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

		Print(string.Format("[DCO_Straggler] %1 grp=%2 dist=%3 behavior=%4 threat=%5 combat=%6 inCover=%7 wp=%8",
			c, group, Math.Round(dist), behavior, threat, combatMode, inCover, group.GetCurrentWaypoint()));
	}
}
