void SCR_AIOnTacticChange(SCR_AIInfoComponent agent, DCO_GroupTactics tactics);
typedef func SCR_AIOnTacticChange;

modded class SCR_AIGroupUtilityComponent
{
	ref ScriptInvokerBase<SCR_AIOnTacticChange> m_OnTacticsChange = new ScriptInvokerBase<SCR_AIOnTacticChange>();
	override protected void OnAgentAdded(AIAgent agent)
	{
		super.OnAgentAdded(agent);
		// Add to array of AIInfo
		SCR_ChimeraAIAgent chimeraAgent = SCR_ChimeraAIAgent.Cast(agent);
		if (!chimeraAgent)
			return;

		SCR_AIInfoComponent info = chimeraAgent.m_InfoComponent;
		
		if (!info)
			return;
		
		info.SetMyGroup(m_Owner);
	}
	
	//------------------------------------------------------------------------------------------------
	//! \param[in] group
	//! \param[in] agent
	override protected void OnAgentRemoved(SCR_AIGroup group, AIAgent agent)
	{	
		super.OnAgentRemoved(group, agent);
		
		/*
		for (int i = m_aInfoComponents.Count() - 1; i >= 0; i--)
		{
			if (!m_aInfoComponents[i])
			{
				Debug.Error("Null AI info occured");
				m_aInfoComponents.RemoveOrdered(i);
			}
			else if (m_aInfoComponents[i].IsOwnerAgent(agent))
			{
				SCR_AIInfoComponent infoComp = m_aInfoComponents[i];

				break;
			}
		}*/
	}
	
	override protected void OnAgentLifeStateChanged(AIAgent incapacitatedAgent, SCR_AIInfoComponent infoIncap, IEntity vehicle, ECharacterLifeState lifeState)
	{
		m_OnAgentLifeStateChanged.Invoke(incapacitatedAgent, infoIncap, vehicle, lifeState);
		SCR_AIDangerEvent_Killzone kz = new SCR_AIDangerEvent_Killzone();
		kz.SetPosition(incapacitatedAgent.GetControlledEntity().GetOrigin());
	}
}