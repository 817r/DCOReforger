class DCO_GlobalAIComponentClass: ScriptComponentClass
{
}

class DCO_GlobalAIComponent: ScriptComponent
{
	[Attribute("0", UIWidgets.ComboBox, "Global AI unit skill level", "", ParamEnumArray.FromEnum(DCO_EAISkill))]
	protected DCO_EAISkill unitSkill;
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
	}
	
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}
}
