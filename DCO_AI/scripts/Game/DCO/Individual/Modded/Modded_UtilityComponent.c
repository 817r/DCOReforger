modded class SCR_AIUtilityComponent
{
	protected ref DCO_AIMoraleSystem moraleSystem;
	DCO_AIConfigComponent m_DCOConfig;
	AIAgent myAgent;

	override SCR_AIBehaviorBase EvaluateBehavior(BaseTarget unknownTarget)
	{
		if (!m_OwnerController || !m_ConfigComponent || !m_OwnerEntity)
			return null;

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

		m_Callqueue.Tick(0.001 * deltaTime);

		m_ThreatSystem.Update(this, deltaTime);
		moraleSystem.Update(this, deltaTime);
		m_SectorThreatFilter.Update(0.001 * deltaTime);

		if (m_AIInfo && m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
		{
			int pt = DCO_Perf.Begin();
			DCO_VehicleCombat.UpdateCrew(this);
			DCO_Perf.End("agent_vehicle_crew", pt);
		}
		m_CombatComponent.UpdatePerceptionFactor(m_PerceptionComponent, m_ThreatSystem);

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

		bool weaponEvent;
		bool selectedTargetChanged;
		bool retreatTargetChanged;
		bool compartmentChanged;
		BaseTarget prevTarget;
		BaseTarget selectedTarget;
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

		m_CombatComponent.Update(deltaTime);

		RemoveObsoleteActions();
		AIActionBase selectedAction = EvaluateActions();
		#ifdef AI_DEBUG
		DiagIncreaseCounter();
		DebugLogActionsPriority();
		#endif

		if (selectedAction && selectedAction != m_CurrentBehavior && (!m_CurrentBehavior || m_CurrentBehavior.IsActionInterruptable()))
		{
			SetCurrentAction(selectedAction);
			m_CurrentBehavior = SCR_AIBehaviorBase.Cast(selectedAction);
#ifdef WORKBENCH
			SCR_AIDebugVisualization.VisualizeMessage(m_OwnerEntity, SCR_AIDebug.GetBehaviorName(m_CurrentBehavior), EAIDebugCategory.BEHAVIOR, 5);
#endif
		}

		m_CurrentBehavior.OnActionExecuted();

		if (m_CommsHandler.m_bNeedUpdate)
			m_CommsHandler.Update(deltaTime);

		if (m_CombatMoveState.m_bInCover && m_CombatMoveState.GetAssignedCover())
			m_CombatMoveState.VerifyCurrentCover(m_OwnerEntity.GetOrigin());

		#ifdef AI_DEBUG
		AddDebugMessage("EvaluateBehavior END\n");
		#endif

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

		DCO_GlobalAIComponent.ApplyLOD(agent);

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