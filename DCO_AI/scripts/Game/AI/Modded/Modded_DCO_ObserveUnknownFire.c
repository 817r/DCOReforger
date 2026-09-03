modded class SCR_AIObserveThreatSystemBehavior : SCR_AIBehaviorBase
{
	protected static const float OBSERVE_DURATION_MIN_S = 1.0;

	void ~SCR_AIObserveThreatSystemBehavior()
	{
		if (!m_Utility || !m_Utility.m_SectorThreatFilter)
			return;

		m_Utility.m_SectorThreatFilter.GetOnEscalationInvoker().Remove(OnThreatSectorEscalation);
		m_Utility.m_SectorThreatFilter.GetOnMajorSectorChangedInvoker().Remove(OnMajorSectorChanged);
		m_Utility.m_SectorThreatFilter.GetOnDamageTaken().Remove(OnDamageTaken);
	}

	override void OnMajorSectorChanged(SCR_AISectorThreatFilter ts, int newSectorId, int oldSectorId, float dangerValue)
	{
		if (newSectorId != -1 && newSectorId != oldSectorId)
			m_iCurrentSectorObserveCounter = 0;

		super.OnMajorSectorChanged(ts, newSectorId, oldSectorId, dangerValue);
	}

	override protected void SwitchToHighPriorityState(float duration_s)
	{
		if (duration_s <= 0)
			duration_s = OBSERVE_DURATION_MIN_S;

		super.SwitchToHighPriorityState(duration_s);
	}

	override protected float CalculateObserveDuration(int sectorId)
	{
		float duration_s = super.CalculateObserveDuration(sectorId);

		duration_s *= DCO_PersonalityCombatUtility.GetObserveDurationScale(m_Utility);
		duration_s *= DCO_MoraleCombatUtility.GetObserveDurationScale(m_Utility.GetMoraleSystem());

		if (duration_s < OBSERVE_DURATION_MIN_S)
			duration_s = OBSERVE_DURATION_MIN_S;

		return duration_s;
	}
	
	override void OnActionExecuted()
	{
		if (!m_Utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
			m_CombatMoveLogic.Update();
		else if (m_Utility.m_AIInfo.HasUnitState(EUnitState.IN_TURRET))
			VehicleObserve();
	}
	
	override float CustomEvaluate()
	{
		if (!m_bBehaviorActive)
			return 0;
		
		// Makes no sense for driver
		if (m_Utility.m_AIInfo.HasUnitState(EUnitState.PILOT))
			return 0;
		
		if (m_fHighPriorityDuration_s != 0)
		{
			WorldTimestamp timestamp = GetGame().GetWorld().GetTimestamp();
			float highPriorityTimePassed_s = timestamp.DiffSeconds(m_TimestampStartHighPriorityState);
			if (highPriorityTimePassed_s > m_fHighPriorityDuration_s)
			{
				m_iCurrentSectorObserveCounter++;
				StopHighPriorityState();
			}
		}

		if (m_fHighPriorityDuration_s != 0)
			return PRIORITY_BEHAVIOR_OBSERVE_THREATS_HIGH_PRIORITY;

		if (m_Utility.ShouldKeepFormation())
		{
			if (!m_Utility.GetSubformationLeaderMoving() && m_Utility.GetNearSubformationLeader())
				return PRIORITY_BEHAVIOR_OBSERVE_THREATS_LOW_PRIORITY;
			else
				return 0;
		}
		else
			return PRIORITY_BEHAVIOR_OBSERVE_THREATS_LOW_PRIORITY;
	}
	
	void VehicleObserve()
	{
		IEntity m_MyEntity = m_Utility.m_OwnerEntity;
		AIGroup myGrp = m_Utility.GetAIAgent().GetParentGroup();
		Vehicle m_MyVehicle;
		SCR_AIUtilityComponent m_DriverUtility;
		SCR_AICombatMoveState m_DriverState;
		
		SCR_CompartmentAccessComponent m_CompartmentAccessComponent = SCR_CompartmentAccessComponent.Cast(m_MyEntity.FindComponent(SCR_CompartmentAccessComponent));
		SCR_AICombatComponent m_CombatComp = SCR_AICombatComponent.Cast(m_MyEntity.FindComponent(SCR_AICombatComponent));
		
		if (m_CompartmentAccessComponent && m_CompartmentAccessComponent.IsInCompartment())
		{
			IEntity turretEnt = m_CompartmentAccessComponent.GetCompartment().GetOwner();
			if (turretEnt)
			{
				TurretControllerComponent contr = TurretControllerComponent.Cast(turretEnt.FindComponent(TurretControllerComponent));
				if (contr)
					TurretComponent m_TurretComponent = contr.GetTurretComponent();
			}
			m_MyVehicle = Vehicle.Cast(m_CompartmentAccessComponent.GetVehicle());
		}
		
		if (!m_MyVehicle)
			return;
		
		IEntity driverEntity = m_MyVehicle.GetPilot();
		if (!driverEntity)
			return;
		
		AIControlComponent driverControlComp = AIControlComponent.Cast(driverEntity.FindComponent(AIControlComponent));
		
		if (!driverControlComp)
			return;
		
		AIAgent driverAgent = driverControlComp.GetAIAgent();
		
		if (!driverAgent)
			return;
		
		m_DriverUtility = SCR_AIUtilityComponent.Cast(driverAgent.FindComponent(SCR_AIUtilityComponent));
		
		if (!m_DriverUtility)
			return;
		
		m_DriverState = m_DriverUtility.m_CombatMoveState;
		
		if (CanMoveVehicle(m_DriverState))
			m_CombatMoveLogic.UpdateVehicle(m_DriverUtility.m_OwnerEntity, m_DriverState, m_DriverUtility);

	}
	
	protected bool IsFirstExecution(SCR_AICombatMoveState driverState)
	{
		return !driverState.GetRequest();
	}
	
	protected bool CanMoveVehicle(SCR_AICombatMoveState driverState)
	{
		float stoppedWaitTime = ResolveStoppedWaitTime();	
		return IsFirstExecution(driverState) || driverState.m_fTimerStopped_s > stoppedWaitTime;
	}
	
	protected float ResolveStoppedWaitTime()
	{
		float waitTime = Math.RandomFloat(20, 30);

		return waitTime;
	}
};