[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_GlobalAIMaxAIMBoost: SCR_ValidTypeBaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return null;
		
		SCR_DCOAISettingsComponent dcoAIGlobalSetting = SCR_DCOAISettingsComponent.Cast(gamemode.FindComponent(SCR_DCOAISettingsComponent));
		if (!dcoAIGlobalSetting)
			return null;
		
		float skill;
		dcoAIGlobalSetting.GetDefaultIndividualAIAimImprovementBoost();
		
		return SCR_BaseEditorAttributeVar.CreateFloat(skill);
	}
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		BaseGameMode gamemode = BaseGameMode.Cast(item);
		if (!gamemode)
			return;
		
		SCR_DCOAISettingsComponent dcoAIGlobalSetting = SCR_DCOAISettingsComponent.Cast(gamemode.FindComponent(SCR_DCOAISettingsComponent));
		if (!dcoAIGlobalSetting)
			return;
				
		float skill = var.GetFloat();
		dcoAIGlobalSetting.SetDefaultIndividualAIAimImprovementBoost(skill);
		skill = dcoAIGlobalSetting.GetDefaultIndividualAIAimImprovementBoost();
		
	}
};