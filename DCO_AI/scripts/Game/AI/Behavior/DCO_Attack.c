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
		m_bUseCombatMove = !m_Utility.m_AIInfo.HasUnitState(EUnitState.IN_TURRET);
		
		/* targetNow = m_Utility.m_PerceptionComponent.GetClosestTarget(ETargetCategory.ENEMY, 2.0, 5.0);
		if (targetNow != null && baseTarget == null)
		{
			float targetNowDistance = targetNow.GetDistance();
			if (targetNowDistance <= 10)
				return PRIORITY_BEHAVIOR_ATTACK_HIGH_PRIORITY;
		}*/
		
		float targetScore = m_Utility.m_CombatComponent.m_WeaponTargetSelector.CalculateTargetScore(baseTarget);

				
		if (baseTarget.IsEndangering() || baseTarget.GetTimeSinceEndangered() < SCR_AICombatComponent.TARGET_ENDANGERED_TIMEOUT_S)
			targetScore *= SCR_AICombatComponent.ENDANGERING_TARGET_SCORE_MULTIPLIER;
		
		if (targetScore >= SCR_AICombatComponent.TARGET_SCORE_HIGH_PRIORITY_ATTACK)
			return PRIORITY_BEHAVIOR_ATTACK_HIGH_PRIORITY;
			
		if (m_bSelected)
			return PRIORITY_BEHAVIOR_ATTACK_SELECTED;		
		
		return PRIORITY_BEHAVIOR_ATTACK_NOT_SELECTED;
	}
};