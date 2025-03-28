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
	
	private static const float MORALE_SHOT_RECOVERY 				= 			0.008 * 0.001;	//!< Falloff (percentual drop per milisecond)
	private static const float MORALE_SUPPRESSION_RECOVERY 			= 			0.04 * 0.001;
	private static const float MORALE_ENDANGERED_RECOVERY 			= 			0.02 * 0.001;
	private static const float LOW_SUPPLY_RECOVERY					=			0.02 * 0.001;
	private static const float MORALE_RECOVERY_THREAT_STATE			=			0.003 * 0.001;

	private static const float MORALE_BOOST_FRIENDLY_VALUE			=			0.05 * 0.001;
	private static const float MORALE_BOOST_FIXED_FRIENDLY_VALUE	=			0.08;
	
	private static const float MORALE_DROP_SUPPRESSION				=			0.12;
	private static const float MORALE_DROP_FIREFIGHT_FIXED			=			0.2;
	
	private static const float MORALE_RESISTANCE_BASE				=			0.2;
		
	private static const float MORALE_DROP_BLEEDING_FIXED_INCREMENT	=			0.35;
	
	private static const float MOTIVATED_THRESHOLD 					= 			0.4;
	private static const float ANXIOUS_THRESHOLD					=			1.5;
	private static const float MANIAC_THRESHOLD						=			1.8;
	private static const float BREAK_THRESHOLD						=			3.7;
	
	private static const float ENDANGERED_INCREMENT 				= 			0.0003 * 0.001;
	private static const float SUPPRESSION_BULLET_INCREMENT			=			0.1;
	private static const float LOW_SUPPLY							=			0.05;
	
	private static const float aimImprovementPerSecond				=			0.00003 * 0.001;
	private static const float aimRecoveryPerSecond					=			0.005 * 0.001;
	private static const float aimRecoveryPerSecondTargetChange		=			0.0002 * 0.001;
	
	private float aimImprovementTotal;
	private float aimImprovement;
	private float improvementAims;
	private float aimDecrase;
	
	private float m_fMoraleTotal;
	private float m_fMoraleSuppression;
	private float m_fMoraleSuppressionPlus;
	private float m_fMoraleInjury;
	private float m_fMoraleEndangered;
	private float m_fMoraleEndangeredPlus;
	private float m_fMoraleSupply;
	private float m_fMoraleThreat;
	private float m_fMoraleThreatMod;
	
	private float m_fMoraleResistance;
	
	private float friendlys;
	
	private SCR_AIUtilityComponent				m_Utility;
	private SCR_AICombatComponent				m_Combat;
	private SCR_DamageManagerComponent			m_DamageManager;
	private SCR_AIThreatSystem					m_Threat;
	private DCO_SkillComponent					m_Skill;
	private DCO_UnitScanComponent				m_Scanner;
	
	private SCR_ChimeraAIAgent m_Agent;
	
	private EAIThreatState m_ThreatState;
	private moraleState m_State;
	private DCO_CUSTOMRANK rank;
	private DCO_GroupTactic tacs;
	protected DCO_IndividualRoles MyRole;
	private EAISkill Skills;
	
	private ref SCR_AIMoraleStateChangedInvoker m_OnThreatStateChanged = new SCR_AIMoraleStateChangedInvoker();
	
	void DCO_AIMoraleSystem(SCR_AIUtilityComponent utility)
	{
		m_Utility = utility;
		m_Combat = utility.m_CombatComponent;
		m_Skill = utility.m_DCO_Skill;
		m_Threat = utility.m_ThreatSystem;
		tacs = utility.getTactics();
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
			
		rank = m_Skill.GetCharacterRank(utility.GetOwner());
		m_Scanner = DCO_UnitScanComponent.Cast(utility.m_OwnerEntity.FindComponent(DCO_UnitScanComponent));
		m_State = moraleState.NORMAL;
		Skills = m_Combat.GetAISkill();
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
		SCR_AIDebugVisualization.VisualizeMessage(m_Utility.m_OwnerEntity," Morale Total : " + m_fMoraleTotal.ToString() + " | Hostile Detected : " + m_Utility.hh.ToString() + " | Friendly Detected : " + m_Utility.gg.ToString(), EAIDebugCategory.INFO, 1.4, Color.White);	
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
		//friendlys = m_Utility.m_Awareness.getNumberFriendlyRecognized();
		
		m_fMoraleSuppression -= m_fMoraleSuppression * (MORALE_SUPPRESSION_RECOVERY + m_fMoraleResistance) * timeSlice;
		aimDecrase -= aimDecrase * aimRecoveryPerSecond * timeSlice;
		
		switch (Skills)
		{
			case EAISkill.RECRUIT :
			{
				m_fMoraleResistance = MORALE_RESISTANCE_BASE;
				break;
			}
			case EAISkill.ROOKIE :
			{
				m_fMoraleResistance = MORALE_RESISTANCE_BASE * 2;
				break;
			}
			case EAISkill.TRAINED :
			{
				m_fMoraleResistance = MORALE_RESISTANCE_BASE * 4;
				break;
			}
			case EAISkill.REGULAR :
			{
				m_fMoraleResistance = MORALE_RESISTANCE_BASE * 6;
				break;
			}
			case EAISkill.VETERAN :
			{
				m_fMoraleResistance = MORALE_RESISTANCE_BASE * 8;
				break;
			}
			case EAISkill.EXPERT :
			{
				m_fMoraleResistance = MORALE_RESISTANCE_BASE * 10;
				break;
			}
			case EAISkill.CYLON :
			{
				m_fMoraleResistance = MORALE_RESISTANCE_BASE * 12;
				break;
			}
		}

		if (m_Combat)
		{
			if (m_Combat.GetCurrentTarget())
			{
				switch (rank)
				{
					case DCO_CUSTOMRANK.RECRUIT:
					{
						m_fMoraleEndangeredPlus += ENDANGERED_INCREMENT * 2 * timeSlice;
						break;
					}
					case DCO_CUSTOMRANK.PRIVATE:
					{
						m_fMoraleEndangeredPlus += ENDANGERED_INCREMENT * timeSlice;
						break;
					}
					case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
					{
						m_fMoraleEndangeredPlus += ENDANGERED_INCREMENT * 0.7 * timeSlice;
						break;
					}
					case DCO_CUSTOMRANK.SPECIALIST:
					{
						m_fMoraleEndangeredPlus += ENDANGERED_INCREMENT * 0.5 * timeSlice;
						break;
					}
				}
				
				m_fMoraleEndangered = Math.Clamp(m_fMoraleEndangeredPlus, 0 , 1.2)
			}
			else
				m_fMoraleEndangered -= m_fMoraleEndangered * MORALE_ENDANGERED_RECOVERY * timeSlice;
			
			if (m_Combat.lowAmmo())
				m_fMoraleSupply = m_fMoraleSupply + LOW_SUPPLY;
			else
				m_fMoraleSupply -= m_fMoraleSupply * LOW_SUPPLY_RECOVERY * timeSlice;

			if (m_Combat.GetCurrentTarget())
			{
				switch(rank)
				{
					case DCO_CUSTOMRANK.RECRUIT:
					{
						improvementAims = Math.Clamp((improvementAims + aimImprovementPerSecond / 2) * timeSlice, 0, 20);
						break;
					}
					case DCO_CUSTOMRANK.PRIVATE:
					{
						improvementAims = Math.Clamp((improvementAims + aimImprovementPerSecond) * timeSlice, 0, 20);
						break;
					}
					case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
					{
						improvementAims = Math.Clamp((improvementAims + aimImprovementPerSecond * 2) * timeSlice, 0, 20);
						break;
					}
					case DCO_CUSTOMRANK.SPECIALIST:
					{
						improvementAims = Math.Clamp((improvementAims + aimImprovementPerSecond * 5) * timeSlice, 0, 20);
						break;
					}
				}
			}

			if (m_Combat.selectedTargetChanged)
			{
				//aimImprovement -= aimRecoveryPerSecond / 50;
				improvementAims = Math.Clamp((improvementAims - aimRecoveryPerSecondTargetChange) * timeSlice, 0, 20);
			}
		}
		else
		{
			m_fMoraleThreatMod = Math.Clamp(m_fMoraleThreatMod - 0.001 * 0.0001 * timeSlice, 0, 1.0);
			//aimImprovement -= aimRecoveryPerSecond / 20;
			improvementAims = Math.Clamp((improvementAims - aimRecoveryPerSecond) * timeSlice, 0, 20);
		}
		
		tacs = utility.getTactics();
		rank = m_Skill.GetCharacterRank(utility.m_OwnerEntity);
		m_fMoraleTotal = Math.Clamp(m_fMoraleSuppression + m_fMoraleInjury + m_fMoraleEndangered + m_fMoraleSupply + m_fMoraleThreatMod, 0, 4.5);
		aimImprovementTotal = Math.Clamp(improvementAims, 0, 20 - aimDecrase);
		m_Combat.improvement(aimImprovementTotal);
		UpdateState();
#ifdef WORKBENCH
		ShowDebug();
#endif
	}
	
	void ThreatProjectileFlyby(int count)
	{
		switch(rank)
		{
			case DCO_CUSTOMRANK.RECRUIT:
			{
				m_fMoraleSuppressionPlus = Math.Clamp(m_fMoraleSuppressionPlus + (count * 1.2) * SUPPRESSION_BULLET_INCREMENT, 0, 4.0);
				break;
			}
			case DCO_CUSTOMRANK.PRIVATE:
			{
				m_fMoraleSuppressionPlus = Math.Clamp(m_fMoraleSuppressionPlus + count * SUPPRESSION_BULLET_INCREMENT, 0, 4.0);
				break;
			}
			case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
			{
				m_fMoraleSuppressionPlus = Math.Clamp(m_fMoraleSuppressionPlus + (count/5) * SUPPRESSION_BULLET_INCREMENT, 0, 4.0);
				break;
			}
			case DCO_CUSTOMRANK.SPECIALIST:
			{
				m_fMoraleSuppressionPlus = Math.Clamp(m_fMoraleSuppressionPlus + (count/10) * SUPPRESSION_BULLET_INCREMENT, 0, 4.0);
				break;
			}
		}
		m_fMoraleSuppression = Math.Clamp(m_fMoraleSuppression + m_fMoraleSuppressionPlus, 0, 3.0);
		aimDecrase = Math.Clamp(aimDecrase + count * SUPPRESSION_BULLET_INCREMENT/10, 0, 2.0);
	}
	
	void ThreatBulletImpact(float distance, int count)
	{			
		switch(rank)
		{
			case DCO_CUSTOMRANK.RECRUIT:
			{
				m_fMoraleSuppressionPlus = Math.Clamp(m_fMoraleSuppressionPlus + (count * 1.2) * SUPPRESSION_BULLET_INCREMENT, 0, 4.0);
				break;
			}
			case DCO_CUSTOMRANK.PRIVATE:
			{
				m_fMoraleSuppressionPlus = Math.Clamp(m_fMoraleSuppressionPlus + count * SUPPRESSION_BULLET_INCREMENT, 0, 4.0);
				break;
			}
			case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
			{
				m_fMoraleSuppressionPlus = Math.Clamp(m_fMoraleSuppressionPlus + (count/5) * SUPPRESSION_BULLET_INCREMENT, 0, 4.0);
				break;
			}
			case DCO_CUSTOMRANK.SPECIALIST:
			{
				m_fMoraleSuppressionPlus = Math.Clamp(m_fMoraleSuppressionPlus + (count/8) * SUPPRESSION_BULLET_INCREMENT, 0, 4.0);
				break;
			}
		}
		m_fMoraleSuppression += Math.Clamp(m_fMoraleSuppression + m_fMoraleSuppressionPlus, 0, 3.0);
		aimDecrase = Math.Clamp(aimDecrase + count * SUPPRESSION_BULLET_INCREMENT/5, 0, 2.0);
	}
	
	void threatmodifierToMorale(float modifier)
	{
		switch(rank)
		{
			case DCO_CUSTOMRANK.RECRUIT:
			{
				m_fMoraleThreatMod = Math.Clamp(m_fMoraleThreatMod + modifier * 1.2, 0, 0.5);
				break;
			}
			case DCO_CUSTOMRANK.PRIVATE:
			{
				m_fMoraleThreatMod = Math.Clamp(m_fMoraleThreatMod + modifier, 0, 0.5);
				break;
			}
			case DCO_CUSTOMRANK.PRIVATE_FIRST_CLASS:
			{
				m_fMoraleThreatMod = Math.Clamp(m_fMoraleThreatMod + modifier / 0.8, 0, 0.5);
				break;
			}
			case DCO_CUSTOMRANK.SPECIALIST:
			{
				m_fMoraleThreatMod = Math.Clamp(m_fMoraleThreatMod + modifier / 0.65, 0, 0.5);
				break;
			}
		}		
	}
	
	float friendlyMoraleBoost()
	{
		return MORALE_BOOST_FIXED_FRIENDLY_VALUE;
	}
	
	DCO_GroupTactic setTac(DCO_GroupTactic tactics)
	{
		tacs = tactics;
		return tactics;
	}
}