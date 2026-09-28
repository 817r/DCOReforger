[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_EngagementDistanceBaseAttribute : SCR_BaseValueListEditorAttribute
{
	protected DCO_GroupConfigComponent GetGroupConfig(Managed item)
	{
		SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(item);
		if (!editableGroup || editableGroup.HasEntityState(EEditableEntityState.PLAYER))
			return null;

		SCR_AIGroup group = editableGroup.GetAIGroupComponent();
		if (!group)
			return null;

		return DCO_GroupConfigComponent.Cast(group.FindComponent(DCO_GroupConfigComponent));
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_EngagementDistanceInfantryAttribute : DCO_EngagementDistanceBaseAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GroupConfigComponent cfg = GetGroupConfig(item);
		if (!cfg)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(cfg.GetEngagementDistanceInfantry());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GroupConfigComponent cfg = GetGroupConfig(item);
		if (var && cfg)
			cfg.SetEngagementDistance(var.GetFloat(), cfg.GetEngagementDistanceVehicle());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_EngagementDistanceVehicleAttribute : DCO_EngagementDistanceBaseAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GroupConfigComponent cfg = GetGroupConfig(item);
		if (!cfg)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(cfg.GetEngagementDistanceVehicle());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GroupConfigComponent cfg = GetGroupConfig(item);
		if (var && cfg)
			cfg.SetEngagementDistance(cfg.GetEngagementDistanceInfantry(), var.GetFloat());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SquadInvestigateMaxDistAttribute : DCO_EngagementDistanceBaseAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GroupConfigComponent cfg = GetGroupConfig(item);
		if (!cfg)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(cfg.GetInvestigateMaxDist());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GroupConfigComponent cfg = GetGroupConfig(item);
		if (var && cfg)
			cfg.SetInvestigateMaxDist(var.GetFloat());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SquadInvestigateChanceAttribute : DCO_EngagementDistanceBaseAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GroupConfigComponent cfg = GetGroupConfig(item);
		if (!cfg)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(cfg.GetInvestigateChance());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GroupConfigComponent cfg = GetGroupConfig(item);
		if (var && cfg)
			cfg.SetInvestigateChance(var.GetFloat());
	}
}
