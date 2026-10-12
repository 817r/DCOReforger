class DCO_DoCQBAttributeHelper
{
	static DCO_DoCQBWaypoint Get(Managed item)
	{
		SCR_EditableWaypointComponent editable = SCR_EditableWaypointComponent.Cast(item);
		if (!editable)
			return null;

		return DCO_DoCQBWaypoint.Cast(editable.GetOwner());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DoCQBFloatAttribute : SCR_BaseValueListEditorAttribute
{
	protected float Get(DCO_DoCQBWaypoint wp) { return 0; }
	protected void Set(DCO_DoCQBWaypoint wp, float value) {}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_DoCQBWaypoint wp = DCO_DoCQBAttributeHelper.Get(item);
		if (!wp)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(Get(wp));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_DoCQBWaypoint wp = DCO_DoCQBAttributeHelper.Get(item);
		if (wp && var)
			Set(wp, var.GetFloat());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DoCQBAttemptsAttribute : DCO_DoCQBFloatAttribute
{
	override protected float Get(DCO_DoCQBWaypoint wp) { return wp.GetMaxAttempts(); }
	override protected void Set(DCO_DoCQBWaypoint wp, float value) { wp.SetMaxAttempts(Math.Round(value)); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DoCQBTargetRadiusAttribute : DCO_DoCQBFloatAttribute
{
	override protected float Get(DCO_DoCQBWaypoint wp) { return wp.GetTargetRadius(); }
	override protected void Set(DCO_DoCQBWaypoint wp, float value) { wp.SetTargetRadius(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DoCQBHoldAfterClearAttribute : SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_DoCQBWaypoint wp = DCO_DoCQBAttributeHelper.Get(item);
		if (!wp)
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(wp.GetHoldAfterClear());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_DoCQBWaypoint wp = DCO_DoCQBAttributeHelper.Get(item);
		if (wp && var)
			wp.SetHoldAfterClear(var.GetBool());
	}
}
