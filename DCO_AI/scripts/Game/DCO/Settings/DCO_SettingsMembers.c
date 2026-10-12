class DCO_SettingsMembers
{
	protected static ref map<int, ref map<string, vector>> s_mStore;
	protected static SCR_AttributesManagerEditorComponent s_Manager;

	static int GroupKey(IEntity group)
	{
		if (!group)
			return 0;
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(group.FindComponent(SCR_EditableEntityComponent));
		if (!editable)
			return 0;
		return Replication.FindItemId(editable);
	}

	static bool IsPlayer(IEntity entity)
	{
		if (!entity)
			return false;
		PlayerManager players = GetGame().GetPlayerManager();
		return players && players.GetPlayerIdFromControlledEntity(entity) > 0;
	}

	static void Store(int key, notnull array<string> classes, notnull array<float> values, SCR_AttributesManagerEditorComponent manager)
	{
		if (key == 0)
			return;
		if (!s_mStore)
			s_mStore = new map<int, ref map<string, vector>>();
		map<string, vector> stored;
		if (!s_mStore.Find(key, stored) || !stored)
		{
			stored = new map<string, vector>();
			s_mStore.Set(key, stored);
		}
		foreach (int i, string className : classes)
		{
			int k = i * 3;
			if (k + 2 >= values.Count())
				break;
			stored.Set(className, Vector(values[k], values[k + 1], values[k + 2]));
		}
		s_Manager = manager;
	}

	static string StoredContent(int key)
	{
		map<string, vector> stored;
		if (!s_mStore || !s_mStore.Find(key, stored) || !stored)
			return string.Empty;
		string content;
		foreach (string className, vector value : stored)
		{
			if (!content.IsEmpty())
				content += ";";
			content += DCO_SettingsCodec.Entry(className, value);
		}
		return content;
	}

	static void OnJoin(SCR_AIGroup group, AIAgent agent)
	{
		if (!s_mStore || !s_Manager || !group || !agent || !Replication.IsServer())
			return;
		if (!s_mStore.Contains(GroupKey(group)))
			return;
		GetGame().GetCallqueue().CallLater(s_Manager.DCO_ApplyStored, 1500, false, group, agent);
	}

	static void ApplyStored(SCR_AIGroup group, AIAgent agent, SCR_AttributesManagerEditorComponent manager)
	{
		if (!group || !agent || !manager || !s_mStore)
			return;
		IEntity entity = agent.GetControlledEntity();
		if (!entity || IsPlayer(entity))
			return;
		map<string, vector> stored;
		if (!s_mStore.Find(GroupKey(group), stored) || !stored)
			return;
		SCR_EditableEntityComponent member = SCR_EditableEntityComponent.Cast(entity.FindComponent(SCR_EditableEntityComponent));
		if (!member)
			return;
		int written = 0;
		foreach (string className, vector value : stored)
		{
			SCR_BaseEditorAttribute attribute = manager.DCO_AttributeByClass(className);
			if (!attribute || !attribute.ReadVariable(member, manager))
				continue;
			attribute.WriteVariable(member, SCR_BaseEditorAttributeVar.CreateVector(value), manager, -1);
			written++;
		}
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "dco_settings_member_inherit group=" + DCO_SettingsBaseline.LogText(DCO_SettingsBaseline.ItemName(SCR_EditableEntityComponent.Cast(group.FindComponent(SCR_EditableEntityComponent)))) + " member=" + DCO_SettingsBaseline.LogText(DCO_SettingsBaseline.ItemName(member)) + " settings=" + written.ToString());
	}
}

modded class SCR_AIGroup
{
	override void OnAgentAdded(AIAgent child)
	{
		super.OnAgentAdded(child);
		DCO_SettingsMembers.OnJoin(this, child);
	}
}
