modded class SCR_AIInfoComponent
{
	protected SCR_AIUtilityComponent m_UtilityComponent;
	
	//------------------------------------------------------------------------------------------------
	override protected void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
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
	
	SCR_AIUtilityComponent GetUtilityComp()
	{
		return m_UtilityComponent;
	}
}
