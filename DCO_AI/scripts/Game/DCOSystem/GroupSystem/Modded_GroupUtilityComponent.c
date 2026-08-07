void SCR_AIOnTacticChange(SCR_AIInfoComponent agent, DCO_GroupTactics tactics);
typedef func SCR_AIOnTacticChange;

modded class SCR_AIGroupUtilityComponent
{
	// === MODIFIED: dulu CLOSE_RESPONSIVE_DIST (50m, dicabut pas Discipline system
	// dibangun), sekarang balik lagi tapi jauh lebih ketat -- fallback safety net
	// biar grup non-combat-role gak "diem gak nembak" pas musuh udah literally
	// deket, tanpa balik ke behavior lama yang gampang aggro dari jarak jauh.
	protected const float CLOSE_VISIBLE_TRIGGER_DIST = 20.0;
	// === END MODIFIED ===
	
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
	    
	    // === MODIFIED: Discipline -- bypass yang sebelumnya PURE JARAK (CLOSE_RESPONSIVE_DIST/
	    // MG 250m) diganti jadi DIRECT THREAT ONLY (IsDirectlyThreatened -- lagi ditembakin/
	    // di-endanger beneran, bukan cuma "ada musuh kedeteksi di radius sekian"). Grup gak
	    // lagi otomatis buka tembak cuma karena ada target dalam jarak tertentu -- harus
	    // bener-bener lagi diancam langsung. Self-preservation individual (IsCloseDirectThreat/
	    // ShouldReturnFireWhenEndangered di ResolveFireTree) tetep jalan normal di luar ini.
	    if (IsDirectlyThreatened())
	    {
	        m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
	        return;
	    }
	    // === END MODIFIED ===
	    
	    // === MODIFIED: MG bypass -- sama, diganti direct-threat-only. MG tetep dapet
	    // prioritas (gak perlu nunggu role-based threshold), tapi cuma kalau BENERAN
	    // lagi diancam, bukan cuma ada target kedeteksi 250m.
	    if (HasMachineGunGunner() && IsDirectlyThreatened())
	    {
	        m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
	        return;
	    }
	    // === END MODIFIED ===

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
	
	    // === MODIFIED: Discipline -- fallback ini kepake buat role NONE/RESERVE/TRANSPORT
	    // (grup idle/reserve/patrol yang gak lagi dikasih combat order eksplisit) --
	    // sama kayak RECON/RETREAT, ini bukan role combat, jadi disiplin penuh.
	    if (IsDirectlyThreatened())
	        m_eCombatModeActual = EAIGroupCombatMode.FIRE_AT_WILL;
	    else
	        m_eCombatModeActual = EAIGroupCombatMode.HOLD_FIRE;
	    // === END MODIFIED ===
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
	
	// === ADDED: Discipline -- Direct Threat ===
	//! True kalau grup ini BENERAN lagi diancam langsung (endangering+fresh dari
	//! cluster target) -- bukan cuma "ada musuh kedeteksi di radius sekian" kayak
	//! IsAnyTargetRelevant. Ini yang dipake buat gerbang "grup boleh nge-engage combat
	//! walau lagi ngerjain non-combat task (RECON/RETREAT/RESERVE/dll)" -- disiplin,
	//! cuma respon kalau BENERAN ditembakin/di-endanger, bukan proaktif nyari ribut.
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
			
			// === ADDED: BUG FIX -- sebelumnya PURE endangering-only. Masalahnya
			// m_iCountEndangering butuh musuh BENERAN nembak/near-miss dulu baru
			// ke-set, ada delay. Efeknya mayoritas grup (RECON/RETREAT/RESERVE/NONE --
			// itu kebanyakan populasi AI di battlefield manapun) jadi literally
			// nunggu ditembak duluan sebelum boleh balas -- keliatan "diem, gak
			// langsung nembak" padahal musuh udah kelihatan jelas di depan mata.
			// Fallback ini: kalau musuh DEKET BANGET (jauh lebih ketat dari
			// threshold role-based lama yang 40-250m), tetep izinin FIRE_AT_WILL
			// walau belum ke-endanger -- biar gak "aggro" dari jarak jauh, tapi
			// juga gak diem pas musuh udah literally di depan.
			bool isVeryClose = (c.m_State.m_fDistMin < CLOSE_VISIBLE_TRIGGER_DIST);
			if (isVeryClose)
				return true;
			// === END ADDED ===
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