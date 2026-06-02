modded class SCR_AIThreatSystem
{
	static const float EXPLOSION_MAX_DISTANCE = 80;
	
	private static const float ENDANGERED_INCREMENT = 0.3;
	
	static const float VIGILANT_THRESHOLD = 0.3;
	static const float ALERTED_THRESHOLD = 0.9;
	static const float THREATENED_THRESHOLD = 1.9;
	
	private static const float BLEEDING_FIXED_INCREMENT = 0.7;
	
	private static const float SUPPRESSION_BULLET_INCREMENT = 0.15;
	private static const float ZERO_DISTANCE_SHOT_INCREMENT = 0.018;
	private static const float DISTANT_SHOT_INCREMENT = 0.005;
	private static const float EXPLOSION_MAX_INCREMENT = 0.35;
	
	float m_fThreatFlyBy;
	
	override void ThreatBulletImpact(int count)
	{
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("ThreatBulletImpact: %1", count));
		#endif
		
		m_fThreatSuppression = Math.Clamp(m_fThreatSuppression + count*SUPPRESSION_BULLET_INCREMENT, 0, 2.5);
		m_Combat.DangerSuppressedDecreaseAIM(count);
		m_Utility.GetMoraleSystem().ThreatBulletImpact(count);
	}
	
	override void ThreatProjectileFlyby(int count)
	{
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("ThreatProjectileFlyby"));
		#endif
		m_fThreatFlyBy = Math.Clamp(m_fThreatSuppression + count * SUPPRESSION_BULLET_INCREMENT, 0, 1.2);
		m_Combat.DangerSuppressedDecreaseAIM(count/2);
		m_Utility.GetMoraleSystem().ThreatProjectileFlyby(count);
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
			int i;
			AIDangerEvent dangerEvent;
			
#ifdef AI_DEBUG
			if (m_Agent.GetDangerEventsCount() != 0)
				AddDebugMessage(string.Format("Processing danger events: %1 in the queue", m_Agent.GetDangerEventsCount()));
#endif
			
			for (int max = m_Agent.GetDangerEventsCount(); i < max; i++)
			{
				int eventAggregationCount;
				dangerEvent = m_Agent.GetDangerEvent(i, eventAggregationCount);
				
				
				#ifdef AI_DEBUG
				AddDebugMessage(string.Format("PerformDangerReaction: %1x %2", eventAggregationCount, dangerEvent));
				#endif
				
				if (dangerEvent)
				{
					if (m_Config.PerformDangerReaction(m_Utility, dangerEvent, eventAggregationCount))
					{
#ifdef WORKBENCH
						string message = typename.EnumToString(EAIDangerEventType, dangerEvent.GetDangerType());
						SCR_AIDebugVisualization.VisualizeMessage(m_Utility.m_OwnerEntity, message, EAIDebugCategory.DANGER, 2);	// Show message above AI's head
#endif
					}
				}
			}

			m_Agent.ClearDangerEvents(i + 1);
		}		

		// Add threat value from current behavior
		float threatFromBehavior;
		if (utility.m_CurrentBehavior)
			threatFromBehavior = utility.m_CurrentBehavior.m_fThreat;
		
		m_fThreatTotal = Math.Clamp(threatFromBehavior + m_fThreatSuppression + m_fThreatFlyBy + m_fThreatInjury + m_fThreatShotsFired + m_fThreatIsEndangered, 0, 2.5);
		
		UpdateState();
#ifdef WORKBENCH
		ShowDebug();
#endif
	}
	
#ifdef WORKBENCH
	override void ShowDebug()
	{
		// Show message above AI's head

		Color color;
		
		switch (m_State)
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
		}
		
		SCR_AIDebugVisualization.VisualizeMessage(m_Utility.m_OwnerEntity, m_fThreatTotal.ToString(), EAIDebugCategory.THREAT, 1.4, color);	
	}
#endif
}