class DCO_DoctrineGM
{
	static void TableClasses(bool commander, notnull array<string> outNames)
	{
		string prefix = "DCO_Doc";
		if (commander)
			prefix = "DCO_CmdDoc";
		array<string> bands = {"Strong", "Favorable", "Equal", "Weaker", "Unknown"};
		array<string> jobs = {"Supplies", "Reinforcement", "Support", "Firesupport", "Medevac"};
		foreach (string band : bands)
		{
			outNames.Insert(prefix + "Battle" + band + "Attribute");
		}
		foreach (string defBand : bands)
		{
			outNames.Insert(prefix + "Defense" + defBand + "Attribute");
		}
		foreach (string job : jobs)
		{
			outNames.Insert(prefix + "Support" + job + "Attribute");
		}
	}

	static string TableClass(int index, bool commander)
	{
		array<string> names = {};
		TableClasses(commander, names);
		if (!names.IsIndexValid(index))
			return string.Empty;
		return names[index];
	}

	static int TableIndex(string className)
	{
		array<string> names = {};
		TableClasses(false, names);
		int index = names.Find(className);
		if (index >= 0)
			return index;
		names.Clear();
		TableClasses(true, names);
		return names.Find(className);
	}

	static void NumberClasses(notnull array<string> outNames)
	{
		outNames.Insert("DCO_DocReconTimeoutAttribute");
		outNames.Insert("DCO_DocWeakerScaleAttribute");
		outNames.Insert("DCO_DocWaitTimeoutAttribute");
		outNames.Insert("DCO_DocReserveAttribute");
		outNames.Insert("DCO_DocAttackRatioAttribute");
		outNames.Insert("DCO_DocCounterMinAttribute");
		outNames.Insert("DCO_DocCounterMaxAttribute");
		outNames.Insert("DCO_DocRetreatLossAttribute");
		outNames.Insert("DCO_DocConsolidateAttribute");
	}

	static int NumberIndex(string className)
	{
		array<string> names = {};
		NumberClasses(names);
		return names.Find(className);
	}

	static void ForceClasses(notnull array<string> outNames, notnull array<string> outKeys, notnull array<int> outFields)
	{
		outNames.Insert("DCO_DocForceReserveMinAttribute");
		outKeys.Insert("reserve");
		outFields.Insert(0);
		outNames.Insert("DCO_DocForceReserveMaxAttribute");
		outKeys.Insert("reserve");
		outFields.Insert(2);
		outNames.Insert("DCO_DocForceSbfMinAttribute");
		outKeys.Insert("sbf");
		outFields.Insert(0);
		outNames.Insert("DCO_DocForceSbfShareAttribute");
		outKeys.Insert("sbf");
		outFields.Insert(1);
		outNames.Insert("DCO_DocForceSbfMaxAttribute");
		outKeys.Insert("sbf");
		outFields.Insert(2);
		outNames.Insert("DCO_DocForceAttackMinAttribute");
		outKeys.Insert("attack");
		outFields.Insert(0);
		outNames.Insert("DCO_DocForceAttackMaxAttribute");
		outKeys.Insert("attack");
		outFields.Insert(2);
		outNames.Insert("DCO_DocForceObjectivesMaxAttribute");
		outKeys.Insert("objectives");
		outFields.Insert(2);
		outNames.Insert("DCO_DocForceReconMinAttribute");
		outKeys.Insert("recon");
		outFields.Insert(0);
		outNames.Insert("DCO_DocForceReconFrontMaxAttribute");
		outKeys.Insert("recon_front");
		outFields.Insert(2);
		outNames.Insert("DCO_DocForceMtcLeadMinAttribute");
		outKeys.Insert("mtc_lead");
		outFields.Insert(0);
		outNames.Insert("DCO_DocForceMtcLeadShareAttribute");
		outKeys.Insert("mtc_lead");
		outFields.Insert(1);
		outNames.Insert("DCO_DocForceMtcLeadMaxAttribute");
		outKeys.Insert("mtc_lead");
		outFields.Insert(2);
		outNames.Insert("DCO_DocForceOpMaxAttribute");
		outKeys.Insert("op");
		outFields.Insert(2);
		outNames.Insert("DCO_DocForcePatrolMaxAttribute");
		outKeys.Insert("patrol");
		outFields.Insert(2);
		outNames.Insert("DCO_DocForceAmbushMaxAttribute");
		outKeys.Insert("ambush");
		outFields.Insert(2);
		outNames.Insert("DCO_DocForceRaidMinAttribute");
		outKeys.Insert("raid");
		outFields.Insert(0);
		outNames.Insert("DCO_DocForceRaidMaxAttribute");
		outKeys.Insert("raid");
		outFields.Insert(2);
		outNames.Insert("DCO_DocForceDefendFrontMinAttribute");
		outKeys.Insert("defend_front");
		outFields.Insert(0);
		outNames.Insert("DCO_DocForceDefendFrontShareAttribute");
		outKeys.Insert("defend_front");
		outFields.Insert(1);
		outNames.Insert("DCO_DocForceGarrisonMaxAttribute");
		outKeys.Insert("garrison");
		outFields.Insert(2);
		outNames.Insert("DCO_DocForceCounterMinAttribute");
		outKeys.Insert("counter");
		outFields.Insert(0);
		outNames.Insert("DCO_DocForceCounterMaxAttribute");
		outKeys.Insert("counter");
		outFields.Insert(2);
		outNames.Insert("DCO_DocForceDelayMoveShareAttribute");
		outKeys.Insert("delay_move");
		outFields.Insert(1);
		outNames.Insert("DCO_DocForceQrfMaxAttribute");
		outKeys.Insert("qrf");
		outFields.Insert(2);
		outNames.Insert("DCO_DocForceFeintMaxAttribute");
		outKeys.Insert("feint");
		outFields.Insert(2);
	}

	static int ForceIndex(string className, out string key, out int field)
	{
		array<string> names = {};
		array<string> keys = {};
		array<int> fields = {};
		ForceClasses(names, keys, fields);
		int i = names.Find(className);
		if (i < 0)
			return -1;
		key = keys[i];
		field = fields[i];
		return i;
	}

	static bool IsDoctrineClass(string className)
	{
		string fk;
		int ff;
		return className == "DCO_DocPresetAttribute" || TableIndex(className) >= 0 || NumberIndex(className) >= 0 || ForceIndex(className, fk, ff) >= 0;
	}

	static string PresetNameAt(int index)
	{
		array<string> names = {};
		array<bool> builtIn = {};
		DCO_DoctrineStore.List(names, builtIn);
		if (!names.IsIndexValid(index))
			return string.Empty;
		return names[index];
	}

	static int PresetIndexOf(string name)
	{
		array<string> names = {};
		array<bool> builtIn = {};
		DCO_DoctrineStore.List(names, builtIn);
		return names.Find(name);
	}

	static DCO_Doctrine ApplyOverrides(DCO_Doctrine source, map<string, vector> overrides)
	{
		if (!overrides)
			return source;
		DCO_Doctrine d = source;
		vector presetValue;
		if (overrides.Find("DCO_DocPresetAttribute", presetValue))
		{
			string name = PresetNameAt(presetValue[0]);
			DCO_DoctrinePreset preset = DCO_DoctrineStore.Find(name);
			if (preset)
				d.FromPreset(preset, "faction_gm");
		}
		foreach (string className, vector value : overrides)
		{
			int table = TableIndex(className);
			if (table >= 0 && d.Mask(table) != value[0])
			{
				d.SetMask(table, value[0]);
				d.m_bModified = true;
			}
			int number = NumberIndex(className);
			if (number >= 0 && Math.AbsFloat(d.Number(number) - value[0]) > 0.001)
			{
				d.SetNumber(number, value[0]);
				d.m_bModified = true;
			}
			string forceKey;
			int forceField;
			if (ForceIndex(className, forceKey, forceField) >= 0 && Math.AbsFloat(d.ForceValue(forceKey, forceField) - value[0]) > 0.001)
			{
				d.SetForce(forceKey, forceField, value[0]);
				d.m_bModified = true;
			}
		}
		return d;
	}

	static bool BuiltinRowValue(SCR_BaseEditorAttribute attribute, string factionKey, out vector value)
	{
		string builtinName = DCO_DoctrineStore.FactionDefault(factionKey);
		if (builtinName.IsEmpty() || !attribute)
			return false;
		DCO_DoctrinePreset preset = DCO_DoctrineStore.BuiltIn(builtinName);
		if (!preset)
			return false;
		DCO_Doctrine d = new DCO_Doctrine();
		d.FromPreset(preset, "builtin");
		string className = attribute.Type().ToString();
		if (className == "DCO_DocPresetAttribute")
		{
			value = Vector(Math.Max(0, PresetIndexOf(builtinName)), 0, 0);
			return true;
		}
		int table = TableIndex(className);
		if (table >= 0)
		{
			value = Vector(d.Mask(table), 0, 0);
			return true;
		}
		int number = NumberIndex(className);
		if (number >= 0)
		{
			value = Vector(d.Number(number), 0, 0);
			return true;
		}
		string forceKey;
		int forceField;
		if (ForceIndex(className, forceKey, forceField) >= 0)
		{
			value = Vector(d.ForceValue(forceKey, forceField), 0, 0);
			return true;
		}
		return false;
	}

	static void Log(string what, string target, string value)
	{
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "gm_doctrine_set what=" + what + " target=" + target + " value=" + value);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocTableAttribute : SCR_BaseMultiSelectPresetsEditorAttribute
{
	int GetIndex()
	{
		return DCO_DoctrineGM.TableIndex(Type().ToString());
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalFloatAttribute.GetGlobal(item);
		if (!cfg)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(cfg.DCO_GetDoctrine().Mask(GetIndex()));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalFloatAttribute.GetGlobal(item);
		if (!cfg || !var)
			return;
		cfg.DCO_SetDocMask(GetIndex(), var.GetInt());
		DCO_DoctrineGM.Log("table" + GetIndex().ToString(), "layer", var.GetInt().ToString());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocBattleStrongAttribute : DCO_DocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocBattleFavorableAttribute : DCO_DocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocBattleEqualAttribute : DCO_DocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocBattleWeakerAttribute : DCO_DocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocBattleUnknownAttribute : DCO_DocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocDefenseStrongAttribute : DCO_DocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocDefenseFavorableAttribute : DCO_DocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocDefenseEqualAttribute : DCO_DocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocDefenseWeakerAttribute : DCO_DocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocDefenseUnknownAttribute : DCO_DocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocSupportSuppliesAttribute : DCO_DocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocSupportReinforcementAttribute : DCO_DocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocSupportSupportAttribute : DCO_DocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocSupportFiresupportAttribute : DCO_DocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocSupportMedevacAttribute : DCO_DocTableAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocNumberAttribute : DCO_GlobalFloatAttribute
{
	int Which()
	{
		return DCO_DoctrineGM.NumberIndex(Type().ToString());
	}

	override protected float Get(DCO_GlobalAIComponent cfg)
	{
		return cfg.DCO_GetDoctrine().Number(Which());
	}

	override protected void Set(DCO_GlobalAIComponent cfg, float value)
	{
		cfg.DCO_SetDocNumber(Which(), value);
		DCO_DoctrineGM.Log("number" + Which().ToString(), "layer", value.ToString());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocReconTimeoutAttribute : DCO_DocNumberAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocWeakerScaleAttribute : DCO_DocNumberAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocWaitTimeoutAttribute : DCO_DocNumberAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocReserveAttribute : DCO_DocNumberAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocAttackRatioAttribute : DCO_DocNumberAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocCounterMinAttribute : DCO_DocNumberAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocCounterMaxAttribute : DCO_DocNumberAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocRetreatLossAttribute : DCO_DocNumberAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocConsolidateAttribute : DCO_DocNumberAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocPresetAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		m_aValues.Clear();
		array<string> names = {};
		array<bool> builtIn = {};
		DCO_DoctrineStore.List(names, builtIn);
		foreach (int i, string name : names)
		{
			SCR_EditorAttributeFloatStringValueHolder value = new SCR_EditorAttributeFloatStringValueHolder();
			value.SetName(name);
			value.SetFloatValue(i);
			m_aValues.Insert(value);
		}
		return super.GetEntries(outEntries);
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalFloatAttribute.GetGlobal(item);
		if (!cfg)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(Math.Max(0, DCO_DoctrineGM.PresetIndexOf(cfg.DCO_GetDoctrine().m_sPreset)));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalFloatAttribute.GetGlobal(item);
		if (!cfg || !var)
			return;
		string name = DCO_DoctrineGM.PresetNameAt(var.GetInt());
		if (name.IsEmpty() || name == cfg.DCO_GetDoctrine().m_sPreset)
			return;
		cfg.DCO_SetDocPreset(name);
		DCO_DoctrineGM.Log("preset", "layer", name);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdDocTableAttribute : SCR_BaseMultiSelectPresetsEditorAttribute
{
	int GetIndex()
	{
		return DCO_DoctrineGM.TableIndex(Type().ToString());
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!cmd || !cmd.GetDoctrine())
			return null;
		float overridden = 0;
		if (cmd.DCO_DocRowOverridden(GetIndex()))
			overridden = 1;
		return SCR_BaseEditorAttributeVar.CreateVector(Vector(cmd.GetDoctrine().Mask(GetIndex()), overridden, 0));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!cmd || !var)
			return;
		vector value = var.GetVector();
		cmd.DCO_SetDocOverride(GetIndex(), value[0]);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdDocBattleStrongAttribute : DCO_CmdDocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdDocBattleFavorableAttribute : DCO_CmdDocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdDocBattleEqualAttribute : DCO_CmdDocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdDocBattleWeakerAttribute : DCO_CmdDocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdDocBattleUnknownAttribute : DCO_CmdDocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdDocDefenseStrongAttribute : DCO_CmdDocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdDocDefenseFavorableAttribute : DCO_CmdDocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdDocDefenseEqualAttribute : DCO_CmdDocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdDocDefenseWeakerAttribute : DCO_CmdDocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdDocDefenseUnknownAttribute : DCO_CmdDocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdDocSupportSuppliesAttribute : DCO_CmdDocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdDocSupportReinforcementAttribute : DCO_CmdDocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdDocSupportSupportAttribute : DCO_CmdDocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdDocSupportFiresupportAttribute : DCO_CmdDocTableAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdDocSupportMedevacAttribute : DCO_CmdDocTableAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdDocPresetAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		m_aValues.Clear();
		SCR_EditorAttributeFloatStringValueHolder follow = new SCR_EditorAttributeFloatStringValueHolder();
		follow.SetName("Faction");
		follow.SetFloatValue(0);
		m_aValues.Insert(follow);
		array<string> names = {};
		array<bool> builtIn = {};
		DCO_DoctrineStore.List(names, builtIn);
		foreach (int i, string name : names)
		{
			SCR_EditorAttributeFloatStringValueHolder value = new SCR_EditorAttributeFloatStringValueHolder();
			value.SetName(name);
			value.SetFloatValue(i + 1);
			m_aValues.Insert(value);
		}
		return super.GetEntries(outEntries);
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!cmd)
			return null;
		int index = 0;
		if (!cmd.GetDoctrinePreset().IsEmpty())
			index = DCO_DoctrineGM.PresetIndexOf(cmd.GetDoctrinePreset()) + 1;
		return SCR_BaseEditorAttributeVar.CreateInt(Math.Max(0, index));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!cmd || !var)
			return;
		string name;
		if (var.GetInt() > 0)
			name = DCO_DoctrineGM.PresetNameAt(var.GetInt() - 1);
		cmd.DCO_SetDocPresetOverride(name);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdPersonalityPresetAttribute : DCO_CommanderBaseAttribute
{
	override protected void GetPresetLabels(notnull out array<string> outLabels)
	{
		DCO_Personality.Names(outLabels);
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = GetCommander(item);
		if (!cmd)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(DCO_Personality.IndexOf(cmd.GetPersonalityPreset()));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		AICommander_BaseComponent cmd = GetCommander(item);
		if (cmd && var)
			cmd.DCO_ApplyPersonalityPreset(DCO_Personality.NameAt(var.GetInt()));
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdTraitAttribute : DCO_CommanderPersonalityBaseAttribute
{
	protected int Which()
	{
		return 0;
	}

	override protected float ReadPersonality(AICommander_BaseComponent cmd)
	{
		switch (Which())
		{
			case 1: return cmd.GetTraitStrategy();
			case 2: return cmd.GetTraitAggression();
			case 3: return cmd.GetTraitLogistics();
			case 4: return cmd.GetTraitSupport();
			case 5: return cmd.GetTraitLeadership();
		}
		return cmd.GetTraitIntel();
	}

	override protected void WritePersonality(AICommander_BaseComponent cmd, float value)
	{
		cmd.DCO_SetTrait(Which(), value);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdTraitIntelAttribute : DCO_CmdTraitAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdTraitStrategyAttribute : DCO_CmdTraitAttribute { override protected int Which() { return 1; } }
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdTraitAggressionAttribute : DCO_CmdTraitAttribute { override protected int Which() { return 2; } }
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdTraitLogisticsAttribute : DCO_CmdTraitAttribute { override protected int Which() { return 3; } }
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdTraitSupportAttribute : DCO_CmdTraitAttribute { override protected int Which() { return 4; } }
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdTraitLeadershipAttribute : DCO_CmdTraitAttribute { override protected int Which() { return 5; } }

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdCourageAttribute : DCO_CommanderBaseAttribute
{
	override protected void GetPresetLabels(notnull out array<string> outLabels)
	{
		outLabels.Insert("Hesitant");
		outLabels.Insert("Careful");
		outLabels.Insert("Calculated");
		outLabels.Insert("Bold");
		outLabels.Insert("Reckless");
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = GetCommander(item);
		if (!cmd)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(cmd.GetCourage());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		AICommander_BaseComponent cmd = GetCommander(item);
		if (cmd && var)
			cmd.DCO_SetTrait(6, var.GetInt());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdDecisionAttribute : DCO_CommanderBaseAttribute
{
	override protected void GetPresetLabels(notnull out array<string> outLabels)
	{
		outLabels.Insert("Fast");
		outLabels.Insert("Moderate");
		outLabels.Insert("Slow");
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = GetCommander(item);
		if (!cmd)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(cmd.GetDecision());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		AICommander_BaseComponent cmd = GetCommander(item);
		if (cmd && var)
			cmd.DCO_SetTrait(7, var.GetInt());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdPreBattleIntelAttribute : DCO_CommanderBaseAttribute
{
	override protected void GetPresetLabels(notnull out array<string> outLabels)
	{
		outLabels.Insert("Accurate");
		outLabels.Insert("Normal");
		outLabels.Insert("Weak");
		outLabels.Insert("None");
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = GetCommander(item);
		if (!cmd)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(cmd.GetPreBattleIntel());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		AICommander_BaseComponent cmd = GetCommander(item);
		if (cmd && var)
		{
			cmd.DCO_SetPreBattleIntel(var.GetInt());
			DCO_DoctrineGM.Log("prebattle", cmd.GetCommanderUID(), var.GetInt().ToString());
		}
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdRandomPersonalityAttribute : SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!cmd)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(cmd.GetRandomPersonality());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (cmd && var)
		{
			cmd.DCO_SetRandomPersonality(var.GetBool());
			DCO_DoctrineGM.Log("random_personality", cmd.GetCommanderUID(), var.GetBool().ToString());
		}
	}
}

class DCO_ObjAttrUtil
{
	static CMD_AICommanderObjectiveComponent GetObjective(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable || !editable.GetOwner())
			return null;
		return CMD_AICommanderObjectiveComponent.Cast(editable.GetOwner().FindComponent(CMD_AICommanderObjectiveComponent));
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjStrategicValueAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAttrUtil.GetObjective(item);
		if (!obj)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(obj.GetStrategicValue());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAttrUtil.GetObjective(item);
		if (!obj || !var)
			return;
		obj.SetStrategicValue(var.GetInt());
		DCO_DoctrineGM.Log("strategic_value", obj.GetLogName(), var.GetInt().ToString());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjCategoryAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAttrUtil.GetObjective(item);
		if (!obj)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(obj.GetCategory());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAttrUtil.GetObjective(item);
		if (!obj || !var)
			return;
		obj.SetCategory(var.GetInt());
		DCO_DoctrineGM.Log("category", obj.GetLogName(), var.GetInt().ToString());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjRetainAttribute : SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAttrUtil.GetObjective(item);
		if (!obj)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(obj.IsRetain());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAttrUtil.GetObjective(item);
		if (!obj || !var)
			return;
		obj.SetRetain(var.GetBool());
		DCO_DoctrineGM.Log("retain", obj.GetLogName(), var.GetBool().ToString());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg)
	{
		string key;
		int field;
		DCO_DoctrineGM.ForceIndex(Type().ToString(), key, field);
		return cfg.DCO_GetDoctrine().ForceValue(key, field);
	}

	override protected void Set(DCO_GlobalAIComponent cfg, float value)
	{
		string key;
		int field;
		DCO_DoctrineGM.ForceIndex(Type().ToString(), key, field);
		cfg.DCO_SetDocForce(key, field, value);
		DCO_DoctrineGM.Log("force_" + key + "_" + field.ToString(), "layer", value.ToString());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceReserveMinAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceReserveMaxAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceSbfMinAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceSbfShareAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceSbfMaxAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceAttackMinAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceAttackMaxAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceObjectivesMaxAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceReconMinAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceReconFrontMaxAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceMtcLeadMinAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceMtcLeadShareAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceMtcLeadMaxAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceOpMaxAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForcePatrolMaxAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceAmbushMaxAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceRaidMinAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceRaidMaxAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceDefendFrontMinAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceDefendFrontShareAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceGarrisonMaxAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceCounterMinAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceCounterMaxAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceDelayMoveShareAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceQrfMaxAttribute : DCO_DocForceAttribute {}
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_DocForceFeintMaxAttribute : DCO_DocForceAttribute {}
