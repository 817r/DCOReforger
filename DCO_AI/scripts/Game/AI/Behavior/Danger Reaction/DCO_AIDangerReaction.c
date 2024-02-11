[BaseContainerProps()]
modded class SCR_AIDangerReaction : SCR_AIReactionBase
{
	bool IsDangerChanceByDistance(float distanceToDanger, out float dangerReactionChance)
	{
		dangerReactionChance = 15;
		
		if (distanceToDanger < 80)
			dangerReactionChance = 100;
		else if (distanceToDanger < 150)
			dangerReactionChance = 60;
		else if (distanceToDanger < 350)
			dangerReactionChance = 35;
		else if (distanceToDanger < 600)
			dangerReactionChance = 15;
		
		if (Math.RandomFloat(0,100) < dangerReactionChance)
			return true;
		
		return false;
	}

	bool IsMoveFromDangerChanceBySetting(SCR_AIUtilityComponent utility)
	{
		DCO_AIInfoComponent aiInfoComponent = utility.m_DCO_AIInfoComponent;
		
		if (aiInfoComponent)
		{
			float threatSuppressionIsDanger = Math.RandomFloat(0.5,1.0);
			
			threatSuppressionIsDanger = Math.RandomFloat(0.3,0.7);
			
			float moveFromDangerThreatThresholdModifier = aiInfoComponent.GetMoveFromDangerThreatThresholdModifier();
			
			if (moveFromDangerThreatThresholdModifier != 0)
				threatSuppressionIsDanger += moveFromDangerThreatThresholdModifier;
			
			int moveFromDangerChance = aiInfoComponent.GetMoveFromDangerChance();
			
			float threatSuppression = utility.m_ThreatSystem.GetThreatSuppression();
			
			if (utility.m_CombatComponent.GetCurrentTarget() == null)
			{
				moveFromDangerChance = 100;
				
				threatSuppressionIsDanger = 1;
				
				moveFromDangerChance = Math.RandomInt(50,100);
				
				threatSuppressionIsDanger += moveFromDangerThreatThresholdModifier;
				
			}
			
			if (threatSuppression > threatSuppressionIsDanger && Math.RandomInt(0,100) < moveFromDangerChance)
				return true;
		}
		
		return false;
	}
};