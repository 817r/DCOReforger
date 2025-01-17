modded class SCR_AICombatMoveLogic_MovingCommander : AITaskScripted
{
	protected SCR_AICombatMoveState m_DriverState;
	protected SCR_AIUtilityComponent m_DriverUtility;
	protected const float WEAPON_MIN_DIST = 5.0;
	
	// minimal distance Driver should be from enemy
	protected const float MIN_ENGAGEMENT_DISTANCE_TO_TARGET_SQ = 30.0 * 30.0;	// when too close to target, try to move backwards
	protected const float MAX_MOVE_STEP_TO_TARGET = 100.0; 						// max step to move towards target
	protected const float MAX_MOVE_STEP_TO_TARGET_THREATENED = 45.0; 			// move less under threat
	protected const float MIN_MOVE_STEP_TARGET = 20.0; 							// min step to move from target

	protected override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{		
		float currentTime_ms = GetGame().GetWorld().GetWorldTime();
		if (currentTime_ms < m_fNextUpdate_ms)
			return ENodeResult.RUNNING;
		m_fNextUpdate_ms = currentTime_ms + m_fUpdateInterval_ms;
		
		if (!UpdateDriverAndTarget(owner))
			return ENodeResult.FAIL;
		
		if (!m_DriverState || !m_MyEntity || !m_GunnerUtility)
			return ENodeResult.FAIL;
		
		if (!m_GunnerUtility.m_AIInfo.HasUnitState(EUnitState.IN_TURRET) || m_DriverUtility.m_AIInfo.HasUnitState(EUnitState.UNCONSCIOUS))
			return ENodeResult.RUNNING;
				
		SCR_AIBehaviorBase executedBehavior = SCR_AIBehaviorBase.Cast(m_DriverUtility.GetExecutedAction());
		if (executedBehavior && !executedBehavior.m_bUseCombatMove)
			return ENodeResult.RUNNING;
		
		// Update cached variables
		m_fTargetDist = GetTargetDistance();		
		m_eThreatState = m_GunnerUtility.m_ThreatSystem.GetState();
		m_fWeaponMinDist = 2.0;
		m_eWeaponType = m_WeaponManagerComponent.GetCurrentWeapon().GetWeaponType();		
		
		//------------------------------------------------------------------------------------
				
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
		
		return ENodeResult.RUNNING;
	}
	
	//--------------------------------------------------------------------------------------------
	//! decides if we should move backwards from target 
	override protected bool MoveFromTargetCondition()
	{
		IEntity targetEntity = m_Target.GetTargetEntity();
		if (!targetEntity || !m_WeaponManagerComponent)
			return false;
		vector mat[4];
		m_WeaponManagerComponent.GetCurrentMuzzleTransform(mat);
		vector muzzlePos = mat[3];
		vector muzzleDir = mat[2].Normalized();
		vector targetPos = targetEntity.GetOrigin();
		vector targetDir = (targetPos - muzzlePos).Normalized();
		
		if (vector.Dot(muzzleDir, targetDir) < 0) // target is behind our aiming -> we move backward while changing the aim
			return true;
		vector aimingLimits = m_TurretComponent.GetAimingAngleExcess(targetPos);
		if (aimingLimits[0] != 0 || aimingLimits[1] != 0) // target is unreachable by our turret -> we move backward
			return true;
		if (vector.DistanceSq(targetPos, muzzlePos) < MIN_ENGAGEMENT_DISTANCE_TO_TARGET_SQ) // target is too close infront -> we move backward
			return true;
		
		return false;
	}
	
	//--------------------------------------------------------------------------------------------
	override protected bool MoveFromTargetNewRequestCondition()
	{
		if (!m_DriverState.IsExecutingRequest())
			return true;
		
		// Still executing ...
		// Send new request only if we are executing NOT move_from_target
		SCR_AICombatMoveRequest_Move rq = SCR_AICombatMoveRequest_Move.Cast(m_DriverState.GetRequest());
		if (!rq)
			return true;
		
		return rq.m_eReason != SCR_EAICombatMoveReason.MOVE_FROM_TARGET;
	}
	
	//--------------------------------------------------------------------------------------------
	override protected void PushRequestMoveFromTarget()
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_TARGET;
		
		rq.m_vMovePos = ResolveRequestTargetPos();
		rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
		rq.m_bAimAtTarget = false;
		rq.m_bAimAtTargetEnd = false;
		rq.m_fMoveDuration_s = Math.RandomFloat(3.0, 10.5);
		
		m_DriverState.ApplyNewRequest(rq);
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
		
		float moveDistanceMax = MAX_MOVE_STEP_TO_TARGET;
		
		// Long range combat
		
		switch (m_eThreatState)
		{
			case EAIThreatState.EXHAUSTED:
			{
				moveDistanceMax = MAX_MOVE_STEP_TO_TARGET_THREATENED;				
				break;
			}
			default:
			{
				moveDistanceMax = MAX_MOVE_STEP_TO_TARGET;				
				break;
			}
		}
		
		rq.m_eMovementType = EMovementType.SPRINT;
		rq.m_bAimAtTarget = false; 
		rq.m_bAimAtTargetEnd = true; // turn towards the target should be true!
		rq.m_bFailIfNoCover = false;
		
		rq.m_fMoveDuration_s = Math.RandomFloat(3.0, 10.5); // Move distance randomized
		
		// Subscribe to events
		// We will pronounce voice lines once we start or end moving
		rq.GetOnMovementStarted().Insert(OnMovementStarted);
		rq.GetOnCompleted().Insert(OnMovementCompleted);
		
		m_DriverState.ApplyNewRequest(rq);
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
		movePos = targetPos;
		if (!wp)
		{
			switch(m_eThreatState)
			{
				case EAIThreatState.EXHAUSTED:
				{
					eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
					break;
				}
				case EAIThreatState.PINNED:
				{
					eDirection = SCR_EAICombatMoveDirection.FORWARD;
					break;
				}	
				case EAIThreatState.THREATENED:
				{
					eDirection = SCR_EAICombatMoveDirection.FORWARD;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					eDirection = SCR_EAICombatMoveDirection.FORWARD;
					break;
				}
				case EAIThreatState.VIGILANT:
				{
					eDirection = SCR_EAICombatMoveDirection.FORWARD;
					break;
				}
				case EAIThreatState.SAFE:
				{
					eDirection = SCR_EAICombatMoveDirection.FORWARD;
					break;
				}
			}	
			
			if (m_Target.GetUnitType() == EAIUnitType.UnitType_Infantry)
			{
				eDirection = SCR_EAICombatMoveDirection.FORWARD;
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
	
	//--------------------------------------------------------------------------------------------
	override protected void PushRequestFFAvoidance()
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		rq.m_eReason = SCR_EAICombatMoveReason.FF_AVOIDANCE;
		
		// If prev. request was FF avoidance too, keep direction.
		// Otherwise choose a new direction.
		SCR_AICombatMoveRequest_Move prevRequest = SCR_AICombatMoveRequest_Move.Cast(m_DriverState.GetRequest());
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
		
		rq.m_vMovePos = ResolveRequestTargetPos();
		rq.m_fMoveDuration_s = MIN_MOVE_STEP_TARGET/5;
		rq.m_bAimAtTarget = false;
		rq.m_bAimAtTargetEnd = true;
		
		m_DriverState.ApplyNewRequest(rq);
	}

	
	//--------------------------------------------------------------------------------------------
	override protected float ResolveStoppedWaitTime(EAIThreatState threat)
	{
		float waitTime;
		
		// based on threat we wait longer
			switch(m_eThreatState)
			{
				case EAIThreatState.EXHAUSTED:
				{
					waitTime = Math.RandomFloat(3.0, 5.0);
					break;
				}
				case EAIThreatState.PINNED:
				{
					waitTime = Math.RandomFloat(10.0, 25.0);
					break;
				}	
				case EAIThreatState.THREATENED:
				{
					waitTime = Math.RandomFloat(5.0, 10.0);
					break;
				}
				case EAIThreatState.ALERTED:
				{
					waitTime = Math.RandomFloat(10.0, 15.0);
					break;
				}
				case EAIThreatState.VIGILANT:
				{
					waitTime = Math.RandomFloat(15.0, 35.0);
					break;
				}
				case EAIThreatState.SAFE:
				{
					waitTime = Math.RandomFloat(20.0, 55.0);
					break;
				}
			}	
				
		return waitTime;
	}
	
	//--------------------------------------------------------------------------------------------
	override protected bool MoveToNextPosCondition()
	{
		if (IsFirstExecution())
			return true;
		
		float optimalDist = ResolveOptimalDistance(m_fWeaponMinDist);
		//if (m_fTargetDist < optimalDist && m_Target.GetTimeSinceSeen() < 7)
		//	return false;
		
		if (m_fTargetDist < optimalDist && m_Target.GetTraceFraction() > 0.7)
			return false;
		
		if (m_DriverState.IsExecutingRequest())
			return false;
		
		float stoppedWaitTime = ResolveStoppedWaitTime(m_eThreatState);	
		return m_DriverState.m_fTimerStopped_s > stoppedWaitTime;
	}
	
	//--------------------------------------------------------------------------------------------
	// Returns 'optimal' distance
	// If we are between weaponMinDist and 'optimal' dist, we don't need to move closer to tgt
	override protected static float ResolveOptimalDistance(float weaponMinDist)
	{
		return Math.Max(weaponMinDist + 5.0, 400);
	}
}

class SCR_AICombatMoveLogic_SuppressiveCommander : AITaskScripted
{	
	// Inputs
	protected static const string PORT_SUPPRESSION_VOLUME = "SuppressionVolume";
	protected static const string PORT_VISIBLE = "Visible";
	protected static const string PORT_TIME_LAST_SEEN = "TimeLastSeen_ms";
	
	protected SCR_AISuppressionVolumeBase m_SuppressionVolume;
	protected bool m_bTargetVisible = false;
	protected float m_fTargetLastSeenTime_ms = 0; // World time
	
	protected bool m_bGoodVision;
	
	protected SCR_AIUtilityComponent m_GunnerUtility;
	protected SCR_AICombatComponent m_CombatComp;
	protected SCR_CompartmentAccessComponent m_CompartmentAccessComponent;
	protected BaseWeaponManagerComponent m_WeaponManagerComponent;
	protected TurretComponent m_TurretComponent;
	protected IEntity m_MyEntity;
	protected Vehicle m_MyVehicle;
	
	protected SCR_AICombatMoveState m_DriverState;
	protected SCR_AIUtilityComponent m_DriverUtility;
	protected const float WEAPON_MIN_DIST = 2.0;
	
	// minimal distance Driver should be from enemy
	protected const float MIN_ENGAGEMENT_DISTANCE_TO_TARGET_SQ = 60.0 * 60.0;	// when too close to target, try to move backwards
	protected const float MAX_MOVE_STEP_TO_TARGET = 30.0; 						// max step to move towards target
	protected const float MAX_MOVE_STEP_TO_TARGET_THREATENED = 10.0; 			// move less under threat
	protected const float MIN_MOVE_STEP_TARGET = 5.0; 							// min step to move from target
	
	// Values updated on each update, to avoid passing them through calls
	protected EAIThreatState m_eThreatState;
	protected EWeaponType m_eWeaponType;
	protected float m_fTargetDist;
	protected float m_fWeaponMinDist = WEAPON_MIN_DIST;
	
	[Attribute("500", UIWidgets.EditBox, "Update interval of the node")]
	protected float m_fUpdateInterval_ms;
	protected float m_fNextUpdate_ms;
	
	//--------------------------------------------------------------------------------------------
	protected override void OnInit(AIAgent owner)
	{
		m_GunnerUtility = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
		m_MyEntity = owner.GetControlledEntity();
		
		if (m_MyEntity)
		{
			m_CompartmentAccessComponent = SCR_CompartmentAccessComponent.Cast(m_MyEntity.FindComponent(SCR_CompartmentAccessComponent));
			m_CombatComp = SCR_AICombatComponent.Cast(m_MyEntity.FindComponent(SCR_AICombatComponent));		
			
			if (m_CompartmentAccessComponent && m_CompartmentAccessComponent.IsInCompartment())
			{
				IEntity turretEnt = m_CompartmentAccessComponent.GetCompartment().GetOwner();
				if (turretEnt)
				{
					TurretControllerComponent contr = TurretControllerComponent.Cast(turretEnt.FindComponent(TurretControllerComponent));
					if (contr)
						m_TurretComponent = contr.GetTurretComponent();
					m_WeaponManagerComponent = BaseWeaponManagerComponent.Cast(turretEnt.FindComponent(BaseWeaponManagerComponent));
				}	
				m_MyVehicle = Vehicle.Cast(m_CompartmentAccessComponent.GetVehicle());	
			}	
		}
	}
	
	protected bool Suppress()
	{
		GetVariableIn(PORT_SUPPRESSION_VOLUME, m_SuppressionVolume);
		
		if (!GetVariableIn(PORT_VISIBLE, m_bTargetVisible))
			return false;
		
		if (!GetVariableIn(PORT_TIME_LAST_SEEN, m_fTargetLastSeenTime_ms))
			return false;
		
		if (!m_SuppressionVolume)
			return false;

		m_bGoodVision = m_bTargetVisible;
		
		return true;
	}
	
	protected override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{		
		float currentTime_ms = GetGame().GetWorld().GetWorldTime();
		if (currentTime_ms < m_fNextUpdate_ms)
			return ENodeResult.RUNNING;
		m_fNextUpdate_ms = currentTime_ms + m_fUpdateInterval_ms;
		
		if (!UpdateDriverAndTarget(owner))
			return ENodeResult.FAIL;
		
		if (!m_DriverState || !m_MyEntity || !m_GunnerUtility)
			return ENodeResult.FAIL;
		
		if (!m_GunnerUtility.m_AIInfo.HasUnitState(EUnitState.IN_TURRET) || m_DriverUtility.m_AIInfo.HasUnitState(EUnitState.UNCONSCIOUS))
			return ENodeResult.RUNNING;
				
		SCR_AIBehaviorBase executedBehavior = SCR_AIBehaviorBase.Cast(m_DriverUtility.GetExecutedAction());
		if (executedBehavior && !executedBehavior.m_bUseCombatMove)
			return ENodeResult.RUNNING;
		
		Suppress();
		
		m_fTargetDist = GetTargetDistance();
		m_eThreatState = m_GunnerUtility.m_ThreatSystem.GetState();
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

		if (MoveToNextPosCondition())
		{
			// We've waited here too long, move to next place
			PushRequestMove();
		}
		else if (!m_DriverState.IsExecutingRequest())
		{
			PushRequestMove();			
		}
		
		return ENodeResult.RUNNING;
	}
	
	protected float GetTargetDistance()
	{
		return vector.Distance(m_SuppressionVolume.GetCenterPosition(), m_MyEntity.GetOrigin());
	}
	
	protected bool UpdateDriverAndTarget(AIAgent owner)
	{
		if (!m_MyVehicle)
			return false;
		IEntity driverEntity = m_MyVehicle.GetPilot();
		if (!driverEntity)
			return false;
		AIControlComponent controlComp = AIControlComponent.Cast(driverEntity.FindComponent(AIControlComponent));
		if (!controlComp)
			return false;
		AIAgent driverAgent = controlComp.GetAIAgent();
		if (!driverAgent)
			return false;
		m_DriverUtility = SCR_AIUtilityComponent.Cast(driverAgent.FindComponent(SCR_AIUtilityComponent));
		if (!m_DriverUtility)
			return false;
				
		m_DriverState = m_DriverUtility.m_CombatMoveState;
		
		GetVariableIn(PORT_SUPPRESSION_VOLUME, m_SuppressionVolume);
		
		if (!m_SuppressionVolume)
			return false;
		
		return true;
	}
	
	//--------------------------------------------------------------------------------------------
	protected bool MoveFromTargetNewRequestCondition()
	{
		if (!m_DriverState.IsExecutingRequest())
			return true;
		
		// Still executing ...
		// Send new request only if we are executing NOT move_from_target
		SCR_AICombatMoveRequest_Move rq = SCR_AICombatMoveRequest_Move.Cast(m_DriverState.GetRequest());
		if (!rq)
			return true;
		
		return rq.m_eReason != SCR_EAICombatMoveReason.MOVE_FROM_TARGET;
	}
	
	protected vector ResolveRequestTargetPos()
	{
		return m_SuppressionVolume.GetCenterPosition();
	}
	
	//--------------------------------------------------------------------------------------------
	// Movement
	
	protected void PushRequestMove()
	{		
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
		
		// Common values
		rq.m_vTargetPos = ResolveRequestTargetPos();
		ResolveMoveRequestMovePosAndDir(rq.m_vTargetPos, rq.m_vMovePos, rq.m_eDirection);
		rq.m_bTryFindCover = false;
		rq.m_bUseCoverSearchDirectivity = false;
		rq.m_bCheckCoverVisibility = false;
		
		
		float moveDistanceMax = MAX_MOVE_STEP_TO_TARGET;
		
		// Long range combat
		
		switch (m_eThreatState)
		{
			case EAIThreatState.EXHAUSTED:
			{
				moveDistanceMax = MAX_MOVE_STEP_TO_TARGET_THREATENED;				
				break;
			}
			default:
			{
				moveDistanceMax = MAX_MOVE_STEP_TO_TARGET;				
				break;
			}
		}
		
		rq.m_eMovementType = EMovementType.SPRINT;
		rq.m_bAimAtTarget = false; 
		rq.m_bAimAtTargetEnd = true; // turn towards the target should be true!
		rq.m_bFailIfNoCover = false;
		
		rq.m_fMoveDuration_s = Math.RandomFloat(3.0, 10.5); // Move distance randomized
		
		// Subscribe to events
		// We will pronounce voice lines once we start or end moving
		rq.GetOnMovementStarted().Insert(OnMovementStarted);
		rq.GetOnCompleted().Insert(OnMovementCompleted);
		
		m_DriverState.ApplyNewRequest(rq);
	}
	
	protected static void OnMovementStarted(SCR_AIUtilityComponent utility, SCR_AICombatMoveRequest_Move rq, vector pos, bool destinationIsCover)
	{
		if (!utility.m_CommsHandler.CanBypass())
		{
			SCR_AITalkRequest talkRq = new SCR_AITalkRequest(ECommunicationType.REPORT_MOVING, null, vector.Zero, 0, false, false, SCR_EAITalkRequestPreset.IRRELEVANT_IMMEDIATE);
			utility.m_CommsHandler.AddRequest(talkRq);
		}
	}
	
	//--------------------------------------------------------------------------------------------
	protected static void OnMovementCompleted(SCR_AIUtilityComponent utility, SCR_AICombatMoveRequestBase rq)
	{		
		if (!utility.m_CommsHandler.CanBypass())
		{
			SCR_AITalkRequest talkRq = new SCR_AITalkRequest(ECommunicationType.REPORT_COVERING, null, vector.Zero, 0, false, false, SCR_EAITalkRequestPreset.IRRELEVANT_IMMEDIATE);
			utility.m_CommsHandler.AddRequest(talkRq);
		}
	}
	
	//--------------------------------------------------------------------------------------------
	// Resolves which move pos and dir. we should use for _MOVE_ request
	// By now rq.m_vTargetPos must be already calculated!
	protected void ResolveMoveRequestMovePosAndDir(vector targetPos, out vector outMovePos, out SCR_EAICombatMoveDirection outDirection)
	{	
		AIWaypoint wp = null;
		AIAgent agent = m_DriverUtility.GetAIAgent();
		AIGroup group = agent.GetParentGroup();
		if (group)
			wp = group.GetCurrentWaypoint();
		
		vector movePos;
		SCR_EAICombatMoveDirection eDirection;		
		
		movePos = targetPos;
			switch(m_eThreatState)
			{
				case EAIThreatState.EXHAUSTED:
				{
					int rand = Math.RandomIntInclusive(1, 3);
					if (rand == 2)
						eDirection = SCR_EAICombatMoveDirection.LEFT;
					else if (rand == 1)
						eDirection = SCR_EAICombatMoveDirection.RIGHT;
					else
						eDirection = SCR_EAICombatMoveDirection.FORWARD;
					break;
				}
				case EAIThreatState.PINNED:
				{
					eDirection = SCR_EAICombatMoveDirection.FORWARD;
					break;
				}	
				case EAIThreatState.THREATENED:
				{
					eDirection = SCR_EAICombatMoveDirection.FORWARD;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					eDirection = SCR_EAICombatMoveDirection.FORWARD;
					break;
				}
				case EAIThreatState.VIGILANT:
				{
					eDirection = SCR_EAICombatMoveDirection.FORWARD;
					break;
				}
				case EAIThreatState.SAFE:
				{
					eDirection = SCR_EAICombatMoveDirection.FORWARD;
					break;
				}
			}		
		outMovePos = movePos;
		outDirection = eDirection;		
	}

	//--------------------------------------------------------------------------------------------
	protected float ResolveStoppedWaitTime(EAIThreatState threat)
	{
		float waitTime;
		
		// based on threat we wait longer
			switch(m_eThreatState)
			{
				case EAIThreatState.EXHAUSTED:
				{
					waitTime = Math.RandomFloat(5.0, 12.0);
					break;
				}
				case EAIThreatState.PINNED:
				{
					waitTime = Math.RandomFloat(14.0, 25.0);
					break;
				}	
				case EAIThreatState.THREATENED:
				{
					waitTime = Math.RandomFloat(15.0, 30.0);
					break;
				}
				case EAIThreatState.ALERTED:
				{
					waitTime = Math.RandomFloat(10.0, 15.0);
					break;
				}
				case EAIThreatState.VIGILANT:
				{
					waitTime = Math.RandomFloat(15.0, 35.0);
					break;
				}
				case EAIThreatState.SAFE:
				{
					waitTime = Math.RandomFloat(9.0, 40.0);
					break;
				}
			}	
				
		return waitTime;
	}
	
	protected bool IsFirstExecution()
	{
		return !m_DriverState.GetRequest();
	}
	
	//--------------------------------------------------------------------------------------------
	protected bool MoveToNextPosCondition()
	{				
		if (m_DriverState.IsExecutingRequest())
			return false;
		
		if (IsFirstExecution())
			return true;
		
		if (m_DriverState.m_fTimerStopped_s > 30)
			return true;
			
		float stoppedWaitTime = ResolveStoppedWaitTime(m_eThreatState);	
		return m_DriverState.m_fTimerStopped_s > stoppedWaitTime;
			
		return false;
	}
	
	//--------------------------------------------------------------------------------------------
	override bool VisibleInPalette() { return true; }
	
	protected static ref TStringArray s_aVarsIn = {
		PORT_VISIBLE,
		PORT_TIME_LAST_SEEN,
		PORT_SUPPRESSION_VOLUME
	};
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
}

class DCO_AITravelCommanderMove : AITaskScripted
{	
	// Inputs
	protected static const string AREA_PORT = "Area Pos";
	protected BaseTarget m_Target;
	protected SCR_AIUtilityComponent m_GunnerUtility;
	protected SCR_AICombatComponent m_CombatComp;
	protected SCR_CompartmentAccessComponent m_CompartmentAccessComponent;
	protected BaseWeaponManagerComponent m_WeaponManagerComponent;
	protected TurretComponent m_TurretComponent;
	protected IEntity m_MyEntity;
	protected Vehicle m_MyVehicle;
	
	protected vector target;
	
	protected SCR_AICombatMoveState m_DriverState;
	protected SCR_AIUtilityComponent m_DriverUtility;
	protected const float WEAPON_MIN_DIST = 2.0;
	
	// minimal distance Driver should be from enemy
	protected const float MIN_ENGAGEMENT_DISTANCE_TO_TARGET_SQ = 60.0 * 60.0;	// when too close to target, try to move backwards
	protected const float MAX_MOVE_STEP_TO_TARGET = 50.0; 						// max step to move towards target
	protected const float MAX_MOVE_STEP_TO_TARGET_THREATENED = 10.0; 			// move less under threat
	protected const float MIN_MOVE_STEP_TARGET = 5.0; 							// min step to move from target
	
	// Values updated on each update, to avoid passing them through calls
	protected EAIThreatState m_eThreatState;
	protected EWeaponType m_eWeaponType;
	protected float m_fTargetDist;
	protected float m_fWeaponMinDist = WEAPON_MIN_DIST;
	
	[Attribute("500", UIWidgets.EditBox, "Update interval of the node")]
	protected float m_fUpdateInterval_ms;
	protected float m_fNextUpdate_ms;
	
	//--------------------------------------------------------------------------------------------
	protected override void OnInit(AIAgent owner)
	{
		m_GunnerUtility = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
		m_MyEntity = owner.GetControlledEntity();

		if (m_MyEntity)
		{
			m_CompartmentAccessComponent = SCR_CompartmentAccessComponent.Cast(m_MyEntity.FindComponent(SCR_CompartmentAccessComponent));
			m_CombatComp = SCR_AICombatComponent.Cast(m_MyEntity.FindComponent(SCR_AICombatComponent));		
			
			if (m_CompartmentAccessComponent && m_CompartmentAccessComponent.IsInCompartment())
			{
				IEntity turretEnt = m_CompartmentAccessComponent.GetCompartment().GetOwner();
				if (turretEnt)
				{
					TurretControllerComponent contr = TurretControllerComponent.Cast(turretEnt.FindComponent(TurretControllerComponent));
					if (contr)
						m_TurretComponent = contr.GetTurretComponent();
					m_WeaponManagerComponent = BaseWeaponManagerComponent.Cast(turretEnt.FindComponent(BaseWeaponManagerComponent));
				}	
				m_MyVehicle = Vehicle.Cast(m_CompartmentAccessComponent.GetVehicle());	
			}	
		}
	}
	
	//--------------------------------------------------------------------------------------------
	protected override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		float currentTime_ms = GetGame().GetWorld().GetWorldTime();
		if (currentTime_ms < m_fNextUpdate_ms)
			return ENodeResult.RUNNING;
		m_fNextUpdate_ms = currentTime_ms + m_fUpdateInterval_ms;
		
		GetVariableIn(AREA_PORT, target);
		
		if (!UpdateDriverAndTarget(owner))
			return ENodeResult.FAIL;
		
		if (!m_DriverState || !m_MyEntity || !m_GunnerUtility)
			return ENodeResult.FAIL;
		
		if (!m_GunnerUtility.m_AIInfo.HasUnitState(EUnitState.IN_TURRET) || m_DriverUtility.m_AIInfo.HasUnitState(EUnitState.UNCONSCIOUS))
			return ENodeResult.RUNNING;
				
		SCR_AIBehaviorBase executedBehavior = SCR_AIBehaviorBase.Cast(m_DriverUtility.GetExecutedAction());
		if (executedBehavior && !executedBehavior.m_bUseCombatMove)
			return ENodeResult.RUNNING;
		
		// Update cached variables
		m_fTargetDist = GetTargetDistance();		
		m_eThreatState = m_GunnerUtility.m_ThreatSystem.GetState();
		m_fWeaponMinDist = 2.0;
		m_eWeaponType = m_WeaponManagerComponent.GetCurrentWeapon().GetWeaponType();		

		if (MoveToNextPosCondition())
		{
			// We've waited here too long, move to next place
			PushRequestMove();
		}
		
		return ENodeResult.RUNNING;
	}
	
	protected void PushRequestMove()
	{		
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
		
		// Common values
		rq.m_vTargetPos = ResolveRequestTargetPos();
		ResolveMoveRequestMovePosAndDir(rq.m_vTargetPos, rq.m_vMovePos, rq.m_eDirection);
		rq.m_bTryFindCover = false;
		rq.m_bUseCoverSearchDirectivity = false;
		rq.m_bCheckCoverVisibility = false;
		
		
		float moveDistanceMax = MAX_MOVE_STEP_TO_TARGET;
		
		// Long range combat
		
		switch (m_eThreatState)
		{
			case EAIThreatState.THREATENED:
			{
				moveDistanceMax = MAX_MOVE_STEP_TO_TARGET_THREATENED;				
				break;
			}
			default:
			{
				moveDistanceMax = MAX_MOVE_STEP_TO_TARGET;				
				break;
			}
		}
		
		rq.m_eMovementType = EMovementType.SPRINT;
		rq.m_bAimAtTarget = false; 
		rq.m_bAimAtTargetEnd = true; // turn towards the target should be true!
		rq.m_bFailIfNoCover = false;
		
		rq.m_fMoveDuration_s = Math.RandomFloat(2.0, 7.0); // Move distance randomized
		
		// Subscribe to events
		// We will pronounce voice lines once we start or end moving
		rq.GetOnMovementStarted().Insert(OnMovementStarted);
		rq.GetOnCompleted().Insert(OnMovementCompleted);
		
		m_DriverState.ApplyNewRequest(rq);
	}
	
	//--------------------------------------------------------------------------------------------
	protected static void OnMovementStarted(SCR_AIUtilityComponent utility, SCR_AICombatMoveRequest_Move rq, vector pos, bool destinationIsCover)
	{
		if (!utility.m_CommsHandler.CanBypass())
		{
			SCR_AITalkRequest talkRq = new SCR_AITalkRequest(ECommunicationType.REPORT_MOVING, null, vector.Zero, 0, false, false, SCR_EAITalkRequestPreset.IRRELEVANT_IMMEDIATE);
			utility.m_CommsHandler.AddRequest(talkRq);
		}
	}
	
	//--------------------------------------------------------------------------------------------
	protected static void OnMovementCompleted(SCR_AIUtilityComponent utility, SCR_AICombatMoveRequestBase rq)
	{
		if (!utility.m_CommsHandler.CanBypass())
		{
			SCR_AITalkRequest talkRq = new SCR_AITalkRequest(ECommunicationType.REPORT_COVERING, null, vector.Zero, 0, false, false, SCR_EAITalkRequestPreset.IRRELEVANT_IMMEDIATE);
			utility.m_CommsHandler.AddRequest(talkRq);
		}
	}
	
	//--------------------------------------------------------------------------------------------
	// Resolves which move pos and dir. we should use for _MOVE_ request
	// By now rq.m_vTargetPos must be already calculated!
	protected void ResolveMoveRequestMovePosAndDir(vector targetPos, out vector outMovePos, out SCR_EAICombatMoveDirection outDirection)
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

			
	//--------------------------------------------------------------------------------------------
	protected bool UpdateDriverAndTarget(AIAgent owner)
	{
		if (!m_MyVehicle)
			return false;
		IEntity driverEntity = m_MyVehicle.GetPilot();
		if (!driverEntity)
			return false;
		AIControlComponent controlComp = AIControlComponent.Cast(driverEntity.FindComponent(AIControlComponent));
		if (!controlComp)
			return false;
		AIAgent driverAgent = controlComp.GetAIAgent();
		if (!driverAgent)
			return false;
		m_DriverUtility = SCR_AIUtilityComponent.Cast(driverAgent.FindComponent(SCR_AIUtilityComponent));
		if (!m_DriverUtility)
			return false;
				
		m_DriverState = m_DriverUtility.m_CombatMoveState;
		
		if (!m_Target || !m_Target.GetTargetEntity())
			return false;
		
		return true;
	}
	
	//--------------------------------------------------------------------------------------------
	protected float GetTargetDistance()
	{
		return m_Target.GetDistance();
	}
	
	//--------------------------------------------------------------------------------------------
	protected vector ResolveRequestTargetPos()
	{
		return target;		
	}
	
	//--------------------------------------------------------------------------------------------
	protected float ResolveStoppedWaitTime(EAIThreatState threat)
	{
		float waitTime;
				
		waitTime = Math.RandomFloat(3.0, 7.0);
		return waitTime;
	}
	
	//--------------------------------------------------------------------------------------------
	protected bool MoveToNextPosCondition()
	{
		if (m_DriverState.IsExecutingRequest())
			return false;
		
		if (m_DriverState.m_fTimerStopped_s > 10)
			return true;
		
		float stoppedWaitTime = ResolveStoppedWaitTime(m_eThreatState);	
		return m_DriverState.m_fTimerStopped_s > stoppedWaitTime;
	}

	
	//--------------------------------------------------------------------------------------------
	// Returns true if it's first of combat movement logic. Doesn't mean first execution of this node.
	protected bool IsFirstExecution()
	{
		return !m_DriverState.GetRequest();
	}

	//--------------------------------------------------------------------------------------------
	override bool VisibleInPalette() { return true; }
	
	protected static ref TStringArray s_aVarsIn = {
		AREA_PORT
	};
	override TStringArray GetVariablesIn() { return s_aVarsIn; }
}