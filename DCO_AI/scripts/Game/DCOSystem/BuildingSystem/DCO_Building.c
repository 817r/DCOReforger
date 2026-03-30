class DCO_BuildingPositionComponentClass: ScriptComponentClass
{
}


// CREDITS TO GME TEAM TO ALLOW ME ABLE TO MODIFY THIS SCRIPT FROM GARRISON

// DO NOT USE THIS SCRIPT AS IT OFTEN BREAKS THE GAME ENTIRELY

// NEED HELP OPTIMIZING THIS



class DCO_BuildingPositionComponent: ScriptComponent
{	
	protected IEntity m_Building;

	override void OnPostInit(IEntity owner)
	{
		//super.OnPostInit(owner);
		//m_DestroyableBuildingEntity = SCR_DestructibleBuildingEntity.Cast(owner.FindComponent(SCR_DestructibleBuildingEntity));
		//SetEventMask(owner, EntityEvent.INIT);
		m_Building = owner;
	}
	
	IEntity GetBuildingEntity()
	{
		return m_Building;
	}
}
