void SCR_AIOnTacticChange(SCR_AIInfoComponent agent, DCO_GroupTactics tactics);
typedef func SCR_AIOnTacticChange;

modded class SCR_AIGroupUtilityComponent
{
	// === ADDED: Responsive close-range bypass tuning ===
	protected const float CLOSE_RESPONSIVE_DIST = 50.0;
	// === END ADDED ===
	
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
		CMD_EGroupRole currentRole = CMD_EGroupRole.NONE;
	    DCO_GroupUtilityComponent groupUtil = DCO_GroupUtilityComponent.Cast(m_Owner.FindComponent(DCO_GroupUtilityComponent));
		
		if (!groupUtil.GetMyCommander())
		{
			super.EvaluateCombatMode();
		}
		
	    if (m_eCombatModeExternal != EAIGroupCombatMode.RETURN_FIRE)
	    {
	        m_eCombatModeActual = m_eCombatModeExternal;
	        return;
	    }
	

	    
	    bool hasActiveOrder = false;
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
	    
	    // === ADDED: Responsive close-range bypass ===
	    // Threshold per-role di bawah bisa sampe 40-160m tergantung role/order, dan
	    // itu sebagian gantung ke hasThreat (endangering+fresh) yang gak selalu reliable
	    // buat suppressive fire yang landing terus-menerus. Ini safety net kedua: kalau
	    // ada threat DEKET & fresh, langsung FIRE_AT_WILL, jangan nunggu role-based logic.
	    if (IsAnyTargetRelevant(CLOSE_RESPONSIVE_DIST))
	    {
	        m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
	        return;
	    }
	    // === END ADDED ===
	    
	    // === ADDED: MG bypass ===
	    // Kalau grup punya gunner MG dan ada target yang relevan, langsung FIRE_AT_WILL --
	    // gak perlu nunggu threshold jarak per-role (40-160m tergantung role) yang bisa
	    // bikin telat return fire. MG efektif buat suppress dari jarak jauh, gak masuk
	    // akal nunggu musuh mendekat dulu. Riflemen tetap ngikutin behavior existing
	    // (cari cover, suppressive fire) begitu grup gak lagi HOLD_FIRE.
	    if (HasMachineGunGunner() && IsAnyTargetRelevant(250.0))
	    {
	        m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
	        return;
	    }
	    // === END ADDED ===
	
	    if (currentRole == CMD_EGroupRole.RECON || currentRole == CMD_EGroupRole.RETREAT)
	    {
			if (hasActiveOrder)
			{
		        if (IsAnyTargetRelevant(40.0))
		            m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
		        else
		            m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;			
			} else
			{
		        if (IsAnyTargetRelevant(120.0))
		            m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
		        else
		            m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;			
			}			

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
	    // === ADDED: SUPPRESS role -- tugasnya emang nembak/suppress, jadi threshold
	    // paling permisif (gak perlu nunggu target deket kayak role lain).
	    else if (currentRole == CMD_EGroupRole.SUPPRESS)
	    {
	        if (IsAnyTargetRelevant(300.0))
	            m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
	        else
	            m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;
	        return;
	    }
	    // === END ADDED ===
	
	    if (IsAnyTargetRelevant(250.0))
	        m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
	    else
	        m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;
	}
	
	// === ADDED: MG bypass helper ===
	// Cek apakah ADA member di grup ini yang bawa MG. Dipake buat bypass threshold
	// jarak di EvaluateCombatMode() -- MG idealnya langsung return fire, gak nunggu
	// musuh mendekat dulu kayak riflemen.
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
	// === END ADDED ===
	
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