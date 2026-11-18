class DCO_SettingsPersist
{
	protected static ref map<string, ref SCR_BaseEditorAttribute> s_mAttr;
	protected static ref array<string> s_aClasses;
	protected static ref SCR_EditorAttributeList s_List;
	protected static bool s_bListTried;

	static SCR_EditorAttributeList List()
	{
		if (s_bListTried)
			return s_List;
		s_bListTried = true;
		Resource res = BaseContainerTools.LoadContainer("{F3D6C6D25642352C}Configs/Editor/AttributeLists/Edit.conf");
		if (res && res.IsValid())
			s_List = SCR_EditorAttributeList.Cast(BaseContainerTools.CreateInstanceFromContainer(res.GetResource().ToBaseContainer()));
		int n;
		if (s_List)
			n = s_List.GetAttributesCount();
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("persist_attr_list loaded=%1 attrs=%2", DCO_Util.B01(s_List != null), n));
		return s_List;
	}

	static SCR_BaseEditorAttribute FromList(string className)
	{
		SCR_EditorAttributeList list = List();
		if (!list)
			return null;
		for (int i = 0, count = list.GetAttributesCount(); i < count; i++)
		{
			SCR_BaseEditorAttribute attribute = list.GetAttribute(i);
			if (attribute && attribute.Type().ToString() == className)
				return attribute;
		}
		return null;
	}

	static void FactionAttributes(notnull array<SCR_BaseEditorAttribute> outAttributes)
	{
		SCR_EditorAttributeList list = List();
		if (!list)
			return;
		for (int i = 0, count = list.GetAttributesCount(); i < count; i++)
		{
			SCR_BaseEditorAttribute attribute = list.GetAttribute(i);
			if (attribute && DCO_SettingsTree.TargetOf(attribute) == 0 && !DCO_SettingsTree.IsWorldWide(attribute))
				outAttributes.Insert(attribute);
		}
	}

	static array<string> Classes()
	{
		if (s_aClasses)
			return s_aClasses;
		s_aClasses = {};
		DCO_SettingsHelp.AllAttributes(s_aClasses);
		return s_aClasses;
	}

	static SCR_BaseEditorAttribute Attr(string className)
	{
		className = DCO_ClassAlias.Resolve(className);
		if (!s_mAttr)
			s_mAttr = new map<string, ref SCR_BaseEditorAttribute>();
		SCR_BaseEditorAttribute a;
		if (s_mAttr.Find(className, a))
			return a;
		typename t = className.ToType();
		bool ok = t && t.IsInherited(SCR_BaseEditorAttribute) && !t.IsInherited(DCO_NameEditorAttribute);
		if (ok && className != "DCO_CommanderPresetLoadAttribute")
		{
			a = FromList(className);
			if (!a)
			{
				a = SCR_BaseEditorAttribute.Cast(t.Spawn());
				DCO_Perf.Count("persist:attr_spawned");
			}
		}
		s_mAttr.Set(className, a);
		return a;
	}

	static bool Same(vector a, vector b)
	{
		return Math.AbsFloat(a[0] - b[0]) < 0.0001 && Math.AbsFloat(a[1] - b[1]) < 0.0001 && Math.AbsFloat(a[2] - b[2]) < 0.0001;
	}

	static string Dump(Managed item, out int count)
	{
		count = 0;
		string content;
		if (!item)
			return content;
		foreach (string className : Classes())
		{
			SCR_BaseEditorAttribute a = Attr(className);
			if (!a)
				continue;
			SCR_BaseEditorAttributeVar v = a.ReadVariable(item, null);
			if (!v)
				continue;
			if (!content.IsEmpty())
				content += ";";
			content += DCO_SettingsCodec.Entry(className, v.GetVector());
			count++;
		}
		return content;
	}

	static int Apply(Managed item, string content, out int skipped, array<string> skipClasses = null)
	{
		skipped = 0;
		int applied = 0;
		if (!item || content.IsEmpty())
			return 0;
		array<string> keys = {};
		array<vector> values = {};
		DCO_SettingsCodec.Decode(content, keys, values);
		foreach (int i, string className : keys)
		{
			if (skipClasses && skipClasses.Contains(DCO_ClassAlias.Resolve(className)))
				continue;
			SCR_BaseEditorAttribute a = Attr(className);
			if (!a)
			{
				skipped++;
				continue;
			}
			SCR_BaseEditorAttributeVar cur = a.ReadVariable(item, null);
			if (!cur)
			{
				skipped++;
				continue;
			}
			if (Same(cur.GetVector(), values[i]))
				continue;
			a.WriteVariable(item, SCR_BaseEditorAttributeVar.CreateVector(values[i]), null, -1);
			applied++;
		}
		return applied;
	}

	static void WriteParts(SaveContext context, string name, string content)
	{
		array<string> parts = {};
		DCO_SettingsPresets.SplitChunks(content, 4096, parts);
		context.WriteValue(name, parts);
	}

	static string ReadParts(LoadContext context, string name)
	{
		array<string> parts = {};
		context.ReadValue(name, parts);
		string content;
		foreach (string p : parts)
			content += p;
		return content;
	}

	static SCR_EditableEntityComponent Editable(IEntity e)
	{
		if (!e)
			return null;
		return SCR_EditableEntityComponent.Cast(e.FindComponent(SCR_EditableEntityComponent));
	}

	static string Name(IEntity e)
	{
		if (!e)
			return "-";
		SCR_EditableEntityComponent ed = Editable(e);
		if (ed)
			return DCO_SettingsBaseline.LogText(DCO_SettingsBaseline.ItemName(ed));
		return e.GetName();
	}

	static ESerializeResult SaveEditable(IEntity owner, SaveContext context, string tag)
	{
		SCR_EditableEntityComponent ed = Editable(owner);
		if (!ed)
			return ESerializeResult.DEFAULT;
		int count;
		string content = Dump(ed, count);
		if (count == 0)
			return ESerializeResult.DEFAULT;
		context.WriteValue("version", 1);
		WriteParts(context, "attrs", content);
		DCO_Perf.Count("persist:save_" + tag);
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("persist_save_settings target=%1 name=%2 classes=%3 len=%4", tag, Name(owner), count, content.Length()));
		return ESerializeResult.OK;
	}

	static bool LoadEditable(IEntity owner, LoadContext context, string tag)
	{
		int version;
		context.ReadValue("version", version);
		string content = ReadParts(context, "attrs");
		int skipped;
		int applied = Apply(Editable(owner), content, skipped);
		DCO_Perf.Count("persist:load_" + tag);
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("persist_load_settings target=%1 name=%2 applied=%3 skipped=%4 len=%5", tag, Name(owner), applied, skipped, content.Length()));
		return true;
	}

	static void ApplyUnit(IEntity unit, string content, int tries)
	{
		if (!unit || content.IsEmpty())
			return;
		int skipped;
		int applied = Apply(Editable(unit), content, skipped);
		if (applied == 0 && skipped > 0 && tries < 5)
		{
			GetGame().GetCallqueue().CallLater(ApplyUnit, 2000, false, unit, content, tries + 1);
			DCO_Perf.Count("persist:unit_retry");
			return;
		}
		DCO_Perf.Count("persist:load_unit");
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdUnit(unit), string.Format("persist_load_settings target=unit name=%1 applied=%2 skipped=%3 tries=%4", Name(unit), applied, skipped, tries));
	}
}

class DCO_GmFactionCfg
{
	static BaseWorld s_SaveWorld;

	static void Apply(string factionKey, string content)
	{
		if (s_SaveWorld && s_SaveWorld == GetGame().GetWorld())
		{
			DCO_Perf.Count("cfg:gm_faction_skip_save");
			DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cfg_gm_faction_skip faction=%1 reason=save_game_loaded", factionKey));
			return;
		}
		array<string> keys = {};
		array<vector> values = {};
		DCO_SettingsCodec.Decode(content, keys, values);
		DCO_GlobalAIComponent.DCO_SetOverrides(factionKey, keys, values, null);
		DCO_Perf.Count("cfg:gm_faction_loaded");
		DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.INFO, "-", string.Format("cfg_gm_faction_loaded faction=%1 overrides=%2", factionKey, keys.Count()));
	}

	static string Json(out int count)
	{
		count = 0;
		array<string> factions = {};
		DCO_GlobalAIComponent.DCO_OverrideFactions(factions);
		string json;
		foreach (string fk : factions)
		{
			array<string> classes = {};
			array<vector> values = {};
			DCO_GlobalAIComponent.DCO_GetOverrides(fk, classes, values);
			string content;
			foreach (int i, string className : classes)
			{
				if (!content.IsEmpty())
					content += ";";
				content += DCO_SettingsCodec.Entry(className, values[i]);
			}
			if (!json.IsEmpty())
				json += ",";
			json += "\"" + fk + "\":\"" + content + "\"";
			count++;
		}
		return json;
	}
}

class DCO_SettingsState : PersistentState
{
}

class DCO_SettingsStateSerializer : ScriptedStateSerializer
{
	override static typename GetTargetType()
	{
		return DCO_SettingsState;
	}

	override ESerializeResult Serialize(notnull Managed instance, notnull SaveContext context)
	{
		BaseGameMode gm = GetGame().GetGameMode();
		if (!gm)
			return ESerializeResult.DEFAULT;
		int count;
		string content = DCO_SettingsPersist.Dump(gm, count);
		array<string> factions = {};
		array<string> factionValues = {};
		DCO_GlobalAIComponent.DCO_OverrideFactions(factions);
		foreach (string fk : factions)
		{
			array<string> classes = {};
			array<vector> values = {};
			DCO_GlobalAIComponent.DCO_GetOverrides(fk, classes, values);
			string fc;
			foreach (int i, string className : classes)
			{
				if (!fc.IsEmpty())
					fc += ";";
				fc += DCO_SettingsCodec.Entry(className, values[i]);
			}
			factionValues.Insert(fc);
		}
		context.WriteValue("version", 1);
		DCO_SettingsPersist.WriteParts(context, "global", content);
		context.WriteValue("factions", factions);
		context.WriteValue("faction_values", factionValues);
		DCO_Perf.Count("persist:save_global");
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("persist_save_settings target=global classes=%1 len=%2 factions=%3", count, content.Length(), factions.Count()));
		return ESerializeResult.OK;
	}

	override bool Deserialize(notnull Managed instance, notnull LoadContext context)
	{
		int version;
		context.ReadValue("version", version);
		string content = DCO_SettingsPersist.ReadParts(context, "global");
		array<string> factions = {};
		array<string> factionValues = {};
		context.ReadValue("factions", factions);
		context.ReadValue("faction_values", factionValues);
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetInstance();
		int skipped;
		int applied = DCO_SettingsPersist.Apply(GetGame().GetGameMode(), content, skipped);
		int loadedFactions;
		foreach (int i, string fk : factions)
		{
			if (!factionValues.IsIndexValid(i) || fk.IsEmpty())
				continue;
			array<string> keys = {};
			array<vector> values = {};
			DCO_SettingsCodec.Decode(factionValues[i], keys, values);
			DCO_GlobalAIComponent.DCO_SetOverrides(fk, keys, values, null);
			loadedFactions++;
		}
		DCO_GmFactionCfg.s_SaveWorld = GetGame().GetWorld();
		DCO_GlobalAIComponent.DCO_RebuildAll(null);
		DCO_GlobalAIComponent.DCO_BumpVerAll();
		DCO_Perf.Count("persist:load_global");
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("persist_load_settings target=global applied=%1 skipped=%2 len=%3 factions=%4 global_ok=%5", applied, skipped, content.Length(), loadedFactions, DCO_Util.B01(g != null)));
		return true;
	}
}

class DCO_GroupSettingsSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType()
	{
		return DCO_GroupConfigComponent;
	}

	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull SaveContext context)
	{
		SCR_EditableEntityComponent ed = DCO_SettingsPersist.Editable(owner);
		if (!ed)
			return ESerializeResult.DEFAULT;
		int count;
		string content = DCO_SettingsPersist.Dump(ed, count);
		string members = DCO_SettingsMembers.StoredContent(DCO_SettingsMembers.GroupKey(owner));
		array<UUID> unitIds = {};
		array<string> unitValues = {};
		SCR_AIGroup grp = SCR_AIGroup.Cast(owner);
		if (grp)
		{
			array<AIAgent> agents = {};
			grp.GetAgents(agents);
			foreach (AIAgent a : agents)
			{
				if (!a || !a.GetControlledEntity() || EntityUtils.IsPlayer(a.GetControlledEntity()))
					continue;
				IEntity ch = a.GetControlledEntity();
				UUID id = GetSystem().GetId(ch);
				if (id.IsNull())
					continue;
				int uc;
				string uContent = DCO_SettingsPersist.Dump(DCO_SettingsPersist.Editable(ch), uc);
				if (uc == 0)
					continue;
				unitIds.Insert(id);
				unitValues.Insert(uContent);
			}
		}
		if (count == 0 && members.IsEmpty() && unitIds.IsEmpty())
			return ESerializeResult.DEFAULT;
		context.WriteValue("version", 1);
		DCO_SettingsPersist.WriteParts(context, "attrs", content);
		context.WriteValue("members", members);
		context.WriteValue("unit_ids", unitIds);
		context.WriteValue("unit_values", unitValues);
		DCO_Perf.Count("persist:save_group");
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("persist_save_settings target=group name=%1 classes=%2 members_len=%3 units=%4", DCO_SettingsPersist.Name(owner), count, members.Length(), unitIds.Count()));
		return ESerializeResult.OK;
	}

	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull LoadContext context)
	{
		int version;
		context.ReadValue("version", version);
		string content = DCO_SettingsPersist.ReadParts(context, "attrs");
		string members;
		context.ReadValue("members", members);
		array<UUID> unitIds = {};
		array<string> unitValues = {};
		context.ReadValue("unit_ids", unitIds);
		context.ReadValue("unit_values", unitValues);
		int skipped;
		int applied = DCO_SettingsPersist.Apply(DCO_SettingsPersist.Editable(owner), content, skipped);
		DCO_SettingsMembers.Restore(DCO_SettingsMembers.GroupKey(owner), members);
		int queued;
		foreach (int i, UUID id : unitIds)
		{
			if (!unitValues.IsIndexValid(i) || unitValues[i].IsEmpty())
				continue;
			Tuple1<string> ctx = new Tuple1<string>(unitValues[i]);
			PersistenceWhenAvailableTask task = new PersistenceWhenAvailableTask(OnUnitAvailable, ctx);
			GetSystem().WhenAvailable(id, task, 120.0);
			queued++;
		}
		DCO_Perf.Count("persist:load_group");
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdGroup(SCR_AIGroup.Cast(owner)), string.Format("persist_load_settings target=group name=%1 applied=%2 skipped=%3 members_len=%4 units_queued=%5", DCO_SettingsPersist.Name(owner), applied, skipped, members.Length(), queued));
		return true;
	}

	protected static void OnUnitAvailable(Managed instance, PersistenceDeferredDeserializeTask task, bool expired, Managed context)
	{
		IEntity unit = IEntity.Cast(instance);
		Tuple1<string> ctx = Tuple1<string>.Cast(context);
		if (!unit || expired || !ctx)
		{
			DCO_Perf.Count("persist:unit_missing");
			return;
		}
		GetGame().GetCallqueue().CallLater(DCO_SettingsPersist.ApplyUnit, 2000, false, unit, ctx.param1, 0);
	}
}

class DCO_HubSettingsSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType()
	{
		return DCO_LogisticsHubComponent;
	}

	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull SaveContext context)
	{
		return DCO_SettingsPersist.SaveEditable(owner, context, "hub");
	}

	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull LoadContext context)
	{
		return DCO_SettingsPersist.LoadEditable(owner, context, "hub");
	}
}

class DCO_TransportSettingsSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType()
	{
		return DCO_TransportTeamComponent;
	}

	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull SaveContext context)
	{
		return DCO_SettingsPersist.SaveEditable(owner, context, "transport");
	}

	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull LoadContext context)
	{
		return DCO_SettingsPersist.LoadEditable(owner, context, "transport");
	}
}

class DCO_GarrisonWaypointSerializer : GenericEntitySerializer
{
	override static typename GetTargetType()
	{
		return DCO_GarrisonWaypoint;
	}

	override protected ESerializeResult Serialize(notnull IEntity entity, notnull SaveContext context)
	{
		context.StartObject("base");
		ESerializeResult baseResult = super.Serialize(entity, context);
		context.EndObject();
		if (baseResult == ESerializeResult.ERROR)
			return baseResult;
		ESerializeResult own = DCO_SettingsPersist.SaveEditable(entity, context, "garrison_wp");
		if (own == ESerializeResult.DEFAULT)
			return baseResult;
		return own;
	}

	override protected bool Deserialize(notnull IEntity entity, notnull LoadContext context)
	{
		if (context.DoesObjectExist("base"))
		{
			if (!context.StartObject("base") || !super.Deserialize(entity, context) || !context.EndObject())
				return false;
		}
		return DCO_SettingsPersist.LoadEditable(entity, context, "garrison_wp");
	}
}

class DCO_DoCQBWaypointSerializer : GenericEntitySerializer
{
	override static typename GetTargetType()
	{
		return DCO_DoCQBWaypoint;
	}

	override protected ESerializeResult Serialize(notnull IEntity entity, notnull SaveContext context)
	{
		context.StartObject("base");
		ESerializeResult baseResult = super.Serialize(entity, context);
		context.EndObject();
		if (baseResult == ESerializeResult.ERROR)
			return baseResult;
		ESerializeResult own = DCO_SettingsPersist.SaveEditable(entity, context, "cqb_wp");
		if (own == ESerializeResult.DEFAULT)
			return baseResult;
		return own;
	}

	override protected bool Deserialize(notnull IEntity entity, notnull LoadContext context)
	{
		if (context.DoesObjectExist("base"))
		{
			if (!context.StartObject("base") || !super.Deserialize(entity, context) || !context.EndObject())
				return false;
		}
		return DCO_SettingsPersist.LoadEditable(entity, context, "cqb_wp");
	}
}

class DCO_BuildWaypointSerializer : GenericEntitySerializer
{
	override static typename GetTargetType()
	{
		return DCO_BuildWaypoint;
	}

	override protected ESerializeResult Serialize(notnull IEntity entity, notnull SaveContext context)
	{
		context.StartObject("base");
		ESerializeResult baseResult = super.Serialize(entity, context);
		context.EndObject();
		if (baseResult == ESerializeResult.ERROR)
			return baseResult;
		ESerializeResult own = DCO_SettingsPersist.SaveEditable(entity, context, "build_wp");
		if (own == ESerializeResult.DEFAULT)
			return baseResult;
		return own;
	}

	override protected bool Deserialize(notnull IEntity entity, notnull LoadContext context)
	{
		if (context.DoesObjectExist("base"))
		{
			if (!context.StartObject("base") || !super.Deserialize(entity, context) || !context.EndObject())
				return false;
		}
		return DCO_SettingsPersist.LoadEditable(entity, context, "build_wp");
	}
}

class DCO_TacticWaypointSerializer : GenericEntitySerializer
{
	override static typename GetTargetType()
	{
		return DCO_TacticWaypoint;
	}

	override protected ESerializeResult Serialize(notnull IEntity entity, notnull SaveContext context)
	{
		context.StartObject("base");
		ESerializeResult baseResult = super.Serialize(entity, context);
		context.EndObject();
		if (baseResult == ESerializeResult.ERROR)
			return baseResult;
		ESerializeResult own = DCO_SettingsPersist.SaveEditable(entity, context, "tactic_wp");
		if (own == ESerializeResult.DEFAULT)
			return baseResult;
		return own;
	}

	override protected bool Deserialize(notnull IEntity entity, notnull LoadContext context)
	{
		if (context.DoesObjectExist("base"))
		{
			if (!context.StartObject("base") || !super.Deserialize(entity, context) || !context.EndObject())
				return false;
		}
		return DCO_SettingsPersist.LoadEditable(entity, context, "tactic_wp");
	}
}
