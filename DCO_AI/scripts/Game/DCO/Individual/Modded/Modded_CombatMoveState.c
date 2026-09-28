modded class SCR_AICombatMoveState
{
	protected static const float DCO_COVER_PROTECT_GRACE_S = 0.5;

	protected SCR_AICombatMoveRequestBase m_DCOCoverRq;
	protected float m_fDCOCoverAppliedAt_ms;
	protected float m_fDCOCoverProtectUntil_ms;
	protected bool  m_bDCOApplyingCover;
	protected int   m_iDCOBlockedCount;

	protected static const float DCO_INDOOR_RELOCATE_RADIUS_DEFAULT   = 12.0;
	protected static const float DCO_INDOOR_RELOCATE_MIN_MOVE_DEFAULT = 2.5;
	protected static const float DCO_INDOOR_RELOCATE_DURATION_SCALE  = 1.5;
	protected static const float DCO_INDOOR_RELOCATE_HOLD_MARGIN_S   = 1.0;

	protected static const float DCO_INDOOR_TRIGGER_WINDOW_MS    = 3000.0;
	protected static const float DCO_INDOOR_RELOCATE_COOLDOWN_MS = 6000.0;
	protected static const float DCO_INDOOR_OVERRUN_CACHE_MS     = 1000.0;
	protected static const int   DCO_OVERRUN_ENEMY_RATIO         = 2;

	protected SCR_AIUtilityComponent m_DCOOwnerUtility;

	protected float m_fDCOLastRelocateTrigger_ms = -1;
	protected float m_fDCOLastRelocate_ms        = -1;

	protected IEntity m_DCOOverrunBuilding;
	protected float   m_fDCOOverrunCheckedAt_ms = -1;
	protected bool    m_bDCOOverrunCached;

	protected SCR_AICombatMoveRequestBase m_DCOGateReplacement;
	protected float m_fDCOGateReplacementProtect_s;
	protected bool  m_bDCOGateDropped;

	protected ref array<IEntity> m_aDCOQueryChars = {};

	void DCO_ApplyCoverRequest(SCR_AICombatMoveRequestBase rq, float protect_s)
	{
		if (!rq)
			return;

		m_DCOGateReplacement = null;
		m_bDCOGateDropped    = false;

		m_bDCOApplyingCover = true;
		ApplyNewRequest(rq);
		m_bDCOApplyingCover = false;

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

		float now_ms = GetGame().GetWorld().GetWorldTime();
		m_iDCOBlockedCount         = 0;
		m_DCOCoverRq               = rq;
		m_fDCOCoverAppliedAt_ms    = now_ms;
		m_fDCOCoverProtectUntil_ms = now_ms + Math.Max(protect_s, DCO_COVER_PROTECT_GRACE_S) * 1000.0;
	}

	bool DCO_IsCoverProtected()
	{
		if (!m_DCOCoverRq)
			return false;

		float now_ms = GetGame().GetWorld().GetWorldTime();

		if (now_ms > m_fDCOCoverProtectUntil_ms)
		{
			DCO_ReleaseCoverProtection("expired");
			return false;
		}

		if (now_ms - m_fDCOCoverAppliedAt_ms < DCO_COVER_PROTECT_GRACE_S * 1000.0)
			return true;

		if (m_DCOCoverRq.m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
			return true;

		if (IsExecutingRequest() && GetRequest() == m_DCOCoverRq)
			return true;

		DCO_ReleaseCoverProtection("finished");
		return false;
	}

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
		rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;

		rq.m_Building     = building;
		rq.m_fMinMoveDist = minMoveDist;

		rq.m_vTargetPos = threatPos;
		rq.m_vMovePos   = threatPos;
		rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
		rq.m_vAvoidStraightPathDir = vector.Zero;

		rq.m_bTryFindCover              = false;
		rq.m_bFailIfNoCover             = false;
		rq.m_bUseCoverSearchDirectivity = false;
		rq.m_bCheckCoverVisibility      = false;

		rq.m_fCoverSearchDistMin = 0;
		rq.m_fCoverSearchDistMax = searchRadius;

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

		if (GetRequest() != rq)
			return null;

		return rq;
	}

	override void ApplyNewRequest(notnull SCR_AICombatMoveRequestBase request)
	{
		if (SCR_AICombatMoveRequest_Move.Cast(request) && m_DCOOwnerUtility && m_DCOOwnerUtility.m_DCOConfig
			&& m_DCOOwnerUtility.m_DCOConfig.IsHoldPosition())
			return;

		DCO_CQC.ClampProne(m_DCOOwnerUtility, request);

		SCR_AICombatMoveRequest_Move moveRq = SCR_AICombatMoveRequest_Move.Cast(request);
		if (moveRq && DCO_CQC.IsCQC(m_DCOOwnerUtility))
			DCO_CQC.ClampMoveRequest(moveRq);

		if (!m_bDCOApplyingCover && DCO_IsCoverProtected())
		{
			m_iDCOBlockedCount++;

			if (m_iDCOBlockedCount == 1 && SCR_AIDangerReaction_WeaponFired.DCO_IsCoverDebugOn())
			{
				Print(string.Format("[DCO_Cover] t=%1 state=%2 | PROTECT BLOCKED request lain saat lari ke cover (%3)",
					GetGame().GetWorld().GetWorldTime(), this, request), LogLevel.NORMAL);
			}
			return;
		}

		if (DCO_LeashGate(request))
			return;

		if (DCO_IdleHoldGate(moveRq))
			return;

		if (moveRq && moveRq.m_eReason != SCR_EAICombatMoveReason.MOVE_FROM_DANGER && DCO_LongRange.ShouldHold(m_DCOOwnerUtility))
			return;

		if (DCO_ContactMoveGate(moveRq))
			return;

		if (DCO_IndoorGate(request))
			return;

		super.ApplyNewRequest(request);
	}

	void DCO_SetDodgeStance(notnull SCR_AICombatMoveRequest_Move rq)
	{
		rq.m_eStanceEnd = ECharacterStance.PRONE;
		rq.GetOnCompleted().Insert(DCO_OnDodgeCompleted);
	}

	protected void DCO_OnDodgeCompleted(SCR_AIUtilityComponent utility, SCR_AICombatMoveRequestBase request)
	{
		if (!IsInValidCover() || !utility || !utility.m_CombatComponent)
			return;

		SCR_CharacterControllerComponent charCon = utility.m_CombatComponent.GetCharacterController();
		if (charCon && charCon.GetStance() == ECharacterStance.STAND)
			charCon.SetStanceChange(2);
	}

	protected bool DCO_ContactMoveGate(SCR_AICombatMoveRequest_Move rq)
	{
		if (!rq || !m_DCOOwnerUtility || !m_DCOOwnerUtility.m_ThreatSystem || !m_DCOOwnerUtility.m_OwnerEntity)
			return false;

		EAIThreatState threat = m_DCOOwnerUtility.m_ThreatSystem.GetState();
		if (threat != EAIThreatState.ALERTED && threat != EAIThreatState.THREATENED)
			return false;

		if (m_DCOOwnerUtility.m_AIInfo && m_DCOOwnerUtility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
			return false;

		if (DCO_CQC.IsCQC(m_DCOOwnerUtility))
			return false;

		if (rq.m_eStanceMoving == ECharacterStance.STAND && rq.m_eMovementType != EMovementType.SPRINT)
			rq.m_eStanceMoving = ECharacterStance.CROUCH;

		bool advancing = rq.m_eReason == SCR_EAICombatMoveReason.STANDARD
			&& (rq.m_eDirection == SCR_EAICombatMoveDirection.FORWARD || rq.m_eDirection == SCR_EAICombatMoveDirection.CUSTOM_POS);
		if (!advancing)
			return false;

		SCR_AIGroup group = SCR_AIGroup.Cast(m_DCOOwnerUtility.GetOwner().GetParentGroup());
		if (!group || !group.GetGroupUtilityComponent() || !group.GetGroupUtilityComponent().m_FireteamMgr)
			return false;

		bool bounding;
		bool myTurn = group.GetGroupUtilityComponent().m_FireteamMgr.DCO_IsBoundingTurn(m_DCOOwnerUtility.GetOwner(), bounding);
		if (bounding && myTurn)
			return false;

		bool sheltered = m_bInCover || SCR_CoverManagerComponent.IsEntityInsideBuilding(m_DCOOwnerUtility.m_OwnerEntity);
		if (bounding && sheltered)
			return true;

		if (rq.m_bTryFindCover)
			rq.m_bFailIfNoCover = true;

		return false;
	}

	protected bool DCO_IdleHoldGate(SCR_AICombatMoveRequest_Move rq)
	{
		if (!rq || !m_bInCover || rq.m_eReason == SCR_EAICombatMoveReason.MOVE_FROM_DANGER || !m_DCOOwnerUtility)
			return false;

		SCR_AIGroup group = SCR_AIGroup.Cast(m_DCOOwnerUtility.GetOwner().GetParentGroup());
		if (!group)
			return false;

		SCR_AIGroupUtilityComponent groupUtil = group.GetGroupUtilityComponent();
		if (!groupUtil || !groupUtil.DCO_IsIdle() || groupUtil.DCO_HasClusterActivity())
			return false;

		float overrun = 15;
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (cfg)
			overrun = cfg.GetIdleOverrunDist();

		PerceptionComponent perception = m_DCOOwnerUtility.m_PerceptionComponent;
		if (!perception)
			return false;

		BaseTarget enemy = perception.GetClosestTarget(ETargetCategory.ENEMY, 10, 10);
		if (enemy && enemy.GetDistance() <= overrun)
			return false;

		BaseTarget detected = perception.GetClosestTarget(ETargetCategory.DETECTED, 10, 10);
		return !detected || detected.GetDistance() > overrun;
	}

	void DCO_SetOwnerUtility(SCR_AIUtilityComponent utility)
	{
		m_DCOOwnerUtility = utility;
	}

	void DCO_MarkIndoorRelocateTrigger()
	{
		m_fDCOLastRelocateTrigger_ms = GetGame().GetWorld().GetWorldTime();
	}

	protected bool DCO_IndoorGate(SCR_AICombatMoveRequestBase request)
	{
		if (!DCO_IsGatedRequest(request))
			return false;

		IEntity building = SCR_CoverManagerComponent.DCO_GetBuildingAt(m_DCOOwnerUtility.m_OwnerEntity);
		if (!building)
			return false;

		if (DCO_CQC.IsAdvanceRequest(request) && (DCO_CQC.IsCloseFight(m_DCOOwnerUtility) || DCO_CQC.ShouldAssault(m_DCOOwnerUtility)))
		{
			DCO_GateDebug(string.Format("CQC ADVANCE -> lolos (%1)", request));
			return false;
		}

		if (DCO_IsOverrun(building) && DCO_MayAbandonBuilding())
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

			bool nested = m_bDCOApplyingCover;

			DCO_AICombatMoveRequest_IndoorRelocate reloc = DCO_ApplyIndoorRelocate(building, threatPos, -1, -1, !nested);
			if (reloc)
			{
				m_fDCOLastRelocate_ms        = now_ms;
				m_fDCOLastRelocateTrigger_ms = -1;

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

	protected bool DCO_LeashGate(SCR_AICombatMoveRequestBase request)
	{
		if (!m_DCOOwnerUtility || !m_DCOOwnerUtility.m_DCOConfig)
			return false;

		DCO_AIConfigComponent cfg = m_DCOOwnerUtility.m_DCOConfig;
		DCO_ELeashType leash = cfg.GetLeashType();
		if (leash != DCO_ELeashType.BUILDING && leash != DCO_ELeashType.RADIUS)
			return false;

		if (request.m_eType != SCR_EAICombatMoveRequestType.MOVE && request.m_eType != SCR_EAICombatMoveRequestType.BUILDING)
			return false;

		if (request.m_eReason == SCR_EAICombatMoveReason.SUPPLYING || request.m_eReason == SCR_EAICombatMoveReason.FF_AVOIDANCE)
			return false;

		SCR_AICombatMoveRequest_Move moveRq = SCR_AICombatMoveRequest_Move.Cast(request);

		if (leash == DCO_ELeashType.RADIUS)
		{
			if (moveRq && moveRq.m_eDirection == SCR_EAICombatMoveDirection.CUSTOM_POS && !cfg.LeashAllows(moveRq.m_vMovePos))
			{
				if (m_bDCOApplyingCover)
					m_bDCOGateDropped = true;
				DCO_GateDebug(string.Format("LEASH RADIUS drop %1", request));
				return true;
			}
			return false;
		}

		if (!cfg.GetLeashBuilding())
			return false;
		if (moveRq && moveRq.m_eDirection == SCR_EAICombatMoveDirection.CUSTOM_POS && cfg.LeashAllows(moveRq.m_vMovePos))
			return false;

		float now_ms = GetGame().GetWorld().GetWorldTime();
		bool triggered = request.m_eReason == SCR_EAICombatMoveReason.MOVE_FROM_DANGER
			|| (m_fDCOLastRelocateTrigger_ms >= 0 && (now_ms - m_fDCOLastRelocateTrigger_ms) < DCO_INDOOR_TRIGGER_WINDOW_MS);
		bool cooldownReady = m_fDCOLastRelocate_ms < 0 || (now_ms - m_fDCOLastRelocate_ms) >= DCO_INDOOR_RELOCATE_COOLDOWN_MS;

		if (triggered && cooldownReady)
		{
			vector threatPos = vector.Zero;
			if (moveRq)
				threatPos = moveRq.m_vTargetPos;

			bool nested = m_bDCOApplyingCover;
			DCO_AICombatMoveRequest_IndoorRelocate reloc = DCO_ApplyIndoorRelocate(cfg.GetLeashBuilding(), threatPos, -1, -1, !nested);
			if (reloc)
			{
				m_fDCOLastRelocate_ms        = now_ms;
				m_fDCOLastRelocateTrigger_ms = -1;

				if (nested)
				{
					m_DCOGateReplacement           = reloc;
					m_fDCOGateReplacementProtect_s = reloc.m_fMoveDuration_s + DCO_INDOOR_RELOCATE_HOLD_MARGIN_S;
				}

				DCO_GateDebug(string.Format("LEASH RELOCATE menggantikan %1", request));
				return true;
			}
		}

		if (m_bDCOApplyingCover)
			m_bDCOGateDropped = true;

		DCO_GateDebug(string.Format("LEASH drop %1", request));
		return true;
	}

	protected bool DCO_MayAbandonBuilding()
	{
		DCO_AIMoraleSystem morale = m_DCOOwnerUtility.GetMoraleSystem();
		if (morale && morale.GetState() == moraleState.BREAK)
			return true;

		DCO_EAIPersonality p = DCO_PersonalityCombatUtility.GetPersonalitySafe(m_DCOOwnerUtility);
		return p != DCO_EAIPersonality.AGGRESSIVE && p != DCO_EAIPersonality.RECKLESS;
	}

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
		DCO_Perf.Count("q:Modded_CombatMoveState");
		GetGame().GetWorld().QueryEntitiesBySphere(center, radius, DCO_OverrunQueryCallback);

		int enemies = 0;
		int friends = 1;

		IEntity myEntity = m_DCOOwnerUtility.m_OwnerEntity;
		foreach (IEntity e : m_aDCOQueryChars)
		{
			if (!e || e == myEntity)
				continue;

			if (SCR_CoverManagerComponent.DCO_GetBuildingAt(e) != building)
				continue;

			if (agent.IsEnemy(e))
				enemies++;
			else
				friends++;
		}

		m_aDCOQueryChars.Clear();

		return enemies > 0 && enemies >= DCO_OVERRUN_ENEMY_RATIO * friends;
	}

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

	protected void DCO_GateDebug(string msg)
	{
		if (!SCR_AIDangerReaction_WeaponFired.DCO_IsCoverDebugOn())
			return;

		Print(string.Format("[DCO_IndoorGate] t=%1 state=%2 | %3", GetGame().GetWorld().GetWorldTime(), this, msg), LogLevel.NORMAL);
	}

	SCR_AICombatMoveRequestBase GetOldRequest()
	{
		return m_Request;
	}

	bool IsMovingToBuilding()
	{
	    if (!m_Request)
	        return false;

	    if (m_Request.m_eState != SCR_EAICombatMoveRequestState.EXECUTING)
	        return false;

	    return m_Request.m_eType == SCR_EAICombatMoveRequestType.BUILDING;
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
