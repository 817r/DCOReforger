enum DCO_EGunnerState
{
	WATCH,
	FIRE,
	SUPPRESS,
	SETTLE,
	MOVE,
	LOB
}

class DCO_GunnerBehavior : SCR_AIBehaviorBase
{
	protected string m_sRole;
	protected int m_iState = -1;
	protected vector m_vThreat;
	protected bool m_bMovePend;
	protected vector m_vMovePos;
	protected bool m_bMoveCover;
	protected ECharacterStance m_eMoveStance = ECharacterStance.PRONE;
	protected float m_fMoveDur_s;
	protected string m_sMoveWhy = "-";
	protected bool m_bMoveCrit;
	protected int m_iMoveRetry;
	protected int m_iResume;
	protected float m_fResumeAt_ms;
	protected ref SCR_AICombatMoveRequest_Move m_Rq;
	protected float m_fMoveSent_ms;
	protected float m_fMoveUntil_ms;
	protected vector m_vMoveFrom;
	protected float m_fSettleUntil_ms;
	protected ref SCR_AISuppressionVolumeBase m_Vol;
	protected float m_fRate;
	protected float m_fSuppUntil_ms;
	protected string m_sSuppWhy = "-";
	protected float m_fLastJob_ms;
	protected float m_fCritUntil_ms;
	protected string m_sYield;
	protected bool m_bDone;
	protected float m_fStart_ms;
	protected float m_fStateAt_ms;
	protected bool m_bReloadLow;
	protected int m_iMoves;
	protected int m_iBursts;
	protected vector m_vFinal;
	protected bool m_bRush;
	protected ECharacterStance m_eFinalStance = ECharacterStance.PRONE;
	protected bool m_bFinalCover;
	protected float m_fLegPauseUntil_ms;
	protected int m_iLeg;
	protected bool m_bFireRecent = true;
	protected bool m_bHold;
	protected string m_sHoldWhy = "-";
	protected vector m_vLob;
	protected float m_fLobUntil_ms;
	protected string m_sLobWhy = "-";
	protected int m_iLobs;
	protected float m_fWatchUntil_ms;
	protected string m_sWatchWhy = "-";

	void DCO_GunnerBehavior(SCR_AIUtilityComponent utility, SCR_AIActivityBase groupActivity, string role, vector threat)
	{
		m_sBehaviorTree = "AI/BehaviorTrees/Chimera/Soldier/DCO_Spec_MG.bt";
		SetPriority(95);
		m_sRole = role;
		m_vThreat = threat;
		m_bFireRecent = role == "mg" || role == "ar" || role == "mm";
		m_bUseCombatMove = true;
		m_bAllowLook = false;
		m_bResetLook = true;
		if (!GetGame() || !GetGame().GetWorld())
			return;
		m_fStart_ms = GetGame().GetWorld().GetWorldTime();
		m_fLastJob_ms = m_fStart_ms;
	}

	static bool On(SCR_AIUtilityComponent u)
	{
		return DCO_Stress.T(u, "spec.bt", 1.0) > 0;
	}

	static DCO_GunnerBehavior Get(SCR_AIUtilityComponent u)
	{
		if (!u)
			return null;
		DCO_GunnerBehavior b = DCO_GunnerBehavior.Cast(u.FindActionOfType(DCO_GunnerBehavior));
		if (!b || b.m_bDone)
			return null;
		return b;
	}

	static DCO_GunnerBehavior Ensure(SCR_AIUtilityComponent u, string role, vector threat, float now)
	{
		if (!u || !u.m_OwnerEntity || !u.m_CombatMoveState || !On(u) || (u.m_AIInfo && u.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE)))
			return null;
		DCO_GunnerBehavior b = Get(u);
		if (b)
		{
			b.Touch(now, threat);
			return b;
		}
		b = new DCO_GunnerBehavior(u, null, role, threat);
		u.AddAction(b);
		DCO_Perf.Count("spec_bt:start_" + role);
		string hand = "-";
		DCO_RelocateBehavior old = DCO_RelocateBehavior.Cast(u.FindActionOfType(DCO_RelocateBehavior));
		if (old && !old.DCO_IsDone() && (old.DCO_Why().StartsWith("mg_") || old.DCO_Why().StartsWith("ar_") || old.DCO_Why().StartsWith("at_") || old.DCO_Why().StartsWith("marksman_")))
		{
			vector p;
			vector th;
			ECharacterStance stc;
			bool cov;
			float dur;
			old.DCO_GetJob(p, th, stc, cov, dur);
			hand = old.DCO_Why();
			old.DCO_Supersede("spec_bt");
			b.MoveTo(p, th, cov, stc, dur, hand, false);
			DCO_Perf.Count("spec_bt:handover");
		}
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("spec_bt_start unit=%1 role=%2 threat=%3 threat_dist=%4 prio=95 handover=%5", DCO_Log.IdUnit(u.m_OwnerEntity), role, DCO_Log.Pos(threat), Math.Round(vector.DistanceXZ(u.m_OwnerEntity.GetOrigin(), threat)), hand));
		return b;
	}

	void Touch(float now, vector threat)
	{
		m_fLastJob_ms = now;
		if (threat != vector.Zero)
			m_vThreat = threat;
	}

	bool IsMoving()
	{
		return m_bMovePend || m_iState == DCO_EGunnerState.MOVE;
	}

	bool IsSuppressing(float now)
	{
		return now < m_fSuppUntil_ms;
	}

	void MoveTo(vector pos, vector threat, bool cover, ECharacterStance stance, float dur_s, string why, bool crit)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		Touch(now, threat);
		if ((IsMoving() || m_bRush) && why == m_sMoveWhy && vector.DistanceXZ(pos, m_vFinal) <= 3.0)
		{
			DCO_Perf.Count("spec_bt:move_keep");
			return;
		}
		m_vFinal = pos;
		m_eFinalStance = stance;
		m_bFinalCover = cover;
		m_sMoveWhy = why;
		m_bMoveCrit = crit;
		m_fSuppUntil_ms = 0;
		m_iLeg = 0;
		m_bRush = m_sRole == "ar" && vector.DistanceXZ(m_Utility.m_OwnerEntity.GetOrigin(), pos) > 14.0;
		if (m_bRush)
			StartLeg(now);
		else
		{
			m_vMovePos = pos;
			m_bMoveCover = cover;
			m_eMoveStance = stance;
			m_fMoveDur_s = Math.Clamp(dur_s, 2, 30);
			m_iMoveRetry = 0;
			m_iResume = 0;
			m_fResumeAt_ms = 0;
			m_fMoveUntil_ms = now + (m_fMoveDur_s + 8.0) * 1000.0;
			m_bMovePend = true;
		}
		if (crit)
			m_fCritUntil_ms = now + 8000.0;
		DCO_Perf.Count("spec_bt:move_order_" + why);
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("spec_bt_move_order unit=%1 role=%2 why=%3 to=%4 dist=%5 stance=%6 cover=%7 crit=%8", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), m_sRole, why, DCO_Log.Pos(pos), Math.Round(vector.DistanceXZ(m_Utility.m_OwnerEntity.GetOrigin(), pos)), typename.EnumToString(ECharacterStance, stance), DCO_Util.B01(cover), DCO_Util.B01(crit)));
	}

	protected void StartLeg(float now)
	{
		IEntity me = m_Utility.m_OwnerEntity;
		vector dir = m_vFinal - me.GetOrigin();
		dir[1] = 0;
		float remaining = dir.Length();
		if (remaining <= 4.0)
		{
			m_bRush = false;
			return;
		}
		dir = dir * (1.0 / remaining);
		float step = Math.Min(remaining, Math.RandomFloat(12.0, 18.0));
		bool last = remaining - step < 6.0;
		if (last)
		{
			step = remaining;
			m_vMovePos = m_vFinal;
			m_bMoveCover = m_bFinalCover;
			m_eMoveStance = m_eFinalStance;
		}
		else
		{
			m_vMovePos = DCO_Util.Surface(me.GetOrigin() + dir * step);
			m_bMoveCover = true;
			m_eMoveStance = ECharacterStance.PRONE;
		}
		m_iLeg = m_iLeg + 1;
		m_fMoveDur_s = Math.Clamp(step / 3.5 + 3.0, 2, 30);
		m_iMoveRetry = 0;
		m_iResume = 0;
		m_fResumeAt_ms = 0;
		m_fMoveUntil_ms = now + (m_fMoveDur_s + 8.0) * 1000.0;
		m_bMovePend = true;
		DCO_Perf.Count("spec_bt:rush_leg");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_bt_rush unit=%1 role=%2 why=%3 leg=%4 step=%5 left=%6 last=%7 to=%8", DCO_Log.IdUnit(me), m_sRole, m_sMoveWhy, m_iLeg, Math.Round(step), Math.Round(remaining), DCO_Util.B01(last), DCO_Log.Pos(m_vMovePos)));
	}

	void SetHold(bool hold, string why)
	{
		if (hold == m_bHold)
			return;
		m_bHold = hold;
		if (hold)
		{
			m_sHoldWhy = why;
			m_fLastJob_ms = GetGame().GetWorld().GetWorldTime();
		}
		DCO_Perf.Count("spec_bt:hold_" + DCO_Util.B01(hold));
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("spec_bt_hold unit=%1 role=%2 hold=%3 why=%4", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), m_sRole, DCO_Util.B01(hold), why));
	}

	void Watch(vector pos, float dur_s, string why)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		Touch(now, pos);
		bool fresh = now >= m_fWatchUntil_ms;
		m_fWatchUntil_ms = now + dur_s * 1000.0;
		m_sWatchWhy = why;
		if (!fresh)
			return;
		DCO_Perf.Count("spec_bt:watch_" + why);
		IEntity me = m_Utility.m_OwnerEntity;
		CharacterControllerComponent ctrl = m_Utility.GetCharacterController();
		if (ctrl && ctrl.GetStance() == ECharacterStance.STAND && !IsMoving())
		{
			ECharacterStance low = ECharacterStance.CROUCH;
			if (DCO_CQC.IsProneAllowed(me, vector.Distance(me.GetOrigin(), pos)))
				low = ECharacterStance.PRONE;
			DCO_Stance.Set(ctrl, low, "spec_bt_watch");
		}
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("spec_bt_watch unit=%1 role=%2 why=%3 threat=%4 dist=%5", DCO_Log.IdUnit(me), m_sRole, why, DCO_Log.Pos(pos), Math.Round(vector.DistanceXZ(me.GetOrigin(), pos))));
	}

	void Lob(vector pos, float dur_s, string why)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		Touch(now, pos);
		m_vLob = pos;
		m_fLobUntil_ms = now + dur_s * 1000.0;
		m_sLobWhy = why;
		m_iLobs = m_iLobs + 1;
		DCO_Perf.Count("spec_bt:lob_order");
	}

	void Suppress(SCR_AISuppressionVolumeBase vol, float rate, float dur_s, string why)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		Touch(now, vector.Zero);
		m_Vol = vol;
		m_fRate = rate;
		m_fSuppUntil_ms = now + dur_s * 1000.0;
		m_sSuppWhy = why;
		m_iBursts = m_iBursts + 1;
	}

	protected void SetState(int s, string why, float now)
	{
		if (s == m_iState)
			return;
		string from = "-";
		if (m_iState >= 0)
			from = typename.EnumToString(DCO_EGunnerState, m_iState);
		string to = typename.EnumToString(DCO_EGunnerState, s);
		DCO_Perf.Count("spec_bt:" + to);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("spec_bt_state unit=%1 role=%2 from=%3 to=%4 why=%5 held_ms=%6", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), m_sRole, from, to, why, Math.Round(now - m_fStateAt_ms)));
		m_iState = s;
		m_fStateAt_ms = now;
	}

	protected void SendMove(SCR_AICombatMoveState cms, float now)
	{
		IEntity me = m_Utility.m_OwnerEntity;
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
		rq.m_eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
		rq.m_vTargetPos = m_vThreat;
		rq.m_vMovePos = m_vMovePos;
		rq.m_bTryFindCover = m_bMoveCover;
		rq.m_bFailIfNoCover = false;
		rq.m_bUseCoverSearchDirectivity = false;
		rq.m_bCheckCoverVisibility = m_bMoveCover;
		rq.m_fCoverSearchDistMin = 0;
		rq.m_fCoverSearchDistMax = 8.0;
		rq.m_eStanceMoving = ECharacterStance.CROUCH;
		if (m_eMoveStance == ECharacterStance.PRONE && vector.DistanceXZ(me.GetOrigin(), m_vMovePos) <= 6.0)
			rq.m_eStanceMoving = ECharacterStance.PRONE;
		rq.m_eStanceEnd = m_eMoveStance;
		rq.m_eMovementType = EMovementType.RUN;
		if (m_bRush)
		{
			rq.m_eStanceMoving = ECharacterStance.STAND;
			rq.m_eMovementType = EMovementType.SPRINT;
		}
		rq.m_bAimAtTarget = false;
		rq.m_bAimAtTargetEnd = true;
		rq.m_fMoveDuration_s = m_fMoveDur_s;
		cms.ApplyNewRequest(rq);
		m_Rq = rq;
		m_bMovePend = false;
		m_fMoveSent_ms = now;
		if (m_iResume == 0 && m_iMoveRetry == 0)
		{
			m_vMoveFrom = me.GetOrigin();
			m_iMoves = m_iMoves + 1;
		}
		SetState(DCO_EGunnerState.MOVE, m_sMoveWhy, now);
	}

	protected string MoveResult(SCR_AICombatMoveState cms, float now)
	{
		if (!m_Rq)
			return "no_request";
		SCR_EAICombatMoveRequestState rs = m_Rq.m_eState;
		if (rs == SCR_EAICombatMoveRequestState.SUCCESS)
			return "arrived";
		if (rs == SCR_EAICombatMoveRequestState.FAIL)
		{
			if (m_Rq.m_eFailReason == SCR_EAICombatMoveRequestFailReason.COVER_NOT_FOUND && m_bMoveCover && m_iMoveRetry < 1)
			{
				m_iMoveRetry++;
				m_bMoveCover = false;
				m_bMovePend = true;
				DCO_Perf.Count("spec_bt:move_retry_no_cover");
				return "";
			}
			return "fail_" + typename.EnumToString(SCR_EAICombatMoveRequestFailReason, m_Rq.m_eFailReason);
		}
		bool mine = cms.GetRequest() == m_Rq;
		if (rs == SCR_EAICombatMoveRequestState.CANCELED || (!mine && rs != SCR_EAICombatMoveRequestState.EXECUTING && now - m_fMoveSent_ms > 3000.0))
		{
			float left = vector.DistanceXZ(m_Utility.m_OwnerEntity.GetOrigin(), m_vMovePos);
			if (m_iResume < 3 && left > 4.0 && now < m_fMoveUntil_ms - 2000.0)
			{
				m_iResume++;
				m_bMovePend = true;
				m_fResumeAt_ms = now + 1500.0;
				DCO_Perf.Count("spec_bt:move_resume");
				if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
					DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("spec_bt_move_resume unit=%1 why=%2 try=%3 left=%4 mine=%5", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), m_sMoveWhy, m_iResume, Math.Round(left), DCO_Util.B01(mine)));
				return "";
			}
			return "canceled";
		}
		if (now > m_fMoveUntil_ms)
			return "timeout";
		return "";
	}

	protected void Settle(float now, string result)
	{
		IEntity me = m_Utility.m_OwnerEntity;
		float lo = 600.0;
		float hi = 1200.0;
		if (now < m_fCritUntil_ms)
		{
			lo = 400.0;
			hi = 800.0;
		}
		float settle = Math.RandomFloat(lo, hi);
		m_fSettleUntil_ms = now + settle;
		bool bipod;
		CharacterControllerComponent ctrl = m_Utility.GetCharacterController();
		if (ctrl && ctrl.GetStance() == ECharacterStance.PRONE && ctrl.GetWeaponManagerComponent())
		{
			BaseWeaponComponent w = ctrl.GetWeaponManagerComponent().GetCurrentWeapon();
			if (w && w.HasBipod())
			{
				w.SetBipod(true);
				bipod = true;
			}
		}
		if (m_Utility.m_CombatMoveState)
			m_Utility.m_CombatMoveState.EnableAiming(true);
		DCO_Perf.Count("spec_bt:deploy");
		if (bipod)
			DCO_Perf.Count("spec_bt:deploy_bipod");
		string stance = "-";
		if (ctrl)
			stance = typename.EnumToString(ECharacterStance, ctrl.GetStance());
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_bt_deploy unit=%1 role=%2 why=%3 result=%4 moved=%5 left=%6 stance=%7 bipod=%8 settle_ms=%9", DCO_Log.IdUnit(me), m_sRole, m_sMoveWhy, result, Math.Round(vector.DistanceXZ(m_vMoveFrom, me.GetOrigin())), Math.Round(vector.DistanceXZ(me.GetOrigin(), m_vMovePos)), stance, DCO_Util.B01(bipod), Math.Round(settle)));
		SetState(DCO_EGunnerState.SETTLE, result, now);
		if (m_bRush)
		{
			if (vector.DistanceXZ(me.GetOrigin(), m_vFinal) > 4.0)
			{
				float pause = Math.RandomFloat(2000.0, 4000.0);
				m_fLegPauseUntil_ms = m_fSettleUntil_ms + pause;
				DCO_Perf.Count("spec_bt:rush_down");
				if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
					DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("spec_bt_rush_down unit=%1 leg=%2 left=%3 pause_ms=%4", DCO_Log.IdUnit(me), m_iLeg, Math.Round(vector.DistanceXZ(me.GetOrigin(), m_vFinal)), Math.Round(pause)));
			}
			else
				m_bRush = false;
		}
	}

	protected bool WeaponReady()
	{
		BaseWeaponComponent sel;
		int muzzle;
		m_Utility.m_CombatComponent.GetSelectedWeapon(sel, muzzle);
		CharacterControllerComponent ctrl = m_Utility.GetCharacterController();
		if (!sel || !ctrl || !ctrl.GetWeaponManagerComponent())
			return false;
		return ctrl.GetWeaponManagerComponent().GetCurrentWeapon() == sel;
	}

	protected void CheckReload(float now)
	{
		CharacterControllerComponent ctrl = m_Utility.GetCharacterController();
		if (!ctrl || !ctrl.GetWeaponManagerComponent())
			return;
		BaseWeaponComponent w = ctrl.GetWeaponManagerComponent().GetCurrentWeapon();
		if (!w)
			return;
		BaseMagazineComponent mag = w.GetCurrentMagazine();
		bool empty = !mag || mag.GetAmmoCount() <= 0;
		if (empty == m_bReloadLow)
			return;
		m_bReloadLow = empty;
		if (!empty)
			return;
		IEntity me = m_Utility.m_OwnerEntity;
		bool lowered;
		if (ctrl.GetStance() != ECharacterStance.PRONE && DCO_CQC.IsProneAllowed(me, vector.Distance(me.GetOrigin(), m_vThreat)))
			lowered = DCO_Stance.Set(ctrl, ECharacterStance.PRONE, "spec_bt_reload");
		DCO_Perf.Count("spec_bt:reload_cover");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_bt_reload unit=%1 role=%2 state=%3 lowered=%4 stance=%5", DCO_Log.IdUnit(me), m_sRole, typename.EnumToString(DCO_EGunnerState, m_iState), DCO_Util.B01(lowered), typename.EnumToString(ECharacterStance, ctrl.GetStance())));
	}

	ENodeResult Update(out int fire, out BaseTarget tgt, out SCR_AISuppressionVolumeBase vol, out float rate, out vector aim, out bool ready)
	{
		fire = 0;
		aim = m_vThreat + Vector(0, 1.0, 0);
		IEntity me = m_Utility.m_OwnerEntity;
		if (m_bDone || !me)
			return ENodeResult.SUCCESS;
		float now = GetGame().GetWorld().GetWorldTime();
		SCR_AICombatMoveState cms = m_Utility.m_CombatMoveState;
		if (!cms)
			return ENodeResult.FAIL;
		if (m_bMovePend && now >= m_fResumeAt_ms)
			SendMove(cms, now);
		if (m_iState == DCO_EGunnerState.MOVE)
		{
			string res;
			if (m_bMovePend)
			{
				if (now <= m_fMoveUntil_ms)
					return ENodeResult.RUNNING;
				m_bMovePend = false;
				res = "timeout_blocked";
			}
			else
				res = MoveResult(cms, now);
			if (res == "")
				return ENodeResult.RUNNING;
			Settle(now, res);
		}
		ready = WeaponReady();
		if (m_iState == DCO_EGunnerState.SETTLE)
		{
			if (now < m_fSettleUntil_ms)
			{
				fire = 3;
				return ENodeResult.RUNNING;
			}
			SetState(DCO_EGunnerState.WATCH, "settled", now);
		}
		if (m_bRush && !IsMoving() && now >= m_fLegPauseUntil_ms)
		{
			StartLeg(now);
			if (m_bMovePend)
			{
				fire = 3;
				return ENodeResult.RUNNING;
			}
		}
		if (now < m_fLobUntil_ms)
		{
			SetState(DCO_EGunnerState.LOB, m_sLobWhy, now);
			fire = 4;
			aim = m_vLob;
			return ENodeResult.RUNNING;
		}
		CheckReload(now);
		BaseTarget t = m_Utility.m_CombatComponent.GetCurrentTarget();
		bool recentOk = m_bFireRecent || m_bHold;
		bool suppPrio = m_Vol && now < m_fSuppUntil_ms && m_sSuppWhy.StartsWith("designated") && t && t.GetTimeSinceSeen() > 2.0;
		if (t && t.GetTargetEntity() && t.GetTimeSinceSeen() < 2.0)
		{
			SetState(DCO_EGunnerState.FIRE, "target_seen", now);
			fire = 1;
			tgt = t;
			aim = t.GetLastSeenPosition() + Vector(0, 1.0, 0);
			return ENodeResult.RUNNING;
		}
		if (recentOk && !suppPrio && t && t.GetTargetEntity() && t.GetTimeSinceSeen() < 6.0)
		{
			if (m_Vol && now < m_fSuppUntil_ms)
				DCO_Perf.Count("spec_bt:suppress_skip_recent");
			SetState(DCO_EGunnerState.FIRE, "target_recent", now);
			fire = 1;
			tgt = t;
			aim = t.GetLastSeenPosition() + Vector(0, 1.0, 0);
			return ENodeResult.RUNNING;
		}
		if (m_Vol && now < m_fSuppUntil_ms)
		{
			SetState(DCO_EGunnerState.SUPPRESS, m_sSuppWhy, now);
			fire = 2;
			vol = m_Vol;
			rate = m_fRate;
			aim = m_Vol.GetCenterPosition();
			return ENodeResult.RUNNING;
		}
		if (recentOk && t && t.GetTargetEntity() && t.GetTimeSinceSeen() < 10.0)
		{
			SetState(DCO_EGunnerState.FIRE, "target_recent", now);
			fire = 1;
			tgt = t;
			aim = t.GetLastSeenPosition() + Vector(0, 1.0, 0);
			return ENodeResult.RUNNING;
		}
		SetState(DCO_EGunnerState.WATCH, "idle", now);
		fire = 3;
		return ENodeResult.RUNNING;
	}

	protected bool HasJob(float now)
	{
		if (m_bMovePend || m_bRush || m_iState == DCO_EGunnerState.MOVE || m_iState == DCO_EGunnerState.SETTLE)
			return true;
		if (m_Vol && now < m_fSuppUntil_ms)
			return true;
		if (now < m_fLobUntil_ms || m_bHold || now < m_fWatchUntil_ms)
			return true;
		if (!m_bFireRecent)
			return false;
		BaseTarget t = m_Utility.m_CombatComponent.GetCurrentTarget();
		return t && t.GetTargetEntity() && t.GetTimeSinceSeen() < 10.0;
	}

	protected bool BleedYield(IEntity me)
	{
		SCR_CharacterDamageManagerComponent dm = SCR_CharacterDamageManagerComponent.Cast(me.FindComponent(SCR_CharacterDamageManagerComponent));
		if (dm && dm.GetHealthScaled() < 0.5)
			return true;
		float now = GetGame().GetWorld().GetWorldTime();
		BaseTarget t = m_Utility.m_CombatComponent.GetCurrentTarget();
		bool busy = (t && t.GetTargetEntity() && t.GetTimeSinceSeen() < 10.0) || (m_Vol && now < m_fSuppUntil_ms) || now < m_fLobUntil_ms;
		if (busy)
			DCO_Perf.Count("spec_bt:bleed_fight_on");
		return !busy;
	}

	protected string YieldWhy()
	{
		IEntity me = m_Utility.m_OwnerEntity;
		if (m_Utility.m_AIInfo && m_Utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
			return "vehicle";
		if (DCO_AimedYield.TargetVisible(m_Utility, DCO_Stress.T(m_Utility, "rt.close_m", 30.0)))
			return "close_target";
		if (DCO_Util.IsBleeding(me) && BleedYield(me))
			return "bleeding";
		DCO_AIMoraleSystem ms = m_Utility.GetMoraleSystem();
		if (ms && ms.GetState() >= moraleState.PANIC)
			return "panic";
		if (m_Utility.m_CombatComponent.DCO_IsPinned())
			return "pinned";
		int owner = DCO_CtrlLock.OwnerOf(m_Utility.GetAIAgent());
		if (owner != DCO_ECtrlOwner.NONE && owner != DCO_ECtrlOwner.IDLE_DEFEND)
			return "owner_" + DCO_CtrlLock.OwnerName(owner);
		if (m_Utility.m_CombatComponent.GetCombatMode() == EAIGroupCombatMode.HOLD_FIRE)
			return "hold_fire";
		return "";
	}

	protected void Finish(string why)
	{
		if (m_bDone)
			return;
		m_bDone = true;
		IEntity me = m_Utility.m_OwnerEntity;
		float now = GetGame().GetWorld().GetWorldTime();
		DCO_Perf.Count("spec_bt:end_" + why);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("spec_bt_end unit=%1 role=%2 why=%3 dur_s=%4 moves=%5 suppress_orders=%6 lobs=%7", DCO_Log.IdUnit(me), m_sRole, why, Math.Round((now - m_fStart_ms) / 1000.0), m_iMoves, m_iBursts, m_iLobs));
		Complete();
	}

	override float CustomEvaluate()
	{
		EAIActionState as = GetActionState();
		if (m_bDone || as == EAIActionState.COMPLETED || as == EAIActionState.FAILED)
			return 0;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fLastJob_ms > 45000.0)
		{
			Finish("no_fight");
			return 0;
		}
		string y = YieldWhy();
		if (y == "" && !HasJob(now))
			y = "idle";
		if (y != m_sYield)
		{
			IEntity me = m_Utility.m_OwnerEntity;
			string ev = "spec_bt_yield";
			string why = y;
			if (y == "")
			{
				ev = "spec_bt_resume";
				why = m_sYield;
				DCO_Perf.Count("spec_bt:resume");
			}
			else
				DCO_Perf.Count("spec_bt:yield_" + y);
			DCO_ELogLevel lvl = DCO_ELogLevel.INFO;
			if (why == "idle")
				lvl = DCO_ELogLevel.DEBUG;
			if (DCO_Log.On(DCO_ELogChannel.COMBAT, lvl))
				DCO_Log.Write(DCO_ELogChannel.COMBAT, lvl, DCO_Log.IdUnit(me), string.Format("%1 unit=%2 role=%3 why=%4 state=%5", ev, DCO_Log.IdUnit(me), m_sRole, why, m_iState));
			m_sYield = y;
		}
		if (y != "")
			return 0;
		if (now < m_fCritUntil_ms)
			return 105;
		return 95;
	}

	override void OnActionSelected()
	{
		super.OnActionSelected();
		if (m_Utility && m_Utility.m_PerceptionComponent)
			m_Utility.m_PerceptionComponent.SetFriendlyFireCheck(true);
		if (m_iState == DCO_EGunnerState.MOVE && !m_bMovePend)
			m_bMovePend = true;
	}

	override void OnActionDeselected()
	{
		super.OnActionDeselected();
		if (DCO_Util.Teardown("gunner_deselect"))
			return;
		if (m_Utility && m_Utility.m_PerceptionComponent)
			m_Utility.m_PerceptionComponent.SetFriendlyFireCheck(false);
	}

	override int GetCause()
	{
		return SCR_EAIBehaviorCause.COMBAT;
	}

	override string GetActionDebugInfo()
	{
		return string.Format("%1 DCO gunner %2 state=%3", this, m_sRole, m_iState);
	}
}

class DCO_AISpecGunnerUpdate : SCR_AIActionTask
{
	protected ref TStringArray m_aDCOOut = {"Fire", "Target", "Volume", "FireRate", "AimPos", "WeaponReady"};

	override TStringArray GetVariablesOut() { return m_aDCOOut; }

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		DCO_GunnerBehavior b = DCO_GunnerBehavior.Cast(GetExecutedAction());
		if (!b)
			return ENodeResult.FAIL;
		int fire;
		BaseTarget tgt;
		SCR_AISuppressionVolumeBase vol;
		float rate;
		vector aim;
		bool ready;
		ENodeResult r = b.Update(fire, tgt, vol, rate, aim, ready);
		SetVariableOut("Fire", fire);
		if (tgt)
			SetVariableOut("Target", tgt);
		else
			ClearVariable("Target");
		if (vol)
			SetVariableOut("Volume", vol);
		else
			ClearVariable("Volume");
		SetVariableOut("FireRate", rate);
		SetVariableOut("AimPos", aim);
		SetVariableOut("WeaponReady", ready);
		return r;
	}

	static override bool VisibleInPalette() { return true; }
	static override string GetOnHoverDescription() { return "DCO: specialist brain step (MG, AR, AT, marksman, GL). Sends move requests, settles on arrival (bipod), and outputs Fire 0 none, 1 target via vanilla fire trees, 2 suppress volume, 3 aim only, 4 UGL lob at AimPos."; }
}
