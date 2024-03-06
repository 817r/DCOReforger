modded class SCR_AIGetAimErrorOffset: AITaskScripted
{
	//------------------------------------------------------------------------------------------------
	protected string m_Faction;
	protected IEntity m_ControlledEntity;
	protected DCO_AIInfoComponent m_DCO_AIInfoComponent;
	protected SCR_AIUtilityComponent m_SCR_AIUtilityComponent;
	
	//------------------------------------------------------------------------------------------------
	override void OnInit(AIAgent owner)
	{
		super.OnInit(owner);
		
		SCR_AIGroup aiGroup = SCR_AIGroup.Cast(owner);
		
		m_ControlledEntity = owner.GetControlledEntity();
		
		if (aiGroup)
			m_Faction = aiGroup.m_faction;
		
		m_SCR_AIUtilityComponent = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
		
		m_DCO_AIInfoComponent = m_SCR_AIUtilityComponent.m_DCO_AIInfoComponent;
	}

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
    {
		BaseTarget baseTarget;
		
		GetVariableIn(PORT_BASE_TARGET, baseTarget);
		
		if (baseTarget)
			return super.EOnTaskSimulate(owner, dt);
		
		return ENodeResult.FAIL;
	}
	
	static float GetAimError(EAISkill skill)
	{
		float sigma = 1.50;
		
		switch (skill)
		{
			case EAISkill.RECRUIT:
			{
				sigma = 2.50;
				break;
			}
			case EAISkill.ROOKIE:
			{
				sigma = 2.00;
				break;
			}
			case EAISkill.TRAINED:
			{
				sigma = 1.70;
				break;
			}
			case EAISkill.REGULAR:
			{
				sigma = 1.50;
				break;
			}
			case EAISkill.VETERAN:
			{
				sigma = 1.00;
				break;
			}
			case EAISkill.EXPERT:
			{
				sigma = 0.50;
				break;
			}
			case EAISkill.CYLON:
			{
				sigma = 0.10;
				break;
			}
		}
		
		return sigma;
	}

	override float GetRandomFactor(EAISkill skill, float mu)
	{
		float aimAccuracyError = m_DCO_AIInfoComponent.GetAimAccuracyError();
		
		float sigma = aimAccuracyError;
		
		float aimAccuracyErrorThreatIncrement = m_SCR_AIUtilityComponent.m_ThreatSystem.GetThreatInjury();
		
		aimAccuracyErrorThreatIncrement += m_SCR_AIUtilityComponent.m_ThreatSystem.GetThreatSuppression();
		
		sigma += aimAccuracyErrorThreatIncrement;
		
		float sigmaClamp = Math.Clamp(sigma, 0, 10);
		
		sigma = sigmaClamp;
		
		if (sigma < 0)
			sigma = 0;
		
		return Math.RandomGaussFloat(sigma, mu);
	}
};