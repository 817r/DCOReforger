modded class SCR_AIGetAimErrorOffset: AITaskScripted
{
	static const string PORT_ERROR_OFFSET = "ErrorOffset";
	static const string PORT_BASE_TARGET = "BaseTargetIn";
	static const string PORT_AIM_POINT = "AimPoint";
	static const string PORT_TOLERANCE = "AimingTolerance";
	static const float CLOSE_RANGE_THRESHOLD = 15.0;
	static const float LONG_RANGE_THRESHOLD = 150.0;
	static const float AIMING_ERROR_SCALE = 1.1; // TODO: game master and server option
	static const float AIMING_ERROR_FACTOR_MIN = 0.35; 
	static const float AIMING_ERROR_CLOSE_RANGE_FACTOR_MIN = 0.1;
	static const float AIMING_ERROR_FACTOR_MAX = 1.2;
	static const float MAXIMAL_TOLERANCE = 10.0;	
	static const float MINIMAL_TOLERANCE = 0.16;

	//------------------------------------------------------------------------------------------------
	// returns random factor based on AI skill
	override float GetRandomFactor(EAISkill skill,float mu)
	{
		float sigma;
		switch (skill)
		{
			case EAISkill.RECRUIT :
			{
				sigma = 2.73;
				break;
			}
			case EAISkill.ROOKIE :
			{
				sigma = 1.43;
				break;
			}
			case EAISkill.REGULAR :
			{
				sigma = 1.03;
				break;
			}
			case EAISkill.TRAINED :
			{
				sigma = 1.33;
				break;
			}
			case EAISkill.VETERAN :
			{
				sigma = 0.63;
				break;
			}
			case EAISkill.EXPERT :
			{
				sigma = 0.43;
				break;
			}
			case EAISkill.CYLON :
			{
				return 0.3;
			}
		}
		
		return Math.RandomGaussFloat(sigma,mu);
	}
	
	//------------------------------------------------------------------------------------------------
	// returns skill corrected by current threat level and if AI can shoot under such suppression
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
						return EAISkill.VETERAN;
					}
					case EAISkill.CYLON :
					{
						return EAISkill.EXPERT;
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
						return EAISkill.ROOKIE;
					}
					case EAISkill.REGULAR :
					{
						return EAISkill.REGULAR;
					}
					case EAISkill.VETERAN :
					{
						return EAISkill.REGULAR;
					}
					case EAISkill.EXPERT :
					{
						return EAISkill.VETERAN;
					}
					case EAISkill.CYLON :
					{
						return EAISkill.EXPERT;
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

