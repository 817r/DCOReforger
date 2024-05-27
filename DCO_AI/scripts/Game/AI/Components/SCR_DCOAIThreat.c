modded enum EAIThreatState
{
	PINNED,
	EXHAUSTED
};

typedef func SCR_AIThreatStatesChangedCallback;
void SCR_AIThreatStatesChangedCallback(EAIThreatState prevState, EAIThreatState newStates);
typedef ScriptInvokerBase<SCR_AIThreatStatesChangedCallback> SCR_AIThreatStatesChangedInvoker;

modded class SCR_AIThreatSystem
{
	private DCO_AIMoraleSystem m_moraleSystem;
	
	static const float PINNED_THRESHOLD = 1.4;
	static const float EXHAUSTED_THRESHOLD = 2.5;
	
	private static const float SUPPRESSION_BULLET_INCREMENT = 0.11;
	private static const float ENDANGERED_INCREMENT = 0.4;
	private static const float BLEEDING_FIXED_INCREMENT = 0.2;
	private static const float SUPPRESSION_BULLET_INCREMENT = 0.10;
	private static const float ZERO_DISTANCE_SHOT_INCREMENT = 0.008;
	private static const float DISTANT_SHOT_INCREMENT = 0.0006;
	private static const float EXPLOSION_MAX_INCREMENT = 0.8;
	private static const float EXPLOSION_CLOSE_DISTANCE = 15;	//!< What distance in m is considered close - max increment is used
	static const float EXPLOSION_MAX_DISTANCE = 500;
	
	//private static const float THREAT_PINNED_DROP__RATE = 0.08 * 0.001;
	//private static const float THREAT_EXHAUSTED_DROP_RATE = 0.03 * 0.001;
	//private static const float THREAT_ENDANGERED_DROP_RATE  = 0.12 * 0.001;
	//private static const float THREAT_SUPPRESSION_DROP_RATE = 0.25 * 0.001; 
	
	private static const float THREAT_SHOT_DROP_RATE = 	0.08 * 0.001; // Falloff (percentual drop per milisecond)
	private static const float THREAT_SUPPRESSION_DROP_RATE = 0.05 * 0.001;
	private static const float THREAT_ENDANGERED_DROP_RATE = 	0.08 * 0.001;
	
	private EAIThreatState m_States;
	
	private ref SCR_AIThreatStatesChangedInvoker m_OnThreatStatesChanged = new SCR_AIThreatStatesChangedInvoker();
	
	#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	//!
	
	void SCR_AIThreatSystem(SCR_AIUtilityComponent utility)
	{
		m_Utility = utility;
		m_Config = utility.m_ConfigComponent;	
		m_Combat = utility.m_CombatComponent;
		m_DamageManager = SCR_DamageManagerComponent.Cast(utility.m_OwnerEntity.FindComponent(SCR_DamageManagerComponent));
		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
		if (!agent)
			return;
		m_Agent = agent;
		
		// AI threat system is owned by Utility Component, therefore we don't unsubscribe from the event
		if (m_DamageManager)
		{
			m_DamageManager.GetOnDamageOverTimeAdded().Insert(OnDamageOverTimeAdded);
			m_DamageManager.GetOnDamageOverTimeRemoved().Insert(OnDamageOverTimeRemoved);
		}
			
		m_States = EAIThreatState.SAFE;
	}
	
	SCR_AIThreatStatesChangedInvoker GetOnThreatStatesChanged()
	{
		return m_OnThreatStatesChanged;
	}
	
	override EAIThreatState GetState()
	{
		return m_States;
	}
	
	override void ShowDebug()
	{
		// Show message above AI's head

		Color color;
		
		switch (m_States)
		{
			case EAIThreatState.SAFE:
			{
				color = Color.FromInt(Color.GREEN);
				break;
			}
			case EAIThreatState.VIGILANT:
			{
				color = Color.FromInt(Color.DARK_GREEN);
				break;
			}
			case EAIThreatState.ALERTED:
			{
				color = Color.FromInt(Color.DARK_YELLOW);
				break;
			}
			case EAIThreatState.THREATENED:
			{
				color = Color.FromInt(Color.DARK_RED);
				break;
			}
			case EAIThreatState.PINNED:
			{
				color = Color.FromInt(Color.MAGENTA);
				break;
			}
			case EAIThreatState.EXHAUSTED:
			{
				color = Color.FromInt(Color.DARK_BLUE);
				break;
			}
		}
		
		SCR_AIDebugVisualization.VisualizeMessage(m_Utility.m_OwnerEntity, m_fThreatTotal.ToString(), EAIDebugCategory.THREAT, 1.4, color);	
		//SCR_AIDebugVisualization.VisualizeMessage(m_Utility.m_OwnerEntity, typename.EnumToString(EAIThreatState, m_State), EAIDebugCategory.THREAT, 1.4, color);
	}
#endif // WORKBENCH
	
	float GetThreatTotal()
	{
		return m_fThreatTotal;
	}
	
	float GetThreatInjury()
	{
		return m_fThreatInjury;
	}
	
	float GetThreatShotsFired()
	{
		return m_fThreatShotsFired;
	}
	
	float GetThreatSuppression()
	{
		return m_fThreatSuppression;
	}
	
	float GetThreatEndangered()
	{
		return m_fThreatIsEndangered;
	}
	
	private void StateTransitions(EAIThreatState newStates)
	{
		if (newStates == m_States)
			return;
		
		m_OnThreatStatesChanged.Invoke(m_States, newStates);
		
		m_States = newStates;
	}
	
	private void UpdateStates()
	{
		EAIThreatState newStates = EAIThreatState.SAFE;
		
		if (m_fThreatTotal > EXHAUSTED_THRESHOLD)
			newStates = EAIThreatState.EXHAUSTED;
		else if (m_fThreatTotal > PINNED_THRESHOLD)
			newStates = EAIThreatState.PINNED;
		else if (m_fThreatTotal > THREATENED_THRESHOLD)
			newStates = EAIThreatState.THREATENED;
		else if (m_fThreatTotal > ALERTED_THRESHOLD)
			newStates = EAIThreatState.ALERTED;
		else if (m_fThreatTotal > VIGILANT_THRESHOLD)
			newStates = EAIThreatState.VIGILANT;

		StateTransitions(newStates);
	}
	
	override void Update(SCR_AIUtilityComponent utility, float timeSlice)
	{
		// Threat falloff
		m_fThreatSuppression -= m_fThreatSuppression * THREAT_SUPPRESSION_DROP_RATE * timeSlice;
		m_fThreatShotsFired -= m_fThreatShotsFired * THREAT_SHOT_DROP_RATE * timeSlice;
		
		if (m_Combat)
		{
			if (m_Combat.GetCurrentTarget())
				m_fThreatIsEndangered = ENDANGERED_INCREMENT;
			else
				m_fThreatIsEndangered -= m_fThreatIsEndangered * THREAT_ENDANGERED_DROP_RATE * timeSlice;
		}

		// Process all danger events and clear the array
		if (m_Agent && m_Config.m_EnableDangerEvents)
		{
			bool handled;
			int i = 0;
			for (; i < m_Agent.GetDangerEventsCount(); i++)
			{
				AIDangerEvent dangerEvent = m_Agent.GetDangerEvent(i);
				
				if (dangerEvent)
				{
					#ifdef AI_DEBUG
					AddDebugMessage(string.Format("PerformDangerReaction: %1, %2", dangerEvent, typename.EnumToString(EAIDangerEventType, dangerEvent.GetDangerType())));
					#endif
					
					if (m_Config.PerformDangerReaction(m_Utility, dangerEvent))
					{
#ifdef WORKBENCH	
						string message = typename.EnumToString(EAIDangerEventType, dangerEvent.GetDangerType());
						SCR_AIDebugVisualization.VisualizeMessage(m_Utility.m_OwnerEntity, message, EAIDebugCategory.DANGER, 1);	// Show message above AI's head
#endif
						break;
					}
				}
			}
			m_Agent.ClearDangerEvents(i+1);
		}		

		// Add threat value from current behavior
		float threatFromBehavior = 0;
		if (utility.m_CurrentBehavior)
			threatFromBehavior = utility.m_CurrentBehavior.m_fThreat;
		
		m_fThreatTotal = Math.Clamp(threatFromBehavior + m_fThreatSuppression + m_fThreatInjury + m_fThreatShotsFired + m_fThreatIsEndangered, 0, 3.0);
		
		UpdateStates();
#ifdef WORKBENCH
		ShowDebug();
#endif
	}
	
	override void ThreatBulletImpact(int count)
	{
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("ThreatBulletImpact: %1", count));
		#endif
		
		m_fThreatSuppression = Math.Clamp(m_fThreatSuppression + count*SUPPRESSION_BULLET_INCREMENT, 0, 2.4);
	}
	
	override void ThreatShotFired(float distance, int count)
	{
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("ThreatShotFired: %1, %2", distance, count));
		#endif
		
		// google can show you the increment function if you write it in
		m_fThreatShotsFired = Math.Clamp(m_fThreatShotsFired + count*(DISTANT_SHOT_INCREMENT + ZERO_DISTANCE_SHOT_INCREMENT/(distance + 1)), 0, PINNED_THRESHOLD);
	}
	
	override void ThreatProjectileFlyby(int count)
	{
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("ThreatProjectileFlyby"));
		#endif
		
		m_fThreatSuppression = Math.Clamp(m_fThreatSuppression + count * SUPPRESSION_BULLET_INCREMENT, 0, 2.0);
	}
	
	void decreaseMoraleWeaponFired(int counts)
	{
		m_moraleSystem.ThreatProjectileFlyby(counts);
	}
	
	override void DebugPrintToWidget(TextWidget w)
	{
		w.SetText(
			typename.EnumToString(EAIThreatState, m_States) + "\n "
			+ m_fThreatTotal.ToString(1,4) + "\n "
			+ m_fThreatSuppression.ToString(1,4) + "\n "
			+ m_fThreatShotsFired.ToString(1,4) + "\n "
			+ m_fThreatInjury.ToString(1,4) + "\n "
			+ m_fThreatIsEndangered.ToString(1,4));
		;
	}
};