modded class SCR_AIThreatSystem
{
	static const float EXPLOSION_MAX_DISTANCE = 120;
	
	private static const float ENDANGERED_INCREMENT = 0.25;
	
	static const float VIGILANT_THRESHOLD = 0.4;
	static const float ALERTED_THRESHOLD = 1;
	static const float THREATENED_THRESHOLD = 2.2;
	
	private static const float BLEEDING_FIXED_INCREMENT = 0.7;
	
	private static const float SUPPRESSION_BULLET_INCREMENT = 0.11;
	private static const float ZERO_DISTANCE_SHOT_INCREMENT = 0.01;
	private static const float DISTANT_SHOT_INCREMENT = 0.005;
	private static const float EXPLOSION_MAX_INCREMENT = 0.35;
	
	float m_fThreatFlyBy;
	
	private static const float ENDANGERED_DIST_NEAR = 25.0;
	private static const float ENDANGERED_DIST_FAR = 300.0;
	private static const float ENDANGERED_DIST_NEAR_SCALE = 2.0;
	private static const float ENDANGERED_DIST_FAR_SCALE = 0.4;
	
	private static const float ENDANGERED_SEEN_FRESH_S = 2.0;
	private static const float ENDANGERED_SEEN_STALE_S = 11.0;
	private static const float ENDANGERED_SEEN_STALE_SCALE = 0.4;
	
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
		// === FIXED: dulu `Clamp(m_fThreatSuppression + count * INC, 0, 1.2)` -- base-nya
		// m_fThreatSuppression (bukan m_fThreatFlyBy) jadi gak akumulatif, dan nilai itu
		// nyangkut permanen karena gak ada falloff. Sekarang akumulatif dari dirinya sendiri,
		// cap 1.2 tetap, falloff ditambah di Update().
		m_fThreatFlyBy = Math.Clamp(m_fThreatFlyBy + count * SUPPRESSION_BULLET_INCREMENT, 0, 1.2);
		// === END FIXED ===
		m_Combat.DangerSuppressedDecreaseAIM(count/2);
		m_Utility.GetMoraleSystem().ThreatProjectileFlyby(count);
	}
	
	override void Update(SCR_AIUtilityComponent utility, float timeSlice)
	{
		// Threat falloff
		m_fThreatSuppression -= m_fThreatSuppression * THREAT_SUPPRESSION_DROP_RATE * timeSlice;
		m_fThreatShotsFired -= m_fThreatShotsFired * THREAT_SHOT_DROP_RATE * timeSlice;
		
		// === ADDED: falloff buat flyby, rate sama kayak suppression ===
		m_fThreatFlyBy -= m_fThreatFlyBy * THREAT_SUPPRESSION_DROP_RATE * timeSlice;
		m_fThreatFlyBy = Math.Max(m_fThreatFlyBy, 0.0);
		// === END ADDED ===
		
		if (m_Combat)
		{
			// === FIXED: dulu flat `m_fThreatIsEndangered = ENDANGERED_INCREMENT` begitu punya
			// target, gak peduli jarak / seberapa basi info-nya. Sekarang diskala jarak +
			// visibilitas. Naik langsung (reaksi cepet), turun pelan pakai drop rate vanilla
			// (gak kedip-kedip pas target sebentar ketutup).
			BaseTarget endangeredTarget = m_Combat.GetCurrentTarget();
			if (endangeredTarget)
			{
				float targetDist = endangeredTarget.GetDistance();
				float distClamped = Math.Clamp(targetDist, ENDANGERED_DIST_NEAR, ENDANGERED_DIST_FAR);
				float distScale = Math.Map(distClamped, ENDANGERED_DIST_NEAR, ENDANGERED_DIST_FAR, ENDANGERED_DIST_NEAR_SCALE, ENDANGERED_DIST_FAR_SCALE);
				
				float sinceSeen = endangeredTarget.GetTimeSinceSeen();
				float seenClamped = Math.Clamp(sinceSeen, ENDANGERED_SEEN_FRESH_S, ENDANGERED_SEEN_STALE_S);
				float seenScale = Math.Map(seenClamped, ENDANGERED_SEEN_FRESH_S, ENDANGERED_SEEN_STALE_S, 1.0, ENDANGERED_SEEN_STALE_SCALE);
				
				float endangeredTargetValue = ENDANGERED_INCREMENT * distScale * seenScale;
				float endangeredDecayed = m_fThreatIsEndangered - m_fThreatIsEndangered * THREAT_ENDANGERED_DROP_RATE * timeSlice;
				m_fThreatIsEndangered = Math.Max(endangeredTargetValue, endangeredDecayed);
			}
			// === END FIXED ===
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