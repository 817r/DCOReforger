[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIGlobalGrenadeUsageAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return null;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(dcoAiSetting.GetGrenadeUsage());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return;

		dcoAiSetting.SetGrenadeUsage(var.GetFloat());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIGlobalGLUsageAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return null;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(dcoAiSetting.GetGLUsage());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return;

		dcoAiSetting.SetGLUsage(var.GetFloat());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIIndividualGrenadeUsageAttribute : SCR_ValidTypeBaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_AIConfigComponent aiConf = GetConfig(item);
		if (!aiConf)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(aiConf.GetGrenadeUsage());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_AIConfigComponent aiConf = GetConfig(item);
		if (!aiConf)
			return;

		aiConf.SetGrenadeUsage(var.GetFloat());
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
class SCR_AIIndividualGLUsageAttribute : SCR_ValidTypeBaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_AIConfigComponent aiConf = GetConfig(item);
		if (!aiConf)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(aiConf.GetGLUsage());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_AIConfigComponent aiConf = GetConfig(item);
		if (!aiConf)
			return;

		aiConf.SetGLUsage(var.GetFloat());
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
class SCR_AIGlobalGLAccuracyAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return null;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(dcoAiSetting.GetGLAccuracy());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return;

		dcoAiSetting.SetGLAccuracy(var.GetFloat());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIIndividualGLAccuracyAttribute : SCR_AIIndividualGLUsageAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_AIConfigComponent aiConf = GetConfig(item);
		if (!aiConf)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(aiConf.GetGLAccuracy());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_AIConfigComponent aiConf = GetConfig(item);
		if (!aiConf)
			return;

		aiConf.SetGLAccuracy(var.GetFloat());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIGlobalSmokeUsageAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return null;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(dcoAiSetting.GetSmokeUsage());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return;

		dcoAiSetting.SetSmokeUsage(var.GetFloat());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIIndividualSmokeUsageAttribute : SCR_AIIndividualGLUsageAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_AIConfigComponent aiConf = GetConfig(item);
		if (!aiConf)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(aiConf.GetSmokeUsage());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_AIConfigComponent aiConf = GetConfig(item);
		if (!aiConf)
			return;

		aiConf.SetSmokeUsage(var.GetFloat());
	}
}
