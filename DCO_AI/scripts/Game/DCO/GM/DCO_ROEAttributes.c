[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROEModeAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	protected DCO_EGroupTask Task() { return DCO_EGroupTask.NONE; }

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		m_aValues.Clear();
		array<string> labels = {"Hold fire", "Tight", "Free"};
		foreach (int i, string l : labels)
		{
			SCR_EditorAttributeFloatStringValueHolder value = new SCR_EditorAttributeFloatStringValueHolder();
			value.SetName(l);
			value.SetFloatValue(i);
			m_aValues.Insert(value);
		}
		return super.GetEntries(outEntries);
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!cmd)
			return null;
		DCO_ROEEntry e = cmd.DCO_GetROEEntry(Task());
		if (!e)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(e.m_eMode);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (cmd && var)
			cmd.DCO_SetROEEntry(Task(), Math.Round(var.GetFloat()), -1, -1, playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROEDistAttribute : SCR_BaseValueListEditorAttribute
{
	protected DCO_EGroupTask Task() { return DCO_EGroupTask.NONE; }
	protected bool Moving() { return false; }

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!cmd)
			return null;
		DCO_ROEEntry e = cmd.DCO_GetROEEntry(Task());
		if (!e)
			return null;
		if (Moving())
			return SCR_BaseEditorAttributeVar.CreateFloat(e.m_fEngageDistEnRoute);
		return SCR_BaseEditorAttributeVar.CreateFloat(e.m_fEngageDist);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!cmd || !var)
			return;
		if (Moving())
			cmd.DCO_SetROEEntry(Task(), -1, -1, var.GetFloat(), playerID);
		else
			cmd.DCO_SetROEEntry(Task(), -1, var.GetFloat(), -1, playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CmdGarrisonModeAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		m_aValues.Clear();
		array<string> labels = {"Hold", "Defend", "Release"};
		foreach (int i, string l : labels)
		{
			SCR_EditorAttributeFloatStringValueHolder value = new SCR_EditorAttributeFloatStringValueHolder();
			value.SetName(l);
			value.SetFloatValue(i);
			m_aValues.Insert(value);
		}
		return super.GetEntries(outEntries);
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!cmd)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(cmd.DCO_GetGarrisonModeDefault());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (cmd && var)
			cmd.DCO_SetGarrisonModeDefault(Math.Round(var.GetFloat()), playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROE_NoneModeAttribute : DCO_ROEModeAttribute
{
	override protected DCO_EGroupTask Task() { return DCO_EGroupTask.NONE; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROE_NoneDistAttribute : DCO_ROEDistAttribute
{
	override protected DCO_EGroupTask Task() { return DCO_EGroupTask.NONE; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROE_AttackModeAttribute : DCO_ROEModeAttribute
{
	override protected DCO_EGroupTask Task() { return DCO_EGroupTask.ATTACK; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROE_AttackDistAttribute : DCO_ROEDistAttribute
{
	override protected DCO_EGroupTask Task() { return DCO_EGroupTask.ATTACK; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROE_FlankModeAttribute : DCO_ROEModeAttribute
{
	override protected DCO_EGroupTask Task() { return DCO_EGroupTask.FLANK; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROE_FlankDistAttribute : DCO_ROEDistAttribute
{
	override protected DCO_EGroupTask Task() { return DCO_EGroupTask.FLANK; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROE_SupportByFireModeAttribute : DCO_ROEModeAttribute
{
	override protected DCO_EGroupTask Task() { return DCO_EGroupTask.SUPPORT_BY_FIRE; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROE_SupportByFireDistAttribute : DCO_ROEDistAttribute
{
	override protected DCO_EGroupTask Task() { return DCO_EGroupTask.SUPPORT_BY_FIRE; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROE_DefendModeAttribute : DCO_ROEModeAttribute
{
	override protected DCO_EGroupTask Task() { return DCO_EGroupTask.DEFEND; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROE_DefendDistAttribute : DCO_ROEDistAttribute
{
	override protected DCO_EGroupTask Task() { return DCO_EGroupTask.DEFEND; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROE_GarrisonModeAttribute : DCO_ROEModeAttribute
{
	override protected DCO_EGroupTask Task() { return DCO_EGroupTask.GARRISON; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROE_GarrisonDistAttribute : DCO_ROEDistAttribute
{
	override protected DCO_EGroupTask Task() { return DCO_EGroupTask.GARRISON; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROE_ReconModeAttribute : DCO_ROEModeAttribute
{
	override protected DCO_EGroupTask Task() { return DCO_EGroupTask.RECON; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROE_ReconDistAttribute : DCO_ROEDistAttribute
{
	override protected DCO_EGroupTask Task() { return DCO_EGroupTask.RECON; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROE_PatrolModeAttribute : DCO_ROEModeAttribute
{
	override protected DCO_EGroupTask Task() { return DCO_EGroupTask.PATROL; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROE_PatrolDistAttribute : DCO_ROEDistAttribute
{
	override protected DCO_EGroupTask Task() { return DCO_EGroupTask.PATROL; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROE_ReinforceModeAttribute : DCO_ROEModeAttribute
{
	override protected DCO_EGroupTask Task() { return DCO_EGroupTask.REINFORCE; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROE_ReinforceDistAttribute : DCO_ROEDistAttribute
{
	override protected DCO_EGroupTask Task() { return DCO_EGroupTask.REINFORCE; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROE_TransportModeAttribute : DCO_ROEModeAttribute
{
	override protected DCO_EGroupTask Task() { return DCO_EGroupTask.TRANSPORT; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROE_TransportDistAttribute : DCO_ROEDistAttribute
{
	override protected DCO_EGroupTask Task() { return DCO_EGroupTask.TRANSPORT; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROE_FireMissionModeAttribute : DCO_ROEModeAttribute
{
	override protected DCO_EGroupTask Task() { return DCO_EGroupTask.FIRE_MISSION; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ROE_FireMissionDistAttribute : DCO_ROEDistAttribute
{
	override protected DCO_EGroupTask Task() { return DCO_EGroupTask.FIRE_MISSION; }
}

