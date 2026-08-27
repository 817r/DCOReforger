[BaseContainerProps()]
modded class SCR_AIDangerReaction_ProjectileHit : SCR_AIDangerReaction
{
	protected static const float BULLET_IMPACT_DISTANCE_SQ_MAX = 3*3;
	protected static const float COVER_SEARCH_DIST_MAX         = 20.0;
	protected static const float COVER_QUERY_SECTOR_ANGLE_RAD  = 0.35 * Math.PI;
	protected static const float IMPACT_DIST_VERY_CLOSE = 2.0;
	protected static const float IMPACT_DIST_POINT_BLANK = 1.5;
	protected static const float SHOOTER_DIST_CLOSE = 20.0;
	protected static const float SHOOTER_DIST_FAR = 100.0;
	protected static const int PRONE_ROLL_ATTEMPTS = 2;


	[Attribute("2", UIWidgets.EditBox, "Jumlah peluru minimum buat reposisi dari cover pas belum punya target.")]
	protected int m_iBulletsRepositionNoTarget;

	[Attribute("4", UIWidgets.EditBox, "Jumlah peluru minimum buat reposisi cepat walau udah punya target dan penembaknya jauh.")]
	protected int m_iBulletsRepositionHeavy;

	[Attribute("1", UIWidgets.EditBox, "Jumlah peluru minimum buat kabur pas GAK di cover dan belum punya target.")]
	protected int m_iBulletsFleeNoCover;

	[Attribute("3", UIWidgets.EditBox, "Jumlah peluru minimum buat kabur pas GAK di cover walau udah punya target.")]
	protected int m_iBulletsFleeEngaged;

	[Attribute("1", UIWidgets.CheckBox, "Skala threshold di atas pake personality AI (CAUTIOUS bereaksi lebih cepat, RECKLESS lebih lambat).")]
	protected bool m_bScaleThresholdsByPersonality;

	[Attribute("1", UIWidgets.CheckBox, "Lempar smoke pas retreat dari posisi tanpa cover (butuh smoke grenade di inventory).")]
	protected bool m_bDeploySmokeOnFlee;

	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{
		vector impactPos = dangerEvent.GetPosition();
		float distanceToDangerSq = vector.DistanceSq(utility.GetOrigin(), impactPos);

		if (distanceToDangerSq > BULLET_IMPACT_DISTANCE_SQ_MAX)
			return false;

		threatSystem.ThreatBulletImpact(dangerEventCount);

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

		int thrRepoNoTarget = ScaleThreshold(utility, m_iBulletsRepositionNoTarget);
		int thrRepoHeavy    = ScaleThreshold(utility, m_iBulletsRepositionHeavy);
		int thrFleeNoCover  = ScaleThreshold(utility, m_iBulletsFleeNoCover);
		int thrFleeEngaged  = ScaleThreshold(utility, m_iBulletsFleeEngaged);

		bool veryClose  = distanceToDanger < IMPACT_DIST_VERY_CLOSE;
		bool shooterNear = shooterDistance < SHOOTER_DIST_CLOSE;

		if (utility.m_CombatComponent.GetSelectedWeaponType() == EWeaponType.WT_MACHINEGUN
			&& veryClose && isNullTarget)
		{
			TrySuppressShooter(utility, shooterRoot, shooterDistance, 6, 1.5);
		}

		if (veryClose && inCover && isNullTarget && bulletCount > thrRepoNoTarget)
		{
			return PushCoverMove(utility, state, shooterPos,
				ECharacterStance.STAND, ECharacterStance.CROUCH,
				EMovementType.RUN, SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_RUN,
				COVER_SEARCH_DIST_MAX, true, false, false, false, false);
		}

		if (veryClose && inCover && shooterNear && bulletCount > thrRepoNoTarget)
		{
			return PushCoverMove(utility, state, shooterPos,
				ECharacterStance.CROUCH, ECharacterStance.CROUCH,
				EMovementType.RUN, SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN,
				COVER_SEARCH_DIST_MAX / 2, true, false, true, true, false);
		}

		if (veryClose && inCover && bulletCount > thrRepoHeavy)
		{
			bool moved = PushCoverMove(utility, state, shooterPos,
				ECharacterStance.STAND, ECharacterStance.CROUCH,
				EMovementType.RUN, SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT,
				COVER_SEARCH_DIST_MAX, true, true, true, true, false);

			if (moved)
				TrySuppressShooter(utility, shooterRoot, shooterDistance, 5, 1.5);

			return moved;
		}

		if (charCon.GetStance() == ECharacterStance.PRONE
			&& distanceToDanger < IMPACT_DIST_POINT_BLANK
			&& bulletCount > thrRepoNoTarget)
		{
			for (int i = 0; i < PRONE_ROLL_ATTEMPTS; i++)
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
				COVER_SEARCH_DIST_MAX, false, false, false, false, m_bDeploySmokeOnFlee);
		}

		if (veryClose && shooterNear)
		{
			bool moved = PushCoverMove(utility, state, shooterPos,
				ECharacterStance.CROUCH, ECharacterStance.CROUCH,
				EMovementType.WALK, SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN,
				COVER_SEARCH_DIST_MAX / 2, false, true, true, true, false);

			if (moved)
				TrySuppressShooter(utility, shooterRoot, shooterDistance, 5, 1.5);

			return moved;
		}

		if (veryClose && bulletCount > thrFleeEngaged)
		{
			bool moved = PushCoverMove(utility, state, shooterPos,
				ECharacterStance.STAND, ECharacterStance.CROUCH,
				EMovementType.RUN, SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT,
				COVER_SEARCH_DIST_MAX, false, true, true, true, false);

			if (moved)
				TrySuppressShooter(utility, shooterRoot, shooterDistance, 5, 1.5);

			return moved;
		}

		if (shooterDistance > SHOOTER_DIST_FAR)
		{
			if (charCon.GetStance() == ECharacterStance.STAND)
				charCon.SetStanceChange(2);
			else
				charCon.SetStanceChange(3);

			return true;
		}

		return PushCoverMove(utility, state, shooterPos,
			ECharacterStance.CROUCH, ECharacterStance.CROUCH,
			EMovementType.RUN, SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN,
			COVER_SEARCH_DIST_MAX, false, false, true, false, m_bDeploySmokeOnFlee);
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
		if (state.IsExecutingRequest())
			return false;

		if (utility.m_DCOConfig && utility.m_DCOConfig.IsHoldPosition())
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

		rq.m_eStanceMoving = stanceMoving;
		rq.m_eStanceEnd    = stanceEnd;
		rq.m_eMovementType = movementType;

		rq.m_fCoverSearchDistMax = coverSearchDistMax;
		rq.m_fCoverSearchDistMin = 2;
		rq.m_fMoveDuration_s     = Math.RandomFloat(1.0, 1.5) * coverSearchDistMax / speedReference;

		rq.m_eDirection = ResolveDirection(randomDirection);
		rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;

		rq.m_bAimAtTarget    = aimAtTarget;
		rq.m_bAimAtTargetEnd = aimAtTargetEnd;

		if (state.GetOldRequest() && state.GetOldRequest().m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
			rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		else
		{	
			rq.m_eType = SCR_EAICombatMoveRequestType.BUILDING;
			rq.m_bTryFindCover = false;
		}	

		state.ApplyNewRequest(rq);
		return true;
	}

	protected SCR_EAICombatMoveDirection ResolveDirection(bool randomDirection)
	{
		if (!randomDirection)
			return SCR_EAICombatMoveDirection.BACKWARD;

		if (Math.RandomIntInclusive(0, 1) == 0)
			return SCR_EAICombatMoveDirection.BACKWARD;

		if (Math.RandomIntInclusive(0, 1) == 0)
			return SCR_EAICombatMoveDirection.LEFT;

		return SCR_EAICombatMoveDirection.RIGHT;
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

		return Math.Max(1, Math.Round(baseThreshold * scale));
	}
};