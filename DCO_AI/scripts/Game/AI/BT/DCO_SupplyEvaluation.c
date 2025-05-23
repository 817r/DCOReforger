class DCO_LowSupplyEvaluation : AITaskScripted
{
	// Output ports
	protected static const string PORT_ENTITY_TO_PICK = "Entity To Pick";
	protected static const string PORT_ENTITY_LOCATION = "Entity Location";
	
	protected SCR_AICombatComponent m_CombatComponent;
	protected SCR_AIUtilityComponent m_UtilityComponent;
	protected DCO_AIDetectionSystemComponent DCO_AIDetections;
	
	//--------------------------------------------------------------------------------------------
	override void OnInit(AIAgent owner)
	{
		IEntity controlledEnt = owner.GetControlledEntity();
		m_CombatComponent = SCR_AICombatComponent.Cast(controlledEnt.FindComponent(SCR_AICombatComponent));
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
		
		DCO_AIDetections.GetDeadAllies(m_UtilityComponent.dedAlly);
		if (m_UtilityComponent.dedAlly.IsEmpty()) return ENodeResult.FAIL;
		float dist = 50;
		IEntity toPick = null;
		for(int i = m_UtilityComponent.dedAlly.Count(); i < 0; i--)
		{
			IEntity toPickTemp = m_UtilityComponent.dedAlly[i];
			float temp = vector.Distance(owner.GetOrigin(), toPickTemp.GetOrigin());
			if (dist > temp)
			{
				dist = temp;
				toPick = toPickTemp;
			}
		}
		
		//IEntity toPick = m_UtilityComponent.dedAlly.GetRandomElement();
		if (!toPick) return ENodeResult.FAIL;
		
		SetVariableOut(PORT_ENTITY_TO_PICK, toPick);
		SetVariableOut(PORT_ENTITY_LOCATION, toPick.GetOrigin());
		
		return ENodeResult.SUCCESS;
	}
	
	//--------------------------------------------------------------------------------------------
	static override bool VisibleInPalette()
	{
		return true;
	}
	
	protected static ref TStringArray s_aVarsOut = {
		PORT_ENTITY_TO_PICK,
		PORT_ENTITY_LOCATION
	};

	//------------------------------------------------------------------------------------------------
	override TStringArray GetVariablesOut()
	{
		return s_aVarsOut;
	}
}