modded enum ChimeraMenuPreset
{
	DCO_SettingsDialog,
	DCO_SettingsChooser
}

class DCO_SettingsBaseline
{
	protected static ref map<string, vector> s_mBase;

	static string Key(Managed item, SCR_BaseEditorAttribute attribute)
	{
		return Replication.FindItemId(item).ToString() + ":" + attribute.Type().ToString();
	}

	static string ItemName(Managed item)
	{
		if (BaseGameMode.Cast(item))
			return "Global";
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable)
			return "?";
		string name = WidgetManager.Translate(editable.GetDisplayName());
		if (name.IsEmpty() && editable.GetOwner())
			name = editable.GetOwner().GetName();
		if (name.IsEmpty())
			name = "entity" + Replication.FindItemId(item).ToString();
		return name;
	}

	static string LogText(string s)
	{
		s.Replace(" ", "_");
		s.Replace("=", "-");
		return s;
	}

	static string FormatValue(SCR_BaseEditorAttribute attribute, vector v)
	{
		if (DCO_NameEditorAttribute.Cast(attribute))
		{
			if (v[0] != 0)
				return "renamed";
			return "same";
		}
		array<ref SCR_BaseEditorAttributeEntry> entries = {};
		attribute.GetEntries(entries);
		bool multi = SCR_BaseMultiSelectPresetsEditorAttribute.Cast(attribute) != null;
		foreach (SCR_BaseEditorAttributeEntry entry : entries)
		{
			SCR_BaseEditorAttributeFloatStringValues values = SCR_BaseEditorAttributeFloatStringValues.Cast(entry);
			if (!values || multi)
				continue;
			for (int i = 0, n = values.GetValueCount(); i < n; i++)
			{
				if (Math.AbsFloat(values.GetEntryFloatValue(i) - v[0]) < 0.0001)
					return WidgetManager.Translate(values.GetValuesEntry(i).GetName());
			}
		}
		if (v[1] != 0 || v[2] != 0)
			return v.ToString();
		float f = v[0];
		if (Math.AbsFloat(f - Math.Round(f)) < 0.0001)
			return ((int)Math.Round(f)).ToString();
		return f.ToString(-1, 3);
	}

	static void Collect(SCR_AttributesManagerEditorComponent manager, SCR_AttributesManagerEditorComponentClass data, notnull array<Managed> items, notnull array<int> outIds, notnull array<float> outValues)
	{
		if (!data)
			return;
		if (!s_mBase)
			s_mBase = new map<string, vector>();

		for (int i = 0, count = data.GetAttributesCount(); i < count; i++)
		{
			SCR_BaseEditorAttribute attribute = data.GetAttribute(i);
			if (!attribute || !attribute.IsServer() || !DCO_SettingsTree.IsDCO(attribute))
				continue;

			bool first = true;
			foreach (Managed item : items)
			{
				if (!item)
					continue;
				SCR_BaseEditorAttributeVar var = attribute.ReadVariable(item, manager);
				if (!var)
					continue;

				string key = Key(item, attribute);
				vector v;
				if (!s_mBase.Find(key, v))
				{
					v = var.GetVector();
					s_mBase.Set(key, v);
				}
				if (first)
				{
					outIds.Insert(i);
					outValues.Insert(v[0]);
					outValues.Insert(v[1]);
					outValues.Insert(v[2]);
					first = false;
				}
			}
		}
	}
}

modded class SCR_AttributesManagerEditorComponent
{
	protected bool m_bDCO_Mode;
	protected bool m_bDCO_Swapped;
	protected ChimeraMenuPreset m_eDCO_SavedPreset;
	protected ref map<int, vector> m_mDCO_Defaults;
	protected ref array<int> m_aDCO_LogIds;
	protected ref array<int> m_aDCO_LogItems;
	protected ref array<string> m_aDCO_LogOld;
	protected ref ScriptInvoker m_DCO_OnEvent;

	bool DCO_IsMode()
	{
		return m_bDCO_Mode;
	}

	bool DCO_GetDefault(SCR_BaseEditorAttribute attribute, out vector value)
	{
		if (!m_mDCO_Defaults || !m_PrefabData || !attribute)
			return false;
		return m_mDCO_Defaults.Find(m_PrefabData.FindAttribute(attribute), value);
	}

	int DCO_IndexOf(SCR_BaseEditorAttribute attribute)
	{
		if (!m_PrefabData)
			return -1;
		return m_PrefabData.FindAttribute(attribute);
	}

	static SCR_AttributesManagerEditorComponent DCO_Get()
	{
		return SCR_AttributesManagerEditorComponent.Cast(SCR_AttributesManagerEditorComponent.GetInstance(SCR_AttributesManagerEditorComponent));
	}

	static bool DCO_CanUseNow()
	{
		SCR_StatesEditorComponent states = SCR_StatesEditorComponent.Cast(SCR_StatesEditorComponent.GetInstance(SCR_StatesEditorComponent));
		return !states || states.GetState() == EEditorState.SELECTING;
	}

	static void DCO_OpenGlobal()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		SCR_AttributesManagerEditorComponent manager = DCO_Get();
		if (!gameMode || !manager || !DCO_CanUseNow())
			return;
		array<Managed> items = {gameMode};
		manager.DCO_Open(items);
	}

	static void DCO_OpenEntities(notnull set<SCR_EditableEntityComponent> entities)
	{
		SCR_AttributesManagerEditorComponent manager = DCO_Get();
		if (!manager || !DCO_CanUseNow())
			return;
		array<Managed> items = {};
		foreach (SCR_EditableEntityComponent entity : entities)
		{
			if (entity)
				items.Insert(entity);
		}
		if (items.IsEmpty() || !manager.DCO_CanOpen(items, true))
			return;
		manager.DCO_Open(items);
	}

	static void DCO_OpenSelectionOrGlobal()
	{
		set<SCR_EditableEntityComponent> selected = new set<SCR_EditableEntityComponent>();
		SCR_BaseEditableEntityFilter.GetEnititiesStatic(selected, EEditableEntityState.SELECTED);
		SCR_AttributesManagerEditorComponent manager = DCO_Get();
		if (manager && !selected.IsEmpty())
		{
			array<Managed> items = {};
			foreach (SCR_EditableEntityComponent entity : selected)
			{
				if (entity)
					items.Insert(entity);
			}
			if (!items.IsEmpty() && manager.DCO_CanOpen(items, true))
			{
				DCO_OpenEntities(selected);
				return;
			}
		}
		DCO_OpenGlobal();
	}

	bool DCO_CanOpen(notnull array<Managed> items, bool dco)
	{
		if (!m_PrefabData)
			return false;
		for (int i = 0, count = m_PrefabData.GetAttributesCount(); i < count; i++)
		{
			SCR_BaseEditorAttribute attribute = m_PrefabData.GetAttribute(i);
			if (!attribute || DCO_SettingsTree.IsDCO(attribute) != dco)
				continue;
			foreach (Managed item : items)
			{
				if (item && attribute.ReadVariable(item, this))
					return true;
			}
		}
		return false;
	}

	bool DCO_Open(notnull array<Managed> items)
	{
		if (!m_PrefabData || !IsOwner() || m_aEditedItems || items.IsEmpty())
			return false;

		array<int> itemIds = {};
		foreach (Managed item : items)
		{
			int id = Replication.FindItemId(item);
			if (id != -1 && !itemIds.Contains(id))
				itemIds.Insert(id);
		}

		m_bDCO_Mode = true;
		m_mDCO_Defaults = new map<int, vector>();
		if (!m_bDCO_Swapped)
		{
			m_eDCO_SavedPreset = m_MenuPreset;
			m_bDCO_Swapped = true;
		}
		m_MenuPreset = ChimeraMenuPreset.DCO_SettingsDialog;

		Rpc(DCO_RpcOpenServer, itemIds);
		StartEditing(items);
		return true;
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcOpenServer(array<int> itemIds)
	{
		m_bDCO_Mode = true;
		array<Managed> items = {};
		foreach (int id : itemIds)
		{
			Managed item = Replication.FindItem(id);
			if (item)
				items.Insert(item);
		}

		array<int> ids = {};
		array<float> values = {};
		DCO_SettingsBaseline.Collect(this, m_PrefabData, items, ids, values);
		Rpc(DCO_RpcDefaults, ids, values);

		string target = "none";
		if (!items.IsEmpty())
			target = DCO_SettingsBaseline.LogText(DCO_SettingsBaseline.ItemName(items[0]));
		DCO_BenchmarkLoggerComponent.Event(string.Format("GM %1 dco_settings_open target=%2 items=%3 settings=%4", DCO_LogWho(), target, items.Count(), ids.Count()));
	}

	string DCO_LogWho()
	{
		return "P" + DCO_PlayerId().ToString();
	}

	int DCO_PlayerId()
	{
		SCR_EditorManagerEntity editorManager = GetManager();
		if (editorManager)
			return editorManager.GetPlayerID();
		return -1;
	}

	protected string DCO_EditedTarget()
	{
		if (!m_aEditedItems || m_aEditedItems.IsEmpty())
			return "none";
		if (m_aEditedItems.Count() > 1)
			return m_aEditedItems.Count().ToString() + "_entities";
		return DCO_SettingsBaseline.LogText(DCO_SettingsBaseline.ItemName(m_aEditedItems[0]));
	}

	ScriptInvoker DCO_GetOnEvent()
	{
		if (!m_DCO_OnEvent)
			m_DCO_OnEvent = new ScriptInvoker();
		return m_DCO_OnEvent;
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcEvent(int kind, string a, string b)
	{
		if (m_DCO_OnEvent)
			m_DCO_OnEvent.Invoke(kind, a, b);
	}

	void DCO_BeginApply(notnull array<int> attributeIds, int resetAll, int warnings)
	{
		if (IsOwner() && m_bDCO_Mode)
			Rpc(DCO_RpcApplyBegin, attributeIds, resetAll, warnings);
	}

	void DCO_ReportImport(bool ok, string scope, string reason, int count)
	{
		if (IsOwner())
			Rpc(DCO_RpcImportLog, ok, scope, reason, count);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcImportLog(bool ok, string scope, string reason, int count)
	{
		string result = "fail";
		if (ok)
			result = "ok";
		DCO_BenchmarkLoggerComponent.Event(string.Format("GM %1 dco_settings_import result=%2 scope=%3 reason=%4 count=%5", DCO_LogWho(), result, DCO_SettingsBaseline.LogText(scope), DCO_SettingsBaseline.LogText(reason), count));
	}

	void DCO_PresetList(string scope)
	{
		if (IsOwner())
			Rpc(DCO_RpcPresetList, scope);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcPresetList(string scope)
	{
		DCO_SendPresetList(scope);
	}

	protected void DCO_SendPresetList(string scope)
	{
		array<string> names = {};
		DCO_SettingsPresets.List(scope, names);
		string joined;
		foreach (int i, string n : names)
		{
			if (i > 0)
				joined += ";";
			joined += n;
		}
		Rpc(DCO_RpcEvent, 1, scope, joined);
	}

	void DCO_PresetLoad(string scope, string name)
	{
		if (IsOwner())
			Rpc(DCO_RpcPresetLoad, scope, name);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcPresetLoad(string scope, string name)
	{
		name = DCO_CommanderPresets.Sanitize(name);
		string content;
		if (!DCO_SettingsPresets.Load(scope, name, content))
		{
			DCO_BenchmarkLoggerComponent.Event(string.Format("GM %1 dco_preset_load result=fail scope=%2 name=%3 reason=missing_or_invalid", DCO_LogWho(), scope, name));
			Rpc(DCO_RpcEvent, 3, "load_fail", name);
			return;
		}
		DCO_BenchmarkLoggerComponent.Event(string.Format("GM %1 dco_preset_load result=ok scope=%2 name=%3 target=%4", DCO_LogWho(), scope, name, DCO_EditedTarget()));
		DCO_SettingsChangeLog.Add(DCO_PlayerId(), DCO_EditedTarget(), "preset_load_" + scope, "-", name);
		Rpc(DCO_RpcEvent, 2, name, content);
	}

	void DCO_PresetSave(string scope, string name, string content, bool overwrite)
	{
		if (IsOwner())
			Rpc(DCO_RpcPresetSave, scope, name, content, overwrite);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcPresetSave(string scope, string name, string content, bool overwrite)
	{
		name = DCO_CommanderPresets.Sanitize(name);
		if (name.IsEmpty() || !DCO_SettingsPresets.ValidScope(scope))
		{
			Rpc(DCO_RpcEvent, 3, "save_invalid", name);
			return;
		}
		if (!overwrite && DCO_SettingsPresets.Exists(scope, name))
		{
			Rpc(DCO_RpcEvent, 3, "save_exists", name);
			return;
		}
		if (scope == "Group" && !content.Contains("m:"))
			content += DCO_MemberContent();
		bool ok = DCO_SettingsPresets.Save(scope, name, content);
		string result = "fail";
		if (ok)
			result = "ok";
		DCO_BenchmarkLoggerComponent.Event(string.Format("GM %1 dco_preset_save result=%2 scope=%3 name=%4 target=%5", DCO_LogWho(), result, scope, name, DCO_EditedTarget()));
		if (ok)
		{
			DCO_SettingsChangeLog.Add(DCO_PlayerId(), DCO_EditedTarget(), "preset_save_" + scope, "-", name);
			Rpc(DCO_RpcEvent, 3, "save_ok", name);
		}
		else
		{
			Rpc(DCO_RpcEvent, 3, "save_fail", name);
		}
		DCO_SendPresetList(scope);
	}

	void DCO_SaveServerConfig()
	{
		if (IsOwner())
			Rpc(DCO_RpcSaveServerConfig);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcSaveServerConfig()
	{
		int pid = DCO_GroupPresets.VerifiedGM(GetManager(), DCO_PlayerId(), "save_server_config");
		if (pid < 0)
		{
			Rpc(DCO_RpcEvent, 3, "cfgsave_denied", string.Empty);
			return;
		}
		string why;
		if (!DCO_GlobalAIComponent.DCO_CanSaveConfig(pid, why))
		{
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.WARN, "-", string.Format("cfg_save_gm result=denied player=%1 mode=%2", pid, why));
			Rpc(DCO_RpcEvent, 3, "cfgsave_denied", why);
			return;
		}
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetInstance();
		string path;
		bool ok = g && g.DCO_SaveActiveConfig(pid, path);
		DCO_BenchmarkLoggerComponent.Event(string.Format("GM %1 dco_cfg_save result=%2 allowed_by=%3 path=%4", DCO_LogWho(), ok.ToString(), why, path));
		if (!ok)
		{
			Rpc(DCO_RpcEvent, 3, "cfgsave_fail", path);
			return;
		}
		DCO_SettingsChangeLog.Add(pid, "Global", "save_server_config", "-", path);
		Rpc(DCO_RpcEvent, 3, "cfgsave_ok", path);
	}

	protected static bool s_bDCO_StartupAsked;
	protected static bool s_bDCO_StartupDone;
	protected int m_iDCO_StartupTries;

	protected void DCO_StartupQuery()
	{
		if (s_bDCO_StartupAsked || !IsOwner())
			return;
		s_bDCO_StartupAsked = true;
		Rpc(DCO_RpcStartupQuery);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcStartupQuery()
	{
		if (s_bDCO_StartupDone)
			return;
		SCR_EditorManagerEntity em = GetManager();
		int pid = DCO_PlayerId();
		if (!em || em.IsLimited())
		{
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("startup_preset_skip player=%1 reason=limited_gm", pid));
			return;
		}
		s_bDCO_StartupDone = true;
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetInstance();
		if (!g || !g.DCO_StartupPrompt())
		{
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("startup_preset_skip player=%1 reason=disabled", pid));
			return;
		}
		array<string> names = {};
		DCO_SettingsPresets.List("Global", names);
		if (names.IsEmpty())
		{
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("startup_preset_skip player=%1 reason=no_presets dir=%2", pid, DCO_SettingsPresets.Dir("Global")));
			return;
		}
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("startup_preset_prompt player=%1 presets=%2", pid, names.Count()));
		Rpc(DCO_RpcStartupPrompt, names.Count());
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcStartupPrompt(int count)
	{
		m_iDCO_StartupTries = 0;
		DCO_StartupOpen();
	}

	protected void DCO_StartupOpen()
	{
		if (m_aEditedItems || !DCO_CanUseNow() || !GetGame().GetGameMode())
		{
			m_iDCO_StartupTries++;
			if (m_iDCO_StartupTries <= 30)
			{
				GetGame().GetCallqueue().CallLater(DCO_StartupOpen, 1000);
				return;
			}
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.WARN, "-", "startup_preset_skip reason=editor_busy tries=" + m_iDCO_StartupTries.ToString());
			return;
		}
		DCO_SettingsDialogUI.DCO_RequestStartupPick();
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "startup_preset_open tries=" + m_iDCO_StartupTries.ToString());
		DCO_OpenGlobal();
	}

	void DCO_PresetDelete(string scope, string name)
	{
		if (IsOwner())
			Rpc(DCO_RpcPresetDelete, scope, name);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcPresetDelete(string scope, string name)
	{
		name = DCO_CommanderPresets.Sanitize(name);
		bool ok = DCO_SettingsPresets.Delete(scope, name);
		string result = "fail";
		if (ok)
			result = "ok";
		DCO_BenchmarkLoggerComponent.Event(string.Format("GM %1 dco_preset_delete result=%2 scope=%3 name=%4", DCO_LogWho(), result, scope, name));
		if (ok)
			Rpc(DCO_RpcEvent, 3, "delete_ok", name);
		else
			Rpc(DCO_RpcEvent, 3, "delete_fail", name);
		DCO_SendPresetList(scope);
	}

	protected SCR_EditableEntityComponent DCO_FirstMember(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable)
			return null;
		SCR_AIGroup group = SCR_AIGroup.Cast(editable.GetOwner());
		if (!group)
			return null;
		array<AIAgent> agents = {};
		group.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (!agent || !agent.GetControlledEntity())
				continue;
			SCR_EditableEntityComponent member = SCR_EditableEntityComponent.Cast(agent.GetControlledEntity().FindComponent(SCR_EditableEntityComponent));
			if (member)
				return member;
		}
		return null;
	}

	protected string DCO_MemberContent()
	{
		if (!m_aEditedItems || !m_PrefabData)
			return string.Empty;
		SCR_EditableEntityComponent member;
		foreach (Managed item : m_aEditedItems)
		{
			member = DCO_FirstMember(item);
			if (member)
				break;
		}
		if (!member)
			return string.Empty;
		string outText;
		for (int i = 0, count = m_PrefabData.GetAttributesCount(); i < count; i++)
		{
			SCR_BaseEditorAttribute attribute = m_PrefabData.GetAttribute(i);
			if (!attribute || DCO_SettingsTree.TargetOf(attribute) != 3 || DCO_NameEditorAttribute.Cast(attribute))
				continue;
			SCR_BaseEditorAttributeVar var = attribute.ReadVariable(member, this);
			if (!var)
				continue;
			outText += ";m:" + DCO_SettingsCodec.Entry(attribute.Type().ToString(), var.GetVector());
		}
		return outText;
	}

	protected SCR_BaseEditorAttribute DCO_FindByClass(string className)
	{
		if (!m_PrefabData)
			return null;
		for (int i = 0, count = m_PrefabData.GetAttributesCount(); i < count; i++)
		{
			SCR_BaseEditorAttribute attribute = m_PrefabData.GetAttribute(i);
			if (attribute && attribute.Type().ToString() == className)
				return attribute;
		}
		return null;
	}

	SCR_BaseEditorAttribute DCO_AttributeByClass(string className)
	{
		return DCO_FindByClass(className);
	}

	void DCO_AttributesOfTarget(int target, notnull array<SCR_BaseEditorAttribute> outAttributes)
	{
		if (!m_PrefabData)
			return;
		for (int i = 0, count = m_PrefabData.GetAttributesCount(); i < count; i++)
		{
			SCR_BaseEditorAttribute attribute = m_PrefabData.GetAttribute(i);
			if (attribute && !DCO_NameEditorAttribute.Cast(attribute) && DCO_SettingsTree.TargetOf(attribute) == target)
				outAttributes.Insert(attribute);
		}
	}

	protected void DCO_MembersOf(notnull array<SCR_EditableEntityComponent> outMembers, notnull array<int> outGroups, out int skippedPlayers)
	{
		if (!m_aEditedItems)
			return;
		foreach (Managed item : m_aEditedItems)
		{
			SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
			if (!editable)
				continue;
			SCR_AIGroup group = SCR_AIGroup.Cast(editable.GetOwner());
			if (!group)
				continue;
			array<AIAgent> agents = {};
			group.GetAgents(agents);
			foreach (AIAgent agent : agents)
			{
				if (!agent || !agent.GetControlledEntity())
					continue;
				if (DCO_SettingsMembers.IsPlayer(agent.GetControlledEntity()))
				{
					skippedPlayers++;
					continue;
				}
				SCR_EditableEntityComponent member = SCR_EditableEntityComponent.Cast(agent.GetControlledEntity().FindComponent(SCR_EditableEntityComponent));
				if (!member)
					continue;
				outMembers.Insert(member);
				outGroups.Insert(DCO_SettingsMembers.GroupKey(group));
			}
		}
	}

	void DCO_RequestMembers()
	{
		if (IsOwner() && m_bDCO_Mode)
			Rpc(DCO_RpcMembers);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcMembers()
	{
		array<SCR_EditableEntityComponent> members = {};
		array<int> groups = {};
		int skipped;
		DCO_MembersOf(members, groups, skipped);
		array<SCR_BaseEditorAttribute> attributes = {};
		DCO_AttributesOfTarget(3, attributes);
		string list = "S|" + skipped.ToString();
		string lines;
		if (!groups.IsEmpty())
			lines = DCO_SettingsMembers.StoredContent(groups[0]);
		foreach (int mi, SCR_EditableEntityComponent member : members)
		{
			string name = WidgetManager.Translate(member.GetDisplayName());
			if (name.IsEmpty() && member.GetOwner())
				name = member.GetOwner().GetName();
			name = (mi + 1).ToString() + ". " + name;
			name.Replace("|", "/");
			name.Replace(";", ",");
			list += ";" + Replication.FindItemId(member).ToString() + "|" + name;
			string content;
			foreach (SCR_BaseEditorAttribute attribute : attributes)
			{
				SCR_BaseEditorAttributeVar var = attribute.ReadVariable(member, this);
				if (!var)
					continue;
				if (!content.IsEmpty())
					content += ";";
				content += DCO_SettingsCodec.Entry(attribute.Type().ToString(), var.GetVector());
			}
			lines += "\n" + content;
		}
		Rpc(DCO_RpcEvent, 7, list, lines);
	}

	void DCO_ApplyMemberOverride(int memberId, notnull array<string> classes, notnull array<float> values)
	{
		if (IsOwner() && m_bDCO_Mode && !classes.IsEmpty())
			Rpc(DCO_RpcApplyMemberOverride, memberId, classes, values);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcApplyMemberOverride(int memberId, array<string> classes, array<float> values)
	{
		array<SCR_EditableEntityComponent> members = {};
		array<int> groups = {};
		int skipped;
		DCO_MembersOf(members, groups, skipped);
		SCR_EditableEntityComponent target;
		foreach (SCR_EditableEntityComponent member : members)
		{
			if (Replication.FindItemId(member) == memberId)
				target = member;
		}
		if (!target)
			return;
		int playerId = DCO_PlayerId();
		int written = 0;
		foreach (int i, string className : classes)
		{
			int k = i * 3;
			if (k + 2 >= values.Count())
				break;
			SCR_BaseEditorAttribute attribute = DCO_FindByClass(className);
			if (!attribute || DCO_SettingsTree.TargetOf(attribute) != 3 || !attribute.ReadVariable(target, this))
				continue;
			attribute.WriteVariable(target, SCR_BaseEditorAttributeVar.CreateVector(Vector(values[k], values[k + 1], values[k + 2])), this, playerId);
			written++;
		}
		string memberName = DCO_SettingsBaseline.LogText(DCO_SettingsBaseline.ItemName(target));
		DCO_SettingsChangeLog.Add(playerId, DCO_EditedTarget(), "member_override_" + memberName, "-", written.ToString() + "_settings");
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_member_override group=" + DCO_EditedTarget() + " member=" + memberName + " settings=" + written.ToString());
	}

	void DCO_ApplyStored(SCR_AIGroup group, AIAgent agent)
	{
		DCO_SettingsMembers.ApplyStored(group, agent, this);
	}

	void DCO_FactionAttributes(notnull array<SCR_BaseEditorAttribute> outAttributes)
	{
		if (!m_PrefabData)
			return;
		for (int i = 0, count = m_PrefabData.GetAttributesCount(); i < count; i++)
		{
			SCR_BaseEditorAttribute attribute = m_PrefabData.GetAttribute(i);
			if (attribute && DCO_SettingsTree.TargetOf(attribute) == 0 && !DCO_SettingsTree.IsWorldWide(attribute))
				outAttributes.Insert(attribute);
		}
	}

	void DCO_FactionState(string factionKey)
	{
		if (IsOwner())
			Rpc(DCO_RpcFactionState, factionKey);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcFactionState(string factionKey)
	{
		array<string> classes = {};
		array<vector> values = {};
		DCO_GlobalAIComponent.DCO_GetOverrides(factionKey, classes, values);
		string content;
		foreach (int i, string className : classes)
		{
			if (!content.IsEmpty())
				content += ";";
			content += DCO_SettingsCodec.Entry(className, values[i]);
		}
		Rpc(DCO_RpcEvent, 5, factionKey, content);
	}

	void DCO_FactionApply(string factionKey, string content)
	{
		if (IsOwner())
			Rpc(DCO_RpcFactionApply, factionKey, content);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcFactionApply(string factionKey, string content)
	{
		FactionManager fm = GetGame().GetFactionManager();
		if (factionKey.IsEmpty() || !fm || !fm.GetFactionByKey(factionKey))
		{
			Rpc(DCO_RpcEvent, 6, factionKey, "invalid");
			return;
		}
		array<string> keys = {};
		array<vector> values = {};
		DCO_SettingsCodec.Decode(content, keys, values);
		array<string> classes = {};
		array<vector> accepted = {};
		foreach (int i, string className : keys)
		{
			SCR_BaseEditorAttribute attribute = DCO_FindByClass(className);
			if (!attribute || DCO_SettingsTree.TargetOf(attribute) != 0 || DCO_SettingsTree.IsWorldWide(attribute))
				continue;
			classes.Insert(className);
			accepted.Insert(values[i]);
		}

		array<string> oldClasses = {};
		array<vector> oldValues = {};
		DCO_GlobalAIComponent.DCO_GetOverrides(factionKey, oldClasses, oldValues);
		int playerId = DCO_PlayerId();
		string target = "Faction_" + factionKey;
		int changes = 0;
		foreach (int i, string className : classes)
		{
			SCR_BaseEditorAttribute attribute = DCO_FindByClass(className);
			string label = DCO_LabelOf(className);
			string now = DCO_SettingsBaseline.FormatValue(attribute, accepted[i]);
			int oldIndex = oldClasses.Find(className);
			string before = "global";
			string action = "added";
			if (oldIndex >= 0)
			{
				before = DCO_SettingsBaseline.FormatValue(attribute, oldValues[oldIndex]);
				action = "changed";
				if (before == now)
					continue;
			}
			changes++;
			DCO_BenchmarkLoggerComponent.Event(string.Format("GM %1 dco_settings_change target=%2 setting=%3 from=%4 to=%5 override=%6", DCO_LogWho(), target, DCO_SettingsBaseline.LogText(label), DCO_SettingsBaseline.LogText(before), DCO_SettingsBaseline.LogText(now), action));
			if (DCO_SettingsHelp.IsBeta(className))
				DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("dco_settings_beta_set setting=%1 value=%2 player=%3 target=%4", className, DCO_SettingsBaseline.LogText(now), playerId, target));
			DCO_SettingsChangeLog.Add(playerId, target, label, before, now);
		}
		foreach (int i, string className : oldClasses)
		{
			if (classes.Find(className) >= 0)
				continue;
			SCR_BaseEditorAttribute attribute = DCO_FindByClass(className);
			string label = DCO_LabelOf(className);
			string before = DCO_SettingsBaseline.FormatValue(attribute, oldValues[i]);
			changes++;
			DCO_BenchmarkLoggerComponent.Event(string.Format("GM %1 dco_settings_change target=%2 setting=%3 from=%4 to=global override=removed", DCO_LogWho(), target, DCO_SettingsBaseline.LogText(label), DCO_SettingsBaseline.LogText(before)));
			DCO_SettingsChangeLog.Add(playerId, target, label, before, "global");
		}

		DCO_GlobalAIComponent.DCO_SetOverrides(factionKey, classes, accepted, this);
		DCO_BenchmarkLoggerComponent.Event(string.Format("GM %1 dco_settings_apply target=%2 overrides=%3 changed=%4", DCO_LogWho(), target, classes.Count(), changes));
		Rpc(DCO_RpcEvent, 6, factionKey, "ok");
	}

	string DCO_LabelOf(string className)
	{
		SCR_BaseEditorAttribute attribute = DCO_FindByClass(className);
		if (attribute && attribute.GetUIInfo())
			return WidgetManager.Translate(attribute.GetUIInfo().GetName());
		return className;
	}

	void DCO_ApplyMembers(notnull array<string> classes, notnull array<float> values)
	{
		if (IsOwner() && m_bDCO_Mode && !classes.IsEmpty())
			Rpc(DCO_RpcApplyMembers, classes, values);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcApplyMembers(array<string> classes, array<float> values)
	{
		if (!m_aEditedItems || !m_PrefabData)
			return;
		int playerId = DCO_PlayerId();
		int written = 0;
		foreach (Managed item : m_aEditedItems)
		{
			SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
			if (!editable)
				continue;
			SCR_AIGroup group = SCR_AIGroup.Cast(editable.GetOwner());
			if (!group)
				continue;
			array<AIAgent> agents = {};
			group.GetAgents(agents);
			int groupMembers = 0;
			int groupPlayers = 0;
			DCO_SettingsMembers.Store(DCO_SettingsMembers.GroupKey(group), classes, values, this);
			foreach (AIAgent agent : agents)
			{
				if (!agent || !agent.GetControlledEntity())
					continue;
				if (DCO_SettingsMembers.IsPlayer(agent.GetControlledEntity()))
				{
					groupPlayers++;
					continue;
				}
				SCR_EditableEntityComponent member = SCR_EditableEntityComponent.Cast(agent.GetControlledEntity().FindComponent(SCR_EditableEntityComponent));
				if (!member)
					continue;
				groupMembers++;
				foreach (int i, string className : classes)
				{
					int k = i * 3;
					if (k + 2 >= values.Count())
						break;
					SCR_BaseEditorAttribute attribute = DCO_FindByClass(className);
					if (!attribute || DCO_SettingsTree.TargetOf(attribute) != 3 || !attribute.ReadVariable(member, this))
						continue;
					SCR_BaseEditorAttributeVar var = SCR_BaseEditorAttributeVar.CreateVector(Vector(values[k], values[k + 1], values[k + 2]));
					attribute.WriteVariable(member, var, this, playerId);
					written++;
				}
			}
			DCO_SettingsChangeLog.Add(playerId, DCO_SettingsBaseline.LogText(DCO_SettingsBaseline.ItemName(item)), "members_all", "-", classes.Count().ToString() + "_settings");
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_members_apply group=" + DCO_SettingsBaseline.LogText(DCO_SettingsBaseline.ItemName(item)) + " members=" + groupMembers.ToString() + " skipped_players=" + groupPlayers.ToString() + " settings=" + classes.Count().ToString());
		}
		DCO_BenchmarkLoggerComponent.Event(string.Format("GM %1 dco_settings_members target=%2 settings=%3 writes=%4", DCO_LogWho(), DCO_EditedTarget(), classes.Count(), written));
	}

	void DCO_RequestChangeLog(string target)
	{
		if (IsOwner())
			Rpc(DCO_RpcChangeLog, target);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcChangeLog(string target)
	{
		Rpc(DCO_RpcEvent, 4, target, DCO_SettingsChangeLog.Recent(target, 200));
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void DCO_RpcApplyBegin(array<int> attributeIds, int resetAll, int warnings)
	{
		if (!m_aEditedItems || !m_PrefabData)
			return;
		DCO_BenchmarkLoggerComponent.Event(string.Format("GM %1 dco_settings_review_confirm target=%2 changes=%3 warnings=%4", DCO_LogWho(), DCO_EditedTarget(), attributeIds.Count(), warnings));
		m_aDCO_LogIds = {};
		m_aDCO_LogItems = {};
		m_aDCO_LogOld = {};
		foreach (int id : attributeIds)
		{
			SCR_BaseEditorAttribute attribute = m_PrefabData.GetAttribute(id);
			if (!attribute)
				continue;
			foreach (int itemIndex, Managed item : m_aEditedItems)
			{
				SCR_BaseEditorAttributeVar var = attribute.ReadVariable(item, this);
				if (!var)
					continue;
				m_aDCO_LogIds.Insert(id);
				m_aDCO_LogItems.Insert(itemIndex);
				m_aDCO_LogOld.Insert(DCO_SettingsBaseline.FormatValue(attribute, var.GetVector()));
			}
		}
		if (resetAll)
			DCO_BenchmarkLoggerComponent.Event(string.Format("GM %1 dco_settings_reset_all target=%2", DCO_LogWho(), DCO_EditedTarget()));
	}

	protected void DCO_LogApplied()
	{
		if (!m_aDCO_LogIds || !m_aDCO_LogItems || !m_aEditedItems || !m_PrefabData)
			return;
		int changed = 0;
		foreach (int i, int id : m_aDCO_LogIds)
		{
			SCR_BaseEditorAttribute attribute = m_PrefabData.GetAttribute(id);
			int itemIndex = m_aDCO_LogItems[i];
			if (!attribute || !m_aEditedItems.IsIndexValid(itemIndex))
				continue;
			Managed item = m_aEditedItems[itemIndex];
			SCR_BaseEditorAttributeVar var = attribute.ReadVariable(item, this);
			if (!var)
				continue;
			string now = DCO_SettingsBaseline.FormatValue(attribute, var.GetVector());
			if (now == m_aDCO_LogOld[i])
				continue;
			changed++;
			string label = attribute.Type().ToString();
			if (attribute.GetUIInfo())
				label = WidgetManager.Translate(attribute.GetUIInfo().GetName());
			string target = DCO_SettingsBaseline.LogText(DCO_SettingsBaseline.ItemName(item));
			DCO_BenchmarkLoggerComponent.Event(string.Format("GM %1 dco_settings_change target=%2 setting=%3 from=%4 to=%5", DCO_LogWho(), target, DCO_SettingsBaseline.LogText(label), DCO_SettingsBaseline.LogText(m_aDCO_LogOld[i]), DCO_SettingsBaseline.LogText(now)));
			if (DCO_SettingsHelp.IsBeta(attribute.Type().ToString()))
				DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("dco_settings_beta_set setting=%1 value=%2 player=%3 target=%4", attribute.Type().ToString(), DCO_SettingsBaseline.LogText(now), DCO_PlayerId(), target));
			DCO_SettingsChangeLog.Add(DCO_PlayerId(), target, label, m_aDCO_LogOld[i], now);
		}
		DCO_BenchmarkLoggerComponent.Event(string.Format("GM %1 dco_settings_apply target=%2 items=%3 changed=%4", DCO_LogWho(), DCO_EditedTarget(), m_aEditedItems.Count(), changed));
		if (changed > 0 && (DCO_GlobalAIComponent.DCO_FactionActive() || DCO_GlobalAIComponent.DCO_CfgAnyFaction()))
		{
			foreach (Managed edited : m_aEditedItems)
			{
				if (BaseGameMode.Cast(edited))
				{
					DCO_GlobalAIComponent.DCO_RebuildAll(this);
					DCO_BenchmarkLoggerComponent.Event(string.Format("GM %1 dco_faction_rebuild reason=global_changed", DCO_LogWho()));
					break;
				}
			}
		}
		if (changed > 0)
			DCO_AutoSaveConfig();
		m_aDCO_LogIds = null;
		m_aDCO_LogItems = null;
		m_aDCO_LogOld = null;
	}

	protected void DCO_AutoSaveConfig()
	{
		bool global = false;
		foreach (Managed edited : m_aEditedItems)
		{
			if (BaseGameMode.Cast(edited))
			{
				global = true;
				break;
			}
		}
		if (!global)
			return;
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetInstance();
		if (!g || !g.DCO_GmAutoSave())
			return;
		int pid = DCO_PlayerId();
		string why;
		if (!DCO_GlobalAIComponent.DCO_CanSaveConfig(pid, why))
		{
			DCO_Perf.Count("cfg:autosave_denied");
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("cfg_autosave result=denied player=%1 mode=%2 (changes apply to this session only; an admin can save them)", pid, why));
			return;
		}
		string path;
		bool ok = g.DCO_SaveActiveConfig(pid, path);
		DCO_Perf.Count("cfg:autosave_" + DCO_Util.B01(ok).ToString());
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("cfg_autosave result=%1 player=%2 allowed_by=%3 path=%4", DCO_Util.B01(ok), pid, why, path));
		if (ok)
		{
			DCO_SettingsChangeLog.Add(pid, "Global", "auto_save_server_config", "-", path);
			DCO_Report.Gm("Saved to server config automatically", path);
		}
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void DCO_RpcDefaults(array<int> ids, array<float> values)
	{
		m_mDCO_Defaults = new map<int, vector>();
		foreach (int i, int id : ids)
		{
			int k = i * 3;
			if (k + 2 < values.Count())
				m_mDCO_Defaults.Set(id, Vector(values[k], values[k + 1], values[k + 2]));
		}
	}

	override protected int GetVariables(bool onlyServer, notnull array<Managed> items, notnull out array<int> outIds, notnull out array<ref SCR_BaseEditorAttributeVar> outVars, notnull out array<ref EEditorAttributeMultiSelect> outAttributesMultiSelect)
	{
		int start = outIds.Count();
		super.GetVariables(onlyServer, items, outIds, outVars, outAttributesMultiSelect);
		for (int i = outIds.Count() - 1; i >= start; i--)
		{
			if (DCO_SettingsTree.IsDCO(m_PrefabData.GetAttribute(outIds[i])) == m_bDCO_Mode)
				continue;
			outIds.RemoveOrdered(i);
			outVars.RemoveOrdered(i);
			outAttributesMultiSelect.RemoveOrdered(i);
		}
		return outVars.Count();
	}

	override void EOnEditorActivate()
	{
		super.EOnEditorActivate();
		if (!IsOwner())
			return;
		InputManager im = GetGame().GetInputManager();
		if (im)
			im.AddActionListener("DCO_OpenSettings", EActionTrigger.DOWN, DCO_OnHotkey);
		GetGame().GetCallqueue().Remove(DCO_HotkeyContext);
		GetGame().GetCallqueue().CallLater(DCO_HotkeyContext, 0, true);
		DCO_StartupQuery();
	}

	override void EOnEditorDeactivate()
	{
		GetGame().GetCallqueue().Remove(DCO_HotkeyContext);
		GetGame().GetCallqueue().Remove(DCO_StartupOpen);
		InputManager im = GetGame().GetInputManager();
		if (im)
			im.RemoveActionListener("DCO_OpenSettings", EActionTrigger.DOWN, DCO_OnHotkey);
		super.EOnEditorDeactivate();
	}

	protected void DCO_HotkeyContext()
	{
		InputManager im = GetGame().GetInputManager();
		if (im)
			im.ActivateContext("DCO_SettingsHotkeyContext");
	}

	protected void DCO_OnHotkey()
	{
		if (m_aEditedItems)
		{
			if (!m_bDCO_Mode)
				return;
			MenuBase menu = GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.DCO_SettingsDialog);
			DCO_SettingsDialogUI dialog = DCO_SettingsDialogUI.Cast(menu);
			if (dialog)
				dialog.HotkeyClose();
			else if (menu)
				GetGame().GetMenuManager().CloseMenu(menu);
			return;
		}
		DCO_SettingsChooserUI.LogOpenPath("hotkey");
		DCO_OpenSelectionOrGlobal();
	}

	override bool CanOpenAttributeDialog(notnull array<Managed> items)
	{
		return DCO_CanOpen(items, false);
	}

	override protected void Clean()
	{
		if (m_aDCO_LogIds && Replication.IsServer())
			DCO_LogApplied();
		m_aDCO_LogIds = null;
		m_aDCO_LogItems = null;
		m_aDCO_LogOld = null;
		super.Clean();
		m_bDCO_Mode = false;
		if (m_bDCO_Swapped)
		{
			m_MenuPreset = m_eDCO_SavedPreset;
			m_bDCO_Swapped = false;
		}
	}
}
