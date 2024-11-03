/*modded class SCR_AIDangerReaction_ProjectileHit : SCR_AIDangerReaction
{
	protected static const float BULLET_IMPACT_DISTANCE_MAX_SQ = 10*10;
	
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent)
	{
		float distanceSq = vector.DistanceSq(utility.GetOrigin(), dangerEvent.GetPosition());
		
		IEntity shooter = dangerEvent.GetObject();
		
		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
		IEntity shooterRoot = shooter.GetRootParent();
		if (!agent || !agent.IsEnemy(shooterRoot))
			return false;
		
		vector shooterPos = shooter.GetOrigin();
		
		float distanceToShooter = vector.Distance(utility.GetOrigin(), shooterPos);
		
		if (utility.m_CombatComponent.GetCurrentTarget() == null && distanceToShooter > SCR_AICombatComponent.LONG_RANGE_FIRE_DISTANCE && shooter)
			utility.AddAction(new SCR_AIMoveFromUnknownFire(utility, null, shooterPos, shooter));
		
		threatSystem.ThreatBulletImpact(dangerEvent.GetCount());
		
		float distanceDanger = vector.Distance(utility.GetOrigin(), dangerEvent.GetPosition());
		
		int bulletFly = dangerEvent.GetCount();
		
		if (shooter)
		{			
			if (agent && agent.IsEnemy(shooter))
			{
				if (distanceDanger < 2 && bulletFly > 5)
				{
					int randomChances = Math.RandomInt(1,10);
					
					if (randomChances > 4)
						utility.AddAction(new SCR_AIMoveFromDangerBehavior(utility, null, shooterPos, shooter));
					
				}
			}
		
		}
		
		return true;
	}	
};*/

[BaseContainerProps()]
modded class SCR_AIDangerReaction_ProjectileHit : SCR_AIDangerReaction
{
	protected static const float BULLET_IMPACT_DISTANCE_SQ_MAX = 3*3;
	protected SCR_AICombatMoveState m_State;
	protected SCR_AICombatComponent m_CombatComp;
	protected static const float COVER_SEARCH_DIST_MAX = 15.0;
	protected const float COVER_QUERY_SECTOR_ANGLE_RAD = 0.35 * Math.PI;
	protected bool m_bPushedMoveRequest = false;
	
	
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent)
	{
		if (utility.m_AIInfo.GetAIState() == EUnitState.PILOT)
			return false;
		
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		float distanceSq = vector.DistanceSq(utility.GetOrigin(), dangerEvent.GetPosition());
				
		if (distanceSq > BULLET_IMPACT_DISTANCE_SQ_MAX)
			return false;
		
		if (utility.m_CombatMoveState.IsExecutingRequest())
			return false;
		
		threatSystem.ThreatBulletImpact(dangerEvent.GetCount());
		IEntity shooter = dangerEvent.GetObject();
		
		if (!shooter)
			return false;
		
		DCO_CUSTOMRANK rank = utility.getRanks();
		moraleState morale = utility.m_DCOMoraleSystem.GetState();
		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
		IEntity shooterRoot = shooter.GetRootParent();
		vector shooterPos = shooter.GetOrigin();
		float shooterDistance = vector.Distance(utility.GetOrigin(), shooterRoot.GetOrigin());
		float distanceToDanger = vector.Distance(utility.GetOrigin(), dangerEvent.GetPosition());
		bool isNullTarget = utility.m_CombatComponent.GetCurrentTarget() == null;
		m_State = utility.m_CombatMoveState;
		
		if (!agent || !agent.IsEnemy(shooterRoot))
			return false;

		float distanceToShooter = vector.Distance(utility.GetOrigin(), shooterPos);
		
		int bulletCount = dangerEvent.GetCount();
		rq.m_eReason = SCR_EAICombatMoveReason.SUPPRESSED_IN_COVER;		
		
		if(distanceToDanger < 1 && !m_State.IsInValidCover() && bulletCount >= 3)
		{
			rq.m_vTargetPos = shooterPos;
			rq.m_vMovePos = rq.m_vTargetPos;
			rq.m_bTryFindCover = true;
			rq.m_bUseCoverSearchDirectivity = true;
			rq.m_bCheckCoverVisibility = true;
			rq.m_bFailIfNoCover = false;
			rq.m_eStanceMoving = ECharacterStance.STAND;
			rq.m_eStanceEnd = ECharacterStance.CROUCH;
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_fCoverSearchDistMax = 12;
			rq.m_fCoverSearchDistMin = 5;
			rq.m_fMoveDistance = Math.RandomFloat(1.0, 1.5) * 5;
			int rand = Math.RandomIntInclusive(1,3);
			if (rand == 1) rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
			else if (rand == 2) rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
			else if (rand == 3)	rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
			rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
			rq.m_bAimAtTarget = true; // Don't aim while running
			rq.m_bAimAtTargetEnd = true;
			
			if (SCR_AICompartmentHandling.IsInCompartment(utility.GetAIAgent()))
			{
				rq.m_bTryFindCover = false;
				rq.m_bUseCoverSearchDirectivity = false;
				rq.m_bCheckCoverVisibility = false;
				rq.m_eMovementType = EMovementType.SPRINT;
				rq.m_fMoveDistance = Math.RandomFloat(1.0, 1.5) * 35;
				rq.m_bAimAtTarget = false;
				rq.m_bAimAtTargetEnd = false;
			}
			
			m_State.ApplyNewRequest(rq);
			m_bPushedMoveRequest = true;

			return true;	
		}
		
		if (m_State.IsInValidCover() && shooterDistance > 10 && bulletCount >= 4)
		{
			int randomizer = Math.RandomIntInclusive(1,2);
			
			if (randomizer == 1 && m_State.m_bExposedInCover)
			{
				ApplyRequestChangeStanceInCover(true);
				return true;
			} else if (randomizer == 1)
			{
				rq.m_vTargetPos = shooterPos;
				rq.m_vMovePos = rq.m_vTargetPos;
				rq.m_bTryFindCover = true;
				rq.m_bUseCoverSearchDirectivity = true;
				rq.m_bCheckCoverVisibility = true;
				rq.m_bFailIfNoCover = true;
				rq.m_eStanceMoving = ECharacterStance.STAND;
				rq.m_eStanceEnd = ECharacterStance.CROUCH;
				rq.m_eMovementType = EMovementType.RUN;
				rq.m_fCoverSearchDistMax = 12;
				rq.m_fCoverSearchDistMin = 5;
				rq.m_fMoveDistance = Math.RandomFloat(1.0, 1.5) * 5;
				int rand = Math.RandomIntInclusive(1,3);
				if (rand == 1) rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
				else if (rand == 2) rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
				else if (rand == 3)	rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
				rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
				rq.m_bAimAtTarget = true; // Don't aim while running
				rq.m_bAimAtTargetEnd = true;
				
				if (SCR_AICompartmentHandling.IsInCompartment(utility.GetAIAgent()))
				{
					rq.m_bTryFindCover = false;
					rq.m_bUseCoverSearchDirectivity = false;
					rq.m_bCheckCoverVisibility = false;
					rq.m_eMovementType = EMovementType.SPRINT;
					rq.m_fMoveDistance = Math.RandomFloat(1.0, 1.5) * 35;
					rq.m_bAimAtTarget = false;
					rq.m_bAimAtTargetEnd = false;
				}
				m_State.ApplyNewRequest(rq);
				m_bPushedMoveRequest = true;
			
				return true;			
			} else if (randomizer == 2)
			{
				rq.m_vTargetPos = shooterPos;
				rq.m_vMovePos = rq.m_vTargetPos;
				rq.m_bTryFindCover = true;
				rq.m_bUseCoverSearchDirectivity = true;
				rq.m_bCheckCoverVisibility = true;
				rq.m_bFailIfNoCover = true;
				rq.m_eStanceMoving = ECharacterStance.CROUCH;
				rq.m_eStanceEnd = ECharacterStance.PRONE;
				rq.m_eMovementType = EMovementType.WALK;
				rq.m_fCoverSearchDistMax = 12;
				rq.m_fCoverSearchDistMin = 5;
				rq.m_fMoveDistance = Math.RandomFloat(1.0, 1.5) * 4;
				int rand = Math.RandomIntInclusive(1,3);
				if (rand == 1) rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
				else if (rand == 2) rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
				else if (rand == 3)	rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
				rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
				rq.m_bAimAtTarget = true; // Don't aim while running
				rq.m_bAimAtTargetEnd = true;
				
				if (SCR_AICompartmentHandling.IsInCompartment(utility.GetAIAgent()))
				{
					rq.m_bTryFindCover = false;
					rq.m_bUseCoverSearchDirectivity = false;
					rq.m_bCheckCoverVisibility = false;
					rq.m_eMovementType = EMovementType.SPRINT;
					rq.m_fMoveDistance = Math.RandomFloat(1.0, 1.5) * 35;
					rq.m_bAimAtTarget = false;
					rq.m_bAimAtTargetEnd = false;
				}
				m_State.ApplyNewRequest(rq);
				m_bPushedMoveRequest = true;
			
				return true;			
			}
		}
		
		if (!m_State.IsInValidCover() && shooterDistance > 10 && distanceToDanger < 5 && bulletCount > 3)
		{
			rq.m_vTargetPos = shooterPos;
			rq.m_vMovePos = rq.m_vTargetPos;
			rq.m_bTryFindCover = true;
			rq.m_bUseCoverSearchDirectivity = true;
			rq.m_bCheckCoverVisibility = true;
			rq.m_bFailIfNoCover = false;
			rq.m_eStanceMoving = ECharacterStance.STAND;
			rq.m_eStanceEnd = ECharacterStance.CROUCH;
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_fCoverSearchDistMax = 12;
			rq.m_fCoverSearchDistMin = 5;
			rq.m_fMoveDistance = Math.RandomFloat(1.0, 1.5) * 5;
			rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
			rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
			rq.m_bAimAtTarget = true; // Don't aim while running
			rq.m_bAimAtTargetEnd = true;
			if (SCR_AICompartmentHandling.IsInCompartment(utility.GetAIAgent()))
			{
				rq.m_bTryFindCover = false;
				rq.m_bUseCoverSearchDirectivity = false;
				rq.m_bCheckCoverVisibility = false;
				rq.m_eMovementType = EMovementType.SPRINT;
				rq.m_fMoveDistance = Math.RandomFloat(1.0, 1.5) * 35;
				rq.m_bAimAtTarget = false;
				rq.m_bAimAtTargetEnd = false;
			}
			m_State.ApplyNewRequest(rq);
			m_bPushedMoveRequest = true;
			
			return true;
		}
			
		
		if (shooter)
		{
			if (agent && agent.IsEnemy(shooter))
			{												
				if (distanceToDanger < 1 && bulletCount > 5)
				{
					if(m_State != null)
					{					
							rq.m_vTargetPos = shooterPos;
							rq.m_vMovePos = rq.m_vTargetPos;
							rq.m_bTryFindCover = true;
							rq.m_bUseCoverSearchDirectivity = true;
							rq.m_bCheckCoverVisibility = true;
							rq.m_bFailIfNoCover = false;
							rq.m_eStanceMoving = ECharacterStance.STAND;
							rq.m_eStanceEnd = ECharacterStance.CROUCH;
							rq.m_eMovementType = EMovementType.SPRINT;
							rq.m_fCoverSearchDistMax = COVER_SEARCH_DIST_MAX;
							rq.m_fCoverSearchDistMin = 2;
							rq.m_fMoveDistance = Math.RandomFloat(1.0, 1.5) * COVER_SEARCH_DIST_MAX;
							rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
							rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
							rq.m_bAimAtTarget = false; // Don't aim while running
							rq.m_bAimAtTargetEnd = true;
							if (SCR_AICompartmentHandling.IsInCompartment(utility.GetAIAgent()))
							{
								rq.m_bTryFindCover = false;
								rq.m_bUseCoverSearchDirectivity = false;
								rq.m_bCheckCoverVisibility = false;
								rq.m_eMovementType = EMovementType.SPRINT;
								rq.m_fMoveDistance = Math.RandomFloat(1.0, 1.5) * 35;
								rq.m_bAimAtTarget = false;
								rq.m_bAimAtTargetEnd = false;
							}
							m_State.ApplyNewRequest(rq);
							m_bPushedMoveRequest = true;

							return true;		
						
					}
					
					return true;
				}
				else if (distanceToDanger < 1 && distanceToShooter < 5 && bulletCount > 5)
				{
					if(m_State != null)
					{
											
							rq.m_vTargetPos = shooterPos;
							rq.m_vMovePos = rq.m_vTargetPos;
							rq.m_bTryFindCover = true;
							rq.m_bUseCoverSearchDirectivity = true;
							rq.m_bCheckCoverVisibility = true;
							rq.m_bFailIfNoCover = false;
							rq.m_eStanceMoving = ECharacterStance.STAND;
							rq.m_eStanceEnd = ECharacterStance.CROUCH;
							rq.m_eMovementType = EMovementType.RUN;
							rq.m_fCoverSearchDistMax = COVER_SEARCH_DIST_MAX;
							rq.m_fCoverSearchDistMin = 2;
							rq.m_fMoveDistance = Math.RandomFloat(1.0, 1.5) * COVER_SEARCH_DIST_MAX;
							rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
							rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
							rq.m_bAimAtTarget = true; // Don't aim while running
							rq.m_bAimAtTargetEnd = true;
							if (SCR_AICompartmentHandling.IsInCompartment(utility.GetAIAgent()))
							{
								rq.m_bTryFindCover = false;
								rq.m_bUseCoverSearchDirectivity = false;
								rq.m_bCheckCoverVisibility = false;
								rq.m_eMovementType = EMovementType.SPRINT;
								rq.m_fMoveDistance = Math.RandomFloat(1.0, 1.5) * 35;
								rq.m_bAimAtTarget = false;
								rq.m_bAimAtTargetEnd = false;
							}
							m_State.ApplyNewRequest(rq);
							m_bPushedMoveRequest = true;

							return true;		
						
					}
				}
				else if (isNullTarget && distanceToDanger < 3)
				{
					if(m_State != null)
					{						
							rq.m_vTargetPos = shooterPos;
							rq.m_vMovePos = rq.m_vTargetPos;
							rq.m_bTryFindCover = true;
							rq.m_bUseCoverSearchDirectivity = true;
							rq.m_bCheckCoverVisibility = true;
							rq.m_bFailIfNoCover = false;
							rq.m_eStanceMoving = ECharacterStance.STAND;
							rq.m_eStanceEnd = ECharacterStance.PRONE;
							rq.m_eMovementType = EMovementType.SPRINT;
							rq.m_fCoverSearchDistMax = COVER_SEARCH_DIST_MAX;
							rq.m_fCoverSearchDistMin = 2;
							rq.m_fMoveDistance = Math.RandomFloat(1.0, 1.5) * COVER_SEARCH_DIST_MAX;
							rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
							rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
							rq.m_bAimAtTarget = false; // Don't aim while running
							rq.m_bAimAtTargetEnd = true;
							if (SCR_AICompartmentHandling.IsInCompartment(utility.GetAIAgent()))
							{
								rq.m_bTryFindCover = false;
								rq.m_bUseCoverSearchDirectivity = false;
								rq.m_bCheckCoverVisibility = false;
								rq.m_eMovementType = EMovementType.SPRINT;
								rq.m_fMoveDistance = Math.RandomFloat(1.0, 1.5) * 35;
								rq.m_bAimAtTarget = false;
								rq.m_bAimAtTargetEnd = false;
							}
							m_State.ApplyNewRequest(rq);
							m_bPushedMoveRequest = true;

							return true;		
					}
				}
				return true;
			}
		}
		
		return true;
	}
	
	void ApplyRequestChangeStanceInCover(bool exposed)
	{
		SCR_AICombatMoveRequest_ChangeStanceInCover rq = new SCR_AICombatMoveRequest_ChangeStanceInCover();
		
		rq.m_bExposedInCover = exposed;
		rq.m_bAimAtTarget = true;
		rq.m_bAimAtTargetEnd = true;
		
		m_State.ApplyNewRequest(rq);
	}
	
};

[BaseContainerProps()]
modded class SCR_AIDangerReaction_Explosion : SCR_AIDangerReaction
{
	private static const float EXPLOSION_OBSERVE_DISTANCE = 30; // Maximal distance from explosion to trigger observe behavior
	private static const float EXPLOSION_REACTION = 10;
	protected const float COVER_QUERY_SECTOR_ANGLE_RAD = 0.35 * Math.PI;
	
	override void CreateObserveUnknownBehavior(SCR_AIUtilityComponent utility, vector observeReactionPosition)
	{
		if (observeReactionPosition == vector.Zero || utility.m_CombatComponent.GetCurrentTarget() != null)
			return;
				
		vector myOrigin = utility.m_OwnerEntity.GetOrigin();
								
		SCR_AIMoveAndInvestigateBehavior investigateBehavior = SCR_AIMoveAndInvestigateBehavior.Cast(utility.FindActionOfType(SCR_AIMoveAndInvestigateBehavior));
		SCR_AIObserveUnknownFireBehavior oldObserveBehavior = SCR_AIObserveUnknownFireBehavior.Cast(utility.FindActionOfType(SCR_AIObserveUnknownFireBehavior));
		
		// Exit if investigating
		if (investigateBehavior && investigateBehavior.GetActionState() == EAIActionState.RUNNING)
			return;
		
		// Exit if already observing something else
		if (oldObserveBehavior)
			return;
		
		SCR_AIObserveUnknownFireBehavior observeBehavior = new SCR_AIObserveUnknownFireBehavior(utility, null, posWorld: observeReactionPosition, useMovement: true);
		utility.AddAction(observeBehavior);
	}
	
	//------------------------------------------------------------------------------------------------
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent)
	{
		if (utility.m_AIInfo.GetAIState() == EUnitState.PILOT)
			return false;
		
		IEntity ownerEntity = utility.m_OwnerEntity;
		protected bool m_bPushedMoveRequest = false;
		
		if (!ownerEntity)
			return false;
		
		protected SCR_AICombatMoveState m_State;
		vector position = dangerEvent.GetPosition();	
		float distance = vector.Distance(ownerEntity.GetOrigin(), position);
		m_State = utility.m_CombatMoveState;
		
		if (distance > SCR_AIThreatSystem.EXPLOSION_MAX_DISTANCE)
			return false;
		
		IEntity instigatorRoot = dangerEvent.GetObject();
		if (instigatorRoot)
		{
			instigatorRoot = instigatorRoot.GetRootParent();
			bool isMilitary = utility.IsMilitary();
			SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
			if (isMilitary && (!agent || !agent.IsEnemy(instigatorRoot)))
				return false;
		}
		
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
		
		if (distance < EXPLOSION_REACTION)
		{
			rq.m_vTargetPos = position;
			rq.m_vMovePos = rq.m_vTargetPos;
			rq.m_bTryFindCover = true;
			rq.m_bUseCoverSearchDirectivity = true;
			rq.m_bCheckCoverVisibility = true;
			rq.m_bFailIfNoCover = false;
			rq.m_eStanceMoving = ECharacterStance.STAND;
			rq.m_eStanceEnd = ECharacterStance.CROUCH;
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_fCoverSearchDistMax = 15;
			rq.m_fCoverSearchDistMin = 2;
			rq.m_fMoveDistance = Math.RandomFloat(1.0, 1.5) * 8;
			int rand = Math.RandomIntInclusive(1,3);
			if (rand == 1) rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
			else if (rand == 2) rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
			else if (rand == 3)	rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
			rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
			rq.m_bAimAtTarget = true; // Don't aim while running
			rq.m_bAimAtTargetEnd = true;
			if (SCR_AICompartmentHandling.IsInCompartment(utility.GetAIAgent()))
			{
				rq.m_bTryFindCover = false;
				rq.m_bUseCoverSearchDirectivity = false;
				rq.m_bCheckCoverVisibility = false;
				rq.m_eMovementType = EMovementType.SPRINT;
				rq.m_fMoveDistance = Math.RandomFloat(1.0, 1.5) * 35;
				rq.m_bAimAtTarget = false;
				rq.m_bAimAtTargetEnd = false;
			}
			m_State.ApplyNewRequest(rq);
			m_bPushedMoveRequest = true;
		}
			
		
		// Increase threat level
		threatSystem.ThreatExplosion(distance);
		
		// Look at explosion
		utility.m_LookAction.LookAt(position, SCR_AILookAction.PRIO_DANGER_EVENT, 1.2);
		
		// Observe if not investigating or already observing something else
		if (distance <= EXPLOSION_OBSERVE_DISTANCE)
			CreateObserveUnknownBehavior(utility, position);
		
		return true;
	}
};