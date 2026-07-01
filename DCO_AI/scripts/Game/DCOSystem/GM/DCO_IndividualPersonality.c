// GM/Editor attribute buat DCO_EAIPersonality per-INDIVIDUAL unit.
// Pola disamain PERSIS sama SCR_AIIndividualSkillAttribute (DCO_SetAISkillIndividual.c)
// yang udah ada -- akses lewat SCR_AICombatComponent -> GetUtilityComponent() -> m_DCOConfig,
// bukan lewat ChimeraAIControlComponent kayak versi AIM/Perception.
//
// CATATAN: entry UI-nya (Name, Category, Layout dropdown) masih perlu didaftarin manual
// di Edit.conf lewat Workbench -- ini cuma logic Read/WriteVariable-nya.
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIIndividualPersonalityAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity) 
			return null;
		
		if (editableEntity.HasEntityState(EEditableEntityState.PLAYER))
			return null;
		
		//if (!IsValidEntityType(editableEntity.GetEntityType()))	
		//	return null;
		
		IEntity owner = editableEntity.GetOwner();
		if (!owner) 
			return null;
		
		SCR_AICombatComponent aiComponents = SCR_AICombatComponent.Cast(owner.FindComponent(SCR_AICombatComponent));
		if (!aiComponents) 
			return null;
		
		SCR_AIUtilityComponent util = aiComponents.GetUtilityComponent();
		DCO_AIConfigComponent conf = util.m_DCOConfig;
		DCO_EAIPersonality p = conf.GetPersonality();
		
		return SCR_BaseEditorAttributeVar.CreateInt(p);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		SCR_EditableEntityComponent editableEntity = SCR_EditableEntityComponent.Cast(item);
		if (!editableEntity) 
			return;
		
		//if (!IsValidEntityType(editableEntity.GetEntityType()))
		//	return;
		
		IEntity owner = editableEntity.GetOwner();
		if (!owner) 
			return;
		
		SCR_AICombatComponent aiComponents = SCR_AICombatComponent.Cast(owner.FindComponent(SCR_AICombatComponent));
		if (!aiComponents) 
			return;
		
		SCR_AIUtilityComponent util = aiComponents.GetUtilityComponent();
		DCO_AIConfigComponent conf = util.m_DCOConfig;

		conf.SetPersonality(var.GetInt());
	}
}