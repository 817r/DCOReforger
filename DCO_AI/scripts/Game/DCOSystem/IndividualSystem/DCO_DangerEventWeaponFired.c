//------------------------------------------------------------------------------------------------
//! Kunci arah tolehan ke sumber tembakan per AI (dipakai SCR_AIDangerReaction_WeaponFired)
class DCO_ShotLookLock
{
	IEntity m_Source;
	vector  m_vPos;
	float   m_fDist;
	float   m_fPriority;
	float   m_fUntil_ms;
	int     m_iToken;
}

//------------------------------------------------------------------------------------------------
//! Bias arah dodge / cover ke waypoint grup. Dipakai reaksi WeaponFired & ProjectileHit.
//! - AI di LUAR radius WP  -> lari ke arah WP (FORWARD ke titik WP)
//! - AI di DALAM radius WP -> arah asli dipakai, KECUALI perkiraan titik akhirnya keluar radius -> dibelokin ke WP
//! Entity waypoint (follow, dll) diabaikan, sama kayak DCO_Move_Investigate_Push.
class DCO_DodgeWaypointUtility
{
	//------------------------------------------------------------------------------------------------
	static AIWaypoint ResolvePositionalWaypoint(notnull SCR_AIUtilityComponent utility)
	{
		AIAgent agent = utility.GetOwner();
		if (!agent)
			return null;

		AIGroup group = agent.GetParentGroup();
		if (!group)
			return null;

		AIWaypoint wp = group.GetCurrentWaypoint();
		if (!wp)
			return null;

		if (SCR_EntityWaypoint.Cast(wp))
			return null;

		return wp;
	}

	//------------------------------------------------------------------------------------------------
	//! Perkiraan kasar titik akhir dodge berdasarkan arah request (relatif ke ancaman)
	static vector ProjectDodgeEndPos(vector myPos, vector threatPos, SCR_EAICombatMoveDirection direction, float dist)
	{
		vector toThreat = threatPos - myPos;
		toThreat[1] = 0;

		float len = toThreat.Length();
		if (len < 0.1)
			return myPos;

		vector fwd   = toThreat * (1.0 / len);
		vector right = Vector(fwd[2], 0, -fwd[0]);

		switch (direction)
		{
			case SCR_EAICombatMoveDirection.FORWARD:  return myPos + fwd * dist;
			case SCR_EAICombatMoveDirection.BACKWARD: return myPos - fwd * dist;
			case SCR_EAICombatMoveDirection.LEFT:     return myPos - right * dist;
			case SCR_EAICombatMoveDirection.RIGHT:    return myPos + right * dist;
		}

		// ANYWHERE / CUSTOM_POS: cover di sekitar posisi sekarang
		return myPos;
	}

	//------------------------------------------------------------------------------------------------
	//! Belokkan request ke WP kalau perlu. Return true kalau request diubah.
	//! Panggil SETELAH m_vTargetPos & m_eDirection di-set, SEBELUM request di-apply.
	static bool ApplyWaypointBias(notnull SCR_AIUtilityComponent utility, notnull SCR_AICombatMoveRequest_Move rq, float searchDist)
	{
		if (!utility.m_OwnerEntity)
			return false;

		AIWaypoint wp = ResolvePositionalWaypoint(utility);
		if (!wp)
			return false;

		vector myPos    = utility.m_OwnerEntity.GetOrigin();
		vector wpPos    = wp.GetOrigin();
		float  wpRadius = wp.GetCompletionRadius();

		bool outside = vector.DistanceXZ(myPos, wpPos) > wpRadius;
		if (!outside)
		{
			vector endPos = ProjectDodgeEndPos(myPos, rq.m_vTargetPos, rq.m_eDirection, searchDist);
			if (vector.DistanceXZ(endPos, wpPos) <= wpRadius)
				return false;
		}

		// m_vTargetPos tetap posisi ancaman -> cover masih dicek terhadap arah tembakan
		rq.m_vMovePos              = wpPos;
		rq.m_eDirection            = SCR_EAICombatMoveDirection.FORWARD;
		rq.m_vAvoidStraightPathDir = vector.Zero;

		return true;
	}
}

[BaseContainerProps()]
modded class SCR_AIDangerReaction_WeaponFired
{
	protected static const float DISMOUNT_DIST_FALLBACK = 700.0;
	protected static const float SUPPRESSED_ROLL_DIST_MIN = 30.0;
	protected static const float SUPPRESSED_ROLL_DIST_MAX = 150.0;
	protected static const float SUPPRESSED_ROLL_CHANCE_AT_MIN = 0.35;
	
	protected static const float DODGE_CHANCE_FALLBACK      = 0.6;
	protected static const float DODGE_COOLDOWN_FALLBACK    = 8.0;
	protected static const float DODGE_MAX_DIST_FALLBACK    = 250.0;
	protected static const float DODGE_SEARCH_DIST_FALLBACK = 30.0;

	protected bool m_bScaleDodgeByPersonality = true;

	protected static ref map<IEntity, float> s_mLastDodgeTime = new map<IEntity, float>();
	
	protected static const float coverSearchDistMax = 20;
	
	protected static const float COVER_QUERY_SECTOR_ANGLE_RAD  = 0.51 * Math.PI;
	
	protected static ref map<IEntity, float> s_mLastInvestigateTime = new map<IEntity, float>();
	protected static const int INVESTIGATE_MAP_PRUNE_THRESHOLD = 128;

	//------------------------------------------------------------------------------------------------
	// REALISTIC REACTION — konstanta
	//------------------------------------------------------------------------------------------------
	protected static const float HEAD_HEIGHT                   = 1.6;
	protected static const float MISS_DIST_DIRECT              = 2.0;   // <= ini dianggap dibidik langsung (missScore = 1)
	protected static const float MISS_DIST_IRRELEVANT          = 25.0;  // >= ini tembakan bukan untuk AI ini (missScore = 0)
	protected static const float MISS_DIST_NOT_TOWARD_ME       = 9999.0;

	protected static const float THREAT_WEIGHT_MISS            = 0.6;
	protected static const float THREAT_WEIGHT_PROXIMITY       = 0.25;
	protected static const float THREAT_WEIGHT_BURST           = 0.15;
	protected static const float THREAT_BURST_SATURATION       = 5.0;   // jumlah event yang dianggap "burst penuh"

	protected static const int   BUILDING_FALLBACK_POLL_MS     = 250;
	protected static const int   BUILDING_FALLBACK_MAX_POLLS   = 8;     // total ~2 detik nunggu hasil pencarian bangunan

	//------------------------------------------------------------------------------------------------
	// COVER PROTECT — lama waktu request cover dilindungi di combat move state
	//------------------------------------------------------------------------------------------------
	protected static const float COVER_MOVE_HOLD_MARGIN_S      = 1.0;   // tambahan hold setelah durasi move
	protected static const float COVER_BUILDING_HOLD_MARGIN_S  = 0.25;  // tambahan hold setelah jendela poll bangunan

	//! Salinan flag debug supaya bisa dibaca class lain (combat move state, observe behavior)
	protected static bool s_bDCOCoverDebug;

	//! Kunci arah tolehan aktif per AI
	protected static ref map<IEntity, ref DCO_ShotLookLock> s_mLookLocks = new map<IEntity, ref DCO_ShotLookLock>();
	protected static const int LOOK_LOCK_MAP_PRUNE_THRESHOLD = 128;

	[Attribute("50.0", UIWidgets.EditBox, "Jarak maksimum (m) tembakan SENYAP yang bikin AI mau maju investigasi.")]
	protected float m_fSuppressedInvestigateDist;

	[Attribute("25.0", UIWidgets.EditBox, "Radius area investigasi.")]
	protected float m_fInvestigateRadius;

	[Attribute("150.0", UIWidgets.EditBox, "Durasi behavior investigasi (detik).")]
	protected float m_fInvestigateDuration;

	[Attribute("20.0", UIWidgets.EditBox, "Cooldown (detik) sebelum AI yang sama boleh dikasih behavior investigasi lagi. Nyegah numpuk pas ditembakin beruntun.")]
	protected float m_fInvestigateCooldown;

	//------------------------------------------------------------------------------------------------
	// REALISTIC REACTION — attribute
	//------------------------------------------------------------------------------------------------
	[Attribute("1", UIWidgets.CheckBox, "Pakai model reaksi realistis (jangkauan dengar, noleh berbasis score, cari cover prioritas bangunan). Kalau mati, balik ke IsAudiable + TryDodge lama.")]
	protected bool m_bUseRealisticReaction;

	[Attribute("180.0", UIWidgets.EditBox, "Jangkauan dengar (m) tembakan SENYAP. Tembakan normal pakai AUDIBLE_DISTANCE_NORMAL vanilla.")]
	protected float m_fSuppressedAudibleRange;

	[Attribute("0.5", UIWidgets.EditBox, "Pengali jangkauan dengar kalau AI di dalam kendaraan (0.5 = kira-kira -6 dB).")]
	protected float m_fInVehicleHearingFactor;

	[Attribute("0.3", UIWidgets.EditBox, "Audibility >= nilai ini = pasti dengar. Di bawahnya peluang = audibility / nilai ini.")]
	protected float m_fAudibleGuaranteeThreshold;

	[Attribute("5.0", UIWidgets.EditBox, "Error arah minimum (derajat) saat AI memperkirakan posisi penembak.")]
	protected float m_fDirectionErrorBaseDeg;

	[Attribute("20.0", UIWidgets.EditBox, "Tambahan error arah maksimum (derajat) saat tembakan hampir tidak terdengar.")]
	protected float m_fDirectionErrorExtraDeg;

	[Attribute("60.0", UIWidgets.EditBox, "Jarak (m) di mana AI pasti noleh ke arah tembakan.")]
	protected float m_fLookDistFull;

	[Attribute("400.0", UIWidgets.EditBox, "Jarak (m) maksimum AI mau noleh. Di atas ini tidak noleh.")]
	protected float m_fLookDistMax;

	[Attribute("0.2", UIWidgets.EditBox, "Peluang dasar noleh di jarak maksimum.")]
	protected float m_fLookChanceAtMax;

	[Attribute("0.5", UIWidgets.EditBox, "Bonus peluang noleh berdasarkan threat score (0..1 dikali nilai ini).")]
	protected float m_fLookThreatBonus;

	[Attribute("0.15", UIWidgets.EditBox, "Delay reaksi noleh minimum (detik) setelah suara sampai.")]
	protected float m_fLookDelayMin;

	[Attribute("0.35", UIWidgets.EditBox, "Delay reaksi noleh maksimum (detik) setelah suara sampai.")]
	protected float m_fLookDelayMax;

	[Attribute("0.2", UIWidgets.EditBox, "Delay minimum (detik) sebelum AI mulai lari ke cover.")]
	protected float m_fCoverDelayMin;

	[Attribute("0.6", UIWidgets.EditBox, "Delay maksimum (detik) sebelum AI mulai lari ke cover.")]
	protected float m_fCoverDelayMax;

	[Attribute("0.7", UIWidgets.EditBox, "Threat score >= nilai ini: fallback cover (non-bangunan) berakhir tiarap, bukan jongkok.")]
	protected float m_fHighThreatProneThreshold;

	[Attribute("1", UIWidgets.CheckBox, "Setelah sampai di cover, AI noleh lagi ke arah perkiraan tembakan (tolehan awal biasanya sudah kalah sama gerakan).")]
	protected bool m_bLookAfterCover;

	[Attribute("3", UIWidgets.EditBox, "Berapa kali tolehan setelah sampai di cover diulang, supaya pandangan bertahan.")]
	protected int m_iLookAfterCoverRepeats;

	[Attribute("1.0", UIWidgets.EditBox, "Jeda (detik) antar tolehan setelah sampai di cover.")]
	protected float m_fLookAfterCoverInterval;

	[Attribute("0.3", UIWidgets.EditBox, "Jeda tambahan (detik) setelah gerakan selesai sebelum AI noleh ke arah tembakan.")]
	protected float m_fLookAfterCoverDelay;

	//------------------------------------------------------------------------------------------------
	// LOOK PRIORITY — noleh ke sumber terdekat + kunci arah
	//------------------------------------------------------------------------------------------------
	[Attribute("1", UIWidgets.CheckBox, "Noleh diprioritaskan ke sumber tembakan TERDEKAT. Arah tolehan dikunci selama Look Lock Duration; tembakan yang lebih jauh tidak bisa merebut arah.")]
	protected bool m_bLookPreferClosest;

	[Attribute("3.0", UIWidgets.EditBox, "Lama (detik) arah tolehan dikunci setelah dengar tembakan. Tembakan lanjutan dari penembak yang sama memperpanjang kunci.")]
	protected float m_fLookLockDuration;

	[Attribute("0.8", UIWidgets.EditBox, "Sumber baru boleh merebut arah kalau jaraknya <= nilai ini x jarak sumber yang sedang dikunci. Di bawah 1 supaya tidak geleng-geleng antara dua penembak yang jaraknya mirip.")]
	protected float m_fLookOverrideDistRatio;

	[Attribute("0.5", UIWidgets.EditBox, "Interval (detik) arah tolehan dipasang ulang selama terkunci, supaya orientasi tidak lepas.")]
	protected float m_fLookLockRefreshInterval;

	[Attribute("10.0", UIWidgets.EditBox, "Bonus prioritas look maksimum (di jarak 0), menurun linear sampai 0 di Look Dist Max. Dibatasi di bawah PRIO_UNKNOWN_TARGET supaya tidak mengalahkan target asli.")]
	protected float m_fLookPriorityProximityBonus;

	[Attribute("12.0", UIWidgets.EditBox, "Batas atas (m) jarak pencarian cover, membatasi nilai dari DodgeSearchDist supaya AI tidak sprint jauh-jauh.")]
	protected float m_fCoverSearchDistCap;

	[Attribute("0.4", UIWidgets.EditBox, "Threat score minimum supaya AI mau lari ke cover. Di bawah ini AI cuma noleh dan turun stance.")]
	protected float m_fCoverThreatGate;

	[Attribute("1", UIWidgets.CheckBox, "Turunkan stance kalau threat di bawah gate (bukan lari ke cover).")]
	protected bool m_bLowerStanceOnLowThreat;

	[Attribute("8.0", UIWidgets.EditBox, "Jarak waktu minimum (detik) dari gerakan cover terakhir AI ini, termasuk yang dari reaksi peluru mendarat.")]
	protected float m_fSharedMoveCooldown;

	[Attribute("0", UIWidgets.CheckBox, "Coba cari BANGUNAN dulu sebelum cover biasa. Matikan supaya AI langsung cari cover biasa saja.")]
	protected bool m_bCoverPreferBuilding;

	[Attribute("1", UIWidgets.CheckBox, "Cover = VERY HIGH PRIORITY: tanpa roll chance, menimpa combat move yang sedang jalan, dan request sistem lain ditolak selama AI lari ke cover.")]
	protected bool m_bCoverVeryHighPriority;

	[Attribute("1", UIWidgets.CheckBox, "Roll peluang lari ke cover pakai DodgeChance dari DCO config (+ skala personality kalau aktif), walau Cover Very High Priority nyala. Very High Priority tetap berlaku buat menimpa & proteksi request, cuma gak lagi maksa chance = 1.")]
	protected bool m_bUseDodgeChanceOnCover;

	//------------------------------------------------------------------------------------------------
	// ARAH GERAK COVER
	//------------------------------------------------------------------------------------------------
	[Attribute("1", UIWidgets.CheckBox, "Arah lari ke cover diacak pakai bobot di bawah (mundur / kiri / kanan / bebas). Matikan supaya balik ke perilaku lama.")]
	protected bool m_bRandomizeCoverDirection;

	[Attribute("1.0", UIWidgets.EditBox, "Bobot arah MUNDUR (menjauhi ancaman).")]
	protected float m_fCoverDirWeightBackward;

	[Attribute("1.0", UIWidgets.EditBox, "Bobot arah KIRI (menyamping relatif ke ancaman).")]
	protected float m_fCoverDirWeightLeft;

	[Attribute("1.0", UIWidgets.EditBox, "Bobot arah KANAN (menyamping relatif ke ancaman).")]
	protected float m_fCoverDirWeightRight;

	[Attribute("1.0", UIWidgets.EditBox, "Bobot arah BEBAS (ANYWHERE, cover terdekat dari arah mana pun).")]
	protected float m_fCoverDirWeightAnywhere;

	[Attribute("1", UIWidgets.CheckBox, "Bobot arah di atas dikali skala personality (CAUTIOUS lebih sering mundur, AGGRESSIVE/RECKLESS lebih sering menyamping/maju). Matikan supaya semua personality pakai bobot yang sama.")]
	protected bool m_bScaleCoverDirByPersonality;

	[Attribute("1.0", UIWidgets.EditBox, "Pengali bobot arah MAJU dari personality (AGGRESSIVE 0.5, RECKLESS 1.0, lainnya 0). 0 = tidak pernah maju. Maju cuma dipakai untuk cover biasa (bukan bangunan) dan saat threat di bawah High Threat Prone Threshold.")]
	protected float m_fCoverDirForwardWeightScale;

	[Attribute("1", UIWidgets.CheckBox, "Kalau grup punya waypoint posisi, dodge/cover diarahkan ke waypoint. Di luar radius WP -> lari ke WP. Di dalam radius -> arah biasa, kecuali bakal keluar radius -> dibelokin ke WP.")]
	protected bool m_bDodgeTowardWaypoint;

	//------------------------------------------------------------------------------------------------
	// DEBUG
	//------------------------------------------------------------------------------------------------
	[Attribute("0", UIWidgets.CheckBox, "Debug: print log reaksi tembakan (dengar, noleh, cover, fallback bangunan).")]
	protected bool m_bDebugCoverReaction;

	[Attribute("100.0", UIWidgets.EditBox, "Debug: hanya log tembakan yang jaraknya <= nilai ini (m), supaya log tidak banjir.")]
	protected float m_fDebugMaxDist;

	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{
		AIDangerEventWeaponFire eventWeaponFire = AIDangerEventWeaponFire.Cast(dangerEvent);
		if (!eventWeaponFire)
			return false;

		IEntity shooter = eventWeaponFire.GetObject();
		if (!shooter)
			return false;

		IEntity instigatorEntity = eventWeaponFire.GetInstigatorEntity();
		if (!instigatorEntity)
			return false;

		Faction instigatorFaction = SCR_AIFactionHandling.GetEntityPerceivedFaction(instigatorEntity);
		if (!instigatorFaction)
			return false;

		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
		if (!agent)
			return false;

		bool myFactionIsMilitary = utility.IsMilitary();
		if (myFactionIsMilitary && !agent.IsEnemy(instigatorFaction))
			return false;

		if (!utility.m_OwnerEntity)
			return false;

		vector shotPos          = eventWeaponFire.GetPosition();
		vector shotDir          = eventWeaponFire.GetDirection();
		bool   isShotSuppressed = eventWeaponFire.IsSuppressed();
		vector myOrigin         = utility.m_OwnerEntity.GetOrigin();
		float  distance         = vector.Distance(myOrigin, shotPos);
		float  distanceSQ       = vector.DistanceSq(myOrigin, shotPos);

		float dismountDist = DISMOUNT_DIST_FALLBACK;
		if (utility.m_DCOConfig)
			dismountDist = utility.m_DCOConfig.GetDismountDistance();

		float dismountDistSq = dismountDist * dismountDist;

		bool isFlyby = IsFlyby(myOrigin, shotPos, shotDir, distance);
		bool endangeringForGroup = isFlyby || distance < ENDANGERING_FOR_GROUP_RADIUS;
		bool isInVehicle = utility.m_AIInfo && utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE);

		if (isInVehicle)
		{
			if (distanceSQ > dismountDistSq)
				return false;

			if (utility.m_AIInfo.HasUnitState(EUnitState.PILOT))
			{

			}
			else if (utility.m_AIInfo.HasUnitState(EUnitState.IN_TURRET))
			{

			}
			else
			{
				CompartmentAccessComponent compartmentAccess = CompartmentAccessComponent.Cast(utility.m_OwnerEntity.FindComponent(CompartmentAccessComponent));
				if (!compartmentAccess)
					return false;

				if (!compartmentAccess.IsInCompartment())
					return false;

				SCR_AIGetOutVehicle getOutAction = new SCR_AIGetOutVehicle(utility, null, compartmentAccess.GetOwner(), priority: SCR_AIActionBase.PRIORITY_BEHAVIOR_GET_OUT_VEHICLE_HIGH_PRIORITY);
				utility.AddAction(getOutAction);
				
				SCR_AICombatMoveState state = utility.m_CombatMoveState;
				
				if (state.IsExecutingRequest())
					return false;
		
				if (utility.m_DCOConfig && utility.m_DCOConfig.IsHoldPosition())
					return false;
				
				SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

				rq.m_eReason  = SCR_EAICombatMoveReason.STANDARD;
				rq.m_vTargetPos = shotPos;
				rq.m_vMovePos   = rq.m_vTargetPos;
		
				rq.m_bTryFindCover              = true;
				rq.m_bUseCoverSearchDirectivity = true;
				rq.m_bCheckCoverVisibility      = true;
				rq.m_bFailIfNoCover             = false;
		
				rq.m_eStanceMoving = ECharacterStance.STAND;
				rq.m_eStanceEnd    = ECharacterStance.CROUCH;
				rq.m_eMovementType = EMovementType.SPRINT;
		
				rq.m_fCoverSearchDistMax = coverSearchDistMax;
				rq.m_fCoverSearchDistMin = 2;
				rq.m_fMoveDuration_s     = Math.RandomFloat(1.0, 1.5) * coverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;
		
				rq.m_eDirection = ResolveCoverDirectionForUnit(utility, false);
				rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
		
				rq.m_bAimAtTarget    = false;
				rq.m_bAimAtTargetEnd = true;
		
				if (state.GetOldRequest() && state.GetOldRequest().m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
					rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
				else
				{	
					rq.m_eType = SCR_EAICombatMoveRequestType.BUILDING;
					rq.m_bTryFindCover = false;
				}	
		
				ApplyWaypointDodgeBias(utility, rq, coverSearchDistMax);

				state.ApplyNewRequest(rq);

				return true;
			}
		}

		s_bDCOCoverDebug = m_bDebugCoverReaction;

		// Model dengar: realistis (jangkauan efektif) atau lama (IsAudiable)
		float audibility = 0;
		bool isAudible;
		if (m_bUseRealisticReaction)
		{
			audibility = ComputeAudibility(distance, isShotSuppressed, isInVehicle);
			isAudible  = RollAudible(audibility);
		}
		else
		{
			isAudible = IsAudiable(distance, isShotSuppressed);
		}

		if (!isFlyby && !isAudible)
			return false;

		// Perkiraan posisi penembak + threat score, dihitung sekali di sini supaya konsisten untuk semua reaksi yang ditunda
		vector perceivedShotPos = shotPos;
		float  threatScore      = 0;
		if (m_bUseRealisticReaction && isAudible)
		{
			perceivedShotPos = ComputePerceivedShotPos(myOrigin, shotPos, audibility);

			float missDist = ComputeMissDistance(myOrigin, shotPos, shotDir);
			threatScore    = ComputeThreatScore(missDist, audibility, dangerEventCount);

			if (IsDebugOn(distance))
			{
				DebugCover(utility.m_OwnerEntity, string.Format("PERCEIVE dist=%1 suppressed=%2 flyby=%3 audibility=%4 miss=%5 events=%6 threat=%7 perceivedErr=%8m",
					distance, isShotSuppressed, isFlyby, audibility, missDist, dangerEventCount, threatScore, vector.Distance(perceivedShotPos, shotPos)));
			}
		}
		else if (m_bUseRealisticReaction && IsDebugOn(distance))
		{
			DebugCover(utility.m_OwnerEntity, string.Format("PERCEIVE dist=%1 NOT AUDIBLE (audibility=%2) flyby=%3 -> tidak ada reaksi cover", distance, audibility, isFlyby));
		}

		float timeTillFlyby_s = float.MAX;
		float timeTillGunshotHeard_s = float.MAX;

		if (isFlyby)
		{
			float projectileSpeed = eventWeaponFire.GetInitialSpeed();

			if (projectileSpeed <= 0)
			{
				OnProjectileFlyby(utility, dangerEventCount, shotPos);
				timeTillFlyby_s = 0;
			}
			else
			{
				WorldTimestamp eventTimestamp = eventWeaponFire.GetTimestamp();
				float flightTime_s = distance / projectileSpeed;
				WorldTimestamp flybyTimestamp = eventTimestamp.PlusSeconds(flightTime_s + PROJECTILE_FLYBY_DELAY_S);
				timeTillFlyby_s = flybyTimestamp.DiffSeconds(GetGame().GetWorld().GetTimestamp());

				if (timeTillFlyby_s < 0)
					OnProjectileFlyby(utility, dangerEventCount, shotPos);
				else
				{
					utility.GetCallqueue().CallLater(OnProjectileFlyby, 1000*timeTillFlyby_s, false,
						utility, dangerEventCount, shotPos);
				}
			}
		}

		if (isAudible)
		{
			WorldTimestamp eventTimestamp = eventWeaponFire.GetTimestamp();
			float wavefrontTravelTime_s = distance / SOUND_SPEED_MS;
			WorldTimestamp wavefrontArrivalTimestamp = eventTimestamp.PlusSeconds(wavefrontTravelTime_s);
			timeTillGunshotHeard_s = wavefrontArrivalTimestamp.DiffSeconds(GetGame().GetWorld().GetTimestamp());

			bool ignoreGunshotHeard = isFlyby && timeTillFlyby_s < timeTillGunshotHeard_s;

			if (m_bUseRealisticReaction && IsDebugOn(distance))
			{
				DebugCover(utility.m_OwnerEntity, string.Format("TIMING flyby_in=%1s heard_in=%2s ignoreHeard=%3 endangeringGroup=%4 executingNow=%5",
					timeTillFlyby_s, timeTillGunshotHeard_s, ignoreGunshotHeard, endangeringForGroup, utility.m_CombatMoveState && utility.m_CombatMoveState.IsExecutingRequest()));
			}

			if (!ignoreGunshotHeard || endangeringForGroup)
			{
				if (timeTillGunshotHeard_s < 0)
				{
					OnGunshotHeard(utility, distance, dangerEventCount, shotPos);

					if (m_bUseRealisticReaction)
						ReactToGunshot(utility, perceivedShotPos, distance, threatScore, instigatorEntity);
					else
						TryDodge(utility, shotPos, distance);
				}
				else
				{
					utility.GetCallqueue().CallLater(OnGunshotHeard, 1000*timeTillGunshotHeard_s, false,
						utility, distance, dangerEventCount, shotPos);

					if (m_bUseRealisticReaction)
					{
						utility.GetCallqueue().CallLater(ReactToGunshot, 1000*timeTillGunshotHeard_s, false,
							utility, perceivedShotPos, distance, threatScore, instigatorEntity);
					}
					else
					{
						utility.GetCallqueue().CallLater(TryDodge, 1000*timeTillGunshotHeard_s, false,
							utility, shotPos, distance);
					}
				}
			}
		}

		AIGroup myGroup = utility.GetOwner().GetParentGroup();
		if (myGroup)
		{
			float timeTillGroupNotified_s = Math.Min(timeTillFlyby_s, timeTillGunshotHeard_s);

			if (timeTillGroupNotified_s < 0)
				NotifyGroup(myGroup, shooter, instigatorEntity, instigatorFaction, shotPos, endangeringForGroup);
			else
			{
				utility.GetCallqueue().CallLater(NotifyGroup, 1000*timeTillGroupNotified_s, false,
					myGroup, shooter, instigatorEntity, instigatorFaction, shotPos, endangeringForGroup);
			}
		}

		return true;
	}

	protected bool IsAudiable(float dist, bool isSuppressed)
	{
		if (!isSuppressed)
			return dist < AUDIBLE_DISTANCE_NORMAL;
 
		if (dist <= SUPPRESSED_ROLL_DIST_MIN)
			return true;
 
		if (dist >= SUPPRESSED_ROLL_DIST_MAX)
			return false;
 
		float t = (dist - SUPPRESSED_ROLL_DIST_MIN) / (SUPPRESSED_ROLL_DIST_MAX - SUPPRESSED_ROLL_DIST_MIN);
		float chance = Math.Lerp(SUPPRESSED_ROLL_CHANCE_AT_MIN, 0.0, t);
 
		return Math.RandomFloat01() < chance;
	}

	//------------------------------------------------------------------------------------------------
	static bool DCO_IsCoverDebugOn()
	{
		return s_bDCOCoverDebug;
	}

	//------------------------------------------------------------------------------------------------
	// DEBUG — helper
	//------------------------------------------------------------------------------------------------
	protected bool IsDebugOn(float distance)
	{
		return m_bDebugCoverReaction && distance <= m_fDebugMaxDist;
	}

	//------------------------------------------------------------------------------------------------
	protected void DebugCover(IEntity ent, string msg)
	{
		string entName = "null";
		if (ent)
			entName = ent.ToString();

		Print(string.Format("[DCO_Cover] t=%1 %2 | %3", GetGame().GetWorld().GetWorldTime(), entName, msg), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected string DebugFailReason(SCR_AICombatMoveRequestBase rq)
	{
		if (!rq)
			return "none";

		return typename.EnumToString(SCR_EAICombatMoveRequestFailReason, rq.m_eFailReason);
	}

	//------------------------------------------------------------------------------------------------
	// REALISTIC REACTION — perhitungan
	//------------------------------------------------------------------------------------------------

	//! 1 = sangat jelas (dekat), 0 = di luar jangkauan dengar
	protected float ComputeAudibility(float dist, bool isSuppressed, bool isInVehicle)
	{
		float rangeEff = AUDIBLE_DISTANCE_NORMAL;
		if (isSuppressed)
			rangeEff = m_fSuppressedAudibleRange;

		if (isInVehicle)
			rangeEff *= m_fInVehicleHearingFactor;

		if (rangeEff <= 0)
			return 0;

		return Math.Clamp(1.0 - (dist / rangeEff), 0.0, 1.0);
	}

	//------------------------------------------------------------------------------------------------
	protected bool RollAudible(float audibility)
	{
		if (audibility <= 0)
			return false;

		if (m_fAudibleGuaranteeThreshold <= 0 || audibility >= m_fAudibleGuaranteeThreshold)
			return true;

		float chance = audibility / m_fAudibleGuaranteeThreshold;
		return Math.RandomFloat01() < chance;
	}

	//------------------------------------------------------------------------------------------------
	//! Posisi penembak menurut AI: shotPos diputar di sekitar AI (sumbu Y) dengan error sudut
	protected vector ComputePerceivedShotPos(vector myOrigin, vector shotPos, float audibility)
	{
		float errorDeg = m_fDirectionErrorBaseDeg + (1.0 - audibility) * m_fDirectionErrorExtraDeg;
		if (errorDeg <= 0)
			return shotPos;

		float angleRad = Math.RandomFloat(-errorDeg, errorDeg) * Math.DEG2RAD;
		float c = Math.Cos(angleRad);
		float s = Math.Sin(angleRad);

		vector offset = shotPos - myOrigin;
		float rx = offset[0] * c - offset[2] * s;
		float rz = offset[0] * s + offset[2] * c;

		return myOrigin + Vector(rx, offset[1], rz);
	}

	//------------------------------------------------------------------------------------------------
	//! Jarak tegak lurus dari kepala AI ke lintasan peluru. MISS_DIST_NOT_TOWARD_ME kalau AI di belakang penembak.
	protected float ComputeMissDistance(vector myOrigin, vector shotPos, vector shotDir)
	{
		float dirLen = shotDir.Length();
		if (dirLen < 0.001)
			return MISS_DIST_NOT_TOWARD_ME;

		vector dir     = shotDir * (1.0 / dirLen);
		vector headPos = myOrigin + Vector(0, HEAD_HEIGHT, 0);
		vector toMe    = headPos - shotPos;

		float t = vector.Dot(toMe, dir);
		if (t < 0)
			return MISS_DIST_NOT_TOWARD_ME;

		vector perpendicular = toMe - dir * t;
		return perpendicular.Length();
	}

	//------------------------------------------------------------------------------------------------
	//! 0..1 — gabungan seberapa dekat peluru ke AI, seberapa dekat penembak, dan jumlah tembakan
	protected float ComputeThreatScore(float missDist, float audibility, int dangerEventCount)
	{
		float missScore = 0;
		if (missDist <= MISS_DIST_DIRECT)
			missScore = 1.0;
		else if (missDist < MISS_DIST_IRRELEVANT)
			missScore = 1.0 - (missDist - MISS_DIST_DIRECT) / (MISS_DIST_IRRELEVANT - MISS_DIST_DIRECT);

		float burstScore = Math.Clamp(dangerEventCount / THREAT_BURST_SATURATION, 0.0, 1.0);

		float threat = missScore * THREAT_WEIGHT_MISS
					 + audibility * THREAT_WEIGHT_PROXIMITY
					 + burstScore * THREAT_WEIGHT_BURST;

		return Math.Clamp(threat, 0.0, 1.0);
	}

	//------------------------------------------------------------------------------------------------
	protected float ComputeLookChance(float distance, float threatScore)
	{
		if (distance > m_fLookDistMax)
			return 0;

		float baseChance = 1.0;
		if (distance > m_fLookDistFull && m_fLookDistMax > m_fLookDistFull)
		{
			float t = (distance - m_fLookDistFull) / (m_fLookDistMax - m_fLookDistFull);
			baseChance = Math.Lerp(1.0, m_fLookChanceAtMax, t);
		}

		return Math.Clamp(baseChance + threatScore * m_fLookThreatBonus, 0.0, 1.0);
	}

	//------------------------------------------------------------------------------------------------
	// REALISTIC REACTION — eksekusi
	//------------------------------------------------------------------------------------------------

	//! Dipanggil saat suara tembakan sampai. Jadwalkan noleh (refleks) lalu cari cover.
	protected void ReactToGunshot(SCR_AIUtilityComponent utility, vector perceivedShotPos, float distance, float threatScore, IEntity sourceEntity = null)
	{
		if (!utility || !utility.m_OwnerEntity)
			return;

		bool dbg = IsDebugOn(distance);

		float lookChance = ComputeLookChance(distance, threatScore);
		float lookRoll   = Math.RandomFloat01();
		int lookDelay_ms = -1;
		if (lookRoll < lookChance)
		{
			lookDelay_ms = (int)(Math.RandomFloat(m_fLookDelayMin, m_fLookDelayMax) * 1000.0);
			if (m_bLookPreferClosest)
				utility.GetCallqueue().CallLater(LookAtShotLocked, lookDelay_ms, false, utility, perceivedShotPos, distance, sourceEntity);
			else
				utility.GetCallqueue().CallLater(LookAtShot, lookDelay_ms, false, utility, perceivedShotPos);
		}

		int coverDelay_ms = (int)(Math.RandomFloat(m_fCoverDelayMin, m_fCoverDelayMax) * 1000.0);
		utility.GetCallqueue().CallLater(TryTakeCoverFromShot, coverDelay_ms, false, utility, perceivedShotPos, distance, threatScore);

		if (dbg)
		{
			DebugCover(utility.m_OwnerEntity, string.Format("HEARD dist=%1 threat=%2 lookChance=%3 roll=%4 lookDelay=%5ms coverDelay=%6ms executingNow=%7",
				distance, threatScore, lookChance, lookRoll, lookDelay_ms, coverDelay_ms, utility.m_CombatMoveState && utility.m_CombatMoveState.IsExecutingRequest()));
		}
	}

	//------------------------------------------------------------------------------------------------
	// LOOK PRIORITY — eksekusi
	//------------------------------------------------------------------------------------------------

	//! Prioritas look: makin dekat makin tinggi, tapi tidak melewati PRIO_UNKNOWN_TARGET
	protected float ComputeLookPriority(float distance)
	{
		float basePrio = SCR_AILookAction.PRIO_DANGER_EVENT;

		float closeness = 1.0;
		if (m_fLookDistMax > 0)
			closeness = Math.Clamp(1.0 - (distance / m_fLookDistMax), 0.0, 1.0);

		float prio = basePrio + closeness * m_fLookPriorityProximityBonus;

		float cap = SCR_AILookAction.PRIO_UNKNOWN_TARGET - 0.1;
		if (cap > basePrio)
			return Math.Min(prio, cap);

		return basePrio;
	}

	//------------------------------------------------------------------------------------------------
	//! Noleh dengan arbitrase jarak: sumber terdekat menang, arah dikunci dan dipasang ulang berkala.
	protected void LookAtShotLocked(SCR_AIUtilityComponent utility, vector perceivedShotPos, float distance, IEntity sourceEntity)
	{
		if (!utility || !utility.m_OwnerEntity || !utility.m_LookAction)
			return;

		IEntity ownerEnt = utility.m_OwnerEntity;
		float now_ms = GetGame().GetWorld().GetWorldTime();

		DCO_ShotLookLock lock = s_mLookLocks.Get(ownerEnt);
		bool active = false;
		if (lock)
			active = now_ms < lock.m_fUntil_ms;

		if (active)
		{
			bool sameSource = sourceEntity && lock.m_Source == sourceEntity;

			// Sumber lain yang tidak cukup dekat -> arah tetap terkunci ke sumber sekarang
			if (!sameSource && distance > lock.m_fDist * m_fLookOverrideDistRatio)
			{
				if (m_bDebugCoverReaction)
					DebugCover(ownerEnt, string.Format("LOOK SKIP: terkunci ke sumber %1m, sumber baru %2m (ratio %3)", lock.m_fDist, distance, m_fLookOverrideDistRatio));
				return;
			}

			// Penembak yang sama nembak lagi -> perbarui posisi & perpanjang kunci, loop refresh yang jalan dipakai terus
			if (sameSource)
			{
				lock.m_vPos      = perceivedShotPos;
				lock.m_fDist     = distance;
				lock.m_fPriority = ComputeLookPriority(distance);
				lock.m_fUntil_ms = now_ms + m_fLookLockDuration * 1000.0;

				utility.m_LookAction.LookAt(lock.m_vPos, lock.m_fPriority);

				if (m_bDebugCoverReaction)
					DebugCover(ownerEnt, string.Format("LOOK EXTEND: sumber sama %1m prio=%2", distance, lock.m_fPriority));
				return;
			}
		}

		if (!lock)
		{
			lock = new DCO_ShotLookLock();
			s_mLookLocks.Set(ownerEnt, lock);

			if (s_mLookLocks.Count() > LOOK_LOCK_MAP_PRUNE_THRESHOLD)
				PruneLookLockMap(now_ms);
		}

		lock.m_Source    = sourceEntity;
		lock.m_vPos      = perceivedShotPos;
		lock.m_fDist     = distance;
		lock.m_fPriority = ComputeLookPriority(distance);
		lock.m_fUntil_ms = now_ms + m_fLookLockDuration * 1000.0;
		lock.m_iToken++;

		utility.m_LookAction.LookAt(lock.m_vPos, lock.m_fPriority);

		if (m_bDebugCoverReaction)
			DebugCover(ownerEnt, string.Format("LOOK LOCK -> %1 dist=%2m prio=%3 durasi=%4s (menimpa aktif=%5)", perceivedShotPos, distance, lock.m_fPriority, m_fLookLockDuration, active));

		if (m_fLookLockRefreshInterval > 0)
		{
			utility.GetCallqueue().CallLater(RefreshLookLock, (int)(m_fLookLockRefreshInterval * 1000.0), false,
				utility, lock.m_iToken);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Pasang ulang arah tolehan selama kunci masih aktif. Token beda = kunci sudah diganti, loop ini berhenti.
	protected void RefreshLookLock(SCR_AIUtilityComponent utility, int token)
	{
		if (!utility || !utility.m_OwnerEntity || !utility.m_LookAction)
			return;

		DCO_ShotLookLock lock = s_mLookLocks.Get(utility.m_OwnerEntity);
		if (!lock || lock.m_iToken != token)
			return;

		if (GetGame().GetWorld().GetWorldTime() >= lock.m_fUntil_ms)
		{
			if (m_bDebugCoverReaction)
				DebugCover(utility.m_OwnerEntity, "LOOK UNLOCK: durasi kunci habis");
			return;
		}

		utility.m_LookAction.LookAt(lock.m_vPos, lock.m_fPriority);

		utility.GetCallqueue().CallLater(RefreshLookLock, (int)(m_fLookLockRefreshInterval * 1000.0), false,
			utility, token);
	}

	//------------------------------------------------------------------------------------------------
	//! Posisi yang sedang dikunci kalau ada, kalau tidak pakai posisi fallback
	protected vector ResolveLockedLookPos(notnull SCR_AIUtilityComponent utility, vector fallbackPos)
	{
		if (!m_bLookPreferClosest || !utility.m_OwnerEntity)
			return fallbackPos;

		DCO_ShotLookLock lock = s_mLookLocks.Get(utility.m_OwnerEntity);
		if (!lock || GetGame().GetWorld().GetWorldTime() >= lock.m_fUntil_ms)
			return fallbackPos;

		return lock.m_vPos;
	}

	//------------------------------------------------------------------------------------------------
	protected void PruneLookLockMap(float now_ms)
	{
		array<IEntity> toRemove = {};

		foreach (IEntity ent, DCO_ShotLookLock lock : s_mLookLocks)
		{
			if (!ent || !lock || now_ms >= lock.m_fUntil_ms)
				toRemove.Insert(ent);
		}

		foreach (IEntity entRemove : toRemove)
		{
			s_mLookLocks.Remove(entRemove);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void LookAtShot(SCR_AIUtilityComponent utility, vector perceivedShotPos)
	{
		if (!utility || !utility.m_OwnerEntity)
			return;

		if (!utility.m_LookAction)
		{
			if (m_bDebugCoverReaction)
				DebugCover(utility.m_OwnerEntity, "LOOK gagal: m_LookAction null");
			return;
		}

		utility.m_LookAction.LookAt(perceivedShotPos, SCR_AILookAction.PRIO_DANGER_EVENT);

		if (m_bDebugCoverReaction)
			DebugCover(utility.m_OwnerEntity, string.Format("LOOK -> %1", perceivedShotPos));
	}

	//------------------------------------------------------------------------------------------------
	//! Turun satu tingkat stance tanpa bergerak
	protected void LowerStance(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_CombatComponent)
			return;

		SCR_CharacterControllerComponent charCon = utility.m_CombatComponent.GetCharacterController();
		if (!charCon)
			return;

		if (charCon.GetStance() == ECharacterStance.STAND)
			charCon.SetStanceChange(2);
	}

	//------------------------------------------------------------------------------------------------
	//! Jadwalkan tolehan ke arah tembakan setelah gerakan ke cover selesai
	protected void ScheduleLookAfterCover(notnull SCR_AIUtilityComponent utility, vector perceivedShotPos, float moveDuration_s)
	{
		if (!m_bLookAfterCover || m_iLookAfterCoverRepeats <= 0)
			return;

		int delay_ms = (int)((moveDuration_s + m_fLookAfterCoverDelay) * 1000.0);

		utility.GetCallqueue().CallLater(LookAfterCover, delay_ms, false,
			utility, perceivedShotPos, m_iLookAfterCoverRepeats);
	}

	//------------------------------------------------------------------------------------------------
	protected void LookAfterCover(SCR_AIUtilityComponent utility, vector perceivedShotPos, int repeatsLeft)
	{
		if (!utility || !utility.m_OwnerEntity)
			return;

		LookAtShot(utility, ResolveLockedLookPos(utility, perceivedShotPos));

		repeatsLeft--;
		if (repeatsLeft <= 0)
			return;

		utility.GetCallqueue().CallLater(LookAfterCover, (int)(m_fLookAfterCoverInterval * 1000.0), false,
			utility, perceivedShotPos, repeatsLeft);
	}

	//------------------------------------------------------------------------------------------------
	protected void TryTakeCoverFromShot(SCR_AIUtilityComponent utility, vector perceivedShotPos, float distance, float threatScore)
	{
		if (!utility || !utility.m_OwnerEntity)
			return;

		DCO_AIConfigComponent cfg = utility.m_DCOConfig;

		float maxDist    = DODGE_MAX_DIST_FALLBACK;
		float cooldown_s = DODGE_COOLDOWN_FALLBACK;
		float chance     = DODGE_CHANCE_FALLBACK;
		bool  scalePers  = true;

		if (cfg)
		{
			maxDist    = cfg.GetDodgeMaxDist();
			cooldown_s = cfg.GetDodgeCooldown();
			chance     = cfg.GetDodgeChance();
			scalePers  = cfg.GetDodgeScaleByPersonality();
		}

		bool dbg = IsDebugOn(distance);
		IEntity ownerEnt = utility.m_OwnerEntity;

		if (distance > maxDist)
		{
			if (dbg) DebugCover(ownerEnt, string.Format("COVER STOP: dist %1 > maxDist %2", distance, maxDist));
			return;
		}

		SCR_AICombatMoveState state = utility.m_CombatMoveState;
		if (!state)
		{
			if (dbg) DebugCover(ownerEnt, "COVER STOP: m_CombatMoveState null");
			return;
		}

		if (m_bCoverVeryHighPriority)
		{
			// Masih lari ke cover dari tembakan sebelumnya -> jangan ganggu
			if (state.DCO_IsCoverProtected())
			{
				if (dbg) DebugCover(ownerEnt, "COVER STOP: sudah lari ke cover (protected)");
				return;
			}

			if (state.IsExecutingRequest() && dbg)
				DebugCover(ownerEnt, string.Format("COVER OVERRIDE: request lain sedang jalan -> akan ditimpa (oldRq fail=%1)", DebugFailReason(state.GetOldRequest())));
		}
		else if (state.IsExecutingRequest())
		{
			if (dbg) DebugCover(ownerEnt, string.Format("COVER STOP: IsExecutingRequest = true (oldRq fail=%1)", DebugFailReason(state.GetOldRequest())));
			return;
		}

		if (cfg && cfg.IsHoldPosition())
		{
			if (dbg) DebugCover(ownerEnt, "COVER STOP: HoldPosition");
			return;
		}

		if (utility.m_AIInfo && utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
		{
			if (dbg) DebugCover(ownerEnt, "COVER STOP: IN_VEHICLE");
			return;
		}

		if (state.IsInValidCover())
		{
			if (dbg) DebugCover(ownerEnt, "COVER STOP: IsInValidCover = true");
			return;
		}

		// Ancaman kecil (tembakan jauh / tidak mengarah ke AI): cukup turun stance, tidak perlu lari
		if (threatScore < m_fCoverThreatGate)
		{
			if (dbg) DebugCover(ownerEnt, string.Format("COVER STOP: threat %1 < gate %2 -> turun stance saja", threatScore, m_fCoverThreatGate));

			if (m_bLowerStanceOnLowThreat)
				LowerStance(utility);

			return;
		}

		// Budget gerakan bersama: batasi total pergerakan AI ini dari semua danger reaction
		if (!DCO_CoverMoveBudget.CanMove(ownerEnt, m_fSharedMoveCooldown))
		{
			if (dbg) DebugCover(ownerEnt, string.Format("COVER STOP: budget gerakan (gerak terakhir %1s lalu, minimal %2s)",
				DCO_CoverMoveBudget.GetTimeSinceLastMove(ownerEnt), m_fSharedMoveCooldown));
			return;
		}

		if (SCR_CoverManagerComponent.IsEntityInsideBuilding(ownerEnt))
		{
			if (dbg) DebugCover(ownerEnt, "COVER STOP: di dalam bangunan");
			return;
		}

		if (!CanDodgeNow(ownerEnt, cooldown_s))
		{
			if (dbg) DebugCover(ownerEnt, string.Format("COVER STOP: cooldown aktif (%1s)", cooldown_s));
			return;
		}

		// Cooldown ditandai sebelum roll (disengaja, cegah spam percobaan)
		MarkDodged(ownerEnt, cooldown_s);

		float baseChance = chance;
		float persScale  = 1.0;
		if (scalePers)
		{
			persScale = DCO_PersonalityCombatUtility.GetTakeCoverChanceScale(utility);
			chance *= persScale;
		}

		// Makin tinggi ancaman, makin mendekati pasti cari cover
		chance = Math.Clamp(chance, 0.0, 1.0);
		chance = chance + threatScore * (1.0 - chance);

		// Very high priority: selalu lari ke cover, kecuali DodgeChance dari config dipakai
		if (m_bCoverVeryHighPriority && !m_bUseDodgeChanceOnCover)
			chance = 1.0;

		float coverRoll = Math.RandomFloat01();
		if (coverRoll >= chance)
		{
			if (dbg) DebugCover(ownerEnt, string.Format("COVER STOP: roll gagal roll=%1 chance=%2 (base=%3 pers=%4 threat=%5) -> cooldown tetap jalan",
				coverRoll, chance, baseChance, persScale, threatScore));
			return;
		}

		// Bangunan dimatikan: langsung cover biasa
		if (!m_bCoverPreferBuilding)
		{
			if (dbg) DebugCover(ownerEnt, string.Format("COVER GO: MOVE-cover (bangunan dimatikan) roll=%1 chance=%2 threat=%3", coverRoll, chance, threatScore));
			PushCoverMove(utility, state, perceivedShotPos, threatScore);
			return;
		}

		// Prioritas bangunan. Kalau request sebelumnya baru gagal karena tidak ada bangunan, langsung cover biasa.
		SCR_AICombatMoveRequestBase oldRq = state.GetOldRequest();
		if (oldRq && oldRq.m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
		{
			if (dbg) DebugCover(ownerEnt, string.Format("COVER GO: MOVE langsung (oldRq gagal NO_BUILDING_FOUND) roll=%1 chance=%2 threat=%3", coverRoll, chance, threatScore));
			PushCoverMove(utility, state, perceivedShotPos, threatScore);
			return;
		}

		if (dbg) DebugCover(ownerEnt, string.Format("COVER GO: BUILDING dulu roll=%1 chance=%2 threat=%3 oldRq fail=%4", coverRoll, chance, threatScore, DebugFailReason(oldRq)));

		SCR_AICombatMoveRequest_Move buildingRq = PushBuildingMove(utility, state, perceivedShotPos);
		utility.GetCallqueue().CallLater(CheckBuildingFallback, BUILDING_FALLBACK_POLL_MS, false,
			utility, buildingRq, perceivedShotPos, threatScore, BUILDING_FALLBACK_MAX_POLLS);
	}

	//------------------------------------------------------------------------------------------------
	//! Catatan: radius pencarian bangunan diatur BUILDING_SEARCH_RADIUS_BT di file .bt, bukan m_fCoverSearchDistMax.
	protected SCR_AICombatMoveRequest_Move PushBuildingMove(notnull SCR_AIUtilityComponent utility, notnull SCR_AICombatMoveState state, vector perceivedShotPos)
	{
		float searchDist = DODGE_SEARCH_DIST_FALLBACK;
		if (utility.m_DCOConfig)
			searchDist = utility.m_DCOConfig.GetDodgeSearchDist();

		if (m_fCoverSearchDistCap > 0)
			searchDist = Math.Min(searchDist, m_fCoverSearchDistCap);

		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

		rq.m_eType      = SCR_EAICombatMoveRequestType.BUILDING;
		rq.m_eReason    = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
		rq.m_vTargetPos = perceivedShotPos;
		rq.m_vMovePos   = rq.m_vTargetPos;

		rq.m_bTryFindCover              = false;
		rq.m_bFailIfNoCover             = false;
		rq.m_bUseCoverSearchDirectivity = false;
		rq.m_bCheckCoverVisibility      = false;

		// BUILDING: jangan pernah maju, supaya AI tidak sprint masuk bangunan ke arah musuh
		rq.m_eDirection = ResolveCoverDirectionForUnit(utility, false);
		rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;

		rq.m_eStanceMoving = ECharacterStance.STAND;
		rq.m_eStanceEnd    = ECharacterStance.CROUCH;
		rq.m_eMovementType = EMovementType.SPRINT;

		rq.m_bAimAtTarget    = false;
		rq.m_bAimAtTargetEnd = true;

		rq.m_fCoverSearchDistMin = 0;
		rq.m_fCoverSearchDistMax = searchDist;
		rq.m_fMoveDuration_s     = searchDist / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;

		ApplyWaypointDodgeBias(utility, rq, searchDist);

		DCO_CoverMoveBudget.MarkMove(utility.m_OwnerEntity);

		float buildingHold_s = (BUILDING_FALLBACK_POLL_MS * BUILDING_FALLBACK_MAX_POLLS) / 1000.0 + COVER_BUILDING_HOLD_MARGIN_S;

		// Proteksi bangunan minimal selama jendela fallback, maksimal selama durasi lari
		if (m_bCoverVeryHighPriority)
			state.DCO_ApplyCoverRequest(rq, Math.Max(buildingHold_s, rq.m_fMoveDuration_s + COVER_MOVE_HOLD_MARGIN_S));
		else
			state.ApplyNewRequest(rq);

		ScheduleLookAfterCover(utility, perceivedShotPos, rq.m_fMoveDuration_s);

		if (m_bDebugCoverReaction)
			DebugCover(utility.m_OwnerEntity, string.Format("PUSH BUILDING searchDist=%1 dir=%2 executingAfter=%3", searchDist, typename.EnumToString(SCR_EAICombatMoveDirection, rq.m_eDirection), state.IsExecutingRequest()));

		return rq;
	}

	//------------------------------------------------------------------------------------------------
	//! Cover biasa yang harus menutupi AI dari arah perkiraan tembakan
	protected void PushCoverMove(notnull SCR_AIUtilityComponent utility, notnull SCR_AICombatMoveState state, vector perceivedShotPos, float threatScore)
	{
		float searchDist = DODGE_SEARCH_DIST_FALLBACK;
		if (utility.m_DCOConfig)
			searchDist = utility.m_DCOConfig.GetDodgeSearchDist();

		if (m_fCoverSearchDistCap > 0)
			searchDist = Math.Min(searchDist, m_fCoverSearchDistCap);

		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

		rq.m_eType      = SCR_EAICombatMoveRequestType.MOVE;
		rq.m_eReason    = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
		rq.m_vTargetPos = perceivedShotPos;
		rq.m_vMovePos   = rq.m_vTargetPos;

		rq.m_bTryFindCover              = true;
		rq.m_bFailIfNoCover             = false;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility      = true;

		// Maju cuma boleh kalau threat di bawah threshold tiarap
		rq.m_eDirection = ResolveCoverDirectionForUnit(utility, threatScore < m_fHighThreatProneThreshold);
		rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;

		rq.m_eStanceMoving = ECharacterStance.STAND;
		rq.m_eMovementType = EMovementType.SPRINT;

		if (threatScore >= m_fHighThreatProneThreshold)
			rq.m_eStanceEnd = ECharacterStance.PRONE;
		else
			rq.m_eStanceEnd = ECharacterStance.CROUCH;

		rq.m_bAimAtTarget    = false;
		rq.m_bAimAtTargetEnd = true;

		rq.m_fCoverSearchDistMin = 0;
		rq.m_fCoverSearchDistMax = searchDist;
		rq.m_fMoveDuration_s     = searchDist / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;

		ApplyWaypointDodgeBias(utility, rq, searchDist);

		DCO_CoverMoveBudget.MarkMove(utility.m_OwnerEntity);

		if (m_bCoverVeryHighPriority)
			state.DCO_ApplyCoverRequest(rq, rq.m_fMoveDuration_s + COVER_MOVE_HOLD_MARGIN_S);
		else
			state.ApplyNewRequest(rq);

		ScheduleLookAfterCover(utility, perceivedShotPos, rq.m_fMoveDuration_s);

		if (m_bDebugCoverReaction)
		{
			DebugCover(utility.m_OwnerEntity, string.Format("PUSH MOVE-cover searchDist=%1 dir=%2 stanceEnd=%3 executingAfter=%4",
				searchDist, typename.EnumToString(SCR_EAICombatMoveDirection, rq.m_eDirection), typename.EnumToString(ECharacterStance, rq.m_eStanceEnd), state.IsExecutingRequest()));
		}

		utility.GetCallqueue().CallLater(CheckCoverMoveWatchdog, BUILDING_FALLBACK_POLL_MS, false,
			utility, rq, BUILDING_FALLBACK_MAX_POLLS);
	}

	//------------------------------------------------------------------------------------------------
	//! Kalau request cover pun tidak pernah dieksekusi, lepas proteksi supaya AI tidak beku di tempat.
	protected void CheckCoverMoveWatchdog(SCR_AIUtilityComponent utility, SCR_AICombatMoveRequest_Move coverRq, int pollsLeft)
	{
		if (!utility || !utility.m_OwnerEntity || !coverRq)
			return;

		SCR_AICombatMoveState state = utility.m_CombatMoveState;
		if (!state)
			return;

		if (state.IsExecutingRequest())
		{
			if (m_bDebugCoverReaction)
				DebugCover(utility.m_OwnerEntity, "MOVE-cover OK: request dieksekusi");
			return;
		}

		if (coverRq.m_eFailReason != SCR_EAICombatMoveRequestFailReason.NONE)
		{
			if (m_bDebugCoverReaction)
				DebugCover(utility.m_OwnerEntity, string.Format("MOVE-cover GAGAL fail=%1 -> lepas proteksi", DebugFailReason(coverRq)));

			state.DCO_ReleaseCoverProtection("cover-failed");
			return;
		}

		pollsLeft--;
		if (pollsLeft <= 0)
		{
			if (m_bDebugCoverReaction)
				DebugCover(utility.m_OwnerEntity, "MOVE-cover STUCK: tidak pernah dieksekusi -> lepas proteksi, AI balik ke behavior normal");

			state.DCO_ReleaseCoverProtection("cover-stuck");
			return;
		}

		utility.GetCallqueue().CallLater(CheckCoverMoveWatchdog, BUILDING_FALLBACK_POLL_MS, false,
			utility, coverRq, pollsLeft);
	}

	//------------------------------------------------------------------------------------------------
	//! Pantau request bangunan. Kalau gagal karena tidak ada bangunan, langsung ganti ke cover biasa.
	protected void CheckBuildingFallback(SCR_AIUtilityComponent utility, SCR_AICombatMoveRequest_Move buildingRq, vector perceivedShotPos, float threatScore, int pollsLeft)
	{
		if (!utility || !utility.m_OwnerEntity || !buildingRq)
			return;

		SCR_AICombatMoveState state = utility.m_CombatMoveState;
		if (!state)
			return;

		IEntity ownerEnt = utility.m_OwnerEntity;
		bool executing   = state.IsExecutingRequest();

		if (m_bDebugCoverReaction)
		{
			DebugCover(ownerEnt, string.Format("BUILDING POLL left=%1 fail=%2 executing=%3 isOldRq=%4",
				pollsLeft, DebugFailReason(buildingRq), executing, state.GetOldRequest() == SCR_AICombatMoveRequestBase.Cast(buildingRq)));
		}

		if (buildingRq.m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
		{
			// Jangan timpa request lain yang sudah jalan (mis. dari reaksi vanilla)
			if (executing && !m_bCoverVeryHighPriority)
			{
				if (m_bDebugCoverReaction) DebugCover(ownerEnt, "FALLBACK STOP: bangunan tidak ada, tapi ada request lain yang jalan -> tidak push cover");
				return;
			}

			if (utility.m_DCOConfig && utility.m_DCOConfig.IsHoldPosition())
			{
				if (m_bDebugCoverReaction) DebugCover(ownerEnt, "FALLBACK STOP: HoldPosition");
				return;
			}

			if (m_bDebugCoverReaction) DebugCover(ownerEnt, "FALLBACK GO: bangunan tidak ada -> push MOVE-cover");
			PushCoverMove(utility, state, perceivedShotPos, threatScore);
			return;
		}

		// Request sudah dijalankan BT -> sudah beres, tidak perlu poll lagi
		if (executing)
		{
			if (m_bDebugCoverReaction)
				DebugCover(ownerEnt, "BUILDING OK: request dieksekusi");
			return;
		}

		pollsLeft--;
		if (pollsLeft <= 0)
		{
			// Tidak gagal, tapi juga tidak pernah dieksekusi: BT tidak mengambil request ini.
			// Jangan biarkan AI beku -> coba cover biasa sekali, lalu lepas proteksi kalau itu pun tidak jalan.
			if (m_bDebugCoverReaction)
				DebugCover(ownerEnt, string.Format("BUILDING STUCK: tidak dieksekusi (fail=%1) -> coba MOVE-cover", DebugFailReason(buildingRq)));

			if (utility.m_DCOConfig && utility.m_DCOConfig.IsHoldPosition())
			{
				state.DCO_ReleaseCoverProtection("stuck-hold");
				return;
			}

			PushCoverMove(utility, state, perceivedShotPos, threatScore);
			return;
		}

		utility.GetCallqueue().CallLater(CheckBuildingFallback, BUILDING_FALLBACK_POLL_MS, false,
			utility, buildingRq, perceivedShotPos, threatScore, pollsLeft);
	}

	protected bool CanInvestigateNow(IEntity entity)
	{
		if (!entity)
			return false;

		float now_ms = GetGame().GetWorld().GetWorldTime();

		float lastTime_ms;
		if (!s_mLastInvestigateTime.Find(entity, lastTime_ms))
			return true;

		return (now_ms - lastTime_ms) > (m_fInvestigateCooldown * 1000.0);
	}

	protected void MarkInvestigated(IEntity entity)
	{
		if (!entity)
			return;

		float now_ms = GetGame().GetWorld().GetWorldTime();
		s_mLastInvestigateTime.Set(entity, now_ms);

		if (s_mLastInvestigateTime.Count() > INVESTIGATE_MAP_PRUNE_THRESHOLD)
			PruneInvestigateMap(now_ms);
	}

	protected void PruneInvestigateMap(float now_ms)
	{
		float staleAge_ms = m_fInvestigateCooldown * 1000.0 * 2.0;

		array<IEntity> toRemove = {};

		foreach (IEntity ent, float lastTime_ms : s_mLastInvestigateTime)
		{
			if (!ent || (now_ms - lastTime_ms) > staleAge_ms)
				toRemove.Insert(ent);
		}

		foreach (IEntity ent : toRemove)
		{
			s_mLastInvestigateTime.Remove(ent);
		}
	}
	
	protected void TryDodge(SCR_AIUtilityComponent utility, vector shotPos, float distance)
	{
		if (!utility || !utility.m_OwnerEntity)
			return;

		DCO_AIConfigComponent cfg = utility.m_DCOConfig;

		float maxDist    = DODGE_MAX_DIST_FALLBACK;
		float cooldown_s = DODGE_COOLDOWN_FALLBACK;
		float chance     = DODGE_CHANCE_FALLBACK;
		bool  scalePers  = true;

		if (cfg)
		{
			maxDist    = cfg.GetDodgeMaxDist();
			cooldown_s = cfg.GetDodgeCooldown();
			chance     = cfg.GetDodgeChance();
			scalePers  = cfg.GetDodgeScaleByPersonality();
		}

		if (distance > maxDist)
			return;

		SCR_AICombatMoveState state = utility.m_CombatMoveState;
		if (!state || state.IsExecutingRequest())
			return;

		if (cfg && cfg.IsHoldPosition())
			return;

		if (utility.m_AIInfo && utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
			return;

		if (state.IsInValidCover())
			return;
		
		if (SCR_CoverManagerComponent.IsEntityInsideBuilding(utility.m_OwnerEntity))
			return;

		if (!CanDodgeNow(utility.m_OwnerEntity, cooldown_s))
			return;

		MarkDodged(utility.m_OwnerEntity, cooldown_s);

		if (scalePers)
			chance *= DCO_PersonalityCombatUtility.GetTakeCoverChanceScale(utility);

		if (Math.RandomFloat01() >= Math.Clamp(chance, 0.0, 1.0))
			return;

		PushDodgeMove(utility, state, shotPos);
	}

	//------------------------------------------------------------------------------------------------
	protected void PushDodgeMove(notnull SCR_AIUtilityComponent utility, notnull SCR_AICombatMoveState state, vector shotPos)
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		
		float searchDist = DODGE_SEARCH_DIST_FALLBACK;
		if (utility.m_DCOConfig)
			searchDist = utility.m_DCOConfig.GetDodgeSearchDist();

		if (m_fCoverSearchDistCap > 0)
			searchDist = Math.Min(searchDist, m_fCoverSearchDistCap);

		rq.m_fCoverSearchDistMin = 0;
		rq.m_fCoverSearchDistMax = searchDist;
		rq.m_fMoveDuration_s     = searchDist / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;

		rq.m_eReason    = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
		rq.m_vTargetPos = shotPos;
		rq.m_vMovePos   = rq.m_vTargetPos;
		
		if (Math.RandomFloat01() < 0.6)
		{
			rq.m_eType         = SCR_EAICombatMoveRequestType.BUILDING;
			rq.m_bTryFindCover = false;
			rq.m_bFailIfNoCover = false;		
			rq.m_bUseCoverSearchDirectivity = false;
			rq.m_bCheckCoverVisibility = false;
		} else
		{
			rq.m_eType         = SCR_EAICombatMoveRequestType.MOVE;
			rq.m_bTryFindCover = true;
			rq.m_bFailIfNoCover = false;	
			rq.m_bUseCoverSearchDirectivity = true;
			rq.m_bCheckCoverVisibility = true;
		}

		// Path lama: maju cuma untuk MOVE-cover, bukan BUILDING
		rq.m_eDirection = ResolveCoverDirectionForUnit(utility, rq.m_eType == SCR_EAICombatMoveRequestType.MOVE);
		rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;

		rq.m_eStanceMoving = ECharacterStance.STAND;
		rq.m_eStanceEnd    = ECharacterStance.CROUCH;
		rq.m_eMovementType = EMovementType.SPRINT;

		rq.m_bAimAtTarget    = false;
		rq.m_bAimAtTargetEnd = true;

		rq.m_fCoverSearchDistMin = 0;
		rq.m_fCoverSearchDistMax = searchDist;
		rq.m_fMoveDuration_s     = searchDist / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;

		ApplyWaypointDodgeBias(utility, rq, searchDist);

		state.ApplyNewRequest(rq);
	}

	//------------------------------------------------------------------------------------------------
	//! Toggle + debug untuk DCO_DodgeWaypointUtility.ApplyWaypointBias
	protected void ApplyWaypointDodgeBias(notnull SCR_AIUtilityComponent utility, notnull SCR_AICombatMoveRequest_Move rq, float searchDist)
	{
		if (!m_bDodgeTowardWaypoint)
			return;

		if (!DCO_DodgeWaypointUtility.ApplyWaypointBias(utility, rq, searchDist))
			return;

		if (m_bDebugCoverReaction)
			DebugCover(utility.m_OwnerEntity, string.Format("WP BIAS: dodge dibelokin ke waypoint %1", rq.m_vMovePos));
	}

	//------------------------------------------------------------------------------------------------
	//! Arah lari ke cover. Randomize mati = mundur (perilaku lama).
	protected SCR_EAICombatMoveDirection ResolveCoverDirection()
	{
		if (!m_bRandomizeCoverDirection)
			return SCR_EAICombatMoveDirection.BACKWARD;

		return PickCoverDirection();
	}

	//------------------------------------------------------------------------------------------------
	//! Pilih arah lari ke cover berdasarkan bobot. Semua bobot 0 = mundur.
	protected SCR_EAICombatMoveDirection PickCoverDirection()
	{
		float wBack     = Math.Max(0.0, m_fCoverDirWeightBackward);
		float wLeft     = Math.Max(0.0, m_fCoverDirWeightLeft);
		float wRight    = Math.Max(0.0, m_fCoverDirWeightRight);
		float wAnywhere = Math.Max(0.0, m_fCoverDirWeightAnywhere);

		float total = wBack + wLeft + wRight + wAnywhere;
		if (total <= 0)
			return SCR_EAICombatMoveDirection.BACKWARD;

		float r = Math.RandomFloat(0, total);

		if (r < wBack)
			return SCR_EAICombatMoveDirection.BACKWARD;
		r -= wBack;

		if (r < wLeft)
			return SCR_EAICombatMoveDirection.LEFT;
		r -= wLeft;

		if (r < wRight)
			return SCR_EAICombatMoveDirection.RIGHT;

		return SCR_EAICombatMoveDirection.ANYWHERE;
	}

	//------------------------------------------------------------------------------------------------
	//! Arah lari ke cover dengan skala personality.
	//! Randomize mati / skala personality mati = sama persis dengan ResolveCoverDirection().
	//! allowForward: izinkan arah MAJU (hanya AGGRESSIVE/RECKLESS yang punya bobot maju).
	protected SCR_EAICombatMoveDirection ResolveCoverDirectionForUnit(SCR_AIUtilityComponent utility, bool allowForward)
	{
		if (!m_bRandomizeCoverDirection || !m_bScaleCoverDirByPersonality || !utility)
			return ResolveCoverDirection();

		return DCO_PersonalityCombatUtility.PickCoverDirectionForPersonality(utility,
			m_fCoverDirWeightBackward, m_fCoverDirWeightLeft, m_fCoverDirWeightRight, m_fCoverDirWeightAnywhere,
			m_fCoverDirForwardWeightScale, allowForward);
	}

	protected bool CanDodgeNow(IEntity entity, float cooldown_s)
	{
		if (!entity)
			return false;

		float lastTime_ms;
		if (!s_mLastDodgeTime.Find(entity, lastTime_ms))
			return true;

		return (GetGame().GetWorld().GetWorldTime() - lastTime_ms) > (cooldown_s * 1000.0);
	}

	protected void MarkDodged(IEntity entity, float cooldown_s)
	{
		if (!entity)
			return;

		float now_ms = GetGame().GetWorld().GetWorldTime();
		s_mLastDodgeTime.Set(entity, now_ms);

		if (s_mLastDodgeTime.Count() > INVESTIGATE_MAP_PRUNE_THRESHOLD)
			PruneDodgeMap(now_ms, cooldown_s);
	}

	protected void PruneDodgeMap(float now_ms, float cooldown_s)
	{
		float staleAge_ms = cooldown_s * 1000.0 * 2.0;

		array<IEntity> toRemove = {};

		foreach (IEntity ent, float lastTime_ms : s_mLastDodgeTime)
		{
			if (!ent || (now_ms - lastTime_ms) > staleAge_ms)
				toRemove.Insert(ent);
		}

		foreach (IEntity ent : toRemove)
		{
			s_mLastDodgeTime.Remove(ent);
		}
	}
}