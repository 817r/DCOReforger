modded class SCR_AICombatMoveLogicVehicleGunner_Attack : SCR_AICombatMoveLogicVehicleGunnerBase
{
	// Inputs
	protected static const string PORT_BASE_TARGET = "BaseTarget";
	
	protected BaseTarget m_Target;
	
	protected const float WEAPON_MIN_DIST = 2.0;
	
	// minimal distance Driver should be from enemy
	protected const float MIN_ENGAGEMENT_DISTANCE_TO_TARGET_SQ = 80.0 * 80.0;	// when too close to target, try to move backwards
	protected const float MAX_MOVE_DURATION_TO_TARGET_S = 9; 					// max step to move towards target
	protected const float MAX_MOVE_DURATION_TO_TARGET_THREATENED_S = 7; 		// move less under threat
	protected const float REVERSE_MOVE_DURATION_S = 3; 						// step to move from target. This must be consistent with reverse distance in car movement component, so that car moves backwards.
	
	// Values updated on each update, to avoid passing them through calls
	protected EAIThreatState m_eThreatState;
	protected float m_fTargetDist;
	protected float m_fWeaponMinDist = WEAPON_MIN_DIST;
	
	//------------------------------------------------------------------------------------
	override bool UpdateCombatMoveLogic()
	{
		// Read target data
		GetVariableIn(PORT_BASE_TARGET, m_Target);
		if (!m_Target || !m_Target.GetTargetEntity())
			return false;
		
		// Update cached variables
		m_fTargetDist = GetTargetDistance();		
		m_eThreatState = m_Utility.m_ThreatSystem.GetState();
		m_fWeaponMinDist = 2.0;
		
		// Conditions and states of combat movement
		if (MoveFromTargetCondition())
		{
			// Target out of reach for turret or too close
			// Step backwards
			if (MoveFromTargetNewRequestCondition())
				PushRequestMoveFromTarget();
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
		else if (!m_DriverState.IsExecutingRequest())
		{
			// TODO: We are stopped keep distance, scan the perimeter			
		}
		
		return true;
	}
	//--------------------------------------------------------------------------------------------
	// Movement
	
	override protected void PushRequestMove()
	{		
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
		
		// Common values
		rq.m_vTargetPos = ResolveRequestTargetPos();
		ResolveMoveRequestMovePosAndDir(rq.m_vTargetPos, rq.m_vMovePos, rq.m_eDirection);
		rq.m_bTryFindCover = false;
		rq.m_bUseCoverSearchDirectivity = false;
		rq.m_bCheckCoverVisibility = false;
		
		
		float moveDurationMax = MAX_MOVE_DURATION_TO_TARGET_S;
		
		// Long range combat
		
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
		rq.m_bAimAtTargetEnd = false; // turn towards the target should be true!
		rq.m_bFailIfNoCover = false;
		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		rq.m_fMoveDuration_s = Math.RandomFloat(0.8, 1.3) * moveDurationMax; // Move distance randomized
		vector dirToTgt = m_Target.GetLastSeenPosition() - m_DriverUtility.m_OwnerEntity.GetOrigin();
		dirToTgt.Normalize();
		rq.m_vAvoidStraightPathDir = dirToTgt;
		// Subscribe to events
		// We will pronounce voice lines once we start or end moving
		rq.GetOnMovementStarted().Insert(OnMovementStarted);
		rq.GetOnCompleted().Insert(OnMovementCompleted);
		
		ApplyNewRequest(rq);
	}
	
	//--------------------------------------------------------------------------------------------
	// Resolves which move pos and dir. we should use for _MOVE_ request
	// By now rq.m_vTargetPos must be already calculated!
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
			// No waypoint, standard move logic
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
				// We are outside WP, move towards center
				movePos = wpPos;
				eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;				
			}
			else if (myDistToWp > 0.5 * wpRadius)
			{
				// We are between 50% and 100% of wp radius
				
				if (tgtInWaypoint)
				{
					// Towards target
					movePos = targetPos;
					eDirection = SCR_EAICombatMoveDirection.FORWARD;					
				}
				else
				{
					// Move around current pos.
					movePos = targetPos;
					eDirection = SCR_EAICombatMoveDirection.ANYWHERE;					
				}
			}
			else
			{
				// We are within 50% radius of wp,
				// Move towards tgt, regardless where tgt is
				movePos = targetPos;
				eDirection = SCR_EAICombatMoveDirection.FORWARD;								
			}
		}
		
		outMovePos = movePos;
		outDirection = eDirection;		
	}
	
	override protected bool MoveToNextPosCondition()
	{
		// Don't get any more closer
		// Except we should still move closer if we haven't seen target for a long time
		float optimalDist = ResolveOptimalDistance(m_fWeaponMinDist);
		if (m_fTargetDist < optimalDist && m_Target.GetTimeSinceSeen() < 5)
			return false;
			
		if (m_DriverState.IsExecutingRequest())
			return false;
		
		// If it's first run, ignore timers
		// TODO: add effect of explosion from threat system, i.e. if BOOM -> move away asap
		if (IsFirstExecution())
			return true;
		
		float stoppedWaitTime = ResolveStoppedWaitTime(m_eThreatState);	
		return m_DriverState.m_fTimerStopped_s > stoppedWaitTime;
	}
	
	//--------------------------------------------------------------------------------------------
	// Returns 'optimal' distance
	// If we are between weaponMinDist and 'optimal' dist, we don't need to move closer to tgt
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
	
	//-------------------------------------------------------------------------------------------
	override bool UpdateCombatMoveLogic()
	{
		GetVariableIn(PORT_SUPPRESSION_VOLUME, m_SuppressionVolume);
		if (!m_SuppressionVolume)
			return false;
		
		GetVariableIn(PORT_VISIBLE, m_bTargetVisible);
		GetVariableIn(PORT_TIME_LAST_SEEN, m_fTargetLastSeenTime_ms);
		
		float timeSinceLastSeen_ms = GetGame().GetWorld().GetWorldTime() - m_fTargetLastSeenTime_ms;
		m_bGoodVision = m_bTargetVisible || (timeSinceLastSeen_ms < TIME_SINCE_GOOD_VISIBILITY_MIN_MS);
		
		if (MoveFromTargetCondition())
		{
			// Too close to target
			// Step away
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
		
		// Still executing ...
		// Send new request only if we are executing NOT side-step
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
	
	//-------------------------------------------------------------------------------------------
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
	
	//-------------------------------------------------------------------------------------------
	protected static ref TStringArray s_aVarisIn = { 
		PORT_SUPPRESSION_VOLUME, 
		PORT_VISIBLE,
		PORT_TIME_LAST_SEEN
	};	
	override TStringArray GetVariablesIn() { return s_aVarisIn; }
}