[BaseContainerProps()]
modded class SCR_AIDangerReaction_ProjectileHit : SCR_AIDangerReaction
{
	protected static const float BULLET_IMPACT_DISTANCE_SQ_MAX = 3*3;
	protected SCR_AICombatMoveState m_State;
	protected static const float COVER_SEARCH_DIST_MAX = 20.0;
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
		SCR_CharacterControllerComponent charCon = utility.m_CombatComponent.GetCharacterController();
		m_State = utility.m_CombatMoveState;
		
		if (!agent || !agent.IsEnemy(shooterRoot))
			return false;

		float distanceToShooter = vector.Distance(utility.GetOrigin(), shooterPos);
		
		int bulletCount = dangerEventCount;
		rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;				
		
		if (shooter)
		{
			if (agent && agent.IsEnemy(shooter))
			{						
				if (utility.m_CombatComponent.GetSelectedWeaponType() == EWeaponType.WT_MACHINEGUN && distanceToDanger < 2 && isNullTarget)
				{
					float radius = Math.Map(shooterDistance, 0, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE, 1, 3);
					vector bbMax, bbMin;
					shooterRoot.GetBounds(bbMin, bbMax);
					SCR_AISuppressionVolumeBase.CreateSuppressionBox(shooterRoot.GetOrigin(), radius, 3, bbMin, bbMax);
					SCR_AISuppressionObjectVolumeBox createSupp = new SCR_AISuppressionObjectVolumeBox(bbMin, bbMax);
					if (createSupp)
					{
						SCR_AISuppressBehavior supp = new SCR_AISuppressBehavior(utility, null, createSupp, 6, 1.5);
						utility.AddAction(supp);	
					}
				
				}
				
				if (distanceToDanger < 2 && bulletCount > 2 && isNullTarget && m_State.IsInValidCover())
				{
					if(m_State != null && !m_State.IsExecutingRequest())
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
						rq.m_fCoverSearchDistMax = COVER_SEARCH_DIST_MAX;
						rq.m_fCoverSearchDistMin = 2;
						rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_RUN;
						rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
						rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
						rq.m_bAimAtTarget = false;
						rq.m_bAimAtTargetEnd = false;
						if (m_State.GetOldRequest() && m_State.GetOldRequest().m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
							rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
						else
							rq.m_eType = SCR_EAICombatMoveRequestType.BUILDING;			
						m_State.ApplyNewRequest(rq);
						m_bPushedMoveRequest = true;
						return true;	
					}
				}
				else if (distanceToDanger < 2 && shooterDistance < 20 && m_State.IsInValidCover() && bulletCount > 2)
				{
					if(m_State != null && !m_State.IsExecutingRequest())
					{					
						rq.m_vTargetPos = shooterPos;
						rq.m_vMovePos = rq.m_vTargetPos;
						rq.m_bTryFindCover = true;
						rq.m_bUseCoverSearchDirectivity = true;
						rq.m_bCheckCoverVisibility = true;
						rq.m_bFailIfNoCover = true;
						rq.m_eStanceMoving = ECharacterStance.CROUCH;
						rq.m_eStanceEnd = ECharacterStance.CROUCH;
						rq.m_eMovementType = EMovementType.RUN;
						rq.m_fCoverSearchDistMax = COVER_SEARCH_DIST_MAX / 2;
						rq.m_fCoverSearchDistMin = 2;
						rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN;
						if (Math.RandomIntInclusive(0, 1) == 0)
							rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
						else
						{
							if (Math.RandomIntInclusive(0, 1) == 0)
								rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
							else
								rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
						}
						rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
						rq.m_bAimAtTarget = false; // Don't aim while running
						rq.m_bAimAtTargetEnd = true;
						if (m_State.GetOldRequest() && m_State.GetOldRequest().m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
							rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
						else
							rq.m_eType = SCR_EAICombatMoveRequestType.BUILDING;			
						m_State.ApplyNewRequest(rq);
						m_bPushedMoveRequest = true;
						return true;	
					}
				}
				else if (distanceToDanger < 2 && bulletCount > 4 && m_State.IsInValidCover())
				{
					if(m_State != null && !m_State.IsExecutingRequest())
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
						rq.m_fCoverSearchDistMax = COVER_SEARCH_DIST_MAX;
						rq.m_fCoverSearchDistMin = 2;
						rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;
						if (Math.RandomIntInclusive(0, 1) == 0)
							rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
						else
						{
							if (Math.RandomIntInclusive(0, 1) == 0)
								rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
							else
								rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
						}
						rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
						rq.m_bAimAtTarget = true; // Don't aim while running
						rq.m_bAimAtTargetEnd = true;
						if (m_State.GetOldRequest() && m_State.GetOldRequest().m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
							rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
						else
							rq.m_eType = SCR_EAICombatMoveRequestType.BUILDING;			
						m_State.ApplyNewRequest(rq);
						m_bPushedMoveRequest = true;
						return true;	
					}
				}				
				else if (charCon.GetStance() == ECharacterStance.PRONE && distanceToDanger < 1.5 && bulletCount > 2)
				{
					int roll = 0;
					for (int i = 0; i < roll; i++)
					{
						if (Math.RandomIntInclusive(0, 1) == 0)
							charCon.SetRoll(1);
						else
							charCon.SetRoll(2);			
					}
				}
				else if (distanceToDanger < 2 && bulletCount > 1 && isNullTarget)
				{
					if(m_State != null && !m_State.IsExecutingRequest())
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
						rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;
						rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
						rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
						rq.m_bAimAtTarget = false;
						rq.m_bAimAtTargetEnd = false;
						if (m_State.GetOldRequest() && m_State.GetOldRequest().m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
							rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
						else
							rq.m_eType = SCR_EAICombatMoveRequestType.BUILDING;			
						m_State.ApplyNewRequest(rq);
						m_bPushedMoveRequest = true;
						return true;	
					}
				}
				else if (distanceToDanger < 2 && shooterDistance < 20)
				{
					if(m_State != null && !m_State.IsExecutingRequest())
					{					
						rq.m_vTargetPos = shooterPos;
						rq.m_vMovePos = rq.m_vTargetPos;
						rq.m_bTryFindCover = true;
						rq.m_bUseCoverSearchDirectivity = true;
						rq.m_bCheckCoverVisibility = true;
						rq.m_bFailIfNoCover = false;
						rq.m_eStanceMoving = ECharacterStance.CROUCH;
						rq.m_eStanceEnd = ECharacterStance.CROUCH;
						rq.m_eMovementType = EMovementType.WALK;
						rq.m_fCoverSearchDistMax = COVER_SEARCH_DIST_MAX / 2;
						rq.m_fCoverSearchDistMin = 2;
						rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN;
						if (Math.RandomIntInclusive(0, 1) == 0)
							rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
						else
						{
							if (Math.RandomIntInclusive(0, 1) == 0)
								rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
							else
								rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
						}
						rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
						rq.m_bAimAtTarget = true; // Don't aim while running
						rq.m_bAimAtTargetEnd = true;
						if (m_State.GetOldRequest() && m_State.GetOldRequest().m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
							rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
						else
							rq.m_eType = SCR_EAICombatMoveRequestType.BUILDING;			
						m_State.ApplyNewRequest(rq);
						m_bPushedMoveRequest = true;
						return true;	
					}
				}
				else if (distanceToDanger < 2 && bulletCount > 3)
				{
					if(m_State != null && !m_State.IsExecutingRequest())
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
						rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;
						if (Math.RandomIntInclusive(0, 1) == 0)
							rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
						else
						{
							if (Math.RandomIntInclusive(0, 1) == 0)
								rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
							else
								rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
						}
						rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;  // - not needed since direction is ANYWHERE
						rq.m_bAimAtTarget = true; // Don't aim while running
						rq.m_bAimAtTargetEnd = true;
						if (m_State.GetOldRequest() && m_State.GetOldRequest().m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
							rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
						else
							rq.m_eType = SCR_EAICombatMoveRequestType.BUILDING;			
						m_State.ApplyNewRequest(rq);
						m_bPushedMoveRequest = true;
						return true;	
					}
				} else if (shooterDistance > 100)
				{
					if (charCon.GetStance() == ECharacterStance.STAND)
						charCon.SetStanceChange(2);
					else
						charCon.SetStanceChange(3);
				}
				
				if (rq.m_bAimAtTarget && utility.m_CombatComponent.HasWeaponOfType(EWeaponType.WT_MACHINEGUN))
				{
					float radius = Math.Map(shooterDistance, 0, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE, 1, 3);
					vector bbMax, bbMin;
					SCR_AISuppressionVolumeBase.CreateSuppressionBox(shooterRoot.GetOrigin(), radius, 3, bbMin, bbMax);
					SCR_AISuppressionObjectVolumeBox createSupp = new SCR_AISuppressionObjectVolumeBox(bbMin, bbMax);
					if (createSupp)
					{
						SCR_AISuppressBehavior supp = new SCR_AISuppressBehavior(utility, null, createSupp, 5, 1.5);
						utility.AddAction(supp);					
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