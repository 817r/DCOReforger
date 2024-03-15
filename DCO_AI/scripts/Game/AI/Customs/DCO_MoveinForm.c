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
};