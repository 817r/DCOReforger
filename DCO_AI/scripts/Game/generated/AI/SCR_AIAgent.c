  /* ================================================================================================ /
 / DCO-EAI Modded Class ( #DCO-EAIModdedClass )                                                      /
/ ================================================================================================ */

//------------------------------------------------------------------------------------------------
modded class SCR_ChimeraAIAgent : ChimeraAIAgent
{
	AIGroup m_ParentGroup;
	
	DCO_AIInfoComponent m_DCO_AIInfoComponent;
	SCR_AICombatComponent m_SCR_AICombatComponent;
	DCO_AIInfoGroupComponent m_DCO_AIGroupInfoComponent;
	
	protected IEntity m_ControlledEntity;
	protected SCR_ChimeraAIAgent m_ChimeraAIAgent;
	protected AICharacterMovementComponent m_CharacterMovementComponent;
	protected CharacterControllerComponent m_CharacterControllerComponent;
	
	protected vector m_vSpawnPositionOrigin;
	protected float m_fAimAccuracyErrorOriginal;
	
	protected ref SCR_AIGroupFireteam m_SCR_AIGroupFireteam;
	protected ref array<ref SCR_AIGroupFireteam> m_SCR_AIGroupFireteams = {};
	
	protected SCR_AIConfigComponent m_SCR_AIConfigComponent;
	protected SCR_AISettingsComponent m_SCR_AISettingsComponent;
	
	protected SCR_GadgetManagerComponent m_SCR_GadgetManagerComponent;
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner) 
	{
		super.EOnInit(owner);
		
		m_ChimeraAIAgent = this;
		
		if (m_ChimeraAIAgent)
		{
			bool initialize = true;
			
			float initializeDelay = 500;
			
			int permanentLOD = GetPermanentLOD();
			
			m_ControlledEntity = m_ChimeraAIAgent.GetControlledEntity();
			
			if (m_ControlledEntity)
			{
				m_vSpawnPositionOrigin = m_ControlledEntity.GetOrigin();
				
				m_SCR_GadgetManagerComponent = SCR_GadgetManagerComponent.GetGadgetManager(m_ControlledEntity);
				
				m_SCR_AICombatComponent = SCR_AICombatComponent.Cast(m_ControlledEntity.FindComponent(SCR_AICombatComponent));
				
				m_CharacterMovementComponent = AICharacterMovementComponent.Cast(m_ControlledEntity.FindComponent(AICharacterMovementComponent));
				
				m_CharacterControllerComponent = CharacterControllerComponent.Cast(m_ControlledEntity.FindComponent(CharacterControllerComponent));
			}
			
			m_SCR_AISettingsComponent = SCR_AISettingsComponent.GetInstance();
			
			m_DCO_AIInfoComponent = DCO_AIInfoComponent.Cast(m_ChimeraAIAgent.FindComponent(DCO_AIInfoComponent));
			
			m_SCR_AIConfigComponent = SCR_AIConfigComponent.Cast(m_ChimeraAIAgent.FindComponent(SCR_AIConfigComponent));
			
			GetGame().GetCallqueue().CallLater(SCR_ChimeraAIAgentInitializeDelayed, initializeDelay, false, initialize);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void SCR_ChimeraAIAgentInitializeDelayed(bool initialize)
	{
		if (initialize)
		{
			int agentIndex = 1;
			
			int agentsCount = 1;
			
			m_ParentGroup = m_ChimeraAIAgent.GetParentGroup();
			
			if (m_ParentGroup)
			{
				array<AIAgent> agents = {};
				
				m_ParentGroup.GetAgents(agents);
				
				agentIndex = agents.Find(m_ChimeraAIAgent);
				
				agentsCount = m_ParentGroup.GetAgentsCount();
			}
			
			EAISkill skill = DCO_Skill.SetSkill	(agentIndex, agentsCount, m_ControlledEntity, m_SCR_AICombatComponent);
			
			m_fAimAccuracyErrorOriginal = SCR_AIGetAimErrorOffset.GetAimError(skill);
		}
		
		if (m_SCR_AISettingsComponent)
		{			
			if (initialize && m_ParentGroup)
			{
				m_SCR_AIConfigComponent = SCR_AIConfigComponent.Cast(m_ParentGroup.FindComponent(SCR_AIConfigComponent));
				
				m_DCO_AIGroupInfoComponent = DCO_AIInfoGroupComponent.Cast(m_ParentGroup.FindComponent(DCO_AIInfoGroupComponent));
			}
			
		}
	}

	void SetFireteam(SCR_AIGroupFireteam fireteam)
	{
		m_SCR_AIGroupFireteam = fireteam;
	}
	
	//------------------------------------------------------------------------------------------------
	SCR_AIGroupFireteam GetFireteam()
	{
		return m_SCR_AIGroupFireteam;
	}

	ref array<ref SCR_AIGroupFireteam> GetFireteams()
	{
		if (m_SCR_AIGroupFireteams.IsEmpty())
			return null;
		
		return m_SCR_AIGroupFireteams;
	}
	
	//------------------------------------------------------------------------------------------------
	vector GetSpawnPositinOrigin()
	{
		return m_vSpawnPositionOrigin;
	}
	
	//------------------------------------------------------------------------------------------------
	void AddFireteam(SCR_AIGroupFireteam fireteam)
	{	
		m_SCR_AIGroupFireteams.Insert(fireteam);
	}
};