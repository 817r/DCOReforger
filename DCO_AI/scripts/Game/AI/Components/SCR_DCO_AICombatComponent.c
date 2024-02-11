modded class SCR_AICombatComponent : ScriptComponent
{
	protected SCR_AIGroup m_SCR_AIGroup;
	protected IEntity m_ControlledEntity;
	protected SCR_ChimeraAIAgent m_SCR_ChimeraAIAgent;
	
	protected DCO_AIInfoComponent m_DCO_AIInfoComponent;
	
	protected const float PERCEPTION_FACTOR_SAFE = 6.0;
	protected const float PERCEPTION_FACTOR_ALERTED = 7.0; 
	protected const float PERCEPTION_FACTOR_THREATENED = 4.0;
	
	protected static const float TARGET_MAX_LAST_SEEN_DIRECT_ATTACK = 1.6;
	
	protected static const float TARGET_INVISIBLE_TIME = 10.0;

	override protected void EOnInit(IEntity owner)
	{
		GetAiAgent();
		
		super.EOnInit(owner);
		
		if (m_Agent)
		{
			m_ControlledEntity = m_Agent.GetControlledEntity();
			
			m_SCR_ChimeraAIAgent = SCR_ChimeraAIAgent.Cast(m_Agent);
			
			m_DCO_AIInfoComponent = DCO_AIInfoComponent.Cast(m_Agent.FindComponent(DCO_AIInfoComponent));
		}
	}
};