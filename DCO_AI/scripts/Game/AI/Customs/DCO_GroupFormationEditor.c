[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_AIGroupFormationEditorAttribute : SCR_BaseFloatValueHolderEditorAttribute
{
	ref const array<string> m_Formations = {"Wedge", "Line", "Staggered Col", "Vee", "Diamond"};
	
	//------------------------------------------------------------------------------------------------
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		DCO_EFormationType formation;
		
		SCR_EditableEntityComponent editableEntityComponent = SCR_EditableEntityComponent.Cast(item);
		
		SCR_AIGroup aiGroup = DCO_EditableEntityTypes.GetAIGroup(editableEntityComponent);
		
		if (aiGroup == null)
			return null;
		
		AIFormationComponent aiFormationComponent = AIFormationComponent.Cast(aiGroup.FindComponent(AIFormationComponent));
		
		DCO_AIInfoGroupComponent aiGroupInfoComponent = DCO_AIInfoGroupComponent.Cast(aiGroup.FindComponent(DCO_AIInfoGroupComponent));
		
		if (aiFormationComponent)
			formation = m_Formations.Find(aiFormationComponent.GetFormation().GetName());
		
		return SCR_BaseEditorAttributeVar.CreateInt(formation);
	}
	
	//------------------------------------------------------------------------------------------------
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (var)
		{
			DCO_EFormationType formation = var.GetInt();
			
			SCR_EditableEntityComponent editableEntityComponent = SCR_EditableEntityComponent.Cast(item);
			
			SCR_AIGroup aiGroup = DCO_EditableEntityTypes.GetAIGroup(editableEntityComponent);
			
			if (aiGroup)
			{
				string formationName = m_Formations[formation];
				
				AIFormationComponent aiFormationComponent = AIFormationComponent.Cast(aiGroup.FindComponent(AIFormationComponent));
				
				DCO_AIInfoGroupComponent aiGroupInfoComponent = DCO_AIInfoGroupComponent.Cast(aiGroup.FindComponent(DCO_AIInfoGroupComponent));
				
				if (aiFormationComponent)
				{
					if (formationName == "Autonomous")
					{
						formationName = "VEE";
						
						formation = DCO_EFormationType.VEE;
					}
					
					aiFormationComponent.SetFormation(formationName);
					
					if (aiGroupInfoComponent)
						aiGroupInfoComponent.SetFormaton(formation);
				}
			}
		}
	}
};