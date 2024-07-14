/*!
Behavior for AI suppressing an area
*/

class DCO_AIEvasiveBehavior : SCR_AIBehaviorBase
{
	ref SCR_BTParamAssignable<vector> m_Target = new SCR_BTParamAssignable<vector>("RetreatToPoint");
	ref SCR_BTParamAssignable<vector> m_LookAt = new SCR_BTParamAssignable<vector>("RetreatFrom");
	ref SCR_BTParamRef<BaseTarget> m_RetreatFromTarget = new SCR_BTParamRef<BaseTarget>("RetreatTargetFrom");
	
	void DCO_AIEvasiveBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector retreatFromTargetPos, BaseTarget retreatFromTarget, float priorityLevel = PRIORITY_LEVEL_NORMAL)
	{
		vector tempVector;
		m_Target.Init(this, retreatFromTargetPos);
		m_LookAt.Init(this, tempVector);
		m_RetreatFromTarget.Init(this, retreatFromTarget);
		m_sBehaviorTree = "{1C46A82914F3A28E}AI/BehaviorTrees/Chimera/Soldier/Custom/Combat/DCO_Group_Tactics_Evasive.bt";
		SetPriority(PRIORITY_BEHAVIOR_RETREAT_FROM_TARGET);
		m_fPriorityLevel.m_Value = priorityLevel;
	}
};


class DCO_AIEvasiveActivity : SCR_AIActivityBase
{
	void DCO_AIEvasiveActivity(SCR_AIGroupUtilityComponent utility, AIWaypoint relatedWaypoint, float priorityLevel = PRIORITY_LEVEL_NORMAL)
	{
		SetPriority(PRIORITY_ACTIVITY_EVADE);
		
		m_sBehaviorTree = "{E62CB3068511E731}AI/BehaviorTrees/Chimera/Group/DCO_Activity_Evade.bt";
		
		m_fPriorityLevel.Init(this, priorityLevel);
	}
	
	//-------------------------------------------------------------------------------------------------
	override void OnActionDeselected()
	{
		super.OnActionDeselected();
		SendCancelMessagesToAllAgents();
	}
}