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