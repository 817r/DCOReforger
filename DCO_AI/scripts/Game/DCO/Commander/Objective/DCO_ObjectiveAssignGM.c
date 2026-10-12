class DCO_ObjAssign
{
	protected static int s_iCapLogCmd;
	protected static int s_iCapLogObj;

	static void CapLog(string kind, int total)
	{
		if (kind == "commanders")
		{
			if (s_iCapLogCmd == total)
				return;
			s_iCapLogCmd = total;
		}
		else
		{
			if (s_iCapLogObj == total)
				return;
			s_iCapLogObj = total;
		}
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.WARN, "-", string.Format("obj_assign_list_cap kind=%1 total=%2 shown=%3", kind, total, Cap()));
	}

	static string RosterKey()
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return string.Empty;
		array<string> names = {};
		mgr.DCO_GetObjectiveRoster(names);
		string s = mgr.DCO_GetRosterString() + "#";
		foreach (string n : names)
			s += n + ";";
		return s;
	}

	static bool Stale(string readKey, string what)
	{
		if (readKey.IsEmpty() || readKey == RosterKey())
			return false;
		DCO_Perf.Count("obj_assign:stale");
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.WARN, "-", string.Format("obj_assign_stale what=%1 -- commander/objective list changed while the dialog was open; change ignored, reopen the dialog", what));
		return true;
	}

	static void MergeChecked(notnull array<string> oldList, notnull array<string> uids, int mask, notnull array<string> outList)
	{
		outList.Clear();
		foreach (string o : oldList)
		{
			if (!uids.Contains(o) && !outList.Contains(o))
				outList.Insert(o);
		}
		foreach (int i, string uid : uids)
		{
			if ((mask & Bit(i)) && !outList.Contains(uid))
				outList.Insert(uid);
		}
	}

	static string GmName(CMD_AICommanderObjectiveComponent o)
	{
		if (!o)
			return "none";
		string n = o.GetDisplayName();
		if (n.IsEmpty())
			n = o.GetLogName();
		return n;
	}

	static int Bit(int i)
	{
		int b = 1;
		for (int k = 0; k < i; k++)
			b = b * 2;
		return b;
	}

	static int Cap()
	{
		return 24;
	}

	static CMD_AICommanderObjectiveComponent Obj(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable || !editable.GetOwner())
			return null;
		return CMD_AICommanderObjectiveComponent.Cast(editable.GetOwner().FindComponent(CMD_AICommanderObjectiveComponent));
	}

	static void Commanders(notnull array<string> outUids, notnull array<FactionKey> outFactions)
	{
		outUids.Clear();
		outFactions.Clear();
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;
		array<string> uids = {};
		array<FactionKey> factions = {};
		mgr.DCO_GetRoster(uids, factions);
		foreach (int i, string uid : uids)
		{
			if (outUids.Count() >= Cap())
			{
				CapLog("commanders", uids.Count());
				break;
			}
			outUids.Insert(uid);
			outFactions.Insert(factions[i]);
		}
	}

	static void Objectives(notnull array<string> outNames)
	{
		outNames.Clear();
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;
		array<string> names = {};
		mgr.DCO_GetObjectiveRoster(names);
		foreach (string n : names)
		{
			if (outNames.Count() >= Cap())
			{
				CapLog("objectives", names.Count());
				break;
			}
			outNames.Insert(n);
		}
	}

	static void FillCommanderEntries(notnull array<ref SCR_EditorAttributeFloatStringValueHolder> values)
	{
		values.Clear();
		array<string> uids = {};
		array<FactionKey> factions = {};
		Commanders(uids, factions);
		foreach (int i, string uid : uids)
		{
			SCR_EditorAttributeFloatStringValueHolder v = new SCR_EditorAttributeFloatStringValueHolder();
			v.SetName(uid + " · " + factions[i]);
			v.SetFloatValue(Bit(i));
			values.Insert(v);
		}
	}

	static void FillObjectiveEntries(notnull array<ref SCR_EditorAttributeFloatStringValueHolder> values, bool withNone)
	{
		values.Clear();
		int offset = 0;
		if (withNone)
		{
			SCR_EditorAttributeFloatStringValueHolder none = new SCR_EditorAttributeFloatStringValueHolder();
			none.SetName("None");
			none.SetFloatValue(0);
			values.Insert(none);
			offset = 1;
		}
		array<string> names = {};
		Objectives(names);
		foreach (int i, string n : names)
		{
			SCR_EditorAttributeFloatStringValueHolder v = new SCR_EditorAttributeFloatStringValueHolder();
			v.SetName(n);
			if (withNone)
				v.SetFloatValue(i + offset);
			else
				v.SetFloatValue(Bit(i));
			values.Insert(v);
		}
	}

	static bool SetFocusObjective(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, bool force)
	{
		if (!cmd)
			return false;
		return cmd.SetFocusObjective(obj, force, "script");
	}

	static void ClearFocus(AICommander_BaseComponent cmd)
	{
		if (cmd)
			cmd.ClearFocus("script", "script");
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjHandledByAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (!obj)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(obj.DCO_AssignModeEff());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (!obj || !var)
			return;
		array<string> list = {};
		obj.DCO_GetAssignListEff(list);
		obj.DCO_SetAssign(var.GetInt(), list, "gm", playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjCommandersAttribute : SCR_BaseMultiSelectPresetsEditorAttribute
{
	protected string m_sReadKey;

	override protected void CreatePresets()
	{
		DCO_ObjAssign.FillCommanderEntries(m_aValues);
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (!obj)
			return null;
		m_sReadKey = DCO_ObjAssign.RosterKey();
		array<string> list = {};
		obj.DCO_GetAssignListEff(list);
		array<string> uids = {};
		array<FactionKey> factions = {};
		DCO_ObjAssign.Commanders(uids, factions);
		int mask = 0;
		foreach (int i, string uid : uids)
		{
			if (list.Contains(uid))
				mask |= DCO_ObjAssign.Bit(i);
		}
		return SCR_BaseEditorAttributeVar.CreateInt(mask);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (!obj || !var)
			return;
		int mask = var.GetInt();
		array<string> uids = {};
		array<FactionKey> factions = {};
		DCO_ObjAssign.Commanders(uids, factions);
		if (DCO_ObjAssign.Stale(m_sReadKey, "commanders"))
			return;
		array<string> old = {};
		obj.DCO_GetAssignListEff(old);
		array<string> list = {};
		DCO_ObjAssign.MergeChecked(old, uids, mask, list);
		obj.DCO_SetAssign(obj.DCO_AssignModeEff(), list, "gm", playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjFocusForAttribute : SCR_BaseMultiSelectPresetsEditorAttribute
{
	protected string m_sReadKey;

	override protected void CreatePresets()
	{
		DCO_ObjAssign.FillCommanderEntries(m_aValues);
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!obj || !mgr)
			return null;
		m_sReadKey = DCO_ObjAssign.RosterKey();
		array<string> uids = {};
		array<FactionKey> factions = {};
		DCO_ObjAssign.Commanders(uids, factions);
		int mask = 0;
		foreach (int i, string uid : uids)
		{
			AICommander_BaseComponent cmd = mgr.FindCommanderByUID(uid);
			if (cmd && cmd.DCO_GetFocus() == obj)
				mask |= DCO_ObjAssign.Bit(i);
		}
		return SCR_BaseEditorAttributeVar.CreateInt(mask);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!obj || !mgr || !var)
			return;
		int mask = var.GetInt();
		array<string> uids = {};
		array<FactionKey> factions = {};
		DCO_ObjAssign.Commanders(uids, factions);
		if (DCO_ObjAssign.Stale(m_sReadKey, "focus_for"))
			return;
		foreach (int i, string uid : uids)
		{
			AICommander_BaseComponent cmd = mgr.FindCommanderByUID(uid);
			if (!cmd)
				continue;
			bool want = (mask & DCO_ObjAssign.Bit(i)) != 0;
			if (want && cmd.DCO_GetFocus() != obj)
				cmd.SetFocusObjective(obj, cmd.DCO_FocusForced(), "gm", playerID);
			else if (!want && cmd.DCO_GetFocus() == obj)
				cmd.ClearFocus("gm_unchecked", "gm", playerID);
		}
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjForceFocusAttribute : SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!obj || !mgr)
			return null;
		bool forced = false;
		foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
		{
			if (cmd && cmd.DCO_IsForcedFocus(obj))
				forced = true;
		}
		return SCR_BaseEditorAttributeVar.CreateBool(forced);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!obj || !mgr || !var)
			return;
		foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
		{
			if (cmd && cmd.DCO_GetFocus() == obj)
				cmd.DCO_SetFocusForce(var.GetBool(), "gm", playerID);
		}
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdFocusObjectiveAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	protected string m_sReadKey;

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!cmd || !mgr)
			return null;
		m_sReadKey = DCO_ObjAssign.RosterKey();
		int index = mgr.DCO_ObjectiveIndex(cmd.DCO_GetFocus());
		if (index >= DCO_ObjAssign.Cap())
			index = -1;
		return SCR_BaseEditorAttributeVar.CreateInt(index + 1);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!cmd || !mgr || !var)
			return;
		if (DCO_ObjAssign.Stale(m_sReadKey, "focus_objective"))
			return;
		int index = var.GetInt() - 1;
		if (index < 0)
		{
			cmd.ClearFocus("gm_none", "gm", playerID);
			return;
		}
		CMD_AICommanderObjectiveComponent obj = mgr.DCO_ObjectiveAt(index);
		if (obj && obj != cmd.DCO_GetFocus())
			cmd.SetFocusObjective(obj, cmd.DCO_FocusForced(), "gm", playerID);
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		DCO_ObjAssign.FillObjectiveEntries(m_aValues, true);
		return super.GetEntries(outEntries);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdForceFocusAttribute : SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!cmd)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(cmd.DCO_FocusForced());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (cmd && var)
			cmd.DCO_SetFocusForce(var.GetBool(), "gm", playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdBlockedObjectivesAttribute : SCR_BaseMultiSelectPresetsEditorAttribute
{
	protected string m_sReadKey;

	override protected void CreatePresets()
	{
		DCO_ObjAssign.FillObjectiveEntries(m_aValues, false);
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!cmd || !mgr)
			return null;
		array<string> names = {};
		DCO_ObjAssign.Objectives(names);
		int mask = 0;
		foreach (int i, string n : names)
		{
			CMD_AICommanderObjectiveComponent obj = mgr.DCO_ObjectiveAt(i);
			if (obj && obj.IsCommanderBlackListed(cmd.GetCommanderUID()))
				mask |= DCO_ObjAssign.Bit(i);
		}
		m_sReadKey = DCO_ObjAssign.RosterKey();
		return SCR_BaseEditorAttributeVar.CreateInt(mask);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!cmd || !mgr || !var)
			return;
		if (DCO_ObjAssign.Stale(m_sReadKey, "blocked_objectives"))
			return;
		string uid = cmd.GetCommanderUID();
		int mask = var.GetInt();
		array<string> names = {};
		DCO_ObjAssign.Objectives(names);
		foreach (int i, string n : names)
		{
			CMD_AICommanderObjectiveComponent obj = mgr.DCO_ObjectiveAt(i);
			if (!obj)
				continue;
			bool want = (mask & DCO_ObjAssign.Bit(i)) != 0;
			bool blocked = obj.IsCommanderBlackListed(uid);
			if (want == blocked)
				continue;
			int mode = obj.DCO_AssignModeEff();
			array<string> list = {};
			obj.DCO_GetAssignListEff(list);
			if (want)
			{
				if (mode == 1)
					list.RemoveItem(uid);
				else
				{
					mode = 2;
					if (!list.Contains(uid))
						list.Insert(uid);
				}
			}
			else
			{
				if (mode == 1)
				{
					if (!list.Contains(uid))
						list.Insert(uid);
				}
				else
				{
					list.RemoveItem(uid);
					if (list.IsEmpty())
						mode = 0;
				}
			}
			obj.DCO_SetAssign(mode, list, "gm_commander_tab", playerID);
		}
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjRoleBaseAttribute : SCR_BaseMultiSelectPresetsEditorAttribute
{
	protected int Role()
	{
		return 1;
	}

	override protected void CreatePresets()
	{
		DCO_ObjAssign.FillCommanderEntries(m_aValues);
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (!obj)
			return null;
		array<string> list = {};
		obj.DCO_GetRoleList(Role(), list);
		array<string> uids = {};
		array<FactionKey> factions = {};
		DCO_ObjAssign.Commanders(uids, factions);
		int mask = 0;
		foreach (int i, string uid : uids)
		{
			if (list.Contains(uid))
				mask |= DCO_ObjAssign.Bit(i);
		}
		return SCR_BaseEditorAttributeVar.CreateInt(mask);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (!obj || !var)
			return;
		int mask = var.GetInt();
		array<string> uids = {};
		array<FactionKey> factions = {};
		DCO_ObjAssign.Commanders(uids, factions);
		array<string> old = {};
		obj.DCO_GetRoleList(Role(), old);
		array<string> list = {};
		DCO_ObjAssign.MergeChecked(old, uids, mask, list);
		obj.DCO_SetRoleList(Role(), list, "gm", playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjAttackOnlyAttribute : DCO_ObjRoleBaseAttribute
{
	override protected int Role()
	{
		return 1;
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjDefendOnlyAttribute : DCO_ObjRoleBaseAttribute
{
	override protected int Role()
	{
		return 2;
	}
}
