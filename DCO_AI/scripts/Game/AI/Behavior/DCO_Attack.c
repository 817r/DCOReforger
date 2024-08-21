modded class SCR_AIAttackBehavior : SCR_AIBehaviorBase
{

	protected static const float WAIT_TIME_UNEXPECTED = 0.2;
	protected static const float WAIT_TIME_OVERTHREATENED = 0.4;
	
	void SCR_AIAttackBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, BaseTarget target, BaseTarget prevTarget, float priorityLevel = PRIORITY_LEVEL_NORMAL)
	{
		InitParameters(target, 0.5);
		if (!utility)
			return;
		
		m_fPriorityLevel.m_Value = priorityLevel;
		SetPriority(PRIORITY_BEHAVIOR_ATTACK_NOT_SELECTED);
		m_fThreat = 1.01 * SCR_AIThreatSystem.VIGILANT_THRESHOLD;
		m_sBehaviorTree = "AI/BehaviorTrees/Chimera/Soldier/Attack.bt";
		m_bAllowLook = false;
		m_bResetLook = true;
		SetIsUniqueInActionQueue(false);
		m_AIWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		m_CombatComponent = utility.m_CombatComponent;

		if (!prevTarget)
			InitWaitTime(utility);
		else
			m_fWaitTime.m_Value = 0;
	}
	
	override float CustomEvaluate()
	{
		BaseTarget baseTarget = m_Target.m_Value;
		BaseTarget targetNow;
		
		// Return 0 if it's not current target selected by weapon&target selector
		// Since weapon&target selector selects both weapon and target, if we chose to attack a different target,
		// it might happen that we use a wrong weapon
		if (baseTarget != m_CombatComponent.GetCurrentTarget())
			return 0;
		
		// Update m_bUseCombatMove
		m_bUseCombatMove = true;
		
		if (m_Utility.m_AIInfo.HasUnitState(EUnitState.IN_TURRET))
			m_bUseCombatMove = false;
		else if (m_Utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE)) // inside vehicle we dont attack unless in turret
			return 0;
		
		targetNow = m_Utility.m_PerceptionComponent.GetClosestTarget(ETargetCategory.ENEMY, 2.0, 5.0);
		if (targetNow != null)
		{
			float targetNowDistance = targetNow.GetDistance();
			bool cqb = targetNowDistance < SCR_AICombatComponent.CLOSE_RANGE_COMBAT_DISTANCE;
			m_bCloseRange = cqb;
			m_CombatComponent.SetTargetSelectionProperties(m_bCloseRange);
			if (targetNowDistance <= baseTarget.GetDistance())
				return PRIORITY_BEHAVIOR_ATTACK_HIGH_PRIORITY;
		}
		
		bool closeRange = baseTarget.GetDistance() < SCR_AICombatComponent.CLOSE_RANGE_COMBAT_DISTANCE;
		if (closeRange != m_bCloseRange)
		{
			m_bCloseRange = closeRange;
			m_CombatComponent.SetTargetSelectionProperties(m_bCloseRange);
		}
		
		float targetScore = m_Utility.m_CombatComponent.m_WeaponTargetSelector.CalculateTargetScore(baseTarget);
				
		if (baseTarget.IsEndangering() || baseTarget.GetTimeSinceEndangered() < SCR_AICombatComponent.TARGET_ENDANGERED_TIMEOUT_S)
			targetScore *= SCR_AICombatComponent.ENDANGERING_TARGET_SCORE_MULTIPLIER;
		
		if (targetScore >= SCR_AICombatComponent.TARGET_SCORE_HIGH_PRIORITY_ATTACK)
			return PRIORITY_BEHAVIOR_ATTACK_HIGH_PRIORITY;
			
		if (m_bSelected)
			return PRIORITY_BEHAVIOR_ATTACK_SELECTED;		
		
		return PRIORITY_BEHAVIOR_ATTACK_NOT_SELECTED;
	}
	
	override protected void InitWaitTime(SCR_AIUtilityComponent utility)
	{
		float threatMeasure = utility.m_ThreatSystem.GetThreatMeasure();
		
		// Delay depending on threat
		float threatDelay;
		if (threatMeasure < SCR_AIThreatSystem.ATTACK_DELAYED_THRESHOLD)
			threatDelay = WAIT_TIME_UNEXPECTED;
		else if (threatMeasure < SCR_AIThreatSystem.THREATENED_THRESHOLD)
			threatDelay = 0;
		else if (threatMeasure < SCR_AIThreatSystem.EXHAUSTED_THRESHOLD)
			threatDelay = 0.2 + WAIT_TIME_OVERTHREATENED;
		else
			threatDelay = WAIT_TIME_OVERTHREATENED;
		
		// Delay depending on distance
		// 0m - 0ms
		// 100m - 340ms
		// 300m - 700ms
		// 500m - 870ms
		// 800m - 1018ms
		float distance = vector.Distance(m_Utility.m_OwnerEntity.GetOrigin(), m_Target.m_Value.GetLastSeenPosition());
		float distanceDelay = (1.4 * distance) / (30 + distance);
		
		m_fWaitTime.m_Value = threatDelay + distanceDelay;
	}
};