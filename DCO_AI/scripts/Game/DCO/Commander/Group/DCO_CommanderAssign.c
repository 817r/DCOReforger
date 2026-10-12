[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderAssignBaseAttribute : SCR_BaseFloatValueHolderEditorAttribute
{

	protected SCR_EditableEntityComponent GetValidEditable(Managed item)
	{
		return null;
	}

	protected AICommander_BaseComponent GetCurrentCommander(SCR_EditableEntityComponent editable)
	{
		return null;
	}

	protected void ApplyCommander(SCR_EditableEntityComponent editable, AICommander_BaseComponent newCmd, AICommander_BaseComponent current)
	{
	}

	protected FactionKey GetEditableFactionKey(SCR_EditableEntityComponent editable)
	{
		Faction faction = editable.GetFaction();
		if (!faction)
			return string.Empty;

		return faction.GetFactionKey();
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return null;

		SCR_EditableEntityComponent editable = GetValidEditable(item);
		if (!editable)
			return null;

		FactionKey fk = GetEditableFactionKey(editable);
		if (fk.IsEmpty())
			return null;

		int index = 0;
		AICommander_BaseComponent current = GetCurrentCommander(editable);
		if (current)
		{
			array<string> uids = {};
			mgr.GetRosterUIDsForFaction(fk, uids);

			int found = uids.Find(current.GetCommanderUID());
			if (found != -1)
				index = found + 1;
		}

		return SCR_BaseEditorAttributeVar.CreateInt(index);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;

		SCR_EditableEntityComponent editable = GetValidEditable(item);
		if (!editable)
			return;

		FactionKey fk = GetEditableFactionKey(editable);
		if (fk.IsEmpty())
			return;

		int index = var.GetInt();
		AICommander_BaseComponent current = GetCurrentCommander(editable);
		AICommander_BaseComponent target;

		if (index > 0)
		{
			array<string> uids = {};
			mgr.GetRosterUIDsForFaction(fk, uids);

			if (index > uids.Count())
			{
				DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.WARN, "-", string.Format("Index %1 is outside the roster (%2 commanders) -- cancelled", index, uids.Count()));
				return;
			}

			target = mgr.FindCommanderByUID(uids[index - 1]);
			if (!target)
			{
				DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.WARN, "-", string.Format("Commander '%1' not found -- cancelled", uids[index - 1]));
				return;
			}
		}

		if (target == current)
			return;

		ApplyCommander(editable, target, current);
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		CreateCommanderPresets();
		return super.GetEntries(outEntries);
	}

	protected void CreateCommanderPresets()
	{
		m_aValues.Clear();

		SCR_EditorAttributeFloatStringValueHolder noneValue = new SCR_EditorAttributeFloatStringValueHolder();
		noneValue.SetName("None");
		noneValue.SetFloatValue(0);
		m_aValues.Insert(noneValue);

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
		{
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.WARN, "-", "Dropdown shows only None: no commander manager on this machine yet (replication?)");
			return;
		}

		SCR_AttributesManagerEditorComponent attributesManager = SCR_AttributesManagerEditorComponent.Cast(SCR_AttributesManagerEditorComponent.GetInstance(SCR_AttributesManagerEditorComponent));
		if (!attributesManager)
			return;

		array<Managed> items = {};
		attributesManager.GetEditedItems(items);

		FactionKey fk;
		bool first = true;
		foreach (Managed item : items)
		{
			SCR_EditableEntityComponent editable = GetValidEditable(item);
			if (!editable)
				continue;

			FactionKey itemFk = GetEditableFactionKey(editable);
			if (first)
			{
				fk = itemFk;
				first = false;
			}
			else if (itemFk != fk)
			{
				return;
			}
		}

		if (fk.IsEmpty())
		{
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.WARN, "-", string.Format("Dropdown shows only None: faction item is empty (items %1)", items.Count()));
			return;
		}

		array<string> uids = {};
		mgr.GetRosterUIDsForFaction(fk, uids);
		if (uids.IsEmpty())
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.WARN, "-", string.Format("Dropdown shows only None: roster has no commander for faction %1 (roster '%2')", fk, mgr.DCO_GetRosterString()));

		for (int i = 0; i < uids.Count(); i++)
		{
			SCR_EditorAttributeFloatStringValueHolder value = new SCR_EditorAttributeFloatStringValueHolder();
			value.SetName(uids[i]);
			value.SetFloatValue(i + 1);
			m_aValues.Insert(value);
		}
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderAssignGroupAttribute : DCO_CommanderAssignBaseAttribute
{
	protected DCO_GroupUtilityComponent GetGroupUtil(SCR_EditableEntityComponent editable)
	{
		SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(editable);
		if (!editableGroup)
			return null;

		SCR_AIGroup aiGroup = editableGroup.GetAIGroupComponent();
		if (!aiGroup)
			return null;

		return DCO_GroupUtilityComponent.Cast(aiGroup.FindComponent(DCO_GroupUtilityComponent));
	}

	override protected SCR_EditableEntityComponent GetValidEditable(Managed item)
	{
		SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(item);
		if (!editableGroup)
			return null;

		if (editableGroup.HasEntityState(EEditableEntityState.PLAYER))
			return null;

		DCO_GroupUtilityComponent groupUtil = GetGroupUtil(editableGroup);
		if (!groupUtil || !groupUtil.IsCommanderEligible())
			return null;

		return editableGroup;
	}

	override protected AICommander_BaseComponent GetCurrentCommander(SCR_EditableEntityComponent editable)
	{
		DCO_GroupUtilityComponent groupUtil = GetGroupUtil(editable);
		if (!groupUtil)
			return null;

		return groupUtil.GetMyCommander();
	}

	override protected void ApplyCommander(SCR_EditableEntityComponent editable, AICommander_BaseComponent newCmd, AICommander_BaseComponent current)
	{
		DCO_GroupUtilityComponent groupUtil = GetGroupUtil(editable);
		if (!groupUtil)
			return;

		if (newCmd)
		{
			if (!newCmd.AssignGroup(groupUtil))
				DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.WARN, "-", string.Format("AssignGroup refused: %1 -> %2", groupUtil.GetOwner().GetName(), newCmd.GetCommanderUID()));
			return;
		}

		if (current)
			current.ReleaseGroup(groupUtil);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderAssignVehicleAttribute : DCO_CommanderAssignBaseAttribute
{
	protected DCO_TransportMissionComponent GetMission(SCR_EditableEntityComponent editable)
	{
		IEntity owner = editable.GetOwner();
		if (!owner)
			return null;

		return DCO_TransportMissionComponent.Cast(owner.FindComponent(DCO_TransportMissionComponent));
	}

	override protected SCR_EditableEntityComponent GetValidEditable(Managed item)
	{
		SCR_EditableVehicleComponent editableVehicle = SCR_EditableVehicleComponent.Cast(item);
		if (!editableVehicle)
			return null;

		if (!GetMission(editableVehicle))
			return null;

		return editableVehicle;
	}

	override protected FactionKey GetEditableFactionKey(SCR_EditableEntityComponent editable)
	{
		FactionKey fk = super.GetEditableFactionKey(editable);
		if (!fk.IsEmpty() || editable.IsDestroyed())
			return fk;

		SCR_VehicleFactionAffiliationComponent fac = SCR_VehicleFactionAffiliationComponent.Cast(editable.GetOwner().FindComponent(SCR_VehicleFactionAffiliationComponent));
		if (fac)
			return fac.GetDefaultFactionKey();
		return fk;
	}

	override protected AICommander_BaseComponent GetCurrentCommander(SCR_EditableEntityComponent editable)
	{
		DCO_TransportMissionComponent mission = GetMission(editable);
		if (!mission)
			return null;

		return mission.GetCommanderOwner();
	}

	override protected void ApplyCommander(SCR_EditableEntityComponent editable, AICommander_BaseComponent newCmd, AICommander_BaseComponent current)
	{
		IEntity veh = editable.GetOwner();
		if (!veh)
			return;

		if (newCmd)
		{
			if (!newCmd.AssignVehicle(veh))
				DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.WARN, "-", string.Format("AssignVehicle refused: %1 -> %2", veh.GetName(), newCmd.GetCommanderUID()));
			return;
		}

		if (current)
			current.ReleaseVehicle(veh);
	}
}