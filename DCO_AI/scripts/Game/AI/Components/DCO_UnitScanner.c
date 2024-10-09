enum DCO_IndividualRoles
{
	UNARMED,
	MEDIC,
	RIFLEMAN,
	ANTI_TANK,
	MACHINEGUNNER,
	AUTOMATIC_RIFLEMAN,
	GRENADIER,
	MARKSMAN,
	SNIPER,
	AMMO_BEARER,
	RADIO_OPERATOR,
	TEAM_LEADER,
	SQUAD_LEADER
};


class DCO_UnitScanComponentClass : ScriptComponentClass
{
}

class DCO_UnitScanComponent : ScriptComponent
{
	bool MEDIC, RIFLEMAN, ANTI_TANK, MACHINEGUNNER, AUTOMATIC_RIFLEMAN, GRENADIER, MARKSMAN, SNIPER, AMMO_BEARER, RADIO_OPERATOR, TEAM_LEADER, SQUAD_LEADER;
	
	protected IEntity Owners;
	protected SCR_ChimeraAIAgent agent;
	
	protected CharacterWeaponManagerComponent CharWeaponManager;
	protected SCR_CharacterPerceivableComponent CharacterPerceivableComp;
	protected SCR_AIInfoComponent InfoComponent;
	protected SCR_AICombatComponent CombatComponent;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
		SetEventMask(owner, EntityEvent.FRAME);
	}
	
	override void EOnInit(IEntity owner)
	{
		Owners = owner;
		CharWeaponManager = CharacterWeaponManagerComponent.Cast(owner.FindComponent(CharacterWeaponManagerComponent));
		CharacterPerceivableComp = SCR_CharacterPerceivableComponent.Cast(owner.FindComponent(SCR_CharacterPerceivableComponent));
		CombatComponent = SCR_AICombatComponent.Cast(owner.FindComponent(SCR_AICombatComponent));
		AIControlComponent ctrl = AIControlComponent.Cast(owner.FindComponent(AIControlComponent));
		
		if (ctrl)
		{
			agent = SCR_ChimeraAIAgent.Cast(ctrl.GetAIAgent());
			if (agent)
			{
				InfoComponent = agent.m_InfoComponent;
			}
		}
	}
	
	int GetRoleCount()
	{
		int roles = 0;
		
		if (MEDIC) roles++;
		if (RIFLEMAN) roles++;
		if (AUTOMATIC_RIFLEMAN) roles++;
		if (MACHINEGUNNER) roles++;
		if (ANTI_TANK) roles++;
		if (GRENADIER) roles++;
		if (MARKSMAN) roles++;
		if (SNIPER) roles++;
		if (AMMO_BEARER) roles++;
		if (RADIO_OPERATOR) roles++;
		
		return roles;
	}
	
	protected void CalculateRoles()
	{			
		// !Handle Role By Weapon
		if (!InfoComponent) return;
		
		if(CombatComponent.HasWeaponOfType(EWeaponType.WT_MACHINEGUN))
		{
			 MACHINEGUNNER = true;
		} 
		if (CombatComponent.HasWeaponOfType(EWeaponType.WT_SNIPERRIFLE))
		{
			 MARKSMAN = true;
		} 
		if (CombatComponent.HasWeaponOfType(EWeaponType.WT_ROCKETLAUNCHER))
		{
			 ANTI_TANK = true;
		} 
		if (CombatComponent.HasWeaponOfType(EWeaponType.WT_GRENADELAUNCHER))
		{
			GRENADIER = true;
		} 
		if (CombatComponent.HasWeaponOfType(EWeaponType.WT_RIFLE))
		{
			RIFLEMAN = true;
		} 
		if (InfoComponent.HasRole(EUnitRole.MEDIC))
		{
			MEDIC = true;
		}
	}
	
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		CalculateRoles();
	}
}