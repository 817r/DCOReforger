modded class SCR_AIUpdateTargetAttackData : AITaskScripted
{
	protected const int FIRE_TREE_INVALID 		= -1;
	protected const int FIRE_TREE_LOOK			= 0;
	protected const int FIRE_TREE_BURST			= 1;
	protected const int FIRE_TREE_SINGLE		= 2;
	protected const int FIRE_TREE_SUPPRESSIVE	= 3;
	protected const int FIRE_TREE_MELEE			= 4;
	protected const int FIRE_TREE_LOOK_THREATS	= 5;

	protected const float BURST_FIRE_MAX_DISTANCE = 70.0;
	protected float m_fDCOLaneAt_ms = -100000;
	protected int m_iDCOLane;
	protected float m_fDCOGateLogAt_ms = -100000;

	override protected void OnThreatSectorEscalation(SCR_AISectorThreatFilter ts, int sectorId, float dangerValue)
	{
		if (!m_Target)
		{
			DCO_Perf.Count("attack_data:escalation_no_target");
			return;
		}
		super.OnThreatSectorEscalation(ts, sectorId, dangerValue);
	}

	override protected void OnThreatSectorDamageTaken(SCR_AISectorThreatFilter ts, int sectorId)
	{
		if (!m_Target)
		{
			DCO_Perf.Count("attack_data:damage_no_target");
			return;
		}
		super.OnThreatSectorDamageTaken(ts, sectorId);
	}

	protected bool DCO_LaneBlocked(vector target, float width)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now - m_fDCOLaneAt_ms < 250.0)
			return m_iDCOLane != 0;
		m_fDCOLaneAt_ms = now;
		IEntity blocker;
		m_iDCOLane = DCO_Util.FireLaneCheck(m_UtilityComponent.m_OwnerEntity, target, width, blocker);
		if (m_iDCOLane != 0)
			DCO_Util.LaneEvent(m_UtilityComponent.m_OwnerEntity, target, m_iDCOLane, blocker, "cease");
		return m_iDCOLane != 0;
	}


	override int ResolveFireTree(BaseTarget target, bool visible, bool weaponReady, out float fireRate)
	{
		SCR_AIBehaviorBase executedBehavior = SCR_AIBehaviorBase.Cast(m_UtilityComponent.GetExecutedAction());
		if (executedBehavior && executedBehavior.m_bUseCombatMove && !m_UtilityComponent.m_CombatMoveState.m_bAimAtTarget)
		{
			if (!visible || !target || target.GetDistance() >= DCO_Stress.T(m_UtilityComponent, "rt.close_m", 30.0))
			{
				if (visible && target && target.GetDistance() < 50.0)
				{
					DCO_Perf.Count("atk:fire_block_aim_off");
					float gNow = GetGame().GetWorld().GetWorldTime();
					if (gNow - m_fDCOGateLogAt_ms > 2000.0 && DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
					{
						m_fDCOGateLogAt_ms = gNow;
						string gRq = "-";
						SCR_AICombatMoveRequest_Move gMv = SCR_AICombatMoveRequest_Move.Cast(m_UtilityComponent.m_CombatMoveState.GetRequest());
						if (gMv)
							gRq = typename.EnumToString(SCR_EAICombatMoveReason, gMv.m_eReason) + "/" + typename.EnumToString(EMovementType, gMv.m_eMovementType);
						DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_UtilityComponent.m_OwnerEntity), string.Format("fire_gate unit=%1 tgt=%2 dist=%3 reason=aim_off rq=%4 beh=%5", DCO_Log.IdUnit(m_UtilityComponent.m_OwnerEntity), DCO_Log.IdUnit(target.GetTargetEntity()), Math.Round(target.GetDistance()), gRq, executedBehavior.ClassName()));
					}
				}
				return FIRE_TREE_INVALID;
			}
			if (target.GetDistance() >= 15.0)
				DCO_Perf.Count("atk:fire_move_near");
		}

		if (m_bLookAtThreats)
			return FIRE_TREE_LOOK_THREATS;

		if (m_CombatComponent.DCO_ReactionBlocked())
			return FIRE_TREE_LOOK;

		BaseTarget dcoCloseT = m_CombatComponent.DCO_GetCloseTarget();
		bool dcoClose = target && dcoCloseT && target.GetTargetEntity() == dcoCloseT.GetTargetEntity();
		if (m_CombatComponent.DCO_CEHold() && !dcoClose)
			return FIRE_TREE_LOOK;

		if (dcoClose && weaponReady && visible && (m_CombatComponent.DCO_IsPinned() || m_CombatComponent.DCO_IsPinRecovering() || m_CombatComponent.DCO_CEHold()))
		{
			DCO_Perf.Count("reflex:fire_override");
			if (m_PerceptionComponent.GetFriendlyInLineOfFire())
				return FIRE_TREE_LOOK;
			if (m_bWeaponHasBurstOrAuto)
				return FIRE_TREE_BURST;
			return FIRE_TREE_SINGLE;
		}

		if (target && m_CombatComponent.DCO_StressNoFire(target.GetDistance()))
		{
			DCO_Perf.Count("tm:no_fire");
			return FIRE_TREE_LOOK;
		}

		if (m_CombatComponent.DCO_IsPinned())
		{
			if (weaponReady && m_CombatComponent.DCO_PinFireAllowed())
			{
				if (target && (m_PerceptionComponent.GetFriendlyInLineOfFire() || DCO_LaneBlocked(target.GetLastSeenPosition(), 1.5)))
				{
					DCO_Perf.Count("pin_burst_lane_block");
					return FIRE_TREE_LOOK;
				}
				if (m_bWeaponHasBurstOrAuto)
					return FIRE_TREE_BURST;
				return FIRE_TREE_SINGLE;
			}
			return FIRE_TREE_LOOK;
		}
		if (m_CombatComponent.DCO_IsPinRecovering())
			return FIRE_TREE_LOOK;

		if (!weaponReady)
			return FIRE_TREE_LOOK;

		BaseWeaponComponent selectedWeaponComp;
		int selectedMuzzleId;
		m_CombatComponent.GetSelectedWeapon(selectedWeaponComp, selectedMuzzleId);

		bool directDamage;
		float weaponMinDist, weaponMaxDist;
		m_CombatComponent.GetSelectedWeaponProperties(weaponMinDist, weaponMaxDist, directDamage);

		if (!selectedWeaponComp)
			return FIRE_TREE_LOOK;

		if (selectedWeaponComp != m_DCOBurstWeapon || selectedMuzzleId != m_iDCOBurstMuzzle)
		{
			m_DCOBurstWeapon = selectedWeaponComp;
			m_iDCOBurstMuzzle = selectedMuzzleId;
			m_bWeaponHasBurstOrAuto = WeaponHasBurstOrAutoMode(selectedWeaponComp, selectedMuzzleId);
		}

		EWeaponType weaponType = selectedWeaponComp.GetWeaponType();

		if (m_CombatComponent.GetCombatMode() == EAIGroupCombatMode.HOLD_FIRE)
		{
			if (!IsCloseDirectThreat(target, visible) || (!ShouldReturnFireWhenEndangered() && !DCO_DisciplineBreak(target, visible)))
			{
				DCO_Perf.Count("hold_fire_look");
				return FIRE_TREE_LOOK;
			}
			DCO_Perf.Count("hold_fire_break");
		}

		float targetDistance = target.GetDistance();

		if (targetDistance < MELEE_MAX_DISTANCE &&
			!m_CharacterController.CanFire() &&
			m_CharacterController.GetStance() != ECharacterStance.PRONE)
		{
			return FIRE_TREE_MELEE;
		}

		if (m_PerceptionComponent.GetFriendlyInLineOfFire() || DCO_LaneBlocked(target.GetLastSeenPosition(), 1.5))
		{
			return FIRE_TREE_LOOK;
		}

		if (DCO_Opp.PatientHold(m_UtilityComponent, target, visible, weaponType, targetDistance))
			return FIRE_TREE_LOOK;

		float threat =Math.Min(m_UtilityComponent.m_ThreatSystem.GetThreatMeasure(), 1.0);
		float mgRaw = threat;
		if (weaponType == EWeaponType.WT_MACHINEGUN)
			threat = Math.Max(threat, DCO_Stress.T(m_UtilityComponent, "ctx.mg_floor", 1.0));

		bool longHold = targetDistance > DCO_LongRange.EffectiveRange(weaponType) && DCO_LongRange.ShouldHold(m_UtilityComponent);
		bool farRifle = longHold && weaponType != EWeaponType.WT_MACHINEGUN;
		if (longHold)
			DCO_LongRange.ApplyStance(m_UtilityComponent, visible);

		if (targetDistance < weaponMinDist || targetDistance > weaponMaxDist)
		{
			if (weaponType == EWeaponType.WT_MACHINEGUN)
			{
				if (DCO_AmmoUtility.ShouldAvoidSuppressiveFire(m_UtilityComponent, selectedWeaponComp))
					return FIRE_TREE_LOOK;

				float maxFireRate = Math.Max(1, Math.Map(targetDistance, 0, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE, 2, 1));
				fireRate = DCO_Air.CapRate(target, maxFireRate * threat, targetDistance, weaponType);
				DCO_MgFloorUsed(mgRaw, threat, targetDistance, visible, "oor");
				if (weaponType == EWeaponType.WT_MACHINEGUN)
					fireRate *= DCO_Specialist.RateMul(m_CombatComponent.DCO_GetSpec());

				fireRate *= DCO_AmmoUtility.GetAmmoConservationScale(m_UtilityComponent, selectedWeaponComp);

				return FIRE_TREE_SUPPRESSIVE;
			}
			return FIRE_TREE_LOOK;
		}

		DCO_CtxState ctx = DCO_Ctx.Get(m_UtilityComponent);
		bool ctxFrag = !ctx || DCO_Ctx.P(ctx, DCO_ECtxParam.FRAG) > 0;
		if (!ctxFrag)
			DCO_Perf.Count("ctx:frag_block");

		ChimeraCharacter atTc = ChimeraCharacter.Cast(target.GetTargetEntity());
		if (atTc && atTc.IsInVehicle() && target.GetTimeSinceSeen() < 5.0 && m_CombatComponent.HasWeaponOfType(EWeaponType.WT_ROCKETLAUNCHER)
			&& DCO_GlobalAIComponent.BehaviorOnUtility(m_UtilityComponent, DCO_EBehavior.AT_SPECIALIST)
			&& DCO_RpgUnseen.Allowed(m_UtilityComponent, atTc, target.GetLastSeenPosition()))
			return 7;

		if (ctx && visible && target.GetTargetEntity())
		{
			float reactNow = GetGame().GetWorld().GetWorldTime();
			if (target.GetTargetEntity() != m_DCOCtxReactTgt || m_fDCOCtxReactAt_ms < 0)
			{
				m_DCOCtxReactTgt = target.GetTargetEntity();
				m_fDCOCtxReactAt_ms = reactNow;
				m_fDCOCtxReactDelay_s = DCO_Ctx.ReactDelay_s(m_UtilityComponent, ctx);
				if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
					DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_UtilityComponent.m_OwnerEntity), string.Format("ctxreact unit=%1 delay_s=%2 dist=%3 ctx=%4",
						DCO_Log.IdUnit(m_UtilityComponent.m_OwnerEntity), m_fDCOCtxReactDelay_s.ToString(-1, 2), Math.Round(targetDistance), DCO_Ctx.Tag(ctx)));
			}
			if (reactNow - m_fDCOCtxReactAt_ms < m_fDCOCtxReactDelay_s * 1000.0)
			{
				DCO_Perf.Count("ctx:react_wait");
				return FIRE_TREE_LOOK;
			}
		}
		if (ctx && !visible && target.GetTimeSinceSeen() > 3.0)
			m_fDCOCtxReactAt_ms = -1;

		if (visible)
		{
			if (ctxFrag &&m_CombatComponent.HasWeaponOfType(EWeaponType.WT_FRAGGRENADE) && DCO_GrenadeUtility.TryCloseFrag(m_UtilityComponent, target, targetDistance, true))
				return FIRE_TREE_LOOK;

			if (targetDistance >= 50.0 && DCO_UGLUtility.IsPointTarget(target) && DCO_UGLUtility.ShouldUseGL(m_UtilityComponent, selectedWeaponComp, target.GetLastSeenPosition()))
			{
				DCO_Perf.Count("spec:gl_visible_point");
				return 8;
			}

			float effectiveBurstMaxDist = BURST_FIRE_MAX_DISTANCE * DCO_PersonalityCombatUtility.GetBurstDistanceScale(m_UtilityComponent);

			if (weaponType == EWeaponType.WT_MACHINEGUN)
			{
				if (targetDistance > DCO_LongRange.EffectiveRange(weaponType) && !DCO_Air.IsAirTarget(target))
				{
					fireRate = 0.5;
					return FIRE_TREE_SUPPRESSIVE;
				}
				return FIRE_TREE_BURST;
			}
			else if (farRifle)
			{
				if (!DCO_LongRange.IsShotWindow(m_UtilityComponent.m_OwnerEntity))
					return FIRE_TREE_LOOK;
				return FIRE_TREE_SINGLE;
			}
			else if (ctx && weaponType != EWeaponType.WT_SNIPERRIFLE)
			{
				int fm = DCO_Ctx.P(ctx, DCO_ECtxParam.FIRE);
				int tree = FIRE_TREE_SINGLE;
				string mode = "single";
				if (fm >= 3)
				{
					tree = FIRE_TREE_LOOK;
					mode = "hold";
				}
				else if (fm >= 1 && m_bWeaponHasBurstOrAuto)
				{
					tree = FIRE_TREE_BURST;
					mode = "burst";
				}
				DCO_Perf.Count("ctx:fire_" + mode);
				if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
					DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_UtilityComponent.m_OwnerEntity), string.Format("fire unit=%1 mode=%2 dist=%3 ctx=%4 src=profile",
						DCO_Log.IdUnit(m_UtilityComponent.m_OwnerEntity), mode, Math.Round(targetDistance), DCO_Ctx.Tag(ctx)));
				return tree;
			}
			else if (targetDistance < effectiveBurstMaxDist && m_bWeaponHasBurstOrAuto)
				return FIRE_TREE_BURST;
			else
				return FIRE_TREE_SINGLE;
		}
		else
		{
			float lastSeenThreshold;
			if (weaponType == EWeaponType.WT_MACHINEGUN)
				lastSeenThreshold = SCR_AICombatComponent.TARGET_MAX_LAST_SEEN_INDIRECT_ATTACK_MG;
			else
			{
				if (targetDistance < SCR_AICombatComponent.CLOSE_RANGE_COMBAT_DISTANCE)
					lastSeenThreshold = SCR_AICombatComponent.TARGET_MAX_LAST_SEEN_INDIRECT_ATTACK_CLOSE;
				else
					lastSeenThreshold = SCR_AICombatComponent.TARGET_MAX_LAST_SEEN_INDIRECT_ATTACK;
			}

			bool lkOn = DCO_GlobalAIComponent.BehaviorOnUtility(m_UtilityComponent, DCO_EBehavior.SUPPRESS_LAST_KNOWN);
			if (lkOn)
			{
				lastSeenThreshold = DCO_SuppressWindow(weaponType, targetDistance);
				DCO_TrackLastKnown(target, lastSeenThreshold);
			}
			else
			{
				lastSeenThreshold = Math.Max(SCR_AICombatComponent.TARGET_MIN_LAST_SEEN_INDIRECT_ATTACK * 2, lastSeenThreshold * threat * ThreatRememberFromPersonality());
				DCO_MgFloorUsed(mgRaw, threat, targetDistance, visible, "window");
			}
			if (!lkOn && weaponType == EWeaponType.WT_MACHINEGUN && DCO_GlobalAIComponent.BehaviorOnUtility(m_UtilityComponent, DCO_EBehavior.MG_GUNNER))
				lastSeenThreshold = Math.Max(lastSeenThreshold, 8.0);
			if (target.GetTimeSinceSeen() > lastSeenThreshold && target.GetTimeSinceSeen() < 30.0 && DCO_Stress.T(m_UtilityComponent, "tb.ow_sup", 1.0) > 0 && DCO_TeamBound.OverwatchFor(m_UtilityComponent))
			{
				lastSeenThreshold = Math.Max(lastSeenThreshold, 30.0);
				DCO_Perf.Count("tb:ow_suppress");
			}
			if ((weaponType == EWeaponType.WT_MACHINEGUN || weaponType == EWeaponType.WT_AUTOCANNON) && DCO_SquadMovingInVehicleSupport())
			{
				lastSeenThreshold = Math.Max(lastSeenThreshold, 20.0);
				if (target.GetTimeSinceSeen() > 8.0)
					DCO_Perf.Count("veh:cover_squad_move");
			}
			float ctxSupMul = 1.0;
			if (ctx && DCO_Stress.T(m_UtilityComponent, "ctx.sup_on", 1.0) > 0 && m_UtilityComponent.m_CombatMoveState && !m_UtilityComponent.m_CombatMoveState.IsExecutingRequest())
			{
				float ctxMate = DCO_Ctx.MateMoveAge_s(m_UtilityComponent);
				float ctxSet = DCO_Stress.T(m_UtilityComponent, "ctx.sup_set_s", 3.0);
				bool ctxAssault = ctx.m_iRel == DCO_ECtxRel.ASSAULT && ctx.m_iBand <= DCO_ERangeBand.MEDIUM;
				bool ctxStand = ctx.m_iKind == DCO_ECtxKind.HARD_STANDOFF && ctxMate <= ctxSet;
				if (ctxAssault || ctxStand)
				{
					int ctxMags = DCO_Ctx.SpareMags(m_UtilityComponent.m_OwnerEntity, selectedWeaponComp);
					if (ctxMags <= DCO_Stress.T(m_UtilityComponent, "ctx.sup_min_mags", 2.0) && weaponType != EWeaponType.WT_MACHINEGUN)
						DCO_Perf.Count("ctx:sup_ammo_cap");
					else
					{
						float ctxFloor = DCO_Stress.T(m_UtilityComponent, "ctx.sup_floor_s", 8.0);
						float ctxWin = ctxFloor;
						if (ctxMate <= ctxSet)
							ctxWin = Math.Clamp(target.GetTimeSinceSeen() + 1.0, ctxFloor, DCO_Stress.T(m_UtilityComponent, "ctx.sup_cap_s", 30.0));
						if (ctxWin > lastSeenThreshold)
						{
							lastSeenThreshold = ctxWin;
							DCO_Perf.Count("ctx:sup_window");
							if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
								DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_UtilityComponent.m_OwnerEntity), string.Format("sup_window unit=%1 kind=%2 rel=%3 mate_move_age_s=%4 window_s=%5 ammo_mags=%6", DCO_Log.IdUnit(m_UtilityComponent.m_OwnerEntity), DCO_Ctx.KindName(ctx.m_iKind), DCO_Ctx.RelName(ctx.m_iRel), ctxMate.ToString(-1, 1), ctxWin.ToString(-1, 1), ctxMags));
						}
						ctxSupMul = 0.5 + DCO_Ctx.P(ctx, DCO_ECtxParam.SUP);
					}
				}
			}

			if (DCO_SmokeField.Any())
			{
				int smokeGate = DCO_SmokeGate(target, weaponType, selectedWeaponComp);
				if (smokeGate == 1)
					return FIRE_TREE_LOOK;
				if (smokeGate == 2)
				{
					fireRate = 1.0;
					if (weaponType == EWeaponType.WT_MACHINEGUN)
						fireRate = 2.0;
					return FIRE_TREE_SUPPRESSIVE;
				}
			}

			vector grenadeThrowPos;

			if (target.GetTimeSinceSeen() < lastSeenThreshold
				&& DCO_UGLUtility.ShouldUseGL(m_UtilityComponent, selectedWeaponComp, target.GetLastSeenPosition()))
			{
				return 8;
			}

			if (ctxFrag && m_CombatComponent.HasWeaponOfType(EWeaponType.WT_FRAGGRENADE) && DCO_GrenadeUtility.TryCloseFrag(m_UtilityComponent, target, targetDistance, false))
				return FIRE_TREE_LOOK;

			if ((!directDamage || weaponType != EWeaponType.WT_ROCKETLAUNCHER) &&
				target.GetTimeSinceSeen() < lastSeenThreshold &&
				target.GetTraceFraction() > 0.4)
			{
				if (DCO_AmmoUtility.ShouldAvoidSuppressiveFire(m_UtilityComponent, selectedWeaponComp))
					return FIRE_TREE_LOOK;

				if (farRifle && !DCO_LongRange.IsShotWindow(m_UtilityComponent.m_OwnerEntity))
					return FIRE_TREE_LOOK;

				float maxFireRate = Math.Max(1, Math.Map(targetDistance, 0, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE, 3, 1));
				fireRate = DCO_Air.CapRate(target, maxFireRate * threat, targetDistance, weaponType);
				DCO_MgFloorUsed(mgRaw, threat, targetDistance, visible, "suppress");
				if (weaponType == EWeaponType.WT_MACHINEGUN)
					fireRate *= DCO_Specialist.RateMul(m_CombatComponent.DCO_GetSpec());

				fireRate *= DCO_AmmoUtility.GetAmmoConservationScale(m_UtilityComponent, selectedWeaponComp);
				if (ctxSupMul != 1.0)
				{
					fireRate *= ctxSupMul;
					DCO_Perf.Count("ctx:sup_fire");
					if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
						DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_UtilityComponent.m_OwnerEntity), string.Format("ctxsup unit=%1 rate=%2 window_s=%3 unseen_s=%4 ctx=%5", DCO_Log.IdUnit(m_UtilityComponent.m_OwnerEntity), fireRate.ToString(-1, 2), lastSeenThreshold.ToString(-1, 1), target.GetTimeSinceSeen().ToString(-1, 1), DCO_Ctx.Tag(ctx)));
				}

				return FIRE_TREE_SUPPRESSIVE;
			}
			else if (target.GetTimeSinceSeen() > 2 && ctxFrag
				&& m_CombatComponent.HasWeaponOfType(EWeaponType.WT_FRAGGRENADE)
				&& DCO_GrenadeUtility.InThrowRange(m_UtilityComponent, targetDistance)
				&& DCO_GrenadeUtility.CanThrowGrenadeNow(m_UtilityComponent)
				&& DCO_GrenadeUtility.ResolveThrowPos(m_UtilityComponent, target.GetLastSeenPosition(), grenadeThrowPos))
			{
				SCR_AIThrowGrenadeToBehavior gren = new SCR_AIThrowGrenadeToBehavior(m_UtilityComponent, null, grenadeThrowPos, EWeaponType.WT_FRAGGRENADE, 1, SCR_AIThrowGrenadeToBehavior.PRIORITY_BEHAVIOR_THROW_GRENADE +
				SCR_AIThrowGrenadeToBehavior.PRIORITY_LEVEL_PLAYER);
				m_UtilityComponent.AddAction(gren);
				DCO_GrenadeUtility.NotifyGrenadeThrown(m_UtilityComponent);

				return FIRE_TREE_LOOK;
			}
			else if ((!directDamage || weaponType == EWeaponType.WT_ROCKETLAUNCHER) &&
				target.GetTimeSinceSeen() < lastSeenThreshold &&
				target.GetTraceFraction() > 0.4 &&
				(weaponType != EWeaponType.WT_ROCKETLAUNCHER || DCO_RpgUnseen.Allowed(m_UtilityComponent, target.GetTargetEntity(), target.GetLastSeenPosition())))
			{
				return 7;
			}
			else
				return FIRE_TREE_LOOK;
		}

		return FIRE_TREE_LOOK;
	}

	protected int m_iDCOSmokeLogCloud;
	protected int m_iDCOSmokeLogMode;

	protected int DCO_SmokeGate(BaseTarget target, EWeaponType weaponType, BaseWeaponComponent weapon)
	{
		IEntity me = m_UtilityComponent.m_OwnerEntity;
		if (!target || !me)
			return 0;
		DCO_SmokeCloud cloud = DCO_SmokeField.OnLine(me.GetOrigin() + vector.Up * 1.6, target.GetLastSeenPosition() + vector.Up * 1.0);
		if (!cloud)
			return 0;
		float window = 5.0;
		if (weaponType == EWeaponType.WT_MACHINEGUN)
			window = 12.0;
		float since = target.GetTimeSinceSeen();
		bool endangered = target.GetTimeSinceEndangered() < 3.0;
		int mode = 1;
		if ((since < window || endangered) && !DCO_AmmoUtility.ShouldAvoidSuppressiveFire(m_UtilityComponent, weapon))
			mode = 2;
		if (mode == 2)
			DCO_Perf.Count("smoke:suppress");
		else
			DCO_Perf.Count("smoke:fire_block");
		if ((cloud.m_iId != m_iDCOSmokeLogCloud || mode != m_iDCOSmokeLogMode) && DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
		{
			string modeName = "block";
			if (mode == 2)
				modeName = "suppress";
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("smoke_fire unit=%1 mode=%2 cloud=%3 since_s=%4 endangered=%5 weapon=%6 target_dist=%7",
				DCO_Log.IdUnit(me), modeName, cloud.m_iId, Math.Round(since * 10) / 10, endangered, typename.EnumToString(EWeaponType, weaponType), Math.Round(target.GetDistance())));
		}
		m_iDCOSmokeLogCloud = cloud.m_iId;
		m_iDCOSmokeLogMode = mode;
		return mode;
	}

	protected bool DCO_SquadMovingInVehicleSupport()
	{
		IEntity me = m_UtilityComponent.m_OwnerEntity;
		if (!DCO_VehicleCombat.GetVehicle(me))
			return false;
		SCR_AIGroup grp = SCR_AIGroup.Cast(m_UtilityComponent.GetAIAgent().GetParentGroup());
		if (!grp)
			return false;
		DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));
		if (g && g.GetPhase() == DCO_ETaskPhase.MOVING)
			return true;
		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			ChimeraCharacter c = ChimeraCharacter.Cast(a.GetControlledEntity());
			if (!c || c.IsInVehicle())
				continue;
			if (c.GetPhysics() && c.GetPhysics().GetVelocity().Length() > 0.5)
				return true;
		}
		return false;
	}

	override void ResolveAimpointTypes(notnull BaseTarget target, out EAimPointType aimpointType0, out EAimPointType aimpointType1)
	{
		IEntity targetEntity = target.GetTargetEntity();
		if (!targetEntity)
		{
			aimpointType0 = -1;
			aimpointType1 = -1;
			return;
		}

		EWeaponType weaponType = m_CombatComponent.GetSelectedWeaponType();
		ChimeraCharacter character = ChimeraCharacter.Cast(targetEntity);
		if (character)
		{
			if (character.IsInVehicle())
			{
				aimpointType0 = EAimPointType.WEAK;
				aimpointType1 = EAimPointType.NORMAL;
				return;
			}

			if (weaponType == EWeaponType.WT_SNIPERRIFLE)
			{
				aimpointType0 = EAimPointType.INCAPACITATE;
				aimpointType1 = EAimPointType.NORMAL;
				return;
			}

			aimpointType0 = EAimPointType.NORMAL;
			aimpointType1 = EAimPointType.WEAK;
			return;
		}
		else
		{
			if (weaponType == EWeaponType.WT_ROCKETLAUNCHER)
			{
				aimpointType0 = EAimPointType.WEAK;
				aimpointType1 = EAimPointType.NORMAL;
				return;
			}

			aimpointType0 = EAimPointType.NORMAL;
			aimpointType1 = EAimPointType.WEAK;
			return;
		}
	}

	protected void DCO_MgFloorUsed(float raw, float used, float dist, bool visible, string site)
	{
		if (used <= raw)
			return;
		DCO_Perf.Count("ctx:mg_floor_" + site);
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_UtilityComponent.m_OwnerEntity), string.Format("mg_floor unit=%1 site=%2 threat=%3 used=%4 dist=%5 visible=%6",
				DCO_Log.IdUnit(m_UtilityComponent.m_OwnerEntity), site, raw.ToString(-1, 2), used.ToString(-1, 2), Math.Round(dist), DCO_Util.B01(visible)));
	}

	protected bool IsCloseDirectThreat(BaseTarget target, bool visible)
	{
		if (!target || !visible)
			return false;

		return target.GetDistance() < 80.0;
	}

	protected bool ShouldReturnFireWhenEndangered()
	{
		if (!m_UtilityComponent || !m_UtilityComponent.m_ThreatSystem)
			return false;

		float threat = m_UtilityComponent.m_ThreatSystem.GetThreatMeasure();
		float threshold = DCO_PersonalityCombatUtility.GetEndangeredReturnFireThreshold(m_UtilityComponent);

		return threat >= threshold;
	}

	protected BaseWeaponComponent m_DCOBurstWeapon;
	protected int m_iDCOBurstMuzzle = -1;
	protected IEntity m_DCOCtxReactTgt;
	protected float m_fDCOCtxReactAt_ms = -1;
	protected float m_fDCOCtxReactDelay_s;
	protected BaseTarget m_DCORollTarget;
	protected float m_fDCORollAt_ms = -1;
	protected bool m_bDCORoll;

	protected bool DCO_DisciplineBreak(BaseTarget target, bool visible)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (target != m_DCORollTarget || m_fDCORollAt_ms < 0 || now - m_fDCORollAt_ms >= 3000.0)
		{
			m_DCORollTarget = target;
			m_fDCORollAt_ms = now;
			m_bDCORoll = ShouldBreakDisciplineByChance(visible);
		}
		return m_bDCORoll;
	}

	protected bool ShouldBreakDisciplineByChance(bool visible)
	{
		if (!visible)
			return false;

		float skillFactor       = DCO_PersonalityCombatUtility.GetSkillDisciplineFactor(m_UtilityComponent);
		float personalityScale  = DCO_PersonalityCombatUtility.GetDisciplineBreakChanceScale(m_UtilityComponent);

		float breakChance = 0.35 * skillFactor * personalityScale;
		breakChance = Math.Clamp(breakChance, 0.0, 0.4);

		return Math.RandomFloat01() < breakChance;
	}

	protected float ThreatRememberFromPersonality()
	{
		return DCO_PersonalityCombatUtility.GetThreatRememberFromPersonalityScale(m_UtilityComponent);
	}

	protected float DCO_SuppressWindow(EWeaponType weaponType, float dist)
	{
		float w = 5.0;
		if (weaponType == EWeaponType.WT_MACHINEGUN)
			w = 12.0;
		else if (dist < 50.0)
			w = 8.0;
		switch (DCO_AIConfigComponent.PersonalityOf(m_UtilityComponent))
		{
			case DCO_EAIPersonality.CAUTIOUS: w *= 1.3; break;
			case DCO_EAIPersonality.AGGRESSIVE: w *= 0.7; break;
			case DCO_EAIPersonality.RECKLESS: w *= 0.5; break;
		}
		return Math.Clamp(w, 3.0, 15.0);
	}

	protected BaseTarget m_DCOLkTarget;
	protected bool m_bDCOLkStarted;
	protected bool m_bDCOLkEnded;

	protected void DCO_TrackLastKnown(BaseTarget target, float window)
	{
		if (target != m_DCOLkTarget)
		{
			m_DCOLkTarget = target;
			m_bDCOLkStarted = false;
			m_bDCOLkEnded = false;
		}
		float tss = target.GetTimeSinceSeen();
		if (tss < 0.5)
		{
			m_bDCOLkStarted = false;
			m_bDCOLkEnded = false;
			return;
		}
		if (!m_bDCOLkStarted && tss < window)
		{
			m_bDCOLkStarted = true;
			DCO_Perf.Count("sup_lk_start");
			return;
		}
		if (m_bDCOLkStarted && !m_bDCOLkEnded && tss >= window)
		{
			m_bDCOLkEnded = true;
			DCO_Perf.Count("sup_lk_end");
			vector pos = target.GetLastSeenPosition() + Vector(0, 1.2, 0);
			if (m_UtilityComponent.m_LookAction)
				m_UtilityComponent.m_LookAction.LookAt(pos, 30.0, 10.0);
			GetGame().GetCallqueue().CallLater(DCO_ReportLostContact, 10000, false, m_UtilityComponent.m_OwnerEntity, pos);
		}
	}

	static void DCO_ReportLostContact(IEntity me, vector pos)
	{
		if (!me || !DCO_Util.IsAlive(me))
			return;
		ChimeraCharacter ch = ChimeraCharacter.Cast(me);
		AIControlComponent ctrl;
		if (ch)
			ctrl = AIControlComponent.Cast(ch.FindComponent(AIControlComponent));
		AIAgent agent;
		if (ctrl)
			agent = ctrl.GetControlAIAgent();
		SCR_AIGroup grp;
		if (agent)
			grp = SCR_AIGroup.Cast(agent.GetParentGroup());
		if (grp && grp.GetLeaderAgent() && grp.GetLeaderAgent() != agent)
		{
			SCR_ChimeraAIAgent la = SCR_ChimeraAIAgent.Cast(grp.GetLeaderAgent());
			if (la && la.m_UtilityComponent && la.m_UtilityComponent.m_LookAction)
				la.m_UtilityComponent.m_LookAction.LookAt(pos, 25.0, 5.0);
		}
		DCO_Perf.Count("lost_contact_report");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, "-", string.Format("lost_contact unit=%1 grp=%2 pos=%3", DCO_Log.IdUnit(me), DCO_Log.IdGroup(grp), pos));
	}
}