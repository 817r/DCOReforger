class DCO_AICombatMoveLogic_SuppressiveEvasive : SCR_AICombatMoveLogicBase
{
	protected static const string PORT_SUPPRESSION_VOLUME = "SuppressionVolume";
	protected static const string PORT_VISIBLE = "Visible";
	protected static const string PORT_TIME_LAST_SEEN = "TimeLastSeen_ms";
	
	protected vector DefendPoint;
	
	// Variables updated from input ports
	protected SCR_AISuppressionVolumeBase m_SuppressionVolume;
	protected bool m_bTargetVisible = false;
	protected float m_fTargetLastSeenTime_ms = 0; // World time
	
	protected bool m_bGoodVision;
	
	protected static const float TIME_SINCE_GOOD_VISIBILITY_MIN_MS = 5000.0;
	protected MoraleState m_MoraleState;
	
	protected override void OnInit(AIAgent owner)
	{
		m_Utility = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
		if (m_Utility)
			m_State = m_Utility.m_CombatMoveState;
		
		m_MyEntity = owner.GetControlledEntity();
		
		if (m_MyEntity)
		{
			m_CharacterController = CharacterControllerComponent.Cast(m_MyEntity.FindComponent(CharacterControllerComponent));
			m_CombatComp = SCR_AICombatComponent.Cast(m_MyEntity.FindComponent(SCR_AICombatComponent));
		}
		
		DefendPoint = owner.GetParentGroup().GetLeaderEntity().GetOrigin();
	}
	
	protected override bool OnUpdate(AIAgent owner, float dt)
	{
		GetVariableIn(PORT_SUPPRESSION_VOLUME, m_SuppressionVolume);
		
		if (!GetVariableIn(PORT_VISIBLE, m_bTargetVisible))
			return false;
		
		if (!GetVariableIn(PORT_TIME_LAST_SEEN, m_fTargetLastSeenTime_ms))
			return false;
		
		if (!m_SuppressionVolume)
			return false;
		
		// Update m_bGoodVision
		// The timer criteria is to exclude occlusion due to us hiding in cover
		float timeSinceLastSeen_ms = GetGame().GetWorld().GetWorldTime() - m_fTargetLastSeenTime_ms;
		m_bGoodVision = m_bTargetVisible || (timeSinceLastSeen_ms < TIME_SINCE_GOOD_VISIBILITY_MIN_MS);
		
		return true;
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
		
		// Don't run combat movement logic if CombatMove BT is not used now (like in turret)
		SCR_AIBehaviorBase executedBehavior = SCR_AIBehaviorBase.Cast(m_Utility.GetExecutedAction());
		if (executedBehavior && !executedBehavior.m_bUseCombatMove)
			return ENodeResult.RUNNING;
		
		// Update cached variables
		m_fTargetDist = GetTargetDistance();
		m_bCloseRangeCombat = m_fTargetDist < SCR_AICombatMoveUtils.CLOSE_RANGE_COMBAT_DIST;
		m_bVeryLongRangeCombat = m_fTargetDist > SCR_AICombatMoveUtils.VERY_LONG_RANGE_COMBAT_DIST;
		m_eThreatState = m_Utility.m_ThreatSystem.GetState();
		m_eStance = m_CharacterController.GetStance();
		m_fWeaponMinDist = m_CombatComp.GetSelectedWeaponMinDist();
		m_eWeaponType = m_CombatComp.GetSelectedWeaponType();
		
		
		/*		
		//------------------------------------------------------------------------------------
		Combat movement logic
		
		Conditions represent states inside which we want to remain.
		
		Conditions are organized based on their priority, highest first.
		
		Within each state there can be extra logic which decides if it's worth to
		send a new request, because even though we have selected a state, we should avoid
		spamming same request over and over.
		
		Conditions for states mostly depend on Combat Move State and its timers.
		
		It is important to write logic in such a way that it doesn't depend on state
		of this node. In this case the state flow also doesn't depend on it, and AI
		does movement is more fluent when switching to a new behavior which also utilizes
		combat movement, including attacking a different target.
		*/
		
		if (MoveFromTargetCondition())
		{
			// Too close to target
			// Step away
			if (MoveFromTargetNewRequestCondition())
				PushRequestMoveFromTarget();
		}
		else if (CurrentCoverUselessCondition())
		{
			// Current cover has been compromised, it's not directed at enemy any more
			// Find a new cover nearby
			PushRequestLeaveUselessCover();
		}
		else if (m_State.m_bInCover && m_CharacterController.IsReloading())
		{
			// We're reloading and can't do much else now
			// Hide in cover
			if (m_State.m_bExposedInCover)
				m_State.ApplyRequestChangeStanceInCover(false);
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
			// We've waited here too long, move to next place
			PushRequestMove();
		}
		else if (!m_State.IsExecutingRequest() && !m_State.m_bInCover)
		{
			// We are stopped and not in cover, manage our stance
			ECharacterStance newStance = ResolveStanceOutsideCover(m_bCloseRangeCombat, m_eThreatState);
			if (newStance > m_eStance)
			{
				// Only let stance go down, no need to get back up
				m_State.ApplyRequestChangeStanceOutsideCover(newStance);
			}
		}
		
		return ENodeResult.RUNNING;
	}
	
	protected override bool MoveFromTargetCondition()
	{
		float weaponMinDist = Math.Max(20.0, m_fWeaponMinDist);
		
		return m_fTargetDist < weaponMinDist;
	}
	
	protected override bool MoveToNextPosCondition()
	{	
		if (m_State.IsExecutingRequest())
			return false;
		
		if (m_bGoodVision && m_State.m_bInCover)
		{
			// We have good vision and we are in cover, just stay here
			return false;
		}
		
		// If vision is bad, move out until we have good visibility
		// Here we operate with visibility of the suppression volume, still concept is same as during normal attack.
		// Most important thing is to exit area with poor vision of target, but beyond that we don't need to move.
		if (!m_bGoodVision || (m_bGoodVision && !m_State.m_bInCover && m_eWeaponType != EWeaponType.WT_MACHINEGUN) || (m_fTargetLastSeenTime_ms == 0) || (!m_State.m_bInCover && m_eWeaponType == EWeaponType.WT_MACHINEGUN && m_eThreatState == EAIThreatState.THREATENED))
		{
			// If it's first run, ignore timers, only if:
			// - If we are not in cover.
			if (IsFirstExecution() && !m_State.m_bInCover)
				return true;
			
			if (m_eThreatState == EAIThreatState.THREATENED && m_MoraleState > MoraleState.NORMAL)
				return true;
			
			float stoppedWaitTime = ResolveStoppedWaitTime(m_State.m_bInCover, m_eThreatState, m_eWeaponType);	
			return m_State.m_fTimerStopped_s > stoppedWaitTime;
		}
		
		return false;
	}
	
	//--------------------------------------------------------------------------------------------
	protected override float ResolveStoppedWaitTime(bool inCover, EAIThreatState threat, EWeaponType weaponType)
	{
		m_MoraleState = m_CombatComp.GetMoraleComponent().GetMoraleStates();
		float waitTime;
		const float minRandomTime = 1;
		const float maxRandomTime = 4;
		
		if (inCover)
		{
			// In cover
			switch (threat)
			{
				case EAIThreatState.THREATENED:
					waitTime = Math.RandomFloat(minRandomTime, maxRandomTime * 4);	// Stay in cover for a long time, until we are not suppressed any more
					break;
				default:
					waitTime = Math.RandomFloat(minRandomTime, maxRandomTime);
			}
		}
		else
		{
			// Not in cover
			switch (threat)
			{
				case EAIThreatState.THREATENED:
					waitTime = Math.RandomFloat(minRandomTime, maxRandomTime);
					break;
				default:
					waitTime = Math.RandomFloat(minRandomTime, minRandomTime + 3);
					break;
			}
		}
		
		switch (m_MoraleState)
		{
			case MoraleState.FRESH:
			{
				waitTime *= 0.5;
				break;
			}
			case MoraleState.NORMAL:
			{
				waitTime *= 0.8;
				break;
			}
			case MoraleState.STRESSED:
			{
				waitTime *= 1.2;
				break;
			}
			case MoraleState.PRESSURED:
			{
				waitTime *= 1.6;
				break;
			}
			case MoraleState.BREAK:
			{
				waitTime *= 2.0;
				break;
			}
		}
		
		return Math.Max(waitTime, 5);
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

		float coverSearchDistMin = 0;
		float coverSearchDistMax = 30;
		float moveDurationMax = 6;
		if (m_bCloseRangeCombat)
		{
			// Close range combat
			
			switch (m_eThreatState)
			{
				case EAIThreatState.THREATENED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					coverSearchDistMin = 2.0;
					coverSearchDistMax = 10.0;
					moveDurationMax = 2;
					break;
				}
				default:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 15.0;
					moveDurationMax = 3;
					break;
				}
			}
			
			rq.m_eMovementType = EMovementType.WALK;
			rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType) &&
								IsAimingAndMovingAllowedForWeapon(m_eWeaponType);
			rq.m_bAimAtTargetEnd = true;
		}
		else
		{
			// Long range combat
			
			switch (m_eThreatState)
			{
				case EAIThreatState.THREATENED:
				{
					coverSearchDistMin = 2.0;
					coverSearchDistMax = 20.0;
					moveDurationMax = 2.5;
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.PRONE;
					break;
				}
				default:
				{
					coverSearchDistMin = 10.0;
					coverSearchDistMax = 30.0;
					moveDurationMax = 4; // Shouldn't be so large because we are sprinting and can't shoot
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					break;
				}
			}
			
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType) &&
								IsAimingAndMovingAllowedForWeapon(m_eWeaponType); // Can't aim at tgt while sprinting
			rq.m_bAimAtTargetEnd = true;
		}
		
		rq.m_bFailIfNoCover = ResolveFailMoveIfNoCover();
		
		// If we are not in cover, min cover search distance is overridden to 0, we should find any cover ASAP
		if (!m_State.m_bInCover)
			coverSearchDistMin = 0;
		
		rq.m_fCoverSearchDistMin = coverSearchDistMin;
		rq.m_fCoverSearchDistMax = coverSearchDistMax;
		rq.m_fMoveDuration_s = Math.RandomFloat(0.5, 1.0) * moveDurationMax;
		
		rq.GetOnMovementStarted().Insert(OnMovementStarted);
		rq.GetOnCompleted().Insert(OnMovementCompleted);
		
		m_State.ApplyNewRequest(rq);
	}
	
	protected override void ResolveMoveRequestMovePosAndDir(vector targetPos, out vector outMovePos, out vector outAvoidStraightPathDir, out SCR_EAICombatMoveDirection outDirection, out float outCoverSearchSectorHalfAngleRad)
	{	
		AIWaypoint wp = null;
		AIAgent agent = m_Utility.GetAIAgent();
		AIGroup group = agent.GetParentGroup();
		AIAgent Leader = group.GetLeaderAgent();
		SCR_AIGroup myGroup = SCR_AIGroup.Cast(agent.GetParentGroup());
		if (group)
			wp = group.GetCurrentWaypoint();
		
		vector movePos;
		SCR_EAICombatMoveDirection eDirection;
		float coverSearchSectorHalfAngleRad;
		vector avoidStraightPathDir;
		
		if (!wp || SCR_EntityWaypoint.Cast(wp))
		{
			// No waypoint, or it's an entity-associated waypoint, like Follow waypoint.
			// Therefore use standard movement logic.
			// Otherwise they will want to run towards position where the waypoint is placed, which makes no sense.
			movePos = DefendPoint;
			avoidStraightPathDir = GetAvoidStraightPathDir(); // Use flanking
			
			switch(m_eThreatState)
			{
				case EAIThreatState.SAFE:
				{
					eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
					break;
				}
				case EAIThreatState.VIGILANT:
				{
					eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
					break;		
				}
				case EAIThreatState.ALERTED:
				{
					eDirection = SCR_EAICombatMoveDirection.BACKWARD;
					avoidStraightPathDir = vector.Zero;
					break;					
				}
				case EAIThreatState.THREATENED:
				{
					eDirection = SCR_EAICombatMoveDirection.BACKWARD;
					avoidStraightPathDir = vector.Zero;
					break;			
				}
			}
			
			if (IsFirstExecution())
					coverSearchSectorHalfAngleRad = Math.PI; // Full circle, on first run we just want any cover if possible
				else
					coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
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
				eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
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
				eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
				avoidStraightPathDir = GetAvoidStraightPathDir();
				
				coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
			}
		}
		
		outMovePos = movePos;
		outDirection = eDirection;
		outAvoidStraightPathDir = avoidStraightPathDir;
		outCoverSearchSectorHalfAngleRad = coverSearchSectorHalfAngleRad;
	}
	
	//--------------------------------------------------------------------------------------------
	protected static ref TStringArray s_aVarsIn = {
		PORT_SUPPRESSION_VOLUME,
		PORT_VISIBLE,
		PORT_TIME_LAST_SEEN
	};
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
}