//------------------------------------------------------------------------------------------------
//! === ADDED: GM Editor attribute buat slider Weapon Usage (Grenade + GL).
//! Nilai 0-1: 0 = gak pernah, 0.5 = default, 1 = sering banget (chance dikali 2).
//!
//! Pola persis ngikutin DCO_AIO.c:
//!   float global      -> SCR_BaseValueListEditorAttribute          (BaseGameMode -> DCO_GlobalAIComponent)
//!   float individual  -> SCR_ValidTypeBaseValueListEditorAttribute (AI agent -> DCO_AIConfigComponent)
//!
//! CATATAN: global di-snapshot ke tiap unit pas unit itu init (DCO_AIConfigComponent.EOnInit).
//! Ngubah slider GLOBAL cuma ngefek ke AI yang spawn SETELAHNYA -- sama kayak attribute
//! global lain di DCO. Unit yang udah ada diubah lewat slider INDIVIDUAL.
//------------------------------------------------------------------------------------------------


//======================================================================================
// GLOBAL -- nempel di BaseGameMode
//======================================================================================

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIGlobalGrenadeUsageAttribute : SCR_BaseValueListEditorAttribute
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
		
		return SCR_BaseEditorAttributeVar.CreateFloat(dcoAiSetting.GetGrenadeUsage());
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
		
		dcoAiSetting.SetGrenadeUsage(var.GetFloat());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIGlobalGLUsageAttribute : SCR_BaseValueListEditorAttribute
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
		
		return SCR_BaseEditorAttributeVar.CreateFloat(dcoAiSetting.GetGLUsage());
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
		
		dcoAiSetting.SetGLUsage(var.GetFloat());
	}
}


//======================================================================================
// INDIVIDUAL -- nempel di unit AI yang dipilih
//======================================================================================

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIIndividualGrenadeUsageAttribute : SCR_ValidTypeBaseValueListEditorAttribute
{
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_AIConfigComponent aiConf = GetConfig(item);
		if (!aiConf)
			return null;
		
		return SCR_BaseEditorAttributeVar.CreateFloat(aiConf.GetGrenadeUsage());
	}
	
	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_AIConfigComponent aiConf = GetConfig(item);
		if (!aiConf)
			return;
		
		aiConf.SetGrenadeUsage(var.GetFloat());
	}
	
	//------------------------------------------------------------------------------------------------
	//! Unit AI (bukan player) yang dipilih di GM -> DCO_AIConfigComponent di AI agent-nya.
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
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_AIConfigComponent aiConf = GetConfig(item);
		if (!aiConf)
			return null;
		
		return SCR_BaseEditorAttributeVar.CreateFloat(aiConf.GetGLUsage());
	}
	
	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		DCO_AIConfigComponent aiConf = GetConfig(item);
		if (!aiConf)
			return;
		
		aiConf.SetGLUsage(var.GetFloat());
	}
	
	//------------------------------------------------------------------------------------------------
	//! Unit AI (bukan player) yang dipilih di GM -> DCO_AIConfigComponent di AI agent-nya.
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