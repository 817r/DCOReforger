[BaseContainerProps()]
modded class SCR_AIDangerReaction_DamageTaken
{
	protected SCR_AICombatMoveState m_State;
	protected CharacterControllerComponent m_CharacterController;
	
	protected static const float COVER_SEARCH_DIST_MAX = 30.0;
	protected const float COVER_QUERY_SECTOR_ANGLE_RAD = 0.35 * Math.PI;
	
	protected bool m_bPushedMoveRequest = false;
	
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{
		if (!super.PerformReaction(utility, threatSystem, dangerEvent, dangerEventCount))
			return false;
		
		IEntity shooter = dangerEvent.GetObject();
		
		if (!shooter)
			return super.PerformReaction(utility, threatSystem, dangerEvent, dangerEventCount);
		
		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
		IEntity shooterRoot = shooter.GetRootParent();
		vector shooterPos = shooter.GetOrigin();
		float shooterDistance = vector.Distance(utility.GetOrigin(), shooterRoot.GetOrigin());
		float shooterDistances = vector.Distance(utility.GetOrigin(), shooterPos);
		float distanceToDanger = vector.Distance(utility.GetOrigin(), dangerEvent.GetPosition());
		bool isNullTarget = utility.m_CombatComponent.GetCurrentTarget() == null;
		SCR_CharacterControllerComponent charCon = utility.m_CombatComponent.GetCharacterController();
		m_State = utility.m_CombatMoveState;
		
		if (!agent || !agent.IsEnemy(shooterRoot))
			return false;

		float distanceToShooter = vector.Distance(utility.GetOrigin(), shooterPos);
		
		if (utility.m_CombatComponent.IsEnemyKnown(shooterRoot) && Math.RandomFloat01() > 0.7)
		{
			float dist = vector.Distance(shooterRoot.GetOrigin(), utility.GetOrigin());
			float radius = Math.Map(dist, 0, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE, 1, 3);
			vector bbMax, bbMin;
			SCR_AISuppressionVolumeBase.CreateSuppressionBox(shooterRoot.GetOrigin(), radius, 3, bbMin, bbMax);
			SCR_AISuppressionObjectVolumeBox createSupp = new SCR_AISuppressionObjectVolumeBox(bbMin, bbMax);
			SCR_AISuppressBehavior supp = new SCR_AISuppressBehavior(utility, null, createSupp, 5, 1.5);
		}
		
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		if (!m_State.IsExecutingRequest() && m_State.IsInValidCover() && shooterDistances > 50)
		{
			rq.m_vTargetPos = shooterPos;
			rq.m_vMovePos = rq.m_vTargetPos;
			rq.m_bTryFindCover = true;
			rq.m_bUseCoverSearchDirectivity = true;
			rq.m_bCheckCoverVisibility = true;
			rq.m_bFailIfNoCover = true;
			rq.m_eStanceMoving = ECharacterStance.STAND;
			rq.m_eStanceEnd = ECharacterStance.CROUCH;
			rq.m_eMovementType = EMovementType.SPRINT;
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
			rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
			rq.m_bAimAtTarget = false;
			rq.m_bAimAtTargetEnd = true;
			if (m_State.GetOldRequest() && m_State.GetOldRequest().m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
				rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
			else
				rq.m_eType = SCR_EAICombatMoveRequestType.BUILDING;			
			m_State.ApplyNewRequest(rq);
			m_bPushedMoveRequest = true;
			return true;			
		} else if (!m_State.IsExecutingRequest() && !m_State.IsInValidCover() && shooterDistances > 50)
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
			if (Math.RandomIntInclusive(0, 1) == 0)
				rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
			else
			{
				if (Math.RandomIntInclusive(0, 1) == 0)
					rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
				else
					rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
			}
			rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
			rq.m_bAimAtTarget = false;
			rq.m_bAimAtTargetEnd = true;
			if (m_State.GetOldRequest() && m_State.GetOldRequest().m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
				rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
			else
				rq.m_eType = SCR_EAICombatMoveRequestType.BUILDING;			
			m_State.ApplyNewRequest(rq);
			m_bPushedMoveRequest = true;
			return true;			
		} else if (charCon.GetStance() == ECharacterStance.PRONE && shooterDistances > 50)
		{
			int roll = 2;
			for (int i = 0; i < roll; i++)
			{
				if (Math.RandomIntInclusive(0, 1) == 0)
					charCon.SetRoll(1);
				else
					charCon.SetRoll(2);			
			}

		} else if (!m_State.IsExecutingRequest() && m_State.IsInValidCover())
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
			rq.m_bAimAtTarget = true;
			rq.m_bAimAtTargetEnd = true;
			if (m_State.GetOldRequest() && m_State.GetOldRequest().m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
				rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
			else
				rq.m_eType = SCR_EAICombatMoveRequestType.BUILDING;			
			m_State.ApplyNewRequest(rq);
			m_bPushedMoveRequest = true;
			return true;		
		} else if (!m_State.IsExecutingRequest() && !m_State.IsInValidCover())
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
			rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_RUN;
			rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
			rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
			rq.m_bAimAtTarget = true;
			rq.m_bAimAtTargetEnd = true;
			if (m_State.GetOldRequest() && m_State.GetOldRequest().m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
				rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
			else
				rq.m_eType = SCR_EAICombatMoveRequestType.BUILDING;			
			m_State.ApplyNewRequest(rq);
			m_bPushedMoveRequest = true;
			return true;		
		}
		
		return true;
	}
};
