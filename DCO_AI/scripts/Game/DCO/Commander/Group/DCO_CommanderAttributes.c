[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderBaseAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	static AICommander_BaseComponent GetCommander(Managed item)
	{
		SCR_EditableEntityComponent editable = SCR_EditableEntityComponent.Cast(item);
		if (!editable)
			return null;

		IEntity owner = editable.GetOwner();
		if (!owner)
			return null;

		return AICommander_BaseComponent.Cast(owner.FindComponent(AICommander_BaseComponent));
	}

	protected void GetPresetLabels(notnull out array<string> outLabels)
	{
	}

	protected float GetPresetValue(int index)
	{
		return index;
	}

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		m_aValues.Clear();

		array<string> labels = {};
		GetPresetLabels(labels);

		for (int i = 0; i < labels.Count(); i++)
		{
			SCR_EditorAttributeFloatStringValueHolder value = new SCR_EditorAttributeFloatStringValueHolder();
			value.SetName(labels[i]);
			value.SetFloatValue(GetPresetValue(i));
			m_aValues.Insert(value);
		}

		return super.GetEntries(outEntries);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderFactionAttribute : DCO_CommanderBaseAttribute
{
	protected static void GetFactionKeys(notnull out array<FactionKey> outKeys)
	{
		outKeys.Clear();
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

	override protected void GetPresetLabels(notnull out array<string> outLabels)
	{
		array<FactionKey> keys = {};
		GetFactionKeys(keys);
		outLabels.Clear();
		foreach (FactionKey k : keys)
			outLabels.Insert(k);
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = GetCommander(item);
		if (!cmd)
			return null;

		array<FactionKey> keys = {};
		GetFactionKeys(keys);
		return SCR_BaseEditorAttributeVar.CreateInt(Math.Max(keys.Find(cmd.GetCommanderFactionKey()), 0));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		AICommander_BaseComponent cmd = GetCommander(item);
		if (!cmd)
			return;

		array<FactionKey> keys = {};
		GetFactionKeys(keys);
		int index = var.GetInt();
		if (keys.IsIndexValid(index))
			cmd.SetCommanderFactionKey(keys[index]);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderModeAttribute : DCO_CommanderBaseAttribute
{
	override protected void GetPresetLabels(notnull out array<string> outLabels)
	{
		outLabels.Clear();
		outLabels.Insert("Offensive");
		outLabels.Insert("Defensive");
		outLabels.Insert("Balanced");
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = GetCommander(item);
		if (!cmd)
			return null;

		return SCR_BaseEditorAttributeVar.CreateInt(cmd.GetCommanderMode());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		AICommander_BaseComponent cmd = GetCommander(item);
		if (!cmd)
			return;

		cmd.SetCommanderMode(var.GetInt());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderPersonalityBaseAttribute : SCR_BaseValueListEditorAttribute
{
	protected float ReadPersonality(AICommander_BaseComponent cmd)
	{
		return 0.5;
	}

	protected void WritePersonality(AICommander_BaseComponent cmd, float value)
	{
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!cmd)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(ReadPersonality(cmd));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;

		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (cmd)
			WritePersonality(cmd, var.GetFloat());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderAggressionAttribute : DCO_CommanderPersonalityBaseAttribute
{
	override protected float ReadPersonality(AICommander_BaseComponent cmd) { return cmd.GetAggression(); }
	override protected void WritePersonality(AICommander_BaseComponent cmd, float value) { cmd.SetAggression(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderAdaptabilityAttribute : DCO_CommanderPersonalityBaseAttribute
{
	override protected float ReadPersonality(AICommander_BaseComponent cmd) { return cmd.GetAdaptability(); }
	override protected void WritePersonality(AICommander_BaseComponent cmd, float value) { cmd.SetAdaptability(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderRiskTakingAttribute : DCO_CommanderPersonalityBaseAttribute
{
	override protected float ReadPersonality(AICommander_BaseComponent cmd) { return cmd.GetRiskTaking(); }
	override protected void WritePersonality(AICommander_BaseComponent cmd, float value) { cmd.SetRiskTaking(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderResilienceAttribute : DCO_CommanderPersonalityBaseAttribute
{
	override protected float ReadPersonality(AICommander_BaseComponent cmd) { return cmd.GetResilience(); }
	override protected void WritePersonality(AICommander_BaseComponent cmd, float value) { cmd.SetResilience(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderPatienceAttribute : DCO_CommanderPersonalityBaseAttribute
{
	override protected float ReadPersonality(AICommander_BaseComponent cmd) { return cmd.GetPatience(); }
	override protected void WritePersonality(AICommander_BaseComponent cmd, float value) { cmd.SetPatience(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderCombatFocusAttribute : DCO_CommanderPersonalityBaseAttribute
{
	override protected float ReadPersonality(AICommander_BaseComponent cmd) { return cmd.GetCombatFocus(); }
	override protected void WritePersonality(AICommander_BaseComponent cmd, float value) { cmd.SetCombatFocus(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderMaxFrontlineReconAttribute : DCO_CommanderPersonalityBaseAttribute
{
	override protected float ReadPersonality(AICommander_BaseComponent cmd) { return cmd.GetMaxFrontlineRecon(); }
	override protected void WritePersonality(AICommander_BaseComponent cmd, float value) { cmd.SetMaxFrontlineRecon(Math.Round(value)); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderVehiclePatrolRadiusAttribute : DCO_CommanderPersonalityBaseAttribute
{
	override protected float ReadPersonality(AICommander_BaseComponent cmd) { return cmd.GetVehiclePatrolRadiusMul(); }
	override protected void WritePersonality(AICommander_BaseComponent cmd, float value) { cmd.SetVehiclePatrolRadiusMul(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderVehicleSearchRadiusAttribute : DCO_CommanderPersonalityBaseAttribute
{
	override protected float ReadPersonality(AICommander_BaseComponent cmd) { return cmd.GetVehicleSearchRadius(); }
	override protected void WritePersonality(AICommander_BaseComponent cmd, float value) { cmd.SetVehicleSearchRadius(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderVehiclePatrolAttribute : SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!cmd)
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(cmd.GetVehiclePatrol());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (cmd && var)
			cmd.SetVehiclePatrol(var.GetBool());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderAllowedTacticsAttribute : SCR_BaseMultiSelectPresetsEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!cmd)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(cmd.GetAllowedTactics());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (cmd)
			cmd.SetAllowedTactics(var.GetInt());
	}
}
