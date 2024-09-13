class SCR_AICommanderUtilityComponentClass : ScriptComponentClass
{	
}

class SCR_AICommanderUtilityComponent : ScriptComponent
{
	protected DCO_BaseAICommander commanderComp;
	protected DCO_AICommanderPersonality personalityComp;
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		commanderComp = DCO_BaseAICommander.Cast(owner.FindComponent(DCO_BaseAICommander));
		personalityComp = DCO_AICommanderPersonality.Cast(owner.FindComponent(DCO_AICommanderPersonality));
	}
}
