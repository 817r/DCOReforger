[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalFloatAttribute : SCR_BaseValueListEditorAttribute
{
	protected float Get(DCO_GlobalAIComponent cfg) { return 0; }
	protected void Set(DCO_GlobalAIComponent cfg, float value) {}

	static DCO_GlobalAIComponent GetGlobal(Managed item)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return null;

		return DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GlobalAIComponent cfg = GetGlobal(item);
		if (!cfg)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(Get(cfg));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GlobalAIComponent cfg = GetGlobal(item);
		if (cfg && var)
			Set(cfg, var.GetFloat());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalBoolAttribute : SCR_BaseEditorAttribute
{
	protected bool Get(DCO_GlobalAIComponent cfg) { return false; }
	protected void Set(DCO_GlobalAIComponent cfg, bool value) {}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalFloatAttribute.GetGlobal(item);
		if (!cfg)
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(Get(cfg));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalFloatAttribute.GetGlobal(item);
		if (cfg && var)
			Set(cfg, var.GetBool());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_IdleEnterTimeAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetIdleEnterTime(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetIdleEnterTime(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_IdleCoverSearchDistAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetIdleCoverSearchDist(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetIdleCoverSearchDist(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_IdleOverrunDistAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetIdleOverrunDist(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetIdleOverrunDist(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_IdleLeaveToAssistAttribute : DCO_GlobalBoolAttribute
{
	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetIdleLeaveToAssist(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetIdleLeaveToAssist(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_IdleLeaveToInvestigateAttribute : DCO_GlobalBoolAttribute
{
	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetIdleLeaveToInvestigate(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetIdleLeaveToInvestigate(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_IdleFollowCommanderAttribute : DCO_GlobalBoolAttribute
{
	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetIdleFollowCommander(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.SetIdleFollowCommander(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_InvestigateMaxDistAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetInvestigateMaxDist(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetInvestigateMaxDist(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_InvestigateChanceAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetInvestigateChance(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetInvestigateChance(value); }
}
