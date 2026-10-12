[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjTypeTimeAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (!obj)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(obj.DCO_GetTypeTime());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (obj && var)
			obj.DCO_SetTypeTime(var.GetFloat(), "gm", playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjTypeDeadlineAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (!obj)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(obj.DCO_GetTypeDeadline());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (obj && var)
			obj.DCO_SetTypeDeadline(var.GetFloat(), "gm", playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjTypeDefendRatioAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (!obj)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(obj.DCO_GetTypeDefendRatio());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (obj && var)
			obj.DCO_SetTypeDefendRatio(var.GetFloat(), "gm", playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjTypeRetainAttribute : SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (!obj)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(obj.DCO_GetTypeRetain());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (obj && var)
			obj.DCO_SetTypeRetain(var.GetBool(), "gm", playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjTypeFactionAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	protected static void Factions(notnull array<string> keys)
	{
		keys.Clear();
		FactionManager fm = GetGame().GetFactionManager();
		if (!fm)
			return;
		array<Faction> facs = {};
		fm.GetFactionsList(facs);
		foreach (Faction f : facs)
		{
			if (f)
				keys.Insert(f.GetFactionKey());
		}
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (!obj)
			return null;
		array<string> keys = {};
		Factions(keys);
		int index = keys.Find(obj.DCO_GetTypeFaction());
		return SCR_BaseEditorAttributeVar.CreateInt(index + 1);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAssign.Obj(item);
		if (!obj || !var)
			return;
		array<string> keys = {};
		Factions(keys);
		int index = var.GetInt() - 1;
		string key;
		if (index >= 0 && index < keys.Count())
			key = keys[index];
		else if (index >= keys.Count())
		{
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("obj_fields_skip obj=%1 field=type_faction entry=%2 reason=index_out_of_range factions=%3", obj.GetLogName(), index, keys.Count()));
			return;
		}
		obj.DCO_SetTypeFaction(key, "gm", playerID);
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		m_aValues.Clear();
		SCR_EditorAttributeFloatStringValueHolder autoEntry = new SCR_EditorAttributeFloatStringValueHolder();
		autoEntry.SetName("Auto");
		autoEntry.SetFloatValue(0);
		m_aValues.Insert(autoEntry);
		array<string> keys = {};
		Factions(keys);
		foreach (int i, string k : keys)
		{
			SCR_EditorAttributeFloatStringValueHolder v = new SCR_EditorAttributeFloatStringValueHolder();
			v.SetName(k);
			v.SetFloatValue(i + 1);
			m_aValues.Insert(v);
		}
		return super.GetEntries(outEntries);
	}
}
