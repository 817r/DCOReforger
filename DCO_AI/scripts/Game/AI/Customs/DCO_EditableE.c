class DCO_EditableEntityTypes
{
	//------------------------------------------------------------------------------------------------
	static bool CheckEditableEntity(SCR_EditableEntityComponent editableEntity)
	{
		if (editableEntity)
		{
			if (editableEntity.HasEntityState(EEditableEntityState.PLAYER))
				return false;
			
			if (editableEntity.GetEntityType() == EEditableEntityType.GROUP || editableEntity.GetEntityType() == EEditableEntityType.CHARACTER)
				return true;
		}
		
		return false;
	}
	
	//------------------------------------------------------------------------------------------------
	static SCR_AIGroup GetAIGroup(SCR_EditableEntityComponent editableEntity)
	{
		SCR_AIGroup group;
		
		if (editableEntity)
		{
			if (editableEntity.HasEntityState(EEditableEntityState.PLAYER))
				return null;
			
			if (editableEntity.GetEntityType() == EEditableEntityType.CHARACTER)
				return null;
			
			if (editableEntity.GetEntityType() == EEditableEntityType.GROUP)
			{
				IEntity owner = editableEntity.GetOwner();
				
				if (owner)
					group = SCR_AIGroup.Cast(owner);
			}
		}
		
		return group;
	}
	
	//------------------------------------------------------------------------------------------------
	static AIAgent GetAIAgent(SCR_EditableEntityComponent editableEntity)
	{
		AIAgent agent;
		
		if (editableEntity)
		{
			if (editableEntity.HasEntityState(EEditableEntityState.PLAYER))
				return null;
			
			if (editableEntity.GetEntityType() == EEditableEntityType.GROUP)
				return null;
			
			if (editableEntity.GetEntityType() == EEditableEntityType.CHARACTER)
			{
				IEntity owner = editableEntity.GetOwner();
				
				if (owner)
				{
					ChimeraCharacter character = ChimeraCharacter.Cast(owner);
					
					if (character)
					{
						AIControlComponent controlComponent = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
						
						if (controlComponent)
							agent = controlComponent.GetAIAgent();
					}
				}
			}
		}
		
		return agent;
	}
};