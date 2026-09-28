class DCO_BuildingPositionComponentClass: ScriptComponentClass
{
}

class DCO_BuildingPositionComponent: ScriptComponent
{
	protected IEntity m_Building;

	override void OnPostInit(IEntity owner)
	{
		m_Building = owner;
	}

	IEntity GetBuildingEntity()
	{
		return m_Building;
	}
}
