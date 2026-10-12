enum moraleState
{
	CALM,
	TENSE,
	SHAKEN,
	PANIC,
	BREAK
}

class DCO_AIMoraleSystem
{
	private float m_fMoraleTotal;
	private float m_fMoraleSuppression;
	private float m_fMoraleInjury;
	private float m_fMoraleEndangered;
	private float m_fMoraleEvents;
	private float m_fMoraleRelief;
	private float m_fMoraleCondition;
	private float m_fBoost;
	private int m_iBleedings;
	private float m_fSpiritedUntil_ms;
	private float m_fNextCondition_ms;

	private float m_fLastSuppressionHit_ms;
	private float m_fVeteranBonus;
	private float m_fDCOLevelMul = 1.0;
	private float m_fDCOLevelBase;
	private float m_fContactStart_ms = -1;
	private float m_fFatigue_s;
	private float m_fFatigueAt_ms = -1;
	private bool m_bFatigueStrain;
	private float m_fExert;
	private float m_fExertAt_ms = -1;
	private float m_fLastContact_ms = -1;

	private float friendlys = 1;
	private float m_fLastBad_ms = -100000;
	private float m_fStateEnter_ms;
	private float m_fBelowSince_ms = -1;
	private float m_fUnseenNext_ms;
	private float m_fDCOCalmSince_ms = -1;
	private float m_fDCOPanicRefractUntil_ms = -1;
	private bool m_bDCOCalm;
	private string m_sLastEvent = "-";
	private string m_sCondTop = "-";

	private SCR_AIUtilityComponent				m_Utility;
	private SCR_AICombatComponent				m_Combat;
	private SCR_DamageManagerComponent			m_DamageManager;

	private SCR_ChimeraAIAgent m_Agent;

	private moraleState m_State;

	protected float m_fNextUpdate_ms = 2000;
	protected float m_fUpdateInterval_ms = 2000.0;

	void DCO_AIMoraleSystem(SCR_AIUtilityComponent utility)
	{
		m_Utility = utility;
		m_Combat = utility.m_CombatComponent;

		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
		if (!agent)
			return;

		m_Agent = agent;
		m_State = moraleState.CALM;
	}

	protected static ref map<string, float> s_mFearLogged;

	static float ArmorFearScale(IEntity me)
	{
		string fk = DCO_Util.FactionKeyOf(me, false);
		float scale = -1;
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(me);
		if (cfg)
			scale = cfg.GetArmorFearScale();
		string source = "gm";
		if (scale < 0)
		{
			source = "auto";
			scale = 1.0;
			if (fk == "US" || fk == "USSR")
				scale = 0.5;
		}
		if (!s_mFearLogged)
			s_mFearLogged = new map<string, float>();
		float was;
		if (!s_mFearLogged.Find(fk, was) || was != scale)
		{
			s_mFearLogged.Set(fk, scale);
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, "-", string.Format("armor_fear_scale faction=%1 scale=%2 source=%3", fk, scale.ToString(-1, 2), source));
		}
		return scale;
	}

	void RegisterDamageManager(SCR_DamageManagerComponent dmg)
	{
		m_DamageManager = dmg;

		if (m_DamageManager)
		{
			m_DamageManager.GetOnDamageOverTimeAdded().Insert(OnDamageOverTimeAdded);
			m_DamageManager.GetOnDamageOverTimeRemoved().Insert(OnDamageOverTimeRemoved);
		}
	}

	moraleState GetState()
	{
		return m_State;
	}

	bool IsSpirited()
	{
		return GetGame().GetWorld().GetWorldTime() < m_fSpiritedUntil_ms;
	}

	bool m_bDCODirty;

	protected void OnDamageOverTimeAdded(EDamageType dType, float dps, HitZone hz)
	{
		if (dType != EDamageType.BLEEDING)
			return;
		m_bDCODirty = true;
		m_iBleedings++;
		m_fMoraleInjury = Math.Min(0.35 * m_iBleedings, 1.4);
		m_fLastBad_ms = GetGame().GetWorld().GetWorldTime();
		m_sLastEvent = "injury";
	}

	protected void OnDamageOverTimeRemoved(EDamageType dType, HitZone hz)
	{
		if (dType != EDamageType.BLEEDING)
			return;
		m_iBleedings = Math.Max(m_iBleedings - 1, 0);
		if (m_DamageManager && !m_DamageManager.IsDamagedOverTime(EDamageType.BLEEDING))
			m_iBleedings = 0;
		m_fMoraleInjury = Math.Min(0.35 * m_iBleedings, 1.4);
	}

	float GetMoraleMeasure()
	{
		return m_fMoraleTotal;
	}

	void AddStress(float v, string why)
	{
		if (m_Utility && m_Utility.m_CombatMoveState && (m_Utility.m_CombatMoveState.IsInValidCover() || m_Utility.m_CombatMoveState.m_bInCover))
			v *= DCO_Stress.T(m_Utility, "tm.morale_cover_mul", 0.85);
		m_fMoraleEvents = Math.Min(m_fMoraleEvents + v, 2.0);
		m_bDCODirty = true;
		m_fLastBad_ms = GetGame().GetWorld().GetWorldTime();
		m_sLastEvent = why;
		DCO_Perf.Count("morale:" + why);
	}

	void DCO_NoteUnseen()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fUnseenNext_ms)
			return;
		m_fUnseenNext_ms = now + 5000.0;
		AddStress(0.1, "unseen_shooter");
	}

	void AddRelief(float v, string why)
	{
		m_fMoraleRelief = Math.Min(m_fMoraleRelief + v, 1.0);
		if (m_State == moraleState.CALM || m_State == moraleState.TENSE)
			m_fSpiritedUntil_ms = GetGame().GetWorld().GetWorldTime() + 30000.0;
		DCO_Perf.Count("morale:" + why);
	}

	#ifdef WORKBENCH
	void ShowDebug()
	{
		Color color;

		switch (m_State)
		{
			case moraleState.CALM:
			{
				color = Color.FromInt(Color.GREEN);
				break;
			}
			case moraleState.SHAKEN:
			{
				color = Color.FromInt(Color.BLUE);
				break;
			}
			case moraleState.TENSE:
			{
				color = Color.FromInt(Color.YELLOW);
				break;
			}
			case moraleState.PANIC:
			{
				color = Color.FromInt(Color.ORANGE);
				break;
			}
			case moraleState.BREAK:
			{
				color = Color.FromInt(Color.RED);
				break;
			}
		}

		SCR_AIDebugVisualization.VisualizeMessage(m_Utility.m_OwnerEntity, typename.EnumToString(moraleState, m_State), EAIDebugCategory.INFO, 1.4, color);
	}
	#endif

	private void StateTransition(moraleState newState)
	{
		if (newState == m_State)
			return;
		moraleState prev = m_State;
		float now = GetGame().GetWorld().GetWorldTime();
		float hold_s = (now - m_fStateEnter_ms) / 1000.0;
		m_State = newState;
		m_fStateEnter_ms = now;
		m_fBelowSince_ms = -1;
		DCO_Perf.Count("morale:to_" + typename.EnumToString(moraleState, newState));
		if (m_Utility && m_Utility.m_OwnerEntity)
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("morale_state unit=%1 from=%2 to=%3 total=%4 top_cause=%5 hold_s=%6",
				DCO_Log.IdUnit(m_Utility.m_OwnerEntity), typename.EnumToString(moraleState, prev), typename.EnumToString(moraleState, newState), m_fMoraleTotal.ToString(-1, 2), TopCause(), Math.Round(hold_s)));
		if (m_Utility)
			DCO_Events.Morale(m_Utility.m_OwnerEntity, prev, newState);
		if (m_Combat)
			m_Combat.DCO_OnMoraleState(prev, newState);
	}

	protected string TopCause()
	{
		string top = "supp_time";
		float best = m_fMoraleSuppression;
		if (m_fMoraleEvents > best)
		{
			best = m_fMoraleEvents;
			top = m_sLastEvent;
		}
		if (m_fMoraleInjury > best)
		{
			best = m_fMoraleInjury;
			top = "injury";
		}
		if (m_fMoraleCondition > best)
		{
			best = m_fMoraleCondition;
			top = m_sCondTop;
		}
		if (m_fDCOLevelBase > best)
			top = "base";
		return top;
	}

	protected float Threshold(moraleState st, float scale)
	{
		switch (st)
		{
			case moraleState.TENSE: return 0.4 * scale;
			case moraleState.SHAKEN: return 1.5 * scale;
			case moraleState.PANIC: return DCO_Stress.T(m_Utility, "tm.morale_panic_th", 2.4) * scale;
			case moraleState.BREAK: return 3.7 * scale;
		}
		return 0;
	}

	private void UpdateState()
	{
		moraleState newState = moraleState.CALM;
		float personalityScale = Math.Clamp(DCO_PersonalityCombatUtility.GetMoraleThresholdScale(m_Utility) * SkillScale() * (1.0 + m_fVeteranBonus) * m_fDCOLevelMul, 0.7, 1.3);

		if (m_fMoraleTotal > Threshold(moraleState.BREAK, personalityScale))
			newState = moraleState.BREAK;
		else if (m_fMoraleTotal > Threshold(moraleState.PANIC, personalityScale))
			newState = moraleState.PANIC;
		else if (m_fMoraleTotal > Threshold(moraleState.SHAKEN, personalityScale))
			newState = moraleState.SHAKEN;
		else if (m_fMoraleTotal > Threshold(moraleState.TENSE, personalityScale))
			newState = moraleState.TENSE;

		float panicNow = GetGame().GetWorld().GetWorldTime();
		if (m_State == moraleState.PANIC)
		{
			float panicMax = DCO_Stress.T(m_Utility, "tm.panic_max_s", 30.0);
			if (panicMax > 0 && panicNow - m_fStateEnter_ms >= panicMax * 1000.0)
			{
				float cap = Threshold(moraleState.PANIC, personalityScale) * 0.9;
				float excess = m_fMoraleTotal - cap;
				if (excess > 0)
				{
					float fromEvents = Math.Min(excess, m_fMoraleEvents);
					m_fMoraleEvents -= fromEvents;
					excess -= fromEvents;
					if (excess > 0)
						m_fMoraleSuppression = Math.Max(m_fMoraleSuppression - excess, 0.0);
					m_fMoraleTotal = cap;
				}
				m_fDCOPanicRefractUntil_ms = panicNow + DCO_Stress.T(m_Utility, "tm.panic_refract_s", 20.0) * 1000.0;
				DCO_Perf.Count("morale:panic_release");
				if (m_Utility && m_Utility.m_OwnerEntity)
					DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("panic_release unit=%1 held_s=%2 total=%3", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), Math.Round((panicNow - m_fStateEnter_ms) / 1000.0), m_fMoraleTotal.ToString(-1, 2)));
				StateTransition(moraleState.SHAKEN);
				return;
			}
		}
		if (newState == moraleState.PANIC && panicNow < m_fDCOPanicRefractUntil_ms)
		{
			newState = moraleState.SHAKEN;
			DCO_Perf.Count("morale:panic_refract");
		}

		if (newState >= moraleState.PANIC && DCO_AssaultLock.CheckBlock(m_Utility, "morale_flee"))
			newState = moraleState.SHAKEN;
		if (m_State >= moraleState.PANIC && newState <= moraleState.SHAKEN && DCO_AssaultLock.CheckBlock(m_Utility, "morale_flee"))
		{
			StateTransition(moraleState.SHAKEN);
			return;
		}

		if (newState > m_State)
		{
			StateTransition(newState);
			return;
		}
		if (newState == m_State)
		{
			m_fBelowSince_ms = -1;
			return;
		}
		float now = GetGame().GetWorld().GetWorldTime();
		if (m_fMoraleTotal < Threshold(m_State, personalityScale) * 0.8)
		{
			if (m_fBelowSince_ms < 0)
				m_fBelowSince_ms = now;
		}
		else
			m_fBelowSince_ms = -1;
		if (m_fBelowSince_ms < 0 || now - m_fBelowSince_ms < 3000.0)
			return;
		if (now - m_fLastBad_ms < DCO_Stress.T(m_Utility, "tm.morale_down_hold_s", 40.0) * 1000.0)
		{
			DCO_Perf.Count("morale:down_hold");
			return;
		}
		if (m_State == moraleState.PANIC && now - m_fStateEnter_ms < 25000.0)
			return;
		moraleState down = moraleState.CALM;
		if (m_State == moraleState.BREAK)
			down = moraleState.PANIC;
		else if (m_State == moraleState.PANIC)
			down = moraleState.SHAKEN;
		else if (m_State == moraleState.SHAKEN)
			down = moraleState.TENSE;
		StateTransition(down);
	}

	protected float m_fDCONextState_ms;

	void DCO_SetMoraleLevel(float mul, float base)
	{
		m_fDCOLevelMul = mul;
		m_fDCOLevelBase = base;
	}

	void Update(SCR_AIUtilityComponent utility, float timeSlice)
	{
		AIAgent moraleAgent = m_Utility.GetAIAgent();
		if (moraleAgent && moraleAgent.GetParentGroup())
			friendlys = moraleAgent.GetParentGroup().GetAgentsCount();
		float now = GetGame().GetWorld().GetWorldTime();
		EAIThreatState tst = EAIThreatState.SAFE;
		if (m_Utility.m_ThreatSystem)
			tst = m_Utility.m_ThreatSystem.GetState();
		if (tst <= EAIThreatState.VIGILANT)
		{
			if (m_fDCOCalmSince_ms < 0)
				m_fDCOCalmSince_ms = now;
		}
		else
			m_fDCOCalmSince_ms = -1;
		bool calm = m_fDCOCalmSince_ms >= 0 && now - m_fDCOCalmSince_ms >= DCO_Stress.T(m_Utility, "tm.morale_rec_calm_s", 10.0) * 1000.0;
		if (calm != m_bDCOCalm)
		{
			m_bDCOCalm = calm;
			if (calm)
				DCO_Perf.Count("morale:recover_on");
			else
				DCO_Perf.Count("morale:recover_off");
		}
		float recRate = DCO_Stress.T(m_Utility, "tm.morale_rec_friend_s", 0.001) * friendlys;
		if (calm && now - m_fLastSuppressionHit_ms > 4000.0)
			recRate += DCO_Stress.T(m_Utility, "tm.morale_rec_s", 0.01);
		m_fMoraleSuppression = Math.Max(m_fMoraleSuppression - m_fMoraleSuppression * recRate * timeSlice / 1000.0, 0.0);

		int lvl = DCO_Stress.Level(m_Utility);
		float rate = 0;
		if (lvl == 1)
			rate = DCO_Stress.T(m_Utility, "tm.morale_supp_s1", 0.04);
		else if (lvl == 2)
			rate = DCO_Stress.T(m_Utility, "tm.morale_supp_s2", 0.07);
		if (tst == EAIThreatState.THREATENED)
			rate += DCO_Stress.T(m_Utility, "tm.morale_threatened_s", 0.03);
		else if (tst == EAIThreatState.ALERTED)
			rate += DCO_Stress.T(m_Utility, "tm.morale_alerted_s", 0.015);
		if (rate > 0)
		{
			ApplySuppressionGain(ComputeSuppressionGain(1, rate * timeSlice / 1000.0));
			if (lvl == 2 || tst == EAIThreatState.THREATENED)
			{
				m_fLastBad_ms = now;
				if (m_fMoraleSuppression > m_fMoraleEvents)
					m_sLastEvent = "supp_time";
			}
		}
		m_fMoraleEndangered = 0;

		float eventDecay = Math.Pow(0.5, timeSlice / (DCO_Stress.T(m_Utility, "tm.morale_event_half_s", 90.0) * 1000.0));
		m_fMoraleEvents *= eventDecay;
		m_fMoraleRelief *= eventDecay;

		if (now >= m_fNextCondition_ms)
		{
			m_fNextCondition_ms = now + 2000.0;
			RecomputeConditions();
		}

		if (m_fDCOLevelBase > 0)
			m_fDCOLevelBase = Math.Max(m_fDCOLevelBase - 0.5 * timeSlice / 300000.0, 0);
		m_fMoraleTotal = Math.Clamp(m_fMoraleSuppression + m_fMoraleInjury + m_fMoraleEvents + m_fMoraleCondition + m_fDCOLevelBase - m_fMoraleRelief - m_fBoost, 0, 5.0);
		if (now >= m_fDCONextState_ms)
		{
			m_fDCONextState_ms = now + 250.0;
			UpdateState();
		}
		if (now > m_fNextUpdate_ms)
		{
			DropAim();
			m_fNextUpdate_ms = now + m_fUpdateInterval_ms;
		}
#ifdef WORKBENCH
		ShowDebug();
#endif
	}

	protected float SkillScale()
	{
		switch (DCO_AIConfigComponent.SkillOf(m_Utility))
		{
			case DCO_AISKILL.NOOB: return 0.8;
			case DCO_AISKILL.ROOKIE: return 0.9;
			case DCO_AISKILL.VETERAN: return 1.15;
			case DCO_AISKILL.EXPERT:
			case DCO_AISKILL.SPECIAL_OPS:
			case DCO_AISKILL.TERMINATOR:
				return 1.3;
		}
		return 1.0;
	}

	protected bool FatigueContact()
	{
		if (!m_Combat || !m_Utility || !m_Utility.m_OwnerEntity)
			return false;
		BaseTarget t = m_Combat.GetCurrentTarget();
		if (!t)
			return false;
		vector tp = t.GetLastSeenPosition();
		if (tp == vector.Zero)
			return false;
		return vector.Distance(tp, m_Utility.m_OwnerEntity.GetOrigin()) <= DCO_Ctx.TuneForU(m_Utility, "fatigue.contact_m", 400.0);
	}

	protected void UpdateContactTrack(float now)
	{
		if (m_Combat && m_Combat.GetCurrentTarget())
		{
			m_fLastContact_ms = now;
			if (m_fContactStart_ms < 0)
				m_fContactStart_ms = now;
		}
		else if (m_fLastContact_ms >= 0)
		{
			float quiet = now - m_fLastContact_ms;
			if (m_fContactStart_ms >= 0 && quiet > 60000.0)
			{
				m_fContactStart_ms = -1;
				if (m_fVeteranBonus < 0.2)
				{
					m_fVeteranBonus = Math.Min(m_fVeteranBonus + 0.05, 0.2);
					DCO_Perf.Count("morale:veteran");
				}
			}
		}

		float dt = 0;
		if (m_fFatigueAt_ms >= 0)
			dt = Math.Clamp((now - m_fFatigueAt_ms) / 1000.0, 0, 30.0);
		m_fFatigueAt_ms = now;
		string src = "none";
		bool contactAcc = DCO_Ctx.TuneForU(m_Utility, "fatigue.contact_acc", 0) > 0;
		if (now - m_fLastSuppressionHit_ms < 30000.0)
		{
			m_fFatigue_s = Math.Min(m_fFatigue_s + dt, 1200.0);
			src = "fire";
			if (contactAcc && dt > 0)
				DCO_Perf.Count("fatigue:acc_fire");
		}
		else if (contactAcc && FatigueContact())
		{
			m_fFatigue_s = Math.Min(m_fFatigue_s + 0.5 * dt, 1200.0);
			src = "contact";
			if (dt > 0)
				DCO_Perf.Count("fatigue:acc_contact");
		}
		else if (contactAcc)
			m_fFatigue_s = Math.Max(m_fFatigue_s - dt / 3.0, 0);
		else
			m_fFatigue_s = Math.Max(m_fFatigue_s - dt / 1.5, 0);

		bool strain = m_fFatigue_s >= 300.0;
		if (strain != m_bFatigueStrain)
		{
			m_bFatigueStrain = strain;
			if (strain)
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("fatigue_strain_on unit=%1 fat_s=%2 src=%3", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), Math.Round(m_fFatigue_s), src));
			else
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("fatigue_strain_off unit=%1", DCO_Log.IdUnit(m_Utility.m_OwnerEntity)));
		}
		if (m_fFatigue_s > 300.0)
		{
			m_fMoraleCondition += Math.Clamp((m_fFatigue_s - 300.0) / 60.0 * 0.1, 0, 0.5);
			DCO_Perf.Count("morale:fatigue");
		}
	}

	static float Stamina(IEntity ent)
	{
		if (!ent)
			return -1;
		CharacterStaminaComponent sc = CharacterStaminaComponent.Cast(ent.FindComponent(CharacterStaminaComponent));
		if (!sc)
			return -1;
		return sc.GetStamina();
	}

	float GetFatigueSeconds()
	{
		return m_fFatigue_s;
	}

	float GetExertion()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (m_fExertAt_ms >= 0 && m_fExert > 0)
		{
			float s = Math.Clamp((m_fFatigue_s - 300.0) / 300.0, 0, 1);
			m_fExert = Math.Max(m_fExert - (now - m_fExertAt_ms) / 20000.0 * (1.0 - 0.3 * s), 0);
		}
		m_fExertAt_ms = now;
		return m_fExert;
	}

	void AddExertion()
	{
		m_fExert = Math.Min(GetExertion() + 1.0, 12.0);
	}

	protected void RecomputeConditions()
	{
		m_fBoost = 0;
		m_fMoraleCondition = 0;
		m_sCondTop = "fatigue";
		UpdateContactTrack(GetGame().GetWorld().GetWorldTime());
		IEntity me = m_Utility.m_OwnerEntity;
		AIAgent myAgent = m_Utility.GetAIAgent();
		if (!me || !myAgent)
			return;
		SCR_AIGroup grp = SCR_AIGroup.Cast(myAgent.GetParentGroup());
		if (!grp)
			return;

		vector mp = me.GetOrigin();
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		int near30;
		int alive;
		int contagion;
		bool armorNear;
		bool activeNear;
		foreach (AIAgent a : agents)
		{
			IEntity e = a.GetControlledEntity();
			if (!e || !DCO_Util.IsAlive(e))
				continue;
			alive++;
			if (e == me)
				continue;
			float d = vector.Distance(e.GetOrigin(), mp);
			if (d <= 30.0)
			{
				near30++;
				SCR_ChimeraAIAgent na = SCR_ChimeraAIAgent.Cast(a);
				if (!activeNear && na && na.m_UtilityComponent && na.m_UtilityComponent.m_CombatComponent && na.m_UtilityComponent.m_CombatComponent.GetCurrentTarget())
					activeNear = true;
			}
			if (d <= 20.0)
			{
				SCR_ChimeraAIAgent fa = SCR_ChimeraAIAgent.Cast(a);
				if (fa && fa.m_UtilityComponent && fa.m_UtilityComponent.GetMoraleSystem())
				{
					moraleState fs = fa.m_UtilityComponent.GetMoraleSystem().GetState();
					if (fs == moraleState.PANIC || fs == moraleState.BREAK)
						contagion++;
				}
			}
		}
		IEntity leader = grp.GetLeaderEntity();
		m_fBoost = Math.Min(near30, 6) * 0.1;
		if (leader && leader != me && DCO_Util.IsAlive(leader) && vector.Distance(leader.GetOrigin(), mp) < 100.0)
		{
			m_fBoost += 0.15;
			SCR_ChimeraAIAgent la = SCR_ChimeraAIAgent.Cast(grp.GetLeaderAgent());
			if (la && la.m_UtilityComponent && la.m_UtilityComponent.GetMoraleSystem() && la.m_UtilityComponent.GetMoraleSystem().GetState() == moraleState.CALM
				&& vector.Distance(leader.GetOrigin(), mp) < 30.0)
				m_fMoraleCondition -= 0.15;
		}
		if (contagion > 0)
		{
			m_fMoraleCondition += 0.1 * contagion;
			m_sCondTop = "contagion";
			DCO_Perf.Count("morale:contagion");
		}

		bool inContact = m_Combat && m_Combat.GetCurrentTarget();
		if (inContact && !activeNear)
		{
			m_fMoraleCondition += 0.3;
			m_sCondTop = "isolated";
			DCO_Perf.Count("morale:isolated");
		}

		SCR_AIGroupUtilityComponent gu = grp.GetGroupUtilityComponent();
		PerceptionManager pm = GetGame().GetPerceptionManager();
		if (gu && pm && gu.GetPercGroupComp())
		{
			float pmNow = pm.GetTime();
			foreach (SCR_AITargetInfo t : gu.GetPercGroupComp().m_aTargets)
			{
				if (t && pmNow - t.m_fTimestamp <= 10.0 && t.m_Entity && (t.m_eCategory == EAITargetInfoCategory.DETECTED || t.m_eCategory == EAITargetInfoCategory.IDENTIFIED)
					&& vector.Distance(t.m_vWorldPos, mp) < 300.0 && DCO_VehicleCombat.IsArmored(t.m_Entity))
				{
					armorNear = true;
					break;
				}
			}
		}

		DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));
		if (armorNear)
		{
			float fearScale = ArmorFearScale(me);
			if (g && g.HasAT())
				m_fMoraleCondition += 0.1 * fearScale;
			else
				m_fMoraleCondition += 0.4 * fearScale;
			m_sCondTop = "armor_fear";
			DCO_Perf.Count("morale:armor_fear");
			if (fearScale != 1.0)
				DCO_Perf.Count("morale:armor_fear_scaled");
		}
		AICommander_BaseComponent cmd;
		if (g)
			cmd = g.GetMyCommander();
		if (cmd)
		{
			foreach (DCO_GroupUtilityComponent o : cmd.GetOwnedGroups())
			{
				if (!o || o == g || !o.GetOwner() || o.GetUnitCount() <= 0)
					continue;
				if (vector.Distance(o.GetOwner().GetOrigin(), mp) < 100.0)
				{
					m_fMoraleCondition -= 0.3;
					DCO_Perf.Count("morale:friendly_support");
					break;
				}
			}
		}
	}

	void DCO_GetComponents(out float supp, out float endangered, out float events, out float relief, out float condition, out float boost)
	{
		supp = m_fMoraleSuppression;
		endangered = m_fMoraleEndangered;
		events = m_fMoraleEvents;
		relief = m_fMoraleRelief;
		condition = m_fMoraleCondition;
		boost = m_fBoost;
	}

	void DropAim()
	{
		float drop = Math.Map(m_fMoraleTotal, 0, 4.5, 0, 10);
		m_Combat.MoraleDropAIM(drop);
	}

	private float ComputeSuppressionGain(int count, float baseIncrement)
	{
		float fCount = count;
		float gain = fCount * baseIncrement;

		if (!m_Utility)
			return gain;

		gain *= Math.Max(DCO_AIConfigComponent.SuppressionOf(m_Utility), 0.0);

		if (m_Utility.m_CombatMoveState && m_Utility.m_CombatMoveState.m_bInCover)
			gain *= DCO_Stress.T(m_Utility, "tm.morale_cover_mul", 0.85);

		gain *= GetPersonalitySuppressionGainScale();

		return gain;
	}

	private void ApplySuppressionGain(float gain)
	{
		m_fLastSuppressionHit_ms = GetGame().GetWorld().GetWorldTime();

		if (gain <= 0.0)
			return;

		float headroom = 1.0 - (m_fMoraleSuppression / 3.0);
		headroom = Math.Clamp(headroom, 0.0, 1.0);

		m_fMoraleSuppression = Math.Clamp(m_fMoraleSuppression + gain * headroom, 0.0, 3.0);
	}

	private float GetPersonalitySuppressionGainScale()
	{
		switch (DCO_AIConfigComponent.PersonalityOf(m_Utility))
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 1.3;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 0.8;
			case DCO_EAIPersonality.RECKLESS:
				return 0.6;
		}
		return 1.0;
	}
}

class DCO_MoraleEvents
{
	static void OnDeath(IEntity dead)
	{
		if (!dead || !Replication.IsServer())
			return;
		FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(dead.FindComponent(FactionAffiliationComponent));
		if (!fac || !fac.GetAffiliatedFaction())
			return;
		Faction df = fac.GetAffiliatedFaction();
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return;
		vector dp = dead.GetOrigin();
		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent || !ca.m_UtilityComponent.GetMoraleSystem())
				continue;
			IEntity e = ca.GetControlledEntity();
			if (!e || e == dead || !DCO_Util.IsAlive(e))
				continue;
			float d = vector.DistanceXZ(e.GetOrigin(), dp);
			if (d > 60.0)
				continue;
			FactionAffiliationComponent ef = FactionAffiliationComponent.Cast(e.FindComponent(FactionAffiliationComponent));
			if (!ef || !ef.GetAffiliatedFaction())
				continue;
			Faction f = ef.GetAffiliatedFaction();
			DCO_AIMoraleSystem ms = ca.m_UtilityComponent.GetMoraleSystem();
			if (f == df)
			{
				if (d < 30.0)
					ms.AddStress(0.4, "friend_killed");
				else
					ms.AddStress(0.2, "friend_killed");
			}
			else if (f.IsFactionEnemy(df))
				ms.AddRelief(0.2, "enemy_killed");
		}
	}

	static void OnEnemyRetreat(IEntity leader, Faction retreating)
	{
		if (!leader || !retreating || !Replication.IsServer())
			return;
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return;
		vector lp = leader.GetOrigin();
		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraAIAgent ca = SCR_ChimeraAIAgent.Cast(a);
			if (!ca || !ca.m_UtilityComponent || !ca.m_UtilityComponent.GetMoraleSystem())
				continue;
			IEntity e = ca.GetControlledEntity();
			if (!e)
				continue;
			float rd = vector.DistanceXZ(e.GetOrigin(), lp);
			if (rd > 300.0)
				continue;
			if (rd > 150.0 && (!ca.m_UtilityComponent.m_PerceptionComponent || !ca.m_UtilityComponent.m_PerceptionComponent.FindTargetPerceptionObject(leader)))
				continue;
			FactionAffiliationComponent ef = FactionAffiliationComponent.Cast(e.FindComponent(FactionAffiliationComponent));
			if (ef && ef.GetAffiliatedFaction() && ef.GetAffiliatedFaction().IsFactionEnemy(retreating))
				ca.m_UtilityComponent.GetMoraleSystem().AddRelief(0.3, "enemy_retreat");
		}
	}
}
