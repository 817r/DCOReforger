modded class SCR_AICombatMoveState
{
	//! Tanpa syarat terlindungi selama ini setelah apply, karena BT butuh 1 tick untuk mulai eksekusi
	protected static const float DCO_COVER_PROTECT_GRACE_S = 0.5;

	protected SCR_AICombatMoveRequestBase m_DCOCoverRq;           // weak ref, state sendiri yang memegang request
	protected float m_fDCOCoverAppliedAt_ms;
	protected float m_fDCOCoverProtectUntil_ms;
	protected bool  m_bDCOApplyingCover;
	protected int   m_iDCOBlockedCount;                           // untuk log, dicetak sekali saat release
	
	protected static const float DCO_INDOOR_RELOCATE_RADIUS_DEFAULT   = 12.0;
	protected static const float DCO_INDOOR_RELOCATE_MIN_MOVE_DEFAULT = 2.5;
	protected static const float DCO_INDOOR_RELOCATE_DURATION_SCALE  = 1.5;
	protected static const float DCO_INDOOR_RELOCATE_HOLD_MARGIN_S   = 1.0; 
	
	// === ADDED: Indoor defense gate ===
	protected static const float DCO_INDOOR_TRIGGER_WINDOW_MS    = 3000.0; //! Hit dianggap "baru" selama ini
	protected static const float DCO_INDOOR_RELOCATE_COOLDOWN_MS = 6000.0; //! Jeda antar relocate, nyegah pindah bolak-balik
	protected static const float DCO_INDOOR_OVERRUN_CACHE_MS     = 1000.0; //! Hasil hitung overrun dipake ulang segini

	protected SCR_AIUtilityComponent m_DCOOwnerUtility; // weak, di-set utility component di EOnInit

	protected float m_fDCOLastRelocateTrigger_ms = -1;
	protected float m_fDCOLastRelocate_ms        = -1;

	protected IEntity m_DCOOverrunBuilding;
	protected float   m_fDCOOverrunCheckedAt_ms = -1;
	protected bool    m_bDCOOverrunCached;

	// Koordinasi gate <-> DCO_ApplyCoverRequest
	protected SCR_AICombatMoveRequestBase m_DCOGateReplacement; // weak
	protected float m_fDCOGateReplacementProtect_s;
	protected bool  m_bDCOGateDropped;

	// Scratch buat query overrun
	protected ref array<IEntity> m_aDCOQueryChars = {};
	// === END ADDED ===
	
	protected ref SCR_AICombatMoveRequestBase m_OldRequest;

	//------------------------------------------------------------------------------------------------
	//! Kirim request cover dan lindungi maksimal protect_s detik
	void DCO_ApplyCoverRequest(SCR_AICombatMoveRequestBase rq, float protect_s)
	{
		if (!rq)
			return;

		// === ADDED: Indoor defense gate ===
		m_DCOGateReplacement = null;
		m_bDCOGateDropped    = false;
		// === END ADDED ===

		m_bDCOApplyingCover = true;
		ApplyNewRequest(rq);
		m_bDCOApplyingCover = false;

		// === ADDED: Indoor defense gate ===
		// Gate bisa nge-drop / ngeganti rq di dalem ApplyNewRequest di atas.
		// Diganti relocate -> proteksi dipasang ke request relocate-nya.
		// Di-drop          -> jangan pasang proteksi buat request yang gak pernah jalan.
		if (m_DCOGateReplacement)
		{
			rq        = m_DCOGateReplacement;
			protect_s = m_fDCOGateReplacementProtect_s;
			m_DCOGateReplacement = null;
		}
		else if (m_bDCOGateDropped)
		{
			m_bDCOGateDropped = false;
			return;
		}
		// === END ADDED ===

		float now_ms = GetGame().GetWorld().GetWorldTime();
		m_iDCOBlockedCount         = 0;
		m_DCOCoverRq               = rq;
		m_fDCOCoverAppliedAt_ms    = now_ms;
		m_fDCOCoverProtectUntil_ms = now_ms + Math.Max(protect_s, DCO_COVER_PROTECT_GRACE_S) * 1000.0;
	}

	//------------------------------------------------------------------------------------------------
	//! True selama request cover masih berjalan / menunggu fallback bangunan
	bool DCO_IsCoverProtected()
	{
		if (!m_DCOCoverRq)
			return false;

		float now_ms = GetGame().GetWorld().GetWorldTime();

		// Batas keras
		if (now_ms > m_fDCOCoverProtectUntil_ms)
		{
			DCO_ReleaseCoverProtection("expired");
			return false;
		}

		// Grace: BT belum sempat mulai
		if (now_ms - m_fDCOCoverAppliedAt_ms < DCO_COVER_PROTECT_GRACE_S * 1000.0)
			return true;

		// Bangunan tidak ketemu: tetap dilindungi sampai fallback cover dikirim (atau batas keras)
		if (m_DCOCoverRq.m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
			return true;

		// Masih lari
		if (IsExecutingRequest() && GetRequest() == m_DCOCoverRq)
			return true;

		DCO_ReleaseCoverProtection("finished");
		return false;
	}

	//------------------------------------------------------------------------------------------------
	void DCO_ReleaseCoverProtection(string reason)
	{
		if (!m_DCOCoverRq)
			return;

		if (SCR_AIDangerReaction_WeaponFired.DCO_IsCoverDebugOn())
		{
			Print(string.Format("[DCO_Cover] t=%1 state=%2 | PROTECT RELEASE (%3) fail=%4 blocked=%5",
				GetGame().GetWorld().GetWorldTime(), this, reason,
				typename.EnumToString(SCR_EAICombatMoveRequestFailReason, m_DCOCoverRq.m_eFailReason), m_iDCOBlockedCount), LogLevel.NORMAL);
		}

		m_DCOCoverRq = null;
		m_fDCOCoverProtectUntil_ms = 0;
	}
	
	DCO_AICombatMoveRequest_IndoorRelocate DCO_ApplyIndoorRelocate(IEntity building, vector threatPos, float searchRadius = -1, float minMoveDist = -1, bool protect = true)
	{
		if (!building)
			return null;

		if (searchRadius <= 0)
			searchRadius = DCO_INDOOR_RELOCATE_RADIUS_DEFAULT;

		if (minMoveDist <= 0)
			minMoveDist = DCO_INDOOR_RELOCATE_MIN_MOVE_DEFAULT;

		DCO_AICombatMoveRequest_IndoorRelocate rq = new DCO_AICombatMoveRequest_IndoorRelocate();

		rq.m_eType   = SCR_EAICombatMoveRequestType.INDOOR_RELOCATE;
		rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_DANGER; // biar IsMoving(MOVE_FROM_DANGER) di HideFromThreat ikut nunggu

		rq.m_Building     = building;
		rq.m_fMinMoveDist = minMoveDist;

		rq.m_vTargetPos = threatPos;
		rq.m_vMovePos   = threatPos; // tujuan asli diisi node pencarian di BT (MovePos), ini cuma placeholder
		rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
		rq.m_vAvoidStraightPathDir = vector.Zero;

		// Cover engine gak dipake sama sekali. FailIfNoCover wajib false kalau TryFindCover false.
		rq.m_bTryFindCover              = false;
		rq.m_bFailIfNoCover             = false;
		rq.m_bUseCoverSearchDirectivity = false;
		rq.m_bCheckCoverVisibility      = false;

		// Node pencarian baca radius dari sini.
		rq.m_fCoverSearchDistMin = 0;
		rq.m_fCoverSearchDistMax = searchRadius;

		// Di dalem gedung: jongkok, jangan prone (kepotong ambang jendela / furnitur).
		rq.m_eStanceMoving = ECharacterStance.CROUCH;
		rq.m_eStanceEnd    = ECharacterStance.CROUCH;
		rq.m_eMovementType = EMovementType.RUN;

		rq.m_bAimAtTarget    = DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection);
		rq.m_bAimAtTargetEnd = true;

		rq.m_fMoveDuration_s = (searchRadius / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN) * DCO_INDOOR_RELOCATE_DURATION_SCALE;

		if (protect)
			DCO_ApplyCoverRequest(rq, rq.m_fMoveDuration_s + DCO_INDOOR_RELOCATE_HOLD_MARGIN_S);
		else
			ApplyNewRequest(rq);

		// ApplyNewRequest bisa diam-diam nolak (cover protection). Jangan pura-pura sukses.
		if (GetRequest() != rq)
			return null;

		return rq;
	}

	//------------------------------------------------------------------------------------------------
	override void ApplyNewRequest(notnull SCR_AICombatMoveRequestBase request)
	{
		if (!m_bDCOApplyingCover && DCO_IsCoverProtected())
		{
			m_iDCOBlockedCount++;

			// Log hanya blokir pertama per periode proteksi (sistem lain bisa coba tiap tick)
			if (m_iDCOBlockedCount == 1 && SCR_AIDangerReaction_WeaponFired.DCO_IsCoverDebugOn())
			{
				Print(string.Format("[DCO_Cover] t=%1 state=%2 | PROTECT BLOCKED request lain saat lari ke cover (%3)",
					GetGame().GetWorld().GetWorldTime(), this, request), LogLevel.NORMAL);
			}
			return;
		}

		// === ADDED: Indoor defense gate ===
		if (DCO_IndoorGate(request))
			return;
		// === END ADDED ===

		super.ApplyNewRequest(request);
	}
	
	//================================================================================================
	// === ADDED: Indoor defense gate ===
	// Defender di dalem gedung gak boleh lari keluar cuma gara-gara reaksi danger.
	// Request MOVE / BUILDING yang masuk waktu unit di dalem gedung:
	//   - Overrun (morale BREAK, atau musuh di gedung yang sama >= teman di dalem) -> lolos, boleh keluar.
	//   - Baru kena hit (DCO_MarkIndoorRelocateTrigger) & cooldown siap          -> diganti INDOOR_RELOCATE.
	//   - Selain itu                                                              -> di-drop, unit hold.
	// Gak di-gate: INDOOR_RELOCATE, STOP / change stance, reason INVESTIGATE (AttackPush / clearing),
	// SUPPLYING, FF_AVOIDANCE.
	//================================================================================================

	//------------------------------------------------------------------------------------------------
	void DCO_SetOwnerUtility(SCR_AIUtilityComponent utility)
	{
		m_DCOOwnerUtility = utility;
	}

	//------------------------------------------------------------------------------------------------
	//! Dipanggil sumber yang boleh memicu relocate (sekarang: DamageTaken).
	void DCO_MarkIndoorRelocateTrigger()
	{
		m_fDCOLastRelocateTrigger_ms = GetGame().GetWorld().GetWorldTime();
	}

	//------------------------------------------------------------------------------------------------
	//! True = request udah ditangani gate (di-drop atau diganti relocate), jangan diteruskan ke super.
	protected bool DCO_IndoorGate(SCR_AICombatMoveRequestBase request)
	{
		if (!DCO_IsGatedRequest(request))
			return false;

		IEntity building = SCR_CoverManagerComponent.DCO_GetBuildingAt(m_DCOOwnerUtility.m_OwnerEntity);
		if (!building)
			return false;

		if (DCO_IsOverrun(building))
		{
			DCO_GateDebug(string.Format("OVERRUN -> lolos (%1)", request));
			return false;
		}

		float now_ms = GetGame().GetWorld().GetWorldTime();

		bool triggered     = m_fDCOLastRelocateTrigger_ms >= 0 && (now_ms - m_fDCOLastRelocateTrigger_ms) < DCO_INDOOR_TRIGGER_WINDOW_MS;
		bool cooldownReady = m_fDCOLastRelocate_ms < 0 || (now_ms - m_fDCOLastRelocate_ms) >= DCO_INDOOR_RELOCATE_COOLDOWN_MS;

		if (triggered && cooldownReady)
		{
			vector threatPos = vector.Zero;
			SCR_AICombatMoveRequest_Move moveRq = SCR_AICombatMoveRequest_Move.Cast(request);
			if (moveRq)
				threatPos = moveRq.m_vTargetPos;

			// Lagi di dalem DCO_ApplyCoverRequest: apply tanpa proteksi di sini,
			// DCO_ApplyCoverRequest yang mindahin proteksinya ke request relocate.
			bool nested = m_bDCOApplyingCover;

			DCO_AICombatMoveRequest_IndoorRelocate reloc = DCO_ApplyIndoorRelocate(building, threatPos, -1, -1, !nested);
			if (reloc)
			{
				m_fDCOLastRelocate_ms        = now_ms;
				m_fDCOLastRelocateTrigger_ms = -1; // satu hit = satu relocate

				if (nested)
				{
					m_DCOGateReplacement           = reloc;
					m_fDCOGateReplacementProtect_s = reloc.m_fMoveDuration_s + DCO_INDOOR_RELOCATE_HOLD_MARGIN_S;
				}

				DCO_GateDebug(string.Format("RELOCATE menggantikan %1", request));
				return true;
			}
		}

		if (m_bDCOApplyingCover)
			m_bDCOGateDropped = true;

		DCO_GateDebug(string.Format("HOLD, drop %1 (trigger=%2 cooldown=%3)", request, triggered, cooldownReady));
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool DCO_IsGatedRequest(SCR_AICombatMoveRequestBase request)
	{
		if (!m_DCOOwnerUtility || !m_DCOOwnerUtility.m_OwnerEntity)
			return false;

		if (request.m_eType != SCR_EAICombatMoveRequestType.MOVE && request.m_eType != SCR_EAICombatMoveRequestType.BUILDING)
			return false;

		SCR_EAICombatMoveReason reason = request.m_eReason;
		if (reason == SCR_EAICombatMoveReason.INVESTIGATE)
			return false;

		if (reason == SCR_EAICombatMoveReason.SUPPLYING)
			return false;

		if (reason == SCR_EAICombatMoveReason.FF_AVOIDANCE)
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Overrun = morale BREAK, atau ada musuh di gedung yang sama dan jumlahnya >= teman di dalem
	//! (unit ini ikut dihitung sebagai teman). Hasil hitung di-cache DCO_INDOOR_OVERRUN_CACHE_MS.
	protected bool DCO_IsOverrun(IEntity building)
	{
		DCO_AIMoraleSystem morale = m_DCOOwnerUtility.GetMoraleSystem();
		if (morale && morale.GetState() == moraleState.BREAK)
			return true;

		float now_ms = GetGame().GetWorld().GetWorldTime();
		if (building == m_DCOOverrunBuilding && m_fDCOOverrunCheckedAt_ms >= 0 && (now_ms - m_fDCOOverrunCheckedAt_ms) < DCO_INDOOR_OVERRUN_CACHE_MS)
			return m_bDCOOverrunCached;

		m_DCOOverrunBuilding      = building;
		m_fDCOOverrunCheckedAt_ms = now_ms;
		m_bDCOOverrunCached       = DCO_CountOverrun(building);
		return m_bDCOOverrunCached;
	}

	//------------------------------------------------------------------------------------------------
	protected bool DCO_CountOverrun(IEntity building)
	{
		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(m_DCOOwnerUtility.GetOwner());
		if (!agent)
			return false;

		vector mins, maxs;
		building.GetBounds(mins, maxs);
		vector center = building.CoordToParent((mins + maxs) * 0.5);
		float  radius = 0.5 * vector.Distance(mins, maxs);

		m_aDCOQueryChars.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(center, radius, DCO_OverrunQueryCallback);

		int enemies = 0;
		int friends = 1; // unit ini sendiri

		IEntity myEntity = m_DCOOwnerUtility.m_OwnerEntity;
		foreach (IEntity e : m_aDCOQueryChars)
		{
			if (!e || e == myEntity)
				continue;

			// Karakter di deket gedung belum tentu di DALEM gedung -- pake cek yang sama persis.
			if (SCR_CoverManagerComponent.DCO_GetBuildingAt(e) != building)
				continue;

			if (agent.IsEnemy(e))
				enemies++;
			else
				friends++;
		}

		m_aDCOQueryChars.Clear();

		return enemies > 0 && enemies >= friends;
	}

	//------------------------------------------------------------------------------------------------
	bool DCO_OverrunQueryCallback(IEntity e)
	{
		if (!e || !ChimeraCharacter.Cast(e))
			return true;

		SCR_CharacterDamageManagerComponent dmg = SCR_CharacterDamageManagerComponent.Cast(e.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!dmg || dmg.IsDestroyed())
			return true;

		m_aDCOQueryChars.Insert(e);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void DCO_GateDebug(string msg)
	{
		if (!SCR_AIDangerReaction_WeaponFired.DCO_IsCoverDebugOn())
			return;

		Print(string.Format("[DCO_IndoorGate] t=%1 state=%2 | %3", GetGame().GetWorld().GetWorldTime(), this, msg), LogLevel.NORMAL);
	}
	// === END ADDED ===

	SCR_AICombatMoveRequestBase GetOldRequest()
	{
		return m_OldRequest;
	}
	
	bool IsMovingToBuilding()
	{
	    if (!m_Request)
	        return false;
	
	    if (m_Request.m_eState != SCR_EAICombatMoveRequestState.EXECUTING)
	        return false;
	
	    return m_Request.m_eType == SCR_EAICombatMoveRequestType.BUILDING;
	}
	
	bool IsPositionInBuilding()
	{
		bool isIt = false;
		
		return isIt;
	}
}

modded enum SCR_EAICombatMoveRequestFailReason
{
	NO_BUILDING_FOUND
}

modded class SCR_AICombatMoveRequestBase
{
	SCR_EAICombatMoveRequestType m_eType = SCR_EAICombatMoveRequestType.STOP;
}