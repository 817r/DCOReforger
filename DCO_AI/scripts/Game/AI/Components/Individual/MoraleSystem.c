class DCO_AIMoraleSystemComponentClass : ScriptComponentClass
{
}

enum MoraleState
{
	FRESH,
	NORMAL,
	STRESSED,
	PRESSURED,
	BREAK
}

class DCO_AIMoraleSystemComponent : ScriptComponent
{
	[Attribute("0", UIWidgets.ComboBox, "AI Default Morale", "", ParamEnumArray.FromEnum(MoraleState) )]
	protected MoraleState m_MoraleState;
	
	[Attribute("100", UIWidgets.Slider, "AI Default Morale", params: "0 100 1" )]
	protected float m_fMorale;
	
	static const float NORMAL_THRESHOLD = 80;
	static const float STRESSED_THRESHOLD = 60;
	static const float PRESSURED_THRESHOLD = 35;
	static const float BREAK_THRESHOLD = 10;
	
	protected float COMBAT_MORALE_DECREASE_TARGET_CLUSTER	= 0.55;
	protected float COMBAT_MORALE_DECREASE_TARGET			= 0.4;
	protected float COMBAT_MORALE_KIA						= 0.5;
	protected float COMBAT_MORALE_SUPPRESSED				= 0.19;
	protected float COMBAT_MORALE_ENEMY_NEAR				= 0.35;
	
	protected float MORALE_RECOVERY_FIXED					= 0.08;
	protected float MORALE_BOOST_RECOVERY_LEADER_NEAR		= 0.3;
	protected float MORALE_BOOST_RECOVERY_FRIENDLY_NEAR		= 0.05;

	protected SCR_AIUtilityComponent 				m_Utility;
	protected SCR_AIInfoComponent					m_AIInfo;
	protected SCR_AICombatComponent					m_CombatComponent;
	protected SCR_ChimeraAIAgent 					agent;
	protected DCO_AIDetectionSystemComponent 		DCO_DetectionSystem;
	
	
	override void OnPostInit(IEntity owner)
    {		
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
        SetEventMask(owner, EntityEvent.FRAME);
    }
	
	override void EOnInit(IEntity owner)
	{
		AIControlComponent ctrl = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		if (ctrl)
		{
			agent = SCR_ChimeraAIAgent.Cast(ctrl.GetAIAgent());
			if (agent)
			{
				m_AIInfo = agent.m_InfoComponent;
				m_Utility = SCR_AIUtilityComponent.Cast(agent.FindComponent(SCR_AIUtilityComponent));
			}
		}
        
		m_CombatComponent = SCR_AICombatComponent.Cast(owner.FindComponent(SCR_AICombatComponent));
		DCO_DetectionSystem = DCO_AIDetectionSystemComponent.Cast(owner.FindComponent(DCO_AIDetectionSystemComponent));	
	}

    override void EOnFrame(IEntity owner, float timeSlice)
    {
		UpdateMorale(owner, timeSlice);
    }
	
	protected void UpdateMorale(IEntity owner, float timeSlice)
    {		
		if (!m_Utility || !m_AIInfo || !m_CombatComponent) return;
		if (m_AIInfo.HasUnitState(EUnitState.UNCONSCIOUS)) return;
		
		if (m_CombatComponent.GetAITargetClusterState())
		{
			if (m_CombatComponent.GetAITargetClusterState().m_eState > EAITargetClusterState.INVESTIGATING)
        		m_fMorale -= COMBAT_MORALE_DECREASE_TARGET_CLUSTER * timeSlice;
			else if (m_CombatComponent.GetCurrentTarget())
			{
				m_fMorale -= COMBAT_MORALE_DECREASE_TARGET * timeSlice;
			}
		}
		else
			m_fMorale += MORALE_RECOVERY_FIXED * MoraleRecoverySkill() * timeSlice;

        DamageManagerComponent healthComp = DamageManagerComponent.Cast(owner.FindComponent(DamageManagerComponent));

        m_fMorale -= DCO_DetectionSystem.GetEnemiesNumber() * COMBAT_MORALE_ENEMY_NEAR * timeSlice;

        m_fMorale += DCO_DetectionSystem.GetFriendlyNumber() * MORALE_BOOST_RECOVERY_FRIENDLY_NEAR * timeSlice;
		
		if (m_AIInfo.GetThreatState() > EAIThreatState.VIGILANT)
		{
			m_fMorale -= COMBAT_MORALE_SUPPRESSED * timeSlice;
		}

        m_fMorale = Math.Clamp(m_fMorale, 0, 100);
		
		MoraleState newState;
		
		if (m_fMorale < BREAK_THRESHOLD)
			newState = MoraleState.BREAK;
		else if (m_fMorale < PRESSURED_THRESHOLD)
			newState = MoraleState.PRESSURED;
		else if (m_fMorale < STRESSED_THRESHOLD)
			newState = MoraleState.STRESSED;
		else if (m_fMorale < NORMAL_THRESHOLD)
			newState = MoraleState.NORMAL;
		else
			newState = MoraleState.FRESH;
		
		StateTransition(newState);
		
#ifdef WORKBENCH
		if (m_Utility)
		{
			if (m_Utility.m_OwnerEntity)
				ShowDebug();
		}
#endif
    }
	
#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	//!
	void ShowDebug()
	{
		// Show message above AI's head

		Color color;
		
		switch (m_MoraleState)
		{
			case MoraleState.FRESH:
			{
				color = Color.FromInt(Color.BLUE);
				break;
			}
			case MoraleState.NORMAL:
			{
				color = Color.FromInt(Color.GREEN);
				break;
			}
			case MoraleState.STRESSED:
			{
				color = Color.FromInt(Color.YELLOW);
				break;
			}
			case MoraleState.PRESSURED:
			{
				color = Color.FromInt(Color.RED);
				break;
			}
			case MoraleState.BREAK:
			{
				color = Color.FromInt(Color.DARK_RED);
				break;
			}
		}
		
		//SCR_AIDebugVisualization.VisualizeMessage(m_Utility.m_OwnerEntity, m_fMorale.ToString(), EAIDebugCategory.COMBAT, 1.4, color);	
	}
#endif
	
	MoraleState GetMoraleStates()
	{
		return m_MoraleState;
	}
	
	float GetMoraleValue()
	{
		return m_fMorale;
	}
	
	float MoraleRecoverySkill()
	{
		switch(m_Utility.DCO_ConfComponent.GetSkillLevel())
		{
			case DCO_SKILL.CONSCPRIT:
			{
				return 0.5;
				break;
			}
			case DCO_SKILL.GREEN:
			{
				return 0.7;
				break;
			}
			case DCO_SKILL.REGULAR:
			{
				return 1.0;
				break;
			}
			case DCO_SKILL.VETERAN:
			{
				return 1.2;
				break;
			}
			case DCO_SKILL.CRACK:
			{
				return 1.5;
				break;
			}
			case DCO_SKILL.ELITE:
			{
				return 2.0;
				break;
			}
		}
		return 1.0;
	}
	
	private void StateTransition(MoraleState newState)
	{
		if (newState == m_MoraleState)
			return;
		
		m_MoraleState = newState;
	}
}