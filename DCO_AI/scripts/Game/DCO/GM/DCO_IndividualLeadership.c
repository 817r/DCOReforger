[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIIndividualLeadershipAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return null;

		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER))
			return null;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return null;

		SCR_AICombatComponent aiComponents = SCR_AICombatComponent.Cast(owner.FindComponent(SCR_AICombatComponent));
		if (!aiComponents)
			return null;

		SCR_AIUtilityComponent util = aiComponents.GetUtilityComponent();
		DCO_ELeadership l = DCO_AIConfigComponent.LeadershipOf(util);

		return SCR_BaseEditorAttributeVar.CreateInt(l);
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return;

		SCR_AICombatComponent aiComponents = SCR_AICombatComponent.Cast(owner.FindComponent(SCR_AICombatComponent));
		if (!aiComponents)
			return;

		SCR_AIUtilityComponent util = aiComponents.GetUtilityComponent();
		DCO_AIConfigComponent conf = DCO_AIConfigComponent.Of(util);
		if (!conf)
			return;

		int old = conf.GetLeadership();
		conf.SetLeadership(var.GetInt());
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, DCO_Log.IdUnit(owner), string.Format("gm_leadership unit=%1 from=%2 to=%3 by=%4", DCO_Log.IdUnit(owner), DCO_Leadership.Name(old), DCO_Leadership.Name(var.GetInt()), playerID));
	}
}
