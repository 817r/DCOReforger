modded class SCR_AIAttackBehavior : SCR_AIBehaviorBase
{
    protected float m_fAttackBehaviorPriority;

    // protected static const float WAIT_TIME_OVERTHREATENED = 0.0; // 0.8;

    //------------------------------------------------------------------------------------------------
    override float CustomEvaluate()
    {
        m_fAttackBehaviorPriority = 0;

        BaseTarget baseTarget = m_Target.m_Value;

        if (baseTarget == m_CombatComponent.GetCurrentTarget())
        {
            float targetScore = m_Utility.m_CombatComponent.m_WeaponTargetSelector.CalculateTargetScore(baseTarget);

            if (baseTarget.IsEndangering() || targetScore >= SCR_AICombatComponent.TARGET_SCORE_HIGH_PRIORITY_ATTACK)
                m_fAttackBehaviorPriority = PRIORITY_BEHAVIOR_ATTACK_HIGH_PRIORITY;
            else if (m_bSelected)
                m_fAttackBehaviorPriority = PRIORITY_BEHAVIOR_ATTACK_SELECTED;
            else
                m_fAttackBehaviorPriority = PRIORITY_BEHAVIOR_ATTACK_NOT_SELECTED;

            return m_fAttackBehaviorPriority;
        }

        return m_fAttackBehaviorPriority;
    }
}