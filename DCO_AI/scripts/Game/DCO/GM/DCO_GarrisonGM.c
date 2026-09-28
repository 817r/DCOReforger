class DCO_GarrisonAttributeHelper
{
	static DCO_GarrisonWaypoint Get(Managed item)
	{
		SCR_EditableWaypointComponent editable = SCR_EditableWaypointComponent.Cast(item);
		if (!editable)
			return null;

		return DCO_GarrisonWaypoint.Cast(editable.GetOwner());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GarrisonModeAttribute : DCO_CommanderBaseAttribute
{
	override protected void GetPresetLabels(notnull out array<string> outLabels)
	{
		outLabels.Clear();
		outLabels.Insert("Hold");
		outLabels.Insert("Defend");
		outLabels.Insert("Release");
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GarrisonWaypoint wp = DCO_GarrisonAttributeHelper.Get(item);
		if (!wp)
			return null;

		return SCR_BaseEditorAttributeVar.CreateInt(wp.GetMode());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GarrisonWaypoint wp = DCO_GarrisonAttributeHelper.Get(item);
		if (wp && var)
			wp.SetMode(Math.ClampInt(var.GetInt(), 0, 2));
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GarrisonFillOrderAttribute : DCO_CommanderBaseAttribute
{
	override protected void GetPresetLabels(notnull out array<string> outLabels)
	{
		outLabels.Clear();
		outLabels.Insert("One building at a time");
		outLabels.Insert("All windows first");
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GarrisonWaypoint wp = DCO_GarrisonAttributeHelper.Get(item);
		if (!wp)
			return null;

		return SCR_BaseEditorAttributeVar.CreateInt(wp.GetFillOrder());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GarrisonWaypoint wp = DCO_GarrisonAttributeHelper.Get(item);
		if (wp && var)
			wp.SetFillOrder(Math.ClampInt(var.GetInt(), 0, 1));
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GarrisonFloatAttribute : SCR_BaseValueListEditorAttribute
{
	protected float Get(DCO_GarrisonWaypoint wp) { return 0; }
	protected void Set(DCO_GarrisonWaypoint wp, float value) {}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GarrisonWaypoint wp = DCO_GarrisonAttributeHelper.Get(item);
		if (!wp)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(Get(wp));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GarrisonWaypoint wp = DCO_GarrisonAttributeHelper.Get(item);
		if (wp && var)
			Set(wp, var.GetFloat());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GarrisonBoolAttribute : SCR_BaseEditorAttribute
{
	protected bool Get(DCO_GarrisonWaypoint wp) { return false; }
	protected void Set(DCO_GarrisonWaypoint wp, bool value) {}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GarrisonWaypoint wp = DCO_GarrisonAttributeHelper.Get(item);
		if (!wp)
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(Get(wp));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GarrisonWaypoint wp = DCO_GarrisonAttributeHelper.Get(item);
		if (wp && var)
			Set(wp, var.GetBool());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GarrisonFillRatioAttribute : DCO_GarrisonFloatAttribute
{
	override protected float Get(DCO_GarrisonWaypoint wp) { return wp.GetFillRatio(); }
	override protected void Set(DCO_GarrisonWaypoint wp, float value) { wp.SetFillRatio(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GarrisonMaxBuildingsAttribute : DCO_GarrisonFloatAttribute
{
	override protected float Get(DCO_GarrisonWaypoint wp) { return wp.GetMaxBuildings(); }
	override protected void Set(DCO_GarrisonWaypoint wp, float value) { wp.SetMaxBuildings(Math.Round(value)); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GarrisonThreatWeightAttribute : DCO_GarrisonFloatAttribute
{
	override protected float Get(DCO_GarrisonWaypoint wp) { return wp.GetThreatWeight(); }
	override protected void Set(DCO_GarrisonWaypoint wp, float value) { wp.SetThreatWeight(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GarrisonRefillDelayAttribute : DCO_GarrisonFloatAttribute
{
	override protected float Get(DCO_GarrisonWaypoint wp) { return wp.GetRefillDelayMax(); }
	override protected void Set(DCO_GarrisonWaypoint wp, float value) { wp.SetRefillDelayMax(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GarrisonAllowRoofAttribute : DCO_GarrisonBoolAttribute
{
	override protected bool Get(DCO_GarrisonWaypoint wp) { return wp.GetAllowRoof(); }
	override protected void Set(DCO_GarrisonWaypoint wp, bool value) { wp.SetAllowRoof(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GarrisonPullLoneAttribute : DCO_GarrisonBoolAttribute
{
	override protected bool Get(DCO_GarrisonWaypoint wp) { return wp.GetPullLoneToMain(); }
	override protected void Set(DCO_GarrisonWaypoint wp, bool value) { wp.SetPullLoneToMain(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GarrisonDebugDrawAttribute : DCO_GarrisonBoolAttribute
{
	override protected bool Get(DCO_GarrisonWaypoint wp) { return wp.GetDebugDraw(); }
	override protected void Set(DCO_GarrisonWaypoint wp, bool value) { wp.SetDebugDraw(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GarrisonDefendLeashAttribute : DCO_GarrisonFloatAttribute
{
	override protected float Get(DCO_GarrisonWaypoint wp) { return wp.GetDefendLeashExtra(); }
	override protected void Set(DCO_GarrisonWaypoint wp, float value) { wp.SetDefendLeashExtra(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GarrisonFlankTeamAttribute : DCO_GarrisonFloatAttribute
{
	override protected float Get(DCO_GarrisonWaypoint wp) { return wp.GetFlankTeamSize(); }
	override protected void Set(DCO_GarrisonWaypoint wp, float value) { wp.SetFlankTeamSize(Math.Round(value)); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GarrisonReleaseDistAttribute : DCO_GarrisonFloatAttribute
{
	override protected float Get(DCO_GarrisonWaypoint wp) { return wp.GetReleaseEnemyDist(); }
	override protected void Set(DCO_GarrisonWaypoint wp, float value) { wp.SetReleaseEnemyDist(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GarrisonReleaseCasualtiesAttribute : DCO_GarrisonFloatAttribute
{
	override protected float Get(DCO_GarrisonWaypoint wp) { return wp.GetReleaseCasualtyPct(); }
	override protected void Set(DCO_GarrisonWaypoint wp, float value) { wp.SetReleaseCasualtyPct(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GarrisonReleaseSuppressedAttribute : DCO_GarrisonFloatAttribute
{
	override protected float Get(DCO_GarrisonWaypoint wp) { return wp.GetReleaseSuppressedTime(); }
	override protected void Set(DCO_GarrisonWaypoint wp, float value) { wp.SetReleaseSuppressedTime(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GarrisonReturnAfterReleaseAttribute : DCO_GarrisonBoolAttribute
{
	override protected bool Get(DCO_GarrisonWaypoint wp) { return wp.GetReturnAfterRelease(); }
	override protected void Set(DCO_GarrisonWaypoint wp, bool value) { wp.SetReturnAfterRelease(value); }
}
