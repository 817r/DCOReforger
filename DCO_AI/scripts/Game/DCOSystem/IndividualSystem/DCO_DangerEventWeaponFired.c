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

	[Attribute("50.0", UIWidgets.EditBox, "Jarak maksimum (m) tembakan SENYAP yang bikin AI mau maju investigasi.")]
	protected float m_fSuppressedInvestigateDist;

	[Attribute("25.0", UIWidgets.EditBox, "Radius area investigasi.")]
	protected float m_fInvestigateRadius;

	[Attribute("150.0", UIWidgets.EditBox, "Durasi behavior investigasi (detik).")]
	protected float m_fInvestigateDuration;

	[Attribute("20.0", UIWidgets.EditBox, "Cooldown (detik) sebelum AI yang sama boleh dikasih behavior investigasi lagi. Nyegah numpuk pas ditembakin beruntun.")]
	protected float m_fInvestigateCooldown;

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
		
				rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
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
		
				state.ApplyNewRequest(rq);

				return super.PerformReaction(utility, threatSystem, dangerEvent, dangerEventCount);
			}
		}

		bool isAudible = IsAudiable(distance, isShotSuppressed);

		if (!isFlyby && !isAudible)
			return false;

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

			if (!ignoreGunshotHeard)
			{
				if (timeTillGunshotHeard_s < 0)
				{
					OnGunshotHeard(utility, distance, dangerEventCount, shotPos);
					TryDodge(utility, shotPos, distance);
				}
				else
				{
					utility.GetCallqueue().CallLater(OnGunshotHeard, 1000*timeTillGunshotHeard_s, false,
						utility, distance, dangerEventCount, shotPos);
					utility.GetCallqueue().CallLater(TryDodge, 1000*timeTillGunshotHeard_s, false,
						utility, shotPos, distance);
				}
			}

			if (isShotSuppressed && !isInVehicle && distance < m_fSuppressedInvestigateDist
				&& CanInvestigateNow(utility.m_OwnerEntity))
			{
				SCR_AIMoveAndInvestigateBehavior investigate = new SCR_AIMoveAndInvestigateBehavior(
					utility, null, shotPos,
					SCR_AIActionBase.PRIORITY_BEHAVIOR_MOVE_AND_INVESTIGATE,
					SCR_AIActionBase.PRIORITY_LEVEL_NORMAL,
					isDangerous: true,
					radius: m_fInvestigateRadius,
					targetUnitType: EAIUnitType.UnitType_Infantry,
					duration: m_fInvestigateDuration);

				utility.AddAction(investigate);
				MarkInvestigated(utility.m_OwnerEntity);
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

		rq.m_fCoverSearchDistMin = 0;
		rq.m_fCoverSearchDistMax = searchDist;
		rq.m_fMoveDuration_s     = searchDist / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;

		rq.m_eReason    = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
		rq.m_vTargetPos = shotPos;
		rq.m_vMovePos   = rq.m_vTargetPos;

		rq.m_eType         = SCR_EAICombatMoveRequestType.BUILDING;
		rq.m_bTryFindCover = true;
		rq.m_bFailIfNoCover = false;

		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility = false;

		rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
		rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;

		rq.m_eStanceMoving = ECharacterStance.STAND;
		rq.m_eStanceEnd    = ECharacterStance.CROUCH;
		rq.m_eMovementType = EMovementType.SPRINT;

		rq.m_bAimAtTarget    = false;
		rq.m_bAimAtTargetEnd = true;

		rq.m_fCoverSearchDistMin = 0;
		rq.m_fCoverSearchDistMax = searchDist;
		rq.m_fMoveDuration_s     = searchDist / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;

		state.ApplyNewRequest(rq);
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