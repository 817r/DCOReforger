class DCO_CQC
{
	static const float CQC_DIST = 12.0;
	static const float AIM_SPEEDUP = 3.0;
	static const float MOVING_PENALTY_SCALE = 0.4;
	static const float TARGET_SWITCH_SCALE = 0.3;
	static const float COVER_MAX_DIST = 6.0;
	static const float STEP_MAX_S = 2.0;

	static const float LEAN_OFFSET = 0.45;
	static const float LEAN_EVAL_MS = 500;
	static const float LEAN_HOLD_MS = 1500;
	static const float LEAN_MAX_DIST = 150;
	static const float LEAN_TARGET_SEEN_S = 10;
	static const float LEAN_AIM_HEIGHT = 1.3;
	static const float LEAN_MAX_SPEED = 0.5;

	static float TargetDistance(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_CombatComponent || !utility.m_OwnerEntity)
			return float.MAX;

		BaseTarget target = utility.m_CombatComponent.GetCurrentTarget();
		if (!target || !target.GetTargetEntity())
			return float.MAX;

		return vector.Distance(utility.m_OwnerEntity.GetOrigin(), target.GetLastSeenPosition());
	}

	static bool IsCQC(SCR_AIUtilityComponent utility)
	{
		return TargetDistance(utility) < CQC_DIST;
	}

	static bool IsTargetInMyBuilding(SCR_AIUtilityComponent utility)
	{
		if (!utility || !utility.m_CombatComponent || !utility.m_OwnerEntity)
			return false;

		BaseTarget target = utility.m_CombatComponent.GetCurrentTarget();
		if (!target || !target.GetTargetEntity())
			return false;

		IEntity myBuilding = SCR_CoverManagerComponent.DCO_GetBuildingAt(utility.m_OwnerEntity);
		return myBuilding && SCR_CoverManagerComponent.DCO_GetBuildingAt(target.GetTargetEntity()) == myBuilding;
	}

	static bool IsCloseFight(SCR_AIUtilityComponent utility)
	{
		return IsCQC(utility) || IsTargetInMyBuilding(utility);
	}

	static bool IsProneAllowed(IEntity me, float threatDist)
	{
		return threatDist >= SCR_AICombatMoveUtils.CLOSE_RANGE_COMBAT_DIST && !SCR_CoverManagerComponent.IsEntityInsideBuilding(me);
	}

	static void ClampProne(SCR_AIUtilityComponent utility, notnull SCR_AICombatMoveRequestBase request)
	{
		if (!utility || !utility.m_OwnerEntity)
			return;

		SCR_AICombatMoveRequest_Move moveRq = SCR_AICombatMoveRequest_Move.Cast(request);
		SCR_AICombatMoveRequest_ChangeStance stanceRq = SCR_AICombatMoveRequest_ChangeStance.Cast(request);
		if (moveRq)
		{
			if (moveRq.m_eStanceEnd != ECharacterStance.PRONE && moveRq.m_eStanceMoving != ECharacterStance.PRONE)
				return;
		}
		else if (!stanceRq || stanceRq.m_eStance != ECharacterStance.PRONE)
			return;

		float threatDist = TargetDistance(utility);
		if (moveRq && moveRq.m_vTargetPos != vector.Zero)
			threatDist = Math.Min(threatDist, vector.Distance(utility.m_OwnerEntity.GetOrigin(), moveRq.m_vTargetPos));

		if (IsProneAllowed(utility.m_OwnerEntity, threatDist))
			return;

		if (stanceRq)
		{
			stanceRq.m_eStance = ECharacterStance.CROUCH;
			return;
		}

		if (moveRq.m_eStanceEnd == ECharacterStance.PRONE)
			moveRq.m_eStanceEnd = ECharacterStance.CROUCH;
		if (moveRq.m_eStanceMoving == ECharacterStance.PRONE)
			moveRq.m_eStanceMoving = ECharacterStance.CROUCH;
		moveRq.m_bAimAtTargetEnd = true;
	}

	static bool ShouldAssault(SCR_AIUtilityComponent utility)
	{
		if (TargetDistance(utility) >= SCR_AICombatMoveUtils.CLOSE_RANGE_COMBAT_DIST)
			return false;

		if (DCO_PostureCombatUtility.GetPosture(utility) == DCO_GroupTactics.AGGRESIVE)
			return true;

		DCO_EAIPersonality p = DCO_PersonalityCombatUtility.GetPersonalitySafe(utility);
		return p == DCO_EAIPersonality.AGGRESSIVE || p == DCO_EAIPersonality.RECKLESS;
	}

	static bool IsAdvanceRequest(SCR_AICombatMoveRequestBase request)
	{
		SCR_AICombatMoveRequest_Move rq = SCR_AICombatMoveRequest_Move.Cast(request);
		return rq && (rq.m_eDirection == SCR_EAICombatMoveDirection.FORWARD || rq.m_eDirection == SCR_EAICombatMoveDirection.CUSTOM_POS);
	}

	static void ClampMoveRequest(notnull SCR_AICombatMoveRequest_Move rq)
	{
		if (rq.m_eUnitType != SCR_EAICombatMoveUnitType.CHARACTER
			|| rq.m_eReason == SCR_EAICombatMoveReason.MOVE_FROM_DANGER
			|| DCO_AICombatMoveRequest_IndoorRelocate.Cast(rq))
			return;

		if (rq.m_bTryFindCover && rq.m_fCoverSearchDistMax > COVER_MAX_DIST)
		{
			rq.m_fCoverSearchDistMax = COVER_MAX_DIST;
			rq.m_fCoverSearchDistMin = Math.Min(rq.m_fCoverSearchDistMin, COVER_MAX_DIST * 0.5);
		}

		switch (rq.m_eDirection)
		{
			case SCR_EAICombatMoveDirection.BACKWARD:
			case SCR_EAICombatMoveDirection.LEFT:
			case SCR_EAICombatMoveDirection.RIGHT:
			case SCR_EAICombatMoveDirection.ANYWHERE:
				rq.m_fMoveDuration_s = Math.Min(rq.m_fMoveDuration_s, STEP_MAX_S);
				break;
		}

		if (rq.m_eMovementType == EMovementType.SPRINT)
			rq.m_eMovementType = EMovementType.RUN;
		rq.m_bAimAtTarget = true;
	}

	static float ResolveLean(notnull ChimeraCharacter self, IEntity target, vector targetPos, float currentLean)
	{
		vector eye = self.EyePosition();
		vector dir = targetPos - eye;
		dir[1] = 0;
		if (dir.LengthSq() < 0.01)
			return 0;
		dir.Normalize();
		vector right = -(dir * vector.Up);
		eye = eye - right * (currentLean * LEAN_OFFSET);

		if (IsClear(self, target, eye, targetPos))
			return 0;

		bool rightOpen = IsSideOpen(self, target, eye, right, targetPos);
		bool leftOpen = IsSideOpen(self, target, eye, -right, targetPos);
		if (rightOpen)
			return 1;
		if (leftOpen)
			return -1;
		return 0;
	}

	protected static bool IsSideOpen(ChimeraCharacter self, IEntity target, vector eye, vector side, vector targetPos)
	{
		vector peek = eye + side * LEAN_OFFSET;
		return IsClear(self, null, eye, peek) && IsClear(self, target, peek, targetPos);
	}

	protected static bool IsClear(IEntity self, IEntity target, vector from, vector to)
	{
		TraceParam param = new TraceParam();
		param.Start = from;
		param.End = to;
		param.Exclude = self;
		param.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
		param.LayerMask = EPhysicsLayerDefs.Projectile;

		DCO_Perf.Count("t:DCO_CQC");
		if (GetGame().GetWorld().TraceMove(param, null) >= 0.98)
			return true;

		return target && param.TraceEnt && param.TraceEnt.GetRootParent() == target.GetRootParent();
	}
}
