modded class SCR_AIGroupUtilityComponent : SCR_AIBaseUtilityComponent
{
	SCR_AIGroup m_Owner;
	SCR_AIConfigComponent m_ConfigComponent;
	SCR_AIGroupInfoComponent m_GroupInfo;
	SCR_MailboxComponent m_Mailbox;
	ref array<SCR_AIInfoComponent> m_aInfoComponents = {};
	
	ref ScriptInvoker_GroupMoveFailed m_OnMoveFailed;
	
	protected float m_fLastUpdateTime = -1.0;
	protected float m_fPerceptionUpdateTimer_ms;
	
	// Update interval of group perception and target clusters and their processing
	protected const float PERCEPTION_UPDATE_TIMER_MS = 500.0;
	
	protected bool m_bNewGroupMemberAdded;
	protected ref SCR_AIActionBase m_CurrentActivity;
	
	// Waypoint state
	protected ref SCR_AIWaypointState m_WaypointState;
	
	// Group perception and clusters
	ref SCR_AIGroupPerception m_Perception;
	
	protected ref SCR_AIGroupTargetClusterProcessor m_TargetClusterProcessor;
	
	// Fireteams
	ref SCR_AIGroupFireteamManager m_FireteamMgr;
	
	// Used by SCR_AIGetMemberByGoal nodes
	int m_iGetMemberByGoalNextIndex = 0;
	
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
		// Rebalance fireteams if needed
		if (m_FireteamMgr.m_bRebalanceFireteams)
		{
			if (CanRebalanceFireteams()) // In some cases we can't rebalance fireteams yet
			{
				m_FireteamMgr.RebalanceFireteams();
			}
		}
		
		// Perception and clusters
		m_fPerceptionUpdateTimer_ms += deltaTime_ms;
		if (m_fPerceptionUpdateTimer_ms > PERCEPTION_UPDATE_TIMER_MS)
		{
			m_Perception.Update();
			if (!m_Perception.m_aTargetClusters.IsEmpty())
				UpdateClustersState(m_fPerceptionUpdateTimer_ms);
			
			m_fPerceptionUpdateTimer_ms -= PERCEPTION_UPDATE_TIMER_MS;
		}
			
		m_fLastUpdateTime = currentTime;
		m_bNewGroupMemberAdded = false; // resetting reaction on group member added
		
		return m_CurrentActivity;
	}
}
