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
	// Inputs
	protected static const string PORT_BASE_TARGET = "BaseTarget";
	protected static const string PORT_AVOID_STRAIGHT_PATH_DIR = "AvoidStraightPathDir";
	
	protected BaseTarget m_Target;
	protected vector m_vAvoidStraightPathDir;
	protected DCO_AIMoraleSystem moraleSystem;
	
	protected static const float CRITICAL_BID_COOLDOWN_MS   = 3000.0;
	protected static const float CRITICAL_BUILDING_DIST_MAX = 60.0;
	protected static const float CRITICAL_COVER_DIST_MAX    = 25.0;
	
	protected static const float ENGAGED_BID_COOLDOWN_MS = 3000.0;
	protected static const float ENGAGED_COVER_DIST_MAX  = 18.0;
	
	//! Lama ditekan (THREATENED) sebelum AI maksa balas tembak, sebelum diskala personality.
	protected static const float CRITICAL_RETURN_FIRE_BASE_S = 5.0;
	
	//! Lama "lock" reposisi selagi musuh masih dalam jarak optimal, sebelum diskala personality.
	protected static const float REPOSITION_LOCK_BASE_S = 5.0;
	
	//! Area terbuka bukan lagi SYARAT buat cari cover, tapi pengali urgensi.
	protected static const float OPEN_AREA_COVER_URGENCY      = 1.35;
	protected static const float OPEN_AREA_BID_COOLDOWN_SCALE = 0.6;
	
	//! Dipakai kalau DCO_GroupConfigComponent belum kepasang di prefab grup.
	protected static const float COHESION_DIST_FALLBACK  = 50.0;
	protected static const float COHESION_REGROUP_RADIUS = 25.0;
	
	//! Dulu di-new tiap panggilan di dalam ResolveMoveRequestMovePosAndDir().
	protected static ref RandomGenerator s_CohesionRand = new RandomGenerator();
	
	protected float m_fNextEngagedBid_ms = -1;
	protected float m_fNextCriticalBid_ms = -1;
	
	//! Waktu masuk state THREATENED. -1 = lagi ga critical.
	protected float m_fCriticalEnterTime_ms = -1;
	
	//! IsInOpenArea() = 8 raycast. Di-cache sekali per tick di EOnTaskSimulate().
	protected bool m_bInOpenAreaCached = false;
	
	//--------------------------------------------------------------------------------------------
	protected override bool OnUpdate(AIAgent owner, float dt)
	{
		GetVariableIn(PORT_BASE_TARGET, m_Target);
		GetVariableIn(PORT_AVOID_STRAIGHT_PATH_DIR, m_vAvoidStraightPathDir);
		
		if (!m_Target || !m_Target.GetTargetEntity())
			return false;
		
		moraleSystem = m_Utility.GetMoraleSystem();
		return true;
	}
	
	//--------------------------------------------------------------------------------------------
	protected override float GetTargetDistance()
	{
		return m_Target.GetDistance();
	}
	
	//--------------------------------------------------------------------------------------------
	protected override vector GetTargetPosition()
	{
		return m_Target.GetLastSeenPosition();
	}
	
	protected override vector GetAvoidStraightPathDir()
	{
		return m_vAvoidStraightPathDir;
	}
	
	//--------------------------------------------------------------------------------------------
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
	
	//--------------------------------------------------------------------------------------------
	//! Dipanggil tiap tick dari EOnTaskSimulate(), SETELAH m_eThreatState di-refresh.
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
	
	//! Udah kelamaan ditekan tanpa balas — berhenti nunduk, biarin node nembak kerja.
	//! Jalan paralel (OR) sama ShouldReturnFireWhenEndangered() di UpdateAttackData,
	//! yang gatenya pakai BESARAN threat. Yang ini gatenya WAKTU.
	protected bool ShouldForceReturnFire(float now_ms)
	{
		float threshold_s = CRITICAL_RETURN_FIRE_BASE_S * DCO_PersonalityCombatUtility.GetReturnFireDelayScale(m_Utility);
		
		return GetCriticalElapsed_s(now_ms) > threshold_s;
	}
	
	//--------------------------------------------------------------------------------------------
	//! true kalau unit kejauhan dari Squad Leader dan harus regroup.
	//! Jarak diambil dari DCO_GroupConfigComponent kalau kepasang di prefab grup,
	//! kalau ga ada jatuh ke COHESION_DIST_FALLBACK — jadi ga hard-depend ke wiring Workbench.
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
		
		float maxDist = COHESION_DIST_FALLBACK;
		
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
	        searchDistMax      = CRITICAL_COVER_DIST_MAX;
	    }
	    else
	    {
	        // Tahap 1: gedung terdekat.
	        rq.m_eType         = SCR_EAICombatMoveRequestType.BUILDING;
	        rq.m_bTryFindCover = false;
	        searchDistMax      = CRITICAL_BUILDING_DIST_MAX;
	    }
	
	    rq.m_fCoverSearchDistMin = 0;
	    rq.m_fCoverSearchDistMax = searchDistMax;
	    rq.m_fMoveDuration_s     = searchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;
	
	    rq.GetOnMovementStarted().Insert(OnMovementStarted);
	    rq.GetOnCompleted().Insert(OnMovementCompleted);
	
	    m_fNextCriticalBid_ms = GetGame().GetWorld().GetWorldTime() + CRITICAL_BID_COOLDOWN_MS;
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
			
			// It's a vehicle
			vector pos = tgtEntity.GetOrigin();
			pos = pos + Vector(0, 2.0, 0);
			return pos;
		}
		
		// Target is either not visible or tgtEntity is null, use last seen position
		vector lastSeenPos = m_Target.GetLastSeenPosition();
		lastSeenPos = lastSeenPos + Vector(0, 1.8, 0);
		return lastSeenPos;		
	}
	
	//--------------------------------------------------------------------------------------------
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
			// Long range combat
			if (IsFirstExecution())
				return true; // On first run we want to move to cover, or stay where we are if there is no cover, and shoot.
			else
				return m_State.m_bInCover; // Don't leave cover if there is no next cover
		}
	}
	
	//--------------------------------------------------------------------------------------------
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
			// In cover
			switch (threat)
			{
				case EAIThreatState.THREATENED:
					waitTime = Math.RandomFloat(60, 90);	// Stay in cover for a long time, until we are not suppressed any more
					break;
				default:
					waitTime = Math.RandomFloat(30, 45);
			}
		}
		else
		{
			// Not in cover
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
		
		// When using those weapons we want to move much less
		bool longWaitTime = false;
		switch (weaponType)
		{
			case EWeaponType.WT_MACHINEGUN:
			case EWeaponType.WT_ROCKETLAUNCHER:
			case EWeaponType.WT_GRENADELAUNCHER:
			case EWeaponType.WT_SNIPERRIFLE:
				longWaitTime = true;
		}
		
		// Note: it's important to let bots enough time to aim at very long range
		// Stop time should be more than just a few seconds.
		if (m_bVeryLongRangeCombat)
			waitTime *= 1.5;
		else
			waitTime *= Math.RandomFloat(0.8, 1);
		
		if (longWaitTime)
			waitTime *= 2;
		
		float mult = Math.Map(moraleSystem.GetMoraleMeasure(), 0, 4.5, 1, 2.5);
		waitTime += mult;
		
		// === ADDED: Personality System ===
		waitTime *= DCO_PersonalityCombatUtility.GetStoppedWaitTimeScale(m_Utility);
		// === END ADDED ===
		
		return waitTime;
	}
	
	//--------------------------------------------------------------------------------------------
	protected override bool MoveToNextPosCondition()
	{
		if (m_Utility && m_Utility.m_DCOConfig && m_Utility.m_DCOConfig.IsHoldPosition())
			return false;

		if (IsCriticalCombatMoment())
			return false;
		
		// Dulu hardcoded 10 detik. Sekarang base 5 detik, diskala personality:
		// CAUTIOUS 3.0s / STANDARD 5.0s / AGGRESSIVE 7.0s / RECKLESS 9.0s
		float lockTime_s = REPOSITION_LOCK_BASE_S * DCO_PersonalityCombatUtility.GetRepositionLockScale(m_Utility);
		
		float optimalDist = ResolveOptimalDistance(m_fWeaponMinDist);
		if (m_fTargetDist < optimalDist && m_Target.GetTimeSinceSeen() < lockTime_s)
			return false;
			
		if (m_State.IsExecutingRequest())
			return false;
		
		if (IsFirstExecution() && !m_State.m_bInCover)
			return true;
		
		// If we should keep formation, don't move too far away
		if (m_Utility.ShouldKeepFormation() || m_CombatComp.GetCombatMode() == EAIGroupCombatMode.HOLD_FIRE)
		{
			// In this case we move only once
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
				vector mp = s_CohesionRand.GenerateRandomPointInRadius(0, COHESION_REGROUP_RADIUS, leaderPos, false);
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
				//eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS; // Move to target
				avoidStraightPathDir = GetAvoidStraightPathDir(); // Use flanking
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
				// We are outside WP, move towards center
				movePos = wpPos;
				eDirection = SCR_EAICombatMoveDirection.FORWARD;
				coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
				avoidStraightPathDir = vector.Zero; // Go straight
			}
			else if (myDistToWp > 0.5 * wpRadius)
			{
				// We are between 50% and 100% of wp radius
				
				if (tgtInWaypoint)
				{
					// Towards target
					movePos = targetPos;
					eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
					avoidStraightPathDir = GetAvoidStraightPathDir(); // Use flanking
					coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
				}
				else
				{
					// Move around current pos.
					movePos = targetPos;
					eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
					avoidStraightPathDir = vector.Zero;
					coverSearchSectorHalfAngleRad = -1.0;
				}
			}
			else
			{
				// We are within 50% radius of wp,
				// Move towards tgt, regardless where tgt is
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
		
		// Common values
		rq.m_vTargetPos = ResolveRequestTargetPos();
		ResolveMoveRequestMovePosAndDir(rq.m_vTargetPos, rq.m_vMovePos, rq.m_vAvoidStraightPathDir, rq.m_eDirection, rq.m_fCoverSearchSectorHalfAngleRad);
		rq.m_bTryFindCover = true;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility = true;
		
		// === ADDED: Concealment-seeking pas exposed ===
		// Kalau AI lagi di tempat terbuka, cover-search jangan dibatesin cuma ke arah
		// target (m_bUseCoverSearchDirectivity=false = cari ke SEGALA arah) -- yang
		// penting nemu concealment SEKARANG, bukan concealment yang convenient buat
		// nyerang doang.
		bool isExposed = DCO_ConcealmentUtility.IsPositionExposed(m_MyEntity.GetOrigin(), m_MyEntity);
		if (isExposed)
			rq.m_bUseCoverSearchDirectivity = false;
		// === END ADDED ===

		float coverSearchDistMin = 5;
		float coverSearchDistMax = 30;
		float moveDurationMax = 10;
		if (m_bCloseRangeCombat)
		{
			// Close range combat
			
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
			
			//rq.m_eMovementType = EMovementType.WALK;
			// === MODIFIED: dibungkus DCO_MoraleCombatUtility.CanAimWhileMoving -- BREAK
			// gak sempet aim-while-moving walau secara teknis diizinin ===
			rq.m_bAimAtTarget = DCO_MoraleCombatUtility.CanAimWhileMoving(
				DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection) &&
				IsAimingAndMovingAllowedForWeapon(m_eWeaponType),
				moraleSystem);
			rq.m_bAimAtTargetEnd = true;
		}
		else
		{
			// Long range combat
			
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
			
			//rq.m_eMovementType = EMovementType.RUN;
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
		
		if (m_State.GetOldRequest() && m_State.GetOldRequest().m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
			rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		else
		{
			rq.m_eType = SCR_EAICombatMoveRequestType.BUILDING;
			rq.m_bTryFindCover = false;
			DCO_BreachUtility.TryThrowBreachGrenade(m_Utility, rq.m_vTargetPos);
		}
		
		rq.m_bFailIfNoCover = ResolveFailMoveIfNoCover();
		
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
		
		// Subscribe to events
		// We will pronounce voice lines once we start or end moving
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
		// We're pinned in cover and can't do much else now
		// Alternate hiding in cover and unhiding
		
		// How long to wait here? Depends on timer value from previous request.
		
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
			float newWaitTime = Math.RandomFloat(5, 9.0); // Hide in cover for this time
			PushRequestChangeStanceInCover(false, SCR_EAICombatMoveReason.SUPPRESSED_IN_COVER, newWaitTime);
		}
		else if (!m_State.m_bExposedInCover && m_State.m_fTimerRequest_s > waitTime_s)
		{
			float newWaitTime = Math.RandomFloat(1.5, 2.5); // Expose out of cover for this time
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
		
		// IsInOpenArea() = 8 raycast. Dulu dipanggil 3x per tick (MoveToNextPosCondition,
		// SuppressedInCoverCondition, dan fallback bound). Sekarang sekali di sini,
		// setelah m_eThreatState valid.
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
			// We're in cover but we are still hiding in it, unhide
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
			// IsInOpenArea BUKAN lagi gate. Dulu unit yang ga di cover TAPI juga ga di
			// area terbuka jatuh ke sini dan ga ngelakuin apa-apa — diam nembak selamanya.
			// Sekarang semua unit not-in-cover dapet jalur cari cover; open area cuma
			// naikin urgensinya.
			if (IsCriticalCombatMoment())
			{
				if (ShouldForceReturnFire(currentTime_ms))
				{
					// Udah lewat ambang ditekan tanpa balas. Sengaja TIDAK push move
					// request apa pun dan TIDAK push stance turun — tujuannya balas
					// tembak, bukan pindah. Naikin dari PRONE biar senjata ga keblok.
					if (m_eStance == ECharacterStance.PRONE)
						m_CharacterController.SetStanceChange(1);
				}
				else if (CriticalBoundCooldownReady())
				{
					PushRequestCriticalBound();
				}
				else
				{
					// Cuma jeda antar-bid: nunduk sambil nunggu, bukan lagi respons default.
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
					takeCoverChance *= OPEN_AREA_COVER_URGENCY;
				
				takeCoverChance = Math.Clamp(takeCoverChance, 0.0, 1.0);
				
				if (Math.RandomFloat01() < takeCoverChance)
				{
					float optimalDist = ResolveOptimalDistance(m_fWeaponMinDist);
					bool contactFresh = m_Target && m_Target.GetTimeSinceSeen() < 5.0;
					bool inEngagementRange = m_Target && m_fTargetDist <= optimalDist * 1.2;
					
					if (contactFresh && inEngagementRange)
						PushRequestEngagedBound();
					else
						PushRequestOpenArea();
				}
				else
				{
					float bidCooldown_ms = ENGAGED_BID_COOLDOWN_MS;
					
					if (m_bInOpenAreaCached)
						bidCooldown_ms *= OPEN_AREA_BID_COOLDOWN_SCALE;
					
					m_fNextEngagedBid_ms = currentTime_ms + bidCooldown_ms;
				}
			}

		} else if (!m_State.IsExecutingRequest())
		{
			if (m_Utility.GetCharacterController().GetWeaponObstructedState() != EWeaponObstructedState.UNOBSTRUCTED)
			{
				if (m_CharacterController.GetStance() == ECharacterStance.CROUCH)
					m_CharacterController.SetStanceChange(0);
				else if (m_CharacterController.GetStance() == ECharacterStance.PRONE)
					m_CharacterController.SetStanceChange(1);
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
	    rq.m_fCoverSearchDistMax = ENGAGED_COVER_DIST_MAX;
	    rq.m_fMoveDuration_s     = (ENGAGED_COVER_DIST_MAX / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN) * MoraleAmplifyMove();
	
	    rq.GetOnMovementStarted().Insert(OnMovementStarted);
	    rq.GetOnCompleted().Insert(OnMovementCompleted);
	
	    m_fNextEngagedBid_ms = GetGame().GetWorld().GetWorldTime() + ENGAGED_BID_COOLDOWN_MS;
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
		
		// Common values
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
			// Long range combat
			
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
					moveDurationMax = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_RUN; // Shouldn't be so large because we are sprinting and can't shoot
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
			}
			
			//rq.m_eMovementType = EMovementType.RUN;
			// === MODIFIED: dibungkus DCO_MoraleCombatUtility.CanAimWhileMoving -- BREAK
			// gak sempet aim-while-moving walau secara teknis diizinin ===
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
		
		// If prev. request was FF avoidance too, keep direction.
		// Otherwise choose a new direction.
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
		
		rq.m_eStanceMoving = m_CharacterController.GetStance(); // Don't change stance
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
		rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD; // Move back from target
		// === MODIFIED: dibungkus DCO_MoraleCombatUtility.CanAimWhileMoving ===
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
	
	//--------------------------------------------------------------------------------------------
	protected static ref TStringArray s_aVarsIn = {
		PORT_BASE_TARGET,
		PORT_AVOID_STRAIGHT_PATH_DIR
	};
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
}