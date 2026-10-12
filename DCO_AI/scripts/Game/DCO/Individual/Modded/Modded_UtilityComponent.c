modded class SCR_AIUtilityComponent
{
	protected ref DCO_AIMoraleSystem moraleSystem;
	DCO_AIConfigComponent m_DCOConfig;
	AIAgent myAgent;
	int m_iDCO_LOD;
	int m_iDCORoles;
	float m_fDCORolesAt_ms;
	protected float m_fDCOMoraleAcc;
	protected float m_fDCOMoraleNext_ms;

	override SCR_AIBehaviorBase EvaluateBehavior(BaseTarget unknownTarget)
	{
		int evalStart = DCO_Perf.Begin();
		if (!m_OwnerController || !m_ConfigComponent || !m_OwnerEntity)
		{
			DCO_Perf.End("agent_eval", evalStart);
			return null;
		}

		#ifdef AI_DEBUG
		AddDebugMessage("EvaluateBehavior START");
		if (m_bEvaluationBreakpoint)
		{
			Print("EvaluateBehavior breakpoint triggered", LogLevel.NORMAL);
			debug;
			m_bEvaluationBreakpoint = false;
		}
		#endif

		float time = m_OwnerEntity.GetWorld().GetWorldTime();
		float deltaTime = time - m_fLastUpdateTime;
		m_fLastUpdateTime = time;

		int cqT = DCO_Perf.Begin();
		m_Callqueue.Tick(0.001 * deltaTime);
		DCO_Perf.End("ag_callq", cqT);

		int threatStart = DCO_Perf.Begin();
		m_ThreatSystem.Update(this, deltaTime);
		DCO_Perf.End("agent_threat", threatStart);
		int moT = DCO_Perf.Begin();
		m_fDCOMoraleAcc += deltaTime;
		if (time >= m_fDCOMoraleNext_ms || moraleSystem.m_bDCODirty)
		{
			if (moraleSystem.m_bDCODirty && time < m_fDCOMoraleNext_ms)
				DCO_Perf.Count("morale:upd_dirty");
			else
				DCO_Perf.Count("morale:upd_timer");
			moraleSystem.Update(this, m_fDCOMoraleAcc);
			moraleSystem.m_bDCODirty = false;
			m_fDCOMoraleAcc = 0;
			m_fDCOMoraleNext_ms = time + 400.0 + Math.RandomFloat(0, 200.0);
		}
		m_SectorThreatFilter.Update(0.001 * deltaTime);
		DCO_Perf.End("ag_morale", moT);

		if (m_AIInfo && m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
		{
			int pt = DCO_Perf.Begin();
			DCO_VehicleCombat.UpdateCrew(this);
			DCO_Perf.End("agent_vehicle_crew", pt);
		}
		int pfT = DCO_Perf.Begin();
		m_CombatComponent.UpdatePerceptionFactor(m_PerceptionComponent, m_ThreatSystem);
		DCO_Perf.End("ag_percept", pfT);

		int msgT = DCO_Perf.Begin();
		AIMessage msgBase = m_Mailbox.ReadMessage(true);
		if (msgBase)
		{
			SCR_AIMessageGoal msgGoal = SCR_AIMessageGoal.Cast(msgBase);
			if (msgGoal)
			{
				#ifdef AI_DEBUG
				AddDebugMessage(string.Format("PerformGoalReaction: %1, from BT: %2", msgGoal, msgGoal.m_sSentFromBt));
				#endif
				m_ConfigComponent.PerformGoalReaction(this, msgGoal);
			}
			else
			{
				SCR_AIMessageInfo msgInfo = SCR_AIMessageInfo.Cast(msgBase);
				if (msgInfo)
				{
					bool overrideReaction = CallActionsOnMessage(msgInfo);

					#ifdef AI_DEBUG
					if (overrideReaction)
					{
						AddDebugMessage(string.Format("InfoMessage consumed by action: %1, from BT: %2", msgInfo, msgInfo.m_sSentFromBt));
					}
					#endif

					if (!overrideReaction)
					{
						#ifdef AI_DEBUG
						AddDebugMessage(string.Format("PerformInfoReaction: %1, from BT: %2", msgInfo, msgInfo.m_sSentFromBt));
						#endif
						m_ConfigComponent.PerformInfoReaction(this, msgInfo);
					}
				}
			}
		}

		bool reactToUnknownTarget = false;
		if (unknownTarget)
		{
			if (unknownTarget == m_UnknownTarget)
			{
				if (GetGame().GetWorld().GetWorldTime() - m_fReactionUnknownTargetTime_ms > REACTION_TO_SAME_UNKNOWN_TARGET_INTERVAL_MS)
					reactToUnknownTarget = true;
			}
			else
			{
				reactToUnknownTarget = true;
			}
		}
		if (reactToUnknownTarget && m_ConfigComponent.m_Reaction_UnknownTarget)
		{
			#ifdef AI_DEBUG
			AddDebugMessage(string.Format("PerformReaction: Unknown Target: %1", unknownTarget));
			#endif

			m_ConfigComponent.m_Reaction_UnknownTarget.PerformReaction(this, m_ThreatSystem, unknownTarget, unknownTarget.GetLastSeenPosition());
			m_fReactionUnknownTargetTime_ms = GetGame().GetWorld().GetWorldTime();
		}
		m_UnknownTarget = unknownTarget;
		DCO_Perf.End("ag_msg", msgT);

		bool weaponEvent;
		bool selectedTargetChanged;
		bool retreatTargetChanged;
		bool compartmentChanged;
		BaseTarget prevTarget;
		BaseTarget selectedTarget;
		int tgT = DCO_Perf.Begin();
		m_CombatComponent.EvaluateWeaponAndTarget(weaponEvent, selectedTargetChanged,
			prevTarget, selectedTarget, retreatTargetChanged, compartmentChanged);

		if (selectedTargetChanged && m_ConfigComponent.m_Reaction_SelectedTargetChanged)
		{
			#ifdef AI_DEBUG
			AddDebugMessage(string.Format("PerformReaction: Selected Target Changed: %1", selectedTarget));
			#endif

			m_ConfigComponent.m_Reaction_SelectedTargetChanged.PerformReaction(this, prevTarget, selectedTarget);
		}

		BaseTarget retreatTarget = m_CombatComponent.GetRetreatTarget();
		if (retreatTarget &&
			(compartmentChanged ||
			(selectedTargetChanged && !selectedTarget && retreatTarget) ||
			(!selectedTarget && retreatTargetChanged)))
		{
			if (m_ConfigComponent.m_Reaction_RetreatFromTarget)
			{
				#ifdef AI_DEBUG
				AddDebugMessage(string.Format("PerformReaction: Retreat From Target: %1", retreatTarget));
				#endif

				m_ConfigComponent.m_Reaction_RetreatFromTarget.PerformReaction(this, m_ThreatSystem, retreatTarget, retreatTarget.GetLastSeenPosition());
			}
		}

		DCO_Perf.End("ag_target", tgT);

		int ccT = DCO_Perf.Begin();
		m_CombatComponent.Update(deltaTime);
		DCO_Perf.End("ag_combat", ccT);

		int acT = DCO_Perf.Begin();
		RemoveObsoleteActions();
		AIActionBase selectedAction = EvaluateActions();
		DCO_Perf.End("ag_actions", acT);
		#ifdef AI_DEBUG
		DiagIncreaseCounter();
		DebugLogActionsPriority();
		#endif

		if (selectedAction && selectedAction != m_CurrentBehavior && (!m_CurrentBehavior || m_CurrentBehavior.IsActionInterruptable()))
		{
			SCR_AIBehaviorBase dcoPrevBehavior = m_CurrentBehavior;
			SetCurrentAction(selectedAction);
			m_CurrentBehavior = SCR_AIBehaviorBase.Cast(selectedAction);
			if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.TRACE))
				DCO_TestCaseComponent.TraceUnit(this, dcoPrevBehavior, m_CurrentBehavior);
#ifdef WORKBENCH
			SCR_AIDebugVisualization.VisualizeMessage(m_OwnerEntity, SCR_AIDebug.GetBehaviorName(m_CurrentBehavior), EAIDebugCategory.BEHAVIOR, 5);
#endif
		}

		int exT = DCO_Perf.Begin();
		m_CurrentBehavior.OnActionExecuted();

		if (m_CommsHandler.m_bNeedUpdate)
			m_CommsHandler.Update(deltaTime);

		if (m_CombatMoveState.m_bInCover && m_CombatMoveState.GetAssignedCover())
			m_CombatMoveState.VerifyCurrentCover(m_OwnerEntity.GetOrigin());
		DCO_Perf.End("ag_exec", exT);

		int cidT = DCO_Perf.Begin();
		DCO_CoverId.Tick(this);
		DCO_Perf.End("ag_cid", cidT);

		int oscT = DCO_Perf.Begin();
		DCO_Osc.Tick(this);
		DCO_Perf.End("ag_osc", oscT);

		int nvT = DCO_Perf.Begin();
		DCO_Nav.Tick(this, time);
		DCO_Perf.End("ag_nav", nvT);

		int opT = DCO_Perf.Begin();
		DCO_Opp.Tick(this, time);
		DCO_Perf.End("ag_opp", opT);

		int puT = DCO_Perf.Begin();
		DCO_Pursue.Tick(this, time);
		DCO_Perf.End("ag_pursue", puT);

		int ilT = DCO_Perf.Begin();
		DCO_IndLearn.Tick(this, time);
		DCO_PlyLearn.Tick(this, time);
		DCO_SquadChatter.Tick(this, time);
		DCO_Perf.End("ag_ilearn", ilT);

		#ifdef AI_DEBUG
		AddDebugMessage("EvaluateBehavior END\n");
		#endif

		DCO_Perf.End("agent_eval", evalStart);
		return m_CurrentBehavior;
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (m_CombatMoveState)
			m_CombatMoveState.DCO_SetOwnerUtility(this);

		AIAgent agent = GetOwner();
		myAgent = agent;
		if (!agent)
			return;

		m_OwnerEntity = GenericEntity.Cast(agent.GetControlledEntity());
		if (!m_OwnerEntity)
			return;

		AIGroup parent = agent.GetParentGroup();
		if (parent && Replication.IsServer())
		{
			DCO_GroupUtilityComponent groupUtil = DCO_GroupUtilityComponent.Cast(parent.FindComponent(DCO_GroupUtilityComponent));
			if (groupUtil)
				groupUtil.DCO_ApplyAgentLOD(agent);
		}

		moraleSystem = new DCO_AIMoraleSystem(this);
		SCR_DamageManagerComponent m_DamageManager = SCR_DamageManagerComponent.Cast(m_OwnerEntity.FindComponent(SCR_DamageManagerComponent));
		moraleSystem.RegisterDamageManager(m_DamageManager);
		m_DCOConfig = DCO_AIConfigComponent.Cast(agent.FindComponent(DCO_AIConfigComponent));
	}

	ref DCO_AIMoraleSystem GetMoraleSystem()
	{
		return moraleSystem;
	}
}