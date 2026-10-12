modded class CMD_AICommanderObjectiveComponent
{
	void DCO_SetObjectiveType(CMD_EObjectiveType t, string by, int playerID)
	{
		if (t == m_eObjectiveType)
			return;
		CMD_EObjectiveType old = m_eObjectiveType;
		m_eObjectiveType = t;
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("obj_fields_set obj=%1 field=type value=%2 old=%3 by=%4 player=%5",
			GetLogName(), typename.EnumToString(CMD_EObjectiveType, t), typename.EnumToString(CMD_EObjectiveType, old), by, playerID));
	}

	void DCO_SetLinkedSecondaries(notnull array<string> names, string by, int playerID)
	{
		if (!m_aDCOLinkedSecondaries)
			m_aDCOLinkedSecondaries = {};
		m_aDCOLinkedSecondaries.Copy(names);
		string joined;
		foreach (string n : names)
		{
			if (!joined.IsEmpty())
				joined += ",";
			joined += n;
		}
		if (joined.IsEmpty())
			joined = "none";
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("obj_fields_set obj=%1 field=secondaries value=%2 primary=%3 by=%4 player=%5",
			GetLogName(), joined, DCO_IsPrimary(), by, playerID));
	}
}

class DCO_ObjFields
{
	static string Key(CMD_AICommanderObjectiveComponent o)
	{
		if (!o || !o.GetOwner())
			return string.Empty;
		string n = o.GetOwner().GetName();
		if (!n.IsEmpty())
			return n;
		return o.GetLogName();
	}

	static bool Matches(CMD_AICommanderObjectiveComponent o, string rosterName, string stored)
	{
		if (stored.IsEmpty())
			return false;
		if (stored == rosterName)
			return true;
		return o && (stored == Key(o) || stored == o.GetLogName());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjTypeAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (!obj)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(obj.GetObjectiveType());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (!obj || !var)
			return;
		int t = Math.ClampInt(var.GetInt(), CMD_EObjectiveType.CAPTURE, CMD_EObjectiveType.HOLD_UNTIL);
		obj.DCO_SetObjectiveType(t, "gm", playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjLinkedSecondariesAttribute : SCR_BaseMultiSelectPresetsEditorAttribute
{
	override protected void CreatePresets()
	{
		DCO_ObjAssign.FillObjectiveEntries(m_aValues, false);
		CMD_AICommanderObjectiveComponent self = EditedObjective();
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!self || !mgr || !self.GetOwner())
			return;
		vector sp = self.GetOwner().GetOrigin();
		foreach (int i, SCR_EditorAttributeFloatStringValueHolder v : m_aValues)
		{
			CMD_AICommanderObjectiveComponent o = mgr.DCO_ObjectiveAt(i);
			if (!v || !o || !o.GetOwner())
				continue;
			if (o == self)
				v.SetName(v.GetName() + " · this objective");
			else
				v.SetName(string.Format("%1 · %2 m", v.GetName(), Math.Round(vector.DistanceXZ(sp, o.GetOwner().GetOrigin()))));
		}
	}

	protected CMD_AICommanderObjectiveComponent EditedObjective()
	{
		SCR_AttributesManagerEditorComponent am = SCR_AttributesManagerEditorComponent.Cast(SCR_AttributesManagerEditorComponent.GetInstance(SCR_AttributesManagerEditorComponent));
		if (!am)
			return null;
		array<Managed> items = {};
		am.GetEditedItems(items);
		if (items.Count() != 1)
			return null;
		return DCO_ObjAssign.Obj(items[0]);
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!obj || !mgr)
			return null;
		array<string> linked = obj.DCO_LinkedSecondaries();
		array<string> names = {};
		DCO_ObjAssign.Objectives(names);
		int mask = 0;
		if (linked)
		{
			foreach (int i, string rn : names)
			{
				CMD_AICommanderObjectiveComponent o = mgr.DCO_ObjectiveAt(i);
				foreach (string s : linked)
				{
					if (DCO_ObjFields.Matches(o, rn, s))
					{
						mask |= DCO_ObjAssign.Bit(i);
						break;
					}
				}
			}
		}
		return SCR_BaseEditorAttributeVar.CreateInt(mask);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!obj || !var || !mgr)
			return;
		int mask = var.GetInt();
		array<string> names = {};
		DCO_ObjAssign.Objectives(names);
		array<string> list = {};
		array<string> old = obj.DCO_LinkedSecondaries();
		if (old)
		{
			foreach (string s : old)
			{
				bool shown = false;
				foreach (int j, string rn : names)
				{
					if (DCO_ObjFields.Matches(mgr.DCO_ObjectiveAt(j), rn, s))
					{
						shown = true;
						break;
					}
				}
				if (!shown)
					list.Insert(s);
			}
		}
		foreach (int i, string rn : names)
		{
			if (!(mask & DCO_ObjAssign.Bit(i)))
				continue;
			CMD_AICommanderObjectiveComponent o = mgr.DCO_ObjectiveAt(i);
			if (!o || o == obj)
			{
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("obj_fields_skip obj=%1 field=secondaries entry=%2 reason=%3", obj.GetLogName(), rn, "self_or_missing"));
				continue;
			}
			list.Insert(DCO_ObjFields.Key(o));
		}
		obj.DCO_SetLinkedSecondaries(list, "gm", playerID);
	}
}
