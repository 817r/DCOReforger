modded class SCR_AIThreatSystem
{

	private static const float ENDANGERED_INCREMENT = 0.25;



	private static const float SUPPRESSION_BULLET_INCREMENT = 0.11;

	float m_fThreatFlyBy;



	override void ThreatBulletImpact(int count)
	{
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("ThreatBulletImpact: %1", count));
		#endif

		if (DCO_VehicleCombat.IsProtectedFromBullets(m_Utility.m_OwnerEntity))
			return;

		m_fThreatSuppression = Math.Clamp(m_fThreatSuppression + count*SUPPRESSION_BULLET_INCREMENT, 0, 2.5);
		m_Combat.DangerSuppressedDecreaseAIM(count);
		m_Utility.GetMoraleSystem().ThreatBulletImpact(count);
	}

	override void ThreatProjectileFlyby(int count)
	{
		#ifdef AI_DEBUG
		AddDebugMessage(string.Format("ThreatProjectileFlyby"));
		#endif

		if (DCO_VehicleCombat.IsProtectedFromBullets(m_Utility.m_OwnerEntity))
			return;
		m_fThreatFlyBy = Math.Clamp(m_fThreatFlyBy + count * SUPPRESSION_BULLET_INCREMENT, 0, 1.2);
		m_Combat.DangerSuppressedDecreaseAIM(count * 0.5);
		m_Utility.GetMoraleSystem().ThreatProjectileFlyby(count);
	}

	protected static float Falloff(float value, float ratePerMs, float timeSliceMs)
	{
		return value * Math.Max(0.0, 1.0 - ratePerMs * timeSliceMs);
	}

	override void Update(SCR_AIUtilityComponent utility, float timeSlice)
	{
		m_fThreatSuppression = Falloff(m_fThreatSuppression, THREAT_SUPPRESSION_DROP_RATE, timeSlice);
		m_fThreatShotsFired = Falloff(m_fThreatShotsFired, THREAT_SHOT_DROP_RATE, timeSlice);
		m_fThreatFlyBy = Falloff(m_fThreatFlyBy, THREAT_SUPPRESSION_DROP_RATE, timeSlice);

		if (m_Combat)
		{
			BaseTarget endangeredTarget = m_Combat.GetCurrentTarget();
			if (endangeredTarget)
			{
				float targetDist = endangeredTarget.GetDistance();
				float distClamped = Math.Clamp(targetDist, 25.0, 300.0);
				float distScale = Math.Map(distClamped, 25.0, 300.0, 2.0, 0.4);

				float sinceSeen = endangeredTarget.GetTimeSinceSeen();
				float seenClamped = Math.Clamp(sinceSeen, 2.0, 11.0);
				float seenScale = Math.Map(seenClamped, 2.0, 11.0, 1.0, 0.4);

				float endangeredTargetValue = ENDANGERED_INCREMENT * distScale * seenScale;
				float endangeredDecayed = Falloff(m_fThreatIsEndangered, THREAT_ENDANGERED_DROP_RATE, timeSlice);
				m_fThreatIsEndangered = Math.Max(endangeredTargetValue, endangeredDecayed);
			}
			else
				m_fThreatIsEndangered = Falloff(m_fThreatIsEndangered, THREAT_ENDANGERED_DROP_RATE, timeSlice);
		}

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
						SCR_AIDebugVisualization.VisualizeMessage(m_Utility.m_OwnerEntity, message, EAIDebugCategory.DANGER, 2);
#endif
					}
				}
			}

			m_Agent.ClearDangerEvents(i + 1);
		}

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