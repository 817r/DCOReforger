enum moraleState
{
	NORMAL,
	MOTIVATED,
	ANXIOUS,
	MANIAC,
	BREAK
}

typedef func SCR_AIMoraleStateChangedCallback;
void SCR_AIMoraleStateChangedCallback(moraleState prevState, moraleState newState);
typedef ScriptInvokerBase<SCR_AIMoraleStateChangedCallback> SCR_AIMoraleStateChangedInvoker;

class DCO_AIMoraleSystem
{
	// it should be like this DROP is the number going up 
	// RECOVERY is the number going down
	// Courage is the resistance to Morale 
	// Round classification (EWeaponType) classification
	
	private static const float MORALE_SHOT_RECOVERY 				= 			0.00008 * 0.001;	//!< Falloff (percentual drop per milisecond)
	private static const float MORALE_SUPPRESSION_RECOVERY 			= 			0.0005 * 0.001;
	private static const float MORALE_ENDANGERED_RECOVERY 			= 			0.0002 * 0.001;
	private static const float LOW_SUPPLY_RECOVERY					=			0.01 * 0.001;
	private static const float MORALE_RECOVERY_THREAT_STATE			=			0.003 * 0.001;

	private static const float MORALE_BOOST_FRIENDLY_VALUE			=			0.002 * 0.001;
	private static const float MORALE_BOOST_FIXED_FRIENDLY_VALUE	=			0.08;
	
	private static const float MORALE_DROP_BLEEDING_FIXED_INCREMENT	=			0.35;
	
	private static const float MOTIVATED_THRESHOLD 					= 			0.4;
	private static const float ANXIOUS_THRESHOLD					=			1.5;
	private static const float MANIAC_THRESHOLD						=			2.0;
	private static const float BREAK_THRESHOLD						=			3.7;
	
	private static const float ENDANGERED_INCREMENT 				= 			0.02 * 0.001;
	private static const float SUPPRESSION_BULLET_INCREMENT			=			0.005;
	private static const float LOW_SUPPLY							=			0.05;
	
	private float m_fMoraleTotal;
	private float m_fMoraleSuppression;
	private float m_fMoraleSuppressionPlus;
	private float m_fMoraleInjury;
	private float m_fMoraleEndangered;
	private float m_fMoraleEndangeredPlus;
	private float m_fMoraleSupply;
	private float m_fMoraleThreat;
	private float m_fMoraleThreatMod;
	
	private float friendlys;
	
	private SCR_AIUtilityComponent				m_Utility;
	private SCR_AICombatComponent				m_Combat;
	private SCR_DamageManagerComponent			m_DamageManager;
	private SCR_AIThreatSystem					m_Threat;
	
	private SCR_ChimeraAIAgent m_Agent;
	
	private EAIThreatState m_ThreatState;
	private moraleState m_State;
	
	protected float m_fNextUpdate_ms = 2000;
	protected float m_fUpdateInterval_ms;
	
	private ref SCR_AIMoraleStateChangedInvoker m_OnThreatStateChanged = new SCR_AIMoraleStateChangedInvoker();
	
	void DCO_AIMoraleSystem(SCR_AIUtilityComponent utility)
	{
		m_Utility = utility;
		m_Combat = utility.m_CombatComponent;
		m_Threat = utility.m_ThreatSystem;
		m_DamageManager = SCR_DamageManagerComponent.Cast(utility.m_OwnerEntity.FindComponent(SCR_DamageManagerComponent));
		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
		if (!agent)
			return;
		m_Agent = agent;
		
		if (m_DamageManager)
		{
			m_DamageManager.GetOnDamageOverTimeAdded().Insert(OnDamageOverTimeAdded);
			m_DamageManager.GetOnDamageOverTimeRemoved().Insert(OnDamageOverTimeRemoved);
		}
		
		m_State = moraleState.NORMAL;
	}
	
	//------------------------------------------------------------------------------------------------
	//! \return
	SCR_AIMoraleStateChangedInvoker GetOnThreatStateChanged()
	{
		return m_OnThreatStateChanged;
	}
	
	//------------------------------------------------------------------------------------------------
	//! \return
	moraleState GetState()
	{
		return m_State;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnDamageOverTimeAdded(EDamageType dType, float dps, HitZone hz)
	{
		if (dType != EDamageType.BLEEDING)
			return;
		
		if (m_DamageManager.IsDamagedOverTime(EDamageType.BLEEDING))
			m_fMoraleInjury += MORALE_DROP_BLEEDING_FIXED_INCREMENT;
	}
	
	//------------------------------------------------------------------------------------------------
	protected void OnDamageOverTimeRemoved(EDamageType dType, HitZone hz)
	{
		if (dType != EDamageType.BLEEDING)
			return;
		
		if (!m_DamageManager.IsDamagedOverTime(EDamageType.BLEEDING))
			m_fMoraleInjury = m_fMoraleInjury - MORALE_DROP_BLEEDING_FIXED_INCREMENT;
	}
	
	//------------------------------------------------------------------------------------------------
	//! \return Sum of all threats without the effect of injuries - used for deciding to patch oneself
	float GetMoraleMeasureWithoutInjuryFactor()
	{
		return m_fMoraleTotal - m_fMoraleInjury;
	}
	
	//------------------------------------------------------------------------------------------------
	//! \return
	float GetMoraleMeasure()
	{
		return m_fMoraleTotal;
	}
	
	#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	//!
	void ShowDebug()
	{
		// Show message above AI's head

		Color color;
		
		switch (m_State)
		{
			case moraleState.NORMAL:
			{
				color = Color.FromInt(Color.GREEN);
				break;
			}
			case moraleState.ANXIOUS:
			{
				color = Color.FromInt(Color.BLUE);
				break;
			}
			case moraleState.MOTIVATED:
			{
				color = Color.FromInt(Color.YELLOW);
				break;
			}
			case moraleState.MANIAC:
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
		
		//SCR_AIDebugVisualization.VisualizeMessage(m_Utility.m_OwnerEntity, typename.EnumToString(moraleState, m_State), EAIDebugCategory.INFO, 1.4, color);	
	}
	#endif // WORKBENCH
	
	private void StateTransition(moraleState newState)
	{
		if (newState == m_State)
			return;
		
		m_OnThreatStateChanged.Invoke(m_State, newState);
		
		m_State = newState;
	}
	
	//------------------------------------------------------------------------------------------------
	private void UpdateState()
	{
		moraleState newState = moraleState.NORMAL;
		
		if (m_fMoraleTotal > BREAK_THRESHOLD)
			newState = moraleState.BREAK;		
		else if (m_fMoraleTotal > MANIAC_THRESHOLD)
			newState = moraleState.MANIAC;		
		else if (m_fMoraleTotal > ANXIOUS_THRESHOLD)
			newState = moraleState.ANXIOUS;
		else if (m_fMoraleTotal > MOTIVATED_THRESHOLD)
			newState = moraleState.MOTIVATED;
		
		StateTransition(newState);
	}
	
	void Update(SCR_AIUtilityComponent utility, float timeSlice)
	{
		friendlys = m_Utility.GetAIAgent().GetParentGroup().GetAgentsCount();
		m_fMoraleSuppression -= m_fMoraleSuppression * (MORALE_SUPPRESSION_RECOVERY + (friendlys * MORALE_BOOST_FRIENDLY_VALUE)) * timeSlice;
		if (m_Combat)
		{
			if (m_Combat.GetCurrentTarget())
			{
				m_fMoraleEndangeredPlus += ENDANGERED_INCREMENT * timeSlice;
				m_fMoraleEndangered = Math.Clamp(m_fMoraleEndangered + m_fMoraleEndangeredPlus, 0 , 1.2)
			}
			else
				m_fMoraleEndangered -= m_fMoraleEndangered * MORALE_ENDANGERED_RECOVERY * timeSlice;
		}
		else
		{
			m_fMoraleThreatMod = Math.Clamp(m_fMoraleThreatMod - 0.001 * 0.0001 * timeSlice, 0, 1.0);
		}
		
		m_fMoraleTotal = Math.Clamp((m_fMoraleSuppression + m_fMoraleInjury + m_fMoraleEndangered + m_fMoraleSupply + m_fMoraleThreatMod) - friendlyMoraleBoost(), 0, 4.5);
		UpdateState();
		float currentTime_ms = GetGame().GetWorld().GetWorldTime();
		if (currentTime_ms > m_fNextUpdate_ms)
		{
			//DropAim();
		}
		m_fNextUpdate_ms = currentTime_ms + m_fUpdateInterval_ms;
#ifdef WORKBENCH
		ShowDebug();
#endif
	}
	
	void ThreatProjectileFlyby(int count)
	{
		m_fMoraleSuppressionPlus = Math.Clamp(m_fMoraleSuppressionPlus + count * SUPPRESSION_BULLET_INCREMENT, 0, 3.0);
		m_fMoraleSuppression += Math.Clamp(m_fMoraleSuppression + m_fMoraleSuppressionPlus, 0, 1.5);
	}
	
	void DropAim()
	{
		float drop = Math.Map(m_fMoraleTotal, 0, 4.5, 0, 100);
		m_Combat.MoraleDropAIM(drop);
	}
	
	void ThreatBulletImpact(int count)
	{			
		m_fMoraleSuppressionPlus = Math.Clamp(m_fMoraleSuppressionPlus + count * SUPPRESSION_BULLET_INCREMENT, 0, 3.2);
		m_fMoraleSuppression += Math.Clamp(m_fMoraleSuppression + m_fMoraleSuppressionPlus, 0, 4.5);
	}
	
	void threatmodifierToMorale(float modifier)
	{	
		m_fMoraleThreatMod = Math.Clamp(m_fMoraleThreatMod + modifier, 0, 1.6);
	}
	
	float friendlyMoraleBoost()
	{
		return friendlys * MORALE_BOOST_FIXED_FRIENDLY_VALUE;
	}
}