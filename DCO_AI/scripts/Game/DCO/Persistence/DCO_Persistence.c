class DCO_PersistenceLink
{
	static void AssignLoaded(AICommander_BaseComponent cmd, IEntity grpEnt, int tries)
	{
		if (!cmd || !grpEnt)
			return;
		DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(grpEnt.FindComponent(DCO_GroupUtilityComponent));
		string result = "no_group_util";
		if (g && g.GetFactionKey().IsEmpty() && tries < 10)
		{
			GetGame().GetCallqueue().CallLater(AssignLoaded, 2000, false, cmd, grpEnt, tries + 1);
			DCO_Perf.Count("persist:link_retry");
			return;
		}
		if (g)
		{
			if (cmd.AssignGroup(g))
				result = "ok";
			else
				result = "refused";
		}
		DCO_Perf.Count("persist:link_" + result);
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), string.Format("persist_group_link cmd=%1 grp=%2 result=%3 tries=%4", cmd.GetCommanderUID(), grpEnt.GetName(), result, tries));
	}
}

class DCO_CommanderSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType()
	{
		return AICommander_BaseComponent;
	}

	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull SaveContext context)
	{
		AICommander_BaseComponent cmd = AICommander_BaseComponent.Cast(component);
		if (!cmd)
			return ESerializeResult.DEFAULT;
		context.WriteValue("version", 3);
		cmd.DCO_SaveState(context);
		cmd.DCO_SavePresetState(context);
		int attrCount;
		string attrs = DCO_SettingsPersist.Dump(DCO_SettingsPersist.Editable(owner), attrCount);
		DCO_SettingsPersist.WriteParts(context, "attrs", attrs);
		array<UUID> groups = {};
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || !g.GetOwner())
				continue;
			UUID id = GetSystem().GetId(g.GetOwner());
			if (!id.IsNull())
				groups.Insert(id);
		}
		context.WriteValue("groups", groups);
		DCO_Perf.Count("persist:save_cmd");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), string.Format("persist_save_cmd cmd=%1 faction=%2 groups=%3 attrs=%4", cmd.GetCommanderUID(), cmd.GetCommanderFactionKey(), groups.Count(), attrCount));
		return ESerializeResult.OK;
	}

	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull LoadContext context)
	{
		AICommander_BaseComponent cmd = AICommander_BaseComponent.Cast(component);
		if (!cmd)
			return false;
		int version;
		context.ReadValue("version", version);
		string savedFaction = cmd.DCO_LoadState(context);
		if (version >= 3)
			cmd.DCO_LoadPresetState(context);
		int attrsApplied;
		int attrsSkipped;
		if (version >= 2)
			attrsApplied = DCO_SettingsPersist.Apply(DCO_SettingsPersist.Editable(owner), DCO_SettingsPersist.ReadParts(context, "attrs"), attrsSkipped);
		if (!savedFaction.IsEmpty() && cmd.GetCommanderFactionKey() != savedFaction)
		{
			DCO_Perf.Count("persist:cmd_faction_fix");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, DCO_Log.IdCmd(cmd), string.Format("persist_cmd_faction_fix cmd=%1 from=%2 to=%3", cmd.GetCommanderUID(), cmd.GetCommanderFactionKey(), savedFaction));
			cmd.SetCommanderFactionKey(savedFaction);
		}
		array<UUID> groups = {};
		context.ReadValue("groups", groups);
		foreach (UUID id : groups)
		{
			Tuple1<AICommander_BaseComponent> ctx = new Tuple1<AICommander_BaseComponent>(cmd);
			PersistenceWhenAvailableTask task = new PersistenceWhenAvailableTask(OnGroupAvailable, ctx);
			GetSystem().WhenAvailable(id, task, 120.0);
		}
		DCO_Perf.Count("persist:load_cmd");
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), string.Format("persist_load_cmd cmd=%1 faction=%2 groups=%3 version=%4 attrs_applied=%5 attrs_skipped=%6", cmd.GetCommanderUID(), cmd.GetCommanderFactionKey(), groups.Count(), version, attrsApplied, attrsSkipped));
		return true;
	}

	protected static void OnGroupAvailable(Managed instance, PersistenceDeferredDeserializeTask task, bool expired, Managed context)
	{
		IEntity ent = IEntity.Cast(instance);
		Tuple1<AICommander_BaseComponent> ctx = Tuple1<AICommander_BaseComponent>.Cast(context);
		if (!ctx || !ctx.param1)
			return;
		if (!ent || expired)
		{
			DCO_Perf.Count("persist:link_missing");
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, DCO_Log.IdCmd(ctx.param1), string.Format("persist_group_link cmd=%1 result=missing expired=%2", ctx.param1.GetCommanderUID(), DCO_Util.B01(expired)));
			return;
		}
		GetGame().GetCallqueue().CallLater(DCO_PersistenceLink.AssignLoaded, 2000, false, ctx.param1, ent, 0);
	}
}

class DCO_SpawnerSerializer : ScriptedComponentSerializer
{
	override static typename GetTargetType()
	{
		return DCO_CommanderSpawnerComponent;
	}

	override protected ESerializeResult Serialize(notnull IEntity owner, notnull GenericComponent component, notnull SaveContext context)
	{
		DCO_CommanderSpawnerComponent s = DCO_CommanderSpawnerComponent.Cast(component);
		if (!s)
			return ESerializeResult.DEFAULT;
		context.WriteValue("version", 1);
		s.DCO_SaveState(context);
		return ESerializeResult.OK;
	}

	override protected bool Deserialize(notnull IEntity owner, notnull GenericComponent component, notnull LoadContext context)
	{
		DCO_CommanderSpawnerComponent s = DCO_CommanderSpawnerComponent.Cast(component);
		if (!s)
			return false;
		int version;
		context.ReadValue("version", version);
		s.DCO_LoadState(context);
		return true;
	}
}
