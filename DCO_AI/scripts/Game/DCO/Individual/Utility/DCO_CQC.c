class DCO_CQC
{


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
		return TargetDistance(utility) < 12.0;
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
		if (DCO_AssaultLock.CheckBlock(DCO_BldLearn.UtilOf(me), "prone"))
			return false;
		if (threatDist >= SCR_AICombatMoveUtils.CLOSE_RANGE_COMBAT_DIST && !SCR_CoverManagerComponent.IsEntityInsideBuilding(me))
			return ProneRoll(me);
		return CQBProneRoll(me);
	}

	static bool CQBProneRoll(IEntity me)
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetFor(me);
		if (!g)
			return false;
		float chance = g.GetCQBProneChance();
		bool ok;
		if (chance >= 1.0)
			ok = true;
		else if (chance > 0.0)
		{
			SCR_AICombatComponent cc = SCR_AICombatComponent.Cast(me.FindComponent(SCR_AICombatComponent));
			if (cc)
				ok = cc.DCO_CQBProneRoll(chance);
			else
				ok = Math.RandomFloat01() < chance;
		}
		if (ok)
			DCO_Perf.Count("prone_cqb:allowed");
		else
			DCO_Perf.Count("prone_cqb:blocked");
		return ok;
	}

	static bool ProneRoll(IEntity me)
	{
		DCO_GlobalAIComponent g = DCO_GlobalAIComponent.GetFor(me);
		if (!g || g.GetProneChance() >= 1.0)
			return true;
		float chance = g.GetProneChance();
		if (chance <= 0.0)
		{
			DCO_Perf.Count("prone_blocked:chance0");
			return false;
		}
		ChimeraCharacter ch = ChimeraCharacter.Cast(me);
		if (!ch)
			return true;
		CharacterControllerComponent ctrl = ch.GetCharacterController();
		if (ctrl && ctrl.GetStance() == ECharacterStance.PRONE)
			return true;
		SCR_AICombatComponent cc = SCR_AICombatComponent.Cast(ch.FindComponent(SCR_AICombatComponent));
		if (cc && cc.DCO_ProneRoll(chance))
			return true;
		if (!cc && Math.RandomFloat01() < chance)
			return true;
		DCO_Perf.Count("prone_blocked:chance");
		return false;
	}

	static void ClampProneStance(SCR_AICombatMoveRequestBase request, IEntity me)
	{
		SCR_AICombatMoveRequest_Move moveRq = SCR_AICombatMoveRequest_Move.Cast(request);
		if (!moveRq || (moveRq.m_eStanceEnd != ECharacterStance.PRONE && moveRq.m_eStanceMoving != ECharacterStance.PRONE) || ProneRoll(me))
			return;
		if (moveRq.m_eStanceEnd == ECharacterStance.PRONE)
			moveRq.m_eStanceEnd = ECharacterStance.CROUCH;
		if (moveRq.m_eStanceMoving == ECharacterStance.PRONE)
			moveRq.m_eStanceMoving = ECharacterStance.CROUCH;
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

		if (rq.m_bTryFindCover && rq.m_fCoverSearchDistMax > 6.0)
		{
			rq.m_fCoverSearchDistMax = 6.0;
			rq.m_fCoverSearchDistMin = Math.Min(rq.m_fCoverSearchDistMin, 6.0 * 0.5);
		}

		switch (rq.m_eDirection)
		{
			case SCR_EAICombatMoveDirection.BACKWARD:
			case SCR_EAICombatMoveDirection.LEFT:
			case SCR_EAICombatMoveDirection.RIGHT:
			case SCR_EAICombatMoveDirection.ANYWHERE:
				rq.m_fMoveDuration_s = Math.Min(rq.m_fMoveDuration_s, 2.0);
				break;
		}

		if (rq.m_eMovementType == EMovementType.SPRINT)
			rq.m_eMovementType = EMovementType.RUN;
		rq.m_bAimAtTarget = true;
	}

	static float ResolveLean(notnull ChimeraCharacter self, IEntity target, vector targetPos, float currentLean, float prefer = 0)
	{
		vector eye = self.EyePosition();
		vector dir = targetPos - eye;
		dir[1] = 0;
		if (dir.LengthSq() < 0.01)
			return 0;
		dir.Normalize();
		vector right = -(dir * vector.Up);
		eye = eye - right * (currentLean * 0.45);

		if (IsClear(self, target, eye, targetPos))
			return 0;

		float first = 1;
		if (prefer < 0)
			first = -1;
		if (IsSideOpen(self, target, eye, right * first, targetPos, 0.35))
			return first;
		if (IsSideOpen(self, target, eye, -right * first, targetPos, 0.35))
			return -first;
		if (IsSideOpen(self, target, eye, right * first, targetPos, 0.6))
			return first;
		if (IsSideOpen(self, target, eye, -right * first, targetPos, 0.6))
			return -first;
		return 0;
	}

	static float CornerLean(notnull ChimeraCharacter self, out int reason, out vector corner)
	{
		reason = 0;
		vector fwd = self.GetWorldTransformAxis(2);
		if (self.GetPhysics())
		{
			vector vel = self.GetPhysics().GetVelocity();
			vel[1] = 0;
			if (vel.LengthSq() > 0.25)
				fwd = vel;
		}
		fwd[1] = 0;
		if (fwd.LengthSq() < 0.01)
			return 0;
		fwd.Normalize();
		vector right = -(fwd * vector.Up);
		vector eye = self.EyePosition();
		bool wallR = !IsClear(self, null, eye, eye + right * 1.4);
		bool wallL = !IsClear(self, null, eye, eye - right * 1.4);
		if (!wallR && !wallL)
			return 0;
		vector ahead = eye + fwd * 1.8;
		if (!IsClear(self, null, eye, ahead))
			return 0;
		bool openR = wallR && IsClear(self, null, ahead, ahead + right * 2.5);
		bool openL = wallL && IsClear(self, null, ahead, ahead - right * 2.5);
		if (openR == openL)
			return 0;
		reason = 5;
		if (wallR && wallL)
			reason = 6;
		if (openR)
		{
			corner = ahead + right * 2.5;
			return 1;
		}
		corner = ahead - right * 2.5;
		return -1;
	}

	protected static bool IsSideOpen(ChimeraCharacter self, IEntity target, vector eye, vector side, vector targetPos, float offset)
	{
		vector peek = eye + side * offset;
		return IsClear(self, null, eye, peek) && IsClear(self, target, peek, targetPos);
	}

	protected static bool IsClear(IEntity self, IEntity target, vector from, vector to)
	{
		return DCO_Util.TraceClear(from, to, self, 0.98, TraceFlags.WORLD | TraceFlags.ENTS, EPhysicsLayerDefs.Projectile, target, "t:DCO_CQC");
	}
}
