class DCO_TacticsEvaluation : AITaskScripted
{	
	protected static const string PORT_TAC_TREE_ID = "Tactics Tree ID";

	// These IDs must match to actual trees in attack tree
	protected const int DEFENSIVE 		= 0;
	protected const int EVASIVE			= 1;
	protected const int OFFENSIVE		= 2;
	protected const int AGGRESIVE		= 3;
	
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
		int tacTreeId;
		
		switch(tacs)
		{
			case DCO_GroupTactic.DEFENSIVE: tacTreeId = 0; break;
			case DCO_GroupTactic.EVASIVE: tacTreeId = 1; break;
			case DCO_GroupTactic.OFFENSIVE: tacTreeId = 2; break;
			case DCO_GroupTactic.AGGRESIVE: tacTreeId = 3; break;
		}
		
		SetVariableOut(PORT_TAC_TREE_ID, tacTreeId);
		
		return ENodeResult.SUCCESS;
	}
	
		protected ref TStringArray s_aVarsOut = {
		PORT_TAC_TREE_ID
	};

	override TStringArray GetVariablesOut() { return s_aVarsOut; }
	
	override bool VisibleInPalette() { return true; }
	
	override string GetOnHoverDescription() { return "Special node which is used in Tactical Evaluation behavior"; };

}