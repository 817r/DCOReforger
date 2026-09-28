[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIGlobalDodgeShotThresholdAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GlobalAIComponent dcoAiSetting = GetGlobal(item);
		if (!dcoAiSetting)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(dcoAiSetting.GetDodgeShotThreshold());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GlobalAIComponent dcoAiSetting = GetGlobal(item);
		if (!dcoAiSetting)
			return;

		dcoAiSetting.SetDodgeShotThreshold((int)Math.Round(var.GetFloat()));
	}

	protected DCO_GlobalAIComponent GetGlobal(Managed item)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return null;

		return DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIGlobalDodgeShotWindowAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_GlobalAIComponent dcoAiSetting = GetGlobal(item);
		if (!dcoAiSetting)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(dcoAiSetting.GetDodgeShotWindow());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_GlobalAIComponent dcoAiSetting = GetGlobal(item);
		if (!dcoAiSetting)
			return;

		dcoAiSetting.SetDodgeShotWindow(var.GetFloat());
	}

	protected DCO_GlobalAIComponent GetGlobal(Managed item)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return null;

		return DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIIndividualDodgeShotThresholdAttribute : SCR_ValidTypeBaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_AIConfigComponent aiConf = GetConfig(item);
		if (!aiConf)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(aiConf.GetDodgeShotThreshold());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_AIConfigComponent aiConf = GetConfig(item);
		if (!aiConf)
			return;

		aiConf.SetDodgeShotThreshold((int)Math.Round(var.GetFloat()));
	}

	protected DCO_AIConfigComponent GetConfig(Managed item)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return null;

		if (!IsValidEntityType(editableEntity.GetEntityType()))
			return null;

		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER))
			return null;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return null;

		ChimeraAIControlComponent aiComponents = ChimeraAIControlComponent.Cast(owner.FindComponent(ChimeraAIControlComponent));
		if (!aiComponents || !aiComponents.GetAIAgent())
			return null;

		return DCO_AIConfigComponent.Cast(aiComponents.GetAIAgent().FindComponent(DCO_AIConfigComponent));
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIIndividualDodgeShotWindowAttribute : SCR_ValidTypeBaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_AIConfigComponent aiConf = GetConfig(item);
		if (!aiConf)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(aiConf.GetDodgeShotWindow());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_AIConfigComponent aiConf = GetConfig(item);
		if (!aiConf)
			return;

		aiConf.SetDodgeShotWindow(var.GetFloat());
	}

	protected DCO_AIConfigComponent GetConfig(Managed item)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return null;

		if (!IsValidEntityType(editableEntity.GetEntityType()))
			return null;

		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER))
			return null;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return null;

		ChimeraAIControlComponent aiComponents = ChimeraAIControlComponent.Cast(owner.FindComponent(ChimeraAIControlComponent));
		if (!aiComponents || !aiComponents.GetAIAgent())
			return null;

		return DCO_AIConfigComponent.Cast(aiComponents.GetAIAgent().FindComponent(DCO_AIConfigComponent));
	}
}