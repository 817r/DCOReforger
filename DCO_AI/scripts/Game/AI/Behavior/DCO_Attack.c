modded class SCR_AIAttackBehavior : SCR_AIBehaviorBase
{

	protected static const float WAIT_TIME_UNEXPECTED = 0.1;
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
};