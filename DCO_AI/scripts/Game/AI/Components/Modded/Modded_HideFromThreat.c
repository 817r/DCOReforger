modded class SCR_AICombatMoveLogic_HideFromThreatSystem
{	
	// Parent behavior which owns this
	protected SCR_AIObserveThreatSystemBehavior m_ParentBehavior;
	
	// Other components of this agent
	protected IEntity m_MyEntity;
	protected SCR_AICombatMoveState m_State;
	protected SCR_AIUtilityComponent m_Utility;
	protected CharacterControllerComponent m_CharacterController;
	
	// Data about current threat sector
	protected int m_iCurrentSector;
	
	// State of this combat move logic
	protected bool m_bPushedRequest = false;	// Set to true once we pushed request
	protected bool m_bReachedSafety = false;	// Set to true once we reached some safety (movement request finished or we are in good cover already)
	protected SCR_AICombatMoveRequest_Move m_LastMoveRequest;
	
	protected const float DANGER_HIGH = 2.0;
	protected const float DANGER_MEDIUM = 0.4;
	
	
	//--------------------------------------------------------------------------------------------
	override void Update()
	{
		// Bail if there's nothing to do, or pointers are invalid
		if (m_iCurrentSector == -1 || !m_Utility.m_SectorThreatFilter.IsSectorActive(m_iCurrentSector) || !m_MyEntity)
			return;

		// Wait until old requests are done, if they are important
		if (m_State.IsMoving(SCR_EAICombatMoveReason.MOVE_FROM_DANGER) || m_State.IsMovingToCover())
			return;
		
		vector threatPos = m_Utility.m_SectorThreatFilter.GetSectorPos(m_iCurrentSector);
		if (!m_bPushedRequest)
		{
			if (IsCurrentCoverSafe(threatPos))
				m_bReachedSafety = true;
		}
		
		float sectorDanger = m_Utility.m_SectorThreatFilter.GetSectorDanger(m_iCurrentSector);
		if (!m_bPushedRequest && !m_bReachedSafety && !m_State.IsMoving())
		{
			if (sectorDanger < DANGER_MEDIUM && Math.RandomFloat01() < 0.5)
			{
				// If it's low danger, we can randomly decide to not move at all, or move a little bit
				m_bReachedSafety = true;
				m_ParentBehavior.OnMovementCompleted(m_State.IsInValidCover());
			}
			else
			{
				SCR_EAIThreatSectorFlags sectorFlags = m_Utility.m_SectorThreatFilter.GetSectorFlags(m_iCurrentSector);
				PushRequestMove(threatPos, sectorDanger, sectorFlags);
				m_bPushedRequest = true;
			}
		}
		else if (m_bReachedSafety)
		{
			// We are not moving, manage our stance based on threat and range
			
			EAIThreatState threatState = m_Utility.m_ThreatSystem.GetState();
			float distToThreat = vector.Distance(m_MyEntity.GetOrigin(), threatPos);
			
			SCR_EAIThreatSectorFlags sectorFlags = m_Utility.m_SectorThreatFilter.GetSectorFlags(m_iCurrentSector);
			bool causedDamage = sectorFlags & SCR_EAIThreatSectorFlags.CAUSED_DAMAGE;
			
			if (distToThreat < SCR_AICombatMoveUtils.CLOSE_RANGE_COMBAT_DIST)
			{	
				if (m_State.IsInValidCover())
				{
					bool newExposedInCover = !causedDamage && (threatState != EAIThreatState.THREATENED);
					
					if (m_State.m_bExposedInCover != newExposedInCover)
						m_State.ApplyRequestChangeStanceInCover(newExposedInCover);
				}
				else
				{
					ECharacterStance newStance;
				
					if ((threatState == EAIThreatState.THREATENED) || causedDamage)
						newStance = ECharacterStance.CROUCH;
					else
						newStance = ECharacterStance.STAND;
					
					if (newStance != m_CharacterController.GetStance())
						m_State.ApplyRequestChangeStanceOutsideCover(newStance);
				}
			}
			else if (distToThreat < SCR_AICombatMoveUtils.VERY_LONG_RANGE_COMBAT_DIST)
			{
				ECharacterStance newStance;
				
				if ((threatState == EAIThreatState.THREATENED) || causedDamage)
					newStance = ECharacterStance.PRONE;
				else
					newStance = ECharacterStance.CROUCH;
				
				if (newStance != m_CharacterController.GetStance())
						m_State.ApplyRequestChangeStanceOutsideCover(newStance);
			}
			else
			{
				// Very long range
				
				ECharacterStance newStance;
				SCR_EAIThreatSectorFlags flags = m_Utility.m_SectorThreatFilter.GetSectorFlags(m_iCurrentSector);
				
				if ((threatState == EAIThreatState.THREATENED) || (flags & SCR_EAIThreatSectorFlags.DIRECTED_AT_ME) || causedDamage)
					newStance = ECharacterStance.PRONE;
				else
					newStance = ECharacterStance.CROUCH;
				
				if (newStance != m_CharacterController.GetStance())
						m_State.ApplyRequestChangeStanceOutsideCover(newStance);
			}
		} else
		{
			SCR_EAIThreatSectorFlags sectorFlags = m_Utility.m_SectorThreatFilter.GetSectorFlags(m_iCurrentSector);
			PushRequestMove(threatPos, sectorDanger, sectorFlags);
			m_bPushedRequest = true;
		}
	}
	
	//--------------------------------------------------------------------------------------------	
	override protected void PushRequestMove(vector threatPos, float danger, SCR_EAIThreatSectorFlags sectorFlags)
	{
		float distance = vector.Distance(m_MyEntity.GetOrigin(), threatPos);
		bool closeRange = distance < SCR_AICombatMoveUtils.CLOSE_RANGE_COMBAT_DIST;
			
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		if (danger > DANGER_HIGH || (sectorFlags & SCR_EAIThreatSectorFlags.DIRECTED_AT_ME))
		{
			if (closeRange)
			{
				rq.m_fCoverSearchDistMax = 16;
				rq.m_bUseCoverSearchDirectivity = true;
				rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD; // Awaw from danger
				rq.m_fCoverSearchSectorHalfAngleRad = 0.75 * Math.PI; // Almost full sector - except for direction directly at target
				rq.m_eMovementType = EMovementType.RUN; // Don't sprint
				rq.m_bAimAtTarget = true; // Aim while moving
				rq.m_bAimAtTargetEnd = true;	
				rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * 2;
			}
			else
			{
				rq.m_fCoverSearchDistMax = 25;
				rq.m_bUseCoverSearchDirectivity = false;
				rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE; // Random direction
				rq.m_fCoverSearchSectorHalfAngleRad = Math.PI; // Full sector
				rq.m_eMovementType = EMovementType.SPRINT;
				rq.m_bAimAtTarget = false; // Can't aim while sprinting
				rq.m_bAimAtTargetEnd = true;
				rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * 3;
			}	
			
			rq.m_bTryFindCover = true;
			rq.m_eStanceMoving = ECharacterStance.STAND;
			rq.m_eStanceEnd = ECharacterStance.PRONE;
		}
		else if (danger > DANGER_MEDIUM)
		{
			rq.m_bTryFindCover = true;
			rq.m_fCoverSearchDistMax = 16;
			rq.m_bUseCoverSearchDirectivity = true;
			rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
			//rq.m_fCoverSearchSectorHalfAngleRad - not needed since direction is ANYWHERE
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * 1.5;
			
			rq.m_eStanceMoving = m_CharacterController.GetStance();
			rq.m_eStanceEnd = rq.m_eStanceMoving;
			
			rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType); // Aim while moving
			rq.m_bAimAtTargetEnd = true;
		}
		else
		{
			rq.m_bTryFindCover = true;
			rq.m_fCoverSearchDistMax = 16;
			rq.m_bUseCoverSearchDirectivity = true;
			rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
			//rq.m_fCoverSearchSectorHalfAngleRad - not needed since direction is ANYWHERE
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * 2 / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_RUN;
			
			rq.m_eStanceMoving = ECharacterStance.STAND;
			rq.m_eStanceEnd = ECharacterStance.STAND;
			
			rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
			rq.m_bAimAtTargetEnd = true;
		}
		
		rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
		rq.m_vTargetPos = threatPos;
		rq.m_vMovePos = rq.m_vTargetPos;
		rq.m_bCheckCoverVisibility = true;
		rq.m_bFailIfNoCover = false;
		rq.m_fCoverSearchDistMin = 0;
		// rq.m_fCoverSearchSectorHalfAngleRad - not needed since direction is ANYWHERE
		
		rq.GetOnCompleted().Insert(OnMoveRequestCompleted);
		
		m_State.ApplyNewRequest(rq);
		m_LastMoveRequest = rq;
	}
}
