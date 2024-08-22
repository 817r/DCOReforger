modded class SCR_ChimeraAIAgent : ChimeraAIAgent
{
	SCR_AICombatComponent m_SCR_AICombatComponent;
	ref DCO_Group_Info m_DCO_AIGroupInfoComponent;
	DCO_AIMoraleSystem m_DCO_AIMoraleSystem;
	DCO_SkillComponent m_DCO_SkillComponent;
	
	protected IEntity m_ControlledEntity;
	protected SCR_ChimeraAIAgent m_ChimeraAIAgent;
	protected AICharacterMovementComponent m_CharacterMovementComponent;
	protected CharacterControllerComponent m_CharacterControllerComponent;
	
	protected SCR_AIConfigComponent m_SCR_AIConfigComponent;
	protected SCR_AISettingsComponent m_SCR_AISettingsComponent;
	
	protected SCR_GadgetManagerComponent m_SCR_GadgetManagerComponent;
	
	
	override void EOnInit(IEntity owner) 
	{
		IEntity controlledEntity = GetControlledEntity();
		if (!controlledEntity)
			return;
		
		ChimeraCharacter character = ChimeraCharacter.Cast(controlledEntity);
		if (character)
		{
			m_CharacterController = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
			if (m_CharacterController)
				m_CharacterController.m_OnLifeStateChanged.Insert(OnLifeStateChanged);
		}
			
		GetGame().GetCallqueue().CallLater(EnsureAILimit, 1, false);
		
		m_FactionAffiliationComponent = FactionAffiliationComponent.Cast(controlledEntity.FindComponent(FactionAffiliationComponent));
		m_InfoComponent = SCR_AIInfoComponent.Cast(FindComponent(SCR_AIInfoComponent));
		m_UtilityComponent = SCR_AIUtilityComponent.Cast(FindComponent(SCR_AIUtilityComponent));
		m_DCO_AIGroupInfoComponent = new DCO_Group_Info(SCR_AIGroup.Cast(GetParentGroup()));
	}
};