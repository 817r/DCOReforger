//------------------------------------------------------------------------------------------------
class DCO_AIGroupTactics : DecoratorScripted
{
	static private string THRESHOLD_PORT = "ThresholdIn";
	
	SCR_DCO_AIGroupConfigComponent groupConfig;
	
	[Attribute("1", UIWidgets.ComboBox, "Threat threshold", "", ParamEnumArray.FromEnum(DCO_GroupTactics) )]
	private DCO_GroupTactics m_threatThreshold;
	
	//------------------------------------------------------------------------------------------------
	protected override void OnInit(AIAgent owner)
	{
		AIGroup grp = owner.GetParentGroup();
		groupConfig = SCR_DCO_AIGroupConfigComponent.Cast(grp.FindComponent(SCR_DCO_AIGroupConfigComponent));
	}
	
	//------------------------------------------------------------------------------------------------
	protected override bool TestFunction(AIAgent owner)
	{		
		if (!groupConfig)
		{
			return false;
		};
		
		DCO_GroupTactics threshold;
		if (!GetVariableIn(THRESHOLD_PORT,threshold))
			threshold = m_threatThreshold;
		
		return threshold == groupConfig.GetTactics();	
	}
	
	//------------------------------------------------------------------------------------------------
	protected static override string GetOnHoverDescription()
	{
		return "Returns true if current threat state is higher than given threshold.";
	}
	
	//------------------------------------------------------------------------------------------------
	override protected string GetNodeMiddleText()
	{
		return "Threshold: " + typename.EnumToString(DCO_GroupTactics,m_threatThreshold);
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
