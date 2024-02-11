modded class SCR_AIUtilityComponent : SCR_AIBaseUtilityComponent
{
	SCR_ChimeraAIAgent m_ChimeraAIAgent;
	
	DCO_AIInfoComponent m_DCO_AIInfoComponent;
	
	protected ref SCR_AIBehaviorBase m_PreviousBehavior;

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		m_ChimeraAIAgent = SCR_ChimeraAIAgent.Cast(owner);
		
		if (m_ChimeraAIAgent)
			m_DCO_AIInfoComponent = DCO_AIInfoComponent.Cast(m_ChimeraAIAgent.FindComponent(DCO_AIInfoComponent));
	}

	override SCR_AIBehaviorBase EvaluateBehavior(BaseTarget unknownTarget)
	{
		m_CurrentBehavior = super.EvaluateBehavior(unknownTarget);
		
		if (m_CurrentBehavior && m_PreviousBehavior)
		{
			if ((m_CurrentBehavior.Type() == SCR_AIAttackBehavior) && (m_PreviousBehavior.Type() == SCR_AIAttackBehavior))
			{
				if (m_CurrentBehavior == m_PreviousBehavior)
				{
					m_CurrentBehavior = m_PreviousBehavior;
				}
				else
				{
					SCR_AIAttackBehavior currentAttackBehavior = SCR_AIAttackBehavior.Cast(m_CurrentBehavior);
					
					SCR_AIAttackBehavior previousAttackBehavior = SCR_AIAttackBehavior.Cast(m_PreviousBehavior);
					
					previousAttackBehavior.m_Target.m_Value = currentAttackBehavior.m_Target.m_Value;
					
					previousAttackBehavior.m_fWaitTime.m_Value = currentAttackBehavior.m_fWaitTime.m_Value;

					currentAttackBehavior.Complete();
					
					SetCurrentAction(m_PreviousBehavior);

					m_CurrentBehavior = m_PreviousBehavior;
				}
			}
			else
			{
				if (m_CurrentBehavior.Type() == SCR_AIIdleBehavior && m_PreviousBehavior.Type() == SCR_AIIdleBehavior)
				{
					BaseTarget currentTarget = m_CombatComponent.GetCurrentTarget();
					
					if (currentTarget)
					{
						float timeSinceSeen = currentTarget.GetTimeSinceSeen();
						
						if (timeSinceSeen < 1)
						{
							if (m_ConfigComponent.m_Reaction_EnemyTarget)
								m_ConfigComponent.m_Reaction_EnemyTarget.PerformReaction(this, m_ThreatSystem, currentTarget, currentTarget.GetLastSeenPosition());
						}
					}
				}
			}
		}

		m_PreviousBehavior = m_CurrentBehavior;
		
		return m_CurrentBehavior;
	}
};