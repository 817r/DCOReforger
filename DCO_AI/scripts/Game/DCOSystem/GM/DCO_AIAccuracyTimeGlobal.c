[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class SCR_AIGlobalAIMTimeAttribute : SCR_BaseValueListEditorAttribute
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
		
		float index = dcoAiSetting.GetAccuracyTime();
		
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
		
		dcoAiSetting.SetAccuracyTime(index);
	}
}