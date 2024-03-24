class DCO_AIInfoGroupComponentClass: ScriptComponentClass
{
};

enum DCO_EFormationType
{
	WEDGE,
	LINE,
	STAGGERED_COL,
	VEE,
	DIAMOND,
	AUTONOMOUS
};

enum DCO_ECombatBehaviorType
{
	DEFAULT,
	DEFENSIVE,
	OFFENSIVE
};

enum DCO_ECombatMovementType
{
	GROUP,
	TEAM,
	INDIVIDUAL,
	AUTONOMOUS
};


class DCO_AIInfoGroupComponent : ScriptComponent
{
	
	private int unitPrefabSlots;
	
	private SCR_AIGroup m_SCR_AIGroup;

	private DCO_ECombatBehaviorType m_eCombatBehaviorType = DCO_ECombatBehaviorType.DEFAULT;
	private DCO_ECombatMovementType m_eCombatMovementType = DCO_ECombatMovementType.AUTONOMOUS;
	private DCO_EFormationType m_eFormation;
	
	private AIFormationComponent m_AIFormationComponent;
	private SCR_AIConfigComponent m_SCR_AIConfigComponent;
	private SCR_AISettingsComponent m_SCR_AISettingsComponent;
	
	private int m_iWeaponFiredReactionDistance = SCR_AICombatComponent.LONG_RANGE_FIRE_DISTANCE;
	
	private float m_fThrowGrenadeTime;
	private bool m_bThrowGrenade = true;
	
	private bool m_bIsAnyFireteamNearby;
	
	static DCO_AIInfoGroupComponent m_sInstance;
	static DCO_AIInfoGroupComponent GetInstance()
	{
		return m_sInstance;
	}
	
	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		m_SCR_AIGroup = SCR_AIGroup.Cast(owner);
		
		if (m_SCR_AIGroup)
		{
			bool initialize = true;
			
			unitPrefabSlots = m_SCR_AIGroup.m_aUnitPrefabSlots.Count();	
			
			m_SCR_AISettingsComponent = SCR_AISettingsComponent.GetInstance();
			
			m_AIFormationComponent = AIFormationComponent.Cast(m_SCR_AIGroup.FindComponent(AIFormationComponent));
			
			m_SCR_AIConfigComponent = SCR_AIConfigComponent.Cast(m_SCR_AIGroup.FindComponent(SCR_AIConfigComponent));
			
			DCO_AIGroupInfoComponentInitialize(initialize);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	void DCO_AIGroupInfoComponentInitialize(bool initialize)
	{				
		if (m_eCombatMovementType == DCO_ECombatMovementType.AUTONOMOUS)
		{
			m_eCombatMovementType = DCO_ECombatMovementType.INDIVIDUAL;
		}
		
		if (m_eCombatBehaviorType == DCO_ECombatBehaviorType.DEFAULT)
		{
			m_eCombatBehaviorType = DCO_ECombatBehaviorType.OFFENSIVE;
		}
	}
	
	void SetFormaton(DCO_EFormationType formation)
	{
		m_eFormation = formation;
	}

	override protected void OnPostInit(IEntity owner)
	{
		m_sInstance = this;
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}
	
	DCO_ECombatBehaviorType GetCombatBehaviorType()
	{
		return m_eCombatBehaviorType;
	}
	
	void SetCombatBehaviorType(DCO_ECombatBehaviorType combatBehaviorType)
	{
		m_eCombatBehaviorType = combatBehaviorType;
	}
	
	DCO_ECombatMovementType GetCombatMovementType()
	{
		return m_eCombatMovementType;
	}
	
	void SetCombatMovementType(DCO_ECombatMovementType combatMovementType)
	{
		m_eCombatMovementType = combatMovementType;
	}
	
	int GetWeaponFiredReactionDistance()
	{
		return m_iWeaponFiredReactionDistance;
	}
	
	void SetWeaponFiredReactionDistance(int weaponFiredReactionDistance)
	{
		m_iWeaponFiredReactionDistance = weaponFiredReactionDistance;
	}
	
	bool GetIsAnyFireteamNearby()
	{
		return m_bIsAnyFireteamNearby;
	}
	
	void SetIsAnyFireteamNearby(bool isAnyFireteamNearby)
	{
		m_bIsAnyFireteamNearby = isAnyFireteamNearby;
	}

	bool GetThrowGrenade()
	{
		return m_bThrowGrenade;
	}
	
	void SetThrowGrenade(bool throwGrenade)
	{
		m_bThrowGrenade = throwGrenade;
	}
	
	float GetThrowGrenadeTime()
	{
		return m_fThrowGrenadeTime;
	}
	
	void SetThrowGrenadeTime(float throwGrenadeTime)
	{
		m_fThrowGrenadeTime = throwGrenadeTime;
	}
};