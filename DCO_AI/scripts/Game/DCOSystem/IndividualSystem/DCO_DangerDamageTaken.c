[BaseContainerProps()]
modded class SCR_AIDangerReaction_DamageTaken
{
	// --------------------------------------------------------
	//  Tuning constants
	// --------------------------------------------------------
	protected static const float COVER_SEARCH_DIST_MAX      = 30.0;
	protected static const float COVER_SEARCH_DIST_MIN      = 2.0;

	//! Di atas jarak ini reaksinya "break contact" (sprint, arah acak, ga bidik).
	//! Di bawahnya "reposition" (run, mundur, tetap bidik).
	protected static const float LONG_RANGE_THRESHOLD       = 60.0;

	protected static const float SUPPRESS_CHANCE            = 0.1;
	protected static const float SUPPRESS_COOLDOWN_S        = 25.0;
	protected static const float SUPPRESS_DURATION_S        = 4.0;
	protected static const float SUPPRESS_PRIORITY          = 1.5;
	protected static const float SUPPRESS_BOX_HEIGHT        = 3.0;
	protected static const float SUPPRESS_RADIUS_MIN        = 1.5;
	protected static const float SUPPRESS_RADIUS_MAX        = 8.0;

	protected static const int   SUPPRESS_MAP_PRUNE_THRESHOLD = 128;

	protected const float COVER_QUERY_SECTOR_ANGLE_RAD = 0.35 * Math.PI;

	//! Cooldown suppress disimpan di static map keyed by entity, BUKAN member field.
	//! Alasannya: instance SCR_AIDangerReaction dibikin dari config lewat
	//! BaseContainerProps, jadi belum tentu satu instance per agent. Member field
	//! bakal jadi cooldown global buat semua AI kalau instance-nya ternyata shared.
	//! Idiom ini sama persis sama s_mLastDodgeTime di DCO_DangerEventWeaponFired.c.
	protected static ref map<IEntity, float> s_mLastSuppressTime = new map<IEntity, float>();

	// --------------------------------------------------------
	//  Main reaction
	// --------------------------------------------------------
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{
		IEntity shooter = dangerEvent.GetObject();

		if (!shooter)
			return super.PerformReaction(utility, threatSystem, dangerEvent, dangerEventCount);

		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());

		if (!agent)
			return false;

		IEntity shooterRoot = shooter.GetRootParent();

		if (!shooterRoot || !agent.IsEnemy(shooterRoot))
			return false;

		SCR_AICombatMoveState state = utility.m_CombatMoveState;

		if (!state)
			return false;

		// === ADDED: Indoor defense ===
		// Kena hit = satu-satunya pemicu relocate di dalem gedung. Gate di ApplyNewRequest
		// yang mutusin: kalau unit di dalem gedung, request mundur di bawah diganti INDOOR_RELOCATE.
		state.DCO_MarkIndoorRelocateTrigger();
		// === END ADDED ===

		SCR_AICombatComponent combat = utility.m_CombatComponent;

		if (!combat)
			return false;

		// Guard yang sama kayak TryDodge: di kendaraan ga ada combat move sama sekali.
		if (utility.m_AIInfo && utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
			return false;

		vector shooterPos    = shooter.GetOrigin();
		float  distToShooter = vector.Distance(utility.GetOrigin(), shooterPos);
		bool   isLongRange   = distToShooter > LONG_RANGE_THRESHOLD;

		TrySuppressShooter(utility, combat, shooterRoot);

		SCR_CharacterControllerComponent charCon = combat.GetCharacterController();

		if (charCon && charCon.GetStance() == ECharacterStance.PRONE && isLongRange)
		{
			if (Math.RandomIntInclusive(0, 1) == 0)
				charCon.SetRoll(1);
			else
				charCon.SetRoll(2);

			return true;
		}

		// Unit yang lagi disuruh hold (garrison/sector) ga boleh ninggalin posisi
		// cuma gara-gara kena tembak.
		DCO_AIConfigComponent cfg = utility.m_DCOConfig;

		if (cfg && cfg.IsHoldPosition())
			return true;

		if (state.IsExecutingRequest())
			return true;

		PushRetreatToCover(state, shooterPos, isLongRange);

		return true;
	}

	// --------------------------------------------------------
	//  Move request
	// --------------------------------------------------------
	protected void PushRetreatToCover(notnull SCR_AICombatMoveState state, vector shooterPos, bool longRange)
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

		rq.m_vTargetPos = shooterPos;
		rq.m_vMovePos   = rq.m_vTargetPos;

		rq.m_bTryFindCover              = true;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility      = true;
		rq.m_bFailIfNoCover             = false;

		rq.m_eStanceMoving = ECharacterStance.STAND;
		rq.m_eStanceEnd    = ECharacterStance.CROUCH;

		rq.m_fCoverSearchDistMax            = COVER_SEARCH_DIST_MAX;
		rq.m_fCoverSearchDistMin            = COVER_SEARCH_DIST_MIN;
		rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;

		float speed;

		if (longRange)
		{
			rq.m_eMovementType = EMovementType.SPRINT;
			rq.m_eDirection    = PickBreakContactDirection();
			rq.m_bAimAtTarget  = false;
			speed              = SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;
		}
		else
		{
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_eDirection    = SCR_EAICombatMoveDirection.BACKWARD;
			rq.m_bAimAtTarget  = true;
			speed              = SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_RUN;
		}

		rq.m_bAimAtTargetEnd = true;
		rq.m_fMoveDuration_s = Math.RandomFloat(1.0, 1.5) * COVER_SEARCH_DIST_MAX / speed;

		SCR_AICombatMoveRequestBase oldRq = state.GetOldRequest();

		if (oldRq && oldRq.m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
		{
			rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		}
		else
		{
			rq.m_eType         = SCR_EAICombatMoveRequestType.BUILDING;
			rq.m_bTryFindCover = false;
		}

		state.ApplyNewRequest(rq);
	}

	//! Distribusi dipertahankan persis dari versi lama: 50% BACKWARD, 25% LEFT, 25% RIGHT.
	protected SCR_EAICombatMoveDirection PickBreakContactDirection()
	{
		int roll = Math.RandomIntInclusive(0, 3);

		if (roll <= 1)
			return SCR_EAICombatMoveDirection.BACKWARD;

		if (roll == 2)
			return SCR_EAICombatMoveDirection.LEFT;

		return SCR_EAICombatMoveDirection.RIGHT;
	}

	// --------------------------------------------------------
	//  Suppression
	// --------------------------------------------------------
	protected void TrySuppressShooter(notnull SCR_AIUtilityComponent utility, notnull SCR_AICombatComponent combat, notnull IEntity shooterRoot)
	{
		if (!combat.IsEnemyKnown(shooterRoot))
			return;

		IEntity owner = utility.m_OwnerEntity;

		if (!owner)
			return;

		if (!CanSuppressNow(owner))
			return;

		if (Math.RandomFloat01() >= SUPPRESS_CHANCE)
			return;

		vector shooterOrigin = shooterRoot.GetOrigin();
		float  dist          = vector.Distance(shooterOrigin, utility.GetOrigin());
		float  radius        = Math.Map(dist, 0, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE, SUPPRESS_RADIUS_MIN, SUPPRESS_RADIUS_MAX);

		vector bbMin, bbMax;
		SCR_AISuppressionVolumeBase.CreateSuppressionBox(shooterOrigin, radius, SUPPRESS_BOX_HEIGHT, bbMin, bbMax);

		SCR_AISuppressionObjectVolumeBox volume = new SCR_AISuppressionObjectVolumeBox(bbMin, bbMax);

		if (!volume)
			return;

		SCR_AISuppressBehavior supp = new SCR_AISuppressBehavior(utility, null, volume, SUPPRESS_DURATION_S, SUPPRESS_PRIORITY);

		if (!supp)
			return;

		utility.AddAction(supp);
		MarkSuppressed(owner);
	}

	protected bool CanSuppressNow(IEntity entity)
	{
		if (!entity)
			return false;

		float lastTime_ms;

		if (!s_mLastSuppressTime.Find(entity, lastTime_ms))
			return true;

		return (GetGame().GetWorld().GetWorldTime() - lastTime_ms) > (SUPPRESS_COOLDOWN_S * 1000.0);
	}

	protected void MarkSuppressed(IEntity entity)
	{
		if (!entity)
			return;

		float now_ms = GetGame().GetWorld().GetWorldTime();
		s_mLastSuppressTime.Set(entity, now_ms);

		if (s_mLastSuppressTime.Count() > SUPPRESS_MAP_PRUNE_THRESHOLD)
			PruneSuppressMap(now_ms);
	}

	protected void PruneSuppressMap(float now_ms)
	{
		float staleAge_ms = SUPPRESS_COOLDOWN_S * 1000.0 * 2.0;

		array<IEntity> toRemove = {};

		foreach (IEntity ent, float lastTime_ms : s_mLastSuppressTime)
		{
			if (!ent || (now_ms - lastTime_ms) > staleAge_ms)
				toRemove.Insert(ent);
		}

		foreach (IEntity ent : toRemove)
		{
			s_mLastSuppressTime.Remove(ent);
		}
	}
};