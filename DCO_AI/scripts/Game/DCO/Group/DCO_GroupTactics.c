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
		if (!rq)
			return;

		if (posture == DCO_GroupTactics.SNEAK)
		{
			rq.m_eMovementType = EMovementType.WALK;
			rq.m_eStanceMoving = ECharacterStance.CROUCH;
			return;
		}

		if (posture != DCO_GroupTactics.AGGRESIVE)
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
		if (morale >= moraleState.SHAKEN && threat >= EAIThreatState.ALERTED)
			return;

		switch (posture)
		{
			case DCO_GroupTactics.AGGRESIVE:
			{
				dir = SCR_EAICombatMoveDirection.CUSTOM_POS;
				break;
			}
			case DCO_GroupTactics.DEFENSIVE:
			case DCO_GroupTactics.DELAY:
			{
				dir = Lateral();
				break;
			}
			case DCO_GroupTactics.SNEAK:
			case DCO_GroupTactics.AMBUSH:
			{
				dir = SCR_EAICombatMoveDirection.ANYWHERE;
				break;
			}
			case DCO_GroupTactics.EVASIVE:
			{
				dir = SCR_EAICombatMoveDirection.BACKWARD;
				break;
			}
		}
	}

	static void ApplyAreaDirection(DCO_GroupTactics posture, inout SCR_EAICombatMoveDirection dir)
	{
		switch (posture)
		{
			case DCO_GroupTactics.DEFENSIVE:
			case DCO_GroupTactics.DELAY:
			{
				dir = Lateral();
				break;
			}
			case DCO_GroupTactics.SNEAK:
			case DCO_GroupTactics.AMBUSH:
			case DCO_GroupTactics.EVASIVE:
			{
				dir = SCR_EAICombatMoveDirection.ANYWHERE;
				break;
			}
		}
	}

	static SCR_EAICombatMoveDirection Lateral()
	{
		if (Math.RandomIntInclusive(0, 1) == 1)
			return SCR_EAICombatMoveDirection.LEFT;
		return SCR_EAICombatMoveDirection.RIGHT;
	}

	static bool IsHoldPosture(DCO_GroupTactics posture)
	{
		return posture == DCO_GroupTactics.SNEAK || posture == DCO_GroupTactics.AMBUSH || posture == DCO_GroupTactics.DELAY || posture == DCO_GroupTactics.EVASIVE;
	}

	static float BoundPeriod_ms(DCO_GroupTactics posture)
	{
		if (posture == DCO_GroupTactics.AGGRESIVE)
			return 6000.0;
		if (posture == DCO_GroupTactics.BALANCE)
			return 12000.0;
		return 0;
	}
}
