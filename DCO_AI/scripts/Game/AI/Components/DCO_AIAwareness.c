class DCO_AIAwarenessClass : ScriptComponentClass
{
}

class DCO_AIAwareness : ScriptComponent
{
	protected SCR_AIInfoComponent m_InfoComp;
	protected SCR_AIUtilityComponent m_UtilityComp;
	protected SCR_AICombatComponent m_CombatComp;
	protected PerceptionComponent m_Perceptions;
	
	ref array<BaseTarget> friendly = new array<BaseTarget>;
	
	[Attribute(defvalue: "100", uiwidget: UIWidgets.Auto, desc: "Awareness Friendly Radius")]
	float searchRad;
	
	protected void getFriendlyEvaluation()
	{
		ref array<BaseTarget> friendlyDetected = new array<BaseTarget>;
		ref array<BaseTarget> detected = new array<BaseTarget>; 
		m_Perceptions.GetTargetsList(friendlyDetected, ETargetCategory.FRIENDLY);
		m_Perceptions.GetTargetsList(detected, ETargetCategory.DETECTED);
		
		foreach (BaseTarget target : friendlyDetected)
		{
			if (target.GetDistance() < searchRad)
			{
				if (!friendly.Contains(target))
					friendly.Insert(target);
			} else if (target.GetDistance() > searchRad + 50)
			{
				if (friendly.Contains(target))
					friendly.RemoveItem(target);
			}
		}
		
		foreach (BaseTarget target : detected)
		{
			if (friendly.Contains(target))
			{
				friendly.RemoveItem(target);
			}
		}
	}

	
	void initialize(SCR_AIUtilityComponent util)
	{
		m_UtilityComp = util;
		m_CombatComp = m_UtilityComp.m_CombatComponent;
		m_InfoComp = m_UtilityComp.m_AIInfo;
		m_Perceptions = m_UtilityComp.m_PerceptionComponent;
	}
	
	void Update()
	{
		getFriendlyEvaluation();
		#ifdef Workbench
		float friendlyNumber = friendly.Count();
		//SCR_AIDebugVisualization.VisualizeMessage(m_UtilityComp.m_OwnerEntity, friendlyNumber.ToString(), EAIDebugCategory.INFO, 1.4, Color.White);
		#endif	
	}
	
	float getNumberFriendlyRecognized()
	{
		return friendly.Count();
	}
}