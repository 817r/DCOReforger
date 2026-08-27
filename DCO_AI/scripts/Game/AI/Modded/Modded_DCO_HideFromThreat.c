modded class SCR_AICombatMoveLogic_HideFromThreatSystem
{	
	protected const float DANGER_HIGH = 2.0;
	protected const float DANGER_MEDIUM = 0.5;
	
	// === ADDED: Building retreat ===
	//! Radius cover search pas request bertipe BUILDING. Posisi tujuan ada DI DALAM
	//! ruangan, jadi radius 30m bakal narik AI keluar gedung lagi.
	protected const float COVER_DIST_MIN_BUILDING = 0.0;
	protected const float COVER_DIST_MAX_BUILDING = 5.0;
	
	//! Masuk gedung butuh lebih lama daripada lari ke cover statis -- ada pintu,
	//! koridor, dan routing interior yang harus dilewati.
	protected const float MOVE_DURATION_SCALE_BUILDING = 1.5;
	// === END ADDED ===
	
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
					// === CHANGED: dulu blok stance di bawah tetep jalan walau
					// PushRequestMoveDanger udah dipanggil, jadi request kabur langsung
					// dibatalin sama ApplyRequestChangeStanceOutsideCover. Sekarang dua
					// jalur ini eksklusif. ===
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
				// === CHANGED: sama kayak cabang close range -- jangan push stance
				// change setelah request gerak, itu ngebatalin request-nya. ===
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
	
	//--------------------------------------------------------------------------------------------
	// === ADDED: Building retreat ===
	//! Tentuin tipe request: BUILDING = engine nyariin posisi di dalem gedung terdekat,
	//! MOVE = cover statis biasa. Pola fallback-nya sama persis kayak
	//! Modded_CombatMoveLogic_Attack.PushRequestMove() dan DCO_DangerDamageTaken.
	protected SCR_EAICombatMoveRequestType ResolveRequestType(float danger)
	{
		// Percobaan sebelumnya gagal nemu gedung -> jangan ngotot, langsung cover biasa.
		// Tanpa ini AI bisa nyangkut minta gedung terus di area yang emang gak ada gedungnya.
		SCR_AICombatMoveRequestBase oldRq = m_State.GetOldRequest();
		if (oldRq && oldRq.m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
			return SCR_EAICombatMoveRequestType.MOVE;
		
		// Danger rendah: lari jauh ke gedung lebih beresiko daripada nunduk di cover terdekat
		if (danger <= DANGER_MEDIUM)
			return SCR_EAICombatMoveRequestType.MOVE;
		
		// Udah di dalem gedung -> gak perlu nyari gedung lagi.
		// Konsisten sama gate yang udah ada di Update().
		if (SCR_CoverManagerComponent.IsEntityInsideBuilding(m_Utility.m_OwnerEntity))
			return SCR_EAICombatMoveRequestType.MOVE;
		
		return SCR_EAICombatMoveRequestType.BUILDING;
	}
	
	//! Terapin tipe request + sesuain radius cover & durasi kalau tujuannya gedung.
	//! Dipanggil SETELAH semua field lain di-set, biar override-nya gak ketimpa
	//! (ini persis bug yang ada di Modded_CombatMoveLogic_Attack, di mana blok
	//! IsMovingToBuilding() ngubah variabel lokal setelah nilainya nyampe ke rq).
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
	// === END ADDED ===
	
	//--------------------------------------------------------------------------------------------	
	override protected void PushRequestMove(vector threatPos, float danger, SCR_EAIThreatSectorFlags sectorFlags)
	{
		float distance = vector.Distance(m_MyEntity.GetOrigin(), threatPos);
		bool closeRange = distance < SCR_AICombatMoveUtils.CLOSE_RANGE_COMBAT_DIST;
			
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		if (danger > DANGER_HIGH || (sectorFlags & SCR_EAIThreatSectorFlags.DIRECTED_AT_ME))
		{
			// === ADDED: Smoke grenade buat cover reposition ===
			DCO_SmokeUtility.TryDeploySmokeForRetreat(m_Utility, threatPos, danger);
			// === END ADDED ===
			
			if (closeRange)
			{
				rq.m_fCoverSearchDistMax = 30;
				rq.m_bUseCoverSearchDirectivity = true;
				rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD; // Awaw from danger
				rq.m_fCoverSearchSectorHalfAngleRad = 0.75 * Math.PI; // Almost full sector - except for direction directly at target
				rq.m_eMovementType = EMovementType.SPRINT; // Don't sprint
				rq.m_bAimAtTarget = false; // Aim while moving
				rq.m_bAimAtTargetEnd = true;	
				rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * 2;
			}
			else
			{
				rq.m_fCoverSearchDistMax = 30;
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
			rq.m_eStanceEnd = ECharacterStance.CROUCH;
		}
		else if (danger > DANGER_MEDIUM)
		{
			rq.m_bTryFindCover = true;
			rq.m_fCoverSearchDistMax = 30;
			rq.m_bUseCoverSearchDirectivity = true;
			rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
			//rq.m_fCoverSearchSectorHalfAngleRad - not needed since direction is ANYWHERE
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * 1.5;
			
			rq.m_eStanceMoving = m_CharacterController.GetStance();
			rq.m_eStanceEnd = rq.m_eStanceMoving;
			
			rq.m_bAimAtTarget = DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection); // Aim while moving
			rq.m_bAimAtTargetEnd = true;
		}
		else
		{
			rq.m_bTryFindCover = true;
			rq.m_fCoverSearchDistMax = 30;
			rq.m_bUseCoverSearchDirectivity = true;
			rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
			//rq.m_fCoverSearchSectorHalfAngleRad - not needed since direction is ANYWHERE
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * 2; // CHANGED: dulu dibagi CHARACTER_SPEED_STAND_RUN (3.6) -> cuma 0.56-0.83 detik
			
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
		// rq.m_fCoverSearchSectorHalfAngleRad - not needed since direction is ANYWHERE
		
		// === ADDED: set m_eType (dulu gak pernah di-set = default STOP) + radius cover ===
		ApplyRequestType(rq, danger, 5);
		// === END ADDED ===
		
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
				rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD; // Awaw from danger
				rq.m_fCoverSearchSectorHalfAngleRad = 0.75 * Math.PI; // Almost full sector - except for direction directly at target
				rq.m_eMovementType = EMovementType.WALK; // Don't sprint
				rq.m_bAimAtTarget = false; // Aim while moving
				rq.m_bAimAtTargetEnd = true;	
				rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * 2;
			}
			else
			{
				rq.m_fCoverSearchDistMax = 30;
				rq.m_bUseCoverSearchDirectivity = false;
				rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE; // Random direction
				rq.m_fCoverSearchSectorHalfAngleRad = Math.PI; // Full sector
				rq.m_eMovementType = EMovementType.RUN;
				rq.m_bAimAtTarget = false; // Can't aim while sprinting
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
			//rq.m_fCoverSearchSectorHalfAngleRad - not needed since direction is ANYWHERE
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * 1.5;
			
			rq.m_eStanceMoving = m_CharacterController.GetStance();
			rq.m_eStanceEnd = rq.m_eStanceMoving;
			
			rq.m_bAimAtTarget = DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection); // Aim while moving
			rq.m_bAimAtTargetEnd = true;
		}
		else
		{
			rq.m_bTryFindCover = true;
			rq.m_fCoverSearchDistMax = 30;
			rq.m_bUseCoverSearchDirectivity = true;
			rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
			//rq.m_fCoverSearchSectorHalfAngleRad - not needed since direction is ANYWHERE
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * 2; // CHANGED: dulu dibagi CHARACTER_SPEED_STAND_RUN (3.6) -> cuma 0.56-0.83 detik
			
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
		// rq.m_fCoverSearchSectorHalfAngleRad - not needed since direction is ANYWHERE
		
		// === ADDED: set m_eType (dulu gak pernah di-set = default STOP) + radius cover ===
		ApplyRequestType(rq, danger, 8);
		// === END ADDED ===
		
		rq.GetOnCompleted().Insert(OnMoveRequestCompleted);
		
		m_State.ApplyNewRequest(rq);
		m_LastMoveRequest = rq;
	}
	
	void UpdateVehicle(IEntity m_DriverEntity, SCR_AICombatMoveState m_DriverCombatState, SCR_AIUtilityComponent m_DriverUtilityComp)
	{
		// Bail if there's nothing to do, or pointers are invalid
		if (m_iCurrentSector == -1 || !m_DriverUtilityComp.m_SectorThreatFilter.IsSectorActive(m_iCurrentSector) || !m_DriverEntity)
			return;
		
		if (m_MyEntity == m_DriverEntity)
			return;

		// Wait until old requests are done, if they are important
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
			// We are not moving, manage our stance based on threat and range
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