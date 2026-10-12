modded enum SCR_EAICombatMoveReason
{
	INVESTIGATE,
	SUPPLYING
}

modded enum SCR_EAICombatMoveRequestType
{
	INVESTIGATE,
	RESUPPLYING,
	BUILDING,
	INDOOR_RELOCATE
}

modded class SCR_AICombatMoveLogic_Attack : SCR_AICombatMoveLogicBase
{
	protected static const string PORT_AVOID_STRAIGHT_PATH_DIR = "AvoidStraightPathDir";
	protected static const string PORT_BASE_TARGET = "BaseTarget";

	protected BaseTarget m_Target;
	protected vector m_vAvoidStraightPathDir;
	protected DCO_AIMoraleSystem moraleSystem;







	protected static ref RandomGenerator s_CohesionRand;
	protected static RandomGenerator GetCohesionRand()
	{
		if (!s_CohesionRand)
			s_CohesionRand = new RandomGenerator();
		return s_CohesionRand;
	}

	protected float m_fNextEngagedBid_ms = -1;
	protected float m_fNextCriticalBid_ms = -1;

	protected float m_fCriticalEnterTime_ms = -1;

	protected bool m_bInOpenAreaCached = false;
	protected float m_fDCOWaitRoll = 0.5;
	protected float m_fDCOLastStopped_s;

	protected override bool OnUpdate(AIAgent owner, float dt)
	{
		GetVariableIn(PORT_BASE_TARGET, m_Target);
		GetVariableIn("AvoidStraightPathDir", m_vAvoidStraightPathDir);

		if (!m_Target || !m_Target.GetTargetEntity())
			return false;

		moraleSystem = m_Utility.GetMoraleSystem();
		return true;
	}

	protected override float GetTargetDistance()
	{
		return m_Target.GetDistance();
	}

	protected override vector GetTargetPosition()
	{
		return m_Target.GetLastSeenPosition();
	}

	protected override vector GetAvoidStraightPathDir()
	{
		return m_vAvoidStraightPathDir;
	}


	protected bool IsInvestigating()
	{
		if (!m_Target)
			return false;

		if (m_CombatComp.IsTargetVisible(m_Target))
			return false;

		float tss = m_Target.GetTimeSinceSeen();
		if (tss < 8.0 || tss > 30.0)
			return false;

		return m_Target.GetDistance() < 60.0;
	}

	protected bool CriticalBoundCooldownReady()
	{
	    return GetGame().GetWorld().GetWorldTime() >= m_fNextCriticalBid_ms;
	}

	protected void UpdateCriticalTimer(float now_ms)
	{
		if (!IsCriticalCombatMoment())
		{
			m_fCriticalEnterTime_ms = -1;
			return;
		}

		if (m_fCriticalEnterTime_ms < 0)
			m_fCriticalEnterTime_ms = now_ms;
	}

	protected float GetCriticalElapsed_s(float now_ms)
	{
		if (m_fCriticalEnterTime_ms < 0)
			return 0;

		return (now_ms - m_fCriticalEnterTime_ms) / 1000.0;
	}

	protected bool ShouldForceReturnFire(float now_ms)
	{
		float threshold_s = 5.0 * DCO_PersonalityCombatUtility.GetReturnFireDelayScale(m_Utility);

		return GetCriticalElapsed_s(now_ms) > threshold_s;
	}

	protected bool ShouldRegroupToLeader(out vector outLeaderPos, out float outRegroupRadius)
	{
		outLeaderPos = vector.Zero;
		outRegroupRadius = 0;

		AIAgent agent = m_Utility.GetAIAgent();
		if (!agent)
			return false;

		AIGroup group = agent.GetParentGroup();
		if (!group || agent == group.GetLeaderAgent())
			return false;

		IEntity leader = group.GetLeaderEntity();
		if (!leader)
			return false;

		outLeaderPos = leader.GetOrigin();

		bool urban;
		float maxDist = DCO_Util.CombatCohesionDist(group, m_MyEntity, leader, urban);
		outRegroupRadius = maxDist * 0.5;

		if (vector.Distance(outLeaderPos, m_MyEntity.GetOrigin()) <= maxDist)
			return false;
		if (urban)
			DCO_Perf.Count("combat_cohesion_regroup:urban");
		else
			DCO_Perf.Count("combat_cohesion_regroup:open");
		return true;
	}

	protected void PushRequestCriticalBound()
	{
	    SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

	    rq.m_eReason    = SCR_EAICombatMoveReason.STANDARD;
	    rq.m_vTargetPos = ResolveRequestTargetPos();
	    rq.m_vMovePos   = rq.m_vTargetPos;

	    rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
	    rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;

	    rq.m_eStanceMoving = ECharacterStance.STAND;
	    rq.m_eStanceEnd    = ECharacterStance.CROUCH;
	    rq.m_eMovementType = EMovementType.SPRINT;

	    rq.m_bAimAtTarget    = false;
	    rq.m_bAimAtTargetEnd = true;

	    rq.m_bUseCoverSearchDirectivity = true;
	    rq.m_bCheckCoverVisibility      = true;
	    rq.m_bFailIfNoCover             = false;

	    bool buildingExhausted = m_State.GetOldRequest()
	        && m_State.GetOldRequest().m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND;

	    float searchDistMax;
	    if (buildingExhausted)
	    {
	        rq.m_eType         = SCR_EAICombatMoveRequestType.MOVE;
	        rq.m_bTryFindCover = true;
	        searchDistMax      = 25.0;
	    }
	    else
	    {
	        rq.m_eType         = SCR_EAICombatMoveRequestType.BUILDING;
	        rq.m_bTryFindCover = false;
	        searchDistMax      = 60.0;
	    }

	    rq.m_fCoverSearchDistMin = 0;
	    rq.m_fCoverSearchDistMax = searchDistMax;
	    rq.m_fMoveDuration_s     = searchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_SPRINT;

	    rq.GetOnMovementStarted().Insert(OnMovementStarted);
	    rq.GetOnCompleted().Insert(OnMovementCompleted);

	    m_fNextCriticalBid_ms = GetGame().GetWorld().GetWorldTime() + 3000.0;
	    m_State.ApplyNewRequest(rq);
	}

	protected override vector ResolveRequestTargetPos()
	{
		IEntity tgtEntity = m_Target.GetTargetEntity();
		if (tgtEntity && m_CombatComp.IsTargetVisible(m_Target))
		{
			ChimeraCharacter character = ChimeraCharacter.Cast(tgtEntity);
			if (character)
			{
				vector eyePos = character.EyePosition();
				return eyePos;
			}

			vector pos = tgtEntity.GetOrigin();
			pos = pos + Vector(0, 2.0, 0);
			return pos;
		}

		vector lastSeenPos = m_Target.GetLastSeenPosition();
		lastSeenPos = lastSeenPos + Vector(0, 1.8, 0);
		return lastSeenPos;
	}

	protected override bool ResolveFailMoveIfNoCover()
	{
		if (m_bCloseRangeCombat)
		{
			if (m_State && m_State.IsMovingToBuilding())
				return true;
			else
				return false;
		}
		else
		{
			if (IsFirstExecution())
				return true;
			else
				return m_State.m_bInCover;
		}
	}

	protected override float ResolveStoppedWaitTime(bool inCover, EAIThreatState threat, EWeaponType weaponType)
	{
		float holdMul = 1.0;
		if (inCover)
			holdMul = DCO_Stress.TA(m_Utility, "tm.cover_hold_mul", 2.0, -0.7, 0.8, 5.0);
		if (holdMul > 1.0)
			DCO_Perf.Count("tm:cover_hold_applied");
		float cqbAggr = DCO_Engage.CqbAggr(m_Utility, m_fTargetDist);
		if (cqbAggr > 1.0)
			DCO_Perf.Count("tm:cqb_aggr_wait");
		if (m_bCloseRangeCombat)
		{
			return Math.RandomFloat(1.0, 4.5) * holdMul / cqbAggr;
		}

		if (IsInvestigating())
		{
			float investigateWait = Math.RandomFloat(4.0, 8.0) * DCO_PersonalityCombatUtility.GetInvestigateEagernessScale(m_Utility);
			return Math.Max(investigateWait, 2.0) / cqbAggr;
		}

		float waitTime;

		if (inCover)
		{
			switch (threat)
			{
				case EAIThreatState.THREATENED:
					waitTime = Math.RandomFloat(60, 90);
					break;
				default:
					waitTime = Math.RandomFloat(30, 45);
			}
		}
		else
		{
			switch (threat)
			{
				case EAIThreatState.THREATENED:
					waitTime = Math.RandomFloat(2, 4);
					break;
				default:
					waitTime = Math.RandomFloat(6, 9);
					break;
			}
		}

		bool longWaitTime = false;
		switch (weaponType)
		{
			case EWeaponType.WT_MACHINEGUN:
			case EWeaponType.WT_ROCKETLAUNCHER:
			case EWeaponType.WT_GRENADELAUNCHER:
			case EWeaponType.WT_SNIPERRIFLE:
				longWaitTime = true;
		}

		if (m_bVeryLongRangeCombat)
			waitTime *= 1.5;
		else
			waitTime *= Math.RandomFloat(0.8, 1);

		if (longWaitTime)
			waitTime *= 2;

		float mult = Math.Map(moraleSystem.GetMoraleMeasure(), 0, 4.5, 1, 2.5);
		waitTime += mult;

		waitTime *= DCO_PersonalityCombatUtility.GetStoppedWaitTimeScale(m_Utility);

		waitTime *= DCO_PostureCombatUtility.GetStoppedWaitTimeScale(DCO_PostureCombatUtility.GetPosture(m_Utility));
		waitTime *= holdMul;

		return waitTime / cqbAggr;
	}

	override protected bool MoveFromTargetCondition()
	{
		if (!m_State.m_bInCover)
		{
			float postureDist = DCO_PostureCombatUtility.GetMoveFromTargetDist(DCO_PostureCombatUtility.GetPosture(m_Utility));
			if (m_fTargetDist < postureDist)
				return true;
		}

		return super.MoveFromTargetCondition();
	}


	protected SCR_AIGroupUtilityComponent DCO_GetGroupUtility()
	{
		AIAgent agent = m_Utility.GetAIAgent();
		if (!agent)
			return null;

		SCR_AIGroup group = SCR_AIGroup.Cast(agent.GetParentGroup());
		if (!group)
			return null;
		return group.GetGroupUtilityComponent();
	}

	protected bool DCO_InOvermatch()
	{
		if (m_Utility.m_AIInfo && m_Utility.m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE))
			return false;

		SCR_AIGroupUtilityComponent groupUtil = DCO_GetGroupUtility();
		return groupUtil && groupUtil.DCO_IsOvermatch();
	}

	protected bool DCO_OvermatchMoveCondition()
	{
		if (m_State.IsExecutingRequest())
			return false;

		if (m_Utility.m_ThreatSystem.GetSuppressionMeasure() > 0.85)
			return false;

		if (m_fTargetDist > 30.0)
		{
			SCR_AIGroupUtilityComponent groupUtil = DCO_GetGroupUtility();
			if (groupUtil && groupUtil.m_FireteamMgr)
			{
				bool bounding;
				float omPeriod = DCO_PostureCombatUtility.BoundPeriod_ms(DCO_GroupTactics.AGGRESIVE);
				bool myTurn = groupUtil.m_FireteamMgr.DCO_IsBoundingTurn(m_Utility.GetAIAgent(), bounding, omPeriod);
				if (bounding && !myTurn)
					return false;
			}
		}

		return m_State.m_fTimerStopped_s > 2.5 * DCO_PersonalityCombatUtility.GetStoppedWaitTimeScale(m_Utility);
	}

	protected bool DCO_OvermatchBoundPos(vector targetPos, out vector outPos)
	{
		vector myPos = m_MyEntity.GetOrigin();
		vector toTarget = targetPos - myPos;
		toTarget[1] = 0;
		float dist = toTarget.Length();
		if (dist < 6.0 + 1)
			return false;

		vector fwd = toTarget / dist;
		vector dir = fwd;
		float advance;
		if (dist <= 30.0)
		{
			advance = Math.Min(dist - 6.0, 15.0);
		}
		else
		{
			advance = Math.Min(dist - 15.0, Math.RandomFloat(15.0, 30.0));
			float side = 1.0;
			if (Math.RandomIntInclusive(0, 1) == 1)
				side = -1.0;
			float angle = Math.RandomFloat(15.0, 40.0) * Math.DEG2RAD;
			vector right = Vector(fwd[2], 0, -fwd[0]);
			dir = (fwd * Math.Cos(angle)) + (right * Math.Sin(angle) * side);
		}

		if (advance < 3)
			return false;

		vector candidate = myPos + dir * advance;
		candidate = DCO_Util.Surface(candidate);

		NavmeshWorldComponent navmesh = GetGame().GetAIWorld().GetNavmeshWorldComponent("Soldiers");
		if (navmesh && navmesh.IsTileLoaded(candidate) && navmesh.GetReachablePoint(candidate, 3.0, outPos))
			return true;

		outPos = candidate;
		return true;
	}

	protected bool m_bDCOOpenRuleMove;
	protected float m_fDCOOpenRuleNext_ms;
	protected bool m_bDCONoAdvance;

	protected void DCO_OpenProneCrawl(string src, string reason)
	{
		if (m_eStance != ECharacterStance.PRONE && !DCO_Nav.Busy(m_CharacterController, "open_prone"))
			DCO_Stance.Request(m_CharacterController, 3, "open_prone");
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
		rq.m_vTargetPos = ResolveRequestTargetPos();
		rq.m_vMovePos = rq.m_vTargetPos;
		if (Math.RandomIntInclusive(0, 1) == 0)
			rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
		else
			rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
		rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
		rq.m_eStanceMoving = ECharacterStance.PRONE;
		rq.m_eStanceEnd = ECharacterStance.PRONE;
		rq.m_eMovementType = EMovementType.WALK;
		rq.m_bAimAtTarget = false;
		rq.m_bAimAtTargetEnd = true;
		rq.m_bTryFindCover = true;
		rq.m_bUseCoverSearchDirectivity = false;
		rq.m_bCheckCoverVisibility = true;
		rq.m_bFailIfNoCover = false;
		rq.m_fCoverSearchDistMin = 1.0;
		rq.m_fCoverSearchDistMax = 3.0;
		rq.m_fMoveDuration_s = 4.0;
		m_State.ApplyNewRequest(rq);
		DCO_Perf.Count("tm:open_prone_crawl_" + reason);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_MyEntity), string.Format("open_rule unit=%1 action=prone_crawl reason=%2 src=%3 stance=%4 target_d=%5 s=%6",
			DCO_Log.IdUnit(m_MyEntity), reason, src, typename.EnumToString(ECharacterStance, m_eStance), Math.Round(m_fTargetDist), DCO_Stress.LevelName(DCO_Stress.Level(m_Utility))));
	}

	protected void DCO_RushToCover(vector cover, int tier, string reason)
	{
		float d = vector.DistanceXZ(cover, m_MyEntity.GetOrigin());
		bool crawl = tier >= 2 || reason == "no_support" || d <= DCO_Stress.T(m_Utility, "tm.rush_crawl_m", 6.0);
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
		rq.m_vTargetPos = ResolveRequestTargetPos();
		rq.m_vMovePos = cover;
		rq.m_eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
		rq.m_fCoverSearchSectorHalfAngleRad = -1.0;
		rq.m_bUseCoverSearchDirectivity = false;
		rq.m_bTryFindCover = true;
		rq.m_bCheckCoverVisibility = true;
		rq.m_bFailIfNoCover = false;
		rq.m_fCoverSearchDistMin = 0;
		rq.m_fCoverSearchDistMax = d + 3.0;
		if (crawl)
		{
			rq.m_eStanceMoving = ECharacterStance.PRONE;
			rq.m_eStanceEnd = ECharacterStance.PRONE;
			rq.m_eMovementType = EMovementType.WALK;
		}
		else
		{
			rq.m_eStanceMoving = ECharacterStance.CROUCH;
			rq.m_eStanceEnd = ECharacterStance.CROUCH;
			rq.m_eMovementType = EMovementType.SPRINT;
		}
		rq.m_bAimAtTarget = false;
		rq.m_bAimAtTargetEnd = true;
		rq.m_fMoveDuration_s = d / Math.Max(DCO_Stress.Speed(rq), 0.5) + 2.0;
		m_State.ApplyNewRequest(rq);
		string mode = "run";
		if (crawl)
			mode = "crawl";
		DCO_Perf.Count("tm:rush_cover_" + mode);
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_MyEntity), string.Format("open_rule unit=%1 action=rush_cover_direct mode=%2 dist=%3 reason=%4 tier=%5 stance=%6 target_d=%7",
			DCO_Log.IdUnit(m_MyEntity), mode, Math.Round(d), reason, DCO_Stress.TierName(tier), typename.EnumToString(ECharacterStance, m_eStance), Math.Round(m_fTargetDist)));
	}

	protected bool DCO_OpenRule()
	{
		if (m_State.IsExecutingRequest() || m_State.m_bInCover || m_bCloseRangeCombat || !m_Target)
			return false;
		bool v2 = DCO_Stress.T(m_Utility, "tm.open_rule_v2", 1.0) > 0;
		if (m_eThreatState < EAIThreatState.VIGILANT || (!v2 && m_eThreatState < EAIThreatState.ALERTED))
			return false;
		if (m_eWeaponType == EWeaponType.WT_MACHINEGUN || m_eWeaponType == EWeaponType.WT_SNIPERRIFLE)
			return false;
		DCO_GroupTactics posture = DCO_PostureCombatUtility.GetPosture(m_Utility);
		if (posture == DCO_GroupTactics.SNEAK || posture == DCO_GroupTactics.AMBUSH)
			return false;
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fDCOOpenRuleNext_ms)
			return false;
		int tier = DCO_Stress.Tier(m_Utility);
		if (tier >= 3 || (!v2 && tier >= 2))
			return false;
		float lim = DCO_Stress.T(m_Utility, "tm.open_still_s", 2.0);
		if (tier == 0)
			lim = DCO_Stress.T(m_Utility, "tm.open_kneel_s", 3.0);
		if (m_eStance == ECharacterStance.PRONE)
			lim *= 2.0;
		if (m_State.m_fTimerStopped_s < lim)
			return false;
		if (SCR_CoverManagerComponent.IsEntityInsideBuilding(m_MyEntity))
			return false;
		DCO_EngageInfo ei = DCO_Engage.Info(m_Utility);
		if (ei && ei.m_iZone == 0 && !v2)
		{
			if (m_eStance == ECharacterStance.PRONE)
			{
				m_fDCOOpenRuleNext_ms = now + 3000.0;
				DCO_Perf.Count("tm:open_prone_ok");
				return false;
			}
			vector oldCover;
			string oldRush = DCO_Engage.RushCheck(m_Utility, oldCover);
			if (oldRush != "ok")
			{
				m_fDCOOpenRuleNext_ms = now + 6000.0;
				DCO_OpenProneCrawl("open_rule", oldRush);
				return false;
			}
		}
		else if (ei && ei.m_iZone == 0)
		{
			vector rushCover;
			string rush = DCO_Engage.RushCheck(m_Utility, rushCover);
			if (rushCover != vector.Zero && vector.DistanceXZ(rushCover, m_MyEntity.GetOrigin()) >= 1.5)
			{
				m_fDCOOpenRuleNext_ms = now + 6000.0;
				DCO_RushToCover(rushCover, tier, rush);
				return false;
			}
			if (m_eStance == ECharacterStance.PRONE && tier == 0)
			{
				m_fDCOOpenRuleNext_ms = now + 3000.0;
				DCO_Perf.Count("tm:open_prone_ok");
				return false;
			}
			if (rush != "ok")
			{
				m_fDCOOpenRuleNext_ms = now + 8000.0;
				DCO_OpenProneCrawl("open_rule", rush);
				return false;
			}
		}
		if (tier >= 2)
			return false;
		m_fDCOOpenRuleNext_ms = now + 6000.0;
		m_bDCOOpenRuleMove = true;
		m_bDCONoAdvance = !DCO_Engage.AdvanceAllowed(m_Utility, "open_rule");
		DCO_Perf.Count("tm:open_rule");
		DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_MyEntity), string.Format("open_rule unit=%1 action=rush_cover still_s=%2 tier=%3 stance=%4 threat=%5 target_d=%6 zone=%7",
			DCO_Log.IdUnit(m_MyEntity), m_State.m_fTimerStopped_s.ToString(-1, 1), DCO_Stress.TierName(tier), typename.EnumToString(ECharacterStance, m_eStance), typename.EnumToString(EAIThreatState, m_eThreatState), Math.Round(m_fTargetDist), DCO_Engage.ZoneOf(m_Utility)));
		return true;
	}

	protected override bool MoveToNextPosCondition()
	{
		if (m_Utility && m_Utility.m_DCOConfig && m_Utility.m_DCOConfig.IsHoldPosition())
			return false;

		if (DCO_OpenRule())
			return true;

		if (DCO_InOvermatch())
			return DCO_OvermatchMoveCondition();

		m_bDCONoAdvance = !DCO_Engage.AdvanceAllowed(m_Utility, "move_next");
		DCO_CtxState ctx = DCO_Ctx.Get(m_Utility);
		if (ctx && !IsInvestigating())
		{
			int ctxMove = DCO_Ctx.P(ctx, DCO_ECtxParam.MOVE);
			bool ctxAtk = DCO_CtxAttacker();
			if (ctxAtk && (ctxMove == 0 || ctxMove == 5 || ctxMove == 8 || (ctxMove == 2 && ctx.m_iKind == DCO_ECtxKind.NONE)))
			{
				DCO_Perf.Count("ctx:atk_keep_advance");
				ctxMove = 3;
			}
			if ((ctxMove == 0 || ctxMove == 1) && m_State.m_bInCover)
			{
				DCO_Perf.Count("ctx:move_hold_cover");
				return false;
			}
			if (ctxMove == 2 || ctxMove == 5 || ctxMove == 8)
			{
				if (!m_bDCONoAdvance)
					DCO_Perf.Count("ctx:no_advance");
				m_bDCONoAdvance = true;
			}
			int ctxUnit = DCO_Ctx.P(ctx, DCO_ECtxParam.UNIT);
			if (m_State.m_bInCover && (ctxUnit == 1 || ctxUnit == 2) && !m_State.IsExecutingRequest())
			{
				float ctxPeriod = Math.Clamp((DCO_Ctx.P(ctx, DCO_ECtxParam.BMAX) / 3.5 + 3.0) * 1000.0, 4000.0, 12000.0);
				bool ctxTurn = true;
				bool ctxPaired;
				if (ctxUnit == 1)
					ctxTurn = DCO_Ctx.BuddyTurn(m_Utility.GetAIAgent(), ctxPeriod, ctxPaired);
				else
				{
					SCR_AIGroupUtilityComponent ctxGu = DCO_GetGroupUtility();
					if (ctxGu && ctxGu.m_FireteamMgr)
						ctxTurn = ctxGu.m_FireteamMgr.DCO_IsBoundingTurn(m_Utility.GetAIAgent(), ctxPaired, ctxPeriod);
				}
				if (!ctxTurn)
				{
					DCO_Perf.Count("ctx:bound_wait_" + ctxUnit);
					return false;
				}
				if (ctxPaired)
					DCO_Perf.Count("ctx:bound_turn_" + ctxUnit);
			}
		}
		if (m_bDCONoAdvance && m_State.m_bInCover)
		{
			DCO_Perf.Count("tm:adv_hold_cover");
			return false;
		}

		bool cqbPush = DCO_Engage.CqbPush(m_Utility);
		bool aggrPush = DCO_AggressivePush() || cqbPush;
		if (!aggrPush && IsCriticalCombatMoment() && !DCO_CanPushUnderFire())
			return false;

		float cqbAggr = DCO_Engage.CqbAggr(m_Utility, m_fTargetDist);
		float lockTime_s = 5.0 * DCO_PersonalityCombatUtility.GetRepositionLockScale(m_Utility)
			* DCO_PostureCombatUtility.GetRepositionLockScale(DCO_PostureCombatUtility.GetPosture(m_Utility)) / cqbAggr;

		float optimalDist = Math.Max(ResolveOptimalDistance(m_fWeaponMinDist) * DCO_PersonalityCombatUtility.GetCloseInDistanceScale(m_Utility), m_fWeaponMinDist);

		if (m_fTargetDist < optimalDist && DCO_CQC.IsTargetInMyBuilding(m_Utility))
			optimalDist = Math.Max(optimalDist * 0.5, m_fWeaponMinDist);
		if (aggrPush && m_fTargetDist >= 20.0 && !DCO_CQC.IsTargetInMyBuilding(m_Utility))
			optimalDist = 0;
		if (cqbPush && m_fTargetDist >= 6.0)
			optimalDist = 0;
		if (m_fTargetDist < optimalDist && m_Target.GetTimeSinceSeen() < lockTime_s)
			return false;

		if (m_State.IsExecutingRequest())
			return false;

		if (IsFirstExecution() && !m_State.m_bInCover)
			return true;

		DCO_GroupTactics posture = DCO_PostureCombatUtility.GetPosture(m_Utility);
		if (DCO_PostureCombatUtility.IsHoldPosture(posture))
		{
			DCO_Perf.Count("pos_hold_move");
			return false;
		}
		if (DCO_MoveBrain.On(m_Utility))
		{
			bool mbAttacker = posture != DCO_GroupTactics.DEFENSIVE && DCO_MbAttacker();
			bool mbTb = false;
			bool mbTurn = true;
			if (m_fTargetDist > 30.0 && !m_bCloseRangeCombat)
			{
				SCR_AIGroupUtilityComponent mbUtil = DCO_GetGroupUtility();
				if (mbUtil && mbUtil.m_FireteamMgr && DCO_TeamBound.On(SCR_AIGroup.Cast(m_Utility.GetAIAgent().GetParentGroup())))
				{
					bool mbBounding;
					mbTurn = mbUtil.m_FireteamMgr.DCO_IsBoundingTurn(m_Utility.GetAIAgent(), mbBounding, 8000.0);
					mbTb = mbBounding;
				}
			}
			int mb = DCO_MoveBrain.Decide(m_Utility, m_State, m_Target, mbAttacker, mbTb, mbTurn);
			if (mb >= 0)
				return mb == 1;
		}
		if (posture == DCO_GroupTactics.DEFENSIVE && m_State.m_bInCover)
			return false;

		bool holdLock = m_CombatComp.GetCombatMode() == EAIGroupCombatMode.HOLD_FIRE && !DCO_AssaultMoveAllowed();
		if (m_Utility.ShouldKeepFormation() || holdLock)
		{
			if (!IsFirstExecution())
				return false;
		}

		float boundPeriod = DCO_PostureCombatUtility.BoundPeriod_ms(posture);
		if (boundPeriod > 0 && m_fTargetDist > 30.0 && !m_bCloseRangeCombat)
		{
			SCR_AIGroupUtilityComponent ftUtil = DCO_GetGroupUtility();
			if (ftUtil && ftUtil.m_FireteamMgr)
			{
				bool bounding;
				bool myTurn = ftUtil.m_FireteamMgr.DCO_IsBoundingTurn(m_Utility.GetAIAgent(), bounding, boundPeriod);
				if (bounding && !myTurn)
				{
					DCO_Perf.Count("ft_cover");
					return false;
				}
			}
		}

		if (m_State.m_fTimerStopped_s < m_fDCOLastStopped_s)
			m_fDCOWaitRoll = Math.RandomFloat(0, 1);
		m_fDCOLastStopped_s = m_State.m_fTimerStopped_s;
		bool heavyWeapon = m_eWeaponType == EWeaponType.WT_MACHINEGUN || m_eWeaponType == EWeaponType.WT_ROCKETLAUNCHER;

		if (aggrPush)
		{
			float aggrWait = 3.0 + 3.0 * m_fDCOWaitRoll;
			if (heavyWeapon)
				aggrWait = 8.0 + 4.0 * m_fDCOWaitRoll;
			if (m_State.m_bInCover)
				aggrWait *= DCO_Stress.T(m_Utility, "tm.cover_hold_mul", 2.0);
			aggrWait /= cqbAggr;
			if (m_State.m_fTimerStopped_s > aggrWait)
			{
				DCO_Perf.Count("aggr_push");
				if (cqbAggr > 1.0)
					DCO_Perf.Count("tm:cqb_aggr_push");
				return true;
			}
			return false;
		}

		if (posture == DCO_GroupTactics.BALANCE && m_State.m_bInCover)
		{
			if (m_Target.GetTimeSinceEndangered() < 4.0 / cqbAggr)
				return false;
			float balWait = 10.0 + 10.0 * m_fDCOWaitRoll;
			if (heavyWeapon)
				balWait *= 1.5;
			balWait *= DCO_Stress.T(m_Utility, "tm.cover_hold_mul", 2.0);
			balWait /= cqbAggr;
			if (m_State.m_fTimerStopped_s > balWait)
			{
				DCO_Perf.Count("bal_bound");
				return true;
			}
			return false;
		}

		float openAreaMultiplier = 1;
		if (m_bInOpenAreaCached)
			openAreaMultiplier = DCO_Stress.TA(m_Utility, "tm.open_wait_mul", 2.0, -0.7, 0.8, 5.0);

		float stoppedWaitTime = ResolveStoppedWaitTime(m_State.m_bInCover, m_eThreatState, m_eWeaponType) * openAreaMultiplier;
		return m_State.m_fTimerStopped_s > stoppedWaitTime;
	}

	override protected void ResolveMoveRequestMovePosAndDir(vector targetPos, out vector outMovePos, out vector outAvoidStraightPathDir, out SCR_EAICombatMoveDirection outDirection, out float outCoverSearchSectorHalfAngleRad)
	{
		vector boundPos;
		if (DCO_InOvermatch() && DCO_OvermatchBoundPos(targetPos, boundPos))
		{
			outMovePos = boundPos;
			outDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
			outAvoidStraightPathDir = GetAvoidStraightPathDir();
			outCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
			return;
		}

		AIWaypoint wp = null;
		AIAgent agent = m_Utility.GetAIAgent();
		AIGroup group = agent.GetParentGroup();
		if (group)
			wp = group.GetCurrentWaypoint();

		vector movePos;
		SCR_EAICombatMoveDirection eDirection;
		float coverSearchSectorHalfAngleRad;
		vector avoidStraightPathDir;
		vector cohLeader;
		float cohRadius;

		if (wp && !SCR_EntityWaypoint.Cast(wp) && DCO_Util.OutsideCombatCohesion(agent, m_MyEntity, cohLeader, cohRadius))
		{
			movePos = DCO_Util.Surface(GetCohesionRand().GenerateRandomPointInRadius(0, Math.Max(cohRadius, 2.0), cohLeader, false));
			eDirection = SCR_EAICombatMoveDirection.FORWARD;
			coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
			avoidStraightPathDir = vector.Zero;
		}
		else if (!wp || SCR_EntityWaypoint.Cast(wp))
		{
			vector leaderPos;
			float regroupRadius;

			if (ShouldRegroupToLeader(leaderPos, regroupRadius))
			{
				vector mp = GetCohesionRand().GenerateRandomPointInRadius(0, Math.Max(regroupRadius, 2.0), leaderPos, false);
				mp = DCO_Util.Surface(mp);
				movePos = mp;
				eDirection = SCR_EAICombatMoveDirection.FORWARD;
				coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
				avoidStraightPathDir = vector.Zero;
			}
			else
			{
				movePos = targetPos;
				MoraleAndThreatPushMove(eDirection);
				DCO_PostureCombatUtility.ApplyMoveDirection(DCO_PostureCombatUtility.GetPosture(m_Utility), m_eThreatState, moraleSystem.GetState(), eDirection);
				avoidStraightPathDir = GetAvoidStraightPathDir();
				coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
			}
		}
		else
		{
			vector wpPos = wp.GetOrigin();
			float wpRadius = wp.GetCompletionRadius();
			bool tgtInWaypoint = vector.DistanceXZ(wpPos, targetPos) < wpRadius;
			float myDistToWp = vector.DistanceXZ(wpPos, m_MyEntity.GetOrigin());

			if (myDistToWp > wpRadius)
			{
				movePos = wpPos;
				eDirection = SCR_EAICombatMoveDirection.FORWARD;
				coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
				avoidStraightPathDir = vector.Zero;
			}
			else if (myDistToWp > 0.5 * wpRadius)
			{
				if (tgtInWaypoint)
				{
					movePos = targetPos;
					eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
					avoidStraightPathDir = GetAvoidStraightPathDir();
					coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
				}
				else
				{
					movePos = targetPos;
					eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
					avoidStraightPathDir = vector.Zero;
					coverSearchSectorHalfAngleRad = -1.0;
				}
			}
			else
			{
				movePos = targetPos;
				eDirection = SCR_EAICombatMoveDirection.FORWARD;
				avoidStraightPathDir = GetAvoidStraightPathDir();

				coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
			}
			if (myDistToWp <= wpRadius)
				DCO_PostureCombatUtility.ApplyAreaDirection(DCO_PostureCombatUtility.GetPosture(m_Utility), eDirection);
		}

		if (moraleSystem && moraleSystem.GetState() >= moraleState.SHAKEN && m_eThreatState >= EAIThreatState.ALERTED
			&& DCO_AssaultLock.Mode(group) == 0)
		{
			if (moraleSystem.GetState() == moraleState.SHAKEN)
			{
				int roll = Math.RandomIntInclusive(0, 2);
				if (roll == 0)
					eDirection = SCR_EAICombatMoveDirection.LEFT;
				else if (roll == 1)
					eDirection = SCR_EAICombatMoveDirection.RIGHT;
				else
					eDirection = SCR_EAICombatMoveDirection.BACKWARD;
			}
			else
				eDirection = SCR_EAICombatMoveDirection.BACKWARD;
			movePos = targetPos;
			avoidStraightPathDir = vector.Zero;
			coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
			DCO_Perf.Count("tm:morale_dir");
		}

		if (eDirection == SCR_EAICombatMoveDirection.BACKWARD && (!moraleSystem || moraleSystem.GetState() < moraleState.SHAKEN) && DCO_MoveBrain.Advancing(m_Utility))
		{
			eDirection = SCR_EAICombatMoveDirection.FORWARD;
			movePos = targetPos;
			coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
			DCO_Perf.Count("mb:dir_fix_forward");
		}

		if (m_bDCONoAdvance && eDirection != SCR_EAICombatMoveDirection.BACKWARD && eDirection != SCR_EAICombatMoveDirection.LEFT && eDirection != SCR_EAICombatMoveDirection.RIGHT)
		{
			vector myPos = m_MyEntity.GetOrigin();
			bool advancing = eDirection == SCR_EAICombatMoveDirection.FORWARD || vector.DistanceXZ(movePos, targetPos) < vector.DistanceXZ(myPos, targetPos) - 2.0;
			if (advancing)
			{
				if (Math.RandomIntInclusive(0, 1) == 0)
					eDirection = SCR_EAICombatMoveDirection.LEFT;
				else
					eDirection = SCR_EAICombatMoveDirection.RIGHT;
				movePos = targetPos;
				avoidStraightPathDir = vector.Zero;
				coverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
				DCO_Perf.Count("tm:adv_convert_lateral");
			}
		}

		outMovePos = movePos;
		outDirection = eDirection;
		outAvoidStraightPathDir = avoidStraightPathDir;
		outCoverSearchSectorHalfAngleRad = coverSearchSectorHalfAngleRad;
	}

	override protected void PushRequestMove()
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;

		rq.m_vTargetPos = ResolveRequestTargetPos();
		ResolveMoveRequestMovePosAndDir(rq.m_vTargetPos, rq.m_vMovePos, rq.m_vAvoidStraightPathDir, rq.m_eDirection, rq.m_fCoverSearchSectorHalfAngleRad);
		rq.m_bTryFindCover = true;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility = true;

		bool isExposed = DCO_ConcealmentUtility.IsPositionExposed(m_MyEntity.GetOrigin(), m_MyEntity);
		if (isExposed)
			rq.m_bUseCoverSearchDirectivity = false;

		float coverSearchDistMin = 5;
		float coverSearchDistMax = 30;
		if (m_bCloseRangeCombat)
		{
			switch (m_eThreatState)
			{
				case EAIThreatState.THREATENED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.PRONE;
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 10.0;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 10.0;
					rq.m_eMovementType = EMovementType.WALK;
					break;
				}
				default:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 15.0;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
			}

			rq.m_bAimAtTarget = DCO_MoraleCombatUtility.CanAimWhileMoving(
				DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection) &&
				IsAimingAndMovingAllowedForWeapon(m_eWeaponType),
				moraleSystem);
			rq.m_bAimAtTargetEnd = true;
		}
		else
		{
			switch (m_eThreatState)
			{
				case EAIThreatState.THREATENED:
				{
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 20.0;
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.PRONE;
					rq.m_eMovementType = EMovementType.SPRINT;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					coverSearchDistMin = 5.0;
					coverSearchDistMax = 10.0;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
				default:
				{
					coverSearchDistMin = 10.0;
					coverSearchDistMax = 30.0;
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
			}

			rq.m_bAimAtTarget = DCO_MoraleCombatUtility.CanAimWhileMoving(
				DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection) &&
				IsAimingAndMovingAllowedForWeapon(m_eWeaponType),
				moraleSystem);
			rq.m_bAimAtTargetEnd = true;
		}

		if (IsInvestigating())
		{
			rq.m_eStanceMoving = ECharacterStance.STAND;
			rq.m_eStanceEnd = ECharacterStance.CROUCH;
			rq.m_eMovementType = EMovementType.WALK;
			rq.m_bAimAtTarget = DCO_MoraleCombatUtility.CanAimWhileMoving(true, moraleSystem);
			rq.m_bAimAtTargetEnd = true;
		}

		DCO_PostureCombatUtility.ApplyMovePace(DCO_PostureCombatUtility.GetPosture(m_Utility), m_eThreatState, rq, !m_bCloseRangeCombat);
		DCO_CtxState ctx;
		if (!IsInvestigating() && !DCO_InOvermatch())
			ctx = DCO_Ctx.Get(m_Utility);
		if (ctx)
		{
			int ctxPace = DCO_Ctx.P(ctx, DCO_ECtxParam.PACE);
			if (ctxPace <= 0 && rq.m_eMovementType != EMovementType.WALK)
			{
				rq.m_eMovementType = EMovementType.WALK;
				DCO_Perf.Count("ctx:pace_walk");
			}
			else if (ctxPace == 1 && rq.m_eMovementType == EMovementType.SPRINT)
			{
				rq.m_eMovementType = EMovementType.RUN;
				DCO_Perf.Count("ctx:pace_run");
			}
		}

		if (m_State.GetOldRequest() && m_State.GetOldRequest().m_eFailReason == SCR_EAICombatMoveRequestFailReason.NO_BUILDING_FOUND)
			rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		else
		{
			rq.m_eType = SCR_EAICombatMoveRequestType.BUILDING;
			rq.m_bTryFindCover = false;
			if (m_Utility.m_CombatComponent && m_Utility.m_CombatComponent.GetCurrentTarget() && m_Utility.m_CombatComponent.GetCurrentTarget().GetTimeSinceSeen() > 2.0)
				DCO_BreachUtility.TryThrowBreachGrenade(m_Utility, rq.m_vTargetPos);
		}

		rq.m_bFailIfNoCover = ResolveFailMoveIfNoCover();

		if (DCO_InOvermatch())
		{
			if (m_fTargetDist > 40.0)
			{
				rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
				rq.m_bTryFindCover = true;
			}
			rq.m_bFailIfNoCover = false;
			rq.m_eStanceMoving = ECharacterStance.CROUCH;
			rq.m_eStanceEnd = ECharacterStance.CROUCH;
			rq.m_eMovementType = EMovementType.RUN;
			coverSearchDistMin = 0;
			coverSearchDistMax = 10;
		}

		if (!m_State.m_bInCover)
			coverSearchDistMin = 3;

		if (ctx)
		{
			int ctxMv = DCO_Ctx.P(ctx, DCO_ECtxParam.MOVE);
			if (DCO_CtxAttacker() && (ctxMv == 0 || ctxMv == 5 || ctxMv == 8 || (ctxMv == 2 && ctx.m_iKind == DCO_ECtxKind.NONE)))
				ctxMv = 3;
			if (ctxMv <= 1)
			{
				coverSearchDistMin = 0;
				coverSearchDistMax = Math.Max(DCO_Ctx.P(ctx, DCO_ECtxParam.CMAX), 2.0);
				rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
				rq.m_fCoverSearchSectorHalfAngleRad = -1.0;
				rq.m_bUseCoverSearchDirectivity = false;
			}
			else
			{
				coverSearchDistMin = DCO_Ctx.P(ctx, DCO_ECtxParam.BMIN);
				coverSearchDistMax = Math.Max(DCO_Ctx.P(ctx, DCO_ECtxParam.BMAX), coverSearchDistMin + 2.0);
				vector ctxSpot;
				string ctxSpotKind;
				if (ctxMv == 9 && !m_bDCONoAdvance && ctx.m_iTgt == DCO_EEnv.INDOOR)
				{
					if (DCO_Ctx.FindDeadAngle(m_MyEntity, rq.m_vTargetPos, coverSearchDistMax + 15.0, ctxSpot))
						ctxSpotKind = "dead_angle";
					else
						DCO_Perf.Count("ctx:dead_angle_miss");
				}
				if (ctxSpotKind.IsEmpty() && DCO_Ctx.P(ctx, DCO_ECtxParam.C2C) > 0 && !m_bDCONoAdvance && m_eThreatState >= EAIThreatState.VIGILANT)
				{
					if (DCO_Ctx.FindCorner(m_MyEntity, rq.m_vTargetPos, coverSearchDistMin, coverSearchDistMax + 8.0, ctxSpot))
						ctxSpotKind = "corner";
					else
						DCO_Perf.Count("ctx:c2c_miss");
				}
				if (ctxSpotKind.IsEmpty() && DCO_Ctx.P(ctx, DCO_ECtxParam.CONC) > 0 && !m_State.m_bInCover)
				{
					if (DCO_Ctx.FindConcealment(m_MyEntity, rq.m_vTargetPos, coverSearchDistMax, !m_bDCONoAdvance, ctxSpot))
						ctxSpotKind = "concealment";
					else
						DCO_Perf.Count("ctx:conc_miss");
				}
				if (!ctxSpotKind.IsEmpty())
				{
					float ctxSpotD = vector.DistanceXZ(ctxSpot, m_MyEntity.GetOrigin());
					rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
					rq.m_bTryFindCover = true;
					rq.m_bFailIfNoCover = false;
					rq.m_eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
					rq.m_vMovePos = ctxSpot;
					rq.m_bUseCoverSearchDirectivity = false;
					rq.m_fCoverSearchSectorHalfAngleRad = -1.0;
					coverSearchDistMin = 0;
					coverSearchDistMax = ctxSpotD + 3.0;
					rq.m_fMoveDuration_s = ctxSpotD / Math.Max(DCO_Stress.Speed(rq), 0.5) + 2.0;
					DCO_Perf.Count("ctx:spot_" + ctxSpotKind);
					if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
						DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_MyEntity), string.Format("ctxspot unit=%1 kind=%2 dist=%3 pos=%4 ctx=%5",
							DCO_Log.IdUnit(m_MyEntity), ctxSpotKind, Math.Round(ctxSpotD), DCO_Log.Pos(ctxSpot), DCO_Ctx.Tag(ctx)));
				}
			}
			if (DCO_Ctx.P(ctx, DCO_ECtxParam.SMOKE) > 0 && !m_State.m_bInCover && m_eThreatState >= EAIThreatState.ALERTED && ctxMv > 0)
			{
				DCO_Perf.Count("ctx:smoke_try");
				if (DCO_SmokeUtility.TryDeploySmokeForRetreat(m_Utility, rq.m_vTargetPos, 2.0))
					DCO_Perf.Count("ctx:smoke_thrown");
			}
			DCO_Perf.Count("ctx:move");
			if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_MyEntity), string.Format("ctxmove unit=%1 move=%2 cover_min=%3 cover_max=%4 pace=%5 dir=%6 ctx=%7 dist=%8",
					DCO_Log.IdUnit(m_MyEntity), ctxMv, Math.Round(coverSearchDistMin), Math.Round(coverSearchDistMax), typename.EnumToString(EMovementType, rq.m_eMovementType),
					typename.EnumToString(SCR_EAICombatMoveDirection, rq.m_eDirection), DCO_Ctx.Tag(ctx), Math.Round(m_fTargetDist)));
		}

		if (m_State && m_State.IsMovingToBuilding())
		{
			coverSearchDistMin = 0;
			coverSearchDistMax = 5;
		}
		else
		{
			string src = "move";
			if (m_bDCOOpenRuleMove)
				src = "open_rule";
			int tier = DCO_Stress.Shape(m_Utility, rq, m_bCloseRangeCombat, m_State.m_bInCover, !DCO_InOvermatch(), coverSearchDistMin, coverSearchDistMax, src);
			coverSearchDistMax *= DCO_MoraleCombatUtility.GetPersonalityCoverSearchScale(m_Utility);
			if (isExposed)
				coverSearchDistMax *= 1.5;
			if (m_bDCOOpenRuleMove)
			{
				rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
				rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
				rq.m_bTryFindCover = true;
				rq.m_bFailIfNoCover = false;
				if (tier >= 1)
				{
					rq.m_eDirection = SCR_EAICombatMoveDirection.ANYWHERE;
					rq.m_fCoverSearchSectorHalfAngleRad = -1.0;
					rq.m_bUseCoverSearchDirectivity = false;
				}
			}
			else if (rq.m_eType == SCR_EAICombatMoveRequestType.MOVE && !m_bCloseRangeCombat && tier <= 1 && m_eThreatState >= EAIThreatState.VIGILANT && !DCO_InOvermatch() && !IsInvestigating() && !DCO_Engage.CqbPush(m_Utility) && DCO_Stress.T(m_Utility, "tm.cover_first", 0.0) > 0)
			{
				vector moveDir = SCR_AICombatMoveUtils.CalculateMoveDirection(rq.m_eDirection, m_MyEntity.GetOrigin(), rq.m_vMovePos);
				vector cfPos;
				if (DCO_Stress.FindCover(m_MyEntity, rq.m_vTargetPos, coverSearchDistMax, cfPos, moveDir) && vector.DistanceXZ(cfPos, m_MyEntity.GetOrigin()) >= 2.0)
				{
					float cfd = vector.DistanceXZ(cfPos, m_MyEntity.GetOrigin());
					rq.m_eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
					rq.m_vMovePos = cfPos;
					rq.m_bUseCoverSearchDirectivity = false;
					rq.m_fCoverSearchSectorHalfAngleRad = -1.0;
					coverSearchDistMin = 0;
					coverSearchDistMax = cfd + 3.0;
					rq.m_fMoveDuration_s = cfd / Math.Max(DCO_Stress.Speed(rq), 0.5) + 1.5;
					DCO_Perf.Count("tm:cover_first_hit");
				}
				else
					DCO_Perf.Count("tm:cover_first_miss");
			}
		}
		if (rq.m_eType == SCR_EAICombatMoveRequestType.MOVE && DCO_Stress.T(m_Utility, "cqb.kind_beh", 1.0) > 0)
			DCO_CqbKindMove(rq, coverSearchDistMin, coverSearchDistMax);
		vector sfmFlank;
		if (rq.m_eType == SCR_EAICombatMoveRequestType.MOVE && rq.m_eDirection != SCR_EAICombatMoveDirection.CUSTOM_POS && rq.m_eDirection != SCR_EAICombatMoveDirection.BACKWARD && !(m_State && m_State.IsMovingToBuilding()) && DCO_TeamBound.FlankFor(m_Utility, sfmFlank))
		{
			vector sfmTo = sfmFlank - m_MyEntity.GetOrigin();
			sfmTo[1] = 0;
			float sfmLeft = sfmTo.Length();
			if (sfmLeft > 6.0)
			{
				sfmTo.Normalize();
				vector sfmSpot = DCO_Util.Surface(m_MyEntity.GetOrigin() + sfmTo * Math.Min(sfmLeft, Math.Clamp(coverSearchDistMax, 8.0, 25.0)));
				vector sfmField;
				int sfmProt;
				if (DCO_MoveBrain.FieldOn(m_Utility) && DCO_MoveBrain.FieldSpot(m_Utility, sfmSpot, rq.m_vTargetPos, sfmField, sfmProt))
					sfmSpot = sfmField;
				float sfmD = vector.DistanceXZ(sfmSpot, m_MyEntity.GetOrigin());
				rq.m_eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
				rq.m_vMovePos = sfmSpot;
				rq.m_bTryFindCover = true;
				rq.m_bFailIfNoCover = false;
				rq.m_bUseCoverSearchDirectivity = false;
				rq.m_fCoverSearchSectorHalfAngleRad = -1.0;
				coverSearchDistMin = 0;
				coverSearchDistMax = sfmD + 3.0;
				rq.m_fMoveDuration_s = sfmD / Math.Max(DCO_Stress.Speed(rq), 0.5) + 1.5;
				DCO_Perf.Count("sfm:flank_step");
				if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
					DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_MyEntity), string.Format("sfm_step unit=%1 step=%2 left=%3 prot=%4", DCO_Log.IdUnit(m_MyEntity), Math.Round(sfmD), Math.Round(sfmLeft), sfmProt));
			}
		}
		if (rq.m_eType == SCR_EAICombatMoveRequestType.MOVE && rq.m_eDirection != SCR_EAICombatMoveDirection.CUSTOM_POS && !(m_State && m_State.IsMovingToBuilding()) && m_eThreatState >= EAIThreatState.VIGILANT && (DCO_MoveBrain.FieldOn(m_Utility) || DCO_MoveBrain.V2(m_Utility)))
		{
			vector fsDir = SCR_AICombatMoveUtils.CalculateMoveDirection(rq.m_eDirection, m_MyEntity.GetOrigin(), rq.m_vMovePos);
			fsDir[1] = 0;
			if (fsDir.Length() > 0.1)
			{
				fsDir.Normalize();
				vector fsCenter = m_MyEntity.GetOrigin() + fsDir * Math.Clamp(coverSearchDistMax * 0.7, 3.0, 15.0);
				vector fsSpot;
				int fsProt;
				bool fsOk;
				bool fsCrawl;
				int fsT = DCO_Perf.Begin();
				if (DCO_MoveBrain.V2(m_Utility))
				{
					fsCrawl = DCO_MoveBrain.Crawl(m_Utility);
					if (!fsCrawl && rq.m_eDirection == SCR_EAICombatMoveDirection.FORWARD)
						fsOk = DCO_MoveBrain.TakeSpot(m_Utility, fsSpot, fsProt);
					if (fsOk)
						DCO_Perf.Count("mb:v2_use_cached");
					else
					{
						float fsRush = DCO_MoveBrain.RushMax(m_Utility);
						vector fsFrom = fsCenter;
						if (fsCrawl)
						{
							fsRush = 6.0;
							fsFrom = m_MyEntity.GetOrigin();
						}
						float fsGain;
						fsOk = DCO_MoveBrain.FieldSpotV2(m_Utility, fsFrom, rq.m_vTargetPos, DCO_MbAttacker(), fsRush, fsSpot, fsProt, fsGain);
					}
				}
				else
					fsOk = DCO_MoveBrain.FieldSpot(m_Utility, fsCenter, rq.m_vTargetPos, fsSpot, fsProt);
				DCO_Perf.End("mb_field", fsT);
				if (fsOk)
				{
					float fsD = vector.DistanceXZ(fsSpot, m_MyEntity.GetOrigin());
					rq.m_eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
					rq.m_vMovePos = fsSpot;
					rq.m_bTryFindCover = true;
					rq.m_bFailIfNoCover = false;
					rq.m_bUseCoverSearchDirectivity = false;
					rq.m_fCoverSearchSectorHalfAngleRad = -1.0;
					coverSearchDistMin = 0;
					coverSearchDistMax = fsD + 3.0;
					if (fsProt == 1)
						rq.m_eStanceEnd = ECharacterStance.PRONE;
					else if (rq.m_eStanceEnd == ECharacterStance.STAND)
						rq.m_eStanceEnd = ECharacterStance.CROUCH;
					if (fsCrawl)
					{
						rq.m_eStanceMoving = ECharacterStance.PRONE;
						rq.m_eStanceEnd = ECharacterStance.PRONE;
						DCO_Perf.Count("mb:v2_crawl_move");
					}
					rq.m_fMoveDuration_s = fsD / Math.Max(DCO_Stress.Speed(rq), 0.5) + 1.5;
					DCO_Perf.Count("mb:field_hit_" + DCO_CoverId.ProtName(fsProt));
					if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
						DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_MyEntity), string.Format("mb_field unit=%1 prot=%2 dist=%3 pos=%4", DCO_Log.IdUnit(m_MyEntity), DCO_CoverId.ProtName(fsProt), Math.Round(fsD), DCO_Log.Pos(fsSpot)));
				}
				else
					DCO_Perf.Count("mb:field_miss");
			}
		}
		m_bDCOOpenRuleMove = false;

		rq.m_fCoverSearchDistMin = coverSearchDistMin;
		rq.m_fCoverSearchDistMax = coverSearchDistMax;
		if (rq.m_fMoveDuration_s <= 0)
			rq.m_fMoveDuration_s = coverSearchDistMax / Math.Max(DCO_Stress.Speed(rq), 0.5) + 1.0;

		DCO_AssaultFire(rq);

		rq.GetOnMovementStarted().Insert(OnMovementStarted);
		rq.GetOnCompleted().Insert(OnMovementCompleted);

		m_State.ApplyNewRequest(rq);
	}

	protected bool DCO_AssaultMoveAllowed()
	{
		SCR_AIGroupUtilityComponent gu = DCO_GetGroupUtility();
		if (!gu)
			return false;
		if (gu.DCO_IsOvermatch() || gu.DCO_IsClearingBuilding())
			return true;
		DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(gu.GetOwner().FindComponent(DCO_GroupUtilityComponent));
		if (!g)
			return false;
		if (g.m_eSOPAction == DCO_ESOPAction.OBSERVE || g.m_eSOPAction == DCO_ESOPAction.BYPASS || g.m_eSOPAction == DCO_ESOPAction.AMBUSH)
			return false;
		DCO_EGroupTask t = g.GetTask();
		if (t == DCO_EGroupTask.ATTACK || t == DCO_EGroupTask.FLANK || t == DCO_EGroupTask.REINFORCE)
		{
			DCO_Perf.Count("hold_fire_assault_move");
			return true;
		}
		return false;
	}

	protected IEntity m_DCODoorHit;

	protected bool DCO_OnDoor(IEntity e)
	{
		if (e && e.FindComponent(BaseDoorComponent))
		{
			m_DCODoorHit = e;
			return false;
		}
		return true;
	}

	protected void DCO_CqbKindMove(SCR_AICombatMoveRequest_Move rq, inout float coverSearchDistMin, inout float coverSearchDistMax)
	{
		int kind = DCO_Engage.CqbKind(m_Utility, m_fTargetDist);
		if (kind == 0)
			return;
		bool pinned = DCO_Stress.Level(m_Utility) >= 2;
		bool picked = rq.m_eDirection == SCR_EAICombatMoveDirection.CUSTOM_POS;
		string reason = "bound";
		if (picked)
			reason = "picked";
		int door;
		if (kind == 1)
		{
			if (!picked)
			{
				coverSearchDistMax = Math.Min(coverSearchDistMax, DCO_Stress.T(m_Utility, "cqb.in_bound_m", 6.0));
				coverSearchDistMin = Math.Max(coverSearchDistMin, Math.Min(DCO_Stress.T(m_Utility, "cqb.in_min_m", 2.0), coverSearchDistMax - 0.5));
			}
			if (pinned)
			{
				if (rq.m_eMovementType == EMovementType.SPRINT)
					rq.m_eMovementType = EMovementType.RUN;
			}
			else
			{
				rq.m_eMovementType = EMovementType.WALK;
				if (rq.m_eStanceMoving == ECharacterStance.PRONE)
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
				if (rq.m_eStanceEnd == ECharacterStance.PRONE)
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
			}
			DCO_Perf.Count("cqb:in_move");
			if (picked && DCO_Stress.T(m_Utility, "cqb.door_avoid", 0.0) > 0)
			{
				vector doorDir = rq.m_vMovePos - m_MyEntity.GetOrigin();
				doorDir[1] = 0;
				m_DCODoorHit = null;
				DCO_Perf.Count("q:DCO_CqbDoor");
				GetGame().GetWorld().QueryEntitiesBySphere(rq.m_vMovePos, 1.6, DCO_OnDoor);
				if (m_DCODoorHit && doorDir.Length() > 0.3)
				{
					door = 1;
					rq.m_vMovePos = rq.m_vMovePos + doorDir.Normalized() * DCO_Stress.T(m_Utility, "cqb.door_push_m", 1.8);
					reason = "door_shift";
					DCO_Perf.Count("cqb:door_shift");
				}
				m_DCODoorHit = null;
				if (doorDir.Length() > 3.0)
				{
					DCO_Perf.Count("q:DCO_CqbDoor");
					GetGame().GetWorld().QueryEntitiesBySphere((m_MyEntity.GetOrigin() + rq.m_vMovePos) * 0.5, 1.6, DCO_OnDoor);
					if (m_DCODoorHit)
					{
						rq.m_eMovementType = EMovementType.RUN;
						reason = "danger_cross";
						DCO_Perf.Count("cqb:danger_cross");
					}
					m_DCODoorHit = null;
				}
			}
		}
		else
		{
			if (!picked)
			{
				coverSearchDistMax = Math.Min(coverSearchDistMax, DCO_Stress.T(m_Utility, "cqb.out_cover_m", 12.0));
				coverSearchDistMin = Math.Max(coverSearchDistMin, Math.Min(DCO_Stress.T(m_Utility, "cqb.out_min_m", 5.0), coverSearchDistMax - 1.0));
			}
			bool forward = rq.m_eDirection == SCR_EAICombatMoveDirection.FORWARD;
			if (forward && !pinned && !DCO_MoveBrain.MateFired(m_Utility, 3000.0))
			{
				if (Math.RandomInt(0, 2) == 0)
					rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
				else
					rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
				coverSearchDistMax = Math.Min(coverSearchDistMax, DCO_Stress.T(m_Utility, "cqb.out_lateral_m", 5.0));
				coverSearchDistMin = Math.Min(2.0, coverSearchDistMax - 0.5);
				reason = "lateral";
				DCO_Perf.Count("cqb:out_lateral");
			}
			else
			{
				if (!pinned && !picked)
				{
					rq.m_eMovementType = EMovementType.SPRINT;
					rq.m_bTryFindCover = true;
					if (DCO_Stress.T(m_Utility, "cqb.out_need_cover", 1.0) > 0)
						rq.m_bFailIfNoCover = true;
					rq.m_fMoveDuration_s = DCO_Stress.T(m_Utility, "cqb.out_rush_s", 3.0);
					reason = "rush";
				}
				DCO_Perf.Count("cqb:out_move");
			}
		}
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_MyEntity), string.Format("cqb_kind_move unit=%1 kind=%2 reason=%3 dir=%4 cover=%5-%6 pinned=%7 dist=%8 door=%9",
				DCO_Log.IdUnit(m_MyEntity), DCO_Engage.CqbKindName(kind), reason, typename.EnumToString(SCR_EAICombatMoveDirection, rq.m_eDirection), Math.Round(coverSearchDistMin), Math.Round(coverSearchDistMax), DCO_Util.B01(pinned), Math.Round(m_fTargetDist), door));
	}

	protected bool DCO_MbAttacker()
	{
		if (DCO_CtxAttacker())
			return true;
		SCR_AIGroupUtilityComponent gu = DCO_GetGroupUtility();
		if (!gu)
			return false;
		if (gu.DCO_IsOvermatch() || gu.DCO_IsClearingBuilding())
			return true;
		DCO_GroupUtilityComponent g = DCO_GroupUtilityComponent.Cast(gu.GetOwner().FindComponent(DCO_GroupUtilityComponent));
		if (!g || g.m_eSOPAction == DCO_ESOPAction.OBSERVE || g.m_eSOPAction == DCO_ESOPAction.BYPASS || g.m_eSOPAction == DCO_ESOPAction.AMBUSH)
			return false;
		DCO_EGroupTask t = g.GetTask();
		return t == DCO_EGroupTask.ATTACK || t == DCO_EGroupTask.FLANK || t == DCO_EGroupTask.REINFORCE;
	}

	protected bool DCO_CtxAttacker()
	{
		if (DCO_AggressivePush())
			return true;
		AIAgent agent = m_Utility.GetAIAgent();
		if (!agent || !agent.GetParentGroup())
			return false;
		return SCR_SearchAndDestroyWaypoint.Cast(agent.GetParentGroup().GetCurrentWaypoint()) != null;
	}

	protected bool DCO_AggressivePush()
	{
		if (DCO_PostureCombatUtility.GetPosture(m_Utility) != DCO_GroupTactics.AGGRESIVE)
			return false;
		return !moraleSystem || moraleSystem.GetState() != moraleState.BREAK;
	}

	protected void DCO_CEAdvance()
	{
		if (m_State.IsExecutingRequest())
			return;
		AIGroup grp = m_Utility.GetAIAgent().GetParentGroup();
		if (!grp || !grp.GetCurrentWaypoint())
			return;
		vector me = m_MyEntity.GetOrigin();
		vector dest = grp.GetCurrentWaypoint().GetOrigin();
		vector dir = dest - me;
		dir[1] = 0;
		float len = dir.Length();
		if (len < 10.0)
			return;
		dir = dir * (1.0 / len);

		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
		rq.m_eDirection = SCR_EAICombatMoveDirection.CUSTOM_POS;
		rq.m_vMovePos = DCO_Util.Surface(me + dir * Math.Min(len, 30.0));
		if (m_Target)
			rq.m_vTargetPos = m_Target.GetLastSeenPosition();
		else
			rq.m_vTargetPos = dest;
		rq.m_eMovementType = EMovementType.RUN;
		rq.m_eStanceMoving = ECharacterStance.STAND;
		rq.m_eStanceEnd = ECharacterStance.STAND;
		rq.m_bTryFindCover = false;
		rq.m_bFailIfNoCover = false;
		rq.m_bAimAtTarget = true;
		rq.m_bAimAtTargetEnd = true;
		rq.m_fMoveDuration_s = 6.0;
		m_State.ApplyNewRequest(rq);
	}

	protected void DCO_AssaultFire(SCR_AICombatMoveRequest_Move rq)
	{
		if (!DCO_GlobalAIComponent.BehaviorOnFor(DCO_GlobalAIComponent.DCO_OwnerOf(m_CombatComp), DCO_EBehavior.ASSAULT_FIRE) || !m_Target || m_fTargetDist >= 30.0 || !m_CombatComp.IsTargetVisible(m_Target))
			return;
		if (rq.m_eDirection == SCR_EAICombatMoveDirection.BACKWARD || rq.m_eReason == SCR_EAICombatMoveReason.MOVE_FROM_TARGET)
			return;
		vector tp = m_Target.GetLastSeenPosition();
		if (rq.m_eDirection == SCR_EAICombatMoveDirection.CUSTOM_POS && vector.DistanceXZ(rq.m_vMovePos, tp) > vector.DistanceXZ(m_MyEntity.GetOrigin(), tp) - 1.0)
			return;
		if (m_Utility.m_ThreatSystem && m_Utility.m_ThreatSystem.GetSuppressionMeasure() > 0.6)
			return;
		if (rq.m_eMovementType == EMovementType.SPRINT)
			rq.m_eMovementType = EMovementType.RUN;
		rq.m_bAimAtTarget = DCO_MoraleCombatUtility.CanAimWhileMoving(
			DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection) &&
			IsAimingAndMovingAllowedForWeapon(m_eWeaponType),
			moraleSystem);
		if (rq.m_bAimAtTarget)
			DCO_Perf.Count("assault_fire_move");
	}

	void MoraleAndThreatPushMove(out SCR_EAICombatMoveDirection moveDir)
	{
		switch (m_eThreatState)
		{
			case EAIThreatState.THREATENED:
			{
				switch (moraleSystem.GetState())
				{
					case moraleState.BREAK:
					{
						moveDir = SCR_EAICombatMoveDirection.BACKWARD;
						break;
					}
					case moraleState.PANIC:
					{
						if (Math.RandomIntInclusive(0,1) == 1)
							moveDir = SCR_EAICombatMoveDirection.LEFT;
						else
							moveDir = SCR_EAICombatMoveDirection.RIGHT;
						break;
					}
					case moraleState.SHAKEN:
					{
						if (Math.RandomIntInclusive(0,1) == 1)
							moveDir = SCR_EAICombatMoveDirection.BACKWARD;
						else
							moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;
					}
					case moraleState.CALM:
					{
						if (Math.RandomIntInclusive(0,1) == 1)
							moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						else
							moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
						break;
					}
					case moraleState.TENSE:
					{
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;
					}
					default:
					{
						moveDir = SCR_EAICombatMoveDirection.BACKWARD;
						break;
					}
				}
				break;
			}
			case EAIThreatState.ALERTED:
			{
				switch (moraleSystem.GetState())
				{
					case moraleState.BREAK:
					{
						if (Math.RandomIntInclusive(0,1) == 1)
							moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
						else
							moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;
					}
					case moraleState.PANIC:
					{
						if (Math.RandomIntInclusive(0,1) == 1)
							moveDir = SCR_EAICombatMoveDirection.LEFT;
						else
							moveDir = SCR_EAICombatMoveDirection.RIGHT;
						break;
					}
					case moraleState.SHAKEN:
					{
						moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
						break;
					}
					case moraleState.CALM:
					{
						moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
						break;
					}
					case moraleState.TENSE:
					{
						moveDir = SCR_EAICombatMoveDirection.FORWARD;
						break;
					}
					default:
					{
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;
					}
				}
				break;
			}
			case EAIThreatState.VIGILANT:
			{
				switch (moraleSystem.GetState())
				{
					case moraleState.BREAK:
					{
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;
					}
					case moraleState.PANIC:
					{
						moveDir = SCR_EAICombatMoveDirection.FORWARD;
						break;
					}
					case moraleState.SHAKEN:
					{
						moveDir = SCR_EAICombatMoveDirection.FORWARD;
						break;
					}
					case moraleState.CALM:
					{
						moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
						break;
					}
					case moraleState.TENSE:
					{
						moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
						break;
					}
					default:
					{
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;
					}
				}
				break;
			}
			case EAIThreatState.SAFE:
			{
				switch (moraleSystem.GetState())
				{
					case moraleState.BREAK:
					{
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;
					}
					case moraleState.PANIC:
					{
						moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
						break;
					}
					case moraleState.SHAKEN:
					{
						moveDir = SCR_EAICombatMoveDirection.CUSTOM_POS;
						break;
					}
					case moraleState.CALM:
					{
						moveDir = SCR_EAICombatMoveDirection.FORWARD;
						break;
					}
					case moraleState.TENSE:
					{
						moveDir = SCR_EAICombatMoveDirection.FORWARD;
						break;
					}
					default:
					{
						moveDir = SCR_EAICombatMoveDirection.ANYWHERE;
						break;
					}
				}
				break;
			}
			default:
			{
				break;
			}
		}
	}

	override protected void SuppressedInCoverLogic()
	{
		float waitTime_s;
		SCR_AICombatMoveRequestBase rq = m_State.GetRequest();
		if (SCR_AICombatMoveRequest_ChangeStanceInCover.Cast(rq) && rq.m_eReason == SCR_EAICombatMoveReason.SUPPRESSED_IN_COVER)
			waitTime_s = rq.m_f_UserTimer_s;
		else
		{
			if (m_State.m_bExposedInCover)
				waitTime_s = 1.5;
			else
				waitTime_s = 5.0;
		}

		if (m_State.m_bExposedInCover && m_State.m_fTimerRequest_s > waitTime_s)
		{
			float newWaitTime = Math.RandomFloat(5, 9.0);
			PushRequestChangeStanceInCover(false, SCR_EAICombatMoveReason.SUPPRESSED_IN_COVER, newWaitTime);
		}
		else if (!m_State.m_bExposedInCover && m_State.m_fTimerRequest_s > waitTime_s)
		{
			if (m_Utility && m_Utility.m_CombatComponent && m_Utility.m_CombatComponent.DCO_GetLean() != 0)
				return;
			if (m_Utility && m_Utility.m_CombatComponent && m_Utility.m_CombatComponent.DCO_TryPeekShoot())
				return;
			float newWaitTime = Math.RandomFloat(1.5, 2.5);
			PushRequestChangeStanceInCover(true, SCR_EAICombatMoveReason.SUPPRESSED_IN_COVER, newWaitTime);
		}
	}

	protected override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (GetGame().GetWorld().GetWorldTime() < m_fNextUpdate_ms)
			return ENodeResult.RUNNING;
		int perfT = DCO_Perf.Begin();
		ENodeResult r = DCO_AttackMoveSim(owner, dt);
		DCO_Perf.End("bt_cm_attack", perfT);
		return r;
	}

	protected ENodeResult DCO_AttackMoveSim(AIAgent owner, float dt)
	{
		float currentTime_ms = GetGame().GetWorld().GetWorldTime();
		if (currentTime_ms < m_fNextUpdate_ms)
			return ENodeResult.RUNNING;
		m_fNextUpdate_ms = currentTime_ms + m_fUpdateInterval_ms;

		if (!OnUpdate(owner, dt))
			return ENodeResult.FAIL;

		if (!m_State || !m_MyEntity || !m_Utility || !m_CombatComp || !m_CharacterController)
			return ENodeResult.FAIL;

		if (m_Utility.m_DCOConfig && m_Utility.m_DCOConfig.IsHoldPosition())
			return ENodeResult.RUNNING;

		if (m_CombatComp.DCO_CEHold())
		{
			DCO_CEAdvance();
			return ENodeResult.RUNNING;
		}

		SCR_AIBehaviorBase executedBehavior = SCR_AIBehaviorBase.Cast(m_Utility.GetExecutedAction());
		if (executedBehavior && !executedBehavior.m_bUseCombatMove)
			return ENodeResult.RUNNING;

		m_fTargetDist = GetTargetDistance();
		m_bCloseRangeCombat = m_fTargetDist < SCR_AICombatMoveUtils.CLOSE_RANGE_COMBAT_DIST;
		m_bVeryLongRangeCombat = m_fTargetDist > SCR_AICombatMoveUtils.VERY_LONG_RANGE_COMBAT_DIST;
		m_eThreatState = m_Utility.m_ThreatSystem.GetState();
		m_eStance = m_CharacterController.GetStance();
		m_fWeaponMinDist = m_CombatComp.GetSelectedWeaponMinDist();
		m_eWeaponType = m_CombatComp.GetSelectedWeaponType();

		m_bInOpenAreaCached = IsInOpenArea(m_MyEntity);
		UpdateCriticalTimer(currentTime_ms);

		if (SuppressedInCoverCondition())
		{
			SuppressedInCoverLogic();
		}
		else if (MoveFromTargetCondition())
		{
			if (MoveFromTargetNewRequestCondition())
				PushRequestMoveFromTarget();
		}
		else if (CurrentCoverUselessCondition())
		{
			PushRequestLeaveUselessCover();
		}
		else if (m_CharacterController.IsReloading())
		{
			if (m_State.m_bInCover)
			{
				if (m_State.m_bExposedInCover)
					m_State.ApplyRequestChangeStanceInCover(false);
			} else if (!DCO_CloseDuck(currentTime_ms))
			{
				if (m_CharacterController.GetStance() ==  ECharacterStance.STAND && !DCO_Nav.Busy(m_CharacterController, "attack_stance"))
					DCO_Stance.Request(m_CharacterController, 2, "move_attack");
			}
		}
		else if (DCO_PeekShouldHide(currentTime_ms))
		{
			DCO_PeekHide(currentTime_ms);
		}
		else if (m_State.m_bInCover && !m_State.m_bExposedInCover)
		{
			DCO_PeekExpose(currentTime_ms);
		}
		else if (FFAvoidanceCondition())
		{
			if (FFAvoidanceNewRequestCondition())
				PushRequestFFAvoidance();
		}
		else if (MoveToNextPosCondition())
		{
			PushRequestMove();
		}
		else if (!m_State.IsExecutingRequest() && !m_State.m_bInCover)
		{
			if (DCO_FieldPeek(currentTime_ms))
				return ENodeResult.RUNNING;
			if (IsCriticalCombatMoment())
			{
				if (ShouldForceReturnFire(currentTime_ms) && DCO_Stress.Tier(m_Utility) == 0)
				{
					if (m_eStance == ECharacterStance.PRONE)
						DCO_Stance.Request(m_CharacterController, 2, "move_attack");
				}
				else if (CriticalBoundCooldownReady())
				{
					if (m_fTargetDist < DCO_Stress.T(m_Utility, "rt.close_m", 30.0) && m_CombatComp.IsTargetVisible(m_Target))
					{
						m_fNextCriticalBid_ms = currentTime_ms + 3000.0;
						DCO_Perf.Count("tm:critical_bound_skip_near");
						DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_MyEntity), string.Format("critical_bound unit=%1 action=skip_near target_d=%2 open=%3", DCO_Log.IdUnit(m_MyEntity), Math.Round(m_fTargetDist), DCO_Util.B01(m_bInOpenAreaCached)));
					}
					else
					{
						DCO_Perf.Count("tm:critical_bound");
						DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_MyEntity), string.Format("critical_bound unit=%1 action=sprint_back target_d=%2 open=%3", DCO_Log.IdUnit(m_MyEntity), Math.Round(m_fTargetDist), DCO_Util.B01(m_bInOpenAreaCached)));
						PushRequestCriticalBound();
					}
				}
				else
				{
					ECharacterStance criticalStance = ResolveStanceOutsideCover(m_bCloseRangeCombat, m_eThreatState);
					criticalStance = DCO_MoraleCombatUtility.ApplyMoraleStanceOverride(criticalStance, moraleSystem);
					if (criticalStance > m_eStance)
						m_State.ApplyRequestChangeStanceOutsideCover(criticalStance);
				}
			}
			else if (currentTime_ms >= m_fNextEngagedBid_ms)
			{
				float takeCoverChance;
				if (DCO_Stress.T(m_Utility, "agg.bound_v2", 1.0) > 0)
					takeCoverChance = DCO_AIConfigComponent.BoundChanceOf(m_Utility);
				else
				{
					takeCoverChance = DCO_AIConfigComponent.TakeCoverOf(m_Utility) * DCO_PersonalityCombatUtility.GetTakeCoverChanceScale(m_Utility);
					float boundA = DCO_Stress.AggrRel(m_Utility);
					if (boundA != 1.0)
					{
						DCO_AIConfigComponent boundCfg = DCO_AIConfigComponent.Of(m_Utility);
						if (boundCfg)
							takeCoverChance = boundCfg.DCO_TakeCoverRaw() * 0.4 * DCO_PersonalityCombatUtility.GetTakeCoverChanceScale(m_Utility) * Math.Pow(boundA, 0.7);
						DCO_Perf.Count("agg:bound_profile");
					}
				}

				if (m_bInOpenAreaCached)
					takeCoverChance *= 1.35;

				takeCoverChance = Math.Clamp(takeCoverChance, 0.0, 1.0);

				if (Math.RandomFloat01() < takeCoverChance)
				{
					float optimalDist = ResolveOptimalDistance(m_fWeaponMinDist);
					bool contactFresh = m_Target && m_Target.GetTimeSinceSeen() < 5.0;
					bool inEngagementRange = m_Target && m_fTargetDist <= optimalDist * 1.2;

					if ((contactFresh && inEngagementRange) || DCO_InOvermatch())
						PushRequestEngagedBound();
					else
						PushRequestOpenArea();
				}
				else
				{
					float bidCooldown_ms = 3000.0;

					if (m_bInOpenAreaCached)
						bidCooldown_ms *= 0.6;

					m_fNextEngagedBid_ms = currentTime_ms + bidCooldown_ms;
				}
			}
		} else if (!m_State.IsExecutingRequest())
		{
			if (m_Utility.GetCharacterController().GetWeaponObstructedState() != EWeaponObstructedState.UNOBSTRUCTED)
			{
				if (m_CharacterController.GetStance() == ECharacterStance.CROUCH)
					DCO_Stance.Request(m_CharacterController, 1, "move_attack");
				else if (m_CharacterController.GetStance() == ECharacterStance.PRONE)
					DCO_Stance.Request(m_CharacterController, 2, "move_attack");
			}
		}

		return ENodeResult.RUNNING;
	}

	protected void PushRequestEngagedBound()
	{
	    SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

	    rq.m_eReason    = SCR_EAICombatMoveReason.STANDARD;
	    rq.m_vTargetPos = ResolveRequestTargetPos();
	    rq.m_vMovePos   = rq.m_vTargetPos;

	    if ((DCO_AggressivePush() || DCO_Engage.CqbPush(m_Utility)) && DCO_Engage.AdvanceAllowed(m_Utility, "engaged_bound"))
	        rq.m_eDirection = SCR_EAICombatMoveDirection.FORWARD;
	    else if (Math.RandomIntInclusive(0, 1) == 0)
	        rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
	    else
	        rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;

	    rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;

	    rq.m_eStanceMoving = ECharacterStance.CROUCH;
	    rq.m_eStanceEnd    = ECharacterStance.CROUCH;
	    rq.m_eMovementType = EMovementType.RUN;

	    rq.m_bAimAtTarget = DCO_MoraleCombatUtility.CanAimWhileMoving(
	        DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection) &&
	        IsAimingAndMovingAllowedForWeapon(m_eWeaponType),
	        moraleSystem);
	    rq.m_bAimAtTargetEnd = true;

	    rq.m_eType         = SCR_EAICombatMoveRequestType.MOVE;
	    rq.m_bTryFindCover = true;
	    rq.m_bUseCoverSearchDirectivity = true;
	    rq.m_bCheckCoverVisibility      = true;
	    rq.m_bFailIfNoCover             = false;

	    float ebMin = 0;
	    float ebMax = 18.0;
	    DCO_Stress.Shape(m_Utility, rq, m_bCloseRangeCombat, m_State.m_bInCover, false, ebMin, ebMax, "engaged_bound");
	    rq.m_fCoverSearchDistMin = ebMin;
	    rq.m_fCoverSearchDistMax = ebMax;

	    rq.GetOnMovementStarted().Insert(OnMovementStarted);
	    rq.GetOnCompleted().Insert(OnMovementCompleted);

	    m_fNextEngagedBid_ms = GetGame().GetWorld().GetWorldTime() + 3000.0;
	    m_State.ApplyNewRequest(rq);
	    DCO_Opp.BoundCallout(m_Utility);
	}

	override protected bool SuppressedInCoverCondition()
	{
		return m_State.m_bInCover && m_eThreatState == EAIThreatState.THREATENED && moraleSystem.GetState() >= moraleState.PANIC;
	}

	protected bool IsCriticalCombatMoment()
	{
	    return m_eThreatState == EAIThreatState.THREATENED;
	}

	protected bool DCO_CanPushUnderFire()
	{
		DCO_EAIPersonality personality = DCO_PersonalityCombatUtility.GetPersonalitySafe(m_Utility);
		if (personality == DCO_EAIPersonality.RECKLESS)
			return true;

		if (personality == DCO_EAIPersonality.AGGRESSIVE || DCO_PostureCombatUtility.GetPosture(m_Utility) == DCO_GroupTactics.AGGRESIVE)
			return ShouldForceReturnFire(GetGame().GetWorld().GetWorldTime());

		return false;
	}

	protected bool DCO_OpenFwdOk(out string task)
	{
		task = "-";
		AIAgent ag = m_Utility.GetAIAgent();
		if (!ag)
			return false;
		AIGroup grp = ag.GetParentGroup();
		if (!grp)
			return false;
		DCO_GroupUtilityComponent dg = DCO_GroupUtilityComponent.Cast(grp.FindComponent(DCO_GroupUtilityComponent));
		if (!dg || dg.IsRetreating() || dg.IsBreakingContact())
			return false;
		DCO_EGroupTask gt = dg.GetTask();
		task = typename.EnumToString(DCO_EGroupTask, gt);
		return gt == DCO_EGroupTask.ATTACK || gt == DCO_EGroupTask.FLANK;
	}

	protected void PushRequestOpenArea()
	{
		if (DCO_Stress.Tier(m_Utility) >= 1)
		{
			DCO_EngageInfo ei = DCO_Engage.Info(m_Utility);
			if (ei && ei.m_iZone == 0)
			{
				vector rushCover;
				string rush = DCO_Engage.RushCheck(m_Utility, rushCover);
				if (rush != "ok")
				{
					m_fNextEngagedBid_ms = GetGame().GetWorld().GetWorldTime() + 4000.0;
					if (m_eStance == ECharacterStance.PRONE && rush == "enemy_close")
					{
						DCO_Perf.Count("tm:open_area_hold_prone");
						return;
					}
					DCO_OpenProneCrawl("open_area", rush);
					return;
				}
			}
		}
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;

		rq.m_vTargetPos = ResolveRequestTargetPos();
		rq.m_vMovePos = rq.m_vTargetPos;
		rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
		rq.m_fCoverSearchSectorHalfAngleRad;
		string fwdTask;
		if (DCO_Ctx.TuneForU(m_Utility, "agg.open_fwd", 0) > 0 && DCO_OpenFwdOk(fwdTask))
		{
			rq.m_eDirection = SCR_EAICombatMoveDirection.FORWARD;
			DCO_Perf.Count("agg:open_fwd");
			if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("agg_open_fwd unit=%1 task=%2 tgt_d=%3", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), fwdTask, Math.Round(m_fTargetDist)));
		}
		rq.m_bTryFindCover = true;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility = true;

		float coverSearchDistMin = 5;
		float coverSearchDistMax = 50;
		if (m_bCloseRangeCombat)
		{
			switch (m_eThreatState)
			{
				case EAIThreatState.THREATENED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eMovementType = EMovementType.WALK;
					break;
				}
				default:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
			}

			rq.m_bAimAtTarget = DCO_MoraleCombatUtility.CanAimWhileMoving(
				DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection) &&
				IsAimingAndMovingAllowedForWeapon(m_eWeaponType),
				moraleSystem);
			rq.m_bAimAtTargetEnd = true;
		}
		else
		{
			switch (m_eThreatState)
			{
				case EAIThreatState.THREATENED:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.PRONE;
					rq.m_eMovementType = EMovementType.SPRINT;
					break;
				}
				case EAIThreatState.ALERTED:
				{
					rq.m_eStanceMoving = ECharacterStance.CROUCH;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
				default:
				{
					rq.m_eStanceMoving = ECharacterStance.STAND;
					rq.m_eStanceEnd = ECharacterStance.CROUCH;
					rq.m_eMovementType = EMovementType.RUN;
					break;
				}
			}

			rq.m_bAimAtTarget = DCO_MoraleCombatUtility.CanAimWhileMoving(
				DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection) &&
				IsAimingAndMovingAllowedForWeapon(m_eWeaponType),
				moraleSystem);
			rq.m_bAimAtTargetEnd = true;
		}

		rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;

		rq.m_bFailIfNoCover = false;
		DCO_Stress.Shape(m_Utility, rq, m_bCloseRangeCombat, m_State.m_bInCover, false, coverSearchDistMin, coverSearchDistMax, "open_area");
		rq.m_fCoverSearchDistMin = coverSearchDistMin;
		rq.m_fCoverSearchDistMax = coverSearchDistMax;

		rq.GetOnMovementStarted().Insert(OnMovementStarted);
		rq.GetOnCompleted().Insert(OnMovementCompleted);

		m_State.ApplyNewRequest(rq);
	}

	override protected void PushRequestFFAvoidance()
	{
		if (m_CharacterController.GetStance() == ECharacterStance.PRONE)
		{
			if (Math.RandomFloat01() > 0.5)
				m_CharacterController.SetRoll(1);
			else
				m_CharacterController.SetRoll(2);

			return;
		}
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

		rq.m_eReason = SCR_EAICombatMoveReason.FF_AVOIDANCE;

		SCR_AICombatMoveRequest_Move prevRequest = SCR_AICombatMoveRequest_Move.Cast(m_State.GetRequest());
		if (prevRequest && prevRequest.m_eReason == SCR_EAICombatMoveReason.FF_AVOIDANCE)
		{
			rq.m_eDirection = prevRequest.m_eDirection;
		}
		else
		{
			if (Math.RandomIntInclusive(0, 1) == 1)
				rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
			else
				rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
		}

		rq.m_eStanceMoving = m_CharacterController.GetStance();
		rq.m_eStanceEnd = rq.m_eStanceMoving;
		rq.m_vMovePos = ResolveRequestTargetPos();
		rq.m_eMovementType = EMovementType.WALK;
		rq.m_fMoveDuration_s = 1.0;
		rq.m_bAimAtTarget = SCR_AICombatMoveUtils.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType);
		rq.m_bAimAtTargetEnd = true;

		m_State.ApplyNewRequest(rq);
	}

	override protected void PushRequestLeaveUselessCover()
	{
		SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();

		rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;

		rq.m_vTargetPos = ResolveRequestTargetPos();
		rq.m_vMovePos = rq.m_vTargetPos;
		rq.m_bTryFindCover = true;
		rq.m_bUseCoverSearchDirectivity = true;
		rq.m_bCheckCoverVisibility = true;
		rq.m_bFailIfNoCover = false;
		rq.m_fCoverSearchDistMin = 0;
		rq.m_fCoverSearchDistMax = 30;
		if (m_CharacterController.GetStance() == ECharacterStance.PRONE)
			rq.m_eStanceMoving = ECharacterStance.CROUCH;
		else
			rq.m_eStanceMoving = m_CharacterController.GetStance();
		rq.m_eStanceEnd = ECharacterStance.CROUCH;
		rq.m_eMovementType = EMovementType.RUN;
		rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
		rq.m_bAimAtTarget = DCO_MoraleCombatUtility.CanAimWhileMoving(
			DCO_CombatMoveUtility.IsAimingAndMovementPossible(rq.m_eStanceMoving, rq.m_eMovementType, rq.m_eDirection) &&
			IsAimingAndMovingAllowedForWeapon(m_eWeaponType),
			moraleSystem);
		rq.m_bAimAtTargetEnd = true;
		if (m_CharacterController.GetStance() == ECharacterStance.STAND)
			rq.m_fMoveDuration_s = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_STAND_RUN;
		else
			rq.m_fMoveDuration_s = rq.m_fCoverSearchDistMax / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN;

		m_State.ApplyNewRequest(rq);
	}

	protected static ref TStringArray s_aVarsIn;
	protected static TStringArray GetVarsIn()
	{
		if (!s_aVarsIn)
			s_aVarsIn = { PORT_BASE_TARGET, "AvoidStraightPathDir" };
		return s_aVarsIn;
	}
	override TStringArray GetVariablesIn() { return GetVarsIn(); }

	protected float m_fDCOPeekExposeUntil_ms;
	protected float m_fDCOPeekHideUntil_ms;
	protected vector m_vDCOPeekSpot;
	protected int m_iDCOPeeks;
	protected int m_iDCOPeekLimit;
	protected float m_fDCOPeekSide = 1;
	protected float m_fDCOPeekShiftNext_ms;
	protected bool m_bDCOHiddenSeen;
	protected vector m_vDCOFieldSpot;
	protected int m_iDCOFieldPair = -1;
	protected bool m_bDCOFieldUp;

	protected int DCO_FieldCoverPair(vector me)
	{
		vector dir = DCO_Specialist.Flat(GetTargetPosition() - me);
		if (dir.LengthSq() < 0.01)
			return 0;
		vector reach = dir * 3.0;
		int flags = TraceFlags.WORLD | TraceFlags.ENTS;
		float lo = DCO_Util.TraceFrac(me + vector.Up * 0.4, me + vector.Up * 0.4 + reach, m_MyEntity, flags, -1, "trace:fieldpeek");
		float mid = DCO_Util.TraceFrac(me + vector.Up * 0.9, me + vector.Up * 0.9 + reach, m_MyEntity, flags, -1, "trace:fieldpeek");
		float cr = DCO_Util.TraceFrac(me + vector.Up * 1.0, me + vector.Up * 1.0 + reach, m_MyEntity, flags, -1, "trace:fieldpeek");
		float hi = DCO_Util.TraceFrac(me + vector.Up * 1.5, me + vector.Up * 1.5 + reach, m_MyEntity, flags, -1, "trace:fieldpeek");
		if (mid < 1.0 && hi >= 1.0)
			return 1;
		if (lo < 1.0 && cr >= 1.0 && DCO_CQC.IsProneAllowed(m_MyEntity, m_fTargetDist))
			return 2;
		if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_MyEntity), string.Format("cover_field_none unit=%1 lo=%2 mid=%3 cr=%4 hi=%5 stance=%6 target_dist=%7",
				DCO_Log.IdUnit(m_MyEntity), lo.ToString(1, 2), mid.ToString(1, 2), cr.ToString(1, 2), hi.ToString(1, 2), typename.EnumToString(ECharacterStance, m_CharacterController.GetStance()), Math.Round(m_fTargetDist)));
		return 0;
	}

	protected bool DCO_FieldPeek(float now)
	{
		if (!m_Target || !DCO_PeekCadenceOn() || m_CharacterController.IsReloading())
			return false;
		vector me = m_MyEntity.GetOrigin();
		if (m_iDCOFieldPair < 0 || vector.DistanceXZ(me, m_vDCOFieldSpot) > 1.0)
		{
			m_vDCOFieldSpot = me;
			m_iDCOFieldPair = DCO_FieldCoverPair(me);
			m_bDCOFieldUp = true;
			m_iDCOPeeks = 0;
			m_iDCOPeekLimit = Math.RandomIntInclusive(2, 3);
			m_fDCOPeekExposeUntil_ms = now + Math.RandomFloat(2.5, 4.5) * 1000.0;
			m_fDCOPeekHideUntil_ms = 0;
			if (m_iDCOFieldPair > 0)
				DCO_Perf.Count("cover:field_spot_" + m_iDCOFieldPair);
			else
				DCO_Perf.Count("cover:field_no_cover");
		}
		if (m_iDCOFieldPair <= 0)
			return false;
		ECharacterStance up = ECharacterStance.STAND;
		ECharacterStance down = ECharacterStance.CROUCH;
		if (m_iDCOFieldPair == 2)
		{
			up = ECharacterStance.CROUCH;
			down = ECharacterStance.PRONE;
		}
		bool fire = m_CombatComp && m_CombatComp.DCO_NearMissCount(now, 1500.0) >= 2;
		if (m_bDCOFieldUp)
		{
			if (now < m_fDCOPeekExposeUntil_ms && !fire)
				return false;
			m_bDCOFieldUp = false;
			float hide_s = Math.RandomFloat(1.5, 3.5);
			if (fire)
				hide_s = Math.RandomFloat(3.0, 5.0);
			m_fDCOPeekHideUntil_ms = now + hide_s * 1000.0;
			m_State.m_fDCOFieldHiddenUntil_ms = m_fDCOPeekHideUntil_ms;
			m_State.ApplyRequestChangeStanceOutsideCover(down);
			DCO_Perf.Count("cover:peek_hide");
			if (fire)
				DCO_Perf.Count("cover:peek_hide_under_fire");
			if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_MyEntity), string.Format("cover_peek_hide unit=%1 field=%2 hide_s=%3 under_fire=%4 peeks=%5/%6",
					DCO_Log.IdUnit(m_MyEntity), m_iDCOFieldPair, Math.Round(hide_s * 10) / 10, fire, m_iDCOPeeks, m_iDCOPeekLimit));
			return true;
		}
		if (now < m_fDCOPeekHideUntil_ms)
			return true;
		if (m_iDCOPeeks >= m_iDCOPeekLimit && now >= m_fDCOPeekShiftNext_ms && DCO_PeekShift(now))
		{
			m_iDCOFieldPair = -1;
			return true;
		}
		m_iDCOPeeks++;
		m_bDCOFieldUp = true;
		float expose_s = Math.RandomFloat(2.5, 4.5);
		m_fDCOPeekExposeUntil_ms = now + expose_s * 1000.0;
		m_State.ApplyRequestChangeStanceOutsideCover(up);
		DCO_Perf.Count("cover:peek");
		if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_MyEntity), string.Format("cover_peek unit=%1 field=%2 n=%3/%4 expose_s=%5 target_dist=%6",
				DCO_Log.IdUnit(m_MyEntity), m_iDCOFieldPair, m_iDCOPeeks, m_iDCOPeekLimit, Math.Round(expose_s * 10) / 10, Math.Round(m_fTargetDist)));
		return true;
	}

	protected bool DCO_PeekCadenceOn()
	{
		if (!m_Target || m_bCloseRangeCombat || m_Target.GetTimeSinceSeen() > 10.0)
			return false;
		if (m_eWeaponType == EWeaponType.WT_MACHINEGUN)
			return false;
		return true;
	}

	protected bool DCO_PeekShouldHide(float now)
	{
		if (!m_State.m_bInCover || !m_State.m_bExposedInCover || !DCO_PeekCadenceOn())
			return false;
		if (m_fDCOPeekExposeUntil_ms <= 0)
		{
			if (m_fDCOPeekHideUntil_ms > 0 && !m_bDCOHiddenSeen)
			{
				DCO_Perf.Count("cover:hide_not_applied");
				if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
				{
					string rqName = "none";
					if (m_State.GetRequest())
						rqName = m_State.GetRequest().ClassName();
					DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_MyEntity), string.Format("cover_hide_not_applied unit=%1 rq=%2 executing=%3 protected=%4 since_hide_ms=%5", DCO_Log.IdUnit(m_MyEntity), rqName, m_State.IsExecutingRequest(), m_State.DCO_IsCoverProtected(), Math.Round(now - (m_fDCOPeekHideUntil_ms - 2500.0))));
				}
				m_fDCOPeekHideUntil_ms = 0;
			}
			m_fDCOPeekExposeUntil_ms = now + Math.RandomFloat(2.5, 4.5) * 1000.0;
			DCO_Perf.Count("cover:peek_cycle_start");
			return false;
		}
		if (now >= m_fDCOPeekExposeUntil_ms)
			return true;
		return m_CombatComp && m_CombatComp.DCO_NearMissCount(now, 1500.0) >= 2;
	}

	protected void DCO_PeekHide(float now)
	{
		bool fire = m_CombatComp && m_CombatComp.DCO_NearMissCount(now, 1500.0) >= 2;
		float hide_s = Math.RandomFloat(1.5, 3.5);
		if (fire)
			hide_s = Math.RandomFloat(3.0, 5.0);
		m_fDCOPeekExposeUntil_ms = 0;
		m_bDCOHiddenSeen = false;
		m_fDCOPeekHideUntil_ms = now + hide_s * 1000.0;
		m_State.ApplyRequestChangeStanceInCover(false);
		DCO_Perf.Count("cover:peek_hide");
		if (fire)
			DCO_Perf.Count("cover:peek_hide_under_fire");
		if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_MyEntity), string.Format("cover_peek_hide unit=%1 hide_s=%2 under_fire=%3 peeks=%4/%5",
				DCO_Log.IdUnit(m_MyEntity), Math.Round(hide_s * 10) / 10, fire, m_iDCOPeeks, m_iDCOPeekLimit));
	}

	protected void DCO_PeekExpose(float now)
	{
		if (!DCO_PeekCadenceOn())
		{
			m_fDCOPeekExposeUntil_ms = 0;
			m_State.ApplyRequestChangeStanceInCover(true);
			return;
		}
		if (!m_bDCOHiddenSeen && m_fDCOPeekHideUntil_ms > 0)
		{
			m_bDCOHiddenSeen = true;
			DCO_Perf.Count("cover:hidden_seen");
		}
		if (now < m_fDCOPeekHideUntil_ms)
			return;

		vector me = m_MyEntity.GetOrigin();
		if (m_iDCOPeekLimit == 0 || vector.DistanceXZ(me, m_vDCOPeekSpot) > 1.5)
		{
			if (m_iDCOPeeks > 0)
				DCO_Perf.Count("cover:peek_spot_reset");
			m_vDCOPeekSpot = me;
			m_iDCOPeeks = 0;
			m_iDCOPeekLimit = Math.RandomIntInclusive(2, 3);
		}

		if (m_iDCOPeeks >= m_iDCOPeekLimit && now >= m_fDCOPeekShiftNext_ms && DCO_PeekShift(now))
			return;

		m_iDCOPeeks++;
		float expose_s = Math.RandomFloat(2.5, 4.5);
		m_fDCOPeekExposeUntil_ms = now + expose_s * 1000.0;
		m_State.ApplyRequestChangeStanceInCover(true);
		DCO_Perf.Count("cover:peek");
		if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(m_MyEntity), string.Format("cover_peek unit=%1 n=%2/%3 expose_s=%4 target_dist=%5",
				DCO_Log.IdUnit(m_MyEntity), m_iDCOPeeks, m_iDCOPeekLimit, Math.Round(expose_s * 10) / 10, Math.Round(m_fTargetDist)));
	}

	protected bool DCO_PeekShift(float now)
	{
		m_fDCOPeekShiftNext_ms = now + 15000.0;
		vector threat = GetTargetPosition();
		IEntity building = SCR_CoverManagerComponent.DCO_GetBuildingAt(m_MyEntity);
		string where = "outdoor";
		if (building)
		{
			where = "indoor";
			DCO_AICombatMoveRequest_IndoorRelocate irq = m_State.DCO_ApplyIndoorRelocate(building, threat, 8.0, 3.0, true, "peek_shift");
			if (!irq)
			{
				DCO_Perf.Count("cover:shift_fail_indoor");
				return false;
			}
			irq.m_eIntent = DCO_EIndoorRelocateIntent.FIRE_POSITION;
		}
		else
		{
			m_fDCOPeekSide = -m_fDCOPeekSide;
			SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
			rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
			rq.m_eReason = SCR_EAICombatMoveReason.STANDARD;
			rq.m_vTargetPos = threat;
			rq.m_vMovePos = threat;
			if (m_fDCOPeekSide > 0)
				rq.m_eDirection = SCR_EAICombatMoveDirection.RIGHT;
			else
				rq.m_eDirection = SCR_EAICombatMoveDirection.LEFT;
			rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
			rq.m_eStanceMoving = ECharacterStance.CROUCH;
			rq.m_eStanceEnd = ECharacterStance.CROUCH;
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_bAimAtTarget = false;
			rq.m_bAimAtTargetEnd = true;
			rq.m_bTryFindCover = true;
			rq.m_bUseCoverSearchDirectivity = true;
			rq.m_bCheckCoverVisibility = true;
			rq.m_bFailIfNoCover = true;
			rq.m_fCoverSearchDistMin = 3.0;
			rq.m_fCoverSearchDistMax = 8.0;
			rq.m_fMoveDuration_s = 8.0 / SCR_AICombatMoveUtils.CHARACTER_SPEED_CROUCH_RUN;
			m_State.ApplyNewRequest(rq);
		}
		m_iDCOPeeks = 0;
		m_iDCOPeekLimit = 0;
		m_fDCOPeekExposeUntil_ms = 0;
		m_fDCOPeekHideUntil_ms = 0;
		DCO_Perf.Count("cover:shift_" + where);
		if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_MyEntity), string.Format("cover_shift unit=%1 where=%2 side=%3 target_dist=%4",
				DCO_Log.IdUnit(m_MyEntity), where, m_fDCOPeekSide, Math.Round(m_fTargetDist)));
		return true;
	}
}