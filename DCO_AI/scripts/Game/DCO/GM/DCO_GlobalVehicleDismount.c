[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIGlobalVehicleDismountAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		IEntity gamemode = IEntity.Cast(item);
		if (!gamemode)
			return null;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return null;

		float index = dcoAiSetting.GetDismountDistance();

		return SCR_BaseEditorAttributeVar.CreateFloat(index);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		IEntity gamemode = IEntity.Cast(item);
		if (!gamemode)
			return;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return;

		float index = var.GetFloat();

		dcoAiSetting.SetDismountDistance(index);
	}
}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIGlobalDisembarkRuleAttribute : DCO_CommanderBaseAttribute
{
	override protected void GetPresetLabels(notnull out array<string> outLabels)
	{
		outLabels.Clear();
		outLabels.Insert("Auto");
		outLabels.Insert("Only when disabled");
		outLabels.Insert("Only on orders");
		outLabels.Insert("Never");
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		IEntity gamemode = IEntity.Cast(item);
		if (!gamemode)
			return null;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return null;

		return SCR_BaseEditorAttributeVar.CreateInt(dcoAiSetting.GetDisembarkRule());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		IEntity gamemode = IEntity.Cast(item);
		if (!gamemode || !var)
			return;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return;

		dcoAiSetting.SetDisembarkRule(var.GetInt());
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("disembark_rule_set scope=global rule=%1 player=%2", typename.EnumToString(DCO_EDisembarkRule, dcoAiSetting.GetDisembarkRule()), playerID));
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalVehicleExitAttribute : SCR_BaseMultiSelectPresetsEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalFloatAttribute.GetGlobal(item);
		if (!cfg)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(cfg.GetVehicleExit());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalFloatAttribute.GetGlobal(item);
		if (!cfg || !var)
			return;
		cfg.SetVehicleExit(var.GetInt());
		DCO_Report.Gm("Vehicle exit (global)", cfg.GetVehicleExit().ToString());
	}
}
