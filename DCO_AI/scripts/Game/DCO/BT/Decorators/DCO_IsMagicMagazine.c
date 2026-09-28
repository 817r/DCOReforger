class DCO_IsAIMagicMagazine : DecoratorScripted
{
	SCR_AIInfoComponent m_InfoComponent;

	protected override void OnInit(AIAgent owner)
	{
		SCR_ChimeraAIAgent chimeraAgent = SCR_ChimeraAIAgent.Cast(owner);
		if (!chimeraAgent)
			SCR_AgentMustChimera(this, owner);
		m_InfoComponent = chimeraAgent.m_InfoComponent;
	}

	protected override bool TestFunction(AIAgent owner)
	{
		if (!m_InfoComponent)
		{
			return false;
		};

		return m_InfoComponent.GetUtilityComp().m_DCOConfig.GetMagicMag();
	}
};