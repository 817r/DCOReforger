[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIGlobalLODModeAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return null;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return null;

		return SCR_BaseEditorAttributeVar.CreateInt(dcoAiSetting.GetLODMode());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return;

		dcoAiSetting.SetLODMode(var.GetInt());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalPerfProfilingAttribute : DCO_GlobalBoolAttribute
{
	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetPerfProfiling(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetPerfProfiling(value); }
}
