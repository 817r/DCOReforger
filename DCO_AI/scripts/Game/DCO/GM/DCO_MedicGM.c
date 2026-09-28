[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_MedicCrossGroupAttribute : DCO_GlobalBoolAttribute
{
	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetMedicCrossGroup(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetMedicCrossGroup(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_MedicRadiusBleedingAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetMedicRadiusBleeding(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetMedicRadiusBleeding(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_MedicRadiusUnconsciousAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetMedicRadiusUnconscious(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetMedicRadiusUnconscious(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_MedicIncludePlayersAttribute : DCO_GlobalBoolAttribute
{
	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetMedicIncludePlayers(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetMedicIncludePlayers(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_MedicUnderFireAttribute : DCO_GlobalBoolAttribute
{
	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetMedicUnderFire(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetMedicUnderFire(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_MedicMaxLentAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetMedicMaxLentPerGroup(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetMedicMaxLentPerGroup(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_MedicTimeoutAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetMedicTimeout(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetMedicTimeout(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_OverlayModeAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetOverlayMode(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetOverlayMode(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_OverlayRadiusAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetOverlayRadius(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetOverlayRadius(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_RadioRadiusAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetRadioRadius(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetRadioRadius(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ArtyWarnRadiusAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetArtyWarnRadius(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetArtyWarnRadius(value); }
}
