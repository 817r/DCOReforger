class DCO_MoraleCombatUtility
{
	static float GetCoverSearchDistScale(DCO_AIMoraleSystem moraleSystem, SCR_AIUtilityComponent utility = null)
	{
		float moraleScale = 1.0;

		if (moraleSystem)
		{
			switch (moraleSystem.GetState())
			{
				case moraleState.BREAK:
					moraleScale = 0.4;
					break;
				case moraleState.MANIAC:
					moraleScale = 0.6;
					break;
				case moraleState.ANXIOUS:
					moraleScale = 0.8;
					break;
				case moraleState.MOTIVATED:
					moraleScale = 1.2;
					break;
				default:
					moraleScale = 1.0;
					break;
			}
		}

		return moraleScale * GetPersonalityCoverSearchScale(utility);
	}

	static float GetPersonalityCoverSearchScale(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_DCOConfig)
			return 1.0;

		switch (utility.m_DCOConfig.GetPersonality())
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 1.4;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 0.7;
			case DCO_EAIPersonality.RECKLESS:
				return 0.5;
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

		if (moraleSystem.GetState() == moraleState.BREAK && Math.RandomFloat01() < 0.4)
			return false;

		return baseCanAim;
	}

	static float GetObserveDurationScale(DCO_AIMoraleSystem moraleSystem)
	{
		if (!moraleSystem)
			return 1.0;

		switch (moraleSystem.GetState())
		{
			case moraleState.MOTIVATED:
				return 0.85;
			case moraleState.ANXIOUS:
				return 1.3;
			case moraleState.MANIAC:
				return 0.6;
			case moraleState.BREAK:
				return 1.5;
			default:
				return 1.0;
		}
		return 1.0;
	}
}