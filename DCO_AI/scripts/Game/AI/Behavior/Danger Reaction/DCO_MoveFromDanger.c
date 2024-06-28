class SCR_AIMoveFromSuppressBehavior : SCR_AIMoveFromDangerBehavior
{
	float timeOut = 3000;
	float m_fBehaviorTimeout;
	
	void SCR_AIMoveFromSuppressBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector dangerPos, IEntity dangerEntity)
	{
		m_fBehaviorTimeout = GetGame().GetWorld().GetWorldTime() + timeOut;
		SetPriority(PRIORITY_BEHAVIOR_MOVE_FROM_DANGER);
		m_Stance.m_Value = ECharacterStance.STAND;
		m_MovementType.m_Value = EMovementType.SPRINT;
		m_bIsInterruptable = false;
		m_bAllowLook = false;

		m_sBehaviorTree = "{6F9819BF7D1A5A72}AI/BehaviorTrees/Chimera/Soldier/Custom/DCO_MoveFromSuppressiveFire.bt";
	}
	
	override float CustomEvaluate()
	{			
		if (GetGame().GetWorld().GetWorldTime() > m_fBehaviorTimeout)
			return PRIORITY_BEHAVIOR_MOVE_FROM_DANGER;
		
		return GetPriority();
	}
	
	override void OnActionSelected()
	{
		super.OnActionSelected();
		bool m_bSelected = true;
	}
}