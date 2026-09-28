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
