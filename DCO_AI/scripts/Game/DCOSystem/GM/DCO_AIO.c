//------------------------------------------------------------------------------------------------
//! GM Editor attribute buat knob yang selama ini cuma bisa disetel lewat prefab
//! attribute atau JSON server config: 5 parameter dodge + takeCoverChance,
//! masing-masing versi Global dan Individual.
//!
//! Setter-nya (SetDodgeChance/SetDodgeCooldown/SetDodgeMaxDist/SetDodgeSearchDist/
//! SetDodgeScaleByPersonality/SetTakeCoverChance) udah lama ada di DCO_GlobalAIComponent
//! dan DCO_AIConfigComponent tapi NOL pemanggil di seluruh repo -- file ini yang jadi
//! konsumennya.
//!
//! Pola persis ngikutin DCO_PerceptionGlobal/Individual (float) dan
//! MagicAmmoGlobal/Individual (bool), termasuk base class-nya:
//!   float global      -> SCR_BaseValueListEditorAttribute
//!   float individual  -> SCR_ValidTypeBaseValueListEditorAttribute
//!   bool  global      -> SCR_BaseEditorAttribute
//!   bool  individual  -> SCR_ValidTypeBaseEditorAttribute
//!
//! CATATAN: personalityWeightStandard/Cautious/Aggressive/Reckless SENGAJA gak masuk
//! sini. Empat field itu belum punya setter sama sekali di DCO_GlobalAIComponent --
//! cuma dibaca dari JSON di ReadFromJson(). Setternya harus ditambah dulu.
//------------------------------------------------------------------------------------------------


//======================================================================================
// GLOBAL -- nempel di BaseGameMode, kena ke semua AI
//======================================================================================

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIGlobalDodgeChanceAttribute : SCR_BaseValueListEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return null;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return null;

		float index = dcoAiSetting.GetDodgeChance();

		return SCR_BaseEditorAttributeVar.CreateFloat(index);
	}

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return;

		float index = var.GetFloat();

		dcoAiSetting.SetDodgeChance(index);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIGlobalDodgeCooldownAttribute : SCR_BaseValueListEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return null;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return null;

		float index = dcoAiSetting.GetDodgeCooldown();

		return SCR_BaseEditorAttributeVar.CreateFloat(index);
	}

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return;

		float index = var.GetFloat();

		dcoAiSetting.SetDodgeCooldown(index);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIGlobalDodgeMaxDistAttribute : SCR_BaseValueListEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return null;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return null;

		float index = dcoAiSetting.GetDodgeMaxDist();

		return SCR_BaseEditorAttributeVar.CreateFloat(index);
	}

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return;

		float index = var.GetFloat();

		dcoAiSetting.SetDodgeMaxDist(index);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIGlobalDodgeSearchDistAttribute : SCR_BaseValueListEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return null;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return null;

		float index = dcoAiSetting.GetDodgeSearchDist();

		return SCR_BaseEditorAttributeVar.CreateFloat(index);
	}

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return;

		float index = var.GetFloat();

		dcoAiSetting.SetDodgeSearchDist(index);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIGlobalTakeCoverChanceAttribute : SCR_BaseValueListEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return null;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return null;

		float index = dcoAiSetting.GetTakeCoverChance();

		return SCR_BaseEditorAttributeVar.CreateFloat(index);
	}

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return;

		float index = var.GetFloat();

		dcoAiSetting.SetTakeCoverChance(index);
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIGlobalDodgeScaleByPersonalityAttribute : SCR_BaseEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return null;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return null;

		bool index = dcoAiSetting.GetDodgeScaleByPersonality();

		return SCR_BaseEditorAttributeVar.CreateBool(index);
	}

	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return;

		DCO_GlobalAIComponent dcoAiSetting = DCO_GlobalAIComponent.Cast(gamemode.FindComponent(DCO_GlobalAIComponent));
		if (!dcoAiSetting)
			return;

		bool index = var.GetBool();

		dcoAiSetting.SetDodgeScaleByPersonality(index);
	}
}


//======================================================================================
// INDIVIDUAL -- nempel di entity yang dipilih GM, cuma kena ke unit itu
//======================================================================================

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIIndividualDodgeChanceAttribute : SCR_ValidTypeBaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
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
		if (!aiComponents)
			return null;

		DCO_AIConfigComponent aiConf = DCO_AIConfigComponent.Cast(aiComponents.GetAIAgent().FindComponent(DCO_AIConfigComponent));

		if (!aiConf)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(aiConf.GetDodgeChance());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return;

		if (!IsValidEntityType(editableEntity.GetEntityType()))
			return;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return;

		ChimeraAIControlComponent aiComponents = ChimeraAIControlComponent.Cast(owner.FindComponent(ChimeraAIControlComponent));
		if (!aiComponents)
			return;

		DCO_AIConfigComponent aiConf = DCO_AIConfigComponent.Cast(aiComponents.GetAIAgent().FindComponent(DCO_AIConfigComponent));

		if (!aiConf)
			return;

		aiConf.SetDodgeChance(var.GetFloat());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIIndividualDodgeCooldownAttribute : SCR_ValidTypeBaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
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
		if (!aiComponents)
			return null;

		DCO_AIConfigComponent aiConf = DCO_AIConfigComponent.Cast(aiComponents.GetAIAgent().FindComponent(DCO_AIConfigComponent));

		if (!aiConf)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(aiConf.GetDodgeCooldown());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return;

		if (!IsValidEntityType(editableEntity.GetEntityType()))
			return;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return;

		ChimeraAIControlComponent aiComponents = ChimeraAIControlComponent.Cast(owner.FindComponent(ChimeraAIControlComponent));
		if (!aiComponents)
			return;

		DCO_AIConfigComponent aiConf = DCO_AIConfigComponent.Cast(aiComponents.GetAIAgent().FindComponent(DCO_AIConfigComponent));

		if (!aiConf)
			return;

		aiConf.SetDodgeCooldown(var.GetFloat());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIIndividualDodgeMaxDistAttribute : SCR_ValidTypeBaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
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
		if (!aiComponents)
			return null;

		DCO_AIConfigComponent aiConf = DCO_AIConfigComponent.Cast(aiComponents.GetAIAgent().FindComponent(DCO_AIConfigComponent));

		if (!aiConf)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(aiConf.GetDodgeMaxDist());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return;

		if (!IsValidEntityType(editableEntity.GetEntityType()))
			return;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return;

		ChimeraAIControlComponent aiComponents = ChimeraAIControlComponent.Cast(owner.FindComponent(ChimeraAIControlComponent));
		if (!aiComponents)
			return;

		DCO_AIConfigComponent aiConf = DCO_AIConfigComponent.Cast(aiComponents.GetAIAgent().FindComponent(DCO_AIConfigComponent));

		if (!aiConf)
			return;

		aiConf.SetDodgeMaxDist(var.GetFloat());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIIndividualDodgeSearchDistAttribute : SCR_ValidTypeBaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
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
		if (!aiComponents)
			return null;

		DCO_AIConfigComponent aiConf = DCO_AIConfigComponent.Cast(aiComponents.GetAIAgent().FindComponent(DCO_AIConfigComponent));

		if (!aiConf)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(aiConf.GetDodgeSearchDist());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return;

		if (!IsValidEntityType(editableEntity.GetEntityType()))
			return;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return;

		ChimeraAIControlComponent aiComponents = ChimeraAIControlComponent.Cast(owner.FindComponent(ChimeraAIControlComponent));
		if (!aiComponents)
			return;

		DCO_AIConfigComponent aiConf = DCO_AIConfigComponent.Cast(aiComponents.GetAIAgent().FindComponent(DCO_AIConfigComponent));

		if (!aiConf)
			return;

		aiConf.SetDodgeSearchDist(var.GetFloat());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIIndividualTakeCoverChanceAttribute : SCR_ValidTypeBaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
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
		if (!aiComponents)
			return null;

		DCO_AIConfigComponent aiConf = DCO_AIConfigComponent.Cast(aiComponents.GetAIAgent().FindComponent(DCO_AIConfigComponent));

		if (!aiConf)
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(aiConf.GetTakeCoverChance());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return;

		if (!IsValidEntityType(editableEntity.GetEntityType()))
			return;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return;

		ChimeraAIControlComponent aiComponents = ChimeraAIControlComponent.Cast(owner.FindComponent(ChimeraAIControlComponent));
		if (!aiComponents)
			return;

		DCO_AIConfigComponent aiConf = DCO_AIConfigComponent.Cast(aiComponents.GetAIAgent().FindComponent(DCO_AIConfigComponent));

		if (!aiConf)
			return;

		aiConf.SetTakeCoverChance(var.GetFloat());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIIndividualDodgeScaleByPersonalityAttribute : SCR_ValidTypeBaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
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
		if (!aiComponents)
			return null;

		DCO_AIConfigComponent aiConf = DCO_AIConfigComponent.Cast(aiComponents.GetAIAgent().FindComponent(DCO_AIConfigComponent));

		if (!aiConf)
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(aiConf.GetDodgeScaleByPersonality());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity)
			return;

		if (!IsValidEntityType(editableEntity.GetEntityType()))
			return;

		IEntity owner = editableEntity.GetOwner();
		if (!owner)
			return;

		ChimeraAIControlComponent aiComponents = ChimeraAIControlComponent.Cast(owner.FindComponent(ChimeraAIControlComponent));
		if (!aiComponents)
			return;

		DCO_AIConfigComponent aiConf = DCO_AIConfigComponent.Cast(aiComponents.GetAIAgent().FindComponent(DCO_AIConfigComponent));

		if (!aiConf)
			return;

		aiConf.SetDodgeScaleByPersonality(var.GetBool());
	}
}