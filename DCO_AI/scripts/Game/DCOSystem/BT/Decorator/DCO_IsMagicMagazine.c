class DCO_IsAIMagicMagazine : DecoratorScripted
{
	SCR_AIInfoComponent m_InfoComponent;
	
	//------------------------------------------------------------------------------------------------
	protected override void OnInit(AIAgent owner)
	{
		SCR_ChimeraAIAgent chimeraAgent = SCR_ChimeraAIAgent.Cast(owner);
		if (!chimeraAgent)
			SCR_AgentMustChimera(this, owner);
		m_InfoComponent = chimeraAgent.m_InfoComponent;
	}
	
	//------------------------------------------------------------------------------------------------
	protected override bool TestFunction(AIAgent owner)
	{		
		if (!m_InfoComponent)
		{
			//Print("NO INFO COMPONENT");
			return false;
		};
		
		//Print("DIST : " + vector.Distance(dist, m_InfoComponent.GetUtilityComp().m_OwnerEntity.GetOrigin()).ToString() + " THRESHOLD : " + m_distanceThreshold);
		return m_InfoComponent.GetUtilityComp().m_DCOConfig.GetMagicMag();	
	}
};