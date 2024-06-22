class AICustomTaskFire: AITaskFire
{
	protected SCR_AICombatComponent m_CombatComponent;
	
	void improveAccuracy()
	{
		m_CombatComponent.improvement(0.35);
	}
}

class AIResetCustomTaskFire: AITaskFire
{
	protected SCR_AICombatComponent m_CombatComponent;
	
	void improveAccuracy()
	{
		m_CombatComponent.resetImprovement();
	}
}