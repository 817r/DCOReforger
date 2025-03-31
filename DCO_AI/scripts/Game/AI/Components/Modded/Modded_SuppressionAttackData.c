modded class SCR_AIUpdateTargetSuppressionData : AITaskScripted
{
	protected const float VISIBILITY_CHECK_TRACE_RESULT_THRESHOLD = 0.2;
	
	protected const int FIRE_TREE_RPG				= 6;
	
	//---------------------------------------------------------------------------
	override int ResolveFireTree(bool targetVisible)
	{
		// Is aiming forbidden by combat move?
		SCR_AIBehaviorBase executedBehavior = SCR_AIBehaviorBase.Cast(m_UtilityComponent.GetExecutedAction());
		if (executedBehavior && executedBehavior.m_bUseCombatMove && !m_UtilityComponent.m_CombatMoveState.m_bAimAtTarget)
			return FIRE_TREE_INVALID;
		
		// Friendly in aim?
		if (m_PerceptionComponent.GetFriendlyInLineOfFire())
			return FIRE_TREE_LOOK;
				
		if (targetVisible)
			return FIRE_TREE_SUPPRESSIVE;
		
		return FIRE_TREE_LOOK;
	}
}