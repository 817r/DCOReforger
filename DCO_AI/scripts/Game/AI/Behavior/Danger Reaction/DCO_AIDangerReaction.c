modded class SCR_AIDangerReaction : SCR_AIReactionBase
{
	bool IsDangerChanceByDistance(float distanceToDanger, out float dangerReactionChance)
	{
		dangerReactionChance = 90;
		
		if (Math.RandomFloat(0,100) < dangerReactionChance)
			return true;
		
		return false;
	}

	bool IsMoveFromDangerChanceBySetting(SCR_AIUtilityComponent utility)
	{
		DCO_AIInfoComponent aiInfoComponent = utility.m_DCO_AIInfoComponent;
		
		if (aiInfoComponent)
		{
			float threatSuppressionIsDanger = 0.8;
			
			float threatSuppression = utility.m_ThreatSystem.GetThreatSuppression();
			
			if (utility.m_CombatComponent.GetCurrentTarget() == null)
			{				
				threatSuppressionIsDanger = 1;				
			}
			
			if (Math.RandomInt(0,100) < 85)
				return true;
		}
		
		return false;
	}
};