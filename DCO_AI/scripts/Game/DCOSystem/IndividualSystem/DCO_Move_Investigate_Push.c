class SCR_AIDCO_AttackPush: AITaskScripted
{
	protected static const float COVER_SEARCH_DIST_MAX        = 20.0;
	protected static const float COVER_SEARCH_DIST_MIN        = 10.0;
	protected static const float COVER_QUERY_SECTOR_ANGLE_RAD = 0.25 * Math.PI;

	protected static const float COVER_SEARCH_DIST_MIN_BUILDING = 0.0;
	protected static const float COVER_SEARCH_DIST_MAX_BUILDING = 5.0;

	protected static const float BOUND_DIST_MAX      = 20.0;
	protected static const float BOUND_DIST_MIN      = 3.0;
	protected static const float STANDOFF_MIN        = 10.0;
	protected static const float FLANK_ANGLE_MIN_DEG = 15.0;
	protected static const float FLANK_ANGLE_MAX_DEG = 40.0;
	protected static const float CQB_BOUND_SCALE     = 0.35;
	protected static const float NAVMESH_SNAP_RADIUS = 2.0;

	protected static const float SUPPRESSION_CUTOFF_DEFAULT = 0.6;
	protected static const float SUPPRESSION_CUTOFF_CQB     = 0.85;
	protected static const float STANCE_SUPPRESSION_CROUCH  = 0.8;
	protected static const float WAIT_TIME_IN_COVER_S       = 8.0;
	protected static const float WAIT_TIME_OPEN_S           = 5.0;
	protected static const float WAIT_SCALE_CQB             = 1.2;
	protected static const float WAIT_SCALE_THREATENED      = 2;
	protected static const float MOVE_DURATION_MIN_S        = 1.5;

	protected static const string PORT_POSITION = "Position";

	//------------------------------------------------------------------------------------------------
	protected SCR_AICombatMoveState  m_State;
	protected SCR_AIUtilityComponent m_Utility;
	protected NavmeshWorldComponent  m_pNavmesh;

	protected float m_fNextUpdate_ms = -1;

	[Attribute("500", UIWidgets.EditBox, "Interval re-evaluasi push (ms)", category: "Timing")]
	protected float m_fUpdateInterval_ms;

	[Attribute("1", UIWidgets.CheckBox, "Izinin assault masuk building kalau target ada di dalem", category: "Building Assault")]
	protected bool m_bAllowBuildingAssault;

	[Attribute("40", UIWidgets.EditBox, "Jarak maksimum buat mulai coba masuk building (m)", category: "Building Assault")]
	protected float m_fBuildingAssaultDist;

	[Attribute("0.35", UIWidgets.Slider, "Peluang bound pake flank (CUSTOM_POS) instead of FORWARD", "0 1 0.05", category: "Bounding")]
	protected float m_fFlankChance;

	//------------------------------------------------------------------------------------------------
	override void OnInit(AIAgent owner)
	{
		// CHANGED: m_CharacterController dibuang -- di-fetch tapi gak pernah dipake sama sekali.
		m_Utility = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
		if (m_Utility)
			m_State = m_Utility.m_CombatMoveState;
	}

	//------------------------------------------------------------------------------------------------
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!m_Utility || !m_State || !m_Utility.m_ThreatSystem)
			return ENodeResult.FAIL;

		IEntity myEntity = owner.GetControlledEntity();
		if (!myEntity)
			return ENodeResult.FAIL;

		World world = GetGame().GetWorld();
		if (!world)
			return ENodeResult.FAIL;

		float currentTime_ms = world.GetWorldTime();

		if (m_fNextUpdate_ms < 0)
			m_fNextUpdate_ms = currentTime_ms + Math.RandomFloat(0, m_fUpdateInterval_ms);

		if (currentTime_ms < m_fNextUpdate_ms)
			return ENodeResult.RUNNING;
		m_fNextUpdate_ms = currentTime_ms + m_fUpdateInterval_ms;

		vector threatPos;
		GetVariableIn(PORT_POSITION, threatPos);
		if (threatPos == vector.Zero)
			return ENodeResult.FAIL;

		float distToThreat = vector.Distance(myEntity.GetOrigin(), threatPos);
		bool isCQB = distToThreat < SCR_AICombatMoveUtils.CLOSE_RANGE_COMBAT_DIST;

		if (MoveToNextPosCondition(isCQB))
			CombatMoveLogic(owner, myEntity, threatPos, distToThreat, isCQB);

		return ENodeResult.RUNNING;
	}

	//------------------------------------------------------------------------------------------------
	void CombatMoveLogic(AIAgent owner, IEntity myEntity, vector threatPos, float distToThreat, bool isCQB)
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

		rq.m_eReason    = SCR_EAICombatMoveReason.INVESTIGATE;
		rq.m_vTargetPos = threatPos;

		bool assaultBuilding = false;
		if (m_bAllowBuildingAssault && distToThreat <= m_fBuildingAssaultDist && !LastBuildingAttemptFailed())
		{
			rq.m_eType      = SCR_EAICombatMoveRequestType.BUILDING;
			assaultBuilding = true;
		}
		else
		{
			rq.m_eType = SCR_EAICombatMoveRequestType.INVESTIGATE;
		}

		vector movePos      = threatPos;
		SCR_EAICombatMoveDirection direction = SCR_EAICombatMoveDirection.FORWARD;

		if (!assaultBuilding && Math.RandomFloat01() < m_fFlankChance)
		{
			vector flankPos;
			if (CalcBoundPosition(myEntity, threatPos, distToThreat, isCQB, flankPos))
			{
				movePos   = flankPos;
				direction = SCR_EAICombatMoveDirection.CUSTOM_POS;
			}
		}

		rq.m_vMovePos  = movePos;
		rq.m_eDirection = direction;

		rq.m_bTryFindCover              = true;
		rq.m_bCheckCoverVisibility      = true;
		rq.m_bFailIfNoCover             = false;
		rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;

		rq.m_bUseCoverSearchDirectivity = !DCO_ConcealmentUtility.IsPositionExposed(myEntity.GetOrigin(), myEntity);

		float coverMin = COVER_SEARCH_DIST_MIN;
		float coverMax = COVER_SEARCH_DIST_MAX;

		if (assaultBuilding)
		{
			coverMin = COVER_SEARCH_DIST_MIN_BUILDING;
			coverMax = COVER_SEARCH_DIST_MAX_BUILDING;
			rq.m_bTryFindCover = false;
		}
		else if (isCQB)
		{
			coverMin = 3.0;
			coverMax = 10.0;
		}

		if (!m_State.m_bInCover)
			coverMin = Math.Min(coverMin, 3.0);

		coverMax *= DCO_MoraleCombatUtility.GetCoverSearchDistScale(m_Utility.GetMoraleSystem(), m_Utility);

		rq.m_fCoverSearchDistMin = coverMin;
		rq.m_fCoverSearchDistMax = coverMax;

		ResolveMoveandStopStance(distToThreat, isCQB, assaultBuilding, rq.m_eStanceMoving, rq.m_eStanceEnd);

		rq.m_eMovementType = EMovementType.RUN;

		float speed = SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_RUN;
		if (rq.m_eStanceMoving == ECharacterStance.CROUCH)
			speed = SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN;

		float advanceDist = Math.Min(distToThreat, COVER_SEARCH_DIST_MAX);
		rq.m_fMoveDuration_s = Math.Max(advanceDist / speed, MOVE_DURATION_MIN_S);

		rq.m_bAimAtTarget = DCO_MoraleCombatUtility.CanAimWhileMoving(
			DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection),
			m_Utility.GetMoraleSystem());
		rq.m_bAimAtTargetEnd = true;

		vector dirToTgt = threatPos - myEntity.GetOrigin();
		rq.m_vAvoidStraightPathDir = dirToTgt;

		m_State.ApplyNewRequest(rq);

		if (assaultBuilding)
			DCO_BreachUtility.TryThrowBreachGrenade(m_Utility, threatPos);
	}

	protected bool CalcBoundPosition(IEntity myEntity, vector threatPos, float distToThreat, bool isCQB, out vector movePos)
	{
		vector myPos    = myEntity.GetOrigin();
		vector toThreat = threatPos - myPos;
		toThreat[1] = 0;

		float dist2D = toThreat.Length();
		if (dist2D < 1.0)
			return false;

		float scale = 1.0;
		if (isCQB)
			scale = CQB_BOUND_SCALE;

		float advance = Math.Min(dist2D - STANDOFF_MIN, BOUND_DIST_MAX * scale);
		if (advance < BOUND_DIST_MIN)
			return false;

		vector fwd   = toThreat / dist2D;
		vector right = Vector(fwd[2], 0, -fwd[0]);   // rotasi 90 deg di sumbu Y

		float side = 1.0;
		if (Math.RandomInt(0, 2) == 1)
			side = -1.0;

		float angleRad = Math.RandomFloat(FLANK_ANGLE_MIN_DEG, FLANK_ANGLE_MAX_DEG) * scale * Math.DEG2RAD;

		vector dirOff = (fwd * Math.Cos(angleRad)) + (right * Math.Sin(angleRad) * side);
		vector candidate = myPos + (dirOff * advance);
		candidate[1] = myPos[1];

		return SnapToNavmesh(candidate, movePos);
	}

	//------------------------------------------------------------------------------------------------
	protected bool SnapToNavmesh(vector pos, out vector outPos)
	{
		if (!m_pNavmesh)
			m_pNavmesh = GetGame().GetAIWorld().GetNavmeshWorldComponent("Soldiers");

		if (!m_pNavmesh)
			return false;

		if (!m_pNavmesh.IsTileLoaded(pos))
			return false;

		return m_pNavmesh.GetReachablePoint(pos, NAVMESH_SNAP_RADIUS, outPos);
	}

	//------------------------------------------------------------------------------------------------
	protected bool LastBuildingAttemptFailed()
	{
		SCR_AICombatMoveRequestBase oldRq = m_State.GetOldRequest();
		if (!oldRq)
			return false;

		return oldRq.m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND;
	}

	//------------------------------------------------------------------------------------------------
	protected DCO_EAIPersonality GetPersonality()
	{
		if (!m_Utility || !m_Utility.m_DCOConfig)
			return DCO_EAIPersonality.STANDARD;

		return m_Utility.m_DCOConfig.GetPersonality();
	}

	//------------------------------------------------------------------------------------------------
	protected void ResolveMoveandStopStance(float dist, bool isCQB, bool assaultBuilding, out ECharacterStance moving, out ECharacterStance end)
	{
		DCO_EAIPersonality personality = GetPersonality();

		// --- Stance gerak ---
		if (m_Utility.m_ThreatSystem.GetSuppressionMeasure() > STANCE_SUPPRESSION_CROUCH)
		{
			moving = ECharacterStance.CROUCH;
		}
		else if (personality == DCO_EAIPersonality.RECKLESS)
		{
			moving = ECharacterStance.STAND;   // RECKLESS hampir gak peduli profil
		}
		else if (personality == DCO_EAIPersonality.CAUTIOUS)
		{
			moving = ECharacterStance.CROUCH;
		}
		else
		{
			if (Math.RandomInt(0, 2) == 1)
				moving = ECharacterStance.STAND;
			else
				moving = ECharacterStance.CROUCH;
		}

		if (assaultBuilding || isCQB)
		{
			moving = ECharacterStance.CROUCH;
			end    = ECharacterStance.CROUCH;
			return;
		}

		if (dist < 35)
		{
			if (moving == ECharacterStance.STAND)
			{
				end = ECharacterStance.CROUCH;
			}
			else
			{
				if (Math.RandomInt(0, 2) == 1)
					end = ECharacterStance.CROUCH;
				else
					end = ECharacterStance.STAND;
			}
			return;
		}

		if (moving == ECharacterStance.STAND)
			end = ECharacterStance.CROUCH;
		else
			end = ECharacterStance.PRONE;
	}

	//------------------------------------------------------------------------------------------------
	protected float ResolveStoppedWaitTime(bool inCover, bool isCQB, bool threatened)
	{
		float waitTime = WAIT_TIME_OPEN_S;
		if (inCover)
			waitTime = WAIT_TIME_IN_COVER_S;

		if (m_State && m_State.IsMovingToBuilding())
			waitTime *= 2;

		if (m_Utility.m_AIInfo && m_Utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
			waitTime *= 3;

		if (isCQB)
			waitTime *= WAIT_SCALE_CQB;

		if (threatened)
			waitTime *= WAIT_SCALE_THREATENED;

		switch (GetPersonality())
		{
			case DCO_EAIPersonality.CAUTIOUS:   waitTime *= 1.5; break;
			case DCO_EAIPersonality.AGGRESSIVE: waitTime *= 0.7; break;
			case DCO_EAIPersonality.RECKLESS:   waitTime *= 0.4; break;
		}

		return Math.RandomFloat(0.8, 1.2) * waitTime;
	}

	//------------------------------------------------------------------------------------------------
	protected bool MoveToNextPosCondition(bool isCQB)
	{
		if (m_State.IsExecutingRequest())
			return false;

		bool threatened = m_Utility.m_ThreatSystem.GetState() == EAIThreatState.THREATENED;

		if (threatened && !isCQB)
		{
			DCO_EAIPersonality p = GetPersonality();
			if (p == DCO_EAIPersonality.CAUTIOUS)
				return false;
		}

		float stoppedWaitTime = ResolveStoppedWaitTime(m_State.m_bInCover, isCQB, threatened);
		return m_State.m_fTimerStopped_s > stoppedWaitTime;
	}

	//------------------------------------------------------------------------------------------------
	protected static ref TStringArray s_aVarsIn = { PORT_POSITION };
	override TStringArray GetVariablesIn() { return s_aVarsIn; }

	override static bool VisibleInPalette() { return true; }
}