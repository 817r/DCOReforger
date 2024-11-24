//------------------------------------------------------------------------------------------------
class SCR_AIDecoIsAboveMoraleLevel : DecoratorScripted
{
	static private string THRESHOLD_PORT = "ThresholdIn";
	
	SCR_AIInfoComponent m_InfoComponent;
	
	[Attribute("1", UIWidgets.ComboBox, "Morale threshold", "", ParamEnumArray.FromEnum(moraleState) )]
	private moraleState m_MoraleThreshold;
	
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
		
		moraleState threshold;
		if (!GetVariableIn(THRESHOLD_PORT,threshold))
			threshold = m_MoraleThreshold;
		
		return threshold < m_InfoComponent.getMoraleState();	
	}
	
	//------------------------------------------------------------------------------------------------
	protected override string GetOnHoverDescription()
	{
		return "Returns true if current threat state is higher than given threshold.";
	}
	
	//------------------------------------------------------------------------------------------------
	override protected string GetNodeMiddleText()
	{
		return "Threshold: " + typename.EnumToString(moraleState,m_MoraleThreshold);
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

class SCR_AIDecoTactics : DecoratorScripted
{
	static private string THRESHOLD_PORT = "Tactics";
	
	SCR_AIUtilityComponent m_UtilityComponent;
	
	[Attribute("1", UIWidgets.ComboBox, "Tactics", "", ParamEnumArray.FromEnum(DCO_GroupTactic) )]
	private DCO_GroupTactic m_TacticsTreshold;
	
	//------------------------------------------------------------------------------------------------
	protected override void OnInit(AIAgent owner)
	{
		SCR_ChimeraAIAgent chimeraAgent = SCR_ChimeraAIAgent.Cast(owner);
		if (!chimeraAgent)
			SCR_AgentMustChimera(this, owner);
		m_UtilityComponent = chimeraAgent.m_UtilityComponent;
	}
	
	//------------------------------------------------------------------------------------------------
	protected override bool TestFunction(AIAgent owner)
	{		
		if (!m_UtilityComponent)
		{
			return false;
		};
		
		int threshold;
		if (!GetVariableIn(THRESHOLD_PORT,threshold))
			threshold = m_TacticsTreshold;
		
		return threshold == m_UtilityComponent.getTactics();	
	}
	
	//------------------------------------------------------------------------------------------------
	protected override string GetOnHoverDescription()
	{
		return "Returns true if current threat state is higher than given threshold.";
	}
	
	//------------------------------------------------------------------------------------------------
	override protected string GetNodeMiddleText()
	{
		return "Threshold: " + typename.EnumToString(DCO_GroupTactic,m_TacticsTreshold);
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

class DCO_AllowMovement : DecoratorScripted
{
	SCR_AIInfoComponent m_InfoComponent;
	
	[Attribute("1", UIWidgets.ComboBox, "Morale threshold", "", ParamEnumArray.FromEnum(moraleState) )]
	private bool allowMovement;
	
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
		
		allowMovement = SCRDCO_AIConfigComponent.GetEnableMovement(owner.GetControlledEntity());
		return allowMovement;	
	}
	
	//------------------------------------------------------------------------------------------------
	protected override string GetOnHoverDescription()
	{
		return "Returns true if current threat state is higher than given threshold.";
	}
};
