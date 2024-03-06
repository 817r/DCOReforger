modded class SCR_AIMoveInFormationBehavior : SCR_AIMoveBehaviorBase
{
	protected bool m_DisableMovementControls;
	
	protected DCO_AIInfoComponent m_DCO_AIInfoComponent;

	override void OnActionSelected()
	{
		super.OnActionSelected();
		
		IEntity ownerEntity = m_Utility.m_OwnerEntity;
		
		if (m_CharacterControllerComponent)
		{
			m_CharacterControllerComponent.SetWeaponADS(false);
			
			m_CharacterControllerComponent.SetWeaponRaised(false);
			
			m_CharacterControllerComponent.SetStanceChange(ECharacterStanceChange.STANCECHANGE_TOERECTED);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnActionDeselected()
	{
		super.OnActionDeselected();
		
		if (m_DisableMovementControls && m_CharacterControllerComponent)
			m_CharacterControllerComponent.SetDisableMovementControls(true);
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnActionCompleted()
	{
		super.OnActionCompleted();
	}
	
	//------------------------------------------------------------------------------------------------
	void SCR_AIMoveInFormationBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector pos, float priority = PRIORITY_BEHAVIOR_MOVE_IN_FORMATION, float priorityLevel = PRIORITY_LEVEL_NORMAL)
	{
		if (priorityLevel > 0)
		{
			SCR_ChimeraAIAgent chimeraAIAgent = utility.m_ChimeraAIAgent;
			
			if (chimeraAIAgent)
			{
				m_DCO_AIInfoComponent = DCO_AIInfoComponent.Cast(chimeraAIAgent.FindComponent(DCO_AIInfoComponent));
				
				if (m_DCO_AIInfoComponent)
				{
					m_DisableMovementControls = m_DCO_AIInfoComponent.GetDisableMovementControls();
					
					if (m_DisableMovementControls)
					{
						IEntity ownerEntity = utility.m_OwnerEntity;
						
						if (ownerEntity)
						{
							if (m_CharacterControllerComponent && m_CharacterControllerComponent.GetDisableMovementControls())
								m_CharacterControllerComponent.SetDisableMovementControls(false);
						}
					}
				}
			}
		}
	}
};