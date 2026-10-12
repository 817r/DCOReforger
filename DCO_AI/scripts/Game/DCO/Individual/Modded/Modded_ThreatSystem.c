modded class SCR_AIThreatSystem
{
	static const float EXPLOSION_MAX_DISTANCE = 120;
	static const float VIGILANT_THRESHOLD = 0.4;
	static const float ALERTED_THRESHOLD = 1;
	static const float THREATENED_THRESHOLD = 2.2;

	private static const float ENDANGERED_INCREMENT = 0.25;



	private static const float SUPPRESSION_BULLET_INCREMENT = 0.11;

	float m_fThreatFlyBy;
	protected vector m_vDCOLastFireDir;
	protected float m_fDCOLastFireDir_ms = -1;
	protected float m_fDCOCrossUntil_ms;
	protected float m_fDCOCrossStress_ms = -1;
	protected float m_fDCODownSince_ms = -1;
	protected float m_fDCOBlastStress_ms = -1;
	protected float m_fDCOHeardUntil_ms = -1;
	protected float m_fDCOAimedStress_ms = -1;
	protected float m_fDCOUnseenClose_ms = -1;

	float DCO_UnseenCloseAt()
	{
		return m_fDCOUnseenClose_ms;
	}

	protected float DCO_CoverDamp()
	{
		IEntity me = m_Utility.m_OwnerEntity;
		if (me && SCR_CoverManagerComponent.IsEntityInsideBuilding(me))
			return DCO_Stress.T(m_Utility, "tm.supp_building_mul", 0.6);
		if (m_Utility.m_CombatMoveState && m_Utility.m_CombatMoveState.IsInValidCover())
			return DCO_Stress.T(m_Utility, "tm.supp_cover_mul", 0.65);
		return 1.0;
	}

	protected float DCO_MoraleMul(bool recovery)
	{
		DCO_AIMoraleSystem ms = m_Utility.GetMoraleSystem();
		if (!ms)
			return 1.0;
		moraleState st = ms.GetState();
		if (recovery)
		{
			if (st >= moraleState.PANIC)
				return DCO_Stress.T(m_Utility, "tm.supp_rec_panic_mul", 0.5);
			if (st == moraleState.SHAKEN)
				return DCO_Stress.T(m_Utility, "tm.supp_rec_shaken_mul", 0.7);
			if (ms.IsSpirited())
				return DCO_Stress.T(m_Utility, "tm.supp_rec_spirited_mul", 1.15);
			return 1.0;
		}
		if (st >= moraleState.PANIC)
			return DCO_Stress.T(m_Utility, "tm.supp_in_panic_mul", 1.5);
		if (st == moraleState.SHAKEN)
			return DCO_Stress.T(m_Utility, "tm.supp_in_shaken_mul", 1.25);
		if (st == moraleState.TENSE)
			return DCO_Stress.T(m_Utility, "tm.supp_in_tense_mul", 1.1);
		if (ms.IsSpirited())
			return DCO_Stress.T(m_Utility, "tm.supp_in_spirited_mul", 0.9);
		return 1.0;
	}

	override void ThreatShotFired(float distance, int count)
	{
		super.ThreatShotFired(distance, count);
		if (distance <= DCO_Stress.T(m_Utility, "tm.heard_floor_m", 300.0))
			m_fDCOHeardUntil_ms = GetGame().GetWorld().GetWorldTime() + DCO_Stress.T(m_Utility, "tm.heard_floor_s", 45.0) * 1000.0;
	}

	protected bool DCO_InCQB()
	{
		if (DCO_CQBMemberBehavior.Cast(m_Utility.m_CurrentBehavior))
			return true;
		AIAgent agent = m_Utility.GetAIAgent();
		if (!agent)
			return false;
		SCR_AIGroupUtilityComponent gu = DCO_AssaultLock.GroupUtil(agent.GetParentGroup());
		return gu && gu.DCO_IsClearingBuilding();
	}

	protected string DCO_VigilFloor()
	{
		if (m_Combat && m_Combat.GetCurrentTarget())
			return "target";
		if (m_Utility.m_CurrentBehavior && SCR_AIObserveThreatSystemBehavior.Cast(m_Utility.m_CurrentBehavior))
			return "observe";
		if (GetGame().GetWorld().GetWorldTime() < m_fDCOHeardUntil_ms)
			return "heard";
		return "";
	}

	protected void DCO_AimedStress()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (m_fDCOAimedStress_ms >= 0 && now - m_fDCOAimedStress_ms < 5000.0)
			return;
		m_fDCOAimedStress_ms = now;
		DCO_AIMoraleSystem ms = m_Utility.GetMoraleSystem();
		if (ms)
			ms.AddStress(DCO_Stress.T(m_Utility, "tm.morale_aimed", 0.04), "aimed_at");
	}

	static float DCO_MissWeight(float miss)
	{
		if (miss < 1.0)
			return 1.0;
		if (miss < 3.0)
			return 0.8;
		if (miss < 6.0)
			return 0.5;
		return 0;
	}

	override float GetSuppressionMeasure()
	{
		return Math.Clamp(m_fThreatSuppression + m_fThreatFlyBy, 0, 2.5);
	}

	float DCO_GetFlyby()
	{
		return m_fThreatFlyBy;
	}

	protected float DCO_Crossfire(vector shooterPos)
	{
		IEntity me = m_Utility.m_OwnerEntity;
		float now = GetGame().GetWorld().GetWorldTime();
		if (me && shooterPos != vector.Zero)
		{
			vector dir = shooterPos - me.GetOrigin();
			dir[1] = 0;
			if (dir.Length() > 0.5)
			{
				dir.Normalize();
				if (m_fDCOLastFireDir_ms >= 0 && now - m_fDCOLastFireDir_ms < 10000.0 && vector.Dot(dir, m_vDCOLastFireDir) < 0)
				{
					m_fDCOCrossUntil_ms = now + 10000.0;
					if (m_fDCOCrossStress_ms < 0 || now - m_fDCOCrossStress_ms >= 10000.0)
					{
						m_fDCOCrossStress_ms = now;
						DCO_Perf.Count("threat:crossfire");
						DCO_AIMoraleSystem ms = m_Utility.GetMoraleSystem();
						if (ms)
							ms.AddStress(0.3, "crossfire");
					}
				}
				m_vDCOLastFireDir = dir;
				m_fDCOLastFireDir_ms = now;
			}
		}
		if (now < m_fDCOCrossUntil_ms)
			return 1.3;
		return 1.0;
	}

	protected bool DCO_UnseenShooter(IEntity shooter)
	{
		if (!shooter)
			return false;
		IEntity root = shooter.GetRootParent();
		if (m_Combat && m_Combat.GetCurrentTarget() && m_Combat.GetCurrentTarget().GetTargetEntity() == root)
			return false;
		if (m_Utility.m_PerceptionComponent && m_Utility.m_PerceptionComponent.FindTargetPerceptionObject(root))
			return false;
		DCO_Perf.Count("morale:unseen_shooter");
		return true;
	}

	protected bool DCO_RetfireOn()
	{
		return DCO_Stress.T(m_Utility, "tm.retfire_chance", 0.0) > 0 || DCO_Stress.T(m_Utility, "agg.retfire", 1.0) > 0;
	}

	protected bool DCO_UnseenForRetfire(IEntity shooter)
	{
		if (!shooter)
			return false;
		IEntity root = shooter.GetRootParent();
		if (m_Combat && m_Combat.GetCurrentTarget() && m_Combat.GetCurrentTarget().GetTargetEntity() == root)
			return false;
		if (!m_Utility.m_PerceptionComponent)
			return true;
		BaseTarget t = m_Utility.m_PerceptionComponent.FindTargetPerceptionObject(root);
		if (!t)
		{
			DCO_Perf.Count("tm:retfire_unseen_none");
			return true;
		}
		if (t.GetTimeSinceSeen() >= DCO_Stress.T(m_Utility, "tm.retfire_unseen_s", 5.0))
		{
			DCO_Perf.Count("tm:retfire_unseen_heard");
			return true;
		}
		DCO_Perf.Count("tm:retfire_skip_seen");
		return false;
	}

	void DCO_ThreatImpact(int count, float w, vector shooterPos, IEntity shooter)
	{
		if (w <= 0 || DCO_VehicleCombat.IsProtectedFromBullets(m_Utility.m_OwnerEntity))
			return;
		w *= DCO_Ctx.ThreatW(m_Utility, shooterPos);
		float cf = DCO_Crossfire(shooterPos);
		if (m_Combat)
			m_Combat.DCO_PinNearMiss(w, shooterPos);
		m_fThreatSuppression =Math.Clamp(m_fThreatSuppression + SUPPRESSION_BULLET_INCREMENT * count * w * cf * DCO_CoverDamp() * DCO_MoraleMul(false), 0, 2.5);
		m_Combat.DangerSuppressedDecreaseAIM(count);
		DCO_AIMoraleSystem ms = m_Utility.GetMoraleSystem();
		if (m_Combat && shooterPos != vector.Zero && DCO_RetfireOn() && DCO_UnseenForRetfire(shooter))
			m_Combat.DCO_ReturnFireUnseen(shooterPos);
		if (DCO_UnseenShooter(shooter))
		{
			if (ms)
				ms.DCO_NoteUnseen();
			if (m_Utility.m_OwnerEntity)
				m_fDCOUnseenClose_ms = GetGame().GetWorld().GetWorldTime();
			if (shooterPos != vector.Zero)
				DCO_FireEstimate.Report(m_Utility.GetAIAgent(), m_Utility.m_OwnerEntity.GetOrigin(), shooterPos, GetGame().GetWorld().GetWorldTime());
		}
	}

	void DCO_ThreatFlyby(int count, float w, vector shooterPos, IEntity shooter)
	{
		if (w <= 0 || DCO_VehicleCombat.IsProtectedFromBullets(m_Utility.m_OwnerEntity))
			return;
		w *= DCO_Ctx.ThreatW(m_Utility, shooterPos);
		float cf = DCO_Crossfire(shooterPos);
		if (m_Combat)
			m_Combat.DCO_PinNearMiss(w, shooterPos);
		m_fThreatFlyBy =Math.Clamp(m_fThreatFlyBy + SUPPRESSION_BULLET_INCREMENT * count * w * cf * DCO_CoverDamp() * DCO_MoraleMul(false), 0, 1.2);
		m_Combat.DangerSuppressedDecreaseAIM(0.5 * count);
		DCO_AIMoraleSystem ms = m_Utility.GetMoraleSystem();
		if (m_Combat && shooterPos != vector.Zero && DCO_RetfireOn() && DCO_UnseenForRetfire(shooter))
			m_Combat.DCO_ReturnFireUnseen(shooterPos);
		if (DCO_UnseenShooter(shooter))
		{
			if (ms)
				ms.DCO_NoteUnseen();
			if (shooterPos != vector.Zero && m_Utility.m_OwnerEntity && vector.Distance(shooterPos, m_Utility.m_OwnerEntity.GetOrigin()) < DCO_Stress.T(m_Utility, "heal.unseen_m", 25.0))
				m_fDCOUnseenClose_ms = GetGame().GetWorld().GetWorldTime();
			if (shooterPos != vector.Zero)
				DCO_FireEstimate.Report(m_Utility.GetAIAgent(), m_Utility.m_OwnerEntity.GetOrigin(), shooterPos, GetGame().GetWorld().GetWorldTime());
		}
	}

	override void ThreatBulletImpact(int count)
	{
		DCO_ThreatImpact(count, 1.0, vector.Zero, null);
	}

	override void ThreatProjectileFlyby(int count)
	{
		DCO_ThreatFlyby(count, 1.0, vector.Zero, null);
	}

	override void ThreatExplosion(float distance)
	{
		DCO_ThreatExplosion(distance, 1.0);
		if (distance >= 50.0 || !m_Utility || !m_Utility.m_OwnerEntity || DCO_VehicleCombat.IsProtectedFromBullets(m_Utility.m_OwnerEntity))
			return;
		float w = 0.5;
		float stress = 0.1;
		if (distance < 15.0)
		{
			w = 1.0;
			stress = 0.3;
		}
		if (m_Combat)
			m_Combat.DCO_PinNearMiss(w, vector.Zero);
		float now = GetGame().GetWorld().GetWorldTime();
		if (m_fDCOBlastStress_ms >= 0 && now - m_fDCOBlastStress_ms < 3000.0)
			return;
		m_fDCOBlastStress_ms = now;
		DCO_AIMoraleSystem ms = m_Utility.GetMoraleSystem();
		if (ms)
			ms.AddStress(stress, "explosion");
	}

	void DCO_ThreatExplosion(float distance, float scale)
	{
		float inc;
		if (distance < 15.0)
		{
			inc = 0.8;
			DCO_Perf.Count("threat:explosion_close");
		}
		else if (distance < 50.0)
			inc = Math.Map(distance, 15.0, 50.0, 0.3, 0.1);
		else if (distance < 120.0)
			inc = Math.Map(distance, 50.0, 120.0, 0.1, 0);
		if (inc <= 0)
			return;
		m_fThreatSuppression = Math.Clamp(m_fThreatSuppression + inc * scale * DCO_CoverDamp() * DCO_MoraleMul(false), 0, 2.5);
	}

	protected void DCO_UpdateState()
	{
		EAIThreatState raw = EAIThreatState.SAFE;
		if (m_fThreatTotal > THREATENED_THRESHOLD)
			raw = EAIThreatState.THREATENED;
		else if (m_fThreatTotal > ALERTED_THRESHOLD)
			raw = EAIThreatState.ALERTED;
		else if (m_fThreatTotal > VIGILANT_THRESHOLD)
			raw = EAIThreatState.VIGILANT;

		if (raw < EAIThreatState.ALERTED && DCO_InCQB())
		{
			raw = EAIThreatState.ALERTED;
			if (m_State < EAIThreatState.ALERTED)
			{
				DCO_Perf.Count("threat:cqb_floor");
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("threat_floor unit=%1 why=cqb from=%2 total=%3", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), typename.EnumToString(EAIThreatState, m_State), m_fThreatTotal.ToString(-1, 2)));
			}
		}

		if (raw == EAIThreatState.SAFE)
		{
			string floorWhy = DCO_VigilFloor();
			if (!floorWhy.IsEmpty())
			{
				raw = EAIThreatState.VIGILANT;
				if (m_State == EAIThreatState.SAFE)
				{
					DCO_Perf.Count("threat:vigil_floor_" + floorWhy);
					if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
						DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("threat_floor unit=%1 why=%2 total=%3", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), floorWhy, m_fThreatTotal.ToString(-1, 2)));
				}
			}
		}

		if (raw >= m_State)
		{
			m_fDCODownSince_ms = -1;
			StateTransition(raw);
			return;
		}
		float now = GetGame().GetWorld().GetWorldTime();
		if (m_fDCODownSince_ms < 0)
		{
			m_fDCODownSince_ms = now;
			DCO_Perf.Count("threat:hysteresis_hold");
			return;
		}
		if (now - m_fDCODownSince_ms < 3000.0)
			return;
		m_fDCODownSince_ms = -1;
		StateTransition(raw);
	}

	protected bool DCO_TargetFacingMe(BaseTarget t)
	{
		IEntity te = t.GetTargetEntity();
		IEntity me = m_Utility.m_OwnerEntity;
		if (!te || !me)
			return false;
		vector fwd = te.GetWorldTransformAxis(2);
		vector toMe = me.GetOrigin() - te.GetOrigin();
		fwd[1] = 0;
		toMe[1] = 0;
		if (fwd.Length() < 0.1 || toMe.Length() < 0.1)
			return false;
		return vector.Dot(fwd.Normalized(), toMe.Normalized()) > 0.866;
	}

	protected static float Falloff(float value, float ratePerMs, float timeSliceMs)
	{
		return value * Math.Max(0.0, 1.0 - ratePerMs * timeSliceMs);
	}

	override void Update(SCR_AIUtilityComponent utility, float timeSlice)
	{
		float suppDrop = DCO_Stress.T(m_Utility, "tm.supp_drop_s", 0.06) * 0.001 * DCO_MoraleMul(true);
		m_fThreatSuppression = Falloff(m_fThreatSuppression, suppDrop, timeSlice);
		m_fThreatShotsFired = Falloff(m_fThreatShotsFired, THREAT_SHOT_DROP_RATE, timeSlice);
		m_fThreatFlyBy = Falloff(m_fThreatFlyBy, suppDrop, timeSlice);

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

				float endangeredTargetValue = 0;
				if (endangeredTarget.IsEndangering() || endangeredTarget.GetTimeSinceEndangered() < 10.0)
				{
					endangeredTargetValue = ENDANGERED_INCREMENT * distScale * seenScale;
					DCO_AimedStress();
				}
				else if (sinceSeen < 2.0 && DCO_TargetFacingMe(endangeredTarget))
					endangeredTargetValue = ENDANGERED_INCREMENT * distScale * seenScale * 0.3;
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

		DCO_UpdateState();
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