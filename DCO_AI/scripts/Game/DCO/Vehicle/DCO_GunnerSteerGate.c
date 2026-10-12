modded class SCR_AICombatMoveLogicVehicleGunnerBase
{
	protected float m_fDCOSteerGateAt;
	protected bool m_bDCOSteerBlocked;
	protected float m_fDCOSteerLogAt;

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		float t = GetGame().GetWorld().GetWorldTime();
		if (t >= m_fDCOSteerGateAt)
		{
			m_fDCOSteerGateAt = t + 500.0;
			m_bDCOSteerBlocked = DCO_SteerBlocked(owner, t);
		}
		if (m_bDCOSteerBlocked)
			return ENodeResult.FAIL;
		return super.EOnTaskSimulate(owner, dt);
	}

	protected bool DCO_SteerBlocked(AIAgent owner, float t)
	{
		if (!UpdateDriver(owner) || !m_DriverUtility || !m_DriverUtility.GetAIAgent())
			return false;
		SCR_AIBehaviorBase b = SCR_AIBehaviorBase.Cast(m_DriverUtility.GetExecutedAction());
		if (!b || (!SCR_AIMoveInFormationBehavior.Cast(b) && !SCR_AIMoveIndividuallyBehavior.Cast(b)))
			return false;
		AIGroup grp = m_DriverUtility.GetAIAgent().GetParentGroup();
		if (!grp)
			return false;
		AIWaypoint wp = grp.GetCurrentWaypoint();
		IEntity drv = m_DriverUtility.GetAIAgent().GetControlledEntity();
		if (!wp || !drv)
			return false;
		float d = vector.DistanceXZ(wp.GetOrigin(), drv.GetOrigin());
		if (d <= wp.GetCompletionRadius())
			return false;
		DCO_Perf.Count("cmove:driver_steer_block");
		if (t >= m_fDCOSteerLogAt)
		{
			m_fDCOSteerLogAt = t + 30000.0;
			string veh = "-";
			if (m_MyVehicle && m_MyVehicle.GetPrefabData())
				veh = FilePath.StripExtension(FilePath.StripPath(m_MyVehicle.GetPrefabData().GetPrefabName()));
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdGroup(grp), string.Format("cmove_driver_steer_block grp=%1 veh=%2 behavior=%3 wp_dist=%4 wp_radius=%5 -- driver is travelling to its waypoint; gunner may not steer it (vanilla rule)", DCO_Log.IdGroup(grp), veh, b.ClassName(), Math.Round(d), Math.Round(wp.GetCompletionRadius())));
		}
		return true;
	}
}
