modded class SCR_AICombatMoveLogicVehicleGunner_Attack : SCR_AICombatMoveLogicVehicleGunnerBase
{
	protected static const string PORT_BASE_TARGET = "BaseTarget";

	protected BaseTarget m_Target;

	protected const float WEAPON_MIN_DIST = 2.0;

	protected const float MIN_ENGAGEMENT_DISTANCE_TO_TARGET_SQ = 80.0 * 80.0;
	protected const float MAX_MOVE_DURATION_TO_TARGET_S = 9;
	protected const float MAX_MOVE_DURATION_TO_TARGET_THREATENED_S = 7;
	protected const float REVERSE_MOVE_DURATION_S = 3;


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

		SCR_AICombatMoveRequest_Move atRq;
		if (DCO_VehicleCombat.ATReact(m_MyVehicle, m_Utility, m_DriverState, atRq))
		{
			if (atRq)
				ApplyNewRequest(atRq);
		}
		else if (DCO_UpdateDuel())
		{
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
			DCO_VehicleActive.NoteFirePos(m_MyVehicle, firePos);
			bool aggr = DCO_VehiclePositioning.Aggr(m_Utility);
			if (aggr)
				DCO_Perf.Count("veh:aggr_firepos");
			DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("veh_firepos veh=%1 move=%2 tgt=%3 aggr=%4 standoff=%5 bound_r=%6 at=%7", DCO_Log.IdVeh(m_MyVehicle),
				Math.Round(vector.Distance(m_MyVehicle.GetOrigin(), firePos)), Math.Round(vector.Distance(firePos, m_Target.GetLastSeenPosition())), aggr, Math.Round(ResolveStandoffDistance()), Math.Round(wpRadius), DCO_IsATKnown()));
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
			float wpRadius = wp.GetCompletionRadius() + DCO_ChaseExtra();
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
			&& m_DriverState.m_fTimerStopped_s > 6.0;
	}

	protected float DCO_ChaseExtra()
	{
		if (DCO_VehiclePositioning.Aggr(m_Utility))
			return 150.0;
		return 0;
	}

	protected bool DCO_GetMoveBounds(out vector wpPos, out float wpRadius)
	{
		wpRadius = 0;
		AIGroup group = m_DriverUtility.GetAIAgent().GetParentGroup();
		if (!group)
			return true;

		float extra = DCO_ChaseExtra();
		DCO_GroupUtilityComponent dg = DCO_GroupUtilityComponent.Cast(group.FindComponent(DCO_GroupUtilityComponent));
		if (dg && dg.GetGroupObjective() && dg.GetGroupObjective().GetOwner())
		{
			vector objPos = dg.GetGroupObjective().GetOwner().GetOrigin();
			float objR = dg.GetGroupObjective().GetRadius();
			if (vector.DistanceXZ(objPos, m_MyVehicle.GetOrigin()) <= objR + 150.0 + extra)
			{
				wpPos = objPos;
				wpRadius = Math.Max(objR + 50.0, 60.0) + extra;
				return vector.DistanceXZ(wpPos, m_MyVehicle.GetOrigin()) <= wpRadius;
			}
		}

		if (!group.GetCurrentWaypoint())
			return true;

		wpPos = group.GetCurrentWaypoint().GetOrigin();
		wpRadius = Math.Max(group.GetCurrentWaypoint().GetCompletionRadius(), 60) + extra;
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
				DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("veh_hide veh=%1 posture=%2", DCO_Log.IdVeh(m_MyVehicle), typename.EnumToString(DCO_GroupTactics, posture)));
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
		rq.m_fMoveDuration_s = Math.RandomFloat(3.0, 5.0);
		rq.m_bAimAtTarget = false;
		rq.m_bAimAtTargetEnd = false;

		ApplyNewRequest(rq);
	}

	protected int m_iDCODuelAmmo = -1;
	protected int m_iDCODuelShots;
	protected float m_fDCODuelAt_ms;
	protected float m_fDCOOutgunAt_ms = -100000;

	protected bool DCO_UpdateDuel()
	{
		IEntity tv = DCO_VehicleCombat.TargetVehicle(m_Target.GetTargetEntity());
		if (!tv || tv == m_MyVehicle || !DCO_VehicleCombat.IsArmored(tv))
		{
			m_iDCODuelAmmo = -1;
			return false;
		}
		if (m_DriverState.IsExecutingRequest())
			return true;

		float now = GetGame().GetWorld().GetWorldTime();
		vector tpos = m_Target.GetLastSeenPosition();
		if (DCO_Outgunned(tv))
		{
			if (now - m_fDCOOutgunAt_ms > 30000.0)
			{
				m_fDCOOutgunAt_ms = now;
				DCO_DuelOutgunned(tpos, tv);
			}
			return true;
		}

		int ammo = DCO_CurrentAmmo();
		if (m_iDCODuelAmmo < 0)
		{
			vector wpPos, firePos;
			float wpRadius;
			DCO_GetMoveBounds(wpPos, wpRadius);
			float desired = Math.Clamp(m_fTargetDist, 150.0, 600.0);
			if (DCO_VehiclePositioning.FindFirePosition(m_MyVehicle, tpos, desired, 0.8, wpPos, wpRadius, firePos, true)
				&& vector.DistanceXZ(firePos, m_MyVehicle.GetOrigin()) > 8.0)
				ApplyNewRequest(DCO_VehiclePositioning.CreateMoveRequest(m_MyVehicle.GetOrigin(), firePos, tpos, SCR_EAICombatMoveReason.STANDARD));
			m_iDCODuelAmmo = Math.Max(ammo, 0);
			m_iDCODuelShots = Math.RandomIntInclusive(1, 2);
			if (DCO_VehiclePositioning.Aggr(m_Utility))
				m_iDCODuelShots = Math.RandomIntInclusive(2, 4);
			m_fDCODuelAt_ms = now;
			DCO_Perf.Count("veh:duel_hulldown");
			return true;
		}

		int fired = m_iDCODuelAmmo - ammo;
		if (ammo > m_iDCODuelAmmo)
			fired = m_iDCODuelShots;
		if (ammo < 0)
			fired = 0;
		bool idle = now - m_fDCODuelAt_ms > 25000.0;
		if (fired < m_iDCODuelShots && !idle)
			return true;

		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_TARGET;
		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		rq.m_vMovePos = tpos;
		rq.m_vTargetPos = tpos;
		rq.m_eMovementType = EMovementType.RUN;
		rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
		rq.m_fMoveDuration_s = Math.RandomFloat(20.0, 40.0) / SCR_AICombatMoveUtils.GROUND_VEHICLE_GENERIC_SPEED;
		string scootDir = "back";
		if (DCO_VehiclePositioning.Aggr(m_Utility))
		{
			rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
			if (Math.RandomInt(0, 2) == 0)
				rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
			else
				rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
			rq.m_fMoveDuration_s = Math.RandomFloat(15.0, 25.0) / SCR_AICombatMoveUtils.GROUND_VEHICLE_GENERIC_SPEED;
			scootDir = "lateral";
		}
		rq.m_bAimAtTarget = false;
		rq.m_bAimAtTargetEnd = false;
		ApplyNewRequest(rq);
		m_iDCODuelAmmo = -1;
		DCO_Perf.Count("veh:duel_scoot_" + scootDir);
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("veh_duel_scoot veh=%1 fired=%2 idle=%3 dir=%4", DCO_Log.IdVeh(m_MyVehicle), fired, idle, scootDir));
		return true;
	}

	protected int DCO_CurrentAmmo()
	{
		if (!m_WeaponManagerComponent || !m_WeaponManagerComponent.GetCurrentWeapon())
			return -1;
		BaseMagazineComponent mag = m_WeaponManagerComponent.GetCurrentWeapon().GetCurrentMagazine();
		if (!mag)
			return -1;
		return mag.GetAmmoCount();
	}

	protected bool DCO_Outgunned(IEntity tv)
	{
		if (m_CombatComp.GetCurrentWeaponType() == EWeaponType.WT_MACHINEGUN)
			return true;
		PerceivableComponent mine = PerceivableComponent.Cast(m_MyVehicle.FindComponent(PerceivableComponent));
		PerceivableComponent theirs = PerceivableComponent.Cast(tv.FindComponent(PerceivableComponent));
		return mine && theirs && theirs.GetUnitType() == EAIUnitType.UnitType_VehicleHeavy && mine.GetUnitType() != EAIUnitType.UnitType_VehicleHeavy;
	}

	protected void DCO_DuelOutgunned(vector tpos, IEntity tv)
	{
		bool mgVsHeavy = false;
		PerceivableComponent tvPerc = PerceivableComponent.Cast(tv.FindComponent(PerceivableComponent));
		if (m_CombatComp.GetCurrentWeaponType() == EWeaponType.WT_MACHINEGUN && tvPerc && tvPerc.GetUnitType() == EAIUnitType.UnitType_VehicleHeavy)
			mgVsHeavy = true;
		AIGroup group = m_DriverUtility.GetAIAgent().GetParentGroup();
		DCO_GroupUtilityComponent dg;
		if (group)
			dg = DCO_GroupUtilityComponent.Cast(group.FindComponent(DCO_GroupUtilityComponent));
		if (dg && dg.GetMyCommander() && dg.GetMyCommander().GetThreatResponseComponent())
		{
			CMD_ContactReport report = new CMD_ContactReport(tpos, 1, DCO_Util.NowS(), "ARMOR:" + dg.GetOwner().GetName());
			report.m_bArmorSeen = true;
			report.m_iArmor = 1;
			report.m_bFiringAtUs = true;
			dg.GetMyCommander().GetThreatResponseComponent().ReceiveContactReport(report, dg);
			DCO_Perf.Count("veh:duel_request");
		}

		vector pos;
		DCO_GroupTactics posture = DCO_VehiclePositioning.GetPosture(m_Utility);
		if (DCO_VehiclePositioning.Aggr(m_Utility) && !mgVsHeavy)
		{
			vector away = m_MyVehicle.GetOrigin() - tpos;
			away[1] = 0;
			float d = away.Length();
			if (d > 1)
			{
				away = away * (1.0 / d);
				vector side = Vector(away[2], 0, -away[0]);
				if (Math.RandomFloat01() < 0.5)
					side = side * -1.0;
				vector flank = tpos + side * Math.Clamp(d, 150.0, 400.0);
				if (DCO_VehiclePositioning.FindFirePosition(m_MyVehicle, tpos, Math.Clamp(d, 150.0, 400.0), 0.8, flank, 120.0, pos))
				{
					ApplyNewRequest(DCO_VehiclePositioning.CreateMoveRequest(m_MyVehicle.GetOrigin(), pos, tpos, SCR_EAICombatMoveReason.STANDARD));
					DCO_Perf.Count("veh:duel_flank");
					DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("veh_duel_flank veh=%1 posture=%2", DCO_Log.IdVeh(m_MyVehicle), typename.EnumToString(DCO_GroupTactics, posture)));
					return;
				}
			}
		}

		if (DCO_VehiclePositioning.FindHidePosition(m_MyVehicle, tpos, vector.Zero, 0, pos))
		{
			ApplyNewRequest(DCO_VehiclePositioning.CreateMoveRequest(m_MyVehicle.GetOrigin(), pos, tpos, SCR_EAICombatMoveReason.MOVE_FROM_TARGET));
			DCO_VehiclePositioning.MarkHiding(m_MyVehicle);
		}
		DCO_Perf.Count("veh:duel_withdraw");
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("veh_duel_withdraw veh=%1 posture=%2 mg_vs_heavy=%3", DCO_Log.IdVeh(m_MyVehicle), typename.EnumToString(DCO_GroupTactics, posture), mgVsHeavy));
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
		if (!DCO_IsATKnown() && m_fTargetDist > 25.0)
			return false;
		return super.MoveFromTargetCondition();
	}

	protected float ResolveStandoffDistance()
	{
		float dist = ResolveOptimalDistance(m_fWeaponMinDist);
		if (!DCO_IsATKnown())
		{
			dist = 60.0;
			if (DCO_VehiclePositioning.Aggr(m_Utility))
				dist = 50.0;
		}
		else
		{
			switch (m_CombatComp.GetCurrentWeaponType())
			{
				case EWeaponType.WT_MACHINEGUN:	dist = Math.Max(dist, 150.0); break;
				case EWeaponType.WT_AUTOCANNON:	dist = Math.Max(dist, 250.0); break;
			}
			if (DCO_VehicleActive.VehClass(m_MyVehicle) == 3)
				dist = Math.Max(dist, 300.0);
		}
		return dist * Math.Min(DCO_VehiclePositioning.GetStandoffScale(DCO_VehiclePositioning.GetPosture(m_Utility)), 1.2);
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



	override bool UpdateCombatMoveLogic()
	{
		GetVariableIn(PORT_SUPPRESSION_VOLUME, m_SuppressionVolume);
		if (!m_SuppressionVolume)
			return false;

		GetVariableIn(PORT_VISIBLE, m_bTargetVisible);
		GetVariableIn(PORT_TIME_LAST_SEEN, m_fTargetLastSeenTime_ms);

		float timeSinceLastSeen_ms = GetGame().GetWorld().GetWorldTime() - m_fTargetLastSeenTime_ms;
		m_bGoodVision = m_bTargetVisible || (timeSinceLastSeen_ms < TIME_SINCE_GOOD_VISIBILITY_MIN_MS);

		SCR_AICombatMoveRequest_Move atRq;
		if (DCO_VehicleCombat.ATReact(m_MyVehicle, m_Utility, m_DriverState, atRq))
		{
			if (atRq)
				m_DriverState.ApplyNewRequest(atRq);
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

		float tooCloseSq = 1600.0;
		if (DCO_VehiclePositioning.Aggr(m_Utility))
			tooCloseSq = 400.0;
		if (vector.DistanceSq(targetPos, muzzlePos) < tooCloseSq)
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
		float backDist = 80;
		if (DCO_VehiclePositioning.Aggr(m_Utility))
			backDist = 40;
		rq.m_fMoveDuration_s = backDist * Math.RandomFloat(1, 1.5) / SCR_AICombatMoveUtils.GROUND_VEHICLE_GENERIC_SPEED;
		rq.m_bAimAtTarget = false;
		rq.m_bAimAtTargetEnd = false;

		m_DriverState.ApplyNewRequest(rq);
		DCO_Perf.Count("veh:supp_backoff");
		DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.INFO, "-", string.Format("veh_supp_backoff veh=%1 dist=%2 back=%3", DCO_Log.IdVeh(m_MyVehicle), Math.Round(vector.Distance(m_MyVehicle.GetOrigin(), rq.m_vMovePos)), backDist));
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
		{
			standoff = 60.0;
			if (DCO_VehiclePositioning.Aggr(m_Utility))
				standoff = 50.0;
		}
		vector firePos;
		if (DCO_VehiclePositioning.FindFirePosition(m_MyVehicle, center, standoff * Math.Min(DCO_VehiclePositioning.GetStandoffScale(posture), 1.2),
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

	protected static ref TStringArray s_aVarisIn;
	protected static TStringArray GetVarisIn()
	{
		if (!s_aVarisIn)
			s_aVarisIn = { PORT_SUPPRESSION_VOLUME, PORT_VISIBLE, PORT_TIME_LAST_SEEN };
		return s_aVarisIn;
	}
	override TStringArray GetVariablesIn() { return GetVarisIn(); }
}