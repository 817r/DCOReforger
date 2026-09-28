[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjectiveControllerAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	protected static CMD_AICommanderObjectiveComponent GetObjective(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable || !editable.GetOwner())
			return null;
		return CMD_AICommanderObjectiveComponent.Cast(editable.GetOwner().FindComponent(CMD_AICommanderObjectiveComponent));
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = GetObjective(item);
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!obj || !mgr)
			return null;

		array<string> uids = {};
		array<FactionKey> factions = {};
		mgr.DCO_GetRoster(uids, factions);
		int index = uids.Find(obj.GetOwnerCommanderUID()) + 1;
		if (index == 0 && !obj.GetOwnerFaction().IsEmpty())
			index = factions.Find(obj.GetOwnerFaction()) + 1;
		return SCR_BaseEditorAttributeVar.CreateInt(index);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = GetObjective(item);
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!obj || !mgr || !var)
			return;

		array<string> uids = {};
		array<FactionKey> factions = {};
		mgr.DCO_GetRoster(uids, factions);
		int index = var.GetInt() - 1;
		if (index < 0)
		{
			obj.DCO_SetController(null);
			return;
		}
		if (index >= uids.Count())
			return;

		AICommander_BaseComponent cmd = mgr.FindCommanderByUID(uids[index]);
		if (cmd)
			obj.DCO_SetController(cmd);
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		m_aValues.Clear();
		SCR_EditorAttributeFloatStringValueHolder neutral = new SCR_EditorAttributeFloatStringValueHolder();
		neutral.SetName("Neutral");
		neutral.SetFloatValue(0);
		m_aValues.Insert(neutral);

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
		{
			array<string> uids = {};
			array<FactionKey> factions = {};
			mgr.DCO_GetRoster(uids, factions);
			foreach (int i, string uid : uids)
			{
				SCR_EditorAttributeFloatStringValueHolder v = new SCR_EditorAttributeFloatStringValueHolder();
				v.SetName(string.Format("%1 (%2)", uid, factions[i]));
				v.SetFloatValue(i + 1);
				m_aValues.Insert(v);
			}
		}
		return super.GetEntries(outEntries);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderObjectivesAtOnceAttribute : DCO_CommanderSupportBaseAttribute
{
	override protected bool Read(AICommander_BaseComponent cmd, out float value) { value = cmd.GetObjectivesAtOnce(); return true; }
	override protected void Write(AICommander_BaseComponent cmd, float value) { cmd.SetObjectivesAtOnce(value); }
}
