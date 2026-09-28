modded class SCR_AICombatMoveLogicVehicleGunner_Attack : SCR_AICombatMoveLogicVehicleGunnerBase
{
	protected static const string PORT_BASE_TARGET = "BaseTarget";

	protected BaseTarget m_Target;

	protected const float WEAPON_MIN_DIST = 2.0;

	protected const float MIN_ENGAGEMENT_DISTANCE_TO_TARGET_SQ = 80.0 * 80.0;
	protected const float MAX_MOVE_DURATION_TO_TARGET_S = 9;
	protected const float MAX_MOVE_DURATION_TO_TARGET_THREATENED_S = 7;
	protected const float REVERSE_MOVE_DURATION_S = 3;

	protected const float STANDOFF_MG = 150;
	protected const float STANDOFF_AUTOCANNON = 250;
	protected const float THREATENED_MAX_STOP_S = 6;
	protected const float RELOCATE_MIN_S = 3;
	protected const float RELOCATE_MAX_S = 5;

	protected EAIThreatState m_eThreatState;
	protected float m_fTargetDist;
	protected float m_fWeaponMinDist = WEAPON_MIN_DIST;

	override bool UpdateCombatMoveLogic()
	{
		GetVariableIn(PORT_BASE_TARGET, m_Target);
		if (!m_Target || !m_Target.GetTargetEntity())
			return false;

		m_fTargetDist = GetTargetDistance();
		m_eThreatState = m_Utility.m_ThreatSystem.GetState();
		m_fWeaponMinDist = 2.0;

		IEntity atShooter;
		vector atPos;
		if (DCO_VehicleCombat.GetATThreat(m_MyVehicle, atShooter, atPos)
			&& vector.Distance(atPos, m_MyVehicle.GetOrigin()) < DCO_VehicleCombat.AT_EVADE_DIST)
		{
			if (DCO_VehicleCombat.CanPushEvade(m_DriverState))
			{
				ApplyNewRequest(DCO_VehiclePositioning.CreateATEvadeRequest(m_MyVehicle, atPos));
				DCO_BenchmarkLoggerComponent.Event(string.Format("at_evade veh=%1 dist=%2", m_MyVehicle, Math.Round(vector.Distance(atPos, m_MyVehicle.GetOrigin()))));
			}
		}
		else if (MoveFromTargetCondition())
		{
			if (MoveFromTargetNewRequestCondition())
				PushRequestMoveFromTarget();
		}
		else if (FFAvoidanceCondition())
		{
			if (FFAvoidanceNewRequestCondition())
				PushRequestFFAvoidance();
		}
		else if (DCO_VehiclePositioning.IsHiding(m_MyVehicle))
		{
		}
		else if (RelocateUnderFireCondition())
		{
			DCO_RelocateUnderFire();
		}
		else if (MoveToNextPosCondition())
		{
			PushRequestMove();
		}
		else if (!m_DriverState.IsExecutingRequest())
		{
		}

		return true;
	}

	override protected void PushRequestMove()
	{
		vector wpPos, firePos;
		float wpRadius;
		if (DCO_GetMoveBounds(wpPos, wpRadius) && DCO_VehiclePositioning.FindFirePosition(m_MyVehicle, m_Target.GetLastSeenPosition(),
			ResolveStandoffDistance(), DCO_VehiclePositioning.GetHullDownWeight(DCO_VehiclePositioning.GetPosture(m_Utility)), wpPos, wpRadius, firePos))
		{
			SCR_AICombatMoveRequest_Move fireRq = DCO_VehiclePositioning.CreateMoveRequest(m_MyVehicle.GetOrigin(), firePos,
				m_Target.GetLastSeenPosition(), SCR_EAICombatMoveReason.STANDARD);
			fireRq.GetOnMovementStarted().Insert(OnMovementStarted);
			fireRq.GetOnCompleted().Insert(OnMovementCompleted);
			ApplyNewRequest(fireRq);
			DCO_BenchmarkLoggerComponent.Event(string.Format("veh_firepos veh=%1 move=%2 tgt=%3", m_MyVehicle,
				Math.Round(vector.Distance(m_MyVehicle.GetOrigin(), firePos)), Math.Round(vector.Distance(firePos, m_Target.GetLastSeenPosition()))));
			return;
		}

		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;

		rq.m_vTargetPos = ResolveRequestTargetPos();
		ResolveMoveRequestMovePosAndDir(rq.m_vTargetPos, rq.m_vMovePos, rq.m_eDirection);
		rq.m_bTryFindCover = false;
		rq.m_bUseCoverSearchDirectivity = false;
		rq.m_bCheckCoverVisibility = false;

		float moveDurationMax = MAX_MOVE_DURATION_TO_TARGET_S;

		switch (m_eThreatState)
		{
			case EAIThreatState.THREATENED:
			{
				moveDurationMax = MAX_MOVE_DURATION_TO_TARGET_THREATENED_S;
				break;
			}
			default:
			{
				moveDurationMax = MAX_MOVE_DURATION_TO_TARGET_S;
				break;
			}
		}

		rq.m_bAimAtTarget = false;
		rq.m_bAimAtTargetEnd = false;
		rq.m_bFailIfNoCover = false;
		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		rq.m_fMoveDuration_s = Math.RandomFloat(0.8, 1.3) * moveDurationMax;
		vector dirToTgt = m_Target.GetLastSeenPosition() - m_DriverUtility.m_OwnerEntity.GetOrigin();
		dirToTgt.Normalize();
		rq.m_vAvoidStraightPathDir = dirToTgt;
		rq.GetOnMovementStarted().Insert(OnMovementStarted);
		rq.GetOnCompleted().Insert(OnMovementCompleted);

		ApplyNewRequest(rq);
	}

	override protected void ResolveMoveRequestMovePosAndDir(vector targetPos, out vector outMovePos, out SCR_EAICombatMoveDirection outDirection)
	{
		AIWaypoint wp = null;
		AIAgent agent = m_DriverUtility.GetAIAgent();
		AIGroup group = agent.GetParentGroup();
		if (group)
			wp = group.GetCurrentWaypoint();

		vector movePos;
		SCR_EAICombatMoveDirection eDirection;

		if (!wp)
		{
			eDirection = SCR_EAICombatMoveDirection.FORWARD;
			movePos = targetPos;
		}
		else
		{
			vector wpPos = wp.GetOrigin();
			float wpRadius = wp.GetCompletionRadius();
			bool tgtInWaypoint = vector.DistanceXZ(wpPos, targetPos) < wpRadius;
			float myDistToWp = vector.DistanceXZ(wpPos, m_MyEntity.GetOrigin());

			if (myDistToWp > wpRadius)
			{
				movePos = wpPos;
				eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
			}
			else if (myDistToWp > 0.5 * wpRadius)
			{
				if (tgtInWaypoint)
				{
					movePos = targetPos;
					eDirection = SCR_EAICombatMoveDirection.FORWARD;
				}
				else
				{
					movePos = targetPos;
					eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
				}
			}
			else
			{
				movePos = targetPos;
				eDirection = SCR_EAICombatMoveDirection.FORWARD;
			}
		}

		outMovePos = movePos;
		outDirection = eDirection;
	}

	protected bool RelocateUnderFireCondition()
	{
		return m_eThreatState == EAIThreatState.THREATENED
			&& !m_DriverState.IsExecutingRequest()
			&& m_DriverState.m_fTimerStopped_s > THREATENED_MAX_STOP_S;
	}

	protected bool DCO_GetMoveBounds(out vector wpPos, out float wpRadius)
	{
		wpRadius = 0;
		AIGroup group = m_DriverUtility.GetAIAgent().GetParentGroup();
		if (!group || !group.GetCurrentWaypoint())
			return true;

		wpPos = group.GetCurrentWaypoint().GetOrigin();
		wpRadius = Math.Max(group.GetCurrentWaypoint().GetCompletionRadius(), 60);
		return vector.DistanceXZ(wpPos, m_MyVehicle.GetOrigin()) <= wpRadius;
	}

	protected void DCO_RelocateUnderFire()
	{
		vector threatPos = m_Target.GetLastSeenPosition();
		vector wpPos, pos;
		float wpRadius;
		if (DCO_GetMoveBounds(wpPos, wpRadius))
		{
			DCO_GroupTactics posture = DCO_VehiclePositioning.GetPosture(m_Utility);
			if (DCO_VehiclePositioning.PrefersHiding(posture, m_MyVehicle)
				&& DCO_VehiclePositioning.FindHidePosition(m_MyVehicle, threatPos, wpPos, wpRadius, pos))
			{
				ApplyNewRequest(DCO_VehiclePositioning.CreateMoveRequest(m_MyVehicle.GetOrigin(), pos, threatPos, SCR_EAICombatMoveReason.MOVE_FROM_TARGET));
				DCO_VehiclePositioning.MarkHiding(m_MyVehicle);
				DCO_BenchmarkLoggerComponent.Event(string.Format("veh_hide veh=%1 posture=%2", m_MyVehicle, typename.EnumToString(DCO_GroupTactics, posture)));
				return;
			}

			if (DCO_VehiclePositioning.FindFirePosition(m_MyVehicle, threatPos, ResolveStandoffDistance(),
				DCO_VehiclePositioning.GetHullDownWeight(posture), wpPos, wpRadius, pos))
			{
				ApplyNewRequest(DCO_VehiclePositioning.CreateMoveRequest(m_MyVehicle.GetOrigin(), pos, threatPos, SCR_EAICombatMoveReason.STANDARD));
				return;
			}
		}

		PushRequestRelocate();
	}

	protected void PushRequestRelocate()
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		rq.m_vMovePos = m_Target.GetLastSeenPosition();
		rq.m_vTargetPos = rq.m_vMovePos;
		if (Math.RandomInt(0, 2) == 0)
			rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
		else
			rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
		rq.m_fMoveDuration_s = Math.RandomFloat(RELOCATE_MIN_S, RELOCATE_MAX_S);
		rq.m_bAimAtTarget = false;
		rq.m_bAimAtTargetEnd = false;

		ApplyNewRequest(rq);
	}

	protected float m_fDCOATCheck_ms = -1;
	protected bool m_bDCOATKnown;

	protected bool DCO_IsATKnown()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (m_fDCOATCheck_ms < 0 || now - m_fDCOATCheck_ms > 1000)
		{
			m_fDCOATCheck_ms = now;
			m_bDCOATKnown = DCO_VehicleCombat.IsATKnown(m_MyVehicle, m_Utility);
		}
		return m_bDCOATKnown;
	}

	override protected bool MoveFromTargetCondition()
	{
		if (!DCO_IsATKnown() && m_fTargetDist > DCO_VehicleCombat.NO_AT_MIN_DIST)
			return false;
		return super.MoveFromTargetCondition();
	}

	protected float ResolveStandoffDistance()
	{
		float dist = ResolveOptimalDistance(m_fWeaponMinDist);
		if (!DCO_IsATKnown())
		{
			dist = DCO_VehicleCombat.NO_AT_STANDOFF;
		}
		else
		{
			switch (m_CombatComp.GetCurrentWeaponType())
			{
				case EWeaponType.WT_MACHINEGUN:	dist = Math.Max(dist, STANDOFF_MG); break;
				case EWeaponType.WT_AUTOCANNON:	dist = Math.Max(dist, STANDOFF_AUTOCANNON); break;
			}
		}
		return dist * DCO_VehiclePositioning.GetStandoffScale(DCO_VehiclePositioning.GetPosture(m_Utility));
	}

	override protected bool MoveToNextPosCondition()
	{
		float optimalDist = ResolveStandoffDistance();
		if (m_fTargetDist < optimalDist && m_Target.GetTimeSinceSeen() < 5)
			return false;

		if (m_DriverState.IsExecutingRequest())
			return false;

		if (IsFirstExecution())
			return true;

		float stoppedWaitTime = ResolveStoppedWaitTime(m_eThreatState);
		return m_DriverState.m_fTimerStopped_s > stoppedWaitTime;
	}

	override protected static float ResolveOptimalDistance(float weaponMinDist)
	{
		return Math.Max(weaponMinDist + 5.0, 70);
	}
}

class SCR_AICombatMoveLogicVehicleGunner_SuppressiveDCO : SCR_AICombatMoveLogicVehicleGunner_Suppressive
{
	protected static const string PORT_VISIBLE = "Visible";
	protected static const string PORT_TIME_LAST_SEEN = "TimeLastSeen_ms";

	protected bool m_bTargetVisible = false;
	protected bool m_bGoodVision;
	protected float m_fTargetLastSeenTime_ms = 0;
	protected static const float TIME_SINCE_GOOD_VISIBILITY_MIN_MS = 15000.0;

	protected const float MIN_ENGAGEMENT_DISTANCE_TO_TARGET_SQ = 40.0 * 40.0;

	protected const float REVERSE_MOVE_DURATION_S = 3;

	override bool UpdateCombatMoveLogic()
	{
		GetVariableIn(PORT_SUPPRESSION_VOLUME, m_SuppressionVolume);
		if (!m_SuppressionVolume)
			return false;

		GetVariableIn(PORT_VISIBLE, m_bTargetVisible);
		GetVariableIn(PORT_TIME_LAST_SEEN, m_fTargetLastSeenTime_ms);

		float timeSinceLastSeen_ms = GetGame().GetWorld().GetWorldTime() - m_fTargetLastSeenTime_ms;
		m_bGoodVision = m_bTargetVisible || (timeSinceLastSeen_ms < TIME_SINCE_GOOD_VISIBILITY_MIN_MS);

		IEntity atShooter;
		vector atPos;
		if (DCO_VehicleCombat.GetATThreat(m_MyVehicle, atShooter, atPos)
			&& vector.Distance(atPos, m_MyVehicle.GetOrigin()) < DCO_VehicleCombat.AT_EVADE_DIST)
		{
			if (DCO_VehicleCombat.CanPushEvade(m_DriverState))
			{
				m_DriverState.ApplyNewRequest(DCO_VehiclePositioning.CreateATEvadeRequest(m_MyVehicle, atPos));
				DCO_BenchmarkLoggerComponent.Event(string.Format("at_evade veh=%1 dist=%2", m_MyVehicle, Math.Round(vector.Distance(atPos, m_MyVehicle.GetOrigin()))));
			}
		}
		else if (MoveFromTargetCondition())
		{
			if (MoveFromTargetNewRequestCondition())
				PushRequestMoveFromTarget();
		}
		else if (TimeToMove())
		{
			PushRequestRotateToTarget();
		}

		return true;
	}

	protected bool MoveFromTargetCondition()
	{
		if (!m_WeaponManagerComponent)
			return false;

		vector mat[4];
		m_WeaponManagerComponent.GetCurrentMuzzleTransform(mat);
		vector muzzlePos = mat[3];
		vector muzzleDir = mat[2].Normalized();
		vector targetPos = m_SuppressionVolume.GetCenterPosition();
		vector targetDir = (targetPos - muzzlePos).Normalized();

		if (vector.DistanceSq(targetPos, muzzlePos) < MIN_ENGAGEMENT_DISTANCE_TO_TARGET_SQ)
			return true;

		return false;
	}

	protected bool MoveFromTargetNewRequestCondition()
	{
		if (!m_DriverState.IsExecutingRequest())
			return true;

		SCR_AICombatMoveRequest_Move rq = SCR_AICombatMoveRequest_Move.Cast(m_DriverState.GetRequest());
		if (!rq)
			return true;

		return rq.m_eReason != SCR_EAICombatMoveReason.MOVE_FROM_TARGET;
	}

	protected void PushRequestMoveFromTarget()
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

		rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_TARGET;
		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		rq.m_vMovePos = m_SuppressionVolume.GetCenterPosition();
		rq.m_eMovementType = EMovementType.RUN;
		rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
		rq.m_fMoveDuration_s = 80 * Math.RandomFloat(1, 1.5) / SCR_AICombatMoveUtils.GROUND_VEHICLE_GENERIC_SPEED;
		rq.m_bAimAtTarget = false;
		rq.m_bAimAtTargetEnd = false;

		m_DriverState.ApplyNewRequest(rq);
	}

	protected bool TimeToMove()
	{
		vector targetPos = m_SuppressionVolume.GetCenterPosition();
		if (m_DriverState.IsExecutingRequest())
			return false;

		if (!TargetWithinTurretSafeHorizontalLimits(targetPos))
			return true;

		if (!m_bGoodVision)
			return true;

		if (m_DriverState.m_fTimerStopped_s > Math.RandomFloatInclusive(20, 30))
			return true;

		return false;
	}

	override void PushRequestRotateToTarget()
	{
		vector center = m_SuppressionVolume.GetCenterPosition();
		DCO_GroupTactics posture = DCO_VehiclePositioning.GetPosture(m_Utility);
		float standoff = 150;
		if (!DCO_VehicleCombat.IsATKnown(m_MyVehicle, m_Utility))
			standoff = DCO_VehicleCombat.NO_AT_STANDOFF;
		vector firePos;
		if (DCO_VehiclePositioning.FindFirePosition(m_MyVehicle, center, standoff * DCO_VehiclePositioning.GetStandoffScale(posture),
			DCO_VehiclePositioning.GetHullDownWeight(posture), vector.Zero, 0, firePos))
		{
			ApplyNewRequest(DCO_VehiclePositioning.CreateMoveRequest(m_MyVehicle.GetOrigin(), firePos, center, SCR_EAICombatMoveReason.STANDARD));
			return;
		}

		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		rq.m_vMovePos = m_SuppressionVolume.GetCenterPosition();
		rq.m_vTargetPos = rq.m_vMovePos;
		if (Math.RandomInt(0, 3) >= 1)
		{
			rq.m_eDirection = SCR_EAICombatMoveDirection.FORWARD;
		}
		else
		{
			if (Math.RandomInt(0, 1) == 0)
				rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
			else
				rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
		}
		rq.m_fMoveDuration_s = 50 * Math.RandomFloat(1, 1.5) / SCR_AICombatMoveUtils.GROUND_VEHICLE_GENERIC_SPEED;
		rq.m_bAimAtTarget = false;
		rq.m_bAimAtTargetEnd = false;

		ApplyNewRequest(rq);
	}

	protected static ref TStringArray s_aVarisIn = {
		PORT_SUPPRESSION_VOLUME,
		PORT_VISIBLE,
		PORT_TIME_LAST_SEEN
	};
	override TStringArray GetVariablesIn() { return s_aVarisIn; }
}