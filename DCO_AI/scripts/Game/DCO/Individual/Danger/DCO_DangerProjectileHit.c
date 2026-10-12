[BaseContainerProps()]
modded class SCR_AIDangerReaction_ProjectileHit : SCR_AIDangerReaction
{
	protected static const float COVER_QUERY_SECTOR_ANGLE_RAD  = 0.35 * Math.PI;



	protected static ref map<IEntity, float> s_mLastDodgeRollTime;
	protected static map<IEntity, float> GetLastDodgeRollTime()
	{
		if (!s_mLastDodgeRollTime)
			s_mLastDodgeRollTime = new map<IEntity, float>();
		return s_mLastDodgeRollTime;
	}

	protected float m_fLastCoverMoveDuration_s;

	[Attribute("2", UIWidgets.EditBox, "Minimum rounds to reposition from cover when there is no target yet.")]
	protected int m_iBulletsRepositionNoTarget;

	[Attribute("4", UIWidgets.EditBox, "Minimum rounds to reposition quickly even with a target and a distant shooter.")]
	protected int m_iBulletsRepositionHeavy;

	[Attribute("1", UIWidgets.EditBox, "Minimum rounds to flee when NOT in cover and there is no target yet.")]
	protected int m_iBulletsFleeNoCover;

	[Attribute("3", UIWidgets.EditBox, "Minimum rounds to flee when NOT in cover even with a target.")]
	protected int m_iBulletsFleeEngaged;

	[Attribute("1", UIWidgets.CheckBox, "Scale the thresholds above by AI personality (CAUTIOUS reacts sooner, RECKLESS later).")]
	protected bool m_bScaleThresholdsByPersonality;

	[Attribute("1", UIWidgets.CheckBox, "Throw smoke when retreating from a position without cover (needs a smoke grenade in the inventory).")]
	protected bool m_bDeploySmokeOnFlee;

	[Attribute("0", UIWidgets.CheckBox, "Try to find a BUILDING before regular cover. Turn off so AI go straight for regular cover.")]
	protected bool m_bCoverPreferBuilding;

	[Attribute("1", UIWidgets.CheckBox, "Impacts = highest priority: react even while another combat move is running, and this request is protected from other systems.")]
	protected bool m_bCoverVeryHighPriority;

	[Attribute("3", UIWidgets.EditBox, "No longer used. Pinned now comes from weighted near misses (Pinned toggle in GM).")]
	protected int m_iBulletsPinned;

	[Attribute("4.0", UIWidgets.EditBox, "No longer used. Pinned lasts while near misses keep coming + 3 s.")]
	protected float m_fPinnedDuration_s;

	[Attribute("1", UIWidgets.CheckBox, "Respect a move in progress: if the AI is already heading to cover, do not interrupt. Only a threat from a NEW DIRECTION may interrupt.")]
	protected bool m_bRespectOngoingMove;

	[Attribute("60.0", UIWidgets.EditBox, "Angle difference (degrees) between a new shooter and the threat being avoided before a running move may be interrupted.")]
	protected float m_fNewThreatAngleDeg;

	[Attribute("1", UIWidgets.CheckBox, "Do not send a new cover move while a cover move from this reaction is still running. Stops AI retreating every time a round lands.")]
	protected bool m_bSkipWhileCoverActive;

	[Attribute("5.0", UIWidgets.EditBox, "Minimum time (s) since this AI's last cover move, including those from heard gunfire reactions. Shorter than the WeaponFired value because impacts are more urgent.")]
	protected float m_fCoverMoveCooldown_s;

	[Attribute("6.0", UIWidgets.EditBox, "Maximum cover search distance (m) when REPOSITIONING (AI already in cover, only moving).")]
	protected float m_fRepositionSearchDist;

	[Attribute("10.0", UIWidgets.EditBox, "Maximum cover search distance (m) when FLEEING from a position without cover.")]
	protected float m_fFleeSearchDist;

	[Attribute("1", UIWidgets.CheckBox, "Force STANDING stance while moving to cover, whatever stance the rule requests. Crouching is too slow.")]
	protected bool m_bForceStandWhileMoving;

	[Attribute("0", UIWidgets.CheckBox, "Let AI fire (suppress) WHILE moving to cover. Turn off so suppression starts only after the AI arrives.")]
	protected bool m_bSuppressWhileMoving;

	[Attribute("0", UIWidgets.CheckBox, "Let AI keep aiming at the shooter WHILE MOVING to cover. Turn off so AI do not walk backward while firing (aim is still set at the end of the move).")]
	protected bool m_bAimWhileMovingToCover;

	[Attribute("1", UIWidgets.CheckBox, "Reaction to impacts at medium range (between IMPACT_DIST_VERY_CLOSE and the 3 m limit).")]
	protected bool m_bReactToNearMiss;

	[Attribute("1", UIWidgets.CheckBox, "Roll the chance to run for cover using DodgeChance from the DCO config (+ personality scale if on). The roll is marked before it is made, so a failed roll is not re-rolled until Cover Move Cooldown ends, unless the threat comes from a new direction.")]
	protected bool m_bUseDodgeChance;

	[Attribute("1", UIWidgets.CheckBox, "Randomise the direction to cover using the weights below (back / left / right / any). Turn off for the previous behavior.")]
	protected bool m_bRandomizeCoverDirection;

	[Attribute("1.0", UIWidgets.EditBox, "Weight for the BACK direction (away from the threat).")]
	protected float m_fCoverDirWeightBackward;

	[Attribute("1.0", UIWidgets.EditBox, "Weight for the LEFT direction (sideways relative to the threat).")]
	protected float m_fCoverDirWeightLeft;

	[Attribute("1.0", UIWidgets.EditBox, "Weight for the RIGHT direction (sideways relative to the threat).")]
	protected float m_fCoverDirWeightRight;

	[Attribute("1.0", UIWidgets.EditBox, "Weight for the ANY direction (ANYWHERE, nearest cover in any direction).")]
	protected float m_fCoverDirWeightAnywhere;

	[Attribute("1", UIWidgets.CheckBox, "The direction weights above are multiplied by a personality scale (CAUTIOUS retreats more often, AGGRESSIVE/RECKLESS move sideways more often). Impacts never cause a forward move. Only applies when Randomize Cover Direction is on.")]
	protected bool m_bScaleCoverDirByPersonality;

	[Attribute("1", UIWidgets.CheckBox, "If the group has a position waypoint, running to cover is steered toward it. Outside the WP radius -> run to the WP. Inside the radius -> normal direction, unless it would leave the radius -> turned toward the WP.")]
	protected bool m_bDodgeTowardWaypoint;

	protected static int s_iDCOMissSample;
	protected static ref array<int> s_aDCOMissBand;

	protected void DCO_MissCalib(SCR_AIUtilityComponent utility, AIDangerEvent dangerEvent)
	{
		IEntity src = dangerEvent.GetObject();
		if (!src || !utility.m_OwnerEntity)
			return;
		ChimeraCharacter shooter = ChimeraCharacter.Cast(src);
		if (!shooter)
			shooter = ChimeraCharacter.Cast(src.GetRootParent());
		if (!shooter)
			return;
		AIControlComponent ctrl = AIControlComponent.Cast(shooter.FindComponent(AIControlComponent));
		if (!ctrl || !ctrl.GetAIAgent())
			return;
		SCR_AIUtilityComponent su = SCR_AIUtilityComponent.Cast(ctrl.GetAIAgent().FindComponent(SCR_AIUtilityComponent));
		if (!su || !su.m_CombatComponent)
			return;
		BaseTarget t = su.m_CombatComponent.GetCurrentTarget();
		if (!t || t.GetTargetEntity() != utility.m_OwnerEntity)
			return;
		vector from = shooter.GetOrigin();
		vector dir = DCO_Specialist.Flat(utility.m_OwnerEntity.GetOrigin() - from);
		float dist = vector.DistanceXZ(utility.m_OwnerEntity.GetOrigin(), from);
		if (dist > 400.0)
			return;
		s_iDCOMissSample++;
		if (!DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
			return;
		int band = 300;
		if (dist < 150.0)
			band = 100;
		else if (dist < 250.0)
			band = 200;
		if (!s_aDCOMissBand)
		{
			s_aDCOMissBand = {};
			s_aDCOMissBand.Insert(0);
			s_aDCOMissBand.Insert(0);
			s_aDCOMissBand.Insert(0);
		}
		int bi = band / 100 - 1;
		string pick = "1in20";
		if (s_aDCOMissBand[bi] < 30)
		{
			s_aDCOMissBand[bi] = s_aDCOMissBand[bi] + 1;
			pick = "band_quota";
		}
		else if (s_iDCOMissSample % 20 != 0)
		{
			if (!DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
				return;
			pick = "debug";
		}
		vector rel = dangerEvent.GetPosition() - from;
		float lat = Math.AbsFloat(dir[0] * rel[2] - dir[2] * rel[0]);
		float impactToTarget = vector.DistanceXZ(dangerEvent.GetPosition(), utility.m_OwnerEntity.GetOrigin());
		string weapon = "rifle";
		EWeaponType wt = su.m_CombatComponent.GetCurrentWeaponType();
		if (DCO_Util.IsMGShooter(shooter) || wt == EWeaponType.WT_MACHINEGUN)
			weapon = "mg";
		else if (wt == EWeaponType.WT_SNIPERRIFLE)
			weapon = "dmr";
		else if (wt == EWeaponType.WT_HANDGUN)
			weapon = "pistol";
		SCR_AICombatComponent sc = su.m_CombatComponent;
		string posture = "stand";
		if (sc.m_iDCOAimPosture == 0)
			posture = "prone";
		else if (sc.m_iDCOAimPosture == 1)
			posture = "crouch";
		string head = string.Format("miss_calib skill=%1 weapon=%2 band=%3 lat_m=%4 dist=%5 sigma_mrad=%6 posture=%7 supported=%8 stress=%9",
			typename.EnumToString(DCO_AISKILL, DCO_AIConfigComponent.SkillOf(su)), weapon, band, lat.ToString(-1, 2), Math.Round(dist), sc.m_fDCOAimSigma.ToString(-1, 2), posture, sc.m_bDCOAimSupported, sc.m_fDCOAimStress.ToString(-1, 2));
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(shooter), head + string.Format(" ramp=%1 light=%2 optic=%3 moving=%4 target=%5 src=impact pick=%6 impact_to_target_m=%7 band_n=%8",
			sc.m_fDCOAimRamp.ToString(-1, 2), sc.m_fDCOAimLight, sc.m_fDCOAimOptic.ToString(-1, 2), sc.m_bDCOAimMoving, DCO_Log.IdUnit(utility.m_OwnerEntity), pick, impactToTarget.ToString(-1, 1), s_aDCOMissBand[bi]));
	}

	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{
		DCO_Perf.Count("ev:hit");
		if (dangerEvent)
			DCO_MissCalib(utility, dangerEvent);
		if (dangerEvent && utility.m_OwnerEntity && utility.m_CombatComponent && vector.Distance(dangerEvent.GetPosition(), utility.m_OwnerEntity.GetOrigin()) < 2.0)
			utility.m_CombatComponent.DCO_OnNearImpact();
		int perfStart = DCO_Perf.Begin();
		bool reacted = DCO_PerformHitReaction(utility, threatSystem, dangerEvent, dangerEventCount);
		DCO_Perf.End("danger_hit", perfStart);
		return reacted;
	}

	protected bool DCO_PerformHitReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{
		vector impactPos = dangerEvent.GetPosition();
		float distanceToDangerSq = vector.DistanceSq(utility.GetOrigin(), impactPos);

		if (distanceToDangerSq > 9.0)
			return false;

		IEntity hitShooter = dangerEvent.GetObject();
		vector hitFrom;
		if (hitShooter && hitShooter.GetRootParent())
			hitFrom = hitShooter.GetRootParent().GetOrigin();

		if (utility.m_CombatComponent)
		{
			utility.m_CombatComponent.DCO_TryStartle(impactPos);
			utility.m_CombatComponent.DCO_NoteNearMiss(hitFrom);
		}

		threatSystem.DCO_ThreatImpact(dangerEventCount, SCR_AIThreatSystem.DCO_MissWeight(Math.Sqrt(distanceToDangerSq)), hitFrom, hitShooter);

		IEntity shooter = dangerEvent.GetObject();
		if (!shooter)
			return false;

		IEntity shooterRoot = shooter.GetRootParent();
		if (!shooterRoot)
			return false;

		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
		if (!agent || !agent.IsEnemy(shooterRoot))
			return false;

		if (!utility.m_CombatComponent)
			return false;

		SCR_AICombatMoveState state = utility.m_CombatMoveState;
		SCR_CharacterControllerComponent charCon = utility.m_CombatComponent.GetCharacterController();

		if (!state || !charCon)
			return false;

		vector shooterPos      = shooter.GetOrigin();
		float  distanceToDanger = Math.Sqrt(distanceToDangerSq);
		float  shooterDistance  = vector.Distance(utility.GetOrigin(), shooterRoot.GetOrigin());
		bool   isNullTarget     = utility.m_CombatComponent.GetCurrentTarget() == null;
		bool   inCover          = state.IsInValidCover();
		int    bulletCount      = dangerEventCount;

		IEntity ownerEnt = utility.m_OwnerEntity;
		bool allowProne = DCO_CQC.IsProneAllowed(ownerEnt, shooterDistance);

		if (DCO_AssaultLock.CheckBlock(utility, "take_cover"))
		{
			LowerStance(charCon, false);
			return true;
		}

		if (utility.m_CombatComponent.DCO_IsPinned())
		{
			KeepProne(charCon, allowProne);
			return true;
		}

		int thrRepoNoTarget = ScaleThreshold(utility, m_iBulletsRepositionNoTarget);
		int thrRepoHeavy    = ScaleThreshold(utility, m_iBulletsRepositionHeavy);
		int thrFleeNoCover  = ScaleThreshold(utility, m_iBulletsFleeNoCover);
		int thrFleeEngaged  = ScaleThreshold(utility, m_iBulletsFleeEngaged);

		bool veryClose  = distanceToDanger < 2.0;
		bool shooterNear = shooterDistance < 20.0;

		if (utility.m_CombatComponent.GetSelectedWeaponType() == EWeaponType.WT_MACHINEGUN
			&& veryClose && isNullTarget)
		{
			TrySuppressShooter(utility, shooterRoot, shooterDistance, 6, 1.5);
		}

		if (veryClose && inCover && isNullTarget && bulletCount > thrRepoNoTarget)
		{
			return PushCoverMove(utility, state, shooterPos,
				ECharacterStance.STAND, ECharacterStance.CROUCH,
				EMovementType.SPRINT, SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT,
				m_fRepositionSearchDist, true, false, false, false, false);
		}

		if (veryClose && inCover && shooterNear && bulletCount > thrRepoNoTarget)
		{
			return PushCoverMove(utility, state, shooterPos,
				ECharacterStance.STAND, ECharacterStance.CROUCH,
				EMovementType.SPRINT, SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT,
				m_fRepositionSearchDist * 0.5, true, false, true, true, false);
		}

		if (veryClose && inCover && bulletCount > thrRepoHeavy)
		{
			bool moved = PushCoverMove(utility, state, shooterPos,
				ECharacterStance.STAND, ECharacterStance.CROUCH,
				EMovementType.SPRINT, SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT,
				m_fRepositionSearchDist, true, true, true, true, false);

			if (moved)
				SuppressAfterMove(utility, shooterRoot, shooterDistance, 5, 1.5);

			return moved;
		}

		if (charCon.GetStance() == ECharacterStance.PRONE
			&& distanceToDanger < 1.5
			&& bulletCount > thrRepoNoTarget)
		{
			int rollAttempt = 2 * Math.RandomInt(1,3);
			for (int i = 0; i < rollAttempt; i++)
			{
				if (Math.RandomIntInclusive(0, 1) == 0)
					charCon.SetRoll(1);
				else
					charCon.SetRoll(2);
			}

			return true;
		}

		if (veryClose && isNullTarget && bulletCount > thrFleeNoCover)
		{
			return PushCoverMove(utility, state, shooterPos,
				ECharacterStance.STAND, ECharacterStance.CROUCH,
				EMovementType.SPRINT, SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT,
				m_fFleeSearchDist, false, false, false, false, m_bDeploySmokeOnFlee);
		}

		if (veryClose && shooterNear)
		{
			bool moved = PushCoverMove(utility, state, shooterPos,
				ECharacterStance.STAND, ECharacterStance.CROUCH,
				EMovementType.SPRINT, SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT,
				m_fFleeSearchDist * 0.5, false, true, true, true, false);

			if (moved)
				SuppressAfterMove(utility, shooterRoot, shooterDistance, 5, 1.5);

			return moved;
		}

		if (veryClose && bulletCount > thrFleeEngaged)
		{
			bool moved = PushCoverMove(utility, state, shooterPos,
				ECharacterStance.STAND, ECharacterStance.CROUCH,
				EMovementType.SPRINT, SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT,
				m_fFleeSearchDist, false, true, true, true, false);

			if (moved)
				SuppressAfterMove(utility, shooterRoot, shooterDistance, 5, 1.5);

			return moved;
		}


		if (m_bReactToNearMiss && !veryClose)
		{
			if (!inCover && bulletCount > thrFleeEngaged)
			{
				return PushCoverMove(utility, state, shooterPos,
					ECharacterStance.STAND, ECharacterStance.CROUCH,
					EMovementType.SPRINT, SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT,
					m_fFleeSearchDist, false, false, true, false, false);
			}

			LowerStance(charCon, allowProne);
			return true;
		}

		if (shooterDistance > 100.0)
		{
			LowerStance(charCon, allowProne);
			return true;
		}

		return PushCoverMove(utility, state, shooterPos,
			ECharacterStance.STAND, ECharacterStance.CROUCH,
			EMovementType.SPRINT, SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT,
			m_fFleeSearchDist, false, false, true, false, m_bDeploySmokeOnFlee);
	}

	protected bool PushCoverMove(
		notnull SCR_AIUtilityComponent utility,
		notnull SCR_AICombatMoveState state,
		vector shooterPos,
		ECharacterStance stanceMoving,
		ECharacterStance stanceEnd,
		EMovementType movementType,
		float speedReference,
		float coverSearchDistMax,
		bool failIfNoCover,
		bool aimAtTarget,
		bool aimAtTargetEnd,
		bool randomDirection,
		bool deploySmoke)
	{
		bool threatFromNewDirection = IsThreatFromNewDirection(utility, state, shooterPos);

		if (m_bSkipWhileCoverActive && state.DCO_IsCoverProtected() && !threatFromNewDirection)
			return false;

		if (state.IsExecutingRequest())
		{
			if (!m_bCoverVeryHighPriority)
				return false;

			if (m_bRespectOngoingMove && !threatFromNewDirection)
				return false;
		}

		if (!threatFromNewDirection && !DCO_CoverMoveBudget.CanMove(utility.m_OwnerEntity, m_fCoverMoveCooldown_s))
			return false;

		if (utility.m_DCOConfig && utility.m_DCOConfig.IsHoldPosition())
			return false;

		if (m_bUseDodgeChance && !RollDodgeChance(utility, threatFromNewDirection))
			return false;

		if (deploySmoke)
			DCO_SmokeUtility.TryDeploySmokeForRetreat(utility, shooterPos);

		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

		rq.m_eReason  = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
		rq.m_vTargetPos = shooterPos;
		rq.m_vMovePos   = rq.m_vTargetPos;

		rq.m_bTryFindCover              = true;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility      = true;
		rq.m_bFailIfNoCover             = failIfNoCover;

		if (m_bForceStandWhileMoving)
			rq.m_eStanceMoving = ECharacterStance.STAND;
		else
			rq.m_eStanceMoving = stanceMoving;
		rq.m_eStanceEnd    = stanceEnd;
		rq.m_eMovementType = movementType;

		rq.m_fCoverSearchDistMax = coverSearchDistMax;
		rq.m_fCoverSearchDistMin = 2;
		rq.m_fMoveDuration_s     = Math.RandomFloat(1.0, 1.5) * coverSearchDistMax / speedReference;
		m_fLastCoverMoveDuration_s = rq.m_fMoveDuration_s;

		rq.m_eDirection = ResolveDirectionForUnit(utility, randomDirection);
		rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;

		if (!m_bAimWhileMovingToCover)
			rq.m_bAimAtTarget = false;
		else
			rq.m_bAimAtTarget = aimAtTarget;

		rq.m_bAimAtTargetEnd = aimAtTargetEnd;

		bool buildingFailedBefore = state.GetOldRequest() && state.GetOldRequest().m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND;

		if (!m_bCoverPreferBuilding || buildingFailedBefore)
		{
			rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		}
		else
		{
			rq.m_eType = SCR_EAICombatMoveRequestType.BUILDING;
			rq.m_bTryFindCover = false;
			rq.m_bFailIfNoCover = false;
		}

		if (rq.m_eType == SCR_EAICombatMoveRequestType.MOVE)
			state.DCO_SetDodgeStance(rq);

		if (m_bDodgeTowardWaypoint)
			DCO_DodgeWaypointUtility.ApplyWaypointBias(utility, rq, coverSearchDistMax);

		DCO_CoverMoveBudget.MarkMove(utility.m_OwnerEntity);

		if (m_bCoverVeryHighPriority)
			state.DCO_ApplyCoverRequest(rq, rq.m_fMoveDuration_s + 1.0);
		else
			state.ApplyNewRequest(rq);

		return true;
	}

	protected bool RollDodgeChance(notnull SCR_AIUtilityComponent utility, bool threatFromNewDirection)
	{
		IEntity ownerEnt = utility.m_OwnerEntity;
		if (!ownerEnt)
			return false;

		float now_ms = GetGame().GetWorld().GetWorldTime();

		if (!threatFromNewDirection)
		{
			float lastRoll_ms;
			if (GetLastDodgeRollTime().Find(ownerEnt, lastRoll_ms))
			{
				if ((now_ms - lastRoll_ms) < (m_fCoverMoveCooldown_s * 1000.0))
					return false;
			}
		}

		GetLastDodgeRollTime().Set(ownerEnt, now_ms);

		if (GetLastDodgeRollTime().Count() > 128)
			PruneDodgeRollMap(now_ms);

		float chance    = 0.6;
		bool  scalePers = true;

		DCO_AIConfigComponent cfg = utility.m_DCOConfig;
		if (cfg)
		{
			chance    = cfg.GetDodgeChance();
			scalePers = cfg.GetDodgeScaleByPersonality();
		}

		if (scalePers)
			chance *= DCO_PersonalityCombatUtility.GetTakeCoverChanceScale(utility);

		return Math.RandomFloat01() < Math.Clamp(chance, 0.0, 1.0);
	}

	protected void PruneDodgeRollMap(float now_ms)
	{
		float staleAge_ms = m_fCoverMoveCooldown_s * 1000.0 * 2.0;

		array<IEntity> toRemove = {};

		foreach (IEntity ent, float lastTime_ms : GetLastDodgeRollTime())
		{
			if (!ent || (now_ms - lastTime_ms) > staleAge_ms)
				toRemove.Insert(ent);
		}

		foreach (IEntity entRemove : toRemove)
		{
			GetLastDodgeRollTime().Remove(entRemove);
		}
	}

	protected bool IsThreatFromNewDirection(notnull SCR_AIUtilityComponent utility, notnull SCR_AICombatMoveState state, vector shooterPos)
	{
		SCR_AICombatMoveRequest_Move currentRq = SCR_AICombatMoveRequest_Move.Cast(state.GetRequest());
		if (!currentRq)
			return false;

		vector myPos = utility.GetOrigin();

		vector dirOld = currentRq.m_vTargetPos - myPos;
		vector dirNew = shooterPos - myPos;

		dirOld[1] = 0;
		dirNew[1] = 0;

		float lenOld = dirOld.Length();
		float lenNew = dirNew.Length();

		if (lenOld < 0.5 || lenNew < 0.5)
			return false;

		dirOld = dirOld * (1.0 / lenOld);
		dirNew = dirNew * (1.0 / lenNew);

		float angleDeg = Math.Acos(Math.Clamp(vector.Dot(dirOld, dirNew), -1.0, 1.0)) * Math.RAD2DEG;

		return angleDeg > m_fNewThreatAngleDeg;
	}

	protected void KeepProne(SCR_CharacterControllerComponent charCon, bool allowProne)
	{
		if (!charCon || DCO_Nav.Busy(charCon, "hit_stance"))
			return;

		if (!allowProne)
		{
			if (charCon.GetStance() == ECharacterStance.STAND)
				DCO_Stance.Request(charCon, 2, "hit");
			return;
		}

		if (charCon.GetStance() != ECharacterStance.PRONE)
			DCO_Stance.Request(charCon, 3, "hit");
	}

	protected void LowerStance(SCR_CharacterControllerComponent charCon, bool allowProne)
	{
		if (!charCon || DCO_Nav.Busy(charCon, "hit_stance"))
			return;

		if (charCon.GetStance() == ECharacterStance.STAND)
			DCO_Stance.Request(charCon, 2, "hit");
		else if (allowProne && charCon.GetStance() == ECharacterStance.CROUCH)
			DCO_Stance.Request(charCon, 3, "hit");
	}

	protected SCR_EAICombatMoveDirection ResolveDirection(bool randomDirection)
	{
		if (m_bRandomizeCoverDirection)
			return DCO_Util.PickCoverDirection(m_fCoverDirWeightBackward, m_fCoverDirWeightLeft, m_fCoverDirWeightRight, m_fCoverDirWeightAnywhere);

		if (!randomDirection)
			return SCR_EAICombatMoveDirection.BACKWARD;

		if (Math.RandomIntInclusive(0, 1) == 0)
			return SCR_EAICombatMoveDirection.BACKWARD;

		if (Math.RandomIntInclusive(0, 1) == 0)
			return SCR_EAICombatMoveDirection.LEFT;

		return SCR_EAICombatMoveDirection.RIGHT;
	}

	protected SCR_EAICombatMoveDirection ResolveDirectionForUnit(SCR_AIUtilityComponent utility, bool randomDirection)
	{
		if (!m_bRandomizeCoverDirection || !m_bScaleCoverDirByPersonality || !utility)
			return DCO_Osc.KeepDir(utility, ResolveDirection(randomDirection), false, "proj_hit");

		return DCO_Osc.KeepDir(utility, DCO_PersonalityCombatUtility.PickCoverDirectionForPersonality(utility,
			m_fCoverDirWeightBackward, m_fCoverDirWeightLeft, m_fCoverDirWeightRight, m_fCoverDirWeightAnywhere,
			0.0, false), false, "proj_hit");
	}

	protected void SuppressAfterMove(notnull SCR_AIUtilityComponent utility, IEntity shooterRoot, float shooterDistance, float duration, float interval)
	{
		if (m_bSuppressWhileMoving || m_fLastCoverMoveDuration_s <= 0)
		{
			TrySuppressShooter(utility, shooterRoot, shooterDistance, duration, interval);
			return;
		}

		utility.GetCallqueue().CallLater(TrySuppressShooter, (int)(m_fLastCoverMoveDuration_s * 1000.0), false,
			utility, shooterRoot, shooterDistance, duration, interval);
	}

	protected void TrySuppressShooter(notnull SCR_AIUtilityComponent utility, IEntity shooterRoot, float shooterDistance, float duration, float interval)
	{
		if (!shooterRoot || !utility.m_CombatComponent)
			return;

		if (!utility.m_CombatComponent.HasWeaponOfType(EWeaponType.WT_MACHINEGUN))
			return;

		float radius = Math.Map(shooterDistance, 0, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE, 1, 3);

		vector bbMin, bbMax;

		SCR_AISuppressionVolumeBase.CreateSuppressionBox(shooterRoot.GetOrigin(), radius, 3, bbMin, bbMax);

		SCR_AISuppressionObjectVolumeBox suppVolume = new SCR_AISuppressionObjectVolumeBox(bbMin, bbMax);
		if (!suppVolume)
			return;

		SCR_AISuppressBehavior supp = new SCR_AISuppressBehavior(utility, null, suppVolume, duration, interval);
		utility.AddAction(supp);
	}

	protected int ScaleThreshold(notnull SCR_AIUtilityComponent utility, int baseThreshold)
	{
		if (!m_bScaleThresholdsByPersonality || !utility.m_DCOConfig)
			return baseThreshold;

		float scale;
		switch (utility.m_DCOConfig.GetPersonality())
		{
			case DCO_EAIPersonality.CAUTIOUS:   scale = 0.7; break;
			case DCO_EAIPersonality.AGGRESSIVE: scale = 1.2; break;
			case DCO_EAIPersonality.RECKLESS:   scale = 1.4; break;
			default:                            scale = 1.0; break;
		}

		return Math.Max(1, Math.Round(1.0 * baseThreshold * scale));
	}
};