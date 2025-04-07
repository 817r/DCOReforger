class DCO_Scavange : AITaskScripted
{
	// Output ports
	protected static const string PORT_ID_OUT = "ID To Do";
	
	protected static const int ID_DONT_SCAVANGE = 0;	
	protected static const int ID_SCAVANGE = 1;
	protected static const int ID_RESUPPLY_BOX = 2;
	
	protected SCR_AICombatComponent m_CombatComponent;
	protected SCR_AIUtilityComponent m_UtilityComponent;
	protected DCO_AIDetectionSystemComponent DCO_AIDetections;
	protected DCO_AIMoraleSystemComponent DCO_MoraleSystem;
	
	//--------------------------------------------------------------------------------------------
	override void OnInit(AIAgent owner)
	{
		IEntity controlledEnt = owner.GetControlledEntity();
		m_CombatComponent = SCR_AICombatComponent.Cast(controlledEnt.FindComponent(SCR_AICombatComponent));
		DCO_MoraleSystem = DCO_AIMoraleSystemComponent.Cast(controlledEnt.FindComponent(DCO_AIMoraleSystemComponent));
		m_UtilityComponent = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
		DCO_AIDetections = m_UtilityComponent.DCO_AIDetection;
		
		if (!m_CombatComponent || !m_UtilityComponent || !DCO_AIDetections)
		{
			NodeError(this, owner, "SCR_AIGetCombatComponentWeapon didn't find necessary components!");
		}
	}

	
	//--------------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{	
		if (!m_CombatComponent || !m_UtilityComponent || !DCO_AIDetections)
			return ENodeResult.FAIL;
		
		int ID;
		
		if (m_UtilityComponent.isScavangeWeapon()) ID = ID_SCAVANGE;
		else if (m_UtilityComponent.isResupply()) ID = ID_RESUPPLY_BOX;
		else ID = ID_DONT_SCAVANGE;
		
		SetVariableOut(PORT_ID_OUT, ID);
		return ENodeResult.RUNNING;
	}
	
	//--------------------------------------------------------------------------------------------
	static override bool VisibleInPalette()
	{
		return true;
	}
	
	protected static ref TStringArray s_aVarsOut = {
		PORT_ID_OUT
	};

	//------------------------------------------------------------------------------------------------
	override TStringArray GetVariablesOut()
	{
		return s_aVarsOut;
	}
}