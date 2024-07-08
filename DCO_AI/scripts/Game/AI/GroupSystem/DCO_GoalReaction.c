modded enum EMessageType_Goal
{
	EVADE_TACTICS
};

[BaseContainerProps()]
class DCO_AIEvasive : SCR_AIGoalReaction
{
	override void PerformReaction(notnull SCR_AIUtilityComponent utility, SCR_AIMessageBase message)
	{
		DCO_AIEvadeFrom msg = DCO_AIEvadeFrom.Cast(message);
		if (!msg)
			return;
		
		DCO_AIEvasiveBehavior behavior = new DCO_AIEvasiveBehavior(utility, msg.m_RelatedGroupActivity, msg.m_vTargetPosition, msg.m_bTarget, msg.m_fPriorityLevel);
		utility.AddAction(behavior);
	}
};

class DCO_AIEvadeFrom : SCR_AIMessageGoal
{
	vector m_vTargetPosition;
	BaseTarget m_bTarget;
	
	void DCO_AIEvadeFrom()
	{
		m_MessageType = EMessageType_Goal.EVADE_TACTICS;
	}

	static DCO_AIEvadeFrom Create(vector position, BaseTarget target)
	{
		DCO_AIEvadeFrom msg = new DCO_AIEvadeFrom();
		msg.m_vTargetPosition = position;
		msg.m_bTarget = target;
		return msg;
	}
};