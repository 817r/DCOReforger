class DCO_IsAIDistanceMoreThan : DecoratorScripted
{
	static private string THRESHOLD_PORT = "ThresholdIn";
	
	SCR_AIInfoComponent m_InfoComponent;
	
	[Attribute("1", UIWidgets.Auto, "Distance Threshold" )]
	private float m_distanceThreshold;
	
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
			return false;
		};
		
		vector dist;
		if (!GetVariableIn(THRESHOLD_PORT,dist))
			return false;

		return m_distanceThreshold < vector.Distance(dist, m_InfoComponent.GetOwner().GetOrigin());	
	}
	
	//------------------------------------------------------------------------------------------------
	protected static override string GetOnHoverDescription()
	{
		return "Returns true if current Distance is higher than given distance.";
	}
	
	//------------------------------------------------------------------------------------------------
	override protected string GetNodeMiddleText()
	{
		return "Threshold: " + m_distanceThreshold.ToString();
	}	

	//------------------------------------------------------------------------------------------------
	protected static ref TStringArray s_aVarsIn = {
		THRESHOLD_PORT
	};
	protected override TStringArray GetVariablesIn()
	{
		return s_aVarsIn;
	}
};