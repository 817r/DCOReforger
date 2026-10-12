[BaseContainerProps()]
modded class SCR_AIDangerReaction_DamageTaken
{
	protected static const float COVER_SEARCH_DIST_MAX      = 30.0;




	protected const float COVER_QUERY_SECTOR_ANGLE_RAD = 0.35 * Math.PI;

	protected static ref map<IEntity, float> s_mLastSuppressTime;
	protected static map<IEntity, float> GetLastSuppressTime()
	{
		if (!s_mLastSuppressTime)
			s_mLastSuppressTime = new map<IEntity, float>();
		return s_mLastSuppressTime;
	}

	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{
		IEntity shooter = dangerEvent.GetObject();

		if (!shooter || dangerEvent.GetVictim() != utility.m_OwnerEntity)
			return super.PerformReaction(utility, threatSystem, dangerEvent, dangerEventCount);

		utility.m_SectorThreatFilter.OnDamageTaken(shooter.GetOrigin());

		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());

		if (!agent)
			return true;

		IEntity shooterRoot = shooter.GetRootParent();
		DCO_DodgeAssist.NoteHit(utility, shooterRoot);
		bool known;
		if (shooterRoot && utility.m_CombatComponent)
			known = utility.m_CombatComponent.IsEnemyKnown(shooterRoot);
		DCO_Perf.Count("hit:sector_react");
		if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(utility.m_OwnerEntity), string.Format("hit_react unit=%1 shooter_dist=%2 known=%3",
				DCO_Log.IdUnit(utility.m_OwnerEntity), Math.Round(vector.Distance(utility.GetOrigin(), shooter.GetOrigin())), DCO_Util.B01(known)));

		if (!shooterRoot || !agent.IsEnemy(shooterRoot))
			return true;

		SCR_AICombatMoveState state = utility.m_CombatMoveState;

		if (!state)
			return false;

		state.DCO_MarkIndoorRelocateTrigger();

		SCR_AICombatComponent combat = utility.m_CombatComponent;

		if (!combat)
			return false;

		combat.DCO_NoteNearMiss(shooterRoot.GetOrigin());

		if (utility.m_AIInfo && utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
			return false;

		vector shooterPos    = shooter.GetOrigin();
		float  distToShooter = vector.Distance(utility.GetOrigin(), shooterPos);
		bool   isLongRange   = distToShooter > 60.0;

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

		DCO_AIConfigComponent cfg = utility.m_DCOConfig;

		if (cfg && cfg.IsHoldPosition())
			return true;

		if (DCO_CoverPierced(utility, state, shooterPos))
			return true;

		if (state.IsExecutingRequest())
			return true;

		PushRetreatToCover(state, shooterPos, isLongRange);

		return true;
	}

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
		rq.m_fCoverSearchDistMin            = 2.0;
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
			state.DCO_SetDodgeStance(rq);
		}
		else
		{
			rq.m_eType         = SCR_EAICombatMoveRequestType.BUILDING;
			rq.m_bTryFindCover = false;
		}

		state.ApplyNewRequest(rq);
	}

	protected SCR_EAICombatMoveDirection PickBreakContactDirection()
	{
		int roll = Math.RandomIntInclusive(0, 3);

		if (roll <= 1)
			return SCR_EAICombatMoveDirection.BACKWARD;

		if (roll == 2)
			return SCR_EAICombatMoveDirection.LEFT;

		return SCR_EAICombatMoveDirection.RIGHT;
	}

	protected void TrySuppressShooter(notnull SCR_AIUtilityComponent utility, notnull SCR_AICombatComponent combat, notnull IEntity shooterRoot)
	{
		if (!combat.IsEnemyKnown(shooterRoot))
			return;

		IEntity owner = utility.m_OwnerEntity;

		if (!owner)
			return;

		if (!CanSuppressNow(owner))
			return;

		if (Math.RandomFloat01() >= 0.1)
			return;

		vector shooterOrigin = shooterRoot.GetOrigin();
		float  dist          = vector.Distance(shooterOrigin, utility.GetOrigin());
		float  radius        = Math.Map(dist, 0, SCR_AICombatComponent.LONG_RANGE_COMBAT_DISTANCE, 1.5, 8.0);

		vector bbMin, bbMax;
		SCR_AISuppressionVolumeBase.CreateSuppressionBox(shooterOrigin, radius, 3.0, bbMin, bbMax);

		SCR_AISuppressionObjectVolumeBox volume = new SCR_AISuppressionObjectVolumeBox(bbMin, bbMax);

		if (!volume)
			return;

		SCR_AISuppressBehavior supp = new SCR_AISuppressBehavior(utility, null, volume, 4.0, 1.5);

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

		if (!GetLastSuppressTime().Find(entity, lastTime_ms))
			return true;

		return (GetGame().GetWorld().GetWorldTime() - lastTime_ms) > (25.0 * 1000.0);
	}

	protected void MarkSuppressed(IEntity entity)
	{
		if (!entity)
			return;

		float now_ms = GetGame().GetWorld().GetWorldTime();
		GetLastSuppressTime().Set(entity, now_ms);

		if (GetLastSuppressTime().Count() > 128)
			PruneSuppressMap(now_ms);
	}

	protected void PruneSuppressMap(float now_ms)
	{
		float staleAge_ms = 25.0 * 1000.0 * 2.0;

		array<IEntity> toRemove = {};

		foreach (IEntity ent, float lastTime_ms : GetLastSuppressTime())
		{
			if (!ent || (now_ms - lastTime_ms) > staleAge_ms)
				toRemove.Insert(ent);
		}

		foreach (IEntity ent : toRemove)
		{
			GetLastSuppressTime().Remove(ent);
		}
	}

	protected static ref map<IEntity, float> s_mDCOPierceNext;

	protected bool DCO_CoverPierced(SCR_AIUtilityComponent utility, SCR_AICombatMoveState state, vector shooterPos)
	{
		IEntity me = utility.m_OwnerEntity;
		float now = GetGame().GetWorld().GetWorldTime();
		bool coverHidden = state.m_bInCover && !state.m_bExposedInCover;
		bool fieldHidden = now < state.m_fDCOFieldHiddenUntil_ms;
		if (!coverHidden && !fieldHidden)
			return false;
		if (!s_mDCOPierceNext)
			s_mDCOPierceNext = new map<IEntity, float>();
		if (now < s_mDCOPierceNext.Get(me))
			return false;
		s_mDCOPierceNext.Set(me, now + 8000.0);

		IEntity building = SCR_CoverManagerComponent.DCO_GetBuildingAt(me);
		string where = "outdoor";
		if (building)
		{
			where = "indoor";
			if (!state.DCO_ApplyIndoorRelocate(building, shooterPos, 12.0, 3.0, true, "cover_pierced"))
				return false;
		}
		else
		{
			SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
			rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
			rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
			rq.m_vTargetPos = shooterPos;
			rq.m_vMovePos = shooterPos;
			rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
			rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
			rq.m_eStanceMoving = ECharacterStance.CROUCH;
			rq.m_eStanceEnd = ECharacterStance.CROUCH;
			rq.m_eMovementType = EMovementType.SPRINT;
			rq.m_bAimAtTarget = false;
			rq.m_bAimAtTargetEnd = true;
			rq.m_bTryFindCover = true;
			rq.m_bUseCoverSearchDirectivity = false;
			rq.m_bCheckCoverVisibility = true;
			rq.m_bFailIfNoCover = false;
			rq.m_fCoverSearchDistMin = 5.0;
			rq.m_fCoverSearchDistMax = 15.0;
			rq.m_fMoveDuration_s = 4.0;
			state.DCO_ApplyCoverRequest(rq, 2.0);
		}
		DCO_Perf.Count("cover:pierced_" + where);
		if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("cover_pierced unit=%1 where=%2 shooter_dist=%3",
				DCO_Log.IdUnit(me), where, Math.Round(vector.Distance(me.GetOrigin(), shooterPos))));
		return true;
	}
};