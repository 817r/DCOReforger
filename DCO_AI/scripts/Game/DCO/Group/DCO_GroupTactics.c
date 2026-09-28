class SCR_AIGroupTacticsComponentClass : ScriptComponentClass
{
}

class SCR_AIGroupTacticsComponent : ScriptComponent
{
	[Attribute("0", UIWidgets.ComboBox, "AI Tactics in combat", "", ParamEnumArray.FromEnum(DCO_GroupTactics) )]
	protected DCO_GroupTactics m_eAITacticsDefault;

	protected SCR_AIGroup m_Group;
	protected SCR_AIGroupPerception m_GroupPerception;

	override protected void OnPostInit(IEntity owner)
	{
		RplComponent rplComponent = RplComponent.Cast(owner.FindComponent(RplComponent));

		m_Group = SCR_AIGroup.Cast(owner);

		if (!m_Group)
			return;

		if (!rplComponent || !rplComponent.IsMaster())
			return;
	}

	DCO_GroupTactics GetGroupTactics()
	{
		return m_eAITacticsDefault;
	}
}

class DCO_PostureCombatUtility
{
	static DCO_GroupTactics GetPosture(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.GetAIAgent())
			return DCO_GroupTactics.BALANCE;

		AIGroup group = utility.GetAIAgent().GetParentGroup();
		if (!group)
			return DCO_GroupTactics.BALANCE;

		SCR_AIGroupUtilityComponent groupUtil = SCR_AIGroupUtilityComponent.Cast(group.FindComponent(SCR_AIGroupUtilityComponent));
		if (!groupUtil)
			return DCO_GroupTactics.BALANCE;

		return groupUtil.DCO_GetPosture();
	}

	static float GetStoppedWaitTimeScale(DCO_GroupTactics posture)
	{
		switch (posture)
		{
			case DCO_GroupTactics.DEFENSIVE: return 1.5;
			case DCO_GroupTactics.AGGRESIVE: return 0.6;
			case DCO_GroupTactics.EVASIVE:   return 1.2;
		}

		return 1.0;
	}

	static float GetRepositionLockScale(DCO_GroupTactics posture)
	{
		if (posture == DCO_GroupTactics.AGGRESIVE)
			return 0.5;

		return 1.0;
	}

	static float GetAdvanceSuppressionCutoffScale(DCO_GroupTactics posture)
	{
		if (posture == DCO_GroupTactics.AGGRESIVE)
			return 1.3;

		return 1.0;
	}

	static void ApplyMovePace(DCO_GroupTactics posture, EAIThreatState threat, SCR_AICombatMoveRequest_Move rq, bool allowStand)
	{
		if (posture != DCO_GroupTactics.AGGRESIVE || !rq)
			return;

		if (rq.m_eMovementType == EMovementType.WALK)
			rq.m_eMovementType = EMovementType.RUN;

		if (allowStand && threat != EAIThreatState.THREATENED && rq.m_eStanceMoving == ECharacterStance.CROUCH)
			rq.m_eStanceMoving = ECharacterStance.STAND;
	}

	static float GetMoveFromTargetDist(DCO_GroupTactics posture)
	{
		switch (posture)
		{
			case DCO_GroupTactics.DEFENSIVE: return 20.0;
			case DCO_GroupTactics.EVASIVE:   return 55.0;
		}

		return 0;
	}

	static void ApplyMoveDirection(DCO_GroupTactics posture, EAIThreatState threat, moraleState morale, inout SCR_EAICombatMoveDirection dir)
	{
		if (morale == moraleState.BREAK)
			return;

		switch (posture)
		{
			case DCO_GroupTactics.AGGRESIVE:
			{
				dir = SCR_EAICombatMoveDirection.CUSTOM_POS;
				break;
			}
			case DCO_GroupTactics.DEFENSIVE:
			{
				if (threat == EAIThreatState.THREATENED)
					dir = SCR_EAICombatMoveDirection.BACKWARD;
				break;
			}
			case DCO_GroupTactics.EVASIVE:
			{
				dir = SCR_EAICombatMoveDirection.BACKWARD;
				break;
			}
		}
	}
}
