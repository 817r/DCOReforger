modded class SCR_AICombatComponent
{
	static const int			 TARGET_ENDANGERED_TIMEOUT_S = 12;
	protected const float FRAG_GRENADE_MAX_THREAT = 2.0;
	static const float			 ENDANGERING_TARGET_SCORE_MULTIPLIER = 1.1;
	          static const float TARGET_MAX_LAST_SEEN = 11.0;
	protected static const float TARGET_MIN_INDIRECT_TRACE_FRACTION_MIN = 0.5;
	protected static const float TARGET_MAX_DISTANCE_VEHICLE = 800.0;
	protected static const float TARGET_MAX_DISTANCE_DISARMED = 0.2;
	protected static const float TARGET_MAX_TIME_SINCE_ENDANGERED = 5.0;
	protected static const float TARGET_SCORE_RETREAT = 75.0;
	static const float TARGET_SCORE_HIGH_PRIORITY_ATTACK = 98.5;




	protected static bool s_bDCOPercLogged;
	protected float m_fDCOMissionAt_ms = -1;
	protected bool m_bDCOMission;
	protected float m_fDCORetFireAt_ms = -100000;
	float m_fDCORetFireUntil_ms = -1;

	void DCO_ReturnFireUnseen(vector shooterPos)
	{
		if (!m_Utility || !m_Utility.m_OwnerEntity)
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		DCO_Perf.Count("tm:retfire_call");
		if (now - m_fDCORetFireAt_ms < DCO_Stress.T(m_Utility, "tm.retfire_cooldown_s", 12.0) * 1000.0)
		{
			DCO_Perf.Count("tm:retfire_skip_cd");
			return;
		}
		if (DCO_IsPinned() || (m_Utility.m_AIInfo && m_Utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE)))
		{
			DCO_Perf.Count("tm:retfire_skip_pinned_veh");
			return;
		}
		if (GetCombatMode() == EAIGroupCombatMode.HOLD_FIRE)
		{
			DCO_Perf.Count("tm:retfire_skip_hold");
			return;
		}
		vector me = m_Utility.m_OwnerEntity.GetOrigin();
		float dist = vector.Distance(shooterPos, me);
		if (dist < 15.0 || dist > DCO_Stress.T(m_Utility, "tm.retfire_max_m", 400.0))
		{
			DCO_Perf.Count("tm:retfire_skip_dist");
			return;
		}
		m_fDCORetFireAt_ms = now;
		float rfChance = DCO_Stress.T(m_Utility, "tm.retfire_chance", 0.0);
		if (DCO_Stress.T(m_Utility, "agg.retfire", 1.0) > 0)
			rfChance = Math.Max(rfChance, Math.Min(0.3 * Math.Clamp(DCO_Stress.Aggr(m_Utility) / 2.5, 0.5, 4.0), 0.7));
		if (Math.RandomFloat01() >= rfChance)
		{
			DCO_Perf.Count("tm:retfire_skip_roll");
			return;
		}
		DCO_Perf.Count("agg:retfire_roll_ok");
		BaseWeaponComponent rw;
		int rm;
		GetSelectedWeapon(rw, rm);
		if (DCO_AmmoUtility.ShouldAvoidSuppressiveFire(m_Utility, rw))
		{
			DCO_Perf.Count("tm:return_fire_unseen_ammo");
			return;
		}
		vector dir = shooterPos - me;
		dir[1] = 0;
		dir.Normalize();
		float errDeg = Math.RandomFloat(-1.0, 1.0) * (5.0 + dist / 40.0);
		float a = errDeg * Math.DEG2RAD;
		vector bent = Vector(dir[0] * Math.Cos(a) - dir[2] * Math.Sin(a), 0, dir[0] * Math.Sin(a) + dir[2] * Math.Cos(a));
		vector aim = me + bent * (dist * Math.RandomFloat(0.9, 1.1));
		aim[1] = shooterPos[1];
		float radius = Math.Map(Math.Min(dist, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE), 0, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE, 2.0, 10.0);
		vector bbMin, bbMax;
		SCR_AISuppressionVolumeBase.CreateSuppressionBox(aim, radius, 3.0, bbMin, bbMax);
		SCR_AISuppressionObjectVolumeBox volume = new SCR_AISuppressionObjectVolumeBox(bbMin, bbMax);
		float dur = Math.RandomFloat(3.0, 5.0);
		SCR_AISuppressBehavior supp = new SCR_AISuppressBehavior(m_Utility, null, volume, dur, 1.2);
		m_Utility.AddAction(supp);
		m_fDCORetFireUntil_ms = now + (dur + 2.0) * 1000.0;
		DCO_Perf.Count("tm:return_fire_unseen");
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("return_fire_unseen unit=%1 dist=%2 err_deg=%3 radius=%4 dur_s=%5",
				DCO_Log.IdUnit(m_Utility.m_OwnerEntity), Math.Round(dist), Math.Round(errDeg), Math.Round(radius), dur.ToString(-1, 1)));
	}

	protected bool DCO_MissionAlert()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (m_fDCOMissionAt_ms >= 0 && now - m_fDCOMissionAt_ms < 2000.0)
			return m_bDCOMission;
		m_fDCOMissionAt_ms = now;
		bool was = m_bDCOMission;
		m_bDCOMission = false;
		AIAgent ag = m_Utility.GetAIAgent();
		if (ag && ag.GetParentGroup())
		{
			DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(ag.GetParentGroup().FindComponent(DCO_GroupUtilityComponent));
			if (g)
			{
				DCO_EGroupTask t = g.GetTask();
				if (t == DCO_EGroupTask.ATTACK || t == DCO_EGroupTask.FLANK || t == DCO_EGroupTask.SUPPORT_BY_FIRE || t == DCO_EGroupTask.RECON)
					m_bDCOMission = true;
				else if ((t == DCO_EGroupTask.DEFEND || t == DCO_EGroupTask.GARRISON) && g.HasState(DCO_EGroupState.IN_CONTACT))
					m_bDCOMission = true;
			}
		}
		if (m_bDCOMission && !was)
			DCO_Perf.Count("perc:mission_floor_on");
		return m_bDCOMission;
	}

	float CURRENT_AIM_IMPROVEMENT;

	bool ChangeTarget = false;

	protected IEntity ownerEntity;

	float m_fTimeElapsed = 0.0;
	protected float m_fDurationN = 1500.0;
	protected float m_fDCOAimHold_ms;
	protected vector m_vDCOAimDir;
	protected bool m_bDCOAimSwitchDone = true;
	protected bool m_bDCOSettleLog;
	protected bool m_bDCOFirstShotPending;
	protected bool m_bDCOFirstHitPending;
	protected float m_fDCOFirstShotAt_ms;
	protected vector m_vDCOAimFrom;
	protected vector m_vDCOAimRef;
	protected float m_fDCOAimStart_ms = -1;
	protected static int s_iDCOFirstShotSample;

	protected float slicedTime;

	protected DCO_GroupTactics m_eAICurrentTactics;

	float m_fDCORpgUnseenAt_ms = -1;
	protected float m_fDCOEngageInfantry = -1;
	protected float m_fDCOEngageVehicle = -1;
	protected bool m_bDCOCloseCombat;
	protected AIGroup m_DCOEngageGroup;
	protected DCO_GroupConfigComponent m_DCOEngageConfig;

	protected void DCO_ResolveEngagement(out float infantry, out float vehicle)
	{
		infantry = TARGET_MAX_DISTANCE_INFANTRY;
		vehicle = 800.0;

		SCR_ChimeraAIAgent agent = GetAiAgent();
		if (!agent)
			return;

		AIGroup group = agent.GetParentGroup();
		if (group != m_DCOEngageGroup)
		{
			m_DCOEngageGroup = group;
			m_DCOEngageConfig = null;
			if (group)
				m_DCOEngageConfig = DCO_GroupConfigComponent.Cast(group.FindComponent(DCO_GroupConfigComponent));
		}

		if (!m_DCOEngageConfig)
			return;

		infantry = m_DCOEngageConfig.GetEngagementDistanceInfantry();
		vehicle = m_DCOEngageConfig.GetEngagementDistanceVehicle();
	}

	override void SetTargetSelectionProperties(bool closeCombat)
	{
		m_bDCOCloseCombat = closeCombat;
		DCO_ResolveEngagement(m_fDCOEngageInfantry, m_fDCOEngageVehicle);

		if (closeCombat)
		{
			m_WeaponTargetSelector.SetSelectionProperties(TARGET_MAX_LAST_SEEN_DIRECT_ATTACK_CLOSE, TARGET_MAX_LAST_SEEN_INDIRECT_ATTACK_CLOSE, TARGET_MAX_LAST_SEEN_INDIRECT_ATTACK_CLOSE,
				0.5, m_fDCOEngageInfantry, m_fDCOEngageVehicle, 5.0, 0.2);
			return;
		}

		float indirect = TARGET_MAX_LAST_SEEN_INDIRECT_ATTACK;
		float maxLast = 11.0;
		if (DCO_GlobalAIComponent.BehaviorOnFor(GetOwner(), DCO_EBehavior.SUPPRESS_LAST_KNOWN))
		{
			indirect = Math.Max(indirect, 15.5);
			maxLast = 16.0;
		}
		m_WeaponTargetSelector.SetSelectionProperties(TARGET_MAX_LAST_SEEN_DIRECT_ATTACK, indirect, maxLast,
			0.5, m_fDCOEngageInfantry, m_fDCOEngageVehicle, 5.0, 0.2);
	}

	void DecreaseAim()
	{
		m_fTimeElapsed = Math.Max(m_fTimeElapsed - slicedTime * 1.2 * m_fDurationN / 15000.0, 0);
		m_fDCOAimHold_ms = 0;
	}

	void ChangeTargetCompensation()
	{
		if (!m_bDCOAimSwitchDone)
		{
			m_bDCOAimSwitchDone = true;
			float before = m_fTimeElapsed;
			string reason = "target_change";
			vector dir = DCO_AimDir();
			if (dir != vector.Zero && m_vDCOAimDir != vector.Zero && vector.Dot(dir, m_vDCOAimDir) > Math.Cos(5.0 * Math.DEG2RAD))
			{
				m_fTimeElapsed *= 0.5;
				reason = "target_near";
			}
			else
				m_fTimeElapsed = 0;
			m_fDCOAimHold_ms = 0;
			m_fDCOAimStart_ms = GetGame().GetWorld().GetWorldTime();
			m_bDCOFirstShotPending = true;
			m_bDCOFirstHitPending = true;
			m_fDCOFirstShotAt_ms = -1;
			if (ownerEntity)
				m_vDCOAimFrom = ownerEntity.GetOrigin();
			m_vDCOAimRef = dir;
			m_bDCOSettleLog = true;
			DCO_LogAimReset(reason, before);
		}
		UpdateAccuracy(slicedTime);
	}

	protected float DCO_CloseResetMul()
	{
		if (!ownerEntity || !m_SelectedTarget || DCO_Stress.T(m_Utility, "rt.fast", 1.0) <= 0)
			return 1.0;
		if (vector.Distance(ownerEntity.GetOrigin(), DCO_AimTargetPos()) >= DCO_Stress.T(m_Utility, "rt.close_m", 30.0))
			return 1.0;
		DCO_Perf.Count("rt:close_reset_damped");
		return DCO_Stress.T(m_Utility, "rt.close_reset_mul", 0.5);
	}

	void DangerSuppressedDecreaseAIM(float dec)
	{
		dec = dec * DCO_CloseResetMul();
		float before = m_fTimeElapsed;
		m_fTimeElapsed = Math.Max(m_fTimeElapsed - slicedTime * 1.5 * dec * m_fDurationN / 15000.0, 0);
		DCO_LogAimReset("suppressed", before);
	}

	void MoraleDropAIM(float val)
	{
		val = val * DCO_CloseResetMul();
		float before = m_fTimeElapsed;
		m_fTimeElapsed = Math.Max(m_fTimeElapsed - slicedTime * 1.2 * val * m_fDurationN / 15000.0, 0);
		DCO_LogAimReset("morale", before);
	}

	protected void DCO_LogAimReset(string reason, float before)
	{
		if (m_fDurationN <= 0 || (before - m_fTimeElapsed) < m_fDurationN * 0.05 || !DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
			return;
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(ownerEntity), string.Format("aim_reset reason=%1 progress_before=%2 progress_after=%3",
			reason, Math.Clamp(before / m_fDurationN, 0, 1).ToString(-1, 2), Math.Clamp(m_fTimeElapsed / m_fDurationN, 0, 1).ToString(-1, 2)));
	}

	protected vector DCO_AimDir()
	{
		if (!ownerEntity || !m_SelectedTarget)
			return vector.Zero;
		vector to = DCO_AimTargetPos() - ownerEntity.GetOrigin();
		if (to.LengthSq() < 0.01)
			return vector.Zero;
		return to.Normalized();
	}

	protected vector DCO_AimTargetPos()
	{
		IEntity tgt = m_SelectedTarget.GetTargetEntity();
		if (tgt)
			return tgt.GetOrigin();
		return m_SelectedTarget.GetLastSeenPosition();
	}

	float DCO_GetAimHold() { return m_fDCOAimHold_ms; }

	protected float m_fDCONoEngageSince_ms = -1;
	protected float m_fDCOEngageBlockNext_ms;

	protected void DCO_CheckEngageBlock()
	{
		if (!m_Utility || !ownerEntity || !DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
			return;
		if (m_AIInfo && m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE) && !m_AIInfo.HasUnitState(EUnitState.IN_TURRET))
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		AIActionBase act = m_Utility.GetCurrentAction();
		IEntity tgt = m_SelectedTarget.GetTargetEntity();
		bool engaging = SCR_AIAttackBehavior.Cast(act) || SCR_AISuppressBehavior.Cast(act) || SCR_AIThrowGrenadeToBehavior.Cast(act);
		if (engaging || !tgt || !DCO_Util.IsAlive(tgt) || m_SelectedTarget.GetTimeSinceSeen() > 2.0 || !m_SelectedWeaponComp
			|| vector.Distance(ownerEntity.GetOrigin(), tgt.GetOrigin()) > DCO_LongRange.EffectiveRange(m_SelectedWeaponComp.GetWeaponType()))
		{
			m_fDCONoEngageSince_ms = -1;
			return;
		}
		if (m_fDCONoEngageSince_ms < 0)
			m_fDCONoEngageSince_ms = now;
		if (now - m_fDCONoEngageSince_ms < 3000.0 || now < m_fDCOEngageBlockNext_ms)
			return;
		m_fDCOEngageBlockNext_ms = now + 10000.0;

		string reason = "outprioritized";
		string roeReason = "-";
		EAIGroupCombatMode mode = GetCombatMode();
		if (mode == EAIGroupCombatMode.HOLD_FIRE)
			reason = "hold_fire_roe";
		else if (m_AIInfo && m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE) && !m_AIInfo.HasUnitState(EUnitState.IN_TURRET))
			reason = "in_vehicle_passenger";
		SCR_AIGroup grp;
		SCR_ChimeraAIAgent agent = GetAiAgent();
		if (agent)
			grp = SCR_AIGroup.Cast(agent.GetParentGroup());
		if (grp)
		{
			SCR_AIGroupUtilityComponent gu = SCR_AIGroupUtilityComponent.Cast(grp.FindComponent(SCR_AIGroupUtilityComponent));
			if (gu)
				roeReason = gu.DCO_GetROEReason();
		}
		ChimeraCharacter tc = ChimeraCharacter.Cast(tgt);
		bool tgtInVeh = tc && tc.IsInVehicle();
		string actName = "-";
		if (act)
			actName = act.ClassName();
		DCO_Perf.Count("engage_block:" + reason);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(ownerEntity), string.Format("target_engage_block tgt=%1 tgt_dist=%2 tgt_type=%3 tgt_in_vehicle=%4 action=%5 cmode=%6 roe_reason=%7 reason=%8 blocked_s=%9",
			DCO_Log.IdUnit(tgt), Math.Round(vector.Distance(ownerEntity.GetOrigin(), tgt.GetOrigin())), typename.EnumToString(EAIUnitType, m_SelectedTarget.GetUnitType()), tgtInVeh,
			actName, typename.EnumToString(EAIGroupCombatMode, mode), roeReason, reason, Math.Round((now - m_fDCONoEngageSince_ms) / 1000.0)));
	}

	float m_fDCOAimSigma;
	int m_iDCOAimPosture;
	bool m_bDCOAimSupported;
	float m_fDCOAimStress;
	float m_fDCOAimRamp;
	float m_fDCOAimLight;
	float m_fDCOAimOptic;
	bool m_bDCOAimMoving;

	void DCO_SetAimDebug(float sigma, int posture, bool supported, float stress, float ramp, float light, float optic, bool moving)
	{
		m_fDCOAimSigma = sigma;
		m_iDCOAimPosture = posture;
		m_bDCOAimSupported = supported;
		m_fDCOAimStress = stress;
		m_fDCOAimRamp = ramp;
		m_fDCOAimLight = light;
		m_fDCOAimOptic = optic;
		m_bDCOAimMoving = moving;
	}

	float DCO_RampValue()
	{
		if (m_fDurationN <= 0)
			return 1.0;
		float p = Math.Clamp(m_fTimeElapsed / m_fDurationN, 0, 1);
		return Math.Lerp(2.5, 1.0, p * p * (3.0 - 2.0 * p));
	}

	protected float m_fDCOLean;
	protected float m_fDCOLeanNextEval_ms;
	protected float m_fDCOLeanHoldUntil_ms;

	protected void DCO_UpdateLean()
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(ownerEntity);
		if (!ch || !m_CharacterController)
			return;

		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fDCOLeanNextEval_ms)
			return;
		m_bDCOLeanHot = false;
		int perfStart = DCO_Perf.Begin();
		DCO_EvalLean(ch, now);
		DCO_Perf.End("lean", perfStart);
		if (m_bDCOLeanHot || m_fDCOLean != 0)
			m_fDCOLeanNextEval_ms = now + 500.0;
		else
		{
			m_fDCOLeanNextEval_ms = now + 1500.0;
			DCO_Perf.Count("lean:slow_tick");
		}
	}

	protected bool m_bDCOLeanHot;
	protected vector m_vDCOLeanSelf;
	protected vector m_vDCOLeanTgt;
	protected float m_fDCOLeanZeroUntil_ms;
	protected vector m_vDCOPeekPos;
	protected float m_fDCOPeekUntil_ms;
	protected int m_iDCOPeekReason;
	protected float m_fDCOLeanBlock_ms;
	protected float m_fDCOPreferSide;
	protected float m_fDCOPeekLostNext_ms;
	protected float m_fDCOPeekLostEnd_ms;
	protected float m_fDCOSectorNext_ms;
	protected float m_fDCOPeekShootNext_ms;
	protected float m_fDCOIndoorCheck_ms;
	protected bool m_bDCOIndoorCtx;

	void DCO_NotePeek(vector pos, int reason, float duration_ms)
	{
		if (!DCO_GlobalAIComponent.BehaviorOnFor(GetOwner(), DCO_EBehavior.LEAN_PEEK))
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (m_iDCOPeekReason != 0 && now < m_fDCOPeekUntil_ms + 1500.0)
			return;
		m_vDCOPeekPos = pos;
		m_iDCOPeekReason = reason;
		m_fDCOPeekUntil_ms = now + duration_ms;
	}

	protected float m_fDCOCloseUnseen_ms = -1;

	void DCO_NoteUnseenFire(IEntity shooter, vector shotPos)
	{
		if (m_SelectedTarget && m_SelectedTarget.GetTargetEntity() == shooter && m_SelectedTarget.GetTimeSinceSeen() < 2.0)
			return;
		DCO_NotePeek(shotPos + Vector(0, 1.2, 0), 1, Math.RandomFloat(1000, 3000));
		if (ownerEntity && vector.DistanceXZ(ownerEntity.GetOrigin(), shotPos) < 40.0)
		{
			m_fDCOCloseUnseen_ms = GetGame().GetWorld().GetWorldTime();
			DCO_Perf.Count("close_unseen_fire");
		}
	}

	bool DCO_CloseUnseenFire(float window_ms)
	{
		return m_fDCOCloseUnseen_ms >= 0 && GetGame().GetWorld().GetWorldTime() - m_fDCOCloseUnseen_ms < window_ms;
	}

	bool DCO_TryPeekShoot()
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(ownerEntity);
		if (!ch || !m_CharacterController || !DCO_GlobalAIComponent.BehaviorOnFor(GetOwner(), DCO_EBehavior.LEAN_PEEK))
			return false;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fDCOPeekShootNext_ms)
			return true;
		if (now < m_fDCOLeanBlock_ms || !m_SelectedTarget || !m_SelectedTarget.GetTargetEntity() || m_SelectedTarget.GetTimeSinceSeen() > 30.0
			|| ch.IsInVehicle() || m_CharacterController.GetStance() == ECharacterStance.PRONE)
			return false;
		vector targetPos = m_SelectedTarget.GetLastSeenPosition() + vector.Up * DCO_TargetAimHeight(m_SelectedTarget.GetTargetEntity());
		float side = DCO_CQC.ResolveLean(ch, m_SelectedTarget.GetTargetEntity(), targetPos, 0, m_fDCOPreferSide);
		if (side == 0)
			return false;
		m_fDCOPeekShootNext_ms = now + 5000.0;
		DCO_ApplyLean(side, 2500.0, now, 2);
		return true;
	}

	protected bool DCO_InCQBContext(ChimeraCharacter ch, float now)
	{
		if (now < m_fDCOIndoorCheck_ms)
			return m_bDCOIndoorCtx;
		m_fDCOIndoorCheck_ms = now + 2000.0;
		m_bDCOIndoorCtx = false;
		if (m_Utility && SCR_AIMoveAndInvestigateBehavior.Cast(m_Utility.GetCurrentAction()))
			m_bDCOIndoorCtx = true;
		else
			m_bDCOIndoorCtx = SCR_CoverManagerComponent.IsEntityInsideBuilding(ch);
		return m_bDCOIndoorCtx;
	}

	protected void DCO_ApplyLean(float wanted, float hold, float now, int reason)
	{
		if (wanted != 0 && m_fDCOLean == 0 && reason != 0)
		{
			switch (reason)
			{
				case 1: DCO_Perf.Count("lean:peek_sound"); break;
				case 2: DCO_Perf.Count("lean:peek_shoot"); break;
				case 3: DCO_Perf.Count("lean:peek_lost"); break;
				case 4: DCO_Perf.Count("lean:peek_sector"); break;
				case 5: DCO_Perf.Count("lean:cqb_pie"); break;
				case 6: DCO_Perf.Count("lean:cqb_hall"); break;
				case 7: DCO_Perf.Count("lean:cqb_door"); break;
			}
		}

		m_fDCOLeanHoldUntil_ms = now + hold;
		if (wanted == m_fDCOLean)
		{
			if (wanted != 0)
				DCO_LeanKeeper.Register(this);
			return;
		}

		if (wanted != 0 && DCO_Nav.Busy(m_CharacterController, "lean"))
			return;

		m_fDCOLean = wanted;
		m_CharacterController.SetWantedLeaning(wanted);
		if (wanted != 0)
			DCO_LeanKeeper.Register(this);
		else
			DCO_LeanKeeper.Unregister(this);
	}

	protected void DCO_EvalLean(ChimeraCharacter ch, float now)
	{
		float wanted = 0;
		int reason = 0;
		float hold = 1500.0;
		bool peekOn = DCO_GlobalAIComponent.BehaviorOnFor(GetOwner(), DCO_EBehavior.LEAN_PEEK);
		float speed = 0;
		if (ch.GetPhysics())
			speed = ch.GetPhysics().GetVelocity().Length();
		bool moveRequest = m_Utility && m_Utility.m_CombatMoveState && m_Utility.m_CombatMoveState.IsExecutingRequest()
			&& SCR_AICombatMoveRequest_Move.Cast(m_Utility.m_CombatMoveState.GetRequest());
		bool indoor = peekOn && speed > 0.3 && speed < 2.4 && !ch.IsInVehicle() && DCO_InCQBContext(ch, now);
		bool peekActive = peekOn && m_iDCOPeekReason != 0 && now < m_fDCOPeekUntil_ms;
		bool slow = speed < 1.0 || (peekOn && speed < 2.4 && (indoor || peekActive));
		bool able = !ch.IsInVehicle() && m_CharacterController.GetStance() != ECharacterStance.PRONE && now >= m_fDCOLeanBlock_ms && (!moveRequest || slow);

		if (able)
		{
			float window = 10.0;
			if (peekOn)
				window = 30.0;
			if (m_SelectedTarget && m_SelectedTarget.GetTargetEntity() && m_SelectedTarget.GetTimeSinceSeen() < window)
			{
				bool lost = m_SelectedTarget.GetTimeSinceSeen() >= 10.0;
				bool look = true;
				if (lost)
				{
					if (m_fDCOLean != 0)
						hold = Math.Max(m_fDCOPeekLostEnd_ms - now, 0);
					else if (now < m_fDCOPeekLostNext_ms)
						look = false;
				}
				vector targetPos = m_SelectedTarget.GetLastSeenPosition() + vector.Up * DCO_TargetAimHeight(m_SelectedTarget.GetTargetEntity());
				if (look && vector.Distance(ch.GetOrigin(), targetPos) < 150.0)
				{
					m_bDCOLeanHot = true;
					if (m_fDCOLean == 0 && now < m_fDCOLeanZeroUntil_ms && vector.DistanceSq(ch.GetOrigin(), m_vDCOLeanSelf) < 0.09 && vector.DistanceSq(targetPos, m_vDCOLeanTgt) < 0.25)
					{
						DCO_Perf.Count("lean:cache_hit");
						return;
					}
					wanted = DCO_CQC.ResolveLean(ch, m_SelectedTarget.GetTargetEntity(), targetPos, m_fDCOLean, m_fDCOPreferSide);
					if (wanted == 0)
					{
						m_vDCOLeanSelf = ch.GetOrigin();
						m_vDCOLeanTgt = targetPos;
						m_fDCOLeanZeroUntil_ms = now + 1500.0;
					}
					if (lost && wanted != 0 && m_fDCOLean == 0)
					{
						reason = 3;
						hold = 2000.0;
						m_fDCOPeekLostEnd_ms = now + 2000.0;
						m_fDCOPeekLostNext_ms = now + Math.RandomFloat(5000, 8000);
					}
				}
			}
			else if (peekActive)
			{
				m_bDCOLeanHot = true;
				wanted = DCO_CQC.ResolveLean(ch, null, m_vDCOPeekPos, m_fDCOLean, m_fDCOPreferSide);
				reason = m_iDCOPeekReason;
				hold = m_fDCOPeekUntil_ms - now;
			}
			else if (indoor)
			{
				m_bDCOLeanHot = true;
				vector corner;
				wanted = DCO_CQC.CornerLean(ch, reason, corner);
				hold = 1200.0;
				if (wanted != 0 && m_Utility && m_Utility.m_LookAction)
					m_Utility.m_LookAction.LookAt(corner, 30.0, 1.0);
			}
			else if (peekOn && speed < 0.3 && now >= m_fDCOSectorNext_ms && !m_SelectedTarget
				&& m_Utility && m_Utility.m_ThreatSystem && m_Utility.m_ThreatSystem.GetState() != EAIThreatState.THREATENED)
			{
				m_fDCOSectorNext_ms = now + Math.RandomFloat(8000, 15000);
				vector fwd = ch.GetWorldTransformAxis(2);
				fwd[1] = 0;
				if (fwd.LengthSq() > 0.01)
				{
					fwd.Normalize();
					wanted = DCO_CQC.ResolveLean(ch, null, ch.EyePosition() + fwd * 25.0, 0, 0);
					reason = 4;
					hold = 2500.0;
				}
			}
			if (wanted == 0 && m_fDCOLean != 0 && now < m_fDCOLeanHoldUntil_ms)
				return;
		}

		if (wanted == 0)
			reason = 0;
		if (wanted == m_fDCOLean && wanted == 0)
			return;
		DCO_ApplyLean(wanted, hold, now, reason);
	}

	protected static float DCO_TargetAimHeight(IEntity target)
	{
		ChimeraCharacter tc = ChimeraCharacter.Cast(target);
		if (!tc || !tc.GetCharacterController())
			return 1.3;
		ECharacterStance stance = tc.GetCharacterController().GetStance();
		if (stance == ECharacterStance.STAND)
			return 1.4;
		if (stance == ECharacterStance.CROUCH)
			return 0.9;
		if (stance == ECharacterStance.PRONE)
			return 0.3;
		return 1.3;
	}

	bool DCO_KeepLean()
	{
		ChimeraCharacter ch = ChimeraCharacter.Cast(ownerEntity);
		if (!ch || !m_CharacterController || m_fDCOLean == 0)
			return false;

		bool drop = m_CharacterController.GetLifeState() != ECharacterLifeState.ALIVE || ch.IsInVehicle()
			|| m_CharacterController.GetStance() == ECharacterStance.PRONE
			|| GetGame().GetWorld().GetWorldTime() > m_fDCOLeanHoldUntil_ms
			|| DCO_Nav.Busy(m_CharacterController, "lean_keep");
		if (drop)
		{
			m_fDCOLean = 0;
			m_CharacterController.SetWantedLeaning(0);
			return false;
		}

		m_CharacterController.SetWantedLeaning(m_fDCOLean);
		return true;
	}

	float DCO_GetLean()
	{
		return m_fDCOLean;
	}

	override void Update(float timeSliceMs)
	{
		super.Update(timeSliceMs);
		slicedTime = timeSliceMs;
		DCO_UpdateLean();
		DCO_UpdateReload();
		DCO_UpdateWindowFire();
		DCO_UpdateSpecialist();
		DCO_UpdateRearm();
		DCO_UpdateGrenadeDrop();
		DCO_UpdateRest();
		DCO_UpdateStress();
		if (m_SelectedTarget)
		{
			if (ChangeTarget)
				ChangeTargetCompensation();
			else
				UpdateAccuracy(timeSliceMs);
			DCO_CheckEngageBlock();
		} else
		{
			DecreaseAim();
			m_vDCOAimDir = vector.Zero;
			CURRENT_AIM_IMPROVEMENT = DCO_RampValue();
		}
	}

	float GetCurrentAimImprovement()
	{
		return CURRENT_AIM_IMPROVEMENT;
	}

	SCR_AIUtilityComponent GetUtilityComponent()
	{
		return m_Utility;
	}

	override void EvaluateWeaponAndTarget(out bool outWeaponEvent, out bool outSelectedTargetChanged,
		out BaseTarget outPrevTarget, out BaseTarget outCurrentTarget,
		out bool outRetreatTargetChanged, out bool outCompartmentChanged)
	{
		float worldTime = GetGame().GetWorld().GetWorldTime();
		if (worldTime < m_fNextWeaponTargetEvaluation_ms)
		{
			outWeaponEvent = false;
			outSelectedTargetChanged = false;
			return;
		}

		m_fNextWeaponTargetEvaluation_ms = worldTime + WEAPON_TARGET_UPDATE_PERIOD_MS;

		float engageInfantry, engageVehicle;
		DCO_ResolveEngagement(engageInfantry, engageVehicle);
		if (engageInfantry != m_fDCOEngageInfantry || engageVehicle != m_fDCOEngageVehicle)
			SetTargetSelectionProperties(m_bDCOCloseCombat);

		SCR_ChimeraAIAgent myAgent = GetAiAgent();
		float agentThreat = m_Utility.m_ThreatSystem.GetThreatMeasure();

		AIGroup myGroup = myAgent.GetParentGroup();
		SCR_AIGroupInfoComponent groupInfoComp;
		if (myGroup)
			groupInfoComp = SCR_AIGroupInfoComponent.Cast(myGroup.FindComponent(SCR_AIGroupInfoComponent));

		BaseTarget newTarget = null;
		bool weaponEvent = false;
		bool selectedTargetChanged = false;
		bool retreatTargetChanged = false;
		bool compartmentChanged = false;

		array<EWeaponType> weaponBlacklist;
		if (groupInfoComp)
		{
			bool threatBlock = agentThreat > FRAG_GRENADE_MAX_THREAT;
			if (threatBlock && DCO_GrenCloseAttack(agentThreat))
				threatBlock = false;
			if (threatBlock || !groupInfoComp.IsGrenadeThrowAllowed(myAgent))
			{
					weaponBlacklist = s_aWeaponBlacklistFragGrenades;
				if (threatBlock && worldTime >= m_fDCOGrenRejAt)
				{
					m_fDCOGrenRejAt = worldTime + 5000.0;
					DCO_Perf.Count("gren:rej_threat");
				}
			}
		}

		if (DCO_AIConfigComponent.GrenadeUsageOf(m_Utility) <= 0.0)
			weaponBlacklist = s_aWeaponBlacklistFragGrenades;

		if (DCO_UpdatePistolMode(worldTime))
			weaponBlacklist = DCO_PistolBlacklist();
		else if (m_DCOSpec && !m_DCOSpec.m_bLauncherAllowed && (DCO_Roles.Get(m_Utility) & 2) && DCO_GlobalAIComponent.BehaviorOnFor(GetOwner(), DCO_EBehavior.AT_SPECIALIST))
			weaponBlacklist = DCO_WithLauncherBlocked(weaponBlacklist);

		bool useCompartmentWeapons = m_AIInfo.HasUnitState(EUnitState.IN_TURRET);

		array<IEntity> assignedTargets;
		if (m_TargetClusterState && m_TargetClusterState.m_Cluster && m_TargetClusterState.m_Cluster.m_aEntities)
			assignedTargets = m_TargetClusterState.m_Cluster.m_aEntities;
		else
			assignedTargets = m_aAssignedTargets;

		bool selectedWpnTarget = m_WeaponTargetSelector.SelectWeaponAndTarget(assignedTargets,
			ASSIGNED_TARGETS_SCORE_INCREMENT, ENDANGERING_TARGETS_SCORE_INCREMENT,
			useCompartmentWeapons, weaponTypesBlacklist: weaponBlacklist);

		m_eUnitTypesCanAttack = m_WeaponTargetSelector.GetUnitTypesCanAttack();
		if (selectedWpnTarget)
		{
			BaseWeaponComponent newWeaponComp;
			BaseMagazineComponent newMagazineComp;
			int newMuzzleId;

			newTarget = m_WeaponTargetSelector.GetSelectedTarget();
			m_WeaponTargetSelector.GetSelectedWeapon(newWeaponComp, newMuzzleId, newMagazineComp);
			m_WeaponTargetSelector.GetSelectedWeaponProperties(m_fSelectedWeaponMinDist, m_fSelectedWeaponMaxDist, m_bSelectedWeaponDirectDamage);

			weaponEvent = newWeaponComp != m_SelectedWeaponComp ||
							newMuzzleId != m_iSelectedMuzzle ||
							newMagazineComp != m_SelectedMagazineComp;

			bool weaponOrMuzzleChanged = newWeaponComp != m_SelectedWeaponComp ||
									newMuzzleId != m_iSelectedMuzzle;

			if (weaponOrMuzzleChanged)
			{
				ref array<BaseMuzzleComponent> muzzles = {};
				newWeaponComp.GetMuzzlesList(muzzles);
				if (newMuzzleId >= muzzles.Count() || newMuzzleId < 0)
					m_SelectedWeaponResource = m_ConfigComponent.GetTreeNameForWeaponType(newWeaponComp.GetWeaponType(),0);
				else
					m_SelectedWeaponResource = m_ConfigComponent.GetTreeNameForWeaponType(newWeaponComp.GetWeaponType(),muzzles[newMuzzleId].GetMuzzleType());

				if (newWeaponComp)
				{
					EWeaponType weaponType = newWeaponComp.GetWeaponType();
					if (groupInfoComp && weaponType == EWeaponType.WT_FRAGGRENADE)
					{
						groupInfoComp.OnAgentSelectedGrenade(myAgent);
					}
				}

				DecreaseAim();
			}

			m_SelectedWeaponComp = newWeaponComp;
			m_iSelectedMuzzle = newMuzzleId;
			m_SelectedMagazineComp = newMagazineComp;
		}

		bool atOverride = false;
		if (newTarget)
		{
			BaseTarget atTarget = DCO_GetVehicleATTarget();
			if (atTarget)
			{
				newTarget = atTarget;
				atOverride = true;
			}
		}

		bool inTurret = m_AIInfo && m_AIInfo.HasUnitState(EUnitState.IN_TURRET);
		BaseTarget selectorTarget = newTarget;
		if (newTarget && !atOverride && inTurret && DCO_GlobalAIComponent.BehaviorOnFor(GetOwner(), DCO_EBehavior.TARGET_PRIORITY))
			newTarget = DCO_TurretPriority(newTarget, worldTime);
		else if (newTarget && !atOverride && DCO_GlobalAIComponent.BehaviorOnFor(GetOwner(), DCO_EBehavior.TARGET_PRIORITY))
			newTarget = DCO_PriorityTarget(newTarget, worldTime);

		if (inTurret && newTarget && newTarget != selectorTarget && DCO_ReselectTurretWeapon(newTarget))
			weaponEvent = true;

		if (DCO_IsTunnelVisionBlocked(newTarget))
		{
			newTarget = m_SelectedTarget;
		}

		newTarget = DCO_CloseOverride(newTarget, worldTime);

		if (newTarget && newTarget != m_SelectedTarget)
			DCO_BuddyCallout(newTarget, worldTime);

		BaseTarget prevTarget = m_SelectedTarget;
		if (newTarget)
		{
			if (!prevTarget && worldTime - m_fDCOLastTarget_ms > 10000.0)
				DCO_StartReaction(newTarget, worldTime);
			m_fDCOLastTarget_ms = worldTime;
		}
		if (newTarget != m_SelectedTarget)
		{
			#ifdef AI_DEBUG
			AddDebugMessage(string.Format("Target has changed. New: %1, Previous: %2", newTarget, m_SelectedTarget));
			#endif
			m_SelectedTarget = newTarget;
			selectedTargetChanged = true;
			DCO_LogIdent(newTarget);
		}

		BaseTarget targetCantAttack;
		float targetCantAttackScore;
		m_WeaponTargetSelector.GetMostRelevantTargetCantAttack(targetCantAttack, targetCantAttackScore);
		if (targetCantAttackScore < 75.0)
			targetCantAttack = null;
		if (targetCantAttack != m_SelectedRetreatTarget)
		{
			m_SelectedRetreatTarget = targetCantAttack;
			retreatTargetChanged = true;
		}

		BaseCompartmentSlot currentCompartment = m_CompartmentAccess.GetCompartment();
		if (currentCompartment != m_WeaponEvaluationCompartment)
		{
			compartmentChanged = true;
			m_WeaponEvaluationCompartment = currentCompartment;
		}

		if (selectedTargetChanged)
		{
			m_SelectedTargetVisible = false;
			m_SelectedTargetDestinationPos = vector.Zero;
		}

		if (newTarget)
		{
			bool visible = IsTargetVisible(newTarget);
			IEntity targetEntity = newTarget.GetTargetEntity();
			ChangeTarget = selectedTargetChanged;
			if (selectedTargetChanged)
				m_bDCOAimSwitchDone = false;
			if (visible != m_SelectedTargetVisible)
			{
				m_SelectedTargetVisible = visible;

				if (!visible && targetEntity)
					m_SelectedTargetDestinationPos = targetEntity.GetOrigin();

				if (!DCO_Air.IsAirTarget(newTarget))
					DecreaseAim();
				else if (!visible)
					m_fDCOAirHideAt_ms = worldTime;
				else if (worldTime - m_fDCOAirHideAt_ms > 1000.0)
					DecreaseAim();
				else
					DCO_Perf.Count("air:aim_keep");
			}
		} else
		{
			ChangeTarget = false;
		}

		outWeaponEvent = weaponEvent;
		outSelectedTargetChanged = selectedTargetChanged;
		outRetreatTargetChanged = retreatTargetChanged;
		outCompartmentChanged = compartmentChanged;
		outCurrentTarget = newTarget;
		outPrevTarget = prevTarget;
	}

	override void UpdatePerceptionFactor(PerceptionComponent perceptionComp, SCR_AIThreatSystem threatSystem)
	{
		EAIThreatState threatState = threatSystem.GetState();
		float perceptionFactor;
		switch (threatState)
		{
			case EAIThreatState.SAFE:
				perceptionFactor = DCO_Stress.T(m_Utility, "pc.safe", 1.2); break;
			case EAIThreatState.VIGILANT:
				perceptionFactor = DCO_Stress.T(m_Utility, "pc.vigilant", 4.2); break;
			case EAIThreatState.ALERTED:
				perceptionFactor = DCO_Stress.T(m_Utility, "pc.alerted", 3.2); break;
			case EAIThreatState.THREATENED:
				perceptionFactor = DCO_Stress.T(m_Utility, "pc.threatened", 1.0); break;
		}
		if (threatState == EAIThreatState.SAFE && DCO_MissionAlert())
			perceptionFactor = DCO_Stress.T(m_Utility, "pc.vigilant", 4.2);
		DCO_AIMoraleSystem pms = m_Utility.GetMoraleSystem();
		if (pms)
		{
			moraleState pst = pms.GetState();
			if (pst >= moraleState.PANIC)
				perceptionFactor *= DCO_Stress.T(m_Utility, "pc.panic_mul", 0.6);
			else if (pst == moraleState.SHAKEN)
				perceptionFactor *= DCO_Stress.T(m_Utility, "pc.shaken_mul", 0.8);
		}
		float identMul = DCO_Stress.T(m_Utility, "pc.identify_mul", 1.0);
		perceptionFactor *= identMul;
		if (!s_bDCOPercLogged)
		{
			s_bDCOPercLogged = true;
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, "-", string.Format("perc_cfg safe=%1 vigilant=%2 alerted=%3 threatened=%4 identify_mul=%5 unit_perception=%6",
				DCO_Stress.T(m_Utility, "pc.safe", 1.2), DCO_Stress.T(m_Utility, "pc.vigilant", 4.2), DCO_Stress.T(m_Utility, "pc.alerted", 3.2), DCO_Stress.T(m_Utility, "pc.threatened", 1.0), identMul, DCO_AIConfigComponent.PerceptionOf(m_Utility)) + string.Format(" mission_floor=vigilant shaken_mul=%1 panic_mul=%2", DCO_Stress.T(m_Utility, "pc.shaken_mul", 0.8), DCO_Stress.T(m_Utility, "pc.panic_mul", 0.6)));
		}

		perceptionFactor *= m_fEquipmentPerceptionFactor;
		perceptionFactor *= m_fPerceptionFactor;
		perceptionFactor *= DCO_AIConfigComponent.PerceptionOf(m_Utility);

		float extraMag = DCO_GetScopedADSExtraMag();
		if (extraMag > 0)
			perceptionFactor *= 1 + extraMag * 0.15;

		perceptionFactor *= DCO_NightPerc();

		perceptionComp.SetPerceptionFactor(perceptionFactor);
	}

	protected IEntity m_DCOIdentLast;

	protected void DCO_LogIdent(BaseTarget t)
	{
		if (!t || !m_Utility || !m_Utility.m_OwnerEntity || !DCO_Util.IsNight())
			return;
		IEntity te = t.GetTargetEntity();
		if (!te || te == m_DCOIdentLast)
			return;
		m_DCOIdentLast = te;
		DCO_Perf.Count("night:ident");
		float illum = -1.0;
		PerceivableComponent tp = t.GetPerceivableComponent();
		if (tp)
			illum = tp.GetIlluminationFactor();
		string stance = "-";
		float speed = -1.0;
		ChimeraCharacter ch = ChimeraCharacter.Cast(te);
		if (ch && ch.GetCharacterController())
		{
			stance = typename.EnumToString(ECharacterStance, ch.GetCharacterController().GetStance());
			speed = DCO_Stance.Speed(ch.GetCharacterController());
		}
		float dist = vector.Distance(m_Utility.m_OwnerEntity.GetOrigin(), te.GetOrigin());
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("perc_ident unit=%1 tgt=%2 dist=%3 tgt_illum=%4 tgt_stance=%5 tgt_speed=%6 f=%7 visible=%8", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), DCO_Log.IdUnit(te), Math.Round(dist), illum.ToString(-1, 3), stance, speed.ToString(-1, 1), m_fDCONightF.ToString(-1, 2), DCO_Util.B01(IsTargetVisible(t))));
	}

	protected float m_fDCONightF = 1.0;
	protected float m_fDCONightNext_ms;
	protected float m_fDCONightLogged = -1.0;

	protected float DCO_NightPerc()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fDCONightNext_ms && now >= m_fDCONightNext_ms - 3000.0)
			return m_fDCONightF;
		m_fDCONightNext_ms = now + 2000.0 + Math.RandomFloat(0, 500.0);
		float f = 1.0;
		string why = "day";
		float lv = 99.0;
		float illum = -1.0;
		IEntity ent;
		if (m_Utility)
			ent = m_Utility.m_OwnerEntity;
		if (!ent)
			return 1.0;
		if (DCO_Stress.T(m_Utility, "pc.night_on", 1.0) < 0.5)
			why = "off";
		else if (DCO_Util.IsNight())
		{
			PerceivableComponent perc = PerceivableComponent.Cast(ent.FindComponent(PerceivableComponent));
			if (perc)
			{
				lv = perc.GetAmbientLV();
				illum = perc.GetIlluminationFactor();
				float lvDay = DCO_Stress.T(m_Utility, "pc.night_lv_day", 8.0);
				float lvDark = DCO_Stress.T(m_Utility, "pc.night_lv_dark", -6.0);
				float minF = DCO_Stress.T(m_Utility, "pc.night_min", 0.25);
				float t = 1.0;
				if (lvDay > lvDark)
					t = Math.Clamp((lv - lvDark) / (lvDay - lvDark), 0.0, 1.0);
				f = minF + (1.0 - minF) * t;
				why = "lv";
				float flareM = DCO_Stress.T(m_Utility, "pc.night_flare_m", 0.0);
				if (f < 1.0 && flareM > 0 && DCO_Night.FlareNear(ent.GetOrigin(), now, flareM))
				{
					f = 1.0;
					why = "flare";
					DCO_Perf.Count("night:pc_flare_full");
				}
			}
			else
				why = "no_perceivable";
		}
		if (f < 1.0)
			DCO_Perf.Count("night:pc_applied");
		m_fDCONightF = f;
		if (Math.AbsFloat(f - m_fDCONightLogged) >= 0.05)
		{
			m_fDCONightLogged = f;
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(ent), string.Format("perc_night unit=%1 lv=%2 illum=%3 f=%4 why=%5 unit_perception=%6", DCO_Log.IdUnit(ent), lv.ToString(-1, 2), illum.ToString(-1, 3), f.ToString(-1, 2), why, DCO_AIConfigComponent.PerceptionOf(m_Utility).ToString(-1, 2)));
		}
		return f;
	}


	protected BaseTarget m_DCOPrioTarget;
	protected float m_fDCOPrioNext_ms;
	protected float m_fDCOAirHideAt_ms;
	protected ref array<BaseTarget> m_aDCOPrioTargets;

	protected bool m_bDCOCEHold;
	protected float m_fDCOCEFreeUntil;
	protected ref array<float> m_aDCONearMiss = {};

	bool DCO_CEHold()
	{
		return m_bDCOCEHold;
	}

	void DCO_FreeCE(float until)
	{
		m_fDCOCEFreeUntil = Math.Max(m_fDCOCEFreeUntil, until);
	}

	void DCO_SetCEHold(bool active, float now)
	{
		if (!active)
		{
			m_bDCOCEHold = false;
			m_fDCOCEFreeUntil = 0;
			return;
		}
		bool hold = now >= m_fDCOCEFreeUntil;
		if (hold == m_bDCOCEHold)
			return;
		if (hold)
		{
			if (m_fDCOCEFreeUntil > 0)
				DCO_Perf.Count("ce_resume");
			else
				DCO_Perf.Count("ce_hold");
		}
		m_bDCOCEHold = hold;
	}

	protected vector m_vDCONearFrom;
	protected float m_fDCONearFromAt = -100000;

	void DCO_NoteNearMiss(vector from = "0 0 0")
	{
		float now = GetGame().GetWorld().GetWorldTime();
		m_aDCONearMiss.Insert(now);
		if (m_aDCONearMiss.Count() > 12)
			m_aDCONearMiss.Remove(0);
		if (from != vector.Zero)
		{
			m_vDCONearFrom = from;
			m_fDCONearFromAt = now;
		}
	}

	bool DCO_LastNearMissFrom(float now, float window, out vector from)
	{
		from = m_vDCONearFrom;
		return now - m_fDCONearFromAt <= window && from != vector.Zero;
	}

	int DCO_NearMissCount(float now, float window)
	{
		int n;
		foreach (float t : m_aDCONearMiss)
		{
			if (now - t <= window)
				n++;
		}
		return n;
	}

	protected float m_fDCOLastTarget_ms = -100000;
	protected float m_fDCOReactUntil_ms;
	protected float m_fDCOStartleAt_ms = -100000;

	bool DCO_ReactionBlocked()
	{
		return m_fDCOReactUntil_ms > 0 && GetGame().GetWorld().GetWorldTime() < m_fDCOReactUntil_ms;
	}

	protected void DCO_StartReaction(BaseTarget target, float now)
	{
		if (!DCO_GlobalAIComponent.BehaviorOnFor(GetOwner(), DCO_EBehavior.REACTION_TIME) || !m_Utility || !m_Utility.m_ThreatSystem || !ownerEntity)
			return;
		float lo, hi;
		switch (m_Utility.m_ThreatSystem.GetState())
		{
			case EAIThreatState.SAFE: lo = 0.4; hi = 0.8; break;
			case EAIThreatState.VIGILANT: lo = 0.2; hi = 0.4; break;
			case EAIThreatState.ALERTED: lo = 0.1; hi = 0.2; break;
			default: return;
		}
		float skill = 1.0;
		switch (DCO_AIConfigComponent.SkillOf(m_Utility))
		{
			case DCO_AISKILL.NOOB: skill = 1.4; break;
			case DCO_AISKILL.ROOKIE: skill = 1.2; break;
			case DCO_AISKILL.VETERAN: skill = 0.85; break;
			case DCO_AISKILL.EXPERT: skill = 0.7; break;
			case DCO_AISKILL.SPECIAL_OPS: skill = 0.6; break;
			case DCO_AISKILL.TERMINATOR: skill = 0.5; break;
		}
		float delay = Math.RandomFloat(lo, hi) * skill;

		vector fwd = ownerEntity.GetWorldTransformAxis(2);
		vector to = target.GetLastSeenPosition() - ownerEntity.GetOrigin();
		fwd[1] = 0;
		to[1] = 0;
		if (fwd.Length() > 0.1 && to.Length() > 0.1 && vector.Dot(fwd.Normalized(), to.Normalized()) < -0.5)
		{
			delay += 0.3;
			DCO_Perf.Count("react_behind");
		}
		m_fDCOReactUntil_ms = now + delay * 1000.0;
		DCO_Perf.Count("react_delay");
	}

	void DCO_TryStartle(vector impactPos)
	{
		if (!DCO_GlobalAIComponent.BehaviorOnFor(GetOwner(), DCO_EBehavior.REACTION_TIME) || !m_Utility || !m_Utility.m_ThreatSystem || !ownerEntity)
			return;
		if (m_Utility.m_ThreatSystem.GetState() != EAIThreatState.SAFE)
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fDCOStartleAt_ms < 2000.0)
			return;
		m_fDCOStartleAt_ms = now;

		ECharacterStance stance = ECharacterStance.PRONE;
		ChimeraCharacter ch = ChimeraCharacter.Cast(ownerEntity);
		if (Math.RandomFloat01() < 0.5 && ch && ch.GetCharacterController())
			stance = ch.GetCharacterController().GetStance();
		DCO_Specialist.CombatMove(m_Utility, ownerEntity.GetOrigin(), impactPos, false, stance, Math.RandomFloat(0.3, 0.7));
		DCO_Perf.Count("startle");
	}

	protected BaseTarget DCO_TurretPriority(BaseTarget vanillaPick, float now)
	{
		if (!m_Utility || !m_Utility.m_PerceptionComponent)
			return vanillaPick;
		if (now < m_fDCOPrioNext_ms)
		{
			if (m_DCOPrioTarget && m_DCOPrioTarget == m_SelectedTarget && m_DCOPrioTarget.GetTargetEntity() && m_DCOPrioTarget.GetTimeSinceSeen() < 2.0)
				return m_DCOPrioTarget;
			return vanillaPick;
		}

		int perfStart = DCO_Perf.Begin();
		if (!m_aDCOPrioTargets)
			m_aDCOPrioTargets = {};
		m_aDCOPrioTargets.Clear();
		m_Utility.m_PerceptionComponent.GetTargetsList(m_aDCOPrioTargets, ETargetCategory.ENEMY);

		bool canArmor = (m_eUnitTypesCanAttack & (EAIUnitType.UnitType_VehicleMedium | EAIUnitType.UnitType_VehicleHeavy)) != 0;
		vector myPos = ownerEntity.GetOrigin();
		BaseTarget best = vanillaPick;
		float bestScore = -1;
		string bestWhy = "vanilla";
		foreach (BaseTarget t : m_aDCOPrioTargets)
		{
			if (!t || t.GetTimeSinceSeen() >= 2.0 || !t.GetTargetEntity())
				continue;
			IEntity ent = t.GetTargetEntity();
			vector tp = t.GetLastSeenPosition();
			float d = vector.Distance(myPos, tp);
			if (d > 1500.0)
				continue;
			bool shooting = t.IsEndangering() || t.GetTimeSinceEndangered() < 3.0;
			IEntity veh = DCO_VehicleCombat.TargetVehicle(ent);
			ChimeraCharacter ch = ChimeraCharacter.Cast(ent);

			if (ch && !DCO_Util.IsAlive(ch))
				continue;

			float score = 10;
			string why = "single";
			if (veh && DCO_Air.On() && DCO_Air.IsAir(veh))
			{
				score = DCO_Ctx.TuneForU(m_Utility, "air.turret_score", 60.0);
				why = "air";
			}
			else if (veh && DCO_VehicleCombat.IsArmored(veh))
			{
				if (!canArmor && !shooting)
					continue;
				score = 60;
				why = "armor";
			}
			else if (ch && DCO_VehicleCombat.HasLauncherInHands(t))
			{
				score = 100;
				why = "at";
			}
			else if (ch && DCO_IsHeavyWeaponCarrier(ch))
			{
				score = 30;
				why = "mg";
			}
			else if (DCO_TargetsNear(tp, 6.0) >= 2)
			{
				score = 20;
				why = "cluster";
			}

			score += 5 * (1.0 - Math.Clamp(d / 1500.0, 0, 1));
			if (shooting)
				score += 5;
			if (score > bestScore)
			{
				bestScore = score;
				best = t;
				bestWhy = why;
			}
		}
		m_aDCOPrioTargets.Clear();
		DCO_Perf.End("tgt_turret", perfStart);

		if (best != m_DCOPrioTarget)
		{
			m_fDCOPrioNext_ms = now + 3000;
			DCO_Perf.Count("veh:tgt_" + bestWhy);
		}
		else
			m_fDCOPrioNext_ms = now + 1000;
		m_DCOPrioTarget = best;
		return best;
	}

	protected int DCO_TargetsNear(vector pos, float radius)
	{
		int n;
		float rSq = radius * radius;
		foreach (BaseTarget o : m_aDCOPrioTargets)
		{
			if (o && o.GetTimeSinceSeen() < 5.0 && vector.DistanceSq(o.GetLastSeenPosition(), pos) <= rSq)
				n++;
		}
		return n - 1;
	}

	protected bool DCO_ReselectTurretWeapon(BaseTarget target)
	{
		IEntity ent = target.GetTargetEntity();
		if (!ent)
			return false;
		IEntity root = DCO_VehicleCombat.TargetVehicle(ent);
		if (!root)
			root = ent;
		PerceivableComponent perc = PerceivableComponent.Cast(root.FindComponent(PerceivableComponent));
		if (!perc)
			return false;

		if (!m_WeaponTargetSelector.SelectWeaponAgainstUnitTypeAndDistance(perc.GetUnitType(), target.GetDistance(), true, true))
			return false;

		BaseWeaponComponent w;
		BaseMagazineComponent mag;
		int muzzle;
		m_WeaponTargetSelector.GetSelectedWeapon(w, muzzle, mag);
		if (!w || (w == m_SelectedWeaponComp && muzzle == m_iSelectedMuzzle && mag == m_SelectedMagazineComp))
			return false;

		m_WeaponTargetSelector.GetSelectedWeaponProperties(m_fSelectedWeaponMinDist, m_fSelectedWeaponMaxDist, m_bSelectedWeaponDirectDamage);
		if (w != m_SelectedWeaponComp || muzzle != m_iSelectedMuzzle)
		{
			array<BaseMuzzleComponent> muzzles = {};
			w.GetMuzzlesList(muzzles);
			if (muzzle >= muzzles.Count() || muzzle < 0)
				m_SelectedWeaponResource = m_ConfigComponent.GetTreeNameForWeaponType(w.GetWeaponType(), 0);
			else
				m_SelectedWeaponResource = m_ConfigComponent.GetTreeNameForWeaponType(w.GetWeaponType(), muzzles[muzzle].GetMuzzleType());
			DecreaseAim();
		}
		m_SelectedWeaponComp = w;
		m_iSelectedMuzzle = muzzle;
		m_SelectedMagazineComp = mag;
		DCO_Perf.Count("veh:ammo_switch");
		if (DCO_Log.On(DCO_ELogChannel.VEH, DCO_ELogLevel.DEBUG) && m_Utility)
		{
			string magName = "-";
			if (mag && mag.GetOwner() && mag.GetOwner().GetPrefabData())
				magName = FilePath.StripExtension(FilePath.StripPath(mag.GetOwner().GetPrefabData().GetPrefabName()));
			DCO_Log.Write(DCO_ELogChannel.VEH, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("veh_ammo unit=%1 tgt_type=%2 dist=%3 weapon=%4 muzzle=%5 mag=%6",
				DCO_Log.IdUnit(m_Utility.m_OwnerEntity), typename.EnumToString(EAIUnitType, perc.GetUnitType()), Math.Round(target.GetDistance()), typename.EnumToString(EWeaponType, w.GetWeaponType()), muzzle, magName));
		}
		return true;
	}

	protected bool DCO_GrenCloseAttack(float threat)
	{
		if (DCO_Stress.T(m_Utility, "gren.atk_close", 1.0) <= 0)
			return false;
		if (threat > FRAG_GRENADE_MAX_THREAT * DCO_Stress.T(m_Utility, "gren.atk_threat_mul", 2.0))
			return false;
		if (!m_SelectedTarget || !m_SelectedTarget.GetTargetEntity() || m_SelectedTarget.GetDistance() > DCO_Stress.T(m_Utility, "gren.atk_close_m", 35.0))
			return false;
		DCO_AIMoraleSystem ms = m_Utility.GetMoraleSystem();
		if (ms && ms.GetState() >= moraleState.PANIC)
			return false;
		AIAgent agent = m_Utility.GetAIAgent();
		if (!agent || !agent.GetParentGroup())
			return false;
		SCR_AIGroup grp = SCR_AIGroup.Cast(agent.GetParentGroup());
		bool attacker = SCR_SearchAndDestroyWaypoint.Cast(grp.GetCurrentWaypoint()) != null;
		if (!attacker && grp.GetGroupUtilityComponent() && grp.GetGroupUtilityComponent().DCO_GetPosture() == DCO_GroupTactics.AGGRESIVE)
			attacker = true;
		if (!attacker)
		{
			DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));
			if (g)
			{
				DCO_EGroupTask t = g.GetTask();
				attacker = t == DCO_EGroupTask.ATTACK || t == DCO_EGroupTask.FLANK || t == DCO_EGroupTask.REINFORCE;
			}
		}
		if (attacker)
			DCO_Perf.Count("gren:atk_close_allow");
		return attacker;
	}

	protected BaseTarget DCO_PriorityTarget(BaseTarget vanillaPick, float now)
	{
		if (!m_Utility || !m_Utility.m_PerceptionComponent || !m_SelectedWeaponComp || !ChimeraCharacter.Cast(vanillaPick.GetTargetEntity()))
			return vanillaPick;

		if (now < m_fDCOPrioNext_ms)
		{
			if (!m_DCOPrioTarget || m_DCOPrioTarget != m_SelectedTarget || !m_DCOPrioTarget.GetTargetEntity() || m_DCOPrioTarget.GetTimeSinceSeen() >= 2.0)
				return vanillaPick;
			float holdNearM = DCO_Stress.T(m_Utility, "rt.close_m", 30.0);
			float holdD = vector.Distance(ownerEntity.GetOrigin(), m_DCOPrioTarget.GetTargetEntity().GetOrigin());
			if (holdD < holdNearM)
				return m_DCOPrioTarget;
			BaseTarget holdNear = DCO_FindCloseTarget(holdNearM);
			if (!holdNear)
				return m_DCOPrioTarget;
			DCO_Perf.Count("tgt:hold_break_near");
			if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(ownerEntity), string.Format("tgt_hold_break unit=%1 held=%2 held_d=%3 near=%4 near_d=%5", DCO_Log.IdUnit(ownerEntity), DCO_Log.IdUnit(m_DCOPrioTarget.GetTargetEntity()), Math.Round(holdD), DCO_Log.IdUnit(holdNear.GetTargetEntity()), Math.Round(vector.Distance(ownerEntity.GetOrigin(), holdNear.GetTargetEntity().GetOrigin()))));
		}

		int perfStart = DCO_Perf.Begin();
		BaseTarget best = DCO_RankTargets(vanillaPick);
		DCO_Perf.End("tgt_priority", perfStart);

		if (best != m_DCOPrioTarget)
			m_fDCOPrioNext_ms = now + 3000;
		else
			m_fDCOPrioNext_ms = now + 1000;
		m_DCOPrioTarget = best;
		return best;
	}

	protected float DCO_CrewBonus(ChimeraCharacter tc)
	{
		if (!tc.IsInVehicle())
			return 0;
		CompartmentAccessComponent ca = CompartmentAccessComponent.Cast(tc.FindComponent(CompartmentAccessComponent));
		if (!ca || !ca.GetCompartment())
			return 1.0;
		if (TurretCompartmentSlot.Cast(ca.GetCompartment()))
		{
			DCO_Perf.Count("tgt:crew_gunner");
			return 4.0;
		}
		if (PilotCompartmentSlot.Cast(ca.GetCompartment()))
			return 2.0;
		return 1.0;
	}

	protected BaseTarget DCO_RankTargets(BaseTarget vanillaPick)
	{
		if (!m_aDCOPrioTargets)
			m_aDCOPrioTargets = {};
		m_aDCOPrioTargets.Clear();
		m_Utility.m_PerceptionComponent.GetTargetsList(m_aDCOPrioTargets, ETargetCategory.ENEMY);

		float range = DCO_LongRange.EffectiveRange(m_SelectedWeaponComp.GetWeaponType());
		int roles = DCO_Roles.Get(m_Utility);
		bool rolePref = (roles & 1) || ((roles & 4) && DCO_GlobalAIComponent.BehaviorOnFor(GetOwner(), DCO_EBehavior.MARKSMAN));
		vector myPos = ownerEntity.GetOrigin();
		BaseTarget best = vanillaPick;
		float bestScore = -1;
		float bestD = 0;
		bool bestHeavy = false;
		bool bestShoot = false;
		BaseTarget second;
		float secondScore = -1;
		float secondD = 0;
		bool secondHeavy = false;
		BaseTarget pref;
		float prefScore = -1;
		float bestOpp = 0;
		string bestOppWhy;
		bool claimOn = DCO_TargetClaim.On(m_Utility);
		float claimPen = 0;
		if (claimOn)
			claimPen = DCO_Stress.T(m_Utility, "tgt.claim_pen", 1.5);
		BaseTarget rawBest;
		float rawBestScore = -1;
		int bestClaims;
		float nearM = DCO_Stress.T(m_Utility, "rt.close_m", 30.0);
		bool nearRpg = m_SelectedWeaponComp && m_SelectedWeaponComp.GetWeaponType() == EWeaponType.WT_ROCKETLAUNCHER;
		foreach (BaseTarget t : m_aDCOPrioTargets)
		{
			if (!t || t.GetTimeSinceSeen() >= 2.0)
				continue;
			IEntity te = t.GetTargetEntity();
			ChimeraCharacter tc = ChimeraCharacter.Cast(te);
			bool air = !tc && DCO_Air.IsAirTarget(t);
			if (!air && (!tc || !DCO_Util.IsAlive(tc)))
				continue;
			float tRange = range;
			if (air)
				tRange = Math.Max(range, DCO_Air.OpenRange());
			float d = vector.Distance(myPos, te.GetOrigin());
			if (d > tRange)
				continue;
			bool shooting = t.IsEndangering() || t.GetTimeSinceEndangered() < 3.0;
			bool heavy = air || DCO_IsHeavyWeaponCarrier(tc) || tc.IsInVehicle();
			float score = 4.0 * (1.0 - d / tRange);
			if (shooting)
				score += 10.0;
			if (heavy)
				score += 2.0;
			if (air)
				score += DCO_Ctx.TuneForU(m_Utility, "air.inf_bonus", 3.0);
			else
				score += DCO_CrewBonus(tc);
			string oppWhy;
			float opp = 0;
			if (!air)
				opp = DCO_Opp.TargetBonus(m_Utility, t, tc, d, oppWhy);
			score += opp;
			bool nearT = !air && !nearRpg && d < nearM;
			if (nearT)
			{
				score += 40.0 + 10.0 * (1.0 - d / nearM);
				DCO_Perf.Count("tgt:near_bonus");
			}
			int claims;
			if (claimOn)
			{
				if (score > rawBestScore)
				{
					rawBestScore = score;
					rawBest = t;
				}
				if (!t.IsEndangering() && !(roles & 1))
				{
					claims = DCO_TargetClaim.Others(m_Utility, te);
					if (!nearT)
						score -= claimPen * Math.Min(claims, 2);
					else if (claims > 0)
					{
						DCO_Perf.Count("tgt:claim_skip_close");
						if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
							DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(ownerEntity), string.Format("tgt_claim_skip_close unit=%1 tgt=%2 dist=%3 claims=%4 near_m=%5", DCO_Log.IdUnit(ownerEntity), DCO_Log.IdUnit(te), Math.Round(d), claims, Math.Round(nearM)));
					}
				}
			}
			if (score > bestScore)
			{
				bestClaims = claims;
				bestOpp = opp;
				bestOppWhy = oppWhy;
				second = best;
				secondScore = bestScore;
				secondD = bestD;
				secondHeavy = bestHeavy;
				best = t;
				bestScore = score;
				bestD = d;
				bestHeavy = heavy;
				bestShoot = shooting;
			}
			else if (score > secondScore)
			{
				second = t;
				secondScore = score;
				secondD = d;
				secondHeavy = heavy;
			}
			if (rolePref && (heavy || DCO_IsLeaderOrRTO(tc)) && score > prefScore)
			{
				pref = t;
				prefScore = score;
			}
		}
		m_aDCOPrioTargets.Clear();
		if (bestScore < 0)
			return vanillaPick;
		if (rolePref && pref && pref != best && prefScore >= bestScore * 0.8)
		{
			DCO_Perf.Count("tgt:role_pref");
			return pref;
		}
		if (second && second != best && secondScore >= bestScore * 0.9)
		{
			DCO_EAIPersonality personality = DCO_AIConfigComponent.PersonalityOf(m_Utility);
			bool swap = false;
			if ((personality == DCO_EAIPersonality.CAUTIOUS || personality == DCO_EAIPersonality.RECKLESS) && secondD < bestD)
				swap = true;
			else if (personality == DCO_EAIPersonality.AGGRESSIVE && secondHeavy && !bestHeavy)
				swap = true;
			if (swap)
			{
				DCO_Perf.Count("tgt:tiebreak_personality");
				return second;
			}
		}
		if (bestShoot)
			DCO_Perf.Count("tgt:threat_pick");
		if (best != m_DCOPrioTarget)
			DCO_Opp.NotePick(m_Utility, best, bestD, bestOpp, bestOppWhy);
		if (claimOn && best && best.GetTargetEntity())
		{
			DCO_TargetClaim.Claim(m_Utility, best.GetTargetEntity());
			if (rawBest && rawBest != best)
			{
				DCO_Perf.Count("tgt:claim_spread");
				if (best != m_DCOPrioTarget && DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
					DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(ownerEntity), string.Format("tgt_claim unit=%1 picked_d=%2 claims_on_pick=%3 skipped_d=%4", DCO_Log.IdUnit(ownerEntity), Math.Round(bestD), bestClaims, Math.Round(vector.Distance(ownerEntity.GetOrigin(), rawBest.GetTargetEntity().GetOrigin()))));
			}
			else
				DCO_Perf.Count("tgt:claim_same");
		}
		if (best != vanillaPick && DCO_Air.IsAirTarget(best))
			DCO_Perf.Count("tgt:air_pick");
		return best;
	}

	protected static float DCO_MarksmanScore(ChimeraCharacter ch)
	{
		float score = 0;
		AIControlComponent ctrl = AIControlComponent.Cast(ch.FindComponent(AIControlComponent));
		if (ctrl && ctrl.GetControlAIAgent() && ctrl.GetControlAIAgent().GetParentGroup() && ctrl.GetControlAIAgent().GetParentGroup().GetLeaderEntity() == ch)
			score += 8;
		BaseWeaponManagerComponent wm = BaseWeaponManagerComponent.Cast(ch.FindComponent(BaseWeaponManagerComponent));
		if (wm && wm.GetCurrentWeapon())
		{
			EWeaponType wt = wm.GetCurrentWeapon().GetWeaponType();
			if (wt == EWeaponType.WT_MACHINEGUN)
				score += 4;
			else if (wt == EWeaponType.WT_ROCKETLAUNCHER)
				score += 2;
		}
		if (DCO_Util.HasManpackRadio(ch))
			score += 1;
		return score;
	}


	protected static bool DCO_IsHeavyWeaponCarrier(ChimeraCharacter ch)
	{
		BaseWeaponManagerComponent wm = BaseWeaponManagerComponent.Cast(ch.FindComponent(BaseWeaponManagerComponent));
		if (!wm || !wm.GetCurrentWeapon())
			return false;
		EWeaponType wt = wm.GetCurrentWeapon().GetWeaponType();
		return wt == EWeaponType.WT_ROCKETLAUNCHER || wt == EWeaponType.WT_MACHINEGUN;
	}

	protected static bool DCO_IsLeaderOrRTO(ChimeraCharacter ch)
	{
		AIControlComponent ctrl = AIControlComponent.Cast(ch.FindComponent(AIControlComponent));
		if (ctrl && ctrl.GetControlAIAgent())
		{
			AIGroup grp = ctrl.GetControlAIAgent().GetParentGroup();
			if (grp && grp.GetLeaderEntity() == ch)
				return true;
		}
		return DCO_Util.HasManpackRadio(ch);
	}


	protected float m_fDCOReloadCheck_ms;
	protected float m_fDCOReloadWaitSince_ms;
	protected float m_fDCOReloadCallout_ms;
	protected bool m_bDCOWasReloading;
	protected bool m_bDCOPistolMode;
	protected float m_fDCOPistolLastClose_ms;
	protected ref array<EWeaponType> m_aDCOPistolBlacklist;

	protected void DCO_UpdateReload()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fDCOReloadCheck_ms || !m_CharacterController)
			return;
		m_fDCOReloadCheck_ms = now + 1000;
		if (!DCO_GlobalAIComponent.BehaviorOnFor(GetOwner(), DCO_EBehavior.RELOAD_DISCIPLINE))
			return;

		int perfStart = DCO_Perf.Begin();
		bool reloading = m_CharacterController.IsReloading();
		if (reloading && !m_bDCOWasReloading)
			DCO_ReloadCallout(now);
		m_bDCOWasReloading = reloading;
		if (!reloading && !m_bDCOPistolMode)
			DCO_EvalTacticalReload(now);
		DCO_Perf.End("reload_disc", perfStart);
	}

	protected void DCO_ReloadCallout(float now)
	{
		if (now - m_fDCOReloadCallout_ms < 10000 || !m_Utility || !m_Utility.m_CommsHandler || m_Utility.m_CommsHandler.CanBypass())
			return;
		m_fDCOReloadCallout_ms = now;
		SCR_AITalkRequest rq = new SCR_AITalkRequest(ECommunicationType.REPORT_RELOADING, null, vector.Zero, 0, false, false, SCR_EAITalkRequestPreset.IRRELEVANT);
		m_Utility.m_CommsHandler.AddRequest(rq);
	}

	protected void DCO_EvalTacticalReload(float now)
	{
		if (!m_SelectedWeaponComp)
			return;
		EWeaponType wt = m_SelectedWeaponComp.GetWeaponType();
		if (wt != EWeaponType.WT_RIFLE && wt != EWeaponType.WT_MACHINEGUN && wt != EWeaponType.WT_SNIPERRIFLE && wt != EWeaponType.WT_HANDGUN)
			return;

		BaseMagazineComponent mag = m_SelectedWeaponComp.GetCurrentMagazine();
		if (!mag || mag.GetMaxAmmoCount() <= 0)
			return;
		int ammo = mag.GetAmmoCount();
		if (ammo <= 0 || ammo >= 0.3 * mag.GetMaxAmmoCount())
		{
			m_fDCOReloadWaitSince_ms = 0;
			return;
		}
		if (m_SelectedTarget && m_SelectedTarget.GetTimeSinceSeen() <= 3.0)
			return;

		bool steady = m_Utility && m_Utility.m_CombatMoveState && m_Utility.m_CombatMoveState.m_bInCover;
		if (!steady && ownerEntity.GetPhysics())
			steady = ownerEntity.GetPhysics().GetVelocity().Length() < 0.5;
		if (!steady)
			return;

		BaseMagazineWell well = mag.GetMagazineWell();
		if (!well || GetMagazineCount(well.Type(), false) <= 0)
			return;

		if (DCO_MateReloading())
		{
			if (m_fDCOReloadWaitSince_ms <= 0)
				m_fDCOReloadWaitSince_ms = now;
			if (now - m_fDCOReloadWaitSince_ms < 2000)
				return;
		}

		m_fDCOReloadWaitSince_ms = 0;
		if (m_CharacterController.ReloadWeapon())
			DCO_Perf.Count("ev:tac_reload");
	}

	protected bool DCO_MateReloading()
	{
		SCR_ChimeraAIAgent me = GetAiAgent();
		if (!me || !me.GetParentGroup())
			return false;
		array<AIAgent> mates = {};
		me.GetParentGroup().GetAgents(mates);
		foreach (AIAgent a : mates)
		{
			if (a == me)
				continue;
			ChimeraCharacter mc = ChimeraCharacter.Cast(a.GetControlledEntity());
			if (mc && mc.GetCharacterController() && mc.GetCharacterController().IsReloading())
				return true;
		}
		return false;
	}

	protected bool DCO_UpdatePistolMode(float now)
	{
		if (!DCO_GlobalAIComponent.BehaviorOnFor(GetOwner(), DCO_EBehavior.PISTOL_CQB) || (m_AIInfo && m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE)))
		{
			m_bDCOPistolMode = false;
			return false;
		}

		bool close = m_SelectedTarget && m_SelectedTarget.GetTargetEntity() && m_SelectedTarget.GetTimeSinceSeen() < 5.0
			&& vector.Distance(ownerEntity.GetOrigin(), m_SelectedTarget.GetTargetEntity().GetOrigin()) < 10.0;
		if (close)
			m_fDCOPistolLastClose_ms = now;

		if (m_bDCOPistolMode)
		{
			if (!close && now - m_fDCOPistolLastClose_ms > 5000)
				m_bDCOPistolMode = false;
			return m_bDCOPistolMode;
		}

		if (!close || !m_SelectedWeaponComp)
			return false;
		EWeaponType wt = m_SelectedWeaponComp.GetWeaponType();
		if (wt != EWeaponType.WT_RIFLE && wt != EWeaponType.WT_MACHINEGUN && wt != EWeaponType.WT_SNIPERRIFLE)
			return false;
		BaseMagazineComponent mag = m_SelectedWeaponComp.GetCurrentMagazine();
		if (mag && mag.GetAmmoCount() > 0)
			return false;
		if (!DCO_HasLoadedHandgun())
			return false;

		m_bDCOPistolMode = true;
		DCO_Perf.Count("ev:pistol_cqb");
		return true;
	}

	protected bool DCO_HasLoadedHandgun()
	{
		BaseWeaponManagerComponent wm = BaseWeaponManagerComponent.Cast(ownerEntity.FindComponent(BaseWeaponManagerComponent));
		if (!wm)
			return false;
		array<IEntity> weapons = {};
		wm.GetWeaponsList(weapons);
		foreach (IEntity w : weapons)
		{
			BaseWeaponComponent wc = BaseWeaponComponent.Cast(w.FindComponent(BaseWeaponComponent));
			if (!wc || wc.GetWeaponType() != EWeaponType.WT_HANDGUN)
				continue;
			BaseMagazineComponent hm = wc.GetCurrentMagazine();
			if (hm && hm.GetAmmoCount() > 0)
				return true;
		}
		return false;
	}

	protected array<EWeaponType> DCO_PistolBlacklist()
	{
		if (!m_aDCOPistolBlacklist)
		{
			m_aDCOPistolBlacklist = {};
			m_aDCOPistolBlacklist.Insert(EWeaponType.WT_RIFLE);
			m_aDCOPistolBlacklist.Insert(EWeaponType.WT_GRENADELAUNCHER);
			m_aDCOPistolBlacklist.Insert(EWeaponType.WT_SNIPERRIFLE);
			m_aDCOPistolBlacklist.Insert(EWeaponType.WT_ROCKETLAUNCHER);
			m_aDCOPistolBlacklist.Insert(EWeaponType.WT_MACHINEGUN);
			m_aDCOPistolBlacklist.Insert(EWeaponType.WT_FRAGGRENADE);
			m_aDCOPistolBlacklist.Insert(EWeaponType.WT_SMOKEGRENADE);
		}
		return m_aDCOPistolBlacklist;
	}

	protected float m_fDCOCalloutAt_ms;
	protected IEntity m_DCOCalloutEnt;

	protected void DCO_BuddyCallout(BaseTarget t, float now)
	{
		if (!t || !t.GetTargetEntity() || t.GetTimeSinceSeen() > 1.0 || !DCO_GlobalAIComponent.BehaviorOnFor(GetOwner(), DCO_EBehavior.BUDDY_CALLOUT))
			return;
		IEntity tEnt = t.GetTargetEntity();
		if (tEnt == m_DCOCalloutEnt || now - m_fDCOCalloutAt_ms < 10000)
			return;
		m_DCOCalloutEnt = tEnt;
		m_fDCOCalloutAt_ms = now;

		SCR_ChimeraAIAgent me = GetAiAgent();
		if (!me || !me.GetParentGroup())
			return;

		int perfStart = DCO_Perf.Begin();
		vector tpos = t.GetLastSeenPosition() + Vector(0, 1.2, 0);
		vector myPos = ownerEntity.GetOrigin();
		array<AIAgent> mates = {};
		me.GetParentGroup().GetAgents(mates);
		foreach (AIAgent a : mates)
		{
			SCR_ChimeraAIAgent mate = SCR_ChimeraAIAgent.Cast(a);
			if (!mate || mate == me || !mate.GetControlledEntity())
				continue;
			if (vector.DistanceSq(myPos, mate.GetControlledEntity().GetOrigin()) > 2500)
				continue;
			GetGame().GetCallqueue().CallLater(DCO_BuddyLook, Math.RandomInt(1000, 2000), false, mate, tpos);
		}

		if (m_Utility && m_Utility.m_CommsHandler && !m_Utility.m_CommsHandler.CanBypass() && DCO_Opp.SquadVoiceOk(me.GetParentGroup(), tEnt, 8000.0))
		{
			SCR_AITalkRequest rq = new SCR_AITalkRequest(ECommunicationType.REPORT_CONTACT, tEnt, tpos, 0, false, false, SCR_EAITalkRequestPreset.MEDIUM);
			m_Utility.m_CommsHandler.AddRequest(rq);
		}
		DCO_Perf.Count("ev:buddy_callout");
		DCO_Perf.End("buddy_callout", perfStart);
	}

	protected static void DCO_BuddyLook(SCR_ChimeraAIAgent mate, vector pos)
	{
		if (mate && mate.m_UtilityComponent && mate.m_UtilityComponent.m_LookAction)
			mate.m_UtilityComponent.m_LookAction.LookAt(pos, 50.0, 2.0);
	}


	protected DCO_GarrisonBuilding m_DCOWindowBuilding;
	protected int m_iDCOWindowSlot = -1;
	protected float m_fDCOWindowCheck_ms;

	protected void DCO_UpdateWindowFire()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fDCOWindowCheck_ms)
			return;
		m_fDCOWindowCheck_ms = now + 4000.0 + Math.RandomFloat(0, 2000.0);

		bool contact = m_SelectedTarget && m_SelectedTarget.GetTargetEntity() && m_SelectedTarget.GetTimeSinceSeen() < 20.0;
		if (!contact || !DCO_GlobalAIComponent.BehaviorOnFor(GetOwner(), DCO_EBehavior.WINDOW_FIRE) || !Replication.IsServer())
		{
			DCO_ReleaseWindow();
			return;
		}
		if (m_DCOWindowBuilding || !m_Utility || !m_Utility.m_CombatMoveState || (m_AIInfo && m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE)))
			return;
		if (m_Utility.m_CombatMoveState.IsExecutingRequest() && DCO_AICombatMoveRequest_IndoorRelocate.Cast(m_Utility.m_CombatMoveState.GetRequest()))
			return;

		SCR_ChimeraAIAgent me = GetAiAgent();
		SCR_AIGroup grp;
		if (me)
			grp = SCR_AIGroup.Cast(me.GetParentGroup());
		if (!grp || !grp.GetGroupUtilityComponent() || grp.GetGroupUtilityComponent().DCO_GetGarrisonMode() >= 0)
			return;
		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		if (!reg)
			return;

		int perfStart = DCO_Perf.Begin();
		vector myPos = ownerEntity.GetOrigin();
		vector enemyPos = m_SelectedTarget.GetLastSeenPosition();
		DCO_GarrisonBuilding bestB;
		int bestIdx = -1;
		float bestSq = 900.0;
		foreach (IEntity building : grp.GetGroupUtilityComponent().DCO_GetNearbyBuildings(now))
		{
			if (!building || vector.DistanceSqXZ(myPos, building.GetOrigin()) > 2500.0)
				continue;
			vector bMin, bMax;
			building.GetWorldBounds(bMin, bMax);
			float bx = Math.Max(Math.Max(bMin[0] - myPos[0], 0.0), myPos[0] - bMax[0]);
			float bz = Math.Max(Math.Max(bMin[2] - myPos[2], 0.0), myPos[2] - bMax[2]);
			if (bx * bx + bz * bz >= bestSq)
			{
				DCO_Perf.Count("window:bld_skip");
				continue;
			}
			DCO_GarrisonBuilding b = reg.Request(building);
			if (!b || !b.m_bReady || reg.IsOwnedByOther(b, grp))
				continue;
			foreach (int i, DCO_GarrisonSlot s : b.m_aSlots)
			{
				if (!s || s.m_bUnreachable || (s.m_eType != DCO_EGarrisonSlotType.WINDOW && s.m_eType != DCO_EGarrisonSlotType.ROOF))
					continue;
				if (!b.m_aBookedBy.IsIndexValid(i) || b.m_aBookedBy[i])
					continue;
				float dSq = vector.DistanceSq(myPos, s.m_vWorldPos);
				if (s.m_fWinDist >= 0 && s.m_fWinDist < 1.0 && DCO_GlobalAIComponent.BehaviorOnFor(GetOwner(), DCO_EBehavior.AWARENESS))
					dSq += 16.0;
				if (dSq >= bestSq)
					continue;
				vector toEnemy = enemyPos - s.m_vWorldPos;
				toEnemy[1] = 0;
				if (toEnemy.LengthSq() < 1)
					continue;
				toEnemy.Normalize();
				vector dir = s.m_vWorldDir;
				dir[1] = 0;
				dir.Normalize();
				if (vector.Dot(dir, toEnemy) < 0.5)
					continue;
				bestSq = dSq;
				bestB = b;
				bestIdx = i;
			}
		}

		if (bestB)
		{
			reg.Book(bestB, bestIdx, me);
			m_DCOWindowBuilding = bestB;
			m_iDCOWindowSlot = bestIdx;
			DCO_GarrisonSlot slot = bestB.m_aSlots[bestIdx];

			vector slotPos = slot.m_vWorldPos;
			ECharacterStance endStance = slot.m_eStance;
			float distIn = slot.m_fWinDist;
			bool roof = slot.m_eType == DCO_EGarrisonSlotType.ROOF;
			if (!roof && slot.m_fWinDist >= 0 && slot.m_fWinDist < 1.0)
			{
				vector inward = slot.m_vWorldDir;
				inward[1] = 0;
				inward.Normalize();
				float shift = 1.5 - slot.m_fWinDist;
				slotPos = slot.m_vWorldPos - inward * shift;
				distIn = slot.m_fWinDist + shift;
				endStance = ECharacterStance.CROUCH;
				DCO_Perf.Count("window_slot:pulled_in");
			}
			if (roof && endStance == ECharacterStance.STAND)
				endStance = ECharacterStance.CROUCH;
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(ownerEntity), string.Format("window_slot_pos unit=%1 dist_in=%2 stance=%3 roof=%4", DCO_Log.IdUnit(ownerEntity), distIn.ToString(-1, 1), typename.EnumToString(ECharacterStance, endStance), roof));

			SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
			rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
			rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
			rq.m_vTargetPos = enemyPos;
			rq.m_vMovePos = slotPos;
			rq.m_bTryFindCover = false;
			rq.m_bFailIfNoCover = false;
			rq.m_eStanceMoving = ECharacterStance.CROUCH;
			rq.m_eStanceEnd = endStance;
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_bAimAtTarget = false;
			rq.m_bAimAtTargetEnd = true;
			rq.m_fMoveDuration_s = Math.Sqrt(bestSq) / 3.0 + 2.0;
			m_Utility.m_CombatMoveState.ApplyNewRequest(rq);
			DCO_Perf.Count("ev:window_slot");
		}
		DCO_Perf.End("window_fire", perfStart);
	}

	protected void DCO_ReleaseWindow()
	{
		if (m_DCOWindowBuilding && m_DCOWindowBuilding.m_aBookedBy.IsIndexValid(m_iDCOWindowSlot) && m_DCOWindowBuilding.m_aBookedBy[m_iDCOWindowSlot] == GetAiAgent())
			m_DCOWindowBuilding.m_aBookedBy[m_iDCOWindowSlot] = null;
		m_DCOWindowBuilding = null;
		m_iDCOWindowSlot = -1;
	}


	protected float m_fDCOGrenRejAt;
	protected float m_fDCODropCheck_ms;

	protected void DCO_UpdateGrenadeDrop()
	{
		if (!m_Utility || !Replication.IsServer())
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fDCODropCheck_ms)
			return;
		m_fDCODropCheck_ms = now + 1000.0;
		DCO_BreachUtility.TryDropGrenade(m_Utility);
	}

	protected ref DCO_RearmState m_DCORearm = new DCO_RearmState();

	int DCO_RearmInit(typename well)
	{
		if (!m_DCORearm || m_DCORearm.m_InitWell != well)
			return -1;
		return m_DCORearm.m_iInitSpare;
	}
	protected ref DCO_PickupState m_DCOPickup = new DCO_PickupState();
	protected float m_fDCORearmCheck_ms;

	protected void DCO_UpdateRearm()
	{
		if (!m_Utility || !Replication.IsServer())
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fDCORearmCheck_ms)
			return;
		m_fDCORearmCheck_ms = now + 2000.0;
		int pt = DCO_Perf.Begin();
		DCO_Rearm.Tick(m_Utility, this, m_DCORearm, now);
		DCO_WeaponPickup.Tick(m_Utility, this, m_DCOPickup, now);
		DCO_Perf.End("agent_rearm", pt);
	}

	protected ref DCO_SpecState m_DCOSpec;
	protected ref array<EWeaponType> m_aDCOCombinedBlacklist;

	DCO_SpecState DCO_GetSpec()
	{
		return m_DCOSpec;
	}

	protected void DCO_UpdateSpecialist()
	{
		if (!m_Utility || !Replication.IsServer())
			return;
		if (!m_DCOSpec)
		{
			if ((DCO_Roles.Get(m_Utility) & 7) == 0)
				return;
			m_DCOSpec = new DCO_SpecState();
		}
		DCO_Specialist.Tick(m_Utility, m_DCOSpec, GetGame().GetWorld().GetWorldTime());
	}

	void DCO_OnNearImpact()
	{
		if (m_DCOSpec)
			m_DCOSpec.m_fNearImpact_ms = GetGame().GetWorld().GetWorldTime();
		if (m_fDCOLean == 0 || !m_CharacterController)
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		m_fDCOPreferSide = -m_fDCOLean;
		m_fDCOLeanBlock_ms = now + 1500.0;
		m_fDCOPeekShootNext_ms = now + 1500.0;
		DCO_ApplyLean(0, 0, now, 0);
	}

	protected array<EWeaponType> DCO_WithLauncherBlocked(array<EWeaponType> base)
	{
		if (!m_aDCOCombinedBlacklist)
			m_aDCOCombinedBlacklist = {};
		m_aDCOCombinedBlacklist.Clear();
		if (base)
			m_aDCOCombinedBlacklist.Copy(base);
		if (!m_aDCOCombinedBlacklist.Contains(EWeaponType.WT_ROCKETLAUNCHER))
			m_aDCOCombinedBlacklist.Insert(EWeaponType.WT_ROCKETLAUNCHER);
		return m_aDCOCombinedBlacklist;
	}


	protected BaseTarget DCO_GetVehicleATTarget()
	{
		if (!m_AIInfo || !m_AIInfo.HasUnitState(EUnitState.IN_TURRET) || !m_Utility || !m_Utility.m_PerceptionComponent)
			return null;

		IEntity shooter;
		vector pos;
		if (!DCO_VehicleCombat.GetATThreat(DCO_VehicleCombat.GetVehicle(ownerEntity), shooter, pos))
			return null;

		BaseTarget target = m_Utility.m_PerceptionComponent.GetTargetPerceptionObject(shooter, ETargetCategory.ENEMY);
		if (!target || target.GetTimeSinceSeen() > 6.0)
			return null;

		return target;
	}

	protected float DCO_GetScopedADSExtraMag()
	{
		if (!m_CharacterController || !m_CharacterController.IsWeaponADS())
			return 0;

		return DCO_AimUtility.GetSightMagnification(this) - 1;
	}

	protected bool DCO_IsTunnelVisionBlocked(BaseTarget candidate)
	{
		if (!candidate || !m_SelectedTarget || candidate == m_SelectedTarget || candidate.IsEndangering())
			return false;

		IEntity current = m_SelectedTarget.GetTargetEntity();
		IEntity other = candidate.GetTargetEntity();
		if (!current || !other || !ownerEntity || !IsTargetVisible(m_SelectedTarget))
			return false;

		float extraMag = DCO_GetScopedADSExtraMag();
		if (extraMag <= 0 || candidate.GetTimeSinceSideRecognized() > extraMag * 1.0)
			return false;

		vector me = ownerEntity.GetOrigin();
		vector toCurrent = current.GetOrigin() - me;
		vector toOther = other.GetOrigin() - me;
		toCurrent[1] = 0;
		toOther[1] = 0;
		if (toOther.Length() < DCO_Stress.T(m_Utility, "rt.close_m", 30.0))
		{
			DCO_Perf.Count("tgt:tunnel_skip_near");
			return false;
		}
		bool tunnel = vector.Dot(toCurrent.Normalized(), toOther.Normalized()) < Math.Cos(20.0 * Math.DEG2RAD);
		if (tunnel)
			DCO_Perf.Count("tgt:tunnel_block");
		return tunnel;
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		CURRENT_AIM_IMPROVEMENT = 2.5;
		ownerEntity = owner;
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
			DCO_ShotCounter.Track(owner);
	}

	protected bool RefreshAccuracyParams()
	{
		if (!m_Utility || !m_SelectedTarget || !ownerEntity)
			return false;

		float speed = DCO_AIConfigComponent.AimSpeedOf(m_Utility);
		if (speed <= 0)
			return false;

		float dist = vector.Distance(ownerEntity.GetOrigin(), DCO_AimTargetPos());
		float t;
		if (DCO_Stress.T(m_Utility, "rt.fast", 1.0) > 0)
		{
			float tc = DCO_Stress.T(m_Utility, "rt.settle_close_s", 0.35);
			if (dist < 25.0)
				t = tc;
			else if (dist < 50.0)
				t = Math.Lerp(tc, 0.9, (dist - 25.0) / 25.0);
			else if (dist < 100.0)
				t = Math.Lerp(0.9, 1.5, (dist - 50.0) / 50.0);
			else
				t = Math.Min(1.5 + (dist - 100.0) / 100.0, 5.0);
		}
		else if (dist < 25.0)
			t = 0.8;
		else if (dist < 100.0)
			t = Math.Lerp(0.8, 1.5, (dist - 25.0) / 75.0);
		else
			t = Math.Min(1.5 + (dist - 100.0) / 100.0, 5.0);

		float skillMul = DCO_AimUtility.SettleMul(DCO_AIConfigComponent.SkillOf(m_Utility));
		t *= skillMul;
		bool supported = m_fDCORestMul > 1.0;
		if (supported)
			t *= 0.6;
		if (dist < 50.0 && DCO_AimUtility.GetSightMagnification(this) >= 3.0)
			t *= 1.2;
		t *= DCO_Opp.SettleMul(m_Utility, m_SelectedTarget);
		m_fDurationN = t * 1000.0 / speed;

		if (m_bDCOSettleLog)
		{
			m_bDCOSettleLog = false;
			if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(ownerEntity), string.Format("aim_settle target=%1 dist=%2 t_settle_s=%3 skill_mul=%4 supported=%5 speed=%6",
					DCO_Log.IdUnit(m_SelectedTarget.GetTargetEntity()), Math.Round(dist), (m_fDurationN / 1000.0).ToString(-1, 2), skillMul, supported, speed));
		}
		return true;
	}

	protected float m_fDCORestCheck_ms;
	protected float m_fDCOStill_ms = -1;
	protected float m_fDCORestMul = 1.0;

	float DCO_GetRestMul() { return m_fDCORestMul; }

	protected void DCO_UpdateRest()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fDCORestCheck_ms)
			return;
		m_fDCORestCheck_ms = now + 1000.0;
		m_fDCORestMul = 1.0;
		ChimeraCharacter ch = ChimeraCharacter.Cast(ownerEntity);
		if (!ch || ch.IsInVehicle() || !m_CharacterController || !DCO_GlobalAIComponent.BehaviorOnFor(GetOwner(), DCO_EBehavior.AWARENESS))
			return;

		BaseWeaponComponent w;
		BaseWeaponManagerComponent wm = m_CharacterController.GetWeaponManagerComponent();
		if (wm)
			w = wm.GetCurrentWeapon();
		float speed = 0;
		if (ch.GetPhysics())
			speed = ch.GetPhysics().GetVelocity().Length();
		if (speed > 0.3)
		{
			m_fDCOStill_ms = -1;
			if (w && w.HasBipod() && w.GetBipod())
				w.SetBipod(false);
			return;
		}
		if (m_fDCOStill_ms < 0)
			m_fDCOStill_ms = now;
		if (!m_SelectedTarget || !m_SelectedTarget.GetTargetEntity())
			return;

		if (w && w.HasBipod())
		{
			if (!w.GetBipod() && now - m_fDCOStill_ms > 3000.0
				&& vector.Distance(ch.GetOrigin(), m_SelectedTarget.GetLastSeenPosition()) > 50.0)
			{
				w.SetBipod(true);
				DCO_Perf.Count("bipod_deploy");
			}
			if (w.GetBipod())
			{
				m_fDCORestMul = 1.8;
				return;
			}
		}

		if (m_DCOWindowBuilding || DCO_IsBraced(ch))
		{
			m_fDCORestMul = 1.4;
			DCO_Perf.Count("weapon_rest");
		}
	}

	protected bool DCO_IsBraced(ChimeraCharacter ch)
	{
		if (m_CharacterController.GetStance() == ECharacterStance.PRONE)
			return false;
		vector fwd = m_SelectedTarget.GetLastSeenPosition() - ch.GetOrigin();
		fwd[1] = 0;
		if (fwd.LengthSq() < 0.01)
			return false;
		fwd.Normalize();
		vector eye = ch.EyePosition();
		vector chest = eye - Vector(0, 0.35, 0);
		if (DCO_Util.TraceFrac(chest, chest + fwd * 0.9, ch, -1, -1, "t:DCO_Rest") >= 0.99)
			return false;
		return DCO_Util.TraceFrac(eye + Vector(0, 0.05, 0), eye + Vector(0, 0.05, 0) + fwd * 2.0, ch, -1, -1, "t:DCO_Rest") >= 0.99;
	}

	void UpdateAccuracy(float timeSlice)
	{
		if (!ownerEntity || !RefreshAccuracyParams())
			return;

		Physics phys = ownerEntity.GetPhysics();
		float currentSpeed = 0.0;
		if (phys)
			currentSpeed = phys.GetVelocity().Length();

		if (currentSpeed > 0.5 && DCO_IsCloseFight())
		{
			DCO_Perf.Count("reflex:aim_keep_moving");
			m_fTimeElapsed = Math.Min(m_fTimeElapsed + timeSlice, m_fDurationN);
		}
		else if (currentSpeed > 0.5)
		{
			bool far = true;
			if (DCO_Stress.T(m_Utility, "rt.fast", 1.0) > 0 && m_vDCOAimFrom != vector.Zero)
			{
				vector nowDir = DCO_AimDir();
				bool turned = nowDir != vector.Zero && m_vDCOAimRef != vector.Zero && vector.Dot(nowDir, m_vDCOAimRef) < 0.866;
				far = turned || vector.Distance(ownerEntity.GetOrigin(), m_vDCOAimFrom) > 3.0;
				if (!far)
					DCO_Perf.Count("rt:aim_keep_small_move");
			}
			if (far)
			{
				if (m_fTimeElapsed > 0)
				{
					float before = m_fTimeElapsed;
					m_fTimeElapsed = 0;
					DCO_LogAimReset("moved", before);
				}
				m_fDCOAimHold_ms = 0;
				m_vDCOAimFrom = ownerEntity.GetOrigin();
				m_vDCOAimRef = DCO_AimDir();
			}
		}
		else
		{
			m_fTimeElapsed = Math.Min(m_fTimeElapsed + timeSlice, m_fDurationN);
			m_fDCOAimHold_ms += timeSlice;
		}

		m_vDCOAimDir = DCO_AimDir();
		CURRENT_AIM_IMPROVEMENT = DCO_RampValue();

		if (m_bDCOFirstShotPending)
			DCO_CheckFirstShot();
		else if (m_bDCOFirstHitPending)
			DCO_CheckFirstHit();
	}

	protected void DCO_CheckFirstHit()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fDCOAimStart_ms > 30000.0 || !DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
		{
			m_bDCOFirstHitPending = false;
			return;
		}
		float hit = DCO_Battle.LastHitAt(ownerEntity);
		if (hit < m_fDCOAimStart_ms)
			return;
		m_bDCOFirstHitPending = false;
		float shotT = -1;
		if (m_fDCOFirstShotAt_ms > 0)
			shotT = (m_fDCOFirstShotAt_ms - m_fDCOAimStart_ms) / 1000.0;
		string state = "-";
		if (m_Utility && m_Utility.m_ThreatSystem)
			state = typename.EnumToString(EAIThreatState, m_Utility.m_ThreatSystem.GetState());
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(ownerEntity), string.Format("aim_first_hit dist=%1 first_shot_s=%2 first_hit_s=%3 threat=%4", Math.Round(vector.Distance(ownerEntity.GetOrigin(), DCO_AimTargetPos())), shotT.ToString(-1, 2), ((hit - m_fDCOAimStart_ms) / 1000.0).ToString(-1, 2), state));
	}

	protected void DCO_CheckFirstShot()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fDCOAimStart_ms > 30000.0 || !DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
		{
			m_bDCOFirstShotPending = false;
			return;
		}
		float shot = DCO_ShotCounter.LastShotAt(ownerEntity);
		if (shot < m_fDCOAimStart_ms)
			return;
		m_bDCOFirstShotPending = false;
		m_fDCOFirstShotAt_ms = shot;
		s_iDCOFirstShotSample++;
		if (s_iDCOFirstShotSample % 10 != 0 && !DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
			return;
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(ownerEntity), string.Format("aim_first_shot skill=%1 dist=%2 t_s=%3",
			typename.EnumToString(DCO_AISKILL, DCO_AIConfigComponent.SkillOf(m_Utility)), Math.Round(vector.Distance(ownerEntity.GetOrigin(), DCO_AimTargetPos())), ((shot - m_fDCOAimStart_ms) / 1000.0).ToString(-1, 2)));
	}

	SCR_CharacterControllerComponent GetCharacterController()
	{
		return m_CharacterController;
	}

	protected ref array<float> m_aDCOPinT = {};
	protected ref array<float> m_aDCOPinW = {};
	protected float m_fDCOPinStart_ms = -1;
	protected float m_fDCOPinCrawlNext_ms;
	protected float m_fDCOPinLast_ms = -1;
	protected float m_fDCOPinHold_ms = 3000;
	protected bool m_bDCOPinLong;
	protected float m_fDCORecoverUntil_ms = -1;
	protected float m_fDCOPinBurstEnd_ms = -1;
	protected float m_fDCOPinNextBurst_ms = -1;
	vector m_vDCOPinFrom;
	protected float m_fDCOS1Until_ms = -1;
	protected int m_iDCOStressLvl;
	protected float m_fDCOStressTick_ms;
	protected int m_iDCOPanicPhase;
	protected float m_fDCOPanicFreezeUntil_ms = -1;
	protected float m_fDCOPanicNoFireUntil_ms = -1;
	protected float m_fDCOPanicStart_ms = -1;
	protected bool m_bDCOBreak;
	protected vector m_vDCOBreakFrom;
	protected float m_fDCOBreakStart_ms = -1;
	protected float m_fDCOBreakLosSince_ms = -1;
	protected bool m_bDCOBreakRally;
	protected int m_iDCOBreakLegs;

	int DCO_StressLevel()
	{
		int lvl = 0;
		float now = GetGame().GetWorld().GetWorldTime();
		if (DCO_IsPinned())
			lvl = 2;
		else if (now < m_fDCOS1Until_ms)
			lvl = 1;
		else if (m_Utility && m_Utility.m_ThreatSystem && m_Utility.m_ThreatSystem.GetSuppressionMeasure() >= DCO_Stress.TA(m_Utility, "tm.supp_s1", 0.6, 0.3, 0.1, 0.9))
			lvl = 1;
		if (lvl != m_iDCOStressLvl)
		{
			DCO_Perf.Count("tm:s_to_" + lvl.ToString());
			if (m_Utility && DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
			{
				string cover = "none";
				if (m_Utility.m_OwnerEntity && SCR_CoverManagerComponent.IsEntityInsideBuilding(m_Utility.m_OwnerEntity))
					cover = "building";
				else if (m_Utility.m_CombatMoveState && m_Utility.m_CombatMoveState.IsInValidCover())
					cover = "object";
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("stress_level unit=%1 from=%2 to=%3 near_miss_3s=%4 cover=%5",
					DCO_Log.IdUnit(m_Utility.m_OwnerEntity), DCO_Stress.LevelName(m_iDCOStressLvl), DCO_Stress.LevelName(lvl), DCO_NearMissCount(now, 3000.0), cover));
			}
			if (m_Utility)
				DCO_Events.Stress(m_Utility.m_OwnerEntity, m_iDCOStressLvl, lvl);
			m_iDCOStressLvl = lvl;
		}
		return lvl;
	}

	bool DCO_StressNoFire(float targetDist)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fDCOPanicFreezeUntil_ms)
			return true;
		if (now < m_fDCOPanicNoFireUntil_ms && targetDist > 15.0)
			return true;
		if (m_bDCOBreak && targetDist > 30.0)
			return true;
		return false;
	}

	bool DCO_StressFrozen()
	{
		return GetGame().GetWorld().GetWorldTime() < m_fDCOPanicFreezeUntil_ms;
	}

	void DCO_OnMoraleState(moraleState prev, moraleState next)
	{
		if (!m_Utility || !m_Utility.m_OwnerEntity || !Replication.IsServer())
			return;
		if (m_Utility.m_AIInfo && m_Utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (next == moraleState.PANIC && prev < moraleState.PANIC)
		{
			m_iDCOPanicPhase = 1;
			m_fDCOPanicStart_ms = now;
			m_fDCOPanicFreezeUntil_ms = now + Math.RandomFloat(DCO_Stress.T(m_Utility, "tm.panic_freeze_min_s", 1.0), DCO_Stress.T(m_Utility, "tm.panic_freeze_max_s", 3.0)) * 1000.0;
			DCO_EngageInfo pei = DCO_Engage.Info(m_Utility);
			if (pei && pei.m_fDist >= 0 && pei.m_fDist < DCO_Stress.T(m_Utility, "tm.panic_close_m", 15.0))
			{
				m_fDCOPanicFreezeUntil_ms = now;
				DCO_Perf.Count("tm:panic_no_freeze_close");
			}
			DCO_Perf.Count("tm:panic_start");
		}
		if (next == moraleState.BREAK && !m_bDCOBreak)
		{
			m_bDCOBreak = true;
			m_vDCOBreakFrom = m_Utility.m_OwnerEntity.GetOrigin();
			m_fDCOBreakStart_ms = now;
			m_fDCOBreakLosSince_ms = -1;
			m_iDCOBreakLegs = 0;
			m_bDCOBreakRally = false;
			m_iDCOPanicPhase = 0;
			m_fDCOPanicFreezeUntil_ms = -1;
			DCO_Perf.Count("tm:break_start");
		}
		if (next < moraleState.BREAK && m_bDCOBreak)
			DCO_EndBreak(now, "morale_recovered");
	}

	protected void DCO_UpdateStress()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fDCOStressTick_ms || !m_Utility || !m_Utility.m_OwnerEntity)
			return;
		m_fDCOStressTick_ms = now + 250.0;
		DCO_StressLevel();
		DCO_TrackCoverReach(now);
		if (m_iDCOPanicPhase == 1 && now >= m_fDCOPanicFreezeUntil_ms)
			DCO_PanicFlight(now);
		if (m_bDCOBreak)
			DCO_BreakStep(now);
	}

	protected float m_fDCOFireStart_ms = -1;
	protected float m_fDCOFireCalm_ms = -1;
	protected float m_fDCOOpenUp_s;
	protected float m_fDCOOpenProne_s;

	protected void DCO_TrackCoverReach(float now)
	{
		bool inCover = m_Utility.m_CombatMoveState && m_Utility.m_CombatMoveState.IsInValidCover();
		int lvl = m_iDCOStressLvl;
		if (m_fDCOFireStart_ms < 0 && (lvl < 1 || inCover))
			return;
		DCO_EngageInfo ei = DCO_Engage.Info(m_Utility);
		if (ei && ei.m_iZone == 2)
			inCover = true;
		if (m_fDCOFireStart_ms < 0)
		{
			if (inCover)
				return;
			m_fDCOFireStart_ms = now;
			m_fDCOFireCalm_ms = -1;
			m_fDCOOpenUp_s = 0;
			m_fDCOOpenProne_s = 0;
			return;
		}
		if (inCover)
		{
			DCO_CoverReachEnd(now, "cover", ei);
			return;
		}
		if (lvl == 0)
		{
			if (m_fDCOFireCalm_ms < 0)
				m_fDCOFireCalm_ms = now;
			else if (now - m_fDCOFireCalm_ms > 10000.0)
				DCO_CoverReachEnd(now, "calm", ei);
			return;
		}
		m_fDCOFireCalm_ms = -1;
		if (ei && ei.m_iZone == 0 && ei.m_fDist > 50.0)
		{
			if (m_CharacterController && m_CharacterController.GetStance() == ECharacterStance.PRONE)
				m_fDCOOpenProne_s += 0.25;
			else
				m_fDCOOpenUp_s += 0.25;
		}
	}

	protected void DCO_CoverReachEnd(float now, string result, DCO_EngageInfo ei)
	{
		float s = (now - m_fDCOFireStart_ms) / 1000.0;
		string zone = "-";
		float d = -1;
		if (ei)
		{
			zone = DCO_Engage.ZoneName(ei.m_iZone);
			d = ei.m_fDist;
		}
		string stance = "-";
		if (m_CharacterController)
			stance = typename.EnumToString(ECharacterStance, m_CharacterController.GetStance());
		DCO_Perf.Count("tm:cover_reach_" + result);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("cover_reach unit=%1 result=%2 s=%3 zone=%4 enemy_d=%5 stance=%6 open_up_s=%7 open_prone_s=%8",
			DCO_Log.IdUnit(m_Utility.m_OwnerEntity), result, s.ToString(-1, 1), zone, Math.Round(d), stance, m_fDCOOpenUp_s.ToString(-1, 1), m_fDCOOpenProne_s.ToString(-1, 1)));
		m_fDCOFireStart_ms = -1;
		m_fDCOFireCalm_ms = -1;
	}

	protected void DCO_PanicFlight(float now)
	{
		m_iDCOPanicPhase = 2;
		float freeze_s = (m_fDCOPanicFreezeUntil_ms - m_fDCOPanicStart_ms) / 1000.0;
		m_fDCOPanicNoFireUntil_ms = now + Math.RandomFloat(DCO_Stress.T(m_Utility, "tm.panic_nofire_min_s", 10.0), DCO_Stress.T(m_Utility, "tm.panic_nofire_max_s", 30.0)) * 1000.0;
		IEntity me = m_Utility.m_OwnerEntity;
		vector threat = DCO_Stress.ThreatPos(m_Utility);
		vector nearest = threat;
		float enemyD = -1;
		int known;
		DCO_EngageInfo ei = DCO_Engage.Info(m_Utility);
		if (ei)
		{
			enemyD = ei.m_fDist;
			known = ei.m_iKnown;
			if (known > 0)
			{
				threat = ei.m_vCentroid;
				nearest = ei.m_vNearest;
			}
		}
		string band = "far";
		if (enemyD < 0)
			band = "unknown";
		else if (enemyD < DCO_Stress.T(m_Utility, "tm.panic_close_m", 15.0))
			band = "close";
		else if (enemyD <= DCO_Stress.T(m_Utility, "tm.panic_mid_m", 50.0))
			band = "mid";
		string result = "cover";
		float dist = 0;
		bool away = threat != vector.Zero;
		if (band == "close")
		{
			result = "fight";
			m_fDCOPanicNoFireUntil_ms = now;
		}
		else if (m_Utility.m_CombatMoveState && m_Utility.m_CombatMoveState.IsInValidCover())
		{
			result = "stay_cover";
			if (m_CharacterController && !DCO_Nav.Busy(m_CharacterController, "panic_prone") && m_CharacterController.GetStance() != ECharacterStance.PRONE)
				DCO_Stance.Request(m_CharacterController, 3, "panic");
		}
		else if (threat == vector.Zero)
		{
			result = "prone";
			if (m_CharacterController && !DCO_Nav.Busy(m_CharacterController, "panic_prone") && m_CharacterController.GetStance() != ECharacterStance.PRONE)
				DCO_Stance.Request(m_CharacterController, 3, "panic");
		}
		else
		{
			bool mid = band == "mid";
			float coverMax = DCO_Stress.T(m_Utility, "tm.panic_flight_max_m", 30.0);
			if (mid)
			{
				coverMax = DCO_Stress.T(m_Utility, "tm.rush_cover_max_m", 15.0);
				m_fDCOPanicNoFireUntil_ms = now + Math.RandomFloat(DCO_Stress.T(m_Utility, "tm.panic_mid_nofire_min_s", 5.0), DCO_Stress.T(m_Utility, "tm.panic_mid_nofire_max_s", 10.0)) * 1000.0;
				if (m_CharacterController && !DCO_Nav.Busy(m_CharacterController, "panic_prone") && m_CharacterController.GetStance() != ECharacterStance.PRONE)
					DCO_Stance.Request(m_CharacterController, 3, "panic");
			}
			vector pos;
			DCO_RelocateBehavior b;
			bool found = DCO_Stress.FindCover(me, threat, coverMax, pos);
			if (found && nearest != vector.Zero && vector.DistanceXZ(pos, nearest) < vector.DistanceXZ(me.GetOrigin(), nearest) - 2.0)
			{
				found = false;
				DCO_Perf.Count("tm:panic_cover_toward_enemy");
			}
			if (found)
			{
				dist = vector.DistanceXZ(me.GetOrigin(), pos);
				if (mid)
				{
					result = "crawl_cover";
					b = DCO_RelocateBehavior.Start(m_Utility, pos, threat, true, ECharacterStance.PRONE, dist * 1.5 + 4.0, "stress_panic");
					if (b)
						b.DCO_SetMove(ECharacterStance.PRONE, EMovementType.WALK, 3.0, true);
				}
				else
				{
					b = DCO_RelocateBehavior.Start(m_Utility, pos, threat, true, ECharacterStance.PRONE, dist / 4.0 + 4.0, "stress_panic");
					if (b)
						b.DCO_SetMove(ECharacterStance.STAND, EMovementType.SPRINT, 3.0, true);
				}
			}
			else
			{
				result = "crawl";
				dist = Math.RandomFloat(4.0, 8.0);
				pos = DCO_Stress.AwayPoint(me, threat, dist, 40.0);
				b = DCO_RelocateBehavior.Start(m_Utility, pos, threat, true, ECharacterStance.PRONE, dist * 1.5 + 4.0, "stress_panic");
				if (b)
					b.DCO_SetMove(ECharacterStance.PRONE, EMovementType.WALK, 8.0, true);
			}
			if (!b)
				result = "no_relocate";
		}
		DCO_Perf.Count("tm:panic_flight_" + result);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("panic_flight unit=%1 freeze_s=%2 dist=%3 away=%4 no_fire_s=%5 result=%6 band=%7 enemy_d=%8 known=%9",
			DCO_Log.IdUnit(me), freeze_s.ToString(-1, 1), Math.Round(dist), DCO_Util.B01(away), Math.Round((m_fDCOPanicNoFireUntil_ms - now) / 1000.0), result, band, Math.Round(enemyD), known));
	}

	protected void DCO_BreakStep(float now)
	{
		IEntity me = m_Utility.m_OwnerEntity;
		float moved = vector.DistanceXZ(me.GetOrigin(), m_vDCOBreakFrom);
		BaseTarget t = GetCurrentTarget();
		bool losBroken = !t || t.GetTimeSinceSeen() > 3.0;
		if (losBroken)
		{
			if (m_fDCOBreakLosSince_ms < 0)
				m_fDCOBreakLosSince_ms = now;
		}
		else
			m_fDCOBreakLosSince_ms = -1;
		float losBroken_s = 0;
		if (m_fDCOBreakLosSince_ms >= 0)
			losBroken_s = (now - m_fDCOBreakLosSince_ms) / 1000.0;
		float minM = DCO_Stress.T(m_Utility, "tm.break_min_m", 100.0);
		float maxM = DCO_Stress.T(m_Utility, "tm.break_max_m", 300.0);
		if ((moved >= minM && losBroken_s >= 30.0) || moved >= maxM || now - m_fDCOBreakStart_ms > 240000.0)
		{
			string why = "max_dist";
			if (moved < maxM)
				why = "los_broken";
			if (now - m_fDCOBreakStart_ms > 240000.0)
				why = "timeout";
			DCO_EndBreak(now, why);
			return;
		}
		DCO_RelocateBehavior cur = DCO_RelocateBehavior.Cast(m_Utility.FindActionOfType(DCO_RelocateBehavior));
		if (cur && cur.DCO_Why() == "stress_break" && !cur.DCO_IsDone())
			return;
		if (moved >= minM && losBroken)
			return;
		vector threat = DCO_Stress.ThreatPos(m_Utility);
		vector pos;
		bool rally = false;
		AIAgent ag = m_Utility.GetAIAgent();
		IEntity leader;
		if (ag && ag.GetParentGroup())
			leader = ag.GetParentGroup().GetLeaderEntity();
		if (threat != vector.Zero && leader && leader != me && DCO_Util.IsAlive(leader)
			&& vector.DistanceXZ(leader.GetOrigin(), threat) > vector.DistanceXZ(me.GetOrigin(), threat) + 20.0 && vector.DistanceXZ(leader.GetOrigin(), me.GetOrigin()) > 10.0)
		{
			vector toL = leader.GetOrigin() - me.GetOrigin();
			toL[1] = 0;
			float leg = Math.Min(toL.Length(), Math.RandomFloat(30.0, 40.0));
			pos = DCO_Util.Surface(me.GetOrigin() + toL.Normalized() * leg);
			rally = true;
		}
		else if (threat != vector.Zero)
			pos = DCO_Stress.AwayPoint(me, threat, Math.RandomFloat(30.0, 40.0), 25.0);
		else
		{
			DCO_EndBreak(now, "no_threat");
			return;
		}
		m_bDCOBreakRally = m_bDCOBreakRally || rally;
		m_iDCOBreakLegs++;
		DCO_RelocateBehavior b = DCO_RelocateBehavior.Start(m_Utility, pos, threat, true, ECharacterStance.CROUCH, 15.0, "stress_break");
		if (b)
			b.DCO_SetMove(ECharacterStance.STAND, EMovementType.SPRINT, 10.0, true);
		DCO_Perf.Count("tm:break_leg");
	}

	protected void DCO_EndBreak(float now, string why)
	{
		if (!m_bDCOBreak)
			return;
		m_bDCOBreak = false;
		IEntity me = m_Utility.m_OwnerEntity;
		float los_s = 0;
		if (m_fDCOBreakLosSince_ms >= 0)
			los_s = (now - m_fDCOBreakLosSince_ms) / 1000.0;
		DCO_Perf.Count("tm:break_end_" + why);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("break_withdraw unit=%1 dist=%2 legs=%3 rally=%4 los_break_s=%5 dur_s=%6 end=%7",
			DCO_Log.IdUnit(me), Math.Round(vector.DistanceXZ(me.GetOrigin(), m_vDCOBreakFrom)), m_iDCOBreakLegs, DCO_Util.B01(m_bDCOBreakRally), Math.Round(los_s), Math.Round((now - m_fDCOBreakStart_ms) / 1000.0), why));
	}

	void DCO_PinNearMiss(float w, vector shooterPos)
	{
		if (w <= 0 || !m_Utility || !m_Utility.m_OwnerEntity)
			return;
		if (w >= 0.8)
			m_fDCOS1Until_ms = GetGame().GetWorld().GetWorldTime() + DCO_Stress.T(m_Utility, "tm.supp_hold_s", 3.0) * 1000.0 + Math.RandomFloat(3000.0, 6000.0);
		if (!DCO_GlobalAIComponent.BehaviorOnFor(GetOwner(), DCO_EBehavior.PINNED))
			return;
		if (DCO_AssaultLock.CheckBlock(m_Utility, "pinned"))
			return;
		if (m_Utility.m_AIInfo && m_Utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
			return;
		float now = GetGame().GetWorld().GetWorldTime();
		if (shooterPos != vector.Zero)
			m_vDCOPinFrom = shooterPos;
		if (DCO_IsPinned())
		{
			m_fDCOPinLast_ms = now;
			DCO_PinCrawl(now);
			return;
		}

		m_aDCOPinT.Insert(now);
		m_aDCOPinW.Insert(w);
		float sum;
		for (int i = m_aDCOPinT.Count() - 1; i >= 0; i--)
		{
			if (now - m_aDCOPinT[i] > 3000.0)
			{
				m_aDCOPinT.Remove(i);
				m_aDCOPinW.Remove(i);
				continue;
			}
			sum += m_aDCOPinW[i];
		}

		float thr = DCO_Stress.T(m_Utility, "tm.supp_s2_open", 2.0);
		switch (DCO_AIConfigComponent.PersonalityOf(m_Utility))
		{
			case DCO_EAIPersonality.CAUTIOUS: thr *= 0.7; break;
			case DCO_EAIPersonality.AGGRESSIVE: thr *= 1.2; break;
			case DCO_EAIPersonality.RECKLESS: thr *= 1.4; break;
		}
		if (m_Utility.m_CombatMoveState && m_Utility.m_CombatMoveState.IsInValidCover())
			thr *= DCO_Stress.T(m_Utility, "tm.supp_s2_cover", 3.0) / Math.Max(DCO_Stress.T(m_Utility, "tm.supp_s2_open", 2.0), 0.1);
		if (DCO_PostureCombatUtility.GetPosture(m_Utility) == DCO_GroupTactics.AGGRESIVE)
			thr *= 1.5;
		thr *= Math.Clamp(DCO_Stress.Aggr(m_Utility), 0.5, 2.0);
		if (sum < thr)
			return;

		m_aDCOPinT.Clear();
		m_aDCOPinW.Clear();
		m_fDCOPinStart_ms = now;
		m_fDCOPinLast_ms = now;
		m_fDCORecoverUntil_ms = -1;
		m_fDCOPinNextBurst_ms = now + Math.RandomFloat(4000.0, 6000.0);
		m_fDCOPinHold_ms = 3000.0;
		m_bDCOPinLong = false;
		DCO_AIMoraleSystem ms = m_Utility.GetMoraleSystem();
		if (ms && ms.GetState() == moraleState.SHAKEN)
		{
			m_fDCOPinHold_ms = 4500.0;
			m_bDCOPinLong = true;
		}
		float pinA = DCO_Stress.AggrRel(m_Utility);
		if (pinA != 1.0)
		{
			m_fDCOPinHold_ms = m_fDCOPinHold_ms * Math.Clamp(Math.Pow(pinA, -0.5), 0.6, 1.4);
			DCO_Perf.Count("agg:p7_pin");
		}
		DCO_Perf.Count("pin_start");
		DCO_PinStance();
		DCO_Opp.UnderFireCallout(m_Utility, now);

		if (ms && ms.GetState() == moraleState.PANIC && m_vDCOPinFrom != vector.Zero)
		{
			vector me = m_Utility.m_OwnerEntity.GetOrigin();
			vector away = DCO_Specialist.Flat(me - m_vDCOPinFrom);
			DCO_RelocateBehavior.Start(m_Utility, DCO_Util.Surface(me + away * 4.5), m_vDCOPinFrom, true, ECharacterStance.PRONE, 8.0, "pin_crawl");
			DCO_Perf.Count("pin_panic_crawl");
		}
	}

	protected void DCO_PinCrawl(float now)
	{
		if (now < m_fDCOPinCrawlNext_ms || m_vDCOPinFrom == vector.Zero || !m_Utility || !m_Utility.m_CombatMoveState || !m_Utility.m_OwnerEntity)
			return;
		if (m_Utility.m_CombatMoveState.IsInValidCover() || m_Utility.m_CombatMoveState.IsExecutingRequest())
			return;
		if (m_Utility.m_AIInfo && m_Utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
			return;
		SCR_CharacterDamageManagerComponent dm = SCR_CharacterDamageManagerComponent.Cast(m_Utility.m_OwnerEntity.FindComponent(SCR_CharacterDamageManagerComponent));
		bool hit = dm && dm.DCO_HitsWithin(3.0) > 0;
		if (!hit && now - m_fDCOPinStart_ms < 1500.0)
			return;
		m_fDCOPinCrawlNext_ms = now + 8000.0;
		vector me = m_Utility.m_OwnerEntity.GetOrigin();
		vector away = DCO_Specialist.Flat(me - m_vDCOPinFrom);
		DCO_RelocateBehavior.Start(m_Utility, DCO_Util.Surface(me + away * 4.5), m_vDCOPinFrom, true, ECharacterStance.PRONE, 8.0, "pin_crawl");
		string why = "long_pinned";
		if (hit)
			why = "hit";
		DCO_Perf.Count("pin_crawl_cover");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("pin_crawl_cover unit=%1 why=%2 shooter_dist=%3 pinned_s=%4", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), why, Math.Round(vector.Distance(me, m_vDCOPinFrom)), Math.Round((now - m_fDCOPinStart_ms) / 1000.0)));
	}

	protected void DCO_PinStance()
	{
		if (!m_CharacterController || DCO_Nav.Busy(m_CharacterController, "pin_stance"))
			return;
		float d = 100.0;
		if (m_vDCOPinFrom != vector.Zero)
			d = vector.Distance(m_vDCOPinFrom, m_Utility.m_OwnerEntity.GetOrigin());
		if (DCO_CQC.IsProneAllowed(m_Utility.m_OwnerEntity, d))
		{
			if (m_CharacterController.GetStance() != ECharacterStance.PRONE)
				DCO_Stance.Request(m_CharacterController, 3, "pin");
		}
		else if (m_CharacterController.GetStance() == ECharacterStance.STAND)
			DCO_Stance.Request(m_CharacterController, 2, "pin");
	}

	protected float m_fDCOProneRollUntil_ms;
	protected bool m_bDCOProneRoll;

	bool DCO_ProneRoll(float chance)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now >= m_fDCOProneRollUntil_ms)
		{
			m_bDCOProneRoll = Math.RandomFloat01() < chance;
			m_fDCOProneRollUntil_ms = now + 10000.0;
		}
		return m_bDCOProneRoll;
	}

	protected float m_fDCOCQBProneRollUntil_ms;
	protected bool m_bDCOCQBProneRoll;

	bool DCO_CQBProneRoll(float chance)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now >= m_fDCOCQBProneRollUntil_ms)
		{
			m_bDCOCQBProneRoll = Math.RandomFloat01() < chance;
			m_fDCOCQBProneRollUntil_ms = now + 10000.0;
		}
		return m_bDCOCQBProneRoll;
	}

	bool DCO_IsPinned()
	{
		if (m_fDCOPinStart_ms < 0)
			return false;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fDCOPinLast_ms <= m_fDCOPinHold_ms)
			return true;
		DCO_Perf.Count("pin_break");
		DCO_Perf.CountN("pin_hold", Math.Round((now - m_fDCOPinStart_ms) / 1000.0));
		m_fDCOPinStart_ms = -1;
		float rec = Math.RandomFloat(3000.0, 6000.0);
		if (m_bDCOPinLong)
			rec *= 1.5;
		bool lead = DCO_Stress.LeaderNear(m_Utility, 20.0);
		bool iso = DCO_Stress.Isolated(m_Utility, 30.0, false);
		if (lead)
			rec *= 0.6;
		if (iso)
			rec *= 1.5;
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("pin_recover unit=%1 wait_s=%2 leader=%3 isolated=%4", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), (rec / 1000.0).ToString(-1, 1), DCO_Util.B01(lead), DCO_Util.B01(iso)));
		m_fDCORecoverUntil_ms = now + rec;
		if (m_Utility && m_Utility.m_LookAction && m_vDCOPinFrom != vector.Zero)
			m_Utility.m_LookAction.LookAt(m_vDCOPinFrom + Vector(0, 1.5, 0), 30.0, rec / 1000.0);
		return false;
	}

	bool DCO_IsPinRecovering()
	{
		if (m_fDCORecoverUntil_ms < 0)
			return false;
		if (GetGame().GetWorld().GetWorldTime() < m_fDCORecoverUntil_ms)
			return true;
		m_fDCORecoverUntil_ms = -1;
		DCO_Perf.Count("pin_recover");
		return false;
	}

	bool DCO_PinFireAllowed()
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fDCOPinBurstEnd_ms)
			return true;
		if (now < m_fDCOPinNextBurst_ms)
			return false;
		m_fDCOPinBurstEnd_ms = now + 700.0;
		m_fDCOPinNextBurst_ms = now + Math.RandomFloat(4000.0, 6000.0);
		DCO_Perf.Count("pin_burst");
		return true;
	}
}