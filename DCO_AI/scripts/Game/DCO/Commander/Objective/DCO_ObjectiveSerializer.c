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
		if (objectiveName == defName && Math.AbsFloat(radius - defRadius) < 0.01 && ownerUid == defOwner)
			return ESerializeResult.DEFAULT;

		context.WriteValue("version", 1);
		context.WriteValueDefault("name", objectiveName, defName);
		context.WriteValueDefault("radius", radius, defRadius);
		context.WriteValueDefault("owner", ownerUid, defOwner);
		DCO_Perf.Count("obj_persist:save");
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
		return true;
	}
}
