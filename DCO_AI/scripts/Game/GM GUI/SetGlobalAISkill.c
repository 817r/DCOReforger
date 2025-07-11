[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalAISkillLevel: SCR_BasePresetsEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		SCR_DCOAISettingsComponent Inst = SCR_DCOAISettingsComponent.GetInstance();
		if (!Inst) return null;
		
		int skill;
		Inst.GetDefaultIndividualAIGlobalSkill();
		
		return SCR_BaseEditorAttributeVar.CreateInt(skill);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var) return;
		SCR_DCOAISettingsComponent Inst = SCR_DCOAISettingsComponent.GetInstance();
		if (!Inst) return;
				
		int skill = var.GetInt();
		Inst.SetDefaultIndividualAIGlobalSkill(skill);
		skill = Inst.GetDefaultIndividualAIGlobalSkill();
		
	}
};