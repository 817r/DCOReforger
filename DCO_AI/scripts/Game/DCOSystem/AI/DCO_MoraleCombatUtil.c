class DCO_MoraleCombatUtility
{
	static float GetCoverSearchDistScale(DCO_AIMoraleSystem moraleSystem)
	{
		if (!moraleSystem)
			return 1.0;
		
		switch (moraleSystem.GetState())
		{
			case moraleState.BREAK:
				return 0.4;
			case moraleState.MANIAC:
				return 0.6;
			case moraleState.ANXIOUS:
				return 0.8;
			case moraleState.MOTIVATED:
				return 1.2;
			default:
				return 1.0;
		}
		
		return 1.0;
	}
	
	static ECharacterStance ApplyMoraleStanceOverride(ECharacterStance baseStance, DCO_AIMoraleSystem moraleSystem)
	{
		if (!moraleSystem)
			return baseStance;
		
		moraleState state = moraleSystem.GetState();
		
		if (state == moraleState.BREAK)
			return ECharacterStance.PRONE;
		
		if (state == moraleState.MANIAC && baseStance == ECharacterStance.PRONE)
			return ECharacterStance.CROUCH;
		
		return baseStance;
	}

	static bool CanAimWhileMoving(bool baseCanAim, DCO_AIMoraleSystem moraleSystem)
	{
		if (!moraleSystem)
			return baseCanAim;
		
		if (moraleSystem.GetState() == moraleState.BREAK)
			return false;
		
		return baseCanAim;
	}
}