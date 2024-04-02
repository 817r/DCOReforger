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
	protected static const float BULLET_IMPACT_DISTANCE_SQ_MAX = 5*5;
	protected SCR_AICombatMoveState m_State;
	protected SCR_AICombatComponent m_CombatComp;
	protected SCR_AICombatMoveLogic_Attack m_CombatLogic;
	protected static const float COVER_SEARCH_DIST_MAX = 10.0;
	protected const float COVER_QUERY_SECTOR_ANGLE_RAD = 0.35 * Math.PI;
	protected bool m_bPushedMoveRequest = false;
	
	
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent)
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;

		float distanceSq = vector.DistanceSq(utility.GetOrigin(), dangerEvent.GetPosition());
				
		if (distanceSq > BULLET_IMPACT_DISTANCE_SQ_MAX)
			return false;
		threatSystem.ThreatBulletImpact(dangerEvent.GetCount());
		IEntity shooter = dangerEvent.GetObject();
		
		if (!shooter)
			return false;
		
		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
		IEntity shooterRoot = shooter.GetRootParent();
		vector shooterPos = shooter.GetOrigin();
		float distanceToDanger = vector.Distance(utility.GetOrigin(), dangerEvent.GetPosition());
		bool isNullTarget = utility.m_CombatComponent.GetCurrentTarget() == null;
		
		if (!agent || !agent.IsEnemy(shooterRoot))
			return false;

		float distanceToShooter = vector.Distance(utility.GetOrigin(), shooterPos);
		
		int bulletCount = dangerEvent.GetCount();
		
		if (shooter)
		{
			if (agent && agent.IsEnemy(shooter))
			{
				if (distanceToDanger < 3 && bulletCount > 2)
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
						rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
						rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
						rq.m_bAimAtTarget = false; // Don't aim while running
						rq.m_bAimAtTargetEnd = true;
			
						m_State.ApplyNewRequest(rq);
						m_bPushedMoveRequest = true;	
						
						return true;
					}
					else
					{
						SCR_AIMoveFromSuppressBehavior aiMovefromSuppress = SCR_AIMoveFromSuppressBehavior.Cast(utility.FindActionOfType(SCR_AIMoveFromSuppressBehavior));
						
						if(aiMovefromSuppress) return true;
						
						utility.AddAction(new SCR_AIMoveFromSuppressBehavior(null, null, shooterPos, shooter))
					}
					
					return true;
				}
				else if (distanceToDanger < 3 && distanceToShooter < 5 || bulletCount > 4)
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
						rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
						rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
						rq.m_bAimAtTarget = false; // Don't aim while running
						rq.m_bAimAtTargetEnd = true;
			
						m_State.ApplyNewRequest(rq);
						m_bPushedMoveRequest = true;	
						
						return true;
					}
					else
					{
						SCR_AIMoveFromSuppressBehavior aiMovefromSuppress = SCR_AIMoveFromSuppressBehavior.Cast(utility.FindActionOfType(SCR_AIMoveFromSuppressBehavior));
						if(aiMovefromSuppress) return true;
						
						utility.AddAction(new SCR_AIMoveFromSuppressBehavior(null, null, shooterPos, shooter))
					}
				}
				else if (isNullTarget && distanceToDanger < 4)
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
						rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
						rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
						rq.m_bAimAtTarget = false; // Don't aim while running
						rq.m_bAimAtTargetEnd = true;
			
						m_State.ApplyNewRequest(rq);
						m_bPushedMoveRequest = true;	
						
						return true;
					}
					else
					{
						SCR_AIMoveFromSuppressBehavior aiMovefromSuppress = SCR_AIMoveFromSuppressBehavior.Cast(utility.FindActionOfType(SCR_AIMoveFromSuppressBehavior));
						if(aiMovefromSuppress) return true;
						
						utility.AddAction(new SCR_AIMoveFromSuppressBehavior(null, null, shooterPos, shooter))
					}
				}
				return true;
			}
		}
		
		return true;
	}
};