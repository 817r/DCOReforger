class DCO_AIGroupInfoComponentClass: ScriptComponentClass
{
};

enum DCO_EFormation
{
	VEE,
	LINE,
	WEDGE,
	COLUMN,
	DIAMOND,
	FIRETEAM,
	ECHELONLEFT,
	ECHELONRIGHT,
	STAGGEREDCOLUMN,
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
	FIRETEAM,
	INDIVIDUAL,
	AUTONOMOUS
};


class DCO_AIInfoGroupComponent : ScriptComponent
{
	
	private int unitPrefabSlots;
	
	private SCR_AIGroup m_SCR_AIGroup;
	
	private int m_iCombatMoveChance;
	private int m_iCombatCoverChance;
	private int m_iCombatDefendChance;
	
	private bool m_bInvestigate;
	private int m_iInvestigateRadius;
	private int m_iInvestigateDuration;
	
	private int m_iRandomPatrolWaypoints;
	
	private DCO_EFormation m_eFormation;
	private DCO_ECombatBehaviorType m_eCombatBehaviorType;
	private DCO_ECombatMovementType m_eCombatMovementType;
	
	private AIFormationComponent m_AIFormationComponent;
	private SCR_AIConfigComponent m_SCR_AIConfigComponent;
	private SCR_AISettingsComponent m_SCR_AISettingsComponent;
	
	private int m_iWeaponFiredReactionDistance = SCR_AIReactionBase.AI_WEAPONFIRED_REACTION_DISTANCE;
	
	//------------------------------------------------------------------------------------------------
	//! Functionality
	private float m_fThrowGrenadeTime;
	private bool m_bThrowGrenade = true;
	
	private bool m_bIsAnyFireteamNearby;
	
	static DCO_AIGroupInfoComponent m_sInstance;
	static DCO_AIGroupInfoComponent GetInstance()
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
		if (m_SCR_AISettingsComponent)
		{
			m_eFormation = m_SCR_AISettingsComponent.m_Formation;
			
			m_bInvestigate = m_SCR_AISettingsComponent.m_Investigate;
			m_iInvestigateRadius = m_SCR_AISettingsComponent.m_InvestigateRadius;
			m_iInvestigateDuration = m_SCR_AISettingsComponent.m_InvestigateDuration;
			
			m_iCombatMoveChance = m_SCR_AISettingsComponent.m_CombatMoveChance;
			m_iCombatCoverChance = m_SCR_AISettingsComponent.m_CombatCoverChance;
			m_iCombatDefendChance = m_SCR_AISettingsComponent.m_CombatDefendChance;
			
			m_eCombatBehaviorType = m_SCR_AISettingsComponent.m_CombatBehaviorType;
			m_eCombatMovementType = m_SCR_AISettingsComponent.m_CombatMovementType;
			
			m_iWeaponFiredReactionDistance = m_SCR_AISettingsComponent.m_WeaponFiredReactionDistance;
			
			if (initialize && m_SCR_AIConfigComponent)
			{
				if (m_SCR_AIConfigComponent.m_Formation != DCO_EFormation.AUTONOMOUS)
					m_eFormation = m_SCR_AIConfigComponent.m_Formation;
				
				if (m_SCR_AIConfigComponent.m_Investigate != true)
					m_bInvestigate = m_SCR_AIConfigComponent.m_Investigate;
				
				if (m_SCR_AIConfigComponent.m_InvestigateRadius != -1)
					m_iInvestigateRadius = m_SCR_AIConfigComponent.m_InvestigateRadius;
				
				if (m_SCR_AIConfigComponent.m_InvestigateDuration != 120)
					m_iInvestigateDuration = m_SCR_AIConfigComponent.m_InvestigateDuration;
				
				if (m_SCR_AIConfigComponent.m_CombatMoveChance != 50)
					m_iCombatMoveChance = m_SCR_AIConfigComponent.m_CombatMoveChance;
				
				if (m_SCR_AIConfigComponent.m_CombatCoverChance != 50)
					m_iCombatCoverChance = m_SCR_AIConfigComponent.m_CombatCoverChance;
				
				if (m_SCR_AIConfigComponent.m_CombatDefendChance != -1)
					m_iCombatDefendChance = m_SCR_AIConfigComponent.m_CombatDefendChance;
				
				if (m_SCR_AIConfigComponent.m_CombatBehaviorType != DCO_ECombatBehaviorType.DEFAULT)
					m_eCombatBehaviorType = m_SCR_AIConfigComponent.m_CombatBehaviorType;
				
				if (m_SCR_AIConfigComponent.m_CombatMovementType != DCO_ECombatMovementType.AUTONOMOUS)
					m_eCombatMovementType = m_SCR_AIConfigComponent.m_CombatMovementType;
				
				if (m_SCR_AIConfigComponent.m_WeaponFiredReactionDistance != 500)
					m_iWeaponFiredReactionDistance = m_SCR_AIConfigComponent.m_WeaponFiredReactionDistance;
			}
		}
		
		if (m_eFormation == DCO_EFormation.AUTONOMOUS)
			m_eFormation = DCO_EFormation.WEDGE;
		else
		{
			if (m_AIFormationComponent)
			{
				string formation;
				
				switch (m_eFormation)
				{
					case DCO_EFormation.VEE:             formation = "Vee";             break;
					case DCO_EFormation.LINE:            formation = "Line";            break;
					case DCO_EFormation.WEDGE:           formation = "Wedge";           break;
					case DCO_EFormation.COLUMN:          formation = "Column";          break;
					case DCO_EFormation.DIAMOND:         formation = "Diamond";         break;
					case DCO_EFormation.FIRETEAM:        formation = "Fireteam";        break;
					case DCO_EFormation.ECHELONLEFT:     formation = "EchelonLeft";     break;
					case DCO_EFormation.ECHELONRIGHT:    formation = "EchelonRight";    break;
					case DCO_EFormation.STAGGEREDCOLUMN: formation = "StaggeredColumn"; break;
				}
				
				m_AIFormationComponent.SetFormation(formation);
			}
		}
		
		if (m_eCombatMovementType == DCO_ECombatMovementType.AUTONOMOUS)
		{
			m_eCombatMovementType = DCO_ECombatMovementType.INDIVIDUAL;
			
			if (unitPrefabSlots > 5 && Math.RandomFloat(0,100) < 50)
				m_eCombatMovementType = DCO_ECombatMovementType.FIRETEAM;
			else
				m_eCombatMovementType = DCO_ECombatMovementType.INDIVIDUAL;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{
		m_sInstance = this;
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	DCO_EFormation GetFormation()
	{
		return m_eFormation;
	}
	
	void SetFormation(DCO_EFormation formation)
	{
		m_eFormation = formation;
	}
	
	//------------------------------------------------------------------------------------------------
	//! Investigate
	bool GetInvestigate()
	{
		return m_bInvestigate;
	}
	
	void SetInvestigate(bool investigate)
	{
		m_bInvestigate = investigate;
	}
	
	int GetInvestigateRadius()
	{
		return m_iInvestigateRadius;
	}
	
	void SetInvestigateRadius(int investigateRadius)
	{
		m_iInvestigateRadius = investigateRadius;
	}
	
	int GetInvestigateDuration()
	{
		return m_iInvestigateDuration;
	}
	
	void SetInvestigateDuration(int investigateDuration)
	{
		m_iInvestigateDuration = investigateDuration;
	}
	
	int GetCombatMoveChance()
	{
		return m_iCombatMoveChance;
	}
	
	void SetCombatMoveChance(int combatMoveChance)
	{
		m_iCombatMoveChance = combatMoveChance;
	}
	
	int GetCombatCoverChance()
	{
		return m_iCombatCoverChance;
	}
	
	void SetCombatCoverChance(int combatCoverChance)
	{
		m_iCombatCoverChance = combatCoverChance;
	}
	
	int GetCombatDefendChance()
	{
		return m_iCombatDefendChance;
	}
	
	void SetCombatDefendChance(int combatDefendChance)
	{
		m_iCombatDefendChance = combatDefendChance;
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

	int GetRandomPatrolWaypoints()
	{
		return m_iRandomPatrolWaypoints;
	}
	
	void SetRandomPatrolWaypoints(int randomPatrolWaypoints)
	{
		m_iRandomPatrolWaypoints = randomPatrolWaypoints;
	}
};