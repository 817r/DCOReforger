[BaseContainerProps(), SCR_BaseGroupCommandTitleField("m_sCommandName")]
modded class SCR_SetCombatModeGroupCommand
{
	override bool Execute(IEntity cursorTarget, IEntity groupEnt, vector targetPosition, int playerID, bool isClient)
	{
		if (!isClient)
		{
			SCR_AIGroup grp = SCR_AIGroup.Cast(groupEnt);
			SCR_AIGroupUtilityComponent u;
			if (grp)
				u = SCR_AIGroupUtilityComponent.Cast(grp.FindComponent(SCR_AIGroupUtilityComponent));
			if (u && u.DCO_IsCommanderEval())
				u.DCO_SetCommanderEval(false);
		}
		return super.Execute(cursorTarget, groupEnt, targetPosition, playerID, isClient);
	}
}

[BaseContainerProps(), SCR_BaseGroupCommandTitleField("m_sCommandName")]
class DCO_CommanderEvalGroupCommand : SCR_SetCombatModeGroupCommand
{
	override bool Execute(IEntity cursorTarget, IEntity groupEnt, vector targetPosition, int playerID, bool isClient)
	{
		if (isClient)
			return true;
		SCR_AIGroup grp = SCR_AIGroup.Cast(groupEnt);
		if (!grp)
			return false;
		SCR_AIGroupUtilityComponent u = SCR_AIGroupUtilityComponent.Cast(grp.FindComponent(SCR_AIGroupUtilityComponent));
		if (!u)
			return false;
		u.DCO_SetCommanderEval(true);
		return true;
	}
}
