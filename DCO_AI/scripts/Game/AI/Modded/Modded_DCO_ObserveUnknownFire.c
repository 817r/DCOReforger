modded class SCR_AIObserveThreatSystemBehavior : SCR_AIBehaviorBase
{
	override void OnActionExecuted()
	{
		if (!m_Utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
			m_CombatMoveLogic.Update();
		else if (m_Utility.m_AIInfo.HasUnitState(EUnitState.IN_TURRET))
			VehicleObserve();
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
		
		vector threatPos = m_Utility.m_SectorThreatFilter.GetSectorPos(m_iCurrentSector);
		float m_fTargetDist = vector.Distance(m_Utility.GetOrigin(), threatPos);
		float radius = Math.Map(m_fTargetDist, 50, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE, 4, 10);
		vector bbMax, bbMin;
		SCR_AISuppressionVolumeBase.CreateSuppressionBox(threatPos, radius, 4, bbMin, bbMax);
		SCR_AISuppressionObjectVolumeBox createSupp = new SCR_AISuppressionObjectVolumeBox(bbMin, bbMax);
		SCR_AISuppressBehavior supp = new SCR_AISuppressBehavior(m_Utility, null, createSupp, 5, 3);
		m_Utility.AddAction(supp);	
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