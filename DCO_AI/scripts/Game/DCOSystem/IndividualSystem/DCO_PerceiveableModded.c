modded class SCR_CharacterPerceivableComponent : CharacterPerceivableComponent
{
	protected SCR_CharacterControllerComponent m_CharacterController;
	bool isDead;
	
	//------------------------------------------------------------------------------------------------
	override protected void OnLifeStateChanged(ECharacterLifeState previousLifeState, ECharacterLifeState newLifeState)
	{
		bool disarmed = newLifeState != ECharacterLifeState.ALIVE;
		isDead = newLifeState == ECharacterLifeState.DEAD;
		SetDisarmed(disarmed);
	}
}