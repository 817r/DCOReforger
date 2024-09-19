modded enum EAIDebugCategory
{
	COMMANDER
};

class DCO_BaseAICommanderClass : ScriptComponentClass
{
}

class DCO_BaseAICommander : ScriptComponent
{
	[Attribute("1", UIWidgets.Slider, "Managed Area Operation at the same time", params: "1 10 1", category: "Commander Setting")]
	protected int m_AOAttheSameTime;
	
	[Attribute("1", UIWidgets.Slider, "Managed Area Operation at the same time", params: "1 5 1", category: "Commander Setting")]
	protected int m_CommanderUniqueNumber;
	
	[Attribute("0.18 0.80 0.44 1", UIWidgets.ColorPicker, desc: "Color Code of Commander", category: "Commander Setting")]
	private ref Color m_CommanderColor;
	
	[Attribute(UIWidgets.ResourceAssignArray, "Managed Group at the same time", params: "et", category: "Commander Group")]
	protected ref array<ResourceName> controlledGroup;
	
	protected ref array<DCO_ObjectiveArea> objectiveArea = {};
	protected DCO_AICommanderPersonality personalityComponent;
	
	protected SCR_AIUtilityComponent m_Utility;
	
	protected Faction m_Factions;
	
	void giveAreaToCommander(array<DCO_ObjectiveArea> area)
	{	
		objectiveArea = area;
	}
	
	int getObjectiveAreaCount()
	{
		return objectiveArea.Count();
	}
	
	void Update(SCR_AIUtilityComponent utility)
	{
		SCR_AIDebugVisualization.VisualizeMessage(m_Utility.m_OwnerEntity,"Objective Area : " + objectiveArea.Count().ToString(), EAIDebugCategory.ORDER, 1.4, m_CommanderColor);
	}
	
	void initialize(IEntity owner)
	{
		m_Utility = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
		// Register self in manager
		DCO_War_GameMode parentManager = DCO_War_GameMode.GetAreaManager();
		if (!parentManager)
		{
			Print("AI Commander cannot find DCO_War_GameMode! Functionality might be limited!", LogLevel.WARNING);
			return;
		}
		parentManager.RegisterCommander(this);
		parentManager.getAllObjectiveArea(objectiveArea);
		personalityComponent = DCO_AICommanderPersonality.Cast(owner.FindComponent(DCO_AICommanderPersonality));
	}
	
	protected void ~DCO_BaseAICommander()
	{
		DCO_War_GameMode parentManager = DCO_War_GameMode.GetAreaManager();
		if (!parentManager)
			return;

		parentManager.unregisterCommander(this);
	}
}