[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ShareEnabledAttribute : DCO_GlobalBoolAttribute
{
	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetShareEnabled(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetShareEnabled(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ShareVoiceRangeAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetShareVoiceRange(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetShareVoiceRange(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ShareRadioRangeAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetShareRadioRange(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetShareRadioRange(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ShareNoiseAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetShareNoisePer100m(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetShareNoisePer100m(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ShareVoiceNoiseAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetShareVoiceNoise(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetShareVoiceNoise(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ShareRadioNoiseAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetShareRadioNoise(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetShareRadioNoise(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ShareRequireRadioAttribute : DCO_GlobalBoolAttribute
{
	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetShareRequireRadio(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetShareRequireRadio(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ShareHopsAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetShareHops(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetShareHops(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ShareMapMarkersAttribute : DCO_GlobalBoolAttribute
{
	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetShareMapMarkers(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetShareMapMarkers(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ShareToPlayersAttribute : DCO_GlobalBoolAttribute
{
	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetShareToPlayers(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetShareToPlayers(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ShareDebugAttribute : DCO_GlobalBoolAttribute
{
	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetShareDebug(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetShareDebug(value); }
}
