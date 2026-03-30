[ComponentEditorProps(category: "GameScripted/Group")]
class DCO_GroupUtilityComponentClass : ScriptComponentClass
{

}

class DCO_GroupUtilityComponent : ScriptComponent
{
	
	protected SCR_AIGroupUtilityComponent utilityComponent;
	protected SCR_AIGroup grp;
	protected DCOG_EGroupStatus m_eGroupStatus = DCOG_EGroupStatus.IDLE;
	
	
	void MoveTo(SCR_AIWaypoint wp)
	{
		grp.AddWaypoint(wp);
	}
	
	FactionKey GetFactionKey()
	{
		SCR_AIGroup grp = SCR_AIGroup.Cast(GetOwner());
		Faction fc = grp.GetFaction();
		return fc.GetFactionKey();
	}
	
	bool SetGroupStatus(DCOG_EGroupStatus st)
	{
		if (m_eGroupStatus != st)
			m_eGroupStatus = st;
		
		return true;
	}
	
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		AICommander_ManagerComponent.GetInstance().RegisterGroup(this);
		grp = SCR_AIGroup.Cast(owner);
		utilityComponent = SCR_AIGroupUtilityComponent.Cast(owner.FindComponent(SCR_AIGroupUtilityComponent));
	}
}

