class DCO_TuneAttr
{
	static IEntity CmdOwner(Managed item)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!cmd)
			return null;
		return cmd.GetOwner();
	}

	static CMD_ThreatResponseComponent Threat(Managed item)
	{
		IEntity owner = CmdOwner(item);
		if (!owner)
			return null;
		return CMD_ThreatResponseComponent.Cast(owner.FindComponent(CMD_ThreatResponseComponent));
	}

	static CMD_ArtillerySupport Arty(Managed item)
	{
		IEntity owner = CmdOwner(item);
		if (!owner)
			return null;
		return CMD_ArtillerySupport.Cast(owner.FindComponent(CMD_ArtillerySupport));
	}

	static DCO_GroupConfigComponent Group(Managed item)
	{
		SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(item);
		if (!editableGroup || editableGroup.HasEntityState(EEditableEntityState.PLAYER))
			return null;
		SCR_AIGroup group = editableGroup.GetAIGroupComponent();
		if (!group)
			return null;
		return DCO_GroupConfigComponent.Cast(group.FindComponent(DCO_GroupConfigComponent));
	}

	static void LogB(string key, bool value, int playerID)
	{
		float v = 0;
		if (value)
			v = 1;
		Log(key, v, playerID);
	}

	static void Log(string key, float value, int playerID)
	{
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("gm_tune key=%1 value=%2 player=%3", key, value.ToString(-1, 3), playerID));
		DCO_Report.Gm(key, value.ToString(-1, 3));
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_AggressionAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.DCO_AggressionSlider(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.DCO_SetAggression(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_TuneFloatAttribute : SCR_BaseValueListEditorAttribute
{
	protected string Key() { return ""; }
	protected float Def() { return 0; }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalFloatAttribute.GetGlobal(item);
		if (!cfg)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(cfg.DCO_TuneGet(Key(), Def()));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalFloatAttribute.GetGlobal(item);
		if (!cfg || !var)
			return;
		cfg.DCO_TuneSet(Key(), var.GetFloat());
		DCO_TuneAttr.Log(Key(), var.GetFloat(), playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_TuneBoolAttribute : SCR_BaseEditorAttribute
{
	protected string Key() { return ""; }
	protected bool Def() { return false; }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalFloatAttribute.GetGlobal(item);
		if (!cfg)
			return null;
		float d = 0;
		if (Def())
			d = 1;
		return SCR_BaseEditorAttributeVar.CreateBool(cfg.DCO_TuneGet(Key(), d) >= 0.5);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalFloatAttribute.GetGlobal(item);
		if (!cfg || !var)
			return;
		float v = 0;
		if (var.GetBool())
			v = 1;
		cfg.DCO_TuneSet(Key(), v);
		DCO_TuneAttr.Log(Key(), v, playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdTuneFloatAttribute : SCR_BaseValueListEditorAttribute
{
	protected float Get(AICommander_BaseComponent c) { return 0; }
	protected void Set(AICommander_BaseComponent c, float v) {}
	protected string Key() { return ""; }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent c = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!c)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(Get(c));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		AICommander_BaseComponent c = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!c || !var)
			return;
		Set(c, var.GetFloat());
		DCO_TuneAttr.Log(Key(), var.GetFloat(), playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdTuneBoolAttribute : SCR_BaseEditorAttribute
{
	protected bool Get(AICommander_BaseComponent c) { return false; }
	protected void Set(AICommander_BaseComponent c, bool v) {}
	protected string Key() { return ""; }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent c = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!c)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(Get(c));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		AICommander_BaseComponent c = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!c || !var)
			return;
		Set(c, var.GetBool());
		float v = 0;
		if (var.GetBool())
			v = 1;
		DCO_TuneAttr.Log(Key(), v, playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ThreatTuneFloatAttribute : SCR_BaseValueListEditorAttribute
{
	protected float Get(CMD_ThreatResponseComponent c) { return 0; }
	protected void Set(CMD_ThreatResponseComponent c, float v) {}
	protected string Key() { return ""; }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_ThreatResponseComponent c = DCO_TuneAttr.Threat(item);
		if (!c)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(Get(c));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_ThreatResponseComponent c = DCO_TuneAttr.Threat(item);
		if (!c || !var)
			return;
		Set(c, var.GetFloat());
		DCO_TuneAttr.Log(Key(), var.GetFloat(), playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ThreatTuneBoolAttribute : SCR_BaseEditorAttribute
{
	protected bool Get(CMD_ThreatResponseComponent c) { return false; }
	protected void Set(CMD_ThreatResponseComponent c, bool v) {}
	protected string Key() { return ""; }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_ThreatResponseComponent c = DCO_TuneAttr.Threat(item);
		if (!c)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(Get(c));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_ThreatResponseComponent c = DCO_TuneAttr.Threat(item);
		if (!c || !var)
			return;
		Set(c, var.GetBool());
		float v = 0;
		if (var.GetBool())
			v = 1;
		DCO_TuneAttr.Log(Key(), v, playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ArtyTuneFloatAttribute : SCR_BaseValueListEditorAttribute
{
	protected float Get(CMD_ArtillerySupport c) { return 0; }
	protected void Set(CMD_ArtillerySupport c, float v) {}
	protected string Key() { return ""; }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_ArtillerySupport c = DCO_TuneAttr.Arty(item);
		if (!c)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(Get(c));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_ArtillerySupport c = DCO_TuneAttr.Arty(item);
		if (!c || !var)
			return;
		Set(c, var.GetFloat());
		DCO_TuneAttr.Log(Key(), var.GetFloat(), playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ArtyTuneBoolAttribute : SCR_BaseEditorAttribute
{
	protected bool Get(CMD_ArtillerySupport c) { return false; }
	protected void Set(CMD_ArtillerySupport c, bool v) {}
	protected string Key() { return ""; }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_ArtillerySupport c = DCO_TuneAttr.Arty(item);
		if (!c)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(Get(c));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_ArtillerySupport c = DCO_TuneAttr.Arty(item);
		if (!c || !var)
			return;
		Set(c, var.GetBool());
		float v = 0;
		if (var.GetBool())
			v = 1;
		DCO_TuneAttr.Log(Key(), v, playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjTuneFloatAttribute : SCR_BaseValueListEditorAttribute
{
	protected float Get(CMD_AICommanderObjectiveComponent c) { return 0; }
	protected void Set(CMD_AICommanderObjectiveComponent c, float v) {}
	protected string Key() { return ""; }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent c = DCO_ObjAttrUtil.GetObjective(item);
		if (!c)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(Get(c));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent c = DCO_ObjAttrUtil.GetObjective(item);
		if (!c || !var)
			return;
		Set(c, var.GetFloat());
		DCO_TuneAttr.Log(Key(), var.GetFloat(), playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjTuneBoolAttribute : SCR_BaseEditorAttribute
{
	protected bool Get(CMD_AICommanderObjectiveComponent c) { return false; }
	protected void Set(CMD_AICommanderObjectiveComponent c, bool v) {}
	protected string Key() { return ""; }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent c = DCO_ObjAttrUtil.GetObjective(item);
		if (!c)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(Get(c));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent c = DCO_ObjAttrUtil.GetObjective(item);
		if (!c || !var)
			return;
		Set(c, var.GetBool());
		float v = 0;
		if (var.GetBool())
			v = 1;
		DCO_TuneAttr.Log(Key(), v, playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GroupTuneFloatAttribute : SCR_BaseValueListEditorAttribute
{
	protected float Get(DCO_GroupConfigComponent c) { return 0; }
	protected void Set(DCO_GroupConfigComponent c, float v) {}
	protected string Key() { return ""; }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GroupConfigComponent c = DCO_TuneAttr.Group(item);
		if (!c)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(Get(c));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GroupConfigComponent c = DCO_TuneAttr.Group(item);
		if (!c || !var)
			return;
		Set(c, var.GetFloat());
		DCO_TuneAttr.Log(Key(), var.GetFloat(), playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GroupTuneBoolAttribute : SCR_BaseEditorAttribute
{
	protected bool Get(DCO_GroupConfigComponent c) { return false; }
	protected void Set(DCO_GroupConfigComponent c, bool v) {}
	protected string Key() { return ""; }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GroupConfigComponent c = DCO_TuneAttr.Group(item);
		if (!c)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(Get(c));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GroupConfigComponent c = DCO_TuneAttr.Group(item);
		if (!c || !var)
			return;
		Set(c, var.GetBool());
		float v = 0;
		if (var.GetBool())
			v = 1;
		DCO_TuneAttr.Log(Key(), v, playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_UseRealisticReactionAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "wf.UseRealisticReaction"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_SuppressedAudibleRangeAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.SuppressedAudibleRange"; }
	override protected float Def() { return 180.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_InVehicleHearingFactorAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.InVehicleHearingFactor"; }
	override protected float Def() { return 0.5; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_AudibleGuaranteeThresholdAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.AudibleGuaranteeThreshold"; }
	override protected float Def() { return 0.3; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_DirectionErrorBaseDegAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.DirectionErrorBaseDeg"; }
	override protected float Def() { return 5.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_DirectionErrorExtraDegAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.DirectionErrorExtraDeg"; }
	override protected float Def() { return 20.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_LookDistFullAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.LookDistFull"; }
	override protected float Def() { return 60.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_LookDistMaxAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.LookDistMax"; }
	override protected float Def() { return 400.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_LookChanceAtMaxAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.LookChanceAtMax"; }
	override protected float Def() { return 0.2; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_LookThreatBonusAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.LookThreatBonus"; }
	override protected float Def() { return 0.5; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_LookDelayMinAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.LookDelayMin"; }
	override protected float Def() { return 0.15; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_LookDelayMaxAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.LookDelayMax"; }
	override protected float Def() { return 0.35; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_LookPreferClosestAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "wf.LookPreferClosest"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_LookLockDurationAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.LookLockDuration"; }
	override protected float Def() { return 3.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_LookOverrideDistRatioAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.LookOverrideDistRatio"; }
	override protected float Def() { return 0.8; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_LookLockRefreshIntervalAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.LookLockRefreshInterval"; }
	override protected float Def() { return 0.5; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_LookPriorityProximityBonusAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.LookPriorityProximityBonus"; }
	override protected float Def() { return 10.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_CoverThreatGateAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.CoverThreatGate"; }
	override protected float Def() { return 0.4; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_LowerStanceOnLowThreatAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "wf.LowerStanceOnLowThreat"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_CoverDelayMinAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.CoverDelayMin"; }
	override protected float Def() { return 0.2; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_CoverDelayMaxAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.CoverDelayMax"; }
	override protected float Def() { return 0.6; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_HighThreatProneThresholdAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.HighThreatProneThreshold"; }
	override protected float Def() { return 0.7; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_CoverSearchDistCapAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.CoverSearchDistCap"; }
	override protected float Def() { return 12.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_SharedMoveCooldownAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.SharedMoveCooldown"; }
	override protected float Def() { return 8.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_CoverPreferBuildingAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "wf.CoverPreferBuilding"; }
	override protected bool Def() { return false; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_CoverVeryHighPriorityAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "wf.CoverVeryHighPriority"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_UseDodgeChanceOnCoverAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "wf.UseDodgeChanceOnCover"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_LookAfterCoverAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "wf.LookAfterCover"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_LookAfterCoverRepeatsAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.LookAfterCoverRepeats"; }
	override protected float Def() { return 3.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_LookAfterCoverIntervalAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.LookAfterCoverInterval"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_LookAfterCoverDelayAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.LookAfterCoverDelay"; }
	override protected float Def() { return 0.3; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_RandomizeCoverDirectionAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "wf.RandomizeCoverDirection"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_CoverDirWeightBackwardAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.CoverDirWeightBackward"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_CoverDirWeightLeftAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.CoverDirWeightLeft"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_CoverDirWeightRightAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.CoverDirWeightRight"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_CoverDirWeightAnywhereAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.CoverDirWeightAnywhere"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_ScaleCoverDirByPersonalityAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "wf.ScaleCoverDirByPersonality"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_CoverDirForwardWeightScaleAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "wf.CoverDirForwardWeightScale"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_WF_DodgeTowardWaypointAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "wf.DodgeTowardWaypoint"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_ReactToNearMissAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ph.ReactToNearMiss"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_BulletsRepositionNoTargetAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "ph.BulletsRepositionNoTarget"; }
	override protected float Def() { return 2.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_BulletsRepositionHeavyAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "ph.BulletsRepositionHeavy"; }
	override protected float Def() { return 4.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_BulletsFleeNoCoverAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "ph.BulletsFleeNoCover"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_BulletsFleeEngagedAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "ph.BulletsFleeEngaged"; }
	override protected float Def() { return 3.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_ScaleThresholdsByPersonalityAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ph.ScaleThresholdsByPersonality"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_DeploySmokeOnFleeAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ph.DeploySmokeOnFlee"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_CoverPreferBuildingAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ph.CoverPreferBuilding"; }
	override protected bool Def() { return false; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_CoverVeryHighPriorityAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ph.CoverVeryHighPriority"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_RespectOngoingMoveAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ph.RespectOngoingMove"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_NewThreatAngleDegAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "ph.NewThreatAngleDeg"; }
	override protected float Def() { return 60.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_SkipWhileCoverActiveAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ph.SkipWhileCoverActive"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_CoverMoveCooldown_sAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "ph.CoverMoveCooldown_s"; }
	override protected float Def() { return 5.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_RepositionSearchDistAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "ph.RepositionSearchDist"; }
	override protected float Def() { return 6.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_FleeSearchDistAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "ph.FleeSearchDist"; }
	override protected float Def() { return 10.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_ForceStandWhileMovingAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ph.ForceStandWhileMoving"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_SuppressWhileMovingAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ph.SuppressWhileMoving"; }
	override protected bool Def() { return false; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_AimWhileMovingToCoverAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ph.AimWhileMovingToCover"; }
	override protected bool Def() { return false; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_UseDodgeChanceAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ph.UseDodgeChance"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_RandomizeCoverDirectionAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ph.RandomizeCoverDirection"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_CoverDirWeightBackwardAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "ph.CoverDirWeightBackward"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_CoverDirWeightLeftAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "ph.CoverDirWeightLeft"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_CoverDirWeightRightAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "ph.CoverDirWeightRight"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_CoverDirWeightAnywhereAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "ph.CoverDirWeightAnywhere"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_ScaleCoverDirByPersonalityAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ph.ScaleCoverDirByPersonality"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PH_DodgeTowardWaypointAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ph.DodgeTowardWaypoint"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CM_MaxBookingDurationAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cm.MaxBookingDuration"; }
	override protected float Def() { return 30.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_IR_TowardThreatToleranceAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "ir.TowardThreatTolerance"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_IR_ThreatWeightDefendAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "ir.ThreatWeightDefend"; }
	override protected float Def() { return 0.6; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_IR_ThreatWeightFireAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "ir.ThreatWeightFire"; }
	override protected float Def() { return 0.7; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_IR_ThreatSampleSpreadAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "ir.ThreatSampleSpread"; }
	override protected float Def() { return 2.5; }
}



[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_MinEnemyToReportAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.MinEnemyToReport"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_ContactMaxAgeAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.ContactMaxAge"; }
	override protected float Def() { return 60.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_ContactClusterRadiusAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.ContactClusterRadius"; }
	override protected float Def() { return 75.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_ReportMinShiftAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.ReportMinShift"; }
	override protected float Def() { return 50.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_MinResendIntervalAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.MinResendInterval"; }
	override protected float Def() { return 30.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_LowConfidenceThresholdAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.LowConfidenceThreshold"; }
	override protected float Def() { return 0.5; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_HighDangerEnemyCountAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.HighDangerEnemyCount"; }
	override protected float Def() { return 8.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_HighDangerDistanceAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.HighDangerDistance"; }
	override protected float Def() { return 100.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_ReinforcementThresholdAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.ReinforcementThreshold"; }
	override protected float Def() { return 3.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_ReinfRequestTimeoutAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.ReinfRequestTimeout"; }
	override protected float Def() { return 90.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_ArtilleryEnemyThresholdAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.ArtilleryEnemyThreshold"; }
	override protected float Def() { return 4.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_ArtilleryRequestCooldownAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.ArtilleryRequestCooldown"; }
	override protected float Def() { return 120.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_NonFavoredRoleArtyRejectChanceAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.NonFavoredRoleArtyRejectChance"; }
	override protected float Def() { return 0.35; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_NoisePer100mAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.NoisePer100m"; }
	override protected float Def() { return 8.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_NightNoiseMulAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.NightNoiseMul"; }
	override protected float Def() { return 2.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_DetectedNoiseMulAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.DetectedNoiseMul"; }
	override protected float Def() { return 1.8; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_BucketSquadMinAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.BucketSquadMin"; }
	override protected float Def() { return 4.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_BucketPlatoonMinAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.BucketPlatoonMin"; }
	override protected float Def() { return 10.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_ArmorIdentifyDistAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.ArmorIdentifyDist"; }
	override protected float Def() { return 400.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_RadioDelayMinAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.RadioDelayMin"; }
	override protected float Def() { return 5.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_RadioDelayMaxAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.RadioDelayMax"; }
	override protected float Def() { return 20.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_RequireRadioAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "cr.RequireRadio"; }
	override protected bool Def() { return false; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_SuppressedDelayMulAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.SuppressedDelayMul"; }
	override protected float Def() { return 1.5; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CR_SuppressedNoiseMulAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "cr.SuppressedNoiseMul"; }
	override protected float Def() { return 1.5; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TT_BoardingDistAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "tt.BoardingDist"; }
	override protected float Def() { return 25.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TT_BoardedRatioAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "tt.BoardedRatio"; }
	override protected float Def() { return 0.7; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TT_ArrivalDistAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "tt.ArrivalDist"; }
	override protected float Def() { return 25.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TT_BoardingTimeoutAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "tt.BoardingTimeout"; }
	override protected float Def() { return 60.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TT_DriveTimeoutAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "tt.DriveTimeout"; }
	override protected float Def() { return 300.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TT_DisembarkTimeoutAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "tt.DisembarkTimeout"; }
	override protected float Def() { return 30.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TT_ReturnToRallyAfterDropAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "tt.ReturnToRallyAfterDrop"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TT_LoadTimeAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "tt.LoadTime"; }
	override protected float Def() { return 10.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TT_ResupplyTimeAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "tt.ResupplyTime"; }
	override protected float Def() { return 20.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TT_ResupplyMagsAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "tt.ResupplyMags"; }
	override protected float Def() { return 4.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_GA_PersonalityWeightStandardAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.DCO_GT_PersonalityWeightStandard(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.DCO_ST_PersonalityWeightStandard(value); DCO_TuneAttr.Log("ga.PersonalityWeightStandard", value, -1); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_GA_PersonalityWeightCautiousAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.DCO_GT_PersonalityWeightCautious(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.DCO_ST_PersonalityWeightCautious(value); DCO_TuneAttr.Log("ga.PersonalityWeightCautious", value, -1); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_GA_PersonalityWeightAggressiveAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.DCO_GT_PersonalityWeightAggressive(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.DCO_ST_PersonalityWeightAggressive(value); DCO_TuneAttr.Log("ga.PersonalityWeightAggressive", value, -1); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_GA_PersonalityWeightRecklessAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.DCO_GT_PersonalityWeightReckless(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.DCO_ST_PersonalityWeightReckless(value); DCO_TuneAttr.Log("ga.PersonalityWeightReckless", value, -1); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_GA_StragglerEnabledAttribute : DCO_GlobalBoolAttribute
{
	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.DCO_GT_StragglerEnabled(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value) { cfg.DCO_ST_StragglerEnabled(value); DCO_TuneAttr.LogB("ga.StragglerEnabled", value, -1); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_GA_StragglerTimeAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.DCO_GT_StragglerTime(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.DCO_ST_StragglerTime(value); DCO_TuneAttr.Log("ga.StragglerTime", value, -1); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_GA_StragglerStuckTimeAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.DCO_GT_StragglerStuckTime(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.DCO_ST_StragglerStuckTime(value); DCO_TuneAttr.Log("ga.StragglerStuckTime", value, -1); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_DelayFirstIterationAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.DelayFirstIteration"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_DelayFirstIteration(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_DelayFirstIteration(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_CaptureCheckIntervalAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.CaptureCheckInterval"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_CaptureCheckInterval(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_CaptureCheckInterval(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_StalemateResponseCooldownAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.StalemateResponseCooldown"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_StalemateResponseCooldown(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_StalemateResponseCooldown(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_EnablePreemptionAttribute : DCO_CmdTuneBoolAttribute
{
	override protected string Key() { return "cmd.EnablePreemption"; }
	override protected bool Get(AICommander_BaseComponent c) { return c.DCO_GT_EnablePreemption(); }
	override protected void Set(AICommander_BaseComponent c, bool v) { c.DCO_ST_EnablePreemption(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_PreemptionMarginAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.PreemptionMargin"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_PreemptionMargin(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_PreemptionMargin(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_PreemptionCooldownAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.PreemptionCooldown"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_PreemptionCooldown(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_PreemptionCooldown(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_ReinforcementStockAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.ReinforcementStock"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_ReinforcementStock(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_ReinforcementStock(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_PlayerTaskingAttribute : DCO_CmdTuneBoolAttribute
{
	override protected string Key() { return "cmd.PlayerTasking"; }
	override protected bool Get(AICommander_BaseComponent c) { return c.DCO_GT_PlayerTasking(); }
	override protected void Set(AICommander_BaseComponent c, bool v) { c.DCO_ST_PlayerTasking(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_PlayerTaskingCountedAttribute : DCO_CmdTuneBoolAttribute
{
	override protected string Key() { return "cmd.PlayerTaskingCounted"; }
	override protected bool Get(AICommander_BaseComponent c) { return c.DCO_GT_PlayerTaskingCounted(); }
	override protected void Set(AICommander_BaseComponent c, bool v) { c.DCO_ST_PlayerTaskingCounted(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_PlayerMaxPerObjectiveAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.PlayerMaxPerObjective"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_PlayerMaxPerObjective(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_PlayerMaxPerObjective(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_PlayerEtaFactorAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.PlayerEtaFactor"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_PlayerEtaFactor(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_PlayerEtaFactor(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_PlayerEtaMarginAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.PlayerEtaMargin"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_PlayerEtaMargin(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_PlayerEtaMargin(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_PlayerEarlyReleaseAttribute : DCO_CmdTuneBoolAttribute
{
	override protected string Key() { return "cmd.PlayerEarlyRelease"; }
	override protected bool Get(AICommander_BaseComponent c) { return c.DCO_GT_PlayerEarlyRelease(); }
	override protected void Set(AICommander_BaseComponent c, bool v) { c.DCO_ST_PlayerEarlyRelease(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_PlayerFailLimitAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.PlayerFailLimit"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_PlayerFailLimit(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_PlayerFailLimit(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_PlayerBonusMinutesAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.PlayerBonusMinutes"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_PlayerBonusMinutes(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_PlayerBonusMinutes(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_MoraleAttackThresholdAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.MoraleAttackThreshold"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_MoraleAttackThreshold(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_MoraleAttackThreshold(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_UseSynchronizedAttackAttribute : DCO_CmdTuneBoolAttribute
{
	override protected string Key() { return "cmd.UseSynchronizedAttack"; }
	override protected bool Get(AICommander_BaseComponent c) { return c.DCO_GT_UseSynchronizedAttack(); }
	override protected void Set(AICommander_BaseComponent c, bool v) { c.DCO_ST_UseSynchronizedAttack(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_SyncAttackMaxWaitTimeAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.SyncAttackMaxWaitTime"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_SyncAttackMaxWaitTime(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_SyncAttackMaxWaitTime(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_ReconWaitTimeoutAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.ReconWaitTimeout"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_ReconWaitTimeout(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_ReconWaitTimeout(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_AttackRatioAggressiveAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.AttackRatioAggressive"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_AttackRatioAggressive(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_AttackRatioAggressive(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_AttackRatioCautiousAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.AttackRatioCautious"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_AttackRatioCautious(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_AttackRatioCautious(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_AttackAbortCooldownAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.AttackAbortCooldown"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_AttackAbortCooldown(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_AttackAbortCooldown(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_StagingLegFractionAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.StagingLegFraction"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_StagingLegFraction(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_StagingLegFraction(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_StagingMinDistanceAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.StagingMinDistance"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_StagingMinDistance(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_StagingMinDistance(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_StagingMaxDistanceAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.StagingMaxDistance"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_StagingMaxDistance(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_StagingMaxDistance(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_StagingMarginAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.StagingMargin"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_StagingMargin(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_StagingMargin(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_WaypointLegDistanceAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.WaypointLegDistance"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_WaypointLegDistance(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_WaypointLegDistance(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_WaypointNudgeRadiusAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.WaypointNudgeRadius"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_WaypointNudgeRadius(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_WaypointNudgeRadius(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_MaxSearchWaypointsAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.MaxSearchWaypoints"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_MaxSearchWaypoints(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_MaxSearchWaypoints(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_FrontlineReconRadiusAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.FrontlineReconRadius"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_FrontlineReconRadius(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_FrontlineReconRadius(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_FrontlineReconDurationAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.FrontlineReconDuration"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_FrontlineReconDuration(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_FrontlineReconDuration(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_ReconRevealIntervalAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.ReconRevealInterval"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_ReconRevealInterval(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_ReconRevealInterval(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_FrontlineNodeBiasAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.FrontlineNodeBias"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_FrontlineNodeBias(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_FrontlineNodeBias(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_EnvelopeArcDegAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.EnvelopeArcDeg"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_EnvelopeArcDeg(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_EnvelopeArcDeg(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_EnvelopeRadiusMulAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.EnvelopeRadiusMul"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_EnvelopeRadiusMul(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_EnvelopeRadiusMul(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_EnvelopeMinRadiusAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.EnvelopeMinRadius"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_EnvelopeMinRadius(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_EnvelopeMinRadius(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_EnvelopeNodesAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.EnvelopeNodes"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_EnvelopeNodes(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_EnvelopeNodes(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_MinGroupClusterDistanceAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.MinGroupClusterDistance"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_MinGroupClusterDistance(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_MinGroupClusterDistance(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_DefendEngageDistAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.DefendEngageDist"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_DefendEngageDist(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_DefendEngageDist(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_DefendObjectiveMarginAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.DefendObjectiveMargin"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_DefendObjectiveMargin(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_DefendObjectiveMargin(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_BaseRadiusAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.BaseRadius"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_BaseRadius(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_BaseRadius(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_MinSectorAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.MinSector"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_MinSector(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_MinSector(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_MaxSectorAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.MaxSector"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_MaxSector(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_MaxSector(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_ArcPerSectorAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.ArcPerSector"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_ArcPerSector(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_ArcPerSector(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_ScaleSectorByPersonalityAttribute : DCO_CmdTuneBoolAttribute
{
	override protected string Key() { return "cmd.ScaleSectorByPersonality"; }
	override protected bool Get(AICommander_BaseComponent c) { return c.DCO_GT_ScaleSectorByPersonality(); }
	override protected void Set(AICommander_BaseComponent c, bool v) { c.DCO_ST_ScaleSectorByPersonality(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_SectorPersonalityMinAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.SectorPersonalityMin"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_SectorPersonalityMin(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_SectorPersonalityMin(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_SectorPersonalityMaxAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.SectorPersonalityMax"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_SectorPersonalityMax(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_SectorPersonalityMax(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_MaxCompletionRadiusAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.MaxCompletionRadius"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_MaxCompletionRadius(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_MaxCompletionRadius(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_GateDefendByManpowerAttribute : DCO_CmdTuneBoolAttribute
{
	override protected string Key() { return "cmd.GateDefendByManpower"; }
	override protected bool Get(AICommander_BaseComponent c) { return c.DCO_GT_GateDefendByManpower(); }
	override protected void Set(AICommander_BaseComponent c, bool v) { c.DCO_ST_GateDefendByManpower(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_ThreatDirectionRadiusAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.ThreatDirectionRadius"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_ThreatDirectionRadius(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_ThreatDirectionRadius(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_FeintLeadAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.FeintLead"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_FeintLead(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_FeintLead(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_MaxPatrolPullDistanceAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.MaxPatrolPullDistance"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_MaxPatrolPullDistance(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_MaxPatrolPullDistance(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_PatrolFrontlineDriftAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.PatrolFrontlineDrift"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_PatrolFrontlineDrift(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_PatrolFrontlineDrift(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_PatrolFrontlineStandoffAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.PatrolFrontlineStandoff"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_PatrolFrontlineStandoff(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_PatrolFrontlineStandoff(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_PatrolTerrainScoringAttribute : DCO_CmdTuneBoolAttribute
{
	override protected string Key() { return "cmd.PatrolTerrainScoring"; }
	override protected bool Get(AICommander_BaseComponent c) { return c.DCO_GT_PatrolTerrainScoring(); }
	override protected void Set(AICommander_BaseComponent c, bool v) { c.DCO_ST_PatrolTerrainScoring(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_PatrolSmartBudgetAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.PatrolSmartBudget"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_PatrolSmartBudget(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_PatrolSmartBudget(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_PatrolMemoryRadiusAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.PatrolMemoryRadius"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_PatrolMemoryRadius(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_PatrolMemoryRadius(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_PatrolMemorySizeAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.PatrolMemorySize"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_PatrolMemorySize(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_PatrolMemorySize(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_PatrolRadiusMulMinAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.PatrolRadiusMulMin"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_PatrolRadiusMulMin(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_PatrolRadiusMulMin(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_PatrolRadiusMulMaxAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.PatrolRadiusMulMax"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_PatrolRadiusMulMax(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_PatrolRadiusMulMax(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_TransportDistanceThresholdAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.TransportDistanceThreshold"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_TransportDistanceThreshold(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_TransportDistanceThreshold(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_LogiPrioMedevacAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.LogiPrioMedevac"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_LogiPrioMedevac(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_LogiPrioMedevac(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_LogiPrioExtractAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.LogiPrioExtract"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_LogiPrioExtract(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_LogiPrioExtract(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_LogiPrioReinforceAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.LogiPrioReinforce"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_LogiPrioReinforce(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_LogiPrioReinforce(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_LogiPrioDeployAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.LogiPrioDeploy"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_LogiPrioDeploy(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_LogiPrioDeploy(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_LogiPrioRedeployAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.LogiPrioRedeploy"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_LogiPrioRedeploy(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_LogiPrioRedeploy(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_LogiPrioResupplyAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.LogiPrioResupply"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_LogiPrioResupply(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_LogiPrioResupply(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_LogiPrioPlayerAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.LogiPrioPlayer"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_LogiPrioPlayer(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_LogiPrioPlayer(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_LogiEtaMarginAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.LogiEtaMargin"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_LogiEtaMargin(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_LogiEtaMargin(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_LogiBatchRadiusAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.LogiBatchRadius"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_LogiBatchRadius(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_LogiBatchRadius(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_LogiBatchWindowAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.LogiBatchWindow"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_LogiBatchWindow(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_LogiBatchWindow(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_LogiLZSafeDistAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.LogiLZSafeDist"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_LogiLZSafeDist(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_LogiLZSafeDist(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_LogiTreatTimeAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.LogiTreatTime"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_LogiTreatTime(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_LogiTreatTime(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_LogiExtractMinDistAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.LogiExtractMinDist"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_LogiExtractMinDist(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_LogiExtractMinDist(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_LogiLowAmmoMagsAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.LogiLowAmmoMags"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_LogiLowAmmoMags(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_LogiLowAmmoMags(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_CMD_LogiHubRearDistAttribute : DCO_CmdTuneFloatAttribute
{
	override protected string Key() { return "cmd.LogiHubRearDist"; }
	override protected float Get(AICommander_BaseComponent c) { return c.DCO_GT_LogiHubRearDist(); }
	override protected void Set(AICommander_BaseComponent c, float v) { c.DCO_ST_LogiHubRearDist(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TR_MergeRadiusAttribute : DCO_ThreatTuneFloatAttribute
{
	override protected string Key() { return "tr.MergeRadius"; }
	override protected float Get(CMD_ThreatResponseComponent c) { return c.DCO_GT_MergeRadius(); }
	override protected void Set(CMD_ThreatResponseComponent c, float v) { c.DCO_ST_MergeRadius(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TR_ClusterRadiusAttribute : DCO_ThreatTuneFloatAttribute
{
	override protected string Key() { return "tr.ClusterRadius"; }
	override protected float Get(CMD_ThreatResponseComponent c) { return c.DCO_GT_ClusterRadius(); }
	override protected void Set(CMD_ThreatResponseComponent c, float v) { c.DCO_ST_ClusterRadius(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TR_EnemyIncrementedScoreAttribute : DCO_ThreatTuneFloatAttribute
{
	override protected string Key() { return "tr.EnemyIncrementedScore"; }
	override protected float Get(CMD_ThreatResponseComponent c) { return c.DCO_GT_EnemyIncrementedScore(); }
	override protected void Set(CMD_ThreatResponseComponent c, float v) { c.DCO_ST_EnemyIncrementedScore(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TR_StalenessThresholdAttribute : DCO_ThreatTuneFloatAttribute
{
	override protected string Key() { return "tr.StalenessThreshold"; }
	override protected float Get(CMD_ThreatResponseComponent c) { return c.DCO_GT_StalenessThreshold(); }
	override protected void Set(CMD_ThreatResponseComponent c, float v) { c.DCO_ST_StalenessThreshold(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TR_UncertaintyGrowthAttribute : DCO_ThreatTuneFloatAttribute
{
	override protected string Key() { return "tr.UncertaintyGrowth"; }
	override protected float Get(CMD_ThreatResponseComponent c) { return c.DCO_GT_UncertaintyGrowth(); }
	override protected void Set(CMD_ThreatResponseComponent c, float v) { c.DCO_ST_UncertaintyGrowth(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TR_ExtrapolateMaxAttribute : DCO_ThreatTuneFloatAttribute
{
	override protected string Key() { return "tr.ExtrapolateMax"; }
	override protected float Get(CMD_ThreatResponseComponent c) { return c.DCO_GT_ExtrapolateMax(); }
	override protected void Set(CMD_ThreatResponseComponent c, float v) { c.DCO_ST_ExtrapolateMax(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TR_ArtilleryAccuracyAttribute : DCO_ThreatTuneFloatAttribute
{
	override protected string Key() { return "tr.ArtilleryAccuracy"; }
	override protected float Get(CMD_ThreatResponseComponent c) { return c.DCO_GT_ArtilleryAccuracy(); }
	override protected void Set(CMD_ThreatResponseComponent c, float v) { c.DCO_ST_ArtilleryAccuracy(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TR_LedgerTTLAttribute : DCO_ThreatTuneFloatAttribute
{
	override protected string Key() { return "tr.LedgerTTL"; }
	override protected float Get(CMD_ThreatResponseComponent c) { return c.DCO_GT_LedgerTTL(); }
	override protected void Set(CMD_ThreatResponseComponent c, float v) { c.DCO_ST_LedgerTTL(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TR_ReinforceBehindDistAttribute : DCO_ThreatTuneFloatAttribute
{
	override protected string Key() { return "tr.ReinforceBehindDist"; }
	override protected float Get(CMD_ThreatResponseComponent c) { return c.DCO_GT_ReinforceBehindDist(); }
	override protected void Set(CMD_ThreatResponseComponent c, float v) { c.DCO_ST_ReinforceBehindDist(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TR_ReinforceStandoffDistAttribute : DCO_ThreatTuneFloatAttribute
{
	override protected string Key() { return "tr.ReinforceStandoffDist"; }
	override protected float Get(CMD_ThreatResponseComponent c) { return c.DCO_GT_ReinforceStandoffDist(); }
	override protected void Set(CMD_ThreatResponseComponent c, float v) { c.DCO_ST_ReinforceStandoffDist(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TR_FlankTaskExpiryAttribute : DCO_ThreatTuneFloatAttribute
{
	override protected string Key() { return "tr.FlankTaskExpiry"; }
	override protected float Get(CMD_ThreatResponseComponent c) { return c.DCO_GT_FlankTaskExpiry(); }
	override protected void Set(CMD_ThreatResponseComponent c, float v) { c.DCO_ST_FlankTaskExpiry(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TR_RatioRadiusAttribute : DCO_ThreatTuneFloatAttribute
{
	override protected string Key() { return "tr.RatioRadius"; }
	override protected float Get(CMD_ThreatResponseComponent c) { return c.DCO_GT_RatioRadius(); }
	override protected void Set(CMD_ThreatResponseComponent c, float v) { c.DCO_ST_RatioRadius(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TR_WithdrawRatioAttribute : DCO_ThreatTuneFloatAttribute
{
	override protected string Key() { return "tr.WithdrawRatio"; }
	override protected float Get(CMD_ThreatResponseComponent c) { return c.DCO_GT_WithdrawRatio(); }
	override protected void Set(CMD_ThreatResponseComponent c, float v) { c.DCO_ST_WithdrawRatio(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TR_MissionTimeoutAttribute : DCO_ThreatTuneFloatAttribute
{
	override protected string Key() { return "tr.MissionTimeout"; }
	override protected float Get(CMD_ThreatResponseComponent c) { return c.DCO_GT_MissionTimeout(); }
	override protected void Set(CMD_ThreatResponseComponent c, float v) { c.DCO_ST_MissionTimeout(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TR_HoldReleaseMaxAttribute : DCO_ThreatTuneFloatAttribute
{
	override protected string Key() { return "tr.HoldReleaseMax"; }
	override protected float Get(CMD_ThreatResponseComponent c) { return c.DCO_GT_HoldReleaseMax(); }
	override protected void Set(CMD_ThreatResponseComponent c, float v) { c.DCO_ST_HoldReleaseMax(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TR_BlockingDistAttribute : DCO_ThreatTuneFloatAttribute
{
	override protected string Key() { return "tr.BlockingDist"; }
	override protected float Get(CMD_ThreatResponseComponent c) { return c.DCO_GT_BlockingDist(); }
	override protected void Set(CMD_ThreatResponseComponent c, float v) { c.DCO_ST_BlockingDist(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TR_UpgradeGarrisonModeAttribute : DCO_ThreatTuneBoolAttribute
{
	override protected string Key() { return "tr.UpgradeGarrisonMode"; }
	override protected bool Get(CMD_ThreatResponseComponent c) { return c.DCO_GT_UpgradeGarrisonMode(); }
	override protected void Set(CMD_ThreatResponseComponent c, bool v) { c.DCO_ST_UpgradeGarrisonMode(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TR_ProjectTimeAttribute : DCO_ThreatTuneFloatAttribute
{
	override protected string Key() { return "tr.ProjectTime"; }
	override protected float Get(CMD_ThreatResponseComponent c) { return c.DCO_GT_ProjectTime(); }
	override protected void Set(CMD_ThreatResponseComponent c, float v) { c.DCO_ST_ProjectTime(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AR_DispatchIntervalAttribute : DCO_ArtyTuneFloatAttribute
{
	override protected string Key() { return "ar.DispatchInterval"; }
	override protected float Get(CMD_ArtillerySupport c) { return c.DCO_GT_DispatchInterval(); }
	override protected void Set(CMD_ArtillerySupport c, float v) { c.DCO_ST_DispatchInterval(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AR_QueueExpiryAttribute : DCO_ArtyTuneFloatAttribute
{
	override protected string Key() { return "ar.QueueExpiry"; }
	override protected float Get(CMD_ArtillerySupport c) { return c.DCO_GT_QueueExpiry(); }
	override protected void Set(CMD_ArtillerySupport c, float v) { c.DCO_ST_QueueExpiry(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AR_MaxQueueSizeAttribute : DCO_ArtyTuneFloatAttribute
{
	override protected string Key() { return "ar.MaxQueueSize"; }
	override protected float Get(CMD_ArtillerySupport c) { return c.DCO_GT_MaxQueueSize(); }
	override protected void Set(CMD_ArtillerySupport c, float v) { c.DCO_ST_MaxQueueSize(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AR_QueueOverloadPenaltyAttribute : DCO_ArtyTuneFloatAttribute
{
	override protected string Key() { return "ar.QueueOverloadPenalty"; }
	override protected float Get(CMD_ArtillerySupport c) { return c.DCO_GT_QueueOverloadPenalty(); }
	override protected void Set(CMD_ArtillerySupport c, float v) { c.DCO_ST_QueueOverloadPenalty(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AR_QueueProcessTimeAttribute : DCO_ArtyTuneFloatAttribute
{
	override protected string Key() { return "ar.QueueProcessTime"; }
	override protected float Get(CMD_ArtillerySupport c) { return c.DCO_GT_QueueProcessTime(); }
	override protected void Set(CMD_ArtillerySupport c, float v) { c.DCO_ST_QueueProcessTime(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AR_MinGlobalCooldownAttribute : DCO_ArtyTuneFloatAttribute
{
	override protected string Key() { return "ar.MinGlobalCooldown"; }
	override protected float Get(CMD_ArtillerySupport c) { return c.DCO_GT_MinGlobalCooldown(); }
	override protected void Set(CMD_ArtillerySupport c, float v) { c.DCO_ST_MinGlobalCooldown(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AR_MaxGlobalCooldownAttribute : DCO_ArtyTuneFloatAttribute
{
	override protected string Key() { return "ar.MaxGlobalCooldown"; }
	override protected float Get(CMD_ArtillerySupport c) { return c.DCO_GT_MaxGlobalCooldown(); }
	override protected void Set(CMD_ArtillerySupport c, float v) { c.DCO_ST_MaxGlobalCooldown(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AR_MinRangeAttribute : DCO_ArtyTuneFloatAttribute
{
	override protected string Key() { return "ar.MinRange"; }
	override protected float Get(CMD_ArtillerySupport c) { return c.DCO_GT_MinRange(); }
	override protected void Set(CMD_ArtillerySupport c, float v) { c.DCO_ST_MinRange(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AR_MaxRangeAttribute : DCO_ArtyTuneFloatAttribute
{
	override protected string Key() { return "ar.MaxRange"; }
	override protected float Get(CMD_ArtillerySupport c) { return c.DCO_GT_MaxRange(); }
	override protected void Set(CMD_ArtillerySupport c, float v) { c.DCO_ST_MaxRange(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AR_LethalRadiusAttribute : DCO_ArtyTuneFloatAttribute
{
	override protected string Key() { return "ar.LethalRadius"; }
	override protected float Get(CMD_ArtillerySupport c) { return c.DCO_GT_LethalRadius(); }
	override protected void Set(CMD_ArtillerySupport c, float v) { c.DCO_ST_LethalRadius(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AR_DangerCloseRadiusAttribute : DCO_ArtyTuneFloatAttribute
{
	override protected string Key() { return "ar.DangerCloseRadius"; }
	override protected float Get(CMD_ArtillerySupport c) { return c.DCO_GT_DangerCloseRadius(); }
	override protected void Set(CMD_ArtillerySupport c, float v) { c.DCO_ST_DangerCloseRadius(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AR_MinDispersionAttribute : DCO_ArtyTuneFloatAttribute
{
	override protected string Key() { return "ar.MinDispersion"; }
	override protected float Get(CMD_ArtillerySupport c) { return c.DCO_GT_MinDispersion(); }
	override protected void Set(CMD_ArtillerySupport c, float v) { c.DCO_ST_MinDispersion(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AR_MaxDispersionAttribute : DCO_ArtyTuneFloatAttribute
{
	override protected string Key() { return "ar.MaxDispersion"; }
	override protected float Get(CMD_ArtillerySupport c) { return c.DCO_GT_MaxDispersion(); }
	override protected void Set(CMD_ArtillerySupport c, float v) { c.DCO_ST_MaxDispersion(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AR_DispersionRangeScaleAttribute : DCO_ArtyTuneFloatAttribute
{
	override protected string Key() { return "ar.DispersionRangeScale"; }
	override protected float Get(CMD_ArtillerySupport c) { return c.DCO_GT_DispersionRangeScale(); }
	override protected void Set(CMD_ArtillerySupport c, float v) { c.DCO_ST_DispersionRangeScale(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AR_ClusterRatioAttribute : DCO_ArtyTuneFloatAttribute
{
	override protected string Key() { return "ar.ClusterRatio"; }
	override protected float Get(CMD_ArtillerySupport c) { return c.DCO_GT_ClusterRatio(); }
	override protected void Set(CMD_ArtillerySupport c, float v) { c.DCO_ST_ClusterRatio(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AR_CorrectionRadiusAttribute : DCO_ArtyTuneFloatAttribute
{
	override protected string Key() { return "ar.CorrectionRadius"; }
	override protected float Get(CMD_ArtillerySupport c) { return c.DCO_GT_CorrectionRadius(); }
	override protected void Set(CMD_ArtillerySupport c, float v) { c.DCO_ST_CorrectionRadius(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AR_CorrectionExpiryAttribute : DCO_ArtyTuneFloatAttribute
{
	override protected string Key() { return "ar.CorrectionExpiry"; }
	override protected float Get(CMD_ArtillerySupport c) { return c.DCO_GT_CorrectionExpiry(); }
	override protected void Set(CMD_ArtillerySupport c, float v) { c.DCO_ST_CorrectionExpiry(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AR_MaxCorrectionsAttribute : DCO_ArtyTuneFloatAttribute
{
	override protected string Key() { return "ar.MaxCorrections"; }
	override protected float Get(CMD_ArtillerySupport c) { return c.DCO_GT_MaxCorrections(); }
	override protected void Set(CMD_ArtillerySupport c, float v) { c.DCO_ST_MaxCorrections(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AR_CorrectionBonusPerStepAttribute : DCO_ArtyTuneFloatAttribute
{
	override protected string Key() { return "ar.CorrectionBonusPerStep"; }
	override protected float Get(CMD_ArtillerySupport c) { return c.DCO_GT_CorrectionBonusPerStep(); }
	override protected void Set(CMD_ArtillerySupport c, float v) { c.DCO_ST_CorrectionBonusPerStep(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_BaseValueAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.BaseValue"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_BaseValue(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_BaseValue(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_MaxGroupCountAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.MaxGroupCount"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_MaxGroupCount(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_MaxGroupCount(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_DefendGroupCountAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.DefendGroupCount"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_DefendGroupCount(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_DefendGroupCount(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_ThreatRadiusAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.ThreatRadius"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_ThreatRadius(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_ThreatRadius(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_FriendlyRadiusAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.FriendlyRadius"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_FriendlyRadius(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_FriendlyRadius(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_IntelCoverageRadiusAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.IntelCoverageRadius"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_IntelCoverageRadius(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_IntelCoverageRadius(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_CaptureHoldDurationAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.CaptureHoldDuration"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_CaptureHoldDuration(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_CaptureHoldDuration(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_UncontestedDwellAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.UncontestedDwell"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_UncontestedDwell(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_UncontestedDwell(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_PresenceScanIntervalAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.PresenceScanInterval"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_PresenceScanInterval(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_PresenceScanInterval(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_FullSpeedUnitsAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.FullSpeedUnits"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_FullSpeedUnits(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_FullSpeedUnits(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_OwnershipGainThresholdAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.OwnershipGainThreshold"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_OwnershipGainThreshold(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_OwnershipGainThreshold(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_OwnershipLossThresholdAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.OwnershipLossThreshold"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_OwnershipLossThreshold(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_OwnershipLossThreshold(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_EmptyDecayRateAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.EmptyDecayRate"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_EmptyDecayRate(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_EmptyDecayRate(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_AllowAllFactionsAttribute : DCO_ObjTuneBoolAttribute
{
	override protected string Key() { return "obj.AllowAllFactions"; }
	override protected bool Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_AllowAllFactions(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, bool v) { c.DCO_ST_AllowAllFactions(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_StalematePercentThresholdAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.StalematePercentThreshold"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_StalematePercentThreshold(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_StalematePercentThreshold(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_StalemateSecondsAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.StalemateSeconds"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_StalemateSeconds(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_StalemateSeconds(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_IdleTimeoutAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.IdleTimeout"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_IdleTimeout(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_IdleTimeout(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_MaxRelevantDistanceAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.MaxRelevantDistance"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_MaxRelevantDistance(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_MaxRelevantDistance(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_ProximityWeightAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.ProximityWeight"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_ProximityWeight(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_ProximityWeight(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_DistantFalloffWeightAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.DistantFalloffWeight"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_DistantFalloffWeight(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_DistantFalloffWeight(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_ThreatWeightAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.ThreatWeight"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_ThreatWeight(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_ThreatWeight(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_ThreatFalloffPowerAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.ThreatFalloffPower"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_ThreatFalloffPower(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_ThreatFalloffPower(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_ThreatFalloffFloorAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.ThreatFalloffFloor"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_ThreatFalloffFloor(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_ThreatFalloffFloor(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_OBJ_ThreatCountCapAttribute : DCO_ObjTuneFloatAttribute
{
	override protected string Key() { return "obj.ThreatCountCap"; }
	override protected float Get(CMD_AICommanderObjectiveComponent c) { return c.DCO_GT_ThreatCountCap(); }
	override protected void Set(CMD_AICommanderObjectiveComponent c, float v) { c.DCO_ST_ThreatCountCap(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_GC_CohessionDistanceAttribute : DCO_GroupTuneFloatAttribute
{
	override protected string Key() { return "gc.CohessionDistance"; }
	override protected float Get(DCO_GroupConfigComponent c) { return c.DCO_GT_CohessionDistance(); }
	override protected void Set(DCO_GroupConfigComponent c, float v) { c.DCO_ST_CohessionDistance(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_GC_CohessionDistanceMultAttribute : DCO_GroupTuneFloatAttribute
{
	override protected string Key() { return "gc.CohessionDistanceMult"; }
	override protected float Get(DCO_GroupConfigComponent c) { return c.DCO_GT_CohessionDistanceMult(); }
	override protected void Set(DCO_GroupConfigComponent c, float v) { c.DCO_ST_CohessionDistanceMult(v); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TM_CqbRangeAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "tm.cqb_range_m"; }
	override protected float Def() { return 25.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TM_CqbChanceAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "tm.cqb_chance"; }
	override protected float Def() { return 0.6; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_VA_HideSoftAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "va.hide_soft_m"; }
	override protected float Def() { return 120.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_VA_HideApcAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "va.hide_apc_m"; }
	override protected float Def() { return 80.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_VA_HideTankAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "va.hide_tank_m"; }
	override protected float Def() { return 50.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_VA_HideMinAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "va.hide_min_s"; }
	override protected float Def() { return 20.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_VA_HideMaxAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "va.hide_max_s"; }
	override protected float Def() { return 40.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_VA_ScootSoftAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "va.scoot_soft_m"; }
	override protected float Def() { return 300.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_VA_ScootApcAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "va.scoot_apc_m"; }
	override protected float Def() { return 250.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_VA_ScootTankAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "va.scoot_tank_m"; }
	override protected float Def() { return 200.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_VA_ExposeSoftAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "va.expose_soft_s"; }
	override protected float Def() { return 15.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_VA_ExposeApcAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "va.expose_apc_s"; }
	override protected float Def() { return 12.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_VA_ExposeTankAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "va.expose_tank_s"; }
	override protected float Def() { return 15.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_VA_WithdrawHpAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "va.withdraw_hp"; }
	override protected float Def() { return 0.35; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_VA_LeashAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "va.bound_extra_m"; }
	override protected float Def() { return 250.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_VA_ReconOnAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "va.recon_on"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_VA_ReconSoftAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "va.recon_soft_per_min"; }
	override protected float Def() { return 8.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_VA_ReconApcAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "va.recon_apc_per_min"; }
	override protected float Def() { return 5.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_VA_EnabledAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "va.enabled"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_PC_IdentifyMulAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "pc.identify_mul"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_RF_GreenAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "reinf.green"; }
	override protected float Def() { return 0.85; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_RF_AmberAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "reinf.amber"; }
	override protected float Def() { return 0.7; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_RF_RedAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "reinf.red"; }
	override protected float Def() { return 0.5; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_RF_RatioAttackAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "reinf.ratio_attack"; }
	override protected float Def() { return 2.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_RF_RatioDefendAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "reinf.ratio_defend"; }
	override protected float Def() { return 0.5; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_RF_ConfMinAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "reinf.conf_min"; }
	override protected float Def() { return 0.4; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_RF_HoldSAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "reinf.hold_s"; }
	override protected float Def() { return 60.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_RadiusScaleAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.RadiusScale"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_CandidatesPerTickScaleAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.CandidatesPerTickScale"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_MaxCandidatesScaleAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.MaxCandidatesScale"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_MaxTileWaitTicksAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.MaxTileWaitTicks"; }
	override protected float Def() { return 20.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_GoodEnoughScoreAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.GoodEnoughScore"; }
	override protected float Def() { return 0.72; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_ResultTTLAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.ResultTTL"; }
	override protected float Def() { return 15.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_CellSizeAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.CellSize"; }
	override protected float Def() { return 2.5; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_CellHeightAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.CellHeight"; }
	override protected float Def() { return 3.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_DedupeRadiusAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.DedupeRadius"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_TopCandidatesAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.TopCandidates"; }
	override protected float Def() { return 3.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_MinHeadroomAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.MinHeadroom"; }
	override protected float Def() { return 1.6; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_OccupancyRadiusAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.OccupancyRadius"; }
	override protected float Def() { return 3.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_DoorBlockRadiusAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.DoorBlockRadius"; }
	override protected float Def() { return 1.2; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_DoorAvoidRadiusAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.DoorAvoidRadius"; }
	override protected float Def() { return 2.5; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_DoorAvoidPenaltyAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.DoorAvoidPenalty"; }
	override protected float Def() { return 0.3; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_RejectChokepointsAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "fi.RejectChokepoints"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_ChokepointWidthAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.ChokepointWidth"; }
	override protected float Def() { return 1.4; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_MinBookedDistanceAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.MinBookedDistance"; }
	override protected float Def() { return 3.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_RequireFireLineAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "fi.RequireFireLine"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_WeightCoverAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.WeightCover"; }
	override protected float Def() { return 0.35; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_WeightFireLineAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.WeightFireLine"; }
	override protected float Def() { return 0.35; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_WeightSpreadAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.WeightSpread"; }
	override protected float Def() { return 0.15; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_WeightProximityAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.WeightProximity"; }
	override protected float Def() { return 0.15; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_InteriorLineValueAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.InteriorLineValue"; }
	override protected float Def() { return 0.5; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FI_StickinessBonusAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "fi.StickinessBonus"; }
	override protected float Def() { return 0.05; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AP_UpdateInterval_msAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "ap.UpdateInterval_ms"; }
	override protected float Def() { return 500.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AP_AllowBuildingAssaultAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "ap.AllowBuildingAssault"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AP_BuildingAssaultDistAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "ap.BuildingAssaultDist"; }
	override protected float Def() { return 40.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AP_FlankChanceAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "ap.FlankChance"; }
	override protected float Def() { return 0.35; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_AIM_HeadMulAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "aim.head_mul"; }
	override protected float Def() { return 1.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_NAV_DebugAttribute : DCO_TuneFloatAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!DCO_TestCaseComponent.RunOn())
			return null;
		return super.ReadVariable(item, manager);
	}

	override protected string Key() { return "nav.debug"; }
	override protected float Def() { return 0.0; }
}


[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TM_CqbAggrInAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "tm.cqb_aggr_in"; }
	override protected float Def() { return 4.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TM_CqbAggrOutAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "tm.cqb_aggr_out"; }
	override protected float Def() { return 4.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_RF_RatioHastyAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "reinf.ratio_hasty"; }
	override protected float Def() { return 1.7; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TM_StrafeFireAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "tm.strafe_fire"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_TM_StrafeMaxAttribute : DCO_TuneFloatAttribute
{
	override protected string Key() { return "tm.strafe_max_m"; }
	override protected float Def() { return 60.0; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_FM_AutoAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "fm.auto"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_SPEC_BtAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "spec.bt"; }
	override protected bool Def() { return true; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_IS_EstShareAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "is.est_share"; }
	override protected bool Def() { return false; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_IS_DesignateAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "is.designate"; }
	override protected bool Def() { return false; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_IS_QualityAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "is.quality"; }
	override protected bool Def() { return false; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_HEAR_V2Attribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "hear.v2"; }
	override protected bool Def() { return false; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_HEAR_MaskAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "hear.mask"; }
	override protected bool Def() { return false; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_HEAR_ContactAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "hear.contact"; }
	override protected bool Def() { return false; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_HEAR_EstAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "hear.est"; }
	override protected bool Def() { return false; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_T_RT_FastAttribute : DCO_TuneBoolAttribute
{
	override protected string Key() { return "rt.fast"; }
	override protected bool Def() { return true; }
}
