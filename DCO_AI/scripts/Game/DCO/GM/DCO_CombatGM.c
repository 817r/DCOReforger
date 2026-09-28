[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CombatSuperiorRatioAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetSuperiorRatio(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetSuperiorRatio(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CombatLongRangeHoldAttribute : DCO_GlobalBoolAttribute
{
	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetLongRangeHold(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetLongRangeHold(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CombatRifleRangeAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetRifleEffectiveRange(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetRifleEffectiveRange(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CombatMGRangeAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetMGEffectiveRange(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetMGEffectiveRange(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CombatShareBuildingsAttribute : DCO_GlobalBoolAttribute
{
	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetShareBuildings(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetShareBuildings(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CombatCoverPreferenceAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalFloatAttribute.GetGlobal(item);
		if (!cfg)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(cfg.GetCoverPreference());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalFloatAttribute.GetGlobal(item);
		if (cfg && var)
			cfg.SetCoverPreference(var.GetInt());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CombatOvermatchAttribute : DCO_GlobalBoolAttribute
{
	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetOvermatchAssault(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetOvermatchAssault(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CombatOvermatchDistAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetOvermatchMaxDist(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetOvermatchMaxDist(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CQBEnabledAttribute : DCO_GlobalBoolAttribute
{
	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetCQBEnabled(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetCQBEnabled(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CQBIsolateMinAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetCQBIsolateMin(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetCQBIsolateMin(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CQBStackWaitAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetCQBStackWait(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetCQBStackWait(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CQBAbortLossesAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetCQBAbortLosses(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetCQBAbortLosses(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CQBCaptureWaitsAttribute : DCO_GlobalBoolAttribute
{
	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetCQBCaptureWaits(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetCQBCaptureWaits(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CQBContactTimeoutAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetCQBContactTimeout(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetCQBContactTimeout(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_NightEnabledAttribute : DCO_GlobalBoolAttribute
{
	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetNightEnabled(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetNightEnabled(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_NightFlareCooldownAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetNightFlareCooldown(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetNightFlareCooldown(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_NightEngageMulAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetNightEngageMul(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetNightEngageMul(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_NightLightDisciplineAttribute : DCO_GlobalBoolAttribute
{
	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetNightLightDiscipline(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetNightLightDiscipline(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ContactReportMaxPerScanAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetContactReportMaxPerScan(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetContactReportMaxPerScan(Math.Round(value)); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ContactReportScanIntervalAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetContactReportScanInterval(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetContactReportScanInterval(value); }
}
