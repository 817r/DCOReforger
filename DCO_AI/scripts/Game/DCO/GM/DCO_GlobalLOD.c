[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIGlobalLODModeAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		IEntity gamemode = IEntity.Cast(item);
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

		IEntity gamemode = IEntity.Cast(item);
		if (!gamemode)
			return;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return;

		dcoAiSetting.SetLODMode(var.GetInt());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalLogModeAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		IEntity gamemode = IEntity.Cast(item);
		if (!gamemode)
			return null;
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!cfg)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(cfg.GetLogMode());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		IEntity gamemode = IEntity.Cast(item);
		if (!gamemode)
			return;
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!cfg)
			return;
		cfg.SetLogMode(var.GetInt());
		DCO_Report.Gm("DCO Logging", typename.EnumToString(DCO_ELogMode, cfg.GetLogMode()));
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalPerfProfilingAttribute : DCO_GlobalBoolAttribute
{
	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetPerfProfiling(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetPerfProfiling(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalCensusAttribute : DCO_GlobalBoolAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!DCO_TestCaseComponent.RunOn())
			return null;
		return super.ReadVariable(item, manager);
	}

	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetCensus(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetCensus(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalCensusIntervalAttribute : DCO_GlobalFloatAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!DCO_TestCaseComponent.RunOn())
			return null;
		return super.ReadVariable(item, manager);
	}

	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetCensusInterval(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetCensusInterval(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBehaviorAttribute : DCO_GlobalBoolAttribute
{
	[Attribute("1", UIWidgets.ComboBox, "Behavior controlled by this checkbox.", "", ParamEnumArray.FromEnum(DCO_EBehavior))]
	protected DCO_EBehavior m_eBehavior;

	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetBehaviorFlag(m_eBehavior); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetBehaviorFlag(m_eBehavior, value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_AMBUSH_DRILL : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_AT_SPECIALIST : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_BREAK_CONTACT : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_BUDDY_CALLOUT : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_CONTACT_SOP : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_ENEMY_WEAPONS : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_EXPLOSION_REACTIONS : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_HEARING : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_LEADER_RTO_GRENADIER : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_MARKSMAN : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_MG_GUNNER : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_PISTOL_CQB : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_REARM : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_REARM_ENEMY_SOURCES : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_RELOAD_DISCIPLINE : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_SECURITY_360 : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_SPECIALIST_RECOVERY : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_TARGET_PRIORITY : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_WINDOW_FIRE : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_PLAYER_VOICE : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_LEAN_PEEK : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_SUPPRESS_LAST_KNOWN : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_ASSAULT_FIRE : DCO_GlobalBehaviorAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_REACTION_TIME : DCO_GlobalBehaviorAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_PRONE_MOVE : DCO_GlobalBehaviorAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_PINNED : DCO_GlobalBehaviorAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_AWARENESS : DCO_GlobalBehaviorAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBeh_COMBINED_ARMS : DCO_GlobalBehaviorAttribute {}

