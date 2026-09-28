[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_WaypointCompletionRadiusAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_EditableWaypointComponent editable = SCR_EditableWaypointComponent.Cast(item);
		if (!editable)
			return null;

		AIWaypoint wp = AIWaypoint.Cast(editable.GetOwner());
		if (!wp)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(wp.GetCompletionRadius());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		SCR_EditableWaypointComponent editable = SCR_EditableWaypointComponent.Cast(item);
		if (editable && var)
			editable.DCO_SetCompletion(Math.Max(var.GetFloat(), 0), -1);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_WaypointCompletionTypeAttribute : DCO_CommanderBaseAttribute
{
	override protected void GetPresetLabels(notnull out array<string> outLabels)
	{
		outLabels.Clear();
		outLabels.Insert("All members");
		outLabels.Insert("Leader");
		outLabels.Insert("Any member");
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_EditableWaypointComponent editable = SCR_EditableWaypointComponent.Cast(item);
		if (!editable)
			return null;

		AIWaypoint wp = AIWaypoint.Cast(editable.GetOwner());
		if (!wp)
			return null;

		return SCR_BaseEditorAttributeVar.CreateInt(wp.GetCompletionType());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		SCR_EditableWaypointComponent editable = SCR_EditableWaypointComponent.Cast(item);
		if (editable && var)
			editable.DCO_SetCompletion(-1, Math.ClampInt(var.GetInt(), 0, 2));
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjectiveRadiusAttribute : SCR_BaseValueListEditorAttribute
{
	protected CMD_AICommanderObjectiveComponent GetObjective(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable || !editable.GetOwner())
			return null;

		return CMD_AICommanderObjectiveComponent.Cast(editable.GetOwner().FindComponent(CMD_AICommanderObjectiveComponent));
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = GetObjective(item);
		if (!obj)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(obj.GetRadius());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = GetObjective(item);
		if (obj && var)
			obj.SetRadius(var.GetFloat());
	}
}

modded class SCR_EditableWaypointComponent
{
	[RplProp(onRplName: "DCO_OnCompletionRpl")]
	protected float m_fDCOCompletionRadius = -1;

	[RplProp(onRplName: "DCO_OnCompletionRpl")]
	protected int m_iDCOCompletionType = -1;

	void DCO_SetCompletion(float radius, int completionType)
	{
		if (!IsServer())
			return;

		if (radius >= 0)
			m_fDCOCompletionRadius = radius;

		if (completionType >= 0)
			m_iDCOCompletionType = completionType;

		Replication.BumpMe();
		DCO_OnCompletionRpl();
	}

	protected void DCO_OnCompletionRpl()
	{
		AIWaypoint wp = AIWaypoint.Cast(GetOwner());
		if (!wp)
			return;

		if (m_fDCOCompletionRadius >= 0)
			wp.SetCompletionRadius(m_fDCOCompletionRadius);

		if (m_iDCOCompletionType >= 0)
			wp.SetCompletionType(m_iDCOCompletionType);

		SCR_AIWaypoint scrWp = SCR_AIWaypoint.Cast(wp);
		if (scrWp)
			scrWp.GetOnWaypointPropertiesChanged().Invoke();

		SCR_BaseAreaMeshComponent mesh = SCR_BaseAreaMeshComponent.Cast(wp.FindComponent(SCR_BaseAreaMeshComponent));
		if (mesh)
			mesh.GenerateAreaMesh();
	}
}
