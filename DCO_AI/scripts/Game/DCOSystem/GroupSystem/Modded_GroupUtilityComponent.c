void SCR_AIOnTacticChange(SCR_AIInfoComponent agent, DCO_GroupTactics tactics);
typedef func SCR_AIOnTacticChange;

modded class SCR_AIGroupUtilityComponent
{
	protected const float CLOSE_VISIBLE_TRIGGER_DIST = 30.0;
	
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
	
	
	override void EvaluateCombatMode()
	{
		if (!AICommander_ManagerComponent.GetInstance())
		{
			super.EvaluateCombatMode();
		}
		
	    if (m_eCombatModeExternal != EAIGroupCombatMode.RETURN_FIRE)
	    {
	        m_eCombatModeActual = m_eCombatModeExternal;
	        return;
	    }
		
		
	    DCO_GroupUtilityComponent groupUtil = DCO_GroupUtilityComponent.Cast(m_Owner.FindComponent(DCO_GroupUtilityComponent));
	    if (!groupUtil.GetMyCommander())
			return;
		
	    bool hasActiveOrder = false;
		CMD_EGroupRole currentRole = CMD_EGroupRole.NONE;
	    if (groupUtil)
	    {
	        currentRole = groupUtil.GetGroupRole();
	        hasActiveOrder = groupUtil.IsOrderActive();
	    }
	
	    int targetCount = m_Perception.m_aTargetEntities.Count();
	
	    if (targetCount == 0)
	    {
	        m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;
	        return;
	    }
	    
	    if (IsDirectlyThreatened())
	    {
	        m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
	        return;
	    }
		
	    if (HasMachineGunGunner() && IsDirectlyThreatened())
	    {
	        m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
	        return;
	    }

	    if (currentRole == CMD_EGroupRole.RECON || currentRole == CMD_EGroupRole.RETREAT)
	    {
	        if (IsDirectlyThreatened())
	            m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
	        else
	            m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;

	        return;
	    }
	    else if (currentRole == CMD_EGroupRole.ASSAULT || currentRole == CMD_EGroupRole.REINFORNCE)
	    {
	        if (hasActiveOrder)
	        {
	            if (IsAnyTargetRelevant(100.0))
	                m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
	            else
	                m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;
	            return;
	        }
	
	        if (IsAnyTargetRelevant(160.0))
	            m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
	        else
	            m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;
	        return;
	    }
	    else if (currentRole == CMD_EGroupRole.FLANK)
	    {
	        array<AIAgent> agents = {};
	        m_Owner.GetAgents(agents);
	
	        if (hasActiveOrder)
	        {
	            if (IsAnyTargetRelevant(50.0))
	                m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
	            else
	                m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;
	            return;
	        }
	
	        if (IsAnyTargetRelevant(70.0) || targetCount >= agents.Count() || IsAnyTargetRelevant(170.0))
	            m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
	        else
	            m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;
	        return;
	    } 
	    else if (currentRole == CMD_EGroupRole.ARTILLERY)
	    {
	        if (IsAnyTargetRelevant(40.0))
	            m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
	        else
	            m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;
	        return;
	    }

	    else if (currentRole == CMD_EGroupRole.SUPPRESS)
	    {
	        if (IsAnyTargetRelevant(300.0))
	            m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
	        else
	            m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;
	        return;
	    }
	
	    if (IsDirectlyThreatened())
	        m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
	    else
	        m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;
	}
	
	protected bool HasMachineGunGunner()
	{
		array<AIAgent> agents = {};
		m_Owner.GetAgents(agents);
		
		foreach (AIAgent agent : agents)
		{
			IEntity controlled = agent.GetControlledEntity();
			if (!controlled)
				continue;
			
			SCR_AICombatComponent combatComp = SCR_AICombatComponent.Cast(controlled.FindComponent(SCR_AICombatComponent));
			if (!combatComp)
				continue;
			
			if (combatComp.HasWeaponOfType(EWeaponType.WT_MACHINEGUN))
				return true;
		}
		
		return false;
	}

	protected bool IsDirectlyThreatened()
	{
		foreach (SCR_AIGroupTargetCluster c : m_Perception.m_aTargetClusters)
		{
			if (!c.m_State)
				continue;
			
			bool hasThreat = (c.m_State.m_iCountEndangering > 0 && c.m_State.m_iCountAlive > 0);
			bool isFresh   = (c.m_State.GetTimeSinceLastNewInformation() < 5.0);
			
			if (hasThreat && isFresh)
				return true;
			
			bool isVeryClose = (c.m_State.m_fDistMin < CLOSE_VISIBLE_TRIGGER_DIST);
			if (isVeryClose)
				return true;
		}
		
		return false;
	}
	
	protected bool IsAnyTargetRelevant(float maxRelevanceDistance = 150.0)
	{
		foreach (SCR_AIGroupTargetCluster c : m_Perception.m_aTargetClusters)
		{
			if (!c.m_State)
				continue;
			
			bool hasThreat = (c.m_State.m_iCountEndangering > 0 && c.m_State.m_iCountAlive > 0);
			
			bool isFresh = (c.m_State.GetTimeSinceLastNewInformation() < 5.0);
			
			bool isRelevantDistance = (c.m_State.m_fDistMin < maxRelevanceDistance);

			if ((hasThreat && isFresh) || isRelevantDistance)
				return true;
		}
	
		return false;
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
	
	override SCR_AIActionBase EvaluateActivity(out bool restartActivity)
	{
		return super.EvaluateActivity(restartActivity);
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		utilDco = DCO_GroupUtilityComponent.Cast(owner.FindComponent(DCO_GroupUtilityComponent));
		if (utilDco)
			utilDco.perc = m_Perception;
	}
}