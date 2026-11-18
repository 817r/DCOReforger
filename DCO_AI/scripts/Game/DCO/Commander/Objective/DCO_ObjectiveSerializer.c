class DCO_ObjectiveSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType()
	{
		return CMD_AICommanderObjectiveComponent;
	}

	protected static void Defaults(IEntity owner, GenericComponent component, out string defName, out float defRadius, out string defOwner)
	{
		defRadius = 30;
		BaseContainer src = component.GetComponentSource(owner);
		if (!src)
			return;

		src.Get("m_sObjectiveName", defName);
		src.Get("m_fRadius", defRadius);
		src.Get("m_sInitialOwnerCommanderUID", defOwner);
	}

	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull SaveContext context)
	{
		CMD_AICommanderObjectiveComponent obj = CMD_AICommanderObjectiveComponent.Cast(component);
		if (!obj)
			return ESerializeResult.DEFAULT;

		string defName, defOwner;
		float defRadius;
		Defaults(owner, component, defName, defRadius, defOwner);

		string objectiveName = obj.GetObjectiveName();
		float radius = obj.GetRadius();
		string ownerUid = obj.GetOwnerCommanderUID();

		context.WriteValue("version", 3);
		context.WriteValueDefault("name", objectiveName, defName);
		context.WriteValueDefault("radius", radius, defRadius);
		context.WriteValueDefault("owner", ownerUid, defOwner);
		DCO_Perf.Count("obj_persist:save");
		obj.DCO_SaveState(context);
		int attrCount;
		DCO_SettingsPersist.WriteParts(context, "attrs", DCO_SettingsPersist.Dump(DCO_SettingsPersist.Editable(owner), attrCount));
		return ESerializeResult.OK;
	}

	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull LoadContext context)
	{
		CMD_AICommanderObjectiveComponent obj = CMD_AICommanderObjectiveComponent.Cast(component);
		if (!obj)
			return true;

		string defName, defOwner;
		float defRadius;
		Defaults(owner, component, defName, defRadius, defOwner);

		int version;
		context.ReadValue("version", version);

		string objectiveName;
		float radius;
		string ownerUid;
		context.ReadValueDefault("name", objectiveName, defName);
		context.ReadValueDefault("radius", radius, defRadius);
		context.ReadValueDefault("owner", ownerUid, defOwner);

		obj.DCO_ApplyPersisted(objectiveName, radius, ownerUid);
		if (version >= 2)
			obj.DCO_LoadState(context);
		if (version >= 3)
		{
			int skipped;
			int applied = DCO_SettingsPersist.Apply(DCO_SettingsPersist.Editable(owner), DCO_SettingsPersist.ReadParts(context, "attrs"), skipped);
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("persist_load_settings target=objective name=%1 applied=%2 skipped=%3", objectiveName, applied, skipped));
		}
		return true;
	}
}
