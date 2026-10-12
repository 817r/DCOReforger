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
		array<string> uids = {};
		array<FactionKey> factions = {};
		mgr.DCO_GetRoster(uids, factions);
		outUIDs.InsertAll(uids);
	}

	static void GetCommanderLabels(notnull array<string> outLabels)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;
		array<string> uids = {};
		array<FactionKey> factions = {};
		mgr.DCO_GetRoster(uids, factions);
		foreach (int i, string uid : uids)
			outLabels.Insert(uid + " · " + factions[i]);
	}

	static void GetObjectiveLabels(notnull array<string> outLabels)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return;
		array<string> names = {};
		mgr.DCO_GetObjectiveRoster(names);
		outLabels.InsertAll(names);
	}

	static int ObjectiveIndexOf(string storedName)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr || storedName.IsEmpty())
			return -1;
		return mgr.DCO_ObjectiveIndex(DCO_CommanderSpawnerComponent.FindObjective(storedName));
	}

	static string ObjectiveNameAt(int rosterIndex)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return string.Empty;
		CMD_AICommanderObjectiveComponent o = mgr.DCO_ObjectiveAt(rosterIndex);
		if (!o || !o.GetOwner())
			return string.Empty;
		string n = o.GetOwner().GetName();
		if (n.IsEmpty())
			n = o.GetLogName();
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("spawner_gm_pick kind=objective index=%1 obj=%2 stored=%3", rosterIndex, o.GetLogName(), n));
		return n;
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
		DCO_SpawnerGM.GetCommanderLabels(outLabels);
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
		DCO_SpawnerGM.GetObjectiveLabels(outLabels);
	}

	override protected int Read(DCO_CommanderSpawnerComponent s)
	{
		return DCO_SpawnerGM.ObjectiveIndexOf(s.GetTriggerObjective()) + 1;
	}

	override protected void Write(DCO_CommanderSpawnerComponent s, int index)
	{
		if (index <= 0)
			s.SetTriggerObjective(string.Empty);
		else
			s.SetTriggerObjective(DCO_SpawnerGM.ObjectiveNameAt(index - 1));
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

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderMaxAttackGroupsAttribute : DCO_CommanderSupportBaseAttribute
{
	override protected bool Read(AICommander_BaseComponent cmd, out float value) { value = cmd.GetMaxAttackGroups(); return true; }
	override protected void Write(AICommander_BaseComponent cmd, float value) { cmd.SetMaxAttackGroups(Math.Round(value)); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerStockModeAttribute : DCO_SpawnerPresetAttribute
{
	override protected void GetLabels(notnull array<string> outLabels)
	{
		outLabels.Insert("No");
		outLabels.Insert("Men");
		outLabels.Insert("Points");
	}
	override protected int Read(DCO_CommanderSpawnerComponent s)	{ return s.StockMode(); }
	override protected void Write(DCO_CommanderSpawnerComponent s, int index)	{ s.SetStockMode(index); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerExperienceAttribute : DCO_SpawnerPresetAttribute
{
	override protected void GetLabels(notnull array<string> outLabels)
	{
		outLabels.Insert("Recruit");
		outLabels.Insert("Regular");
		outLabels.Insert("Veteran");
		outLabels.Insert("Elite");
	}
	override protected int Read(DCO_CommanderSpawnerComponent s)	{ return s.GetDefExperience(); }
	override protected void Write(DCO_CommanderSpawnerComponent s, int index)	{ s.SetDefExperience(index); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerMoraleAttribute : DCO_SpawnerPresetAttribute
{
	override protected void GetLabels(notnull array<string> outLabels)
	{
		outLabels.Insert("Low");
		outLabels.Insert("Normal");
		outLabels.Insert("High");
	}
	override protected int Read(DCO_CommanderSpawnerComponent s)	{ return s.GetDefMorale(); }
	override protected void Write(DCO_CommanderSpawnerComponent s, int index)	{ s.SetDefMorale(index); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerShapeAttribute : DCO_SpawnerPresetAttribute
{
	override protected void GetLabels(notnull array<string> outLabels)
	{
		outLabels.Insert("Rested");
		outLabels.Insert("Reduced");
	}
	override protected int Read(DCO_CommanderSpawnerComponent s)	{ return s.GetDefShape(); }
	override protected void Write(DCO_CommanderSpawnerComponent s, int index)	{ s.SetDefShape(index); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerArrivalAttribute : DCO_SpawnerPresetAttribute
{
	override protected void GetLabels(notnull array<string> outLabels)
	{
		outLabels.Insert("Now");
		outLabels.Insert("10 min");
		outLabels.Insert("25 min");
		outLabels.Insert("40 min");
	}
	override protected int Read(DCO_CommanderSpawnerComponent s)
	{
		int m = s.GetDefArrivalMin();
		if (m >= 40)
			return 3;
		if (m >= 25)
			return 2;
		if (m >= 10)
			return 1;
		return 0;
	}
	override protected void Write(DCO_CommanderSpawnerComponent s, int index)
	{
		int mins = 0;
		if (index == 1)
			mins = 10;
		else if (index == 2)
			mins = 25;
		else if (index == 3)
			mins = 40;
		s.SetDefArrivalMin(mins);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerBackOnAttribute : DCO_SpawnerPresetAttribute
{
	override protected void GetLabels(notnull array<string> outLabels)
	{
		outLabels.Insert("Back on");
		outLabels.Insert("Stays off");
	}
	override protected int Read(DCO_CommanderSpawnerComponent s)
	{
		if (s.GetBackOn())
			return 0;
		return 1;
	}
	override protected void Write(DCO_CommanderSpawnerComponent s, int index)	{ s.SetBackOn(index == 0); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerHomeAttribute : DCO_SpawnerPresetAttribute
{
	override protected void GetLabels(notnull array<string> outLabels)
	{
		outLabels.Insert("Auto");
		outLabels.Insert("None");
		DCO_SpawnerGM.GetObjectiveLabels(outLabels);
	}
	override protected int Read(DCO_CommanderSpawnerComponent s)
	{
		string v = s.GetHomeObjectiveSetting();
		if (v.IsEmpty())
			return 0;
		if (v == "-")
			return 1;
		int idx = DCO_SpawnerGM.ObjectiveIndexOf(v);
		if (idx < 0)
			return 0;
		return idx + 2;
	}
	override protected void Write(DCO_CommanderSpawnerComponent s, int index)
	{
		if (index <= 0)
		{
			s.SetHomeObjectiveSetting(string.Empty);
			return;
		}
		if (index == 1)
		{
			s.SetHomeObjectiveSetting("-");
			return;
		}
		string picked = DCO_SpawnerGM.ObjectiveNameAt(index - 2);
		if (!picked.IsEmpty())
			s.SetHomeObjectiveSetting(picked);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerPresetChoiceAttribute : DCO_SpawnerPresetAttribute
{
	override protected void GetLabels(notnull array<string> outLabels)
	{
		outLabels.Insert("One-shot");
		outLabels.Insert("Counterattack wave");
		outLabels.Insert("Steady top-up");
		outLabels.Insert("Commander on call");
		outLabels.Insert("Custom");
	}
	override protected int Read(DCO_CommanderSpawnerComponent s)	{ return s.GetPreset(); }
	override protected void Write(DCO_CommanderSpawnerComponent s, int index)
	{
		if (index != s.GetPreset())
			s.ApplyPreset(index);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerStopOverrunAttribute : DCO_SpawnerBoolAttribute
{
	override protected bool Get(DCO_CommanderSpawnerComponent s) { return s.GetStopOverrun(); }
	override protected void Set(DCO_CommanderSpawnerComponent s, bool value) { s.SetStopOverrun(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerStopHomeAttribute : DCO_SpawnerBoolAttribute
{
	override protected bool Get(DCO_CommanderSpawnerComponent s) { return s.GetStopHomeLost(); }
	override protected void Set(DCO_CommanderSpawnerComponent s, bool value) { s.SetStopHomeLost(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerStopCommanderAttribute : DCO_SpawnerBoolAttribute
{
	override protected bool Get(DCO_CommanderSpawnerComponent s) { return s.GetStopCommanderGone(); }
	override protected void Set(DCO_CommanderSpawnerComponent s, bool value) { s.SetStopCommanderGone(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerStopLateAttribute : DCO_SpawnerBoolAttribute
{
	override protected bool Get(DCO_CommanderSpawnerComponent s) { return s.GetStopLate(); }
	override protected void Set(DCO_CommanderSpawnerComponent s, bool value) { s.SetStopLate(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerLateMinAttribute : DCO_SpawnerFloatAttribute
{
	override protected float Get(DCO_CommanderSpawnerComponent s) { return s.GetLateMin(); }
	override protected void Set(DCO_CommanderSpawnerComponent s, float value) { s.SetLateMin(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerHoldTimeAttribute : DCO_SpawnerFloatAttribute
{
	override protected float Get(DCO_CommanderSpawnerComponent s) { return s.GetHoldTime(); }
	override protected void Set(DCO_CommanderSpawnerComponent s, float value) { s.SetHoldTime(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerOnAttribute : SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_CommanderSpawnerComponent s = DCO_SpawnerGM.GetSpawner(item);
		if (!s)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(!s.IsGMOff());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_CommanderSpawnerComponent s = DCO_SpawnerGM.GetSpawner(item);
		if (s && var)
			s.SetGMOff(!var.GetBool(), playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderPointsStockAttribute : DCO_CommanderSupportBaseAttribute
{
	override protected bool Read(AICommander_BaseComponent cmd, out float value) { value = cmd.GetPointsStock(); return true; }
	override protected void Write(AICommander_BaseComponent cmd, float value) { cmd.SetPointsStock(Math.Round(value)); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SpawnerVehiclesAttribute : DCO_SpawnerPresetAttribute
{
	override protected void GetLabels(notnull array<string> outLabels)
	{
		outLabels.Insert("Group mounts");
		outLabels.Insert("Crewed");
	}
	override protected int Read(DCO_CommanderSpawnerComponent s)
	{
		if (s.GetCrewed())
			return 1;
		return 0;
	}
	override protected void Write(DCO_CommanderSpawnerComponent s, int index)	{ s.SetCrewed(index == 1); }
}
