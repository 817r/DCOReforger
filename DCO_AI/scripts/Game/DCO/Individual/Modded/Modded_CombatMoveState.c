modded class SCR_AICombatMoveState
{

	float m_fDCOFieldHiddenUntil_ms;

	protected SCR_AICombatMoveRequestBase m_DCOCoverRq;
	protected float m_fDCOCoverAppliedAt_ms;
	protected float m_fDCOCoverProtectUntil_ms;
	protected bool  m_bDCOApplyingCover;
	protected int   m_iDCOBlockedCount;



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
		m_fDCOCoverProtectUntil_ms = now_ms + Math.Max(protect_s, 0.5) * 1000.0;
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

		if (now_ms - m_fDCOCoverAppliedAt_ms < 0.5 * 1000.0)
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
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, "-", string.Format("t=%1 state=%2 | PROTECT RELEASE (%3) fail=%4 blocked=%5",
				GetGame().GetWorld().GetWorldTime(), this, reason,
				typename.EnumToString(SCR_EAICombatMoveRequestFailReason, m_DCOCoverRq.m_eFailReason), m_iDCOBlockedCount));
		}

		m_DCOCoverRq = null;
		m_fDCOCoverProtectUntil_ms = 0;
	}

	DCO_AICombatMoveRequest_IndoorRelocate DCO_ApplyIndoorRelocate(IEntity building, vector threatPos, float searchRadius = -1, float minMoveDist = -1, bool protect = true, string gate = "-")
	{
		if (!building)
			return null;

		if (searchRadius <= 0)
			searchRadius = 12.0;

		if (minMoveDist <= 0)
			minMoveDist = 2.5;

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

		rq.m_fMoveDuration_s = (searchRadius / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN) * 1.5;

		if (protect)
			DCO_ApplyCoverRequest(rq, rq.m_fMoveDuration_s + 1.0);
		else
			ApplyNewRequest(rq);

		if (GetRequest() != rq)
			return null;

		IEntity me;
		if (m_DCOOwnerUtility)
			me = m_DCOOwnerUtility.m_OwnerEntity;
		if (me)
			rq.m_vStartPos = me.GetOrigin();
		rq.m_fDCOStart_ms = GetGame().GetWorld().GetWorldTime();
		rq.GetOnCompleted().Insert(DCO_OnRelocDone);
		rq.GetOnFailed().Insert(DCO_OnRelocFail);
		DCO_Perf.Count("reloc:start");
		string bld = "-";
		if (building.GetPrefabData())
			bld = FilePath.StripExtension(FilePath.StripPath(building.GetPrefabData().GetPrefabName()));
		float threatDist = -1;
		if (me && threatPos != vector.Zero)
			threatDist = vector.DistanceXZ(me.GetOrigin(), threatPos);
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("reloc_start who=%1 intent=%2 gate=%3 bld=%4 threat_dist=%5",
			DCO_Log.IdUnit(me), typename.EnumToString(DCO_EIndoorRelocateIntent, rq.m_eIntent), gate, bld, Math.Round(threatDist)));

		return rq;
	}

	protected void DCO_RelocEnd(SCR_AICombatMoveRequestBase request, string result, string reason)
	{
		DCO_AICombatMoveRequest_IndoorRelocate rq = DCO_AICombatMoveRequest_IndoorRelocate.Cast(request);
		if (!rq)
			return;
		IEntity me;
		if (m_DCOOwnerUtility)
			me = m_DCOOwnerUtility.m_OwnerEntity;
		if (result == "arrived")
			DCO_Perf.Count("reloc:arrived");
		else
			DCO_Perf.Count("reloc:fail_" + reason);
		float moved = -1;
		float left = -1;
		float dy = 0;
		if (me)
		{
			moved = vector.DistanceXZ(rq.m_vStartPos, me.GetOrigin());
			if (rq.m_vDCOGoal != vector.Zero)
			{
				left = vector.DistanceXZ(rq.m_vDCOGoal, me.GetOrigin());
				dy = rq.m_vDCOGoal[1] - me.GetOrigin()[1];
			}
		}
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("reloc_end who=%1 result=%2 reason=%3 moved=%4 ms=%5 left=%6 dy=%7",
			DCO_Log.IdUnit(me), result, reason, Math.Round(moved), Math.Round(GetGame().GetWorld().GetWorldTime() - rq.m_fDCOStart_ms), left.ToString(-1, 1), dy.ToString(-1, 1)));
	}

	void DCO_OnRelocDone(SCR_AIUtilityComponent utility, SCR_AICombatMoveRequestBase request)
	{
		DCO_RelocEnd(request, "arrived", "-");
	}

	void DCO_OnRelocFail(SCR_AIUtilityComponent utility, SCR_AICombatMoveRequestBase request, SCR_EAICombatMoveRequestFailReason failReason)
	{
		DCO_RelocEnd(request, "fail", typename.EnumToString(SCR_EAICombatMoveRequestFailReason, failReason));
	}

	protected float m_fDCOStrafeLogAt_ms;

	protected void DCO_StrafeGate(SCR_AICombatMoveRequest_Move rq)
	{
		SCR_AIUtilityComponent u = m_DCOOwnerUtility;
		if (!u || !u.m_CombatComponent || !u.m_OwnerEntity)
			return;
		if (DCO_Stress.T(u, "tm.strafe_fire", 1.0) <= 0)
			return;
		if (!DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection))
			return;
		BaseTarget t = u.m_CombatComponent.GetCurrentTarget();
		if (!t || !t.GetTargetEntity() || t.GetTimeSinceSeen() > 2.0)
			return;
		EWeaponType wt = u.m_CombatComponent.GetSelectedWeaponType();
		if (wt != EWeaponType.WT_RIFLE && wt != EWeaponType.WT_MACHINEGUN && wt != EWeaponType.WT_HANDGUN)
			return;
		vector me = u.m_OwnerEntity.GetOrigin();
		float tDist = vector.Distance(me, t.GetLastSeenPosition());
		float maxM = DCO_Stress.T(u, "tm.strafe_max_m", 60.0);
		if (wt == EWeaponType.WT_MACHINEGUN)
			maxM = Math.Min(maxM, 40.0);
		if (tDist > maxM)
			return;
		float moveDist = 0;
		if (rq.m_vMovePos != vector.Zero)
			moveDist = vector.DistanceXZ(me, rq.m_vMovePos);
		if (moveDist > 30.0)
			return;
		DCO_AIMoraleSystem ms = u.GetMoraleSystem();
		if (ms && ms.GetState() >= moraleState.PANIC)
			return;
		if (rq.m_bTryFindCover && u.m_ThreatSystem && u.m_ThreatSystem.GetState() == EAIThreatState.THREATENED)
		{
			DCO_Perf.Count("strafe:skip_cover_dash");
			return;
		}
		rq.m_bAimAtTarget = true;
		DCO_Perf.Count("strafe:on");
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fDCOStrafeLogAt_ms > 5000.0 && DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
		{
			m_fDCOStrafeLogAt_ms = now;
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("strafe_fire unit=%1 reason=%2 move=%3 stance=%4 move_dist=%5 tgt_dist=%6 weapon=%7", DCO_Log.IdUnit(u.m_OwnerEntity), typename.EnumToString(SCR_EAICombatMoveReason, rq.m_eReason), typename.EnumToString(EMovementType, rq.m_eMovementType), typename.EnumToString(ECharacterStance, rq.m_eStanceMoving), Math.Round(moveDist), Math.Round(tDist), typename.EnumToString(EWeaponType, wt)));
		}
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
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, "-", string.Format("t=%1 state=%2 | PROTECT BLOCKED another request while running to cover (%3)",
					GetGame().GetWorld().GetWorldTime(), this, request));
			}
			DCO_Drop("cover_protect", request);
			return;
		}

		string drop;
		if (DCO_LeashGate(request))
			drop = "leash";
		else if (DCO_IdleHoldGate(moveRq))
			drop = "idle_hold";
		else if (moveRq && moveRq.m_eReason != SCR_EAICombatMoveReason.MOVE_FROM_DANGER && DCO_LongRange.ShouldHold(m_DCOOwnerUtility))
			drop = "long_range";
		else if (DCO_ContactMoveGate(moveRq))
			drop = "contact_bound";
		else if (DCO_IndoorGate(request))
			drop = "indoor";
		else if (moveRq && DCO_PinGate(moveRq))
			drop = "pin";
		else if (moveRq && DCO_ProneGuard(moveRq))
			drop = "prone_guard";
		else if (moveRq && DCO_FlipGate(moveRq))
			drop = "flip";
		if (!drop.IsEmpty())
		{
			DCO_Drop(drop, request);
			return;
		}

		if (m_DCOOwnerUtility)
			DCO_CQC.ClampProneStance(request, m_DCOOwnerUtility.m_OwnerEntity);

		if (moveRq && moveRq.m_eStanceMoving != ECharacterStance.STAND && m_DCOOwnerUtility && DCO_Nav.Busy(m_DCOOwnerUtility.GetCharacterController(), "req_stance"))
			moveRq.m_eStanceMoving = ECharacterStance.STAND;

		if (moveRq && !moveRq.m_bAimAtTarget)
			DCO_StrafeGate(moveRq);

		if (moveRq && moveRq.m_bTryFindCover)
			DCO_Perf.Count("cm:req_cover");
		else if (moveRq)
			DCO_Perf.Count("cm:req_move");
		else
			DCO_Perf.Count("cm:req_other");
		super.ApplyNewRequest(request);
	}

	protected float m_fDCODropLogAt_ms;

	protected void DCO_Drop(string gate, SCR_AICombatMoveRequestBase request)
	{
		DCO_Perf.Count("cm:drop_" + gate);
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fDCODropLogAt_ms || !m_DCOOwnerUtility || !m_DCOOwnerUtility.m_OwnerEntity || !DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
			return;
		m_fDCODropLogAt_ms = now + 5000.0;
		string action = "-";
		if (m_DCOOwnerUtility.GetCurrentAction())
			action = m_DCOOwnerUtility.GetCurrentAction().ClassName();
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_DCOOwnerUtility.m_OwnerEntity), string.Format("cm_drop unit=%1 gate=%2 rq=%3 action=%4",
			DCO_Log.IdUnit(m_DCOOwnerUtility.m_OwnerEntity), gate, request.ClassName(), action));
	}

	protected int    m_iDCOFlipLast;
	protected int    m_iDCOFlips;
	protected float  m_fDCOFlipStart_ms = -1;
	protected vector m_vDCOFlipStartPos;
	protected float  m_fDCOFlipHoldUntil_ms;
	protected string m_sDCOFlipTrail;

	protected int DCO_MoveSense(SCR_AICombatMoveRequest_Move rq, vector me)
	{
		if (rq.m_eDirection == SCR_EAICombatMoveDirection.FORWARD)
			return 1;
		if (rq.m_eDirection == SCR_EAICombatMoveDirection.BACKWARD)
			return -1;
		if (rq.m_eDirection == SCR_EAICombatMoveDirection.CUSTOM_POS)
		{
			vector toT = DCO_Specialist.Flat(rq.m_vTargetPos - me);
			vector toM = DCO_Specialist.Flat(rq.m_vMovePos - me);
			if (toM.Length() < 1.0 || toT.Length() < 1.0)
				return 0;
			float c = vector.Dot(toT.Normalized(), toM.Normalized());
			if (c > 0.3)
				return 1;
			if (c < -0.3)
				return -1;
			return 0;
		}
		if (rq.m_eReason == SCR_EAICombatMoveReason.MOVE_FROM_DANGER)
			return -1;
		return 0;
	}

	protected bool DCO_FlipGate(SCR_AICombatMoveRequest_Move rq)
	{
		SCR_AIUtilityComponent u = m_DCOOwnerUtility;
		if (!u || !u.m_OwnerEntity)
			return false;
		vector me = u.m_OwnerEntity.GetOrigin();
		float now = GetGame().GetWorld().GetWorldTime();
		int sense = DCO_MoveSense(rq, me);
		bool danger = rq.m_eReason == SCR_EAICombatMoveReason.MOVE_FROM_DANGER;

		if (now < m_fDCOFlipHoldUntil_ms && sense > 0)
		{
			DCO_Perf.Count("cm:flip_hold_block");
			return true;
		}
		if (sense == 0)
			return false;

		if (m_fDCOFlipStart_ms < 0 || now - m_fDCOFlipStart_ms > 20000.0)
		{
			m_fDCOFlipStart_ms = now;
			m_vDCOFlipStartPos = me;
			m_iDCOFlips = 0;
			m_iDCOFlipLast = sense;
			m_sDCOFlipTrail = string.Empty;
		}
		string tag = typename.EnumToString(SCR_EAICombatMoveReason, rq.m_eReason) + "/" + typename.EnumToString(SCR_EAICombatMoveDirection, rq.m_eDirection) + "/" + rq.Type().ToString();
		if (m_sDCOFlipTrail.Length() < 400)
			m_sDCOFlipTrail += tag + ";";
		if (sense != m_iDCOFlipLast)
		{
			m_iDCOFlips++;
			m_iDCOFlipLast = sense;
		}
		if (m_iDCOFlips < 3)
			return false;

		float net = vector.DistanceXZ(me, m_vDCOFlipStartPos);
		float span = (now - m_fDCOFlipStart_ms) / 1000.0;
		int flips = m_iDCOFlips;
		string trail = m_sDCOFlipTrail;
		m_fDCOFlipStart_ms = -1;
		if (net > 6.0)
			return false;

		m_fDCOFlipHoldUntil_ms = now + 10000.0;
		DCO_Perf.Count("cm:flip_detect");
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("move_flip unit=%1 flips=%2 span_s=%3 net=%4 stress=%5 hold_s=10 trail=%6",
			DCO_Log.IdUnit(u.m_OwnerEntity), flips, span.ToString(-1, 1), net.ToString(-1, 1), DCO_Stress.Tier(u), trail));
		return sense > 0;
	}

	protected vector DCO_ReqVec(SCR_AICombatMoveRequest_Move rq, vector me)
	{
		if (rq.m_eDirection == SCR_EAICombatMoveDirection.CUSTOM_POS || (m_DCOOwnerUtility && DCO_Stress.T(m_DCOOwnerUtility, "cm.dir_decode", 1.0) <= 0))
			return rq.m_vMovePos - me;
		vector d = SCR_AICombatMoveUtils.CalculateMoveDirection(rq.m_eDirection, me, rq.m_vMovePos);
		d[1] = 0;
		if (d.Length() < 0.01)
			return rq.m_vMovePos - me;
		d.Normalize();
		DCO_Perf.Count("cm:req_dir_decoded");
		return d * Math.Max(rq.m_fCoverSearchDistMax, 1.0);
	}

	protected bool DCO_PinGate(SCR_AICombatMoveRequest_Move rq)
	{
		SCR_AIUtilityComponent u = m_DCOOwnerUtility;
		if (!u || !u.m_OwnerEntity || !u.m_CombatComponent || !u.m_CombatComponent.DCO_IsPinned())
			return false;
		if (rq.m_eReason == SCR_EAICombatMoveReason.MOVE_FROM_DANGER)
			return false;
		vector enemy = u.m_CombatComponent.m_vDCOPinFrom;
		if (enemy == vector.Zero)
			enemy = rq.m_vTargetPos;
		vector mv = DCO_ReqVec(rq, u.m_OwnerEntity.GetOrigin());
		vector toE = enemy - u.m_OwnerEntity.GetOrigin();
		mv[1] = 0;
		toE[1] = 0;
		float dist = mv.Length();
		if (dist > 0.5 && toE.Length() > 0.5)
		{
			float cosA = vector.Dot(mv.Normalized(), toE.Normalized());
			if (cosA < -0.5 && dist <= 5.0)
			{
				rq.m_eStanceMoving = ECharacterStance.PRONE;
				rq.m_eStanceEnd = ECharacterStance.PRONE;
				return false;
			}
			if (cosA < 0.5 && dist <= 3.0)
				return false;
		}
		DCO_Perf.Count("pin_block_move");
		return true;
	}

	protected static int s_iDCOBackOk;
	protected static int s_iDCOBackFail;
	protected static bool s_bDCOBackOff;

	protected bool DCO_ProneGuard(SCR_AICombatMoveRequest_Move rq)
	{
		SCR_AIUtilityComponent u = m_DCOOwnerUtility;
		if (!u || !u.m_OwnerEntity || !u.m_CombatComponent || !DCO_GlobalAIComponent.BehaviorOnUtility(u, DCO_EBehavior.PRONE_MOVE))
			return false;
		if (u.m_AIInfo && u.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
			return false;
		SCR_CharacterControllerComponent charCon = u.m_CombatComponent.GetCharacterController();
		if (!charCon)
			return false;

		bool curProne = charCon.GetStance() == ECharacterStance.PRONE;
		bool wantProne = rq.m_eStanceMoving == ECharacterStance.PRONE;
		if (!curProne && !wantProne)
			return false;

		vector enemy = rq.m_vTargetPos;
		BaseTarget tgt = u.m_CombatComponent.GetCurrentTarget();
		if (enemy == vector.Zero && tgt)
			enemy = tgt.GetLastSeenPosition();
		if (enemy == vector.Zero)
			return false;

		vector me = u.m_OwnerEntity.GetOrigin();
		vector mv = DCO_ReqVec(rq, me);
		mv[1] = 0;
		float dist = mv.Length();
		vector toE = enemy - me;
		toE[1] = 0;
		if (dist < 0.5 || toE.Length() < 0.5)
			return false;
		mv.Normalize();
		toE.Normalize();
		float cosA = vector.Dot(mv, toE);
		bool forward = cosA >= 0.5;
		bool back = cosA < -0.5;
		bool side = !forward && !back;

		if (!wantProne && curProne)
		{
			if (side && dist <= 3.0)
				return DCO_Roll(charCon, mv, dist);
			if (back && dist <= 5.0 && !s_bDCOBackOff)
				wantProne = true;
			else if (forward && (dist <= 15.0 || DCO_CrawlUncapped(u, dist)) && DCO_CrawlTrigger(u, dist))
				wantProne = true;
			if (!wantProne)
				return false;
			rq.m_eStanceMoving = ECharacterStance.PRONE;
			rq.m_eStanceEnd = ECharacterStance.PRONE;
		}

		if (forward)
		{
			if (dist <= 15.0)
			{
				DCO_Perf.Count("prone_crawl_fwd");
				return false;
			}
			if (DCO_CrawlUncapped(u, dist))
			{
				DCO_Perf.Count("prone_crawl_uncapped");
				return false;
			}
			rq.m_eStanceMoving = ECharacterStance.CROUCH;
			DCO_Perf.Count("prone_to_crouch:far_fwd");
			return false;
		}
		if (side)
		{
			if (dist <= 3.0 && curProne)
				return DCO_Roll(charCon, mv, dist);
			rq.m_eStanceMoving = ECharacterStance.CROUCH;
			DCO_Perf.Count("prone_to_crouch:side");
			return false;
		}
		if (dist <= 5.0 && !s_bDCOBackOff)
		{
			rq.m_bAimAtTarget = true;
			rq.m_vTargetPos = enemy;
			DCO_Perf.Count("prone_crawl_back");
			GetGame().GetCallqueue().CallLater(DCO_CheckBackCrawl, 1500, false, enemy);
			return false;
		}
		rq.m_eStanceMoving = ECharacterStance.CROUCH;
		DCO_Perf.Count("prone_to_crouch:back");
		return false;
	}

	protected bool DCO_CrawlUncapped(SCR_AIUtilityComponent u, float dist)
	{
		if (dist <= 10.0 || !u.m_CombatComponent || (u.m_CombatMoveState && u.m_CombatMoveState.IsInValidCover()))
			return false;
		if (u.m_CombatComponent.DCO_IsPinned())
			return true;
		return u.m_CombatComponent.DCO_NearMissCount(GetGame().GetWorld().GetWorldTime(), 3000.0) > 0;
	}

	protected bool DCO_CrawlTrigger(SCR_AIUtilityComponent u, float dist)
	{
		if (u.m_ThreatSystem && u.m_ThreatSystem.GetSuppressionMeasure() > 0.6)
			return true;
		DCO_GroupTactics p = DCO_PostureCombatUtility.GetPosture(u);
		if (p == DCO_GroupTactics.SNEAK || p == DCO_GroupTactics.AMBUSH)
			return true;
		if (dist <= 5.0)
		{
			EWeaponType wt = u.m_CombatComponent.GetSelectedWeaponType();
			if (wt == EWeaponType.WT_MACHINEGUN || wt == EWeaponType.WT_SNIPERRIFLE)
				return true;
		}
		return false;
	}

	protected bool DCO_Roll(SCR_CharacterControllerComponent charCon, vector moveDir, float dist)
	{
		vector right = m_DCOOwnerUtility.m_OwnerEntity.GetWorldTransformAxis(0);
		right[1] = 0;
		int dir = 1;
		if (vector.Dot(right, moveDir) > 0)
			dir = 2;
		int rolls = Math.ClampInt(Math.Round(dist), 1, 3);
		for (int i = 0; i < rolls; i++)
			charCon.SetRoll(dir);
		DCO_Perf.Count("prone_roll_side");
		return true;
	}

	protected void DCO_CheckBackCrawl(vector enemy)
	{
		if (!m_DCOOwnerUtility || !m_DCOOwnerUtility.m_OwnerEntity || !m_DCOOwnerUtility.m_CombatComponent)
			return;
		SCR_CharacterControllerComponent charCon = m_DCOOwnerUtility.m_CombatComponent.GetCharacterController();
		if (!charCon || charCon.GetStance() != ECharacterStance.PRONE)
			return;
		IEntity ent = m_DCOOwnerUtility.m_OwnerEntity;
		vector fwd = ent.GetWorldTransformAxis(2);
		vector toE = enemy - ent.GetOrigin();
		fwd[1] = 0;
		toE[1] = 0;
		if (fwd.Length() < 0.1 || toE.Length() < 0.1)
			return;
		if (vector.Dot(fwd.Normalized(), toE.Normalized()) > 0)
		{
			s_iDCOBackOk++;
			DCO_Perf.Count("prone_back_facing_ok");
			return;
		}
		s_iDCOBackFail++;
		DCO_Perf.Count("prone_back_facing_fail");
		if (!s_bDCOBackOff && s_iDCOBackFail >= 3 && s_iDCOBackFail > s_iDCOBackOk * 2)
		{
			s_bDCOBackOff = true;
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, "-", string.Format("prone_back_crawl_disabled ok=%1 fail=%2", s_iDCOBackOk, s_iDCOBackFail));
		}
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
		if (charCon && charCon.GetStance() == ECharacterStance.STAND && !DCO_Nav.Busy(charCon, "dodge_stance"))
			DCO_Stance.Request(charCon, 2, "move_state");
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

		DCO_GroupTactics posture = DCO_PostureCombatUtility.GetPosture(m_DCOOwnerUtility);
		int tier = DCO_Stress.Tier(m_DCOOwnerUtility);
		bool aggStand = posture == DCO_GroupTactics.AGGRESIVE && tier == 0;
		if (rq.m_eStanceMoving == ECharacterStance.STAND && rq.m_eMovementType != EMovementType.SPRINT && !aggStand)
			rq.m_eStanceMoving = ECharacterStance.CROUCH;

		bool advancing = rq.m_eReason == SCR_EAICombatMoveReason.STANDARD
			&& (rq.m_eDirection == SCR_EAICombatMoveDirection.FORWARD || rq.m_eDirection == SCR_EAICombatMoveDirection.CUSTOM_POS);
		if (!advancing)
			return false;

		SCR_AIGroup group = SCR_AIGroup.Cast(m_DCOOwnerUtility.GetOwner().GetParentGroup());
		if (!group || !group.GetGroupUtilityComponent() || !group.GetGroupUtilityComponent().m_FireteamMgr)
			return false;

		float period = DCO_PostureCombatUtility.BoundPeriod_ms(posture);
		if (period <= 0)
			return false;
		bool bounding;
		bool myTurn = group.GetGroupUtilityComponent().m_FireteamMgr.DCO_IsBoundingTurn(m_DCOOwnerUtility.GetOwner(), bounding, period);
		if (bounding && myTurn)
			return false;

		bool sheltered = m_bInCover || SCR_CoverManagerComponent.IsEntityInsideBuilding(m_DCOOwnerUtility.m_OwnerEntity);
		if (bounding && sheltered)
			return true;

		if (bounding && !sheltered)
		{
			if (tier >= 1)
				rq.m_eStanceEnd = ECharacterStance.PRONE;
			DCO_Perf.Count("tm:gate_open_allow");
		}

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
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(group);
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
			DCO_GateDebug(string.Format("CQC ADVANCE -> passed (%1)", request));
			return false;
		}

		AIGroup ownGroup = m_DCOOwnerUtility.GetOwner().GetParentGroup();
		SCR_AIGroupUtilityComponent ownGu;
		if (ownGroup)
			ownGu = SCR_AIGroupUtilityComponent.Cast(ownGroup.FindComponent(SCR_AIGroupUtilityComponent));
		if (ownGu && ownGu.DCO_IsClearingBuilding())
		{
			DCO_Perf.Count("cm:indoor_pass_clearing");
			return false;
		}

		if (DCO_IsOverrun(building) && DCO_MayAbandonBuilding())
		{
			DCO_GateDebug(string.Format("OVERRUN -> passed (%1)", request));
			return false;
		}

		float now_ms = GetGame().GetWorld().GetWorldTime();

		bool triggered     = m_fDCOLastRelocateTrigger_ms >= 0 && (now_ms - m_fDCOLastRelocateTrigger_ms) < 3000.0;
		bool cooldownReady = m_fDCOLastRelocate_ms < 0 || (now_ms - m_fDCOLastRelocate_ms) >= 15000.0;

		if (triggered && !cooldownReady)
			DCO_Perf.Count("reloc:cooldown_block");

		if (triggered && cooldownReady)
		{
			vector threatPos = vector.Zero;
			SCR_AICombatMoveRequest_Move moveRq = SCR_AICombatMoveRequest_Move.Cast(request);
			if (moveRq)
				threatPos = moveRq.m_vTargetPos;

			bool nested = m_bDCOApplyingCover;

			DCO_AICombatMoveRequest_IndoorRelocate reloc = DCO_ApplyIndoorRelocate(building, threatPos, -1, -1, !nested, "indoor");
			if (reloc)
			{
				m_fDCOLastRelocate_ms        = now_ms;
				m_fDCOLastRelocateTrigger_ms = -1;

				if (nested)
				{
					m_DCOGateReplacement           = reloc;
					m_fDCOGateReplacementProtect_s = reloc.m_fMoveDuration_s + 1.0;
				}

				DCO_GateDebug(string.Format("RELOCATE replaces %1", request));
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
			|| (m_fDCOLastRelocateTrigger_ms >= 0 && (now_ms - m_fDCOLastRelocateTrigger_ms) < 3000.0);
		bool cooldownReady = m_fDCOLastRelocate_ms < 0 || (now_ms - m_fDCOLastRelocate_ms) >= 15000.0;

		if (triggered && !cooldownReady)
			DCO_Perf.Count("reloc:cooldown_block");

		if (triggered && cooldownReady)
		{
			vector threatPos = vector.Zero;
			if (moveRq)
				threatPos = moveRq.m_vTargetPos;

			bool nested = m_bDCOApplyingCover;
			DCO_AICombatMoveRequest_IndoorRelocate reloc = DCO_ApplyIndoorRelocate(cfg.GetLeashBuilding(), threatPos, -1, -1, !nested, "leash");
			if (reloc)
			{
				m_fDCOLastRelocate_ms        = now_ms;
				m_fDCOLastRelocateTrigger_ms = -1;

				if (nested)
				{
					m_DCOGateReplacement           = reloc;
					m_fDCOGateReplacementProtect_s = reloc.m_fMoveDuration_s + 1.0;
				}

				DCO_GateDebug(string.Format("LEASH RELOCATE replaces %1", request));
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
		if (building == m_DCOOverrunBuilding && m_fDCOOverrunCheckedAt_ms >= 0 && (now_ms - m_fDCOOverrunCheckedAt_ms) < 1000.0)
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

		return enemies > 0 && enemies >= 2 * friends;
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

		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, "-", string.Format("t=%1 state=%2 | %3", GetGame().GetWorld().GetWorldTime(), this, msg));
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
	NO_BUILDING_FOUND,
	MOVE_TIMEOUT
}

modded class SCR_AICombatMoveRequestBase
{
	SCR_EAICombatMoveRequestType m_eType = SCR_EAICombatMoveRequestType.STOP;
}
