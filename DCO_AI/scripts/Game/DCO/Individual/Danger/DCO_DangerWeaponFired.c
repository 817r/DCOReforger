class DCO_ShotLookLock
{
	IEntity m_Source;
	vector  m_vPos;
	float   m_fDist;
	float   m_fPriority;
	float   m_fUntil_ms;
	int     m_iToken;
}

class DCO_DodgeWaypointUtility
{
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

		return myPos;
	}

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

		rq.m_vMovePos              = wpPos;
		rq.m_eDirection            = SCR_EAICombatMoveDirection.FORWARD;
		rq.m_vAvoidStraightPathDir = vector.Zero;

		return true;
	}
}

[BaseContainerProps()]
modded class SCR_AIDangerReaction_WeaponFired
{


	protected bool m_bScaleDodgeByPersonality = true;

	protected static ref map<IEntity, float> s_mLastDodgeTime;
	protected static map<IEntity, float> GetLastDodgeTime()
	{
		if (!s_mLastDodgeTime)
			s_mLastDodgeTime = new map<IEntity, float>();
		return s_mLastDodgeTime;
	}

	protected static ref map<IEntity, int> s_mDodgeShotCount;
	protected static map<IEntity, int> GetDodgeShotCountStore()
	{
		if (!s_mDodgeShotCount)
			s_mDodgeShotCount = new map<IEntity, int>();
		return s_mDodgeShotCount;
	}
	protected static ref map<IEntity, float> s_mDodgeShotLast_ms;
	protected static map<IEntity, float> GetDodgeShotLast_ms()
	{
		if (!s_mDodgeShotLast_ms)
			s_mDodgeShotLast_ms = new map<IEntity, float>();
		return s_mDodgeShotLast_ms;
	}

	protected static const float coverSearchDistMax = 20;

	protected static const float COVER_QUERY_SECTOR_ANGLE_RAD  = 0.51 * Math.PI;

	protected static ref map<IEntity, float> s_mLastInvestigateTime;
	protected static map<IEntity, float> GetLastInvestigateTime()
	{
		if (!s_mLastInvestigateTime)
			s_mLastInvestigateTime = new map<IEntity, float>();
		return s_mLastInvestigateTime;
	}





	protected static bool s_bDCOCoverDebug;

	protected static ref map<IEntity, ref DCO_ShotLookLock> s_mLookLocks;
	protected static map<IEntity, ref DCO_ShotLookLock> GetLookLocks()
	{
		if (!s_mLookLocks)
			s_mLookLocks = new map<IEntity, ref DCO_ShotLookLock>();
		return s_mLookLocks;
	}

	[Attribute("50.0", UIWidgets.EditBox, "Maximum distance (m) of SUPPRESSED gunfire that makes AI move forward to investigate.")]
	protected float m_fSuppressedInvestigateDist;

	[Attribute("25.0", UIWidgets.EditBox, "Investigation area radius.")]
	protected float m_fInvestigateRadius;

	[Attribute("150.0", UIWidgets.EditBox, "Investigate behavior duration (s).")]
	protected float m_fInvestigateDuration;

	[Attribute("20.0", UIWidgets.EditBox, "Cooldown (s) before the same AI may get an investigate behavior again. Stops stacking under sustained fire.")]
	protected float m_fInvestigateCooldown;

	[Attribute("1", UIWidgets.CheckBox, "Use the realistic reaction model (hearing range, score-based head turns, cover search that prefers buildings). If off, falls back to the old IsAudiable + TryDodge.")]
	protected bool m_bUseRealisticReaction;

	[Attribute("180.0", UIWidgets.EditBox, "Hearing range (m) for SUPPRESSED gunfire. Normal gunfire uses the vanilla AUDIBLE_DISTANCE_NORMAL.")]
	protected float m_fSuppressedAudibleRange;

	[Attribute("0.5", UIWidgets.EditBox, "Hearing range multiplier when the AI is inside a vehicle (0.5 = about -6 dB).")]
	protected float m_fInVehicleHearingFactor;

	[Attribute("0.3", UIWidgets.EditBox, "Audibility >= this value = always heard. Below it the chance = audibility / this value.")]
	protected float m_fAudibleGuaranteeThreshold;

	[Attribute("5.0", UIWidgets.EditBox, "Minimum direction error (degrees) when AI estimate the shooter position.")]
	protected float m_fDirectionErrorBaseDeg;

	[Attribute("20.0", UIWidgets.EditBox, "Extra maximum direction error (degrees) when the shot is barely audible.")]
	protected float m_fDirectionErrorExtraDeg;

	[Attribute("60.0", UIWidgets.EditBox, "Distance (m) at which AI always turn toward gunfire.")]
	protected float m_fLookDistFull;

	[Attribute("400.0", UIWidgets.EditBox, "Maximum distance (m) at which AI will turn. Beyond it they do not turn.")]
	protected float m_fLookDistMax;

	[Attribute("0.2", UIWidgets.EditBox, "Base chance to turn at maximum distance.")]
	protected float m_fLookChanceAtMax;

	[Attribute("0.5", UIWidgets.EditBox, "Turn chance bonus from threat score (0..1 times this value).")]
	protected float m_fLookThreatBonus;

	[Attribute("0.15", UIWidgets.EditBox, "Minimum turn reaction delay (s) after the sound arrives.")]
	protected float m_fLookDelayMin;

	[Attribute("0.35", UIWidgets.EditBox, "Maximum turn reaction delay (s) after the sound arrives.")]
	protected float m_fLookDelayMax;

	[Attribute("0.2", UIWidgets.EditBox, "Minimum delay (s) before AI start running to cover.")]
	protected float m_fCoverDelayMin;

	[Attribute("0.6", UIWidgets.EditBox, "Maximum delay (s) before AI start running to cover.")]
	protected float m_fCoverDelayMax;

	[Attribute("0.7", UIWidgets.EditBox, "Threat score >= this value: running to cover may not go FORWARD. (Final dodge stance: cover found = crouch, not found = prone.)")]
	protected float m_fHighThreatProneThreshold;

	[Attribute("1", UIWidgets.CheckBox, "After reaching cover, AI turn again toward the estimated gunfire direction (the first turn is usually overridden by the move).")]
	protected bool m_bLookAfterCover;

	[Attribute("3", UIWidgets.EditBox, "How many times the turn after reaching cover is repeated so the view holds.")]
	protected int m_iLookAfterCoverRepeats;

	[Attribute("1.0", UIWidgets.EditBox, "Delay (s) between turns after reaching cover.")]
	protected float m_fLookAfterCoverInterval;

	[Attribute("0.3", UIWidgets.EditBox, "Extra delay (s) after the move ends before AI turn toward the gunfire.")]
	protected float m_fLookAfterCoverDelay;

	[Attribute("1", UIWidgets.CheckBox, "Turns prefer the NEAREST gunfire source. The turn direction is locked for Look Lock Duration; farther shots cannot take it over.")]
	protected bool m_bLookPreferClosest;

	[Attribute("3.0", UIWidgets.EditBox, "How long (s) the turn direction stays locked after hearing gunfire. More shots from the same shooter extend the lock.")]
	protected float m_fLookLockDuration;

	[Attribute("0.8", UIWidgets.EditBox, "A new source may take over the direction if its distance is <= this value x the distance of the locked source. Below 1 so AI do not swing between two shooters at similar distances.")]
	protected float m_fLookOverrideDistRatio;

	[Attribute("0.5", UIWidgets.EditBox, "Interval (s) at which the turn direction is re-applied while locked, so the orientation does not drift.")]
	protected float m_fLookLockRefreshInterval;

	[Attribute("10.0", UIWidgets.EditBox, "Maximum look priority bonus (at distance 0), falling linearly to 0 at Look Dist Max. Capped below PRIO_UNKNOWN_TARGET so it never beats a real target.")]
	protected float m_fLookPriorityProximityBonus;

	[Attribute("12.0", UIWidgets.EditBox, "Upper limit (m) of the cover search distance. Caps the DodgeSearchDist value so AI do not sprint far away.")]
	protected float m_fCoverSearchDistCap;

	[Attribute("0.4", UIWidgets.EditBox, "Minimum threat score for AI to run for cover. Below this AI only turn and lower their stance.")]
	protected float m_fCoverThreatGate;

	[Attribute("1", UIWidgets.CheckBox, "Lower stance when the threat is below the gate (instead of running to cover).")]
	protected bool m_bLowerStanceOnLowThreat;

	[Attribute("8.0", UIWidgets.EditBox, "Minimum time (s) since this AI's last cover move, including those from impact reactions.")]
	protected float m_fSharedMoveCooldown;

	[Attribute("0", UIWidgets.CheckBox, "Try to find a BUILDING before regular cover. Turn off so AI go straight for regular cover.")]
	protected bool m_bCoverPreferBuilding;

	[Attribute("1", UIWidgets.CheckBox, "Cover = VERY HIGH PRIORITY: no chance roll, overrides any combat move in progress, and requests from other systems are rejected while the AI runs to cover.")]
	protected bool m_bCoverVeryHighPriority;

	[Attribute("1", UIWidgets.CheckBox, "Roll the chance to run for cover using DodgeChance from the DCO config (+ personality scale if on), even when Cover Very High Priority is on. Very High Priority still overrides and protects requests, it just no longer forces chance = 1.")]
	protected bool m_bUseDodgeChanceOnCover;

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

	[Attribute("1", UIWidgets.CheckBox, "The direction weights above are multiplied by a personality scale (CAUTIOUS retreats more often, AGGRESSIVE/RECKLESS move sideways or forward more often). Turn off so all personalities use the same weights.")]
	protected bool m_bScaleCoverDirByPersonality;

	[Attribute("1.0", UIWidgets.EditBox, "FORWARD direction weight multiplier from personality (AGGRESSIVE 0.5, RECKLESS 1.0, others 0). 0 = never forward. Forward is only used for regular cover (not buildings) and when the threat is below High Threat Prone Threshold.")]
	protected float m_fCoverDirForwardWeightScale;

	[Attribute("1", UIWidgets.CheckBox, "If the group has a position waypoint, dodge and cover moves are steered toward it. Outside the WP radius -> run to the WP. Inside the radius -> normal direction, unless it would leave the radius -> turned toward the WP.")]
	protected bool m_bDodgeTowardWaypoint;

	[Attribute("0", UIWidgets.CheckBox, "Debug: print gunfire reaction logs (hearing, turning, cover, building fallback).")]
	protected bool m_bDebugCoverReaction;

	[Attribute("100.0", UIWidgets.EditBox, "Debug: only log gunfire within this distance (m), so the log is not flooded.")]
	protected float m_fDebugMaxDist;

	protected static bool DCO_IsLauncherShot(IEntity instigator)
	{
		BaseWeaponManagerComponent wm = BaseWeaponManagerComponent.Cast(instigator.FindComponent(BaseWeaponManagerComponent));
		return wm && wm.GetCurrentWeapon() && wm.GetCurrentWeapon().GetWeaponType() == EWeaponType.WT_ROCKETLAUNCHER;
	}

	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{
		DCO_Perf.Count("ev:shot");
		int perfStart = DCO_Perf.Begin();
		bool reacted = DCO_PerformShotReaction(utility, threatSystem, dangerEvent, dangerEventCount);
		DCO_Perf.End("danger_shot", perfStart);
		if (reacted)
			DCO_Perf.Count("ev:shot_react");
		return reacted;
	}

	void DCO_OnProjectileFlyby(SCR_AIUtilityComponent utility, int dangerEventCount, vector pos, float weight, IEntity shooter)
	{
		if (!utility || !utility.m_ThreatSystem)
			return;
		utility.m_ThreatSystem.DCO_ThreatFlyby(dangerEventCount, weight, pos, shooter);
		if (utility.m_SectorThreatFilter)
			utility.m_SectorThreatFilter.OnShotsFired(pos, dangerEventCount, true);
	}

	override void OnGunshotHeard(notnull SCR_AIUtilityComponent utility, float distance, int dangerEventCount, vector pos)
	{
		int perfStart = DCO_Perf.Begin();
		super.OnGunshotHeard(utility, distance, dangerEventCount, pos);
		DCO_Perf.End("danger_shot", perfStart);
	}

	protected bool DCO_PerformShotReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{
		AIDangerEventWeaponFire eventWeaponFire = AIDangerEventWeaponFire.Cast(dangerEvent);
		if (!eventWeaponFire)
			return false;
		IEntity gfShooter = eventWeaponFire.GetObject();
		if (gfShooter && vector.DistanceSq(gfShooter.GetOrigin(), utility.m_OwnerEntity.GetOrigin()) < 22500.0)
			utility.m_fDCOGunfireAt = GetGame().GetWorld().GetWorldTime();
		if (Math.RandomFloat01() > DCO_Explosion.DeafFactor(utility.m_OwnerEntity))
			return false;

		IEntity shooter = eventWeaponFire.GetObject();
		if (!shooter)
			return false;

		IEntity instigatorEntity = eventWeaponFire.GetInstigatorEntity();
		if (!instigatorEntity)
			return false;

		Faction instigatorFaction;
		float factNow = GetGame().GetWorld().GetWorldTime();
		if (instigatorEntity == utility.m_DCOShotInstig && factNow - utility.m_fDCOShotInstigAt < 2000.0)
		{
			DCO_Perf.Count("ev:shot_fact_cache");
			instigatorFaction = utility.m_DCOShotFaction;
			if (!instigatorFaction || !utility.m_bDCOShotEnemyOk)
				return false;
		}
		else
		{
			instigatorFaction = SCR_AIFactionHandling.GetEntityPerceivedFaction(instigatorEntity);
			SCR_ChimeraAIAgent factAgent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
			utility.m_DCOShotInstig = instigatorEntity;
			utility.m_fDCOShotInstigAt = factNow;
			utility.m_DCOShotFaction = instigatorFaction;
			utility.m_bDCOShotEnemyOk = instigatorFaction && factAgent && (!utility.IsMilitary() || factAgent.IsEnemy(instigatorFaction));
			if (!utility.m_bDCOShotEnemyOk)
				return false;
		}

		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
		if (!agent)
			return false;

		if (!utility.m_OwnerEntity)
			return false;

		vector shotPos          = eventWeaponFire.GetPosition();
		vector shotDir          = eventWeaponFire.GetDirection();
		bool   isShotSuppressed = eventWeaponFire.IsSuppressed();
		vector myOrigin         = utility.m_OwnerEntity.GetOrigin();
		float  distance         = vector.Distance(myOrigin, shotPos);
		float  distanceSQ       = vector.DistanceSq(myOrigin, shotPos);

		float dismountDist = DCO_AIConfigComponent.DismountOf(utility);

		float dismountDistSq = dismountDist * dismountDist;

		bool isFlyby = IsFlyby(myOrigin, shotPos, shotDir, distance);
		bool endangeringForGroup = isFlyby || distance < ENDANGERING_FOR_GROUP_RADIUS;
		bool isInVehicle = utility.m_AIInfo && utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE);
		if (!isInVehicle && distance < 300.0 && utility.m_CombatComponent)
			utility.m_CombatComponent.DCO_NoteUnseenFire(instigatorEntity, shotPos);

		if (isInVehicle)
		{
			if (distance < 350.0 && DCO_IsLauncherShot(instigatorEntity))
			{
				IEntity atVeh = DCO_VehicleCombat.GetVehicle(utility.m_OwnerEntity);
				DCO_VehicleCombat.ReportATThreat(atVeh, instigatorEntity, shotPos);
				DCO_VehicleActive.NoteRocket(atVeh);
			}

			bool crewSeat = utility.m_AIInfo.HasUnitState(EUnitState.PILOT) || utility.m_AIInfo.HasUnitState(EUnitState.IN_TURRET);
			if (distanceSQ > dismountDistSq)
			{
				if (!crewSeat)
					return false;
				DCO_Perf.Count("veh:crew_heard_far");
			}

			if (crewSeat)
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

		DCO_DodgeAssist.NoteShot(utility, ComputeMissDistance(myOrigin, shotPos, shotDir));
		s_bDCOCoverDebug = m_bDebugCoverReaction;

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
			DebugCover(utility.m_OwnerEntity, string.Format("PERCEIVE dist=%1 NOT AUDIBLE (audibility=%2) flyby=%3 -> no cover reaction", distance, audibility, isFlyby));
		}

		float timeTillFlyby_s = float.MAX;
		float timeTillGunshotHeard_s = float.MAX;

		if (isFlyby)
		{
			vector sd = shotDir.Normalized();
			vector rel = myOrigin - shotPos;
			float flyWeight = SCR_AIThreatSystem.DCO_MissWeight((rel - sd * vector.Dot(rel, sd)).Length());
			float projectileSpeed = eventWeaponFire.GetInitialSpeed();

			if (projectileSpeed <= 0)
			{
				DCO_QueueFlyby(utility, dangerEventCount, shotPos, flyWeight, instigatorEntity, 0);
				timeTillFlyby_s = 0;
			}
			else
			{
				WorldTimestamp eventTimestamp = eventWeaponFire.GetTimestamp();
				float flightTime_s = distance / projectileSpeed;
				WorldTimestamp flybyTimestamp = eventTimestamp.PlusSeconds(flightTime_s + PROJECTILE_FLYBY_DELAY_S);
				timeTillFlyby_s = flybyTimestamp.DiffSeconds(GetGame().GetWorld().GetTimestamp());

				DCO_QueueFlyby(utility, dangerEventCount, shotPos, flyWeight, instigatorEntity, 1000 * timeTillFlyby_s);
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
				float heardDelay_ms = Math.Max(0, 1000 * timeTillGunshotHeard_s);
				DCO_QueueHeard(utility, distance, dangerEventCount, shotPos, heardDelay_ms);
				if (m_bUseRealisticReaction)
					DCO_QueueReact(utility, perceivedShotPos, distance, threatScore, instigatorEntity, heardDelay_ms);
				else if (heardDelay_ms <= 0)
					TryDodge(utility, shotPos, distance);
				else
				{
					DCO_Perf.Count("cl:shot");
					utility.GetCallqueue().CallLater(TryDodge, heardDelay_ms, false, utility, shotPos, distance);
				}
			}
		}

		AIGroup myGroup = utility.GetOwner().GetParentGroup();
		if (myGroup && !endangeringForGroup && !DCO_NotifyAllowed(myGroup))
		{
			DCO_Perf.Count("cl:notify_throttled");
			myGroup = null;
		}
		if (myGroup)
		{
			float timeTillGroupNotified_s = Math.Min(timeTillFlyby_s, timeTillGunshotHeard_s);
			vector groupShotPos = DCO_ShotLocate.Estimate(utility, myGroup, instigatorEntity, shotPos, distance, ComputeAudibility(distance, isShotSuppressed, isInVehicle), isShotSuppressed);

			if (timeTillGroupNotified_s < 0)
				NotifyGroup(myGroup, shooter, instigatorEntity, instigatorFaction, groupShotPos, endangeringForGroup);
			else
			{
				DCO_Perf.Count("cl:shot");
				utility.GetCallqueue().CallLater(NotifyGroup, 1000*timeTillGroupNotified_s, false,
					myGroup, shooter, instigatorEntity, instigatorFaction, groupShotPos, endangeringForGroup);
			}
		}

		return true;
	}

	protected static ref map<AIGroup, float> s_mDCONotifyAt;
	protected static map<AIGroup, float> GetDCONotifyAt()
	{
		if (!s_mDCONotifyAt)
			s_mDCONotifyAt = new map<AIGroup, float>();
		return s_mDCONotifyAt;
	}

	protected bool DCO_NotifyAllowed(AIGroup grp)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		float last;
		if (GetDCONotifyAt().Find(grp, last) && now - last < 500.0)
			return false;
		if (GetDCONotifyAt().Count() > 256)
			GetDCONotifyAt().Clear();
		GetDCONotifyAt().Set(grp, now);
		return true;
	}

	protected void DCO_QueueFlyby(SCR_AIUtilityComponent utility, int count, vector pos, float weight, IEntity shooter, float delay_ms)
	{
		float nowF = GetGame().GetWorld().GetWorldTime();
		if (utility.m_iDCOFlyPending > 0 && nowF - utility.m_fDCOFlyAt > 3000.0)
			utility.m_iDCOFlyPending = 0;
		if (utility.m_iDCOFlyPending > 0)
		{
			if (shooter == utility.m_DCOFlyShooter)
			{
				utility.m_iDCOFlyPending += count;
				utility.m_fDCOFlyWeight = Math.Max(utility.m_fDCOFlyWeight, weight);
				utility.m_vDCOFlyPos = pos;
				DCO_Perf.Count("cl:shot_merged");
				return;
			}
			DCO_FireFlyby(utility);
		}
		utility.m_iDCOFlyPending = Math.Max(1, count);
		utility.m_fDCOFlyAt = nowF;
		utility.m_fDCOFlyWeight = weight;
		utility.m_vDCOFlyPos = pos;
		utility.m_DCOFlyShooter = shooter;
		if (delay_ms <= 0)
		{
			DCO_FireFlyby(utility);
			return;
		}
		DCO_Perf.Count("cl:shot");
		utility.GetCallqueue().CallLater(DCO_FireFlyby, delay_ms, false, utility);
	}

	protected void DCO_FireFlyby(SCR_AIUtilityComponent utility)
	{
		if (!utility || utility.m_iDCOFlyPending <= 0)
			return;
		int n = utility.m_iDCOFlyPending;
		utility.m_iDCOFlyPending = 0;
		DCO_OnProjectileFlyby(utility, n, utility.m_vDCOFlyPos, utility.m_fDCOFlyWeight, utility.m_DCOFlyShooter);
	}

	protected void DCO_QueueHeard(SCR_AIUtilityComponent utility, float distance, int count, vector pos, float delay_ms)
	{
		float nowH = GetGame().GetWorld().GetWorldTime();
		if (utility.m_iDCOHeardPending > 0 && nowH - utility.m_fDCOHeardAt > 3000.0)
			utility.m_iDCOHeardPending = 0;
		if (utility.m_iDCOHeardPending > 0)
		{
			utility.m_iDCOHeardPending += count;
			if (distance < utility.m_fDCOHeardDist)
			{
				utility.m_fDCOHeardDist = distance;
				utility.m_vDCOHeardPos = pos;
			}
			DCO_Perf.Count("cl:shot_merged");
			return;
		}
		utility.m_iDCOHeardPending = Math.Max(1, count);
		utility.m_fDCOHeardAt = nowH;
		utility.m_fDCOHeardDist = distance;
		utility.m_vDCOHeardPos = pos;
		if (delay_ms <= 0)
		{
			DCO_FireHeard(utility);
			return;
		}
		DCO_Perf.Count("cl:shot");
		utility.GetCallqueue().CallLater(DCO_FireHeard, delay_ms, false, utility);
	}

	protected void DCO_FireHeard(SCR_AIUtilityComponent utility)
	{
		if (!utility || utility.m_iDCOHeardPending <= 0)
			return;
		int n = utility.m_iDCOHeardPending;
		utility.m_iDCOHeardPending = 0;
		OnGunshotHeard(utility, utility.m_fDCOHeardDist, n, utility.m_vDCOHeardPos);
	}

	protected void DCO_QueueReact(SCR_AIUtilityComponent utility, vector pos, float distance, float threat, IEntity src, float delay_ms)
	{
		float nowR = GetGame().GetWorld().GetWorldTime();
		if (utility.m_bDCOReactPending && nowR - utility.m_fDCOReactAt > 3000.0)
			utility.m_bDCOReactPending = false;
		if (utility.m_bDCOReactPending)
		{
			if (threat > utility.m_fDCOReactThreat || (threat == utility.m_fDCOReactThreat && distance < utility.m_fDCOReactDist))
			{
				utility.m_vDCOReactPos = pos;
				utility.m_fDCOReactDist = distance;
				utility.m_fDCOReactThreat = threat;
				utility.m_DCOReactSrc = src;
			}
			if (threat >= m_fCoverThreatGate && utility.m_OwnerEntity)
			{
				float window_s = 5.0;
				if (utility.m_DCOConfig)
					window_s = utility.m_DCOConfig.GetDodgeShotWindow();
				RegisterDodgeShot(utility.m_OwnerEntity, window_s);
			}
			DCO_Perf.Count("cl:shot_merged");
			return;
		}
		utility.m_bDCOReactPending = true;
		utility.m_fDCOReactAt = nowR;
		utility.m_vDCOReactPos = pos;
		utility.m_fDCOReactDist = distance;
		utility.m_fDCOReactThreat = threat;
		utility.m_DCOReactSrc = src;
		if (delay_ms <= 0)
		{
			DCO_FireReact(utility);
			return;
		}
		DCO_Perf.Count("cl:shot");
		utility.GetCallqueue().CallLater(DCO_FireReact, delay_ms, false, utility);
	}

	protected void DCO_FireReact(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_bDCOReactPending)
			return;
		utility.m_bDCOReactPending = false;
		ReactToGunshot(utility, utility.m_vDCOReactPos, utility.m_fDCOReactDist, utility.m_fDCOReactThreat, utility.m_DCOReactSrc);
	}

	protected bool IsAudiable(float dist, bool isSuppressed)
	{
		if (!isSuppressed)
			return dist < AUDIBLE_DISTANCE_NORMAL;

		if (dist <= 30.0)
			return true;

		if (dist >= 150.0)
			return false;

		float t = (dist - 30.0) / (150.0 - 30.0);
		float chance = Math.Lerp(0.35, 0.0, t);

		return Math.RandomFloat01() < chance;
	}

	static bool DCO_IsCoverDebugOn()
	{
		return s_bDCOCoverDebug;
	}

	protected bool IsDebugOn(float distance)
	{
		return m_bDebugCoverReaction && distance <= m_fDebugMaxDist;
	}

	protected void DebugCover(IEntity ent, string msg)
	{
		string entName = "null";
		if (ent)
			entName = ent.ToString();

		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, "-", string.Format("t=%1 %2 | %3", GetGame().GetWorld().GetWorldTime(), entName, msg));
	}

	protected string DebugFailReason(SCR_AICombatMoveRequestBase rq)
	{
		if (!rq)
			return "none";

		return typename.EnumToString(SCR_EAICombatMoveRequestFailReason, rq.m_eFailReason);
	}

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

	protected bool RollAudible(float audibility)
	{
		if (audibility <= 0)
			return false;

		if (m_fAudibleGuaranteeThreshold <= 0 || audibility >= m_fAudibleGuaranteeThreshold)
			return true;

		float chance = audibility / m_fAudibleGuaranteeThreshold;
		return Math.RandomFloat01() < chance;
	}

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

	protected float ComputeMissDistance(vector myOrigin, vector shotPos, vector shotDir)
	{
		float dirLen = shotDir.Length();
		if (dirLen < 0.001)
			return 9999.0;

		vector dir     = shotDir * (1.0 / dirLen);
		vector headPos = myOrigin + Vector(0, 1.6, 0);
		vector toMe    = headPos - shotPos;

		float t = vector.Dot(toMe, dir);
		if (t < 0)
			return 9999.0;

		vector perpendicular = toMe - dir * t;
		return perpendicular.Length();
	}

	protected float ComputeThreatScore(float missDist, float audibility, int dangerEventCount)
	{
		float missScore = 0;
		if (missDist <= 2.0)
			missScore = 1.0;
		else if (missDist < 25.0)
			missScore = 1.0 - (missDist - 2.0) / (25.0 - 2.0);

		float burstScore = Math.Clamp(1.0 * dangerEventCount / 5.0, 0.0, 1.0);

		float threat = missScore * 0.6
					 + audibility * 0.25
					 + burstScore * 0.15;

		return Math.Clamp(threat, 0.0, 1.0);
	}

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

	protected void ReactToGunshot(SCR_AIUtilityComponent utility, vector perceivedShotPos, float distance, float threatScore, IEntity sourceEntity = null)
	{
		int perfStart = DCO_Perf.Begin();
		DCO_ReactToGunshot(utility, perceivedShotPos, distance, threatScore, sourceEntity);
		DCO_Perf.End("danger_shot", perfStart);
	}

	protected void DCO_ReactToGunshot(SCR_AIUtilityComponent utility, vector perceivedShotPos, float distance, float threatScore, IEntity sourceEntity)
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
			DCO_Perf.Count("cl:shot");
			if (m_bLookPreferClosest)
				utility.GetCallqueue().CallLater(LookAtShotLocked, lookDelay_ms, false, utility, perceivedShotPos, distance, sourceEntity);
			else
				utility.GetCallqueue().CallLater(LookAtShot, lookDelay_ms, false, utility, perceivedShotPos);
		}

		int coverDelay_ms = (int)(Math.RandomFloat(m_fCoverDelayMin, m_fCoverDelayMax) * 1000.0);
		DCO_Perf.Count("cl:shot");
		utility.GetCallqueue().CallLater(TryTakeCoverFromShot, coverDelay_ms, false, utility, perceivedShotPos, distance, threatScore);

		if (dbg)
		{
			DebugCover(utility.m_OwnerEntity, string.Format("HEARD dist=%1 threat=%2 lookChance=%3 roll=%4 lookDelay=%5ms coverDelay=%6ms executingNow=%7",
				distance, threatScore, lookChance, lookRoll, lookDelay_ms, coverDelay_ms, utility.m_CombatMoveState && utility.m_CombatMoveState.IsExecutingRequest()));
		}
	}

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

	protected void LookAtShotLocked(SCR_AIUtilityComponent utility, vector perceivedShotPos, float distance, IEntity sourceEntity)
	{
		if (!utility || !utility.m_OwnerEntity || !utility.m_LookAction)
			return;

		IEntity ownerEnt = utility.m_OwnerEntity;
		float now_ms = GetGame().GetWorld().GetWorldTime();

		DCO_ShotLookLock lock = GetLookLocks().Get(ownerEnt);
		bool active = false;
		if (lock)
			active = now_ms < lock.m_fUntil_ms;

		if (active)
		{
			bool sameSource = sourceEntity && lock.m_Source == sourceEntity;

			if (!sameSource && distance > lock.m_fDist * m_fLookOverrideDistRatio)
			{
				if (m_bDebugCoverReaction)
					DebugCover(ownerEnt, string.Format("LOOK SKIP: locked to a source at %1m, new source %2m (ratio %3)", lock.m_fDist, distance, m_fLookOverrideDistRatio));
				return;
			}

			if (sameSource)
			{
				lock.m_vPos      = perceivedShotPos;
				lock.m_fDist     = distance;
				lock.m_fPriority = ComputeLookPriority(distance);
				lock.m_fUntil_ms = now_ms + m_fLookLockDuration * 1000.0;

				utility.m_LookAction.LookAt(lock.m_vPos, lock.m_fPriority);

				if (m_bDebugCoverReaction)
					DebugCover(ownerEnt, string.Format("LOOK EXTEND: same source %1m prio=%2", distance, lock.m_fPriority));
				return;
			}
		}

		if (!lock)
		{
			lock = new DCO_ShotLookLock();
			GetLookLocks().Set(ownerEnt, lock);

			if (GetLookLocks().Count() > 128)
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
			DebugCover(ownerEnt, string.Format("LOOK LOCK -> %1 dist=%2m prio=%3 duration=%4s (replaces active=%5)", perceivedShotPos, distance, lock.m_fPriority, m_fLookLockDuration, active));

		if (m_fLookLockRefreshInterval > 0)
		{
			DCO_Perf.Count("cl:shot");
			utility.GetCallqueue().CallLater(RefreshLookLock, (int)(m_fLookLockRefreshInterval * 1000.0), false,
				utility, lock.m_iToken);
		}
	}

	protected void RefreshLookLock(SCR_AIUtilityComponent utility, int token)
	{
		if (!utility || !utility.m_OwnerEntity || !utility.m_LookAction)
			return;

		DCO_ShotLookLock lock = GetLookLocks().Get(utility.m_OwnerEntity);
		if (!lock || lock.m_iToken != token)
			return;

		if (GetGame().GetWorld().GetWorldTime() >= lock.m_fUntil_ms)
		{
			if (m_bDebugCoverReaction)
				DebugCover(utility.m_OwnerEntity, "LOOK UNLOCK: lock duration expired");
			return;
		}

		utility.m_LookAction.LookAt(lock.m_vPos, lock.m_fPriority);

		DCO_Perf.Count("cl:shot");
		utility.GetCallqueue().CallLater(RefreshLookLock, (int)(m_fLookLockRefreshInterval * 1000.0), false,
			utility, token);
	}

	protected vector ResolveLockedLookPos(notnull SCR_AIUtilityComponent utility, vector fallbackPos)
	{
		if (!m_bLookPreferClosest || !utility.m_OwnerEntity)
			return fallbackPos;

		DCO_ShotLookLock lock = GetLookLocks().Get(utility.m_OwnerEntity);
		if (!lock || GetGame().GetWorld().GetWorldTime() >= lock.m_fUntil_ms)
			return fallbackPos;

		return lock.m_vPos;
	}

	protected void PruneLookLockMap(float now_ms)
	{
		array<IEntity> toRemove = {};

		foreach (IEntity ent, DCO_ShotLookLock lock : GetLookLocks())
		{
			if (!ent || !lock || now_ms >= lock.m_fUntil_ms)
				toRemove.Insert(ent);
		}

		foreach (IEntity entRemove : toRemove)
		{
			GetLookLocks().Remove(entRemove);
		}
	}

	protected void LookAtShot(SCR_AIUtilityComponent utility, vector perceivedShotPos)
	{
		if (!utility || !utility.m_OwnerEntity)
			return;

		if (!utility.m_LookAction)
		{
			if (m_bDebugCoverReaction)
				DebugCover(utility.m_OwnerEntity, "LOOK failed: m_LookAction null");
			return;
		}

		utility.m_LookAction.LookAt(perceivedShotPos, SCR_AILookAction.PRIO_DANGER_EVENT);

		if (m_bDebugCoverReaction)
			DebugCover(utility.m_OwnerEntity, string.Format("LOOK -> %1", perceivedShotPos));
	}

	protected void LowerStance(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_CombatComponent)
			return;

		SCR_CharacterControllerComponent charCon = utility.m_CombatComponent.GetCharacterController();
		if (!charCon || DCO_Nav.Busy(charCon, "shot_stance"))
			return;

		if (charCon.GetStance() == ECharacterStance.STAND)
			DCO_Stance.Request(charCon, 2, "fired_at");
	}

	protected void ScheduleLookAfterCover(notnull SCR_AIUtilityComponent utility, vector perceivedShotPos, float moveDuration_s)
	{
		if (!m_bLookAfterCover || m_iLookAfterCoverRepeats <= 0)
			return;

		int delay_ms = (int)((moveDuration_s + m_fLookAfterCoverDelay) * 1000.0);

		DCO_Perf.Count("cl:shot");
		utility.GetCallqueue().CallLater(LookAfterCover, delay_ms, false,
			utility, perceivedShotPos, m_iLookAfterCoverRepeats);
	}

	protected void LookAfterCover(SCR_AIUtilityComponent utility, vector perceivedShotPos, int repeatsLeft)
	{
		if (!utility || !utility.m_OwnerEntity)
			return;

		LookAtShot(utility, ResolveLockedLookPos(utility, perceivedShotPos));

		repeatsLeft--;
		if (repeatsLeft <= 0)
			return;

		DCO_Perf.Count("cl:shot");
		utility.GetCallqueue().CallLater(LookAfterCover, (int)(m_fLookAfterCoverInterval * 1000.0), false,
			utility, perceivedShotPos, repeatsLeft);
	}

	protected void TryTakeCoverFromShot(SCR_AIUtilityComponent utility, vector perceivedShotPos, float distance, float threatScore)
	{
		int perfStart = DCO_Perf.Begin();
		DCO_TryTakeCoverFromShot(utility, perceivedShotPos, distance, threatScore);
		DCO_Perf.End("danger_shot", perfStart);
	}

	protected void DCO_DodgeNote(string key, bool done)
	{
		if (done)
			DCO_Perf.Count("dodge:" + key);
		else
			DCO_Perf.Count("dodge_stop:" + key);
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetInstance();
		if (g)
			g.DCO_DodgeAgg(key);
	}

	protected bool DCO_CoverCompromisedNote(SCR_AIUtilityComponent utility, vector threatPos)
	{
		int rayBlocked;
		float ang;
		string why = DCO_DodgeAssist.CoverCompromised(utility, threatPos, rayBlocked, ang);
		if (why.IsEmpty())
			return false;
		DCO_DodgeNote("cover_compromised_" + why, true);
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(utility.m_OwnerEntity), string.Format("dodge_cover_compromised unit=%1 why=%2 ang=%3 ray_blocked=%4",
				DCO_Log.IdUnit(utility.m_OwnerEntity), why, Math.Round(ang), rayBlocked));
		return true;
	}

	protected int DCO_AssistApply(SCR_AIUtilityComponent utility, SCR_AICombatMoveRequest_Move rq, vector threatPos, float searchDist, bool losOnly)
	{
		string pick;
		int r = DCO_DodgeAssist.Apply(utility, rq, threatPos, searchDist, losOnly, pick);
		if (!pick.IsEmpty())
			DCO_DodgeNote("dir_" + pick, true);
		if (r == 2 && utility.m_OwnerEntity)
		{
			GetLastDodgeTime().Remove(utility.m_OwnerEntity);
			DCO_DodgeNote("done_cancelled", true);
		}
		return r;
	}

	protected void DCO_TryTakeCoverFromShot(SCR_AIUtilityComponent utility, vector perceivedShotPos, float distance, float threatScore)
	{
		if (!utility || !utility.m_OwnerEntity)
			return;

		if (DCO_AssaultLock.DodgeCap(utility) == 0)
		{
			LowerStance(utility);
			DCO_DodgeNote("cov_assault_lock", false);
			return;
		}

		BaseTarget dnT;
		if (utility.m_CombatComponent)
			dnT = utility.m_CombatComponent.GetCurrentTarget();
		if (dnT && dnT.GetTargetEntity() && utility.m_CombatComponent.IsTargetVisible(dnT) && vector.Distance(dnT.GetTargetEntity().GetOrigin(), utility.m_OwnerEntity.GetOrigin()) < DCO_Stress.T(utility, "rt.close_m", 30.0))
		{
			LowerStance(utility);
			DCO_DodgeNote("cov_near_target", false);
			return;
		}

		DCO_DodgeNote("enter_cov", true);
		DCO_AIConfigComponent cfg = utility.m_DCOConfig;

		float maxDist    = 250.0;
		float cooldown_s = 8.0;
		float chance     = 0.6;
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
			DCO_DodgeNote("cov_dist", false);
			if (dbg) DebugCover(ownerEnt, string.Format("COVER STOP: dist %1 > maxDist %2", distance, maxDist));
			return;
		}

		int   shotThreshold = 1;
		float shotWindow_s  = 5.0;
		if (cfg)
		{
			shotThreshold = cfg.GetDodgeShotThreshold();
			shotWindow_s  = cfg.GetDodgeShotWindow();
		}

		float gate = m_fCoverThreatGate;
		if (DCO_Stress.Tier(utility) >= 1)
		{
			gate = 0;
			DCO_Perf.Count("tm:dodge_stressed");
		}
		string gateWhy = DCO_DodgeAssist.GateBypass(utility);
		if (gate > 0 && !gateWhy.IsEmpty())
		{
			gate = 0;
			DCO_DodgeNote("gate_" + gateWhy, true);
		}
		int shotCount;
		if (threatScore >= gate)
			shotCount = RegisterDodgeShot(ownerEnt, shotWindow_s);
		else
			shotCount = GetDodgeShotCount(ownerEnt, shotWindow_s);

		SCR_AICombatMoveState state = utility.m_CombatMoveState;
		if (!state)
		{
			DCO_DodgeNote("cov_no_state", false);
			if (dbg) DebugCover(ownerEnt, "COVER STOP: m_CombatMoveState null");
			return;
		}

		if (m_bCoverVeryHighPriority)
		{
			if (state.DCO_IsCoverProtected())
			{
				DCO_DodgeNote("cov_protected", false);
				if (dbg) DebugCover(ownerEnt, "COVER STOP: already running to cover (protected)");
				return;
			}

			if (state.IsExecutingRequest() && dbg)
				DebugCover(ownerEnt, string.Format("COVER OVERRIDE: another request is running -> it will be replaced (oldRq fail=%1)", DebugFailReason(state.GetOldRequest())));
		}
		else if (state.IsExecutingRequest())
		{
			DCO_DodgeNote("cov_executing", false);
			if (dbg) DebugCover(ownerEnt, string.Format("COVER STOP: IsExecutingRequest = true (oldRq fail=%1)", DebugFailReason(state.GetOldRequest())));
			return;
		}

		if (cfg && cfg.IsHoldPosition())
		{
			DCO_DodgeNote("cov_hold", false);
			if (dbg) DebugCover(ownerEnt, "COVER STOP: HoldPosition");
			return;
		}

		if (utility.m_AIInfo && utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
		{
			DCO_DodgeNote("cov_vehicle", false);
			if (dbg) DebugCover(ownerEnt, "COVER STOP: IN_VEHICLE");
			return;
		}

		if (state.IsInValidCover() && !DCO_CoverCompromisedNote(utility, perceivedShotPos))
		{
			DCO_DodgeNote("cov_in_cover", false);
			if (DCO_DodgeAssist.RecentNearMiss(utility))
			{
				DCO_DodgeNote("cover_hold_lower", true);
				LowerStance(utility);
			}
			if (dbg) DebugCover(ownerEnt, "COVER STOP: IsInValidCover = true");
			return;
		}

		if (threatScore < gate)
		{
			DCO_DodgeNote("cov_low_threat", false);
			if (dbg) DebugCover(ownerEnt, string.Format("COVER STOP: threat %1 < gate %2 -> lower stance only", threatScore, m_fCoverThreatGate));

			if (m_bLowerStanceOnLowThreat)
				LowerStance(utility);

			return;
		}

		if (!DCO_CoverMoveBudget.CanMove(ownerEnt, m_fSharedMoveCooldown))
		{
			DCO_DodgeNote("cov_budget", false);
			if (dbg) DebugCover(ownerEnt, string.Format("COVER STOP: move budget (last move %1s ago, minimum %2s)",
				DCO_CoverMoveBudget.GetTimeSinceLastMove(ownerEnt), m_fSharedMoveCooldown));
			return;
		}

		if (SCR_CoverManagerComponent.IsEntityInsideBuilding(ownerEnt))
		{
			DCO_DodgeNote("cov_in_building", false);
			if (dbg) DebugCover(ownerEnt, "COVER STOP: inside a building");
			return;
		}

		if (shotCount < shotThreshold)
		{
			DCO_DodgeNote("cov_shots", false);
			if (dbg) DebugCover(ownerEnt, string.Format("COVER STOP: shots %1/%2 (window %3s)", shotCount, shotThreshold, shotWindow_s));
			return;
		}

		if (!CanDodgeNow(ownerEnt, cooldown_s))
		{
			DCO_DodgeNote("cov_cooldown", false);
			if (dbg) DebugCover(ownerEnt, string.Format("COVER STOP: cooldown active (%1s)", cooldown_s));
			return;
		}

		MarkDodged(ownerEnt, cooldown_s);
		ResetDodgeShots(ownerEnt);

		float baseChance = chance;
		float persScale  = 1.0;
		if (scalePers)
		{
			persScale = DCO_PersonalityCombatUtility.GetTakeCoverChanceScale(utility);
			chance *= persScale;
		}

		chance = Math.Clamp(chance, 0.0, 1.0);
		chance = chance + threatScore * (1.0 - chance);

		if (m_bCoverVeryHighPriority && !m_bUseDodgeChanceOnCover)
			chance = 1.0;

		float coverRoll = Math.RandomFloat01();
		if (coverRoll >= chance)
		{
			DCO_DodgeNote("cov_roll", false);
			if (dbg) DebugCover(ownerEnt, string.Format("COVER STOP: roll failed roll=%1 chance=%2 (base=%3 pers=%4 threat=%5) -> cooldown still runs",
				coverRoll, chance, baseChance, persScale, threatScore));
			return;
		}

		if (!m_bCoverPreferBuilding)
		{
			DCO_DodgeNote("done_cov_move", true);
			if (dbg) DebugCover(ownerEnt, string.Format("COVER GO: MOVE-cover (buildings disabled) roll=%1 chance=%2 threat=%3", coverRoll, chance, threatScore));
			PushCoverMove(utility, state, perceivedShotPos, threatScore);
			return;
		}

		SCR_AICombatMoveRequestBase oldRq = state.GetOldRequest();
		if (oldRq && oldRq.m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
		{
			DCO_DodgeNote("done_cov_move_nobld", true);
			if (dbg) DebugCover(ownerEnt, string.Format("COVER GO: direct MOVE (oldRq failed NO_BUILDING_FOUND) roll=%1 chance=%2 threat=%3", coverRoll, chance, threatScore));
			PushCoverMove(utility, state, perceivedShotPos, threatScore);
			return;
		}

		DCO_DodgeNote("done_cov_building", true);
		if (dbg) DebugCover(ownerEnt, string.Format("COVER GO: BUILDING first roll=%1 chance=%2 threat=%3 oldRq fail=%4", coverRoll, chance, threatScore, DebugFailReason(oldRq)));

		SCR_AICombatMoveRequest_Move buildingRq = PushBuildingMove(utility, state, perceivedShotPos);
		DCO_Perf.Count("cl:shot");
		utility.GetCallqueue().CallLater(CheckBuildingFallback, 250, false,
			utility, buildingRq, perceivedShotPos, threatScore, 8);
	}

	protected SCR_AICombatMoveRequest_Move PushBuildingMove(notnull SCR_AIUtilityComponent utility, notnull SCR_AICombatMoveState state, vector perceivedShotPos)
	{
		float searchDist = DCO_AIConfigComponent.DodgeSearchDistOf(utility);

		if (m_fCoverSearchDistCap > 0)
			searchDist = Math.Min(searchDist, m_fCoverSearchDistCap);
		float dodgeCap = DCO_AssaultLock.DodgeCap(utility);
		if (dodgeCap > 0)
			searchDist = Math.Min(searchDist, dodgeCap);

		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

		rq.m_eType      = SCR_EAICombatMoveRequestType.BUILDING;
		rq.m_eReason    = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
		rq.m_vTargetPos = perceivedShotPos;
		rq.m_vMovePos   = rq.m_vTargetPos;

		rq.m_bTryFindCover              = false;
		rq.m_bFailIfNoCover             = false;
		rq.m_bUseCoverSearchDirectivity = false;
		rq.m_bCheckCoverVisibility      = false;

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

		int asst = DCO_AssistApply(utility, rq, perceivedShotPos, searchDist, true);
		if (asst == 2)
			return null;
		if (asst == 0)
			ApplyWaypointDodgeBias(utility, rq, searchDist);

		DCO_CoverMoveBudget.MarkMove(utility.m_OwnerEntity);

		float buildingHold_s = (250 * 8) / 1000.0 + 0.25;

		if (m_bCoverVeryHighPriority)
			state.DCO_ApplyCoverRequest(rq, Math.Max(buildingHold_s, rq.m_fMoveDuration_s + 1.0));
		else
			state.ApplyNewRequest(rq);

		ScheduleLookAfterCover(utility, perceivedShotPos, rq.m_fMoveDuration_s);

		if (m_bDebugCoverReaction)
			DebugCover(utility.m_OwnerEntity, string.Format("PUSH BUILDING searchDist=%1 dir=%2 executingAfter=%3", searchDist, typename.EnumToString(SCR_EAICombatMoveDirection, rq.m_eDirection), state.IsExecutingRequest()));

		return rq;
	}

	protected void PushCoverMove(notnull SCR_AIUtilityComponent utility, notnull SCR_AICombatMoveState state, vector perceivedShotPos, float threatScore)
	{
		float searchDist = DCO_AIConfigComponent.DodgeSearchDistOf(utility);

		if (m_fCoverSearchDistCap > 0)
			searchDist = Math.Min(searchDist, m_fCoverSearchDistCap);
		float dodgeCap = DCO_AssaultLock.DodgeCap(utility);
		if (dodgeCap > 0)
			searchDist = Math.Min(searchDist, dodgeCap);

		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

		rq.m_eType      = SCR_EAICombatMoveRequestType.MOVE;
		rq.m_eReason    = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
		rq.m_vTargetPos = perceivedShotPos;
		rq.m_vMovePos   = rq.m_vTargetPos;

		rq.m_bTryFindCover              = true;
		rq.m_bFailIfNoCover             = false;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility      = true;

		rq.m_eDirection = ResolveCoverDirectionForUnit(utility, threatScore < m_fHighThreatProneThreshold);
		rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;

		rq.m_eStanceMoving = ECharacterStance.STAND;
		rq.m_eMovementType = EMovementType.SPRINT;
		state.DCO_SetDodgeStance(rq);

		rq.m_bAimAtTarget    = false;
		rq.m_bAimAtTargetEnd = true;

		rq.m_fCoverSearchDistMin = 0;
		rq.m_fCoverSearchDistMax = searchDist;
		rq.m_fMoveDuration_s     = searchDist / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;

		int asst = DCO_AssistApply(utility, rq, perceivedShotPos, searchDist, false);
		if (asst == 2)
			return;
		if (asst == 0)
			ApplyWaypointDodgeBias(utility, rq, searchDist);

		DCO_CoverMoveBudget.MarkMove(utility.m_OwnerEntity);

		if (m_bCoverVeryHighPriority)
			state.DCO_ApplyCoverRequest(rq, rq.m_fMoveDuration_s + 1.0);
		else
			state.ApplyNewRequest(rq);

		ScheduleLookAfterCover(utility, perceivedShotPos, rq.m_fMoveDuration_s);

		if (m_bDebugCoverReaction)
		{
			DebugCover(utility.m_OwnerEntity, string.Format("PUSH MOVE-cover searchDist=%1 dir=%2 stanceEnd=%3 executingAfter=%4",
				searchDist, typename.EnumToString(SCR_EAICombatMoveDirection, rq.m_eDirection), typename.EnumToString(ECharacterStance, rq.m_eStanceEnd), state.IsExecutingRequest()));
		}

		DCO_Perf.Count("cl:shot");
		utility.GetCallqueue().CallLater(CheckCoverMoveWatchdog, 250, false,
			utility, rq, 8);
	}

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
				DebugCover(utility.m_OwnerEntity, "MOVE-cover OK: request executed");
			return;
		}

		if (coverRq.m_eFailReason != SCR_EAICombatMoveRequestFailReason.NONE)
		{
			if (m_bDebugCoverReaction)
				DebugCover(utility.m_OwnerEntity, string.Format("MOVE-cover FAILED fail=%1 -> protection released", DebugFailReason(coverRq)));

			state.DCO_ReleaseCoverProtection("cover-failed");
			return;
		}

		pollsLeft--;
		if (pollsLeft <= 0)
		{
			if (m_bDebugCoverReaction)
				DebugCover(utility.m_OwnerEntity, "MOVE-cover STUCK: never executed -> protection released, AI back to normal behavior");

			state.DCO_ReleaseCoverProtection("cover-stuck");
			return;
		}

		DCO_Perf.Count("cl:shot");
		utility.GetCallqueue().CallLater(CheckCoverMoveWatchdog, 250, false,
			utility, coverRq, pollsLeft);
	}

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
			if (executing && !m_bCoverVeryHighPriority)
			{
				if (m_bDebugCoverReaction) DebugCover(ownerEnt, "FALLBACK STOP: no building, but another request is running -> no cover push");
				return;
			}

			if (utility.m_DCOConfig && utility.m_DCOConfig.IsHoldPosition())
			{
				if (m_bDebugCoverReaction) DebugCover(ownerEnt, "FALLBACK STOP: HoldPosition");
				return;
			}

			if (m_bDebugCoverReaction) DebugCover(ownerEnt, "FALLBACK GO: no building -> push MOVE-cover");
			PushCoverMove(utility, state, perceivedShotPos, threatScore);
			return;
		}

		if (executing)
		{
			if (m_bDebugCoverReaction)
				DebugCover(ownerEnt, "BUILDING OK: request executed");
			return;
		}

		pollsLeft--;
		if (pollsLeft <= 0)
		{
			if (m_bDebugCoverReaction)
				DebugCover(ownerEnt, string.Format("BUILDING STUCK: not executed (fail=%1) -> trying MOVE-cover", DebugFailReason(buildingRq)));

			if (utility.m_DCOConfig && utility.m_DCOConfig.IsHoldPosition())
			{
				state.DCO_ReleaseCoverProtection("stuck-hold");
				return;
			}

			PushCoverMove(utility, state, perceivedShotPos, threatScore);
			return;
		}

		DCO_Perf.Count("cl:shot");
		utility.GetCallqueue().CallLater(CheckBuildingFallback, 250, false,
			utility, buildingRq, perceivedShotPos, threatScore, pollsLeft);
	}

	protected bool CanInvestigateNow(IEntity entity)
	{
		if (!entity)
			return false;

		float now_ms = GetGame().GetWorld().GetWorldTime();

		float lastTime_ms;
		if (!GetLastInvestigateTime().Find(entity, lastTime_ms))
			return true;

		return (now_ms - lastTime_ms) > (m_fInvestigateCooldown * 1000.0);
	}

	protected void MarkInvestigated(IEntity entity)
	{
		if (!entity)
			return;

		float now_ms = GetGame().GetWorld().GetWorldTime();
		GetLastInvestigateTime().Set(entity, now_ms);

		if (GetLastInvestigateTime().Count() > 128)
			PruneInvestigateMap(now_ms);
	}

	protected void PruneInvestigateMap(float now_ms)
	{
		float staleAge_ms = m_fInvestigateCooldown * 1000.0 * 2.0;

		array<IEntity> toRemove = {};

		foreach (IEntity ent, float lastTime_ms : GetLastInvestigateTime())
		{
			if (!ent || (now_ms - lastTime_ms) > staleAge_ms)
				toRemove.Insert(ent);
		}

		foreach (IEntity ent : toRemove)
		{
			GetLastInvestigateTime().Remove(ent);
		}
	}

	protected void TryDodge(SCR_AIUtilityComponent utility, vector shotPos, float distance)
	{
		if (!utility || !utility.m_OwnerEntity)
			return;

		if (DCO_AssaultLock.DodgeCap(utility) == 0)
		{
			LowerStance(utility);
			DCO_DodgeNote("dg_assault_lock", false);
			return;
		}

		DCO_DodgeNote("enter_dg", true);
		DCO_AIConfigComponent cfg = utility.m_DCOConfig;

		float maxDist    = 250.0;
		float cooldown_s = 8.0;
		float chance     = 0.6;
		bool  scalePers  = true;

		if (cfg)
		{
			maxDist    = cfg.GetDodgeMaxDist();
			cooldown_s = cfg.GetDodgeCooldown();
			chance     = cfg.GetDodgeChance();
			scalePers  = cfg.GetDodgeScaleByPersonality();
		}

		if (distance > maxDist)
		{
			DCO_DodgeNote("dg_dist", false);
			return;
		}

		int   shotThreshold = 1;
		float shotWindow_s  = 5.0;
		if (cfg)
		{
			shotThreshold = cfg.GetDodgeShotThreshold();
			shotWindow_s  = cfg.GetDodgeShotWindow();
		}

		int shotCount = RegisterDodgeShot(utility.m_OwnerEntity, shotWindow_s);

		SCR_AICombatMoveState state = utility.m_CombatMoveState;
		if (!state || state.IsExecutingRequest())
		{
			DCO_DodgeNote("dg_executing", false);
			return;
		}

		if (cfg && cfg.IsHoldPosition())
		{
			DCO_DodgeNote("dg_hold", false);
			return;
		}

		if (utility.m_AIInfo && utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
		{
			DCO_DodgeNote("dg_vehicle", false);
			return;
		}

		if (state.IsInValidCover() && !DCO_CoverCompromisedNote(utility, shotPos))
		{
			DCO_DodgeNote("dg_in_cover", false);
			return;
		}

		if (SCR_CoverManagerComponent.IsEntityInsideBuilding(utility.m_OwnerEntity))
		{
			DCO_DodgeNote("dg_in_building", false);
			return;
		}

		if (shotCount < shotThreshold)
		{
			DCO_DodgeNote("dg_shots", false);
			return;
		}

		if (!CanDodgeNow(utility.m_OwnerEntity, cooldown_s))
		{
			DCO_DodgeNote("dg_cooldown", false);
			return;
		}

		MarkDodged(utility.m_OwnerEntity, cooldown_s);
		ResetDodgeShots(utility.m_OwnerEntity);

		if (scalePers)
			chance *= DCO_PersonalityCombatUtility.GetTakeCoverChanceScale(utility);

		if (Math.RandomFloat01() >= Math.Clamp(chance, 0.0, 1.0))
		{
			DCO_DodgeNote("dg_roll", false);
			return;
		}

		DCO_DodgeNote("done_dg", true);
		PushDodgeMove(utility, state, shotPos);
	}

	protected void PushDodgeMove(notnull SCR_AIUtilityComponent utility, notnull SCR_AICombatMoveState state, vector shotPos)
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

		float searchDist = DCO_AIConfigComponent.DodgeSearchDistOf(utility);

		if (m_fCoverSearchDistCap > 0)
			searchDist = Math.Min(searchDist, m_fCoverSearchDistCap);
		float dodgeCap = DCO_AssaultLock.DodgeCap(utility);
		if (dodgeCap > 0)
			searchDist = Math.Min(searchDist, dodgeCap);

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

		rq.m_eDirection = ResolveCoverDirectionForUnit(utility, rq.m_eType == SCR_EAICombatMoveRequestType.MOVE);
		rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;

		rq.m_eStanceMoving = ECharacterStance.STAND;
		rq.m_eStanceEnd    = ECharacterStance.CROUCH;
		rq.m_eMovementType = EMovementType.SPRINT;
		if (rq.m_eType == SCR_EAICombatMoveRequestType.MOVE)
			state.DCO_SetDodgeStance(rq);

		rq.m_bAimAtTarget    = false;
		rq.m_bAimAtTargetEnd = true;

		rq.m_fCoverSearchDistMin = 0;
		rq.m_fCoverSearchDistMax = searchDist;
		rq.m_fMoveDuration_s     = searchDist / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;

		int asst = DCO_AssistApply(utility, rq, shotPos, searchDist, rq.m_eType == SCR_EAICombatMoveRequestType.BUILDING);
		if (asst == 2)
			return;
		if (asst == 0)
			ApplyWaypointDodgeBias(utility, rq, searchDist);

		state.ApplyNewRequest(rq);
	}

	protected void ApplyWaypointDodgeBias(notnull SCR_AIUtilityComponent utility, notnull SCR_AICombatMoveRequest_Move rq, float searchDist)
	{
		if (!m_bDodgeTowardWaypoint || DCO_AssaultLock.DodgeCap(utility) > 0)
			return;

		if (!DCO_DodgeWaypointUtility.ApplyWaypointBias(utility, rq, searchDist))
			return;

		if (m_bDebugCoverReaction)
			DebugCover(utility.m_OwnerEntity, string.Format("WP BIAS: dodge bent toward waypoint %1", rq.m_vMovePos));
	}

	protected SCR_EAICombatMoveDirection ResolveCoverDirection()
	{
		if (!m_bRandomizeCoverDirection)
			return SCR_EAICombatMoveDirection.BACKWARD;

		return DCO_Util.PickCoverDirection(m_fCoverDirWeightBackward, m_fCoverDirWeightLeft, m_fCoverDirWeightRight, m_fCoverDirWeightAnywhere);
	}

	protected SCR_EAICombatMoveDirection ResolveCoverDirectionForUnit(SCR_AIUtilityComponent utility, bool allowForward)
	{
		if (!m_bRandomizeCoverDirection || !m_bScaleCoverDirByPersonality || !utility)
			return DCO_Osc.KeepDir(utility, ResolveCoverDirection(), allowForward, "weapon_fired");

		return DCO_Osc.KeepDir(utility, DCO_PersonalityCombatUtility.PickCoverDirectionForPersonality(utility,
			m_fCoverDirWeightBackward, m_fCoverDirWeightLeft, m_fCoverDirWeightRight, m_fCoverDirWeightAnywhere,
			m_fCoverDirForwardWeightScale, allowForward), allowForward, "weapon_fired");
	}

	protected int GetDodgeShotCount(IEntity entity, float window_s)
	{
		if (!entity)
			return 0;

		float last_ms;
		if (!GetDodgeShotLast_ms().Find(entity, last_ms))
			return 0;

		if ((GetGame().GetWorld().GetWorldTime() - last_ms) > (window_s * 1000.0))
			return 0;

		int count;
		GetDodgeShotCountStore().Find(entity, count);
		return count;
	}

	protected int RegisterDodgeShot(IEntity entity, float window_s)
	{
		if (!entity)
			return 0;

		int count = GetDodgeShotCount(entity, window_s) + 1;

		float now_ms = GetGame().GetWorld().GetWorldTime();
		GetDodgeShotCountStore().Set(entity, count);
		GetDodgeShotLast_ms().Set(entity, now_ms);

		if (GetDodgeShotLast_ms().Count() > 128)
			PruneDodgeShotMaps(now_ms, window_s);

		return count;
	}

	protected void ResetDodgeShots(IEntity entity)
	{
		if (!entity)
			return;

		GetDodgeShotCountStore().Remove(entity);
		GetDodgeShotLast_ms().Remove(entity);
	}

	protected void PruneDodgeShotMaps(float now_ms, float window_s)
	{
		float staleAge_ms = window_s * 1000.0 * 2.0;

		array<IEntity> toRemove = {};
		foreach (IEntity ent, float last_ms : GetDodgeShotLast_ms())
		{
			if (!ent || (now_ms - last_ms) > staleAge_ms)
				toRemove.Insert(ent);
		}

		foreach (IEntity ent : toRemove)
		{
			GetDodgeShotCountStore().Remove(ent);
			GetDodgeShotLast_ms().Remove(ent);
		}
	}

	protected bool CanDodgeNow(IEntity entity, float cooldown_s)
	{
		if (!entity)
			return false;

		float lastTime_ms;
		if (!GetLastDodgeTime().Find(entity, lastTime_ms))
			return true;

		return (GetGame().GetWorld().GetWorldTime() - lastTime_ms) > (cooldown_s * 1000.0);
	}

	protected void MarkDodged(IEntity entity, float cooldown_s)
	{
		if (!entity)
			return;

		float now_ms = GetGame().GetWorld().GetWorldTime();
		GetLastDodgeTime().Set(entity, now_ms);

		if (GetLastDodgeTime().Count() > 128)
			PruneDodgeMap(now_ms, cooldown_s);
	}

	protected void PruneDodgeMap(float now_ms, float cooldown_s)
	{
		float staleAge_ms = cooldown_s * 1000.0 * 2.0;

		array<IEntity> toRemove = {};

		foreach (IEntity ent, float lastTime_ms : GetLastDodgeTime())
		{
			if (!ent || (now_ms - lastTime_ms) > staleAge_ms)
				toRemove.Insert(ent);
		}

		foreach (IEntity ent : toRemove)
		{
			GetLastDodgeTime().Remove(ent);
		}
	}
}
modded class SCR_AIUtilityComponent
{
	IEntity m_DCOShotInstig;
	float m_fDCOShotInstigAt;
	Faction m_DCOShotFaction;
	bool m_bDCOShotEnemyOk;
	int m_iDCOFlyPending;
	float m_fDCOFlyAt;
	float m_fDCOHeardAt;
	float m_fDCOReactAt;
	float m_fDCOFlyWeight;
	vector m_vDCOFlyPos;
	IEntity m_DCOFlyShooter;
	int m_iDCOHeardPending;
	float m_fDCOHeardDist;
	vector m_vDCOHeardPos;
	bool m_bDCOReactPending;
	vector m_vDCOReactPos;
	float m_fDCOReactDist;
	float m_fDCOReactThreat;
	IEntity m_DCOReactSrc;
	float m_fDCOGunfireAt = -100000;
}
