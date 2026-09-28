modded class SCR_AICombatMoveLogic_HideFromThreatSystem
{
	protected const float DANGER_HIGH = 2.0;
	protected const float DANGER_MEDIUM = 0.5;

	protected const float COVER_DIST_MIN_BUILDING = 0.0;
	protected const float COVER_DIST_MAX_BUILDING = 5.0;

	protected const float MOVE_DURATION_SCALE_BUILDING = 1.5;

	override void Update()
	{
		if (m_iCurrentSector == -1 || !m_Utility.m_SectorThreatFilter.IsSectorActive(m_iCurrentSector) || !m_MyEntity)
			return;

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
			if (sectorDanger < DANGER_MEDIUM && Math.RandomFloat01() < 0.15)
			{
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
					if (((threatState == EAIThreatState.THREATENED) || causedDamage) && !SCR_CoverManagerComponent.IsEntityInsideBuilding(m_Utility.m_OwnerEntity))
					{
						PushRequestMoveDanger(threatPos, sectorDanger, sectorFlags);
					}
					else
					{
						ECharacterStance newStance;
						if (threatState == EAIThreatState.THREATENED)
							newStance = ECharacterStance.PRONE;
						else
							newStance = ECharacterStance.CROUCH;

						if (newStance != m_CharacterController.GetStance())
							m_State.ApplyRequestChangeStanceOutsideCover(newStance);
					}
				}
			}
			else if (distToThreat < SCR_AICombatMoveUtils.VERY_LONG_RANGE_COMBAT_DIST)
			{
				if (((threatState == EAIThreatState.THREATENED) || causedDamage) && !SCR_CoverManagerComponent.IsEntityInsideBuilding(m_Utility.m_OwnerEntity))
				{
					PushRequestMove(threatPos, sectorDanger, sectorFlags);
				}
				else
				{
					ECharacterStance newStance = ECharacterStance.CROUCH;
					if (newStance != m_CharacterController.GetStance())
						m_State.ApplyRequestChangeStanceOutsideCover(newStance);
				}
			}
			else
			{
				ECharacterStance newStance;
				if ((threatState == EAIThreatState.THREATENED) || (sectorFlags & SCR_EAIThreatSectorFlags.DIRECTED_AT_ME) || causedDamage)
					newStance = ECharacterStance.PRONE;
				else
					newStance = ECharacterStance.CROUCH;

				if (newStance != m_CharacterController.GetStance())
						m_State.ApplyRequestChangeStanceOutsideCover(newStance);
			}
		}
	}

	protected SCR_EAICombatMoveRequestType ResolveRequestType(float danger)
	{
		SCR_AICombatMoveRequestBase oldRq = m_State.GetOldRequest();
		if (oldRq && oldRq.m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
			return SCR_EAICombatMoveRequestType.MOVE;

		if (danger <= DANGER_MEDIUM)
			return SCR_EAICombatMoveRequestType.MOVE;

		if (SCR_CoverManagerComponent.IsEntityInsideBuilding(m_Utility.m_OwnerEntity))
			return SCR_EAICombatMoveRequestType.MOVE;

		return SCR_EAICombatMoveRequestType.BUILDING;
	}

	protected void ApplyRequestType(SCR_AICombatMoveRequest_Move rq, float danger, float coverDistMinNormal)
	{
		rq.m_eType = ResolveRequestType(danger);

		if (rq.m_eType == SCR_EAICombatMoveRequestType.BUILDING)
		{
			rq.m_fCoverSearchDistMin = COVER_DIST_MIN_BUILDING;
			rq.m_fCoverSearchDistMax = COVER_DIST_MAX_BUILDING;
			rq.m_fMoveDuration_s    *= MOVE_DURATION_SCALE_BUILDING;
			rq.m_bTryFindCover = false;
		}
		else
		{
			rq.m_fCoverSearchDistMin = coverDistMinNormal;
		}
	}

	override protected void PushRequestMove(vector threatPos, float danger, SCR_EAIThreatSectorFlags sectorFlags)
	{
		float distance = vector.Distance(m_MyEntity.GetOrigin(), threatPos);
		bool closeRange = distance < SCR_AICombatMoveUtils.CLOSE_RANGE_COMBAT_DIST;

		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

		if (danger > DANGER_HIGH || (sectorFlags & SCR_EAIThreatSectorFlags.DIRECTED_AT_ME))
		{
			DCO_SmokeUtility.TryDeploySmokeForRetreat(m_Utility, threatPos, danger);

			if (closeRange)
			{
				rq.m_fCoverSearchDistMax = 30;
				rq.m_bUseCoverSearchDirectivity = true;
				rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
				rq.m_fCoverSearchSectorHalfAngleRad = 0.75 * Math.PI;
				rq.m_eMovementType = EMovementType.SPRINT;
				rq.m_bAimAtTarget = false;
				rq.m_bAimAtTargetEnd = true;
				rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * 2;
			}
			else
			{
				rq.m_fCoverSearchDistMax = 30;
				rq.m_bUseCoverSearchDirectivity = false;
				rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
				rq.m_fCoverSearchSectorHalfAngleRad = Math.PI;
				rq.m_eMovementType = EMovementType.SPRINT;
				rq.m_bAimAtTarget = false;
				rq.m_bAimAtTargetEnd = true;
				rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * 3;
			}

			rq.m_bTryFindCover = true;
			rq.m_eStanceMoving = ECharacterStance.STAND;
			rq.m_eStanceEnd = ECharacterStance.CROUCH;
		}
		else if (danger > DANGER_MEDIUM)
		{
			rq.m_bTryFindCover = true;
			rq.m_fCoverSearchDistMax = 30;
			rq.m_bUseCoverSearchDirectivity = true;
			rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * 1.5;

			rq.m_eStanceMoving = m_CharacterController.GetStance();
			rq.m_eStanceEnd = rq.m_eStanceMoving;

			rq.m_bAimAtTarget = DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection);
			rq.m_bAimAtTargetEnd = true;
		}
		else
		{
			rq.m_bTryFindCover = true;
			rq.m_fCoverSearchDistMax = 30;
			rq.m_bUseCoverSearchDirectivity = true;
			rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * 2;

			rq.m_eStanceMoving = ECharacterStance.CROUCH;
			rq.m_eStanceEnd = ECharacterStance.PRONE;

			rq.m_bAimAtTarget = false;
			rq.m_bAimAtTargetEnd = true;
		}
		rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
		rq.m_vTargetPos = threatPos;
		rq.m_vMovePos = rq.m_vTargetPos;
		rq.m_bCheckCoverVisibility = false;
		rq.m_bFailIfNoCover = false;

		ApplyRequestType(rq, danger, 5);

		rq.GetOnCompleted().Insert(OnMoveRequestCompleted);

		m_State.ApplyNewRequest(rq);
		m_LastMoveRequest = rq;
	}

	protected void PushRequestMoveDanger(vector threatPos, float danger, SCR_EAIThreatSectorFlags sectorFlags)
	{
		float distance = vector.Distance(m_MyEntity.GetOrigin(), threatPos);
		bool closeRange = distance < SCR_AICombatMoveUtils.CLOSE_RANGE_COMBAT_DIST;

		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

		if (danger > DANGER_HIGH || (sectorFlags & SCR_EAIThreatSectorFlags.DIRECTED_AT_ME))
		{
			DCO_SmokeUtility.TryDeploySmokeForRetreat(m_Utility, threatPos, danger);

			if (closeRange)
			{
				rq.m_fCoverSearchDistMax = 30;
				rq.m_bUseCoverSearchDirectivity = true;
				rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
				rq.m_fCoverSearchSectorHalfAngleRad = 0.75 * Math.PI;
				rq.m_eMovementType = EMovementType.WALK;
				rq.m_bAimAtTarget = false;
				rq.m_bAimAtTargetEnd = true;
				rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * 2;
			}
			else
			{
				rq.m_fCoverSearchDistMax = 30;
				rq.m_bUseCoverSearchDirectivity = false;
				rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
				rq.m_fCoverSearchSectorHalfAngleRad = Math.PI;
				rq.m_eMovementType = EMovementType.RUN;
				rq.m_bAimAtTarget = false;
				rq.m_bAimAtTargetEnd = true;
				rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * 3;
			}

			rq.m_bTryFindCover = true;
			rq.m_eStanceMoving = ECharacterStance.STAND;
			rq.m_eStanceEnd = ECharacterStance.CROUCH;
		}
		else if (danger > DANGER_MEDIUM)
		{
			rq.m_bTryFindCover = true;
			rq.m_fCoverSearchDistMax = 30;
			rq.m_bUseCoverSearchDirectivity = true;
			rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * 1.5;

			rq.m_eStanceMoving = m_CharacterController.GetStance();
			rq.m_eStanceEnd = rq.m_eStanceMoving;

			rq.m_bAimAtTarget = DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection);
			rq.m_bAimAtTargetEnd = true;
		}
		else
		{
			rq.m_bTryFindCover = true;
			rq.m_fCoverSearchDistMax = 30;
			rq.m_bUseCoverSearchDirectivity = true;
			rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * 2;

			rq.m_eStanceMoving = ECharacterStance.CROUCH;
			rq.m_eStanceEnd = ECharacterStance.PRONE;

			rq.m_bAimAtTarget = DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection);
			rq.m_bAimAtTargetEnd = true;
		}
		rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
		rq.m_vTargetPos = threatPos;
		rq.m_vMovePos = rq.m_vTargetPos;
		rq.m_bCheckCoverVisibility = false;
		rq.m_bFailIfNoCover = false;

		ApplyRequestType(rq, danger, 8);

		rq.GetOnCompleted().Insert(OnMoveRequestCompleted);

		m_State.ApplyNewRequest(rq);
		m_LastMoveRequest = rq;
	}

	void UpdateVehicle(IEntity m_DriverEntity, SCR_AICombatMoveState m_DriverCombatState, SCR_AIUtilityComponent m_DriverUtilityComp)
	{
		if (m_iCurrentSector == -1 || !m_DriverUtilityComp.m_SectorThreatFilter.IsSectorActive(m_iCurrentSector) || !m_DriverEntity)
			return;

		if (m_MyEntity == m_DriverEntity)
			return;

		if (m_DriverCombatState.IsMoving())
			return;

		vector threatPos = m_DriverUtilityComp.m_SectorThreatFilter.GetSectorPos(m_iCurrentSector);

		float sectorDanger = m_DriverUtilityComp.m_SectorThreatFilter.GetSectorDanger(m_iCurrentSector);
		if (!m_bPushedRequest && !m_bReachedSafety && !m_DriverCombatState.IsMoving())
		{
			if (sectorDanger < DANGER_MEDIUM && Math.RandomFloat01() < 0.5)
			{
				m_bReachedSafety = true;
				m_ParentBehavior.OnMovementCompleted(false);
			}
			else
			{
				SCR_EAIThreatSectorFlags sectorFlags = m_DriverUtilityComp.m_SectorThreatFilter.GetSectorFlags(m_iCurrentSector);
				PushRequestVehicleMove(threatPos, m_DriverCombatState, SCR_EAICombatMoveDirection.ANYWHERE);
				m_bPushedRequest = true;
			}
		}
		else if (m_bReachedSafety)
		{
			EAIThreatState threatState = m_DriverUtilityComp.m_ThreatSystem.GetState();
			float distToThreat = vector.Distance(m_DriverEntity.GetOrigin(), threatPos);

			SCR_EAIThreatSectorFlags sectorFlags = m_DriverUtilityComp.m_SectorThreatFilter.GetSectorFlags(m_iCurrentSector);
			bool causedDamage = sectorFlags & SCR_EAIThreatSectorFlags.CAUSED_DAMAGE;

			if (distToThreat < SCR_AICombatMoveUtils.CLOSE_RANGE_COMBAT_DIST)
			{
				if ((threatState == EAIThreatState.THREATENED) || causedDamage)
					PushRequestVehicleMove(threatPos, m_DriverCombatState, SCR_EAICombatMoveDirection.BACKWARD);
				else
				{
					if (Math.RandomInt(0,3) == 1)
						PushRequestVehicleMove(threatPos, m_DriverCombatState, SCR_EAICombatMoveDirection.BACKWARD);
					else
					{
						if (Math.RandomInt(0,5) > 3)
							PushRequestVehicleMove(threatPos, m_DriverCombatState, SCR_EAICombatMoveDirection.FORWARD);
						else
						{
							if (Math.RandomInt(0,2) == 1)
								PushRequestVehicleMove(threatPos, m_DriverCombatState, SCR_EAICombatMoveDirection.LEFT);
							else
								PushRequestVehicleMove(threatPos, m_DriverCombatState, SCR_EAICombatMoveDirection.RIGHT);
						}
					}
				}
			}
			else if (distToThreat < SCR_AICombatMoveUtils.VERY_LONG_RANGE_COMBAT_DIST)
			{
				if ((threatState == EAIThreatState.THREATENED) || causedDamage)
					PushRequestVehicleMove(threatPos, m_DriverCombatState, SCR_EAICombatMoveDirection.BACKWARD);
				else
				{
					if (Math.RandomInt(0,2) == 1)
						PushRequestVehicleMove(threatPos, m_DriverCombatState, SCR_EAICombatMoveDirection.FORWARD);
					else
					{
						if (Math.RandomInt(0,5) > 1)
							PushRequestVehicleMove(threatPos, m_DriverCombatState, SCR_EAICombatMoveDirection.FORWARD);
						else
						{
							if (Math.RandomInt(0,2) == 1)
								PushRequestVehicleMove(threatPos, m_DriverCombatState, SCR_EAICombatMoveDirection.LEFT);
							else
								PushRequestVehicleMove(threatPos, m_DriverCombatState, SCR_EAICombatMoveDirection.RIGHT);
						}
					}
				}
			}
			else
			{
				SCR_EAIThreatSectorFlags flags = m_Utility.m_SectorThreatFilter.GetSectorFlags(m_iCurrentSector);

				if ((flags & SCR_EAIThreatSectorFlags.DIRECTED_AT_ME) || causedDamage)
				{
					if (Math.RandomInt(0,3) == 1)
						PushRequestVehicleMove(threatPos, m_DriverCombatState, SCR_EAICombatMoveDirection.FORWARD);
					else
					{
						if (Math.RandomInt(0,2) == 1)
							PushRequestVehicleMove(threatPos, m_DriverCombatState, SCR_EAICombatMoveDirection.LEFT);
						else
							PushRequestVehicleMove(threatPos, m_DriverCombatState, SCR_EAICombatMoveDirection.RIGHT);
					}
				}
				else
					PushRequestVehicleMove(threatPos, m_DriverCombatState, SCR_EAICombatMoveDirection.FORWARD);
			}
		}
	}

	void PushRequestVehicleMove(vector threatPos, SCR_AICombatMoveState m_DriverCombatState, SCR_EAICombatMoveDirection dir)
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
		rq.m_vTargetPos = threatPos;
		rq.m_vMovePos = threatPos;
		rq.m_eDirection = dir;
		rq.m_fMoveDuration_s = 120 / SCR_AICombatMoveUtils.GROUND_VEHICLE_GENERIC_SPEED;
		rq.m_bAimAtTarget = false;
		rq.m_bAimAtTargetEnd = false;

		m_DriverCombatState.ApplyNewRequest(rq);
		rq.GetOnCompleted().Insert(OnMoveRequestCompleted);
		m_LastMoveRequest = rq;
	}
}