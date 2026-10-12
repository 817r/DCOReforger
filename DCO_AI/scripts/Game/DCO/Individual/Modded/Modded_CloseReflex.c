modded class SCR_AICombatComponent
{
	protected float m_fDCOCloseUntil_ms;
	protected BaseTarget m_DCOCloseTarget;
	protected ref array<BaseTarget> m_aDCOCloseScan;

	bool DCO_IsCloseFight()
	{
		return m_DCOCloseTarget && GetGame().GetWorld().GetWorldTime() < m_fDCOCloseUntil_ms;
	}

	BaseTarget DCO_GetCloseTarget()
	{
		if (!DCO_IsCloseFight())
			return null;
		return m_DCOCloseTarget;
	}

	protected BaseTarget DCO_FindCloseTarget(float radius = 15.0)
	{
		if (!m_Utility || !m_Utility.m_PerceptionComponent || !GetOwner())
			return null;
		if (!m_aDCOCloseScan)
			m_aDCOCloseScan = {};
		m_aDCOCloseScan.Clear();
		m_Utility.m_PerceptionComponent.GetTargetsList(m_aDCOCloseScan, ETargetCategory.ENEMY);
		BaseTarget best;
		float bestD = radius;
		foreach (BaseTarget t : m_aDCOCloseScan)
		{
			if (!t || t.GetTimeSinceSeen() > 1.5)
				continue;
			ChimeraCharacter tc = ChimeraCharacter.Cast(t.GetTargetEntity());
			if (!tc || !DCO_Util.IsAlive(tc))
				continue;
			float d = vector.Distance(GetOwner().GetOrigin(), tc.GetOrigin());
			if (d < bestD)
			{
				bestD = d;
				best = t;
			}
		}
		return best;
	}

	protected BaseTarget DCO_CloseOverride(BaseTarget newTarget, float now)
	{
		if (m_AIInfo && m_AIInfo.HasUnitState(EUnitState.IN_VEHICLE) && !m_AIInfo.HasUnitState(EUnitState.IN_TURRET))
			return newTarget;
		if (m_SelectedWeaponComp && m_SelectedWeaponComp.GetWeaponType() == EWeaponType.WT_ROCKETLAUNCHER)
			return newTarget;

		BaseTarget close = DCO_FindCloseTarget();
		if (!close)
		{
			if (m_DCOCloseTarget && now >= m_fDCOCloseUntil_ms)
				m_DCOCloseTarget = null;
			return newTarget;
		}

		bool fresh = !DCO_IsCloseFight();
		m_DCOCloseTarget = close;
		m_fDCOCloseUntil_ms = now + 2500.0;

		float closeD = vector.Distance(GetOwner().GetOrigin(), close.GetTargetEntity().GetOrigin());
		float prevD = -1;
		if (newTarget && newTarget.GetTargetEntity())
			prevD = vector.Distance(GetOwner().GetOrigin(), newTarget.GetTargetEntity().GetOrigin());

		BaseTarget result = newTarget;
		string action = "keep";
		if (newTarget != close && (!newTarget || prevD < 0 || prevD > closeD * 1.5 + 2.0))
		{
			result = close;
			action = "switch";
			DCO_Perf.Count("reflex:switch_target");
		}

		if (fresh)
		{
			DCO_Perf.Count("reflex:start");
			if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
			{
				string threat = "?";
				if (m_Utility.m_ThreatSystem)
					threat = typename.EnumToString(EAIThreatState, m_Utility.m_ThreatSystem.GetState());
				string beh = "none";
				if (m_Utility.GetCurrentBehavior())
					beh = m_Utility.GetCurrentBehavior().ClassName();
				DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(GetOwner()), string.Format("close_reflex unit=%1 enemy=%2 dist=%3 action=%4 prev_dist=%5 threat=%6 beh=%7 player=%8",
					DCO_Log.IdUnit(GetOwner()), DCO_Log.IdUnit(close.GetTargetEntity()), Math.Round(closeD * 10) / 10, action, Math.Round(prevD), threat, beh, SCR_CharacterHelper.IsPlayer(close.GetTargetEntity())));
			}
		}
		return result;
	}
}

modded class SCR_AIAttackBehavior
{
	protected bool m_bDCONearFloorLogged;

	override float CustomEvaluate()
	{
		float p = super.CustomEvaluate();
		if (p <= 0 || !m_CombatComponent)
			return p;
		BaseTarget close = m_CombatComponent.DCO_GetCloseTarget();
		BaseTarget t = m_Target.m_Value;
		bool closeMatch = close && t && t.GetTargetEntity() == close.GetTargetEntity();
		if (p < PRIORITY_BEHAVIOR_ATTACK_SELECTED && t && t.GetTargetEntity() && m_Utility && m_Utility.m_OwnerEntity)
		{
			bool holdFire = p <= PRIORITY_BEHAVIOR_ATTACK_DISREGARD_THREATS;
			float nd = vector.Distance(t.GetTargetEntity().GetOrigin(), m_Utility.m_OwnerEntity.GetOrigin());
			bool nearSeen = t.GetTimeSinceSeen() < 1.5 && nd < DCO_Stress.T(m_Utility, "rt.close_m", 30.0);
			if ((!holdFire && nearSeen) || (holdFire && closeMatch))
			{
				if (holdFire)
					DCO_Perf.Count("atk:near_floor_hold");
				else
					DCO_Perf.Count("atk:near_floor");
				if (!m_bDCONearFloorLogged)
				{
					m_bDCONearFloorLogged = true;
					string fb = "-";
					if (m_Utility.GetCurrentBehavior())
						fb = m_Utility.GetCurrentBehavior().ClassName();
					DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("attack_near_floor unit=%1 tgt=%2 dist=%3 from_p=%4 to_p=%5 hold_fire=%6 beh=%7", DCO_Log.IdUnit(m_Utility.m_OwnerEntity), DCO_Log.IdUnit(t.GetTargetEntity()), Math.Round(nd), Math.Round(p), Math.Round(PRIORITY_BEHAVIOR_ATTACK_SELECTED), DCO_Util.B01(holdFire), fb));
				}
				p = PRIORITY_BEHAVIOR_ATTACK_SELECTED;
				if (holdFire)
					return p;
			}
		}
		if (p <= PRIORITY_BEHAVIOR_ATTACK_DISREGARD_THREATS || !closeMatch)
			return p;
		if (p >= PRIORITY_BEHAVIOR_ATTACK_HIGH_PRIORITY)
		{
			DCO_Perf.Count("reflex:attack_already_high");
			return p;
		}
		DCO_Perf.Count("reflex:attack_high");
		return PRIORITY_BEHAVIOR_ATTACK_HIGH_PRIORITY;
	}
}

modded class SCR_AICombatMoveLogic_Attack
{
	protected float m_fDCODuckNext_ms;

	protected bool DCO_CloseDuck(float now)
	{
		if (!m_CombatComp || !m_State || m_State.m_bInCover || now < m_fDCODuckNext_ms)
			return false;
		BaseTarget close = m_CombatComp.DCO_GetCloseTarget();
		if (!close || !close.GetTargetEntity())
			return false;
		m_fDCODuckNext_ms = now + 6000.0;
		vector threat = close.GetTargetEntity().GetOrigin();
		IEntity building = SCR_CoverManagerComponent.DCO_GetBuildingAt(m_MyEntity);
		string where = "outdoor";
		if (building)
		{
			where = "indoor";
			if (!m_State.DCO_ApplyIndoorRelocate(building, threat, 6.0, 2.0, true, "close_duck"))
				return false;
		}
		else
		{
			SCR_AICombatMoveRequest_Move rq = new SCR_AICombatMoveRequest_Move();
			rq.m_eType = SCR_EAICombatMoveRequestType.MOVE;
			rq.m_eReason = SCR_EAICombatMoveReason.MOVE_FROM_DANGER;
			rq.m_vTargetPos = threat;
			rq.m_vMovePos = threat;
			rq.m_eDirection = SCR_EAICombatMoveDirection.BACKWARD;
			rq.m_fCoverSearchSectorHalfAngleRad = COVER_QUERY_SECTOR_ANGLE_RAD;
			rq.m_eStanceMoving = ECharacterStance.CROUCH;
			rq.m_eStanceEnd = ECharacterStance.CROUCH;
			rq.m_eMovementType = EMovementType.RUN;
			rq.m_bAimAtTarget = false;
			rq.m_bAimAtTargetEnd = true;
			rq.m_bTryFindCover = true;
			rq.m_bUseCoverSearchDirectivity = true;
			rq.m_bCheckCoverVisibility = true;
			rq.m_bFailIfNoCover = false;
			rq.m_fCoverSearchDistMin = 0;
			rq.m_fCoverSearchDistMax = 8.0;
			rq.m_fMoveDuration_s = 3.0;
			m_State.ApplyNewRequest(rq);
		}
		DCO_Perf.Count("reflex:duck_" + where);
		if (DCO_Log.On(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO))
			DCO_Log.Write(DCO_ELogChannel.COMBAT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_MyEntity), string.Format("close_duck unit=%1 why=reloading where=%2 enemy_dist=%3",
				DCO_Log.IdUnit(m_MyEntity), where, Math.Round(vector.Distance(threat, m_MyEntity.GetOrigin()) * 10) / 10));
		return true;
	}
}
