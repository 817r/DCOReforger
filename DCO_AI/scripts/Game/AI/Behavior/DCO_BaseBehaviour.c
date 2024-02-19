modded class SCR_AIBehaviorBase : AIActionBase
{
	IEntity m_OwnerEntity;
	
	CharacterControllerComponent m_CharacterControllerComponent;
	
	void SCR_AIBehaviorBase(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity)
	{
		if (utility)
		{
			m_OwnerEntity = utility.m_OwnerEntity;
			
			if(m_OwnerEntity)
			{
				if(!m_CharacterControllerComponent)
					m_CharacterControllerComponent = CharacterControllerComponent.Cast(m_OwnerEntity.FindComponent(CharacterControllerComponent));
			}
		}
	
	};
}