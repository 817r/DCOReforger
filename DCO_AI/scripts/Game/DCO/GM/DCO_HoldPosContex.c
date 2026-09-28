[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class DCO_HoldPositionContextAction : SCR_SelectedEntitiesContextAction
{
	protected bool GetTargetHold()
	{
		return true;
	}

	override int GetParam()
	{
		return GetGame().GetPlayerController().GetPlayerId();
	}

	override bool CanBeShown(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		if (!selectedEntity)
			return false;

		IEntity owner = selectedEntity.GetOwner();
		if (!owner)
			return false;

		SCR_AIGroup group = SCR_AIGroup.Cast(owner);
		if (group)
		{
			if (group.IsPlayable())
				return false;
		}
		else if (!owner.FindComponent(SCR_AICombatComponent) || EntityUtils.IsPlayer(owner))
		{
			return false;
		}

		return selectedEntity.DCO_IsHoldPosition() != GetTargetHold();
	}

	override bool CanBePerformed(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return CanBeShown(selectedEntity, cursorWorldPosition, flags);
	}

	override void Perform(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{
		if (!InitPerform())
			return;

		bool hold = GetTargetHold();
		foreach (SCR_EditableEntityComponent entity : selectedEntities)
		{
			if (!entity || !entity.GetOwner())
				continue;

			AIGroup group = AIGroup.Cast(entity.GetOwner());
			if (group)
			{
				array<AIAgent> agents = {};
				group.GetAgents(agents);
				foreach (AIAgent agent : agents)
					SetHold(agent.GetControlledEntity(), hold);

				entity.DCO_SetHoldPosition(hold);
			}
			else
			{
				SetHold(entity.GetOwner(), hold);
			}
		}
	}

	protected void SetHold(IEntity character, bool hold)
	{
		DCO_AIConfigComponent conf = GetDCOConfig(character);
		if (conf)
			conf.SetHoldPosition(hold);
	}

	protected DCO_AIConfigComponent GetDCOConfig(IEntity owner)
	{
		if (!owner)
			return null;

		SCR_AICombatComponent combatComp = SCR_AICombatComponent.Cast(owner.FindComponent(SCR_AICombatComponent));
		if (!combatComp)
			return null;

		SCR_AIUtilityComponent util = combatComp.GetUtilityComponent();
		if (!util)
			return null;

		return util.m_DCOConfig;
	}
};

[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class DCO_UnholdPositionContextAction : DCO_HoldPositionContextAction
{
	override protected bool GetTargetHold()
	{
		return false;
	}
};
