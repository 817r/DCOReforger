modded class SCR_AIMoveInFormationBehavior
{
	override float CustomEvaluate()
	{
		if (m_Utility && m_Utility.m_CombatMoveState && (m_Utility.m_CombatMoveState.DCO_DodgeHold("formation") || m_Utility.m_CombatMoveState.DCO_CoverHold()))
			return 0;
		return super.CustomEvaluate();
	}
}
