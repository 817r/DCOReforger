modded class SCR_AIInfoComponent : SCR_AIInfoBaseComponent
{
	protected DCO_AIMoraleSystem m_DCOMoraleSystem;
	SCR_AIUtilityComponent m_UtilityComponent;
	protected SCR_AIGroupPerception GroupPerception;
	
	void InitMoraleSystem(DCO_AIMoraleSystem moraleSystem)
	{
		m_DCOMoraleSystem = moraleSystem;	
	}
	
	void SetGroupPerception(SCR_AIGroupPerception perc)
	{
		GroupPerception = perc;
	}
	
	SCR_AIGroupPerception getGroupPerception()
	{
		return GroupPerception;
	}
	
	moraleState getMoraleState()
	{
		if(m_DCOMoraleSystem)
			return m_DCOMoraleSystem.GetState();
		else
			return moraleState.NORMAL;
	}
	
	DCO_AIMoraleSystem getMoraleSystem()
	{
		return m_DCOMoraleSystem;
	}
	
	override protected void EOnInit(IEntity owner)
	{
		IEntity ent = owner;
		AIAgent agent = AIAgent.Cast(owner);
		if (agent)
			ent = agent.GetControlledEntity();
		
		m_UtilityComponent = SCR_AIUtilityComponent.Cast(agent.FindComponent(SCR_AIUtilityComponent));	
		
		if (ent)
		{
			m_inventoryManagerComponent = SCR_InventoryStorageManagerComponent.Cast(ent.FindComponent(SCR_InventoryStorageManagerComponent));
			m_weaponManagerComponent = BaseWeaponManagerComponent.Cast(ent.FindComponent(BaseWeaponManagerComponent));
			m_CompartmentAccessComponent = SCR_CompartmentAccessComponent.Cast(ent.FindComponent(SCR_CompartmentAccessComponent));
			m_DamageManager = SCR_CharacterDamageManagerComponent.Cast(ent.FindComponent(SCR_CharacterDamageManagerComponent));
			m_CombatComponent = SCR_AICombatComponent.Cast(ent.FindComponent(SCR_AICombatComponent));
			
			m_CharacterController = SCR_CharacterControllerComponent.Cast(ent.FindComponent(SCR_CharacterControllerComponent));
			if (m_CharacterController)
				m_CharacterController.m_OnLifeStateChanged.Insert(OnLifeStateChanged);
			
			m_Perception = PerceptionComponent.Cast(ent.FindComponent(PerceptionComponent));
		}
		
		if (m_CompartmentAccessComponent)
		{
			m_CompartmentAccessComponent.GetOnCompartmentEntered().Insert(OnVehicleEntered);
			m_CompartmentAccessComponent.GetOnCompartmentLeft().Insert(OnVehicleLeft);
		}
		
		if (m_DamageManager)
		{
			InitBloodLevel();
			m_DamageManager.GetOnDamageEffectAdded().Insert(OnDamageEffectAdded);
			m_DamageManager.GetOnDamageEffectRemoved().Insert(OnDamageEffectRemoved);
			EvaluateWoundedState();
		}
	}
	
	CharacterControllerComponent getCharCont()
	{
		return m_CharacterController;
	}
	
	SCR_CharacterDamageManagerComponent getCharDamageComp()
	{
		return m_DamageManager;
	}
	
	SCR_AIUtilityComponent getUtilityComponent()
	{
		return m_UtilityComponent;
	}
	
	override bool HasRole(EUnitRole role)
	{
		switch (role)
		{
			case EUnitRole.MEDIC:				return m_inventoryManagerComponent.GetHealthComponentCount() > 4;
			case EUnitRole.MACHINEGUNNER:		return m_CombatComponent.HasWeaponOfType(EWeaponType.WT_MACHINEGUN);
			case EUnitRole.RIFLEMAN:			return m_CombatComponent.HasWeaponOfType(EWeaponType.WT_RIFLE);
			case EUnitRole.AT_SPECIALIST:		return m_CombatComponent.HasWeaponOfType(EWeaponType.WT_ROCKETLAUNCHER);
			case EUnitRole.GRENADIER:			return m_CombatComponent.HasWeaponOfType(EWeaponType.WT_GRENADELAUNCHER); // todo right now it will not detect a UGL muzzle, because weapon type is still rifle
			case EUnitRole.SNIPER:			return m_CombatComponent.HasWeaponOfType(EWeaponType.WT_SNIPERRIFLE);
			case EUnitRole.HAS_SMOKE_GRENADE:	return m_CombatComponent.HasWeaponOfType(EWeaponType.WT_SMOKEGRENADE);
			case EUnitRole.HAS_FRAG_GRENADE:	return m_CombatComponent.HasWeaponOfType(EWeaponType.WT_FRAGGRENADE);
		}
		
		return false;
	}

}