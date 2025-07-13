modded class SCR_AIGroupUtilityComponent : SCR_AIBaseUtilityComponent
{
	protected SCR_DCO_AIGroupConfigComponent m_GroupConfig;
	protected float tacticsSuppression = 1;
	protected float tacticsEvaluationTimeStamp;
	
	protected DCO_GroupTactics eActualTactics;
	
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
		
		m_TargetClusterProcessor = new SCR_AIGroupTargetClusterProcessor(this);
		m_TargetClusterProcessor.m_OnClusterStateChanged.Insert(OnTargetClusterStateChanged);
		
		m_FireteamMgr = new SCR_AIGroupFireteamManager(m_Owner);
		
		m_Perception = new SCR_AIGroupPerception(this, m_Owner);
		m_Perception.GetOnEnemyDetectedFiltered().Insert(OnEnemyDetectedFiltered);
		
		m_VehicleMgr = new SCR_AIGroupVehicleManager();
		
		m_Mailbox = SCR_MailboxComponent.Cast(m_Owner.FindComponent(SCR_MailboxComponent));
		
		m_SettingsComponent = SCR_AIGroupSettingsComponent.Cast(m_Owner.FindComponent(SCR_AIGroupSettingsComponent));
		
		m_fPerceptionUpdateTimer_ms = Math.RandomFloat(0, PERCEPTION_UPDATE_TIMER_MS);
		
		m_GroupMovementComponent = SCR_AIGroupMovementComponent.Cast(owner.FindComponent(SCR_AIGroupMovementComponent));
		
		m_GroupConfig = SCR_DCO_AIGroupConfigComponent.Cast(owner.FindComponent(SCR_DCO_AIGroupConfigComponent));
	}
	
	override void SetFireRateCoef(float coef = 1, bool overridePersistent = false)
	{
		m_fFireRateCoef = coef * tacticsSuppression;
		
		foreach (SCR_AIInfoComponent infoComp : m_aInfoComponents)
		{
			SCR_AICombatComponent comp = infoComp.GetCombatComponent();
			if (comp)
				comp.SetGroupFireRateCoef(coef, overridePersistent);
		}
	}
	
	override SCR_AIActionBase EvaluateActivity(out bool restartActivity)
	{
		SCR_AIActionBase activity;
		restartActivity = false;
		
		if (!m_ConfigComponent)
			return null;
		
		float currentTime = GetGame().GetWorld().GetWorldTime();
		float deltaTime_ms = 0;
		tacticsEvaluationTimeStamp += GetGame().GetWorld().GetTimeSlice();
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
		
		TacticsROE();
		SuppressionCoef();
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
		
		// Update group perception, update fire teams
		bool isMilitary = IsMilitary();
		bool isSlave = m_Owner.IsSlave();
		if (isMilitary)
		{
			if (!isSlave && m_FireteamMgr.m_bRebalanceFireteams)
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
				
				if (!isSlave)
				{
					UpdateSuppressCluster();
					UpdateThreatMeasure();
					EvaluateFlareUsage();
					
					if (!m_Perception.m_aTargetClusters.IsEmpty())
						UpdateClustersState(m_fPerceptionUpdateTimer_ms);
				}
				
				m_fPerceptionUpdateTimer_ms -= PERCEPTION_UPDATE_TIMER_MS;
			}
		}
		
		// Combat modes
		if (isMilitary)
			EvaluateCombatMode();
		
		if (ReevaluateTactics())
			TacticsEvaluations();
		
		m_fLastUpdateTime = currentTime;
		m_bNewGroupMemberAdded = false; // resetting reaction on group member added
		
		return m_CurrentActivity;
	}
	
	protected void TacticsROE()
	{
		switch(m_GroupConfig.m_eActualTactics)
		{
			case DCO_GroupTactics.EVASIVE:
			{
				SetCombatMode(EAIGroupCombatMode.RETURN_FIRE);
				return;
			}
			case DCO_GroupTactics.DEFENSIVE:
			{
				SetCombatMode(EAIGroupCombatMode.FIRE_AT_WILL);
				return;
			}
			case DCO_GroupTactics.AGGRESSIVE:
			{
				SetCombatMode(EAIGroupCombatMode.FIRE_AT_WILL);
				return;
			}
			case DCO_GroupTactics.AUTOMATIC:
			{
				SetCombatMode(EAIGroupCombatMode.FIRE_AT_WILL);
				return;
			}
		}
	}
	
	protected void SuppressionCoef()
	{
		switch(m_GroupConfig.GetTactics())
		{
			case DCO_GroupTactics.EVASIVE:
			{
				tacticsSuppression = 0.3;
				break;
			}
			case DCO_GroupTactics.AGGRESSIVE:
			{
				tacticsSuppression = 2.5;
				break;
			}
			case DCO_GroupTactics.DEFENSIVE:
			{
				tacticsSuppression = 1.5;
			}
			default:
			{
				tacticsSuppression = 1;
				break;
			}
		}
	}
	
	protected bool ReevaluateTactics()
	{
		if (tacticsEvaluationTimeStamp > 30000)
		{
			tacticsEvaluationTimeStamp = 0;
		
			return true;
		} else if (m_GroupConfig.m_eExternalTactics != DCO_GroupTactics.AUTOMATIC)
			return true;
		
		return false;
	}
	
	protected void TacticsEvaluations()
	{
		if (m_GroupConfig.m_eExternalTactics != DCO_GroupTactics.AUTOMATIC)
		{
			m_GroupConfig.m_eActualTactics = m_GroupConfig.m_eExternalTactics;
		} 
		else
		{
			int EnemyCount = 0;
			float GroupMoraleAverage = 0;
			float GroupThreatAverage = 0;
			foreach (SCR_AIGroupTargetCluster c : m_Perception.m_aTargetClusters)
			{
				EnemyCount += c.m_State.m_iCountAlive;
			}
			foreach (SCR_AIInfoComponent i : m_aInfoComponents)
			{
				GroupMoraleAverage += i.m_Utility.DCO_MoraleSystem.GetMoraleValue();
				GroupThreatAverage += i.m_Utility.m_ThreatSystem.GetThreatMeasureWithoutInjuryFactor();
			}
			
			GroupMoraleAverage = GroupMoraleAverage/m_Owner.GetAgentsCount();
			GroupThreatAverage = GroupThreatAverage/m_Owner.GetAgentsCount();
			
			if (EnemyCount > m_Owner.GetAgentsCount() * 3)
			{
				m_GroupConfig.m_eActualTactics = DCO_GroupTactics.EVASIVE;
				// Change to Fallback
			}
			else if (EnemyCount > m_Owner.GetAgentsCount() * 2 && GroupThreatAverage > 0.7)
			{
				m_GroupConfig.m_eActualTactics = DCO_GroupTactics.EVASIVE;
			}
			else if (GroupThreatAverage > 0.7 && GroupMoraleAverage < 50)
			{
				m_GroupConfig.m_eActualTactics = DCO_GroupTactics.DEFENSIVE;
			}
			else if (EnemyCount == m_Owner.GetAgentsCount())
			{
				m_GroupConfig.m_eActualTactics = DCO_GroupTactics.DEFENSIVE;
			}
			else if (EnemyCount < m_Owner.GetAgentsCount())
			{
				m_GroupConfig.m_eActualTactics = DCO_GroupTactics.AGGRESSIVE;
			} 
			else if (GroupThreatAverage < 0.2 && GroupMoraleAverage > 60)
			{
				m_GroupConfig.m_eActualTactics = DCO_GroupTactics.AGGRESSIVE;
			}
			else
			{
				m_GroupConfig.m_eActualTactics = DCO_GroupTactics.DEFENSIVE;
			}
			
		}
	}
	
	DCO_GroupTactics GetActualGroupTactics()
	{
		return eActualTactics;
	}
}
