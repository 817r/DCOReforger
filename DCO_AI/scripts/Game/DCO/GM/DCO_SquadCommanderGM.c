[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SquadCmdBoolAttribute : SCR_BaseEditorAttribute
{
	protected bool Get(DCO_GroupUtilityComponent g) { return false; }
	protected void Set(DCO_GroupUtilityComponent g, bool value) {}

	static DCO_GroupUtilityComponent GetSquad(Managed item)
	{
		SCR_EditableGroupComponent editable = SCR_EditableGroupComponent.Cast(item);
		if (!editable || editable.HasEntityState(EEditableEntityState.PLAYER))
			return null;

		SCR_AIGroup grp = editable.GetAIGroupComponent();
		if (!grp)
			return null;

		return DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GroupUtilityComponent g = GetSquad(item);
		if (!g || !g.IsCommanderEligible())
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(Get(g));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GroupUtilityComponent g = GetSquad(item);
		if (g && var)
			Set(g, var.GetBool());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SquadAllowReinforceAttribute : DCO_SquadCmdBoolAttribute
{
	override protected bool Get(DCO_GroupUtilityComponent g) { return g.CanReinforce(); }
	override protected void Set(DCO_GroupUtilityComponent g, bool value) { g.SetCanReinforce(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SquadCallArtyAttribute : DCO_SquadCmdBoolAttribute
{
	override protected bool Get(DCO_GroupUtilityComponent g) { return g.CanCallArty(); }
	override protected void Set(DCO_GroupUtilityComponent g, bool value) { g.SetCanCallArty(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SquadCallReinforcementAttribute : DCO_SquadCmdBoolAttribute
{
	override protected bool Get(DCO_GroupUtilityComponent g) { return g.CanCallReinforcement(); }
	override protected void Set(DCO_GroupUtilityComponent g, bool value) { g.SetCanCallReinforcement(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SquadOverrideTaskAttribute : DCO_SquadCmdBoolAttribute
{
	override protected bool Get(DCO_GroupUtilityComponent g) { return g.CanCommanderOverrideRole(); }
	override protected void Set(DCO_GroupUtilityComponent g, bool value) { g.SetCanCommanderOverrideRole(value); }

	override void UpdateInterlinkedVariables(SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, bool isInit = false)
	{
		if (!var || !manager)
			return;
		bool on = var.GetBool();
		if (isInit)
		{
			manager.SetAttributeAsSubAttribute(DCO_SquadTaskPresetAttribute);
			manager.SetAttributeAsSubAttribute(DCO_SquadAllowedTasksAttribute);
			manager.SetAttributeAsSubAttribute(DCO_SquadFirstToFightAttribute);
		}
		manager.SetAttributeEnabled(DCO_SquadTaskPresetAttribute, on);
		manager.SetAttributeEnabled(DCO_SquadAllowedTasksAttribute, on);
		manager.SetAttributeEnabled(DCO_SquadFirstToFightAttribute, on);
		if (!isInit)
			DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("grpjob_ui_gate can_task=%1 ui=vanilla", DCO_Util.B01(on)));
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SquadAllowTransportAttribute : DCO_SquadCmdBoolAttribute
{
	override protected bool Get(DCO_GroupUtilityComponent g) { return g.CanBeTransported(); }
	override protected void Set(DCO_GroupUtilityComponent g, bool value) { g.SetCanBeTransported(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SquadAllowPatrolAttribute : DCO_SquadCmdBoolAttribute
{
	override protected bool Get(DCO_GroupUtilityComponent g) { return g.CanPatrol(); }
	override protected void Set(DCO_GroupUtilityComponent g, bool value) { g.SetCanPatrol(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SquadContactReportAttribute : DCO_SquadCmdBoolAttribute
{
	override protected bool Get(DCO_GroupUtilityComponent g) { return g.CanReportContacts(); }
	override protected void Set(DCO_GroupUtilityComponent g, bool value) { g.SetCanReportContacts(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SquadPlayerSupportAttribute : DCO_SquadCmdBoolAttribute
{
	override protected bool Get(DCO_GroupUtilityComponent g) { return g.CanSupportPlayers(); }
	override protected void Set(DCO_GroupUtilityComponent g, bool value) { g.SetCanSupportPlayers(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SquadDedicatedTransportAttribute : DCO_SquadCmdBoolAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GroupUtilityComponent g = GetSquad(item);
		if (!g || !g.IsCommanderEligible())
			return null;
		DCO_TransportTeamComponent team = DCO_TransportTeamComponent.Cast(g.GetOwner().FindComponent(DCO_TransportTeamComponent));
		if (!team || (!g.IsDedicatedTransport() && !team.ResolveVehicle()))
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(g.IsDedicatedTransport());
	}

	override protected void Set(DCO_GroupUtilityComponent g, bool value) { g.SetDedicatedTransport(value); }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_TransportAllowedJobsAttribute : SCR_BaseMultiSelectPresetsEditorAttribute
{
	protected static DCO_TransportTeamComponent GetTeam(Managed item)
	{
		DCO_GroupUtilityComponent g = DCO_SquadCmdBoolAttribute.GetSquad(item);
		if (!g || !g.IsDedicatedTransport())
			return null;
		return DCO_TransportTeamComponent.Cast(g.GetOwner().FindComponent(DCO_TransportTeamComponent));
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_TransportTeamComponent team = GetTeam(item);
		if (!team)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(team.GetAllowedJobs());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var)
			return;
		DCO_TransportTeamComponent team = GetTeam(item);
		if (team)
			team.SetAllowedJobs(var.GetInt());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SquadLODAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GroupUtilityComponent g = DCO_SquadCmdBoolAttribute.GetSquad(item);
		if (!g)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(g.GetLODOverride());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GroupUtilityComponent g = DCO_SquadCmdBoolAttribute.GetSquad(item);
		if (g && var)
			g.SetLODOverride(var.GetInt());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SquadRetreatWhenAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GroupUtilityComponent g = DCO_SquadCmdBoolAttribute.GetSquad(item);
		if (!g)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(g.GetRetreatWhen());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GroupUtilityComponent g = DCO_SquadCmdBoolAttribute.GetSquad(item);
		if (g && var)
			g.SetRetreatWhen(var.GetInt());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SquadAmbushReactionAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GroupUtilityComponent g = DCO_SquadCmdBoolAttribute.GetSquad(item);
		if (!g)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(g.GetAmbushReaction());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GroupUtilityComponent g = DCO_SquadCmdBoolAttribute.GetSquad(item);
		if (g && var)
			g.SetAmbushReaction(var.GetInt());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SquadContactSOPAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GroupUtilityComponent g = DCO_SquadCmdBoolAttribute.GetSquad(item);
		if (!g)
			return null;
		return SCR_BaseEditorAttributeVar.CreateInt(g.GetSOPOverride());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GroupUtilityComponent g = DCO_SquadCmdBoolAttribute.GetSquad(item);
		if (g && var)
			g.SetSOPOverride(var.GetInt());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SquadCommanderEvalAttribute : DCO_SquadCmdBoolAttribute
{
	protected static SCR_AIGroupUtilityComponent Util(DCO_GroupUtilityComponent g)
	{
		if (!g || !g.GetOwner())
			return null;
		return SCR_AIGroupUtilityComponent.Cast(g.GetOwner().FindComponent(SCR_AIGroupUtilityComponent));
	}

	override protected bool Get(DCO_GroupUtilityComponent g)
	{
		SCR_AIGroupUtilityComponent u = Util(g);
		return u && u.DCO_IsCommanderEval();
	}

	override protected void Set(DCO_GroupUtilityComponent g, bool value)
	{
		SCR_AIGroupUtilityComponent u = Util(g);
		if (u && u.DCO_IsCommanderEval() != value)
			u.DCO_SetCommanderEval(value);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SquadFirstToFightAttribute : DCO_SquadCmdBoolAttribute
{
	override protected bool Get(DCO_GroupUtilityComponent g) { return g.IsFirstToFight(); }
	override protected void Set(DCO_GroupUtilityComponent g, bool value) { g.SetFirstToFight(value); }

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GroupUtilityComponent g = DCO_SquadCmdBoolAttribute.GetSquad(item);
		if (!g || !var || g.IsFirstToFight() == var.GetBool())
			return;
		g.SetFirstToFight(var.GetBool());
		DCO_GroupJobs.LogSet(g, playerID);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SquadAllowedTasksAttribute : SCR_BaseMultiSelectPresetsEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GroupUtilityComponent g = DCO_SquadCmdBoolAttribute.GetSquad(item);
		if (!g)
			return null;
		DCO_GroupJobs.NoteRead(g);
		return SCR_BaseEditorAttributeVar.CreateInt(g.GetAllowedJobs());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GroupUtilityComponent g = DCO_SquadCmdBoolAttribute.GetSquad(item);
		if (!g || !var)
			return;
		if (var.GetInt() == DCO_GroupJobs.ReadMask(g))
		{
			DCO_Perf.Count("grpjob:gm_tasks_unchanged");
			return;
		}
		g.SetAllowedJobs(var.GetInt());
		DCO_GroupJobs.LogSet(g, playerID);
	}

	override void UpdateInterlinkedVariables(SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, bool isInit = false)
	{
		if (isInit || !var || !manager)
			return;
		int mask = var.GetInt() & 511;
		int want = DCO_GroupJobs.PresetOf(mask);
		SCR_BaseEditorAttribute presetAttr;
		if (manager.GetActiveAttribute(DCO_SquadTaskPresetAttribute, presetAttr) && presetAttr)
		{
			if (want != 6)
				manager.SetAttributeDescription(presetAttr.GetUIInfo());
			else if (mask == 0)
			{
				manager.SetAttributeDescription(presetAttr.GetUIInfo(), "Custom: no tasks. This squad gets no commander jobs.");
				DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "grpjob_ui_custom mask=0 nearest=none changed=0");
			}
			else
			{
				int changed;
				int near = DCO_GroupJobs.NearestPreset(mask, changed);
				manager.SetAttributeDescription(presetAttr.GetUIInfo(), "Custom: %1 changed from %2.", changed.ToString(), DCO_GroupJobs.PresetLabel(near));
				DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("grpjob_ui_custom mask=%1 nearest=%2 changed=%3", mask, DCO_GroupJobs.PresetLabel(near), changed));
			}
		}
		SCR_BaseEditorAttributeVar pv;
		if (manager.GetAttributeVariable(DCO_SquadTaskPresetAttribute, pv) && pv && pv.GetInt() == want)
			return;
		manager.SetAttributeVariable(DCO_SquadTaskPresetAttribute, SCR_BaseEditorAttributeVar.CreateInt(want));
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("grpjob_ui_sync src=tasks mask=%1 preset=%2 ui=vanilla", mask, DCO_GroupJobs.PresetLabel(want)));
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_SquadTaskPresetAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GroupUtilityComponent g = DCO_SquadCmdBoolAttribute.GetSquad(item);
		if (!g)
			return null;
		DCO_GroupJobs.NoteRead(g);
		return SCR_BaseEditorAttributeVar.CreateInt(DCO_GroupJobs.PresetOf(g.GetAllowedJobs()));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GroupUtilityComponent g = DCO_SquadCmdBoolAttribute.GetSquad(item);
		if (!g || !var)
			return;
		int rm = DCO_GroupJobs.ReadMask(g);
		if (rm >= 0 && var.GetInt() == DCO_GroupJobs.PresetOf(rm))
		{
			DCO_Perf.Count("grpjob:gm_preset_unchanged");
			return;
		}
		int mask = DCO_GroupJobs.PresetMask(var.GetInt());
		if (mask < 0)
			return;
		g.SetAllowedJobs(mask);
		DCO_GroupJobs.LogSet(g, playerID);
	}

	override void UpdateInterlinkedVariables(SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, bool isInit = false)
	{
		if (isInit || !var || !manager)
			return;
		int p = var.GetInt();
		int mask = DCO_GroupJobs.PresetMask(p);
		if (mask < 0)
			return;
		SCR_BaseEditorAttribute selfAttr;
		if (manager.GetActiveAttribute(DCO_SquadTaskPresetAttribute, selfAttr) && selfAttr)
			manager.SetAttributeDescription(selfAttr.GetUIInfo());
		SCR_BaseEditorAttributeVar tv;
		if (manager.GetAttributeVariable(DCO_SquadAllowedTasksAttribute, tv) && tv && (tv.GetInt() & 511) == mask)
			return;
		manager.SetAttributeVariable(DCO_SquadAllowedTasksAttribute, SCR_BaseEditorAttributeVar.CreateInt(mask));
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", string.Format("grpjob_ui_sync src=preset mask=%1 preset=%2 ui=vanilla", mask, DCO_GroupJobs.PresetLabel(p)));
	}
}
