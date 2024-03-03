/*!
Behavior for short-lived fast reactions to get away from some danger source.
*/
class SCR_AIMoveFromDangerBehavior : SCR_AIBehaviorBase
{
	ref SCR_BTParam<vector> m_DangerPosition = new SCR_BTParam<vector>("DangerPosition");
	ref SCR_BTParam<IEntity> m_DangerEntity = new SCR_BTParam<IEntity>("DangerEntity");
	ref SCR_BTParam<ECharacterStance> m_Stance = new SCR_BTParam<ECharacterStance>("CharacterStance");
	ref SCR_BTParam<EMovementType> m_MovementType = new SCR_BTParam<EMovementType>("MovementType");
	
	//-----------------------------------------------------------------------------------------------------
	void InitParameters(IEntity dangerEntity, vector dangerPos)
	{
		m_DangerEntity.Init(this, dangerEntity);
		m_DangerPosition.Init(this, dangerPos);
		m_Stance.Init(this, ECharacterStance.STAND);
		m_MovementType.Init(this, EMovementType.RUN); // Don't use sprint! Character can't reload during sprint, it makes it broken.
	}
	
	//-----------------------------------------------------------------------------------------------------
	void SCR_AIMoveFromDangerBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector dangerPos, IEntity dangerEntity)
	{
		SetPriority(PRIORITY_BEHAVIOR_MOVE_FROM_DANGER);
		InitParameters(dangerEntity, dangerPos);
				
		if (dangerEntity)
		{
			m_DangerPosition.m_Value = dangerEntity.GetOrigin();
		}
		
		m_sBehaviorTree = "{D12937CF422B639B}AI/BehaviorTrees/Chimera/Soldier/MoveFromDanger_Position.bt"
	}
	
	//-----------------------------------------------------------------------------------------------------
	//! Returns true if there exists a SCR_AIMoveFromDangerBehavior with m_DangerEntity assigned to 'ent'
	static bool ExistsBehaviorForEntity(SCR_AIUtilityComponent utility, IEntity ent)
	{
		SCR_AIMoveFromDangerBehavior behavior = SCR_AIMoveFromDangerBehavior.Cast(utility.FindActionOfInheritedType(SCR_AIMoveFromDangerBehavior));
		if (behavior && behavior.m_DangerEntity.m_Value == ent)
			return true;
		else
			return false;
	}
};

class SCR_AIMoveFromUnknownFire : SCR_AIMoveFromDangerBehavior
{
	//-----------------------------------------------------------------------------------------------------
	void SCR_AIMoveFromUnknownFire(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector dangerPos, IEntity dangerEntity)
	{
		SetPriority(PRIORITY_BEHAVIOR_MOVE_FROM_UNKNOWN_FIRE);
		m_Stance.m_Value = ECharacterStance.STAND;
		m_MovementType.m_Value = EMovementType.SPRINT;
		m_bIsInterruptable = false;
		
		m_bAllowLook = false;
	}
	
	//-----------------------------------------------------------------------------------------------------
	override float CustomEvaluate()
	{
		// This behavior is started by the fact that we don't have a current target,
		// so it also ends when we have selected a target
		if (m_Utility.m_CombatComponent.GetCurrentTarget())
		{
			Complete();
			return 0;
		}
			
		return GetPriority();
	}
	
	//-----------------------------------------------------------------------------------------------------
	override void OnActionSelected()
	{
		super.OnActionSelected();
		
		if (!m_Utility.m_CommsHandler.CanBypass())
		{
			SCR_AITalkRequest rq = new SCR_AITalkRequest(ECommunicationType.REPORT_UNDER_FIRE, null, vector.Zero, 0, false, SCR_EAITalkRequestPreset.IRRELEVANT);
			m_Utility.m_CommsHandler.AddRequest(rq);
		}
	}
}

class SCR_AIMoveFromGrenadeBehavior : SCR_AIMoveFromDangerBehavior
{
	static const float MAX_GRENADE_LOOKAT_TIMEOUT_MS = 600;
	vector m_vObserveReactionPosition;
	static const float DISTANCE_MAX_SQ = 13*13;
	float m_fBehaviorTimeout = 0;
	
	//-----------------------------------------------------------------------------------------------------
	void SCR_AIMoveFromGrenadeBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector dangerPos, IEntity dangerEntity, float behaviorDelay)
	{
		m_bAllowLook = false;
		m_fBehaviorTimeout = GetGame().GetWorld().GetWorldTime() + behaviorDelay;
		m_sBehaviorTree = "{478811D2295EAF3E}AI/BehaviorTrees/Chimera/Soldier/MoveFromDanger_Grenade.bt";
		m_MovementType.m_Value = EMovementType.SPRINT;
		SetPriority(0);
	}
	
		void CreateObserveUnknownBehavior()
	{
		if (m_vObserveReactionPosition == vector.Zero || m_Utility.m_CombatComponent.GetCurrentTarget() != null)
			return;
				
		vector myOrigin = m_Utility.m_OwnerEntity.GetOrigin();
								
		// Stare at grenade origin
		bool addObserveBehavior = false;
		SCR_AIMoveAndInvestigateBehavior investigateBehavior = SCR_AIMoveAndInvestigateBehavior.Cast(m_Utility.FindActionOfType(SCR_AIMoveAndInvestigateBehavior));
		SCR_AIObserveUnknownFireBehavior oldObserveBehavior = SCR_AIObserveUnknownFireBehavior.Cast(m_Utility.FindActionOfType(SCR_AIObserveUnknownFireBehavior));
				
		if (investigateBehavior && investigateBehavior.GetActionState() == EAIActionState.RUNNING)
		{
			if (SCR_AIObserveUnknownFireBehavior.IsNewPositionMoreRelevant(myOrigin, investigateBehavior.m_vPosition.m_Value, m_vObserveReactionPosition))
				addObserveBehavior = true;
		}
		else if (oldObserveBehavior)
		{
			if (SCR_AIObserveUnknownFireBehavior.IsNewPositionMoreRelevant(myOrigin, oldObserveBehavior.m_vPosition.m_Value, m_vObserveReactionPosition))
				addObserveBehavior = true;
		}
		else if (!oldObserveBehavior)
			addObserveBehavior = true;
		
		if (addObserveBehavior)
		{
			SCR_AIObserveUnknownFireBehavior observeBehavior = new SCR_AIObserveUnknownFireBehavior(m_Utility, null, posWorld: m_vObserveReactionPosition);
			m_Utility.AddAction(observeBehavior);
		}
	}
	
	//-----------------------------------------------------------------------------------------------------
	override void OnActionCompleted()
	{
		super.OnActionCompleted();
		CreateObserveUnknownBehavior();
	}	
	
	//-----------------------------------------------------------------------------------------------------
	override float CustomEvaluate()
	{
		if (CompleteActionCondition())
		{
			Complete();
			return 0;
		}
		
		if (GetGame().GetWorld().GetWorldTime() > m_fBehaviorTimeout)
			return PRIORITY_BEHAVIOR_MOVE_FROM_DANGER;
		
		return 0;
	}
	
	bool CompleteActionCondition()
	{
		if (!m_DangerEntity.m_Value)
			return true;
		
		IEntity myEntity = m_Utility.m_OwnerEntity;
		if (!myEntity)
			return true;
		
		if (vector.DistanceSq(myEntity.GetOrigin(), m_DangerPosition.m_Value) > DISTANCE_MAX_SQ)
			return true;
		
		return false;
	}
}

class SCR_AIMoveFromIncomingVehicleBehavior : SCR_AIMoveFromDangerBehavior
{
	//-----------------------------------------------------------------------------------------------------
	void SCR_AIMoveFromIncomingVehicleBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector dangerPos, IEntity dangerEntity)
	{
		m_sBehaviorTree = "{2488649728730886}AI/BehaviorTrees/Chimera/Soldier/MoveFromDanger_Vehicle.bt";
		SetPriority(PRIORITY_BEHAVIOR_MOVE_FROM_DANGER);
		m_MovementType.m_Value = EMovementType.SPRINT;
	}
}

class SCR_AIMoveFromVehicleHornBehavior : SCR_AIMoveFromDangerBehavior
{
	//-----------------------------------------------------------------------------------------------------
	void SCR_AIMoveFromVehicleHornBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, vector dangerPos, IEntity dangerEntity)
	{
		m_sBehaviorTree = "{10A3DFFBC3629A79}AI/BehaviorTrees/Chimera/Soldier/MoveFromDanger_VehicleHorn.bt";
		SetPriority(PRIORITY_BEHAVIOR_MOVE_FROM_VEHICLE_HORN);
	}
}