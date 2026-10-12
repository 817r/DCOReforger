[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AISquadVehicleDismountAttribute : SCR_ValidTypeBaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_EditableGroupComponent editableGroupComponent = SCR_EditableGroupComponent.Cast(item);
		if (!editableGroupComponent)
			return null;

		if (editableGroupComponent.HasEntityState(EEditableEntityState.PLAYER))
			return null;

		if (!IsValidEntityType(editableGroupComponent.GetEntityType()))
			return null;

		SCR_AIGroup owner = editableGroupComponent.GetAIGroupComponent();
		if (!owner)
			return null;

		array<AIAgent> agents = {};
		owner.GetAgents(agents);

		DCO_AIConfigComponent aiConf = DCO_AIConfigComponent.Cast(owner.GetLeaderAgent().FindComponent(DCO_AIConfigComponent));

		if (!aiConf)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(aiConf.GetDismountDistance());
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		SCR_EditableGroupComponent editableGroupComponent = SCR_EditableGroupComponent.Cast(item);
		if (!editableGroupComponent)
			return;

		if (editableGroupComponent.HasEntityState(EEditableEntityState.PLAYER))
			return;

		if (!IsValidEntityType(editableGroupComponent.GetEntityType()))
			return;

		SCR_AIGroup owner = editableGroupComponent.GetAIGroupComponent();
		if (!owner)
			return;

		array<AIAgent> agents = {};
		owner.GetAgents(agents);

		foreach(AIAgent a : agents)
		{
			DCO_AIConfigComponent aiConf = DCO_AIConfigComponent.Cast(a.FindComponent(DCO_AIConfigComponent));

			if (!aiConf)
				return;

			aiConf.SetDismountDistance(var.GetFloat());
		}
	}
}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AISquadDisembarkRuleAttribute : DCO_CommanderBaseAttribute
{
	override protected void GetPresetLabels(notnull out array<string> outLabels)
	{
		outLabels.Clear();
		outLabels.Insert("Auto");
		outLabels.Insert("Only when disabled");
		outLabels.Insert("Only on orders");
		outLabels.Insert("Never");
	}

	protected static SCR_AIGroup Group(Managed item)
	{
		SCR_EditableGroupComponent editable = SCR_EditableGroupComponent.Cast(item);
		if (!editable || editable.HasEntityState(EEditableEntityState.PLAYER))
			return null;
		return editable.GetAIGroupComponent();
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_AIGroup owner = Group(item);
		if (!owner || !owner.GetLeaderAgent())
			return null;

		DCO_AIConfigComponent aiConf = DCO_AIConfigComponent.Cast(owner.GetLeaderAgent().FindComponent(DCO_AIConfigComponent));
		if (!aiConf)
			return null;

		return SCR_BaseEditorAttributeVar.CreateInt(aiConf.GetDisembarkRule());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		SCR_AIGroup owner = Group(item);
		if (!owner || !var)
			return;

		array<AIAgent> agents = {};
		owner.GetAgents(agents);
		int n;
		foreach (AIAgent a : agents)
		{
			DCO_AIConfigComponent aiConf = DCO_AIConfigComponent.Cast(a.FindComponent(DCO_AIConfigComponent));
			if (!aiConf)
				continue;
			aiConf.SetDisembarkRule(var.GetInt());
			n++;
		}
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(owner), string.Format("disembark_rule_set scope=squad grp=%1 rule=%2 agents=%3 player=%4", DCO_Log.IdGroup(owner), typename.EnumToString(DCO_EDisembarkRule, Math.ClampInt(var.GetInt(), 0, 3)), n, playerID));
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SquadVehicleExitAttribute : SCR_BaseMultiSelectPresetsEditorAttribute
{
	protected static DCO_GroupUtilityComponent GroupUtil(Managed item, out SCR_AIGroup grp)
	{
		SCR_EditableGroupComponent editable = SCR_EditableGroupComponent.Cast(item);
		if (!editable || editable.HasEntityState(EEditableEntityState.PLAYER))
			return null;
		grp = editable.GetAIGroupComponent();
		if (!grp)
			return null;
		return DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_AIGroup grp;
		DCO_GroupUtilityComponent gc = GroupUtil(item, grp);
		if (!gc)
			return null;
		int own = gc.DCO_VehicleExit();
		if (own >= 0)
			return SCR_BaseEditorAttributeVar.CreateInt(own << 1);
		int global = 15;
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetFor(grp);
		if (g)
			global = g.GetVehicleExit();
		return SCR_BaseEditorAttributeVar.CreateInt(1 | (global << 1));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		SCR_AIGroup grp;
		DCO_GroupUtilityComponent gc = GroupUtil(item, grp);
		if (!gc || !var)
			return;
		int gui = var.GetInt();
		if ((gui & 1) != 0)
			gc.DCO_SetVehicleExit(-1);
		else
			gc.DCO_SetVehicleExit((gui >> 1) & 31);
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("vehicle_exit_set scope=squad grp=%1 flags=%2 player=%3", DCO_Log.IdGroup(grp), gc.DCO_VehicleExit(), playerID));
	}
}
