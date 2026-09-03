class DCO_PersonalityCombatUtility
{
	static float GetMoraleThresholdScale(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);
		
		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 0.75;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 1.05;
			case DCO_EAIPersonality.RECKLESS:
				return 1.15;
			default:
				return 1.0;
		}
		return 1.0;
	}

	static float GetStoppedWaitTimeScale(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);
		
		switch (p)
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
	
	static float GetBurstDistanceScale(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);
		
		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 0.6;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 1.4;
			case DCO_EAIPersonality.RECKLESS:
				return 1.7;
			default:
				return 1.0;
		}
		return 1.0;
	}
	
	//------------------------------------------------------------------------------------------------
	protected static DCO_EAIPersonality GetPersonalitySafe(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_DCOConfig)
			return DCO_EAIPersonality.STANDARD;
		
		return utility.m_DCOConfig.GetPersonality();
	}

	static float GetGrenadeThrowChance(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);
		
		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 0.35;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 0.85;
			case DCO_EAIPersonality.RECKLESS:
				return 0.98;
			default:
				return 0.7;
		}
		return 0.65;
	}

	static float GetEndangeredReturnFireThreshold(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);
		
		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 2;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 1.2;
			case DCO_EAIPersonality.RECKLESS:
				return 1;
			default:
				return 1.7; // STANDARD
		}
		return 1.7;
	}

	static float GetTakeCoverChanceScale(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);
		
		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 1.3;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 0.6;
			case DCO_EAIPersonality.RECKLESS:
				return 0.3;
			default:
				return 1.0; // STANDARD
		}
		return 1.0;
	}
	
	static float GetInvestigateEagernessScale(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);
		
		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 1.6;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 0.6;
			case DCO_EAIPersonality.RECKLESS:
				return 0.4;
			default:
				return 1.0; // STANDARD
		}
		return 1.0;
	}
	
	protected static DCO_AISKILL GetSkillSafe(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_DCOConfig)
			return DCO_AISKILL.REGULAR;
		
		return utility.m_DCOConfig.GetAISkill();
	}
	
	static float GetDisciplineBreakChanceScale(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);
		
		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 0.4;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 1.2;
			case DCO_EAIPersonality.RECKLESS:
				return 1.6;
			default:
				return 1.0; // STANDARD
		}
		return 1.0;
	}
	
	static float GetSkillDisciplineFactor(SCR_AIUtilityComponent utility)
	{
		DCO_AISKILL skill = GetSkillSafe(utility);
		
		switch (skill)
		{
			case DCO_AISKILL.NOOB:        return 1.0;
			case DCO_AISKILL.ROOKIE:      return 0.8;
			case DCO_AISKILL.REGULAR:     return 0.55;
			case DCO_AISKILL.VETERAN:     return 0.35;
			case DCO_AISKILL.EXPERT:      return 0.2;
			case DCO_AISKILL.SPECIAL_OPS: return 0.08;
			case DCO_AISKILL.TERMINATOR:  return 0.0;
			default: return 0.55;
		}
		return 0.55;
	}
	
	static float GetThreatRememberFromPersonalityScale(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);
		
		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 5;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 3;
			case DCO_EAIPersonality.RECKLESS:
				return 0.5;
			default:
				return 1;
		}
		return 2.5;
	}
	
	static float GetObserveDurationScale(SCR_AIUtilityComponent utility)
	{
		DCO_EAIPersonality p = GetPersonalitySafe(utility);

		switch (p)
		{
			case DCO_EAIPersonality.CAUTIOUS:
				return 1.5;
			case DCO_EAIPersonality.AGGRESSIVE:
				return 0.7;
			case DCO_EAIPersonality.RECKLESS:
				return 0.5;
			default:
				return 1.0;
		}
		return 1.0;
	}
}