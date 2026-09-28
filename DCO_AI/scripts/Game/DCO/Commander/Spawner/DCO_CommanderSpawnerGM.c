class DCO_SpawnerGM
{
	static DCO_CommanderSpawnerComponent GetSpawner(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable || !editable.GetOwner())
			return null;
		return DCO_CommanderSpawnerComponent.Cast(editable.GetOwner().FindComponent(DCO_CommanderSpawnerComponent));
	}

	static void GetCommanderUIDs(notnull array<string> outUIDs)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;
		foreach (AICommander_BaseComponent c : mgr.m_aCommander)
		{
			if (c)
				outUIDs.Insert(c.GetCommanderUID());
		}
	}

	static void GetObjectiveNames(notnull array<string> outNames)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;
		foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
		{
			if (o && o.GetOwner() && !o.GetOwner().GetName().IsEmpty())
				outNames.Insert(o.GetOwner().GetName());
		}
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerPresetAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	protected void GetLabels(notnull array<string> outLabels) {}
	protected int Read(DCO_CommanderSpawnerComponent s) { return 0; }
	protected void Write(DCO_CommanderSpawnerComponent s, int index) {}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		m_aValues.Clear();
		array<string> labels = {};
		GetLabels(labels);
		for (int i = 0; i < labels.Count(); i++)
		{
			SCR_EditorAttributeFloatStringValueHolder value = new SCR_EditorAttributeFloatStringValueHolder();
			value.SetName(labels[i]);
			value.SetFloatValue(i);
			m_aValues.Insert(value);
		}
		return super.GetEntries(outEntries);
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_CommanderSpawnerComponent s = DCO_SpawnerGM.GetSpawner(item);
		if (!s)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(Read(s));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_CommanderSpawnerComponent s = DCO_SpawnerGM.GetSpawner(item);
		if (s && var)
			Write(s, var.GetInt());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerTargetAttribute : DCO_SpawnerPresetAttribute
{
	override protected void GetLabels(notnull array<string> outLabels)
	{
		outLabels.Insert("Weakest of faction");
		outLabels.Insert("Nearest of faction");
		DCO_SpawnerGM.GetCommanderUIDs(outLabels);
	}

	override protected int Read(DCO_CommanderSpawnerComponent s)
	{
		if (s.GetTargetMode() == DCO_ESpawnerTarget.NEAREST_OF_FACTION)
			return 1;
		if (s.GetTargetMode() == DCO_ESpawnerTarget.WEAKEST_OF_FACTION)
			return 0;
		array<string> uids = {};
		DCO_SpawnerGM.GetCommanderUIDs(uids);
		int idx = uids.Find(s.GetCommanderUID());
		if (idx < 0)
			return 0;
		return idx + 2;
	}

	override protected void Write(DCO_CommanderSpawnerComponent s, int index)
	{
		if (index == 0)
		{
			s.SetTarget(DCO_ESpawnerTarget.WEAKEST_OF_FACTION, s.GetCommanderUID());
			return;
		}
		if (index == 1)
		{
			s.SetTarget(DCO_ESpawnerTarget.NEAREST_OF_FACTION, s.GetCommanderUID());
			return;
		}
		array<string> uids = {};
		DCO_SpawnerGM.GetCommanderUIDs(uids);
		if (uids.IsIndexValid(index - 2))
			s.SetTarget(DCO_ESpawnerTarget.SPECIFIC_UID, uids[index - 2]);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerFactionAttribute : DCO_SpawnerPresetAttribute
{
	protected static void Keys(notnull array<string> outKeys)
	{
		FactionManager fm = GetGame().GetFactionManager();
		if (!fm)
			return;
		array<Faction> factions = {};
		fm.GetFactionsList(factions);
		foreach (Faction f : factions)
		{
			if (f)
				outKeys.Insert(f.GetFactionKey());
		}
	}

	override protected void GetLabels(notnull array<string> outLabels)
	{
		outLabels.Insert("Any");
		Keys(outLabels);
	}

	override protected int Read(DCO_CommanderSpawnerComponent s)
	{
		array<string> keys = {};
		Keys(keys);
		return keys.Find(s.GetFaction()) + 1;
	}

	override protected void Write(DCO_CommanderSpawnerComponent s, int index)
	{
		array<string> keys = {};
		Keys(keys);
		if (index <= 0 || !keys.IsIndexValid(index - 1))
			s.SetFaction(string.Empty);
		else
			s.SetFaction(keys[index - 1]);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerObjectiveAttribute : DCO_SpawnerPresetAttribute
{
	override protected void GetLabels(notnull array<string> outLabels)
	{
		outLabels.Insert("None");
		DCO_SpawnerGM.GetObjectiveNames(outLabels);
	}

	override protected int Read(DCO_CommanderSpawnerComponent s)
	{
		array<string> names = {};
		DCO_SpawnerGM.GetObjectiveNames(names);
		return names.Find(s.GetTriggerObjective()) + 1;
	}

	override protected void Write(DCO_CommanderSpawnerComponent s, int index)
	{
		array<string> names = {};
		DCO_SpawnerGM.GetObjectiveNames(names);
		if (index <= 0 || !names.IsIndexValid(index - 1))
			s.SetTriggerObjective(string.Empty);
		else
			s.SetTriggerObjective(names[index - 1]);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerObjectiveEventAttribute : DCO_SpawnerPresetAttribute
{
	override protected void GetLabels(notnull array<string> outLabels)
	{
		outLabels.Insert("None");
		outLabels.Insert("Lost");
		outLabels.Insert("Captured");
	}

	override protected int Read(DCO_CommanderSpawnerComponent s)	{ return s.GetObjectiveEvent(); }
	override protected void Write(DCO_CommanderSpawnerComponent s, int index)	{ s.SetObjectiveEvent(index); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerFloatAttribute : SCR_BaseValueListEditorAttribute
{
	protected float Get(DCO_CommanderSpawnerComponent s) { return 0; }
	protected void Set(DCO_CommanderSpawnerComponent s, float value) {}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_CommanderSpawnerComponent s = DCO_SpawnerGM.GetSpawner(item);
		if (!s)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(Get(s));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_CommanderSpawnerComponent s = DCO_SpawnerGM.GetSpawner(item);
		if (s && var)
			Set(s, var.GetFloat());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerBoolAttribute : SCR_BaseEditorAttribute
{
	protected bool Get(DCO_CommanderSpawnerComponent s) { return false; }
	protected void Set(DCO_CommanderSpawnerComponent s, bool value) {}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_CommanderSpawnerComponent s = DCO_SpawnerGM.GetSpawner(item);
		if (!s)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(Get(s));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_CommanderSpawnerComponent s = DCO_SpawnerGM.GetSpawner(item);
		if (s && var)
			Set(s, var.GetBool());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerGroupsPerSpawnAttribute : DCO_SpawnerFloatAttribute
{
	override protected float Get(DCO_CommanderSpawnerComponent s) { return s.GetGroupsPerSpawn(); }
	override protected void Set(DCO_CommanderSpawnerComponent s, float value) { s.SetGroupsPerSpawn(Math.Round(value)); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerRadiusAttribute : DCO_SpawnerFloatAttribute
{
	override protected float Get(DCO_CommanderSpawnerComponent s) { return s.GetSpawnRadius(); }
	override protected void Set(DCO_CommanderSpawnerComponent s, float value) { s.SetSpawnRadius(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerCooldownAttribute : DCO_SpawnerFloatAttribute
{
	override protected float Get(DCO_CommanderSpawnerComponent s) { return s.GetCooldown(); }
	override protected void Set(DCO_CommanderSpawnerComponent s, float value) { s.SetCooldown(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerManpowerBelowAttribute : DCO_SpawnerFloatAttribute
{
	override protected float Get(DCO_CommanderSpawnerComponent s) { return s.GetManpowerBelow(); }
	override protected void Set(DCO_CommanderSpawnerComponent s, float value) { s.SetManpowerBelow(Math.Round(value)); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerSafeRadiusAttribute : DCO_SpawnerFloatAttribute
{
	override protected float Get(DCO_CommanderSpawnerComponent s) { return s.GetSafeRadius(); }
	override protected void Set(DCO_CommanderSpawnerComponent s, float value) { s.SetSafeRadius(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerAtStartAttribute : DCO_SpawnerBoolAttribute
{
	override protected bool Get(DCO_CommanderSpawnerComponent s) { return s.GetSpawnAtStart(); }
	override protected void Set(DCO_CommanderSpawnerComponent s, bool value) { s.SetSpawnAtStart(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerUseStockAttribute : DCO_SpawnerBoolAttribute
{
	override protected bool Get(DCO_CommanderSpawnerComponent s) { return s.GetUseStock(); }
	override protected void Set(DCO_CommanderSpawnerComponent s, bool value) { s.SetUseStock(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderStockAttribute : DCO_CommanderSupportBaseAttribute
{
	override protected bool Read(AICommander_BaseComponent cmd, out float value) { value = cmd.GetReinforcementStock(); return true; }
	override protected void Write(AICommander_BaseComponent cmd, float value) { cmd.SetReinforcementStock(Math.Round(value)); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalCommanderStockAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetCommanderStockDefault(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetCommanderStockDefault(Math.Round(value)); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalSpawnerAICapAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetSpawnerFactionAICap(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value) { cfg.SetSpawnerFactionAICap(Math.Round(value)); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerCommanderChoosesAttribute : DCO_SpawnerBoolAttribute
{
	override protected bool Get(DCO_CommanderSpawnerComponent s) { return s.GetCommanderChooses(); }
	override protected void Set(DCO_CommanderSpawnerComponent s, bool value) { s.SetCommanderChooses(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerCommanderRequestAttribute : DCO_SpawnerBoolAttribute
{
	override protected bool Get(DCO_CommanderSpawnerComponent s) { return s.GetCommanderRequest(); }
	override protected void Set(DCO_CommanderSpawnerComponent s, bool value) { s.SetCommanderRequest(value); }
}
