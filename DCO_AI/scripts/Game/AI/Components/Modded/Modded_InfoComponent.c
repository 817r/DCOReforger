modded class SCR_AIInfoComponent : SCR_AIInfoBaseComponent
{
	IEntity myEnt;
	SCR_AIUtilityComponent m_Utility;

	override protected void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		myEnt = owner;
	}
	
	SCR_AIUtilityComponent RegisterUtility(SCR_AIUtilityComponent utility)
	{
		m_Utility = utility;
		return m_Utility;
	}
}
