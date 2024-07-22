modded class SCR_AIGroupUtilityComponent : SCR_AIBaseUtilityComponent
{
	DCO_GroupIdentifierComponent m_GroupIdentifier;
	DCO_GroupIdentifer m_Idf;
	DCO_GroupTacticComponent m_GroupTactics;
	DCO_GroupTactic m_Tac;
	
	ref array<SCR_AIUtilityComponent> m_Util = {};

	//------------------------------------------------------------------------------------------------
	//!
	//! \param[out] restartActivity
	//! \return
	override SCR_AIActionBase EvaluateActivity(out bool restartActivity)
	{
		SCR_AIActionBase activity;
		restartActivity = false;
		
		if (!m_ConfigComponent)
			return null;
		
		float currentTime = GetGame().GetWorld().GetWorldTime();
		float deltaTime_ms = 0;
		if (m_fLastUpdateTime != -1.0)
			deltaTime_ms = currentTime - m_fLastUpdateTime;
		
		#ifdef AI_DEBUG
		AddDebugMessage("EvaluateActivity START");
		if (m_bEvaluationBreakpoint)
		{
			Print("EvaluateActivity breakpoint triggered");
			debug;
			m_bEvaluationBreakpoint = false;
		}
		#endif
		
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
					
					bool overrideReaction = CallActionsOnMessage(msgInfo);
			
					if (!overrideReaction)
					{
						#ifdef AI_DEBUG
						AddDebugMessage(string.Format("PerformInfoReaction: %1, from BT: %2", msgInfo, msgInfo.m_sSentFromBt));
						#endif
						
						m_ConfigComponent.PerformInfoReaction(this, msgInfo);
					}
					#ifdef AI_DEBUG
					else
					{
						#ifdef AI_DEBUG
						AddDebugMessage(string.Format("InfoMessage consumed by action: %1, from BT: %2", msgInfo, msgInfo.m_sSentFromBt));
						#endif
					}
					#endif
				}
			}
		}
			
		RemoveObsoleteActions();
		
		activity = SCR_AIActionBase.Cast(EvaluateActions());
		#ifdef AI_DEBUG
		DiagIncreaseCounter();
		DebugLogActionsPriority();
		#endif
		
		if (activity && (!m_CurrentActivity || (m_CurrentActivity != activity && m_CurrentActivity.IsActionInterruptable())))
			restartActivity = true;
		else if (m_bNewGroupMemberAdded && activity)
			restartActivity = true;
			
		
		
		if (restartActivity)
		{
			SetCurrentAction(activity);
			UpdateGroupControlMode(activity);
			m_CurrentActivity = activity;
			
#ifdef WORKBENCH
			if (DiagMenu.GetBool(SCR_DebugMenuID.DEBUGUI_AI_PRINT_ACTIVITY))
				PrintFormat("Agent %1 activity %2",m_Owner,m_CurrentActivity.GetActionDebugInfo());
#endif
		}
		
		#ifdef AI_DEBUG
		AddDebugMessage("EvaluateActivity END\n");
		#endif
		
		// Rebalance fireteams if needed
		bool isMilitary = IsMilitary();
		if (isMilitary && !m_Owner.IsSlave())
		{
			if (m_FireteamMgr.m_bRebalanceFireteams)
			{
				if (CanRebalanceFireteams()) // In some cases we can't rebalance fireteams yet
				{
					m_FireteamMgr.RebalanceAllFireteams();
				}
			}
		
			// Perception and clusters
			m_fPerceptionUpdateTimer_ms += deltaTime_ms;
			if (m_fPerceptionUpdateTimer_ms > PERCEPTION_UPDATE_TIMER_MS)
			{
				m_Perception.Update();
				UpdateSuppressCluster();
				UpdateThreatMeasure();
				//evaluateTactics();
				EvaluateFlareUsage();
				if (!m_Perception.m_aTargetClusters.IsEmpty())
					UpdateClustersState(m_fPerceptionUpdateTimer_ms);
				
				m_fPerceptionUpdateTimer_ms -= PERCEPTION_UPDATE_TIMER_MS;
			}
		}
		
		m_Tac = m_GroupTactics.GetGroupTactic(m_Owner);
		
		foreach (SCR_AIUtilityComponent util : m_Util)
		{
			util.setTactics(m_Tac);
		}
			
		m_fLastUpdateTime = currentTime;
		m_bNewGroupMemberAdded = false; // resetting reaction on group member OnAgentAdded
		return m_CurrentActivity;
	}
	
	//---------------------------------------------------------------------------------------------------	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		m_Owner = SCR_AIGroup.Cast(owner);
		if (!m_Owner)
			return;	
		
		m_ConfigComponent = SCR_AIConfigComponent.Cast(m_Owner.FindComponent(SCR_AIConfigComponent));
		
		m_ConfigComponent.AddDefaultActivities(this);
		//AddAction(new SCR_AIIdleActivity(this))
		
		m_Owner.GetOnAgentAdded().Insert(OnAgentAdded);
		m_Owner.GetOnAgentRemoved().Insert(OnAgentRemoved);
		m_Owner.GetOnWaypointCompleted().Insert(OnWaypointCompleted);
		m_Owner.GetOnWaypointRemoved().Insert(OnWaypointRemoved);
		m_Owner.GetOnCurrentWaypointChanged().Insert(OnCurrentWaypointChanged);
		
		m_GroupInfo = SCR_AIGroupInfoComponent.Cast(m_Owner.FindComponent(SCR_AIGroupInfoComponent));
		
		
		m_GroupIdentifier = DCO_GroupIdentifierComponent.Cast(m_Owner.FindComponent(DCO_GroupIdentifierComponent));
		m_GroupTactics = DCO_GroupTacticComponent.Cast(m_Owner.FindComponent(DCO_GroupTactic));
		
		m_TargetClusterProcessor = new SCR_AIGroupTargetClusterProcessor(this);
		m_TargetClusterProcessor.m_OnClusterStateChanged.Insert(OnTargetClusterStateChanged);
		
		m_FireteamMgr = new SCR_AIGroupFireteamManager(m_Owner);
		
		m_Perception = new SCR_AIGroupPerception(this, m_Owner);
		m_Perception.GetOnEnemyDetectedFiltered().Insert(OnEnemyDetectedFiltered);
		
		m_VehicleMgr = new SCR_AIGroupVehicleManager();
		
		m_Mailbox = SCR_MailboxComponent.Cast(m_Owner.FindComponent(SCR_MailboxComponent));
		
		m_fPerceptionUpdateTimer_ms = Math.RandomFloat(0, PERCEPTION_UPDATE_TIMER_MS);
		
		m_GroupMovementComponent = SCR_AIGroupMovementComponent.Cast(owner.FindComponent(SCR_AIGroupMovementComponent));
		
		if (m_GroupIdentifier)
		{
			m_Idf = m_GroupIdentifier.GetGroupIndentification(m_Owner);
			foreach (SCR_AIUtilityComponent util : m_Util)
			{
				util.setIdentifier(m_Idf);
			}
			m_GroupIdentifier.automaticIdentification();
		}
		
		if (m_GroupTactics)
		{
			m_Tac = m_GroupTactics.GetGroupTactic(m_Owner);
			foreach (SCR_AIUtilityComponent util : m_Util)
			{
				util.setTactics(m_Tac);
			}
		}		
	}
	
	override void OnAgentAdded(AIAgent agent)
	{
		// Add to array of AIInfo
		SCR_ChimeraAIAgent chimeraAgent = SCR_ChimeraAIAgent.Cast(agent);
		if (!chimeraAgent)
			return;

		SCR_AIInfoComponent info = chimeraAgent.m_InfoComponent;
		
		if (!info)
			return;
		
		m_aInfoComponents.Insert(info);	
		m_FireteamMgr.OnAgentAdded(agent);

		info.m_OnCompartmentEntered.Insert(OnAgentCompartmentEntered);
		info.m_OnCompartmentLeft.Insert(OnAgentCompartmentLeft);
		info.m_OnAgentLifeStateChanged.Insert(OnAgentLifeStateChanged);
		m_bNewGroupMemberAdded = true;
		
		if (info.HasUnitState(EUnitState.IN_VEHICLE))
		{
			OnJoinGroupFromVehicle(agent, info.HasUnitState(EUnitState.PILOT));
		}
		
		SCR_AIUtilityComponent utilityComp = chimeraAgent.m_UtilityComponent;
		
		m_Util.Insert(utilityComp);
	}
	
	override void OnAgentRemoved(SCR_AIGroup group, AIAgent agent)
	{	
		SCR_AIUtilityComponent utility = SCR_AIUtilityComponent.Cast(agent.FindComponent(SCR_AIUtilityComponent));
		if(!utility)
			return Debug.Error("Null AI utility");
			
		if(agent)
			utility.CancelAllGroupActivityBehaviors(this);
		
		// Remove from array of AIInfo
		for (int i = m_aInfoComponents.Count() - 1; i >= 0; i--)
		{
			if (!m_aInfoComponents[i])
			{
				Debug.Error("Null AI info occured"); // investigate when this happens!
				m_aInfoComponents.RemoveOrdered(i);
			}
			else if (m_aInfoComponents[i].IsOwnerAgent(agent))
			{
				// Unsubscribe from compartment event
				SCR_AIInfoComponent infoComp = m_aInfoComponents[i];
				infoComp.m_OnCompartmentEntered.Remove(OnAgentCompartmentEntered);
				infoComp.m_OnCompartmentLeft.Remove(OnAgentCompartmentLeft);
				
				m_Util.RemoveOrdered(i);
				m_aInfoComponents.RemoveOrdered(i);
				break;
			}
		}
		
		m_FireteamMgr.OnAgentRemoved(agent);
	}
	
	DCO_GroupTactic getTactics()
	{
		return m_Tac;
	}
	
	DCO_GroupIdentifer getIdentifier()
	{
		return m_Idf;
	}
	
	SCR_AIGroupPerception getGroupPerception()
	{
		return m_Perception;
	}
}
