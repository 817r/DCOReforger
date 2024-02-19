modded class SCR_AICombatComponent : ScriptComponent
{
	protected SCR_AIGroup m_SCR_AIGroup;
	protected IEntity m_ControlledEntity;
	protected SCR_ChimeraAIAgent m_SCR_ChimeraAIAgent;
	
	protected DCO_AIInfoComponent m_DCO_AIInfoComponent;
	
	protected static const int	ENEMIES_SIMPLIFY_THRESHOLD = 15;
	

	protected static const float TARGET_INVESTIGATE_TIME = 1.0;	
	
	protected static const float TARGET_MAX_DISTANCE_DISARMED = 2.0;
	
	protected static const float TARGET_MAX_DISTANCE_INFANTRY = 500.0;
	
	protected const float PERCEPTION_FACTOR_SAFE = 4.0;
	protected const float PERCEPTION_FACTOR_VIGILANT = 5.0;
	protected const float PERCEPTION_FACTOR_ALERTED = 5.0; 
	protected const float PERCEPTION_FACTOR_THREATENED = 5.0;
	protected const float PERCEPTION_FACTOR_PINNED = 4.0;
	protected const float PERCEPTION_FACTOR_EXHAUSTED = 4.0;
	
	protected const float PERCEPTION_FACTOR_EQUIPMENT_BINOCULARS = 2.0;
	protected const float PERCEPTION_FACTOR_EQUIPMENT_NONE = 1.0;
	
	protected static const float TARGET_MAX_LAST_SEEN_DIRECT_ATTACK = 2.0;
	
	protected static const float TARGET_SCORE_RETREAT = 150.0;
	
	protected static const float TARGET_INVISIBLE_TIME = 8.0;

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
	
	override void UpdatePerceptionFactor(PerceptionComponent perceptionComp, SCR_AIThreatSystem threatSystem)
	{
		EAIThreatState threatState = threatSystem.GetState();
		float perceptionFactor;
		switch (threatState)
		{
			case EAIThreatState.SAFE:
				perceptionFactor = PERCEPTION_FACTOR_SAFE; break; 
			case EAIThreatState.VIGILANT:
				perceptionFactor = PERCEPTION_FACTOR_VIGILANT; break;
			case EAIThreatState.ALERTED:
				perceptionFactor = PERCEPTION_FACTOR_ALERTED; break; 
			case EAIThreatState.THREATENED:
				perceptionFactor = PERCEPTION_FACTOR_THREATENED; break;
		}
		
		perceptionFactor *= m_fEquipmentPerceptionFactor;
		
		perceptionComp.SetPerceptionFactor(perceptionFactor);
	}
};