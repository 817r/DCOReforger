modded class SCR_AIInfoComponent : SCR_AIInfoBaseComponent
{
	IEntity myEnt;

	override protected void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		myEnt = owner;
	}
}
