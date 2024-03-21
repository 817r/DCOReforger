class DCO_AIInfoComponentClass : ScriptComponentClass
{
};

enum DCO_EUnitAIState
{
	COVER,
	DANGER
};


//------------------------------------------------------------------------------------------------
class DCO_AIInfoComponent : ScriptComponent
{
	private AIAgent m_Agent;
	private IEntity m_Entity;
	
	private float m_fPerceptionSafe;
	private float m_fPerceptionVigilant;
	
	private float m_fAimAccuracyError;
	private float m_fAimAccuracyErrorOriginal;
	private float m_fAimAccuracyErrorModifier;
	
	private bool m_bHoldPosition;
	private int m_fHoldPositionRadius = 80;
	private vector m_vHoldPositionOrigin;
	private DCO_CUSTOMRANK cussRank;
	
	private DCO_ECombatBehaviorType m_eCombatBehaviorType;
	
	private float m_fAttackReactionDelayModifier;

	private bool m_bDisableMovementControls;
	private bool m_bCombatBehaviorTypeDefensive;
	private bool m_bForceStanceAutonomous = true;
	private bool m_bForceMovementTypeAutonomous = true;
	
	//------------------------------------------------------------------------------------------------
	override protected void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		AIAgent agent = AIAgent.Cast(GetOwner());
		
		if (agent)
		{
			m_Agent = agent;
			
			IEntity controlledEntity = agent.GetControlledEntity();
			
			if (controlledEntity)
			{
				m_Entity = controlledEntity;
				
				vector origin = controlledEntity.GetOrigin();
				
				m_vHoldPositionOrigin = origin;
			}
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	bool GetDisableMovementControls()
	{
		return m_bDisableMovementControls;
	}
	
	void SetDisableMovementControls(bool disableMovementControls)
	{
		m_bDisableMovementControls = disableMovementControls;
	}

	float GetPerceptionSafe()
	{
		return m_fPerceptionSafe;
	}
	
	void SetPerceptionSafe(float perceptionSafe)
	{
		m_fPerceptionSafe = perceptionSafe;
	}
	
	float GetPerceptionVigilant()
	{
		return m_fPerceptionVigilant;
	}
	
	void SetPerceptionVigilant(float perceptionVigilant)
	{
		m_fPerceptionVigilant = perceptionVigilant;
	}

	float GetAimAccuracyError()
	{
		return m_fAimAccuracyError;
	}
	
	void SetAimAccuracyError(float aimAccuracyError)
	{
		m_fAimAccuracyError = aimAccuracyError;
	}
	
	float GetAimAccuracyErrorOriginal()
	{
		return m_fAimAccuracyErrorOriginal;
	}
	
	void SetAimAccuracyErrorOriginal(float aimAccuracyErrorOriginal)
	{
		m_fAimAccuracyErrorOriginal = aimAccuracyErrorOriginal;
	}
	
	float GetAimAccuracyErrorModifier()
	{
		return m_fAimAccuracyErrorModifier;
	}
	
	void SetAimAccuracyErrorModifier(float aimAccuracyErrorModifier)
	{
		m_fAimAccuracyErrorModifier = aimAccuracyErrorModifier;
	}

	bool GetHoldPosition()
	{
		return m_bHoldPosition;
	}
	
	void SetHoldPosition(bool holdPosition)
	{
		m_bHoldPosition = holdPosition;
	}
	
	int GetHoldPositionRadius()
	{
		return m_fHoldPositionRadius;
	}
	
	void SetHoldPositionRadius(int holdPositionRadius)
	{
		m_fHoldPositionRadius = holdPositionRadius;
	}
	
	vector GetHoldPositionOrigin()
	{
		return m_vHoldPositionOrigin;
	}
	
	void SetHoldPositionOrigin(vector holdPositionOrigin)
	{
		m_vHoldPositionOrigin = holdPositionOrigin;
	}

	float GetAttackReactionDelayModifier()
	{
		return m_fAttackReactionDelayModifier;
	}
	
	void SetAttackReactionDelayModifier(float attackReactionDelayModifier)
	{
		m_fAttackReactionDelayModifier = attackReactionDelayModifier;
	}

	bool GetForceStanceAutonomous()
	{
		return m_bForceStanceAutonomous;
	}
	
	void SetForceStanceAutonomous(bool forceStanceAutonomous)
	{
		m_bForceStanceAutonomous = forceStanceAutonomous;
	}
	
	bool GetForceMovementTypeAutonomous()
	{
		return m_bForceMovementTypeAutonomous;
	}
	
	void SetForceMovementTypeAutonomous(bool forceMovementTypeAutonomous)
	{
		m_bForceMovementTypeAutonomous = forceMovementTypeAutonomous;
	}
	
	bool IsCombatBehaviorTypeDefensive()
	{
		return m_bCombatBehaviorTypeDefensive;
	}
	
	void SetCombatBehaviorTypeDefensive(bool combatBehaviorTypeDefensive)
	{
		m_bCombatBehaviorTypeDefensive = combatBehaviorTypeDefensive
	}
	
	DCO_ECombatBehaviorType GetCombatBehaviorType()
	{
		return m_eCombatBehaviorType;
	}
	
	void SetCombatBehaviorType(DCO_ECombatBehaviorType combatBehaviorType)
	{
		m_eCombatBehaviorType = combatBehaviorType;
	}
	
	void setRank(int cusRank)
	{
		cussRank = cusRank;
	}
}