[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderSupportBaseAttribute : SCR_BaseValueListEditorAttribute
{
	protected bool Read(AICommander_BaseComponent cmd, out float value)
	{
		return false;
	}

	protected void Write(AICommander_BaseComponent cmd, float value)
	{
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		float value;
		if (!cmd || !Read(cmd, value))
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(value);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (cmd)
			Write(cmd, var.GetFloat());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderThreatBaseAttribute : DCO_CommanderSupportBaseAttribute
{
	protected float ReadThreat(CMD_ThreatResponseComponent t) { return 0; }
	protected void WriteThreat(CMD_ThreatResponseComponent t, float v) {}

	override protected bool Read(AICommander_BaseComponent cmd, out float value)
	{
		CMD_ThreatResponseComponent t = cmd.GetThreatResponseComponent();
		if (!t)
			return false;

		value = ReadThreat(t);
		return true;
	}

	override protected void Write(AICommander_BaseComponent cmd, float value)
	{
		CMD_ThreatResponseComponent t = cmd.GetThreatResponseComponent();
		if (t)
			WriteThreat(t, value);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ThreatEngageThresholdAttribute : DCO_CommanderThreatBaseAttribute
{
	override protected float ReadThreat(CMD_ThreatResponseComponent t) { return t.GetEngageThreshold(); }
	override protected void WriteThreat(CMD_ThreatResponseComponent t, float v) { t.SetEngageThreshold(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ThreatPriorityThresholdAttribute : DCO_CommanderThreatBaseAttribute
{
	override protected float ReadThreat(CMD_ThreatResponseComponent t) { return t.GetReinforcementThreshold(); }
	override protected void WriteThreat(CMD_ThreatResponseComponent t, float v) { t.SetReinforcementThreshold(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ThreatMinClusterScoreAttribute : DCO_CommanderThreatBaseAttribute
{
	override protected float ReadThreat(CMD_ThreatResponseComponent t) { return t.GetClusterMinResponseScore(); }
	override protected void WriteThreat(CMD_ThreatResponseComponent t, float v) { t.SetClusterMinResponseScore(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ThreatMaxReinforcementAttribute : DCO_CommanderThreatBaseAttribute
{
	override protected float ReadThreat(CMD_ThreatResponseComponent t) { return t.GetMaxReinforcementSent(); }
	override protected void WriteThreat(CMD_ThreatResponseComponent t, float v) { t.SetMaxReinforcementSent(Math.Round(v)); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ThreatReinforcementCooldownAttribute : DCO_CommanderThreatBaseAttribute
{
	override protected float ReadThreat(CMD_ThreatResponseComponent t) { return t.GetReinforcementCooldown(); }
	override protected void WriteThreat(CMD_ThreatResponseComponent t, float v) { t.SetReinforcementCooldown(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ThreatExpiryAttribute : DCO_CommanderThreatBaseAttribute
{
	override protected float ReadThreat(CMD_ThreatResponseComponent t) { return t.GetThreatExpiry(); }
	override protected void WriteThreat(CMD_ThreatResponseComponent t, float v) { t.SetThreatExpiry(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ThreatThinkIntervalAttribute : DCO_CommanderThreatBaseAttribute
{
	override protected float ReadThreat(CMD_ThreatResponseComponent t) { return t.GetThinkInterval(); }
	override protected void WriteThreat(CMD_ThreatResponseComponent t, float v) { t.SetThinkInterval(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ThreatFlankDistanceAttribute : DCO_CommanderThreatBaseAttribute
{
	override protected float ReadThreat(CMD_ThreatResponseComponent t) { return t.GetFlankDistance(); }
	override protected void WriteThreat(CMD_ThreatResponseComponent t, float v) { t.SetFlankDistance(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderArtyBaseAttribute : DCO_CommanderSupportBaseAttribute
{
	protected float ReadArty(CMD_ArtillerySupport a) { return 0; }
	protected void WriteArty(CMD_ArtillerySupport a, float v) {}

	override protected bool Read(AICommander_BaseComponent cmd, out float value)
	{
		CMD_ArtillerySupport a = cmd.GetArtySupport();
		if (!a)
			return false;

		value = ReadArty(a);
		return true;
	}

	override protected void Write(AICommander_BaseComponent cmd, float value)
	{
		CMD_ArtillerySupport a = cmd.GetArtySupport();
		if (a)
			WriteArty(a, value);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ArtyRejectionChanceAttribute : DCO_CommanderArtyBaseAttribute
{
	override protected float ReadArty(CMD_ArtillerySupport a) { return a.GetRejectionChance(); }
	override protected void WriteArty(CMD_ArtillerySupport a, float v) { a.SetRejectionChance(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ArtyGlobalCooldownAttribute : DCO_CommanderArtyBaseAttribute
{
	override protected float ReadArty(CMD_ArtillerySupport a) { return a.GetGlobalCooldown(); }
	override protected void WriteArty(CMD_ArtillerySupport a, float v) { a.SetGlobalCooldown(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ArtyCooldownPerShellAttribute : DCO_CommanderArtyBaseAttribute
{
	override protected float ReadArty(CMD_ArtillerySupport a) { return a.GetCooldownPerShell(); }
	override protected void WriteArty(CMD_ArtillerySupport a, float v) { a.SetCooldownPerShell(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ArtyBaseAccuracyAttribute : DCO_CommanderArtyBaseAttribute
{
	override protected float ReadArty(CMD_ArtillerySupport a) { return a.GetBaseAccuracy(); }
	override protected void WriteArty(CMD_ArtillerySupport a, float v) { a.SetBaseAccuracy(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ArtyDispersionAttribute : DCO_CommanderArtyBaseAttribute
{
	override protected float ReadArty(CMD_ArtillerySupport a) { return a.GetDispersionMultiplier(); }
	override protected void WriteArty(CMD_ArtillerySupport a, float v) { a.SetDispersionMultiplier(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ArtyFriendlySafeRadiusAttribute : DCO_CommanderArtyBaseAttribute
{
	override protected float ReadArty(CMD_ArtillerySupport a) { return a.GetFriendlySafeRadius(); }
	override protected void WriteArty(CMD_ArtillerySupport a, float v) { a.SetFriendlySafeRadius(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ArtyClusterCooldownAttribute : DCO_CommanderThreatBaseAttribute
{
	override protected float ReadThreat(CMD_ThreatResponseComponent t) { return t.GetArtilleryCooldown(); }
	override protected void WriteThreat(CMD_ThreatResponseComponent t, float v) { t.SetArtilleryCooldown(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderReservePercentAttribute : DCO_CommanderSupportBaseAttribute
{
	override protected bool Read(AICommander_BaseComponent cmd, out float value) { value = cmd.GetReservePercent(); return true; }
	override protected void Write(AICommander_BaseComponent cmd, float value) { cmd.SetReservePercent(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderAllInAttribute : DCO_CommanderLogiBoolAttribute
{
	override protected bool Get(AICommander_BaseComponent cmd) { return cmd.IsAllIn(); }
	override protected void Set(AICommander_BaseComponent cmd, bool value) { cmd.SetAllIn(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderAllInRearAttribute : DCO_CommanderLogiBoolAttribute
{
	override protected bool Get(AICommander_BaseComponent cmd) { return cmd.IsAllInRearDefenders(); }
	override protected void Set(AICommander_BaseComponent cmd, bool value) { cmd.SetAllInRearDefenders(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderPlayerAcceptTimeAttribute : DCO_CommanderSupportBaseAttribute
{
	override protected bool Read(AICommander_BaseComponent cmd, out float value) { value = cmd.GetPlayerAcceptTime(); return true; }
	override protected void Write(AICommander_BaseComponent cmd, float value) { cmd.SetPlayerAcceptTime(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderPlayerMaxEtaAttribute : DCO_CommanderSupportBaseAttribute
{
	override protected bool Read(AICommander_BaseComponent cmd, out float value) { value = cmd.GetPlayerMaxEtaMin(); return true; }
	override protected void Write(AICommander_BaseComponent cmd, float value) { cmd.SetPlayerMaxEtaMin(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ArtyPointUncertaintyAttribute : DCO_CommanderArtyBaseAttribute
{
	override protected float ReadArty(CMD_ArtillerySupport a) { return a.GetPointUncertainty(); }
	override protected void WriteArty(CMD_ArtillerySupport a, float v) { a.SetPointUncertainty(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ArtyAreaUncertaintyAttribute : DCO_CommanderArtyBaseAttribute
{
	override protected float ReadArty(CMD_ArtillerySupport a) { return a.GetAreaUncertainty(); }
	override protected void WriteArty(CMD_ArtillerySupport a, float v) { a.SetAreaUncertainty(v); }
}
