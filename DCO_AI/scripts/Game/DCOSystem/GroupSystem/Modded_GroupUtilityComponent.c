void SCR_AIOnTacticChange(SCR_AIInfoComponent agent, DCO_GroupTactics tactics);
typedef func SCR_AIOnTacticChange;

modded class SCR_AIGroupUtilityComponent
{
	ref ScriptInvokerBase<SCR_AIOnTacticChange> m_OnTacticsChange = new ScriptInvokerBase<SCR_AIOnTacticChange>();
	protected DCO_GroupUtilityComponent utilDco;
	
	ref SCR_AIGroupPerception GetPercGroupComp()
	{
		return m_Perception;
	}
	
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
	
	override void EvaluateCombatMode()
	{
		if (m_eCombatModeExternal != EAIGroupCombatMode.RETURN_FIRE)
		{
			m_eCombatModeActual = m_eCombatModeExternal;
			return;
		}
	
		CMD_EGroupRole currentRole = CMD_EGroupRole.NONE;
		DCO_GroupUtilityComponent groupUtil = DCO_GroupUtilityComponent.Cast(
			m_Owner.FindComponent(DCO_GroupUtilityComponent));
		if (groupUtil)
			currentRole = groupUtil.GetGroupRole();
	
		int targetCount = m_Perception.m_aTargetEntities.Count();
		
		if (IsAnyTargetEndangering())
		{
			m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
			return;
		}
	
		if (currentRole == CMD_EGroupRole.RECON || currentRole == CMD_EGroupRole.RETREAT)
		{
			if (IsAnyTargetWithinDistance(40.0))
				m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
			else
				m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;
			return;
		}
	
		if (currentRole == CMD_EGroupRole.ASSAULT || currentRole == CMD_EGroupRole.REINFORNCE)
		{
		    if (groupUtil && groupUtil.IsOrderActive())
		    {
		        if (IsAnyTargetWithinDistance(40.0))
		            m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
		        else
		            m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;
		        return;
		    }
		
		    if (targetCount > 0)
		        m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
		    else
		        m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;
		    return;
		}
	
		if (currentRole == CMD_EGroupRole.FLANK)
		{
		    array<AIAgent> agents = {};
		    m_Owner.GetAgents(agents);
		
		    if (groupUtil && groupUtil.IsOrderActive())
		    {
		        if (IsAnyTargetWithinDistance(30.0))
		            m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
		        else
		            m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;
		        return;
		    }
		
		    if (IsAnyTargetWithinDistance(81.0) || targetCount >= agents.Count())
		        m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
		    else
		        m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;
		    return;
		}
	
		if (targetCount > 0)
			m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
		else
			m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;
	}
	
	protected bool IsAnyTargetWithinDistance(float distThreshold)
	{
		array<AIAgent> agents = {};
		m_Owner.GetAgents(agents);
		
		if (m_Perception.m_aTargetEntities.Count() < 1)
			return false;
	
		foreach (ref IEntity target : m_Perception.m_aTargetEntities)
		{
			if (!target) 
				continue;
			foreach (AIAgent agent : agents)
			{
				IEntity controlled = agent.GetControlledEntity();
				if (!controlled)
					continue;
	
				if (vector.Distance(target.GetOrigin(), controlled.GetOrigin()) < distThreshold)
					return true;
			}
		}
	
		return false;
	}
	
	protected bool IsAnyTargetEndangering()
	{
		foreach (SCR_AIGroupTargetCluster c : m_Perception.m_aTargetClusters)
		{
			if (c.m_State && c.m_State.m_iCountEndangering != 0 && c.m_State.m_iCountAlive != 0 && c.m_State.GetTimeSinceLastNewInformation() < 2)
				return true;
		}
	
		return false;
	}
	
	override SCR_AIActionBase EvaluateActivity(out bool restartActivity)
	{
		return super.EvaluateActivity(restartActivity);
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		utilDco = DCO_GroupUtilityComponent.Cast(owner.FindComponent(DCO_GroupUtilityComponent));
		utilDco.perc = m_Perception;
	}
}