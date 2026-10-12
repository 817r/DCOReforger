class DCO_AIIdleWait : AITaskScripted
{

	protected SCR_AIUtilityComponent m_Utility;
	protected float m_fNextTry_ms;

	override void OnInit(AIAgent owner)
	{
		m_Utility = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
	}

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		if (!m_Utility || !m_Utility.m_CombatMoveState || m_Utility.m_CombatMoveState.m_bInCover)
			return ENodeResult.RUNNING;

		if (m_Utility.m_vDCOIdleCoverPos != vector.Zero && m_Utility.m_OwnerEntity)
		{
			float off = vector.DistanceXZ(m_Utility.m_OwnerEntity.GetOrigin(), m_Utility.m_vDCOIdleCoverPos);
			if (off < 2.5)
				return ENodeResult.RUNNING;
			if (off > 4.0)
			{
				bool marked = DCO_AIIdleHoldInPlace.MarkSlotUnreachable(m_Utility.m_vDCOIdleCoverPos);
				DCO_Perf.Count("idle_cover:short_stop");
				DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(m_Utility.m_OwnerEntity), string.Format("idle_cover who=%1 result=short_stop left=%2 slot_marked=%3",
					DCO_Log.IdUnit(m_Utility.m_OwnerEntity), Math.Round(off), DCO_Util.B01(marked)));
			}
			m_Utility.m_vDCOIdleCoverPos = vector.Zero;
		}

		float now_ms = GetGame().GetWorld().GetWorldTime();
		if (now_ms < m_fNextTry_ms)
			return ENodeResult.RUNNING;

		SCR_AIGroup group = SCR_AIGroup.Cast(owner.GetParentGroup());
		if (!group || !group.GetGroupUtilityComponent() || !group.GetGroupUtilityComponent().DCO_IsIdle())
			return ENodeResult.RUNNING;

		m_fNextTry_ms = now_ms + 20000.0;
		return ENodeResult.SUCCESS;
	}

	static override bool VisibleInPalette() { return true; }
	static override string GetOnHoverDescription() { return "RUNNING until the group is IDLE and the AI is not in cover, then SUCCESS."; }
}

class DCO_FindIndoorPositionIdle : DCO_FindIndoorPosition
{
	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(owner);
		if (cfg)
			m_fRadius = DCO_AICreateIdleCoverQueryProps.IdleRadius(owner, cfg.GetIdleCoverSearchDist());

		return super.EOnTaskSimulate(owner, dt);
	}
}

class DCO_AICreateIdleCoverQueryProps : SCR_AICreateBasicCoverQueryProps
{
	static float IdleRadius(AIAgent owner, float radius)
	{
		SCR_AIGroup group = SCR_AIGroup.Cast(owner.GetParentGroup());
		if (!group || !group.GetGroupUtilityComponent())
			return radius;
		DCO_GroupUtilityComponent util = group.GetGroupUtilityComponent().DCO_GetUtil();
		if (!util || !util.GetGroupObjective() || radius <= 20.0)
			return radius;
		DCO_Perf.Count("idle_cover_leash");
		return 20.0;
	}

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetFor(owner);
		if (cfg)
			m_fRadius = IdleRadius(owner, cfg.GetIdleCoverSearchDist());

		return super.EOnTaskSimulate(owner, dt);
	}
}

class DCO_AIIdleCoverReport : AITaskScripted
{
	[Attribute("indoor", UIWidgets.EditBox, "Reported result: indoor or outdoor.")]
	protected string m_sResult;

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		vector pos;
		vector leader;
		GetVariableIn("Position", pos);
		GetVariableIn("LeaderPos", leader);
		IEntity me = owner.GetControlledEntity();
		DCO_Perf.Count("idle_cover:" + m_sResult);
		SCR_AIUtilityComponent u = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
		if (u)
			u.m_vDCOIdleCoverPos = pos;
		if (me)
		{
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("idle_cover who=%1 result=%2 dist=%3 leader_dist=%4",
				DCO_Log.IdUnit(me), m_sResult, Math.Round(vector.DistanceXZ(me.GetOrigin(), pos)), Math.Round(vector.DistanceXZ(leader, pos))));
		}
		return ENodeResult.SUCCESS;
	}

	protected ref TStringArray m_aDCOVarsIn = { "Position", "LeaderPos" };
	override TStringArray GetVariablesIn() { return m_aDCOVarsIn; }

	static override bool VisibleInPalette() { return true; }
	static override string GetOnHoverDescription() { return "DCO: log + counter for the idle cover result (indoor/outdoor)."; }
}

class DCO_AIIdleHoldInPlace : AITaskScripted
{
	protected SCR_AIUtilityComponent m_Utility;

	override void OnInit(AIAgent owner)
	{
		m_Utility = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
	}

	override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
	{
		IEntity me = owner.GetControlledEntity();
		if (!me || !m_Utility)
			return ENodeResult.FAIL;

		bool open = IsInOpenArea(me, 15.0);
		ECharacterStance stance = ECharacterStance.CROUCH;
		if (open && DCO_CQC.IsProneAllowed(me, 1000.0))
			stance = ECharacterStance.PRONE;
		CharacterControllerComponent ctrl = m_Utility.GetCharacterController();
		if (ctrl && !DCO_Nav.Busy(ctrl, "idle_stance"))
			DCO_Stance.Set(ctrl, stance, "idle");

		vector leader;
		GetVariableIn("LeaderPos", leader);
		vector look = vector.Zero;
		vector myPos = me.GetOrigin();
		if (vector.DistanceXZ(leader, myPos) > 2.0)
		{
			look = myPos + DCO_Specialist.Flat(myPos - leader) * 30.0;
		}
		else
		{
			SCR_AIGroup group = SCR_AIGroup.Cast(owner.GetParentGroup());
			DCO_GroupUtilityComponent util;
			if (group && group.GetGroupUtilityComponent())
				util = group.GetGroupUtilityComponent().DCO_GetUtil();
			if (util && util.GetGroupObjective())
				look = util.GetGroupObjective().GetOwner().GetOrigin();
		}
		if (look != vector.Zero && m_Utility.m_LookAction)
			m_Utility.m_LookAction.LookAt(look + Vector(0, 1.6, 0), 1.0, 3.0);

		vector target = m_Utility.m_vDCOIdleCoverPos;
		if (target != vector.Zero && vector.DistanceXZ(myPos, target) >= 2.5)
		{
			m_Utility.m_vDCOIdleCoverPos = vector.Zero;
			bool marked = MarkSlotUnreachable(target);
			DCO_Perf.Count("idle_cover:move_failed");
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(me), string.Format("idle_cover who=%1 result=move_failed left=%2 dy=%3 stance=%4 slot_marked=%5",
				DCO_Log.IdUnit(me), Math.Round(vector.DistanceXZ(myPos, target)), Math.Round(target[1] - myPos[1]), typename.EnumToString(ECharacterStance, stance), DCO_Util.B01(marked)));
			return ENodeResult.SUCCESS;
		}

		DCO_Perf.Count("idle_cover:none");
		if (DCO_Log.On(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG))
		{
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.DEBUG, DCO_Log.IdUnit(me), string.Format("idle_cover who=%1 result=none stance=%2 open=%3",
				DCO_Log.IdUnit(me), typename.EnumToString(ECharacterStance, stance), DCO_Util.B01(open)));
		}
		return ENodeResult.SUCCESS;
	}

	static bool MarkSlotUnreachable(vector target)
	{
		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		if (!reg)
			return false;
		array<IEntity> blds = {};
		reg.FindBuildings(target, 20.0, blds);
		foreach (IEntity bld : blds)
		{
			DCO_GarrisonBuilding gb = reg.Peek(bld);
			if (!gb)
				continue;
			foreach (DCO_GarrisonSlot s : gb.m_aSlots)
			{
				if (s && vector.DistanceSq(s.m_vWorldPos, target) < 0.25)
				{
					s.m_bUnreachable = true;
					DCO_Perf.Count("idle_cover:slot_unreachable");
					return true;
				}
			}
		}
		return false;
	}

	protected ref TStringArray m_aDCOVarsIn = { "LeaderPos" };
	override TStringArray GetVariablesIn() { return m_aDCOVarsIn; }

	static override bool VisibleInPalette() { return true; }
	static override string GetOnHoverDescription() { return "DCO: no idle cover found; hold position, CROUCH (PRONE if in the open), face the outward sector from the leader."; }
}

modded class SCR_AIUtilityComponent
{
	vector m_vDCOIdleCoverPos;
}
