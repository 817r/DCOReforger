modded class SCR_AIDangerReaction_ProjectileHit : SCR_AIDangerReaction
{
	protected static const float BULLET_IMPACT_DISTANCE_SQ_MAX = 3*3;
	protected SCR_AICombatMoveState m_State;
	protected SCR_AICombatComponent m_CombatComp;
	protected static const float COVER_SEARCH_DIST_MAX = 15.0;
	protected const float COVER_QUERY_SECTOR_ANGLE_RAD = 0.35 * Math.PI;
	protected bool m_bPushedMoveRequest = false;
	
	
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		

		float distanceSq = vector.DistanceSq(utility.GetOrigin(), dangerEvent.GetPosition());
				
		if (distanceSq > BULLET_IMPACT_DISTANCE_SQ_MAX)
			return false;
		
		threatSystem.ThreatBulletImpact(dangerEventCount);
		IEntity shooter = dangerEvent.GetObject();
		
		if (!shooter)
			return false;
		
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
		
		int bulletCount = dangerEventCount;
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
			rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * 5;
			int rand = Math.RandomIntInclusive(1,3);
			if (rand == 1) rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
			else if (rand == 2) rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
			else if (rand == 3)	rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
			rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
			rq.m_bAimAtTarget = true; // Don't aim while running
			rq.m_bAimAtTargetEnd = true;
			
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
				rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * 5;
				int rand = Math.RandomIntInclusive(1,3);
				if (rand == 1) rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
				else if (rand == 2) rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
				else if (rand == 3)	rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
				rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
				rq.m_bAimAtTarget = true; // Don't aim while running
				rq.m_bAimAtTargetEnd = true;
			
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
				rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * 4;
				int rand = Math.RandomIntInclusive(1,3);
				if (rand == 1) rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
				else if (rand == 2) rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
				else if (rand == 3)	rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
				rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
				rq.m_bAimAtTarget = true; // Don't aim while running
				rq.m_bAimAtTargetEnd = true;
			
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
			rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * 5;
			rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
			rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
			rq.m_bAimAtTarget = true; // Don't aim while running
			rq.m_bAimAtTargetEnd = true;
			
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
							rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * COVER_SEARCH_DIST_MAX;
							rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
							rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
							rq.m_bAimAtTarget = false; // Don't aim while running
							rq.m_bAimAtTargetEnd = true;
			
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
							rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * COVER_SEARCH_DIST_MAX;
							rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
							rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
							rq.m_bAimAtTarget = true; // Don't aim while running
							rq.m_bAimAtTargetEnd = true;
			
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
							rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * COVER_SEARCH_DIST_MAX;
							rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
							rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
							rq.m_bAimAtTarget = false; // Don't aim while running
							rq.m_bAimAtTargetEnd = true;
			
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