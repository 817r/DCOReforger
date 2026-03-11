[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIGlobalSkillAttribute : SCR_BaseFloatValueHolderEditorAttribute
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
		
		DCO_AISKILL index = dcoAiSetting.GetAISkill();
		
		return SCR_BaseEditorAttributeVar.CreateInt(index);
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
		
		int index = var.GetInt();
		
		dcoAiSetting.SetAISkill(index);
	}
}