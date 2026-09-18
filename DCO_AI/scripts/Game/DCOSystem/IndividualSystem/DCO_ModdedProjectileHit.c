[BaseContainerProps()]
modded class SCR_AIDangerReaction_ProjectileHit : SCR_AIDangerReaction
{
	protected static const float BULLET_IMPACT_DISTANCE_SQ_MAX = 3*3;
	protected static const float COVER_QUERY_SECTOR_ANGLE_RAD  = 0.35 * Math.PI;
	protected static const float IMPACT_DIST_VERY_CLOSE = 2.0;
	protected static const float IMPACT_DIST_POINT_BLANK = 1.5;
	protected static const float SHOOTER_DIST_CLOSE = 20.0;
	protected static const float SHOOTER_DIST_FAR = 100.0;
	protected static const int PRONE_ROLL_ATTEMPTS = 2;

	protected static const float COVER_PROTECT_MARGIN_S = 1.0;
	protected static const int   PINNED_MAP_PRUNE_THRESHOLD = 128;

	//! Sampai kapan (world time ms) AI dianggap tertekan / pinned
	protected static ref map<IEntity, float> s_mPinnedUntil = new map<IEntity, float>();

	//! Durasi cover move terakhir, dipakai buat nunda suppress sampai AI sampai
	protected float m_fLastCoverMoveDuration_s;


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

	[Attribute("0", UIWidgets.CheckBox, "Coba cari BANGUNAN dulu sebelum cover biasa. Matikan supaya AI langsung cari cover biasa saja.")]
	protected bool m_bCoverPreferBuilding;

	[Attribute("1", UIWidgets.CheckBox, "Peluru mendarat = prioritas tertinggi: tetap bereaksi walau ada combat move lain yang sedang jalan, dan request ini dilindungi dari sistem lain.")]
	protected bool m_bCoverVeryHighPriority;

	[Attribute("3", UIWidgets.EditBox, "Jumlah peluru minimum buat PINNED (nempel tanah, tidak bergerak) pas ditembakin akurat dari penembak jauh.")]
	protected int m_iBulletsPinned;

	[Attribute("4.0", UIWidgets.EditBox, "Durasi (detik) AI tetap tertekan / pinned setelah tembakan akurat dari jauh.")]
	protected float m_fPinnedDuration_s;

	[Attribute("1", UIWidgets.CheckBox, "Hormati gerakan yang sedang jalan: kalau AI lagi menuju cover, jangan diganggu. Cuma ancaman dari ARAH BARU yang boleh menyelak.")]
	protected bool m_bRespectOngoingMove;

	[Attribute("60.0", UIWidgets.EditBox, "Selisih sudut (derajat) antara penembak baru dan arah ancaman yang sedang dihindari, sebelum gerakan yang jalan boleh diselak.")]
	protected float m_fNewThreatAngleDeg;

	[Attribute("1", UIWidgets.CheckBox, "Jangan kirim cover move baru selama cover move dari reaksi ini masih jalan. Nyegah AI mundur terus tiap kena peluru.")]
	protected bool m_bSkipWhileCoverActive;

	[Attribute("5.0", UIWidgets.EditBox, "Jarak waktu minimum (detik) dari gerakan cover terakhir AI ini, termasuk yang dari reaksi tembakan terdengar. Lebih kecil dari punya WeaponFired karena peluru mendarat lebih genting.")]
	protected float m_fCoverMoveCooldown_s;

	[Attribute("6.0", UIWidgets.EditBox, "Jarak maksimum (m) pencarian cover pas REPOSISI (AI sudah di cover, cuma pindah posisi).")]
	protected float m_fRepositionSearchDist;

	[Attribute("10.0", UIWidgets.EditBox, "Jarak maksimum (m) pencarian cover pas KABUR dari posisi tanpa cover.")]
	protected float m_fFleeSearchDist;

	[Attribute("1", UIWidgets.CheckBox, "Paksa stance BERDIRI selama bergerak ke cover, apa pun stance yang diminta rule-nya. Jongkok terlalu lambat.")]
	protected bool m_bForceStandWhileMoving;

	[Attribute("0", UIWidgets.CheckBox, "Biarkan AI nembak (suppress) SELAMA bergerak ke cover. Matikan supaya suppress baru jalan setelah AI sampai.")]
	protected bool m_bSuppressWhileMoving;

	[Attribute("0", UIWidgets.CheckBox, "Biarkan AI tetap membidik penembak selama BERGERAK ke cover. Matikan supaya AI tidak jalan mundur sambil nembak (bidik tetap dipasang di akhir gerakan).")]
	protected bool m_bAimWhileMovingToCover;

	[Attribute("1", UIWidgets.CheckBox, "Reaksi untuk impact di jarak menengah (antara IMPACT_DIST_VERY_CLOSE dan batas 3 m).")]
	protected bool m_bReactToNearMiss;

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
		
		Print("PROJECTILE HIT CHECK PASS");

		vector shooterPos      = shooter.GetOrigin();
		float  distanceToDanger = Math.Sqrt(distanceToDangerSq);
		float  shooterDistance  = vector.Distance(utility.GetOrigin(), shooterRoot.GetOrigin());
		bool   isNullTarget     = utility.m_CombatComponent.GetCurrentTarget() == null;
		bool   inCover          = state.IsInValidCover();
		int    bulletCount      = dangerEventCount;

		IEntity ownerEnt = utility.m_OwnerEntity;

		// Masih tertekan dari impact sebelumnya: tetap nempel tanah, jangan bergerak
		if (IsPinned(ownerEnt))
		{
			KeepProne(charCon);
			return true;
		}

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
			&& distanceToDanger < IMPACT_DIST_POINT_BLANK
			&& bulletCount > thrRepoNoTarget)
		{
			int rollAttempt = PRONE_ROLL_ATTEMPTS * Math.RandomInt(1,3);
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

		// PINNED: tembakan akurat dari penembak jauh -> bergerak justru paling berbahaya
		if (veryClose && shooterDistance > SHOOTER_DIST_FAR && bulletCount > ScaleThreshold(utility, m_iBulletsPinned))
		{
			MarkPinned(ownerEnt, m_fPinnedDuration_s);
			KeepProne(charCon);
			return true;
		}

		// Impact jarak menengah (2-3 m): belum sedekat veryClose, tapi tetap perlu reaksi
		if (m_bReactToNearMiss && !veryClose)
		{
			if (!inCover && bulletCount > thrFleeEngaged)
			{
				return PushCoverMove(utility, state, shooterPos,
					ECharacterStance.STAND, ECharacterStance.CROUCH,
					EMovementType.SPRINT, SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT,
					m_fFleeSearchDist, false, false, true, false, false);
			}

			LowerStance(charCon);
			return true;
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
		// Ancaman dari arah baru = cover yang dituju sekarang belum tentu menutupi, jadi boleh menyelak
		bool threatFromNewDirection = IsThreatFromNewDirection(utility, state, shooterPos);

		// Cover move dari reaksi ini masih jalan -> biarkan selesai, jangan restart pelarian tiap peluru
		if (m_bSkipWhileCoverActive && state.DCO_IsCoverProtected() && !threatFromNewDirection)
			return false;

		// Gerakan lain sedang jalan (menuju cover, manuver, dll) -> hormati, kecuali ancaman dari arah baru
		if (state.IsExecutingRequest())
		{
			if (!m_bCoverVeryHighPriority)
				return false;

			if (m_bRespectOngoingMove && !threatFromNewDirection)
				return false;
		}

		// Budget gerakan bersama dengan reaksi tembakan terdengar, dilewati kalau ancamannya dari arah baru
		if (!threatFromNewDirection && !DCO_CoverMoveBudget.CanMove(utility.m_OwnerEntity, m_fCoverMoveCooldown_s))
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

		// Bergerak jongkok terlalu lambat di bawah tembakan -> selalu berdiri
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

		rq.m_eDirection = ResolveDirection(randomDirection);
		rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;

		// Jalan mundur sambil membidik bikin gerakan aneh dan lambat
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
			rq.m_bFailIfNoCover = false;	// BUILDING + TryFindCover=false + FailIfNoCover=true = gagal diam-diam
		}	

		DCO_CoverMoveBudget.MarkMove(utility.m_OwnerEntity);

		// Prioritas di atas reaksi cover dari WeaponFired, sekaligus melindungi request ini dari sistem lain
		if (m_bCoverVeryHighPriority)
			state.DCO_ApplyCoverRequest(rq, rq.m_fMoveDuration_s + COVER_PROTECT_MARGIN_S);
		else
			state.ApplyNewRequest(rq);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! True kalau penembak sekarang ada di arah yang cukup berbeda dari ancaman yang sedang dihindari.
	//! Kalau arah ancaman yang sedang dihindari tidak diketahui, dianggap BUKAN arah baru (gerakan tetap dihormati).
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

	//------------------------------------------------------------------------------------------------
	//! Tetap tiarap tanpa bergerak
	protected void KeepProne(SCR_CharacterControllerComponent charCon)
	{
		if (!charCon)
			return;

		if (charCon.GetStance() != ECharacterStance.PRONE)
			charCon.SetStanceChange(3);
	}

	//------------------------------------------------------------------------------------------------
	//! Turun satu tingkat stance
	protected void LowerStance(SCR_CharacterControllerComponent charCon)
	{
		if (!charCon)
			return;

		if (charCon.GetStance() == ECharacterStance.STAND)
			charCon.SetStanceChange(2);
		else
			charCon.SetStanceChange(3);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsPinned(IEntity entity)
	{
		if (!entity)
			return false;

		float until_ms;
		if (!s_mPinnedUntil.Find(entity, until_ms))
			return false;

		if (GetGame().GetWorld().GetWorldTime() > until_ms)
		{
			s_mPinnedUntil.Remove(entity);
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected void MarkPinned(IEntity entity, float duration_s)
	{
		if (!entity)
			return;

		float now_ms = GetGame().GetWorld().GetWorldTime();
		s_mPinnedUntil.Set(entity, now_ms + duration_s * 1000.0);

		if (s_mPinnedUntil.Count() > PINNED_MAP_PRUNE_THRESHOLD)
			PrunePinnedMap(now_ms);
	}

	//------------------------------------------------------------------------------------------------
	protected void PrunePinnedMap(float now_ms)
	{
		array<IEntity> toRemove = {};

		foreach (IEntity ent, float until_ms : s_mPinnedUntil)
		{
			if (!ent || now_ms > until_ms)
				toRemove.Insert(ent);
		}

		foreach (IEntity entRemove : toRemove)
		{
			s_mPinnedUntil.Remove(entRemove);
		}
	}

	//------------------------------------------------------------------------------------------------
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

	//! Suppress setelah AI selesai bergerak, supaya tidak lari sambil nembak
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

	//------------------------------------------------------------------------------------------------
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