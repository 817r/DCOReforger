modded class SCR_AIUtilityComponent
{
	float m_fDCORearAt_ms;
	float m_fDCOFrontLogAt_ms;
}

class DCO_FrontHold
{
	static bool FrontOn()
	{
		return DCO_Ctx.TuneG("front.on", 1.0) > 0;
	}

	static bool Front(SCR_AIUtilityComponent u, out vector dir)
	{
		if (!u || !u.m_SectorThreatFilter || !u.m_OwnerEntity)
			return false;
		int major, minor;
		u.m_SectorThreatFilter.GetActiveSectors(major, minor);
		if (major == -1)
			return false;
		dir = u.m_SectorThreatFilter.GetSectorPos(major) - u.m_OwnerEntity.GetOrigin();
		dir[1] = 0;
		if (dir.LengthSq() < 1.0)
			return false;
		dir.Normalize();
		return true;
	}

	static float OffDeg(SCR_AIUtilityComponent u, vector front, vector pos)
	{
		vector d = pos - u.m_OwnerEntity.GetOrigin();
		d[1] = 0;
		if (d.LengthSq() < 0.01)
			return 0;
		d.Normalize();
		return Math.Acos(Math.Clamp(vector.Dot(d, front), -1.0, 1.0)) * Math.RAD2DEG;
	}

	static bool IsRearGuard(SCR_AIUtilityComponent u)
	{
		AIAgent me = u.GetAIAgent();
		if (!me || !me.GetParentGroup())
			return true;
		array<AIAgent> agents = {};
		me.GetParentGroup().GetAgents(agents);
		for (int i = agents.Count() - 1; i >= 0; i--)
		{
			if (agents[i] && agents[i].GetControlledEntity())
				return agents[i] == me;
		}
		return true;
	}

	static bool RearGlance(SCR_AIUtilityComponent u, vector front, string src)
	{
		float now = GetGame().GetWorld().GetWorldTime();
		if (now < u.m_fDCORearAt_ms)
			return false;
		u.m_fDCORearAt_ms = now + Math.RandomFloat(DCO_Ctx.TuneForU(u, "front.rear_min_s", 20.0), DCO_Ctx.TuneForU(u, "front.rear_max_s", 30.0)) * 1000.0;
		if (!IsRearGuard(u))
		{
			DCO_Perf.Count("front:rear_skip_role");
			return false;
		}
		float friendR = DCO_Ctx.TuneForU(u, "front.rear_friend_m", 150.0);
		if (friendR > 0 && DCO_Util.HasFriendlyNear(u.m_OwnerEntity, u.m_OwnerEntity.GetOrigin() - front * (friendR + 10.0), friendR, "front:rear_friend_q"))
		{
			DCO_Perf.Count("front:rear_skip_friend");
			return false;
		}
		DCO_Perf.Count("front:rear_glance_" + src);
		DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("front_rear_glance unit=%1 src=%2", DCO_Log.IdUnit(u.m_OwnerEntity), src));
		return true;
	}

	static bool AllowDir(SCR_AIUtilityComponent u, vector pos, float prio, string src)
	{
		bool on = FrontOn();
		if ((!on && DCO_Ctx.TuneForU(u, "front.measure", 0.0) <= 0) || prio >= SCR_AILookAction.PRIO_UNKNOWN_TARGET || !u || !u.m_OwnerEntity)
			return true;
		vector front;
		if (!Front(u, front))
			return true;
		float off = OffDeg(u, front, pos);
		if (off <= DCO_Ctx.TuneForU(u, "front.arc_deg", 60.0))
			return true;
		if (!on)
		{
			DCO_Perf.Count("front:would_clamp_" + src);
			return true;
		}
		if (RearGlance(u, front, src))
			return true;
		DCO_Perf.Count("front:clamp_" + src);
		float now = GetGame().GetWorld().GetWorldTime();
		if (now >= u.m_fDCOFrontLogAt_ms)
		{
			u.m_fDCOFrontLogAt_ms = now + 10000.0;
			DCO_Log.Write(DCO_ELogChannel.UNIT, DCO_ELogLevel.INFO, DCO_Log.IdUnit(u.m_OwnerEntity), string.Format("look_front_clamp unit=%1 src=%2 prio=%3 off_deg=%4 front=%5", DCO_Log.IdUnit(u.m_OwnerEntity), src, Math.Round(prio), Math.Round(off), DCO_Log.Pos(u.m_OwnerEntity.GetOrigin() + front * 50.0)));
		}
		return false;
	}

	static vector ExplosionThreatPos(IEntity src, vector pos)
	{
		if (!FrontOn() || !src || vector.DistanceXZ(src.GetOrigin(), pos) < 10.0)
			return pos;
		DCO_Perf.Count("front:expl_src");
		return src.GetOrigin();
	}
}

modded class SCR_AILookAction
{
	override void LookAt(vector pos, float priority, float duration = 0.8)
	{
		if (!DCO_FrontHold.AllowDir(m_Utility, pos, priority, "look"))
			return;
		super.LookAt(pos, priority, duration);
	}
}

modded class SCR_AILookAtThreatSystemLogic
{
	protected SCR_AIUtilityComponent m_DCOUtil;

	override void OnInit(AIAgent owner)
	{
		super.OnInit(owner);
		m_DCOUtil = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));
	}

	override protected void SwitchState_ObserveMinorSector()
	{
		if (m_ThreatFilter && m_DCOUtil)
		{
			int major, minor;
			m_ThreatFilter.GetActiveSectors(major, minor);
			if (major != -1 && minor != -1 && !DCO_FrontHold.AllowDir(m_DCOUtil, m_ThreatFilter.GetSectorPos(minor), 0, "minor"))
			{
				SwitchState_ObserveMajorSector();
				return;
			}
		}
		super.SwitchState_ObserveMinorSector();
	}
}
