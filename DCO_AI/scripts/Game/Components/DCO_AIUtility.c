modded class SCR_AIUtilityComponent : SCR_AIBaseUtilityComponent
{
	SCR_AIGroup m_Owner;
	ref DCO_AIMoraleSystem m_DCOMoraleSystem;
	DCO_SkillComponent m_DCO_Skill;
	ref DCO_Group_Info m_DCO_GroupInfo;
	DCO_AIAwareness m_Awareness;
	
	DCO_GroupIdentifer identifier;
	DCO_GroupTactic groupTac;

	protected static const float DISTANCE_HYSTERESIS_FACTOR = 0.2; 	//!< how bigger must be old distance to new in IsInvestigationRelevant()
	protected static const float NEARBY_DISTANCE_SQ = 150; 			//!< what is the minimal distance of new vs old in IsInvestigationRelevant()
	protected static const float REACTION_TO_SAME_UNKNOWN_TARGET_INTERVAL_MS = 3000; //!< how often to react to same unknown target if it didn't change
	
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
		
		// Update delta time and players's position
		float time = m_OwnerEntity.GetWorld().GetWorldTime();
		float deltaTime = time - m_fLastUpdateTime;
		m_fLastUpdateTime = time;

		// Create events from commands, danger events, new targets
		m_Awareness.Update();
		m_DCOMoraleSystem.Update(this,deltaTime);
		m_ThreatSystem.Update(this, deltaTime);
		m_CombatComponent.UpdatePerceptionFactor(m_PerceptionComponent, m_ThreatSystem);

		// Read messages
		AIMessage msgBase = m_Mailbox.ReadMessage(true);
		if (msgBase)
		{
			SCR_AIMessageGoal msgGoal = SCR_AIMessageGoal.Cast(msgBase);
			if (msgGoal)
			{
				// Process goal message
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
					// Process info message
					
					// Try to notify actions about the message
					bool overrideReaction = CallActionsOnMessage(msgInfo);
		
					#ifdef AI_DEBUG
					if (overrideReaction)
					{
						AddDebugMessage(string.Format("InfoMessage consumed by action: %1, from BT: %2", msgInfo, msgInfo.m_sSentFromBt));
					}
					#endif
					
					// If message was not consumed by action, process it
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
			{	// Same target
				if (GetGame().GetWorld().GetWorldTime() - m_fReactionUnknownTargetTime_ms > REACTION_TO_SAME_UNKNOWN_TARGET_INTERVAL_MS)
					reactToUnknownTarget = true;
			}
			else
			{	// Different target
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
		
		//------------------------------------------------------------------------------------------------
		// Evaluate current weapon and target
		
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
			((selectedTargetChanged && !selectedTarget && retreatTarget) || // Nothing to attack any more and must retreat from some target
			(!selectedTarget && retreatTargetChanged))) // Not attacking anything and must retreat from a different target
		{
			if (m_ConfigComponent.m_Reaction_RetreatFromTarget)
			{
				#ifdef AI_DEBUG
				AddDebugMessage(string.Format("PerformReaction: Retreat From Target: %1", retreatTarget));
				#endif
				
				m_ConfigComponent.m_Reaction_RetreatFromTarget.PerformReaction(this, m_ThreatSystem, retreatTarget, retreatTarget.GetLastSeenPosition());
			}
		}
		
		//------------------------------------------------------------------------------------------------
		// Update combat component
		m_CombatComponent.Update(deltaTime);
		
		// Evaluation: Remove completed behaviors, evaluate, set new behavior
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
		
		// Update comms handler
		if (m_CommsHandler.m_bNeedUpdate)
			m_CommsHandler.Update(deltaTime);
		
		// Update combat move state
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
		AIAgent agent = GetOwner();
		m_Owner = SCR_AIGroup.Cast(owner.FindComponent(SCR_AIGroup));
		if (!agent)
			return;	
		
		m_ConfigComponent = SCR_AIConfigComponent.Cast(agent.FindComponent(SCR_AIConfigComponent));

		m_OwnerEntity = GenericEntity.Cast(agent.GetControlledEntity());
		if (!m_OwnerEntity)
			return;

		m_Awareness = DCO_AIAwareness.Cast(m_OwnerEntity.FindComponent(DCO_AIAwareness));
		if (m_Awareness)
			m_Awareness.initialize(this);
		m_DCO_Skill = DCO_SkillComponent.Cast(m_OwnerEntity.FindComponent(DCO_SkillComponent));
		m_DCOMoraleSystem = new DCO_AIMoraleSystem(this);
		m_AIInfo = SCR_AIInfoComponent.Cast(agent.FindComponent(SCR_AIInfoComponent));
		m_CombatComponent = SCR_AICombatComponent.Cast(m_OwnerEntity.FindComponent(SCR_AICombatComponent));
		m_PerceptionComponent = PerceptionComponent.Cast(m_OwnerEntity.FindComponent(PerceptionComponent));
		m_OwnerController = SCR_CharacterControllerComponent.Cast(m_OwnerEntity.FindComponent(SCR_CharacterControllerComponent));
		m_ThreatSystem = new SCR_AIThreatSystem(this);
		m_AIInfo.InitMoraleSystem(m_DCOMoraleSystem);
		m_AIInfo.InitThreatSystem(m_ThreatSystem); // let the AIInfo know about the threat system - move along with creating threat system instance!
		m_LookAction = new SCR_AILookAction(this, false); // LookAction is not regular behavior and is evaluated separately
		m_ConfigComponent.AddDefaultBehaviors(this);
		m_Mailbox = SCR_MailboxComponent.Cast(owner.FindComponent(SCR_MailboxComponent));
		m_CommsHandler = new SCR_AICommsHandler(m_OwnerEntity, agent);
		m_CombatMoveState = new SCR_AICombatMoveState();
		m_AIInfo.m_OnCompartmentEntered.Insert(OnCompartmentEntered);
		m_AIInfo.m_OnCompartmentLeft.Insert(OnCompartmentLeft);		
	}
	
	DCO_SkillComponent getSkillComp()
	{
		return m_DCO_Skill;
	}
	
	DCO_CUSTOMRANK getRanks()
	{
		return m_DCO_Skill.GetCharacterSkillRankComponent(m_OwnerEntity);
	}
	
	DCO_GroupTactic setTactics(DCO_GroupTactic tactics)
	{
		groupTac = tactics;
		return tactics;
	}
	
	DCO_GroupTactic getTactics()
	{
		return groupTac;
	}
	
	DCO_GroupIdentifer setIdentifier(DCO_GroupIdentifer indentify)
	{
		identifier = indentify;
		return indentify;
	}
	
	DCO_GroupIdentifer getIdentifier()
	{
		return identifier;
	}
	
	CharacterControllerComponent getCharCon()
	{
		return m_OwnerController;
	}
}
