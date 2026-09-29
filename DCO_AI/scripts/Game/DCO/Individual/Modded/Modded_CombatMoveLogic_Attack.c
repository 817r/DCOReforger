modded enum SCR_EAICombatMoveReason
{
	INVESTIGATE,
	SUPPLYING
}

modded enum SCR_EAICombatMoveRequestType
{
	INVESTIGATE,
	RESUPPLYING,
	BUILDING,
	INDOOR_RELOCATE
}

modded class SCR_AICombatMoveLogic_Attack : SCR_AICombatMoveLogicBase
{
	protected static const string PORT_BASE_TARGET = "BaseTarget";

	protected BaseTarget m_Target;
	protected vector m_vAvoidStraightPathDir;
	protected DCO_AIMoraleSystem moraleSystem;







	protected static ref RandomGenerator s_CohesionRand = new RandomGenerator();

	protected float m_fNextEngagedBid_ms = -1;
	protected float m_fNextCriticalBid_ms = -1;

	protected float m_fCriticalEnterTime_ms = -1;

	protected bool m_bInOpenAreaCached = false;

	protected override bool OnUpdate(AIAgent owner, float dt)
	{
		GetVariableIn(PORT_BASE_TARGET, m_Target);
		GetVariableIn("AvoidStraightPathDir", m_vAvoidStraightPathDir);

		if (!m_Target || !m_Target.GetTargetEntity())
			return false;

		moraleSystem = m_Utility.GetMoraleSystem();
		return true;
	}

	protected override float GetTargetDistance()
	{
		return m_Target.GetDistance();
	}

	protected override vector GetTargetPosition()
	{
		return m_Target.GetLastSeenPosition();
	}

	protected override vector GetAvoidStraightPathDir()
	{
		return m_vAvoidStraightPathDir;
	}

	protected const float INVESTIGATE_MIN_TIME = 8.0;
	protected const float INVESTIGATE_MAX_TIME = 30.0;
	protected const float INVESTIGATE_MAX_DIST = 60.0;

	protected bool IsInvestigating()
	{
		if (!m_Target)
			return false;

		if (m_CombatComp.IsTargetVisible(m_Target))
			return false;

		float tss = m_Target.GetTimeSinceSeen();
		if (tss < INVESTIGATE_MIN_TIME || tss > INVESTIGATE_MAX_TIME)
			return false;

		return m_Target.GetDistance() < INVESTIGATE_MAX_DIST;
	}

	protected bool CriticalBoundCooldownReady()
	{
	    return GetGame().GetWorld().GetWorldTime() >= m_fNextCriticalBid_ms;
	}

	protected void UpdateCriticalTimer(float now_ms)
	{
		if (!IsCriticalCombatMoment())
		{
			m_fCriticalEnterTime_ms = -1;
			return;
		}

		if (m_fCriticalEnterTime_ms < 0)
			m_fCriticalEnterTime_ms = now_ms;
	}

	protected float GetCriticalElapsed_s(float now_ms)
	{
		if (m_fCriticalEnterTime_ms < 0)
			return 0;

		return (now_ms - m_fCriticalEnterTime_ms) / 1000.0;
	}

	protected bool ShouldForceReturnFire(float now_ms)
	{
		float threshold_s = 5.0 * DCO_PersonalityCombatUtility.GetReturnFireDelayScale(m_Utility);

		return GetCriticalElapsed_s(now_ms) > threshold_s;
	}

	protected bool ShouldRegroupToLeader(out vector outLeaderPos)
	{
		outLeaderPos = vector.Zero;

		AIAgent agent = m_Utility.GetAIAgent();
		if (!agent)
			return false;

		AIGroup group = agent.GetParentGroup();
		if (!group || agent == group.GetLeaderAgent())
			return false;

		IEntity leader = group.GetLeaderEntity();
		if (!leader)
			return false;

		outLeaderPos = leader.GetOrigin();

		float maxDist = 50.0;

		DCO_GroupConfigComponent cfg = DCO_GroupConfigComponent.Cast(group.FindComponent(DCO_GroupConfigComponent));
		if (cfg)
			maxDist = cfg.GetCohesionDistance();

		return vector.Distance(outLeaderPos, m_MyEntity.GetOrigin()) > maxDist;
	}

	protected void PushRequestCriticalBound()
	{
	    SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

	    rq.m_eReason    = SCR_EAICombatMoveReason.STANDARD;
	    rq.m_vTargetPos = ResolveRequestTargetPos();
	    rq.m_vMovePos   = rq.m_vTargetPos;

	    rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
	    rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;

	    rq.m_eStanceMoving = ECharacterStance.STAND;
	    rq.m_eStanceEnd    = ECharacterStance.CROUCH;
	    rq.m_eMovementType = EMovementType.SPRINT;

	    rq.m_bAimAtTarget    = false;
	    rq.m_bAimAtTargetEnd = true;

	    rq.m_bUseCoverSearchDirectivity = true;
	    rq.m_bCheckCoverVisibility      = true;
	    rq.m_bFailIfNoCover             = false;

	    bool buildingExhausted = m_State.GetOldRequest()
	        && m_State.GetOldRequest().m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND;

	    float searchDistMax;
	    if (buildingExhausted)
	    {
	        rq.m_eType         = SCR_EAICombatMoveRequestType.MOVE;
	        rq.m_bTryFindCover = true;
	        searchDistMax      = 25.0;
	    }
	    else
	    {
	        rq.m_eType         = SCR_EAICombatMoveRequestType.BUILDING;
	        rq.m_bTryFindCover = false;
	        searchDistMax      = 60.0;
	    }

	    rq.m_fCoverSearchDistMin = 0;
	    rq.m_fCoverSearchDistMax = searchDistMax;
	    rq.m_fMoveDuration_s     = searchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;

	    rq.GetOnMovementStarted().Insert(OnMovementStarted);
	    rq.GetOnCompleted().Insert(OnMovementCompleted);

	    m_fNextCriticalBid_ms = GetGame().GetWorld().GetWorldTime() + 3000.0;
	    m_State.ApplyNewRequest(rq);
	}

	protected override vector ResolveRequestTargetPos()
	{
		IEntity tgtEntity = m_Target.GetTargetEntity();
		if (tgtEntity && m_CombatComp.IsTargetVisible(m_Target))
		{
			ChimeraCharacter character = ChimeraCharacter.Cast(tgtEntity);
			if (character)
			{
				vector eyePos = character.EyePosition();
				return eyePos;
			}

			vector pos = tgtEntity.GetOrigin();
			pos = pos + Vector(0, 2.0, 0);
			return pos;
		}

		vector lastSeenPos = m_Target.GetLastSeenPosition();
		lastSeenPos = lastSeenPos + Vector(0, 1.8, 0);
		return lastSeenPos;
	}

	protected override bool ResolveFailMoveIfNoCover()
	{
		if (m_bCloseRangeCombat)
		{
			if (m_State && m_State.IsMovingToBuilding())
				return true;
			else
				return false;
		}
		else
		{
			if (IsFirstExecution())
				return true;
			else
				return m_State.m_bInCover;
		}
	}

	protected override float ResolveStoppedWaitTime(bool inCover, EAIThreatState threat, EWeaponType weaponType)
	{
		if (m_bCloseRangeCombat)
		{
			return Math.RandomFloat(1.0, 4.5);
		}

		if (IsInvestigating())
		{
			float investigateWait = Math.RandomFloat(4.0, 8.0) * DCO_PersonalityCombatUtility.GetInvestigateEagernessScale(m_Utility);
			return Math.Max(investigateWait, 2.0);
		}

		float waitTime;

		if (inCover)
		{
			switch (threat)
			{
				case EAIThreatState.THREATENED:
					waitTime = Math.RandomFloat(60, 90);
					break;
				default:
					waitTime = Math.RandomFloat(30, 45);
			}
		}
		else
		{
			switch (threat)
			{
				case EAIThreatState.THREATENED:
					waitTime = Math.RandomFloat(2, 4);
					break;
				default:
					waitTime = Math.RandomFloat(6, 9);
					break;
			}
		}

		bool longWaitTime = false;
		switch (weaponType)
		{
			case EWeaponType.WT_MACHINEGUN:
			case EWeaponType.WT_ROCKETLAUNCHER:
			case EWeaponType.WT_GRENADELAUNCHER:
			case EWeaponType.WT_SNIPERRIFLE:
				longWaitTime = true;
		}

		if (m_bVeryLongRangeCombat)
			waitTime *= 1.5;
		else
			waitTime *= Math.RandomFloat(0.8, 1);

		if (longWaitTime)
			waitTime *= 2;

		float mult = Math.Map(moraleSystem.GetMoraleMeasure(), 0, 4.5, 1, 2.5);
		waitTime += mult;

		waitTime *= DCO_PersonalityCombatUtility.GetStoppedWaitTimeScale(m_Utility);

		waitTime *= DCO_PostureCombatUtility.GetStoppedWaitTimeScale(DCO_PostureCombatUtility.GetPosture(m_Utility));

		return waitTime;
	}

	override protected bool MoveFromTargetCondition()
	{
		if (!m_State.m_bInCover)
		{
			float postureDist = DCO_PostureCombatUtility.GetMoveFromTargetDist(DCO_PostureCombatUtility.GetPosture(m_Utility));
			if (m_fTargetDist < postureDist)
				return true;
		}

		return super.MoveFromTargetCondition();
	}


	protected SCR_AIGroupUtilityComponent DCO_GetGroupUtility()
	{
		AIAgent agent = m_Utility.GetAIAgent();
		if (!agent)
			return null;

		SCR_AIGroup group = SCR_AIGroup.Cast(agent.GetParentGroup());
		if (!group)
			return null;
		return group.GetGroupUtilityComponent();
	}

	protected bool DCO_InOvermatch()
	{
		if (m_Utility.m_AIInfo && m_Utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
			return false;

		SCR_AIGroupUtilityComponent groupUtil = DCO_GetGroupUtility();
		return groupUtil && groupUtil.DCO_IsOvermatch();
	}

	protected bool DCO_OvermatchMoveCondition()
	{
		if (m_State.IsExecutingRequest())
			return false;

		if (m_Utility.m_ThreatSystem.GetSuppressionMeasure() > 0.85)
			return false;

		if (m_fTargetDist > 30.0)
		{
			SCR_AIGroupUtilityComponent groupUtil = DCO_GetGroupUtility();
			if (groupUtil && groupUtil.m_FireteamMgr)
			{
				bool bounding;
				bool myTurn = groupUtil.m_FireteamMgr.DCO_IsBoundingTurn(m_Utility.GetAIAgent(), bounding);
				if (bounding && !myTurn)
					return false;
			}
		}

		return m_State.m_fTimerStopped_s > 2.5 * DCO_PersonalityCombatUtility.GetStoppedWaitTimeScale(m_Utility);
	}

	protected bool DCO_OvermatchBoundPos(vector targetPos, out vector outPos)
	{
		vector myPos = m_MyEntity.GetOrigin();
		vector toTarget = targetPos - myPos;
		toTarget[1] = 0;
		float dist = toTarget.Length();
		if (dist < 6.0 + 1)
			return false;

		vector fwd = toTarget / dist;
		vector dir = fwd;
		float advance;
		if (dist <= 30.0)
		{
			advance = Math.Min(dist - 6.0, 15.0);
		}
		else
		{
			advance = Math.Min(dist - 15.0, Math.RandomFloat(15.0, 30.0));
			float side = 1.0;
			if (Math.RandomIntInclusive(0, 1) == 1)
				side = -1.0;
			float angle = Math.RandomFloat(15.0, 40.0) * Math.DEG2RAD;
			vector right = Vector(fwd[2], 0, -fwd[0]);
			dir = (fwd * Math.Cos(angle)) + (right * Math.Sin(angle) * side);
		}

		if (advance < 3)
			return false;

		vector candidate = myPos + dir * advance;
		candidate[1] = GetGame().GetWorld().GetSurfaceY(candidate[0], candidate[2]);

		NavmeshWorldComponent navmesh = GetGame().GetAIWorld().GetNavmeshWorldComponent("Soldiers");
		if (navmesh && navmesh.IsTileLoaded(candidate) && navmesh.GetReachablePoint(candidate, 3.0, outPos))
			return true;

		outPos = candidate;
		return true;
	}

	protected override bool MoveToNextPosCondition()
	{
		if (m_Utility && m_Utility.m_DCOConfig && m_Utility.m_DCOConfig.IsHoldPosition())
			return false;

		if (DCO_InOvermatch())
			return DCO_OvermatchMoveCondition();

		if (IsCriticalCombatMoment() && !DCO_CanPushUnderFire())
			return false;

		float lockTime_s = 5.0 * DCO_PersonalityCombatUtility.GetRepositionLockScale(m_Utility)
			* DCO_PostureCombatUtility.GetRepositionLockScale(DCO_PostureCombatUtility.GetPosture(m_Utility));

		float optimalDist = Math.Max(ResolveOptimalDistance(m_fWeaponMinDist) * DCO_PersonalityCombatUtility.GetCloseInDistanceScale(m_Utility), m_fWeaponMinDist);

		if (m_fTargetDist < optimalDist && DCO_CQC.IsTargetInMyBuilding(m_Utility))
			optimalDist = Math.Max(optimalDist * 0.5, m_fWeaponMinDist);
		if (m_fTargetDist < optimalDist && m_Target.GetTimeSinceSeen() < lockTime_s)
			return false;

		if (m_State.IsExecutingRequest())
			return false;

		if (IsFirstExecution() && !m_State.m_bInCover)
			return true;

		if (m_Utility.ShouldKeepFormation() || m_CombatComp.GetCombatMode() == EAIGroupCombatMode.HOLD_FIRE)
		{
			if (!IsFirstExecution())
				return false;
		}
		float inClosedAreaMultiplier = 1;
		if (m_bInOpenAreaCached)
			inClosedAreaMultiplier = 2;

		float stoppedWaitTime = ResolveStoppedWaitTime(m_State.m_bInCover, m_eThreatState, m_eWeaponType) * inClosedAreaMultiplier;
		return m_State.m_fTimerStopped_s > stoppedWaitTime;
	}

	override protected void ResolveMoveRequestMovePosAndDir(vector targetPos, out vector outMovePos, out vector outAvoidStraightPathDir, out SCR_EAICombatMoveDirection outDirection, out float outCoverSearchSectorHalfAngleRad)
	{
		vector boundPos;
		if (DCO_InOvermatch() && DCO_OvermatchBoundPos(targetPos, boundPos))
		{
			outMovePos = boundPos;
			outDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
			outAvoidStraightPathDir = GetAvoidStraightPathDir();
			outCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
			return;
		}

		AIWaypoint wp = null;
		AIAgent agent = m_Utility.GetAIAgent();
		AIGroup group = agent.GetParentGroup();
		if (group)
			wp = group.GetCurrentWaypoint();

		vector movePos;
		SCR_EAICombatMoveDirection eDirection;
		float coverSearchSectorHalfAngleRad;
		vector avoidStraightPathDir;

		if (!wp || SCR_EntityWaypoint.Cast(wp))
		{
			vector leaderPos;

			if (ShouldRegroupToLeader(leaderPos))
			{
				vector mp = s_CohesionRand.GenerateRandomPointInRadius(0, 25.0, leaderPos, false);
				mp[1] = GetGame().GetWorld().GetSurfaceY(mp[0], mp[2]);
				movePos = mp;
				eDirection = SCR_EAICombatMoveDirection.FORWARD;
				coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
				avoidStraightPathDir = vector.Zero;
			}
			else
			{
				movePos = targetPos;
				MoraleAndThreatPushMove(eDirection);
				DCO_PostureCombatUtility.ApplyMoveDirection(DCO_PostureCombatUtility.GetPosture(m_Utility), m_eThreatState, moraleSystem.GetState(), eDirection);
				avoidStraightPathDir = GetAvoidStraightPathDir();
				coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
			}
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
				eDirection = SCR_EAICombatMoveDirection.FORWARD;
				coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
				avoidStraightPathDir = vector.Zero;
			}
			else if (myDistToWp > 0.5 * wpRadius)
			{
				if (tgtInWaypoint)
				{
					movePos = targetPos;
					eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
					avoidStraightPathDir = GetAvoidStraightPathDir();
					coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
				}
				else
				{
					movePos = targetPos;
					eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
					avoidStraightPathDir = vector.Zero;
					coverSearchSectorHalfAngleRad = -1.0;
				}
			}
			else
			{
				movePos = targetPos;
				eDirection = SCR_EAICombatMoveDirection.FORWARD;
				avoidStraightPathDir = GetAvoidStraightPathDir();

				coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
			}
		}

		outMovePos = movePos;
		outDirection = eDirection;
		outAvoidStraightPathDir = avoidStraightPathDir;
		outCoverSearchSectorHalfAngleRad = coverSearchSectorHalfAngleRad;
	}

	override protected void PushRequestMove()
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;

		rq.m_vTargetPos = ResolveRequestTargetPos();
		ResolveMoveRequestMovePosAndDir(rq.m_vTargetPos, rq.m_vMovePos, rq.m_vAvoidStraightPathDir, rq.m_eDirection, rq.m_fCoverSearchSectorHalfAngleRad);
		rq.m_bTryFindCover = true;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility = true;

		bool isExposed = DCO_ConcealmentUtility.IsPositionExposed(m_MyEntity.GetOrigin(), m_MyEntity);
		if (isExposed)
			rq.m_bUseCoverSearchDirectivity = false;

		float coverSearchDistMin = 5;
		float coverSearchDistMax = 30;
		float moveDurationMax = 10;
		if (m_bCloseRangeCombat)
		{
			switch (m_eThreatState)
			{
				case EAIThreatState.THREATENED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.PRONE;
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 10.0;
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 10.0;
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN;
					rq.m_eMovementType = EMovementType.WALK;
					break;
				}
				default:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 15.0;
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_RUN;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
			}

			rq.m_bAimAtTarget = DCO_MoraleCombatUtility.CanAimWhileMoving(
				DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection) &&
				IsAimingAndMovingAllowedForWeapon(m_eWeaponType),
				moraleSystem);
			rq.m_bAimAtTargetEnd = true;
		}
		else
		{
			switch (m_eThreatState)
			{
				case EAIThreatState.THREATENED:
				{
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 20.0;
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.PRONE;
					rq.m_eMovementType = EMovementType.SPRINT;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 10.0;
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
				default:
				{
					coverSearchDistMin = 10.0;
					coverSearchDistMax = 30.0;
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_RUN;
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
			}

			rq.m_bAimAtTarget = DCO_MoraleCombatUtility.CanAimWhileMoving(
				DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection) &&
				IsAimingAndMovingAllowedForWeapon(m_eWeaponType),
				moraleSystem);
			rq.m_bAimAtTargetEnd = true;
		}

		if (IsInvestigating())
		{
			rq.m_eStanceMoving = ECharacterStance.STAND;
			rq.m_eStanceEnd = ECharacterStance.CROUCH;
			rq.m_eMovementType = EMovementType.WALK;
			rq.m_bAimAtTarget = DCO_MoraleCombatUtility.CanAimWhileMoving(true, moraleSystem);
			rq.m_bAimAtTargetEnd = true;
		}

		DCO_PostureCombatUtility.ApplyMovePace(DCO_PostureCombatUtility.GetPosture(m_Utility), m_eThreatState, rq, !m_bCloseRangeCombat);

		if (m_State.GetOldRequest() && m_State.GetOldRequest().m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
			rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		else
		{
			rq.m_eType = SCR_EAICombatMoveRequestType.BUILDING;
			rq.m_bTryFindCover = false;
			DCO_BreachUtility.TryThrowBreachGrenade(m_Utility, rq.m_vTargetPos);
		}

		rq.m_bFailIfNoCover = ResolveFailMoveIfNoCover();

		if (DCO_InOvermatch())
		{
			if (m_fTargetDist > 40.0)
			{
				rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
				rq.m_bTryFindCover = true;
			}
			rq.m_bFailIfNoCover = false;
			rq.m_eStanceMoving = ECharacterStance.CROUCH;
			rq.m_eStanceEnd = ECharacterStance.CROUCH;
			rq.m_eMovementType = EMovementType.RUN;
			coverSearchDistMin = 0;
			coverSearchDistMax = 10;
		}

		if (!m_State.m_bInCover)
			coverSearchDistMin = 3;

		coverSearchDistMax *= DCO_MoraleCombatUtility.GetCoverSearchDistScale(moraleSystem, m_Utility);
		if (isExposed)
			coverSearchDistMax *= 1.5;

		rq.m_fCoverSearchDistMin = coverSearchDistMin;
		rq.m_fCoverSearchDistMax = coverSearchDistMax;
		rq.m_fMoveDuration_s = moveDurationMax * MoraleAmplifyMove();

		if (m_State && m_State.IsMovingToBuilding())
		{
			coverSearchDistMin = 0;
			coverSearchDistMax = 5;
		}

		rq.GetOnMovementStarted().Insert(OnMovementStarted);
		rq.GetOnCompleted().Insert(OnMovementCompleted);

		m_State.ApplyNewRequest(rq);
	}

	void GetMoraleEffects(out SCR_EAICombatMoveDirection moveDir)
	{
		switch(moraleSystem.GetState())
		{
			case moraleState.BREAK:
			{
				moveDir = SCR_EAICombatMoveDirection.BACKWARD;
				break;
			}
			case moraleState.MANIAC:
			{
				if(Math.RandomFloat(0,5) > 4)
				{
					if(Math.RandomFloat(0,5) > 4)
					{
						if(Math.RandomFloat(0,1) > 1)
							moveDir = SCR_EAICombatMoveDirection.LEFT;
						else
							moveDir = SCR_EAICombatMoveDirection.RIGHT;
					}
					else
						moveDir = SCR_EAICombatMoveDirection.BACKWARD;
				}
				else
					moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
				break;
			}
			case moraleState.ANXIOUS:
			{
				if(Math.RandomFloat(0,1) > 1)
				{
					if(Math.RandomFloat(0,1) > 1)
						moveDir = SCR_EAICombatMoveDirection.LEFT;
					else
						moveDir = SCR_EAICombatMoveDirection.RIGHT;
				}
				else
					moveDir = SCR_EAICombatMoveDirection.BACKWARD;
				break;
			}
			default:
			{
				moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
				break;
			}
		}
	}

	void MoraleAndThreatPushMove(out SCR_EAICombatMoveDirection moveDir)
	{
		switch (m_eThreatState)
		{
			case EAIThreatState.THREATENED:
			{
				switch (moraleSystem.GetState())
				{
					case moraleState.BREAK:
					{
						moveDir = SCR_EAICombatMoveDirection.BACKWARD;
						break;
					}
					case moraleState.MANIAC:
					{
						if (Math.RandomIntInclusive(0,1) == 1)
							moveDir = SCR_EAICombatMoveDirection.LEFT;
						else
							moveDir = SCR_EAICombatMoveDirection.RIGHT;
						break;
					}
					case moraleState.ANXIOUS:
					{
						if (Math.RandomIntInclusive(0,1) == 1)
							moveDir = SCR_EAICombatMoveDirection.BACKWARD;
						else
							moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;
					}
					case moraleState.NORMAL:
					{
						if (Math.RandomIntInclusive(0,1) == 1)
							moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						else
							moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
						break;
					}
					case moraleState.MOTIVATED:
					{
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;
					}
					default:
					{
						moveDir = SCR_EAICombatMoveDirection.BACKWARD;
						break;
					}
				}
				break;
			}
			case EAIThreatState.ALERTED:
			{
				switch (moraleSystem.GetState())
				{
					case moraleState.BREAK:
					{
						if (Math.RandomIntInclusive(0,1) == 1)
							moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
						else
							moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;
					}
					case moraleState.MANIAC:
					{
						if (Math.RandomIntInclusive(0,1) == 1)
							moveDir = SCR_EAICombatMoveDirection.LEFT;
						else
							moveDir = SCR_EAICombatMoveDirection.RIGHT;
						break;
					}
					case moraleState.ANXIOUS:
					{
						moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
						break;
					}
					case moraleState.NORMAL:
					{
						moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
						break;
					}
					case moraleState.MOTIVATED:
					{
						moveDir = SCR_EAICombatMoveDirection.FORWARD;
						break;
					}
					default:
					{
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;
					}
				}
				break;
			}
			case EAIThreatState.VIGILANT:
			{
				switch (moraleSystem.GetState())
				{
					case moraleState.BREAK:
					{
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;
					}
					case moraleState.MANIAC:
					{
						moveDir = SCR_EAICombatMoveDirection.FORWARD;
						break;
					}
					case moraleState.ANXIOUS:
					{
						moveDir = SCR_EAICombatMoveDirection.FORWARD;
						break;
					}
					case moraleState.NORMAL:
					{
						moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
						break;
					}
					case moraleState.MOTIVATED:
					{
						moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
						break;
					}
					default:
					{
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;
					}
				}
				break;
			}
			case EAIThreatState.SAFE:
			{
				switch (moraleSystem.GetState())
				{
					case moraleState.BREAK:
					{
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;
					}
					case moraleState.MANIAC:
					{
						moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
						break;
					}
					case moraleState.ANXIOUS:
					{
						moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
						break;
					}
					case moraleState.NORMAL:
					{
						moveDir = SCR_EAICombatMoveDirection.FORWARD;
						break;
					}
					case moraleState.MOTIVATED:
					{
						moveDir = SCR_EAICombatMoveDirection.FORWARD;
						break;
					}
					default:
					{
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;
					}
				}
				break;
			}
			default:
			{
				break;
			}
		}
	}

	override protected void SuppressedInCoverLogic()
	{
		float waitTime_s;
		SCR_AICombatMoveRequestBase rq = m_State.GetRequest();
		if (SCR_AICombatMoveRequest_ChangeStanceInCover.Cast(rq) && rq.m_eReason == SCR_EAICombatMoveReason.SUPPRESSED_IN_COVER)
			waitTime_s = rq.m_f_UserTimer_s;
		else
		{
			if (m_State.m_bExposedInCover)
				waitTime_s = 1.5;
			else
				waitTime_s = 5.0;
		}

		if (m_State.m_bExposedInCover && m_State.m_fTimerRequest_s > waitTime_s)
		{
			float newWaitTime = Math.RandomFloat(5, 9.0);
			PushRequestChangeStanceInCover(false, SCR_EAICombatMoveReason.SUPPRESSED_IN_COVER, newWaitTime);
		}
		else if (!m_State.m_bExposedInCover && m_State.m_fTimerRequest_s > waitTime_s)
		{
			float newWaitTime = Math.RandomFloat(1.5, 2.5);
			PushRequestChangeStanceInCover(true, SCR_EAICombatMoveReason.SUPPRESSED_IN_COVER, newWaitTime);
		}
	}

	protected override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		float currentTime_ms = GetGame().GetWorld().GetWorldTime();
		if (currentTime_ms < m_fNextUpdate_ms)
			return ENodeResult.RUNNING;
		m_fNextUpdate_ms = currentTime_ms + m_fUpdateInterval_ms;

		if (!OnUpdate(owner, dt))
			return ENodeResult.FAIL;

		if (!m_State || !m_MyEntity || !m_Utility || !m_CombatComp || !m_CharacterController)
			return ENodeResult.FAIL;

		if (m_Utility.m_DCOConfig && m_Utility.m_DCOConfig.IsHoldPosition())
			return ENodeResult.RUNNING;

		SCR_AIBehaviorBase executedBehavior = SCR_AIBehaviorBase.Cast(m_Utility.GetExecutedAction());
		if (executedBehavior && !executedBehavior.m_bUseCombatMove)
			return ENodeResult.RUNNING;

		m_fTargetDist = GetTargetDistance();
		m_bCloseRangeCombat = m_fTargetDist < SCR_AICombatMoveUtils.CLOSE_RANGE_COMBAT_DIST;
		m_bVeryLongRangeCombat = m_fTargetDist > SCR_AICombatMoveUtils.VERY_LONG_RANGE_COMBAT_DIST;
		m_eThreatState = m_Utility.m_ThreatSystem.GetState();
		m_eStance = m_CharacterController.GetStance();
		m_fWeaponMinDist = m_CombatComp.GetSelectedWeaponMinDist();
		m_eWeaponType = m_CombatComp.GetSelectedWeaponType();

		m_bInOpenAreaCached = IsInOpenArea(m_MyEntity);
		UpdateCriticalTimer(currentTime_ms);

		if (SuppressedInCoverCondition())
		{
			SuppressedInCoverLogic();
		}
		else if (MoveFromTargetCondition())
		{
			if (MoveFromTargetNewRequestCondition())
				PushRequestMoveFromTarget();
		}
		else if (CurrentCoverUselessCondition())
		{
			PushRequestLeaveUselessCover();
		}
		else if (m_CharacterController.IsReloading())
		{
			if (m_State.m_bInCover)
			{
				if (m_State.m_bExposedInCover)
					m_State.ApplyRequestChangeStanceInCover(false);
			} else
			{
				if (m_CharacterController.GetStance() ==  ECharacterStance.STAND)
					m_CharacterController.SetStanceChange(2);
			}
		}
		else if (m_State.m_bInCover && !m_State.m_bExposedInCover)
		{
			m_State.ApplyRequestChangeStanceInCover(true);
		}
		else if (FFAvoidanceCondition())
		{
			if (FFAvoidanceNewRequestCondition())
				PushRequestFFAvoidance();
		}
		else if (MoveToNextPosCondition())
		{
			PushRequestMove();
		}
		else if (!m_State.IsExecutingRequest() && !m_State.m_bInCover)
		{
			if (IsCriticalCombatMoment())
			{
				if (ShouldForceReturnFire(currentTime_ms))
				{
					if (m_eStance == ECharacterStance.PRONE)
						m_CharacterController.SetStanceChange(2);
				}
				else if (CriticalBoundCooldownReady())
				{
					PushRequestCriticalBound();
				}
				else
				{
					ECharacterStance criticalStance = ResolveStanceOutsideCover(m_bCloseRangeCombat, m_eThreatState);
					criticalStance = DCO_MoraleCombatUtility.ApplyMoraleStanceOverride(criticalStance, moraleSystem);
					if (criticalStance > m_eStance)
						m_State.ApplyRequestChangeStanceOutsideCover(criticalStance);
				}
			}
			else if (currentTime_ms >= m_fNextEngagedBid_ms)
			{
				float takeCoverChance = 0.7;
				if (m_Utility && m_Utility.m_DCOConfig)
					takeCoverChance = m_Utility.m_DCOConfig.GetTakeCoverChance();

				takeCoverChance *= DCO_PersonalityCombatUtility.GetTakeCoverChanceScale(m_Utility);

				if (m_bInOpenAreaCached)
					takeCoverChance *= 1.35;

				takeCoverChance = Math.Clamp(takeCoverChance, 0.0, 1.0);

				if (Math.RandomFloat01() < takeCoverChance)
				{
					float optimalDist = ResolveOptimalDistance(m_fWeaponMinDist);
					bool contactFresh = m_Target && m_Target.GetTimeSinceSeen() < 5.0;
					bool inEngagementRange = m_Target && m_fTargetDist <= optimalDist * 1.2;

					if ((contactFresh && inEngagementRange) || DCO_InOvermatch())
						PushRequestEngagedBound();
					else
						PushRequestOpenArea();
				}
				else
				{
					float bidCooldown_ms = 3000.0;

					if (m_bInOpenAreaCached)
						bidCooldown_ms *= 0.6;

					m_fNextEngagedBid_ms = currentTime_ms + bidCooldown_ms;
				}
			}
		} else if (!m_State.IsExecutingRequest())
		{
			if (m_Utility.GetCharacterController().GetWeaponObstructedState() != EWeaponObstructedState.UNOBSTRUCTED)
			{
				if (m_CharacterController.GetStance() == ECharacterStance.CROUCH)
					m_CharacterController.SetStanceChange(1);
				else if (m_CharacterController.GetStance() == ECharacterStance.PRONE)
					m_CharacterController.SetStanceChange(2);
			}
		}

		return ENodeResult.RUNNING;
	}

	protected void PushRequestEngagedBound()
	{
	    SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

	    rq.m_eReason    = SCR_EAICombatMoveReason.STANDARD;
	    rq.m_vTargetPos = ResolveRequestTargetPos();
	    rq.m_vMovePos   = rq.m_vTargetPos;

	    if (Math.RandomIntInclusive(0, 1) == 0)
	        rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
	    else
	        rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;

	    rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;

	    rq.m_eStanceMoving = ECharacterStance.CROUCH;
	    rq.m_eStanceEnd    = ECharacterStance.CROUCH;
	    rq.m_eMovementType = EMovementType.RUN;

	    rq.m_bAimAtTarget = DCO_MoraleCombatUtility.CanAimWhileMoving(
	        DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection) &&
	        IsAimingAndMovingAllowedForWeapon(m_eWeaponType),
	        moraleSystem);
	    rq.m_bAimAtTargetEnd = true;

	    rq.m_eType         = SCR_EAICombatMoveRequestType.MOVE;
	    rq.m_bTryFindCover = true;
	    rq.m_bUseCoverSearchDirectivity = true;
	    rq.m_bCheckCoverVisibility      = true;
	    rq.m_bFailIfNoCover             = false;

	    rq.m_fCoverSearchDistMin = 0;
	    rq.m_fCoverSearchDistMax = 18.0;
	    rq.m_fMoveDuration_s     = (18.0 / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN) * MoraleAmplifyMove();

	    rq.GetOnMovementStarted().Insert(OnMovementStarted);
	    rq.GetOnCompleted().Insert(OnMovementCompleted);

	    m_fNextEngagedBid_ms = GetGame().GetWorld().GetWorldTime() + 3000.0;
	    m_State.ApplyNewRequest(rq);
	}

	override protected bool SuppressedInCoverCondition()
	{
		if (m_bInOpenAreaCached && m_eThreatState == EAIThreatState.THREATENED && moraleSystem.GetState() >= moraleState.MANIAC)
			return true;

		return m_State.m_bInCover && m_eThreatState == EAIThreatState.THREATENED && moraleSystem.GetState() >= moraleState.MANIAC;
	}

	protected bool IsCriticalCombatMoment()
	{
	    return m_eThreatState == EAIThreatState.THREATENED;
	}

	protected bool DCO_CanPushUnderFire()
	{
		DCO_EAIPersonality personality = DCO_PersonalityCombatUtility.GetPersonalitySafe(m_Utility);
		if (personality == DCO_EAIPersonality.RECKLESS)
			return true;

		if (personality == DCO_EAIPersonality.AGGRESSIVE || DCO_PostureCombatUtility.GetPosture(m_Utility) == DCO_GroupTactics.AGGRESIVE)
			return ShouldForceReturnFire(GetGame().GetWorld().GetWorldTime());

		return false;
	}

	protected bool IsEngagedCombatMoment()
	{
	    return m_eThreatState >= EAIThreatState.ALERTED;
	}

	float MoraleAmplifyMove()
	{
		return Math.Map(moraleSystem.GetMoraleMeasure(), 0, 4.5, 2, 1);
	}

	protected void PushRequestOpenArea()
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;

		rq.m_vTargetPos = ResolveRequestTargetPos();
		rq.m_vMovePos = rq.m_vTargetPos;
		rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
		rq.m_fCoverSearchSectorHalfAngleRad;
		rq.m_bTryFindCover = true;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility = true;

		float coverSearchDistMin = 5;
		float coverSearchDistMax = 50;
		float moveDurationMax = 10;
		if (m_bCloseRangeCombat)
		{
			switch (m_eThreatState)
			{
				case EAIThreatState.THREATENED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN;
					rq.m_eMovementType = EMovementType.WALK;
					break;
				}
				default:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_RUN;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
			}

			rq.m_bAimAtTarget = DCO_MoraleCombatUtility.CanAimWhileMoving(
				DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection) &&
				IsAimingAndMovingAllowedForWeapon(m_eWeaponType),
				moraleSystem);
			rq.m_bAimAtTargetEnd = true;
		}
		else
		{
			switch (m_eThreatState)
			{
				case EAIThreatState.THREATENED:
				{
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.PRONE;
					rq.m_eMovementType = EMovementType.SPRINT;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
				default:
				{
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_RUN;
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
			}

			rq.m_bAimAtTarget = DCO_MoraleCombatUtility.CanAimWhileMoving(
				DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection) &&
				IsAimingAndMovingAllowedForWeapon(m_eWeaponType),
				moraleSystem);
			rq.m_bAimAtTargetEnd = true;
		}

		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;

		rq.m_bFailIfNoCover = false;
		rq.m_fCoverSearchDistMin = coverSearchDistMin;
		rq.m_fCoverSearchDistMax = coverSearchDistMax;
		rq.m_fMoveDuration_s = moveDurationMax * MoraleAmplifyMove();

		rq.GetOnMovementStarted().Insert(OnMovementStarted);
		rq.GetOnCompleted().Insert(OnMovementCompleted);

		m_State.ApplyNewRequest(rq);
	}

	override protected void PushRequestFFAvoidance()
	{
		if (m_CharacterController.GetStance() == ECharacterStance.PRONE)
		{
			if (Math.RandomFloat01() > 0.5)
				m_CharacterController.SetRoll(1);
			else
				m_CharacterController.SetRoll(2);

			return;
		}
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

		rq.m_eReason = SCR_EAICombatMoveReason.FF_AVOIDANCE;

		SCR_AICombatMoveRequest_Move prevRequest = SCR_AICombatMoveRequest_Move.Cast(m_State.GetRequest());
		if (prevRequest && prevRequest.m_eReason == SCR_EAICombatMoveReason.FF_AVOIDANCE)
		{
			rq.m_eDirection = prevRequest.m_eDirection;
		}
		else
		{
			if (Math.RandomIntInclusive(0, 1) == 1)
				rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
			else
				rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
		}

		rq.m_eStanceMoving = m_CharacterController.GetStance();
		rq.m_eStanceEnd = rq.m_eStanceMoving;
		rq.m_vMovePos = ResolveRequestTargetPos();
		rq.m_eMovementType = EMovementType.WALK;
		rq.m_fMoveDuration_s = 1.0;
		rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
		rq.m_bAimAtTargetEnd = true;

		m_State.ApplyNewRequest(rq);
	}

	override protected void PushRequestLeaveUselessCover()
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;

		rq.m_vTargetPos = ResolveRequestTargetPos();
		rq.m_vMovePos = rq.m_vTargetPos;
		rq.m_bTryFindCover = true;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility = true;
		rq.m_bFailIfNoCover = false;
		rq.m_fCoverSearchDistMin = 0;
		rq.m_fCoverSearchDistMax = 30;
		if (m_CharacterController.GetStance() == ECharacterStance.PRONE)
			rq.m_eStanceMoving = ECharacterStance.CROUCH;
		else
			rq.m_eStanceMoving = m_CharacterController.GetStance();
		rq.m_eStanceEnd = ECharacterStance.CROUCH;
		rq.m_eMovementType = EMovementType.RUN;
		rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
		rq.m_bAimAtTarget = DCO_MoraleCombatUtility.CanAimWhileMoving(
			DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection) &&
			IsAimingAndMovingAllowedForWeapon(m_eWeaponType),
			moraleSystem);
		rq.m_bAimAtTargetEnd = true;
		if (m_CharacterController.GetStance() == ECharacterStance.STAND)
			rq.m_fMoveDuration_s = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_RUN;
		else
			rq.m_fMoveDuration_s = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN;

		m_State.ApplyNewRequest(rq);
	}

	protected static ref TStringArray s_aVarsIn = {
		PORT_BASE_TARGET,
		"AvoidStraightPathDir"
	};
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
}