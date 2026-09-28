modded class SCR_AIInfoComponent
{
	protected SCR_AIUtilityComponent m_UtilityComponent;
	protected SCR_AIGroup m_MyGroup;

	override protected void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		AIAgent agent = AIAgent.Cast(owner);
		if (agent)
			m_UtilityComponent = SCR_AIUtilityComponent.Cast(agent.FindComponent(SCR_AIUtilityComponent));
	}

	SCR_AIUtilityComponent GetUtilityComp()
	{
		return m_UtilityComponent;
	}

	SCR_AIGroup SetMyGroup(SCR_AIGroup grp)
	{
		m_MyGroup = grp;
		return m_MyGroup;
	}

	SCR_AIGroup GetMyGroup()
	{
		return m_MyGroup;
	}

	SCR_InventoryStorageManagerComponent GetInventoryStorageManager()
	{
		return m_inventoryManagerComponent;
	}
}
