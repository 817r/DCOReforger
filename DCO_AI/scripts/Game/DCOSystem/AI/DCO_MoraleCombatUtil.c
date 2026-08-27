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
	
	// === ADDED: Personality System ===
	//! AGGRESSIVE/RECKLESS asal cover cepet (radius kecil, males mikir), CAUTIOUS nyari
	//! yang bener-bener aman (radius lebih lebar).
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
	// === END ADDED ===
	
	//------------------------------------------------------------------------------------------------
	//! Override stance berdasarkan morale. BREAK = refleks nunduk abis (PRONE) gak peduli
	//! baseStance apa. MANIAC = males full-prone, minimal CROUCH aja (reckless).
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
	
	//------------------------------------------------------------------------------------------------
	//! Gate buat aim-while-moving. BREAK = kepanikan, gak sempet aim walau secara teknis
	//! stance/movement-type-nya ngizinin (IsAimingAndMovementPossible bilang true).
	static bool CanAimWhileMoving(bool baseCanAim, DCO_AIMoraleSystem moraleSystem)
	{
		if (!moraleSystem)
			return baseCanAim;
		
		if (moraleSystem.GetState() == moraleState.BREAK && Math.RandomFloat01() < 0.4)
			return false;
		
		return baseCanAim;
	}
}