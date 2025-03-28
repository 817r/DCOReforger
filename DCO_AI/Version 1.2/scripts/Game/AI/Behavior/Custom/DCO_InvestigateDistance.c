class DCO_InvestigationDistanceMax: AITaskScripted
{	
	protected static const string PORT_TAC_MAX_INVESTIGATION = "Max Distance";
	protected static const string PORT_TAC_DELAY_INVESTIGATION = "Delay Time";
	
	protected SCR_AIUtilityComponent m_UtilityComponent;
	
	//-----------------------------------------------------------------------------------------------------
	override void OnInit(AIAgent owner)
	{
		m_UtilityComponent = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
	}
	
	//-----------------------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		DCO_GroupTactic tacs = m_UtilityComponent.getTactics();;
		float tacTreeId;
		float delay;
		tacTreeId = SCRDCO_AIConfigComponent.GetInvestigationDist(owner.GetControlledEntity());
		delay = SCRDCO_AIConfigComponent.GetInvestigateDelay(owner.GetControlledEntity());
		
		SetVariableOut(PORT_TAC_DELAY_INVESTIGATION, delay);
		SetVariableOut(PORT_TAC_MAX_INVESTIGATION, tacTreeId);
		
		return ENodeResult.SUCCESS;
	}
	
		protected ref TStringArray s_aVarsOut = {
		PORT_TAC_MAX_INVESTIGATION,
		PORT_TAC_DELAY_INVESTIGATION
	};

	override TStringArray GetVariablesOut() { return s_aVarsOut; }
	
	override bool VisibleInPalette() { return true; }
	
	override string GetOnHoverDescription() { return "Special node which is used in Tactical Evaluation behavior"; };

}