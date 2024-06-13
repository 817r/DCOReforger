enum moraleState
{
	BREAK,
	NORMAL,
	WISE,
	MOTIVATED,
	MANIAC
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
	
	private static const float MORALE_SHOT_RECOVERY 				= 			0.005 * 0.001;	//!< Falloff (percentual drop per milisecond)
	private static const float MORALE_SUPPRESSION_RECOVERY 			= 			0.012 * 0.001;
	private static const float MORALE_ENDANGERED_RECOVERY 			= 			0.001 * 0.001;
	private static const float LOW_SUPPLY_RECOVERY					=			0.02 * 0.001;
	private static const float MORALE_RECOVERY_THREAT_STATE			=			0.003 * 0.001;
	
	private static const float MORALE_BOOST_LEADER_DISTANCE			=			50;
	private static const float MORALE_BOOST_FRIENDLY_DISTANCE		=			10;
	
	private static const float MORALE_BOOST_LEADER_VALUE			=			0.11;
	private static const float MORALE_BOOST_FRIENDLY_VALUE			=			0.035;
	
	private static const float MORALE_DROP_SUPPRESSION				=			0.12;
	private static const float MORALE_DROP_FIREFIGHT_FIXED			=			0.2;
		
	
	private static const float MORALE_DROP_BLEEDING_FIXED_INCREMENT	=			0.35;
	
	private static const float MOTIVATED_THRESHOLD 					= 			0.4;
	private static const float WISE_THRESHOLD						=			1.5;
	private static const float MANIAC_THRESHOLD						=			1.8;
	private static const float BREAK_THRESHOLD						=			3.5;
	
	private static const float ENDANGERED_INCREMENT 				= 			0.03;
	private static const float SUPPRESSION_BULLET_INCREMENT			=			0.002;
	private static const float LOW_SUPPLY							=			0.05;
	
	private float m_fMoraleTotal;
	private float m_fMoraleSuppression;
	private float m_fMoraleInjury;
	private float m_fMoraleEndangered;
	private float m_fMoraleSupply;
	private float m_fMoraleThreat;
	private float m_fMoraleThreatMod;
	

	private SCR_AIUtilityComponent				m_Utility;
	private SCR_AIConfigComponent				m_Config;
	private SCR_AICombatComponent				m_Combat;
	private SCR_DamageManagerComponent			m_DamageManager;
	private SCR_AIThreatSystem					m_Threat;
	
	private SCR_ChimeraAIAgent m_Agent;
	
	private EAIThreatState m_ThreatState;
	private moraleState m_State;
		
	private ref SCR_AIMoraleStateChangedInvoker m_OnThreatStateChanged = new SCR_AIMoraleStateChangedInvoker();
	
	void DCO_AIMoraleSystem(SCR_AIUtilityComponent utility)
	{
		m_Utility = utility;
		m_Config = utility.m_ConfigComponent;
		m_Combat = utility.m_CombatComponent;
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
			case moraleState.WISE:
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
		SCR_AIDebugVisualization.VisualizeMessage(m_Utility.m_OwnerEntity, m_fMoraleTotal.ToString(), EAIDebugCategory.INFO, 1.4, color);	
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
		else if (m_fMoraleTotal > WISE_THRESHOLD)
			newState = moraleState.WISE;
		else if (m_fMoraleTotal > MOTIVATED_THRESHOLD)
			newState = moraleState.MOTIVATED;
		
		StateTransition(newState);
	}
	
	void Update(SCR_AIUtilityComponent utility, float timeSlice)
	{
		m_fMoraleSuppression -= m_fMoraleSuppression * MORALE_SUPPRESSION_RECOVERY * timeSlice;
		
		if (m_Combat)
		{
			m_fMoraleThreat = Math.Clamp(m_fMoraleThreatMod * 0.0001 * timeSlice, 0, 3.2);
			if (m_Combat.GetCurrentTarget())
				m_fMoraleEndangered = ENDANGERED_INCREMENT;
			else
				m_fMoraleEndangered -= m_fMoraleEndangered * MORALE_ENDANGERED_RECOVERY * timeSlice;
			
			if (m_Combat.lowAmmo())
				m_fMoraleSupply = m_fMoraleSupply + LOW_SUPPLY;
			else
				m_fMoraleSupply -= m_fMoraleSupply * LOW_SUPPLY_RECOVERY * timeSlice;
		}	
		else
		{
			m_fMoraleThreat -= 0.001 * 0.005 * timeSlice;
		}
		
		m_fMoraleTotal = Math.Clamp(m_fMoraleSuppression + m_fMoraleInjury + m_fMoraleEndangered + m_fMoraleSupply + m_fMoraleThreat, 0, 4.0);
		
		UpdateState();
#ifdef WORKBENCH
		ShowDebug();
#endif
	}
	
	void ThreatProjectileFlyby(int count)
	{
		m_fMoraleSuppression = Math.Clamp(m_fMoraleSuppression + (count/2) * SUPPRESSION_BULLET_INCREMENT, 0, 2.8);
	}
	
	void ThreatBulletImpact(float distance, int count)
	{		
		m_fMoraleSuppression = Math.Clamp(m_fMoraleSuppression + (count/2) * SUPPRESSION_BULLET_INCREMENT, 0, 2.8);
	}
	
	void threatmodifierToMorale(float modifier)
	{
		m_fMoraleThreatMod = Math.Clamp(m_fMoraleThreat + modifier, 0, 2.0)
	}
}