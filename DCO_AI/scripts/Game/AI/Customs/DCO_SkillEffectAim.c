modded class SCR_AIGetAimErrorOffset: AITaskScripted
{
	protected IEntity m_ControlledEntity;
	protected DCO_AIInfoComponent m_DCO_AIInfoComponent;
	protected SCR_AIUtilityComponent m_SCR_AIUtilityComponent;
	static const float CLOSE_RANGE_THRESHOLD = 15.0;
	static const float LONG_RANGE_THRESHOLD = 200.0;
	static const float AIMING_ERROR_SCALE = 1.0; // TODO: game master and server option
	static const float AIMING_ERROR_FACTOR_MIN = 0.4; 
	static const float AIMING_ERROR_CLOSE_RANGE_FACTOR_MIN = 0.05;
	static const float AIMING_ERROR_FACTOR_MAX = 2.0;
	static const float MAXIMAL_TOLERANCE = 11.0;
	static const float MINIMAL_TOLERANCE = 0.003;

	override float GetRandomFactor(EAISkill skill,float mu)
	{
		float sigma;
		switch (skill)
		{
			case EAISkill.ROOKIE :
			{
				sigma = 2;
				break;
			}
			case EAISkill.REGULAR :
			{
				sigma = 1.3;
				break;
			}
			case EAISkill.VETERAN :
			{
				sigma = 1.0;
				break;
			}
			case EAISkill.EXPERT :
			{
				sigma = 0.75;
				break;
			}
			case EAISkill.CYLON :
			{
				return 0.4;
			}
		}
		// PrintFormat("Gauss: %1, sigma: %2, skill: %3",result,sigma,typename.EnumToString(EAISkill,skill));
		return Math.RandomGaussFloat(sigma,mu);
	}
	
	override EAISkill GetSkillFromThreat(EAISkill inSkill, EAIThreatState threat)
	{
		switch (threat)
		{
			case EAIThreatState.THREATENED : 
			{		 
				switch (inSkill)
				{
					case EAISkill.ROOKIE :
					{
						return EAISkill.ROOKIE;
					}
					case EAISkill.REGULAR :
					{
						return EAISkill.ROOKIE;
					}
					case EAISkill.VETERAN :
					{
						return EAISkill.REGULAR;
					}
					case EAISkill.EXPERT :
					{
						return EAISkill.REGULAR;
					}
					case EAISkill.CYLON :
					{
						return EAISkill.VETERAN;
					}
				};
				break;
			}
			case EAIThreatState.ALERTED :
			{
				switch (inSkill)
				{
					case EAISkill.ROOKIE :
					{
						return EAISkill.REGULAR;
					}
					case EAISkill.REGULAR :
					{
						return EAISkill.VETERAN;
					}
					case EAISkill.VETERAN :
					{
						return EAISkill.VETERAN;
					}
					case EAISkill.EXPERT :
					{
						return EAISkill.EXPERT;
					}
					case EAISkill.CYLON :
					{
						return EAISkill.CYLON;
					}
				};
				break;
			}
			default :
			{
				return inSkill;
				break;
			}	
		}	
		return EAISkill.NONE;
	}
};